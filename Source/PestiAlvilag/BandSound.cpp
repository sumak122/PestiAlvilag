// Fill out your copyright notice in the Description page of Project Settings.

#include "BandSound.h"

UBandSound::UBandSound(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoActivate = false;
	// Heard as if all round: AMusicBars works out how loud.
	bAllowSpatialization = false;
}

bool UBandSound::Init(int32& SampleRate)
{
	NumChannels = 1;
	SampleRateHz = static_cast<float>(FMath::Max(SampleRate, 8000));
	return true;
}

int32 UBandSound::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	Band.Generate(OutAudio, NumSamples, bPlaying.load(), bSmart.load(), FMath::Clamp(Gain.load(), 0.f, 1.f),
		FMath::Clamp(Far.load(), 0.f, 1.f), SampleRateHz);
	return NumSamples;
}
