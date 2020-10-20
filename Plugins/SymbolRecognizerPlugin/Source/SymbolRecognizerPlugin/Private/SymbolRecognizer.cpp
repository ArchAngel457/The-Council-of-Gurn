// Copyright 2019 Piotr Macharzewski. All Rights Reserved.

#include "SymbolRecognizer.h"
#include "SymbolRecognizerPlugin.h"
#include "SRCanvasHandler.h"
#include "Runtime/CoreUObject/Public/UObject/Package.h"
#include "SRAccuracyTesting.h"
#include "Runtime/AssetRegistry/Public/AssetRegistryModule.h"

const FString USymbolRecognizer::SymbolRecognizerMountPoint = "/SymbolRecognizerPlugin/";
const FString USymbolRecognizer::SRDataDir = "Content/SymbolRecognizer/Data/";


FORCEINLINE FString USymbolRecognizer::GetSRDataPackageName() const
{
	return SymbolRecognizerMountPoint + SRDataDir + USymbolRecognizerData::StaticClass()->GetName();
}

USymbolRecognizer* USymbolRecognizer::CreateSymbolRecognizer(UObject* WorldContextObject)
{
	USymbolRecognizer* SR = NewObject<USymbolRecognizer>(WorldContextObject);
	SR->PostLoad();


	return SR;
}

USymbolRecognizer* USymbolRecognizer::GetSymbolRecognizerInstance()
{
	static USymbolRecognizer* SymbolRecInstance = nullptr;

	if (SymbolRecInstance == nullptr || !SymbolRecInstance->IsValidLowLevel())
	{
		SymbolRecInstance = CreateSymbolRecognizer(GetTransientPackage());
		SymbolRecInstance->AddToRoot();
	}

	return SymbolRecInstance;
}

void USymbolRecognizer::ShowAccuracyTestingWidget()
{
	FSRAccuracyTesting::ShowAccuracyTesting(this);
}

void USymbolRecognizer::HideAccuracyTestingWidget()
{
	FSRAccuracyTesting::HideAccuracyTesting();
}

void USymbolRecognizer::SRShowPreviewCanvasWidget()
{
	FSRAccuracyTesting::ShowPreviewWidget(this);
}

void USymbolRecognizer::SRHidePreviewCanvasWidget()
{
	FSRAccuracyTesting::HidePreviewWidget();
}

bool USymbolRecognizer::TestDrawing(int32 ExpectedSymbol /*= 0*/, float AccuracyTreshold /*= 0.3f*/, bool bCompareToMostAccurateSymbol /*=true*/) const
{
	if (bCompareToMostAccurateSymbol)
	{
		return (GetMostAccurateSymbol(AccuracyTreshold) == ExpectedSymbol);
	}
	else
	{
		TArray<float> AccuracyList = GetAccuracyList();
		if (AccuracyList.IsValidIndex(ExpectedSymbol))
		{
			return AccuracyList[ExpectedSymbol] >= AccuracyTreshold;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Symbol %i does not exist"), ExpectedSymbol);
			return false;
		}
	}
}

void USymbolRecognizer::PostLoad()
{
	Super::PostLoad();

	if (SRData == nullptr)
	{
		LoadSRData();
	}
}

int32 USymbolRecognizer::GetMostAccurateSymbol(float AccuracyThreshold /*= 0.3f*/) const
{
	TArray<float> QueryData;
	GetCanvasHandler()->GetDataFromTexture(QueryData);

	FSRDMatrix Result = NeuralNetwork.Query(QueryData);

	float bestResult = 0;
	int32 bestAnswerIdx = -1;

	for (int32 SymbolIdx = 0; SymbolIdx < Result.R.Num(); ++SymbolIdx)
	{
		if (Result.R[SymbolIdx].C[0] > 0.9f)
		{
			UE_LOG(LogTemp, Error, TEXT("AnswerID: %i | Result: %f"), SymbolIdx, Result.R[SymbolIdx].C[0]);
		}
		else if (Result.R[SymbolIdx].C[0] > 0.5f)
		{
			UE_LOG(LogTemp, Warning, TEXT("AnswerID: %i | Result: %f"), SymbolIdx, Result.R[SymbolIdx].C[0]);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("AnswerID: %i | Result: %f"), SymbolIdx, Result.R[SymbolIdx].C[0]);
		}

		if (Result.R[SymbolIdx].C[0] > bestResult)
		{
			bestResult = Result.R[SymbolIdx].C[0];
			bestAnswerIdx = SymbolIdx;
		}
	}

	if (bestResult < AccuracyThreshold)
	{
		return -1;
	}

	return bestAnswerIdx;
}

TArray<float> USymbolRecognizer::GetAccuracyList() const
{
	TArray<float> QueryData;
	GetCanvasHandler()->GetDataFromTexture(QueryData);

	FSRDMatrix Result = NeuralNetwork.Query(QueryData);

	TArray<float> ResultList;
	for (int32 I = 0; I < Result.R.Num(); ++I)
	{
		ResultList.Add(Result.R[I].C[0]);
	}

	return ResultList;
}

class USRCanvasHandler* USymbolRecognizer::GetCanvasHandler() const
{
	if (NeuralTexture == nullptr)
	{
		NeuralTexture = USRCanvasHandler::CreateSRCanvasHandler(const_cast<USymbolRecognizer*>(this));
	}

	return NeuralTexture;
}


bool USymbolRecognizer::AssignDrawingLayers(TArray<FSRDrawingLayer>& OutDrawingLayers)
{
	if (SRData)
	{
		return SRData->ReadDrawLayers(GetCurrentProfile(), OutDrawingLayers);
	}

	return false;
}

void USymbolRecognizer::BeginDrawing()
{
	bIsDrawing = true;
	bAddNewDrawSpot = true;
}

void USymbolRecognizer::Draw(const FVector2D& InDrawPosition)
{
	if (GetCanvasHandler()->Draw(bAddNewDrawSpot, InDrawPosition))
	{
		bAddNewDrawSpot = false;
	}
}

void USymbolRecognizer::EndDrawing()
{
	bIsDrawing = false;
	bAddNewDrawSpot = false;
}

void USymbolRecognizer::ClearCanvas()
{
	GetCanvasHandler()->ClearCanvas();
}

FSRNeuralNetwork& USymbolRecognizer::GetNeuralNetworkRef(bool bTryLoad)
{
	if (bIsLoaded)
	{
		return NeuralNetwork;
	}
	
	if (bTryLoad)
	{
		LoadNeuralNetworkFromSRData(NeuralNetwork);
		bIsLoaded = true;
	}
	
	return NeuralNetwork;
}


void USymbolRecognizer::SaveNeuralProfile(const FString InProfile, FSRNeuralNetwork& InNeuralNetwork)
{
	SRData->NeuralProfiles.Add(InProfile, InNeuralNetwork);
	SelectProfile(InProfile, false, true);
}

void USymbolRecognizer::SaveNeuralProfile(const FString InProfile)
{
	SaveNeuralProfile(InProfile, NeuralNetwork);
}

bool USymbolRecognizer::LoadNeuralNetworkFromSRData(FSRNeuralNetwork& NeuralData)
{
	if (FSRNeuralNetwork* SavedNeural = SRData->NeuralProfiles.Find(GetCurrentProfile()))
	{
		NeuralData = *SavedNeural;
		return true;
	}

	return false;
}


bool USymbolRecognizer::SelectProfile(const FString InProfile, bool bAddEmptyIfNotFound, bool bShouldSave)
{
	if (SRData->NeuralProfiles.Contains(InProfile))
	{
		SRData->CurrentProfile = InProfile;

		if (LoadNeuralNetworkFromSRData(NeuralNetwork) == false)
		{
			NeuralNetwork = FSRNeuralNetwork();
		}

		if (bShouldSave)
		{
			SaveSRData();
		}
		return true;
	}
	else if (bAddEmptyIfNotFound)
	{
		SRData->NeuralProfiles.Add(InProfile, FSRNeuralNetwork());
		return SelectProfile(InProfile, false, bShouldSave);
	}

	return false;
}


void USymbolRecognizer::SaveSRData()
{
	if (SRDataPackage == nullptr)
	{
		SRDataPackage = CreatePackage(nullptr, *GetSRDataPackageName());
	}
	
	if (SRData == nullptr)
	{
		SRData = NewObject<USymbolRecognizerData>(SRDataPackage, USymbolRecognizerData::StaticClass(), *USymbolRecognizerData::StaticClass()->GetName(), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
		FAssetRegistryModule::AssetCreated(SRData);
	}

	SRData->WriteDrawLayers(GetCurrentProfile(), GetCanvasHandler()->DrawingLayers);

	SRData->MarkPackageDirty();

	FString PackageFileName = FPackageName::LongPackageNameToFilename(GetSRDataPackageName(), FPackageName::GetAssetPackageExtension());
	bool bSuccess = UPackage::SavePackage(SRDataPackage, SRData, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName, GLog, nullptr, false, true, SAVE_None);
		
	UE_LOG(LogTemp, Warning, TEXT("Saved SRData Package: %s"), bSuccess ? TEXT("True") : TEXT("False"));
}

void USymbolRecognizer::LoadSRData()
{
	SRDataPackage = LoadPackage(nullptr, *GetSRDataPackageName(), LOAD_None);

	if (SRDataPackage == nullptr)
	{
		SaveSRData();
	}
	else
	{
		SRData = FindObject<USymbolRecognizerData>(SRDataPackage, *USymbolRecognizerData::StaticClass()->GetName());
	}

	if (!bIsLoaded)
	{
		GetNeuralNetworkRef(true);
	}
}


bool USymbolRecognizer::DeleteProfile(const FString InProfile)
{
	SRData->DrawLayers.Remove(InProfile);
	return SRData->NeuralProfiles.Remove(InProfile) > 0;
}

void USymbolRecognizer::ChangeProfile(FString InProfile, bool bShouldSave)
{
	if (!SelectProfile(InProfile, false, bShouldSave))
	{
		GLog->Log("Profile: " + InProfile + " does not exist!");
	}
}

FString USymbolRecognizer::GetCurrentProfile() const
{
	return SRData->CurrentProfile;
}

TArray<FString> USymbolRecognizer::GetProfilesNames() const
{
	TArray<FString> Result;
	SRData->NeuralProfiles.GetKeys(Result);
	return Result;
	
}

