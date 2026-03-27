#pragma once
#include "ClientIncludes.h"
#include <iostream>

template<typename T>
struct MemoryResult
{
	T Value;       // 内核返回的数据值
	int Sequence;  // 对应的命令序列号
};

extern int BufferReadStart;
extern std::atomic<int> g_CommandSeq;

inline void ResetSequence()
{
	g_CommandSeq.store(-1, std::memory_order_relaxed);
}

inline void ResetBufferReadStart()
{
	BufferReadStart = 0;
}

inline int CommandSequenceGenerator()
{
	return ++g_CommandSeq;
}

inline int GetSequence()
{
	return g_CommandSeq.load(std::memory_order_relaxed) + 1;
}

// 基础读取：绝对地址
template<typename T>
MemoryResult<T> ReadAbsolute(uintptr_t address, uintptr_t offset = 0)
{
	int currentSeq = CommandSequenceGenerator();

	COMMAND_PACKET cmd;
	cmd.Type = CMD_READ_MEMORY;
	cmd.Address = address;
	cmd.Offset = offset;
	cmd.Size = sizeof(T);
	cmd.Value = 0;

	SendCommandToKernel(cmd, currentSeq);

	MemoryResult<T> result{};
	result.Value = ProcessSingleResult<T>(&BufferReadStart); // 得到 T
	result.Sequence = currentSeq;                      // 保存序号
	return result;
}

// 链式读取：基于之前的数据包
template<typename T>
MemoryResult<T> Read(const MemoryResult<uintptr_t>& base, uintptr_t offset)
{
	int currentSeq = CommandSequenceGenerator();
	int packOffset = currentSeq - base.Sequence;

	COMMAND_PACKET cmd;
	cmd.Type = CMD_READ_MEMORY;
	cmd.Address = packOffset;   // 表示是包偏移
	cmd.Offset = offset;
	cmd.Size = sizeof(T);
	cmd.Value = 0;

	SendCommandToKernel(cmd, currentSeq);

	MemoryResult<T> result{};
	result.Value = ProcessSingleResult<T>(&BufferReadStart); // 得到 T
	result.Sequence = currentSeq;                      // 保存序号
	return result;
}

template<typename T>
void WriteAbsolute(uintptr_t address, uintptr_t offset, T value)
{
	int currentSeq = CommandSequenceGenerator();

	COMMAND_PACKET cmd;
	cmd.Type = CMD_WRITE_MEMORY;
	cmd.Address = address;
	cmd.Offset = offset;
	cmd.Size = sizeof(T);

	//static_assert(sizeof(T) <= sizeof(ULONG64), "Write<T> only supports <= 8 bytes value");
	ULONG64 rawValue = 0;
	memcpy(&rawValue, &value, sizeof(T));
	cmd.Value = rawValue;

	SendCommandToKernel(cmd, currentSeq);
}

template<typename T>
void Write(const MemoryResult<uintptr_t>& base, uintptr_t offset, T value)
{
	int currentSeq = CommandSequenceGenerator();

	int packOffset = currentSeq - base.Sequence;

	COMMAND_PACKET cmd;
	cmd.Type = CMD_WRITE_MEMORY;
	cmd.Address = packOffset;   // 表示是包偏移
	cmd.Offset = offset;
	cmd.Size = sizeof(T);
	
	//static_assert(sizeof(T) <= sizeof(ULONG64), "Write<T> only supports <= 8 bytes value");
	ULONG64 rawValue = 0;
	memcpy(&rawValue, &value, sizeof(T));
	cmd.Value = rawValue;

	SendCommandToKernel(cmd, currentSeq);
}

int FillChunkToSize(int size);

MemoryResult<uintptr_t> GetModuleBase(ULONG pid);

MemoryResult<std::vector<uint8_t>> ReadBufferAbsolute(uintptr_t address, uintptr_t offset, ULONG dataSize);

MemoryResult<std::vector<uint8_t>> ReadBuffer(const MemoryResult<uintptr_t>& base, uintptr_t offset, ULONG dataSize);

DWORD GetProcessID(const wchar_t* processName);

