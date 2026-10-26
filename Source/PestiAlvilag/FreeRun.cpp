// Fill out your copyright notice in the Description page of Project Settings.

#include "FreeRun.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace FreeRun
{
	// The probe that looks ahead: a thin upright capsule from his shins to
	// his head, so the ground and kerbs don't count (cm).
	constexpr float ProbeRadius = 15.f;
	constexpr float ProbeHalfHeight = 75.f;
	constexpr double ProbeMiddle = 100.0;
	// Steeper than this the thing ahead is a slope or a step to walk up.
	constexpr double WalkableNormalZ = 0.7;
	// Looking for room on the far side, in steps this long (cm).
	constexpr double SearchStep = 30.0;
	// A landing may be this much lower or higher than where he took off (cm).
	constexpr double MaxDrop = 60.0;
	constexpr double MaxRise = 35.0;
	// Sliding, his body passes this high over the ground (cm).
	constexpr float SlideBodyRadius = 25.f;
	constexpr double SlideBodyHeight = 30.0;

	namespace
	{
		bool Blocked(const UWorld* World, const FVector& From, const FVector& To, const FCollisionQueryParams& Params)
		{
			FHitResult Hit;
			return World->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Params);
		}

		/** The ground under a point, between Top and Bottom; false if there is none. */
		bool GroundAt(const UWorld* World, const FVector2D& At, double Top, double Bottom, const FCollisionQueryParams& Params, double& OutZ)
		{
			FHitResult Hit;
			if (!World->LineTraceSingleByChannel(Hit, FVector(At, Top), FVector(At, Bottom), ECC_Visibility, Params))
			{
				return false;
			}
			OutZ = Hit.ImpactPoint.Z;
			return true;
		}

		/** True if a standing capsule fits with its middle at Middle. */
		bool Fits(const UWorld* World, const FVector& Middle, float Radius, float HalfHeight, const FCollisionQueryParams& Params)
		{
			return !World->OverlapAnyTestByChannel(Middle, FQuat::Identity, ECC_Pawn,
				FCollisionShape::MakeCapsule(Radius, HalfHeight), Params);
		}
	}

	bool FindMove(const UWorld* World, const AActor* Self, const FVector& Middle, const FVector& Dir,
		float CapsuleRadius, float CapsuleHalfHeight, FChance& Out)
	{
		Out = FChance();
		if (!World || Dir.IsNearlyZero())
		{
			return false;
		}
		const FVector Ahead = Dir.GetSafeNormal2D();
		const FVector2D Flat(Ahead.X, Ahead.Y);
		const double Feet = Middle.Z - CapsuleHalfHeight;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(FreeRunLook), false, Self);

		// Is there anything ahead, and how far?
		const FVector Start(Middle.X, Middle.Y, Feet + ProbeMiddle);
		const double Inset = CapsuleRadius - ProbeRadius;
		FHitResult Hit;
		if (!World->SweepSingleByChannel(Hit, Start, Start + Ahead * (Inset + Reach), FQuat::Identity, ECC_Visibility,
				FCollisionShape::MakeCapsule(ProbeRadius, ProbeHalfHeight), Params)
			|| Hit.bStartPenetrating || Hit.ImpactNormal.Z > WalkableNormalZ)
		{
			return false;
		}
		const double Face = FMath::Max(0.0, Hit.Distance - Inset);
		const FVector2D Near = FVector2D(Middle.X, Middle.Y) + Flat * (CapsuleRadius + Face);
		const FVector2D Inside = Near + Flat * 5.0;

		// Its top, from above.
		double TopZ = 0.0;
		const bool bTop = GroundAt(World, Inside, Feet + 220.0, Feet + 5.0, Params, TopZ);
		const double Top = bTop ? TopZ - Feet : 0.0;

		// Room under it: nothing low ahead, then its underside from below.
		double Gap = 0.0;
		const FVector2D Past = Near + Flat * 12.0;
		const bool bLowClear = !Blocked(World, FVector(Middle.X, Middle.Y, Feet + 25.0), FVector(Past, Feet + 25.0), Params)
			&& !Blocked(World, FVector(Middle.X, Middle.Y, Feet + 50.0), FVector(Past, Feet + 50.0), Params);
		if (bLowClear)
		{
			FHitResult Under;
			if (World->LineTraceSingleByChannel(Under, FVector(Inside, Feet + 30.0), FVector(Inside, Feet + 220.0), ECC_Visibility, Params))
			{
				Gap = Under.ImpactPoint.Z - Feet;
			}
		}

		Out.Face = Face;
		const double Along = CapsuleRadius + Face;
		if (bLowClear && Gap >= MinSlideGap && Gap <= MaxSlideGap)
		{
			// Slide on until there is room to stand, low enough all the way.
			for (double D = Along + SearchStep; D <= Along + MaxSlideDepth; D += SearchStep)
			{
				const FVector2D At = FVector2D(Middle.X, Middle.Y) + Flat * D;
				double Ground = 0.0;
				if (!GroundAt(World, At, Feet + 40.0, Feet - MaxDrop, Params, Ground))
				{
					return false;
				}
				const FVector Stand(At, Ground + CapsuleHalfHeight + 2.0);
				if (Fits(World, Stand, CapsuleRadius, CapsuleHalfHeight, Params))
				{
					FHitResult Body;
					const FVector Low(Middle.X, Middle.Y, Feet + SlideBodyHeight);
					if (World->SweepSingleByChannel(Body, Low, FVector(At, Feet + SlideBodyHeight), FQuat::Identity, ECC_Visibility,
							FCollisionShape::MakeSphere(SlideBodyRadius), Params))
					{
						return false;
					}
					Out.Move = EMove::Slide;
					Out.To = Stand;
					Out.Over = static_cast<float>(FMath::Clamp(Along / D, 0.1, 0.9));
					return true;
				}
			}
			return false;
		}

		if (bTop && Top >= MinVaultTop && Top <= MaxVaultTop)
		{
			// Over the top with room for his body, and a place to land beyond.
			const double PeakZ = TopZ + VaultClear;
			FHitResult Body;
			const FVector High(Middle.X, Middle.Y, FMath::Max(Middle.Z, PeakZ + 10.0));
			for (double D = Along + SearchStep; D <= Along + MaxVaultDepth; D += SearchStep)
			{
				const FVector2D At = FVector2D(Middle.X, Middle.Y) + Flat * D;
				double Ground = 0.0;
				if (!GroundAt(World, At, TopZ + 20.0, Feet - MaxDrop, Params, Ground) || Ground > Feet + MaxRise)
				{
					continue;
				}
				const FVector Stand(At, Ground + CapsuleHalfHeight + 2.0);
				if (!Fits(World, Stand, CapsuleRadius, CapsuleHalfHeight, Params))
				{
					continue;
				}
				if (World->SweepSingleByChannel(Body, High, FVector(At, High.Z), FQuat::Identity, ECC_Visibility,
						FCollisionShape::MakeSphere(CapsuleRadius), Params))
				{
					return false;
				}
				// The middle of the obstacle: halfway from its near side to where its top ends.
				double Far = Along;
				for (double E = Along + 10.0; E < D; E += 10.0)
				{
					double Z = 0.0;
					if (GroundAt(World, FVector2D(Middle.X, Middle.Y) + Flat * E, Feet + 220.0, Feet + 5.0, Params, Z) && Z > Feet + MinVaultTop * 0.5)
					{
						Far = E;
					}
				}
				Out.Move = EMove::Vault;
				Out.To = Stand;
				Out.PeakZ = PeakZ;
				Out.Over = static_cast<float>(FMath::Clamp(0.5 * (Along + Far) / D, 0.15, 0.85));
				return true;
			}
		}
		return false;
	}
}
