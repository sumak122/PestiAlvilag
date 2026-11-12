// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "FootSynth.h"
#include <atomic>
#include "FootSound.generated.h"

/**
 * The player's footsteps (FootSynth), played from him, quietly. The game
 * calls Step as a foot comes down, with the ground under it; the sound is
 * worked out on the sound thread, so the two only meet through the atomic
 * values below.
 */
UCLASS()
class PESTIALVILAG_API UFootSound : public USynthComponent
{
	GENERATED_BODY()

public:
	UFootSound(const FObjectInitializer& ObjectInitializer);

	/** A foot comes down: Strength 0..1, on OnGround, Wet 0..1, Running 0 walking to 1 sprinting. */
	void Step(float InStrength, FootSynth::EGround OnGround, float InWet, float InRunning)
	{
		StepStrength.store(InStrength);
		StepGround.store(static_cast<int32>(OnGround));
		StepWet.store(InWet);
		StepRunning.store(InRunning);
		PendingSteps.fetch_add(1);
	}

protected:
	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

private:
	std::atomic<int32> PendingSteps { 0 };
	std::atomic<float> StepStrength { 0.f };
	std::atomic<int32> StepGround { 0 };
	std::atomic<float> StepWet { 0.f };
	std::atomic<float> StepRunning { 0.f };

	// Sound thread only.
	float SampleRateHz = 48000.f;
	FootSynth::FVoice Voice;
};
