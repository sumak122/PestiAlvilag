// Fill out your copyright notice in the Description page of Project Settings.

#include "AnimalSound.h"

UAnimalSound::UAnimalSound(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoActivate = false;
}

bool UAnimalSound::Init(int32& SampleRate)
{
	NumChannels = 1;
	SampleRateHz = static_cast<float>(FMath::Max(SampleRate, 8000));
	return true;
}

int32 UAnimalSound::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	for (int32 k = 0; k < static_cast<int32>(AnimalSynth::EKind::Count); ++k)
	{
		FAsked& Ask = Asked[k];
		if (Ask.Count.exchange(0) > 0)
		{
			Voice.Play(static_cast<AnimalSynth::EKind>(k), FMath::Clamp(Ask.Strength.load(), 0.f, 1.5f),
				FMath::Clamp(Ask.Pitch.load(), 0.5f, 2.f), FMath::Max(0.f, Ask.Delay.load()));
		}
	}
	Voice.Generate(OutAudio, NumSamples, SampleRateHz);
	return NumSamples;
}
