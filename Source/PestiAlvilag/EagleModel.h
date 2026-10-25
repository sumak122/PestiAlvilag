// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PersonKit.h"

/**
 * A golden eagle (the one that circles over Keleti): dark brown with a
 * golden nape, a yellow hooked beak, a wingspan of about two metres with
 * the long primary feathers spread like fingers at the tips.
 *
 * Built with the dress-up kit's lofts and blobs, in four pieces that move:
 * the body (with head and tail), each wing, which hangs from its shoulder
 * so it can beat and fold, and the feet, which only show when it perches.
 * Centimetres, X forward, Y to the right, Z up, the origin in the middle of
 * the body. Header-only so it can be built and looked at outside the engine.
 */
namespace EagleModel
{
	enum EPart : int32 { Body, WingLeft, WingRight, Feet, NumParts };

	struct FEagle
	{
		PersonKit::FPiece Parts[NumParts];
	};

	namespace Colors
	{
		const FLinearColor Plumage(0.13f, 0.085f, 0.05f);
		const FLinearColor Coverts(0.30f, 0.20f, 0.10f);
		const FLinearColor Primaries(0.05f, 0.04f, 0.03f);
		const FLinearColor Nape(0.55f, 0.38f, 0.13f);
		const FLinearColor Cere(0.85f, 0.65f, 0.15f);
		const FLinearColor Horn(0.12f, 0.11f, 0.10f);
		const FLinearColor Eye(0.02f, 0.02f, 0.02f);
	}

	/** The perched eagle's feet are this far below the middle of its body (cm). */
	constexpr double PerchHeight = 27.0;

	/** The right wing's shoulder; the left one is its mirror. */
	inline FVector Shoulder(double Side) { return FVector(4.0, 9.0 * Side, 5.0); }

	inline void Build(FEagle& Out)
	{
		using namespace PersonKit;
		using namespace Colors;
		auto AlongX = [](double X, double Z, double RadiusY, double RadiusZ)
		{
			return FRing{ FVector(X, 0.0, Z), FVector(0.0, RadiusY, 0.0), FVector(0.0, 0.0, RadiusZ) };
		};

		// The body, from the tail coverts to the neck, then head, beak and eyes.
		FPiece& B = Out.Parts[Body];
		Loft(B, Plumage, {
			AlongX(-26.0, 3.0, 5.0, 3.5),
			AlongX(-14.0, 2.0, 10.0, 9.5),
			AlongX(0.0, 0.0, 12.5, 12.0),
			AlongX(14.0, 2.0, 10.0, 10.5),
			AlongX(24.0, 5.0, 6.5, 7.0),
		}, 12, true, false);
		Blob(B, Plumage, FVector(31.0, 0.0, 7.0), FVector(8.5, 6.8, 7.2), 12, 7);
		Blob(B, Nape, FVector(25.0, 0.0, 9.5), FVector(6.5, 6.4, 5.5), 12, 5, -0.2);
		Loft(B, Cere, { AlongX(37.0, 6.0, 2.6, 2.4), AlongX(41.0, 5.0, 2.0, 2.0) }, 8, true, false);
		Loft(B, Horn, { AlongX(41.0, 5.0, 2.0, 2.0), AlongX(44.5, 3.0, 1.2, 1.4), AlongX(45.2, 0.5, 0.3, 0.3) }, 8, false, true);
		for (const double Side : { -1.0, 1.0 })
		{
			Blob(B, Eye, FVector(35.5, 4.7 * Side, 8.5), FVector(1.3, 0.8, 1.2), 6, 3);
			// The heavy brow that gives an eagle its frown.
			Blob(B, Plumage, FVector(35.5, 4.2 * Side, 10.0), FVector(2.4, 1.6, 1.0), 6, 3);
		}
		// The tail, a fan of long feathers.
		Loft(B, Plumage, {
			{ FVector(-24.0, 0.0, 3.0), FVector(0.0, 6.0, 0.0), FVector(0.0, 0.0, 1.6) },
			{ FVector(-40.0, 0.0, 3.0), FVector(0.0, 11.0, 0.0), FVector(0.0, 0.0, 1.3) },
			{ FVector(-56.0, 0.0, 3.0), FVector(0.0, 15.0, 0.0), FVector(0.0, 0.0, 1.0) },
		}, 10, false, true);

		// The wings, reaching out along Y from the shoulders.
		for (const double Side : { -1.0, 1.0 })
		{
			FPiece& W = Out.Parts[Side < 0.0 ? WingLeft : WingRight];
			W.Pivot = Shoulder(Side);
			auto Across = [Side](double X, double Y, double Z, double HalfChord, double Thick)
			{
				return FRing{ FVector(X, Y * Side, Z), FVector(HalfChord, 0.0, 0.0), FVector(0.0, 0.0, Thick) };
			};
			Loft(W, Plumage, {
				Across(-6.0, 8.0, 5.0, 16.0, 4.5),
				Across(-8.0, 30.0, 6.0, 21.0, 3.5),
				Across(-8.0, 60.0, 6.5, 20.0, 2.5),
				Across(-5.0, 82.0, 6.5, 15.0, 1.8),
				Across(-4.0, 90.0, 6.5, 12.0, 1.4),
			}, 10, true, true);
			// The lighter covert feathers along the top of the inner wing.
			Loft(W, Coverts, {
				Across(0.0, 9.0, 7.5, 9.0, 2.2),
				Across(-1.0, 40.0, 8.0, 11.0, 1.6),
				Across(0.0, 70.0, 7.8, 8.0, 1.0),
			}, 10, true, true);
			// Six primaries spread like fingers.
			for (int32 i = 0; i < 6; ++i)
			{
				const FVector Root(6.0 - i * 4.5, 84.0 * Side, 6.5);
				const FVector Tip(10.0 - i * 8.0, (110.0 - FMath::Abs(i - 2) * 3.5) * Side, 6.5);
				Loft(W, Primaries, {
					{ Root, FVector(2.8, 0.0, 0.0), FVector(0.0, 0.0, 0.8) },
					{ (Root + Tip) * 0.5, FVector(2.4, 0.0, 0.0), FVector(0.0, 0.0, 0.6) },
					{ Tip, FVector(0.6, 0.0, 0.0), FVector(0.0, 0.0, 0.3) },
				}, 6, true, true);
			}
		}

		// Feathered legs, yellow feet, black talons.
		FPiece& F = Out.Parts[Feet];
		for (const double Side : { -1.0, 1.0 })
		{
			const double Y = 5.5 * Side;
			Blob(F, Plumage, FVector(1.0, Y, -10.0), FVector(6.0, 4.5, 7.5), 10, 5);
			Rod(F, Cere, FVector(2.0, Y, -15.0), FVector(3.0, Y, -PerchHeight + 1.5), 1.4);
			const FVector Heel(3.0, Y, -PerchHeight + 1.0);
			for (const double Spread : { -25.0, 0.0, 25.0 })
			{
				const double A = FMath::DegreesToRadians(Spread);
				const FVector Toe = Heel + FVector(7.0 * FMath::Cos(A), 7.0 * FMath::Sin(A), 0.0);
				Rod(F, Cere, Heel, Toe, 0.9);
				Rod(F, Horn, Toe, Toe + FVector(2.0 * FMath::Cos(A), 2.0 * FMath::Sin(A), -1.0), 0.5);
			}
			Rod(F, Cere, Heel, Heel + FVector(-5.0, 0.0, 0.0), 0.9);
		}
	}
}
