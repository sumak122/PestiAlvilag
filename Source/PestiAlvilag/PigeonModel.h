// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PersonKit.h"

/**
 * A town pigeon: blue-grey with a shimmering green and purple neck, two dark
 * bars across the folded wing, a dark band at the end of the tail, red feet
 * and an orange eye. Some are darker, some chequered, a few nearly white.
 *
 * Built with the dress-up kit's lofts and blobs, in pieces that move: the
 * body (with the tail and legs), the head (which hangs from the base of the
 * neck, so it can bob and peck), the folded wings, shown on the ground, and
 * the two spread wings, which only show in flight and beat from their
 * shoulders.
 * Centimetres, X forward, Y to the right, Z up, the origin on the ground
 * between the feet. Header-only so it can be built and looked at outside
 * the engine.
 */
namespace PigeonModel
{
	enum EPart : int32 { Body, Head, Folded, WingLeft, WingRight, NumParts };

	enum class EPlumage : uint8 { Blue, Dark, Chequer, Pale, Count };

	struct FPigeon
	{
		PersonKit::FPiece Parts[NumParts];
	};

	/** The middle of the body above the ground (cm), standing. */
	constexpr double BodyHeight = 14.0;

	/** Where the head hangs from: the base of the neck. */
	inline FVector NeckBase() { return FVector(8.0, 0.0, 18.0); }

	/** The right wing's shoulder; the left one is its mirror. */
	inline FVector Shoulder(double Side) { return FVector(3.0, 4.5 * Side, 18.0); }

	struct FColors
	{
		FLinearColor Grey;
		FLinearColor Wing;
		FLinearColor Bar;
		FLinearColor Rump;
		FLinearColor NeckGreen;
		FLinearColor NeckPurple;
		FLinearColor HeadColor;
	};

	inline FColors ColorsFor(EPlumage Plumage)
	{
		switch (Plumage)
		{
		case EPlumage::Dark:
			return { FLinearColor(0.07f, 0.075f, 0.085f), FLinearColor(0.09f, 0.095f, 0.105f), FLinearColor(0.02f, 0.02f, 0.02f),
				FLinearColor(0.08f, 0.08f, 0.09f), FLinearColor(0.04f, 0.10f, 0.06f), FLinearColor(0.10f, 0.04f, 0.10f), FLinearColor(0.06f, 0.06f, 0.07f) };
		case EPlumage::Chequer:
			return { FLinearColor(0.17f, 0.18f, 0.21f), FLinearColor(0.12f, 0.12f, 0.14f), FLinearColor(0.035f, 0.035f, 0.04f),
				FLinearColor(0.20f, 0.21f, 0.24f), FLinearColor(0.06f, 0.14f, 0.09f), FLinearColor(0.14f, 0.06f, 0.15f), FLinearColor(0.12f, 0.12f, 0.14f) };
		case EPlumage::Pale:
			return { FLinearColor(0.62f, 0.61f, 0.58f), FLinearColor(0.70f, 0.69f, 0.66f), FLinearColor(0.35f, 0.33f, 0.31f),
				FLinearColor(0.66f, 0.65f, 0.62f), FLinearColor(0.40f, 0.48f, 0.40f), FLinearColor(0.48f, 0.40f, 0.48f), FLinearColor(0.66f, 0.65f, 0.62f) };
		default:
			return { FLinearColor(0.22f, 0.235f, 0.27f), FLinearColor(0.30f, 0.315f, 0.35f), FLinearColor(0.03f, 0.03f, 0.035f),
				FLinearColor(0.34f, 0.35f, 0.38f), FLinearColor(0.07f, 0.17f, 0.10f), FLinearColor(0.17f, 0.07f, 0.18f), FLinearColor(0.17f, 0.18f, 0.21f) };
		}
	}

	inline void Build(FPigeon& Out, EPlumage Plumage)
	{
		using namespace PersonKit;
		const FColors C = ColorsFor(Plumage);
		const FLinearColor Feet(0.55f, 0.10f, 0.09f);
		const FLinearColor Beak(0.05f, 0.045f, 0.045f);
		const FLinearColor Cere(0.80f, 0.78f, 0.74f);
		const FLinearColor Eye(0.80f, 0.32f, 0.04f);
		const FLinearColor Pupil(0.01f, 0.01f, 0.01f);
		auto AlongX = [](double X, double Z, double RadiusY, double RadiusZ)
		{
			return FRing{ FVector(X, 0.0, Z), FVector(0.0, RadiusY, 0.0), FVector(0.0, 0.0, RadiusZ) };
		};

		// The plump body, breast forward, tail tipped down a little.
		FPiece& B = Out.Parts[Body];
		Loft(B, C.Grey, {
			AlongX(-12.0, 15.5, 3.2, 2.6),
			AlongX(-6.0, 14.5, 6.0, 5.6),
			AlongX(1.0, 14.0, 7.4, 7.2),
			AlongX(6.0, 15.0, 6.8, 7.0),
			AlongX(9.5, 18.0, 4.4, 4.6),
		}, 12, true, true);
		Blob(B, C.Grey, FVector(5.0, 0.0, 13.0), FVector(5.5, 6.2, 6.0), 12, 6);
		// The folded wings along the sides, the two dark bars across them.
		FPiece& FW = Out.Parts[Folded];
		for (const double Side : { -1.0, 1.0 })
		{
			Loft(FW, C.Wing, {
				{ FVector(6.0, 3.5 * Side, 16.5), FVector(0.0, 2.8 * Side, 0.0), FVector(0.0, 0.0, 4.0) },
				{ FVector(-4.0, 4.0 * Side, 16.2), FVector(0.0, 3.2 * Side, 0.0), FVector(0.0, 0.0, 4.8) },
				{ FVector(-13.0, 2.6 * Side, 16.2), FVector(0.0, 2.0 * Side, 0.0), FVector(0.0, 0.0, 2.6) },
				{ FVector(-19.0, 1.2 * Side, 16.0), FVector(0.0, 0.8 * Side, 0.0), FVector(0.0, 0.0, 0.8) },
			}, 12, true, true);
			for (const double X : { -1.5, -5.5 })
			{
				Block(FW, C.Bar, FVector(X, 7.05 * Side, 16.6), FVector(0.7, 0.35, 2.4));
			}
			if (Plumage == EPlumage::Chequer)
			{
				for (int32 k = 0; k < 6; ++k)
				{
					Block(FW, C.Bar, FVector(3.0 - k * 2.3, 6.9 * Side, 17.5 - (k % 2) * 2.0), FVector(0.5, 0.3, 0.5));
				}
			}
		}
		// The tail, fanned a little, with its dark band.
		Loft(B, C.Rump, {
			{ FVector(-11.0, 0.0, 15.5), FVector(0.0, 3.0, 0.0), FVector(0.0, 0.0, 1.2) },
			{ FVector(-19.0, 0.0, 14.5), FVector(0.0, 4.0, 0.0), FVector(0.0, 0.0, 0.9) },
		}, 8, true, false);
		Loft(B, C.Bar, {
			{ FVector(-19.0, 0.0, 14.5), FVector(0.0, 4.0, 0.0), FVector(0.0, 0.0, 0.9) },
			{ FVector(-22.5, 0.0, 14.0), FVector(0.0, 4.3, 0.0), FVector(0.0, 0.0, 0.7) },
		}, 8, false, true);
		// Short red legs and toes.
		for (const double Side : { -1.0, 1.0 })
		{
			const double Y = 2.2 * Side;
			Rod(B, Feet, FVector(1.5, Y, 9.0), FVector(1.0, Y, 0.8), 0.6);
			for (const double Spread : { -25.0, 0.0, 25.0 })
			{
				const double A = FMath::DegreesToRadians(Spread);
				Rod(B, Feet, FVector(1.0, Y, 0.6), FVector(1.0 + 3.0 * FMath::Cos(A), Y + 3.0 * FMath::Sin(A), 0.4), 0.35, 4);
			}
			Rod(B, Feet, FVector(1.0, Y, 0.6), FVector(-1.5, Y, 0.4), 0.35, 4);
		}

		// The neck and the small round head, from the base of the neck.
		FPiece& H = Out.Parts[Head];
		H.Pivot = NeckBase();
		Loft(H, C.NeckPurple, { AlongX(8.0, 17.0, 4.6, 4.6), AlongX(10.0, 21.0, 3.9, 3.9) }, 10, false, false, 0.0);
		Loft(H, C.NeckGreen, { AlongX(10.0, 21.0, 3.9, 3.9), AlongX(11.5, 24.0, 3.1, 3.1) }, 10, false, false, 0.0);
		Blob(H, C.HeadColor, FVector(12.5, 0.0, 26.0), FVector(4.0, 3.2, 3.6), 10, 6);
		Blob(H, Cere, FVector(15.6, 0.0, 26.6), FVector(1.3, 1.0, 0.8), 6, 3);
		Loft(H, Beak, { AlongX(15.8, 26.0, 0.9, 0.8), AlongX(18.6, 25.4, 0.25, 0.25) }, 6, false, true);
		for (const double Side : { -1.0, 1.0 })
		{
			Blob(H, Eye, FVector(14.0, 2.6 * Side, 27.0), FVector(0.9, 0.5, 0.9), 6, 3);
			Blob(H, Pupil, FVector(14.2, 2.95 * Side, 27.0), FVector(0.45, 0.3, 0.45), 6, 3);
		}

		// The spread wings for flight, reaching out along Y.
		for (const double Side : { -1.0, 1.0 })
		{
			FPiece& W = Out.Parts[Side < 0.0 ? WingLeft : WingRight];
			W.Pivot = Shoulder(Side);
			const double Z = Shoulder(Side).Z;
			auto Across = [Side, Z](double X, double Y, double HalfChord, double Thick)
			{
				return FRing{ FVector(X, Y * Side, Z), FVector(HalfChord, 0.0, 0.0), FVector(0.0, 0.0, Thick) };
			};
			Loft(W, C.Wing, {
				Across(1.0, 4.0, 6.0, 1.6),
				Across(0.0, 12.0, 7.0, 1.2),
				Across(-1.0, 20.0, 6.0, 0.9),
				Across(-3.0, 28.0, 4.0, 0.6),
				Across(-4.5, 32.0, 1.5, 0.4),
			}, 8, true, true);
			for (const double Y : { 9.0, 13.0 })
			{
				Loft(W, C.Bar, { Across(-4.5, Y, 1.0, 1.25), Across(-4.5, Y + 2.0, 1.0, 1.25) }, 6, true, true);
			}
			// The dark flight feathers at the trailing edge and tip.
			Loft(W, C.Bar, {
				Across(-6.5, 18.0, 1.5, 0.7),
				Across(-6.5, 26.0, 2.5, 0.5),
				Across(-5.0, 33.0, 1.0, 0.3),
			}, 6, true, true);
		}
	}
}
