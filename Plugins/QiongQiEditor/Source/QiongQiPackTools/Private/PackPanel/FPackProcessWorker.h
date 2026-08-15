// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/Queue.h"
#include "CoreMinimal.h"
#include "HAL/PlatformProcess.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeBool.h"

/**
 * 后台打包进程工作线程。
 * 在子线程中启动外部进程（UAT / UnrealEditor-Cmd），逐行读取输出并写入
 * 线程安全队列，由游戏线程在 Tick 中轮询队列完成 UI 更新，避免跨线程直接操作 Slate。
 */
class FPackProcessWorker : public FRunnable
{
public:
	FPackProcessWorker(const FString& InExecutable, const FString& InParams, const FString& InWorkingDir);
	virtual ~FPackProcessWorker();

	/** 启动进程，失败返回 false */
	bool Start();

	/** 终止进程并等待工作线程退出 */
	void StopProcess();

	/** 是否仍在运行 */
	bool IsRunning() const { return bRunning; }

	/** 进程退出码（仅在 IsRunning()==false 时有效） */
	int32 GetExitCode() const { return ExitCode; }

	/** 输出队列：工作线程写入，UI 线程读取 */
	TQueue<FString, EQueueMode::Spsc> OutputQueue;

	//~ FRunnable
	virtual uint32 Run() override;
	virtual void Stop() override;
	//~ FRunnable

private:
	FString Executable;
	FString Params;
	FString WorkingDir;

	FProcHandle ProcessHandle;
	FRunnableThread* WorkerThread;
	void* ReadPipe;
	void* WritePipe;

	FThreadSafeBool bRunning;
	FThreadSafeBool bShouldStop;
	int32 ExitCode;
};
