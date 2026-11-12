// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FightSynth.h"

/**
 * Footsteps, made sample by sample (no sound files), kept quiet: the heel
 * of a leather shoe coming down and the sole rolling after it. Each ground
 * sounds its own: a hard knock with a click on the cobbles, a flatter tap
 * with a scrape of grit on the pavement slabs, a crunch of little stones on
 * gravel, a soft thud and a brush of blades on grass, a dull thud on the
 * trodden earth of the markets. Wet ground adds a slap of water and a few
 * drops; running, the steps come down harder and all at once.
 *
 * Plain C++ with no engine objects, so UFootSound can run it on the sound
 * thread and the same code can be tried outside the game.
 */
namespace FootSynth
{
	using FightSynth::FResonator;

	enum class EGround : uint8 { Cobbles, Slabs, Gravel, Grass, Earth, Count };

	constexpr int32 MaxSteps = 4;
	constexpr float StepLasts = 0.22f;
	/** How far apart the heel and the sole sound at a walk (s). */
	constexpr float SoleAfter = 0.05f;
	constexpr float Volume = 0.16f;

	struct FStep
	{
		float Strength = 0.f;
		float Delay = 0.f;
		float Age = StepLasts + 1.f;
		EGround Ground = EGround::Cobbles;
		float Wet = 0.f;
		float Running = 0.f;
		/** Each step sounds a little different. */
		float Tone = 1.f;
		float Phase = 0.f;
		/** Gravel: when the next little stone gives (s). */
		float NextGrain = 0.f;
		float Grain = 0.f;
		float Drop = 0.f;
		float DropAt = 0.f;
		float DropHz = 2000.f;
		FResonator Knock;
		FResonator Click;
		FResonator Grit;
		FResonator Splash;
		FResonator Ping;
		float Low = 0.f;
	};

	struct FVoice
	{
		/** A foot comes down, Strength 0..1, on Ground, Wet 0..1, Running 0 walking to 1 sprinting, after Delay seconds. */
		void Step(float Strength, EGround OnGround, float Wet, float Running, float Delay = 0.f)
		{
			int32 Slot = 0;
			for (int32 s = 0; s < MaxSteps; ++s)
			{
				if (Playing[s].Age > StepLasts && Playing[s].Delay <= 0.f)
				{
					Slot = s;
					break;
				}
				if (Playing[s].Age > Playing[Slot].Age)
				{
					Slot = s;
				}
			}
			FStep Fresh;
			Fresh.Strength = Strength;
			Fresh.Delay = Delay;
			Fresh.Age = 0.f;
			Fresh.Ground = OnGround;
			Fresh.Wet = Wet;
			Fresh.Running = Running;
			Fresh.Tone = 0.9f + 0.2f * (0.5f + 0.5f * NextNoise());
			Fresh.DropAt = 0.03f + 0.05f * (0.5f + 0.5f * NextNoise());
			Fresh.DropHz = 1600.f + 1400.f * (0.5f + 0.5f * NextNoise());
			Playing[Slot] = Fresh;
		}

		void Generate(float* Out, int32 NumSamples, float SampleRate)
		{
			const float Dt = 1.f / SampleRate;
			for (int32 i = 0; i < NumSamples; ++i)
			{
				float Sample = 0.f;
				for (FStep& St : Playing)
				{
					if (St.Delay > 0.f)
					{
						St.Delay -= Dt;
						continue;
					}
					if (St.Age > StepLasts)
					{
						continue;
					}
					Sample += St.Strength * Sound(St, SampleRate);
					St.Age += Dt;
				}
				Sample *= Volume;
				Out[i] = Sample / (1.f + FMath::Abs(Sample));
			}
		}

	private:
		/** One sample of one step. */
		float Sound(FStep& St, float SampleRate)
		{
			const float Dt = 1.f / SampleRate;
			const float Age = St.Age;
			const float Tone = St.Tone;
			const float Dry = 1.f - 0.55f * St.Wet;
			// The heel strikes, and walking the sole slaps down a moment later, softer.
			const float SoleAge = Age - SoleAfter * (1.f - St.Running);
			float Strike = Age < 0.0015f ? NextNoise() : 0.f;
			if (St.Running < 0.6f && SoleAge >= 0.f && SoleAge < 0.0015f)
			{
				Strike += 0.55f * NextNoise();
			}
			const float Heavy = 1.f + 0.6f * St.Running;

			// The weight landing: a low thud under every ground.
			St.Phase += 80.f * Tone * Dt;
			St.Phase -= FMath::FloorToFloat(St.Phase);
			const float Thud = FMath::Sin(2.f * PI * St.Phase) * FMath::Exp(-Age / 0.02f) * 0.35f * Heavy;

			float Surface = 0.f;
			switch (St.Ground)
			{
			case EGround::Cobbles:
				// Leather on hard stone: a hollow knock and a sharp click.
				Surface = St.Knock.Run(Strike * 12.f, 1150.f * Tone, 380.f, SampleRate) * 1.3f
					+ St.Click.Run(NextNoise(), 3300.f * Tone, 2400.f, SampleRate) * 2.2f * Dry * (FMath::Exp(-Age / 0.004f) + (SoleAge > 0.f ? 0.5f * FMath::Exp(-SoleAge / 0.004f) * (1.f - St.Running) : 0.f))
					+ St.Grit.Run(NextNoise(), 2600.f, 3000.f, SampleRate) * 0.35f * FMath::Exp(-Age / 0.03f);
				break;
			case EGround::Slabs:
				// Flatter on the smooth slabs, with a scrape of the grit on them.
				Surface = St.Knock.Run(Strike * 12.f, 820.f * Tone, 450.f, SampleRate) * 1.2f
					+ St.Click.Run(NextNoise(), 2500.f * Tone, 2000.f, SampleRate) * 1.4f * Dry * FMath::Exp(-Age / 0.005f)
					+ St.Grit.Run(NextNoise(), 3800.f, 4000.f, SampleRate) * 0.7f * FMath::Exp(-FMath::Max(SoleAge, 0.f) / 0.035f) * (SoleAge > 0.f ? 1.f : 0.3f);
				break;
			case EGround::Gravel:
			{
				// Little stones giving under the foot, quick at first, then fewer.
				St.NextGrain -= Dt;
				if (St.NextGrain <= 0.f && Age < 0.16f)
				{
					St.Grain = 0.4f + 0.6f * FMath::Abs(NextNoise());
					St.NextGrain = (0.0015f + 0.006f * FMath::Abs(NextNoise())) * (1.f + Age * 25.f);
				}
				St.Grain *= FMath::Exp(-Dt / 0.0015f);
				Surface = St.Grit.Run(NextNoise() * St.Grain, 3600.f * Tone, 3200.f, SampleRate) * 3.f
					+ St.Knock.Run(Strike * 8.f, 500.f, 500.f, SampleRate) * 0.8f;
				break;
			}
			case EGround::Grass:
				// A soft thud and the brush of the blades.
				Surface = St.Grit.Run(NextNoise(), 1900.f * Tone, 2600.f, SampleRate) * 2.6f
					* FMath::Min(Age / 0.012f, 1.f) * FMath::Exp(-Age / 0.05f);
				break;
			case EGround::Earth:
			default:
				// A dull thud on trodden earth, a little grit.
				Surface = St.Knock.Run(Strike * 10.f, 420.f * Tone, 420.f, SampleRate) * 1.2f
					+ St.Grit.Run(NextNoise(), 3000.f, 3000.f, SampleRate) * 0.3f * FMath::Exp(-Age / 0.02f);
				break;
			}
			const float SoftGround = St.Ground == EGround::Grass ? 0.85f : 1.f;

			// Wet: a slap of water as the shoe comes down, and a drop or two flying off.
			float Water = 0.f;
			if (St.Wet > 0.01f)
			{
				const float Slap = FMath::Min(Age / 0.003f, 1.f) * FMath::Exp(-Age / 0.045f);
				const float Hiss = NextNoise() - St.Low;
				St.Low += (NextNoise() - St.Low) * 0.2f;
				Water = St.Splash.Run(Hiss, 1700.f + 900.f * Age / StepLasts, 2600.f, SampleRate) * 1.3f * Slap;
				if (Age >= St.DropAt && St.Drop == 0.f)
				{
					St.Drop = 1.f;
				}
				St.Drop *= FMath::Exp(-Dt / 0.012f);
				Water += St.Ping.Run(St.Drop > 0.99f ? 6.f : 0.f, St.DropHz, 60.f, SampleRate) * 0.6f;
				Water *= St.Wet * (St.Ground == EGround::Grass || St.Ground == EGround::Earth ? 0.6f : 1.f);
			}
			return (Thud * SoftGround + Surface * Heavy * SoftGround + Water) * FMath::Clamp(1.f - Age / StepLasts, 0.f, 1.f);
		}

		float NextNoise()
		{
			Seed = Seed * 1664525u + 1013904223u;
			return static_cast<float>(Seed >> 8) / static_cast<float>(1 << 24) * 2.f - 1.f;
		}

		FStep Playing[MaxSteps];
		uint32 Seed = 1932u;
	};
}
