// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FightSynth.h"

/**
 * The sound of a horse and cart on the cobbles, made sample by sample (no
 * sound files): the clop of each iron-shod hoof as it lands, a hollow knock
 * with a click and a faint ring of the shoe, and under it the rumble of the
 * iron tyres over the setts, with the odd rattle of the cart, louder the
 * faster it goes; and now and then, standing, the horse's snort.
 *
 * Plain C++ with no engine objects, so UHorseSound can run it on the sound
 * thread and the same code can be tried outside the game.
 */
namespace HorseSynth
{
	using FightSynth::FResonator;

	constexpr int32 MaxClops = 6;
	constexpr float ClopLasts = 0.12f;
	constexpr float SnortLasts = 0.55f;
	constexpr float Volume = 0.22f;

	struct FClop
	{
		float Strength = 0.f;
		float Delay = 0.f;
		float Age = ClopLasts + 1.f;
		/** Each hoof sounds a little different. */
		float Tone = 1.f;
		float Phase = 0.f;
		FResonator Knock;
		FResonator Click;
		FResonator Ring;
	};

	struct FVoice
	{
		/** A hoof lands, Strength 0..1, after Delay seconds. */
		void Clop(float Strength, float Delay = 0.f)
		{
			int32 Slot = 0;
			for (int32 c = 0; c < MaxClops; ++c)
			{
				if (Clops[c].Age > ClopLasts && Clops[c].Delay <= 0.f)
				{
					Slot = c;
					break;
				}
				if (Clops[c].Age > Clops[Slot].Age)
				{
					Slot = c;
				}
			}
			FClop Fresh;
			Fresh.Strength = Strength;
			Fresh.Delay = Delay;
			Fresh.Age = 0.f;
			Fresh.Tone = 0.9f + 0.2f * (0.5f + 0.5f * NextNoise());
			Clops[Slot] = Fresh;
		}

		void Snort() { SnortAge = 0.f; }

		/** Roll: the cart's speed, 0 standing to 1 at a canter. */
		void Generate(float* Out, int32 NumSamples, float Roll, float SampleRate)
		{
			const float Dt = 1.f / SampleRate;
			const float Follow = 1.f - FMath::Exp(-Dt / 0.15f);
			for (int32 i = 0; i < NumSamples; ++i)
			{
				float Sample = 0.f;
				for (FClop& C : Clops)
				{
					if (C.Delay > 0.f)
					{
						C.Delay -= Dt;
						continue;
					}
					if (C.Age > ClopLasts)
					{
						continue;
					}
					const float A = C.Age;
					// The hollow knock of the hoof, a click of iron on stone, a faint ring of the shoe.
					const float Hit = A < 0.002f ? NextNoise() : 0.f;
					C.Phase += 120.f * C.Tone * Dt;
					C.Phase -= FMath::FloorToFloat(C.Phase);
					const float Thump = FMath::Sin(2.f * PI * C.Phase) * FMath::Exp(-A / 0.018f) * 0.5f;
					const float Knock = C.Knock.Run(Hit * 14.f, 820.f * C.Tone, 160.f, SampleRate) * 1.4f;
					const float Clack = C.Click.Run(NextNoise(), 2600.f * C.Tone, 1800.f, SampleRate) * 3.5f * FMath::Exp(-A / 0.005f);
					const float Ring = C.Ring.Run(Hit * 10.f, 3300.f * C.Tone, 50.f, SampleRate) * 0.6f;
					Sample += C.Strength * (Thump + Knock + Clack + Ring);
					C.Age += Dt;
				}

				// The iron tyres on the setts: a low rumble, and the bumps of the stones.
				Rolling += (Roll - Rolling) * Follow;
				if (Rolling > 0.01f)
				{
					const float Smooth = 1.f - FMath::Exp(-2.f * PI * 180.f * Dt);
					Low += (NextNoise() - Low) * Smooth;
					Low2 += (Low - Low2) * Smooth;
					Sample += Low2 * 1.8f * Rolling;
					Until -= Dt;
					if (Until <= 0.f)
					{
						// The faster, the more stones a second under the four wheels.
						Bump = 0.3f + 0.7f * FMath::Abs(NextNoise());
						Until = (0.02f + 0.08f * FMath::Abs(NextNoise())) / (0.3f + Rolling);
					}
					Bump *= FMath::Exp(-Dt / 0.01f);
					Sample += Rolling * Bump * Rattle.Run(NextNoise(), 900.f, 900.f, SampleRate) * 1.4f;
				}

				// A snort: a burst of breath through the nose, rough and fluttering.
				if (SnortAge < SnortLasts)
				{
					const float T = SnortAge / SnortLasts;
					const float Flutter = 0.6f + 0.4f * FMath::Sin(2.f * PI * 28.f * SnortAge);
					const float Envelope = FMath::Min(1.f, SnortAge / 0.03f) * (1.f - T) * (1.f - T);
					Sample += Breath.Run(NextNoise(), 700.f - 250.f * T, 600.f, SampleRate) * 3.f * Envelope * Flutter;
					SnortAge += Dt;
				}

				Sample *= Volume;
				Out[i] = Sample / (1.f + FMath::Abs(Sample));
			}
		}

	private:
		float NextNoise()
		{
			Seed = Seed * 1664525u + 1013904223u;
			return static_cast<float>(Seed >> 8) / static_cast<float>(1 << 24) * 2.f - 1.f;
		}

		FClop Clops[MaxClops];
		float Rolling = 0.f;
		float Low = 0.f;
		float Low2 = 0.f;
		float Bump = 0.f;
		float Until = 0.f;
		FResonator Rattle;
		FResonator Breath;
		float SnortAge = SnortLasts + 1.f;
		uint32 Seed = 1896u;
	};
}
