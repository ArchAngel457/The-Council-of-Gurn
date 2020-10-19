// Copyright 2019 Piotr Macharzewski. All Rights Reserved.

#include "SRMatrix.h"
#include "SymbolRecognizerPlugin.h"
#include "Engine/Engine.h"


FSRDMatrix::FSRDMatrix()
{
	NumRows = 2;
	NumColumns = 2;

	R.Empty();

	for (uint32 row = 0; row < NumRows; row++)
	{
		R.Add(FSRRowItem());

		for (uint32 col = 0; col < NumColumns; col++)
		{
			R[row].C.Add(0);
		}
	}
}

FSRDMatrix::FSRDMatrix(uint32 rows, uint32 cols, float InitVal /*= 0*/)
{
	NumRows = rows;
	NumColumns = cols;

	R.Empty();
	
	for (uint32 row = 0; row < NumRows; row++)
	{
		R.Add(FSRRowItem());
		for (uint32 col = 0; col < NumColumns; col++)
		{
			R[row].C.Add(InitVal);
		}
	}
}

FSRDMatrix::FSRDMatrix(uint32 rows, uint32 cols, TArray<float> InData)
{
	SetFromData(rows, cols, InData);
}

FSRDMatrix::FSRDMatrix(uint32 rows, uint32 cols, const FMatrixOperationDelegate& Operation)
{
	NumRows = rows;
	NumColumns = cols;

	R.Empty();

	for (uint32 row = 0; row < NumRows; row++)
	{
		R.Add(FSRRowItem());

		for (uint32 col = 0; col < NumColumns; col++)
		{
			float val = Operation.Execute();
			R[row].C.Add(val);
		}
	}
}

FSRDMatrix FSRDMatrix::GetIdentity()
{
	FSRDMatrix Mat = FSRDMatrix(NumRows, NumColumns, 0.0f);

	if (NumRows != NumColumns)
	{
		return Mat;
	}

	uint32 index = 0;

	for (uint32 row = 0; row < NumRows; row++)
	{
		for (uint32 col = 0; col < NumColumns; col++)
		{
			if (index == row && index == col)
			{
				Mat.R[row].C[col] = 1.0f;
				index++;
			}
		}
	}

	return Mat;
}

float FSRDMatrix::GetValue(uint32 row, uint32 col)
{
	return R[row].C[col];
}

FSRDMatrix FSRDMatrix::GetTranspose()
{
	FSRDMatrix Mat = FSRDMatrix(NumColumns, NumRows, 0.0f);

	for (uint32 row = 0; row < Mat.NumRows; row++)
	{
		for (uint32 col = 0; col < Mat.NumColumns; col++)
		{
			Mat.R[row].C[col] = R[col].C[row];
		}
	}

	return Mat;
}

void FSRDMatrix::Transpose()
{
	*this = GetTranspose();
}

FString FSRDMatrix::ToString() const
{
	FString str = FString::FromInt(NumRows) + "x" + FString::FromInt(NumColumns);

	for (uint32 row = 0; row < NumRows; row++)
	{
		str.Append("\n");
		for (uint32 col = 0; col < NumColumns; col++)
		{

			if (col == NumColumns - 1)
			{
				str += FString::SanitizeFloat(R[row].C[col]);
			}
			else
			{
				str += FString::SanitizeFloat(R[row].C[col]) + ", ";
			}

		}
	}

	return str;
}

float& FSRDMatrix::operator()(uint32 row, uint32 col)
{
	return R[row].C[col];
}

FSRDMatrix FSRDMatrix::operator*(const FSRDMatrix& Other)
{
	if (NumColumns != Other.NumRows)
	{

		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, "COLUMNS IN 1st MUST MATCH ROWS IN 2d!!!");
		return *this;
	}

	FSRDMatrix Mat = FSRDMatrix(NumRows, Other.NumColumns);

	for (uint32 r = 0; r < Mat.NumRows; r++)
	{
		for (uint32 c = 0; c < Mat.NumColumns; c++)
		{
			float val = 0;
			for (uint32 cc = 0; cc < NumColumns; cc++)
			{
				val += R[r].C[cc] * Other.R[cc].C[c];
			}
			Mat.R[r].C[c] = val;
		}
	}

	return Mat;
}

FSRDMatrix FSRDMatrix::operator*=(const FSRDMatrix& Other)
{
	*this = *this * Other;
	return *this;
}

FSRDMatrix FSRDMatrix::operator*(const float& val)
{
	FSRDMatrix newMat = FSRDMatrix(NumRows, NumColumns);
	for (uint32 r = 0; r < NumRows; r++)
	{
		for (uint32 c = 0; c < NumColumns; c++)
		{
			newMat.R[r].C[c] = R[r].C[c] * val;
		}
	}
	return newMat;
}

FSRDMatrix FSRDMatrix::operator*=(const float& val)
{
	*this = *this * val;
	return *this;
}

FSRDMatrix FSRDMatrix::operator+(const FSRDMatrix& Other)
{
	if (NumRows != Other.NumRows || NumColumns != Other.NumColumns)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, "COLUMNS IN 1st MUST MATCH ROWS IN 2d!!!");
		return *this;
	}

	FSRDMatrix Mat = FSRDMatrix(NumRows, NumColumns);

	for (uint32 r = 0; r < NumRows; r++)
	{
		for (uint32 c = 0; c < NumColumns; c++)
		{
			Mat.R[r].C[c] = R[r].C[c] + Other.R[r].C[c];
		}
	}

	return Mat;
}

FSRDMatrix FSRDMatrix::operator+=(const FSRDMatrix& Other)
{
	*this = *this + Other;
	return *this;
}

FSRDMatrix FSRDMatrix::operator-(const FSRDMatrix& Other)
{
	if (NumRows != Other.NumRows || NumColumns != Other.NumColumns)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, "Dimensions must MATCH!!!");
		return *this;
	}

	FSRDMatrix Mat = FSRDMatrix(NumRows, NumColumns);

	for (uint32 r = 0; r < NumRows; r++)
	{
		for (uint32 c = 0; c < NumColumns; c++)
		{
			Mat.R[r].C[c] = R[r].C[c] - Other.R[r].C[c];
		}
	}

	return Mat;
}

FSRDMatrix FSRDMatrix::operator-=(const FSRDMatrix& Other)
{
	*this = *this - Other;
	return *this;
}


FSRDMatrix FSRDMatrix::CompWiseMultiply(const FSRDMatrix& Other)
{
	if (NumColumns != Other.NumColumns || NumRows != Other.NumRows)
	{
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, "MATRICES MUST BE THE SAME DIMENSION!!!");
		return *this;
	}
	FSRDMatrix Mat = FSRDMatrix(NumRows, NumColumns);

	for (uint32 r = 0; r < NumRows; r++)
	{
		for (uint32 c = 0; c < NumColumns; c++)
		{
			Mat.R[r].C[c] = R[r].C[c] * Other.R[r].C[c];
		}
	}

	return Mat;
}

FSRDMatrix FSRDMatrix::CompWiseOperation(const FMatrixCompWiseOperation& Operation)
{
	FSRDMatrix Mat = FSRDMatrix(NumRows, NumColumns);

	for (uint32 r = 0; r < NumRows; r++)
	{
		for (uint32 c = 0; c < NumColumns; c++)
		{
			Mat.R[r].C[c] = Operation.Execute(R[r].C[c]);
		}
	}

	return Mat;
}

FSRDMatrix FSRDMatrix::ActivationOperation(EActivationFunc InActivationFunc)
{
	FSRDMatrix Mat = FSRDMatrix(NumRows, NumColumns);

	float(FSRDMatrix::*ActivationFN)(const float&)const = nullptr;
	switch (InActivationFunc)
	{
	case FSRDMatrix::Sigmoid:
		ActivationFN = &FSRDMatrix::SigmoidFunc;
		break;
	case FSRDMatrix::ReLU:
		ActivationFN = &FSRDMatrix::ReLUFunc;
		break;
	case FSRDMatrix::TanH:
		ActivationFN = &FSRDMatrix::TanHFunc;
		break;
	}
	for (uint32 r = 0; r < NumRows; r++)
	{
		for (uint32 c = 0; c < NumColumns; c++)
		{
			//Mat.R[r].C[c] = Operation.Execute(R[r].C[c]);
			//Mat.R[r].C[c] = SigmoidFunc(R[r].C[c]);
			Mat.R[r].C[c] = (this->*ActivationFN)(R[r].C[c]);
		}
	}

	return Mat;
}

FSRDMatrix FSRDMatrix::ToSoftMax()
{
	double ExpSum = 0;
	FSRDMatrix Mat(*this);

	for (uint32 r = 0; r < NumRows; r++)
	{
		for (uint32 c = 0; c < NumColumns; c++)
		{
			double ExpVal = FMath::Exp(Mat.R[r].C[c]);
			Mat.R[r].C[c] = ExpVal;
			ExpSum += ExpVal;
		}
	}
	
	for (uint32 r = 0; r < NumRows; r++)
	{
		for (uint32 c = 0; c < NumColumns; c++)
		{
			Mat.R[r].C[c] /= ExpSum;
		}
	}

	return Mat;
}


void FSRDMatrix::SetOrCreate(uint32 row, uint32 col, float InValue)
{
	if (R.IsValidIndex(row) == false)
	{
		R.Insert(FSRRowItem(), row);
	}

	if (R[row].C.IsValidIndex(col) == false)
	{
		R[row].C.Insert(InValue, col);
	}

	R[row].C[col] = InValue;


	NumRows = R.Num();
	NumColumns = R[0].C.Num();
}

void FSRDMatrix::SetOrCreateRow(uint32 row, TArray<float>& InData)
{
	if (R.IsValidIndex(row) == false)
	{
		R.Insert(FSRRowItem(InData), row);
	}
	else
	{
		R[row] = FSRRowItem(InData);
	}
}

void FSRDMatrix::SetFromData(uint32 rows, uint32 cols, const TArray<float>& InData, int32 from /*= -1*/, int32 to /*= -1*/)
{
	TArray<float> TestData = InData;
	if (from > 0)
	{
		TestData.RemoveAt(0, from);
	}
	if (to > 0)
	{
		TestData.RemoveAt(to, TestData.Num() - to);
	}

	NumRows = rows;
	NumColumns = cols;

	R.Empty();
	uint32 index = 0;


	if ((uint32)TestData.Num() < rows * cols)
	{
		TestData.Insert(-1, rows * cols - 1);
	}

	for (uint32 row = 0; row < NumRows; row++)
	{
		R.Add(FSRRowItem());

		for (uint32 col = 0; col < NumColumns; col++)
		{
			R[row].C.Add(TestData[index]);
			index++;
		}
	}
}

FSRDMatrix operator-(const float & lhs, const FSRDMatrix & rhs)
{
	FSRDMatrix Mat = FSRDMatrix(rhs.NumRows, rhs.NumColumns);

	for (uint32 r = 0; r < rhs.NumRows; r++)
	{
		for (uint32 c = 0; c < rhs.NumColumns; c++)
		{
			Mat.R[r].C[c] = lhs - rhs.R[r].C[c];
		}
	}

	return Mat;
}

FSRDMatrix operator*(const FSRDMatrix & lhs, const FSRDMatrix & rhs)
{
	FSRDMatrix Mat = lhs;
	Mat *= rhs;
	return Mat;
}

