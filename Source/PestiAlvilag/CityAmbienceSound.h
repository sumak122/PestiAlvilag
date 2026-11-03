// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "CitySynth.h"
#include <atomic>
#include "CityAmbienceSound.generated.h"

/**
 * The sound of the city around the listener (CitySynth), in stereo and not
 * placed in the world: ACityAmbience tells it where the listener is and what
 * time it is; the sound is worked out on the sound thread, so the two only
 * meet through the atomic values below.
 */
UCLASS()
class PESTIALVILAG_API UCityAmbienceSound : public USynthComponent
{
	GENERATED_BODY()

public:
	UCityAmbienceSound(const FObjectInitializer& ObjectInitializer);

	void SetScene(const CitySynth::FScene& InScene);

protected:
	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

private:
	std::atomic<float> Hour { 12.f };
	std::atomic<float> Darkness { 0.f };
	std::atomic<float> Park { 0.f };
	std::atomic<float> Market { 0.f };
	std::atomic<float> Busy { 0.f };
	std::atomic<float> Station { 0.f };
	std::atomic<float> StationPan { 0.f };
	std::atomic<float> Church { 0.f };
	std::atomic<float> ChurchPan { 0.f };

	// Sound thread only.
	float SampleRateHz = 48000.f;
	CitySynth::FVoice Voice;
};
