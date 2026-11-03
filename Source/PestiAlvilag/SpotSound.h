// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "CitySynth.h"
#include <atomic>
#include "SpotSound.generated.h"

/**
 * The sound of one sound spot (CitySynth::FSpotVoice), placed in the world
 * where the spot is, so it comes from that side. ASoundSpots moves it to the
 * spot, tells it what kind of spot it is and how loud it reaches the
 * listener; the sound is worked out on the sound thread, so the two only meet
 * through the atomic values below.
 */
UCLASS()
class PESTIALVILAG_API USpotSound : public USynthComponent
{
	GENERATED_BODY()

public:
	USpotSound(const FObjectInitializer& ObjectInitializer);

	/** Give this sound to another spot: it starts afresh. */
	void SetSpot(CitySynth::ESpot InSpot, int32 InGeneration)
	{
		Spot.store(static_cast<int32>(InSpot));
		Generation.store(InGeneration);
	}

	/** InGain 0..1 how loud it reaches the listener; InFar 0 beside it .. 1 at the edge of hearing. */
	void SetHeard(float InGain, float InFar)
	{
		Gain.store(InGain);
		Far.store(InFar);
	}

	void SetHour(float InHour) { Hour.store(InHour); }

protected:
	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

private:
	std::atomic<int32> Spot { 0 };
	std::atomic<int32> Generation { 0 };
	std::atomic<float> Gain { 0.f };
	std::atomic<float> Far { 1.f };
	std::atomic<float> Hour { 12.f };

	// Sound thread only.
	float SampleRateHz = 48000.f;
	CitySynth::FSpotVoice Voice;
};
