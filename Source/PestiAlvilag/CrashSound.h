// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "CrashSynth.h"
#include <atomic>
#include "CrashSound.generated.h"

/**
 * A car's crash sounds (CrashSynth), played from the car: bumps, the crunch
 * of a hard knock, breaking glass, a part clanging on the setts, the grind
 * of a wing scraping along a wall and the tyres skidding. The game asks for a sound with Queue and
 * sets the scrape with SetScrape; the sound thread picks them up, so the two
 * only meet through the atomic values below.
 */
UCLASS()
class PESTIALVILAG_API UCrashSound : public USynthComponent
{
	GENERATED_BODY()

public:
	UCrashSound(const FObjectInitializer& ObjectInitializer);

	/** Starts a sound, Strength 0..1, after Delay seconds. */
	void Queue(CrashSynth::EKind Kind, float Strength, float Delay = 0.f)
	{
		FAsked& Ask = Asked[static_cast<int32>(Kind)];
		Ask.Strength.store(Strength);
		Ask.Delay.store(Delay);
		Ask.Count.fetch_add(1);
	}

	/** 0..1: how hard a wing is scraping along a wall just now (0 when it isn't). */
	void SetScrape(float InScrape) { Scrape.store(InScrape); }

	/** 0..1: how hard the tyres are skidding just now; Wet 0..1 turns the squeal into a hiss. */
	void SetSkid(float InSkid, float InWet)
	{
		Skid.store(InSkid);
		Wet.store(InWet);
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
		std::atomic<float> Delay { 0.f };
	};

	FAsked Asked[static_cast<int32>(CrashSynth::EKind::Count)];
	std::atomic<float> Scrape { 0.f };
	std::atomic<float> Skid { 0.f };
	std::atomic<float> Wet { 0.f };

	// Sound thread only.
	float SampleRateHz = 48000.f;
	CrashSynth::FVoice Voice;
};
