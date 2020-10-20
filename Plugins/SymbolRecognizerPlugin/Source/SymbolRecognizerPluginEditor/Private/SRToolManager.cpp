// Copyright 2019 Piotr Macharzewski. All Rights Reserved.
#pragma once
#include "SRToolManager.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "Runtime/ImageWrapper/Public/IImageWrapper.h"
#include "Runtime/ImageWrapper/Public/IImageWrapperModule.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "DesktopPlatform/Public/IDesktopPlatform.h"
#include "MainFrame/Public/Interfaces/IMainFrameModule.h"
#include "Runtime/SlateCore/Public/Widgets/SWindow.h"
#include "DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Runtime/Core/Public/HAL/FileManager.h"
#include "SRPluginEditorStyle.h"
#include "Runtime/Core/Public/HAL/PlatformFilemanager.h"
#include "HighResScreenshot.h"
#include "Runtime/Engine/Public/ImageUtils.h"
#include "SymbolRecognizer.h"
#include "SRNetworkTrainingAsyncTask.h"
#include "SRCanvasHandler.h"
#include "Runtime/Slate/Public/Widgets/Text/STextBlock.h"
#include "Runtime/Slate/Public/Widgets/Layout/SBox.h"
#include "Runtime/Slate/Public/Framework/Application/SlateApplication.h"
#include "Runtime/SlateCore/Public/Widgets/SBoxPanel.h"
#include "Runtime/Slate/Public/Widgets/Input/SButton.h"
#include "SymbolRecognizerPluginEditor.h"
#include "SRPopupHandler.h"

const FString USRToolManager::SREditorIniPath = "Resources/SymbolRecognizerEditor.ini";
const FString USRToolManager::SRSymbolsPath = "Resources/Symbols";


USRToolManager::USRToolManager(class FObjectInitializer const & ObjInit) : Super(ObjInit)
{

}

bool USRToolManager::TryAddNewProfile(FSRProfileData& InProfileData, bool bCallSelectOnNewData /*= true*/)
{
	//add only unique items.
	for (int32 I = 0; I < Profiles.Num(); ++I)
	{
		if (Profiles[I].Path.Equals(InProfileData.Path, ESearchCase::IgnoreCase))
		{
			if (CurrentProfileDataID != I)
			{
				CallProfileSelected(I);
			}
			return false;
		}
	}

	Profiles.Add(InProfileData);
	CallProfileSelected(Profiles.Num() - 1);
	return true;
}

bool USRToolManager::AddNewProfileByDialogDirectory()
{
	FSRProfileData SRD;
	if (OpenDirectoryDialog(SRD.Path, GetDefaultProfilesDirectory(), "Select New Folder for Symbols Data"))
	{
		return TryAddNewProfile(SRD);
	}

	return false;
}

bool USRToolManager::AddNewProfileInDefaultDirectory(const FString& InNewName)
{
	FSRProfileData SRD;
	SRD.Path = GetDefaultProfilesDirectory() + "/" + InNewName;

	if (FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*SRD.Path))
	{
		return TryAddNewProfile(SRD);
	}

	return false;
}

bool USRToolManager::DeleteProfile(int32 InProfileID)
{
	if (Profiles.IsValidIndex(InProfileID))
	{
		if (FPlatformFileManager::Get().GetPlatformFile().DeleteDirectoryRecursively(*Profiles[InProfileID].Path))
		{
			//remove associated neural data file.
			GetSymbolRecognizer()->DeleteProfile(Profiles[InProfileID].GetProfileName());

			Profiles.RemoveAt(InProfileID);
			bool bMustUpdateProfileIdx = (Profiles.Num() == 0);
			bMustUpdateProfileIdx |= (InProfileID <= CurrentProfileDataID);

			if (bMustUpdateProfileIdx)
			{
				CallProfileSelected(FMath::Max(CurrentProfileDataID - 1, 0));
			}
			return true;
		}
	}
	
	return false;
}

bool USRToolManager::DeleteImage(int32 InSymbolId, int32 InImageId)
{
	if (CheckImageFileExists(InSymbolId, InImageId))
	{
		FString ImgPath = GetCurrentProfileRef().Symbols[InSymbolId].Images[InImageId].Path;
		if (FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*ImgPath))
		{
			CallImageSelected(InSymbolId, InImageId);
			return true;
		}

		return false;
	}
	
	return false;
}

void USRToolManager::CallProfileSelected(int32 InSelectedIdx)
{
	CurrentProfileDataID = InSelectedIdx;
	OnProfilesChangedEvent.Broadcast();
	GetSymbolRecognizer()->SelectProfile(GetCurrentProfileRef().GetProfileName(), true, false);
}

TArray<FSRImageDataItem> USRToolManager::GetImagesWithMissingFile(int32 InSymbolId) const
{
	TArray<FSRImageDataItem> Result;

	for (const FSRImageDataItem& Img : Profiles[CurrentProfileDataID].Symbols[InSymbolId].Images)
	{
		if (CheckImageFileExists(Img) == false)
		{
			Result.Add(Img);
		}
	}

	return Result;
}

int32 USRToolManager::GetDrawnImagesCount(int32 InSymbolId) const
{
	return Profiles[CurrentProfileDataID].ImagesPerSymbol - GetImagesWithMissingFile(InSymbolId).Num();
}

void USRToolManager::CallImageSelected(int32 InSymbolId, int32 InImageId)
{
	GetCurrentProfileRef().CurrentSymbolIdx = InSymbolId;
	GetCurrentProfileRef().CurrentImageIdx = InImageId;
	GetCurrentProfileRef().Symbols[InSymbolId].CurrentImage = InImageId;

	ImagesSelectedEvent.Broadcast(GetCurrentProfileRef().Symbols[InSymbolId], GetCurrentProfileRef().Symbols[InSymbolId].Images[InImageId]);
}

FSRSymbolDataItem& USRToolManager::GetCurrentSymbol()
{
	return GetCurrentProfileRef().Symbols[GetCurrentProfileRef().CurrentSymbolIdx];
}

FSRImageDataItem& USRToolManager::GetCurrentImage()
{
	return GetCurrentSymbol().Images[GetCurrentProfileRef().CurrentImageIdx];
}

FSRSymbolDataItem& USRToolManager::GetNextSymbol(const FSRSymbolDataItem& InRoot, int32 InOffsetIdx /*= 1*/)
{
	int32 SymbolsCount = GetCurrentProfileRef().Symbols.Num();
	int32 NewIdx = InRoot.SymbolId + InOffsetIdx;
	if (NewIdx >= SymbolsCount)
	{
		NewIdx = 0;
	}
	else if (NewIdx < 0)
	{
		NewIdx = SymbolsCount - 1;
	}

	return GetCurrentProfileRef().Symbols[NewIdx];
}

FSRImageDataItem& USRToolManager::GetNextImage(const FSRImageDataItem& InRoot, int32 InOffestIdx /*= 1*/)
{
	int32 NewIdx = InRoot.ImgId + InOffestIdx;
	if (NewIdx >= GetCurrentProfileRef().ImagesPerSymbol)
	{
		NewIdx = 0;
	}
	else if (NewIdx < 0)
	{
		NewIdx = GetCurrentProfileRef().ImagesPerSymbol - 1;
	}

	return GetCurrentSymbol().Images[NewIdx];
}

bool USRToolManager::HasImageValidBursh(const FSRImageDataItem& InImage) const
{
	return InImage.ImgBrush.GetResourceObject() != nullptr;
}

bool USRToolManager::CheckImageFileExists(const FSRImageDataItem& InImage) const
{
	return FPlatformFileManager::Get().GetPlatformFile().FileExists(*InImage.Path);
}

bool USRToolManager::CheckImageFileExists(int32 InSymbolId, int32 InImgId) const
{
	return CheckImageFileExists(Profiles[CurrentProfileDataID].Symbols[InSymbolId].Images[InImgId]);
}

bool USRToolManager::TryLoadBrushForImage(FSRImageDataItem& InImage, bool bForceReload)
{
	RefreshImageFileExists(InImage);
	
	if (InImage.bImageDirty)
	{
		InImage.bImageDirty = false;
		bForceReload = true;
	}

	UTexture2D* LoadedTexture = nullptr;
	const bool bAlreadyLoaded = HasImageValidBursh(InImage);

	if (bAlreadyLoaded)
	{
		if (bForceReload && InImage.bImageFileExists)
		{
			LoadedTexture = LoadTexture2DFromPath(InImage.Path);
		}	
	}
	else if (InImage.bImageFileExists)
	{
		LoadedTexture = LoadTexture2DFromPath(InImage.Path);
	}

	if (LoadedTexture)
	{	
		InImage.SetTextureForBrush(LoadedTexture);
	}

	return LoadedTexture != nullptr;
}

bool USRToolManager::TryLoadBrushForImage(int32 InSymbolId, int32 InImgId, bool bForceReload /*= false*/)
{
	return TryLoadBrushForImage(GetCurrentProfileRef().Symbols[InSymbolId].Images[InImgId], bForceReload);
}

const FSlateBrush* USRToolManager::GetNeedsDrawingBrush() const
{
	return FSRPluginEditorStyle::Get().GetBrush("NeedsDrawingIcon");
}

const FSlateBrush* USRToolManager::ChooseBrushFromImage(int32 InSymbolId, int32 InImgId) const
{
	if (Profiles.Num() <= CurrentProfileDataID)
	{
		return GetNeedsDrawingBrush();
	}

	/*if (Profiles[CurrentProfileDataID].Symbols.Num() <= Profiles[CurrentProfileDataID].SymbolsAmount)
	{
		return GetNeedsDrawingBrush();
	}

	if (Profiles[CurrentProfileDataID].Symbols[InSymbolId].Images.Num() <= Profiles[CurrentProfileDataID].ImagesPerSymbol)
	{
		return GetNeedsDrawingBrush();
	}*/

	return Profiles[CurrentProfileDataID].Symbols[InSymbolId].Images[InImgId].bImageFileExists ? &Profiles[CurrentProfileDataID].Symbols[InSymbolId].Images[InImgId].ImgBrush : GetNeedsDrawingBrush();
}

FString USRToolManager::GetAvailableDirectoryForSymbol(const FSRSymbolDataItem& InSymbol, const TArray<FString>& InDirectoriesInUse) const
{
	FString ExpectedDirectory = GetDefaultSymbolsDirectory() + "/" + FString::FromInt(InSymbol.SymbolId);
	return ExpectedDirectory;
}
void USRToolManager::InitializeParams()
{
	SetShouldStopTraining(true);

	if (!bIsLoaded)
	{
		SRLoadConfig();
		bIsLoaded = true;
	}

	FixupDirectories();

	RefreshSymbolDirectories(ESRRefreshDataReason::RDR_FirstRun);

	if (GetCurrentProfileRef().Symbols.Num() > 0)
	{
		GetSymbolRecognizer()->SelectProfile(GetCurrentProfileRef().GetProfileName(), true, false);
		return;
	}

	//FIRST RUN
	FString DefaultFolder = GetDefaultSymbolsDirectory();
	TArray<FString> FoundDirectories;
	FindDirectories(FoundDirectories, DefaultFolder, true);

	//fake
	for (int32 I = 0; I < GetCurrentProfileRef().SymbolsAmount; ++I)
	{
		FSRSymbolDataItem SDI;

		SDI.SymbolId = I;

		if (FoundDirectories.IsValidIndex(I))
		{
			SDI.Path = FoundDirectories[I];
			LoadFilesForSymbol(SDI, SDI.Path);
		}
		else
		{
			SDI.Path = GetAvailableDirectoryForSymbol(SDI, FoundDirectories);
			if (FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*SDI.Path) == false)
			{
				if (FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*SDI.Path) == false)
				{
					GLog->Log("Cannot create directory: " + SDI.Path);
				}
			}
			for (int32 ImgIdx = 0; ImgIdx < GetCurrentProfileRef().ImagesPerSymbol; ++ImgIdx)
			{
				FSRImageDataItem IDI;
				IDI.ImgId = ImgIdx;
				IDI.Path = "...";
				SDI.Images.Add(IDI);
			}
		}
		
		GetCurrentProfileRef().Symbols.Add(SDI);
	}
	
}

USRCanvasHandler* USRToolManager::GetNeuralHandler() const
{
	return GetSymbolRecognizer()->GetCanvasHandler();
}

class USymbolRecognizer* USRToolManager::GetSymbolRecognizer() const
{
	if (SymbolRecognizer == nullptr)
	{
		SymbolRecognizer = USymbolRecognizer::CreateSymbolRecognizer(const_cast<USRToolManager*>(this));
	}

	return SymbolRecognizer;
}

UTexture2D* USRToolManager::LoadTexture2DFromPath(const FString& FullFilePath)
{
	UTexture2D* LoadedT2D = NULL;

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));

	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

	//Load From File
	TArray<uint8> RawFileData;
	if (!FFileHelper::LoadFileToArray(RawFileData, *FullFilePath))
	{
		return NULL;
	}


	//Create T2D!
	if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
	{
		const TArray<uint8>* UncompressedBGRA = NULL;
		if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
		{
			LoadedT2D = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_B8G8R8A8);

			//Valid?
			if (!LoadedT2D)
			{
				return NULL;
			}

			//Copy!
			void* TextureData = LoadedT2D->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memcpy(TextureData, UncompressedBGRA->GetData(), UncompressedBGRA->Num());
			LoadedT2D->PlatformData->Mips[0].BulkData.Unlock();

			//Update!
			LoadedT2D->UpdateResource();
		}
	}

	return LoadedT2D;
}

bool USRToolManager::OpenDirectoryDialog(FString& OutFolderName, FString DefaultFolder, FString Title)
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform == nullptr)
		return false;

	void* ParentWindowWindowHandle = NULL;

	IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
	const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
	if (MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid())
	{
		ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();

	}

	if (Title.IsEmpty())
	{
		Title = "Choose Directory";
	}

	if (DefaultFolder.IsEmpty())
	{
		DefaultFolder = GetDefaultSymbolsDirectory();
	}
	
	if (DesktopPlatform->OpenDirectoryDialog(ParentWindowWindowHandle, Title, DefaultFolder, OutFolderName))
	{
		FString RelativePath = IFileManager::Get().ConvertToRelativePath(*OutFolderName);
		OutFolderName = RelativePath;
		return true;
	}

	return false;
}

bool USRToolManager::OpenFileDialog(FString& OutFileName, FString DefaultFolder /*= FString("")*/, FString Title /*= FString("")*/)
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform == nullptr)
		return false;

	void* ParentWindowWindowHandle = NULL;

	IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
	const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
	if (MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid())
	{
		ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();

	}

	if (Title.IsEmpty())
	{
		Title = "Choose File";
	}

	if (DefaultFolder.IsEmpty())
	{
		DefaultFolder = GetDefaultSymbolsDirectory();
	}

	TArray<FString> OutFiles;
	const bool bAllowMultiple = false;
	const FString FileTypes = "png";

	bool bOpened = DesktopPlatform->OpenFileDialog(
		ParentWindowWindowHandle,
		Title,
		DefaultFolder,
		OutFileName,
		FileTypes,
		bAllowMultiple ? EFileDialogFlags::Multiple : EFileDialogFlags::None,
		OutFiles);

	if (OutFiles.Num() > 0)
	{
		OutFileName = OutFiles[0];
	}

	return bOpened;
}

void USRToolManager::FindFilesInDirectory(TArray<FString>& OutFiles, const FString& DirectoryToSeek, bool bFullPath)
{
	IFileManager& FileManager = IFileManager::Get();
	FileManager.FindFiles(OutFiles, *(DirectoryToSeek / TEXT("*")), true, false);
	if (bFullPath)
	{
		for (auto& dir : OutFiles)
		{
			dir = DirectoryToSeek + "/" + dir;
		}
	}
}

void USRToolManager::FindDirectories(TArray<FString>& OutDirectory, const FString& DirectoryToSeek, bool bFullPath)
{
	IFileManager& FileManager = IFileManager::Get();
	FileManager.FindFiles(OutDirectory, *(DirectoryToSeek / TEXT("*")), false, true);
	if (bFullPath)
	{
		for (auto& dir : OutDirectory)
		{
			dir = DirectoryToSeek + "/" + dir;
		}
	}
}

void USRToolManager::LoadFilesForSymbol(FSRSymbolDataItem& InSymbol, const FString& DirectoryToSeek)
{
	TArray<FString> FoundFiles;
	
	FindFilesInDirectory(FoundFiles, DirectoryToSeek + "/", true);
	InSymbol.Images.Empty();
	for (int32 ImgIdx = 0; ImgIdx < GetCurrentProfileRef().ImagesPerSymbol; ++ImgIdx)
	{
		FSRImageDataItem IDI;
		IDI.ImgId = ImgIdx;
		if (FoundFiles.IsValidIndex(ImgIdx))
		{
			IDI.Path = FoundFiles[ImgIdx];
		}
		else
		{
			IDI.Path = "";
		}

		InSymbol.Images.Add(IDI);
	}
}

bool USRToolManager::SaveImage(int32 InSymbolId, int32 InImageId)
{
	FSRImageDataItem& ImgData = GetCurrentProfileRef().Symbols[InSymbolId].Images[InImageId];
	bool bSaveResult = GetNeuralHandler()->SaveRenderTargetToDisk(GetCurrentProfileRef().Symbols[InSymbolId].Path, ImgData.GetImageName());//take path from image or symbol?
	if (CheckImageFileExists(ImgData) == false)
	{
		ImgData.Path = GetCurrentProfileRef().Symbols[InSymbolId].Path + "/" + ImgData.GetImageName();
	}
	ImgData.bImageDirty = true;
	TryLoadBrushForImage(ImgData, true);
	CallImageSelected(InSymbolId, InImageId);
	return bSaveResult;
}

bool USRToolManager::SaveCurrentImage()
{
	return SaveImage(GetCurrentProfileRef().CurrentSymbolIdx, GetCurrentProfileRef().CurrentImageIdx);
}

void USRToolManager::PostCDOContruct()
{
	Super::PostCDOContruct();
	if (!bIsLoaded)
	{
		SRLoadConfig();
		bIsLoaded = true;
	}
	SetShouldStopTraining(false);
}

void USRToolManager::RefreshSymbolDirectories(ESRRefreshDataReason InReson)
{
	if (Profiles.Num() == 0)
	{	
		TArray<FString> FoundSRDirectories;
		FindDirectories(FoundSRDirectories, GetDefaultProfilesDirectory(), true);

		if (FoundSRDirectories.Num() == 0)
		{
			FSRProfileData SRD;
			SRD.Path = GetDefaultProfilesDirectory() + "/TEST";
			Profiles.Add(SRD);
		}
		else
		{
			for (const FString& SRDPath : FoundSRDirectories)
			{
				FSRProfileData SRD;
				SRD.Path = SRDPath;
				Profiles.Add(SRD);
			}
		}
	}

	for (int32 I = 0; I < Profiles.Num(); ++I)
	{
		if (Profiles[I].Path.IsEmpty())
		{
			Profiles[I].Path = GetDefaultProfilesDirectory() + "/" + FString::FromInt(I);
		}

		if (FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*Profiles[I].Path) == false)
		{
			FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*Profiles[I].Path);
		}
	}

	if (CurrentProfileDataID > Profiles.Num() - 1)
	{
		CurrentProfileDataID = Profiles.Num() - 1;
	}
	
	while (GetCurrentProfileRef().Symbols.Num() > GetCurrentProfileRef().SymbolsAmount)
	{
		GetCurrentProfileRef().Symbols.Pop();
	}

	auto RetrieveSymbolsPaths = [this](TArray<FString>& InPaths)
	{
		InPaths.Empty();
		for (auto& Symbol : GetCurrentProfileRef().Symbols)
			InPaths.Add(Symbol.Path);
	};
	
	TArray<FString> SymbolsPaths;
	RetrieveSymbolsPaths(SymbolsPaths);

	for (int32 I = 0; I < GetCurrentProfileRef().Symbols.Num(); ++I)
	{
		auto& Symbol = GetCurrentProfileRef().Symbols[I];
		Symbol.SymbolId = I; //fixup id.
		bool bFilesLoaded = false;
		if (FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*Symbol.Path) == false)
		{
			if (FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*Symbol.Path) == false)
			{
				Symbol.Path = GetAvailableDirectoryForSymbol(Symbol, SymbolsPaths);

				if (FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*Symbol.Path) == false)
				{
					GLog->Log("Cannot create Directory:" + Symbol.Path);
				}
			}

			LoadFilesForSymbol(Symbol, Symbol.Path);
			bFilesLoaded = true;
		}

		//fix images count.
		while (Symbol.Images.Num() > GetCurrentProfileRef().ImagesPerSymbol)
		{
			Symbol.Images.Pop();
		}
		bool bShouldReloadFiles = !bFilesLoaded && (InReson == ESRRefreshDataReason::RDR_FirstRun);
		bShouldReloadFiles |= (Symbol.Images.Num() < GetCurrentProfileRef().ImagesPerSymbol);

		if (bShouldReloadFiles)
		{
			LoadFilesForSymbol(Symbol, Symbol.Path);
		}
	}

	int32 CurrentSymbolsCount = GetCurrentProfileRef().Symbols.Num();
	int32 MissingSymbolsCount = (GetCurrentProfileRef().SymbolsAmount - CurrentSymbolsCount);
	while (GetCurrentProfileRef().SymbolsAmount != GetCurrentProfileRef().Symbols.Num())//add missing symbols.
	{
		FSRSymbolDataItem SDI;
		SDI.SymbolId = GetCurrentProfileRef().Symbols.Num();

		TArray<FString> SymPaths;
		RetrieveSymbolsPaths(SymPaths);

		SDI.Path = GetAvailableDirectoryForSymbol(SDI, SymPaths);
		if (FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*SDI.Path) == false)
		{
			if (FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*SDI.Path) == false)
			{
				GLog->Log("Cannot create directory: " + SDI.Path);
			}
		}
		
		LoadFilesForSymbol(SDI, SDI.Path); //creates FSRImageDataItem from found files or adds empty to match ImagesPerSymbol.
		GetCurrentProfileRef().Symbols.Add(SDI);
	}

	bool bShouldCallImageSalacted = false;
	bShouldCallImageSalacted |= (InReson == ESRRefreshDataReason::RDR_ProfilesChanged);

	if (GetCurrentProfileRef().CurrentSymbolIdx >= GetCurrentProfileRef().SymbolsAmount)
	{
		GetCurrentProfileRef().CurrentSymbolIdx = GetCurrentProfileRef().SymbolsAmount - 1;
		bShouldCallImageSalacted = true;
	}

	if (GetCurrentProfileRef().CurrentImageIdx >= GetCurrentProfileRef().ImagesPerSymbol)
	{
		GetCurrentProfileRef().CurrentImageIdx = 0;

		for (auto& Symbol : GetCurrentProfileRef().Symbols)
		{
			Symbol.CurrentImage = 0;
		}

		bShouldCallImageSalacted = true;
	}

	if (bShouldCallImageSalacted)
	{
		CallImageSelected(GetCurrentProfileRef().CurrentSymbolIdx, GetCurrentProfileRef().CurrentImageIdx);
	}
}

void USRToolManager::RefreshImageFileExists(FSRImageDataItem& OutImageData)
{
	OutImageData.bImageFileExists = CheckImageFileExists(OutImageData);
}

FString USRToolManager::GetDefaultSymbolsDirectory() const
{
	return Profiles[CurrentProfileDataID].Path;
}

FString USRToolManager::GetDefaultProfilesDirectory() const
{
	return FSymbolRecognizerPluginEditorModule::GetPluginDir() + SRSymbolsPath;
}

int32 USRToolManager::GetInputNodesCount() const
{
	return GetSymbolRecognizer()->GetSymbolTextureSize() * GetSymbolRecognizer()->GetSymbolTextureSize();
}

void USRToolManager::TrainNetwork()
{
	TArray<FSRTrainingDataSet> TrainingSets;
	const int32 TrainingSetsNumber = GetCurrentProfileRef().SymbolsAmount;
	TrainingSets.Reserve(TrainingSetsNumber + 1);
	for (int32 SymbolId = 0; SymbolId < TrainingSetsNumber; ++SymbolId)
	{
		TArray<TArray<float>> TrainingSet;
		CollectDataForTrainingSet(TrainingSet, GetCurrentProfileRef().Symbols[SymbolId].GetImagesPaths());
		TrainingSets.Emplace(FSRTrainingDataSet(TrainingSet, TrainingSetsNumber, SymbolId));
		GLog->Log("TrainingSet Collected: " + GetCurrentProfileRef().Symbols[SymbolId].Path);
	}

	int32 TrainingImagesCount = 0;
	for (int32 I = 0; I < TrainingSets.Num(); I++)
		TrainingImagesCount += TrainingSets[I].Inputs.Num();

	
	GetSymbolRecognizer()->GetNeuralNetworkRef(false) = FSRNeuralNetwork(GetInputNodesCount(), GetCurrentProfileRef().HiddenNodes, GetCurrentProfileRef().SymbolsAmount, GetCurrentProfileRef().LearningRate);
	GetSymbolRecognizer()->GetNeuralNetworkRef(false).bIsTrained = false;

	bIsTraining = true;

	int32 EpochValue = GetCurrentProfileRef().bAutoTraining ? 0 : GetCurrentProfileRef().LearningCycles;

	//START ASYNC TASK
	 (new FAutoDeleteAsyncTask<NetworkTrainingAsyncTask>(GetSymbolRecognizer()->GetNeuralNetworkRef(false), TrainingSets, GetCurrentProfileRef().SymbolsAmount, TrainingImagesCount,
		GetInputNodesCount(), GetCurrentProfileRef().HiddenNodes, GetCurrentProfileRef().LearningRate, EpochValue, //0 epchs means auto training until Accuracy is reached.
		 GetCurrentProfileRef().AcceptableTrainingAccuracy, GetCurrentProfileRef().DeltaTwoBestOutcomes,
		FTrainingTaskCompleteDelegate::CreateUObject(this, &USRToolManager::OnTrainingComplete),
		 FTrainingTaskStopDelegate::CreateUObject(this, &USRToolManager::OnTrainingStop)))->StartBackgroundTask();

	 OnStartedTraining.Broadcast();

}

void USRToolManager::CollectDataForTrainingSet(TArray<TArray<float>>& OutData, const TArray<FString>& ImagesPaths)
{
	OutData.Empty();
	//FString FilePath /*= GetTrainingImagesPath()*/;//fixme
	//FilePath.Append(FolderName + "/");
	//todo: add validate.
	for (const FString ImgPath : ImagesPaths)
	{
		TArray<float> TrainingSampleData;
		LoadTrainDataFromTexture(ImgPath, TrainingSampleData, false);
		OutData.Add(TrainingSampleData);
	}
}

bool USRToolManager::LoadTrainDataFromTexture(FString InFilePath, TArray<float>& OutData, bool bAppendPNG /*= true*/)
{
	TArray<uint8> RawFileData;
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

	//InFilePath.Append(InFileName);
	if (bAppendPNG) InFilePath.Append(".png");

	if (FFileHelper::LoadFileToArray(RawFileData, *InFilePath))
	{
		if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
		{
			const TArray<uint8>* UncompressedBGRA = NULL;
			if (ImageWrapper->GetRaw(ERGBFormat::Gray, 8, UncompressedBGRA))
			{
				OutData.Append(*UncompressedBGRA);
			}
		}
	}

	for (float& val : OutData)
	{
		val = (val / 255.0f) * 0.98f + 0.01f;
	}

	return OutData.Num() > 0;
}

void USRToolManager::OnTrainingComplete()
{
	bIsTraining = false;
	GLog->Log("OnTrainingComplete");

	FFunctionGraphTask::CreateAndDispatchWhenReady([this]()
	{
		GetSymbolRecognizer()->SaveNeuralProfile(GetCurrentProfileRef().GetProfileName());
	}, TStatId(), NULL, ENamedThreads::GameThread);

}

void USRToolManager::OnTrainingStop()
{
	NetworkTrainingAsyncTask::bShouldStopSymbolTraining = false;
	bIsTraining = false;
	GLog->Log("OnTrainingStop");
}

void USRToolManager::CancelTrainingTask(bool bShouldSaveResult)
{
	if (bIsTraining)
	{
		NetworkTrainingAsyncTask::bShouldStopSymbolTraining = true;
		if (bShouldSaveResult)
		{
			OnTrainingComplete();
		}
	}
}

float USRToolManager::GetSymbolTrainingProgress() const
{
	return NetworkTrainingAsyncTask::Progress;
}

int32 USRToolManager::GetSymbolTrainingEpochs() const
{
	return NetworkTrainingAsyncTask::CurrentEpochs;
}

void USRToolManager::GetSymbolTrainingScores(TArray<float>& OutScoresPerSymbol, float& OutGlobalScore) const
{
	OutGlobalScore = 0.0f;
	OutScoresPerSymbol = NetworkTrainingAsyncTask::SymbolsScores;
	float SumScores = 0;
	for (float ScoreValue : OutScoresPerSymbol)
	{
		SumScores += ScoreValue;
	}
	OutGlobalScore = SumScores / OutScoresPerSymbol.Num();
}

void USRToolManager::SetShouldStopTraining(bool InValue)
{
	NetworkTrainingAsyncTask::bShouldStopSymbolTraining = InValue;
}

void USRToolManager::SRSaveConfig()
{
	FString FilePath = FSymbolRecognizerPluginEditorModule::GetPluginDir() / SREditorIniPath;
	SaveConfig(CPF_Config, *FilePath);
	GetSymbolRecognizer()->SaveSRData();
}

void USRToolManager::SRLoadConfig()
{
	FString FilePath = FSymbolRecognizerPluginEditorModule::GetPluginDir() / SREditorIniPath;
	LoadConfig(USRToolManager::StaticClass(), *FilePath);
}

void USRToolManager::FixupDirectories()
{
	if (!LastRelativePath.IsEmpty())
	{
		bRelativePathsChanged = !LastRelativePath.Equals(FSymbolRecognizerPluginEditorModule::GetPluginDir(), ESearchCase::IgnoreCase);

		if (bRelativePathsChanged)
		{
			for (auto& Profile : Profiles)
			{
				Profile.Path = Profile.Path.Replace(*LastRelativePath, *FSymbolRecognizerPluginEditorModule::GetPluginDir());

				for (auto& Symbol : Profile.Symbols)
				{
					Symbol.Path = Symbol.Path.Replace(*LastRelativePath, *FSymbolRecognizerPluginEditorModule::GetPluginDir());

					for (auto& Img : Symbol.Images)
					{
						Img.Path = Img.Path.Replace(*LastRelativePath, *FSymbolRecognizerPluginEditorModule::GetPluginDir());
					}
				}
			}
		}
	}

	LastRelativePath = FSymbolRecognizerPluginEditorModule::GetPluginDir();
}

//bool USRToolManager::CreateProfileAsset()
//{
//	
//	FString PackageName = "/Game/SymbolRecognizer/Save/" + GetCurrentProfileRef().GetProfileName();//defnine save path.
//	
//	UPackage *Package = CreatePackage(nullptr, *PackageName);
//	USRProfileDataAsset* NewProfileAsset = NewObject<USRProfileDataAsset>(Package, USRProfileDataAsset::StaticClass(), *GetCurrentProfileRef().GetProfileName(), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone | EObjectFlags::RF_Transactional);
//	NewProfileAsset->NeuralFileName = *GetTrainedDataFullPath(CurrentProfileDataID); 
//
//
//	FAssetRegistryModule::AssetCreated(NewProfileAsset);
//	NewProfileAsset->MarkPackageDirty();
//
//	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
//	bool bSuccess = UPackage::SavePackage(Package, NewProfileAsset, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone | EObjectFlags::RF_Transactional, *PackageFileName, GLog, nullptr, false, true, SAVE_None);
//	
//	UE_LOG(LogTemp, Warning, TEXT("Saved Package: %s"), bSuccess ? TEXT("True") : TEXT("False"));
//	return bSuccess;
//}

bool USRToolManager::Validate_CustomProfileCreated()
{
	for (auto& Profile : Profiles)
	{
		if (Profile.GetProfileName() != "TEST")
		{
			return true;
		}
	}

	return false;
}

bool USRToolManager::Validate_ParamsSetCorrectly()
{
	if (GetCurrentProfileRef().DeltaTwoBestOutcomes > 1.0f)
	{
		return false;
	}

	if (GetCurrentProfileRef().HiddenNodes < 1 || GetCurrentProfileRef().HiddenNodes > 1000)
	{
		return false;
	}

	return true;
}

bool USRToolManager::Validate_AllImagesDrawn()
{
	for (auto& SymbolItem : GetCurrentProfileRef().Symbols)
	{
		for (auto& ImageItem : SymbolItem.Images)
		{
			if (CheckImageFileExists(ImageItem) == false)
			{
				return false;
			}
		}
	}

	return true;
}

bool USRToolManager::Validate_NeuralNetworkFileMatchesProfileParams()
{
	FSRNeuralNetwork& NeuralData = GetSymbolRecognizer()->GetNeuralNetworkRef(true);

	if (!NeuralData.bIsTrained)
	{
		return false;
	}

	if (NeuralData.InputNodes != GetInputNodesCount())//28x28
	{
		return false;
	}

	if (NeuralData.HiddenNodes != GetCurrentProfileRef().HiddenNodes)
	{
		return false;
	}
	
	if (NeuralData.OutputNodes != GetCurrentProfileRef().SymbolsAmount)
	{
		return false;
	}

	
	return true;
}

#if WITH_EDITOR

void USRToolManager::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	/*if (PropertyChangedEvent.GetPropertyName() == TEXT("bTest"))
	{	
		CreateProfileAsset();	
	}*/

	/*FName PropName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	if (PropName == TEXT("DrawingLayers"))
	{
	}*/

	/*if (PropertyChangedEvent.GetPropertyName() == TEXT("bShowTutorial"))
	{
		if (bShowTutorial)
		{
			SRPopup::ShowTutorial(this);
		}
		else
		{
			SRPopup::HidePopup();
		}
	}*/
}

void USRToolManager::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == TEXT("BackgroundHelper"))
	{
		OnBackgroundImageChangedEvent.Broadcast(PropertyChangedEvent.GetArrayIndex(TEXT("Symbols")));
	}
}

bool USRToolManager::CanEditChange(const UProperty* InProperty) const
{
	const bool ParentVal = Super::CanEditChange(InProperty);

	if (InProperty->GetFName() == TEXT("LearningCycles"))
	{
		return (Profiles.Num() > 0) && (!Profiles[CurrentProfileDataID].bAutoTraining);
	}

	return ParentVal;
}

#endif

