// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * The sound of an old two-axle tram, made sample by sample (no sound files):
 * the whine of the motors through their gears, the rumble of steel wheels on
 * the rails, a "ta-dum" as both axles roll over each rail joint, a squeal
 * from the wheel flanges in tight curves, the grind of the brake shoes, and
 * the driver's foot bell, rung twice.
 *
 * Plain C++ with no engine objects, so UTramSound can run it on the sound
 * thread and the same code can be tried outside the game.
 */
namespace TramSynth
{
	// Cruising speed is 1 (about 25 km/h, 7 m/s).
	constexpr float CruiseSpeed = 700.f;
	// Rails come in lengths of this many cm, and the two axles are this far apart.
	constexpr float RailLength = 1200.f;
	constexpr float Wheelbase = 300.f;
	// The motor whine at standstill and at cruising speed (Hz).
	constexpr float WhineLow = 70.f;
	constexpr float WhineHigh = 560.f;
	// The foot bell's partials (against its note), how loud each is and how long it rings (s).
	constexpr float BellNote = 1180.f;
	constexpr int32 BellPartials = 5;
	const float BellRatio[BellPartials] = { 1.f, 2.02f, 2.76f, 5.4f, 8.93f };
	const float BellLoud[BellPartials] = { 1.f, 0.55f, 0.45f, 0.22f, 0.1f };
	const float BellRing[BellPartials] = { 1.1f, 0.75f, 0.55f, 0.3f, 0.18f };
	// The two strikes of a "ding-ding" are this far apart (s).
	constexpr float BellGap = 0.26f;
	// The flange squeal (Hz): a main tone and a harsher one above.
	constexpr float SquealNote = 2350.f;
	constexpr float SquealNote2 = 3620.f;
	constexpr float Volume = 0.5f;

	/** What the tram is doing; set from the game. All 0..1 except Speed (1 is cruising). */
	struct FState
	{
		float Speed = 0.f;
		/** Drawing power to speed up. */
		float Pull = 0.f;
		/** Braking. */
		float Brake = 0.f;
		/** How tight the curve under it is. */
		float Curve = 0.f;
	};

	struct FVoice
	{
		/** Rings the bell twice ("ding-ding"). */
		void Ring() { BellStrikesLeft = 2; BellWait = 0.f; }

		void Generate(float* Out, int32 NumSamples, const FState& Wanted, float SampleRate)
		{
			const float Dt = 1.f / SampleRate;
			// Everything eases towards what the game asks over a fraction of a second.
			const float Follow = 1.f - FMath::Exp(-Dt / 0.2f);
			const float ClackDecay = FMath::Exp(-Dt / 0.035f);
			const float ClickDecay = FMath::Exp(-Dt / 0.004f);
			float BellDecay[BellPartials];
			for (int32 p = 0; p < BellPartials; ++p)
			{
				BellDecay[p] = FMath::Exp(-Dt / BellRing[p]);
			}

			for (int32 i = 0; i < NumSamples; ++i)
			{
				Now.Speed += (Wanted.Speed - Now.Speed) * Follow;
				Now.Pull += (Wanted.Pull - Now.Pull) * Follow;
				Now.Brake += (Wanted.Brake - Now.Brake) * Follow;
				Now.Curve += (Wanted.Curve - Now.Curve) * Follow;
				const float Speed = FMath::Max(0.f, Now.Speed);
				const float Moving = FMath::Clamp(Speed * 4.f, 0.f, 1.f);

				// Motors: a gear whine rising with speed, louder while pulling or braking on them.
				const float Whine = WhineLow + (WhineHigh - WhineLow) * FMath::Min(Speed, 1.3f);
				WhinePhase = Wrap(WhinePhase + Whine * Dt);
				GearPhase = Wrap(GearPhase + Whine * 2.97f * Dt);
				const float MotorLoud = Moving * (0.035f + 0.11f * Now.Pull + 0.05f * Now.Brake);
				float Sample = MotorLoud * (Sine(WhinePhase) + 0.35f * Sine(GearPhase));

				// Wheels on the rails: a low rumble, rougher with speed.
				const float Noise = NextNoise();
				const float RumbleCut = 90.f + 380.f * Speed;
				const float RumbleSmooth = 1.f - FMath::Exp(-2.f * PI * RumbleCut * Dt);
				Rumble += (Noise - Rumble) * RumbleSmooth;
				Rumble2 += (Rumble - Rumble2) * RumbleSmooth;
				Sample += Rumble2 * 0.9f * FMath::Min(Speed, 1.3f);

				// Rail joints: the front axle, then the back one a wheelbase later.
				const float Travel = Speed * CruiseSpeed * Dt;
				ToJoint -= Travel;
				if (ToJoint <= 0.f)
				{
					ToJoint += RailLength;
					const float Strength = 0.75f + 0.25f * NextNoise();
					Strike(Strength);
					BackAxleLeft = Wheelbase;
					BackAxleStrength = Strength * 0.85f;
				}
				if (BackAxleLeft > 0.f)
				{
					BackAxleLeft -= Travel;
					if (BackAxleLeft <= 0.f)
					{
						Strike(BackAxleStrength);
					}
				}
				Clack *= ClackDecay;
				Click *= ClickDecay;
				ClackPhase = Wrap(ClackPhase + 150.f * Dt);
				Sample += FMath::Min(Speed, 1.2f) * (Clack * 0.45f * Sine(ClackPhase) + Click * 0.5f * Noise);

				// Flanges squealing in a tight curve, wavering.
				const float Squeal = FMath::Clamp(Now.Curve * 1.4f - 0.3f, 0.f, 1.f) * FMath::Clamp(Speed * 3.f - 0.3f, 0.f, 1.f);
				Waver += (NextNoise() - Waver) * (1.f - FMath::Exp(-Dt / 0.15f));
				SquealPhase = Wrap(SquealPhase + SquealNote * (1.f + 0.03f * Waver) * Dt);
				SquealPhase2 = Wrap(SquealPhase2 + SquealNote2 * (1.f - 0.02f * Waver) * Dt);
				Sample += Squeal * (0.6f + 0.4f * Waver) * (0.06f * Sine(SquealPhase) + 0.025f * Sine(SquealPhase2));

				// Brake shoes on the wheels: a gritty hiss while slowing.
				const float Grit = NextNoise();
				GritLow += (Grit - GritLow) * (1.f - FMath::Exp(-2.f * PI * 1600.f * Dt));
				GritBand += (GritLow - GritBand) * (1.f - FMath::Exp(-2.f * PI * 500.f * Dt));
				Sample += (GritLow - GritBand) * 0.35f * Now.Brake * Moving;

				// The bell.
				if (BellStrikesLeft > 0)
				{
					BellWait -= Dt;
					if (BellWait <= 0.f)
					{
						for (int32 p = 0; p < BellPartials; ++p)
						{
							BellAmp[p] = FMath::Min(1.5f, BellAmp[p] + BellLoud[p]);
						}
						--BellStrikesLeft;
						BellWait = BellGap;
					}
				}
				for (int32 p = 0; p < BellPartials; ++p)
				{
					BellAmp[p] *= BellDecay[p];
					BellPhase[p] = Wrap(BellPhase[p] + BellNote * BellRatio[p] * Dt);
					Sample += 0.11f * BellAmp[p] * Sine(BellPhase[p]);
				}

				Sample *= Volume;
				// Soft limit, so nothing clips harshly.
				Out[i] = Sample / (1.f + FMath::Abs(Sample));
			}
		}

	private:
		static float Wrap(float Phase) { return Phase >= 1.f ? Phase - 1.f : Phase; }
		static float Sine(float Phase) { return FMath::Sin(2.f * PI * Phase); }

		float NextNoise()
		{
			// A tiny random number maker, safe on the sound thread.
			Seed = Seed * 1664525u + 1013904223u;
			return static_cast<float>(Seed >> 8) / static_cast<float>(1 << 24) * 2.f - 1.f;
		}

		/** An axle's wheels over a joint. */
		void Strike(float Strength)
		{
			Clack = FMath::Min(1.5f, Clack + Strength);
			Click = FMath::Min(1.5f, Click + Strength);
		}

		FState Now;
		float WhinePhase = 0.f;
		float GearPhase = 0.f;
		float Rumble = 0.f;
		float Rumble2 = 0.f;
		float ToJoint = RailLength * 0.5f;
		float BackAxleLeft = 0.f;
		float BackAxleStrength = 0.f;
		float Clack = 0.f;
		float Click = 0.f;
		float ClackPhase = 0.f;
		float Waver = 0.f;
		float SquealPhase = 0.f;
		float SquealPhase2 = 0.f;
		float GritLow = 0.f;
		float GritBand = 0.f;
		int32 BellStrikesLeft = 0;
		float BellWait = 0.f;
		float BellAmp[BellPartials] = {};
		float BellPhase[BellPartials] = {};
		uint32 Seed = 1896u;
	};
}
