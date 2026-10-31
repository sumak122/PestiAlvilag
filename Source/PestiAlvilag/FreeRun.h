// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;

/**
 * Running over and under things in a chase, as at the start of Sleeping
 * Dogs: with something low ahead (a crate, a cart, a stall's table) Space
 * vaults over it, with something to get under (a carpet hung on a rack, a
 * plank on trestles) Space slides through feet first. Pressed at the right
 * moment he keeps his speed and gains a little; pressed when he is nearly on
 * it, he gets over but stumbles.
 *
 * The poses are header-only and free of engine objects, so the offline
 * preview shows the same moves; FindMove looks at the world (FreeRun.cpp).
 */
namespace FreeRun
{
	enum class EMove : uint8 { None, Vault, Slide };

	/**
	 * A whole-body pose over the running one. The root turns the body about
	 * the hips (pitch back is positive, yaw to his right is positive, roll
	 * to his right is positive) and lifts or drops it (cm); legs and arms
	 * pitch forward when positive (and turn with the root: an arm forward on
	 * a body lying back points up), the
	 * knees take the foot back when negative, and ArmOut lifts an arm out
	 * to its side.
	 */
	struct FPose
	{
		float Lift = 0.f;
		float Pitch = 0.f;
		float Yaw = 0.f;
		float Roll = 0.f;
		float LegL = 0.f;
		float LegR = 0.f;
		float KneeL = 0.f;
		float KneeR = 0.f;
		float ArmL = 0.f;
		float ArmR = 0.f;
		float ArmOutL = 0.f;
		float ArmOutR = 0.f;
		/** Elbows bent (degrees, positive brings the forearm forward and up). */
		float ElbowL = 0.f;
		float ElbowR = 0.f;
	};

	inline FPose Scaled(const FPose& P, float W)
	{
		return { P.Lift * W, P.Pitch * W, P.Yaw * W, P.Roll * W, P.LegL * W, P.LegR * W, P.KneeL * W, P.KneeR * W,
			P.ArmL * W, P.ArmR * W, P.ArmOutL * W, P.ArmOutR * W, P.ElbowL * W, P.ElbowR * W };
	}

	inline float Smooth(float From, float To, float T)
	{
		const float X = FMath::Clamp((T - From) / (To - From), 0.f, 1.f);
		return X * X * (3.f - 2.f * X);
	}

	// What counts as something to vault: its top this high over his feet (cm).
	constexpr double MinVaultTop = 30.0;
	constexpr double MaxVaultTop = 125.0;
	// Something to slide under: the gap under it at least this high, and its
	// underside low enough that he could not just run under (cm).
	constexpr double MinSlideGap = 60.0;
	constexpr double MaxSlideGap = 170.0;
	// He looks this far ahead of his front for things (cm).
	constexpr double Reach = 260.0;
	// Pressed with his front this far from it, he gets over cleanly and
	// faster; nearer than Late, he stumbles after (cm).
	constexpr double GoodFrom = 70.0;
	constexpr double GoodTo = 200.0;
	constexpr double Late = 35.0;
	// The hips clear a vault's top by this much (cm).
	constexpr double VaultClear = 35.0;
	// The furthest he lands beyond a vault's near side, and slides on under something (cm).
	constexpr double MaxVaultDepth = 220.0;
	constexpr double MaxSlideDepth = 420.0;
	// Speed out of a clean move, against his running speed; a stumble
	// slows him to this share of his jog for this long (s).
	constexpr float CleanBoost = 1.12f;
	constexpr double BoostSeconds = 1.2;
	constexpr float StumbleSlow = 0.45f;
	constexpr double StumbleSeconds = 0.5;

	/** A move he could make from where he is. */
	struct FChance
	{
		EMove Move = EMove::None;
		/** Where his middle ends up, standing (cm, world). */
		FVector To = FVector::ZeroVector;
		/** For a vault: where his middle tops the arc (cm, world Z). */
		double PeakZ = 0.0;
		/** How far along the move the obstacle's middle is (0 to 1). */
		float Over = 0.5f;
		/** From his front to the obstacle (cm). */
		double Face = 0.0;
	};

	/**
	 * Looks along Dir from a standing capsule whose middle is at Middle:
	 * true and the move if there is something to vault or slide under
	 * within Reach, with room to come out the other side.
	 */
	bool FindMove(const UWorld* World, const AActor* Self, const FVector& Middle, const FVector& Dir,
		float CapsuleRadius, float CapsuleHalfHeight, FChance& Out);

	/**
	 * Vaulting at T (0 to 1), a speed vault: the right hand on top, the body
	 * leaning back onto it, the hips turned and both legs swung over
	 * together to the left of the hand, the left arm out for balance.
	 */
	inline FPose Vault(float T)
	{
		FPose Peak;
		Peak.Pitch = 24.f;
		Peak.Yaw = -38.f;
		Peak.Roll = 34.f;
		Peak.LegL = 80.f;
		Peak.LegR = 76.f;
		Peak.KneeL = -25.f;
		Peak.KneeR = -38.f;
		Peak.ArmR = -12.f;
		Peak.ArmOutR = 8.f;
		Peak.ArmL = 40.f;
		Peak.ArmOutL = 75.f;
		const float W = Smooth(0.f, 0.3f, T) * (1.f - Smooth(0.72f, 1.f, T));
		FPose P = Scaled(Peak, W);
		// Landing: the knees give a little as the feet come down.
		const float Land = Smooth(0.75f, 0.9f, T) * (1.f - Smooth(0.9f, 1.f, T));
		P.KneeL -= 30.f * Land;
		P.KneeR -= 30.f * Land;
		P.LegL += 20.f * Land;
		P.LegR += 20.f * Land;
		return P;
	}

	/** Sliding at T (0 to 1): down on his back, one leg out in front, the other tucked, a hand trailing on the ground. */
	inline FPose Slide(float T)
	{
		FPose Low;
		Low.Lift = -58.f;
		Low.Pitch = 70.f;
		Low.LegL = 18.f;
		Low.KneeL = -4.f;
		Low.LegR = 0.f;
		Low.KneeR = -75.f;
		Low.ArmL = -100.f;
		Low.ArmOutL = 25.f;
		Low.ArmR = 15.f;
		Low.ArmOutR = 18.f;
		const float W = Smooth(0.f, 0.22f, T) * (1.f - Smooth(0.74f, 1.f, T));
		FPose P = Scaled(Low, W);
		// Coming up: a lean forward to get back over his feet.
		const float Up = Smooth(0.74f, 0.88f, T) * (1.f - Smooth(0.88f, 1.f, T));
		P.Pitch -= 22.f * Up;
		P.LegR += 30.f * Up;
		P.KneeR -= 25.f * Up;
		return P;
	}

	/** Stumbling at T (0 to 1) after landing badly: a lurch forward, arms out. */
	inline FPose Stumble(float T)
	{
		const float W = FMath::Sin(UE_PI * FMath::Clamp(T, 0.f, 1.f));
		FPose P;
		P.Lift = -8.f * W;
		P.Pitch = -20.f * W;
		P.Roll = 6.f * W;
		P.LegL = 30.f * W;
		P.KneeL = -35.f * W;
		P.LegR = -10.f * W;
		P.KneeR = -20.f * W;
		P.ArmL = 55.f * W;
		P.ArmR = 40.f * W;
		P.ArmOutL = 30.f * W;
		P.ArmOutR = 35.f * W;
		return P;
	}

	/** How high his middle rides at T on the way over a vault: up to the peak over the obstacle, down to land. */
	inline double ArcZ(double FromZ, double ToZ, double PeakZ, float Over, float T)
	{
		const double Base = FMath::Lerp(FromZ, ToZ, static_cast<double>(T));
		const double BaseAtOver = FMath::Lerp(FromZ, ToZ, static_cast<double>(Over));
		const float S = T < Over
			? FMath::Sin(0.5f * UE_PI * T / FMath::Max(Over, 0.05f))
			: FMath::Sin(0.5f * UE_PI * (1.f - T) / FMath::Max(1.f - Over, 0.05f));
		return Base + FMath::Max(0.0, PeakZ - BaseAtOver) * S;
	}
}
