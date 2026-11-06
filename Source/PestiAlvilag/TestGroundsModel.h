// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CarModel.h"
#include "MarketModel.h"

/**
 * The test grounds, far out west of the city where nothing else is: a
 * walled courtyard with a chalk ring for trying out fights, and south of it
 * a big fenced yard with every kind of car and the horse cart, a brick wall,
 * old cars to ram, stone bollards to weave round, a kerbed island and a
 * painted track. ATestGrounds puts the shapes in the game and spawns the
 * people and the vehicles.
 *
 * Header-only, as basic shapes (MarketModel::FShape), so it can be built
 * and looked at outside the engine. The frame: the middle of the fight ring
 * at 0, X east, Y south, Z up from the ground.
 */
namespace TestGroundsModel
{
	using MarketModel::FShape;

	/** Where the grounds stand in the city's frame: 2.5 km west of Blaha Lujza tér, on empty ground (cm). */
	const FVector Place(-250000.0, 0.0, 10.0);

	// The whole slab, the courtyard and the yard (cm, in the frame).
	constexpr double SlabWest = -5200.0, SlabEast = 5200.0, SlabNorth = -1800.0, SlabSouth = 10200.0;
	constexpr double RingHalf = 1500.0;
	constexpr double RingWall = 320.0;
	constexpr double GateHalf = 450.0;
	constexpr double YardSouth = 10000.0, YardWest = -5000.0, YardEast = 5000.0;

	namespace Colors
	{
		const FLinearColor Ground(0.20f, 0.19f, 0.17f);
		const FLinearColor Cobbles(0.30f, 0.29f, 0.27f);
		const FLinearColor CobblesDark(0.22f, 0.21f, 0.20f);
		const FLinearColor Chalk(0.85f, 0.84f, 0.78f);
		const FLinearColor Plaster(0.55f, 0.47f, 0.34f);
		const FLinearColor PlasterDark(0.40f, 0.34f, 0.25f);
		const FLinearColor Brick(0.42f, 0.18f, 0.11f);
		const FLinearColor BrickDark(0.30f, 0.13f, 0.08f);
		const FLinearColor Wood(0.40f, 0.28f, 0.16f);
		const FLinearColor WoodDark(0.22f, 0.15f, 0.09f);
		const FLinearColor Iron(0.05f, 0.05f, 0.055f);
		const FLinearColor Globe(0.95f, 0.88f, 0.70f);
		const FLinearColor Straw(0.70f, 0.58f, 0.30f);
		const FLinearColor Stone(0.52f, 0.50f, 0.46f);
		const FLinearColor Kerb(0.45f, 0.44f, 0.41f);
		const FLinearColor Pavement(0.36f, 0.34f, 0.31f);
		const FLinearColor Paint(0.80f, 0.78f, 0.70f);
		const FLinearColor Red(0.55f, 0.08f, 0.06f);
		const FLinearColor Board(0.08f, 0.14f, 0.10f);
		const FLinearColor Gilt(0.72f, 0.52f, 0.17f);
		const FLinearColor Barrel(0.30f, 0.20f, 0.11f);
	}

	/** Painted lettering on a board: the actor makes it a text in the game. */
	struct FSign
	{
		FString Text;
		FVector At;
		double Yaw;
		double Height;
		FLinearColor Color;
	};

	/** One vehicle standing in the yard, ready to drive. */
	enum class EVehicle : uint8 { Car, Taxi, DeluxeTaxi, Police, HorseCart, Bicycle, ShopBicycle };
	struct FVehicleSpot
	{
		EVehicle Kind;
		CarModel::EBody Body;
		CarModel::EWheel Wheel;
		FLinearColor Paint;
		FVector At;
		double Yaw;
		/** An old wreck to ram: it stands broadside in the middle of the yard. */
		bool bTarget;
	};

	/** Where you arrive (and the way you face): at the courtyard gate, and at the head of the car row. */
	const FVector RingArrive(0.0, 1100.0, 0.0);
	constexpr double RingArriveYaw = -90.0;
	const FVector YardArrive(-4300.0, 2400.0, 0.0);
	constexpr double YardArriveYaw = 0.0;

	/** Where the fighters come from: round the chalk ring, the far side from the gate first. */
	const FVector FoeSpots[] = {
		FVector(0.0, -500.0, 0.0), FVector(-450.0, -300.0, 0.0), FVector(450.0, -300.0, 0.0), FVector(-550.0, 150.0, 0.0), FVector(550.0, 150.0, 0.0)
	};

	/** The courtyard lamps (their tops: the lights hang just under). */
	const FVector LampTops[] = {
		FVector(-1350.0, -1350.0, 380.0), FVector(1350.0, -1350.0, 380.0), FVector(-1350.0, 1350.0, 380.0), FVector(1350.0, 1350.0, 380.0)
	};

	/** The lamp on the kerbed island in the yard. */
	const FVector IslandLampTop(0.0, 5600.0, 380.0);

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
		Add(Out, MarketModel::Cube, Color, (A + B) * 0.5, (B - A).GetAbs(), 0.0, bCollide);
	}

	/** A gas lamp on an iron post. */
	inline void Lamp(TArray<FShape>& Out, const FVector& Top)
	{
		const FVector Foot(Top.X, Top.Y, 0.0);
		Add(Out, MarketModel::Cylinder, Colors::Iron, Foot + FVector(0, 0, 25), FVector(30, 30, 50));
		Add(Out, MarketModel::Cylinder, Colors::Iron, Foot + FVector(0, 0, Top.Z * 0.5), FVector(12, 12, Top.Z));
		Add(Out, MarketModel::Cube, Colors::Globe, Top + FVector(0, 0, 10), FVector(30, 30, 40), 45.0, false);
		Add(Out, MarketModel::Cone, Colors::Iron, Top + FVector(0, 0, 42), FVector(48, 48, 24), 0.0, false);
	}

	/** A park bench facing Yaw. */
	inline void Bench(TArray<FShape>& Out, const FVector& At, double Yaw)
	{
		const FRotator R(0.0, Yaw, 0.0);
		const auto P = [&](double X, double Y, double Z) { return At + R.RotateVector(FVector(X, Y, Z)); };
		Add(Out, MarketModel::Cube, Colors::Wood, P(0, 0, 45), FVector(45, 180, 6), Yaw);
		Add(Out, MarketModel::Cube, Colors::Wood, P(-24, 0, 75), FVector(5, 180, 30), Yaw, false);
		for (const double Y : { -75.0, 75.0 })
		{
			Add(Out, MarketModel::Cube, Colors::Iron, P(0, Y, 22), FVector(40, 6, 44), Yaw, false);
		}
	}

	/** A wooden barrel standing up. */
	inline void Barrel(TArray<FShape>& Out, const FVector& At)
	{
		Add(Out, MarketModel::Cylinder, Colors::Barrel, At + FVector(0, 0, 45), FVector(60, 60, 90));
		Add(Out, MarketModel::Cylinder, Colors::Iron, At + FVector(0, 0, 20), FVector(62, 62, 4), 0.0, false);
		Add(Out, MarketModel::Cylinder, Colors::Iron, At + FVector(0, 0, 70), FVector(62, 62, 4), 0.0, false);
	}

	/** The walled courtyard with the chalk ring. */
	inline void BuildRing(TArray<FShape>& Out)
	{
		const double H = RingHalf, T = 30.0;
		// Cobbles, a hair over the yard ground, and the chalk ring on them.
		Span(Out, Colors::Cobbles, FVector(-H, -H, 0), FVector(H, H, 1), false);
		Add(Out, MarketModel::Cylinder, Colors::Chalk, FVector(0, 0, 1.5), FVector(1100, 1100, 1), 0.0, false);
		Add(Out, MarketModel::Cylinder, Colors::CobblesDark, FVector(0, 0, 2.0), FVector(1060, 1060, 1), 0.0, false);
		// Three walls of a tenement's back yard: plaster over a dark plinth.
		Span(Out, Colors::Plaster, FVector(-H - T, -H - T, 0), FVector(H + T, -H, RingWall));
		Span(Out, Colors::Plaster, FVector(-H - T, -H, 0), FVector(-H, H, RingWall));
		Span(Out, Colors::Plaster, FVector(H, -H, 0), FVector(H + T, H, RingWall));
		Span(Out, Colors::PlasterDark, FVector(-H, -H, 0), FVector(H, -H + 4, 70), false);
		Span(Out, Colors::PlasterDark, FVector(-H, -H, 0), FVector(-H + 4, H, 70), false);
		Span(Out, Colors::PlasterDark, FVector(H - 4, -H, 0), FVector(H, H, 70), false);
		Span(Out, Colors::PlasterDark, FVector(-H - T - 4, -H - T - 4, RingWall - 20), FVector(H + T + 4, -H + 4, RingWall), false);
		// South side: a plank fence each side of an open gateway, with brick gate piers.
		for (const double S : { -1.0, 1.0 })
		{
			const double A = S * GateHalf, B = S * H;
			Span(Out, Colors::Wood, FVector(FMath::Min(A, B), H - 6, 0), FVector(FMath::Max(A, B), H + 6, 180));
			Span(Out, Colors::WoodDark, FVector(FMath::Min(A, B), H - 8, 160), FVector(FMath::Max(A, B), H + 8, 170), false);
			Span(Out, Colors::Brick, FVector(A - 35, H - 35, 0), FVector(A + 35, H + 35, 260));
			Span(Out, Colors::BrickDark, FVector(A - 40, H - 40, 260), FVector(A + 40, H + 40, 275), false);
		}
		// The sign board on the north wall.
		Span(Out, Colors::Board, FVector(-650, -H + 0, 200), FVector(650, -H + 8, 290), false);
		Span(Out, Colors::Gilt, FVector(-660, -H + 0, 196), FVector(660, -H + 10, 200), false);
		Span(Out, Colors::Gilt, FVector(-660, -H + 0, 290), FVector(660, -H + 10, 294), false);
		for (const FVector& Top : LampTops)
		{
			Lamp(Out, Top);
		}
		// Things to fight round and over: benches by the wall, barrels,
		// a stack of crates to vault, a low wall to slide over or slam into.
		Bench(Out, FVector(-700, -1400, 0), -90.0);
		Bench(Out, FVector(700, -1400, 0), -90.0);
		Bench(Out, FVector(-1400, 600, 0), 0.0);
		for (const FVector& At : { FVector(1300, -300, 0), FVector(1320, -200, 0), FVector(1250, -240, 90) })
		{
			Barrel(Out, At);
		}
		Span(Out, Colors::Wood, FVector(1050, 500, 0), FVector(1350, 800, 90));
		Span(Out, Colors::Wood, FVector(1150, 560, 90), FVector(1350, 760, 180));
		Span(Out, Colors::Stone, FVector(-1250, -700, 0), FVector(-1150, -100, 95));
	}

	/** Straw bales round the yard, so a car can't drive off the edge. */
	inline void Bales(TArray<FShape>& Out, const FVector& A, const FVector& B)
	{
		const FVector Way = B - A;
		const double Length = Way.Size();
		const int32 Count = FMath::Max(1, FMath::FloorToInt(Length / 130.0));
		const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(Way.Y, Way.X));
		for (int32 i = 0; i < Count; ++i)
		{
			const FVector At = A + Way * ((i + 0.5) / Count);
			Add(Out, MarketModel::Cube, Colors::Straw, At + FVector(0, 0, 30), FVector(120, 60, 60), Yaw);
			if (i % 2 == 0)
			{
				Add(Out, MarketModel::Cube, Colors::Straw, At + FVector(0, 0, 90), FVector(120, 60, 60), Yaw);
			}
		}
	}

	/** A painted dash of the track (no collision). */
	inline void Dash(TArray<FShape>& Out, const FVector& At, double Yaw, const FLinearColor& Color)
	{
		Add(Out, MarketModel::Cube, Color, At + FVector(0, 0, 1), FVector(150, 15, 1), Yaw, false);
	}

	/** The vehicle yard. */
	inline void BuildYard(TArray<FShape>& Out)
	{
		// The fence of bales all round, behind the courtyard too.
		const double North = SlabNorth + 100.0;
		Bales(Out, FVector(YardWest, North, 0), FVector(YardWest, YardSouth, 0));
		Bales(Out, FVector(YardWest, YardSouth, 0), FVector(YardEast, YardSouth, 0));
		Bales(Out, FVector(YardEast, YardSouth, 0), FVector(YardEast, North, 0));
		Bales(Out, FVector(YardEast, North, 0), FVector(YardWest, North, 0));

		// A brick wall to drive into head-on, with a stone cap.
		Span(Out, Colors::Brick, FVector(-1200, 8200, 0), FVector(1200, 8260, 250));
		Span(Out, Colors::BrickDark, FVector(-1220, 8190, 250), FVector(1220, 8270, 265));
		// Stone bollards to weave between.
		for (int32 i = 0; i < 7; ++i)
		{
			const FVector At(-4000.0, 4200.0 + i * 700.0, 0.0);
			const double Side = (i % 2 == 0) ? -250.0 : 250.0;
			Add(Out, MarketModel::Cylinder, Colors::Stone, At + FVector(Side, 0, 50), FVector(40, 40, 100));
			Add(Out, MarketModel::Sphere, Colors::Stone, At + FVector(Side, 0, 100), FVector(42, 42, 30), 0.0, false);
		}
		// A pavement island with kerbs, as in the streets, and a lamp on it.
		Span(Out, Colors::Kerb, FVector(-600, 5300, 0), FVector(600, 5900, 14));
		Span(Out, Colors::Pavement, FVector(-575, 5325, 14), FVector(575, 5875, 15), false);
		Lamp(Out, IslandLampTop);
		// Barrels stacked to smash through (they hold: it's the car that suffers).
		for (int32 i = 0; i < 5; ++i)
		{
			Barrel(Out, FVector(3000 + (i % 3) * 70.0, 8100 + (i / 3) * 70.0, 0));
		}
		// The track: an oval of painted dashes round the middle, red and white at the start line.
		const FVector Middle(0.0, 6400.0, 0.0);
		const double RX = 3600.0, RY = 3000.0;
		const int32 Dashes = 72;
		for (int32 i = 0; i < Dashes; ++i)
		{
			const double A = 2.0 * PI * i / Dashes;
			const FVector At = Middle + FVector(RX * FMath::Cos(A), RY * FMath::Sin(A), 0.0);
			const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(RY * FMath::Cos(A), -RX * FMath::Sin(A)));
			Dash(Out, At, Yaw, Colors::Paint);
		}
		for (int32 i = 0; i < 8; ++i)
		{
			Add(Out, MarketModel::Cube, (i % 2 == 0) ? Colors::Red : Colors::Paint, FVector(RX - 350 + i * 100.0, Middle.Y, 1), FVector(100, 60, 1), 0.0, false);
		}
	}

	/** Everything: the ground, the courtyard and the yard. */
	inline void Build(TArray<FShape>& Out)
	{
		// The ground, its top at 0.
		Span(Out, Colors::Ground, FVector(SlabWest, SlabNorth, -40), FVector(SlabEast, SlabSouth, 0));
		BuildRing(Out);
		BuildYard(Out);
	}

	inline void Signs(TArray<FSign>& Out)
	{
		Out.Add({ TEXT("PR\u00d3BATEREP \u00b7 VEREKED\u00c9S"), FVector(0, -RingHalf + 10, 255), 90.0, 50.0, Colors::Gilt });
		Out.Add({ TEXT("Test ground \u00b7 fighting"), FVector(0, -RingHalf + 10, 215), 90.0, 24.0, Colors::Chalk });
	}

	/** The cars in a row along the north of the yard, the horse cart and two bicycles at its end, and three old wrecks to ram. */
	inline void Vehicles(TArray<FVehicleSpot>& Out)
	{
		using CarModel::EBody;
		using CarModel::EWheel;
		const double RowY = 2400.0, Gap = 380.0, Face = 90.0;
		const FVehicleSpot Row[] = {
			{ EVehicle::Car, EBody::Saloon, EWheel::Artillery, FLinearColor(0.03f, 0.07f, 0.05f), FVector(-3400, RowY, 0), Face, false },
			{ EVehicle::Car, EBody::Tourer, EWheel::Wire, FLinearColor(0.30f, 0.03f, 0.03f), FVector(-3400 + Gap, RowY, 0), Face, false },
			{ EVehicle::Car, EBody::Coupe, EWheel::Disc, FLinearColor(0.02f, 0.03f, 0.10f), FVector(-3400 + 2 * Gap, RowY, 0), Face, false },
			{ EVehicle::Car, EBody::Limousine, EWheel::Wire, FLinearColor(0.01f, 0.01f, 0.01f), FVector(-3400 + 3 * Gap, RowY, 0), Face, false },
			{ EVehicle::Car, EBody::Van, EWheel::Artillery, FLinearColor(0.20f, 0.17f, 0.10f), FVector(-3400 + 4 * Gap, RowY, 0), Face, false },
			{ EVehicle::Car, EBody::Balilla, EWheel::Disc, FLinearColor(0.35f, 0.30f, 0.20f), FVector(-3400 + 5 * Gap, RowY, 0), Face, false },
			{ EVehicle::Car, EBody::Opel, EWheel::Disc, FLinearColor(0.08f, 0.12f, 0.20f), FVector(-3400 + 6 * Gap, RowY, 0), Face, false },
			{ EVehicle::Taxi, EBody::Saloon, EWheel::Artillery, FLinearColor::Black, FVector(-3400 + 7 * Gap, RowY, 0), Face, false },
			{ EVehicle::DeluxeTaxi, EBody::Limousine, EWheel::Wire, FLinearColor::Black, FVector(-3400 + 8 * Gap, RowY, 0), Face, false },
			{ EVehicle::Police, EBody::Saloon, EWheel::Disc, FLinearColor(0.02f, 0.04f, 0.12f), FVector(-3400 + 9 * Gap, RowY, 0), Face, false },
			{ EVehicle::HorseCart, EBody::Saloon, EWheel::Artillery, FLinearColor::Black, FVector(-3400 + 10 * Gap + 300, RowY + 100, 0), Face, false },
			{ EVehicle::Bicycle, EBody::Saloon, EWheel::Wire, FLinearColor::Black, FVector(-3400 + 10 * Gap + 900, RowY, 0), Face, false },
			{ EVehicle::ShopBicycle, EBody::Saloon, EWheel::Wire, FLinearColor::Black, FVector(-3400 + 10 * Gap + 1150, RowY, 0), Face, false },
			{ EVehicle::Car, EBody::Saloon, EWheel::Artillery, FLinearColor(0.25f, 0.22f, 0.18f), FVector(1800, 7000, 0), 0.0, true },
			{ EVehicle::Car, EBody::Van, EWheel::Artillery, FLinearColor(0.18f, 0.20f, 0.15f), FVector(2500, 7000, 0), 0.0, true },
			{ EVehicle::Car, EBody::Tourer, EWheel::Artillery, FLinearColor(0.22f, 0.12f, 0.08f), FVector(3200, 7000, 0), 0.0, true },
		};
		for (const FVehicleSpot& Spot : Row)
		{
			Out.Add(Spot);
		}
	}
}
