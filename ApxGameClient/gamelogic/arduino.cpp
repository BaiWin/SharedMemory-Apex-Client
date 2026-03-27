#include "arduino.h"
#include <random>
#include <algorithm>
#include <game.h>

SerialPort serial("\\\\.\\COM13", CBR_115200); // COM4

std::vector<vec3_t> arduino_move_info;
std::mutex moveMutex;

void SendFixMouseMovement(SerialPort& serial, float fixstep_x, float fixstep_y, float dist)
{   
    /*std::string ack = serial.read();
    if (!ack.empty())
    {
        std::cout << "ACK" << ack << std::endl;
    }*/
    //std::cout << "fixstep_x" << fixstep_x << std::endl;
    //std::cout << "fixstep_y" << fixstep_y << std::endl;
    std::string data = std::to_string(fixstep_x) + "," + std::to_string(fixstep_y) + "," + std::to_string(dist) + "\n";
    if (!serial.write(data))
    {
        std::cerr << "Failed to send data" << std::endl;
    }
}

void RunArduino()
{
    std::lock_guard<std::mutex> lock(moveMutex);

    if (!arduino_move_info.empty())
    {
        // 开始新的移动（会自动中断当前移动并衰减）
        adaptiveDeltaMoveController.StartNewMove();
    }

    // 更新当前移动（如果正在移动）
    if (adaptiveDeltaMoveController.IsMoving())
    {
        adaptiveDeltaMoveController.UpdateMove();
    }
}

AdaptiveTotalMoveController adaptiveTotalMoveController;

AdaptiveDeltaMoveController adaptiveDeltaMoveController;