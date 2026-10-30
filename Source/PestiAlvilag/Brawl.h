// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FreeRun.h"

/**
 * Fist fighting in the street, after Sleeping Dogs but rougher: no kung fu,
 * a 1932 brawl. A quick jab, a slow heavy swing that breaks a guard, a guard
 * held up, a counter when the other one pulls his fist back (a gold key sign
 * says when), grabbing him by the collar to knee him or shove him into a
 * wall or a stall, and getting tired: every blow costs breath, and a man out
 * of breath stands bent over and can't fight back for a moment.
 *
 * Both sides fight by the same rules (APestiPerson): the player with the
 * keys, the other man with his own head (AChaseThief). The poses are
 * header-only like FreeRun's, so the offline preview shows the same moves.
 */
namespace Brawl
{
	/** What a fighter is doing. */
	enum class EAct : uint8
	{
		None,       // not fighting
		Stance,     // fists up, ready
		Guard,      // covering up
		Jab,
		HeavyWind,  // pulling back for a heavy swing: the moment to counter
		HeavySwing,
		Counter,    // ducking under his swing and hooking back
		GrabReach,  // reaching for his collar: the moment to counter, too
		Holding,    // has him by the collar
		Knee,       // a knee while holding him
		Shove,      // throwing him off
		Held,       // held by the collar
		Hurt,       // a blow got through
		Stagger,    // a heavy blow, a counter, a broken guard
		Shoved,     // thrown off, flying back
		Down,       // falling and lying on the ground
		GetUp,
		Winded,     // out of breath
		Tackle,     // diving at a runner (the player catching the thief)
		Tackled,    // brought down from behind
		Beaten,     // sitting on the ground, hands up: the fight is over
	};

	/** What landed. */
	enum class EBlow : uint8 { Jab, Heavy, Counter, Knee, Slam, Throw };

	// How long each act lasts (s), and how far into it the blow lands (share).
	constexpr float JabSeconds = 0.38f;
	constexpr float JabLands = 0.42f;
	constexpr float HeavyWindSeconds = 0.55f;
	constexpr float HeavySwingSeconds = 0.32f;
	constexpr float HeavyLands = 0.4f;
	constexpr float CounterSeconds = 0.55f;
	constexpr float CounterLands = 0.55f;
	constexpr float GrabReachSeconds = 0.3f;
	/** The other man's reach for a collar is slower, so it can be seen coming. */
	constexpr float SlowGrabReachSeconds = 0.5f;
	constexpr float HoldSeconds = 2.4f;
	constexpr float KneeSeconds = 0.42f;
	constexpr float KneeLands = 0.5f;
	constexpr int32 MaxKnees = 3;
	constexpr float ShoveSeconds = 0.35f;
	constexpr float ShoveLets = 0.4f;
	constexpr float HurtSeconds = 0.32f;
	constexpr float StaggerSeconds = 0.7f;
	constexpr float ShovedSeconds = 0.9f;
	constexpr float DownSeconds = 2.2f;
	constexpr float FallShare = 0.3f;   // of Down: falling; the rest lying there
	constexpr float GetUpSeconds = 0.8f;
	constexpr float WindedSeconds = 1.5f;
	constexpr float TackleSeconds = 1.0f;
	constexpr float TackledSeconds = 2.0f;

	/** The counter key lights up gold this long before his swing comes (s). */
	constexpr float CounterWindow = 0.4f;
	/** Presses that free a man held by the collar. */
	constexpr int32 BreakFreePresses = 4;

	// Reach: a blow lands if he is this close (cm, middle to middle) and
	// this far round from straight ahead (degrees).
	constexpr double Reach = 150.0;
	constexpr double ReachAngle = 55.0;
	constexpr double GrabDistance = 135.0;
	constexpr double HoldGap = 82.0;
	// Fighters keep this far apart, and step in to this when attacking (cm).
	constexpr double KeepApart = 75.0;
	// Speeds in a fight (cm/s): moving with the fists up, covering up, being thrown.
	constexpr float FightSpeed = 240.f;
	constexpr float GuardSpeed = 120.f;
	constexpr float ThrowSpeed = 950.f;
	constexpr float ThrowLift = 180.f;
	constexpr float TurnRate = 720.f;
	// Tackling: a runner this close is brought down (cm), if the one behind
	// comes this fast at least (cm/s).
	constexpr double TackleReach = 140.0;
	constexpr float TackleSpeed = 450.f;

	// Breath (stamina), out of 100: what each move costs, how fast it comes
	// back after a short rest (per s), what a blocked blow takes, and what is
	// left after getting his breath back.
	constexpr float MaxStamina = 100.f;
	constexpr float JabCost = 9.f;
	constexpr float HeavyCost = 20.f;
	constexpr float CounterCost = 6.f;
	constexpr float GrabCost = 12.f;
	constexpr float KneeCost = 8.f;
	constexpr float ShoveCost = 10.f;
	constexpr float BlockCost = 6.f;
	constexpr float GuardDrain = 4.f;
	constexpr float StaminaBack = 25.f;
	constexpr float RestBefore = 0.7f;
	constexpr float AfterWinded = 40.f;

	/** How much health a blow takes (out of 100); a winded man takes more. */
	inline float Damage(EBlow Blow)
	{
		switch (Blow)
		{
		case EBlow::Jab: return 8.f;
		case EBlow::Heavy: return 18.f;
		case EBlow::Counter: return 20.f;
		case EBlow::Knee: return 9.f;
		case EBlow::Slam: return 20.f;
		case EBlow::Throw: return 5.f;
		default: return 0.f;
		}
	}
	constexpr float WindedDamage = 1.5f;
	/** A heavy swing into a guard still gets this share through. */
	constexpr float GuardBreakShare = 0.4f;

	/** True for the acts he can't be hit out of, and is not hit in (on the ground). */
	inline bool IsOnGround(EAct Act)
	{
		return Act == EAct::Down || Act == EAct::GetUp || Act == EAct::Tackled || Act == EAct::Beaten;
	}

	/** True while he can start something new. */
	inline bool IsFree(EAct Act)
	{
		return Act == EAct::Stance || Act == EAct::Guard;
	}

	using FreeRun::FPose;
	using FreeRun::Smooth;

	/** Up and out again over a move: 0 at its ends, 1 between In and Out. */
	inline float Bell(float T, float In, float Out)
	{
		return Smooth(0.f, In, T) * (1.f - Smooth(Out, 1.f, T));
	}

	inline FPose Mix(const FPose& A, const FPose& B, float W)
	{
		auto L = [W](float X, float Y) { return X + (Y - X) * W; };
		return { L(A.Lift, B.Lift), L(A.Pitch, B.Pitch), L(A.Yaw, B.Yaw), L(A.Roll, B.Roll), L(A.LegL, B.LegL), L(A.LegR, B.LegR),
			L(A.KneeL, B.KneeL), L(A.KneeR, B.KneeR), L(A.ArmL, B.ArmL), L(A.ArmR, B.ArmR), L(A.ArmOutL, B.ArmOutL), L(A.ArmOutR, B.ArmOutR),
			L(A.ElbowL, B.ElbowL), L(A.ElbowR, B.ElbowR) };
	}

	/**
	 * Fists up: the left shoulder leading, knees soft, elbows in, the left
	 * fist out in front of the chin and the right one tucked in by it.
	 */
	inline FPose Stance()
	{
		FPose P;
		P.Lift = -5.f;
		P.Pitch = -6.f;
		P.Yaw = 18.f;
		P.LegL = 9.f;
		P.LegR = -8.f;
		P.KneeL = -14.f;
		P.KneeR = -18.f;
		// Upper arms down and forward, elbows bent so the fists are up by the chin.
		P.ArmL = 56.f;
		P.ArmOutL = -18.f;
		P.ElbowL = 104.f;
		P.ArmR = 46.f;
		P.ArmOutR = -26.f;
		P.ElbowR = 128.f;
		return P;
	}

	/** Covering up: hunched, both forearms up across the face. */
	inline FPose Guard()
	{
		FPose P = Stance();
		P.Lift = -10.f;
		P.Pitch = -14.f;
		P.Yaw = 8.f;
		P.LegL = 20.f;
		P.LegR = 4.f;
		P.KneeL = -22.f;
		P.KneeR = -26.f;
		P.ArmL = 62.f;
		P.ArmOutL = -30.f;
		P.ElbowL = 128.f;
		P.ArmR = 58.f;
		P.ArmOutR = -32.f;
		P.ElbowR = 132.f;
		return P;
	}

	/** A jab at T: the left fist straight out and back, the shoulder behind it. */
	inline FPose Jab(float T)
	{
		FPose Out = Stance();
		Out.Yaw = 34.f;
		Out.Pitch = -10.f;
		Out.ArmL = 94.f;
		Out.ArmOutL = -8.f;
		Out.ElbowL = 6.f;
		Out.LegL = 22.f;
		return Mix(Stance(), Out, Bell(T, 0.35f, 0.6f));
	}

	/** Pulling back for a heavy swing at T: the right fist drawn back, the body wound round. */
	inline FPose HeavyWind(float T)
	{
		FPose Back = Stance();
		Back.Yaw = -32.f;
		Back.Pitch = 6.f;
		// The right fist drawn back behind the shoulder, the left out in front measuring him.
		Back.ArmR = -45.f;
		Back.ArmOutR = 60.f;
		Back.ElbowR = 100.f;
		Back.ArmL = 75.f;
		Back.ArmOutL = -10.f;
		Back.ElbowL = 40.f;
		Back.LegL = 20.f;
		Back.KneeR = -28.f;
		return Mix(Stance(), Back, Smooth(0.f, 0.7f, T));
	}

	/** The heavy swing at T: round from the side, the whole body behind it. */
	inline FPose HeavySwing(float T)
	{
		FPose From = HeavyWind(1.f);
		FPose Through = Stance();
		Through.Yaw = 48.f;
		Through.Pitch = -18.f;
		Through.Lift = -10.f;
		// A big right hook: the upper arm out level to the side, the forearm
		// bent round in front, the turning body bringing it across.
		Through.ArmR = 10.f;
		Through.ArmOutR = 80.f;
		Through.ElbowR = 95.f;
		Through.ArmL = 40.f;
		Through.ArmOutL = -10.f;
		Through.ElbowL = 110.f;
		Through.LegL = 30.f;
		Through.KneeL = -26.f;
		Through.LegR = -22.f;
		const FPose Swung = Mix(From, Through, Smooth(0.f, 0.45f, T));
		return Mix(Swung, Stance(), Smooth(0.7f, 1.f, T));
	}

	/** A counter at T: down under his swing, then up with a left hook. */
	inline FPose Counter(float T)
	{
		FPose Duck = Stance();
		Duck.Lift = -26.f;
		Duck.Pitch = -24.f;
		Duck.Yaw = -14.f;
		Duck.KneeL = -42.f;
		Duck.KneeR = -46.f;
		Duck.LegL = 30.f;
		Duck.LegR = -6.f;
		FPose Hook = Stance();
		Hook.Lift = -8.f;
		Hook.Pitch = -10.f;
		Hook.Yaw = 44.f;
		Hook.ArmL = 10.f;
		Hook.ArmOutL = 80.f;
		Hook.ElbowL = 95.f;
		Hook.LegL = 18.f;
		const FPose Low = Mix(Stance(), Duck, Smooth(0.f, 0.3f, T));
		const FPose Up = Mix(Low, Hook, Smooth(0.38f, 0.6f, T));
		return Mix(Up, Stance(), Smooth(0.8f, 1.f, T));
	}

	/** Reaching for his collar at T: both hands out. */
	inline FPose GrabReach(float T)
	{
		FPose Out = Stance();
		Out.Yaw = 0.f;
		Out.Pitch = -14.f;
		Out.ArmL = 78.f;
		Out.ArmR = 78.f;
		Out.ArmOutL = -14.f;
		Out.ArmOutR = -14.f;
		Out.ElbowL = 32.f;
		Out.ElbowR = 32.f;
		Out.LegL = 26.f;
		return Mix(Stance(), Out, Smooth(0.f, 0.8f, T));
	}

	/** Holding him by the collar: both fists in it, leaning in. */
	inline FPose Holding()
	{
		return GrabReach(1.f);
	}

	/** A knee at T while holding him: up into his belly and down again. */
	inline FPose Knee(float T)
	{
		FPose Up = Holding();
		Up.Pitch = -6.f;
		Up.Lift = 4.f;
		Up.LegR = 92.f;
		Up.KneeR = -105.f;
		Up.LegL = -4.f;
		// Pulling him down onto it.
		Up.ArmL = 58.f;
		Up.ArmR = 58.f;
		Up.ElbowL = 62.f;
		Up.ElbowR = 62.f;
		return Mix(Holding(), Up, Bell(T, 0.45f, 0.6f));
	}

	/** Throwing him off at T: both arms thrust out, a step in after him. */
	inline FPose Shove(float T)
	{
		FPose Out = Holding();
		Out.Pitch = -20.f;
		Out.ArmL = 96.f;
		Out.ArmR = 96.f;
		Out.ArmOutL = -4.f;
		Out.ArmOutR = -4.f;
		Out.ElbowL = 4.f;
		Out.ElbowR = 4.f;
		Out.LegL = 34.f;
		Out.KneeL = -20.f;
		Out.LegR = -18.f;
		const FPose Thrust = Mix(Holding(), Out, Smooth(0.f, 0.4f, T));
		return Mix(Thrust, Stance(), Smooth(0.7f, 1.f, T));
	}

	/** Held by the collar at T (seconds): leaning back from it, clawing at the hands. */
	inline FPose Held(float Seconds)
	{
		const float Claw = FMath::Sin(Seconds * 9.f);
		FPose P;
		P.Pitch = 12.f;
		P.Lift = -4.f;
		P.LegL = -8.f;
		P.LegR = 10.f;
		P.KneeL = -10.f;
		P.KneeR = -16.f;
		P.ArmL = 56.f + 10.f * Claw;
		P.ArmR = 56.f - 10.f * Claw;
		P.ArmOutL = -24.f;
		P.ArmOutR = -24.f;
		P.ElbowL = 85.f - 15.f * Claw;
		P.ElbowR = 85.f + 15.f * Claw;
		return P;
	}

	/** A blow got through at T: the head snapped back and to one side, the guard dropped. */
	inline FPose Hurt(float T, float Side = 1.f)
	{
		FPose Hit = Stance();
		Hit.Pitch = 18.f;
		Hit.Yaw = 18.f - 22.f * Side;
		Hit.Roll = 8.f * Side;
		Hit.ArmL = 38.f;
		Hit.ArmR = 30.f;
		Hit.ArmOutL = 14.f;
		Hit.ArmOutR = 14.f;
		Hit.ElbowL = 40.f;
		Hit.ElbowR = 35.f;
		return Mix(Stance(), Hit, Bell(T, 0.2f, 0.55f));
	}

	/** Reeling back at T: a step back to stay up, arms out. */
	inline FPose Stagger(float T)
	{
		FPose Reel;
		Reel.Lift = -8.f;
		Reel.Pitch = 24.f;
		Reel.Roll = -6.f;
		Reel.LegL = 18.f;
		Reel.KneeL = -10.f;
		Reel.LegR = -26.f;
		Reel.KneeR = -34.f;
		Reel.ArmL = 34.f;
		Reel.ArmR = 46.f;
		Reel.ArmOutL = 52.f;
		Reel.ArmOutR = 44.f;
		Reel.ElbowL = 30.f;
		Reel.ElbowR = 25.f;
		return Mix(Mix(Stance(), Reel, Smooth(0.f, 0.25f, T)), Stance(), Smooth(0.75f, 1.f, T));
	}

	/** Flying back, thrown off: leaning back, arms flung forward. */
	inline FPose Shoved(float T)
	{
		FPose Fly = Stagger(0.4f);
		Fly.Pitch = 30.f;
		Fly.ArmL = 70.f;
		Fly.ArmR = 64.f;
		Fly.ArmOutL = 30.f;
		Fly.ArmOutR = 34.f;
		return Mix(Fly, Stance(), Smooth(0.8f, 1.f, T));
	}

	/** Lying on his back, knees up a little. */
	inline FPose Lying()
	{
		FPose P;
		P.Lift = -74.f;
		P.Pitch = 86.f;
		P.Roll = 6.f;
		P.LegL = 16.f;
		P.KneeL = -36.f;
		P.LegR = 4.f;
		P.KneeR = -10.f;
		P.ArmL = 30.f;
		P.ArmOutL = 50.f;
		P.ArmR = 12.f;
		P.ArmOutR = 30.f;
		P.ElbowL = 35.f;
		P.ElbowR = 15.f;
		return P;
	}

	/** Knocked down at T: over backwards, then lying there. */
	inline FPose Down(float T)
	{
		const float Fall = FMath::Clamp(T / FallShare, 0.f, 1.f);
		FPose Mid = Stagger(0.3f);
		const FPose Going = Mix(Mid, Lying(), Smooth(0.f, 1.f, Fall));
		// The legs go up a little as he lands.
		FPose P = Going;
		const float Bump = Bell(Fall, 0.7f, 0.85f) * (1.f - Smooth(0.95f, 1.f, Fall));
		P.LegL += 14.f * Bump;
		P.LegR += 18.f * Bump;
		return P;
	}

	/** Getting up at T: onto a knee, then up into his stance. */
	inline FPose GetUp(float T)
	{
		FPose Kneel;
		Kneel.Lift = -48.f;
		Kneel.Pitch = -12.f;
		Kneel.LegL = 86.f;
		Kneel.KneeL = -88.f;
		Kneel.LegR = -4.f;
		Kneel.KneeR = -112.f;
		Kneel.ArmL = 40.f;
		Kneel.ArmR = 22.f;
		Kneel.ArmOutR = 16.f;
		Kneel.ElbowL = 30.f;
		Kneel.ElbowR = 20.f;
		const FPose First = Mix(Lying(), Kneel, Smooth(0.f, 0.5f, T));
		return Mix(First, Stance(), Smooth(0.5f, 1.f, T));
	}

	/** Out of breath at T (seconds): bent over, hands on his knees, heaving. */
	inline FPose Winded(float Seconds)
	{
		const float Heave = FMath::Sin(Seconds * 7.f);
		FPose P;
		// Bent forward from the hips, the thighs kept under him.
		P.Lift = -16.f + 2.f * Heave;
		P.Pitch = -38.f + 3.f * Heave;
		P.LegL = 46.f;
		P.LegR = 36.f;
		P.KneeL = -36.f;
		P.KneeR = -30.f;
		P.ArmL = 18.f;
		P.ArmR = 18.f;
		P.ArmOutL = -8.f;
		P.ArmOutR = -8.f;
		P.ElbowL = 8.f;
		P.ElbowR = 8.f;
		return P;
	}

	/** Diving at a runner at T: arms out, flat out through the air, then onto a knee. */
	inline FPose Tackle(float T)
	{
		FPose Dive;
		Dive.Lift = -36.f;
		Dive.Pitch = -64.f;
		Dive.LegL = -20.f;
		Dive.LegR = -34.f;
		Dive.KneeL = -20.f;
		Dive.KneeR = -14.f;
		Dive.ArmL = 150.f;
		Dive.ArmR = 146.f;
		Dive.ArmOutL = -10.f;
		Dive.ArmOutR = -10.f;
		FPose Kneel = GetUp(0.5f);
		Kneel.Pitch = -20.f;
		Kneel.ArmL = 60.f;
		Kneel.ArmR = 60.f;
		Kneel.ElbowL = 50.f;
		Kneel.ElbowR = 50.f;
		const FPose Going = Mix(Stance(), Dive, Smooth(0.f, 0.25f, T));
		const FPose Kneeling = Mix(Going, Kneel, Smooth(0.4f, 0.6f, T));
		return Mix(Kneeling, Stance(), Smooth(0.8f, 1.f, T));
	}

	/** Brought down from behind at T: face first, then over and up. */
	inline FPose Tackled(float T)
	{
		FPose Flat;
		Flat.Lift = -76.f;
		Flat.Pitch = -84.f;
		Flat.LegL = -8.f;
		Flat.LegR = -2.f;
		Flat.KneeL = -30.f;
		Flat.KneeR = -12.f;
		Flat.ArmL = 150.f;
		Flat.ArmR = 120.f;
		Flat.ArmOutL = 24.f;
		Flat.ArmOutR = 30.f;
		FPose Lurch = Stance();
		Lurch.Pitch = -34.f;
		Lurch.ArmL = 120.f;
		Lurch.ArmR = 110.f;
		Lurch.LegL = 30.f;
		Lurch.LegR = -20.f;
		const FPose Falling = Mix(Mix(Stance(), Lurch, Smooth(0.f, 0.08f, T)), Flat, Smooth(0.06f, 0.24f, T));
		return Mix(Falling, GetUp(0.5f), Smooth(0.62f, 0.85f, T));
	}

	/** Beaten: sitting on the ground leaning back, both hands up, giving in. */
	inline FPose Beaten(float Seconds)
	{
		const float Shake = FMath::Sin(Seconds * 5.f);
		FPose P;
		P.Lift = -62.f;
		P.Pitch = 22.f;
		P.LegL = 84.f;
		P.LegR = 70.f;
		P.KneeL = -26.f;
		P.KneeR = -54.f;
		// Upper arms out, forearms up, palms to him.
		P.ArmL = 80.f + 6.f * Shake;
		P.ArmR = 76.f - 6.f * Shake;
		P.ArmOutL = 40.f;
		P.ArmOutR = 40.f;
		P.ElbowL = 85.f;
		P.ElbowR = 85.f;
		return P;
	}

	/** The pose for an act, T being its share done (Seconds, for the ones that go on). */
	inline FPose PoseFor(EAct Act, float T, float Seconds, float Side = 1.f)
	{
		switch (Act)
		{
		case EAct::Guard: return Guard();
		case EAct::Jab: return Jab(T);
		case EAct::HeavyWind: return HeavyWind(T);
		case EAct::HeavySwing: return HeavySwing(T);
		case EAct::Counter: return Counter(T);
		case EAct::GrabReach: return GrabReach(T);
		case EAct::Holding: return Holding();
		case EAct::Knee: return Knee(T);
		case EAct::Shove: return Shove(T);
		case EAct::Held: return Held(Seconds);
		case EAct::Hurt: return Hurt(T, Side);
		case EAct::Stagger: return Stagger(T);
		case EAct::Shoved: return Shoved(T);
		case EAct::Down: return Down(T);
		case EAct::GetUp: return GetUp(T);
		case EAct::Winded: return Winded(Seconds);
		case EAct::Tackle: return Tackle(T);
		case EAct::Tackled: return Tackled(T);
		case EAct::Beaten: return Beaten(Seconds);
		default: return Stance();
		}
	}
}
