// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * The music of the two bars' bands, made in code: a csárdás in the
 * Hungarian minor, a slow part (lassú) with the first violin free over the
 * cimbalom's tremolo, then a fast part (friss) with the bass and the
 * cimbalom keeping the "esz-tam" beat. Dzsoki's tavern band plays violin,
 * cimbalom and bass; the coffee house band adds a second violin and a
 * clarinet and plays a little brighter. The tunes are made up from a seed,
 * each phrase played twice, so it sounds like a real tune and not a random
 * walk.
 *
 * Plain C++ with no engine objects, so UBandSound can run it on the sound
 * thread and the same code can be tried outside the game. Mono out.
 */
namespace BandSynth
{
	/** Semitones of the Hungarian (gypsy) minor above its key note: A B C D# E F G# A. */
	constexpr int32 Scale[8] = { 0, 2, 3, 6, 7, 8, 11, 12 };
	/** The chords of each two-bar step of a phrase, as scale steps of their root: i, iv, V, i. */
	constexpr int32 ChordRoot[4] = { 0, 3, 4, 0 };

	/** A two-pole band filter (a string's or a body's resonance). */
	struct FBandPass
	{
		float Y1 = 0.f;
		float Y2 = 0.f;
		float Run(float In, float Hz, float Width, float SampleRate)
		{
			const float R = FMath::Exp(-PI * Width / SampleRate);
			const float C = 2.f * R * FMath::Cos(2.f * PI * Hz / SampleRate);
			const float Out = (1.f - R) * In + C * Y1 - R * R * Y2;
			Y2 = Y1;
			Y1 = Out;
			return Out;
		}
	};

	/** One struck cimbalom string: a few partials that die away. */
	struct FStrike
	{
		float Hz = 0.f;
		float Age = 100.f;
		float Loud = 0.f;
		float Phase[3] = { 0.f, 0.f, 0.f };
	};

	struct FBand
	{
		/** Out: NumSamples mono samples. bPlaying fades the band in and out; Smart is the coffee house band; Gain 0..1; Far 0 in the room .. 1 heard through the wall. */
		void Generate(float* Out, int32 NumSamples, bool bPlaying, bool bSmart, float Gain, float Far, float SampleRate)
		{
			const float Dt = 1.f / SampleRate;
			const float Cut = 1.f - FMath::Exp(-2.f * PI * (7000.f - 6300.f * FMath::Clamp(Far, 0.f, 1.f)) * Dt);
			const float FadeRate = Dt / 2.5f;
			for (int32 i = 0; i < NumSamples; ++i)
			{
				Fade = bPlaying ? FMath::Min(1.f, Fade + FadeRate) : FMath::Max(0.f, Fade - FadeRate);
				float Sample = 0.f;
				if (Fade > 0.f)
				{
					Sample = Play(bSmart, Dt, SampleRate);
				}
				// A small warm room: two short echoes fed back.
				const float Wet = Echo(Sample);
				Low += (Sample + 0.35f * Wet - Low) * Cut;
				Out[i] = Low * Fade * Gain;
			}
		}

	private:
		uint32 Seed = 1932;
		float Fade = 0.f;
		float Low = 0.f;
		/** Time in the current beat, the beat in the bar, the bar in the phrase, the phrase. */
		float BeatTime = 0.f;
		int32 Beat = 0;
		int32 Bar = 0;
		int32 Phrase = 0;
		/** True in the fast part. */
		bool bFast = false;
		bool bStarted = false;
		/** The tune of the current phrase: a scale step (0..14, two octaves) for each half beat of its 8 bars. */
		int32 Tune[64] = {};
		int32 TuneSeed = 0;
		float ViolinHz = 440.f;
		float ViolinAim = 440.f;
		float ViolinPhase = 0.f;
		float SecondPhase = 0.f;
		float ClarinetPhase = 0.f;
		float VibratoPhase = 0.f;
		float Bow = 0.f;
		float BassHz = 55.f;
		float BassPhase = 0.f;
		float BassAge = 10.f;
		FBandPass Body1;
		FBandPass Body2;
		FBandPass Body3;
		FBandPass ClarinetBody;
		FStrike Strikes[6];
		int32 NextStrike = 0;
		float TremoloTime = 0.f;
		float EchoA[2400] = {};
		float EchoB[3100] = {};
		int32 EchoAt = 0;
		int32 EchoBt = 0;

		float Unit()
		{
			Seed = Seed * 1664525u + 1013904223u;
			return static_cast<float>(Seed >> 8) / 16777216.f;
		}

		/** A repeatable dice for the tunes, so each phrase can be played twice alike. */
		static float TuneDice(int32 A, int32 B)
		{
			uint32 H = static_cast<uint32>(A) * 2654435761u ^ static_cast<uint32>(B) * 2246822519u;
			H ^= H >> 13;
			H *= 3266489917u;
			H ^= H >> 16;
			return static_cast<float>(H >> 8) / 16777216.f;
		}

		static float StepHz(int32 Step, float Key)
		{
			const int32 Octave = Step >= 0 ? Step / 7 : (Step - 6) / 7;
			const int32 Inside = Step - Octave * 7;
			return Key * FMath::Pow(2.f, (Scale[Inside] + 12 * Octave) / 12.f);
		}

		/** Makes the tune for a phrase: a walk mostly by steps, leaning to the chord's notes, ending home. */
		void MakeTune(int32 Which)
		{
			int32 Step = 7;
			for (int32 n = 0; n < 64; ++n)
			{
				const int32 Root = ChordRoot[(n / 16) % 4];
				const float R = TuneDice(Which, n);
				if (n % 2 == 0 && R < 0.35f)
				{
					// A chord note: the root, third or fifth, near where the tune is.
					const int32 ChordNotes[3] = { Root, Root + 2, Root + 4 };
					const int32 Pick = ChordNotes[static_cast<int32>(TuneDice(Which, n + 100) * 3.f) % 3];
					Step = Pick + 7 * FMath::RoundToInt((Step - Pick) / 7.f);
				}
				else
				{
					Step += R < 0.55f ? -1 : (R < 0.9f ? 1 : (R < 0.95f ? 2 : -2));
				}
				Step = FMath::Clamp(Step, 3, 13);
				Tune[n] = Step;
			}
			Tune[62] = 8;
			Tune[63] = 7;
		}

		void Strike(float Hz, float Loud)
		{
			FStrike& S = Strikes[NextStrike];
			NextStrike = (NextStrike + 1) % 6;
			S.Hz = Hz;
			S.Age = 0.f;
			S.Loud = Loud;
		}

		float Echo(float In)
		{
			const float A = EchoA[EchoAt];
			const float B = EchoB[EchoBt];
			EchoA[EchoAt] = In + 0.45f * B;
			EchoB[EchoBt] = In + 0.40f * A;
			EchoAt = (EchoAt + 1) % 2400;
			EchoBt = (EchoBt + 1) % 3100;
			return (A + B) * 0.5f;
		}

		float Play(bool bSmart, float Dt, float SampleRate)
		{
			const float Key = bSmart ? 233.08f : 220.f;
			const float BeatLength = bFast ? 0.30f : 0.62f;
			BeatTime += Dt;
			const bool bNewBeat = BeatTime >= BeatLength;
			if (bNewBeat)
			{
				BeatTime -= BeatLength;
				Beat = (Beat + 1) % 4;
				if (Beat == 0)
				{
					Bar = (Bar + 1) % 8;
					if (Bar == 0)
					{
						Phrase += 1;
						// Each tune is played twice; four phrases slow, then six fast.
						bFast = (Phrase % 10) >= 4;
						TuneSeed = Phrase / 2 + (bSmart ? 50 : 0);
						MakeTune(TuneSeed);
					}
				}
			}
			if (!bStarted)
			{
				bStarted = true;
				TuneSeed = bSmart ? 50 : 0;
				MakeTune(TuneSeed);
			}
			const int32 Half = BeatTime >= BeatLength * 0.5f ? 1 : 0;
			const int32 Note = FMath::Clamp(Bar * 8 + Beat * 2 + Half, 0, 63);
			const int32 Root = ChordRoot[(Bar / 2) % 4];

			// First violin: glides to each new note, sings with vibrato, swells on long notes in the slow part.
			ViolinAim = StepHz(Tune[Note] - 7, Key * 2.f);
			const float Glide = 1.f - FMath::Exp(-Dt / (bFast ? 0.012f : 0.05f));
			ViolinHz += (ViolinAim - ViolinHz) * Glide;
			VibratoPhase += Dt * (bFast ? 6.5f : 5.2f);
			VibratoPhase -= FMath::FloorToFloat(VibratoPhase);
			const float Vibrato = 1.f + (bFast ? 0.004f : 0.011f) * FMath::Sin(2.f * PI * VibratoPhase);
			ViolinPhase += ViolinHz * Vibrato * Dt;
			ViolinPhase -= FMath::FloorToFloat(ViolinPhase);
			const float NoteTime = BeatTime - Half * BeatLength * 0.5f;
			const float BowAim = bFast ? (NoteTime < 0.03f ? 1.f : 0.7f) : 0.6f + 0.4f * FMath::Sin(PI * FMath::Clamp(BeatTime / BeatLength, 0.f, 1.f));
			Bow += (BowAim - Bow) * (1.f - FMath::Exp(-Dt / 0.02f));
			const float Saw = ViolinPhase * 2.f - 1.f;
			const float Scrape = (Unit() * 2.f - 1.f) * 0.04f;
			float Violin = Body1.Run(Saw + Scrape, 2800.f, 2200.f, SampleRate) * 2.2f + Body2.Run(Saw, 480.f, 260.f, SampleRate) * 0.9f;
			Violin *= Bow;

			// The second violin (coffee house): long chord notes a third under the tune.
			float Second = 0.f;
			if (bSmart)
			{
				const float Hz = StepHz(Root + 2, Key);
				SecondPhase += Hz * Dt;
				SecondPhase -= FMath::FloorToFloat(SecondPhase);
				Second = Body3.Run(SecondPhase * 2.f - 1.f, 1500.f, 1800.f, SampleRate) * 0.9f * (bFast && Beat % 2 == 1 ? 1.f : 0.45f);
			}

			// The clarinet (coffee house): the tune an octave down in the fast part, weaving in the slow.
			float Clarinet = 0.f;
			if (bSmart)
			{
				const int32 ClarinetNote = bFast ? Tune[Note] - 7 : Tune[(Note + 4) % 64] - 9;
				const float Hz = StepHz(ClarinetNote, Key);
				ClarinetPhase += Hz * Dt;
				ClarinetPhase -= FMath::FloorToFloat(ClarinetPhase);
				const float Square = ClarinetPhase < 0.5f ? 1.f : -1.f;
				Clarinet = ClarinetBody.Run(Square, 1100.f, 1400.f, SampleRate) * 0.5f;
			}

			// The cimbalom: a tremolo on the chord in the slow part, the "tam" after each bass note in the fast.
			if (bFast)
			{
				if (bNewBeat && Beat % 2 == 1)
				{
					Strike(StepHz(Root, Key), 0.8f);
					Strike(StepHz(Root + 2, Key), 0.6f);
					Strike(StepHz(Root + 4, Key), 0.6f);
				}
				if (bNewBeat && Unit() < 0.35f)
				{
					// A run of the tune's notes now and then.
					Strike(StepHz(Tune[Note], Key), 0.5f);
				}
			}
			else
			{
				TremoloTime += Dt;
				if (TremoloTime >= 0.075f)
				{
					TremoloTime = 0.f;
					const int32 Pick = static_cast<int32>(Unit() * 3.f) % 3;
					Strike(StepHz(Root + 2 * Pick, Key), 0.32f + 0.1f * Unit());
				}
			}
			float Cimbalom = 0.f;
			for (FStrike& S : Strikes)
			{
				if (S.Age > 2.5f)
				{
					continue;
				}
				S.Age += Dt;
				const float Ratios[3] = { 1.f, 2.003f, 3.01f };
				const float Decays[3] = { 0.9f, 0.35f, 0.15f };
				const float Levels[3] = { 1.f, 0.5f, 0.3f };
				for (int32 p = 0; p < 3; ++p)
				{
					S.Phase[p] += S.Hz * Ratios[p] * Dt;
					S.Phase[p] -= FMath::FloorToFloat(S.Phase[p]);
					Cimbalom += FMath::Sin(2.f * PI * S.Phase[p]) * Levels[p] * FMath::Exp(-S.Age / Decays[p]) * S.Loud;
				}
				// The hammer's soft knock.
				Cimbalom += (S.Age < 0.004f ? (Unit() * 2.f - 1.f) * 0.3f * S.Loud : 0.f);
			}

			// The bass: plucked on the first and third beats in the fast part, bowed long notes in the slow.
			if (bNewBeat && (bFast ? Beat % 2 == 0 : Beat == 0))
			{
				BassHz = StepHz(Beat == 2 ? Root + 4 : Root, Key * 0.25f);
				BassAge = 0.f;
			}
			BassAge += Dt;
			BassPhase += BassHz * Dt;
			BassPhase -= FMath::FloorToFloat(BassPhase);
			const float BassWave = FMath::Sin(2.f * PI * BassPhase) + 0.3f * FMath::Sin(4.f * PI * BassPhase);
			const float Bass = BassWave * (bFast ? FMath::Exp(-BassAge / 0.22f) : 0.6f * FMath::Min(1.f, BassAge / 0.15f));

			return 0.3f * (0.30f * Violin + 0.16f * Second + 0.16f * Clarinet + 0.20f * Cimbalom + 0.34f * Bass);
		}
	};
}
