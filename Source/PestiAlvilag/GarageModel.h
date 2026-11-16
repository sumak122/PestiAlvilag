// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BozsiHouseModel.h"
#include "MarketModel.h"

/**
 * The player's lock-up garage on Bérkocsis utca, a few steps from his attic
 * door: a room cut into the house front (Tools/OsmGraybox/city_details.py
 * garage_cut makes the hole; keep CityAt, CityYaw, HalfWidth, Depth and
 * DoorTop the same there), whitewashed inside with a lamp, a bench, a can
 * of petrol and a tyre; a timber door frame with GARÁZS painted over it and
 * two green plank doors that swing out to the street.
 *
 * Header-only, as basic shapes (MarketModel::FShape). The garage's frame:
 * X = 0 on the house front, +X out to the street, the room back to -X,
 * Y across (the door's middle at Y = 0), Z up from the pavement.
 */
namespace GarageModel
{
	using MarketModel::FShape;
	using BozsiHouseModel::FSign;
	using BozsiHouseModel::FLamp;
	using BozsiHouseModel::Add;
	using BozsiHouseModel::Span;

	/** The middle of the door on the house front, and the way out to the street. */
	const FVector2D CityAt(18910.0, 30285.0);
	constexpr double CityYaw = -103.09;

	// Sizes (cm).
	constexpr double HalfWidth = 180.0;    // the room, Y -180..180
	constexpr double Depth = 680.0;        // from the front back to X = -680
	constexpr double DoorTop = 320.0;      // the house goes on over the door from here
	constexpr double DoorHalf = 150.0;     // the opening between the posts
	constexpr double FrameThick = 14.0;
	constexpr double LeafThick = 6.0;

	/** Where a kept car stands in the room (its middle on the floor), facing out. */
	const FVector CarAt(-330.0, 0.0, 0.0);
	constexpr double CarYaw = 0.0;
	/** A car whose middle is this far in from the front counts as in (cm), when it is this slow (cm/s). */
	constexpr double InFrom = -180.0;
	/** The doors' hinges (their outer edge, on the front), Y = -+DoorHalf. */
	inline FVector Hinge(double Side) { return FVector(FrameThick, Side * DoorHalf, 0.0); }

	namespace Colors
	{
		const FLinearColor Whitewash(0.66f, 0.64f, 0.58f);
		const FLinearColor Concrete(0.36f, 0.35f, 0.33f);
		const FLinearColor Oil(0.12f, 0.11f, 0.10f);
		const FLinearColor Timber(0.24f, 0.15f, 0.08f);
		const FLinearColor DoorGreen(0.10f, 0.20f, 0.12f);
		const FLinearColor DoorGreenDark(0.06f, 0.12f, 0.07f);
		const FLinearColor Iron(0.07f, 0.07f, 0.075f);
		const FLinearColor SignBoard(0.10f, 0.08f, 0.06f);
		const FLinearColor SignText(0.86f, 0.82f, 0.70f);
		const FLinearColor Bench(0.30f, 0.18f, 0.09f);
		const FLinearColor Can(0.50f, 0.08f, 0.05f);
		const FLinearColor Tyre(0.04f, 0.04f, 0.04f);
		const FLinearColor Bulb(0.98f, 0.92f, 0.72f);
	}

	/** The room's lining, its things and the door frame (not the doors). */
	inline void Build(TArray<FShape>& Out, TArray<FLamp>& Lamps)
	{
		using namespace Colors;
		const double W = HalfWidth;
		// Floor, with a dark oil patch where cars stand; whitewashed walls and a dark ceiling.
		Span(Out, Concrete, FVector(-Depth, -W, -4), FVector(30, W, 2), false);
		Add(Out, MarketModel::Cylinder, Oil, FVector(-300, 10, 2.5), FVector(130, 90, 1), 0.0, false);
		Span(Out, Whitewash, FVector(-Depth, -W, 2), FVector(-Depth + 4, W, DoorTop), false);
		for (const double Side : { -1.0, 1.0 })
		{
			Span(Out, Whitewash, FVector(-Depth, Side * (W - 4), 2), FVector(0, Side * W, DoorTop), false);
			// A dark band along the foot of the walls, as tyres and boots leave.
			Span(Out, Oil, FVector(-Depth, Side * (W - 5), 2), FVector(0, Side * (W - 4), 40), false);
		}
		Span(Out, Oil, FVector(-Depth, -W, DoorTop - 4), FVector(0, W, DoorTop), false);
		// The lamp over the back wall in its tin shade.
		Add(Out, MarketModel::Cone, Iron, FVector(-Depth + 40, 0, DoorTop - 40), FVector(36, 36, 20), 0.0, false);
		Add(Out, MarketModel::Sphere, Bulb, FVector(-Depth + 40, 0, DoorTop - 52), FVector(12, 12, 12), 0.0, false);
		Span(Out, Iron, FVector(-Depth + 4, -2, DoorTop - 32), FVector(-Depth + 40, 2, DoorTop - 28), false);
		Lamps.Add({ FVector(-Depth + 40, 0, DoorTop - 60), 700.f });
		// A bench along the back wall's left end, a petrol can and a spare tyre.
		Span(Out, Bench, FVector(-Depth + 4, -W + 6, 82), FVector(-Depth + 64, -W + 120, 88));
		for (const double Y : { -W + 12, -W + 112 })
		{
			Span(Out, Bench, FVector(-Depth + 8, Y - 3, 2), FVector(-Depth + 58, Y + 3, 82), false);
		}
		Span(Out, Iron, FVector(-Depth + 14, -W + 40, 88), FVector(-Depth + 30, -W + 70, 96), false);
		Span(Out, Can, FVector(-Depth + 20, W - 60, 2), FVector(-Depth + 40, W - 30, 48), false);
		Add(Out, MarketModel::Cylinder, Tyre, FVector(-Depth + 30, W - 110, 40), FVector(76, 76, 16), 0.0, false, 90.0);

		// The timber frame round the door, its iron bump stones, and the board over it.
		for (const double Side : { -1.0, 1.0 })
		{
			Span(Out, Timber, FVector(0, Side * DoorHalf, 0), FVector(FrameThick, Side * (W + 16), DoorTop + 14));
			Add(Out, MarketModel::Cylinder, Iron, FVector(FrameThick + 12, Side * (DoorHalf + 10), 20), FVector(22, 22, 40));
		}
		Span(Out, Timber, FVector(0, -W - 16, DoorTop - 6), FVector(FrameThick, W + 16, DoorTop + 14), false);
		Span(Out, SignBoard, FVector(0, -130, DoorTop + 30), FVector(4, 130, DoorTop + 80), false);
	}

	/** The painted name on the board over the door. */
	inline void Signs(TArray<FSign>& Out)
	{
		Out.Add({ TEXT("GARÁZS"), FVector(5, 0, DoorTop + 55), 0.0, 34.0, Colors::SignText });
	}

	/**
	 * One door leaf in its hinge's frame: the hinge at the origin, the leaf
	 * running across the door towards the middle (to -Side * Y), shut; its
	 * owner turns it about Z to open it out to the street. Planks, two
	 * braces, iron straps and the handle (with the padlock on the left leaf).
	 */
	inline void BuildLeaf(TArray<FShape>& Out, double Side)
	{
		using namespace Colors;
		const double Wide = DoorHalf - 2.0;
		const double Y0 = -Side * 2.0;
		const double Y1 = -Side * Wide;
		const double Top = DoorTop - 10.0;
		Span(Out, DoorGreen, FVector(-LeafThick, FMath::Min(Y0, Y1), 4), FVector(0, FMath::Max(Y0, Y1), Top));
		// The gaps between the planks.
		for (int32 i = 1; i < 7; ++i)
		{
			const double Y = -Side * (2.0 + Wide * i / 7.0);
			Span(Out, DoorGreenDark, FVector(0, Y - 0.8, 4), FVector(0.6, Y + 0.8, Top), false);
		}
		// Braces across, low and high, and an iron strap on each.
		for (const double Z : { 50.0, Top - 50.0 })
		{
			Span(Out, DoorGreen, FVector(0, FMath::Min(Y0, Y1), Z - 9), FVector(3, FMath::Max(Y0, Y1), Z + 9), false);
			Span(Out, Iron, FVector(3, FMath::Min(Y0, Y1 * 0.55), Z - 3), FVector(4, FMath::Max(Y0, Y1 * 0.55), Z + 3), false);
		}
		// The handle near the leaf's free edge, and on the left leaf the hasp and padlock.
		const double Free = -Side * (Wide - 14.0);
		Span(Out, Iron, FVector(3, Free - 2, 110), FVector(9, Free + 2, 140), false);
		if (Side < 0.0)
		{
			Span(Out, Iron, FVector(3, Free - 10, 152), FVector(5, Free + 18, 158), false);
			Span(Out, Iron, FVector(5, Free + 10, 140), FVector(9, Free + 18, 152), false);
		}
	}
}
