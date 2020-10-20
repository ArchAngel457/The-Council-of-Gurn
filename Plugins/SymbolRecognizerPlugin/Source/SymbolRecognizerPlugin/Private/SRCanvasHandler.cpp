// Copyright 2019 Piotr Macharzewski. All Rights Reserved.

#include "SRCanvasHandler.h"
#include "SymbolRecognizerPlugin.h"
#include "SymbolRecognizer.h"
#include "Runtime/Engine/Public/TextureResource.h"
#include "Runtime/Engine/Public/ImageUtils.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "Runtime/CoreUObject/Public/UObject/Package.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"

#define SR_DEBUG_SHOWBORDER 0

USRCanvasHandler::USRCanvasHandler()
{
	DrawingLayers = { FSRDrawingLayer() };
}

USRCanvasHandler* USRCanvasHandler::CreateSRCanvasHandler(USymbolRecognizer* InSymbolRecognizer)
{
	USRCanvasHandler* NewCanvasHandler = NewObject<USRCanvasHandler>(InSymbolRecognizer, "SRCanvasHandler", RF_Transient);
	NewCanvasHandler->InitializeTexture(InSymbolRecognizer);
	return NewCanvasHandler;
}

UCanvasRenderTarget2D* USRCanvasHandler::GetTexture() const
{
	if (RenderTexture == nullptr)
	{
		RenderTexture = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(GetTransientPackage(), UCanvasRenderTarget2D::StaticClass(), TextureSize, TextureSize);
		RenderTexture->ClearColor = FLinearColor::Black;
		RenderTexture->bAutoGenerateMips = false;
		RenderTexture->bNeedsTwoCopies = false;
		RenderTexture->AddressX = TA_Clamp;
		RenderTexture->AddressY = TA_Clamp;
		RenderTexture->InitCustomFormat(TextureSize, TextureSize, PF_B8G8R8A8, true);
		RenderTexture->UpdateResourceImmediate(true);
		RenderTexture->OnCanvasRenderTargetUpdate.AddDynamic(this, &USRCanvasHandler::DrawToTexture);
	}

	return RenderTexture;
}

void USRCanvasHandler::GetTextureDrawData(TArray<FSRDrawLine>& OutDrawPoints, FVector2D& OutSymbolPosition, FVector2D& OutSymbolSize, FVector2D& OutSymbolCenter, FVector2D& OutScaledSymbolSize)
{
	OutDrawPoints = DrawLines;
	OutSymbolPosition = SymbolPosition;
	OutSymbolSize = SymbolSize;
	OutSymbolCenter = SymbolCenter;
	OutScaledSymbolSize = ScaledSymbolSize;
}

void USRCanvasHandler::GetDrawPointsFromLines(TArray<FVector2D>& OutDrawPoints, float GapSubStepMultiplier) const
{
	for (auto& DrawPoint : DrawLines)
	{
		for (int32 I = 0; I < DrawPoint.Points.Num(); ++I)
		{
			OutDrawPoints.Emplace(DrawPoint.Points[I] - FVector2D(BrushSize, BrushSize) * 0.5f);

			if (I < DrawPoint.Points.Num() - 1)
			{
				const float DistCurrentNext = FVector2D::Distance(DrawPoint.Points[I], DrawPoint.Points[I + 1]);
				const float DrawSubstepStepSize = BrushSize *GapSubStepMultiplier;
				const int32 DrawGapsNum = int32(DistCurrentNext / DrawSubstepStepSize);

				for (int32 GapIdx = DrawGapsNum - 1; GapIdx > 0; --GapIdx)
				{
					float Alpha = GapIdx / float(DrawGapsNum);
					FVector2D DrawLoc = FMath::Lerp(DrawPoint.Points[I], DrawPoint.Points[I + 1], Alpha) - FVector2D(BrushSize, BrushSize) * 0.5f;
					OutDrawPoints.Emplace(DrawLoc);
				}
			}
		}
	}
}

float USRCanvasHandler::GetScaledBrushSize() const
{
	if (DrawingLayers.Num() > 0)
	{
		return BrushSize * DrawingLayers[0].BrushScale;
	}
	else
	{
		return BrushSize;
	}
}

void USRCanvasHandler::MakeDrawCallsForPoints(FDrawPointDelegate InVecDelegate, float GapSubStepMultiplier)
{
	if (InVecDelegate.IsBound() == false)
	{
		return;
	}

	for (auto& DrawPoint : DrawLines)
	{
		for (int32 I = 0; I < DrawPoint.Points.Num(); ++I)
		{
			InVecDelegate.Execute(DrawPoint.Points[I] - FVector2D(BrushSize, BrushSize) * 0.5f);

			if (I < DrawPoint.Points.Num() - 1)
			{
				const float DistCurrentNext = FVector2D::Distance(DrawPoint.Points[I], DrawPoint.Points[I + 1]);
				const float DrawSubstepStepSize = BrushSize * GapSubStepMultiplier;
				const int32 DrawGapsNum = int32(DistCurrentNext / DrawSubstepStepSize);

				for (int32 GapIdx = DrawGapsNum - 1; GapIdx > 0; --GapIdx)
				{
					float Alpha = GapIdx / float(DrawGapsNum);
					FVector2D DrawLoc = FMath::Lerp(DrawPoint.Points[I], DrawPoint.Points[I + 1], Alpha) - FVector2D(BrushSize, BrushSize) * 0.5f;
					InVecDelegate.Execute(DrawLoc);
				}
			}
		}
	}
}

void USRCanvasHandler::PostInitProperties()
{
	Super::PostInitProperties();

}

void USRCanvasHandler::InitializeTexture(USymbolRecognizer* InSymbolRecognizer)
{
	TextureSize = InSymbolRecognizer->GetSymbolTextureSize();
	SymbolRecognizer = InSymbolRecognizer;
	SymbolRecognizer->AssignDrawingLayers(DrawingLayers);
	GetTexture();

	ClearCanvas();
}

void USRCanvasHandler::ClearCanvas()
{
	BrushPosition = FVector2D::ZeroVector;//??
	DrawLines.Empty();
	ResetMaxima();
	ScaledSymbolSize = FVector2D(1, 1);
	UpdateTexture();
}

void USRCanvasHandler::GetDataFromTexture(TArray<float>& OutData) const
{
	FTextureRenderTargetResource* RTResource = GetTexture()->GameThread_GetRenderTargetResource();

	FReadSurfaceDataFlags ReadPixelFlags(RCM_UNorm);
	ReadPixelFlags.SetLinearToGamma(true);

	TArray<FColor> OutBMP;
	RTResource->ReadPixels(OutBMP, ReadPixelFlags);

	for (FColor& color : OutBMP)
	{
		FLinearColor LinearCol = color.ReinterpretAsLinear();
		float resultVal = LinearCol.ComputeLuminance() * 0.98f + 0.01f;
		OutData.Add(resultVal);
	}
}

bool USRCanvasHandler::SaveRenderTargetToDisk(FString InFilePath, FString Filename)
{
	FTextureRenderTargetResource* RTResource = GetTexture()->GameThread_GetRenderTargetResource();

	FReadSurfaceDataFlags ReadPixelFlags(RCM_UNorm);
	ReadPixelFlags.SetLinearToGamma(true);

	TArray<FColor> OutBMP;
	RTResource->ReadPixels(OutBMP, ReadPixelFlags);

	FIntPoint DestSize(GetTexture()->GetSurfaceWidth(), GetTexture()->GetSurfaceHeight());

	if (InFilePath.Right(1).Contains("/") == false)
	{
		InFilePath.Append("/");
	}

	//GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, InFilePath + Filename);

	TArray<uint8> CompressedBitmap;
	FImageUtils::CompressImageArray(DestSize.X, DestSize.Y, OutBMP, CompressedBitmap);
	bool imageSavedOk = FFileHelper::SaveArrayToFile(CompressedBitmap, *(InFilePath + Filename));
	return imageSavedOk;
}

bool USRCanvasHandler::Draw(bool bAddNewStartPoint,const FVector2D& InPosition)
{
	const float TestBrushSize = BrushSize* 0.5f;

	DrawPointsHelper.CurrentCell = FSRDrawLine::GetCellID(DrawPointsHelper.LastExistingPoint, TestBrushSize);

	//prevent adding points in the same spot.
	for (FSRDrawLine& DrawPoint : DrawLines)
	{
		if (DrawPoint.CheckPointExists(InPosition, TestBrushSize))
		{
			DrawPointsHelper.LastExistingPoint = InPosition;
			DrawPointsHelper.bWasDrawingBroken = true;
			return false;
		}
	}
	
	
	//could not draw so try to add new 'root' for points or simply continue drawing.
	if (DrawPointsHelper.bWasDrawingBroken)
	{
		if (DrawLines.Num() > 0 && DrawLines.Last().Points.Num() > 0)
		{
			FIntPoint LastBreakingCell = FSRDrawLine::GetCellID(DrawPointsHelper.LastExistingPoint, BrushSize * 3);
			FIntPoint LastActiveCell = FSRDrawLine::GetCellID(DrawLines.Last().Points.Last(), BrushSize * 3);

			if (LastBreakingCell != LastActiveCell)
			{
				bAddNewStartPoint = true;
			}
		}	
	}

	if (bAddNewStartPoint || DrawLines.Num() == 0)
	{
		DrawLines.Emplace(FSRDrawLine(InPosition));
	}
	else
	{
		DrawLines.Last().Points.Emplace(InPosition);
	}

	UpdateBrushPositionAndMaxima(InPosition);
	UpdateTexture();

	DrawPointsHelper.bWasDrawingBroken = false;
	return true;
}

void USRCanvasHandler::UpdateBrushPositionAndMaxima(FVector2D InBrushPosition)
{
	BrushPosition = InBrushPosition;

	//max left.
	if (BrushPosition.X < SymbolMaxima.Left)
	{
		SymbolMaxima.Left = BrushPosition.X;
	}
	//max right.
	if (BrushPosition.X + BrushSize > SymbolMaxima.Right)
	{
		SymbolMaxima.Right = BrushPosition.X + BrushSize;
	}
	//max up.
	if (BrushPosition.Y < SymbolMaxima.Top)
	{
		SymbolMaxima.Top = BrushPosition.Y;
	}
	//max down.
	if (BrushPosition.Y + BrushSize > SymbolMaxima.Bottom)
	{
		SymbolMaxima.Bottom = BrushPosition.Y + BrushSize;
	}
}

void USRCanvasHandler::UpdateTexture()
{
	if (RenderTexture)
	{
		RenderTexture->UpdateResource();
	}
}

void USRCanvasHandler::DrawToTexture(class UCanvas* InCanvas, int32 InWidth, int32 InHeight)
{
	InCanvas->K2_DrawTexture(NULL, FVector2D::ZeroVector, FVector2D(InWidth, InHeight), FVector2D::ZeroVector, FVector2D::UnitVector, FLinearColor::Black);

	//real size.
	SymbolSize = FVector2D(SymbolMaxima.Right - SymbolMaxima.Left, SymbolMaxima.Bottom - SymbolMaxima.Top);

	FVector2D CanvasSize = FVector2D(InWidth, InWidth);
	FVector2D ScaleFactor = FVector2D::UnitVector;//how much the real size got scaled to fit the canvas.

	ScaledSymbolSize = CanvasSize;
	
	//scale to fit texture bounds.
	if (SymbolSize.X > SymbolSize.Y)
	{
		ScaledSymbolSize.Y *= SymbolSize.Y / SymbolSize.X;
	}
	else if (SymbolSize.X < SymbolSize.Y)
	{
		ScaledSymbolSize.X *= SymbolSize.X / SymbolSize.Y;
	}

	ScaleFactor = ScaledSymbolSize / SymbolSize;

	FVector2D ScaledBrushSize = BrushSize * ScaleFactor;
	float paintSize = 1.7f;//we could scale it down if image gets too big/complicated.
	
	SymbolPosition = FVector2D(SymbolMaxima.Left, SymbolMaxima.Top) * ScaleFactor;//subtract to remove real offset and pretend we are at (0, 0)
	FVector2D SymbolUpperLeftCorner = CanvasSize * 0.5f - ScaledSymbolSize * 0.5f;
	SymbolCenter = SymbolUpperLeftCorner - SymbolPosition + ScaledBrushSize * 0.5f;

	auto DrawFunc = [&](float InPaintScale = 1.0f, FLinearColor InColor = FLinearColor::White)
	{
		for (auto& DrawPoint : DrawLines)
		{
			if (DrawPoint.Points.Num() > 1)
			{
				for (int32 I = 0; I < DrawPoint.Points.Num() - 1; ++I)
				{
					FVector2D CurrentPoint = (DrawPoint.Points[I] * ScaleFactor) + SymbolCenter;
					FVector2D NextPoint = (DrawPoint.Points[I + 1] * ScaleFactor) + SymbolCenter;

					InCanvas->K2_DrawLine(CurrentPoint, NextPoint, paintSize * InPaintScale, InColor);
				}
			}
			else
			{
				FVector2D CurrentPoint = (DrawPoint.Points[0] * ScaleFactor) + SymbolCenter;
				FCanvasTileItem CTI(CurrentPoint, FVector2D(paintSize, paintSize) * InPaintScale, InColor);
				InCanvas->DrawItem(CTI);
			}
		}
	};

	for (int32 I = DrawingLayers.Num() - 1; I > -1; --I)
	{
		DrawFunc(DrawingLayers[I].BrushScale, DrawingLayers[I].LayerColor);
	}

#if SR_DEBUG_SHOWBORDER
	InCanvas->K2_DrawBox(SymbolUpperLeftCorner, ScaledSymbolSize, 1.2f, FLinearColor::Red);
#endif

}


void USRCanvasHandler::ResetMaxima()
{
	//max left.
	SymbolMaxima.Left = 99999;
	//max right.
	SymbolMaxima.Right = -99999;
	//max up.
	SymbolMaxima.Top = 99999;
	//max down.
	SymbolMaxima.Bottom = -99999;
}
