// Fill out your copyright notice in the Description page of Project Settings.

#include "FootSound.h"

UFootSound::UFootSound(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoActivate = false;
}

bool UFootSound::Init(int32& SampleRate)
{
	NumChannels = 1;
	SampleRateHz = static_cast<float>(FMath::Max(SampleRate, 8000));
	return true;
}

int32 UFootSound::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	// Steps since the last block (two at most: a landing and a step), a little apart.
	const int32 NewSteps = FMath::Min(PendingSteps.exchange(0), 2);
	const int32 GroundIndex = FMath::Clamp(StepGround.load(), 0, static_cast<int32>(FootSynth::EGround::Count) - 1);
	for (int32 s = 0; s < NewSteps; ++s)
	{
		Voice.Step(FMath::Clamp(StepStrength.load(), 0.f, 1.f), static_cast<FootSynth::EGround>(GroundIndex),
			FMath::Clamp(StepWet.load(), 0.f, 1.f), FMath::Clamp(StepRunning.load(), 0.f, 1.f), s * 0.03f);
	}
	Voice.Generate(OutAudio, NumSamples, SampleRateHz);
	return NumSamples;
}
