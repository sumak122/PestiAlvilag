// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "HorseSynth.h"
#include <atomic>
#include "HorseSound.generated.h"

/**
 * A horse and cart's sound (HorseSynth), played from the cart. The game
 * tells it when a hoof lands and how fast the cart rolls; the sound is
 * worked out on the sound thread, so the two only meet through the atomic
 * values below.
 */
UCLASS()
class PESTIALVILAG_API UHorseSound : public USynthComponent
{
	GENERATED_BODY()

public:
	UHorseSound(const FObjectInitializer& ObjectInitializer);

	/** A hoof lands, Strength 0..1. */
	void Clop(float InStrength)
	{
		ClopStrength.store(InStrength);
		Clops.fetch_add(1);
	}

	void Snort() { Snorts.fetch_add(1); }

	/** 0 standing to 1 at a canter. */
	void SetRoll(float InRoll) { Roll.store(InRoll); }

protected:
	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

private:
	std::atomic<int32> Clops { 0 };
	std::atomic<float> ClopStrength { 0.f };
	std::atomic<int32> Snorts { 0 };
	std::atomic<float> Roll { 0.f };

	// Sound thread only.
	float SampleRateHz = 48000.f;
	HorseSynth::FVoice Voice;
};
