// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PersonKit.h"

/**
 * An open umbrella of 1932 (esernyő): eight black (or dark) cloth panels on
 * steel ribs, a thin shaft, a wooden crook handle and a little ferrule on
 * top. Built in its own frame with the hand's grip at the origin and the
 * shaft straight up Z, so the person holding it only has to put the grip in
 * his hand. Scaling X and Y down to about a tenth closes it into a furled
 * umbrella.
 */
namespace UmbrellaModel
{
	/** Ribs round the canopy. */
	constexpr int32 Ribs = 8;
	/** The canopy's radius at the rib tips and how high its top is above the grip (cm). */
	constexpr double Radius = 50.0;
	constexpr double Top = 92.0;
	/** How far the rim hangs below the top. */
	constexpr double Drop = 24.0;
	/** Furled: X and Y are scaled to this. */
	constexpr double Furled = 0.1;

	/** The cloth colours: mostly black, some dark blue, green, brown and wine red. */
	inline FLinearColor ClothColor(float Pick, bool bWoman)
	{
		static const FLinearColor Colors[] = {
			FLinearColor(0.012f, 0.012f, 0.014f),
			FLinearColor(0.012f, 0.012f, 0.014f),
			FLinearColor(0.012f, 0.012f, 0.014f),
			FLinearColor(0.02f, 0.025f, 0.06f),
			FLinearColor(0.02f, 0.045f, 0.03f),
			FLinearColor(0.05f, 0.03f, 0.018f),
			FLinearColor(0.09f, 0.015f, 0.02f),
			FLinearColor(0.12f, 0.06f, 0.10f),
		};
		const int32 Count = static_cast<int32>(UE_ARRAY_COUNT(Colors));
		// Ladies' ones are more often coloured; men's nearly always black.
		const float Share = bWoman ? Pick : Pick * Pick * 0.7f;
		return Colors[FMath::Clamp(static_cast<int32>(Share * Count), 0, Count - 1)];
	}

	/** Where the canopy is, Along 0 (top) to 1 (rim) at rib Rib (rib Ribs is rib 0 again). */
	inline FVector CanopyPoint(int32 Rib, double Along)
	{
		const double Angle = 2.0 * PI * Rib / Ribs;
		// A dome: flat near the top, curving down to the rim.
		const double Out = Radius * Along;
		const double Down = Drop * FMath::Pow(Along, 1.7);
		return FVector(Out * FMath::Cos(Angle), Out * FMath::Sin(Angle), Top - Down);
	}

	/** One flat triangle, seen from both sides. */
	inline void Facet(PersonKit::FSection& S, const FVector& A, const FVector& B, const FVector& C)
	{
		FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
		// Outward (upward) first, as the game's triangles face.
		if (Normal.Z < 0.0)
		{
			Normal = -Normal;
		}
		const int32 First = S.Vertices.Num();
		S.Vertices.Append({ A, B, C, A, C, B });
		S.Normals.Append({ Normal, Normal, Normal, -Normal, -Normal, -Normal });
		// Unreal's front faces wind clockwise seen from the front.
		const bool bUpward = FVector::DotProduct(FVector::CrossProduct(B - A, C - A), FVector::UpVector) > 0.0;
		if (bUpward)
		{
			S.Triangles.Append({ First + 0, First + 2, First + 1, First + 3, First + 5, First + 4 });
		}
		else
		{
			S.Triangles.Append({ First + 0, First + 1, First + 2, First + 3, First + 4, First + 5 });
		}
	}

	/** The whole umbrella into Out (its pivot left at the grip). */
	inline void Build(PersonKit::FPiece& Out, const FLinearColor& Cloth)
	{
		const FLinearColor Steel(0.05f, 0.05f, 0.055f);
		const FLinearColor Wood(0.16f, 0.08f, 0.035f);
		const FLinearColor Tip(0.45f, 0.40f, 0.30f);

		// The canopy: each panel between two ribs in three bands, the cloth
		// sagging a little between the ribs at the rim.
		PersonKit::FSection& Canopy = Out.Section(Cloth);
		const double Bands[] = { 0.0, 0.4, 0.75, 1.0 };
		for (int32 r = 0; r < Ribs; ++r)
		{
			for (int32 b = 0; b + 1 < static_cast<int32>(UE_ARRAY_COUNT(Bands)); ++b)
			{
				const FVector A0 = CanopyPoint(r, Bands[b]);
				const FVector B0 = CanopyPoint(r + 1, Bands[b]);
				FVector A1 = CanopyPoint(r, Bands[b + 1]);
				FVector B1 = CanopyPoint(r + 1, Bands[b + 1]);
				if (b == 0)
				{
					Facet(Canopy, A0, A1, B1);
					continue;
				}
				Facet(Canopy, A0, A1, B1);
				Facet(Canopy, A0, B1, B0);
			}
			// The scallop: the rim between two rib tips curves up and in.
			const FVector Left = CanopyPoint(r, 1.0);
			const FVector Right = CanopyPoint(r + 1, 1.0);
			const FVector Inner = (CanopyPoint(r, 0.75) + CanopyPoint(r + 1, 0.75)) * 0.5;
			const FVector Sag = (Left + Right) * 0.5 * 0.97 + FVector(0.0, 0.0, 2.5);
			Facet(Canopy, Left, Sag, Inner);
			Facet(Canopy, Sag, Right, Inner);
		}

		// Ribs under the cloth and their tips, the shaft, the ferrule on top.
		for (int32 r = 0; r < Ribs; ++r)
		{
			const FVector Under(0.0, 0.0, -0.8);
			PersonKit::Rod(Out, Steel, CanopyPoint(r, 0.05) + Under, CanopyPoint(r, 0.5) + Under, 0.35, 4);
			PersonKit::Rod(Out, Steel, CanopyPoint(r, 0.5) + Under, CanopyPoint(r, 1.0) + Under, 0.35, 4);
			PersonKit::Blob(Out, Tip, CanopyPoint(r, 1.02), FVector(0.7), 5, 3);
			// The stretchers from the runner on the shaft up to the middle of each rib.
			PersonKit::Rod(Out, Steel, FVector(0.0, 0.0, Top - 30.0), CanopyPoint(r, 0.5) + Under, 0.25, 4);
		}
		PersonKit::Rod(Out, Steel, FVector(0.0, 0.0, 4.0), FVector(0.0, 0.0, Top + 6.0), 0.8, 6);
		PersonKit::Rod(Out, Tip, FVector(0.0, 0.0, Top), FVector(0.0, 0.0, Top + 9.0), 0.5, 5);
		PersonKit::Blob(Out, Steel, FVector(0.0, 0.0, Top - 30.0), FVector(1.3, 1.3, 2.5), 6, 3);

		// The crook: straight down through the hand, then round in a hook.
		PersonKit::Rod(Out, Wood, FVector(0.0, 0.0, 6.0), FVector(0.0, 0.0, -8.0), 1.4, 6);
		FVector Last(0.0, 0.0, -8.0);
		for (int32 k = 1; k <= 5; ++k)
		{
			const double Angle = PI * k / 5.0;
			const FVector Next(-4.5 + 4.5 * FMath::Cos(Angle), 0.0, -8.0 - 4.5 * FMath::Sin(Angle));
			PersonKit::Rod(Out, Wood, Last, Next, 1.3, 6);
			Last = Next;
		}
	}
}
