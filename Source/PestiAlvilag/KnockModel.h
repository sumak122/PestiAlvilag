// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PersonKit.h"

/**
 * Loose things on the pavements and at the market that can be knocked
 * over: wooden crates, crates of apples, potatoes or cabbages, beer
 * barrels, milk churns, tin dustbins and wicker baskets of bread.
 *
 * Each is one mesh (built with the dress-up kit's lofts and blocks) and a
 * simple collision shape: a box or an upright cylinder of Half size. A
 * crate's goods are a second mesh (Load) that is hidden when the crate goes
 * over and its goods roll out as loose lumps of LumpColor.
 * Centimetres, X forward, Y to the right, Z up, the origin in the middle of
 * the thing's bottom. Header-only so it can be built and looked at outside
 * the engine.
 */
namespace KnockModel
{
	enum class EKind : uint8 { Crate, Apples, Potatoes, Cabbages, Barrel, Churn, Dustbin, Basket, Count };

	struct FThing
	{
		PersonKit::FPiece Mesh;
		/** What is heaped in or on it, if anything. */
		PersonKit::FPiece Load;
		/** Half size of the collision shape: a box, or a cylinder of radius X when bRound. */
		FVector Half = FVector(25.0, 18.0, 15.0);
		bool bRound = false;
		/** Kilograms. */
		float Mass = 6.f;
		FLinearColor LumpColor = FLinearColor::Black;
		double LumpRadius = 0.0;
		int32 Lumps = 0;
	};

	namespace Colors
	{
		const FLinearColor Wood(0.40f, 0.28f, 0.15f);
		const FLinearColor DarkWood(0.24f, 0.16f, 0.08f);
		const FLinearColor Oak(0.30f, 0.17f, 0.07f);
		const FLinearColor Iron(0.035f, 0.035f, 0.04f);
		const FLinearColor Tin(0.42f, 0.43f, 0.44f);
		const FLinearColor DullTin(0.27f, 0.28f, 0.29f);
		const FLinearColor Wicker(0.46f, 0.32f, 0.14f);
		const FLinearColor Crust(0.50f, 0.27f, 0.08f);
		const FLinearColor RedApple(0.42f, 0.04f, 0.03f);
		const FLinearColor GreenApple(0.30f, 0.42f, 0.06f);
		const FLinearColor Potato(0.42f, 0.31f, 0.16f);
		const FLinearColor Cabbage(0.22f, 0.40f, 0.12f);
		const FLinearColor Lettering(0.80f, 0.78f, 0.70f);
	}

	/** A slatted wooden crate, open at the top: W long (X), D deep (Y), H high. */
	inline void Crate(PersonKit::FPiece& P, double W, double D, double H)
	{
		using namespace PersonKit;
		using namespace Colors;
		const double Post = 2.2;
		// The floor.
		Block(P, DarkWood, FVector(0.0, 0.0, 1.0), FVector(W * 0.5 - 1.0, D * 0.5 - 1.0, 1.0));
		// Corner posts.
		for (const double SX : { -1.0, 1.0 })
		{
			for (const double SY : { -1.0, 1.0 })
			{
				Block(P, DarkWood, FVector(SX * (W * 0.5 - Post), SY * (D * 0.5 - Post), H * 0.5), FVector(Post, Post, H * 0.5));
			}
		}
		// Three slats a side, with gaps between.
		const double Slat = H / 3.0 - 1.6;
		for (int32 k = 0; k < 3; ++k)
		{
			const double Z = 1.0 + k * (H / 3.0) + Slat * 0.5 + 0.6;
			for (const double S : { -1.0, 1.0 })
			{
				Block(P, Wood, FVector(0.0, S * (D * 0.5 - 0.8), Z), FVector(W * 0.5, 0.8, Slat * 0.5));
				Block(P, Wood, FVector(S * (W * 0.5 - 0.8), 0.0, Z), FVector(0.8, D * 0.5 - 1.6, Slat * 0.5));
			}
		}
		// Hand holes are cut into the ends of the top slats: shown dark.
		for (const double S : { -1.0, 1.0 })
		{
			Block(P, Iron, FVector(S * (W * 0.5 + 0.1), 0.0, H - Slat * 0.5), FVector(0.1, 5.0, 1.4));
		}
	}

	/** A heap of round things in a crate W by D, filled to Top. */
	inline void Heap(PersonKit::FPiece& P, double W, double D, double Top, const FLinearColor& A, const FLinearColor& B, double Radius, double Squash, uint32 Seed)
	{
		using namespace PersonKit;
		uint32 S = Seed;
		auto Next = [&S]() { S = S * 1664525u + 1013904223u; return static_cast<double>(S >> 8) / static_cast<double>(1 << 24); };
		const int32 NX = FMath::Max(1, static_cast<int32>(W / (Radius * 2.0)));
		const int32 NY = FMath::Max(1, static_cast<int32>(D / (Radius * 2.0)));
		for (int32 Layer = 0; Layer < 2; ++Layer)
		{
			for (int32 i = 0; i < NX - Layer; ++i)
			{
				for (int32 j = 0; j < NY - Layer; ++j)
				{
					const double X = -W * 0.5 + Radius * (1.0 + Layer) + i * Radius * 2.0 + (Next() - 0.5) * Radius * 0.4;
					const double Y = -D * 0.5 + Radius * (1.0 + Layer) + j * Radius * 2.0 + (Next() - 0.5) * Radius * 0.4;
					const double Z = Top - Radius * 0.6 + Layer * Radius * 0.9;
					const double R = Radius * (0.88 + Next() * 0.2);
					Blob(P, Next() < 0.7 ? A : B, FVector(X, Y, Z), FVector(R, R, R * Squash), 8, 5);
				}
			}
		}
	}

	/** A round thing lofted from (height, radius) pairs, around the Z axis. */
	inline void Turned(PersonKit::FPiece& P, const FLinearColor& Color, const TArray<FVector2D>& Profile, int32 Sides, bool bCapBottom, bool bCapTop)
	{
		using namespace PersonKit;
		TArray<FRing> Rings;
		for (const FVector2D& Step : Profile)
		{
			Rings.Add({ FVector(0.0, 0.0, Step.X), FVector(Step.Y, 0.0, 0.0), FVector(0.0, Step.Y, 0.0) });
		}
		Loft(P, Color, Rings, Sides, bCapBottom, bCapTop);
	}

	/** A thin ring (a hoop, a rim) of Radius at Z. */
	inline void Hoop(PersonKit::FPiece& P, const FLinearColor& Color, double Z, double Radius, double Height, double Proud = 0.6)
	{
		Turned(P, Color, { FVector2D(Z - Height * 0.5, Radius + Proud), FVector2D(Z + Height * 0.5, Radius + Proud) }, 18, true, true);
	}

	inline void Build(FThing& Out, EKind Kind, uint32 Seed = 1)
	{
		using namespace PersonKit;
		using namespace Colors;
		FPiece& P = Out.Mesh;
		switch (Kind)
		{
		case EKind::Crate:
		case EKind::Apples:
		case EKind::Potatoes:
		case EKind::Cabbages:
		{
			const double W = 54.0, D = 36.0, H = 28.0;
			Crate(P, W, D, H);
			Out.Half = FVector(W * 0.5, D * 0.5, H * 0.5);
			Out.Mass = 4.f;
			if (Kind == EKind::Apples)
			{
				Heap(Out.Load, W - 4.0, D - 4.0, H, RedApple, GreenApple, 4.2, 0.92, Seed);
				Out.LumpColor = RedApple;
				Out.LumpRadius = 4.2;
				Out.Lumps = 8;
				Out.Mass = 16.f;
			}
			else if (Kind == EKind::Potatoes)
			{
				Heap(Out.Load, W - 4.0, D - 4.0, H, Potato, Potato * 0.8f, 4.6, 0.7, Seed);
				Out.LumpColor = Potato;
				Out.LumpRadius = 4.4;
				Out.Lumps = 8;
				Out.Mass = 22.f;
			}
			else if (Kind == EKind::Cabbages)
			{
				Heap(Out.Load, W - 4.0, D - 4.0, H - 2.0, Cabbage, Cabbage * 1.25f, 8.5, 0.85, Seed);
				Out.LumpColor = Cabbage;
				Out.LumpRadius = 8.0;
				Out.Lumps = 4;
				Out.Mass = 14.f;
			}
			break;
		}
		case EKind::Barrel:
		{
			// An oak beer barrel standing on end, bellied, with four iron hoops.
			Turned(P, Oak, {
				FVector2D(0.0, 23.0), FVector2D(14.0, 26.5), FVector2D(28.0, 28.5), FVector2D(42.0, 29.0),
				FVector2D(56.0, 28.5), FVector2D(70.0, 26.5), FVector2D(84.0, 23.0),
			}, 20, true, true);
			// The staves' joins, as faint dark lines.
			for (int32 k = 0; k < 10; ++k)
			{
				const double A = 2.0 * PI * k / 10.0;
				const FVector Out1(FMath::Cos(A), FMath::Sin(A), 0.0);
				Rod(P, DarkWood, Out1 * 23.2 + FVector(0, 0, 1.0), Out1 * 29.2 + FVector(0, 0, 42.0), 0.35, 3);
				Rod(P, DarkWood, Out1 * 29.2 + FVector(0, 0, 42.0), Out1 * 23.2 + FVector(0, 0, 83.0), 0.35, 3);
			}
			for (const FVector2D& H : { FVector2D(6.0, 25.0), FVector2D(22.0, 28.0), FVector2D(62.0, 28.0), FVector2D(78.0, 25.0) })
			{
				Hoop(P, Iron, H.X, H.Y, 3.0);
			}
			// The bung on the side.
			Rod(P, DarkWood, FVector(28.5, 0.0, 42.0), FVector(31.0, 0.0, 42.0), 2.6, 8);
			Out.Half = FVector(28.0, 28.0, 42.0);
			Out.bRound = true;
			Out.Mass = 40.f;
			break;
		}
		case EKind::Churn:
		{
			// A tinned milk churn: a drum, a sloping shoulder, a neck and a cap.
			Turned(P, Tin, {
				FVector2D(0.0, 16.0), FVector2D(36.0, 16.0), FVector2D(44.0, 11.0), FVector2D(47.0, 8.0),
				FVector2D(55.0, 8.0),
			}, 18, true, false);
			Hoop(P, DullTin, 1.5, 16.0, 3.0);
			Hoop(P, DullTin, 18.0, 16.0, 2.0);
			Hoop(P, DullTin, 34.0, 16.0, 2.0);
			Turned(P, DullTin, { FVector2D(54.0, 9.5), FVector2D(59.0, 9.5), FVector2D(61.0, 6.0) }, 18, true, true);
			for (const double S : { -1.0, 1.0 })
			{
				Rod(P, DullTin, FVector(0.0, S * 11.5, 45.0), FVector(0.0, S * 14.5, 50.0), 0.9);
				Rod(P, DullTin, FVector(0.0, S * 14.5, 50.0), FVector(0.0, S * 9.0, 52.0), 0.9);
			}
			Out.Half = FVector(16.0, 16.0, 30.0);
			Out.bRound = true;
			Out.Mass = 25.f;
			break;
		}
		case EKind::Dustbin:
		{
			// A galvanised dustbin with ribs round it and a domed lid.
			Turned(P, DullTin, { FVector2D(0.0, 22.0), FVector2D(64.0, 24.0) }, 20, true, false);
			for (const double Z : { 3.0, 22.0, 42.0, 61.0 })
			{
				Hoop(P, Tin, Z, 22.0 + Z / 32.0, 2.4, 0.9);
			}
			Turned(P, Tin, { FVector2D(64.0, 25.5), FVector2D(67.0, 25.0), FVector2D(71.0, 17.0), FVector2D(72.5, 4.0) }, 20, true, true);
			Rod(P, Iron, FVector(-6.0, 0.0, 74.0), FVector(6.0, 0.0, 74.0), 1.2);
			for (const double S : { -1.0, 1.0 })
			{
				Rod(P, Iron, FVector(S * 6.0, 0.0, 72.0), FVector(S * 6.0, 0.0, 74.0), 0.8);
				Rod(P, Iron, FVector(0.0, S * 23.0, 58.0), FVector(0.0, S * 27.0, 58.0), 1.1);
			}
			Out.Half = FVector(24.0, 24.0, 36.0);
			Out.bRound = true;
			Out.Mass = 12.f;
			break;
		}
		case EKind::Basket:
		{
			// A deep round wicker basket with a handle, full of loaves.
			Turned(P, Wicker, { FVector2D(0.0, 15.0), FVector2D(10.0, 19.0), FVector2D(24.0, 22.0) }, 16, true, false);
			for (double Z = 3.0; Z < 24.0; Z += 4.0)
			{
				Hoop(P, Wicker * 0.75f, Z, 15.0 + Z * 0.29, 1.0, 0.5);
			}
			Hoop(P, Wicker * 0.8f, 24.0, 22.0, 2.4, 0.8);
			// The handle, an arch from rim to rim.
			for (int32 k = 0; k < 8; ++k)
			{
				const double A0 = PI * k / 8.0, A1 = PI * (k + 1) / 8.0;
				Rod(P, Wicker * 0.8f, FVector(0.0, 21.0 * FMath::Cos(A0), 24.0 + 22.0 * FMath::Sin(A0)), FVector(0.0, 21.0 * FMath::Cos(A1), 24.0 + 22.0 * FMath::Sin(A1)), 1.2);
			}
			for (int32 k = 0; k < 4; ++k)
			{
				const double A = 2.0 * PI * k / 4.0 + 0.4;
				Blob(Out.Load, Crust, FVector(9.0 * FMath::Cos(A), 9.0 * FMath::Sin(A), 23.0), FVector(9.0, 6.0, 5.0), 10, 5);
			}
			Out.LumpColor = Crust;
			Out.LumpRadius = 6.5;
			Out.Lumps = 3;
			Out.Half = FVector(22.0, 22.0, 12.0);
			Out.bRound = true;
			Out.Mass = 3.f;
			break;
		}
		default:
			break;
		}
	}
}
