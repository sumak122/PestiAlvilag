// Fill out your copyright notice in the Description page of Project Settings.

#include "SpotSound.h"

USpotSound::USpotSound(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoActivate = false;
	// Heard from the spot's side, but how loud is worked out by ASoundSpots.
	bAllowSpatialization = true;
	bOverrideAttenuation = true;
	AttenuationOverrides.bAttenuate = false;
	AttenuationOverrides.bSpatialize = true;
}

bool USpotSound::Init(int32& SampleRate)
{
	NumChannels = 1;
	SampleRateHz = static_cast<float>(FMath::Max(SampleRate, 8000));
	return true;
}

int32 USpotSound::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	const int32 Kind = FMath::Clamp(Spot.load(), 0, static_cast<int32>(CitySynth::ESpot::Count) - 1);
	Voice.Generate(OutAudio, NumSamples, static_cast<CitySynth::ESpot>(Kind), Generation.load(), FMath::Clamp(Gain.load(), 0.f, 1.f),
		FMath::Clamp(Far.load(), 0.f, 1.f), Hour.load(), SampleRateHz);
	return NumSamples;
}
