// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FightSynth.h"

/**
 * The street animals' voices, made sample by sample (no sound files): a dog's
 * bark (a rough buzz through the shape of an open mouth, falling in pitch),
 * its low growl, a cat's meow (rising and falling, "mi-aa-ow") and a
 * frightened cat's hiss.
 *
 * Plain C++ with no engine objects, so UAnimalSound can run it on the sound
 * thread and the same code can be tried outside the game.
 */
namespace AnimalSynth
{
	using FightSynth::FResonator;

	enum class EKind : uint8 { Bark, Growl, Meow, Hiss, Count };

	// How long each sound lasts (s), after its delay.
	constexpr float Lasts[static_cast<int32>(EKind::Count)] = { 0.2f, 0.75f, 0.7f, 0.65f };
	constexpr int32 MaxSounds = 6;
	constexpr float Volume = 0.55f;

	/** One sound playing. */
	struct FSound
	{
		EKind Kind = EKind::Count;
		float Strength = 0.f;
		float Delay = 0.f;
		float Age = 0.f;
		float Phase = 0.f;
		/** 1 for a middling dog or cat; higher for a smaller one. */
		float Pitch = 1.f;
		FResonator Mouth;
		FResonator Throat;

		bool IsDone() const { return Kind == EKind::Count || Age > Lasts[static_cast<int32>(Kind)]; }
	};

	struct FVoice
	{
		/** Starts a sound, Strength 0..1, Pitch about 1, after Delay seconds. */
		void Play(EKind Kind, float Strength, float Pitch, float Delay = 0.f)
		{
			int32 Slot = 0;
			float Oldest = -1.f;
			for (int32 s = 0; s < MaxSounds; ++s)
			{
				if (Sounds[s].IsDone())
				{
					Slot = s;
					break;
				}
				if (Sounds[s].Age > Oldest)
				{
					Oldest = Sounds[s].Age;
					Slot = s;
				}
			}
			FSound Fresh;
			Fresh.Kind = Kind;
			Fresh.Strength = Strength;
			Fresh.Delay = Delay;
			Fresh.Pitch = Pitch * (0.95f + 0.1f * FMath::Abs(NextNoise()));
			Sounds[Slot] = Fresh;
		}

		void Generate(float* Out, int32 NumSamples, float SampleRate)
		{
			const float Dt = 1.f / SampleRate;
			for (int32 i = 0; i < NumSamples; ++i)
			{
				float Sample = 0.f;
				for (int32 s = 0; s < MaxSounds; ++s)
				{
					FSound& Sound = Sounds[s];
					if (Sound.IsDone())
					{
						continue;
					}
					if (Sound.Delay > 0.f)
					{
						Sound.Delay -= Dt;
						continue;
					}
					Sample += Sound.Strength * Run(Sound, Dt, SampleRate);
					Sound.Age += Dt;
				}
				Sample *= Volume;
				// Soft limit, so nothing clips harshly.
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

		/** A rough buzz at Hz: a sawtooth with a little breath in it. */
		float Buzz(FSound& Sound, float Hz, float Dt, float Breath)
		{
			Sound.Phase += Hz * Dt;
			if (Sound.Phase >= 1.f)
			{
				Sound.Phase -= 1.f;
			}
			return (2.f * Sound.Phase - 1.f) + Breath * NextNoise();
		}

		float Run(FSound& Sound, float Dt, float SampleRate)
		{
			const float Age = Sound.Age;
			const float P = Sound.Pitch;
			switch (Sound.Kind)
			{
			case EKind::Bark:
			{
				// "Vau": a sharp start, the pitch falling, the mouth closing at the end.
				const float Env = Age < 0.012f ? Age / 0.012f : FMath::Exp(-(Age - 0.012f) / 0.06f);
				const float Hz = P * (260.f + 160.f * FMath::Exp(-Age / 0.05f));
				const float Source = Buzz(Sound, Hz, Dt, 0.6f) * Env;
				const float Open = FMath::Exp(-Age / 0.12f);
				return 0.55f * Sound.Mouth.Run(Source, P * (520.f + 260.f * Open), 260.f, SampleRate)
					+ 0.6f * Sound.Throat.Run(Source, P * 1350.f, 420.f, SampleRate);
			}
			case EKind::Growl:
			{
				// A low rumble, rattling about 26 times a second.
				const float Env = FMath::Min(1.f, Age / 0.08f) * FMath::Min(1.f, (Lasts[1] - Age) / 0.15f);
				const float Rattle = 0.55f + 0.45f * FMath::Sin(2.f * PI * 26.f * Age);
				const float Source = Buzz(Sound, P * 95.f, Dt, 0.9f) * Env * Rattle;
				return 0.2f * Sound.Mouth.Run(Source, P * 420.f, 300.f, SampleRate)
					+ 0.12f * Sound.Throat.Run(Source, P * 900.f, 400.f, SampleRate);
			}
			case EKind::Meow:
			{
				// "Mi-aa-ow": the pitch up then down, the mouth opening then rounding.
				const float T = FMath::Clamp(Age / Lasts[2], 0.f, 1.f);
				const float Env = FMath::Min(1.f, Age / 0.06f) * FMath::Min(1.f, (1.f - T) / 0.3f);
				const float Rise = FMath::Sin(PI * FMath::Min(1.f, T * 1.6f));
				const float Hz = P * (480.f + 280.f * Rise - 60.f * T) * (1.f + 0.01f * FMath::Sin(2.f * PI * 6.f * Age));
				const float Source = Buzz(Sound, Hz, Dt, 0.15f) * Env;
				const float First = P * (500.f + 450.f * Rise);
				const float Second = P * (2200.f - 1100.f * T);
				return 0.22f * Sound.Mouth.Run(Source, First, 180.f, SampleRate)
					+ 0.3f * Sound.Throat.Run(Source, Second, 260.f, SampleRate);
			}
			case EKind::Hiss:
			{
				const float Env = FMath::Min(1.f, Age / 0.04f) * FMath::Min(1.f, (Lasts[3] - Age) / 0.25f);
				return 0.9f * Sound.Mouth.Run(NextNoise() * Env, 4800.f, 3500.f, SampleRate);
			}
			default:
				return 0.f;
			}
		}

		FSound Sounds[MaxSounds];
		uint32 Seed = 3141u;
	};
}
