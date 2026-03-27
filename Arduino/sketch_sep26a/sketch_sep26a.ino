#include <AbsMouse.h>

// 屏幕分辨率配置（根据你的屏幕修改）
const int SCREEN_WIDTH = 2560;
const int SCREEN_HEIGHT = 1600;
const int SCREEN_CENTER_X = SCREEN_WIDTH / 2;
const int SCREEN_CENTER_Y = SCREEN_HEIGHT / 2;

// 移动参数配置
const int MIN_STEPS = 3;          // 最小步数
const int MAX_STEPS = 30;         // 最大步数  
const int BASE_DURATION_MS = 200; // 基础移动时间
const float RANDOMNESS_STRENGTH = 0.1f; // 扰动强度

class PreciseMouseMover {
private:
    int currentX, currentY;  // 跟踪的鼠标位置（假设从中心开始）
    
public:
    PreciseMouseMover() {
        // 假设鼠标初始在屏幕中心（实际使用时需要校准）
        currentX = SCREEN_CENTER_X;
        currentY = SCREEN_CENTER_Y;
        AbsMouse.init(SCREEN_WIDTH, SCREEN_HEIGHT);
    }
    
    // 分步绝对移动主函数
    void smoothAbsoluteMove(int targetX, int targetY) {
        // 坐标限制
        targetX = constrain(targetX, 0, SCREEN_WIDTH - 1);
        targetY = constrain(targetY, 0, SCREEN_HEIGHT - 1);
        
        //Serial.print("🎯 从(");
        Serial.print(currentX);
        //Serial.print(",");
        Serial.print(currentY);
        //Serial.print(") 移动到(");
        Serial.print(targetX);
        //Serial.print(",");
        Serial.print(targetY);
        //Serial.println(")");
        
        // 计算移动参数
        int distance = calculateDistance(currentX, currentY, targetX, targetY);
        int steps = calculateSteps(distance);
        int totalDuration = calculateDuration(distance);
        
        Serial.print("距离: ");
        Serial.print(distance);
        Serial.print("像素, 步数: ");
        Serial.print(steps);
        Serial.print(", 时间: ");
        Serial.print(totalDuration);
        Serial.println("ms");
        
        // 执行分步移动
        executeStepMove(targetX, targetY, steps, totalDuration);
        
        // 更新当前位置
        currentX = targetX;
        currentY = targetY;
        
        //Serial.println("✅ 移动完成");
    }
    
private:
    // 计算两点距离
    int calculateDistance(int x1, int y1, int x2, int y2) {
        float dx = (float)(x2 - x1);
        float dy = (float)(y2 - y1);
        return (int)sqrt(dx * dx + dy * dy);
    }
    
    // 根据距离计算步数
    int calculateSteps(int distance) {
        int steps = 5 + (distance / 20);  // 每20像素增加1步
        return constrain(steps, MIN_STEPS, MAX_STEPS);
    }
    
    // 根据距离计算总时间
    int calculateDuration(int distance) {
        int duration = BASE_DURATION_MS + (distance / 10);  // 每10像素增加1ms
        return constrain(duration, 50, 500);  // 限制在50-500ms之间
    }
    
    // 执行分步移动
    void executeStepMove(int targetX, int targetY, int steps, int totalDuration) {
        int stepDelay = totalDuration / steps;  // 每步延迟
        
        for (int i = 0; i <= steps; i++) {
            float progress = (float)i / steps;
            
            // 缓动函数：开始快，结束慢（更精准）
            float easeProgress = easeInOutCubic(progress);
            
            // 计算当前目标位置
            int currentTargetX = currentX + (targetX - currentX) * easeProgress;
            int currentTargetY = currentY + (targetY - currentY) * easeProgress;
            
            // 添加随机扰动（在移动前期）
            if (progress < 0.7f) {  // 前70%路程添加扰动
                addRandomness(currentTargetX, currentTargetY, targetX, targetY, progress);
            }
            
            // 最终几步取消所有扰动，确保精准
            if (progress > 0.9f) {
                currentTargetX = currentX + (targetX - currentX) * easeProgress;
                currentTargetY = currentY + (targetY - currentY) * easeProgress;
            }
            
            // 移动到中间位置
            AbsMouse.move(currentTargetX, currentTargetY);
            
            // 调试输出
            if (i % 5 == 0 || i == steps) {  // 每5步输出一次
                //Serial.print("步骤 ");
                //Serial.print(i);
                //Serial.print("/");
                //Serial.print(steps);
                //Serial.print(": 位置(");
                //Serial.print(currentTargetX);
                //Serial.print(",");
                //Serial.print(currentTargetY);
                //Serial.print("), 进度");
                //Serial.print(progress * 100);
                //Serial.println("%");
            }
            
            delay(stepDelay);
        }
        
        // 最终精确调整（确保到达目标）
        AbsMouse.move(targetX, targetY);
        delay(10);
    }
    
    // 缓动函数：平滑加速和减速
    float easeInOutCubic(float t) {
        return t < 0.5 ? 4 * t * t * t : 1 - pow(-2 * t + 2, 3) / 2;
    }
    
    // 添加随机扰动
    void addRandomness(int& x, int& y, int targetX, int targetY, float progress) {
        // 扰动强度随进度减小
        float currentRandomness = RANDOMNESS_STRENGTH * (1.0f - progress);
        
        if (currentRandomness > 0.01f) {
            int maxOffset = (int)(20.0f * currentRandomness);  // 最大偏移像素
            x += random(-maxOffset, maxOffset + 1);
            y += random(-maxOffset, maxOffset + 1);
            
            // 限制在屏幕范围内
            x = constrain(x, 0, SCREEN_WIDTH - 1);
            y = constrain(y, 0, SCREEN_HEIGHT - 1);
        }
    }
    
    // 校准鼠标位置（可选，需要用户交互）
    void calibratePosition() {
        //Serial.println("🔧 位置校准：将鼠标移动到屏幕中心后按任意键...");
        while (!Serial.available()) {
            delay(100);
        }
        Serial.readString();  // 清空缓冲区
        
        currentX = SCREEN_CENTER_X;
        currentY = SCREEN_CENTER_Y;
        AbsMouse.move(currentX, currentY);
        
        //Serial.println("✅ 位置已校准到屏幕中心");
    }
};

// 全局实例
PreciseMouseMover mouseMover;

void setup() {
  Serial.begin(9600);
  AbsMouse.init(SCREEN_WIDTH, SCREEN_HEIGHT);
  while (!Serial) {
    ; // 等待串口连接
  }
}

void loop() {
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    int commaIndex = data.indexOf(',');
    if (commaIndex != -1) {
        int targetX = data.substring(0, commaIndex).toInt();
        int targetY = data.substring(commaIndex + 1).toInt();
        if (targetX < 0 || targetX >= SCREEN_WIDTH || 
                targetY < 0 || targetY >= SCREEN_HEIGHT) {
                //Serial.println("坐标超出屏幕范围");
                return;
            }
        mouseMover.smoothAbsoluteMove(targetX, targetY);
    }
  }
}