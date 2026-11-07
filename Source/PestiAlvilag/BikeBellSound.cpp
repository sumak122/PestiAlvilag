// Fill out your copyright notice in the Description page of Project Settings.

#include "BikeBellSound.h"

namespace BikeBell
{
	// The dome's partials: against its note (Hz), how loud a strike makes each, how long each rings (s).
	constexpr float Note = 2350.f;
	const float Ratio[4] = { 1.f, 2.41f, 3.87f, 5.62f };
	const float Loud[4] = { 1.f, 0.6f, 0.35f, 0.18f };
	const float RingFor[4] = { 0.55f, 0.32f, 0.18f, 0.1f };
	// A trill of this many strikes this far apart (s), then a pause, then another.
	constexpr int32 TrillStrikes = 8;
	constexpr float StrikeGap = 0.032f;
	constexpr float TrillPause = 0.2f;
	constexpr float Volume = 0.22f;
}

UBikeBellSound::UBikeBellSound(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoActivate = false;
	Pitch = FMath::FRandRange(0.92f, 1.1f);
}

bool UBikeBellSound::Init(int32& SampleRate)
{
	NumChannels = 1;
	SampleRateHz = static_cast<float>(FMath::Max(SampleRate, 8000));
	return true;
}

int32 UBikeBellSound::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	using namespace BikeBell;
	if (Rings.exchange(0) > 0 && StrikesLeft == 0)
	{
		StrikesLeft = TrillStrikes * 2;
		StrikeWait = 0.f;
	}
	const float Dt = 1.f / SampleRateHz;
	float Decay[BikeBellPartials];
	for (int32 p = 0; p < BikeBellPartials; ++p)
	{
		Decay[p] = FMath::Exp(-Dt / RingFor[p]);
	}
	for (int32 i = 0; i < NumSamples; ++i)
	{
		if (StrikesLeft > 0)
		{
			StrikeWait -= Dt;
			if (StrikeWait <= 0.f)
			{
				// The striker hits the dome a little harder and softer by turns.
				const float Hit = StrikesLeft % 2 == 0 ? 1.f : 0.7f;
				for (int32 p = 0; p < BikeBellPartials; ++p)
				{
					Amp[p] = FMath::Min(1.4f, Amp[p] + Loud[p] * Hit * 0.5f);
				}
				--StrikesLeft;
				StrikeWait = StrikesLeft == TrillStrikes ? TrillPause : StrikeGap;
			}
		}
		float Sample = 0.f;
		for (int32 p = 0; p < BikeBellPartials; ++p)
		{
			Phase[p] += Note * Pitch * Ratio[p] * Dt;
			Phase[p] -= FMath::FloorToFloat(Phase[p]);
			Sample += FMath::Sin(Phase[p] * 2.f * PI) * Amp[p];
			Amp[p] *= Decay[p];
		}
		OutAudio[i] = Sample * Volume;
	}
	return NumSamples;
}
