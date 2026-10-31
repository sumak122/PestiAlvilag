// Fill out your copyright notice in the Description page of Project Settings.

#include "FightSound.h"

UFightSound::UFightSound(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoActivate = false;
}

bool UFightSound::Init(int32& SampleRate)
{
	NumChannels = 1;
	SampleRateHz = static_cast<float>(FMath::Max(SampleRate, 8000));
	return true;
}

int32 UFightSound::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	for (int32 k = 0; k < static_cast<int32>(FightSynth::EKind::Count); ++k)
	{
		FAsked& Ask = Asked[k];
		if (Ask.Count.exchange(0) > 0)
		{
			Voice.Play(static_cast<FightSynth::EKind>(k), FMath::Clamp(Ask.Strength.load(), 0.f, 1.5f), FMath::Max(0.f, Ask.Delay.load()));
		}
	}
	Voice.Pitch = FMath::Clamp(Pitch.load(), 60.f, 300.f);
	Voice.Generate(OutAudio, NumSamples, FMath::Clamp(Breathless.load(), 0.f, 1.f), SampleRateHz);
	return NumSamples;
}
