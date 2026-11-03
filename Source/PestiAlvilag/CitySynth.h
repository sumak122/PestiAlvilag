// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FightSynth.h"

/**
 * The sounds of places in the city, made sample by sample (no sound files).
 * Each place that makes a sound (a church tower, the tracks at Keleti, a
 * market, a flock of pigeons, the park, the café, a courtyard) is a sound
 * spot; ASoundSpots plays the nearest few through USpotSound, placed in the
 * world, so they grow louder as you walk up to them and fade as you leave.
 *
 * FSpotVoice makes the sound of one kind of spot: the church strikes the
 * hour, engines whistle and puff, a crowd murmurs, pigeons coo, sparrows
 * chirp, crickets sing, a gramophone plays a waltz, a carpet is beaten, a dog
 * barks, a smith hammers.
 *
 * Plain C++ with no engine objects, so USpotSound can run it on the sound
 * thread and the same code can be tried outside the game. Mono out.
 */
namespace CitySynth
{
	using FightSynth::FResonator;

	/** What kind of place a sound spot is. */
	enum class ESpot : uint8
	{
		Church,
		Station,
		Market,
		Pigeons,
		Sparrows,
		Crickets,
		Gramophone,
		Carpet,
		Dog,
		Smith,
		Count,
	};

	/** One sound within a spot's sound. */
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
		Coo,
		Record,
	};

	constexpr int32 MaxCalls = 16;
	constexpr int32 BabbleVoices = 8;
	constexpr float SpotVolume = 0.35f;
	/** How often the dice are thrown for a new sound (s). */
	constexpr float DiceEvery = 0.25f;
	/** How fast the loudness follows the listener walking (s). */
	constexpr float GainFollow = 0.25f;

	// A big church bell: the partials of its ring (times the strike note), how loud, how long (s).
	constexpr float BellRatio[8] = { 0.5f, 1.f, 1.2f, 1.5f, 2.f, 2.5f, 3.f, 4.2f };
	constexpr float BellLoud[8] = { 0.45f, 0.7f, 0.45f, 0.25f, 0.5f, 0.2f, 0.15f, 0.08f };
	constexpr float BellRing[8] = { 4.5f, 3.2f, 2.4f, 1.8f, 1.6f, 1.f, 0.8f, 0.5f };

	// Vowels for the crowd's murmur: first and second formant (Hz).
	constexpr float VowelLow[6] = { 730.f, 530.f, 300.f, 570.f, 320.f, 660.f };
	constexpr float VowelHigh[6] = { 1090.f, 1840.f, 2200.f, 840.f, 900.f, 1700.f };

	// The gramophone's scale (semitones above the key note) and its waltz chords (scale steps of the bass).
	constexpr int32 TuneSteps[8] = { 0, 2, 4, 5, 7, 9, 11, 12 };
	constexpr int32 WaltzBass[4] = { 0, 3, 4, 0 };

	/** One sound playing. */
	struct FCall
	{
		ECall Kind = ECall::Bell;
		bool bOn = false;
		float Delay = 0.f;
		float Age = 0.f;
		float Lasts = 1.f;
		float Gain = 1.f;
		/** Each one a little different in pitch. */
		float Tone = 1.f;
		/** Repeating sounds (barks, beats, notes): how many left, when the next one comes, how old the current one is. */
		int32 Left = 0;
		float Next = 0.f;
		float Sub = 1000.f;
		float Hz = 0.f;
		float Target = 0.f;
		float Phase[8] = {};
		FResonator R1;
		FResonator R2;
		FResonator R3;
	};

	/** One murmuring voice of a crowd. */
	struct FBabbler
	{
		float Pitch = 150.f;
		float Phase = 0.f;
		int32 Vowel = 0;
		float Syllable = 0.f;
		float SyllableLasts = 0.2f;
		bool bQuiet = false;
		float Level = 0.f;
		FResonator F1;
		FResonator F2;
	};

	/** A small echo (four combs and two all-passes), so sounds ring off the house fronts. Prepare also clears it. */
	struct FEcho
	{
		static constexpr int32 Size = 2048;
		float Comb[4][Size] = {};
		float CombLow[4] = {};
		int32 CombLen[4] = {};
		int32 CombAt[4] = {};
		float Pass[2][Size] = {};
		int32 PassLen[2] = {};
		int32 PassAt[2] = {};

		void Prepare(float SampleRate)
		{
			const int32 Combs[4] = { 1116, 1188, 1277, 1356 };
			const int32 Passes[2] = { 556, 441 };
			const float Scale = SampleRate / 44100.f;
			for (int32 c = 0; c < 4; ++c)
			{
				CombLen[c] = FMath::Clamp(static_cast<int32>(Combs[c] * Scale), 16, Size);
				CombAt[c] = 0;
				CombLow[c] = 0.f;
				for (int32 k = 0; k < Size; ++k)
				{
					Comb[c][k] = 0.f;
				}
			}
			for (int32 p = 0; p < 2; ++p)
			{
				PassLen[p] = FMath::Clamp(static_cast<int32>(Passes[p] * Scale), 16, Size);
				PassAt[p] = 0;
				for (int32 k = 0; k < Size; ++k)
				{
					Pass[p][k] = 0.f;
				}
			}
		}

		float Run(float In)
		{
			float Sum = 0.f;
			for (int32 c = 0; c < 4; ++c)
			{
				const float Held = Comb[c][CombAt[c]];
				CombLow[c] = Held * 0.7f + CombLow[c] * 0.3f;
				Comb[c][CombAt[c]] = In + CombLow[c] * 0.78f;
				CombAt[c] = (CombAt[c] + 1) % CombLen[c];
				Sum += Held;
			}
			for (int32 p = 0; p < 2; ++p)
			{
				const float Held = Pass[p][PassAt[p]];
				Pass[p][PassAt[p]] = Sum + Held * 0.5f;
				PassAt[p] = (PassAt[p] + 1) % PassLen[p];
				Sum = Held - Sum;
			}
			return Sum * 0.25f;
		}
	};

	struct FSpotVoice
	{
		/**
		 * Out: NumSamples mono samples of the spot's sound.
		 * Gain: how loud it reaches the listener (0..1); Far: 0 beside it .. 1 at the edge of hearing (far loses the highs).
		 * Generation: changes when the voice is given to another spot, so it starts afresh.
		 */
		void Generate(float* Out, int32 NumSamples, ESpot Spot, int32 Generation, float Gain, float Far, float Hour, float SampleRate)
		{
			if (Generation != MyGeneration || Spot != MySpot)
			{
				Restart(Spot, Generation, Hour, SampleRate);
			}
			const float Dt = 1.f / SampleRate;
			const float GainSmooth = 1.f - FMath::Exp(-Dt / GainFollow);
			const float Cut = 1.f - FMath::Exp(-2.f * PI * (8000.f - 6500.f * FMath::Clamp(Far, 0.f, 1.f)) * Dt);
			const float CrowdSmooth = 1.f - FMath::Exp(-2.f * PI * 1600.f * Dt);

			for (int32 i = 0; i < NumSamples; ++i)
			{
				Dice -= Dt;
				if (Dice <= 0.f)
				{
					Dice += DiceEvery;
					ThrowDice(Hour);
				}

				float Sample = 0.f;

				// A crowd: voices babbling vowels, dulled by the stalls and the crowd itself.
				if (MySpot == ESpot::Market)
				{
					float Crowd = 0.f;
					for (FBabbler& B : Babblers)
					{
						B.Syllable += Dt;
						if (B.Syllable >= B.SyllableLasts)
						{
							B.Syllable = 0.f;
							B.SyllableLasts = 0.1f + 0.22f * Unit();
							B.bQuiet = Unit() < 0.25f;
							B.Vowel = FMath::Min(static_cast<int32>(Unit() * 6.f), 5);
							B.Pitch = FMath::Clamp(B.Pitch * (0.92f + 0.16f * Unit()), 95.f, 240.f);
						}
						const float Shape = FMath::Sin(PI * B.Syllable / B.SyllableLasts);
						B.Level += ((B.bQuiet ? 0.f : Shape) - B.Level) * (1.f - FMath::Exp(-Dt / 0.02f));
						B.Phase += B.Pitch * (1.f + 0.03f * Shape) * Dt;
						B.Phase -= FMath::FloorToFloat(B.Phase);
						const float Buzz = (B.Phase * 2.f - 1.f) + 0.3f * Noise();
						Crowd += (B.F1.Run(Buzz, VowelLow[B.Vowel], 130.f, SampleRate) * 2.f + B.F2.Run(Buzz, VowelHigh[B.Vowel], 200.f, SampleRate)) * B.Level;
					}
					Crowd1 += (Crowd - Crowd1) * CrowdSmooth;
					Crowd2 += (Crowd1 - Crowd2) * CrowdSmooth;
					Sample += Crowd2 * 0.02f;
				}

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
					Sample += Sound(C, Dt, SampleRate) * C.Gain;
					C.Age += Dt;
					if (C.Age >= C.Lasts)
					{
						C.bOn = false;
					}
				}

				// Far off the highs are lost and more of the sound is echo.
				Dull += (Sample - Dull) * Cut;
				const float Wet = Echo.Run(Dull);
				Level += (Gain - Level) * GainSmooth;
				float Heard = (Dull * (1.f - 0.5f * Far) + Wet * (0.3f + 0.6f * Far)) * Level * SpotVolume;
				// Soft limit, so nothing clips harshly.
				Heard = Heard / (1.f + FMath::Abs(Heard));
				Out[i] = Heard;
			}
		}

	private:
		void Restart(ESpot Spot, int32 Generation, float Hour, float SampleRate)
		{
			MySpot = Spot;
			MyGeneration = Generation;
			Seed = 1932u + static_cast<uint32>(Generation) * 7919u;
			for (FCall& C : Calls)
			{
				C.bOn = false;
			}
			Level = 0.f;
			Dull = 0.f;
			Crowd1 = 0.f;
			Crowd2 = 0.f;
			Echo.Prepare(SampleRate);
			for (int32 b = 0; b < BabbleVoices; ++b)
			{
				Babblers[b] = FBabbler();
				Babblers[b].Pitch = 105.f + 120.f * Unit();
			}
			LastHour = FMath::FloorToInt(Hour);
			// Not all spots start at once.
			Dice = DiceEvery * Unit();
			if (Spot == ESpot::Gramophone)
			{
				// A record already playing when you come by.
				StartRecord(20.f + 30.f * Unit());
			}
			else if (Spot == ESpot::Carpet)
			{
				// Someone is at it soon after you come by, so the courtyard is not silent all the way past.
				FCall& C = Start(ECall::Carpet, 10.f, 0.14f + 0.08f * Unit(), 3.f + 3.f * Unit());
				C.Left = 6 + static_cast<int32>(Unit() * 12.f);
				C.Hz = 0.55f + 0.2f * Unit();
			}
			else if (Spot == ESpot::Dog)
			{
				FCall& C = Start(ECall::Bark, 3.f, 0.15f + 0.1f * Unit(), 3.f + 4.f * Unit());
				C.Left = 1 + static_cast<int32>(Unit() * 4.f);
			}
			else if (Spot == ESpot::Smith)
			{
				FCall& C = Start(ECall::Anvil, 5.f, 0.9f + 0.45f * Unit(), 3.f + 3.f * Unit());
				C.Left = 4 + static_cast<int32>(Unit() * 8.f);
				C.Hz = 0.38f + 0.15f * Unit();
			}
		}

		/** Start a sound; returns it so the caller can set it up further. */
		FCall& Start(ECall Kind, float Lasts, float Gain, float Delay = 0.f)
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
			C.Delay = Delay;
			C.Tone = 0.92f + 0.16f * Unit();
			return C;
		}

		void StartRecord(float Lasts)
		{
			FCall& C = Start(ECall::Record, Lasts, 0.012f);
			C.Tone = 0.85f + 0.3f * Unit();
			C.Target = 2.f;
		}

		bool Roll(float PerMinute)
		{
			return Unit() < PerMinute * DiceEvery / 60.f;
		}

		bool Playing(ECall Kind) const
		{
			for (const FCall& C : Calls)
			{
				if (C.bOn && C.Kind == Kind)
				{
					return true;
				}
			}
			return false;
		}

		/** Decide whether the spot makes a new sound now. */
		void ThrowDice(float Hour)
		{
			const int32 Clock = FMath::FloorToInt(Hour);
			const bool bNewHour = Clock != LastHour;
			LastHour = Clock;

			switch (MySpot)
			{
			case ESpot::Church:
				// The hour struck on the big bell; at noon a peal of three bells.
				if (bNewHour)
				{
					if (Clock == 12)
					{
						for (int32 s = 0; s < 18; ++s)
						{
							const float Fade = FMath::Min(1.f, FMath::Min((s + 1) / 4.f, (18 - s) / 4.f));
							FCall& C = Start(ECall::Bell, 5.f, 0.7f * Fade, s * 0.85f);
							C.Tone = (s % 3 == 0) ? 0.75f : (s % 3 == 1 ? 1.f : 1.26f);
						}
					}
					else
					{
						const int32 Strikes = Clock % 12 == 0 ? 12 : Clock % 12;
						for (int32 s = 0; s < Strikes; ++s)
						{
							FCall& C = Start(ECall::Bell, 5.f, 1.f, 1.f + s * 2.2f);
							C.Tone = 0.8f;
						}
					}
				}
				break;
			case ESpot::Station:
				// Engines whistling, puffing off, wagons bumping together.
				if (Roll(1.5f))
				{
					FCall& C = Start(ECall::Whistle, 0.7f + 1.4f * Unit(), 0.6f);
					if (Unit() < 0.4f)
					{
						FCall& Again = Start(ECall::Whistle, 0.5f + 0.5f * Unit(), 0.6f, C.Lasts + 0.25f);
						Again.Tone = C.Tone;
					}
				}
				if (Roll(0.7f) && !Playing(ECall::Chuff))
				{
					FCall& C = Start(ECall::Chuff, 9.f + 5.f * Unit(), 0.5f);
					C.Hz = 0.55f;
				}
				if (Roll(2.f))
				{
					FCall& C = Start(ECall::Clank, 2.f, 0.5f);
					C.Left = 3 + static_cast<int32>(Unit() * 4.f);
					C.Hz = 0.3f;
				}
				break;
			case ESpot::Market:
				// The murmur runs all the time; now and then a crate is banged down.
				if (Roll(2.f))
				{
					FCall& C = Start(ECall::Carpet, 1.f, 0.15f);
					C.Left = 1;
					C.Hz = 0.3f;
				}
				break;
			case ESpot::Pigeons:
				if (Roll(10.f))
				{
					FCall& C = Start(ECall::Coo, 3.f, 0.02f + 0.016f * Unit());
					C.Left = 1 + static_cast<int32>(Unit() * 3.f);
				}
				break;
			case ESpot::Sparrows:
				if (Roll(14.f))
				{
					FCall& C = Start(ECall::Chirp, 2.f, 0.36f + 0.3f * Unit());
					C.Left = 2 + static_cast<int32>(Unit() * 6.f);
				}
				break;
			case ESpot::Crickets:
				if (Roll(8.f))
				{
					Start(ECall::Cricket, 6.f + 8.f * Unit(), 0.25f + 0.2f * Unit());
				}
				break;
			case ESpot::Gramophone:
				// A record, a short pause to wind it and turn it over, the next record.
				if (!Playing(ECall::Record) && Roll(6.f))
				{
					StartRecord(40.f + 30.f * Unit());
				}
				break;
			case ESpot::Carpet:
				if (!Playing(ECall::Carpet) && Roll(3.f))
				{
					FCall& C = Start(ECall::Carpet, 10.f, 0.14f + 0.08f * Unit());
					C.Left = 6 + static_cast<int32>(Unit() * 12.f);
					C.Hz = 0.55f + 0.2f * Unit();
				}
				break;
			case ESpot::Dog:
				if (!Playing(ECall::Bark) && Roll(2.f))
				{
					FCall& C = Start(ECall::Bark, 3.f, 0.15f + 0.1f * Unit());
					C.Left = 1 + static_cast<int32>(Unit() * 4.f);
				}
				break;
			case ESpot::Smith:
				if (!Playing(ECall::Anvil) && Roll(4.f))
				{
					FCall& C = Start(ECall::Anvil, 5.f, 0.9f + 0.45f * Unit());
					C.Left = 4 + static_cast<int32>(Unit() * 8.f);
					C.Hz = 0.38f + 0.15f * Unit();
				}
				break;
			default:
				break;
			}
		}

		/** One sample of a sound. */
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
			case ECall::Coo:
			{
				// A pigeon: a throaty "rroo-coo-ooo", the last note long and falling.
				C.Next -= Dt;
				if (C.Next <= 0.f && C.Left > 0)
				{
					--C.Left;
					C.Next = 1.2f + 0.6f * Unit();
					C.Sub = 0.f;
					C.Lasts = A + C.Next + 0.2f;
				}
				C.Sub += Dt;
				const float Notes[3] = { 0.28f, 0.16f, 0.55f };
				const float Starts[3] = { 0.f, 0.33f, 0.54f };
				float Sample = 0.f;
				for (int32 n = 0; n < 3; ++n)
				{
					const float T = C.Sub - Starts[n];
					if (T < 0.f || T > Notes[n])
					{
						continue;
					}
					const float U = T / Notes[n];
					const float Pitch = 380.f * C.Tone * (n == 2 ? 1.f - 0.12f * U : (n == 1 ? 1.08f : 0.95f + 0.05f * U));
					C.Phase[0] += Pitch * Dt;
					C.Phase[0] -= FMath::FloorToFloat(C.Phase[0]);
					const float Angle = 2.f * PI * C.Phase[0];
					const float Throat = 0.65f + 0.35f * FMath::Sin(2.f * PI * 32.f * C.Sub);
					Sample += (FMath::Sin(Angle) + 0.3f * FMath::Sin(2.f * Angle)) * FMath::Sin(PI * U) * Throat;
				}
				return C.R1.Run(Sample, 500.f, 900.f, SampleRate) * 3.f;
			}
			case ECall::Record:
			{
				// A gramophone: a waltz on violin and piano, thin and scratchy, with the needle's crackle.
				C.Next -= Dt;
				if (C.Next <= 0.f)
				{
					// Next beat of three.
					C.Next = 0.42f;
					C.Left = (C.Left + 1) % 3;
					if (C.Left == 0)
					{
						C.Hz = static_cast<float>((static_cast<int32>(C.Hz) + 1) % 4);
					}
					if (Unit() < 0.75f)
					{
						C.Target = FMath::Clamp(C.Target + static_cast<float>(FMath::FloorToInt(Unit() * 5.f) - 2), 0.f, 7.f);
					}
					C.Sub = 0.f;
				}
				C.Sub += Dt;
				const float Key = 440.f * C.Tone;
				const int32 Step = TuneSteps[FMath::Clamp(static_cast<int32>(C.Target), 0, 7)];
				const float Tune = Key * FMath::Pow(2.f, Step / 12.f);
				C.Phase[3] += (Tune - C.Phase[3]) * (1.f - FMath::Exp(-Dt / 0.03f));
				const float Vibrato = 1.f + 0.008f * FMath::Sin(2.f * PI * 5.5f * A);
				C.Phase[0] += C.Phase[3] * Vibrato * Dt;
				C.Phase[0] -= FMath::FloorToFloat(C.Phase[0]);
				const float Violin = C.R1.Run(C.Phase[0] * 2.f - 1.f, 1300.f, 1400.f, SampleRate) * 1.5f;
				// Piano: the bass note on the first beat, the chord on the second and third.
				const int32 Bass = TuneSteps[WaltzBass[FMath::Clamp(static_cast<int32>(C.Hz), 0, 3)]];
				const float BassHz = Key * 0.25f * FMath::Pow(2.f, Bass / 12.f);
				const float Struck = FMath::Exp(-C.Sub / 0.25f);
				float Piano = 0.f;
				if (C.Left == 0)
				{
					C.Phase[1] += BassHz * Dt;
					C.Phase[1] -= FMath::FloorToFloat(C.Phase[1]);
					Piano = FMath::Sin(2.f * PI * C.Phase[1]) * Struck;
				}
				else
				{
					C.Phase[1] += BassHz * 2.f * Dt;
					C.Phase[1] -= FMath::FloorToFloat(C.Phase[1]);
					C.Phase[2] += BassHz * 2.f * 1.26f * Dt;
					C.Phase[2] -= FMath::FloorToFloat(C.Phase[2]);
					Piano = (FMath::Sin(2.f * PI * C.Phase[1]) + FMath::Sin(2.f * PI * C.Phase[2])) * 0.5f * Struck;
				}
				// The horn of the gramophone: no deep bass, no bright highs.
				const float Music = C.R2.Run(Violin * 0.6f + Piano * 0.8f, 900.f, 1600.f, SampleRate) * 2.f;
				const float Crackle = (Unit() < 0.0006f ? Noise() * 1.5f : 0.f) + Noise() * 0.02f;
				const float Envelope = FMath::Min(1.f, A / 0.5f) * FMath::Clamp((C.Lasts - A) / 1.f, 0.f, 1.f);
				return (Music + Crackle) * Envelope;
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

		ESpot MySpot = ESpot::Count;
		int32 MyGeneration = -1;
		uint32 Seed = 1932u;
		float Dice = 0.f;
		int32 LastHour = -1;
		float Level = 0.f;
		float Dull = 0.f;
		float Crowd1 = 0.f;
		float Crowd2 = 0.f;

		FCall Calls[MaxCalls];
		FBabbler Babblers[BabbleVoices];
		FEcho Echo;
	};
}
