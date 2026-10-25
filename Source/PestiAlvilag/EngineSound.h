// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include <atomic>
#include "EngineSound.generated.h"

/**
 * The putter of an old four-cylinder engine, made while the game runs (no
 * sound files): a thump for each cylinder firing, a little uneven, with a
 * rumble through it. It idles slowly, runs faster with the revs, gets
 * louder and rougher under load, and can bang out a backfire.
 *
 * Set the revs and load from the game; the sound is worked out on the
 * sound thread, so the two only meet through the atomic values below.
 */
UCLASS()
class PESTIALVILAG_API UEngineSound : public USynthComponent
{
	GENERATED_BODY()

public:
	UEngineSound(const FObjectInitializer& ObjectInitializer);

	/** Revs: 0 is idling, 1 flat out. Load: 0 coasting, 1 pedal to the floor. */
	void SetEngine(float InRevs, float InLoad)
	{
		Revs.store(InRevs);
		Load.store(InLoad);
	}

	/** One bang from the tail pipe. */
	void Backfire() { Bangs.fetch_add(1); }

protected:
	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

private:
	float NextNoise();

	std::atomic<float> Revs { 0.f };
	std::atomic<float> Load { 0.f };
	std::atomic<int32> Bangs { 0 };

	// Sound thread only.
	float SampleRateHz = 48000.f;
	float SmoothRevs = 0.f;
	float SmoothLoad = 0.f;
	double FiringPhase = 0.0;
	double TonePhase = 0.0;
	double HumPhase = 0.0;
	float Thump = 0.f;
	float ThumpStrength = 1.f;
	int32 Cylinder = 0;
	float Bang = 0.f;
	float Filtered = 0.f;
	uint32 Seed = 1932u;
};
