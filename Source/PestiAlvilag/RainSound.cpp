// Fill out your copyright notice in the Description page of Project Settings.

#include "RainSound.h"

URainSound::URainSound(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoActivate = false;
}

bool URainSound::Init(int32& SampleRate)
{
	NumChannels = 1;
	SampleRateHz = static_cast<float>(FMath::Max(SampleRate, 8000));
	return true;
}

int32 URainSound::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	Voice.Generate(OutAudio, NumSamples, FMath::Clamp(Rain.load(), 0.f, 1.f), SampleRateHz);
	return NumSamples;
}
