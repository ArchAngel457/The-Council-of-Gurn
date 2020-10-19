// Copyright 2019 Piotr Macharzewski. All Rights Reserved.
#pragma once
#include "Runtime/CoreUObject/Public/UObject/Object.h"
#include "SRNeuralNetwork.h"
#include "SRToolManager.generated.h"



UENUM(NotBlueprintable)
enum class ESRRefreshDataReason : uint8
{
	RDR_Generic,
	RDR_ImageCountChanged,
	RDR_SymbolsCountChanged,
	RDR_ProfilesChanged,
	RDR_FirstRun,
};

USTRUCT(NotBlueprintable)
struct SYMBOLRECOGNIZERPLUGINEDITOR_API FSRImageDataItem
{
	GENERATED_BODY()
		FSRImageDataItem() {};
	
	UPROPERTY()
	FSlateBrush ImgBrush;
	UPROPERTY(EditAnywhere, Category = "SymbolRecognizerPlugin")
	FString Path = "";
	UPROPERTY()
	int32 ImgId = 0;

	bool bImageDirty = false;
	bool bImageFileExists = false;

	void SetTextureForBrush(UObject* InTex)
	{
		ImgBrush.SetResourceObject(InTex);
	}

	FString GetImageName(bool bWithExtension = true) const
	{
		int32 FoundIdx = Path.Find("/", ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		FString ImgName = (FoundIdx > -1) ? Path.RightChop(FoundIdx + 1) : "Tex_" + FString::FromInt(ImgId) + ".png";
		if (!bWithExtension)
		{
			ImgName.RemoveFromEnd(".png");
		}
		
		return ImgName;
	}

};

USTRUCT(NotBlueprintable)
struct SYMBOLRECOGNIZERPLUGINEDITOR_API FSRSymbolDataItem
{
	GENERATED_BODY()
		FSRSymbolDataItem() {};
	
	UPROPERTY(EditAnywhere, Category = "Hidden")
	FString Path = "";
	UPROPERTY(EditAnywhere, Category = "Hidden")
	TArray<FSRImageDataItem> Images;
	UPROPERTY(EditAnywhere, Category = "Hidden")
	int32 SymbolId = 0;
	UPROPERTY(EditAnywhere, Category = "Hidden")
	int32 CurrentImage = 0;
	/*
	 * Background for canvas to help drawing.
	 * Is not saved to the image.
	 */
	UPROPERTY(EditAnywhere, Category = "Hidden")
	TSoftObjectPtr<UTexture2D> BackgroundHelper;

	FString GetSymbolName() const
	{
		int32 FoundIdx = Path.Find("/", ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		if (FoundIdx)
		{
			return Path.RightChop(FoundIdx + 1);
		}
		else
		{
			return FString::FromInt(SymbolId);
		}
	}

	TArray<FString> GetImagesPaths() const
	{
		TArray<FString> ImgsPaths;
		for (const FSRImageDataItem& img : Images)
		{
			ImgsPaths.Add(img.Path);
		}

		return ImgsPaths;
	}
};

USTRUCT(NotBlueprintable)
struct SYMBOLRECOGNIZERPLUGINEDITOR_API FSRProfileData
{
	GENERATED_BODY()

	UPROPERTY(config)
	FString Path;

	UPROPERTY(EditAnywhere, config, Category = "Symbols")
	TArray<FSRSymbolDataItem> Symbols;

	/*
	* Number of patters that the system will learn to recognize.
	* e.g. You want the program to tell apart given letters(symbols) 'A B and C' then the value should be set to 3.
	*/
	UPROPERTY(EditDefaultsOnly, config, meta = (ClampMin = "0", ClampMax = "100", UIMin = "0", UIMax = "100"), Category = "Params")
	int32 SymbolsAmount = 10;

	/*
	* Number of textures that will be used to learn recognizing an assigned symbol.
	* You must draw all the images for each 'Symbol'.
	*/
	UPROPERTY(EditDefaultsOnly, config, meta = (ClampMin = "2", ClampMax = "100", UIMin = "2", UIMax = "100"), Category = "Params")
	int32 ImagesPerSymbol = 5;
	/*
	* Learn until certain accuracy is reached (AcceptableTrainingAccuracy).
	* Automate learning may last longer (in some cases it may never end).
	*/
	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, config, Category = "Params")
	bool bAutoTraining = false;
	/*
	* How many cycles of training should neural network get in order to learn recognizing symbols.
	* This value is found by trial and error but around 200 seems to work well.
	*/
	UPROPERTY(EditAnywhere, AdvancedDisplay, config, meta = (ClampMin = "1", ClampMax = "10000", UIMin = "1", UIMax = "10000"), Category = "Params")
	int32 LearningCycles = 200.0f;
	/*
	* Complexity of the hidden layer (between Input and Output).
	*/
	UPROPERTY(EditAnywhere, AdvancedDisplay, config, meta = (ClampMin = "1", ClampMax = "1000", UIMin = "1", UIMax = "1000"), Category = "Params")
	int32 HiddenNodes = 250;
	/*
	* How fast gradient descent finds its minima.
	* Higher value speeds up learning but may overshoot the best outcome.
	* Suggested to keep it between 0.1 - 0.25.
	*/
	UPROPERTY(EditAnywhere, AdvancedDisplay, config, meta = (ClampMin = "0.01", ClampMax = "0.99", UIMin = "0.01", UIMax = "0.99"), Category = "Params")
	float LearningRate = 0.15;
	/*
	* Lower value speeds up training.
	* Learning will stop when all given images give such accuracy.
	* This value must be bigger than 'DeltaTwoBestOutcomes.
	*/
	UPROPERTY(EditAnywhere, AdvancedDisplay, config, meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1"), Category = "Params")
	float AcceptableTrainingAccuracy = 0.95f;
	/*
	* Lower value speeds up training.
	* Difference between two best accuracy result while training.
	* This value must be lower than 'AcceptableTrainingAccuracy.
	*/
	UPROPERTY(EditAnywhere, AdvancedDisplay, meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1"), config, Category = "Params")
	float DeltaTwoBestOutcomes = 0.9f;

	UPROPERTY(EditAnywhere, Category = "Save")
	FString TrainingDataImgName = "Tex";
	UPROPERTY(EditAnywhere, config, Category = "SymbolRecognizerPlugin")
	int32 CurrentSymbolIdx = 0;
	UPROPERTY(EditAnywhere, config, Category = "SymbolRecognizerPlugin")
	int32 CurrentImageIdx = 0;


	FString GetProfileName() const
	{
		int32 FoundIdx = Path.Find("/", ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		if (FoundIdx)
		{
			return Path.RightChop(FoundIdx + 1);
		}
		else
		{
			return Path;
		}
	}
};

UCLASS(NotBlueprintable, config=SymbolRecognizerEditor, configdonotcheckdefaults, Transient)
class SYMBOLRECOGNIZERPLUGINEDITOR_API USRToolManager : public UObject
{
	GENERATED_UCLASS_BODY()
	
	friend class SRNetworkTrainingAsyncTask;

public:

	static const FString SREditorIniPath;
	static const FString SRSymbolsPath;

	UPROPERTY(EditAnywhere, config, Category = "Hidden")
	int32 CurrentProfileDataID = 0;
	UPROPERTY(config, EditAnywhere, Category = "Hidden")
	TArray<FSRProfileData> Profiles;
	
	FORCEINLINE int32 GetCurrentProfileDataID() { return CurrentProfileDataID; }
	FORCEINLINE FSRProfileData& GetCurrentProfileRef() { return Profiles[CurrentProfileDataID]; }
	bool TryAddNewProfile(FSRProfileData& InProfileData, bool bCallSelectOnNewData = true);
	bool AddNewProfileByDialogDirectory();
	bool AddNewProfileInDefaultDirectory(const FString& InNewName);
	bool DeleteProfile(int32 InProfileID);
	bool DeleteImage(int32 InSymbolId, int32 InImageId);
	void CallProfileSelected(int32 InSelectedIdx);
	void CallImageSelected(int32 InSymbolId, int32 InImageId);

	DECLARE_EVENT_TwoParams(USRToolManager, FImageSelectedEvent, FSRSymbolDataItem, FSRImageDataItem)
	FImageSelectedEvent ImagesSelectedEvent;
	DECLARE_EVENT(USRToolManager, FOnProfilesChangedEvent)
	FOnProfilesChangedEvent OnProfilesChangedEvent;
	DECLARE_EVENT_OneParam(USRToolManager, FOnBackgroundImageChangedEvent, int32)
	FOnBackgroundImageChangedEvent OnBackgroundImageChangedEvent;
	DECLARE_EVENT(USRToolManager, FOnStartedTrainingEvent)
	FOnStartedTrainingEvent OnStartedTraining;

	TArray<FSRImageDataItem> GetImagesWithMissingFile(int32 InSymbolId) const;
	int32 GetDrawnImagesCount(int32 InSymbolId) const;

	FSRSymbolDataItem& GetCurrentSymbol();
	FSRImageDataItem& GetCurrentImage();
	FSRSymbolDataItem& GetNextSymbol(const FSRSymbolDataItem& InRoot, int32 InOffsetIdx = 1);
	FSRImageDataItem& GetNextImage(const FSRImageDataItem& InRoot, int32 InOffestIdx = 1);
	bool HasImageValidBursh(const FSRImageDataItem& InImage) const;
	bool CheckImageFileExists(const FSRImageDataItem& InImage) const;
	bool CheckImageFileExists(int32 InSymbolId, int32 InImgId) const;
	bool TryLoadBrushForImage(FSRImageDataItem& InImage, bool bForceReload = false);
	bool TryLoadBrushForImage(int32 InSymbolId, int32 InImgId, bool bForceReload = false);
	const FSlateBrush* GetNeedsDrawingBrush() const;
	const FSlateBrush* ChooseBrushFromImage(int32 InSymbolId, int32 InImgId) const;
	
	FString GetAvailableDirectoryForSymbol(const FSRSymbolDataItem& InSymbol, const TArray<FString>& InDirectoriesInUse) const;
	void InitializeParams();

	class USRCanvasHandler* GetNeuralHandler() const;
	class USymbolRecognizer* GetSymbolRecognizer() const;

	UTexture2D* LoadTexture2DFromPath(const FString& FullFilePath);

	bool OpenDirectoryDialog(FString& OutFolderName, FString DefaultFolder = FString(""), FString Title = FString(""));
	bool OpenFileDialog(FString& OutFileName, FString DefaultFolder = FString(""), FString Title = FString(""));
	void FindFilesInDirectory(TArray<FString>& OutFiles, const FString& DirectoryToSeek, bool bFullPath);
	void FindDirectories(TArray<FString>& OutDirectory, const FString& DirectoryToSeek, bool bFullPath = true);
	void LoadFilesForSymbol(FSRSymbolDataItem& InSymbol, const FString& DirectoryToSeek);
	bool SaveImage(int32 InSymbolId, int32 InImageId);
	bool SaveCurrentImage();
	virtual void PostCDOContruct() override;

	void RefreshSymbolDirectories(ESRRefreshDataReason InReson = ESRRefreshDataReason::RDR_Generic);
	void RefreshImageFileExists(FSRImageDataItem& OutImageData);
	FString GetDefaultSymbolsDirectory() const;
	FString GetDefaultProfilesDirectory() const;

	/////////////////////////////////////////////////
	int32 GetInputNodesCount() const;

	FORCEINLINE bool GetIsTraningNetwork() const { return bIsTraining; }
	void TrainNetwork();

	void CollectDataForTrainingSet(TArray<TArray<float>>& OutData, const TArray<FString>& Images);//move
	bool LoadTrainDataFromTexture(FString InFilePath, TArray<float>& OutData, bool bAppendPNG = true);//move

	void OnTrainingComplete();
	void OnTrainingStop();

	void CancelTrainingTask(bool bShouldSaveResult = false);
	float GetSymbolTrainingProgress() const;
	int32 GetSymbolTrainingEpochs() const;
	void GetSymbolTrainingScores(TArray<float>& OutScoresPerSymbol, float& OutGlobalScore) const;
	void SetShouldStopTraining(bool InValue);

	void SRSaveConfig();
	void SRLoadConfig();
	void FixupDirectories();
	//bool CreateProfileAsset();

	///////////////Validate
	bool Validate_CustomProfileCreated();
	bool Validate_ParamsSetCorrectly();
	bool Validate_AllImagesDrawn();
	bool Validate_NeuralNetworkFileMatchesProfileParams();


	//UPROPERTY(EditAnywhere)
	//bool bTest = false;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void  PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual bool CanEditChange(const UProperty* InProperty) const override;
#endif

private:
	bool bRelativePathsChanged = false;
	UPROPERTY(config)
	FString LastRelativePath = "";
	bool bIsTraining = false;
	UPROPERTY(config)
	bool bIsLoaded = false;
	UPROPERTY()
	mutable class USymbolRecognizer* SymbolRecognizer;

};

