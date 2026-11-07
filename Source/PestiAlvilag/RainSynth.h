// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FightSynth.h"

/**
 * The sound of rain in the street, made sample by sample (no sound files):
 * the steady hiss of it falling, the patter of drops on the setts and the
 * tin of the window sills, a soft roar under it when it pours, and now and
 * then a fat drip from a gutter.
 *
 * Plain C++ with no engine objects, so URainSound can run it on the sound
 * thread and the same code can be tried outside the game.
 */
namespace RainSynth
{
	using FightSynth::FResonator;

	constexpr float Volume = 0.5f;
	// Drops a second at full rain, and gutter drips a second.
	constexpr float PatterRate = 260.f;
	constexpr float DripRate = 2.f;

	struct FVoice
	{
		/** Rain 0..1: how hard it rains (0 is silent). */
		void Generate(float* Out, int32 NumSamples, float Rain, float SampleRate)
		{
			const float Dt = 1.f / SampleRate;
			const float Follow = 1.f - FMath::Exp(-Dt / 0.5f);
			const float Hiss = 1.f - FMath::Exp(-2.f * PI * 900.f * Dt);
			const float Roar = 1.f - FMath::Exp(-2.f * PI * 250.f * Dt);
			for (int32 i = 0; i < NumSamples; ++i)
			{
				Level += (Rain - Level) * Follow;
				if (Level < 0.003f)
				{
					Out[i] = 0.f;
					continue;
				}
				const float White = NextNoise();
				// The hiss: noise with its low end taken out.
				Low += (White - Low) * Hiss;
				float Sample = 0.35f * Level * (White - Low);
				// The roar of a downpour: the low end, only when it really rains.
				Deep += (White - Deep) * Roar;
				Sample += 1.2f * Level * Level * Deep;

				// Single drops on stone and tin.
				if (Rand01() < PatterRate * Level * Dt)
				{
					Tick = 0.15f + 0.45f * Rand01();
					TickHz = 1400.f + 4200.f * Rand01();
				}
				Sample += Level * Patter.Run(NextNoise() * Tick, TickHz, 500.f, SampleRate) * 3.f;
				Tick *= FMath::Exp(-Dt / 0.0015f);

				// A gutter's drip: a round "plink".
				if (Rand01() < DripRate * Level * Dt)
				{
					Drip = 0.5f + 0.5f * Rand01();
					DripHz = 800.f + 900.f * Rand01();
				}
				const float Plink = Gutter.Run(NextNoise() * Drip, DripHz, 40.f, SampleRate);
				Drip *= FMath::Exp(-Dt / 0.003f);
				Sample += Level * Plink * 6.f;

				Sample *= Volume;
				Out[i] = Sample / (1.f + FMath::Abs(Sample));
			}
		}

	private:
		float NextNoise()
		{
			// A tiny random number maker, safe on the sound thread.
			Seed = Seed * 1664525u + 1013904223u;
			return static_cast<float>(Seed >> 8) / static_cast<float>(1 << 24) * 2.f - 1.f;
		}

		float Rand01() { return 0.5f + 0.5f * NextNoise(); }

		float Level = 0.f;
		float Low = 0.f;
		float Deep = 0.f;
		float Tick = 0.f;
		float TickHz = 3000.f;
		float Drip = 0.f;
		float DripHz = 1200.f;
		FResonator Patter;
		FResonator Gutter;
		uint32 Seed = 1932u;
	};
}
