// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FreeRun.h"
#include "PersonKit.h"

/**
 * How a man on foot moves his body when he is not fighting or vaulting: the
 * walk, the run and the sprint, each with its own stride; the body rising and
 * dropping with the steps, swaying over the foot that bears him, the shoulders
 * turning against the legs; leaning into a start, back when he pulls up, and
 * into a bend; little steps when he turns on the spot; tucked legs in a jump
 * and a dip of the knees when he lands; and standing, his breath and now and
 * then his weight going over to the other foot.
 *
 * It also tells when a foot comes down, for the footsteps (UFootSound).
 *
 * Plain C++ with no engine objects (like FightAnim), so it can be tried
 * outside the game. Keep one FWalker per man and call Update every frame.
 */
namespace WalkAnim
{
	/** One cycle of the stride (two steps) covers this far at a walk, and this much further at a sprint (cm). */
	constexpr float WalkCycle = 230.f;
	constexpr float SprintCycleExtra = 130.f;
	/** Thigh swing (degrees) at a walk, and how much more at a sprint. */
	constexpr float WalkThigh = 27.f;
	constexpr float SprintThighExtra = 21.f;
	/** The body rising and dropping with each step (cm): walking, running. */
	constexpr float WalkBob = 1.6f;
	constexpr float RunBob = 3.2f;
	/** Leaning forward (degrees) at a walk, at a sprint, and most from speeding up or pulling up. */
	constexpr float WalkLean = 2.5f;
	constexpr float SprintLean = 10.f;
	constexpr float MostPushLean = 9.f;
	/** Speeding up (or slowing down) this fast (cm/s each second) gives the full push lean. */
	constexpr float FullPush = 1800.f;
	/** Swaying over the bearing foot, and the shoulders turning against the legs (degrees). */
	constexpr float WalkSway = 1.6f;
	constexpr float WalkTwist = 5.f;
	constexpr float SprintTwistExtra = 5.f;
	/** Leaning into a bend (degrees, at most) and the turn rate (degrees a second) that gives it at a sprint. */
	constexpr float MostBank = 11.f;
	constexpr float FullBankTurn = 260.f;
	/** Elbows: hanging, walking, running (degrees). */
	constexpr float HangElbow = 8.f;
	constexpr float WalkElbow = 18.f;
	constexpr float RunElbow = 82.f;
	/** TurnSpeed on the spot faster than this (degrees a second) makes him step round. */
	constexpr float ShuffleTurn = 90.f;
	/** Standing: one breath lasts this long (s); the weight goes over to the other foot every so often (s). */
	constexpr float BreathSeconds = 3.8f;
	constexpr float ShiftEvery = 6.f;

	/** What the walker needs to know each frame. */
	struct FInput
	{
		float Dt = 0.f;
		/** How fast he goes over the ground (cm/s). */
		float Speed = 0.f;
		/** His walking pace (the plain W pace) and his sprint (cm/s). */
		float WalkPace = 330.f;
		float SprintPace = 750.f;
		/** How fast he turns (degrees a second, positive to the right). */
		float TurnRate = 0.f;
		/** Off the ground in a jump or a fall, and how fast he goes up (cm/s, negative falling). */
		bool bInAir = false;
		float UpSpeed = 0.f;
		/** Sitting (on a tram seat and the like): keep still. */
		bool bSeated = false;
	};

	/** Eases Current towards Goal, Rate times a second. */
	inline float Ease(float Current, float Goal, float Dt, float Rate)
	{
		return Current + (Goal - Current) * (1.f - FMath::Exp(-Rate * Dt));
	}

	/** Between pose A (W 0) and pose B (W 1). */
	inline FreeRun::FPose BlendPoses(const FreeRun::FPose& A, const FreeRun::FPose& B, float W)
	{
		FreeRun::FPose P;
		P.Lift = FMath::Lerp(A.Lift, B.Lift, W);
		P.Pitch = FMath::Lerp(A.Pitch, B.Pitch, W);
		P.Yaw = FMath::Lerp(A.Yaw, B.Yaw, W);
		P.Roll = FMath::Lerp(A.Roll, B.Roll, W);
		P.LegL = FMath::Lerp(A.LegL, B.LegL, W);
		P.LegR = FMath::Lerp(A.LegR, B.LegR, W);
		P.KneeL = FMath::Lerp(A.KneeL, B.KneeL, W);
		P.KneeR = FMath::Lerp(A.KneeR, B.KneeR, W);
		P.ArmL = FMath::Lerp(A.ArmL, B.ArmL, W);
		P.ArmR = FMath::Lerp(A.ArmR, B.ArmR, W);
		P.ArmOutL = FMath::Lerp(A.ArmOutL, B.ArmOutL, W);
		P.ArmOutR = FMath::Lerp(A.ArmOutR, B.ArmOutR, W);
		P.ElbowL = FMath::Lerp(A.ElbowL, B.ElbowL, W);
		P.ElbowR = FMath::Lerp(A.ElbowR, B.ElbowR, W);
		return P;
	}

	struct FWalker
	{
		/** A different seed for each man, so two standing side by side don't breathe together. */
		void SetSeed(float InSeed) { Seed = InSeed; ShiftClock = InSeed; }

		/** He lands from a jump or a fall coming down at DropSpeed (cm/s): his knees give. */
		void Land(float DropSpeed)
		{
			Dip = FMath::Max(Dip, FMath::Clamp(DropSpeed / 700.f, 0.25f, 1.3f));
			DipClock = 0.f;
			DipLasts = 0.22f + 0.25f * FMath::Clamp(Dip - 0.5f, 0.f, 1.f);
			StepCount = static_cast<int32>(FMath::FloorToFloat((Phase - 0.5f * UE_PI) / UE_PI));
		}

		/**
		 * The body's pose this frame. LegTwist turns the legs about the hips
		 * (as PersonModel::PoseLegs takes it); RootShift moves the whole body
		 * so it leans about its feet rather than its middle (add it to the
		 * pose root's place). Step comes back 1 when the left foot comes down
		 * this frame, 2 for the right, 0 for none, with how hard (0..1).
		 */
		FreeRun::FPose Update(const FInput& In, float& LegTwist, FVector& RootShift, int32& Step, float& StepStrength)
		{
			const float Dt = FMath::Max(In.Dt, 0.f);
			Step = 0;
			StepStrength = 0.f;
			FreeRun::FPose Out;
			LegTwist = 0.f;
			RootShift = FVector::ZeroVector;
			if (In.bSeated)
			{
				Gait = Run = Air = Dip = Bank = Push = 0.f;
				return Out;
			}
			Clock += Dt;

			// How far into walking (0 to 1 at the walking pace) and from there into a sprint (0 to 1).
			const float GroundSpeed = In.bInAir ? 0.f : In.Speed;
			const float TurnSpeed = FMath::Abs(In.TurnRate);
			const float Shuffle = GroundSpeed < 60.f ? FMath::Clamp((TurnSpeed - ShuffleTurn) / 300.f, 0.f, 1.f) * 0.35f : 0.f;
			Gait = Ease(Gait, FMath::Max(FMath::Clamp(GroundSpeed / In.WalkPace, 0.f, 1.f), Shuffle), Dt, 9.f);
			Run = Ease(Run, FMath::Clamp((GroundSpeed - In.WalkPace) / FMath::Max(In.SprintPace - In.WalkPace, 1.f), 0.f, 1.f), Dt, 6.f);
			Air = Ease(Air, In.bInAir ? 1.f : 0.f, Dt, In.bInAir ? 10.f : 18.f);

			// Speeding up or pulling up, felt as a lean.
			const float SpeedChange = Dt > 0.f ? (GroundSpeed - LastSpeed) / Dt : 0.f;
			LastSpeed = GroundSpeed;
			Push = Ease(Push, FMath::Clamp(SpeedChange / FullPush, -1.f, 1.f), Dt, 5.f);
			Bank = Ease(Bank, FMath::Clamp(In.TurnRate / FullBankTurn, -1.f, 1.f) * FMath::Clamp(GroundSpeed / In.SprintPace, 0.f, 1.f), Dt, 6.f);

			// The stride: longer and quicker the faster he goes; small quick steps turning on the spot.
			const float Cycle = WalkCycle + SprintCycleExtra * Run;
			Phase += Dt * (GroundSpeed / Cycle * 2.f * UE_PI + Shuffle * 9.f);
			if (Phase > 1000.f * UE_PI)
			{
				Phase -= 1000.f * UE_PI;
				StepCount -= 1000;
			}
			const float S = FMath::Sin(Phase);
			const float C = FMath::Cos(Phase);

			// A foot comes down as its thigh is furthest forward: the left at a quarter of the cycle, the right at three quarters.
			const int32 StepsTaken = static_cast<int32>(FMath::FloorToFloat((Phase - 0.5f * UE_PI) / UE_PI));
			if (StepsTaken != StepCount)
			{
				if (Gait > 0.12f && !In.bInAir)
				{
					Step = (StepsTaken & 1) == 0 ? 1 : 2;
					StepStrength = FMath::Clamp(0.25f + 0.55f * Gait * FMath::Clamp(GroundSpeed / In.WalkPace, 0.f, 1.f) + 0.3f * Run, 0.f, 1.f);
				}
				StepCount = StepsTaken;
			}

			// Legs: the thigh's swing, reaching further ahead than behind at a run; the knees fold more running.
			const float ThighSwing = Gait * (WalkThigh + SprintThighExtra * Run);
			const float ThighAhead = 6.f * Run * Gait;
			Out.LegL = S * ThighSwing + ThighAhead;
			Out.LegR = -S * ThighSwing + ThighAhead;
			const float Knees = Gait * (1.f + 0.85f * Run);
			Out.KneeL = PersonKit::KneeBend(Phase, Knees) - 14.f * Run * Gait * FMath::Max(0.f, -C);
			Out.KneeR = PersonKit::KneeBend(Phase + UE_PI, Knees) - 14.f * Run * Gait * FMath::Max(0.f, C);

			// The body: up at each passing step, down as the foot lands; lower and bouncier running.
			const float BodyRise = Gait * (WalkBob * (1.f - Run) * FMath::Cos(2.f * Phase) + RunBob * Run * FMath::Cos(2.f * Phase - 0.6f * UE_PI));
			Out.Lift = BodyRise - 2.5f * Run * Gait;
			Out.Pitch = -(Gait * (WalkLean + (SprintLean - WalkLean) * Run) + MostPushLean * Push) - 1.2f * Run * Gait * FMath::Cos(2.f * Phase);
			Out.Roll = Gait * WalkSway * (1.f - 0.6f * Run) * C + MostBank * Bank;
			// The shoulders turn against the legs; the hips a little with them, so the feet keep pointing ahead.
			Out.Yaw = -S * Gait * (WalkTwist + SprintTwistExtra * Run);
			LegTwist = -1.3f * Out.Yaw;

			// Arms against the legs, bent more and pumping when he runs.
			const float ArmReach = ThighSwing * (0.85f + 0.35f * Run);
			Out.ArmL = -S * ArmReach + 4.f * Run * Gait;
			Out.ArmR = S * ArmReach + 4.f * Run * Gait;
			Out.ArmOutL = 3.f + 2.f * Run;
			Out.ArmOutR = 3.f + 2.f * Run;
			const float ElbowBend = FMath::Lerp(HangElbow, FMath::Lerp(WalkElbow, RunElbow, Run), Gait);
			Out.ElbowL = ElbowBend + 14.f * Run * Gait * FMath::Max(0.f, -S);
			Out.ElbowR = ElbowBend + 14.f * Run * Gait * FMath::Max(0.f, S);

			// Standing: breathing, and the weight on one foot with the other knee easy, now and then changing over.
			const float Standing = 1.f - Gait;
			ShiftClock += Dt;
			if (ShiftClock > ShiftEvery + 2.f * FMath::Sin(Seed * 7.f + Clock * 0.37f))
			{
				ShiftClock = 0.f;
				ShiftSide = -ShiftSide;
			}
			Shift = Ease(Shift, ShiftSide, Dt, 2.5f);
			const float BreathNow = FMath::Sin(2.f * UE_PI * Clock / BreathSeconds + Seed);
			Out.Lift += Standing * (0.45f * BreathNow - 0.6f * FMath::Abs(Shift));
			Out.Pitch += Standing * 0.6f * BreathNow;
			Out.Roll += Standing * 1.6f * Shift;
			Out.KneeL += Standing * -7.f * FMath::Max(0.f, -Shift);
			Out.KneeR += Standing * -7.f * FMath::Max(0.f, Shift);
			Out.LegL += Standing * 3.f * FMath::Max(0.f, -Shift);
			Out.LegR += Standing * 3.f * FMath::Max(0.f, Shift);
			Out.ArmL += Standing * 1.5f * FMath::Sin(Clock * 0.9f + Seed);
			Out.ArmR += Standing * 1.5f * FMath::Sin(Clock * 0.8f + Seed + 2.f);

			// In the air: pushing off, then tucked at the top, then reaching down for the ground (JumpPose).
			if (In.bInAir && !bWasInAir)
			{
				// Taking off: a running leap leads with the leg that was swinging forward; a standing hop goes up on both.
				AirClock = 0.f;
				Leap = FMath::Clamp(In.Speed / In.WalkPace, 0.f, 1.f);
				LeadSide = FMath::Sin(Phase) >= 0.f ? 1.f : -1.f;
			}
			bWasInAir = In.bInAir;
			if (In.bInAir)
			{
				AirClock += Dt;
			}
			if (Air > 0.01f)
			{
				Out = BlendPoses(Out, JumpPose(In.UpSpeed, AirClock, Leap, LeadSide), Air);
				LegTwist *= 1.f - Air;
			}

			// Landing: the knees give and spring back.
			if (Dip > 0.001f)
			{
				const float Give = FMath::Sin(FMath::Min(DipClock / DipLasts, 1.f) * UE_PI) * Dip;
				Out.Lift -= 9.f * Give;
				Out.LegL += 18.f * Give;
				Out.LegR += 14.f * Give;
				Out.KneeL -= 36.f * Give;
				Out.KneeR -= 30.f * Give;
				Out.Pitch -= 7.f * Give;
				Out.ArmL += 10.f * Give;
				Out.ArmR += 10.f * Give;
				Out.ElbowL += 15.f * Give;
				Out.ElbowR += 15.f * Give;
				// A hard drop: down low, hands out in front to catch himself.
				const float Hard = FMath::Clamp(Dip - 0.6f, 0.f, 0.7f) * FMath::Sin(FMath::Min(DipClock / DipLasts, 1.f) * UE_PI);
				Out.Lift -= 14.f * Hard;
				Out.KneeL -= 40.f * Hard;
				Out.KneeR -= 40.f * Hard;
				Out.LegL += 25.f * Hard;
				Out.LegR += 25.f * Hard;
				Out.Pitch -= 14.f * Hard;
				Out.ArmL += 40.f * Hard;
				Out.ArmR += 40.f * Hard;
				Out.ArmOutL += 15.f * Hard;
				Out.ArmOutR += 15.f * Hard;
				DipClock += Dt;
				if (DipClock >= DipLasts)
				{
					Dip = 0.f;
					DipClock = 0.f;
				}
			}

			// Lean about the feet, not the middle: move the body so the feet stay under him.
			const FVector FootPoint(0.0, 0.0, -90.0);
			RootShift = FootPoint - FRotator(Out.Pitch, 0.f, Out.Roll).RotateVector(FootPoint);
			return Out;
		}

		/** Where he is in his stride (radians). */
		float GetPhase() const { return Phase; }

	private:
		/**
		 * The body in the air going up at UpSpeed (cm/s), AirSeconds after
		 * leaving the ground: LeapShare is 0 for a standing hop to 1 for a running
		 * leap, LeadLeg +1 when the left leg leads.
		 */
		static FreeRun::FPose JumpPose(float UpSpeed, float AirSeconds, float LeapShare, float LeadLeg)
		{
			// 0 just pushed off, 1 at the top, 2 coming down fast.
			const float ArcShare = FMath::Clamp(1.f - UpSpeed / 380.f, 0.f, 2.f);
			const float GoingUp = FMath::Clamp(1.f - ArcShare, 0.f, 1.f);
			const float AtTop = 1.f - FMath::Abs(ArcShare - 1.f);
			const float ComingDown = FMath::Clamp(ArcShare - 1.f, 0.f, 1.f);
			// Just off the ground the pushing leg is still straight behind him.
			const float PushOff = FMath::Clamp(1.f - AirSeconds / 0.14f, 0.f, 1.f) * GoingUp;
			const float LeadSwing = FMath::Lerp(28.f, 46.f, LeapShare);
			const float TrailSwing = FMath::Lerp(20.f, -28.f, LeapShare);
			// Tucked most at the top; a running leap is split from the start.
			const float Tuck = AtTop + FMath::Lerp(0.45f, 1.f, LeapShare) * GoingUp;
			const float LeadThigh = LeadSwing * Tuck + 18.f * ComingDown;
			const float TrailThigh = TrailSwing * Tuck + 4.f * ComingDown - 12.f * PushOff * LeapShare;
			const float LeadKnee = -FMath::Lerp(50.f, 70.f, LeapShare) * Tuck - 18.f * ComingDown;
			const float TrailKnee = -FMath::Lerp(50.f, 40.f, LeapShare) * AtTop - 12.f * ComingDown - 8.f * GoingUp * (1.f - PushOff);
			FreeRun::FPose P;
			P.LegL = LeadLeg > 0.f ? LeadThigh : TrailThigh;
			P.LegR = LeadLeg > 0.f ? TrailThigh : LeadThigh;
			P.KneeL = LeadLeg > 0.f ? LeadKnee : TrailKnee;
			P.KneeR = LeadLeg > 0.f ? TrailKnee : LeadKnee;
			// Arms: thrown up as he goes, against the legs in a leap, out wide for balance coming down.
			const float ArmThrow = 40.f * (GoingUp + 0.5f * AtTop);
			const float ArmLead = LeapShare * 22.f * (GoingUp + AtTop);
			P.ArmL = ArmThrow - LeadLeg * ArmLead + 10.f * ComingDown;
			P.ArmR = ArmThrow + LeadLeg * ArmLead + 10.f * ComingDown;
			P.ArmOutL = 8.f + 26.f * ComingDown;
			P.ArmOutR = 8.f + 26.f * ComingDown;
			P.ElbowL = 45.f - 15.f * ComingDown;
			P.ElbowR = 45.f - 15.f * ComingDown;
			P.Pitch = -FMath::Lerp(4.f, 10.f, LeapShare) * (GoingUp + AtTop) + 3.f * ComingDown;
			// A long drop: arms flung out higher.
			const float LongDrop = FMath::Clamp((-UpSpeed - 500.f) / 400.f, 0.f, 1.f);
			P.ArmOutL += 25.f * LongDrop;
			P.ArmOutR += 25.f * LongDrop;
			P.ArmL += 25.f * LongDrop;
			P.ArmR += 15.f * LongDrop;
			return P;
		}

		float Phase = 0.f;
		int32 StepCount = -1;
		float Gait = 0.f;
		float Run = 0.f;
		float Air = 0.f;
		float Push = 0.f;
		float Bank = 0.f;
		float LastSpeed = 0.f;
		float Dip = 0.f;
		float DipClock = 0.f;
		float DipLasts = 0.28f;
		bool bWasInAir = false;
		float AirClock = 0.f;
		float Leap = 0.f;
		float LeadSide = 1.f;
		float Clock = 0.f;
		float Seed = 0.f;
		float ShiftClock = 0.f;
		float ShiftSide = 1.f;
		float Shift = 0.f;
	};
}
