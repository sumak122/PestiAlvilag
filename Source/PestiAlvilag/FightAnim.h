// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Brawl.h"
#include "PersonKit.h"

/**
 * The layer between a fighter's moves (Brawl's poses) and his body: it makes
 * the fighting move like a body with weight instead of jumping from pose to pose.
 *
 *  - Melting: when one move cuts into another (a punch into the next, a
 *    punch broken by a blow), what was left of the old pose melts away in a
 *    blink instead of snapping. Quick for punches (they still come out at
 *    once), slower for settling back into the fists-up stance.
 *  - Weight: blows that land, blows taken on the guard and the jolt of one's
 *    own punch landing push the body like springs: the head snaps and
 *    wobbles back, the guard gets knocked down, and it all settles by itself
 *    on top of whatever move is playing.
 *  - Life: standing with the fists up he breathes (harder when out of
 *    breath), sways a little and keeps his fists moving; stepping about he
 *    leans the way he goes and his legs ease into a step and out of it.
 *  - Planted feet: as the body turns into a punch or winds up, the legs turn
 *    back against it so the feet stay where they stand (the twist is in the
 *    hips, not the shoes).
 *
 * Adding a new move (a lesson from Bandi, say): add it to Brawl::EAct, write
 * its pose (Brawl-style math, or a few key poses through Keyed below), add it
 * to Brawl::PoseFor, and give it a line in StyleFor if it should melt in
 * faster or slower than the usual or not keep its feet. Everything here then
 * works for it by itself.
 *
 * Header-only, like Brawl, so the offline preview shows the same motion.
 */
namespace FightAnim
{
	using FreeRun::FPose;

	constexpr int32 NumChannels = 14;

	/** Each number of a pose, by index (0 to NumChannels - 1). */
	inline float FPose::* ChannelOf(int32 Index)
	{
		static float FPose::* const Members[NumChannels] = {
			&FPose::Lift, &FPose::Pitch, &FPose::Yaw, &FPose::Roll, &FPose::LegL, &FPose::LegR, &FPose::KneeL, &FPose::KneeR,
			&FPose::ArmL, &FPose::ArmR, &FPose::ArmOutL, &FPose::ArmOutR, &FPose::ElbowL, &FPose::ElbowR };
		return Members[Index];
	}

	/** A + B * W, number by number. */
	inline FPose Plus(const FPose& A, const FPose& B, float W = 1.f)
	{
		FPose Out = A;
		for (int32 i = 0; i < NumChannels; ++i)
		{
			Out.*ChannelOf(i) += (B.*ChannelOf(i)) * W;
		}
		return Out;
	}

	/** A - B, number by number. */
	inline FPose Minus(const FPose& A, const FPose& B)
	{
		return Plus(A, B, -1.f);
	}

	/** How a key pose eases into the next one. */
	enum class EEase : uint8
	{
		Smooth, // slow out, slow in
		Snap,   // fast out, slowing at the end: a punch going out
		Slow,   // slow out, fast at the end: winding up, falling
		Linear,
	};

	inline float Eased(EEase Ease, float X)
	{
		X = FMath::Clamp(X, 0.f, 1.f);
		switch (Ease)
		{
		case EEase::Snap: return 1.f - (1.f - X) * (1.f - X) * (1.f - X);
		case EEase::Slow: return X * X * X;
		case EEase::Linear: return X;
		default: return X * X * (3.f - 2.f * X);
		}
	}

	/** One key pose of a move: the pose at this share of the move, and how it is reached from the key before. */
	struct FKey
	{
		float At = 0.f;
		FPose Pose;
		EEase Ease = EEase::Smooth;
	};

	/**
	 * A move made of key poses, at T (0 to 1): eased from each key to the
	 * next. Keys go in order of At; before the first and after the last the
	 * move holds that key's pose. For example a new punch:
	 *   const FightAnim::FKey Keys[] = { {0.f, Stance()}, {0.3f, Out, EEase::Snap}, {1.f, Stance()} };
	 *   return FightAnim::Keyed(Keys, T);
	 */
	template <int32 N>
	inline FPose Keyed(const FKey (&Keys)[N], float T)
	{
		if (T <= Keys[0].At)
		{
			return Keys[0].Pose;
		}
		for (int32 i = 1; i < N; ++i)
		{
			if (T <= Keys[i].At)
			{
				const float Span = Keys[i].At - Keys[i - 1].At;
				const float X = Span > KINDA_SMALL_NUMBER ? (T - Keys[i - 1].At) / Span : 1.f;
				return Brawl::Mix(Keys[i - 1].Pose, Keys[i].Pose, Eased(Keys[i].Ease, X));
			}
		}
		return Keys[N - 1].Pose;
	}

	/** How the layer treats a move. */
	struct FMoveStyle
	{
		/** How long what is left of the move before melts into this one (s). */
		float BlendIn = 0.08f;
		/** How much the feet stay put while the body turns (0 to 1): upright moves only. */
		float Plant = 1.f;
		/** Breathing, swaying and fists moving at rest (0 to 1). */
		float Life = 0.f;
		/** Stepping legs and leaning when he moves (0 to 1). */
		float Stride = 0.f;
	};

	/** The style of each move; anything not named here gets the plain one. */
	inline FMoveStyle StyleFor(Brawl::EAct Act)
	{
		using Brawl::EAct;
		FMoveStyle S;
		switch (Act)
		{
		case EAct::Stance: S.BlendIn = 0.22f; S.Life = 1.f; S.Stride = 1.f; break;
		case EAct::Guard: S.BlendIn = 0.12f; S.Life = 0.5f; S.Stride = 1.f; break;
		// Punches start at once; the melt only rounds off where the last one cut in.
		case EAct::Jab: S.BlendIn = 0.08f; break;
		case EAct::Cross:
		case EAct::Hook:
		case EAct::Uppercut: S.BlendIn = 0.1f; break;
		case EAct::Counter: S.BlendIn = 0.08f; break;
		case EAct::HeavyWind: S.BlendIn = 0.12f; break;
		case EAct::HeavySwing: S.BlendIn = 0.06f; break;
		case EAct::Rising:
		case EAct::Haymaker: S.BlendIn = 0.1f; break;
		case EAct::GrabReach: S.BlendIn = 0.07f; break;
		case EAct::Holding: S.BlendIn = 0.1f; break;
		case EAct::Knee:
		case EAct::Shove: S.BlendIn = 0.06f; break;
		case EAct::Held: S.BlendIn = 0.08f; break;
		// Hit: the head goes at once (the springs give the weight).
		case EAct::Hurt: S.BlendIn = 0.07f; break;
		case EAct::Stagger: S.BlendIn = 0.09f; break;
		case EAct::Winded: S.BlendIn = 0.18f; break;
		// On the ground or flying: the body goes where it is thrown.
		case EAct::Shoved: S.BlendIn = 0.05f; S.Plant = 0.f; break;
		case EAct::Down: S.BlendIn = 0.08f; S.Plant = 0.f; break;
		case EAct::GetUp: S.BlendIn = 0.12f; S.Plant = 0.f; break;
		case EAct::Tackle: S.BlendIn = 0.06f; S.Plant = 0.f; break;
		case EAct::Tackled: S.BlendIn = 0.05f; S.Plant = 0.f; break;
		case EAct::Beaten: S.BlendIn = 0.12f; S.Plant = 0.f; break;
		// A hop: out at once, the feet going with the body.
		case EAct::Sidestep: S.BlendIn = 0.07f; S.Plant = 0.f; break;
		default: break;
		}
		return S;
	}

	/** How hard a blow pushes the body it lands on (a jab about half a heavy swing). */
	inline float BlowWeight(Brawl::EBlow Blow)
	{
		using Brawl::EBlow;
		switch (Blow)
		{
		case EBlow::Jab: return 0.6f;
		case EBlow::Cross: return 0.7f;
		case EBlow::Hook: return 0.85f;
		case EBlow::Knee: return 0.9f;
		case EBlow::Counter: return 1.f;
		case EBlow::Throw: return 1.f;
		case EBlow::Uppercut: return 1.1f;
		case EBlow::Heavy: return 1.3f;
		case EBlow::Slam: return 1.4f;
		case EBlow::Rising: return 1.5f;
		case EBlow::Haymaker: return 1.6f;
		default: return 1.f;
		}
	}

	/** A number pushed off zero that springs back, wobbling a little (degrees or cm). */
	struct FSpring
	{
		float Value = 0.f;
		float Speed = 0.f;

		void Push(float Amount)
		{
			Speed += Amount;
		}

		/** Stiffness: how hard it pulls back; Ratio: 1 settles without a wobble, less wobbles. */
		void Tick(float Dt, float Stiffness, float Ratio)
		{
			const float Damping = 2.f * Ratio * FMath::Sqrt(Stiffness);
			const int32 Steps = FMath::Clamp(FMath::CeilToInt(Dt * 120.f), 1, 12);
			const float H = Dt / static_cast<float>(Steps);
			for (int32 i = 0; i < Steps; ++i)
			{
				Speed += (-Stiffness * Value - Damping * Speed) * H;
				Value += Speed * H;
			}
		}
	};

	/** Stepping about with the fists up. */
	constexpr float StrideLength = 70.f;
	constexpr float StrideSwing = 16.f;
	constexpr float StrideKnees = 0.8f;
	/** Leaning into a step at full fighting speed (degrees). */
	constexpr float LeanAhead = 7.f;
	constexpr float LeanAside = 4.f;
	/** The share of the body's turn the legs turn back, and the most they do (degrees). */
	constexpr float PlantShare = 0.6f;
	constexpr float MostTwist = 35.f;

	/** Eases Current toward Goal, Rate times a second. */
	inline float Approach(float Current, float Goal, float Dt, float Rate)
	{
		return Current + (Goal - Current) * (1.f - FMath::Exp(-Rate * Dt));
	}

	/** One fighter's moving body: keep one per fighter, call Begin as each move starts and Update every frame. */
	struct FAnimator
	{
		/** A new move starts: what is shown now melts into it. */
		void Begin(Brawl::EAct NewAct)
		{
			bFresh = true;
			BlendLength = StyleFor(NewAct).BlendIn;
		}

		/** Out of the fight: back to standing, the next fight starts fresh (the seed stays). */
		void Reset()
		{
			const float Kept = Seed;
			*this = FAnimator();
			Seed = Kept;
		}

		/** Gives this fighter his own rhythm of breathing and swaying. */
		void SetSeed(float InSeed)
		{
			Seed = InSeed;
		}

		/** Blows on the body: one that got through (Side as Brawl::Hurt's)... */
		void Hit(float Strength, float Side)
		{
			Pitch.Push(110.f * Strength);
			Yaw.Push(-120.f * Side * Strength);
			Roll.Push(60.f * Side * Strength);
			Lift.Push(-50.f * Strength);
			Arms.Push(-90.f * Strength);
		}

		/** ...one taken on the guard: the forearms knocked back into him... */
		void Block(float Strength)
		{
			Pitch.Push(70.f * Strength);
			Lift.Push(-40.f * Strength);
			Arms.Push(-150.f * Strength);
		}

		/** ...and his own punch landing: the jolt back up his arm. */
		void Recoil(float Strength)
		{
			Pitch.Push(35.f * Strength);
			Lift.Push(-25.f * Strength);
		}

		/**
		 * The pose to show this frame. Target is the move's own pose (Brawl::PoseFor),
		 * Ahead and Aside how fast he goes forward and to his right (cm/s), Tired
		 * how far out of breath he is (0 fresh, 1 empty).
		 */
		FPose Update(const FPose& Target, Brawl::EAct Act, float Dt, float Ahead, float Aside, float Tired)
		{
			Dt = FMath::Clamp(Dt, 0.f, 0.1f);
			Clock += Dt;
			const FMoveStyle Style = StyleFor(Act);

			// What was left of the move before, melting away (fast at first, gently at the end).
			if (bFresh)
			{
				bFresh = false;
				Offset = Minus(Shown, Target);
				BlendLeft = BlendLength;
			}
			FPose Out = Target;
			if (BlendLeft > 0.f && BlendLength > 0.f)
			{
				BlendLeft = FMath::Max(0.f, BlendLeft - Dt);
				const float X = BlendLeft / BlendLength;
				Out = Plus(Out, Offset, X * X);
			}

			// How much of each layer this move has, eased so nothing switches on or off at once.
			const float Speed = FMath::Sqrt(Ahead * Ahead + Aside * Aside);
			LifeWeight = Approach(LifeWeight, Style.Life, Dt, 6.f);
			StrideWeight = Approach(StrideWeight, Style.Stride * FMath::Clamp((Speed - 25.f) / 60.f, 0.f, 1.f), Dt, 10.f);
			PlantWeight = Approach(PlantWeight, Style.Plant, Dt, 12.f);
			LeanAheadNow = Approach(LeanAheadNow, Style.Stride * FMath::Clamp(Ahead / Brawl::FightSpeed, -1.f, 1.f), Dt, 8.f);
			LeanAsideNow = Approach(LeanAsideNow, Style.Stride * FMath::Clamp(Aside / Brawl::FightSpeed, -1.f, 1.f), Dt, 8.f);

			// Breathing (harder when out of breath), a slow sway, the fists never quite still.
			if (LifeWeight > 0.001f)
			{
				const float Breath = 2.3f + 2.8f * Tired;
				const float Deep = 1.f + 1.6f * Tired;
				const float In = FMath::Sin(Clock * Breath + Seed);
				const float Sway = FMath::Sin(Clock * 0.85f + Seed * 1.7f);
				const float Fists = Clock * 1.9f + Seed * 0.6f;
				const float W = LifeWeight;
				Out.Lift += W * 1.1f * Deep * In;
				Out.Pitch += W * (1.2f * Deep * FMath::Sin(Clock * Breath + Seed + 0.6f) - 3.f * Tired);
				Out.Yaw += W * 3.f * Sway;
				Out.Roll += W * 1.2f * FMath::Sin(Clock * 0.85f + Seed * 1.7f + 1.1f);
				Out.KneeL -= W * 2.f * (0.5f + 0.5f * In);
				Out.KneeR -= W * 2.f * (0.5f + 0.5f * In);
				Out.ArmL += W * 3.5f * FMath::Sin(Fists);
				Out.ElbowL += W * 4.f * FMath::Sin(Fists + 1.2f);
				Out.ArmR += W * 3.f * FMath::Sin(Fists + 2.1f);
				Out.ElbowR += W * 4.f * FMath::Sin(Fists + 3.f);
			}

			// Leaning into his steps, and the legs easing into stepping.
			Out.Pitch -= LeanAhead * LeanAheadNow;
			Out.Roll += LeanAside * LeanAsideNow;
			if (Speed > 1.f)
			{
				Phase += Dt * Speed / StrideLength * PI;
			}
			if (StrideWeight > 0.001f)
			{
				const float Thigh = FMath::Sin(Phase) * StrideSwing;
				const float W = StrideWeight;
				Out.LegL += (Thigh - 0.65f * Out.LegL) * W;
				Out.LegR += (-Thigh - 0.65f * Out.LegR) * W;
				Out.KneeL += (PersonKit::KneeBend(Phase, StrideKnees) - 0.5f * Out.KneeL) * W;
				Out.KneeR += (PersonKit::KneeBend(Phase + PI, StrideKnees) - 0.5f * Out.KneeR) * W;
			}

			// Blows on the body, springing back with a wobble.
			Pitch.Tick(Dt, 260.f, 0.38f);
			Yaw.Tick(Dt, 240.f, 0.4f);
			Roll.Tick(Dt, 260.f, 0.4f);
			Lift.Tick(Dt, 300.f, 0.5f);
			Arms.Tick(Dt, 320.f, 0.45f);
			Out.Pitch += Pitch.Value;
			Out.Yaw += Yaw.Value;
			Out.Roll += Roll.Value;
			Out.Lift += Lift.Value;
			Out.ArmL += Arms.Value;
			Out.ArmR += Arms.Value;
			Out.ElbowL -= 0.5f * Arms.Value;
			Out.ElbowR -= 0.5f * Arms.Value;

			// The feet stay put: the legs turn back against the body's turn away from the stance.
			LegTwist = FMath::Clamp(-(Out.Yaw - StanceYaw) * PlantShare * PlantWeight, -MostTwist, MostTwist);

			Shown = Out;
			return Out;
		}

		/** How far to turn the legs back this frame (degrees about the hips), after Update. */
		float GetLegTwist() const
		{
			return LegTwist;
		}

	private:
		/** The fists-up stance's turn: the feet are set for it. */
		static constexpr float StanceYaw = 18.f;

		FPose Shown;
		FPose Offset;
		bool bFresh = false;
		float BlendLength = 0.f;
		float BlendLeft = 0.f;
		float Clock = 0.f;
		float Seed = 0.f;
		float Phase = 0.f;
		float LifeWeight = 0.f;
		float StrideWeight = 0.f;
		float PlantWeight = 1.f;
		float LeanAheadNow = 0.f;
		float LeanAsideNow = 0.f;
		float LegTwist = 0.f;
		FSpring Pitch;
		FSpring Yaw;
		FSpring Roll;
		FSpring Lift;
		FSpring Arms;
	};
}
