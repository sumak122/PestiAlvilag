// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "AnimalSynth.h"
#include <atomic>
#include "AnimalSound.generated.h"

/**
 * The street animals' voices (AnimalSynth): barks, growls, meows and hisses.
 * AAnimals has one and moves it to whichever animal speaks. The game asks
 * for a sound with Queue; the sound thread picks it up, so the two only meet
 * through the atomic values below.
 */
UCLASS()
class PESTIALVILAG_API UAnimalSound : public USynthComponent
{
	GENERATED_BODY()

public:
	UAnimalSound(const FObjectInitializer& ObjectInitializer);

	/** Starts a sound, Strength 0..1, Pitch about 1 (higher for a small animal), after Delay seconds. */
	void Queue(AnimalSynth::EKind Kind, float Strength, float InPitch, float Delay = 0.f)
	{
		FAsked& Ask = Asked[static_cast<int32>(Kind)];
		Ask.Strength.store(Strength);
		Ask.Pitch.store(InPitch);
		Ask.Delay.store(Delay);
		Ask.Count.fetch_add(1);
	}

protected:
	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

private:
	/** The sounds asked for of one kind since the sound thread last looked. */
	struct FAsked
	{
		std::atomic<int32> Count { 0 };
		std::atomic<float> Strength { 0.f };
		std::atomic<float> Pitch { 1.f };
		std::atomic<float> Delay { 0.f };
	};

	FAsked Asked[static_cast<int32>(AnimalSynth::EKind::Count)];

	// Sound thread only.
	float SampleRateHz = 48000.f;
	AnimalSynth::FVoice Voice;
};
