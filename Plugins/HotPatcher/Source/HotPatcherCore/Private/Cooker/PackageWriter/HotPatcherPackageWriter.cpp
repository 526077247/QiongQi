
#include "Cooker/PackageWriter/HotPatcherPackageWriter.h"

#if WITH_PACKAGE_CONTEXT && ENGINE_MAJOR_VERSION > 4
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/AssetRegistryState.h"
#include "Async/Async.h"
#include "Misc/PathViews.h"
#include "Serialization/CompactBinarySerialization.h"
#include "Serialization/LargeMemoryWriter.h"
#include "UObject/SavePackage.h"
#if WITH_IO_STORE_SUPPORT
#include "PackageStoreOptimizer.h"
#endif

void FHotPatcherPackageWriter::Initialize(const FCookInfo& Info){}

#if !UE_VERSION_NEWER_THAN(5,1,1)
void FHotPatcherPackageWriter::AddToExportsSize(int64& ExportsSize)
{
	TPackageWriterToSharedBuffer<ICookedPackageWriter>::AddToExportsSize(ExportsSize);
}
#endif

void FHotPatcherPackageWriter::BeginPackage(const FBeginPackageInfo& Info)
{
	TPackageWriterToSharedBuffer<ICookedPackageWriter>::BeginPackage(Info);
	FScopeLock Lock(&OplogLock);
	FOplogPackageInfo& PackageInfo = Oplog.FindOrAdd(Info.PackageName);
	PackageInfo.PackageName = Info.PackageName;
	PackageInfo.PackageDataChunks.Reset();
	PackageInfo.BulkDataChunks.Reset();
}

void FHotPatcherPackageWriter::BeginCook(
#if UE_VERSION_NEWER_THAN(5,1,1)
		const FCookInfo& Info
#endif
){}

void FHotPatcherPackageWriter::EndCook(
#if UE_VERSION_NEWER_THAN(5,1,1)
		const FCookInfo& Info
#endif
)
{
#if UE_VERSION_NEWER_THAN(5,1,1)
	// Only required when IoStore containers will be generated from the loose files.
	if (MetadataDirectoryPath.IsEmpty())
	{
		return;
	}

	FScopeLock Lock(&OplogLock);

	TArray<const FOplogPackageInfo*> SortedPackages;
	SortedPackages.Reserve(Oplog.Num());
	for (const TPair<FName, FOplogPackageInfo>& Pair : Oplog)
	{
		SortedPackages.Add(&Pair.Value);
	}
	Algo::Sort(SortedPackages,
		[](const FOplogPackageInfo* A, const FOplogPackageInfo* B)
	{
		return A->PackageName.LexicalLess(B->PackageName);
	});

	FCbWriter ManifestWriter;
	ManifestWriter.BeginObject();
	ManifestWriter.BeginObject("oplog");
	ManifestWriter.BeginArray("entries");
	for (const FOplogPackageInfo* Package : SortedPackages)
	{
		WriteOplogEntry(ManifestWriter, *Package);
	}
	ManifestWriter.EndArray();
	ManifestWriter.EndObject();
	ManifestWriter.EndObject();

	const FString PackageStoreManifestFilePath = FPaths::Combine(MetadataDirectoryPath, TEXT("packagestore.manifest"));
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(PackageStoreManifestFilePath), true);
	TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileWriter(*PackageStoreManifestFilePath));
	if (Ar)
	{
		SaveCompactBinary(*Ar, ManifestWriter.Save());
		UE_LOG(LogTemp, Log, TEXT("Saved package store manifest to '%s' (%d entries)"), *PackageStoreManifestFilePath, SortedPackages.Num());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save package store manifest file '%s'"), *PackageStoreManifestFilePath);
	}

	// Generate ScriptObjects.bin for the IoStore commandlet.
	// FZenStoreWriter does this in EndCook() via FPackageStoreOptimizer; we replicate it here
	// so the -run=IoStore commandlet can load it via -ScriptObjects=<path> when there is no ZenStoreClient.
#if WITH_IO_STORE_SUPPORT
	{
		FPackageStoreOptimizer PackageStoreOptimizer;
		PackageStoreOptimizer.Initialize();
		FIoBuffer ScriptObjectsBuffer = PackageStoreOptimizer.CreateScriptObjectsBuffer();

		const FString ScriptObjectsFilePath = FPaths::Combine(MetadataDirectoryPath, TEXT("ScriptObjects.bin"));
		TUniquePtr<FArchive> ScriptObjectsAr(IFileManager::Get().CreateFileWriter(*ScriptObjectsFilePath));
		if (ScriptObjectsAr)
		{
			ScriptObjectsAr->Serialize(const_cast<uint8*>(ScriptObjectsBuffer.Data()), ScriptObjectsBuffer.DataSize());
			UE_LOG(LogTemp, Log, TEXT("Saved script objects to '%s'"), *ScriptObjectsFilePath);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to save script objects file '%s'"), *ScriptObjectsFilePath);
		}
	}
#endif
#endif
}

// void FHotPatcherPackageWriter::Flush()
// {
// 	UPackage::WaitForAsyncFileWrites();
// }

TUniquePtr<FAssetRegistryState> FHotPatcherPackageWriter::LoadPreviousAssetRegistry()
{
	return TUniquePtr<FAssetRegistryState>();
}

FCbObject FHotPatcherPackageWriter::GetOplogAttachment(FName PackageName, FUtf8StringView AttachmentKey)
{
	return FCbObject();
}

#if UE_VERSION_OLDER_THAN(5,5,0)
void FHotPatcherPackageWriter::GetOplogAttachments(TArrayView<FName> PackageNames,
	TArrayView<FUtf8StringView> AttachmentKeys,
	TUniqueFunction<void(FName PackageName, FUtf8StringView AttachmentKey, FCbObject&& Attachment)>&& Callback)
{
	for (FName PackageName : PackageNames)
	{
		for (FUtf8StringView AttachmentKey : AttachmentKeys)
		{
			Callback(PackageName, AttachmentKey, FCbObject());
		}
	}
}

IPackageWriter::ECommitStatus FHotPatcherPackageWriter::GetCommitStatus(FName PackageName)
{
	return IPackageWriter::ECommitStatus::Success;
}
#endif

void FHotPatcherPackageWriter::RemoveCookedPackages(TArrayView<const FName> PackageNamesToRemove)
{
}

void FHotPatcherPackageWriter::RemoveCookedPackages()
{
	UPackage::WaitForAsyncFileWrites();
}

#if UE_VERSION_OLDER_THAN(5,4,0)
void FHotPatcherPackageWriter::MarkPackagesUpToDate(TArrayView<const FName> UpToDatePackages)
{
}
#else
void FHotPatcherPackageWriter::UpdatePackageModificationStatus(FName PackageName, bool bIterativelyUnmodified, bool& bInOutShouldIterativelySkip)
{
}
#endif

bool FHotPatcherPackageWriter::GetPreviousCookedBytes(const FPackageInfo& Info, FPreviousCookedBytesData& OutData)
{
	return ICookedPackageWriter::GetPreviousCookedBytes(Info, OutData);
}
#if UE_VERSION_OLDER_THAN(5,3,0)
void FHotPatcherPackageWriter::CompleteExportsArchiveForDiff(const FPackageInfo& Info,
	FLargeMemoryWriter& ExportsArchive)
{

	ICookedPackageWriter::CompleteExportsArchiveForDiff(Info, ExportsArchive);
}
#endif

void FHotPatcherPackageWriter::CollectForSavePackageData(FRecord& Record, FCommitContext& Context)
{
	Context.ExportsBuffers.AddDefaulted(Record.Packages.Num());
	for (FPackageWriterRecords::FWritePackage& Package : Record.Packages)
	{
		Context.ExportsBuffers[Package.Info.MultiOutputIndex].Add(FExportBuffer{ Package.Buffer, MoveTemp(Package.Regions) });
	}
}

void FHotPatcherPackageWriter::CollectForSaveBulkData(FRecord& Record, FCommitContext& Context)
{
	for (FBulkDataRecord& BulkRecord : Record.BulkDatas)
	{
		if (BulkRecord.Info.BulkDataType == FBulkDataInfo::AppendToExports)
		{
			if (Record.bCompletedExportsArchiveForDiff)
			{
				// Already Added in CompleteExportsArchiveForDiff
				continue;
			}
			Context.ExportsBuffers[BulkRecord.Info.MultiOutputIndex].Add(FExportBuffer{ BulkRecord.Buffer, MoveTemp(BulkRecord.Regions) });
		}
		else
		{
			FWriteFileData& OutputFile = Context.OutputFiles.Emplace_GetRef();
			OutputFile.Filename = BulkRecord.Info.LooseFilePath;
			OutputFile.Buffer = FCompositeBuffer(BulkRecord.Buffer);
			OutputFile.Regions = MoveTemp(BulkRecord.Regions);
			OutputFile.bIsSidecar = true;
			OutputFile.bContributeToHash = BulkRecord.Info.MultiOutputIndex == 0; // Only caculate the main package output hash
		}
	}
}

void FHotPatcherPackageWriter::CollectForSaveLinkerAdditionalDataRecords(FRecord& Record, FCommitContext& Context)
{
	if (Record.bCompletedExportsArchiveForDiff)
	{
		// Already Added in CompleteExportsArchiveForDiff
		return;
	}

	for (FLinkerAdditionalDataRecord& AdditionalRecord : Record.LinkerAdditionalDatas)
	{
		Context.ExportsBuffers[AdditionalRecord.Info.MultiOutputIndex].Add(FExportBuffer{ AdditionalRecord.Buffer, MoveTemp(AdditionalRecord.Regions) });
	}
}

void FHotPatcherPackageWriter::CollectForSaveAdditionalFileRecords(FRecord& Record, FCommitContext& Context)
{
	for (FAdditionalFileRecord& AdditionalRecord : Record.AdditionalFiles)
	{
		FWriteFileData& OutputFile = Context.OutputFiles.Emplace_GetRef();
		OutputFile.Filename = AdditionalRecord.Info.Filename;
		OutputFile.Buffer = FCompositeBuffer(AdditionalRecord.Buffer);
		OutputFile.bIsSidecar = true;
		OutputFile.bContributeToHash = AdditionalRecord.Info.MultiOutputIndex == 0; // Only calculate the main package output hash
	}
}

void FHotPatcherPackageWriter::CollectForSaveExportsFooter(FRecord& Record, FCommitContext& Context)
{
	if (Record.bCompletedExportsArchiveForDiff)
	{
		// Already Added in CompleteExportsArchiveForDiff
		return;
	}

	uint32 FooterData = PACKAGE_FILE_TAG;
	FSharedBuffer Buffer = FSharedBuffer::Clone(&FooterData, sizeof(FooterData));
	for (FPackageWriterRecords::FWritePackage& Package : Record.Packages)
	{
		Context.ExportsBuffers[Package.Info.MultiOutputIndex].Add(FExportBuffer{ Buffer, TArray<FFileRegion>() });
	}
}
void FHotPatcherPackageWriter::CollectForSaveExportsBuffers(FRecord& Record, FCommitContext& Context)
{
	check(Context.ExportsBuffers.Num() == Record.Packages.Num());
	for (FPackageWriterRecords::FWritePackage& Package : Record.Packages)
	{
		TArray<FExportBuffer>& ExportsBuffers = Context.ExportsBuffers[Package.Info.MultiOutputIndex];
		check(ExportsBuffers.Num() > 0);

		// Split the ExportsBuffer into (1) Header and (2) Exports + AllAppendedData
		int64 HeaderSize = Package.Info.HeaderSize;
		FExportBuffer& HeaderAndExportsBuffer = ExportsBuffers[0];
		FSharedBuffer& HeaderAndExportsData = HeaderAndExportsBuffer.Buffer;

		// Header (.uasset/.umap)
		{
			FWriteFileData& OutputFile = Context.OutputFiles.Emplace_GetRef();
			OutputFile.Filename = Package.Info.LooseFilePath;
			OutputFile.Buffer = FCompositeBuffer(
				FSharedBuffer::MakeView(HeaderAndExportsData.GetData(), HeaderSize, HeaderAndExportsData));
			OutputFile.bIsSidecar = false;
			OutputFile.bContributeToHash = Package.Info.MultiOutputIndex == 0; // Only calculate the main package output hash
		}

		// Exports + AllAppendedData (.uexp)
		{
			FWriteFileData& OutputFile = Context.OutputFiles.Emplace_GetRef();
			OutputFile.Filename = FPaths::ChangeExtension(Package.Info.LooseFilePath, LexToString(EPackageExtension::Exports));
			OutputFile.bIsSidecar = false;
			OutputFile.bContributeToHash = Package.Info.MultiOutputIndex == 0; // Only caculate the main package output hash

			int32 NumBuffers = ExportsBuffers.Num();
			TArray<FSharedBuffer> BuffersForComposition;
			BuffersForComposition.Reserve(NumBuffers);

			const uint8* ExportsStart = static_cast<const uint8*>(HeaderAndExportsData.GetData()) + HeaderSize;
			BuffersForComposition.Add(FSharedBuffer::MakeView(ExportsStart, HeaderAndExportsData.GetSize() - HeaderSize,
				HeaderAndExportsData));
			OutputFile.Regions.Append(MoveTemp(HeaderAndExportsBuffer.Regions));

			for (FExportBuffer& ExportsBuffer : TArrayView<FExportBuffer>(ExportsBuffers).Slice(1, NumBuffers - 1))
			{
				BuffersForComposition.Add(ExportsBuffer.Buffer);
				OutputFile.Regions.Append(MoveTemp(ExportsBuffer.Regions));
			}
			OutputFile.Buffer = FCompositeBuffer(BuffersForComposition);

			// Adjust regions so they are relative to the start of the uexp file
			for (FFileRegion& Region : OutputFile.Regions)
			{
				Region.Offset -= HeaderSize;
			}
		}
	}
}


TFuture<FMD5Hash> FHotPatcherPackageWriter::AsyncSave(FRecord& Record, const FCommitPackageInfo& Info)
{
	FCommitContext Context{ Info };

	// The order of these collection calls is important, both for ExportsBuffers (affects the meaning of offsets
	// to those buffers) and for OutputFiles (affects the calculation of the Hash for the set of PackageData)
	// The order of ExportsBuffers must match CompleteExportsArchiveForDiff.
	CollectForSavePackageData(Record, Context);
	CollectForSaveBulkData(Record, Context);
	CollectForSaveLinkerAdditionalDataRecords(Record, Context);
	CollectForSaveAdditionalFileRecords(Record, Context);
	CollectForSaveExportsFooter(Record, Context);
	CollectForSaveExportsBuffers(Record, Context);

	return AsyncSaveOutputFiles(Record, Context);
}

TFuture<FMD5Hash> FHotPatcherPackageWriter::AsyncSaveOutputFiles(FRecord& Record, FCommitContext& Context)
{
	if (!EnumHasAnyFlags(Context.Info.WriteOptions, EWriteOptions::Write | EWriteOptions::ComputeHash))
	{
		return TFuture<FMD5Hash>();
	}

	UE::SavePackageUtilities::IncrementOutstandingAsyncWrites();
	FMD5Hash OutputHash;
	FMD5 AccumulatedHash;
	for (FWriteFileData& OutputFile : Context.OutputFiles)
	{
		OutputFile.Write(AccumulatedHash, Context.Info.WriteOptions);
	}
	OutputHash.Set(AccumulatedHash);
	
	return Async(EAsyncExecution::TaskGraph,[OutputHash]()mutable ->FMD5Hash
	{
		UE::SavePackageUtilities::DecrementOutstandingAsyncWrites();
		return OutputHash;
	});
}

FDateTime FHotPatcherPackageWriter::GetPreviousCookTime() const
{
	FString PreviousMetadataPath = FPaths::ProjectDir() / TEXT("Metadata");
	const FString PreviousAssetRegistry = FPaths::Combine(PreviousMetadataPath, GetDevelopmentAssetRegistryFilename());
	return IFileManager::Get().GetTimeStamp(*PreviousAssetRegistry);
}

void FHotPatcherPackageWriter::CommitPackageInternal(FPackageRecord&& Record,
	const IPackageWriter::FCommitPackageInfo& Info)
{
	FRecord& InRecord = static_cast<FRecord&>(Record);
	TFuture<FMD5Hash> CookedHash;
	if (Info.Status == ECommitStatus::Success)
	{
		CookedHash = AsyncSave(InRecord, Info);
		UpdateManifest(InRecord);
	}
}

void FHotPatcherPackageWriter::UpdateManifest(FRecord& Record)
{
	FScopeLock Lock(&OplogLock);
	for (const FPackageWriterRecords::FWritePackage& Package : Record.Packages)
	{
		FOplogPackageInfo* PackageInfo = Oplog.Find(Package.Info.PackageName);
		check(PackageInfo);
		FOplogChunkInfo& ChunkInfo = PackageInfo->PackageDataChunks.AddDefaulted_GetRef();
		ChunkInfo.ChunkId = Package.Info.ChunkId;
		FStringView RelativePathView;
		if (!FPathViews::TryMakeChildPathRelativeTo(Package.Info.LooseFilePath, OutputPath, RelativePathView))
		{
			// Fallback: if the loose path is not under OutputPath, store the full path so the
			// IoStore tool can still resolve the file when joined against the cooked directory.
			RelativePathView = Package.Info.LooseFilePath;
		}
		ChunkInfo.RelativeFileName = RelativePathView;
	}
	for (const FPackageWriterRecords::FBulkData& BulkData : Record.BulkDatas)
	{
		FOplogPackageInfo* PackageInfo = Oplog.Find(BulkData.Info.PackageName);
		check(PackageInfo);
		FOplogChunkInfo& ChunkInfo = PackageInfo->BulkDataChunks.AddDefaulted_GetRef();
		ChunkInfo.ChunkId = BulkData.Info.ChunkId;
		FStringView RelativePathView;
		if (!FPathViews::TryMakeChildPathRelativeTo(BulkData.Info.LooseFilePath, OutputPath, RelativePathView))
		{
			RelativePathView = BulkData.Info.LooseFilePath;
		}
		ChunkInfo.RelativeFileName = RelativePathView;
	}
}

void FHotPatcherPackageWriter::WriteOplogEntry(FCbWriter& Writer, const FOplogPackageInfo& PackageInfo)
{
	Writer.BeginObject();
	Writer.BeginObject("packagestoreentry");
	Writer << "packagename" << PackageInfo.PackageName;
	Writer.EndObject();
	Writer.BeginArray("packagedata");
	for (const FOplogChunkInfo& ChunkInfo : PackageInfo.PackageDataChunks)
	{
		Writer.BeginObject();
		Writer << "id" << ChunkInfo.ChunkId;
		Writer << "filename" << ChunkInfo.RelativeFileName;
		Writer.EndObject();
	}
	Writer.EndArray();
	Writer.BeginArray("bulkdata");
	for (const FOplogChunkInfo& ChunkInfo : PackageInfo.BulkDataChunks)
	{
		Writer.BeginObject();
		Writer << "id" << ChunkInfo.ChunkId;
		Writer << "filename" << ChunkInfo.RelativeFileName;
		Writer.EndObject();
	}
	Writer.EndArray();
	Writer.EndObject();
}

/** Version of the superclass's per-package record that includes our class-specific data. */
struct FHotRecord : public FPackageWriterRecords::FPackage
{
};

FPackageWriterRecords::FPackage* FHotPatcherPackageWriter::ConstructRecord()
{
	return new FHotRecord();
}

static void WriteToFile(const FString& Filename, const FCompositeBuffer& Buffer)
{
	IFileManager& FileManager = IFileManager::Get();

	struct FFailureReason
	{
		uint32 LastErrorCode = 0;
		bool bSizeMatchFailed = false;
	};
	TOptional<FFailureReason> FailureReason;

	for (int32 Tries = 0; Tries < 3; ++Tries)
	{
		FArchive* Ar = FileManager.CreateFileWriter(*Filename);
		if (!Ar)
		{
			if (!FailureReason)
			{
				FailureReason = FFailureReason{ FPlatformMisc::GetLastError(), false };
			}
			continue;
		}

		int64 DataSize = 0;
		for (const FSharedBuffer& Segment : Buffer.GetSegments())
		{
			int64 SegmentSize = static_cast<int64>(Segment.GetSize());
			Ar->Serialize(const_cast<void*>(Segment.GetData()), SegmentSize);
			DataSize += SegmentSize;
		}
		delete Ar;

		if (FileManager.FileSize(*Filename) != DataSize)
		{
			if (!FailureReason)
			{
				FailureReason = FFailureReason{ 0, true };
			}
			FileManager.Delete(*Filename);
			continue;
		}
		return;
	}

	TCHAR LastErrorText[1024];
	if (FailureReason && FailureReason->bSizeMatchFailed)
	{
		FCString::Strcpy(LastErrorText, TEXT("Unexpected file size. Another operation is modifying the file, or the write operation failed to write completely."));
	}
	else if (FailureReason && FailureReason->LastErrorCode != 0)
	{
		FPlatformMisc::GetSystemErrorMessage(LastErrorText, UE_ARRAY_COUNT(LastErrorText), FailureReason->LastErrorCode);
	}
	else
	{
		FCString::Strcpy(LastErrorText, TEXT("Unknown failure reason."));
	}
	UE_LOG(LogTemp, Fatal, TEXT("SavePackage Async write %s failed: %s"), *Filename, LastErrorText);
}

void FHotPatcherPackageWriter::FWriteFileData::Write(FMD5& AccumulatedHash, EWriteOptions WriteOptions) const
{
	//@todo: FH: Should we calculate the hash of both output, currently only the main package output hash is calculated
	if (EnumHasAnyFlags(WriteOptions, EWriteOptions::ComputeHash) && bContributeToHash)
	{
		for (const FSharedBuffer& Segment : Buffer.GetSegments())
		{
			AccumulatedHash.Update(static_cast<const uint8*>(Segment.GetData()), Segment.GetSize());
		}
	}

	if ((bIsSidecar && EnumHasAnyFlags(WriteOptions, EWriteOptions::WriteSidecars)) ||
		(!bIsSidecar && EnumHasAnyFlags(WriteOptions, EWriteOptions::WritePackage)))
	{
		const FString* WriteFilename = &Filename;
		FString FilenameBuffer;
		if (EnumHasAnyFlags(WriteOptions, EWriteOptions::SaveForDiff))
		{
			FilenameBuffer = FPaths::Combine(FPaths::GetPath(Filename),
				FPaths::GetBaseFilename(Filename) + TEXT("_ForDiff") + FPaths::GetExtension(Filename, true));
			WriteFilename = &FilenameBuffer;
		}
		WriteToFile(*WriteFilename, Buffer);

		if (Regions.Num() > 0)
		{
			TArray<uint8> Memory;
			FMemoryWriter Ar(Memory);
			FFileRegion::SerializeFileRegions(Ar, const_cast<TArray<FFileRegion>&>(Regions));

			WriteToFile(*WriteFilename + FFileRegion::RegionsFileExtension,
				FCompositeBuffer(FSharedBuffer::MakeView(Memory.GetData(), Memory.Num())));
		}
	}
}
#if UE_VERSION_NEWER_THAN(5,1,1)
TFuture<FCbObject> FHotPatcherPackageWriter::WriteMPCookMessageForPackage(FName PackageName)
{
	TPromise<FCbObject> Promise;
	return Promise.GetFuture();
}
#endif

#endif