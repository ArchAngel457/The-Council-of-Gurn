// Copyright 2019 Piotr Macharzewski. All Rights Reserved.
#include "SRNetworkTrainingAsyncTask.h"
#include "SRNeuralNetwork.h"

FThreadSafeBool NetworkTrainingAsyncTask::bShouldStopSymbolTraining = false;
float NetworkTrainingAsyncTask::Progress = 0.0f;
int32 NetworkTrainingAsyncTask::CurrentEpochs = 0;
TArray<float> NetworkTrainingAsyncTask::SymbolsScores = {};

NetworkTrainingAsyncTask::NetworkTrainingAsyncTask(FSRNeuralNetwork& InNeuralItem, TArray<FSRTrainingDataSet>& InTrainigsSet, int32 InSymbolsCount, int32 InAllImagesCount, uint32 InInputs, uint32 InHidden, float InLr, int32 InEpochsLimit, float InAcceptableAccuracy, float InDeltaBestAnswers, FTrainingTaskCompleteDelegate InTrainingTaskComplete, FTrainingTaskStopDelegate InTrainingTaskStop)
	: NeuralItem(InNeuralItem)
	, TrainigsSet(InTrainigsSet)
	, Outputs(InSymbolsCount)
	, AllImagesCount(InAllImagesCount)
	, Inputs(InInputs)
	, Hidden(InHidden)
	, Lr(InLr)
	, EpochsLimit(InEpochsLimit)
	, AcceptableAccuracy(InAcceptableAccuracy)
	, DeltaBestAnswers(InDeltaBestAnswers)
	, TrainingTaskComplete(InTrainingTaskComplete)
	, TrainingTaskStop(InTrainingTaskStop)
{
	Progress = 0.0f;
	bShouldStopSymbolTraining = false;
	CurrentEpochs = 0;
	SymbolsScores.Reset();
	SymbolsScores.Reserve(InSymbolsCount + 1);
	for (int32 I = 0; I < InSymbolsCount; ++I)
		SymbolsScores.Emplace(0);
		
}

void NetworkTrainingAsyncTask::DoWork()
{
	int32 Epochs = 0;
	bool bAutoTraining = EpochsLimit <= 0;
	if (bAutoTraining)
	{
		EpochsLimit = 3;
	}

	while (Epochs < EpochsLimit)
	{
		Epochs++;
		CurrentEpochs = Epochs;

		if (bShouldStopSymbolTraining)
		{
			TrainingTaskStop.ExecuteIfBound();
			GLog->Log("--------------------------------------------------------------------");
			GLog->Log("TASK BROKEN");
			GLog->Log("--------------------------------------------------------------------");
			return;
		}

		for (FSRTrainingDataSet& trainingSet : TrainigsSet)
		{
			for (TArray<float>& trainingData : trainingSet.Inputs)
			{
				if (bShouldStopSymbolTraining)
				{
					TrainingTaskStop.ExecuteIfBound();
					GLog->Log("--------------------------------------------------------------------");
					GLog->Log("TASK BROKEN");
					GLog->Log("--------------------------------------------------------------------");
					return;
				}
				//...

				if (NeuralItem.bIsTrained == false)//initialize all necessary params if it was not in training before.
					NeuralItem = FSRNeuralNetwork(Inputs, Hidden, Outputs, Lr);

				NeuralItem.Train(trainingData, trainingSet.ExpectedOutput);

			}
		}


		//calculate training progress by comparing good answers ratio.
		float GoodAnswersCount = 0;
		for (int32 SymbolIdx = 0; SymbolIdx < TrainigsSet.Num(); ++SymbolIdx)
			//for (FSRTrainingDataSet& trainingSet : TrainigsSet)
		{
			int32 GoodAnswersPerSymbol = 0;
			for (TArray<float>& trainingData : TrainigsSet[SymbolIdx].Inputs)
			{
				GoodAnswersPerSymbol += FSRNeuralNetwork::GetQueryResult(NeuralItem, trainingData, TrainigsSet[SymbolIdx].Answer, AcceptableAccuracy, DeltaBestAnswers);
			}

			SymbolsScores[SymbolIdx] = GoodAnswersPerSymbol / (float)TrainigsSet[SymbolIdx].Inputs.Num();
			GoodAnswersCount += GoodAnswersPerSymbol;
		}

		float Accuracy = GoodAnswersCount / AllImagesCount;
		FString AccuracyStr = FString::SanitizeFloat(Accuracy) + " -- epochs: " + FString::FromInt(Epochs);

		GLog->Log("Accuracy: " + AccuracyStr + " AcceptableAccuracy: " + FString::SanitizeFloat(AcceptableAccuracy));

		if (bAutoTraining)
		{
			//Progress is static var.
			Progress = (Accuracy / AcceptableAccuracy);

			if (Accuracy < AcceptableAccuracy)
			{
				EpochsLimit += 1;
			}
		}
		else
		{
			//Progress is static var.
			Progress = (Epochs / (float)EpochsLimit);
			//GLog->Log("Progress: " + FString::FromInt(Epochs) + "/" + FString::FromInt(EpochsLimit));
		}
	}



	GLog->Log("--------------------------------------------------------------------");
	GLog->Log("End of NetworkTrainingAsyncTask calculation on background thread");
	GLog->Log("--------------------------------------------------------------------");

	TrainingTaskComplete.ExecuteIfBound();
}



