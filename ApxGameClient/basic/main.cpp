#include <iostream>
#include "ClientIncludes.h"
#include <thread>
#include "game.h"
#include <arduino.h>

extern void RunArduino();

std::atomic<bool> exitFlag(false);
HANDLE g_memoryThreadDone;
HANDLE g_serialThreadDone;

BOOL WINAPI ConsoleHandler(DWORD ctrl_type)
{
    switch (ctrl_type)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        std::cout << "\nReceived close event, stopping worker...\n";
        exitFlag = true;             // 通知子线程
        // 等待子线程结束，最多给 5 秒
        WaitForSingleObject(g_memoryThreadDone, 5000);
        WaitForSingleObject(g_serialThreadDone, 5000);  // [+] 等待串口线程
        return TRUE;               // 事件已处理
    }
    return FALSE;
}

bool OnFrameStart()
{
    InsertJunkCodeRND();
    ResetSequence();
    ResetBufferReadStart();
    MemoryBarrier();
    PSHARED_MEMORY_DATA pSharedData = GetSharedDataOnce();
    InsertJunkCode(1);
    if (InterlockedCompareExchange(&pSharedData->Lock, 1, 0) == 1)
    {
        InsertJunkCode(2);
        return false;
    }
    // 初始化，待赋值
    pSharedData->ClientPid = 0;
    pSharedData->TargetPid = 0;
    pSharedData->CommandPackSize = 0;
    RtlZeroMemory(&pSharedData->commandPacks, sizeof(COMMAND_PACKET) * MAX_COMMAND_COUNT); // 单包加密

    InsertJunkCodeRND();

    // Buffer统一解密
    DecryptField((ULONG*)&pSharedData->DataSize);
    DecryptBuffer(pSharedData->Buffer[pSharedData->currentBufferIndex], pSharedData->DataSize); // 直接作用
    InsertJunkCode(3);
    return true;
}

void OnFrameEnd()
{
    InsertJunkCodeRND();
    PSHARED_MEMORY_DATA pSharedData = GetSharedDataOnce();
    pSharedData->CommandPackSize = GetSequence();
    if (pSharedData->CommandPackSize >= MAX_COMMAND_COUNT)
    {
        std::cout << "Exceed max command size: " << std::dec << pSharedData->CommandPackSize << std::endl;
    }
    //std::cout << "TotalCommand Sent: " << std::dec << pSharedData->CommandPackSize << std::endl;
    InsertJunkCodeRND();
    EncryptField(&pSharedData->ClientPid);
    EncryptField(&pSharedData->TargetPid);
    EncryptField((ULONG*) & pSharedData->CommandPackSize);
    //Buffer恢复
    EncryptBuffer(pSharedData->Buffer[pSharedData->currentBufferIndex], pSharedData->DataSize);  // 恢复
    EncryptField((ULONG*)&pSharedData->DataSize);
    InsertJunkCode(1);

    MemoryBarrier();
    InsertJunkCode(2);
    InterlockedExchange(&pSharedData->Lock, 0);
}

static int frameCount = 0;
static const int CHECK_INTERVAL_FRAMES = 200;

DWORD GetPIDCached(const wchar_t* processName)
{
    static DWORD cachedPID = 0;
    frameCount++;
    if (frameCount > CHECK_INTERVAL_FRAMES || cachedPID == 0)
    {
        frameCount = 0;
        cachedPID = GetProcessID(processName);
    }
    return cachedPID;
}

void ReadGame()
{
    Sleep(3000);
    try
    {
        while (!exitFlag.load())
        {
            InsertJunkCodeRND();
            //std::cout << " ----------------READ GAME----------------- " << std::endl;
            if (!OnFrameStart()) continue;

            static int frameCount = 0;
            DWORD clientPid = GetCurrentProcessId();
            DWORD targetPid = GetPIDCached(L"r5apex_dx12.exe");
            
            //std::cout << (ULONG)clientPid << "  " << (ULONG)targetPid << std::endl;
            PSHARED_MEMORY_DATA pSharedData = GetSharedDataOnce();
            ULONG DataSize = pSharedData->DataSize;
            DecryptField((ULONG*)&DataSize);
            //std::cout << "CurrenBufferIndex: " << pSharedData->currentBufferIndex << std::endl;
            //std::cout << "DataSize: " << DataSize << std::endl;
            pSharedData->ClientPid = (ULONG)clientPid;
            pSharedData->TargetPid = (ULONG)targetPid;

            MemoryResult<uintptr_t> baseAddress = GetModuleBase(targetPid);

            /*MemoryResult<uintptr_t> local_player = Read<uintptr_t>(baseAddress, 0x6282c28);
            std::cout << "test : " << std::hex << TT.Value << std::endl;*/

            {
                std::lock_guard<std::mutex> lock(objArrayMutex);
                GameMain((ULONG)targetPid);
            }

            /*MemoryResult<uintptr_t> baseAddress = GetModuleBase(targetPid);
            MemoryResult<int> v1 = Read<int>(baseAddress, 0x5034);
            Write<int>(baseAddress, 0x5034, 100);

            std::cout << "Base Address: " << std::hex << baseAddress.Value << std::endl;
            std::cout << "v1 : " << std::dec << v1.Value << std::endl;*/

            OnFrameEnd();

            int sleepTime = 16;
            Sleep(sleepTime + rand() % sleepTime - sleepTime / 4);
        }
    }
    catch (const std::exception& e)
    {
        printf("[Thread] 异常: %s\n", e.what());
    }
    catch (...)
    {
        printf("[Thread] 未知异常发生\n");
    }
    
    SetEvent(g_memoryThreadDone);
}

void SerialSendThread()
{
    printf("[SerialThread] Serial port transmission thread start\n");

    try
    {
        while (!exitFlag.load())
        {
            //RunArduino();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1 // 1000
        }
    }
    catch (const std::exception& e)
    {
        printf("[SerialThread] 异常: %s\n", e.what());
    }
    catch (...)
    {
        printf("[SerialThread] 未知异常发生\n");
    }

    printf("[SerialThread] 串口发送线程退出\n");
    SetEvent(g_serialThreadDone);
}

int main()
{
    InsertJunkCodeRND();
    printf("User Mode Client Starting...\n");

    g_memoryThreadDone = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    g_serialThreadDone = CreateEvent(nullptr, TRUE, FALSE, nullptr);  // [+] 新增

    if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE))
    {
        std::cerr << "SetConsoleCtrlHandler failed\n";
        return 1;
    }

    if (!InitializeSharedMemory())
    {
        InsertJunkCode(1);
        getchar();
        return 1;
    }

    std::thread memoryThread(ReadGame);
    std::thread serialThread(SerialSendThread);
    RenderOverlay();

    /*while(true)
    {
        
    }*/

    // 等待用户输入，保持程序运行
    printf("Press Enter to exit...\n");
    getchar();

    CleanupSharedMemory();

    exitFlag = true;
    WaitForSingleObject(g_memoryThreadDone, INFINITE);
    WaitForSingleObject(g_serialThreadDone, INFINITE);
    memoryThread.join();
    serialThread.join();
    CloseHandle(g_memoryThreadDone);
    CloseHandle(g_serialThreadDone);

    return 0;
}

