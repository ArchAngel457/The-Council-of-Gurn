// Copyright 2019 Piotr Macharzewski. All Rights Reserved.

#pragma once
#include "SRMatrix.generated.h"

DECLARE_DELEGATE_RetVal_OneParam(float, FMatrixCompWiseOperation, float);
DECLARE_DELEGATE_RetVal(float, FMatrixOperationDelegate);


USTRUCT()
struct SYMBOLRECOGNIZERPLUGIN_API FSRRowItem
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<float> C;

	FSRRowItem() { C.Empty(); };
	FSRRowItem(const TArray<float>& InData)
	{
		C = InData;
	}

	friend FArchive& operator<<(FArchive& Ar, FSRRowItem& RowItem)
	{
		return Ar << RowItem.C;
	}
};

USTRUCT()
struct SYMBOLRECOGNIZERPLUGIN_API FSRDMatrix
{
public:

	enum EActivationFunc
	{
		Sigmoid = 0,
		ReLU,
		TanH
	};

	GENERATED_USTRUCT_BODY()

	UPROPERTY()
		uint32 NumRows = 2;
	UPROPERTY()
		uint32 NumColumns = 2;

	
	//cannot use nesetd array cuz of serialization issues, so go for struct inside struct instead.
	UPROPERTY()
	TArray<FSRRowItem> R;


	FSRDMatrix();
	FSRDMatrix(uint32 rows, uint32 cols, float InitVal = 0);
	FSRDMatrix(uint32 rows, uint32 cols, const FMatrixOperationDelegate& Operation);
	FSRDMatrix(uint32 rows, uint32 cols, TArray<float> InData);

	FSRDMatrix GetIdentity();
	float GetValue(uint32 row, uint32 col);
	FSRDMatrix GetTranspose();
	void Transpose();

	FString ToString() const;

	float& operator()(uint32 row, uint32 col);
	FSRDMatrix operator*(const FSRDMatrix& Other);
	FSRDMatrix operator*(const float& val);
	FSRDMatrix operator*=(const FSRDMatrix& Other);
	FSRDMatrix operator*=(const float& val);
	FSRDMatrix operator+(const FSRDMatrix& Other);
	FSRDMatrix operator+=(const FSRDMatrix& Other);
	FSRDMatrix operator-(const FSRDMatrix& Other);
	friend FSRDMatrix operator-(const float& lhs, const FSRDMatrix& rhs);
	friend FSRDMatrix operator*(const FSRDMatrix& lhs, const FSRDMatrix& rhs);
	FSRDMatrix operator-=(const FSRDMatrix& Other);

	FSRDMatrix CompWiseMultiply(const FSRDMatrix& Other);
	FSRDMatrix CompWiseOperation(const FMatrixCompWiseOperation& Operation);
	FSRDMatrix ActivationOperation(EActivationFunc InActivationFunc);
	FSRDMatrix ToSoftMax();

	void SetOrCreate(uint32 row, uint32 col, float InValue);
	void SetOrCreateRow(uint32 row, TArray<float>& InData);
	void SetFromData(uint32 rows, uint32 cols, const TArray<float>& InData, int32 from = -1, int32 to = -1);

	FORCEINLINE float SigmoidFunc(const float& Z) const
	{
		return 1.0f / (1.0f + FMath::Exp(-Z));
	}

	FORCEINLINE float ReLUFunc(const float& Z) const
	{
		return FMath::Max<float>(Z, 0.001f * Z);
	}

	FORCEINLINE float TanHFunc(const float& Z) const
	{
		return 2.0f * SigmoidFunc(2.0f * Z) - 1.0f;
	}
};
