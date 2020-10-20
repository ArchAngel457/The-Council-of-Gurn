// Copyright 2019 Piotr Macharzewski. All Rights Reserved.

#pragma once
#include "Runtime/CoreUObject/Public/UObject/Object.h"
#include "Runtime/SlateCore/Public/Layout/Margin.h"
#include "SRCanvasHandler.generated.h"

DECLARE_DELEGATE_OneParam(FDrawPointDelegate, const FVector2D&)

USTRUCT(NotBlueprintable)
struct SYMBOLRECOGNIZERPLUGIN_API FSRDrawingLayer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, config, Category = "SR")
	FLinearColor LayerColor = FLinearColor::White;
	UPROPERTY(EditAnywhere, config, Category = "SR")
	float BrushScale = 1.0f;
};

USTRUCT(NotBlueprintable)
struct SYMBOLRECOGNIZERPLUGIN_API FSRDrawLine
{
	GENERATED_BODY()

	TArray<FVector2D> Points;
	
	FSRDrawLine() {}
	FSRDrawLine(const FVector2D& InLoc) :
		Points({ InLoc })
	{}

	static inline FIntPoint GetCellID(const FVector2D& InPos, float InBrushSize)
	{
		return FIntPoint(int32(InPos.X / InBrushSize), int32(InPos.Y / InBrushSize));
	}

	inline bool CheckPointExists(const FVector2D& InPos, float InBrushSize, FVector2D* OutExistingPoint = nullptr) const
	{
		FIntPoint TestCell = GetCellID(InPos, InBrushSize);
		for (int32 I = 0; I < Points.Num(); ++I)
		{
			if (TestCell == GetCellID(Points[I], InBrushSize))
			{
				if (OutExistingPoint)
				{
					*OutExistingPoint = Points[I];
				}
				return true;
			}
		}

		return false;
	}
};
class USymbolRecognizer;

UCLASS(Transient, configdonotcheckdefaults)
class SYMBOLRECOGNIZERPLUGIN_API USRCanvasHandler : public UObject
{
	GENERATED_BODY()

	friend class USymbolRecognizer;
	USRCanvasHandler();

	struct FSRDrawPointsHelper
	{
		bool bWasDrawingBroken = false;
		FVector2D LastExistingPoint;
		FIntPoint CurrentCell;
	};

public:
	/*
	* Defines Bursh size and can add layers for brush.
	* Has big impact on recognizing symbols (e.g. bigger brush could be set for profile with very simple patterns).
	*/
	UPROPERTY(EditDefaultsOnly, Category = "SR")
	TArray<FSRDrawingLayer> DrawingLayers;

	float BrushSize = 25;
	static USRCanvasHandler* CreateSRCanvasHandler(USymbolRecognizer* InSymbolRecognizer);
	void InitializeTexture(USymbolRecognizer* InSymbolRecognizer);

	void GetDataFromTexture(TArray<float>& OutData) const;
	bool SaveRenderTargetToDisk(FString InFilePath, FString Filename);

	bool Draw(bool bAddNewStartPoint,const FVector2D& InPosition);
	void ClearCanvas();

	class UCanvasRenderTarget2D* GetTexture() const;
	FORCEINLINE TArray<FSRDrawLine> GetDrawLines() const { return DrawLines; };
	FORCEINLINE const TArray<FSRDrawLine>& GetDrawLinesRef() const { return DrawLines; };
	FORCEINLINE FIntPoint GetLastActiveDrawCell() const { return DrawPointsHelper.CurrentCell; }
	void GetTextureDrawData(TArray<FSRDrawLine>& OutDrawLines, FVector2D& OutSymbolPosition, FVector2D& OutSymbolSize, FVector2D& OutSymbolCenter, FVector2D& OutScaledSymbolSize);
	void GetDrawPointsFromLines(TArray<FVector2D>& OutDrawPoints, float GapSubStepMultiplier = 0.25f) const;
	float GetScaledBrushSize() const;
	
	void MakeDrawCallsForPoints(FDrawPointDelegate InVecDelegate, float GapSubStepMultiplier = 0.25f);
protected:
	virtual void PostInitProperties() override;

private:
	uint32 TextureSize = 28;
	UPROPERTY()
	USymbolRecognizer* SymbolRecognizer;
	UPROPERTY()
	mutable class UCanvasRenderTarget2D* RenderTexture;

	FVector2D SymbolPosition;
	FVector2D SymbolSize;
	FVector2D SymbolCenter;
	FVector2D ScaledSymbolSize;
	FMargin SymbolMaxima;

	TArray<FSRDrawLine> DrawLines;
	FVector2D BrushPosition;
	FSRDrawPointsHelper DrawPointsHelper;

	UFUNCTION()
	void DrawToTexture(class UCanvas* InCanvas, int32 InWidth, int32 InHeight);
	void ResetMaxima();
	void UpdateBrushPositionAndMaxima(FVector2D InBrushPosition);
	void UpdateTexture();
	
};
