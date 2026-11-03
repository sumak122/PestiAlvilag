// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FightSynth.h"

/**
 * The sound of the city all around the listener, made sample by sample (no
 * sound files), so the streets are never dead quiet:
 *
 * - a bed that is always there: the low hum of a distant city, the murmur of
 *   a crowd near markets and busy streets by day, wind gusts and, near the
 *   park, the rustle of leaves;
 * - and now and then something far off: the church bell striking the hour
 *   (a long peal at noon), steam engines whistling, puffing and clanking at
 *   Keleti, dogs, bulb horns, a carpet beaten in a courtyard, a smith's
 *   hammer, sparrows, crickets in the park at night, a far tram bell and the
 *   squeal of its wheels in a curve, a man whistling a tune.
 *
 * Plain C++ with no engine objects, so UCityAmbienceSound can run it on the
 * sound thread and the same code can be tried outside the game. Stereo out.
 */
namespace CitySynth
{
	using FightSynth::FResonator;

	/** Where the listener is, set by the game every frame. Nearness 0..1, pan -1 left .. 1 right. */
	struct FScene
	{
		float Hour = 12.f;
		/** 0 day .. 1 night. */
		float Darkness = 0.f;
		float Park = 0.f;
		float Market = 0.f;
		float Busy = 0.f;
		float Station = 0.f;
		float StationPan = 0.f;
		float Church = 0.f;
		float ChurchPan = 0.f;
	};

	enum class ECall : uint8
	{
		Bell,
		TramBell,
		Whistle,
		Chuff,
		Clank,
		Bark,
		Horn,
		Carpet,
		Anvil,
		Chirp,
		Cricket,
		Squeal,
		Tune,
	};

	constexpr int32 MaxCalls = 40;
	constexpr int32 BabbleVoices = 8;
	constexpr float MixVolume = 0.15f;
	/** How often the dice are thrown for a new far-off sound (s). */
	constexpr float DiceEvery = 0.25f;

	// A big church bell: the partials of its ring (times the strike note), how loud, how long (s).
	constexpr float BellRatio[8] = { 0.5f, 1.f, 1.2f, 1.5f, 2.f, 2.5f, 3.f, 4.2f };
	constexpr float BellLoud[8] = { 0.45f, 0.7f, 0.45f, 0.25f, 0.5f, 0.2f, 0.15f, 0.08f };
	constexpr float BellRing[8] = { 4.5f, 3.2f, 2.4f, 1.8f, 1.6f, 1.f, 0.8f, 0.5f };

	// Vowels for the crowd's murmur: first and second formant (Hz).
	constexpr float VowelLow[6] = { 730.f, 530.f, 300.f, 570.f, 320.f, 660.f };
	constexpr float VowelHigh[6] = { 1090.f, 1840.f, 2200.f, 840.f, 900.f, 1700.f };

	// The whistler's scale (semitones above the key note).
	constexpr int32 TuneSteps[8] = { 0, 2, 4, 5, 7, 9, 11, 12 };

	/** One far-off sound playing. */
	struct FCall
	{
		ECall Kind = ECall::Bell;
		bool bOn = false;
		float Delay = 0.f;
		float Age = 0.f;
		float Lasts = 1.f;
		float Gain = 1.f;
		float Pan = 0.f;
		/** Each one a little different in pitch. */
		float Tone = 1.f;
		/** How far: 0 near .. 1 very far; far things lose their highs. */
		float Far = 0.5f;
		/** Repeating sounds (barks, beats, notes): how many left, when the next one comes, how old the current one is. */
		int32 Left = 0;
		float Next = 0.f;
		float Sub = 1000.f;
		float Hz = 0.f;
		float Target = 0.f;
		float Phase[8] = {};
		float Dull = 0.f;
		FResonator R1;
		FResonator R2;
		FResonator R3;
	};

	/** One murmuring voice of the crowd. */
	struct FBabbler
	{
		float Pitch = 150.f;
		float Phase = 0.f;
		int32 Vowel = 0;
		float Syllable = 0.f;
		float SyllableLasts = 0.2f;
		bool bQuiet = false;
		float Level = 0.f;
		float Pan = 0.f;
		FResonator F1;
		FResonator F2;
	};

	/** A tiny reverb (four combs and two all-passes a side), so far sounds ring off the house fronts. */
	struct FEcho
	{
		static constexpr int32 Size = 2048;
		float Comb[8][Size] = {};
		float CombLow[8] = {};
		int32 CombLen[8] = {};
		int32 CombAt[8] = {};
		float Pass[4][Size] = {};
		int32 PassLen[4] = {};
		int32 PassAt[4] = {};

		void Prepare(float SampleRate)
		{
			const int32 Combs[4] = { 1116, 1188, 1277, 1356 };
			const int32 Passes[2] = { 556, 441 };
			const float Scale = SampleRate / 44100.f;
			for (int32 c = 0; c < 8; ++c)
			{
				CombLen[c] = FMath::Clamp(static_cast<int32>((Combs[c % 4] + (c >= 4 ? 23 : 0)) * Scale), 16, Size);
			}
			for (int32 p = 0; p < 4; ++p)
			{
				PassLen[p] = FMath::Clamp(static_cast<int32>((Passes[p % 2] + (p >= 2 ? 23 : 0)) * Scale), 16, Size);
			}
		}

		float Side(float In, int32 First)
		{
			float Sum = 0.f;
			for (int32 c = First; c < First + 4; ++c)
			{
				const float Held = Comb[c][CombAt[c]];
				CombLow[c] = Held * 0.7f + CombLow[c] * 0.3f;
				Comb[c][CombAt[c]] = In + CombLow[c] * 0.8f;
				CombAt[c] = (CombAt[c] + 1) % CombLen[c];
				Sum += Held;
			}
			for (int32 p = First / 2; p < First / 2 + 2; ++p)
			{
				const float Held = Pass[p][PassAt[p]];
				Pass[p][PassAt[p]] = Sum + Held * 0.5f;
				PassAt[p] = (PassAt[p] + 1) % PassLen[p];
				Sum = Held - Sum;
			}
			return Sum * 0.25f;
		}
	};

	struct FVoice
	{
		/** Out: NumFrames stereo frames, left and right interleaved. */
		void Generate(float* Out, int32 NumFrames, const FScene& Scene, float SampleRate)
		{
			if (!bPrepared)
			{
				Echo.Prepare(SampleRate);
				for (int32 b = 0; b < BabbleVoices; ++b)
				{
					Babblers[b].Pan = -0.9f + 1.8f * (b + 0.5f) / BabbleVoices;
					Babblers[b].Pitch = 105.f + 120.f * Unit();
				}
				LastHour = FMath::FloorToInt(Scene.Hour);
				bPrepared = true;
			}
			const float Dt = 1.f / SampleRate;
			const float Day = 1.f - FMath::Clamp(Scene.Darkness, 0.f, 1.f);

			// The bed's levels.
			const float HumTarget = 0.35f + 0.25f * Day + 0.4f * Scene.Busy * Day;
			const float CrowdTarget = FMath::Clamp(0.08f + 0.85f * Scene.Market + 0.45f * Scene.Busy, 0.f, 1.f) * (0.2f + 0.8f * Day);
			const float Follow = 1.f - FMath::Exp(-Dt / 1.5f);
			const float HumSmooth = 1.f - FMath::Exp(-2.f * PI * 70.f * Dt);
			const float LeafSmooth = 1.f - FMath::Exp(-2.f * PI * 2500.f * Dt);
			const float CrowdSmooth = 1.f - FMath::Exp(-2.f * PI * 1300.f * Dt);

			for (int32 i = 0; i < NumFrames; ++i)
			{
				Dice -= Dt;
				if (Dice <= 0.f)
				{
					Dice += DiceEvery;
					ThrowDice(Scene, Day);
				}

				float Left = 0.f;
				float Right = 0.f;
				float ToEcho = 0.f;

				// The far hum of the city, rising now and then as something heavy passes a street away.
				HumLevel += (HumTarget - HumLevel) * Follow;
				Swell += (SwellTarget - Swell) * (1.f - FMath::Exp(-Dt / 2.5f));
				SwellUntil -= Dt;
				if (SwellUntil <= 0.f)
				{
					SwellTarget = Unit() < 0.3f ? 0.5f + 0.8f * Unit() : 0.f;
					SwellUntil = 2.f + 5.f * Unit();
				}
				HumL1 += (Noise() - HumL1) * HumSmooth;
				HumL2 += (HumL1 - HumL2) * HumSmooth;
				HumR1 += (Noise() - HumR1) * HumSmooth;
				HumR2 += (HumR1 - HumR2) * HumSmooth;
				const float Hum = HumLevel * (1.f + Swell) * 2.2f;
				Left += HumL2 * Hum;
				Right += HumR2 * Hum;

				// Wind in gusts, and near the park the leaves rustling with it.
				GustUntil -= Dt;
				if (GustUntil <= 0.f)
				{
					GustTarget = 0.15f + 0.85f * Unit() * Unit();
					GustUntil = 1.5f + 4.f * Unit();
				}
				Gust += (GustTarget - Gust) * (1.f - FMath::Exp(-Dt / 1.2f));
				const float Wind = WindBand.Run(Noise(), 260.f + 500.f * Gust, 350.f, SampleRate) * Gust * 0.35f;
				LeafLow += (Noise() - LeafLow) * LeafSmooth;
				const float Leaves = (Noise() - LeafLow) * Gust * Gust * Scene.Park * 0.06f;
				Left += Wind * 0.8f + Leaves * (0.7f + 0.3f * Gust);
				Right += Wind * 0.8f + Leaves * (1.f - 0.3f * Gust);

				// The murmur of a crowd: voices babbling vowels, dulled as if round a corner.
				CrowdLevel += (CrowdTarget - CrowdLevel) * Follow;
				if (CrowdLevel > 0.005f)
				{
					float CrowdL = 0.f;
					float CrowdR = 0.f;
					for (FBabbler& B : Babblers)
					{
						B.Syllable += Dt;
						if (B.Syllable >= B.SyllableLasts)
						{
							B.Syllable = 0.f;
							B.SyllableLasts = 0.1f + 0.22f * Unit();
							B.bQuiet = Unit() < 0.22f;
							B.Vowel = FMath::Min(static_cast<int32>(Unit() * 6.f), 5);
							B.Pitch = FMath::Clamp(B.Pitch * (0.92f + 0.16f * Unit()), 95.f, 240.f);
						}
						const float Shape = FMath::Sin(PI * B.Syllable / B.SyllableLasts);
						B.Level += ((B.bQuiet ? 0.f : Shape) - B.Level) * (1.f - FMath::Exp(-Dt / 0.02f));
						B.Phase += B.Pitch * (1.f + 0.03f * Shape) * Dt;
						B.Phase -= FMath::FloorToFloat(B.Phase);
						const float Buzz = (B.Phase * 2.f - 1.f) + 0.3f * Noise();
						const float Spoken = B.F1.Run(Buzz, VowelLow[B.Vowel], 130.f, SampleRate) * 2.f + B.F2.Run(Buzz, VowelHigh[B.Vowel], 200.f, SampleRate);
						CrowdL += Spoken * B.Level * (0.5f - 0.5f * B.Pan);
						CrowdR += Spoken * B.Level * (0.5f + 0.5f * B.Pan);
					}
					CrowdL1 += (CrowdL - CrowdL1) * CrowdSmooth;
					CrowdL2 += (CrowdL1 - CrowdL2) * CrowdSmooth;
					CrowdR1 += (CrowdR - CrowdR1) * CrowdSmooth;
					CrowdR2 += (CrowdR1 - CrowdR2) * CrowdSmooth;
					Left += CrowdL2 * CrowdLevel * 0.04f;
					Right += CrowdR2 * CrowdLevel * 0.04f;
					ToEcho += (CrowdL2 + CrowdR2) * CrowdLevel * 0.01f;
				}

				// The far-off sounds.
				for (FCall& C : Calls)
				{
					if (!C.bOn)
					{
						continue;
					}
					if (C.Delay > 0.f)
					{
						C.Delay -= Dt;
						continue;
					}
					float Sample = Sound(C, Dt, SampleRate) * C.Gain;
					// Far away the highs are lost.
					const float Cut = 1.f - FMath::Exp(-2.f * PI * (7000.f - 5800.f * C.Far) * Dt);
					C.Dull += (Sample - C.Dull) * Cut;
					Sample = C.Dull;
					const float Angle = (C.Pan + 1.f) * 0.25f * PI;
					const float Dry = 1.f - 0.6f * C.Far;
					Left += Sample * FMath::Cos(Angle) * Dry;
					Right += Sample * FMath::Sin(Angle) * Dry;
					ToEcho += Sample * (0.3f + 0.5f * C.Far);
					C.Age += Dt;
					if (C.Age >= C.Lasts)
					{
						C.bOn = false;
					}
				}

				Left += Echo.Side(ToEcho, 0);
				Right += Echo.Side(ToEcho, 4);

				Left *= MixVolume;
				Right *= MixVolume;
				// Soft limit, so nothing clips harshly.
				Out[2 * i] = Left / (1.f + FMath::Abs(Left));
				Out[2 * i + 1] = Right / (1.f + FMath::Abs(Right));
			}
		}

	private:
		/** Start a far-off sound; returns it so the caller can set it up further. */
		FCall& Start(ECall Kind, float Lasts, float Gain, float Pan, float Far, float Delay = 0.f)
		{
			int32 Slot = 0;
			float Oldest = -1.f;
			for (int32 c = 0; c < MaxCalls; ++c)
			{
				if (!Calls[c].bOn)
				{
					Slot = c;
					break;
				}
				if (Calls[c].Age > Oldest)
				{
					Oldest = Calls[c].Age;
					Slot = c;
				}
			}
			FCall& C = Calls[Slot];
			C = FCall();
			C.Kind = Kind;
			C.bOn = true;
			C.Lasts = Lasts;
			C.Gain = Gain;
			C.Pan = FMath::Clamp(Pan, -1.f, 1.f);
			C.Far = FMath::Clamp(Far, 0.f, 1.f);
			C.Delay = Delay;
			C.Tone = 0.92f + 0.16f * Unit();
			return C;
		}

		bool Roll(float PerMinute)
		{
			return Unit() < PerMinute * DiceEvery / 60.f;
		}

		float AnyPan() { return Noise() * 0.9f; }

		/** Decide whether something far off makes a sound now. */
		void ThrowDice(const FScene& Scene, float Day)
		{
			const float Night = 1.f - Day;
			const int32 Clock = FMath::FloorToInt(Scene.Hour);
			const bool bWorkHours = Scene.Hour >= 7.f && Scene.Hour < 18.5f;
			const bool bTramHours = Scene.Hour >= 5.f && Scene.Hour < 23.5f;

			// The church strikes the hour; at noon all its bells peal.
			if (Clock != LastHour)
			{
				LastHour = Clock;
				const float Loud = 0.15f + 0.45f * Scene.Church;
				const float ChurchFar = 1.f - Scene.Church;
				if (Clock == 12)
				{
					for (int32 s = 0; s < 22; ++s)
					{
						const float Fade = FMath::Min(1.f, FMath::Min((s + 1) / 5.f, (22 - s) / 5.f));
						FCall& C = Start(ECall::Bell, 5.f, Loud * 0.8f * Fade, Scene.ChurchPan, ChurchFar, s * 0.85f);
						C.Tone = (s % 3 == 0) ? 0.75f : (s % 3 == 1 ? 1.f : 1.26f);
					}
				}
				else
				{
					const int32 Strikes = Clock % 12 == 0 ? 12 : Clock % 12;
					for (int32 s = 0; s < Strikes; ++s)
					{
						FCall& C = Start(ECall::Bell, 5.f, Loud, Scene.ChurchPan, ChurchFar, 1.f + s * 2.2f);
						C.Tone = 0.8f;
					}
				}
			}

			// Keleti: engines whistling, puffing off, wagons bumping together.
			const float StationFar = 1.f - 0.7f * Scene.Station;
			const float StationLoud = 0.15f + 0.6f * Scene.Station;
			if (Roll(1.2f + 2.f * Scene.Station))
			{
				FCall& C = Start(ECall::Whistle, 0.7f + 1.6f * Unit(), StationLoud * 0.7f, Scene.StationPan + 0.2f * Noise(), StationFar);
				if (Unit() < 0.4f)
				{
					// A second blast.
					FCall& Again = Start(ECall::Whistle, 0.5f + 0.6f * Unit(), StationLoud * 0.7f, C.Pan, StationFar, C.Lasts + 0.25f);
					Again.Tone = C.Tone;
				}
			}
			if (Roll(0.4f + 1.2f * Scene.Station))
			{
				FCall& C = Start(ECall::Chuff, 9.f + 5.f * Unit(), StationLoud * 0.5f, Scene.StationPan + 0.2f * Noise(), StationFar);
				C.Next = 0.f;
				C.Hz = 0.55f;
			}
			if (Roll(0.8f + 2.5f * Scene.Station))
			{
				FCall& C = Start(ECall::Clank, 2.f, StationLoud * 0.5f, Scene.StationPan + 0.3f * Noise(), StationFar);
				C.Left = 3 + static_cast<int32>(Unit() * 4.f);
				C.Hz = 0.3f;
			}

			// Dogs in the courtyards, more noticed at night.
			if (Roll(1.2f + 1.5f * Night))
			{
				FCall& C = Start(ECall::Bark, 3.f, 0.12f + 0.2f * Unit(), AnyPan(), 0.4f + 0.6f * Unit());
				C.Left = 1 + static_cast<int32>(Unit() * 4.f);
			}

			// A bulb horn a street or two away.
			if (Roll((0.6f + 2.f * Scene.Busy) * (0.3f + 0.7f * Day)))
			{
				FCall& C = Start(ECall::Horn, 1.2f, 0.1f + 0.15f * Unit(), AnyPan(), 0.3f + 0.6f * Unit());
				C.Left = 1 + (Unit() < 0.4f ? 1 : 0);
			}

			// Beating a carpet over the rail in a courtyard, and a smith at his anvil.
			if (bWorkHours && Roll(1.2f * Day))
			{
				FCall& C = Start(ECall::Carpet, 10.f, 0.15f + 0.15f * Unit(), AnyPan(), 0.4f + 0.5f * Unit());
				C.Left = 6 + static_cast<int32>(Unit() * 9.f);
				C.Hz = 0.55f + 0.2f * Unit();
			}
			if (bWorkHours && Roll(0.5f * Day))
			{
				FCall& C = Start(ECall::Anvil, 5.f, 0.1f + 0.1f * Unit(), AnyPan(), 0.5f + 0.4f * Unit());
				C.Left = 3 + static_cast<int32>(Unit() * 6.f);
				C.Hz = 0.38f + 0.15f * Unit();
			}

			// Sparrows, everywhere by day, many in the park and the market.
			const float Dawn = (Scene.Hour > 5.f && Scene.Hour < 9.f) ? 1.5f : 1.f;
			if (Roll((2.5f + 10.f * Scene.Park + 3.f * Scene.Market) * Day * Dawn))
			{
				FCall& C = Start(ECall::Chirp, 2.f, 0.12f + 0.15f * Unit(), AnyPan(), 0.2f + 0.6f * Unit());
				C.Left = 2 + static_cast<int32>(Unit() * 6.f);
			}

			// Crickets in the grass of the park at night.
			if (Roll((4.f * Scene.Park + 0.3f) * Night))
			{
				Start(ECall::Cricket, 6.f + 8.f * Unit(), 0.06f + 0.06f * Scene.Park, AnyPan(), 0.2f + 0.5f * Unit());
			}

			// A tram a few streets off: its foot bell, and its wheels squealing in a curve.
			if (bTramHours && Roll(1.5f + 1.5f * Scene.Busy))
			{
				const float Pan = AnyPan();
				const float Far = 0.4f + 0.5f * Unit();
				const float Loud = 0.2f + 0.2f * Unit();
				FCall& First = Start(ECall::TramBell, 1.5f, Loud, Pan, Far);
				FCall& Second = Start(ECall::TramBell, 1.5f, Loud, Pan, Far, 0.22f);
				Second.Tone = First.Tone;
			}
			if (bTramHours && Roll(0.8f + Scene.Busy))
			{
				Start(ECall::Squeal, 1.2f + 1.8f * Unit(), 0.05f + 0.06f * Unit(), AnyPan(), 0.4f + 0.5f * Unit());
			}

			// Someone whistling a tune as he walks by somewhere.
			if (Scene.Hour > 7.f && Scene.Hour < 23.f && Roll(0.5f * Day + 0.3f))
			{
				FCall& C = Start(ECall::Tune, 8.f, 0.1f + 0.08f * Unit(), AnyPan(), 0.3f + 0.5f * Unit());
				C.Left = 7 + static_cast<int32>(Unit() * 8.f);
				C.Target = 2.f + FMath::FloorToFloat(Unit() * 3.f);
			}
		}

		/** One sample of a far-off sound. */
		float Sound(FCall& C, float Dt, float SampleRate)
		{
			const float A = C.Age;
			switch (C.Kind)
			{
			case ECall::Bell:
			case ECall::TramBell:
			{
				// Partials ringing away, each at its own pace.
				const bool bTram = C.Kind == ECall::TramBell;
				const float Strike = bTram ? 1250.f * C.Tone : 330.f * C.Tone;
				float Sample = 0.f;
				const int32 Count = bTram ? 3 : 8;
				for (int32 p = 0; p < Count; ++p)
				{
					const float Ratio = bTram ? (p == 0 ? 1.f : (p == 1 ? 2.4f : 3.9f)) : BellRatio[p];
					const float Loud = bTram ? (p == 0 ? 0.8f : (p == 1 ? 0.4f : 0.2f)) : BellLoud[p];
					const float Ring = bTram ? (p == 0 ? 0.7f : (p == 1 ? 0.35f : 0.2f)) : BellRing[p];
					C.Phase[p] += Strike * Ratio * Dt;
					C.Phase[p] -= FMath::FloorToFloat(C.Phase[p]);
					Sample += FMath::Sin(2.f * PI * C.Phase[p]) * Loud * FMath::Exp(-A / Ring);
				}
				// The clang of the clapper.
				if (A < 0.004f)
				{
					Sample += Noise() * 0.6f;
				}
				return Sample * FMath::Min(1.f, A / 0.002f);
			}
			case ECall::Whistle:
			{
				// A steam whistle: a three-note chord with breath in it, sliding up as the steam comes.
				const float Envelope = FMath::Min(1.f, A / 0.08f) * FMath::Clamp((C.Lasts - A) / 0.25f, 0.f, 1.f);
				const float Rise = 0.93f + 0.07f * FMath::Min(1.f, A / 0.18f);
				const float Base = 470.f * C.Tone * Rise;
				const float Chord[3] = { 1.f, 1.26f, 1.5f };
				float Sample = 0.f;
				for (int32 p = 0; p < 3; ++p)
				{
					C.Phase[p] += Base * Chord[p] * Dt;
					C.Phase[p] -= FMath::FloorToFloat(C.Phase[p]);
					const float Angle = 2.f * PI * C.Phase[p];
					Sample += FMath::Sin(Angle) + 0.25f * FMath::Sin(2.f * Angle);
				}
				Sample += C.R1.Run(Noise(), Base * 2.f, 500.f, SampleRate) * 3.f;
				return Sample * 0.35f * Envelope;
			}
			case ECall::Chuff:
			{
				// An engine pulling out: puffs of steam, slowly quickening, fading into the distance.
				C.Next -= Dt;
				if (C.Next <= 0.f)
				{
					C.Next = C.Hz;
					C.Hz = FMath::Max(0.24f, C.Hz * 0.95f);
					C.Sub = 0.f;
				}
				C.Sub += Dt;
				const float Puff = FMath::Exp(-C.Sub / 0.1f) * FMath::Min(1.f, C.Sub / 0.01f);
				const float Envelope = FMath::Min(1.f, A / 1.f) * FMath::Clamp((C.Lasts - A) / 4.f, 0.f, 1.f);
				const float Steam = C.R1.Run(Noise(), 420.f, 600.f, SampleRate) * 2.5f + C.R2.Run(Noise(), 1800.f, 1500.f, SampleRate) * 0.6f;
				return Steam * (Puff + 0.08f) * Envelope;
			}
			case ECall::Clank:
			case ECall::Anvil:
			{
				// Iron on iron: buffers banging down a row of wagons, or a hammer on an anvil.
				const bool bAnvil = C.Kind == ECall::Anvil;
				C.Next -= Dt;
				if (C.Next <= 0.f && C.Left > 0)
				{
					--C.Left;
					C.Next = bAnvil ? C.Hz * (C.Left % 2 == 0 ? 1.4f : 0.6f) : C.Hz;
					if (!bAnvil)
					{
						C.Hz *= 0.85f;
					}
					C.Sub = 0.f;
					C.Lasts = A + C.Next + (bAnvil ? 1.2f : 1.f);
				}
				C.Sub += Dt;
				const float Hit = C.Sub < 0.003f ? Noise() * 30.f : 0.f;
				if (bAnvil)
				{
					return C.R1.Run(Hit, 1900.f * C.Tone, 9.f, SampleRate) * 0.6f
						+ C.R2.Run(Hit, 3150.f * C.Tone, 12.f, SampleRate) * 0.4f
						+ C.R3.Run(Hit, 4600.f * C.Tone, 18.f, SampleRate) * 0.25f
						+ Hit * 0.01f;
				}
				return C.R1.Run(Hit, 310.f * C.Tone, 25.f, SampleRate) * 0.6f
					+ C.R2.Run(Hit, 870.f * C.Tone, 35.f, SampleRate) * 0.4f
					+ C.R3.Run(Hit, 1650.f * C.Tone, 60.f, SampleRate) * 0.3f;
			}
			case ECall::Bark:
			case ECall::Horn:
			{
				// A dog's bark, or a rubber bulb horn's honk: a buzz through a mouth that opens and shuts.
				const bool bHorn = C.Kind == ECall::Horn;
				const float Each = bHorn ? 0.28f : 0.15f;
				C.Next -= Dt;
				if (C.Next <= 0.f && C.Left > 0)
				{
					--C.Left;
					C.Next = bHorn ? 0.42f : 0.3f + 0.25f * Unit();
					C.Sub = 0.f;
					C.Lasts = A + C.Next + 0.3f;
				}
				C.Sub += Dt;
				if (C.Sub > Each)
				{
					return 0.f;
				}
				const float T = C.Sub / Each;
				const float Pitch = bHorn ? 330.f * C.Tone * (1.f - 0.06f * T) : 480.f * C.Tone * (1.f - 0.3f * T);
				C.Phase[0] += Pitch * Dt;
				C.Phase[0] -= FMath::FloorToFloat(C.Phase[0]);
				const float Buzz = C.Phase[0] * 2.f - 1.f + (bHorn ? 0.f : 0.4f * Noise());
				const float Envelope = bHorn ? FMath::Min(1.f, C.Sub / 0.02f) * FMath::Min(1.f, (Each - C.Sub) / 0.03f)
					: FMath::Min(1.f, C.Sub / 0.012f) * (1.f - T) * (1.f - T);
				const float Low = bHorn ? 1100.f : 800.f;
				const float High = bHorn ? 2500.f : 1500.f;
				return (C.R1.Run(Buzz, Low, 300.f, SampleRate) * 2.f + C.R2.Run(Buzz, High, 500.f, SampleRate)) * Envelope * 1.5f;
			}
			case ECall::Carpet:
			{
				// The flat thwack of a carpet beater, beat after beat.
				C.Next -= Dt;
				if (C.Next <= 0.f && C.Left > 0)
				{
					--C.Left;
					C.Next = C.Hz * (0.9f + 0.2f * Unit());
					C.Sub = 0.f;
					C.Lasts = A + C.Next + 0.5f;
				}
				C.Sub += Dt;
				const float Hit = Noise() * FMath::Exp(-C.Sub / 0.018f) * FMath::Min(1.f, C.Sub / 0.001f);
				return C.R1.Run(Hit, 170.f * C.Tone, 120.f, SampleRate) * 3.f + C.R2.Run(Hit, 1100.f * C.Tone, 1200.f, SampleRate) * 1.2f;
			}
			case ECall::Chirp:
			{
				// A sparrow: quick bright chirps falling in pitch.
				C.Next -= Dt;
				if (C.Next <= 0.f && C.Left > 0)
				{
					--C.Left;
					C.Next = 0.1f + 0.15f * Unit();
					C.Sub = 0.f;
					C.Hz = 0.06f + 0.04f * Unit();
					C.Target = 3800.f + 1600.f * Unit();
					C.Lasts = A + C.Next + 0.1f;
				}
				C.Sub += Dt;
				if (C.Sub > C.Hz)
				{
					return 0.f;
				}
				const float T = C.Sub / C.Hz;
				const float Pitch = C.Target * (1.f - 0.3f * T) * (1.f + 0.06f * FMath::Sin(2.f * PI * 90.f * C.Sub));
				C.Phase[0] += Pitch * Dt;
				C.Phase[0] -= FMath::FloorToFloat(C.Phase[0]);
				return FMath::Sin(2.f * PI * C.Phase[0]) * FMath::Sin(PI * T);
			}
			case ECall::Cricket:
			{
				// A cricket: chirps of three quick pulses, over and over.
				const float Cycle = FMath::Fmod(A, 0.55f * C.Tone);
				const float Pulse = FMath::Fmod(Cycle, 0.032f);
				const float On = (Cycle < 0.096f && Pulse < 0.018f) ? FMath::Sin(PI * Pulse / 0.018f) : 0.f;
				C.Phase[0] += 4400.f * C.Tone * Dt;
				C.Phase[0] -= FMath::FloorToFloat(C.Phase[0]);
				const float Envelope = FMath::Min(1.f, A / 0.5f) * FMath::Clamp((C.Lasts - A) / 0.5f, 0.f, 1.f);
				return FMath::Sin(2.f * PI * C.Phase[0]) * On * Envelope;
			}
			case ECall::Squeal:
			{
				// Steel wheels squealing round a curve, wavering.
				const float Envelope = FMath::Min(1.f, A / 0.3f) * FMath::Clamp((C.Lasts - A) / 0.4f, 0.f, 1.f);
				const float Pitch = 2900.f * C.Tone + 40.f * FMath::Sin(2.f * PI * 5.f * A) + 25.f * FMath::Sin(2.f * PI * 13.f * A);
				C.Phase[0] += Pitch * Dt;
				C.Phase[0] -= FMath::FloorToFloat(C.Phase[0]);
				C.Phase[1] += Pitch * 1.51f * Dt;
				C.Phase[1] -= FMath::FloorToFloat(C.Phase[1]);
				const float Flutter = 0.7f + 0.3f * FMath::Sin(2.f * PI * 7.f * A);
				return (FMath::Sin(2.f * PI * C.Phase[0]) + 0.35f * FMath::Sin(2.f * PI * C.Phase[1])) * Envelope * Flutter;
			}
			case ECall::Tune:
			{
				// A man whistling: notes of a major scale, gliding into each other, with a little vibrato.
				C.Next -= Dt;
				if (C.Next <= 0.f)
				{
					if (C.Left <= 0)
					{
						// The tune is over: fade out.
						C.Left = -1;
						C.Phase[3] = A;
						C.Lasts = A + 0.3f;
						C.Next = 1000.f;
					}
					else
					{
						--C.Left;
						C.Target = FMath::Clamp(C.Target + static_cast<float>(FMath::FloorToInt(Unit() * 5.f) - 2), 0.f, 7.f);
						const int32 Step = TuneSteps[FMath::Clamp(static_cast<int32>(C.Target), 0, 7)];
						C.Phase[2] = 1150.f * C.Tone * FMath::Pow(2.f, Step / 12.f);
						C.Next = Unit() < 0.3f ? 0.5f : 0.25f;
						C.Sub = 0.f;
						C.Lasts = A + C.Next + 0.5f;
					}
				}
				C.Sub += Dt;
				if (C.Hz <= 0.f)
				{
					C.Hz = C.Phase[2];
				}
				C.Hz += (C.Phase[2] - C.Hz) * (1.f - FMath::Exp(-Dt / 0.025f));
				const float Vibrato = 1.f + 0.012f * FMath::Sin(2.f * PI * 5.5f * A);
				C.Phase[0] += C.Hz * Vibrato * Dt;
				C.Phase[0] -= FMath::FloorToFloat(C.Phase[0]);
				const float Note = C.Left < 0
					? FMath::Max(0.f, 1.f - (A - C.Phase[3]) / 0.3f)
					: FMath::Min(1.f, C.Sub / 0.03f) * (0.75f + 0.25f * FMath::Exp(-C.Sub / 0.1f));
				return (FMath::Sin(2.f * PI * C.Phase[0]) + 0.05f * Noise()) * Note;
			}
			default:
				return 0.f;
			}
		}

		/** -1..1 */
		float Noise()
		{
			// A tiny random number maker, safe on the sound thread.
			Seed = Seed * 1664525u + 1013904223u;
			return static_cast<float>(Seed >> 8) / static_cast<float>(1 << 24) * 2.f - 1.f;
		}

		/** 0..1 */
		float Unit() { return 0.5f + 0.5f * Noise(); }

		bool bPrepared = false;
		uint32 Seed = 1932u;
		float Dice = 0.f;
		int32 LastHour = -1;

		FCall Calls[MaxCalls];
		FBabbler Babblers[BabbleVoices];
		FEcho Echo;

		float HumLevel = 0.f;
		float HumL1 = 0.f;
		float HumL2 = 0.f;
		float HumR1 = 0.f;
		float HumR2 = 0.f;
		float Swell = 0.f;
		float SwellTarget = 0.f;
		float SwellUntil = 0.f;

		float Gust = 0.f;
		float GustTarget = 0.f;
		float GustUntil = 0.f;
		FResonator WindBand;
		float LeafLow = 0.f;

		float CrowdLevel = 0.f;
		float CrowdL1 = 0.f;
		float CrowdL2 = 0.f;
		float CrowdR1 = 0.f;
		float CrowdR2 = 0.f;
	};
}
