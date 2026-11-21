// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CityDetailsModel.h"

/**
 * The street's clutter and the people at the windows, as basic shapes
 * (MarketModel::FShape) that ACityDetails gathers into a few sets:
 * - pillar fire hydrants at the kerb, iron manhole covers in the roadway,
 *   cellar hatches in the pavement against the house fronts, stone horse
 *   troughs with a pump by the cab stands and markets;
 * - the coal man's drop: a cellar hatch standing open, a wooden chute into
 *   it, his handcart with the sacks, sacks on the pavement and spilt coal
 *   (ACityDetails stands the coal man himself by it);
 * - litter (ELitter): newspaper scraps and fallen leaves in the gutter,
 *   horse dung on the road, rubbish heaps in the corners;
 * - people at the open windows (FolkKind): an old woman in her black
 *   kerchief, a man in his shirt sleeves, a young woman, each leaning on a
 *   cushion on the sill; a feather duvet put out to air; a cat on the sill.
 *
 * Header-only so it can be built and looked at outside the engine. Each
 * thing's frame: +X out (from the wall to the street, or across the
 * pavement), Z up from the ground (for the window things: from the bottom
 * of the window's glass, at the wall face).
 */
namespace CityLifeModel
{
	using MarketModel::FShape;
	using BozsiHouseModel::Add;
	using CityDetailsModel::Rod;

	namespace Colors
	{
		const FLinearColor HydrantPaint(0.09f, 0.13f, 0.10f);
		const FLinearColor Brass(0.55f, 0.38f, 0.12f);
		const FLinearColor Iron(0.035f, 0.035f, 0.038f);
		const FLinearColor IronWorn(0.09f, 0.085f, 0.08f);
		const FLinearColor TroughStone(0.48f, 0.46f, 0.42f);
		const FLinearColor Water(0.03f, 0.05f, 0.06f);
		const FLinearColor Coal(0.018f, 0.018f, 0.02f);
		const FLinearColor Jute(0.16f, 0.14f, 0.12f);
		const FLinearColor JuteDusty(0.10f, 0.095f, 0.09f);
		const FLinearColor CartWood(0.28f, 0.20f, 0.12f);
		const FLinearColor Hole(0.01f, 0.01f, 0.01f);
		const FLinearColor ChuteWood(0.20f, 0.15f, 0.10f);
		// Skin and clothes for the people at the windows.
		const FLinearColor Skin(0.62f, 0.42f, 0.32f);
		const FLinearColor SkinOld(0.55f, 0.38f, 0.30f);
		const FLinearColor Black(0.03f, 0.03f, 0.03f);
		const FLinearColor Shirt(0.78f, 0.77f, 0.72f);
		const FLinearColor Waistcoat(0.10f, 0.09f, 0.08f);
		const FLinearColor Hair(0.07f, 0.045f, 0.03f);
		const FLinearColor HairGrey(0.45f, 0.44f, 0.42f);
		const FLinearColor Dress(0.22f, 0.30f, 0.45f);
		const FLinearColor DressFlower(0.55f, 0.28f, 0.25f);
		const FLinearColor CushionRed(0.45f, 0.08f, 0.06f);
		const FLinearColor CushionGreen(0.15f, 0.25f, 0.12f);
		const FLinearColor Duvet(0.86f, 0.85f, 0.80f);
		const FLinearColor DuvetTick(0.62f, 0.25f, 0.22f);
		const FLinearColor CatGinger(0.55f, 0.28f, 0.08f);
		const FLinearColor CatGrey(0.22f, 0.22f, 0.22f);
		// Litter.
		const FLinearColor Newsprint(0.66f, 0.64f, 0.56f);
		const FLinearColor LeafBrown(0.30f, 0.17f, 0.06f);
		const FLinearColor LeafYellow(0.52f, 0.36f, 0.09f);
		const FLinearColor Dung(0.12f, 0.085f, 0.045f);
		const FLinearColor Straw(0.50f, 0.42f, 0.22f);
		const FLinearColor Cabbage(0.33f, 0.42f, 0.18f);
		const FLinearColor TinCan(0.36f, 0.36f, 0.34f);
	}

	/** Moves shapes built in a thing's own frame into a set's frame: turned by Yaw, then put at Origin. */
	inline void Place(const TArray<FShape>& Local, const FVector& Origin, double Yaw, TArray<FShape>& Out)
	{
		const FRotator Turn(0.0, Yaw, 0.0);
		for (const FShape& Piece : Local)
		{
			FShape Moved = Piece;
			Moved.At = Origin + Turn.RotateVector(Piece.At);
			// The set's turn is about Z, the outermost of a rotator's turns, so it simply adds to the yaw.
			Moved.Yaw = Piece.Yaw + Yaw;
			Out.Add(Moved);
		}
	}

	/** Litter kinds, as city_details.py place_litter numbers them. */
	enum class ELitter : int32
	{
		Papers,
		Leaves,
		Dung,
		Heap
	};

	/** A bit of litter, Seed varying it; nothing to bump into. */
	inline void Litter(ELitter Kind, uint32 Seed, TArray<FShape>& Out)
	{
		using namespace Colors;
		MarketModel::FRandom R(Seed * 2654435761u + 977u);
		switch (Kind)
		{
		case ELitter::Papers:
			// A torn newspaper and some scraps blown against the kerb.
			Add(Out, MarketModel::Cube, Newsprint, FVector(0, 0, 1.0), FVector(38, 52, 0.6), R.Range(-30.0, 30.0), false, 0.0, R.Range(-4.0, 4.0));
			Add(Out, MarketModel::Cube, Newsprint * 0.85f, FVector(6, 18, 2.0), FVector(30, 26, 0.6), R.Range(0.0, 90.0), false, 5.0, 0.0);
			for (int32 i = 0; i < 4; ++i)
			{
				Add(Out, MarketModel::Cube, i % 2 ? Newsprint : Newsprint * 0.75f, FVector(R.Range(-20.0, 20.0), R.Range(-90.0, 90.0), 0.8),
					FVector(R.Range(8.0, 16.0), R.Range(8.0, 14.0), 0.5), R.Range(0.0, 180.0), false);
			}
			break;
		case ELitter::Leaves:
			// A drift of plane leaves along the kerb.
			Add(Out, MarketModel::Sphere, LeafBrown, FVector(4, 0, 0), FVector(50, R.Range(110.0, 170.0), 12), 0.0, false);
			for (int32 i = 0; i < 9; ++i)
			{
				Add(Out, MarketModel::Cube, i % 3 == 0 ? LeafYellow : LeafBrown * (i % 2 ? 1.2f : 0.9f),
					FVector(R.Range(-30.0, 25.0), R.Range(-110.0, 110.0), R.Range(1.0, 5.0)), FVector(14, 12, 0.6), R.Range(0.0, 180.0), false, R.Range(-10.0, 10.0), 0.0);
			}
			break;
		case ELitter::Dung:
			// Horse dung, a few balls and a smear, a wisp of straw.
			for (int32 i = 0; i < 5; ++i)
			{
				const double Size = R.Range(9.0, 13.0);
				Add(Out, MarketModel::Sphere, Dung * (i % 2 ? 1.2f : 1.0f), FVector(R.Range(-16.0, 16.0), R.Range(-16.0, 16.0), Size * 0.3), FVector(Size, Size, Size * 0.75), 0.0, false);
			}
			Add(Out, MarketModel::Sphere, Dung * 0.8f, FVector(0, 0, 0), FVector(44, 36, 4), R.Range(0.0, 180.0), false);
			Add(Out, MarketModel::Cube, Straw, FVector(R.Range(-20.0, 20.0), 24, 1.0), FVector(22, 1.5, 0.8), R.Range(0.0, 180.0), false);
			break;
		case ELitter::Heap:
			// Rubbish put out in the corner: a broken crate, cabbage leaves, paper, a tin.
			Add(Out, MarketModel::Cube, CartWood, FVector(-10, 8, 18), FVector(40, 50, 36), 20.0, false, 0.0, 12.0);
			Add(Out, MarketModel::Cube, CartWood * 0.7f, FVector(18, -22, 3), FVector(46, 8, 3), -35.0, false);
			Add(Out, MarketModel::Sphere, Cabbage, FVector(20, 22, 6), FVector(26, 22, 12), 40.0, false);
			Add(Out, MarketModel::Sphere, Cabbage * 0.8f, FVector(30, -6, 5), FVector(18, 16, 9), 0.0, false);
			Add(Out, MarketModel::Cube, Newsprint, FVector(28, 6, 2), FVector(34, 26, 0.6), 15.0, false, 6.0, 0.0);
			Add(Out, MarketModel::Cube, Newsprint * 0.8f, FVector(-2, -30, 1), FVector(20, 18, 0.6), 60.0, false);
			Add(Out, MarketModel::Cylinder, TinCan, FVector(40, 30, 5), FVector(10, 10, 14), 0.0, false, 90.0, 0.0);
			Add(Out, MarketModel::Sphere, LeafBrown, FVector(-4, -8, 0), FVector(60, 60, 14), 0.0, false);
			break;
		}
	}

	// ---------------------------------------------------------------------
	// Street clutter.

	/** A cast iron pillar hydrant: domed top, a big outlet to the street and two small ones. */
	inline void Hydrant(TArray<FShape>& Out)
	{
		using namespace Colors;
		Add(Out, MarketModel::Cylinder, HydrantPaint, FVector(0, 0, 4), FVector(34, 34, 8));
		Add(Out, MarketModel::Cylinder, HydrantPaint, FVector(0, 0, 44), FVector(22, 22, 74));
		Add(Out, MarketModel::Cylinder, HydrantPaint, FVector(0, 0, 76), FVector(28, 28, 6), 0.0, false);
		Add(Out, MarketModel::Sphere, HydrantPaint, FVector(0, 0, 82), FVector(24, 24, 18), 0.0, false);
		Add(Out, MarketModel::Cylinder, Brass, FVector(0, 0, 92), FVector(7, 7, 6), 0.0, false);
		Rod(Out, HydrantPaint, FVector(8, 0, 56), FVector(22, 0, 56), 12.0);
		Add(Out, MarketModel::Cylinder, Brass, FVector(23, 0, 56), FVector(14, 14, 3), 0.0, false, 90.0);
		for (const double Side : { -1.0, 1.0 })
		{
			Rod(Out, HydrantPaint, FVector(0, Side * 8, 62), FVector(0, Side * 17, 62), 7.0);
			Add(Out, MarketModel::Cube, Brass, FVector(0, Side * 18, 62), FVector(6, 2, 6), 0.0, false);
		}
	}

	/** Cast iron painted green, for the public wells and the street urinals. */
	const FLinearColor StreetIronGreen(0.07f, 0.14f, 0.09f);
	const FLinearColor PlateBlue(0.05f, 0.14f, 0.42f);
	const FLinearColor PlateWhite(0.85f, 0.84f, 0.80f);
	const FLinearColor Drain(0.30f, 0.29f, 0.27f);

	/** How high a street name plate hangs on the corner house (cm, its middle). */
	constexpr double PlateHeight = 432.0;

	/** A blue enamel street name plate on a house wall, Width wide; the wall is at X = 0, the plate faces +X. The name is lettered by the caller at PlateLetterX. */
	constexpr double PlateLetterX = 2.6;
	inline void StreetPlate(double Width, TArray<FShape>& Out)
	{
		Add(Out, MarketModel::Cube, PlateWhite, FVector(0.8, 0, PlateHeight), FVector(1.6, Width, 30.0), 0.0, false);
		Add(Out, MarketModel::Cube, PlateBlue, FVector(1.8, 0, PlateHeight), FVector(0.6, Width - 3.0, 27.0), 0.0, false);
	}

	/**
	 * A public street well (közkifolyó): a fluted cast iron column with a cap and finial,
	 * a spout to the front over an iron drain grating, the pump handle at the side.
	 */
	inline void StreetWell(TArray<FShape>& Out)
	{
		using namespace Colors;
		Add(Out, MarketModel::Cube, Drain, FVector(28, 0, 1), FVector(70, 60, 2), 0.0, false);
		for (int32 k = 0; k < 6; ++k)
		{
			Add(Out, MarketModel::Cube, Iron, FVector(28, -24 + k * 9.6, 2.2), FVector(56, 3, 0.6), 0.0, false);
		}
		Add(Out, MarketModel::Cylinder, StreetIronGreen, FVector(0, 0, 10), FVector(40, 40, 20));
		Add(Out, MarketModel::Cylinder, StreetIronGreen, FVector(0, 0, 24), FVector(32, 32, 8), 0.0, false);
		Add(Out, MarketModel::Cylinder, StreetIronGreen, FVector(0, 0, 85), FVector(24, 24, 122));
		for (int32 k = 0; k < 8; ++k)
		{
			const double Angle = k * PI / 4.0;
			Add(Out, MarketModel::Cube, StreetIronGreen, FVector(FMath::Cos(Angle) * 12.0, FMath::Sin(Angle) * 12.0, 85), FVector(3, 3, 110), k * 45.0, false);
		}
		Add(Out, MarketModel::Cylinder, StreetIronGreen, FVector(0, 0, 150), FVector(32, 32, 8), 0.0, false);
		Add(Out, MarketModel::Sphere, StreetIronGreen, FVector(0, 0, 160), FVector(28, 28, 20), 0.0, false);
		Add(Out, MarketModel::Sphere, Brass, FVector(0, 0, 175), FVector(10, 10, 12), 0.0, false);
		// The spout: a lion's head boss and a curved pipe.
		Add(Out, MarketModel::Sphere, StreetIronGreen, FVector(12, 0, 105), FVector(10, 16, 16), 0.0, false);
		Rod(Out, StreetIronGreen, FVector(14, 0, 102), FVector(30, 0, 96), 5.0);
		Rod(Out, StreetIronGreen, FVector(30, 0, 96), FVector(36, 0, 86), 5.0);
		// The handle, pivoted on the side of the column, its end hanging down.
		Add(Out, MarketModel::Cylinder, Brass, FVector(0, 14, 120), FVector(8, 8, 4), 0.0, false, 0.0, 90.0);
		Rod(Out, Iron, FVector(0, 16, 120), FVector(-6, 42, 112), 3.0);
		Rod(Out, Iron, FVector(-6, 42, 112), FVector(-8, 48, 92), 3.0);
		Add(Out, MarketModel::Sphere, Iron, FVector(-8, 48, 90), FVector(6, 6, 8), 0.0, false);
	}

	/**
	 * A cast iron street urinal (piszoár) as on the squares of Pest: a round screen of
	 * panels on legs, open under and over, a pierced frieze, a little roof on a middle post
	 * with a lamp on top; the way in at the front is hidden by a curved screen.
	 */
	inline void Pissoir(TArray<FShape>& Out)
	{
		using namespace Colors;
		constexpr double Radius = 115.0;
		constexpr int32 Panels = 12;
		for (int32 k = 0; k < Panels; ++k)
		{
			// Leave the front two panels out: the way in, behind its own screen.
			if (k == 0 || k == Panels - 1)
			{
				continue;
			}
			const double Angle = (k + 0.5) * 2.0 * PI / Panels;
			const double Wide = 2.0 * Radius * FMath::Sin(PI / Panels) + 1.0;
			const FVector At(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0);
			const double Facing = FMath::RadiansToDegrees(Angle);
			Add(Out, MarketModel::Cube, StreetIronGreen, At + FVector(0, 0, 112), FVector(4, Wide, 156), Facing);
			Add(Out, MarketModel::Cube, IronWorn, At + FVector(0, 0, 196), FVector(5, Wide, 4), Facing, false);
			// The pierced frieze: little uprights between two rails.
			Add(Out, MarketModel::Cube, StreetIronGreen, At + FVector(0, 0, 222), FVector(5, Wide, 3), Facing, false);
			Add(Out, MarketModel::Cube, StreetIronGreen, At + FVector(0, 0, 200) + FVector(-FMath::Sin(Angle), FMath::Cos(Angle), 0) * (Wide * 0.25), FVector(3, 3, 22), Facing, false);
			Add(Out, MarketModel::Cube, StreetIronGreen, At + FVector(0, 0, 200) - FVector(-FMath::Sin(Angle), FMath::Cos(Angle), 0) * (Wide * 0.25), FVector(3, 3, 22), Facing, false);
		}
		// Legs at the panel joints.
		for (int32 k = 1; k < Panels; ++k)
		{
			const double Angle = k * 2.0 * PI / Panels;
			Add(Out, MarketModel::Cylinder, StreetIronGreen, FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 115), FVector(7, 7, 230), 0.0, false);
		}
		// The screen in front of the way in.
		Add(Out, MarketModel::Cube, StreetIronGreen, FVector(Radius + 60.0, 0, 112), FVector(4, 110, 156), 0.0, true);
		for (const double Side : { -1.0, 1.0 })
		{
			Add(Out, MarketModel::Cylinder, StreetIronGreen, FVector(Radius + 60.0, Side * 55.0, 100), FVector(7, 7, 200), 0.0, false);
		}
		// Middle post, roof, lamp and finial.
		Add(Out, MarketModel::Cylinder, StreetIronGreen, FVector(0, 0, 130), FVector(14, 14, 260), 0.0, false);
		Add(Out, MarketModel::Cylinder, Iron, FVector(0, 0, 244), FVector(Radius * 2.0 + 40.0, Radius * 2.0 + 40.0, 6), 0.0, false);
		Add(Out, MarketModel::Cone, StreetIronGreen, FVector(0, 0, 268), FVector(Radius * 2.0 + 30.0, Radius * 2.0 + 30.0, 44), 0.0, false);
		Add(Out, MarketModel::Cylinder, StreetIronGreen, FVector(0, 0, 296), FVector(16, 16, 16), 0.0, false);
		Add(Out, MarketModel::Cube, FLinearColor(0.80f, 0.74f, 0.52f), FVector(0, 0, 312), FVector(16, 16, 18), 45.0, false);
		Add(Out, MarketModel::Cone, StreetIronGreen, FVector(0, 0, 328), FVector(24, 24, 14), 0.0, false);
		Add(Out, MarketModel::Sphere, Brass, FVector(0, 0, 339), FVector(6, 6, 8), 0.0, false);
		// Wet stone floor inside.
		Add(Out, MarketModel::Cylinder, Drain, FVector(0, 0, 1), FVector(Radius * 2.0, Radius * 2.0, 2), 0.0, false);
	}

	/** An iron manhole cover, flush with the roadway. */
	inline void Manhole(int32 Variant, TArray<FShape>& Out)
	{
		using namespace Colors;
		Add(Out, MarketModel::Cylinder, IronWorn, FVector(0, 0, 0.6), FVector(78, 78, 1.2), 0.0, false);
		Add(Out, MarketModel::Cylinder, Iron, FVector(0, 0, 1.0), FVector(66, 66, 1.2), 0.0, false);
		if (Variant % 2 == 0)
		{
			// Ribs across, the town's pattern.
			for (const double Y : { -18.0, 0.0, 18.0 })
			{
				Add(Out, MarketModel::Cube, IronWorn, FVector(0, Y, 1.4), FVector(56, 4, 1.0), 0.0, false);
			}
		}
		else
		{
			Add(Out, MarketModel::Cylinder, IronWorn, FVector(0, 0, 1.4), FVector(30, 30, 1.0), 0.0, false);
		}
	}

	/** A cellar hatch: two iron leaves in a frame on the pavement against the wall (Variant 1: a wooden one). */
	inline void CellarHatch(int32 Variant, TArray<FShape>& Out)
	{
		using namespace Colors;
		const FLinearColor Leaf = Variant % 2 == 0 ? Iron : CartWood * 0.7f;
		Add(Out, MarketModel::Cube, IronWorn, FVector(42, 0, 0.8), FVector(84, 128, 1.6), 0.0, false);
		for (const double Y : { -31.0, 31.0 })
		{
			Add(Out, MarketModel::Cube, Leaf, FVector(42, Y, 1.4), FVector(76, 58, 1.6), 0.0, false);
			Add(Out, MarketModel::Cube, IronWorn, FVector(42, Y, 2.3), FVector(70, 3, 0.8), 0.0, false);
			Add(Out, MarketModel::Cube, Brass * 0.5f, FVector(66, Y * 0.85, 2.4), FVector(6, 8, 1.0), 0.0, false);
		}
	}

	/** A stone horse trough with a pump at one end; it runs along Y, the street side at +X. */
	inline void Trough(TArray<FShape>& Out)
	{
		using namespace Colors;
		constexpr double Long = 230.0;
		constexpr double Wide = 72.0;
		constexpr double High = 66.0;
		// Walls of the basin and the water in it.
		Add(Out, MarketModel::Cube, TroughStone, FVector(0, 0, 8), FVector(Wide + 6, Long + 6, 16));
		Add(Out, MarketModel::Cube, TroughStone, FVector(Wide * 0.5 - 6, 0, High * 0.5), FVector(12, Long, High));
		Add(Out, MarketModel::Cube, TroughStone, FVector(-Wide * 0.5 + 6, 0, High * 0.5), FVector(12, Long, High));
		Add(Out, MarketModel::Cube, TroughStone, FVector(0, Long * 0.5 - 6, High * 0.5), FVector(Wide, 12, High));
		Add(Out, MarketModel::Cube, TroughStone, FVector(0, -Long * 0.5 + 6, High * 0.5), FVector(Wide, 12, High));
		Add(Out, MarketModel::Cube, Water, FVector(0, 0, High - 10), FVector(Wide - 20, Long - 20, 2), 0.0, false);
		// The pump post with its spout over the water.
		const double PumpY = -Long * 0.5 - 22.0;
		Add(Out, MarketModel::Cylinder, HydrantPaint, FVector(0, PumpY, 70), FVector(22, 22, 140));
		Add(Out, MarketModel::Sphere, HydrantPaint, FVector(0, PumpY, 144), FVector(26, 26, 20), 0.0, false);
		Rod(Out, HydrantPaint, FVector(0, PumpY + 8, 105), FVector(0, PumpY + 40, 92), 6.0);
		Rod(Out, HydrantPaint, FVector(0, PumpY - 6, 120), FVector(-30, PumpY - 40, 150), 4.0);
	}

	/** A full coal sack stood up, its middle at At: a lumpy body, the gathered neck and the tie. */
	inline void CoalSack(TArray<FShape>& Out, const FLinearColor& Cloth, const FVector& At, double Yaw, double Lean, bool bCollide)
	{
		Add(Out, MarketModel::Cube, Cloth, At, FVector(42, 32, 50), Yaw, bCollide, Lean);
		Add(Out, MarketModel::Sphere, Cloth, At + FVector(0, 0, -20), FVector(46, 36, 22), Yaw, false);
		Add(Out, MarketModel::Sphere, Cloth, At + FVector(0, 0, 24), FVector(40, 30, 22), Yaw, false);
		Add(Out, MarketModel::Sphere, Cloth, At + FVector(0, 0, 36), FVector(16, 13, 12), Yaw, false);
		Add(Out, MarketModel::Cylinder, Cloth * 0.55f, At + FVector(0, 0, 33), FVector(14, 12, 3), Yaw, false);
	}

	/**
	 * The coal man's drop at a house front (the wall foot at the origin): the
	 * hatch open, a chute into it, his handcart with sacks along the kerb side,
	 * sacks waiting and coal spilt about. Seed varies how the sacks lie.
	 */
	inline void CoalDrop(uint32 Seed, TArray<FShape>& Out)
	{
		using namespace Colors;
		MarketModel::FRandom Random(Seed);
		// The hole, its frame, and the two leaves stood up against the wall side.
		Add(Out, MarketModel::Cube, IronWorn, FVector(42, 0, 0.8), FVector(84, 128, 1.6), 0.0, false);
		Add(Out, MarketModel::Cube, Hole, FVector(44, 0, 1.2), FVector(70, 112, 1.6), 0.0, false);
		for (const double Side : { -1.0, 1.0 })
		{
			Add(Out, MarketModel::Cube, Iron, FVector(44, Side * 62, 30), FVector(76, 2, 58), 0.0, false);
		}
		// The chute from the pavement down into the cellar.
		Add(Out, MarketModel::Cube, ChuteWood, FVector(80, 0, 22), FVector(110, 60, 3), 0.0, false, 24.0);
		for (const double Side : { -1.0, 1.0 })
		{
			Add(Out, MarketModel::Cube, ChuteWood, FVector(80, Side * 31, 28), FVector(110, 3, 12), 0.0, false, 24.0);
		}
		// A sack being emptied down the chute.
		Add(Out, MarketModel::Cube, Jute, FVector(130, 0, 54), FVector(32, 40, 56), 0.0, false, 62.0);
		Add(Out, MarketModel::Sphere, Jute, FVector(150, 0, 66), FVector(30, 44, 30), 0.0, false);
		// Spilt coal round the hatch and on the chute.
		for (int32 i = 0; i < 9; ++i)
		{
			const FVector At(Random.Range(20.0, 160.0), Random.Range(-90.0, 90.0), 2.0);
			const double Size = Random.Range(8.0, 22.0);
			Add(Out, MarketModel::Sphere, Coal, At, FVector(Size, Size * 1.2, Size * 0.5), Random.Range(0.0, 180.0), false);
		}
		// Sacks standing on the pavement, waiting their turn.
		for (int32 i = 0; i < 3; ++i)
		{
			const FVector At(120.0 + Random.Range(-10.0, 10.0), -120.0 - i * 44.0, 30.0);
			CoalSack(Out, i % 2 ? JuteDusty : Jute, At, Random.Range(-30.0, 30.0), Random.Range(-6.0, 6.0), true);
		}
		// The handcart beside the hatch: a flat bed on two big wheels, the
		// shafts resting on the pavement, the sacks stood up on the bed.
		constexpr double CartX = 150.0;
		constexpr double CartY = 300.0;
		constexpr double Axle = 50.0;
		constexpr double Tilt = 7.0;    // the bed slopes down to the shafts' end (+Y)
		Add(Out, MarketModel::Cube, CartWood, FVector(CartX, CartY, 64), FVector(110, 170, 8), 0.0, true, 0.0, Tilt);
		for (const double Side : { -1.0, 1.0 })
		{
			Add(Out, MarketModel::Cube, CartWood, FVector(CartX + Side * 54, CartY, 76), FVector(4, 170, 20), 0.0, false, 0.0, Tilt);
			Add(Out, MarketModel::Cylinder, Iron, FVector(CartX + Side * 66, CartY - 20, Axle), FVector(100, 100, 6), 0.0, false, 90.0);
			Add(Out, MarketModel::Cylinder, CartWood, FVector(CartX + Side * 66, CartY - 20, Axle), FVector(26, 26, 10), 0.0, false, 90.0);
			Rod(Out, CartWood, FVector(CartX + Side * 40, CartY + 80, 58), FVector(CartX + Side * 30, CartY + 230, 6), 6.0);
		}
		Add(Out, MarketModel::Cube, CartWood, FVector(CartX, CartY - 86, 76), FVector(110, 4, 20), 0.0, false);
		Rod(Out, Iron, FVector(CartX - 66, CartY - 20, Axle), FVector(CartX + 66, CartY - 20, Axle), 5.0);
		for (int32 i = 0; i < 4; ++i)
		{
			const FVector At(CartX + (i % 2 ? 24.0 : -24.0), CartY - 45.0 + (i / 2) * 60.0, 98.0 - (i / 2) * 6.0);
			CoalSack(Out, i % 3 ? Jute : JuteDusty, At, Random.Range(-30.0, 30.0), Random.Range(-5.0, 5.0), false);
		}
	}

	// ---------------------------------------------------------------------
	// People and things at the windows.

	enum class EFolk : uint8 { OldWoman, Man, YoungWoman, Duvet, Cat, NumKinds };

	/** Shoulders, arms on a cushion and a head over the sill, leaning out: the window's frame, the sill's top at Z 0. */
	inline void Leaner(const FLinearColor& Top, const FLinearColor& Sleeve, const FLinearColor& SkinColor, const FLinearColor& Cushion, double Size, TArray<FShape>& Out)
	{
		Add(Out, MarketModel::Cube, Cushion, FVector(16, 0, 4), FVector(30, 84, 9), 0.0, false);
		// The body, leaning out through the open window.
		Add(Out, MarketModel::Cube, Top, FVector(4, 0, 34 * Size), FVector(26, 40 * Size, 58 * Size), 0.0, false, -14.0);
		Add(Out, MarketModel::Sphere, Top, FVector(10, 0, 62 * Size), FVector(26, 46 * Size, 18), 0.0, false);
		// The arms folded on the cushion.
		Rod(Out, Sleeve, FVector(22, -24 * Size, 14), FVector(28, 16 * Size, 13), 10.0 * Size);
		Rod(Out, Sleeve, FVector(18, 24 * Size, 16), FVector(30, -14 * Size, 15), 10.0 * Size);
		Add(Out, MarketModel::Sphere, SkinColor, FVector(30, 19 * Size, 14), FVector(9, 9, 8), 0.0, false);
		Add(Out, MarketModel::Sphere, SkinColor, FVector(31, -17 * Size, 16), FVector(9, 9, 8), 0.0, false);
		// Neck and head.
		Add(Out, MarketModel::Cylinder, SkinColor, FVector(14, 0, 72 * Size), FVector(10, 10, 12), 0.0, false);
		Add(Out, MarketModel::Sphere, SkinColor, FVector(16, 0, 86 * Size), FVector(21, 18, 24) * Size, 0.0, false);
	}

	/** One of the window's people or things (Kind an EFolk, Seed for the colours). */
	inline void WindowFolk(int32 Kind, uint32 Seed, TArray<FShape>& Out)
	{
		using namespace Colors;
		const FLinearColor Cushion = Seed % 2 ? CushionRed : CushionGreen;
		switch (static_cast<EFolk>(Kind))
		{
		case EFolk::OldWoman:
			Leaner(Black, Black, SkinOld, Cushion, 0.95, Out);
			// The black kerchief over her head, knotted under the chin.
			Add(Out, MarketModel::Sphere, Black, FVector(13, 0, 87), FVector(25, 22, 27), 0.0, false);
			Add(Out, MarketModel::Cone, Black, FVector(9, 0, 74), FVector(14, 18, 12), 0.0, false, 180.0);
			Add(Out, MarketModel::Sphere, SkinOld, FVector(21, 0, 83), FVector(10, 14, 16), 0.0, false);
			break;
		case EFolk::Man:
			Leaner(Waistcoat, Shirt, Skin, Cushion, 1.05, Out);
			// White shirt collar, moustache, and the hair or a cap.
			Add(Out, MarketModel::Cube, Shirt, FVector(14, 0, 74), FVector(14, 18, 6), 0.0, false);
			Add(Out, MarketModel::Cube, Hair, FVector(27, 0, 86), FVector(2, 10, 2.5), 0.0, false);
			if (Seed % 3 == 0)
			{
				Add(Out, MarketModel::Cylinder, Waistcoat, FVector(15, 0, 99), FVector(23, 22, 7), 0.0, false);
				Add(Out, MarketModel::Cube, Waistcoat, FVector(26, 0, 96), FVector(10, 18, 2), 0.0, false);
			}
			else
			{
				Add(Out, MarketModel::Sphere, Seed % 3 == 1 ? HairGrey : Hair, FVector(13, 0, 95), FVector(22, 20, 12), 0.0, false);
			}
			break;
		case EFolk::YoungWoman:
			Leaner(Seed % 2 ? Dress : DressFlower, Seed % 2 ? Dress : DressFlower, Skin, Cushion, 0.92, Out);
			// Bobbed hair.
			Add(Out, MarketModel::Sphere, Hair, FVector(12, 0, 82), FVector(22, 22, 24), 0.0, false);
			break;
		case EFolk::Duvet:
		{
			// A feather duvet folded over the sill to air, its striped ticking showing.
			Add(Out, MarketModel::Cube, Duvet, FVector(14, 0, 2), FVector(30, 120, 22), 0.0, false);
			Add(Out, MarketModel::Cube, Duvet, FVector(28, 0, -30), FVector(14, 116, 70), 0.0, false, 6.0);
			Add(Out, MarketModel::Sphere, Duvet, FVector(26, 0, -64), FVector(22, 118, 18), 0.0, false);
			for (const double Y : { -40.0, 0.0, 40.0 })
			{
				Add(Out, MarketModel::Cube, DuvetTick, FVector(35.5, Y, -30), FVector(1, 4, 66), 0.0, false, 6.0);
			}
			break;
		}
		default:
		{
			// A cat sitting on the sill, looking down at the street.
			const FLinearColor Fur = Seed % 2 ? CatGinger : CatGrey;
			Add(Out, MarketModel::Sphere, Fur, FVector(10, 0, 12), FVector(20, 24, 26), 0.0, false);
			Add(Out, MarketModel::Sphere, Fur, FVector(12, 0, 30), FVector(14, 14, 13), 0.0, false);
			for (const double Side : { -1.0, 1.0 })
			{
				Add(Out, MarketModel::Cone, Fur, FVector(11, Side * 4.5, 38), FVector(5, 5, 6), 0.0, false);
			}
			Rod(Out, Fur, FVector(4, 10, 2), FVector(22, 16, -24), 4.0);
			break;
		}
		}
	}
}
