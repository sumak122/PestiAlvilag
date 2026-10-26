// Fill out your copyright notice in the Description page of Project Settings.

#include "TramSound.h"

UTramSound::UTramSound(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoActivate = false;
}

bool UTramSound::Init(int32& SampleRate)
{
	NumChannels = 1;
	SampleRateHz = static_cast<float>(FMath::Max(SampleRate, 8000));
	return true;
}

int32 UTramSound::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	if (Rings.exchange(0) > 0)
	{
		Voice.Ring();
	}
	TramSynth::FState Wanted;
	Wanted.Speed = FMath::Clamp(Speed.load(), 0.f, 1.5f);
	Wanted.Pull = FMath::Clamp(Pull.load(), 0.f, 1.f);
	Wanted.Brake = FMath::Clamp(Brake.load(), 0.f, 1.f);
	Wanted.Curve = FMath::Clamp(Curve.load(), 0.f, 1.f);
	Voice.Generate(OutAudio, NumSamples, Wanted, SampleRateHz);
	return NumSamples;
}
