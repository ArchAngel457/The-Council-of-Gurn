// Copyright 2019 Piotr Macharzewski. All Rights Reserved.

#pragma once

#include "SRNeuralNetwork.h"
#include "SRCanvasHandler.h"
#include "SymbolRecognizer.generated.h"

USTRUCT(NotBlueprintable)
struct FSRDrawLayerWrapper 
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FSRDrawingLayer> DrawLayers;

	FSRDrawLayerWrapper() {}
	FSRDrawLayerWrapper(const TArray<FSRDrawingLayer>& InDrawLayers) : DrawLayers(InDrawLayers){}
};
UCLASS()
class SYMBOLRECOGNIZERPLUGIN_API USymbolRecognizerData : public UObject
{
	GENERATED_BODY()

	friend class USymbolRecognizer;

	UPROPERTY()
	FString CurrentProfile = "TEST";
	UPROPERTY()
	TMap<FString, FSRNeuralNetwork> NeuralProfiles;
	UPROPERTY()
	TMap<FString, FSRDrawLayerWrapper> DrawLayers;

	bool ReadDrawLayers(const FString InProfile, TArray<FSRDrawingLayer>& OutDrawLayers)
	{
		if (FSRDrawLayerWrapper* dlw = DrawLayers.Find(InProfile))
		{
			if (dlw->DrawLayers.Num() > 0)
			{
				OutDrawLayers = dlw->DrawLayers;
				return true;
			}
		}

		return false;
	}

	void WriteDrawLayers(const FString InProfile, const TArray<FSRDrawingLayer>& InDrawLayers)
	{
		DrawLayers.Add(InProfile, FSRDrawLayerWrapper(InDrawLayers));
	}
};

/**
 * 
 */
UCLASS(Blueprintable)
class SYMBOLRECOGNIZERPLUGIN_API USymbolRecognizer : public UObject
{
	GENERATED_BODY()

	friend class USRCanvasHandler;
	
public:
	static const FString SymbolRecognizerMountPoint;
	static const FString SRDataDir;

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "SymbolRecognizerPlugin")
	static USymbolRecognizer* CreateSymbolRecognizer(UObject* WorldContextObject);
	UFUNCTION(BlueprintPure, Category = "SymbolRecognizerPlugin")
	static USymbolRecognizer* GetSymbolRecognizerInstance();

	UFUNCTION(BlueprintCallable, Category = "SymbolRecognizerPlugin")
	void ShowAccuracyTestingWidget();
	UFUNCTION(BlueprintCallable, Category = "SymbolRecognizerPlugin")
	void HideAccuracyTestingWidget();
	UFUNCTION(BlueprintCallable, Category = "SymbolRecognizerPlugin")
	void SRShowPreviewCanvasWidget();
	UFUNCTION(BlueprintCallable, Category = "SymbolRecognizerPlugin")
	void SRHidePreviewCanvasWidget();

	/* 
	* Find out whether currently drawn pattern is acceptable.
	* @param ExpectedSymbol					index of a symbol we want to test.
	* @param AccuracyTreshold				(0-1) drawing accuracy must be bigger to pass.
	* @param CompareToMostAccurateSymbol	return false if found more accurate symbol than ExpectedSymbol.
	* @return true when drawn pattern is accurate enough.
	*/
	UFUNCTION(BlueprintCallable, Category = "SymbolRecognizerPlugin")
	bool TestDrawing(int32 ExpectedSymbol = 0, float AccuracyTreshold = 0.3f, bool bCompareToMostAccurateSymbol = true) const;

	/*
	* Test current drawing and return the most accurate SymbolId.
	* @param AccuracyTreshold		(0-1) drawing accuracy must be bigger to pass.
	* @return SymboldId if found, otherwise -1.
	*/
	UFUNCTION(BlueprintCallable, Category = "SymbolRecognizerPlugin")
	int32 GetMostAccurateSymbol(float AccuracyTreshold = 0.3f) const;
	TArray<float> GetAccuracyList() const;

	UFUNCTION(BlueprintCallable, Category = "SymbolRecognizerPlugin")
	class USRCanvasHandler* GetCanvasHandler() const;
	FORCEINLINE int32 GetSymbolTextureSize() const { return NeuralTextureSize; };
	
	bool AssignDrawingLayers(TArray<FSRDrawingLayer>& OutDrawingLayers);

	/*
	* Adds new drawing line. Should be called on drawing button pressed.
	*/
	UFUNCTION(BlueprintCallable, Category = "SymbolRecognizerPlugin")
	void BeginDrawing();
	UFUNCTION(BlueprintCallable, Category = "SymbolRecognizerPlugin")
	/*
	* Extends drawing line. Should be called in Tick.
	*/
	void Draw(const FVector2D& InDrawPosition);
	UFUNCTION(BlueprintCallable, Category = "SymbolRecognizerPlugin")
	/*
	* Breaks drawing line. Should be called on drawing button release.
	*/
	void EndDrawing();
	/*
	* Clears current drawing, so methods TestDrawing and GetMostAccurateSymbol should be called before.
	*/
	UFUNCTION(BlueprintCallable, Category = "SymbolRecognizerPlugin")
	void ClearCanvas();

	FSRNeuralNetwork& GetNeuralNetworkRef(bool bTryLoad = true);

	void SaveNeuralProfile(const FString InProfile, FSRNeuralNetwork& InNeuralNetwork);
	void SaveNeuralProfile(const FString InProfile);
	bool LoadNeuralNetworkFromSRData(FSRNeuralNetwork& NeuralData);
	bool SelectProfile(const FString InProfile, bool bAddEmptyIfNotFound = false, bool bShouldSave = true);

	void SaveSRData();
	void LoadSRData();
	bool DeleteProfile(const FString InProfile);

	UFUNCTION(BlueprintCallable, Category = "SymbolRecognizerPlugin")
	FORCEINLINE bool CheckIsDrawing() const { return bIsDrawing; }
	
	/*
	* Switch profile to load different NeuralData file.
	*/
	UFUNCTION(BlueprintCallable, Category = "SymbolRecognizerPlugin")
	void ChangeProfile(FString InProfile = "TEST", bool bShouldSave = true);

	FString GetCurrentProfile() const;
	TArray<FString> GetProfilesNames() const;

protected:
	virtual void PostLoad() override;


private:
	UPROPERTY(Transient)
	mutable class USRCanvasHandler* NeuralTexture = nullptr;
	UPROPERTY()
	mutable USymbolRecognizerData* SRData = nullptr;
	UPROPERTY()
	UPackage* SRDataPackage = nullptr;

	UPROPERTY(Transient)
	FSRNeuralNetwork NeuralNetwork;
	bool bIsLoaded = false;
	UPROPERTY(EditAnywhere, Category = "Training")
	int32 NeuralTextureSize = 28;
	bool bIsDrawing = false;
	bool bAddNewDrawSpot = false;

	

	FORCEINLINE FString GetSRDataPackageName() const;

};
