// Copyright 2019 Piotr Macharzewski. All Rights Reserved.
#pragma once
#include "Runtime/Core/Public/Async/AsyncWork.h"
#include "SRNetworkTrainingAsyncTask.generated.h"

DECLARE_DELEGATE(FTrainingTaskCompleteDelegate);
DECLARE_DELEGATE(FTrainingTaskStopDelegate);

struct FSRNeuralNetwork;

/*
 * Stores training data for single symbol e.g. Symbol 'A' has 5 images,
 * so we have Answer pointing to Symbol ID and nested array Inputs holding 28x28 pixes floats for each image referring to this symbol ('A')
*/
USTRUCT(NotBlueprintable)
struct SYMBOLRECOGNIZERPLUGINEDITOR_API FSRTrainingDataSet
{
	GENERATED_BODY()
	
	TArray<TArray<float>> Inputs; //<< whole data for single symbol inhere e.g. [Tex01][Fixels28x28]
	int32 Outputs;
	int32 Answer;//symbol id
	TArray<float> ExpectedOutput;
	FSRTrainingDataSet() {};
	FSRTrainingDataSet(TArray<TArray<float>>& InInputs, int32 InOutputs, int32 InAnswer)
		: Inputs(InInputs)
		, Outputs(InOutputs)
		, Answer(InAnswer)
	{
		ExpectedOutput.Reserve(Outputs + 1);
		for (int32 I = 0; I < Outputs; ++I)
			ExpectedOutput.Emplace(0.01f);

		//so its like:
		//A -> [0] = 0.01
		//B -> [1] = 0.01
		//C -> [Answer] = 0.99 
		//D -> [3] = 0.01
		ExpectedOutput[InAnswer] = 0.99f;
	}

};

class SYMBOLRECOGNIZERPLUGINEDITOR_API NetworkTrainingAsyncTask : public FNonAbandonableTask
{
	friend class USRToolManager;

	FSRNeuralNetwork& NeuralItem;
	UPROPERTY()
	TArray<FSRTrainingDataSet> TrainigsSet;
	int32 Outputs;
	int32 AllImagesCount;
	uint32 Inputs;
	uint32 Hidden;
	float Lr;
	int32 EpochsLimit;// if <= 0 then autotraining until Accuracy is reached
	float AcceptableAccuracy;
	float DeltaBestAnswers;
	FTrainingTaskCompleteDelegate TrainingTaskComplete;
	FTrainingTaskStopDelegate TrainingTaskStop;
	static FThreadSafeBool bShouldStopSymbolTraining;
	static float Progress;
	static int32 CurrentEpochs;
	static TArray<float> SymbolsScores;
public:

	NetworkTrainingAsyncTask(
			FSRNeuralNetwork& InNeuralItem
		, TArray<FSRTrainingDataSet>& InTrainigsSet
		, int32 InSymbolsCount
		, int32 InAllImagesCount
		, uint32 InInputs
		, uint32 InHidden
		, float InLr
		, int32 InEpochsLimit
		, float InAcceptableAccuracy, float InDeltaBestAnswers
		, FTrainingTaskCompleteDelegate InTrainingTaskComplete
		, FTrainingTaskStopDelegate InTrainingTaskStop
	);

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(NetworkTrainingAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
	}

	void DoWork();
};