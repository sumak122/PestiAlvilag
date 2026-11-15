// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "BandSynth.h"
#include <atomic>
#include "BandSound.generated.h"

/**
 * One bar's band playing (BandSynth::FBand). AMusicBars says whether the
 * band is playing, how loud the listener hears it and how muffled (inside
 * the room, or through the wall by the street door); the music is worked
 * out on the sound thread, so the two only meet through the atomic values.
 */
UCLASS()
class PESTIALVILAG_API UBandSound : public USynthComponent
{
	GENERATED_BODY()

public:
	UBandSound(const FObjectInitializer& ObjectInitializer);

	void SetSmart(bool bInSmart) { bSmart.store(bInSmart); }
	void SetPlaying(bool bInPlaying) { bPlaying.store(bInPlaying); }
	/** InGain 0..1 how loud; InFar 0 in the room .. 1 heard through the wall. */
	void SetHeard(float InGain, float InFar)
	{
		Gain.store(InGain);
		Far.store(InFar);
	}

protected:
	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

private:
	std::atomic<bool> bSmart { false };
	std::atomic<bool> bPlaying { false };
	std::atomic<float> Gain { 0.f };
	std::atomic<float> Far { 0.f };

	// Sound thread only.
	float SampleRateHz = 48000.f;
	BandSynth::FBand Band;
};
