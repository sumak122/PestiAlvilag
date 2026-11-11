// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * The sounds of a car crash, made sample by sample (no sound files): the
 * dull thump of a small bump, the crunch of steel bending in a hard knock
 * (a low boom, the crumple, the panels ringing after), glass bursting and
 * tinkling down, a bumper or a lamp clanging and bouncing on the setts, and
 * the long grind of a wing scraping along a wall.
 *
 * Plain C++ with no engine objects, so UCrashSound can run it on the sound
 * thread and the same code can be tried outside the game.
 */
namespace CrashSynth
{
	enum class EKind : uint8 { Bump, Crunch, Glass, Clang, Count };

	// How long each sound lasts at most (s), after its delay.
	constexpr float Lasts[static_cast<int32>(EKind::Count)] = { 0.35f, 1.1f, 0.9f, 0.9f };
	// At most this many sounds at once; a new one past it takes the oldest's place.
	constexpr int32 MaxSounds = 8;
	// The steel panels ring at these (Hz), a little different on every car.
	constexpr float PanelHz[3] = { 395.f, 1120.f, 2310.f };
	// A bumper or lamp on the setts rings at these (Hz) and bounces this many times.
	constexpr float ClangHz[2] = { 610.f, 1630.f };
	constexpr int32 Bounces = 3;
	constexpr float Volume = 0.7f;

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
		/** This car's own tuning, about 1. */
		float Tune = 1.f;
		FResonator Band;
		FResonator Ring[3];
		/** Crumple clicks and glass pings: when the next one comes (s), how loud it is now, and where it rings (Hz). */
		float NextTick = 0.f;
		float Tick = 0.f;
		float TickHz = 3000.f;
		int32 Bounce = 0;

		bool IsDone() const { return Kind == EKind::Count || Age > Lasts[static_cast<int32>(Kind)]; }
	};

	struct FVoice
	{
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
			Fresh.Tune = 0.9f + 0.2f * FMath::Abs(NextNoise());
			Fresh.NextTick = 0.f;
			Sounds[Slot] = Fresh;
		}

		/**
		 * Scrape 0..1: a wing grinding along a wall, louder and brighter the faster it goes.
		 * Skid 0..1: the tyres sliding; on dry setts a squeal that chatters over the
		 * stones, on wet ones (Wet 0..1) more of a hiss.
		 */
		void Generate(float* Out, int32 NumSamples, float Scrape, float SampleRate, float Skid = 0.f, float Wet = 0.f)
		{
			const float Dt = 1.f / SampleRate;
			const float Follow = 1.f - FMath::Exp(-Dt / 0.05f);
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

				// The scrape: gritty noise through a band that rises with speed, and the panel squealing in it.
				Grind += (Scrape - Grind) * Follow;
				if (Grind > 0.01f)
				{
					GritTimer -= Dt;
					if (GritTimer <= 0.f)
					{
						Grit = 0.4f + 0.6f * FMath::Abs(NextNoise());
						GritTimer = 0.004f + 0.012f * FMath::Abs(NextNoise());
					}
					const float Hz = 700.f + 1500.f * Grind;
					const float Rasp = ScrapeBand.Run(NextNoise(), Hz, 900.f, SampleRate) * 3.f * Grit;
					const float Squeal = ScrapeRing.Run(NextNoise() * Grit, 1850.f + 300.f * Grind, 25.f, SampleRate) * 6.f;
					Sample += 0.7f * Grind * (Rasp + 0.35f * Grind * Squeal);
				}

				// The skid: a rubber squeal wavering in pitch, chattering as the
				// tyres hop from sett to sett, with a hiss under it that takes over when wet.
				Slide += (Skid - Slide) * Follow;
				if (Slide > 0.01f)
				{
					SkidWobble = Wrap(SkidWobble + Dt * (5.5f + 3.f * Slide));
					ChatterTimer -= Dt;
					if (ChatterTimer <= 0.f)
					{
						Chatter = 0.55f + 0.45f * FMath::Abs(NextNoise());
						ChatterTimer = 0.018f + 0.02f * FMath::Abs(NextNoise());
					}
					const float Hz = 950.f + 250.f * Slide + 60.f * Sine(SkidWobble);
					const float Squeal = SkidRing.Run(NextNoise(), Hz, 18.f, SampleRate) * 9.f * Chatter;
					const float Hiss = SkidHiss.Run(NextNoise(), 3200.f, 2500.f, SampleRate) * 1.6f;
					Sample += 0.45f * Slide * ((1.f - 0.85f * Wet) * Squeal + (0.25f + 0.75f * Wet) * Hiss);
				}

				Sample *= Volume;
				// Soft limit, so nothing clips harshly.
				Out[i] = Sample / (1.f + FMath::Abs(Sample));
			}
		}

	private:
		static float Wrap(float Phase) { return Phase >= 1.f ? Phase - 1.f : Phase; }
		static float Sine(float Phase) { return FMath::Sin(2.f * PI * Phase); }
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

		float Run(FSound& Sound, float Dt, float SampleRate)
		{
			const float S = Sound.Strength;
			switch (Sound.Kind)
			{
			case EKind::Bump:
				// A small knock: a dull thump and a short rattle of the body on its springs.
				return S * (1.1f * Boom(Sound, Dt, 120.f, 60.f, 0.03f, 0.08f)
					+ 0.5f * Muffled(Sound, Dt, 500.f) * Decay(Sound.Age, 0.05f));
			case EKind::Crunch:
			{
				// A hard knock: a big boom, the crumple (a crackle of clicks), then the panels ringing on.
				float Crash = 1.4f * Boom(Sound, Dt, 85.f, 36.f, 0.06f, 0.22f)
					+ 1.1f * Muffled(Sound, Dt, 900.f) * Decay(Sound.Age, 0.09f);
				if (Sound.Age >= Sound.NextTick && Sound.Age < 0.3f)
				{
					Sound.Tick = 0.5f + 0.5f * FMath::Abs(NextNoise());
					Sound.NextTick = Sound.Age + 0.006f + 0.025f * FMath::Abs(NextNoise());
				}
				Sound.Tick *= FMath::Exp(-Dt / 0.004f);
				Crash += Sound.Tick * Sound.Band.Run(NextNoise(), 2100.f, 1800.f, SampleRate) * 3.5f;
				// The struck panels: excited by the first moments of noise, ringing a while after.
				const float Strike = NextNoise() * Decay(Sound.Age, 0.03f);
				float Panels = 0.f;
				for (int32 r = 0; r < 3; ++r)
				{
					Panels += Sound.Ring[r].Run(Strike, PanelHz[r] * Sound.Tune, 6.f + 4.f * r, SampleRate);
				}
				Crash += Panels * 9.f * Decay(Sound.Age, 0.35f);
				return S * Crash;
			}
			case EKind::Glass:
			{
				// Glass bursting (a bright hiss), then the pieces tinkling down, fewer and fainter.
				float Shatter = Sound.Band.Run(NextNoise(), 5200.f, 3500.f, SampleRate) * 2.5f * Decay(Sound.Age, 0.05f);
				if (Sound.Age >= Sound.NextTick && Sound.Age < 0.8f)
				{
					Sound.Tick = (0.3f + 0.7f * FMath::Abs(NextNoise())) * Decay(Sound.Age, 0.35f);
					Sound.TickHz = 3200.f + 4200.f * FMath::Abs(NextNoise());
					Sound.NextTick = Sound.Age + 0.008f + 0.06f * FMath::Abs(NextNoise()) * (1.f + 4.f * Sound.Age);
				}
				const float Ping = Sound.Ring[0].Run(NextNoise() * Sound.Tick, Sound.TickHz, 60.f, SampleRate);
				Sound.Tick *= FMath::Exp(-Dt / 0.002f);
				Shatter += Ping * 14.f;
				return S * Shatter;
			}
			case EKind::Clang:
			{
				// A bumper or lamp hitting the setts and bouncing, each bounce smaller and sooner.
				if (Sound.Age >= Sound.NextTick && Sound.Bounce < Bounces)
				{
					Sound.Tick = FMath::Pow(0.45f, static_cast<float>(Sound.Bounce));
					Sound.NextTick = Sound.Age + 0.22f * FMath::Pow(0.6f, static_cast<float>(Sound.Bounce));
					++Sound.Bounce;
				}
				const float Strike = NextNoise() * Sound.Tick;
				Sound.Tick *= FMath::Exp(-Dt / 0.003f);
				const float Ring = Sound.Ring[0].Run(Strike, ClangHz[0] * Sound.Tune, 9.f, SampleRate)
					+ 0.7f * Sound.Ring[1].Run(Strike, ClangHz[1] * Sound.Tune, 14.f, SampleRate);
				return S * (Ring * 16.f + 0.6f * Strike);
			}
			default:
				return 0.f;
			}
		}

		FSound Sounds[MaxSounds];
		float Grind = 0.f;
		float Grit = 0.f;
		float GritTimer = 0.f;
		FResonator ScrapeBand;
		FResonator ScrapeRing;
		float Slide = 0.f;
		float SkidWobble = 0.f;
		float Chatter = 0.f;
		float ChatterTimer = 0.f;
		FResonator SkidRing;
		FResonator SkidHiss;
		uint32 Seed = 1932u;
	};
}
