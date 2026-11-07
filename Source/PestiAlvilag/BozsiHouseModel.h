// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MarketModel.h"

/**
 * Bozsi neni's tenement, Nepszinhaz utca 29: a three storey Pest house
 * round a courtyard, built in place of the plain map block so that the
 * player can go in (mission 8, the night raid). The street front with its
 * windows and cornice (the shops and Bozsi's door on the ground floor are
 * the street furniture's), an open gateway, the cobbled courtyard with open
 * iron balconies (the "gang") on both upper floors along three sides, an
 * open stair up to the first floor, the low wash-house across the back of
 * the courtyard, and behind it the back yard with the woodshed and a small
 * door in the back wall that leads through the next house to Tisza Kalman ter.
 *
 * The way out in the raid: along the first floor balcony to its far end,
 * down onto the wash-house roof, over it, down the stacked crates into the
 * back yard, and out through the small door.
 *
 * Header-only, as basic shapes (MarketModel::FShape). The house's frame:
 * the street front runs along Y at X = 0 (the middle of the gateway at
 * Y = 0), +X goes into the lot, Z up from the pavement.
 */
namespace BozsiHouseModel
{
	using MarketModel::FShape;
	using MarketModel::FRandom;

	/** Painted lettering: the actor makes it a text in the game. */
	struct FSign
	{
		FString Text;
		FVector At;
		double Yaw;
		double Height;
		FLinearColor Color;
	};

	/** A lamp the actor makes a small light (the gateway, the balcony, the back door). */
	struct FLamp
	{
		FVector At;
		float Radius;
	};

	namespace Colors
	{
		const FLinearColor Plaster(0.58f, 0.50f, 0.38f);
		const FLinearColor PlasterCourt(0.62f, 0.57f, 0.47f);
		const FLinearColor PlasterDark(0.45f, 0.39f, 0.30f);
		const FLinearColor Stone(0.52f, 0.49f, 0.43f);
		const FLinearColor StoneDark(0.40f, 0.37f, 0.33f);
		const FLinearColor Setts(0.30f, 0.29f, 0.27f);
		const FLinearColor Earth(0.27f, 0.23f, 0.18f);
		const FLinearColor Glass(0.07f, 0.09f, 0.11f);
		const FLinearColor Frame(0.80f, 0.77f, 0.68f);
		const FLinearColor Door(0.24f, 0.15f, 0.08f);
		const FLinearColor DoorGreen(0.12f, 0.20f, 0.14f);
		const FLinearColor GateWood(0.16f, 0.09f, 0.045f);
		const FLinearColor Iron(0.05f, 0.05f, 0.055f);
		const FLinearColor Tiles(0.36f, 0.16f, 0.10f);
		const FLinearColor Tar(0.10f, 0.09f, 0.08f);
		const FLinearColor Brick(0.40f, 0.18f, 0.11f);
		const FLinearColor Wood(0.30f, 0.20f, 0.11f);
		const FLinearColor WoodGrey(0.36f, 0.32f, 0.27f);
		const FLinearColor Pot(0.48f, 0.20f, 0.10f);
		const FLinearColor Leaf(0.10f, 0.25f, 0.08f);
		const FLinearColor Flower(0.70f, 0.06f, 0.05f);
		const FLinearColor Zinc(0.42f, 0.43f, 0.44f);
		const FLinearColor Enamel(0.05f, 0.14f, 0.42f);
		const FLinearColor White(0.85f, 0.84f, 0.80f);
		const FLinearColor Globe(0.95f, 0.88f, 0.70f);
		const FLinearColor Washing[] = {
			FLinearColor(0.85f, 0.84f, 0.80f), FLinearColor(0.62f, 0.66f, 0.72f),
			FLinearColor(0.70f, 0.60f, 0.48f), FLinearColor(0.55f, 0.20f, 0.16f) };
	}

	/** Where the house stands in the city: the middle of the gateway on the street front, and the way into the lot (degrees). */
	const FVector2D CityAt(58771.5, 21395.5);
	constexpr double CityYaw = -71.4;
	/** Where the small back door comes out: on the pavement across Tisza Kalman ter's south side, facing the square. */
	const FVector2D ExitAt(58800.0, 15950.0);
	constexpr double ExitYaw = -71.4;
	/** The player's attic room after the raid, Berkocsis utca: its street door (on the wall, facing the street). */
	const FVector2D AtticDoorAt(18173.0, 30456.0);
	constexpr double AtticDoorYaw = -103.0;
	/** The map block this house takes the place of (OsmGraybox leaves it out). */
	const TCHAR* const ReplacesBuilding = TEXT("r3333877");

	// Sizes (cm).
	constexpr double FrontY0 = -1180.0, FrontY1 = 1180.0;  // the street front
	constexpr double LeftOut = -1270.0;    // the left wing's outer wall (against the next house)
	constexpr double RightOut = 1140.0;    // the right wing's outer wall
	constexpr double FrontDepth = 650.0;   // the street wing, to the courtyard
	constexpr double CourtX1 = 2000.0;     // the back of the courtyard (the wash-house)
	constexpr double CourtY = 600.0;       // the courtyard is Y -600..600
	constexpr double WashX1 = 2300.0;      // the wash-house's back wall
	constexpr double WashY1 = 950.0;
	constexpr double WashTop = 320.0;
	constexpr double BackX = 2700.0;       // the back wall (the next house's fire wall)
	constexpr double BackYardY1 = 450.0;
	constexpr double GateHalf = 150.0;     // the gateway, Y -150..150
	constexpr double GateTop = 400.0;
	constexpr double Floor1 = 450.0;       // the first floor balcony's walking level
	constexpr double Floor2 = 850.0;
	constexpr double Eaves = 1250.0;
	constexpr double RoofRise = 260.0;
	constexpr double Gang = 130.0;         // how far a balcony stands out from the wall
	constexpr double RailHeight = 100.0;
	// The open stair: up from the courtyard along the right balcony to the front one.
	constexpr double StairY0 = 340.0, StairY1 = 470.0;
	constexpr double StairX0 = 1420.0;     // its foot
	constexpr double StairX1 = FrontDepth + Gang;  // its head, at the front balcony's edge

	/** Where things are, in the house's frame. */
	namespace Spots
	{
		/** Bozsi neni's own door on the first floor balcony (on the wall), facing into the courtyard. */
		const FVector BozsiDoor(1150.0, -CourtY, Floor1);
		/** Where the player stands outside it on the balcony, and faces (along the balcony, to the back). */
		const FVector OnBalcony(1150.0, -CourtY + 70.0, Floor1);
		constexpr double OnBalconyYaw = 0.0;
		/** The middle of the gateway, inside the street door. */
		const FVector Gateway(320.0, 0.0, 0.0);
		/** The stair's foot and head. */
		const FVector StairFoot(StairX0 + 60.0, (StairY0 + StairY1) * 0.5, 0.0);
		const FVector StairHead(FrontDepth + Gang * 0.5, (StairY0 + StairY1) * 0.5, Floor1);
		/** The front balcony's corner by the left wing, and the left balcony's far end. */
		const FVector GangCorner(FrontDepth + Gang * 0.5, -CourtY + Gang * 0.5, Floor1);
		const FVector GangEnd(CourtX1 - 40.0, -CourtY + Gang * 0.5, Floor1);
		/** On the wash-house roof, near its back edge, above the crates. */
		const FVector WashRoof(WashX1 - 60.0, -330.0, WashTop);
		/** The crates' top, and the back yard below them. */
		const FVector Crates(WashX1 + 55.0, -330.0, 160.0);
		const FVector BackYard(WashX1 + 200.0, -500.0, 0.0);
		/** The small door in the back wall, on the yard side. */
		const FVector BackDoor(BackX - 60.0, -880.0, 0.0);
		constexpr double BackDoorHalf = 55.0;
		/** The middle of the courtyard. */
		const FVector Court(1300.0, 0.0, 0.0);
	}

	inline void Add(TArray<FShape>& Out, int32 Shape, const FLinearColor& Color, const FVector& At, const FVector& Size,
		double Yaw = 0.0, bool bCollide = true, double Pitch = 0.0, double Roll = 0.0)
	{
		FShape S;
		S.Shape = Shape;
		S.Color = Color;
		S.At = At;
		S.Size = Size;
		S.Pitch = Pitch;
		S.Yaw = Yaw;
		S.Roll = Roll;
		S.bCollide = bCollide;
		Out.Add(S);
	}

	/** A box from one corner to the other (axis-aligned). */
	inline void Span(TArray<FShape>& Out, const FLinearColor& Color, const FVector& A, const FVector& B, bool bCollide = true)
	{
		const FVector Lo(FMath::Min(A.X, B.X), FMath::Min(A.Y, B.Y), FMath::Min(A.Z, B.Z));
		const FVector Hi(FMath::Max(A.X, B.X), FMath::Max(A.Y, B.Y), FMath::Max(A.Z, B.Z));
		Add(Out, MarketModel::Cube, Color, (Lo + Hi) * 0.5, Hi - Lo, 0.0, bCollide);
	}

	/**
	 * A window on a wall whose face is at Face along the axis bAlongX says
	 * (X when the wall runs along Y), facing Out (+1 or -1): dark glass in a
	 * pale frame with a cross bar, a sill and a small cornice over it.
	 */
	inline void Window(TArray<FShape>& Out, bool bWallAlongY, double Face, double OutSign, double Mid, double Z0, double W, double H)
	{
		const auto Box = [&](const FLinearColor& C, double D0, double D1, double A0, double A1, double B0, double B1)
		{
			const double P0 = Face + OutSign * D0, P1 = Face + OutSign * D1;
			if (bWallAlongY)
			{
				Span(Out, C, FVector(P0, Mid + A0, B0), FVector(P1, Mid + A1, B1), false);
			}
			else
			{
				Span(Out, C, FVector(Mid + A0, P0, B0), FVector(Mid + A1, P1, B1), false);
			}
		};
		const double Hw = W * 0.5;
		Box(Colors::Glass, -2.0, 1.0, -Hw, Hw, Z0, Z0 + H);
		Box(Colors::Frame, 0.0, 4.0, -Hw - 8.0, -Hw, Z0, Z0 + H);
		Box(Colors::Frame, 0.0, 4.0, Hw, Hw + 8.0, Z0, Z0 + H);
		Box(Colors::Frame, 0.0, 4.0, -Hw - 8.0, Hw + 8.0, Z0 + H, Z0 + H + 8.0);
		Box(Colors::Frame, 0.0, 3.0, -2.5, 2.5, Z0, Z0 + H);
		Box(Colors::Frame, 0.0, 3.0, -Hw, Hw, Z0 + H * 0.66 - 2.5, Z0 + H * 0.66 + 2.5);
		Box(Colors::StoneDark, 0.0, 10.0, -Hw - 14.0, Hw + 14.0, Z0 - 8.0, Z0);
		Box(Colors::PlasterDark, 0.0, 9.0, -Hw - 14.0, Hw + 14.0, Z0 + H + 16.0, Z0 + H + 26.0);
	}

	/** A flat's door on a balcony wall: planked, with a glazed top and a step of stone. */
	inline void FlatDoor(TArray<FShape>& Out, bool bWallAlongY, double Face, double OutSign, double Mid, double Z0, const FLinearColor& Paint)
	{
		const auto Box = [&](const FLinearColor& C, double D0, double D1, double A0, double A1, double B0, double B1)
		{
			const double P0 = Face + OutSign * D0, P1 = Face + OutSign * D1;
			if (bWallAlongY)
			{
				Span(Out, C, FVector(P0, Mid + A0, B0), FVector(P1, Mid + A1, B1), false);
			}
			else
			{
				Span(Out, C, FVector(Mid + A0, P0, B0), FVector(Mid + A1, P1, B1), false);
			}
		};
		Box(Paint, -1.0, 3.0, -50.0, 50.0, Z0, Z0 + 215.0);
		Box(Colors::Glass, 0.0, 4.0, -38.0, 38.0, Z0 + 130.0, Z0 + 200.0);
		Box(Colors::Frame, 0.0, 5.0, -2.0, 2.0, Z0 + 130.0, Z0 + 200.0);
		Box(Colors::Frame, 0.0, 5.0, -56.0, 56.0, Z0 + 215.0, Z0 + 223.0);
		Box(Colors::Zinc, 3.0, 7.0, 34.0, 40.0, Z0 + 100.0, Z0 + 106.0);
	}

	/** The street front: the windows of both upper floors, the bands, the cornice and the gateway's stone surround. */
	inline void BuildStreetFront(TArray<FShape>& Out)
	{
		// The front wing's walls, either side of the gateway and over it.
		Span(Out, Colors::Plaster, FVector(0, FrontY0, 0), FVector(FrontDepth, -GateHalf, Eaves));
		Span(Out, Colors::Plaster, FVector(0, GateHalf, 0), FVector(FrontDepth, FrontY1, Eaves));
		Span(Out, Colors::Plaster, FVector(0, -GateHalf, GateTop), FVector(FrontDepth, GateHalf, Eaves));
		// Bands between the floors, and the cornice under the roof.
		for (const double Z : { Floor1 - 10.0, Floor2 - 10.0 })
		{
			Span(Out, Colors::PlasterDark, FVector(-8, FrontY0, Z), FVector(0, FrontY1, Z + 16), false);
		}
		Span(Out, Colors::PlasterDark, FVector(-22, FrontY0, Eaves - 40), FVector(0, FrontY1, Eaves - 20), false);
		Span(Out, Colors::Stone, FVector(-36, FrontY0, Eaves - 20), FVector(0, FrontY1, Eaves), false);
		// Windows: eight across on each upper floor, a pair over the gateway.
		for (const double Z : { Floor1 + 90.0, Floor2 + 90.0 })
		{
			for (int32 i = 0; i < 8; ++i)
			{
				const double Y = FrontY0 + 160.0 + i * ((FrontY1 - FrontY0 - 320.0) / 7.0);
				Window(Out, true, 0.0, -1.0, Y, Z, 110.0, 185.0);
			}
		}
		// The gateway's stone surround, with a keystone, and the house number plate.
		Span(Out, Colors::Stone, FVector(-14, -GateHalf - 40, 0), FVector(0, -GateHalf, GateTop + 40), false);
		Span(Out, Colors::Stone, FVector(-14, GateHalf, 0), FVector(0, GateHalf + 40, GateTop + 40), false);
		Span(Out, Colors::Stone, FVector(-14, -GateHalf, GateTop), FVector(0, GateHalf, GateTop + 40), false);
		Span(Out, Colors::StoneDark, FVector(-20, -18, GateTop - 6), FVector(0, 18, GateTop + 46), false);
		Span(Out, Colors::Enamel, FVector(-17, GateHalf + 6, 250), FVector(-14, GateHalf + 34, 272), false);
		// Wheel guards.
		for (const double Y : { -GateHalf + 10.0, GateHalf - 10.0 })
		{
			Add(Out, MarketModel::Cone, Colors::StoneDark, FVector(-8, Y, 25), FVector(30, 30, 50), 0.0, false);
		}
	}

	/** Through the gateway: cobbles, the plank leaves swung open against the walls, a lamp. */
	inline void BuildGateway(TArray<FShape>& Out, TArray<FLamp>& Lamps)
	{
		Span(Out, Colors::Setts, FVector(-30, -GateHalf, 0), FVector(FrontDepth, GateHalf, 3), false);
		for (const double Side : { -1.0, 1.0 })
		{
			const double Y = Side * (GateHalf - 5.0);
			Span(Out, Colors::GateWood, FVector(40, Y - 4, 0), FVector(40 + GateHalf, Y + 4, GateTop - 20), false);
			for (const double Z : { 60.0, 200.0, 320.0 })
			{
				Span(Out, Colors::Wood, FVector(50, Y - Side * 9, Z), FVector(30 + GateHalf, Y - Side * 4, Z + 14), false);
			}
		}
		// A lamp hanging from the vault in the middle.
		Add(Out, MarketModel::Cylinder, Colors::Iron, FVector(320, 0, GateTop - 30), FVector(2, 2, 60), 0.0, false);
		Add(Out, MarketModel::Sphere, Colors::Globe, FVector(320, 0, GateTop - 70), FVector(28, 28, 32), 0.0, false);
		Lamps.Add({ FVector(320, 0, GateTop - 80), 900.f });
	}

	/** One balcony at Level along a wall, from A to B (the slab's outer edge), with brackets under and an iron railing; gaps in Skip are left open. */
	inline void Balcony(TArray<FShape>& Out, const FVector2D& Wall0, const FVector2D& Wall1, const FVector2D& OutDir, double Level,
		const TArray<FVector2D>& Gaps)
	{
		const FVector2D Along = (Wall1 - Wall0).GetSafeNormal();
		const double Length = FVector2D::Distance(Wall0, Wall1);
		const FVector2D Mid = (Wall0 + Wall1) * 0.5 + OutDir * (Gang * 0.5);
		const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(Along.Y, Along.X));
		// The stone slab.
		Add(Out, MarketModel::Cube, Colors::Stone, FVector(Mid, Level - 7.0), FVector(Length, Gang, 14.0), Yaw);
		// Iron brackets under it.
		for (double S = 60.0; S < Length; S += 220.0)
		{
			const FVector2D P = Wall0 + Along * S + OutDir * (Gang * 0.45);
			Add(Out, MarketModel::Cube, Colors::Iron, FVector(P, Level - 40.0), FVector(6.0, Gang * 0.9, 6.0), Yaw, false);
			const FVector2D Q = Wall0 + Along * S + OutDir * 18.0;
			Add(Out, MarketModel::Cube, Colors::Iron, FVector(Q, Level - 38.0), FVector(6.0, 6.0, 48.0), Yaw, false);
		}
		// The railing on the outer edge: the top rail, a bottom rail, posts close
		// enough together that nobody slips between them, and thin bars.
		const FVector2D Edge0 = Wall0 + OutDir * (Gang - 4.0);
		const auto InGap = [&](double S)
		{
			for (const FVector2D& G : Gaps)
			{
				if (S > G.X && S < G.Y)
				{
					return true;
				}
			}
			return false;
		};
		double RunStart = 0.0;
		const auto Rails = [&](double S0, double S1)
		{
			if (S1 - S0 < 1.0)
			{
				return;
			}
			const FVector2D P = Edge0 + Along * ((S0 + S1) * 0.5);
			Add(Out, MarketModel::Cube, Colors::Iron, FVector(P, Level + RailHeight - 3.0), FVector(S1 - S0, 7.0, 6.0), Yaw);
			Add(Out, MarketModel::Cube, Colors::Iron, FVector(P, Level + 12.0), FVector(S1 - S0, 5.0, 4.0), Yaw, false);
		};
		for (double S = 0.0; S <= Length; S += 12.0)
		{
			if (InGap(S))
			{
				Rails(RunStart, S);
				while (S <= Length && InGap(S))
				{
					S += 12.0;
				}
				RunStart = S;
				continue;
			}
			const FVector2D P = Edge0 + Along * S;
			const bool bPost = FMath::Fmod(S, 48.0) < 1.0;
			Add(Out, MarketModel::Cube, Colors::Iron, FVector(P, Level + RailHeight * 0.5), bPost ? FVector(5.0, 5.0, RailHeight) : FVector(2.0, 2.0, RailHeight), Yaw, bPost);
		}
		Rails(RunStart, Length);
	}

	/** The courtyard: its walls with doors and windows, the balconies, the stair, the cobbles and what stands about. */
	inline void BuildCourtyard(TArray<FShape>& Out, TArray<FLamp>& Lamps, FRandom& R)
	{
		// The two side wings (solid; their faces are the courtyard walls).
		Span(Out, Colors::PlasterCourt, FVector(FrontDepth, LeftOut, 0), FVector(CourtX1, -CourtY, Eaves));
		Span(Out, Colors::PlasterCourt, FVector(FrontDepth, CourtY, 0), FVector(CourtX1, RightOut, Eaves));
		// The front wing's courtyard face is the same block as the street front (a lighter skin over it).
		Span(Out, Colors::PlasterCourt, FVector(FrontDepth, -CourtY, 0), FVector(FrontDepth + 1, -GateHalf, Eaves), false);
		Span(Out, Colors::PlasterCourt, FVector(FrontDepth, GateHalf, 0), FVector(FrontDepth + 1, CourtY, Eaves), false);
		Span(Out, Colors::PlasterCourt, FVector(FrontDepth, -GateHalf, GateTop), FVector(FrontDepth + 1, GateHalf, Eaves), false);
		// A plinth of darker plaster round the foot of the walls.
		Span(Out, Colors::PlasterDark, FVector(FrontDepth + 1, -CourtY, 0), FVector(FrontDepth + 3, -GateHalf, 60), false);
		Span(Out, Colors::PlasterDark, FVector(FrontDepth + 1, GateHalf, 0), FVector(FrontDepth + 3, CourtY, 60), false);
		Span(Out, Colors::PlasterDark, FVector(FrontDepth, -CourtY, 0), FVector(CourtX1, -CourtY + 2, 60), false);
		Span(Out, Colors::PlasterDark, FVector(FrontDepth, CourtY - 2, 0), FVector(CourtX1, CourtY, 60), false);
		// The cobbles, a drain in the middle.
		Span(Out, Colors::Setts, FVector(FrontDepth, -CourtY, 0), FVector(CourtX1, CourtY, 3), false);
		Add(Out, MarketModel::Cylinder, Colors::Iron, FVector(Spots::Court.X, 0, 3.2), FVector(40, 40, 1), 0.0, false);

		// Doors and windows of the flats along the side wings, on all three floors.
		const double Floors[] = { 0.0, Floor1, Floor2 };
		const FLinearColor DoorPaints[] = { Colors::Door, Colors::DoorGreen, Colors::Door, FLinearColor(0.20f, 0.24f, 0.30f) };
		for (const double Z : Floors)
		{
			for (int32 k = 0; k < 4; ++k)
			{
				const double X = FrontDepth + 250.0 + k * 340.0;
				for (const double Side : { -1.0, 1.0 })
				{
					const double Face = Side * CourtY;
					// Bozsi's door is the second along the left wing's first floor.
					const bool bBozsi = Side < 0.0 && Z == Floor1 && k == 1;
					const double DoorX = bBozsi ? Spots::BozsiDoor.X : X;
					FlatDoor(Out, false, Face, -Side, DoorX, Z, bBozsi ? Colors::DoorGreen : DoorPaints[(k + (Side > 0 ? 1 : 0)) % 4]);
					Window(Out, false, Face, -Side, DoorX + 120.0, Z + 95.0, 80.0, 120.0);
				}
			}
			// The front wing's courtyard side: windows either side of the gateway.
			if (Z > 0.0)
			{
				for (const double Y : { -430.0, -260.0, 260.0, 430.0 })
				{
					Window(Out, true, FrontDepth, 1.0, Y, Z + 95.0, 80.0, 120.0);
				}
			}
		}
		// A doormat and a red geranium by Bozsi's door.
		Span(Out, Colors::Wood, FVector(Spots::BozsiDoor.X - 40, -CourtY + 4, Floor1), FVector(Spots::BozsiDoor.X + 40, -CourtY + 50, Floor1 + 2), false);

		// The balconies on both upper floors: the front wing's (gap at the stair's head
		// on the first floor), the left wing's (open at its far end onto the wash-house
		// roof) and the right wing's.
		for (const double Level : { Floor1, Floor2 })
		{
			const bool bFirst = Level == Floor1;
			TArray<FVector2D> FrontGaps;
			if (bFirst)
			{
				FrontGaps.Add(FVector2D(CourtY + StairY0 - 6.0, CourtY + StairY1 + 6.0));
			}
			Balcony(Out, FVector2D(FrontDepth, -CourtY), FVector2D(FrontDepth, CourtY), FVector2D(1, 0), Level, FrontGaps);
			Balcony(Out, FVector2D(FrontDepth + Gang, -CourtY), FVector2D(CourtX1, -CourtY), FVector2D(0, 1), Level, TArray<FVector2D>());
			Balcony(Out, FVector2D(FrontDepth + Gang, CourtY), FVector2D(CourtX1, CourtY), FVector2D(0, -1), Level, TArray<FVector2D>());
		}
		// The first floor's left balcony runs on, without a railing, to the wash-house roof.
		Span(Out, Colors::Stone, FVector(CourtX1 - 4, -CourtY, Floor1 - 14), FVector(CourtX1 + 30, -CourtY + Gang, Floor1));

		// The open stair: a stone flight from the courtyard up beside the right
		// balcony to the front one, with treads, an iron railing on its open side.
		{
			const double Run = StairX0 - StairX1;
			const double Len = FMath::Sqrt(Run * Run + Floor1 * Floor1);
			const double Pitch = FMath::RadiansToDegrees(FMath::Atan2(Floor1, Run));
			const double Mid = (StairY0 + StairY1) * 0.5;
			const FVector Centre((StairX0 + StairX1) * 0.5, Mid, Floor1 * 0.5 - 8.0);
			Add(Out, MarketModel::Cube, Colors::Stone, Centre, FVector(Len, StairY1 - StairY0, 16.0), 0.0, true, -Pitch);
			const int32 Treads = 24;
			for (int32 i = 1; i < Treads; ++i)
			{
				const double T = static_cast<double>(i) / Treads;
				const FVector P(StairX0 - Run * T, Mid, Floor1 * T + 1.0);
				Add(Out, MarketModel::Cube, Colors::StoneDark, P, FVector(4.0, StairY1 - StairY0, 3.0), 0.0, false);
			}
			// The railing along the open side (Y = StairY0).
			const FVector Rail((StairX0 + StairX1) * 0.5, StairY0 + 3.0, Floor1 * 0.5 + RailHeight - 8.0);
			Add(Out, MarketModel::Cube, Colors::Iron, Rail, FVector(Len, 6.0, 6.0), 0.0, false, -Pitch);
			for (int32 i = 0; i <= 12; ++i)
			{
				const double T = static_cast<double>(i) / 12.0;
				const FVector P(StairX0 - Run * T, StairY0 + 3.0, Floor1 * T + RailHeight * 0.5);
				Add(Out, MarketModel::Cube, Colors::Iron, P, FVector(4.0, 4.0, RailHeight), 0.0, i % 3 == 0);
			}
		}

		// Laundry lines across the courtyard between the first floor balconies, with washing.
		for (const double X : { 1050.0, 1550.0 })
		{
			const double Z = Floor1 + 190.0;
			Add(Out, MarketModel::Cube, Colors::WoodGrey, FVector(X, 0, Z), FVector(1.0, 2 * (CourtY - Gang), 1.0), 0.0, false);
			for (double Y = -CourtY + Gang + 40.0; Y < CourtY - Gang - 40.0; Y += R.Range(55.0, 90.0))
			{
				if (R.Unit() < 0.3)
				{
					continue;
				}
				const double W = R.Range(30.0, 60.0), H = R.Range(40.0, 80.0);
				Add(Out, MarketModel::Cube, Colors::Washing[R.Index(4)], FVector(X, Y, Z - H * 0.5), FVector(1.5, W, H), 0.0, false);
			}
		}
		// Geraniums in pots on the balcony rails.
		for (int32 i = 0; i < 10; ++i)
		{
			const bool bLeft = i % 2 == 0;
			const double X = FrontDepth + 200.0 + R.Range(0.0, 1100.0);
			const double Y = bLeft ? -CourtY + Gang + 8.0 : CourtY - Gang - 8.0;
			const double Z = (i % 3 == 0 ? Floor2 : Floor1) + RailHeight + 8.0;
			Add(Out, MarketModel::Cube, Colors::Pot, FVector(X, Y, Z), FVector(40, 14, 16), 0.0, false);
			Add(Out, MarketModel::Sphere, Colors::Leaf, FVector(X, Y, Z + 16), FVector(42, 18, 22), 0.0, false);
			Add(Out, MarketModel::Sphere, Colors::Flower, FVector(X + 8, Y, Z + 26), FVector(14, 12, 10), 0.0, false);
		}
		// Bozsi's geranium on the floor by her door.
		Add(Out, MarketModel::Cylinder, Colors::Pot, FVector(Spots::BozsiDoor.X - 85, -CourtY + 25, Floor1 + 15), FVector(30, 30, 30), 0.0, false);
		Add(Out, MarketModel::Sphere, Colors::Leaf, FVector(Spots::BozsiDoor.X - 85, -CourtY + 25, Floor1 + 45), FVector(44, 44, 34), 0.0, false);
		Add(Out, MarketModel::Sphere, Colors::Flower, FVector(Spots::BozsiDoor.X - 80, -CourtY + 30, Floor1 + 62), FVector(16, 16, 12), 0.0, false);
		// A lamp on the wall by her door.
		Add(Out, MarketModel::Sphere, Colors::Globe, FVector(Spots::BozsiDoor.X + 70, -CourtY + 12, Floor1 + 240), FVector(18, 18, 22), 0.0, false);
		Lamps.Add({ FVector(Spots::BozsiDoor.X + 70, -CourtY + 30, Floor1 + 230), 800.f });

		// The courtyard well: an iron pump, a stone trough.
		Add(Out, MarketModel::Cylinder, Colors::Iron, FVector(1700, 380, 60), FVector(18, 18, 120));
		Add(Out, MarketModel::Cube, Colors::Iron, FVector(1700, 395, 115), FVector(6, 50, 6), 0.0, false, 0.0, -20.0);
		Add(Out, MarketModel::Cube, Colors::Stone, FVector(1700, 430, 20), FVector(60, 50, 40));
		// The carpet-beating rail, dustbins, a handcart against the wall.
		for (const double Y : { -330.0, -90.0 })
		{
			Add(Out, MarketModel::Cylinder, Colors::Iron, FVector(1750, Y, 90), FVector(6, 6, 180), 0.0, false);
		}
		Add(Out, MarketModel::Cylinder, Colors::Iron, FVector(1750, -210, 178), FVector(5, 5, 240), 0.0, false, 0.0, 90.0);
		for (int32 i = 0; i < 3; ++i)
		{
			Add(Out, MarketModel::Cylinder, Colors::Zinc, FVector(820 + i * 70, -CourtY + 45, 40), FVector(56, 56, 80));
			Add(Out, MarketModel::Cylinder, Colors::Zinc, FVector(820 + i * 70, -CourtY + 45, 82), FVector(60, 60, 5), 0.0, false);
		}
		Add(Out, MarketModel::Cube, Colors::WoodGrey, FVector(1250, CourtY - 50, 45), FVector(150, 70, 12), 0.0, true, 0.0, 0.0);
		Add(Out, MarketModel::Cylinder, Colors::Wood, FVector(1190, CourtY - 50, 38), FVector(76, 76, 6), 0.0, false, 0.0, 90.0);
	}

	/** The wash-house across the back of the courtyard, low and flat-roofed, and the back yard behind it. */
	inline void BuildBack(TArray<FShape>& Out, TArray<FLamp>& Lamps, FRandom& R)
	{
		// The wash-house.
		Span(Out, Colors::PlasterCourt, FVector(CourtX1, -CourtY, 0), FVector(WashX1, WashY1, WashTop - 20));
		Span(Out, Colors::Tar, FVector(CourtX1 - 10, -CourtY, WashTop - 20), FVector(WashX1 + 10, WashY1, WashTop));
		FlatDoor(Out, true, CourtX1, -1.0, 200.0, 0.0, Colors::WoodGrey);
		Window(Out, true, CourtX1, -1.0, -260.0, 110.0, 90.0, 90.0);
		Window(Out, true, CourtX1, -1.0, 420.0, 110.0, 90.0, 90.0);
		// Its chimney, and the copper's flue.
		Span(Out, Colors::Brick, FVector(2180, 300, WashTop), FVector(2240, 360, WashTop + 120));
		// The back yard: beaten earth.
		Span(Out, Colors::Earth, FVector(CourtX1, LeftOut - 30, 0), FVector(BackX, -CourtY, 3), false);
		Span(Out, Colors::Earth, FVector(WashX1, -CourtY, 0), FVector(BackX, BackYardY1, 3), false);
		// The walls round it: the next house's fire wall at the back, a wall on each side.
		Span(Out, Colors::PlasterDark, FVector(BackX, LeftOut - 70, 0), FVector(BackX + 40, BackYardY1 + 100, 1700));
		Span(Out, Colors::PlasterDark, FVector(CourtX1, LeftOut - 70, 0), FVector(BackX, LeftOut - 30, 1500));
		Span(Out, Colors::PlasterDark, FVector(WashX1, BackYardY1, 0), FVector(BackX, BackYardY1 + 100, 1200));
		// Bare brick where the plaster has come off the fire wall.
		Span(Out, Colors::Brick, FVector(BackX - 1.5, -300, 120), FVector(BackX, -60, 260), false);
		Span(Out, Colors::Brick, FVector(BackX - 1.5, 120, 400), FVector(BackX, 260, 470), false);
		// The left wing's back wall faces the back yard: a window or two.
		Window(Out, true, CourtX1, 1.0, -950.0, Floor1 + 90.0, 80.0, 120.0);
		Window(Out, true, CourtX1, 1.0, -950.0, Floor2 + 90.0, 80.0, 120.0);

		// Crates stacked against the wash-house's back wall: the way down from its roof.
		Span(Out, Colors::Wood, FVector(WashX1, -420, 0), FVector(WashX1 + 110, -240, 160));
		Span(Out, Colors::WoodGrey, FVector(WashX1 + 110, -430, 0), FVector(WashX1 + 200, -260, 80));
		for (const double Z : { 40.0, 120.0 })
		{
			Span(Out, Colors::WoodGrey, FVector(WashX1 + 110, -416, Z - 4), FVector(WashX1 + 111, -244, Z + 4), false);
		}
		// The woodshed against the side wall: posts, a sloping roof, split logs stacked.
		Span(Out, Colors::Wood, FVector(BackX - 160, 0, 0), FVector(BackX, BackYardY1, 10), false);
		for (const double Y : { 10.0, BackYardY1 - 10.0 })
		{
			Add(Out, MarketModel::Cube, Colors::Wood, FVector(BackX - 150, Y, 110), FVector(10, 10, 220));
		}
		Add(Out, MarketModel::Cube, Colors::WoodGrey, FVector(BackX - 80, BackYardY1 * 0.5, 240), FVector(180, BackYardY1 + 20, 6), 0.0, false, 12.0);
		for (int32 i = 0; i < 40; ++i)
		{
			const double Y = 40.0 + (i % 10) * 36.0;
			const double Z = 12.0 + (i / 10) * 26.0;
			Add(Out, MarketModel::Cylinder, Colors::Wood, FVector(BackX - 70 + R.Range(-8.0, 8.0), Y, Z), FVector(24, 24, 90), 0.0, false, 90.0, 0.0);
		}
		// The small door through the back wall, with its lamp.
		const FVector Door = Spots::BackDoor;
		Span(Out, Colors::GateWood, FVector(BackX - 3, Door.Y - Spots::BackDoorHalf, 0), FVector(BackX, Door.Y + Spots::BackDoorHalf, 210), false);
		Span(Out, Colors::Iron, FVector(BackX - 5, Door.Y - Spots::BackDoorHalf, 60), FVector(BackX - 3, Door.Y + Spots::BackDoorHalf, 66), false);
		Span(Out, Colors::Iron, FVector(BackX - 5, Door.Y - Spots::BackDoorHalf, 160), FVector(BackX - 3, Door.Y + Spots::BackDoorHalf, 166), false);
		Span(Out, Colors::Stone, FVector(BackX - 8, Door.Y - Spots::BackDoorHalf - 12, 210), FVector(BackX, Door.Y + Spots::BackDoorHalf + 12, 224), false);
		Add(Out, MarketModel::Sphere, Colors::Globe, FVector(BackX - 20, Door.Y + Spots::BackDoorHalf + 40, 260), FVector(18, 18, 22), 0.0, false);
		Lamps.Add({ FVector(BackX - 60, Door.Y, 250), 700.f });
		// Dustbins and a broken chair by the door.
		Add(Out, MarketModel::Cylinder, Colors::Zinc, FVector(BackX - 50, Door.Y + 150, 40), FVector(56, 56, 80));
		Add(Out, MarketModel::Cube, Colors::Wood, FVector(BackX - 120, Door.Y - 170, 22), FVector(40, 40, 6), 25.0, false, 0.0, 70.0);
	}

	/** The roofs: tiled slopes over each wing. */
	inline void BuildRoofs(TArray<FShape>& Out)
	{
		const double Pitch = FMath::RadiansToDegrees(FMath::Atan2(RoofRise, FrontDepth * 0.5));
		const double Slope = FMath::Sqrt(FMath::Square(FrontDepth * 0.5) + FMath::Square(RoofRise));
		// The front wing: one slope to the street, one to the courtyard.
		Add(Out, MarketModel::Cube, Colors::Tiles, FVector(FrontDepth * 0.25, 0, Eaves + RoofRise * 0.5), FVector(Slope + 30, FrontY1 - FrontY0, 12), 0.0, true, Pitch);
		Add(Out, MarketModel::Cube, Colors::Tiles, FVector(FrontDepth * 0.75, 0, Eaves + RoofRise * 0.5), FVector(Slope + 30, FrontY1 - FrontY0, 12), 0.0, true, -Pitch);
		// The side wings: slopes to the courtyard and to the outside.
		for (const double Side : { -1.0, 1.0 })
		{
			const double Inner = Side * CourtY;
			const double Outer = Side < 0.0 ? LeftOut : RightOut;
			const double Depth = FMath::Abs(Outer - Inner);
			const double P = FMath::RadiansToDegrees(FMath::Atan2(RoofRise, Depth * 0.5));
			const double L = FMath::Sqrt(FMath::Square(Depth * 0.5) + FMath::Square(RoofRise));
			const double X0 = FrontDepth, X1 = CourtX1;
			for (const double Part : { 0.25, 0.75 })
			{
				const double Y = Inner + (Outer - Inner) * Part;
				// A slope rising towards the ridge: pitched up along +Y, then turned to run along Y.
				const double Rise = (Part < 0.5) == (Side > 0.0) ? P : -P;
				Add(Out, MarketModel::Cube, Colors::Tiles, FVector((X0 + X1) * 0.5, Y, Eaves + RoofRise * 0.5), FVector(L + 30, X1 - X0, 12), 90.0, true, Rise);
			}
		}
		// Chimneys.
		for (const FVector& C : { FVector(320, -800, 0), FVector(320, 750, 0), FVector(1300, -935, 0), FVector(1500, 870, 0) })
		{
			Span(Out, Colors::Brick, FVector(C.X - 30, C.Y - 30, Eaves + RoofRise - 40), FVector(C.X + 30, C.Y + 30, Eaves + RoofRise + 90));
		}
	}

	inline void Build(TArray<FShape>& Out, TArray<FLamp>& Lamps)
	{
		FRandom R(29);
		BuildStreetFront(Out);
		BuildGateway(Out, Lamps);
		BuildCourtyard(Out, Lamps, R);
		BuildBack(Out, Lamps, R);
		BuildRoofs(Out);
	}

	inline void Signs(TArray<FSign>& Out)
	{
		// The house number on the blue plate by the gateway.
		Out.Add({ TEXT("29"), FVector(-18.0, GateHalf + 20.0, 261.0), 180.0, 16.0, Colors::White });
	}
}
