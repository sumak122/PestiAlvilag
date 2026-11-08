// Fill out your copyright notice in the Description page of Project Settings.

#include "HorseSound.h"

UHorseSound::UHorseSound(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoActivate = false;
}

bool UHorseSound::Init(int32& SampleRate)
{
	NumChannels = 1;
	SampleRateHz = static_cast<float>(FMath::Max(SampleRate, 8000));
	return true;
}

int32 UHorseSound::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	// Hooves that landed since the last block, a little apart so two at once still sound as two.
	const int32 Landed = FMath::Min(Clops.exchange(0), 4);
	const float Strength = FMath::Clamp(ClopStrength.load(), 0.f, 1.f);
	for (int32 c = 0; c < Landed; ++c)
	{
		Voice.Clop(Strength, c * 0.012f);
	}
	if (Snorts.exchange(0) > 0)
	{
		Voice.Snort();
	}
	if (Whinnies.exchange(0) > 0)
	{
		Voice.Whinny();
	}
	Voice.Generate(OutAudio, NumSamples, FMath::Clamp(Roll.load(), 0.f, 1.f), SampleRateHz);
	return NumSamples;
}
