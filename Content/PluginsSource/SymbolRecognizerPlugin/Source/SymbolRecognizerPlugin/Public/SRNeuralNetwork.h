// Copyright 2019 Piotr Macharzewski. All Rights Reserved.

#pragma once
#include "SRMatrix.h"
#include "SRNeuralNetwork.generated.h"

USTRUCT()
struct SYMBOLRECOGNIZERPLUGIN_API FSRNeuralNetwork
{

	GENERATED_BODY()

	UPROPERTY()
		uint32 InputNodes = 0;
	UPROPERTY()
		uint32 HiddenNodes = 0;
	UPROPERTY()
		uint32 OutputNodes = 0;
	UPROPERTY()
		float LearningRate = 0;
	UPROPERTY()
		FSRDMatrix wih = FSRDMatrix(0,0,0);
	UPROPERTY()
		FSRDMatrix who = FSRDMatrix(0, 0, 0);
	UPROPERTY()
		bool bIsTrained = false;
	
	FSRNeuralNetwork() {};
	FSRNeuralNetwork(uint32 InInputNodes, uint32 InHiddenNodes, uint32 InOutputNodes, float InLearningRate);

	void Train(const TArray<float>& InputList, const TArray<float>& OutputList);
	FSRDMatrix Query(const TArray<float>& InputList) const;

	/*
	* @ Neural Item to verify.
	* @ QueryData Data to test in selected network.
	* @ AnwerIdx Output class id (symbol) that QueryData should refer to
	* @ AcceptableAnserSize if the result is less then return 0.
	* @ DeltaBestAnsers if delta between 2 best answers is lest then return 0.
	* @ return 1 when answer found or 0 otherwise.
	*/
	static int32 GetQueryResult(const FSRNeuralNetwork& Neural, const TArray<float>& QueryData, int32 AnswerIdx, float AcceptableAsnwerSize = 0.5, float DeltaBestAnswers = 0.97);


};
