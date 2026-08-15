// Copyright Epic Games, Inc. All Rights Reserved.

#include "PackPanel/FPackProcessWorker.h"

FPackProcessWorker::FPackProcessWorker(const FString& InExecutable, const FString& InParams, const FString& InWorkingDir)
	: Executable(InExecutable)
	, Params(InParams)
	, WorkingDir(InWorkingDir)
	, WorkerThread(nullptr)
	, ReadPipe(nullptr)
	, WritePipe(nullptr)
	, bRunning(false)
	, bShouldStop(false)
	, ExitCode(-1)
{
}

FPackProcessWorker::~FPackProcessWorker()
{
	StopProcess();
}

bool FPackProcessWorker::Start()
{
	if (Executable.IsEmpty())
	{
		return false;
	}

	FPlatformProcess::CreatePipe(ReadPipe, WritePipe);

	uint32 ProcessId = 0;
	ProcessHandle = FPlatformProcess::CreateProc(
		*Executable,
		Params.IsEmpty() ? nullptr : *Params,
		false, // bLaunchDetached
		false, // bLaunchHidded
		true,  // bLaunchReallyHidden
		&ProcessId,
		0,
		WorkingDir.IsEmpty() ? nullptr : *WorkingDir,
		WritePipe,
		ReadPipe);

	if (!ProcessHandle.IsValid())
	{
		FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
		ReadPipe = nullptr;
		WritePipe = nullptr;
		return false;
	}

	bRunning = true;
	WorkerThread = FRunnableThread::Create(this, TEXT("QiongQiPackProcessWorker"));
	if (!WorkerThread)
	{
		bRunning = false;
		FPlatformProcess::TerminateProc(ProcessHandle, true);
		FPlatformProcess::CloseProc(ProcessHandle);
		FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
		ReadPipe = nullptr;
		WritePipe = nullptr;
		return false;
	}

	return true;
}

uint32 FPackProcessWorker::Run()
{
	FString Pending;
	while (!bShouldStop)
	{
		// 读取子进程输出（UE5.5: ReadPipe 返回自上次读取以来的新数据）
		const FString NewData = FPlatformProcess::ReadPipe(ReadPipe);
		if (!NewData.IsEmpty())
		{
			Pending += NewData;

			// 按行切分
			int32 NewLineIdx = INDEX_NONE;
			while (Pending.FindChar(TEXT('\n'), NewLineIdx))
			{
				FString Line = Pending.Left(NewLineIdx);
				Pending.RemoveAt(0, NewLineIdx + 1);
				Line.TrimEndInline();
				if (!Line.IsEmpty())
				{
					OutputQueue.Enqueue(MoveTemp(Line));
				}
			}
		}

		// 检查进程是否已退出
		int32 ReturnCode = -1;
		if (FPlatformProcess::GetProcReturnCode(ProcessHandle, &ReturnCode))
		{
			// 处理剩余输出
			Pending += FPlatformProcess::ReadPipe(ReadPipe);
			Pending.TrimEndInline();
			if (!Pending.IsEmpty())
			{
				OutputQueue.Enqueue(MoveTemp(Pending));
			}

			ExitCode = ReturnCode;
			bRunning = false;
			break;
		}

		FPlatformProcess::Sleep(0.05f);
	}

	// 清理管道（StopProcess 会先终止进程再等待本线程结束）
	FPlatformProcess::CloseProc(ProcessHandle);
	FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
	ReadPipe = nullptr;
	WritePipe = nullptr;

	return 0;
}

void FPackProcessWorker::Stop()
{
	// FRunnable::Stop —— 由 Kill() 触发，仅置信号
	bShouldStop = true;
}

void FPackProcessWorker::StopProcess()
{
	bShouldStop = true;

	if (ProcessHandle.IsValid())
	{
		if (FPlatformProcess::IsProcRunning(ProcessHandle))
		{
			FPlatformProcess::TerminateProc(ProcessHandle, true);
		}
	}

	if (WorkerThread)
	{
		WorkerThread->Kill(true);
		delete WorkerThread;
		WorkerThread = nullptr;
	}

	bRunning = false;
}
