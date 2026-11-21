// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DailyLifeModel.h"
#include "PersonKit.h"

/**
 * A porter's hand cart (kézikocsi) of 1932 Pest: a plank bed with low slatted
 * sides on two tall iron-tyred wheels, two long shafts that run back into
 * handles, a prop leg under the front. Porters (the red-capped "hordár" with
 * his brass number) push them about the streets with a load (ELoad): a
 * traveller's luggage from Keleti, a family's moving (chairs, a rolled
 * mattress, bundled bedding), or sacks. Built in the floor frame (X forward,
 * the way it is pushed; Y right; Z up from the road; the axle's middle at
 * X = Y = 0); the load and each wheel are parts of their own, the wheels
 * built round their hubs.
 */
namespace HandCartModel
{
	using PersonKit::FPiece;

	enum EPart : int32 { Body, Cargo, WheelLeft, WheelRight, NumParts };

	enum class ELoad : uint8 { Luggage, Moving, Sacks, Count };

	constexpr double WheelRadius = 42.0;
	constexpr double WheelY = 50.0;
	/** The bed's floor, its ends and half width (cm). */
	constexpr double BedZ = 56.0;
	constexpr double BedBack = -62.0;
	constexpr double BedFront = 78.0;
	constexpr double BedHalfY = 40.0;
	/** Where the handles' grips are, and how far apart. */
	constexpr double GripX = -150.0;
	constexpr double GripZ = 88.0;
	constexpr double GripY = 30.0;

	/** Half its length, width and height with a load on, for its box (cm). */
	inline FVector CartHalf() { return FVector(118.0, 56.0, 70.0); }
	/** The box's middle in the cart's frame. */
	inline FVector CartMiddle() { return FVector(-36.0, 0.0, 70.0); }
	/** Where the porter stands to push it: his feet, between the handles. */
	inline FVector PorterAt() { return FVector(GripX - 40.0, 0.0, 0.0); }

	inline FVector WheelHub(int32 Part)
	{
		return FVector(0.0, Part == WheelLeft ? -WheelY : WheelY, WheelRadius);
	}

	inline FLinearColor CartWood() { return FLinearColor(0.36f, 0.25f, 0.14f); }
	inline FLinearColor WornWood() { return FLinearColor(0.27f, 0.19f, 0.11f); }
	inline FLinearColor CartIron() { return FLinearColor(0.035f, 0.035f, 0.038f); }
	inline FLinearColor PorterRed() { return FLinearColor(0.42f, 0.04f, 0.03f); }
	inline FLinearColor BadgeBrass() { return FLinearColor(0.60f, 0.43f, 0.14f); }

	/** One wheel round its hub (at the origin): iron tyre, wooden felloe and twelve spokes, the hub's cap. */
	inline void BuildWheel(FPiece& P, double Side)
	{
		using namespace PersonKit;
		TArray<PersonKit::FRing> Tyre;
		Tyre.Add({ FVector(0.0, -2.5, 0.0), FVector(WheelRadius, 0.0, 0.0), FVector(0.0, 0.0, WheelRadius) });
		Tyre.Add({ FVector(0.0, 2.5, 0.0), FVector(WheelRadius, 0.0, 0.0), FVector(0.0, 0.0, WheelRadius) });
		Loft(P, CartIron(), Tyre, 24, false, false);
		DailyLifeModel::Ring(P, WornWood(), FVector::ZeroVector, FVector(WheelRadius - 5.0, 0.0, 0.0), FVector(0.0, 0.0, WheelRadius - 5.0), 2.4, 24);
		for (int32 k = 0; k < 12; ++k)
		{
			const double A = 2.0 * PI * k / 12.0;
			Rod(P, CartWood(), FVector::ZeroVector, FVector(FMath::Cos(A), 0.0, FMath::Sin(A)) * (WheelRadius - 6.0), 1.4, 5);
		}
		DailyLifeModel::Tube(P, WornWood(), FVector(0.0, -7.0, 0.0), FVector(0.0, 7.0, 0.0), 6.0, 10);
		DailyLifeModel::Tube(P, CartIron(), FVector(0.0, 7.0 * Side, 0.0), FVector(0.0, 9.0 * Side, 0.0), 3.5, 8);
	}

	/** The cart without its wheels: bed, slatted sides, shafts and handles, axle, prop leg. */
	inline void BuildCart(FPiece& P)
	{
		using namespace PersonKit;
		const double MidX = (BedFront + BedBack) * 0.5;
		const double HalfX = (BedFront - BedBack) * 0.5;
		// The planks of the bed, with dark gaps between.
		for (int32 k = 0; k < 5; ++k)
		{
			const double Y = -BedHalfY + 8.0 + k * 16.0;
			Block(P, k % 2 ? CartWood() : CartWood() * 0.9f, FVector(MidX, Y, BedZ), FVector(HalfX, 7.6, 1.8));
		}
		// The shafts under the bed and on back into the handles, the grips and the crossbar.
		for (const double S : { -1.0, 1.0 })
		{
			Rod(P, WornWood(), FVector(BedFront + 6.0, S * (BedHalfY - 4.0), BedZ - 4.0), FVector(BedBack, S * (BedHalfY - 4.0), BedZ - 4.0), 3.0, 6);
			Rod(P, WornWood(), FVector(BedBack, S * (BedHalfY - 4.0), BedZ - 4.0), FVector(GripX, S * GripY, GripZ), 2.8, 6);
			Rod(P, FLinearColor(0.20f, 0.13f, 0.07f), FVector(GripX + 16.0, S * (GripY + 0.5), GripZ - 2.3), FVector(GripX - 2.0, S * GripY, GripZ + 0.3), 3.2, 8);
			// Low slatted sides: posts and two rails.
			for (const double X : { BedBack + 3.0, MidX, BedFront - 3.0 })
			{
				Block(P, WornWood(), FVector(X, S * (BedHalfY - 1.5), BedZ + 13.0), FVector(2.0, 1.5, 12.0));
			}
			for (const double Z : { BedZ + 10.0, BedZ + 23.0 })
			{
				Block(P, CartWood(), FVector(MidX, S * (BedHalfY - 1.0), Z), FVector(HalfX, 1.2, 2.4));
			}
			// The axle's iron stays up to the shafts.
			Rod(P, CartIron(), FVector(0.0, S * (BedHalfY - 4.0), BedZ - 6.0), FVector(0.0, S * (WheelY - 8.0), WheelRadius), 1.2, 5);
		}
		// Front and back boards, the crossbar between the handles.
		Block(P, WornWood(), FVector(BedFront - 1.5, 0.0, BedZ + 13.0), FVector(1.5, BedHalfY, 12.0));
		Block(P, WornWood(), FVector(BedBack + 1.5, 0.0, BedZ + 8.0), FVector(1.5, BedHalfY, 7.0));
		Rod(P, WornWood(), FVector(GripX + 30.0, -GripY - 2.0, GripZ - 7.0), FVector(GripX + 30.0, GripY + 2.0, GripZ - 7.0), 1.8, 6);
		// The axle, and the prop leg under the front that it rests on when set down.
		Rod(P, CartIron(), FVector(0.0, -WheelY + 6.0, WheelRadius), FVector(0.0, WheelY - 6.0, WheelRadius), 2.2, 8);
		Rod(P, WornWood(), FVector(BedFront - 10.0, 0.0, BedZ - 4.0), FVector(BedFront - 4.0, 0.0, 10.0), 2.4, 6);
		Block(P, CartIron(), FVector(BedFront - 4.0, 0.0, 9.0), FVector(4.0, 4.0, 1.5));
		// A rope coiled over the back board, to tie the load.
		DailyLifeModel::Ring(P, FLinearColor(0.50f, 0.42f, 0.28f), FVector(BedBack + 1.0, 18.0, BedZ + 10.0), FVector(0.0, 7.0, 0.0), FVector(0.0, 0.0, 7.0), 0.9, 12);
	}

	/** A traveller's luggage: a steamer trunk, suitcases on top, a hat box, a carpet bag on top. */
	inline void BuildLuggage(FPiece& P)
	{
		using namespace PersonKit;
		const FLinearColor Canvas(0.28f, 0.24f, 0.16f);
		const FLinearColor Leather(0.30f, 0.16f, 0.07f);
		const FLinearColor Dark(0.12f, 0.07f, 0.04f);
		const double Z = BedZ + 2.0;
		// The trunk, lying long-ways.
		Block(P, Canvas, FVector(30.0, 0.0, Z + 24.0), FVector(42.0, 30.0, 24.0));
		for (const double X : { -4.0, 30.0, 64.0 })
		{
			Block(P, WornWood(), FVector(X, 0.0, Z + 24.0), FVector(2.0, 30.6, 24.6));
		}
		Block(P, BadgeBrass(), FVector(72.6, 0.0, Z + 40.0), FVector(0.5, 3.0, 3.5));
		// Two suitcases flat on the trunk, a third one standing behind it.
		Block(P, Leather, FVector(26.0, -6.0, Z + 54.0), FVector(30.0, 20.0, 6.0));
		Block(P, Dark, FVector(26.0, -6.0, Z + 54.0), FVector(30.4, 20.4, 0.7));
		Block(P, FLinearColor(0.18f, 0.20f, 0.12f), FVector(30.0, 2.0, Z + 65.0), FVector(22.0, 15.0, 5.0));
		Rod(P, Dark, FVector(30.0, -4.0, Z + 71.0), FVector(30.0, 8.0, 71.0 + Z), 1.0, 5);
		Block(P, Leather * 1.2f, FVector(-30.0, -16.0, Z + 25.0), FVector(9.0, 22.0, 25.0));
		Rod(P, Dark, FVector(-30.0, -22.0, Z + 52.0), FVector(-30.0, -10.0, Z + 52.0), 1.0, 5);
		// A round hat box behind, a carpet bag on top.
		DailyLifeModel::Tube(P, FLinearColor(0.70f, 0.55f, 0.55f), FVector(-34.0, 22.0, Z), FVector(-34.0, 22.0, Z + 22.0), 16.0, 14);
		DailyLifeModel::Tube(P, FLinearColor(0.35f, 0.05f, 0.08f), FVector(-34.0, 22.0, Z + 20.0), FVector(-34.0, 22.0, Z + 24.0), 16.6, 14);
		Blob(P, FLinearColor(0.38f, 0.14f, 0.10f), FVector(16.0, -4.0, Z + 76.0), FVector(16.0, 11.0, 7.0), 10, 5);
	}

	/** A family's moving: a rolled mattress, two chairs upside down on a table, bundled bedding, a clock and a bucket. */
	inline void BuildMoving(FPiece& P)
	{
		using namespace PersonKit;
		const FLinearColor Ticking(0.62f, 0.60f, 0.52f);
		const FLinearColor Stripe(0.22f, 0.26f, 0.42f);
		const FLinearColor Varnish(0.30f, 0.15f, 0.07f);
		const double Z = BedZ + 2.0;
		// The table on its feet over the front of the bed, its top piled.
		Block(P, Varnish, FVector(40.0, 0.0, Z + 60.0), FVector(36.0, 36.0, 2.0));
		for (const double X : { 8.0, 72.0 })
		{
			for (const double Y : { -32.0, 32.0 })
			{
				Block(P, Varnish * 0.8f, FVector(X, Y, Z + 29.0), FVector(2.0, 2.0, 29.0));
			}
		}
		// Two chairs on the table, seat down and legs up.
		for (const double Y : { -16.0, 18.0 })
		{
			const double CZ = Z + 62.0;
			Block(P, Varnish * 1.1f, FVector(20.0, Y, CZ + 2.0), FVector(17.0, 16.0, 1.5));
			for (const double LX : { 6.0, 34.0 })
			{
				for (const double LY : { -13.0, 13.0 })
				{
					Rod(P, Varnish, FVector(LX, Y + LY, CZ + 3.0), FVector(LX, Y + LY, CZ + 44.0), 1.2, 5);
				}
			}
			// Its back hangs down past the table's edge.
			Block(P, Varnish, FVector(1.5, Y, CZ - 16.0), FVector(1.5, 15.0, 18.0));
		}
		// The rolled mattress across the back, tied with rope; a bundle of bedding in a striped sheet beside it.
		DailyLifeModel::Tube(P, Ticking, FVector(-40.0, -38.0, Z + 18.0), FVector(-40.0, 38.0, Z + 18.0), 18.0, 14);
		for (const double Y : { -26.0, 26.0 })
		{
			DailyLifeModel::Ring(P, FLinearColor(0.50f, 0.42f, 0.28f), FVector(-40.0, Y, Z + 18.0), FVector(18.6, 0.0, 0.0), FVector(0.0, 0.0, 18.6), 0.8, 14);
		}
		for (int32 k = -2; k <= 2; ++k)
		{
			Block(P, Stripe, FVector(-40.0, k * 15.0, Z + 18.0 + 18.2), FVector(15.0, 1.5, 0.3));
		}
		Blob(P, FLinearColor(0.70f, 0.66f, 0.58f), FVector(-36.0, 4.0, Z + 46.0), FVector(20.0, 26.0, 14.0), 12, 6);
		Blob(P, FLinearColor(0.68f, 0.30f, 0.24f), FVector(-34.0, 6.0, Z + 59.0), FVector(5.0, 5.0, 4.0), 6, 3);
		// A wall clock lying on the bedding, a zinc bucket hung on the front board.
		Block(P, Varnish * 0.7f, FVector(-30.0, -14.0, Z + 62.0), FVector(14.0, 7.0, 3.0));
		DailyLifeModel::Tube(P, FLinearColor(0.80f, 0.76f, 0.66f), FVector(-22.0, -14.0, Z + 64.5), FVector(-22.0, -14.0, Z + 65.5), 5.0, 12);
		DailyLifeModel::Tube(P, FLinearColor(0.42f, 0.44f, 0.45f), FVector(BedFront + 11.0, -18.0, Z - 2.0), FVector(BedFront + 11.0, -18.0, Z + 20.0), 10.0, 12);
		DailyLifeModel::Ring(P, FLinearColor(0.42f, 0.44f, 0.45f), FVector(BedFront + 11.0, -18.0, Z + 20.0), FVector(10.0, 0.0, 0.0), FVector(0.0, 0.0, 8.0), 0.5, 10);
	}

	/** Sacks of potatoes and flour, a few stood on end, a few lying on top. */
	inline void BuildSacks(FPiece& P)
	{
		using namespace PersonKit;
		const FLinearColor Jute(0.48f, 0.38f, 0.22f);
		const FLinearColor Flour(0.74f, 0.72f, 0.64f);
		const double Z = BedZ + 2.0;
		int32 n = 0;
		for (const double X : { -42.0, -6.0, 30.0, 62.0 })
		{
			for (const double Y : { -20.0, 20.0 })
			{
				const FLinearColor C = (n % 3 == 1) ? Flour : Jute * (n % 2 ? 1.f : 0.9f);
				Blob(P, C, FVector(X, Y, Z + 22.0), FVector(15.0, 17.0, 24.0), 10, 6, 0.0);
				Blob(P, C * 0.9f, FVector(X, Y, Z + 46.0), FVector(6.0, 7.0, 4.0), 8, 3);
				++n;
			}
		}
		for (const double X : { -24.0, 26.0 })
		{
			Blob(P, Jute * 0.95f, FVector(X, 0.0, Z + 54.0), FVector(26.0, 17.0, 12.0), 10, 5);
		}
	}

	inline void BuildLoad(FPiece& P, ELoad Kind)
	{
		switch (Kind)
		{
		case ELoad::Luggage: BuildLuggage(P); break;
		case ELoad::Moving: BuildMoving(P); break;
		case ELoad::Sacks: BuildSacks(P); break;
		default: break;
		}
	}

	/** All its parts: the cart, its load (both in the cart's frame), each wheel round its hub. */
	inline void Build(FPiece (&Parts)[NumParts], ELoad Kind)
	{
		BuildCart(Parts[Body]);
		BuildLoad(Parts[Cargo], Kind);
		BuildWheel(Parts[WheelLeft], -1.0);
		BuildWheel(Parts[WheelRight], 1.0);
	}

	/**
	 * The porter (hordár): the red cap with the brass number, a dark blue
	 * jacket, a leather strap over the shoulder; Pick makes each a little
	 * different.
	 */
	inline PersonKit::FOutfit PorterLook(int32 Pick)
	{
		using namespace PersonKit;
		FOutfit O;
		O.Height = 170.f + (Pick % 4) * 3.f;
		O.Build = 1.0f + (Pick % 3) * 0.06f;
		O.Hat = EHat::PeakedCap;
		O.HatColor = PorterRed();
		O.HatBand = BadgeBrass();
		O.Top = ETop::Jacket;
		O.Coat = Pick % 2 ? FLinearColor(0.07f, 0.09f, 0.18f) : FLinearColor(0.10f, 0.10f, 0.12f);
		O.Waistcoat = FLinearColor(0.08f, 0.07f, 0.06f);
		O.Shirt = FLinearColor(0.70f, 0.68f, 0.62f);
		O.bTie = false;
		O.bScarf = Pick % 3 == 0;
		O.Scarf = FLinearColor(0.30f, 0.26f, 0.20f);
		O.bSash = true;
		O.Sash = FLinearColor(0.22f, 0.12f, 0.05f);
		O.bMoustache = Pick % 4 != 2;
		O.bAged = Pick % 5 == 0;
		O.bBoots = true;
		O.Trousers = FLinearColor(0.08f, 0.08f, 0.09f);
		return O;
	}
}
