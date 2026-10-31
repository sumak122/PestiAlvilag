// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * The sounds of a fist fight, made sample by sample (no sound files): the
 * whoosh of a swing, the smack of a fist landing, the duller knock of one
 * caught on a forearm, the crash of a man thrown into a wall or a stall, the
 * thud of a body hitting the ground, a grab at a collar, and the man's own
 * voice: a grunt as he puts his weight into a blow, a cry when one lands on
 * him, and his panting when he is out of breath.
 *
 * Plain C++ with no engine objects, so UFightSound can run it on the sound
 * thread and the same code can be tried outside the game.
 */
namespace FightSynth
{
	enum class EKind : uint8 { Swish, Smack, Block, Slam, Thud, Grab, Effort, Pain, Count };

	// How long each sound lasts at most (s), after its delay.
	constexpr float Lasts[static_cast<int32>(EKind::Count)] = { 0.25f, 0.2f, 0.15f, 0.6f, 0.4f, 0.16f, 0.26f, 0.38f };
	// At most this many sounds at once; a new one past it takes the oldest's place.
	constexpr int32 MaxSounds = 10;
	// The voice's formants (Hz): a grunt is an "uh", a cry an "ah".
	constexpr float EffortF1 = 560.f;
	constexpr float EffortF2 = 1050.f;
	constexpr float PainF1 = 760.f;
	constexpr float PainF2 = 1250.f;
	// Panting: breaths a second at full breathlessness.
	constexpr float PantRate = 1.6f;
	constexpr float Volume = 0.6f;

	/** A two-pole resonator: rings at Hz, as wide as Width (Hz). */
	struct FResonator
	{
		float Y1 = 0.f;
		float Y2 = 0.f;

		float Run(float X, float Hz, float Width, float SampleRate)
		{
			const float R = FMath::Exp(-PI * Width / SampleRate);
			const float A1 = -2.f * R * FMath::Cos(2.f * PI * Hz / SampleRate);
			const float A2 = R * R;
			const float Y = (1.f - R) * X - A1 * Y1 - A2 * Y2;
			Y2 = Y1;
			Y1 = Y;
			return Y;
		}
	};

	/** One sound playing. */
	struct FSound
	{
		EKind Kind = EKind::Count;
		float Strength = 0.f;
		float Delay = 0.f;
		float Age = 0.f;
		float Phase = 0.f;
		float Low = 0.f;
		float Low2 = 0.f;
		FResonator Band;
		FResonator Form1;
		FResonator Form2;
		/** For a slam: when the next loose thing rattles (s). */
		float NextRattle = 0.f;
		float Rattle = 0.f;

		bool IsDone() const { return Kind == EKind::Count || Age > Lasts[static_cast<int32>(Kind)]; }
	};

	struct FVoice
	{
		/** The man's voice: the pitch of his grunts (Hz); about 110 for a grown man. */
		float Pitch = 110.f;

		/** Starts a sound, Strength 0..1 (more for a big one), after Delay seconds. */
		void Play(EKind Kind, float Strength, float Delay = 0.f)
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
			Fresh.NextRattle = 0.03f;
			Sounds[Slot] = Fresh;
		}

		/** Breathless 0..1: panting, louder the more he is out of breath. */
		void Generate(float* Out, int32 NumSamples, float Breathless, float SampleRate)
		{
			const float Dt = 1.f / SampleRate;
			const float Follow = 1.f - FMath::Exp(-Dt / 0.3f);
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
					Sample += Run(Sound, Dt, SampleRate);
					Sound.Age += Dt;
				}

				// Panting: in and out, the out-breath louder and a little voiced.
				Breath += (Breathless - Breath) * Follow;
				if (Breath > 0.01f)
				{
					PantPhase = Wrap(PantPhase + PantRate * (0.7f + 0.5f * Breath) * Dt);
					const bool bOut = PantPhase < 0.45f;
					const float Within = bOut ? PantPhase / 0.45f : (PantPhase - 0.45f) / 0.55f;
					const float Shape = FMath::Sin(PI * Within);
					const float Air = PantForm.Run(NextNoise(), bOut ? 1100.f : 1500.f, 900.f, SampleRate);
					GlottalPhase = Wrap(GlottalPhase + Pitch * 0.9f * Dt);
					const float Hum = PantVoice.Run(Saw(GlottalPhase), 520.f, 200.f, SampleRate);
					Sample += Breath * Shape * Shape * (bOut ? 0.55f * Air + 0.08f * Hum : 0.32f * Air);
				}

				Sample *= Volume;
				// Soft limit, so nothing clips harshly.
				Out[i] = Sample / (1.f + FMath::Abs(Sample));
			}
		}

	private:
		static float Wrap(float Phase) { return Phase >= 1.f ? Phase - 1.f : Phase; }
		static float Sine(float Phase) { return FMath::Sin(2.f * PI * Phase); }
		static float Saw(float Phase) { return 2.f * Phase - 1.f; }
		static float Decay(float Age, float Time) { return FMath::Exp(-Age / Time); }

		float NextNoise()
		{
			// A tiny random number maker, safe on the sound thread.
			Seed = Seed * 1664525u + 1013904223u;
			return static_cast<float>(Seed >> 8) / static_cast<float>(1 << 24) * 2.f - 1.f;
		}

		/** A low boom falling from High to Low Hz over Fall seconds, dying away over Time. */
		float Boom(FSound& Sound, float Dt, float High, float Low, float Fall, float Time)
		{
			const float Hz = Low + (High - Low) * Decay(Sound.Age, Fall);
			Sound.Phase = Wrap(Sound.Phase + Hz * Dt);
			return Sine(Sound.Phase) * Decay(Sound.Age, Time);
		}

		/** Noise through a smoothing filter at Hz: a dull rumble when low, a hiss when high. */
		float Muffled(FSound& Sound, float Dt, float Hz)
		{
			const float Smooth = 1.f - FMath::Exp(-2.f * PI * Hz * Dt);
			Sound.Low += (NextNoise() - Sound.Low) * Smooth;
			Sound.Low2 += (Sound.Low - Sound.Low2) * Smooth;
			return Sound.Low2 * 2.f;
		}

		/** A grunt or a cry: his voice through two formants, with breath in it. */
		float Voice(FSound& Sound, float Dt, float SampleRate, float From, float To, float F1, float F2, float Rasp)
		{
			const float Length = Lasts[static_cast<int32>(Sound.Kind)];
			const float T = FMath::Clamp(Sound.Age / Length, 0.f, 1.f);
			const float Hz = Pitch * (From + (To - From) * T) * (1.f + 0.03f * NextNoise() * Rasp);
			Sound.Phase = Wrap(Sound.Phase + Hz * Dt);
			const float Source = Saw(Sound.Phase) + Rasp * NextNoise();
			const float Shaped = Sound.Form1.Run(Source, F1, 110.f, SampleRate) + 0.6f * Sound.Form2.Run(Source, F2, 140.f, SampleRate);
			// In quickly, out over the last half.
			const float Envelope = FMath::Min(1.f, Sound.Age / 0.02f) * FMath::Clamp((1.f - T) * 2.f, 0.f, 1.f);
			return Shaped * Envelope * 2.2f;
		}

		float Run(FSound& Sound, float Dt, float SampleRate)
		{
			const float S = Sound.Strength;
			switch (Sound.Kind)
			{
			case EKind::Swish:
			{
				// Air cut by a fist: a hiss rising in pitch, swelling and fading.
				const float Length = Lasts[static_cast<int32>(EKind::Swish)] * (0.6f + 0.4f * FMath::Min(S, 1.f));
				const float T = FMath::Clamp(Sound.Age / Length, 0.f, 1.f);
				const float Hiss = Sound.Band.Run(NextNoise(), 500.f + 1900.f * T, 700.f, SampleRate);
				return Hiss * FMath::Sin(PI * T) * 1.6f * S;
			}
			case EKind::Smack:
				// Knuckles on a face or a coat: a sharp slap and a low thump under it.
				return S * (Sound.Band.Run(NextNoise(), 1700.f, 1400.f, SampleRate) * 5.f * Decay(Sound.Age, 0.016f)
					+ 0.9f * Boom(Sound, Dt, 150.f, 55.f, 0.03f, 0.06f));
			case EKind::Block:
				// Caught on a forearm: duller, shorter, with a rustle of sleeve.
				return S * (0.7f * Muffled(Sound, Dt, 650.f) * Decay(Sound.Age, 0.03f)
					+ 0.45f * Boom(Sound, Dt, 210.f, 120.f, 0.02f, 0.04f)
					+ 0.25f * Sound.Band.Run(NextNoise(), 3200.f, 2500.f, SampleRate) * 4.f * Decay(Sound.Age, 0.01f));
			case EKind::Slam:
			{
				// Thrown into a wall or a stall: a big boom, a crash, and loose things rattling after.
				float Crash = 1.2f * Boom(Sound, Dt, 90.f, 42.f, 0.05f, 0.2f) + 0.9f * Muffled(Sound, Dt, 420.f) * Decay(Sound.Age, 0.12f);
				if (Sound.Age >= Sound.NextRattle && Sound.Age < 0.45f)
				{
					Sound.Rattle = 0.4f + 0.6f * FMath::Abs(NextNoise());
					Sound.NextRattle = Sound.Age + 0.03f + 0.07f * FMath::Abs(NextNoise());
				}
				Sound.Rattle *= FMath::Exp(-Dt / 0.008f);
				Crash += Sound.Rattle * Sound.Band.Run(NextNoise(), 1300.f, 1600.f, SampleRate) * 3.f;
				return S * Crash;
			}
			case EKind::Thud:
				// A body hitting the ground.
				return S * (Boom(Sound, Dt, 95.f, 50.f, 0.04f, 0.13f) + 0.6f * Muffled(Sound, Dt, 300.f) * Decay(Sound.Age, 0.08f));
			case EKind::Grab:
			{
				// Fists closing on cloth: a short rustle.
				const float T = Sound.Age / Lasts[static_cast<int32>(EKind::Grab)];
				const float Envelope = FMath::Min(1.f, Sound.Age / 0.012f) * FMath::Clamp(1.f - T, 0.f, 1.f);
				return S * Sound.Band.Run(NextNoise(), 2600.f, 2200.f, SampleRate) * 2.2f * Envelope;
			}
			case EKind::Effort:
				return S * Voice(Sound, Dt, SampleRate, 1.08f, 0.92f, EffortF1, EffortF2, 0.35f);
			case EKind::Pain:
				return S * Voice(Sound, Dt, SampleRate, 1.4f, 1.05f, PainF1, PainF2, 0.55f);
			default:
				return 0.f;
			}
		}

		FSound Sounds[MaxSounds];
		float Breath = 0.f;
		float PantPhase = 0.f;
		float GlottalPhase = 0.f;
		FResonator PantForm;
		FResonator PantVoice;
		uint32 Seed = 1932u;
	};
}
