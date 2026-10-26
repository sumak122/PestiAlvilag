// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "TramSynth.h"
#include <atomic>
#include "TramSound.generated.h"

/**
 * A tram's sound (TramSynth), played from the tram. Set what the tram is
 * doing from the game; the sound is worked out on the sound thread, so the
 * two only meet through the atomic values below.
 */
UCLASS()
class PESTIALVILAG_API UTramSound : public USynthComponent
{
	GENERATED_BODY()

public:
	UTramSound(const FObjectInitializer& ObjectInitializer);

	/** Speed: 1 is cruising. Pull, Brake and Curve: 0..1. */
	void SetMotion(float InSpeed, float InPull, float InBrake, float InCurve)
	{
		Speed.store(InSpeed);
		Pull.store(InPull);
		Brake.store(InBrake);
		Curve.store(InCurve);
	}

	/** "Ding-ding". */
	void Ring() { Rings.fetch_add(1); }

protected:
	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

private:
	std::atomic<float> Speed { 0.f };
	std::atomic<float> Pull { 0.f };
	std::atomic<float> Brake { 0.f };
	std::atomic<float> Curve { 0.f };
	std::atomic<int32> Rings { 0 };

	// Sound thread only.
	float SampleRateHz = 48000.f;
	TramSynth::FVoice Voice;
};
