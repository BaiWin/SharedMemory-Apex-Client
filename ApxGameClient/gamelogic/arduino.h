#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vectors.h>
#include <mutex>


extern std::vector<vec3_t> arduino_move_info;

class SerialPort
{
private:
    HANDLE hSerial;
public:
    SerialPort(const std::string& portName, DWORD baudRate)
    {
        hSerial = CreateFileA(portName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hSerial == INVALID_HANDLE_VALUE)
        {
            std::cerr << "Error opening serial port: " << GetLastError() << std::endl;
            return;
        }

        DCB dcbSerialParams = { 0 };
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        if (!GetCommState(hSerial, &dcbSerialParams))
        {
            std::cerr << "Error getting serial state: " << GetLastError() << std::endl;
            return;
        }

        dcbSerialParams.BaudRate = baudRate;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;
        if (!SetCommState(hSerial, &dcbSerialParams))
        {
            std::cerr << "Error setting serial state: " << GetLastError() << std::endl;
            return;
        }

        COMMTIMEOUTS timeouts = { 0 };
        timeouts.ReadIntervalTimeout = 50;
        timeouts.ReadTotalTimeoutConstant = 50;
        timeouts.ReadTotalTimeoutMultiplier = 10;
        timeouts.WriteTotalTimeoutConstant = 50;
        timeouts.WriteTotalTimeoutMultiplier = 10;
        if (!SetCommTimeouts(hSerial, &timeouts))
        {
            std::cerr << "Error setting timeouts: " << GetLastError() << std::endl;
        }
    }

    ~SerialPort()
    {
        if (hSerial != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hSerial);
        }
    }

    bool write(const std::string& data)
    {
        DWORD bytesWritten;
        if (!WriteFile(hSerial, data.c_str(), data.length(), &bytesWritten, NULL))
        {
            std::cerr << "Error writing to serial port: " << GetLastError() << std::endl;
            return false;
        }
        return true;
    }

    std::string read()
    {
        if (hSerial == INVALID_HANDLE_VALUE)
            return "";

        COMSTAT status;
        DWORD errors;

        if (!ClearCommError(hSerial, &errors, &status) || status.cbInQue == 0)
        {
            return "";
        }

        char buffer[256] = { 0 };
        DWORD bytesRead;
        if (!ReadFile(hSerial, buffer, sizeof(buffer) - 1, &bytesRead, NULL))
        {
            std::cerr << "Error reading from serial port: " << GetLastError() << std::endl;
            return "";
        }

        buffer[bytesRead] = '\0';  // 确保字符串结尾
        return std::string(buffer);
    }
};

void SendFixMouseMovement(SerialPort& serial, float fixstep_x, float fixstep_y, float dist);

extern SerialPort serial;

class AdaptiveTotalMoveController
{
private:
    bool waitingForAct = 0;
    // 历史记录
    float prev_move_x = 0;
    float prev_move_y = 0;

    float prev_large_angle_x = 0;
    float prev_large_angle_y = 0;

    // 连续同方向计数
    int same_direction_count_x = 0;
    int same_direction_count_y = 0;
    int diff_direction_count_x = 0;
    int diff_direction_count_y = 0;

    float base_sensitivity = 11.09f;  // 9.09091

    float over_shoot_min = 0.8f;//0.8f - 0.05f;         // 如果抖动，调低这个
    float same_dir_max = 1.1f; //2.0f + 0.05f;           // 如果追赶，调高这个        
    float timesToCatch = 0.1f; //0.7f + 0.1f;

    bool isLastFrameState = false; // For sharedMemory Reason

public:
    void SendArduinoMovmentRaw(vec3_t aim_angle, vec3_t view_angle, float dist, float k, vec3_t sway_angle)
    {
        float change_x = normalizeAngle(aim_angle.y - view_angle.y);
        float change_y = aim_angle.x - view_angle.x;

        float base_arduino_x = -change_x * base_sensitivity * k;
        float base_arduino_y = change_y * base_sensitivity * k; // fov 不影响 cameta fov height

        //if (abs(base_arduino_x) > 5.0f)
        //{
        //    float rateX = abs(prev_large_angle_x / change_x);
        //    if (rateX > 0.9f && rateX < 1.1f)
        //    {
        //        base_arduino_x = -1 * base_arduino_x / std::abs(base_arduino_x);
        //        prev_large_angle_x = 0;
        //        //std::cout << "Doubled X" << std::endl;
        //    }
        //    else
        //    {
        //        prev_large_angle_x = std::abs(change_x);
        //        
        //    }
        //}
        //if (abs(base_arduino_y) > 5.0f)
        //{
        //    float rateY = abs(prev_large_angle_y / change_y);

        //    if (rateY > 0.9f && rateY < 1.1f)
        //    {
        //        base_arduino_y = -1 * base_arduino_y / std::abs(base_arduino_y);
        //        prev_large_angle_y = 0;
        //        //std::cout << "Doubled Y" << std::endl;
        //    }
        //    else
        //    {
        //        prev_large_angle_y = std::abs(change_y);
        //    }
        //}
        base_arduino_y = applyAdaptiveControlY(base_arduino_y, prev_move_y);
        base_arduino_x = applyAdaptiveControlX(base_arduino_x, prev_move_x);

        //if (dist < 20) base_arduino_x *= 1.2f;
        
        if (isLastFrameState)
        {
            isLastFrameState = false;

            //change_x = normalizeAngle(sway_angle.y);
            //change_y = sway_angle.y;

            //base_arduino_x = -change_x * base_sensitivity * k;
            //base_arduino_y = change_y * base_sensitivity * k;

            //SendFixMouseMovement(serial, roundToBoundary(base_arduino_x), roundToBoundary(base_arduino_y), dist);
            return;
        }
        else
        {
            SendFixMouseMovement(serial, roundToBoundary(base_arduino_x), roundToBoundary(base_arduino_y), dist);
            isLastFrameState = true;
        }
        
        //arduino_move_info.push_back(vec3_t{ roundToBoundary(base_arduino_x), roundToBoundary(base_arduino_y), dist });

        //updateHistory(roundToBoundary(base_arduino_x), roundToBoundary(base_arduino_y));
    }

    void ResetControllerParam()
    {
        prev_move_x = 0;
        prev_move_y = 0;
        same_direction_count_x = 0;
        same_direction_count_y = 0;
        diff_direction_count_x = 0;
        diff_direction_count_y = 0;
        prev_large_angle_x = 0;
        prev_large_angle_y = 0;
    }

private:
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

    float normalizeAngle(float angle)
    {
        if (angle > 180.0f) angle -= 360.0f;
        if (angle < -180.0f) angle = 360.0f + angle;
        return angle;
    }

    float AccelerationCurve(float x)
    {
        return 0;
    }

    float applyAdaptiveControlX(float current_move, float& prev_move)
    {
        bool over_shoot = (prev_move * current_move < 0);
        bool same_direction = (prev_move * current_move > 0);
        bool no_movement = (current_move == 0);
        float adaptive_Direction_factor = 1;
        //std::cout << prev_move << "..." << current_move << std::endl;
        // 自适应系数调整
        if (no_movement)
        {
            same_direction_count_x = 0;
            diff_direction_count_x = 0;
            adaptive_Direction_factor = 1;
            //std::cout << "no movement" << std::endl;

            //base_sensitivity = 9.09f;
        }
        else if (same_direction)
        {
            same_direction_count_x++;
            diff_direction_count_x = 0;
            float accelaration = timesToCatch * (1 << same_direction_count_x);
            adaptive_Direction_factor = std::clamp(1 + accelaration, 1.0f, same_dir_max);
            //std::cout << "same dir" << std::endl;
            //base_sensitivity = 9.09f;
        }
        else if (over_shoot)
        {
            diff_direction_count_x++;
            same_direction_count_x = 0;
            float accelaration = timesToCatch * (1 << diff_direction_count_x);
            adaptive_Direction_factor = std::clamp(1 - accelaration, over_shoot_min, 1.0f);
            //std::cout << "overshoot" << std::endl;
        }

        // 计算最终移动量
        float final_move = current_move * adaptive_Direction_factor;

        return final_move;
    }

    float applyAdaptiveControlY(float current_move, float& prev_move)
    {
        bool over_shoot = (prev_move * current_move < 0);
        bool same_direction = (prev_move * current_move > 0);
        bool no_movement = (current_move == 0);
        float adaptive_Direction_factor = 1;

        // 自适应系数调整
        if (no_movement)
        {
            same_direction_count_y = 0;
            diff_direction_count_y = 0;
            adaptive_Direction_factor = 1;
        }
        else if (same_direction)
        {
            same_direction_count_y++;
            diff_direction_count_y = 0;
            float accelaration = timesToCatch * (1 << same_direction_count_y);
            adaptive_Direction_factor = std::clamp(1 + accelaration, 1.0f, same_dir_max);
        }
        else if (over_shoot)
        {
            diff_direction_count_y++;
            same_direction_count_y = 0;
            float accelaration = timesToCatch * (1 << diff_direction_count_y);
            adaptive_Direction_factor = std::clamp(1 - accelaration, over_shoot_min, 1.0f);
        }

        // 计算最终移动量
        float final_move = current_move * adaptive_Direction_factor;

        return final_move;
    }

    void updateHistory(float move_x, float move_y)
    {
        prev_move_x = move_x;
        prev_move_y = move_y;
    }
};

extern AdaptiveTotalMoveController adaptiveTotalMoveController;

class AdaptiveDeltaMoveController
{
private:
    float currentTotalX = 0, currentTotalY = 0;
    float movedX = 0, movedY = 0;
    bool isMoving = false;
    int totalSteps = 0;
    int currentStepIndex = 0;
    float frameRate = 0.001f;

public:

    void StartNewMove()
    {
        //std::lock_guard<std::mutex> lock(moveMutex);
        //std::cout << isMoving << std::endl;
        //std::cout << currentStepIndex << "," << totalSteps << std::endl;
        if (isMoving)
        {
            // 中断当前移动，跳转到最后几步衰减
            //currentStepIndex = max(currentStepIndex, totalSteps - 60);
            return;
        }
        
        // 取出最新的移动数据
        auto latest_move = arduino_move_info.back();
        arduino_move_info.clear();

        int dx = latest_move.x;
        int dy = latest_move.y;

        // 设置新的移动目标
        currentTotalX = dx;
        currentTotalY = dy;
        movedX = 0;
        movedY = 0;
        isMoving = true;

        // 计算新的步数
        float abs_dx = abs(dx);
        float abs_dy = abs(dy);
        float v_avg_x = AverageSpeed(abs_dx);
        float totalTime_x = abs_dx / v_avg_x;
        float v_avg_y = AverageSpeed(abs_dy);
        float totalTime_y = abs_dy / v_avg_y;

        totalSteps = ceil(max(totalTime_x, totalTime_y) / frameRate);
        currentStepIndex = 0;
    }

    void UpdateMove()
    {
        //std::lock_guard<std::mutex> lock(moveMutex);

        float t1 = frameRate * currentStepIndex;
        float t2 = frameRate * (currentStepIndex + 1);

        // 计算移动量（保持您的原有算法）
        float abs_Tx = abs(currentTotalX);
        float abs_Ty = abs(currentTotalY);

        float move_x = ScaledSCurve(t2, abs_Tx) - ScaledSCurve(t1, abs_Tx);
        float move_y = ScaledSCurve(t2, abs_Ty) - ScaledSCurve(t1, abs_Ty);

        if (currentTotalX < 0) move_x = -move_x;
        if (currentTotalY < 0) move_y = -move_y;

        // 转换为整数并添加扰动
        float delta_x = move_x;
        float delta_y = move_y;

        if (abs(delta_x) > 1) delta_x += (rand() % 3) - 1;
        if (abs(delta_y) > 1) delta_y += (rand() % 3) - 1;

        // 限制范围
        delta_x = std::clamp(delta_x, -127.0f, 127.0f);
        delta_y = std::clamp(delta_y, -127.0f, 127.0f);

        delta_x = roundToBoundary(delta_x);
        delta_y = roundToBoundary(delta_x);

        // 防止超调
        movedX += delta_x;
        movedY += delta_y;

        if (abs(movedX) > abs(currentTotalX))
        {
            delta_x -= (movedX - currentTotalX);
            movedX = currentTotalX;
        }
        if (abs(movedY) > abs(currentTotalY))
        {
            delta_y -= (movedY - currentTotalY);
            movedY = currentTotalY;
        }

        /*if (delta_x == 0 && delta_y == 0)
        {
            std::cout << ">" << std::endl;
            isMoving = false;
            return;
        }*/

        //std::cout << delta_x << "," << delta_y << std::endl;
        // 发送到Arduino
        SendFixMouseMovement(serial, delta_x, delta_y, 0);

        currentStepIndex++;

        if (currentStepIndex >= totalSteps)
        {
            isMoving = false;
            return;
        }
    }

    bool IsMoving() const
    {
        return isMoving;
    }

private:
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


    // 保持您的数学函数不变
    float AverageSpeed(float totalPixels)
    {
        if (totalPixels < 100.0f) totalPixels = 100.0f;
        return 500.0f * log(totalPixels) - 500.0f;
    }

    float BaseSCurve(float t, float totalPixels)
    {
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        float k = 10.0f;
        float sigmoid = 1.0f / (1.0f + exp(-k * (t - 0.5f)));

        float normalized = (sigmoid - 1.0f / (1.0f + exp(k * 0.5f))) /
            (1.0f / (1.0f + exp(-k * 0.5f)) - 1.0f / (1.0f + exp(k * 0.5f)));

        return normalized * totalPixels;
    }

    float ScaledSCurve(float x, float totalPixels)
    {
        float v_avg = AverageSpeed(totalPixels);
        if (v_avg <= 0.0f) v_avg = 1.0f;
        float totalTime = totalPixels / v_avg;

        float t_norm = x / totalTime;
        return BaseSCurve(t_norm, totalPixels);
    }
};

// 全局实例
extern AdaptiveDeltaMoveController adaptiveDeltaMoveController;

extern std::mutex moveMutex;



