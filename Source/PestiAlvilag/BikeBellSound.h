// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include <atomic>
#include "BikeBellSound.generated.h"

/** How many partials the bell's dome rings with. */
constexpr int32 BikeBellPartials = 4;

/**
 * A bicycle bell made in code: the thumb lever spins a striker against a
 * small nickel dome, a quick trill of strikes ("rrring"), rung twice. Each
 * strike sets the dome's few out-of-tune partials ringing, the high ones
 * dying away first. Ring from the game; the sound is made on the sound thread.
 */
UCLASS()
class PESTIALVILAG_API UBikeBellSound : public USynthComponent
{
	GENERATED_BODY()

public:
	UBikeBellSound(const FObjectInitializer& ObjectInitializer);

	/** "Rrring-rrring". */
	void Ring() { Rings.fetch_add(1); }

protected:
	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

private:
	std::atomic<int32> Rings { 0 };

	// Sound thread only.
	float SampleRateHz = 48000.f;
	float Phase[BikeBellPartials] = { 0.f, 0.f, 0.f, 0.f };
	float Amp[BikeBellPartials] = { 0.f, 0.f, 0.f, 0.f };
	int32 StrikesLeft = 0;
	float StrikeWait = 0.f;
	/** Each bell is tuned a little differently. */
	float Pitch = 1.f;
};
