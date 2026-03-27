#include <hidboot.h>
#include <usbhub.h>
#include <Mouse.h>
#include <SPI.h>

// ==================== USB Host ====================
USB     Usb;
USBHub  Hub(&Usb);
HIDBoot<USB_HID_PROTOCOL_MOUSE>    HidMouse(&Usb);

// ==================== 累加移动 ====================
int pendingX = 0, pendingY = 0;
unsigned long lastWheelTime = 0;

// ==================== 鼠标解析器 ====================
class MouseRptParser : public MouseReportParser {
protected:
  // 鼠标移动
  void OnMouseMove(MOUSEINFO *mi) override {
    pendingX += mi->dX;
    pendingY += mi->dY;

    // 分段移动（防止溢出）
    while (abs(pendingX) >= 127 || abs(pendingY) >= 127) {
      int sendX = constrain(pendingX, -127, 127);
      int sendY = constrain(pendingY, -127, 127);
      Mouse.move(sendX, sendY);
      pendingX -= sendX;
      pendingY -= sendY;
    }

    if (pendingX != 0 || pendingY != 0) {
      Mouse.move(pendingX, pendingY);
      pendingX = 0;
      pendingY = 0;
    }
  }

  // 重载 Parse 来处理滚轮
  void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) override {
    if (len >= 4) {
      int8_t wheel = (int8_t)buf[3];  // 相对滚轮值 (+1/-1/0)

      // 防止滚动太频繁（1ms 防抖）
      unsigned long now = millis();
      if (wheel != 0 && now - lastWheelTime > 1) {
        Mouse.move(0, 0, wheel);  // 直接透传
        lastWheelTime = now;
      }
    }

    // 调用父类解析按键和移动
    MouseReportParser::Parse(hid, is_rpt_id, len, buf);
  }

  // 按键
  void OnLeftButtonDown (MOUSEINFO *mi) override { Mouse.press(MOUSE_LEFT); }
  void OnLeftButtonUp   (MOUSEINFO *mi) override { Mouse.release(MOUSE_LEFT); }
  void OnRightButtonDown(MOUSEINFO *mi) override { Mouse.press(MOUSE_RIGHT); }
  void OnRightButtonUp  (MOUSEINFO *mi) override { Mouse.release(MOUSE_RIGHT); }
  void OnMiddleButtonDown(MOUSEINFO *mi) override { Mouse.press(MOUSE_MIDDLE); }
  void OnMiddleButtonUp  (MOUSEINFO *mi) override { Mouse.release(MOUSE_MIDDLE); }
};

MouseRptParser Prs;

void setup() {
  Serial.begin(115200);
  Mouse.begin();

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2); // 16MHz SPI 加速

  if (Usb.Init() == -1) {
    Serial.println("USB Host 初始化失败！");
    while (1);
  }
  delay(200);
  HidMouse.SetReportParser(0, &Prs);

  //Serial.println("Leonardo + USB Host Shield (滚轮优化版)");
}

float AverageSpeed(float totalPixels) {
    // 调整后的平均速度模型：v_avg = 500 * ln(m) - 500 (像素/秒)
    if (totalPixels < 100.0f) totalPixels = 100.0f;
    return 400.0f * log(totalPixels) - 600.0f; // 400, 600
}
// 更慢:
    // 降低系数（例如从 500 到 300）： v_avg = 300 * ln(m) - 500;
    // 增加负偏移（例如从 -500 到 -800）：v_avg = 500 * ln(m) - 800; 
    // 使用更缓的对数（例如 log10(m) 或 ln(m/2)）：v_avg = 500 * ln(m / 2) - 500;
    // 更快:
    // 提高系数（例如从 500 到 800）：v_avg = 800 * ln(m) - 500;
    // 减少负偏移（例如从 -500 到 -200）：v_avg = 500 * ln(m) - 200;
    // 使用更陡的对数（例如 ln(m * 2)）：v_avg = 500 * ln(m * 2) - 500;

    // 更慢（推荐：降低系数 + 增加负偏移）：400 * log(totalPixels) - 600;
    // 更快（推荐：提高系数 + 减少负偏移）：600 * log(totalPixels) - 400;

float BaseSCurve(float t, float totalPixels) {
    // 确保时间t在[0,1]范围内
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    
    // 使用Sigmoid函数模拟鼠标移动曲线
    float k = 10.0f;
    float sigmoid = 1.0f / (1.0f + exp(-k * (t - 0.5f)));
    
    // 将Sigmoid输出归一化到[0,1]，然后乘以总移动像素
    float normalized = (sigmoid - 1.0f / (1.0f + exp(k * 0.5f))) / 
                      (1.0f / (1.0f + exp(-k * 0.5f)) - 1.0f / (1.0f + exp(k * 0.5f)));
    
    return normalized * totalPixels;
}

float ScaledSCurve(float x, float totalPixels) {
    // 计算总时间t = m / v_avg
    float v_avg = AverageSpeed(totalPixels);
    if (v_avg <= 0.0f) v_avg = 1.0f; // 防止除零
    float totalTime = totalPixels / v_avg;
    
    // 将x映射到归一化时间t_norm = x / t
    float t_norm = x / totalTime;
    
    // 使用BaseSCurve计算y值
    return BaseSCurve(t_norm, totalPixels);
}

float GetDeltaFromSCurve(float x1, float x2, float totalTime, float totalPixels)
{
    float t_norm_1 = x1 / totalTime;
    float t_norm_2 = x2 / totalTime;
    return BaseSCurve(t_norm_2, totalPixels) - BaseSCurve(t_norm_1, totalPixels);
}

float roundToBoundary(float value)
    {
        if (value > -1.0f && value < 1.0f)
        {
            if (value > 0)
            {
                return 1.0f;  // 正数向1取整
            }
            else if (value < 0)
            {
                return -1.0f; // 负数向-1取整
            }
        }
        return value;  // 不在区间内，返回原值
    }

void safeMove(int dx, int dy, int dist) {
    // 处理负值：使用绝对值计算时间和曲线，保留方向
    float abs_dx = abs(dx);
    float abs_dy = abs(dy);

    if(abs_dx + abs_dy < 9)
    {
      Mouse.move(abs_dx, abs_dy, 0);
      return;
    }
    
    // 计算平均速度和总时间
    float v_avg_x = AverageSpeed(abs_dx);
    float totalTime_x = abs_dx / v_avg_x;
    float v_avg_y = AverageSpeed(abs_dy);
    float totalTime_y = abs_dy / v_avg_y;

    totalTime_x = constrain(totalTime_x, 0, 0.032); // 0.024
    totalTime_y = constrain(totalTime_y, 0, 0.032); // 0.024

    // 时间步长：5ms = 0.005秒
    float timeStep = 0.002f;

    if(dist < 20|| dist > 200) {
        //totalTime_y = timeStep * (round(max(abs(dx), abs(dy)) / 127) + 1);
        //totalTime_x = timeStep * (round(max(abs(dx), abs(dy)) / 127) + 1);
    }
    
    // 计算循环次数，取 X 和 Y 轴的最大值
    int steps = ceil(max(totalTime_x, totalTime_y) / timeStep);
    
    // 跟踪累计移动量
    float record_x = 0.0f;
    float record_y = 0.0f;
    
    // 循环模拟鼠标移动
    for (int i = 0; i < steps; i++) {
        float t1 = timeStep * i;
        float t2 = timeStep * (i + 1);

        //float move_x = ScaledSCurve(t2, abs_dx) - ScaledSCurve(t1, abs_dx);
        //float move_y = ScaledSCurve(t2, abs_dy) - ScaledSCurve(t1, abs_dy);

        float move_x = GetDeltaFromSCurve(t1, t2, totalTime_x, abs_dx);
        float move_y = GetDeltaFromSCurve(t1, t2, totalTime_y, abs_dy);
        
        // 恢复方向
        if (dx < 0) move_x = -move_x;
        if (dy < 0) move_y = -move_y;
        
        // 转换为整数像素并添加随机扰动
        int delta_x = roundToBoundary(move_x);
        int delta_y = roundToBoundary(move_y);
        if (abs(delta_x) > 2) delta_x += random(-1, 2); // 对非零移动添加噪声
        if (abs(delta_y) > 2) delta_y += random(-1, 2);
        
        delta_x = constrain(delta_x, -127, 127);  // 限制在 -127 ~ 127
        delta_y = constrain(delta_y, -127, 127);

        // 防止累计移动超过目标
        record_x += delta_x;
        record_y += delta_y;
        if (abs(record_x) > abs_dx) {
            delta_x -= (record_x - dx); // 校正 X 轴
            record_x = dx; // 确保累计值等于目标
        }
        if (abs(record_y) > abs_dy) {
            delta_y -= (record_y - dy); // 校正 Y 轴
            record_y = dy;
        }
        Mouse.move(delta_x, delta_y, 0);
        delayMicroseconds(2000);

        //if(Serial.available() && i < steps - 3)
        //{
            //Mouse.move(100, 100);
            //i = steps - 3;
            //break;
        //}
    }
}

/* -------------------- loop -------------------- */
void loop() {
  Usb.Task();                   // 必须放在最前面，持续读取物理鼠标

  if (Serial.available()) {
    String lastCommand = "";
    while (Serial.available()) {
      String data = Serial.readStringUntil('\n');
      if (data.length() > 0) lastCommand = data;
    }

    int firstComma  = lastCommand.indexOf(',');
    int secondComma = lastCommand.indexOf(',', firstComma + 1);
    if (firstComma != -1 && secondComma != -1) {
      int dx    = lastCommand.substring(0, firstComma).toInt();
      int dy    = lastCommand.substring(firstComma + 1, secondComma).toInt();
      int flags = lastCommand.substring(secondComma + 1).toInt();

      safeMove(dx, dy, flags);        // 执行你的 S-Curve 移动
      // SendAck(dx, dy);      // 如需 ACK 可打开
    }
  }
}