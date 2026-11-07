// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "RainSynth.h"
#include <atomic>
#include "RainSound.generated.h"

/**
 * The rain's sound (RainSynth), all round the listener. AWeather sets how
 * hard it rains; the sound is worked out on the sound thread, so the two
 * only meet through the atomic value below.
 */
UCLASS()
class PESTIALVILAG_API URainSound : public USynthComponent
{
	GENERATED_BODY()

public:
	URainSound(const FObjectInitializer& ObjectInitializer);

	/** 0 dry to 1 pouring. */
	void SetRain(float InRain) { Rain.store(InRain); }

protected:
	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

private:
	std::atomic<float> Rain { 0.f };

	// Sound thread only.
	float SampleRateHz = 48000.f;
	RainSynth::FVoice Voice;
};
