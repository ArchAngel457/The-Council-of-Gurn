// Copyright 2019 Piotr Macharzewski. All Rights Reserved.

#include "SRNeuralNetwork.h"
#include "SymbolRecognizerPlugin.h"


FSRNeuralNetwork::FSRNeuralNetwork(uint32 InInputNodes, uint32 InHiddenNodes, uint32 InOutputNodes, float InLearningRate)
{
	InputNodes = InInputNodes;
	HiddenNodes = InHiddenNodes;
	OutputNodes = InOutputNodes;
	LearningRate = InLearningRate;
	wih = FSRDMatrix(HiddenNodes, InputNodes, FMatrixOperationDelegate::CreateLambda([&]() {return FMath::FRand() * 0.01f + 0.001f; }));
	who = FSRDMatrix(OutputNodes, HiddenNodes, FMatrixOperationDelegate::CreateLambda([&]() {return FMath::FRand() * 0.01f + 0.001f; }));
}

void FSRNeuralNetwork::Train(const TArray<float>& InputList, const TArray<float>& OutputList)
{
	FSRDMatrix Inputs = FSRDMatrix(InputNodes, 1, InputList);
	FSRDMatrix Targets = FSRDMatrix(OutputNodes, 1, OutputList);

	FString InputsStr = "Inputs: " + Inputs.ToString();
	FString TargetsStr = "Targets: " + Targets.ToString();

	//activation FP
	FSRDMatrix HiddenInputs = wih * Inputs;
	FSRDMatrix HiddenOutputs = HiddenInputs.ActivationOperation(FSRDMatrix::Sigmoid);

	FSRDMatrix FinalInputs = who * HiddenOutputs;
	FSRDMatrix FinalOutputs = FinalInputs.ActivationOperation(FSRDMatrix::Sigmoid);

	//errors BP
	FSRDMatrix OutputErrors = Targets - FinalOutputs;
	FSRDMatrix HiddenErrors = who.GetTranspose() * OutputErrors;

	//gradient descent learn
	//update weights from hidden to outputs
	who += OutputErrors.CompWiseMultiply(FinalOutputs).CompWiseMultiply(1.0f - FinalOutputs) * HiddenOutputs.GetTranspose() * LearningRate;
	//update weights from inputs to hidden
	wih += HiddenErrors.CompWiseMultiply(HiddenOutputs).CompWiseMultiply(1.0f - HiddenOutputs) * Inputs.GetTranspose() * LearningRate;

	bIsTrained = true;
}

FSRDMatrix FSRNeuralNetwork::Query(const TArray<float>& InputList) const
{
	FSRDMatrix Inputs = FSRDMatrix(InputNodes, 1, InputList);
	FSRDMatrix HiddenInputs = wih * Inputs;
	FSRDMatrix HiddenOutputs = HiddenInputs.ActivationOperation(FSRDMatrix::Sigmoid);
	FSRDMatrix FinalInputs = who * HiddenOutputs;
	FSRDMatrix FinalOutputs = FinalInputs.ActivationOperation(FSRDMatrix::Sigmoid);

	return FinalOutputs;
}

int32 FSRNeuralNetwork::GetQueryResult(const FSRNeuralNetwork& Neural, const TArray<float>& QueryData, int32 AnswerIdx, float AcceptableAsnwerSize /*= 0.5*/, float DeltaBestAnswers /*= 0.97*/)
{
	FSRDMatrix Result = Neural.Query(QueryData);

	int32 bestAnswer = -1;
	float bestSize = 0;
	float previousSize = 0;

	for (int32 answerIdx = 0; answerIdx < Result.R.Num(); answerIdx++)
	{
		if (Result.R[answerIdx].C[0] > bestSize)
		{
			previousSize = bestSize;
			bestSize = Result.R[answerIdx].C[0];
			bestAnswer = answerIdx;
		}
	}

	float bestPreviousDelta = bestSize - previousSize;

	if (bestAnswer == AnswerIdx
		&& bestPreviousDelta > DeltaBestAnswers
		&& bestSize >= AcceptableAsnwerSize)
	{
		return 1;
	}

	return 0;
}
