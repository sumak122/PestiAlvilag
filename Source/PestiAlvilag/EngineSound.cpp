// Fill out your copyright notice in the Description page of Project Settings.

#include "EngineSound.h"

namespace EngineSound
{
	// Crankshaft speed idling and flat out (turns a minute).
	constexpr float IdleRpm = 650.f;
	constexpr float TopRpm = 2400.f;
	// A four-stroke four fires twice each turn.
	constexpr float FiringsPerTurn = 2.f;
	// How fast a thump dies away (s), and the pitch of its boom against the firing rate.
	constexpr float ThumpTime = 0.014f;
	constexpr float ToneRatio = 1.6f;
	constexpr float ToneBase = 45.f;
	// Each cylinder a little different, so it lopes instead of buzzing.
	const float CylinderStrength[4] = { 1.f, 0.82f, 0.95f, 0.74f };
	constexpr float BangTime = 0.06f;
	constexpr float Volume = 0.45f;
}

UEngineSound::UEngineSound(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoActivate = false;
}

bool UEngineSound::Init(int32& SampleRate)
{
	NumChannels = 1;
	SampleRateHz = static_cast<float>(FMath::Max(SampleRate, 8000));
	return true;
}

float UEngineSound::NextNoise()
{
	// A tiny random number maker, safe on the sound thread.
	Seed = Seed * 1664525u + 1013904223u;
	return static_cast<float>(Seed >> 8) / static_cast<float>(1 << 24) * 2.f - 1.f;
}

int32 UEngineSound::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	const float WantedRevs = FMath::Clamp(Revs.load(), 0.f, 1.f);
	const float WantedLoad = FMath::Clamp(Load.load(), 0.f, 1.f);
	if (Bangs.exchange(0) > 0)
	{
		Bang = 1.f;
	}

	const float ThumpDecay = FMath::Exp(-1.f / (EngineSound::ThumpTime * SampleRateHz));
	const float BangDecay = FMath::Exp(-1.f / (EngineSound::BangTime * SampleRateHz));
	// The revs follow the pedal over a fraction of a second, not at once.
	const float Follow = 1.f - FMath::Exp(-1.f / (0.15f * SampleRateHz));

	for (int32 i = 0; i < NumSamples; ++i)
	{
		SmoothRevs += (WantedRevs - SmoothRevs) * Follow;
		SmoothLoad += (WantedLoad - SmoothLoad) * Follow;
		const float Rpm = FMath::Lerp(EngineSound::IdleRpm, EngineSound::TopRpm, SmoothRevs);
		const float Firing = Rpm / 60.f * EngineSound::FiringsPerTurn;

		FiringPhase += Firing / SampleRateHz;
		if (FiringPhase >= 1.0)
		{
			FiringPhase -= 1.0;
			Cylinder = (Cylinder + 1) % 4;
			ThumpStrength = EngineSound::CylinderStrength[Cylinder] * (0.85f + 0.15f * NextNoise());
			Thump = 1.f;
		}
		Thump *= ThumpDecay;
		Bang *= BangDecay;

		const float Tone = EngineSound::ToneBase + Firing * EngineSound::ToneRatio;
		TonePhase = FMath::Fmod(TonePhase + Tone / SampleRateHz, 1.0);
		HumPhase = FMath::Fmod(HumPhase + Firing * 0.5f / SampleRateHz, 1.0);

		const float Boom = FMath::Sin(2.f * PI * static_cast<float>(TonePhase));
		const float Rough = NextNoise() * (0.35f + 0.5f * SmoothLoad);
		float Sample = Thump * ThumpStrength * (Boom + Rough)
			+ 0.18f * FMath::Sin(2.f * PI * static_cast<float>(HumPhase))
			+ Bang * NextNoise() * 2.5f;

		// Muffle it: more muffled when idling, brighter under load.
		const float Cutoff = 220.f + 600.f * SmoothLoad + 300.f * SmoothRevs;
		const float Smooth = 1.f - FMath::Exp(-2.f * PI * Cutoff / SampleRateHz);
		Filtered += (Sample - Filtered) * Smooth;
		Sample = Filtered * EngineSound::Volume * (0.6f + 0.4f * SmoothLoad) * (1.f - 0.25f * SmoothRevs);

		// Soft limit, so a backfire can't clip harshly.
		OutAudio[i] = Sample / (1.f + FMath::Abs(Sample));
	}
	return NumSamples;
}
