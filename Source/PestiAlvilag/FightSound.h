// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "FightSynth.h"
#include <atomic>
#include "FightSound.generated.h"

/**
 * A fighter's sounds (FightSynth), played from him: his blows, the ones that
 * land on him, his grunts and his panting. The game asks for a sound with
 * Queue; the sound thread picks it up, so the two only meet through the
 * atomic values below.
 */
UCLASS()
class PESTIALVILAG_API UFightSound : public USynthComponent
{
	GENERATED_BODY()

public:
	UFightSound(const FObjectInitializer& ObjectInitializer);

	/** Starts a sound, Strength 0..1, after Delay seconds. */
	void Queue(FightSynth::EKind Kind, float Strength, float Delay = 0.f)
	{
		FAsked& Ask = Asked[static_cast<int32>(Kind)];
		Ask.Strength.store(Strength);
		Ask.Delay.store(Delay);
		Ask.Count.fetch_add(1);
	}

	/** 0..1: how out of breath he is (his panting). */
	void SetBreathless(float InBreathless) { Breathless.store(InBreathless); }
	/** His voice (Hz). */
	void SetPitch(float InPitch) { Pitch.store(InPitch); }

protected:
	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

private:
	/** The sounds asked for of one kind since the sound thread last looked. */
	struct FAsked
	{
		std::atomic<int32> Count { 0 };
		std::atomic<float> Strength { 0.f };
		std::atomic<float> Delay { 0.f };
	};

	FAsked Asked[static_cast<int32>(FightSynth::EKind::Count)];
	std::atomic<float> Breathless { 0.f };
	std::atomic<float> Pitch { 110.f };

	// Sound thread only.
	float SampleRateHz = 48000.f;
	FightSynth::FVoice Voice;
};
