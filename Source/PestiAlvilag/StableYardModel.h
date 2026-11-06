// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MarketModel.h"
#include "PersonKit.h"

/**
 * The carters' yard on Auróra utca, round the corner from Nagy Fuvaros
 * utca, where Pista bácsi keeps his horse Csillag and his cart: a gateway
 * from the street, a low stable with half doors and a hay loft along one
 * side, a water trough and pump, an open hay shed and the dung heap at the
 * back, and an old wagon of another carter. Jóska, the stable lad, looks
 * after the horses.
 *
 * Header-only, as basic shapes (MarketModel::FShape), so it can be built
 * and looked at outside the engine. The yard's frame: the street front runs
 * along Y at X = 0, +X goes into the yard, Z up from the pavement. It fills
 * the empty lot between two houses, Y -650..650, X 0..3000.
 */
namespace StableYardModel
{
	using MarketModel::FShape;
	using MarketModel::FRandom;

	/** Painted lettering on a board: the actor makes it a text in the game. */
	struct FSign
	{
		FString Text;
		FVector At;
		double Yaw;
		double Height;
		FLinearColor Color;
	};

	namespace Colors
	{
		const FLinearColor Plaster(0.56f, 0.49f, 0.37f);
		const FLinearColor PlasterDark(0.40f, 0.34f, 0.26f);
		const FLinearColor Roof(0.33f, 0.17f, 0.10f);
		const FLinearColor Ground(0.27f, 0.23f, 0.18f);
		const FLinearColor Setts(0.30f, 0.29f, 0.27f);
		const FLinearColor Wood(0.30f, 0.20f, 0.11f);
		const FLinearColor DoorGreen(0.12f, 0.20f, 0.12f);
		const FLinearColor Dark(0.04f, 0.035f, 0.03f);
		const FLinearColor Straw(0.66f, 0.55f, 0.30f);
		const FLinearColor OldStraw(0.50f, 0.42f, 0.24f);
		const FLinearColor Trodden(0.30f, 0.26f, 0.17f);
		const FLinearColor Dung(0.18f, 0.13f, 0.08f);
		const FLinearColor Water(0.06f, 0.09f, 0.10f);
		const FLinearColor Iron(0.05f, 0.05f, 0.055f);
		const FLinearColor Sack(0.55f, 0.48f, 0.34f);
		const FLinearColor Leather(0.16f, 0.09f, 0.05f);
		const FLinearColor Blanket(0.36f, 0.10f, 0.08f);
		const FLinearColor Sign(0.80f, 0.74f, 0.58f);
		const FLinearColor Lettering(0.10f, 0.07f, 0.05f);
		const FLinearColor WagonBlue(0.16f, 0.24f, 0.34f);
	}

	/** Where the yard stands in the city: the middle of its street front on Auróra utca, and the way into it (degrees). */
	const FVector2D CityAt(46577.0, 33461.0);
	constexpr double CityYaw = -18.6;

	/** Where things are, in the yard's frame. */
	namespace Spots
	{
		/** Pista bácsi's cart and horse waiting in the middle of the yard, the horse facing the gate (the cart actor's origin, and its yaw). */
		const FVector Cart(1450.0, -230.0, 0.0);
		constexpr double CartYaw = 180.0;
		/** Jóska, by the horse's head, facing it. */
		const FVector Joska(1020.0, 40.0, 0.0);
		constexpr double JoskaYaw = -120.0;
		/** In the gateway, the way in and out. */
		const FVector Gate(0.0, -80.0, 0.0);
	}

	// Sizes (cm).
	constexpr double FrontY = 650.0;        // the lot reaches the houses either side here
	constexpr double Depth = 3000.0;
	constexpr double GateHalf = 260.0;      // half the gateway's width, about Y = GateMid
	constexpr double GateMid = -80.0;
	constexpr double StableY = 220.0;       // the stable's front wall, its back against the house at FrontY
	constexpr double StableX0 = 650.0;
	constexpr double StableX1 = 2650.0;

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

	/** The street front: a wall either side of the gateway, its two big gate leaves standing open into the yard, the board over it. */
	inline void BuildGate(TArray<FShape>& Out)
	{
		const double T = 35.0, H = 290.0;
		const double G0 = GateMid - GateHalf, G1 = GateMid + GateHalf;
		Span(Out, Colors::Plaster, FVector(0, -FrontY, 0), FVector(T, G0 - 30, H));
		Span(Out, Colors::Plaster, FVector(0, G1 + 30, 0), FVector(T, FrontY, H));
		Span(Out, Colors::PlasterDark, FVector(-4, -FrontY, 0), FVector(0, G0 - 30, 45), false);
		Span(Out, Colors::PlasterDark, FVector(-4, G1 + 30, 0), FVector(0, FrontY, 45), false);
		// Coping on the walls.
		Span(Out, Colors::PlasterDark, FVector(-6, -FrontY, H), FVector(T + 6, G0 - 30, H + 10), false);
		Span(Out, Colors::PlasterDark, FVector(-6, G1 + 30, H), FVector(T + 6, FrontY, H + 10), false);
		// Gate piers, a little higher, with caps.
		for (const double Y : { G0 - 15.0, G1 + 15.0 })
		{
			Add(Out, MarketModel::Cube, Colors::Plaster, FVector(T * 0.5, Y, 170), FVector(T + 20, 50, 340));
			Add(Out, MarketModel::Cube, Colors::PlasterDark, FVector(T * 0.5, Y, 345), FVector(T + 34, 64, 12), 0.0, false);
		}
		// The board across the top, between the piers (the lettering is a sign).
		Span(Out, Colors::Wood, FVector(T * 0.5 - 4, G0, 300), FVector(T * 0.5 + 4, G1, 306), false);
		Span(Out, Colors::Sign, FVector(T * 0.5 - 6, G0 + 40, 306), FVector(T * 0.5 - 2, G1 - 40, 350), false);
		// The two leaves, planked, swung open against the walls inside.
		for (const double Side : { -1.0, 1.0 })
		{
			const double Hinge = Side < 0.0 ? G0 : G1;
			const double Open = 75.0 * Side;   // swung in towards the yard
			const double Leaf = GateHalf;
			const FRotator R(0.0, Open, 0.0);
			const FVector Base(T + 10.0, Hinge, 0.0);
			const auto P = [&](double Along, double Z) { return Base + R.RotateVector(FVector(0.0, -Side * Along, Z)); };
			Add(Out, MarketModel::Cube, Colors::DoorGreen, P(Leaf * 0.5, 125), FVector(6, Leaf - 8, 230), Open);
			Add(Out, MarketModel::Cube, Colors::Wood, P(Leaf * 0.5, 40), FVector(9, Leaf - 8, 14), Open, false);
			Add(Out, MarketModel::Cube, Colors::Wood, P(Leaf * 0.5, 215), FVector(9, Leaf - 8, 14), Open, false);
		}
		// Kerbstones for the wheels at the piers' feet.
		for (const double Y : { G0 + 8.0, G1 - 8.0 })
		{
			Add(Out, MarketModel::Cone, Colors::Setts, FVector(-10, Y, 22), FVector(28, 28, 44), 0.0, false);
		}
	}

	/** The stable: plaster walls, half doors with the tops open, small windows, a loft door with hay, a tiled lean-to roof. */
	inline void BuildStable(TArray<FShape>& Out, FRandom& R)
	{
		const double T = 30.0, Eave = 330.0, Ridge = 470.0;
		const double Y0 = StableY, Y1 = FrontY;
		const double Doors[3] = { 1000.0, 1650.0, 2300.0 };
		const double DoorHalf = 75.0, DoorH = 225.0;
		// The front wall, in pieces round the doors.
		double From = StableX0;
		for (const double D : Doors)
		{
			Span(Out, Colors::Plaster, FVector(From, Y0, 0), FVector(D - DoorHalf, Y0 + T, Eave));
			Span(Out, Colors::Plaster, FVector(D - DoorHalf, Y0, DoorH), FVector(D + DoorHalf, Y0 + T, Eave));
			// The door: dark stall behind, the lower half shut, the top half folded back.
			Span(Out, Colors::Dark, FVector(D - DoorHalf, Y0 + T, 0), FVector(D + DoorHalf, Y0 + T + 4, DoorH), false);
			Span(Out, Colors::DoorGreen, FVector(D - DoorHalf + 4, Y0 + 8, 0), FVector(D + DoorHalf - 4, Y0 + 14, 120));
			Span(Out, Colors::Wood, FVector(D - DoorHalf + 4, Y0 + 2, 112), FVector(D + DoorHalf - 4, Y0 + 14, 122), false);
			Add(Out, MarketModel::Cube, Colors::DoorGreen, FVector(D + DoorHalf + 3, Y0 - 50, 172), FVector(6, 100, 100), 0.0, false);
			Span(Out, Colors::Wood, FVector(D - DoorHalf - 8, Y0 - 3, DoorH), FVector(D + DoorHalf + 8, Y0 + 2, DoorH + 12), false);
			From = D + DoorHalf;
		}
		Span(Out, Colors::Plaster, FVector(From, Y0, 0), FVector(StableX1, Y0 + T, Eave));
		Span(Out, Colors::PlasterDark, FVector(StableX0, Y0 - 3, 0), FVector(StableX1, Y0, 40), false);
		// Small barred windows between the doors.
		for (int32 i = 0; i < 2; ++i)
		{
			const double X = (Doors[i] + Doors[i + 1]) * 0.5;
			Span(Out, Colors::Dark, FVector(X - 35, Y0 - 1, 160), FVector(X + 35, Y0 + 2, 210), false);
			for (const double Bar : { -17.0, 0.0, 17.0 })
			{
				Span(Out, Colors::Iron, FVector(X + Bar - 1.5, Y0 - 3, 160), FVector(X + Bar + 1.5, Y0, 210), false);
			}
		}
		// Gable ends.
		Span(Out, Colors::Plaster, FVector(StableX0, Y0, 0), FVector(StableX0 + T, Y1, Eave));
		Span(Out, Colors::Plaster, FVector(StableX1 - T, Y0, 0), FVector(StableX1, Y1, Eave));
		Span(Out, Colors::Plaster, FVector(StableX0, Y1 - T, 0), FVector(StableX1, Y1, Ridge));
		// The loft door over the middle door, with hay showing.
		const double Loft = Doors[1];
		Span(Out, Colors::Plaster, FVector(Loft - 60, Y0, Eave), FVector(Loft + 60, Y0 + T, Eave + 90));
		Span(Out, Colors::Dark, FVector(Loft - 42, Y0 - 1, Eave + 6), FVector(Loft + 42, Y0 + 2, Eave + 80), false);
		for (int32 i = 0; i < 5; ++i)
		{
			Add(Out, MarketModel::Sphere, i % 2 ? Colors::Straw : Colors::OldStraw,
				FVector(Loft + R.Range(-30.0, 30.0), Y0 + 2, Eave + 14 + R.Range(0.0, 20.0)), FVector(34, 24, 18), R.Range(0.0, 180.0), false);
		}
		Add(Out, MarketModel::Cube, Colors::Wood, FVector(Loft, Y0 - 26, Eave + 96), FVector(10, 60, 10), 0.0, false);
		Add(Out, MarketModel::Cylinder, Colors::Iron, FVector(Loft, Y0 - 50, Eave + 70), FVector(3, 3, 50), 0.0, false);
		// The lean-to roof, from the eaves over the yard up to the house wall.
		const double Run = Y1 - Y0 + 40.0;
		const double Rise = Ridge - Eave;
		const double Slope = FMath::RadiansToDegrees(FMath::Atan2(Rise, Run));
		const double Long = FMath::Sqrt(Run * Run + Rise * Rise);
		Add(Out, MarketModel::Cube, Colors::Roof, FVector((StableX0 + StableX1) * 0.5, (Y0 - 40.0 + Y1) * 0.5, (Eave + Ridge) * 0.5 + 6.0),
			FVector(StableX1 - StableX0 + 60.0, Long, 12.0), 0.0, true, 0.0, -Slope);
		// Tile courses.
		for (int32 k = 1; k < 7; ++k)
		{
			const double F = k / 7.0;
			const double Y = Y0 - 40.0 + Run * F;
			const double Z = Eave + Rise * F + 13.0;
			Add(Out, MarketModel::Cube, Colors::PlasterDark, FVector((StableX0 + StableX1) * 0.5, Y, Z), FVector(StableX1 - StableX0 + 60.0, 4.0, 3.0), 0.0, false, 0.0, -Slope);
		}
		// A harness and a horse blanket hung on pegs by the first door, a pitchfork leant against the wall.
		Add(Out, MarketModel::Cylinder, Colors::Leather, FVector(Doors[0] - 140, Y0 - 6, 175), FVector(55, 8, 65), 0.0, false, 90.0);
		Add(Out, MarketModel::Cube, Colors::Blanket, FVector(Doors[2] + 150, Y0 - 4, 150), FVector(90, 4, 80), 0.0, false);
		Add(Out, MarketModel::Cylinder, Colors::Wood, FVector(Doors[1] + 130, Y0 - 18, 85), FVector(4, 4, 170), 0.0, false, 0.0, 10.0);
		Add(Out, MarketModel::Cube, Colors::Iron, FVector(Doors[1] + 130, Y0 - 34, 172), FVector(3, 26, 22), 0.0, false, 0.0, 10.0);
	}

	/** The water trough and the pump by the stable, sacks of oats, a barrel. */
	inline void BuildTrough(TArray<FShape>& Out)
	{
		const FVector At(560.0, 110.0, 0.0);
		Span(Out, Colors::Wood, At + FVector(-110, -30, 0), At + FVector(110, 30, 60));
		Span(Out, Colors::Water, At + FVector(-102, -22, 52), At + FVector(102, 22, 55), false);
		Span(Out, Colors::Wood, At + FVector(-112, -32, 58), At + FVector(112, 32, 62), false);
		// The pump: an iron column, spout over the trough, handle.
		const FVector Pump = At + FVector(150, 0, 0);
		Add(Out, MarketModel::Cube, Colors::Setts, Pump + FVector(0, 0, 5), FVector(50, 50, 10));
		Add(Out, MarketModel::Cylinder, Colors::Iron, Pump + FVector(0, 0, 70), FVector(18, 18, 130));
		Add(Out, MarketModel::Sphere, Colors::Iron, Pump + FVector(0, 0, 140), FVector(22, 22, 18), 0.0, false);
		Add(Out, MarketModel::Cylinder, Colors::Iron, Pump + FVector(-28, 0, 110), FVector(6, 6, 40), 0.0, false, 90.0);
		Add(Out, MarketModel::Cylinder, Colors::Iron, Pump + FVector(22, 0, 150), FVector(4, 4, 60), 0.0, false, -55.0);
		// Oats and a barrel against the stable wall.
		for (int32 i = 0; i < 3; ++i)
		{
			Add(Out, MarketModel::Cylinder, Colors::Sack, FVector(780 + i * 55, StableY - 35, 34), FVector(50, 44, 66), i * 25.0);
		}
		Add(Out, MarketModel::Cylinder, Colors::Wood, FVector(2520, StableY - 50, 45), FVector(62, 62, 90));
		Add(Out, MarketModel::Cylinder, Colors::Iron, FVector(2520, StableY - 50, 20), FVector(64, 64, 4), 0.0, false);
		Add(Out, MarketModel::Cylinder, Colors::Iron, FVector(2520, StableY - 50, 72), FVector(64, 64, 4), 0.0, false);
	}

	/** At the back: an open hay shed on posts, the dung heap, another carter's old wagon. */
	inline void BuildBack(TArray<FShape>& Out, FRandom& R)
	{
		// Hay shed: four posts, a sloping board roof, the hay heaped under it.
		const double X0 = 2700.0, X1 = 2980.0, Y0 = -630.0, Y1 = -120.0;
		for (const double X : { X0, X1 })
		{
			for (const double Y : { Y0, Y1 })
			{
				Add(Out, MarketModel::Cube, Colors::Wood, FVector(X, Y, X == X0 ? 140.0 : 165.0), FVector(16, 16, X == X0 ? 280.0 : 330.0));
			}
		}
		Add(Out, MarketModel::Cube, Colors::Wood, FVector((X0 + X1) * 0.5, (Y0 + Y1) * 0.5, 308), FVector(X1 - X0 + 60, Y1 - Y0 + 50, 8), 0.0, true, -10.0);
		for (int32 i = 0; i < 7; ++i)
		{
			Add(Out, MarketModel::Sphere, i % 3 ? Colors::Straw : Colors::OldStraw,
				FVector(R.Range(X0 + 50.0, X1 - 50.0), R.Range(Y0 + 80.0, Y1 - 80.0), R.Range(60.0, 140.0)),
				FVector(R.Range(150.0, 220.0), R.Range(160.0, 240.0), R.Range(110.0, 170.0)), R.Range(0.0, 180.0), i == 0);
		}
		// Loose straw spread in front of it.
		for (int32 i = 0; i < 6; ++i)
		{
			Add(Out, MarketModel::Sphere, Colors::OldStraw, FVector(R.Range(2450.0, 2700.0), R.Range(-600.0, -150.0), 2.0),
				FVector(R.Range(50.0, 90.0), R.Range(40.0, 80.0), 6.0), R.Range(0.0, 180.0), false);
		}
		// The dung heap in its boarded bay.
		const FVector Heap(2850.0, 60.0, 0.0);
		Span(Out, Colors::Wood, Heap + FVector(-130, -110, 0), Heap + FVector(-120, 110, 70));
		Span(Out, Colors::Wood, Heap + FVector(-130, 100, 0), Heap + FVector(130, 110, 70));
		Span(Out, Colors::Wood, Heap + FVector(-130, -110, 0), Heap + FVector(130, -100, 70));
		Add(Out, MarketModel::Sphere, Colors::Dung, Heap + FVector(20, 0, 20), FVector(220, 190, 110), 20.0);
		Add(Out, MarketModel::Sphere, Colors::OldStraw, Heap + FVector(-20, 30, 60), FVector(90, 70, 30), 50.0, false);
		// An old flat wagon, shafts down on the ground.
		const FVector Wagon(2200.0, -440.0, 0.0);
		Span(Out, Colors::WagonBlue, Wagon + FVector(-170, -70, 70), Wagon + FVector(170, 70, 80));
		Span(Out, Colors::WagonBlue, Wagon + FVector(-170, -74, 80), Wagon + FVector(170, -66, 110), false);
		Span(Out, Colors::WagonBlue, Wagon + FVector(-170, 66, 80), Wagon + FVector(170, 74, 110), false);
		for (const double X : { -110.0, 110.0 })
		{
			for (const double Y : { -82.0, 82.0 })
			{
				Add(Out, MarketModel::Cylinder, Colors::Wood, Wagon + FVector(X, Y, 50), FVector(100, 100, 8), 0.0, Y < 0.0, 0.0, 90.0);
				Add(Out, MarketModel::Cylinder, Colors::Iron, Wagon + FVector(X, Y, 50), FVector(14, 14, 12), 0.0, false, 0.0, 90.0);
			}
		}
		for (const double Y : { -45.0, 45.0 })
		{
			Add(Out, MarketModel::Cube, Colors::Wood, Wagon + FVector(-310, Y, 38), FVector(290, 8, 8), 0.0, false, -14.0);
		}
		Add(Out, MarketModel::Cylinder, Colors::Sack, Wagon + FVector(60, -10, 112), FVector(55, 48, 62), 0.0, false, 0.0, 90.0);
	}

	/** The yard's ground: beaten earth with a sett-paved strip from the gate to the stable doors. */
	inline void BuildGround(TArray<FShape>& Out)
	{
		Span(Out, Colors::Ground, FVector(0, -FrontY, 0), FVector(Depth, FrontY, 2), false);
		Span(Out, Colors::Setts, FVector(0, GateMid - GateHalf, 2), FVector(900, GateMid + GateHalf, 3), false);
		Span(Out, Colors::Setts, FVector(700, StableY - 160, 2), FVector(StableX1, StableY, 3), false);
	}

	/** Outside the gate: a spare wheel and a bench against the wall, a tie ring on the pier, straw and dung trodden into the street. */
	inline void BuildStreet(TArray<FShape>& Out, FRandom& R)
	{
		const double G0 = GateMid - GateHalf, G1 = GateMid + GateHalf;
		// A spare cart wheel leaning on the wall left of the gate.
		Add(Out, MarketModel::Cylinder, Colors::Wood, FVector(-14, G0 - 150, 50), FVector(100, 100, 8), 0.0, false, 80.0);
		Add(Out, MarketModel::Cylinder, Colors::Iron, FVector(-14, G0 - 150, 50), FVector(102, 102, 4), 0.0, false, 80.0);
		Add(Out, MarketModel::Cylinder, Colors::Iron, FVector(-18, G0 - 150, 50), FVector(16, 16, 14), 0.0, false, 80.0);
		// A carters' bench on the right, where they sit and wait for a fare.
		Span(Out, Colors::Wood, FVector(-45, G1 + 120, 42), FVector(-5, G1 + 330, 48));
		for (const double Y : { G1 + 140.0, G1 + 310.0 })
		{
			Span(Out, Colors::Wood, FVector(-40, Y - 4, 0), FVector(-10, Y + 4, 42), false);
		}
		// Iron tie rings on the piers.
		for (const double Y : { G0 - 15.0, G1 + 15.0 })
		{
			Add(Out, MarketModel::Cylinder, Colors::Iron, FVector(-8, Y, 120), FVector(14, 14, 2), 0.0, false, 0.0, 90.0);
		}
		// Straw and dung on the road where the carts turn in.
		for (int32 i = 0; i < 9; ++i)
		{
			const bool bDung = i % 3 == 0;
			Add(Out, MarketModel::Sphere, bDung ? Colors::Dung : Colors::Trodden,
				FVector(R.Range(-650.0, -60.0), GateMid + R.Range(-300.0, 300.0), 1.0),
				bDung ? FVector(26, 22, 8) : FVector(R.Range(25.0, 45.0), R.Range(20.0, 35.0), 3.0), R.Range(0.0, 180.0), false);
		}
	}

	/** Life in the yard: hens pecking about, a wheelbarrow at the dung heap, a rain barrel and downpipe, a lantern and a horseshoe by the doors, a broom. */
	inline void BuildLife(TArray<FShape>& Out, FRandom& R)
	{
		const FLinearColor Hens[3] = { FLinearColor(0.72f, 0.66f, 0.55f), FLinearColor(0.40f, 0.20f, 0.08f), FLinearColor(0.10f, 0.09f, 0.08f) };
		const FLinearColor Comb(0.55f, 0.05f, 0.04f);
		const FVector HenAt[5] = { FVector(2050, -80, 0), FVector(2180, 30, 0), FVector(1960, 90, 0), FVector(2550, -250, 0), FVector(820, -420, 0) };
		for (int32 i = 0; i < 5; ++i)
		{
			const double Yaw = R.Range(0.0, 360.0);
			const FRotator Turn(0.0, Yaw, 0.0);
			const bool bPeck = i % 2 == 0;
			const FVector Head = HenAt[i] + Turn.RotateVector(bPeck ? FVector(16, 0, 10) : FVector(14, 0, 30));
			Add(Out, MarketModel::Sphere, Hens[i % 3], HenAt[i] + FVector(0, 0, 18), FVector(32, 20, 22), Yaw, false);
			Add(Out, MarketModel::Sphere, Hens[i % 3], HenAt[i] + Turn.RotateVector(FVector(-14, 0, 26)), FVector(12, 10, 16), Yaw, false, 40.0);
			Add(Out, MarketModel::Sphere, Hens[i % 3], Head, FVector(10, 9, 11), Yaw, false);
			Add(Out, MarketModel::Cube, Comb, Head + FVector(0, 0, 6), FVector(7, 2, 5), Yaw, false);
			for (const double Side : { -4.0, 4.0 })
			{
				Add(Out, MarketModel::Cylinder, FLinearColor(0.70f, 0.55f, 0.20f), HenAt[i] + Turn.RotateVector(FVector(0, Side, 5)), FVector(1.5, 1.5, 10), Yaw, false);
			}
		}
		// A wheelbarrow by the dung heap, a fork across it.
		const FVector Barrow(2620.0, 140.0, 0.0);
		const double BarrowYaw = 200.0;
		const FRotator BT(0.0, BarrowYaw, 0.0);
		Add(Out, MarketModel::Cube, Colors::Wood, Barrow + FVector(0, 0, 45), FVector(90, 60, 30), BarrowYaw);
		Add(Out, MarketModel::Sphere, Colors::Dung, Barrow + FVector(0, 0, 60), FVector(70, 50, 22), BarrowYaw, false);
		Add(Out, MarketModel::Cylinder, Colors::Wood, Barrow + BT.RotateVector(FVector(60, 0, 25)), FVector(40, 40, 6), BarrowYaw, false, 0.0, 90.0);
		for (const double Side : { -24.0, 24.0 })
		{
			Add(Out, MarketModel::Cube, Colors::Wood, Barrow + BT.RotateVector(FVector(-80, Side, 35)), FVector(70, 4, 4), BarrowYaw, false, 15.0);
		}
		Add(Out, MarketModel::Cylinder, Colors::Wood, Barrow + FVector(0, 0, 80), FVector(3, 3, 140), BarrowYaw + 70.0, false, 0.0, 80.0);
		// A rain barrel under a downpipe at the stable's far end.
		const FVector Rain(StableX1 - 60.0, StableY - 50.0, 0.0);
		Add(Out, MarketModel::Cylinder, Colors::Wood, Rain + FVector(0, 0, 50), FVector(60, 60, 100));
		Add(Out, MarketModel::Cylinder, Colors::Water, Rain + FVector(0, 0, 99), FVector(54, 54, 2), 0.0, false);
		Add(Out, MarketModel::Cylinder, Colors::Iron, Rain + FVector(0, 30, 230), FVector(10, 10, 220), 0.0, false);
		Add(Out, MarketModel::Cube, Colors::Iron, FVector((StableX0 + StableX1) * 0.5, StableY - 45, 328), FVector(StableX1 - StableX0, 12, 10), 0.0, false);
		// A lantern on a bracket and a horseshoe over the middle door, for luck.
		Add(Out, MarketModel::Cube, Colors::Iron, FVector(1650 - 120, StableY - 20, 245), FVector(36, 4, 4), 0.0, false);
		Add(Out, MarketModel::Cube, Colors::Iron, FVector(1650 - 120, StableY - 38, 225), FVector(18, 18, 30), 0.0, false);
		Add(Out, MarketModel::Cube, FLinearColor(0.75f, 0.60f, 0.30f), FVector(1650 - 120, StableY - 38, 225), FVector(12, 19, 20), 0.0, false);
		Add(Out, MarketModel::Cylinder, Colors::Iron, FVector(1650, StableY - 3, 248), FVector(18, 18, 3), 0.0, false, 0.0, 90.0);
		// A broom leant against the trough.
		Add(Out, MarketModel::Cylinder, Colors::Wood, FVector(420, 150, 70), FVector(3, 3, 140), 0.0, false, -12.0);
		Add(Out, MarketModel::Cube, Colors::OldStraw, FVector(406, 150, 8), FVector(14, 30, 18), 0.0, false, -12.0);
	}

	inline void Build(TArray<FShape>& Out)
	{
		FRandom R(1932);
		BuildGround(Out);
		BuildGate(Out);
		BuildStable(Out, R);
		BuildTrough(Out);
		BuildBack(Out, R);
		BuildStreet(Out, R);
		BuildLife(Out, R);
	}

	inline void Signs(TArray<FSign>& Out)
	{
		Out.Add({ TEXT("FUVAROZ\u00c1S \u00b7 IST\u00c1LL\u00d3"), FVector(-8.0, GateMid, 328.0), 180.0, 26.0, Colors::Lettering });
	}

	/** Jóska, the stable lad: young, shirt sleeves, waistcoat, flat cap. */
	inline PersonKit::FOutfit Joska()
	{
		PersonKit::FOutfit O;
		O.Height = 168.f;
		O.Build = 0.9f;
		O.Hat = PersonKit::EHat::FlatCap;
		O.Top = PersonKit::ETop::Waistcoat;
		O.bTie = false;
		O.Waistcoat = FLinearColor(0.20f, 0.17f, 0.12f);
		O.HatColor = FLinearColor(0.22f, 0.19f, 0.15f);
		O.Trousers = FLinearColor(0.24f, 0.21f, 0.16f);
		O.Shirt = FLinearColor(0.72f, 0.68f, 0.58f);
		return O;
	}
}
