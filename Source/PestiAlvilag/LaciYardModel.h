// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MarketModel.h"
#include "PersonKit.h"

/**
 * Laci's motor workshop on Szerdahelyi utca, off Mátyás tér: Bözsi néni's
 * nephew mends cars in an old yard here, far from where the doctor's Steyr
 * is stolen (mission 7). A wall and a wide gate to the street with the
 * painted sign, a cobbled yard with an inspection pit and a lifting
 * gantry over it, an open-fronted workshop shed at the back with the
 * workbench, a hand petrol pump by the gate, oil drums, tyres and spare
 * parts about.
 *
 * Header-only, as basic shapes (MarketModel::FShape), so it can be built
 * and looked at outside the engine. The yard's frame: the street front runs
 * along Y at X = 0, +X goes into the yard, Z up from the pavement. It fills
 * the empty lot between the houses, Y -720..760, X 0..2330.
 */
namespace LaciYardModel
{
	using MarketModel::FShape;
	using MarketModel::FRandom;

	/** Painted lettering on a board or a wall: the actor makes it a text in the game. */
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
		const FLinearColor Plaster(0.52f, 0.47f, 0.38f);
		const FLinearColor PlasterDark(0.36f, 0.32f, 0.26f);
		const FLinearColor Brick(0.36f, 0.16f, 0.10f);
		const FLinearColor Roof(0.20f, 0.20f, 0.21f);
		const FLinearColor Ground(0.24f, 0.22f, 0.20f);
		const FLinearColor Setts(0.30f, 0.29f, 0.27f);
		const FLinearColor Oil(0.05f, 0.045f, 0.04f);
		const FLinearColor Wood(0.28f, 0.19f, 0.11f);
		const FLinearColor DoorBlue(0.12f, 0.17f, 0.24f);
		const FLinearColor Dark(0.035f, 0.03f, 0.03f);
		const FLinearColor Iron(0.05f, 0.05f, 0.055f);
		const FLinearColor Steel(0.34f, 0.34f, 0.35f);
		const FLinearColor Rubber(0.02f, 0.02f, 0.02f);
		const FLinearColor DrumRed(0.32f, 0.05f, 0.03f);
		const FLinearColor DrumGreen(0.08f, 0.16f, 0.09f);
		const FLinearColor DrumGrey(0.22f, 0.23f, 0.24f);
		const FLinearColor PumpRed(0.45f, 0.06f, 0.04f);
		const FLinearColor Globe(0.85f, 0.80f, 0.62f);
		const FLinearColor Brass(0.55f, 0.42f, 0.16f);
		const FLinearColor Rag(0.55f, 0.50f, 0.40f);
		const FLinearColor Sign(0.80f, 0.74f, 0.58f);
		const FLinearColor Lettering(0.10f, 0.07f, 0.05f);
		const FLinearColor WallLettering(0.78f, 0.72f, 0.56f);
		const FLinearColor OldCar(0.20f, 0.07f, 0.05f);
		const FLinearColor Leather(0.18f, 0.08f, 0.04f);
	}

	/** Where the yard stands in the city: the middle of its street front on Szerdahelyi utca, and the way into it (degrees). */
	const FVector2D CityAt(84658.0, 51189.0);
	constexpr double CityYaw = 60.8;

	// Sizes (cm).
	constexpr double LeftY = -720.0;        // the lot reaches the sheds of the next house here
	constexpr double RightY = 760.0;        // and the house on the other side here
	constexpr double Depth = 2330.0;
	constexpr double GateHalf = 190.0;      // half the gateway's width, about Y = GateMid
	constexpr double GateMid = 220.0;
	constexpr double ShedX = 1720.0;        // the workshop shed's open front
	constexpr double PitX0 = 820.0, PitX1 = 1300.0, PitHalf = 45.0;

	/** Where things are, in the yard's frame. */
	namespace Spots
	{
		/** Where the Steyr is to be left: over the pit, facing the workshop (the car's middle, and its yaw). */
		const FVector Park(1060.0, GateMid, 0.0);
		constexpr double ParkYaw = 0.0;
		/** Laci, by the workbench end of the shed, looking at the gate. */
		const FVector Laci(1560.0, -330.0, 0.0);
		constexpr double LaciYaw = 160.0;
		/** In the gateway, the way in and out. */
		const FVector Gate(0.0, GateMid, 0.0);
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
		Add(Out, MarketModel::Cube, Color, (A + B) * 0.5, (B - A).GetAbs(), 0.0, bCollide);
	}

	/** A tyre lying flat (or standing, Roll 90) at At. */
	inline void Tyre(TArray<FShape>& Out, const FVector& At, double Yaw = 0.0, double Roll = 0.0, bool bCollide = false)
	{
		Add(Out, MarketModel::Cylinder, Colors::Rubber, At, FVector(76, 76, 14), Yaw, bCollide, 0.0, Roll);
		Add(Out, MarketModel::Cylinder, Colors::Dark, At, FVector(46, 46, 15), Yaw, false, 0.0, Roll);
	}

	/** An oil drum standing at At. */
	inline void Drum(TArray<FShape>& Out, const FVector& At, const FLinearColor& Color)
	{
		Add(Out, MarketModel::Cylinder, Color, At + FVector(0, 0, 44), FVector(58, 58, 88));
		for (const double Z : { 28.0, 60.0 })
		{
			Add(Out, MarketModel::Cylinder, Colors::Iron, At + FVector(0, 0, Z), FVector(60, 60, 3), 0.0, false);
		}
		Add(Out, MarketModel::Cylinder, Colors::Iron, At + FVector(14, 8, 88.5), FVector(8, 8, 2), 0.0, false);
	}

	/** The street front: a wall either side of the gateway, its two plank leaves standing open into the yard, the sign board over it. */
	inline void BuildGate(TArray<FShape>& Out)
	{
		const double T = 35.0, H = 280.0;
		const double G0 = GateMid - GateHalf, G1 = GateMid + GateHalf;
		Span(Out, Colors::Plaster, FVector(0, LeftY, 0), FVector(T, G0 - 30, H));
		Span(Out, Colors::Plaster, FVector(0, G1 + 30, 0), FVector(T, RightY, H));
		Span(Out, Colors::PlasterDark, FVector(-4, LeftY, 0), FVector(0, G0 - 30, 45), false);
		Span(Out, Colors::PlasterDark, FVector(-4, G1 + 30, 0), FVector(0, RightY, 45), false);
		Span(Out, Colors::PlasterDark, FVector(-6, LeftY, H), FVector(T + 6, G0 - 30, H + 10), false);
		Span(Out, Colors::PlasterDark, FVector(-6, G1 + 30, H), FVector(T + 6, RightY, H + 10), false);
		// Side walls along both edges of the lot, from the street wall back to the
		// shed: the houses either side stand back from the street and have
		// courtyards opening this way, so without these there are gaps.
		for (const double Y : { LeftY - 12.0, RightY + 12.0 })
		{
			Span(Out, Colors::Plaster, FVector(0, Y - 13, 0), FVector(ShedX + 10, Y + 13, H));
			Span(Out, Colors::PlasterDark, FVector(0, Y - 16, H), FVector(ShedX + 10, Y + 16, H + 10), false);
		}
		// Bare brick where the plaster has come off.
		Span(Out, Colors::Brick, FVector(-1.5, LeftY + 120, 90), FVector(0, LeftY + 260, 150), false);
		Span(Out, Colors::Brick, FVector(-1.5, LeftY + 400, 170), FVector(0, LeftY + 470, 230), false);
		// Brick gate piers with caps.
		for (const double Y : { G0 - 15.0, G1 + 15.0 })
		{
			Add(Out, MarketModel::Cube, Colors::Brick, FVector(T * 0.5, Y, 165), FVector(T + 20, 50, 330));
			Add(Out, MarketModel::Cube, Colors::PlasterDark, FVector(T * 0.5, Y, 335), FVector(T + 34, 64, 12), 0.0, false);
		}
		// The sign board across the top, between the piers (the lettering is a sign).
		Span(Out, Colors::Iron, FVector(T * 0.5 - 3, G0, 296), FVector(T * 0.5 + 3, G1, 302), false);
		Span(Out, Colors::Sign, FVector(T * 0.5 - 6, G0 + 20, 302), FVector(T * 0.5 - 2, G1 - 20, 352), false);
		// The two leaves, planked and blue, swung open against the walls inside.
		for (const double Side : { -1.0, 1.0 })
		{
			const double Hinge = Side < 0.0 ? G0 : G1;
			const double Open = 80.0 * Side;
			const double Leaf = GateHalf;
			const FRotator Turn(0.0, Open, 0.0);
			const FVector Base(T + 10.0, Hinge, 0.0);
			const auto P = [&](double Along, double Z) { return Base + Turn.RotateVector(FVector(0.0, -Side * Along, Z)); };
			Add(Out, MarketModel::Cube, Colors::DoorBlue, P(Leaf * 0.5, 120), FVector(6, Leaf - 8, 220), Open);
			Add(Out, MarketModel::Cube, Colors::Wood, P(Leaf * 0.5, 40), FVector(9, Leaf - 8, 14), Open, false);
			Add(Out, MarketModel::Cube, Colors::Wood, P(Leaf * 0.5, 200), FVector(9, Leaf - 8, 14), Open, false);
		}
		// Kerb guards at the piers' feet.
		for (const double Y : { G0 + 8.0, G1 - 8.0 })
		{
			Add(Out, MarketModel::Cone, Colors::Setts, FVector(-10, Y, 22), FVector(28, 28, 44), 0.0, false);
		}
	}

	/** The yard's ground: cobbles, oil stains, the inspection pit with its timber edge and a plank across it. */
	inline void BuildGround(TArray<FShape>& Out, FRandom& R)
	{
		Span(Out, Colors::Ground, FVector(0, LeftY, 0), FVector(Depth, RightY, 2), false);
		Span(Out, Colors::Setts, FVector(0, GateMid - GateHalf, 2), FVector(ShedX, GateMid + GateHalf, 3), false);
		for (int32 i = 0; i < 9; ++i)
		{
			Add(Out, MarketModel::Cylinder, Colors::Oil, FVector(R.Range(300.0, 1900.0), GateMid + R.Range(-260.0, 260.0), 3.2),
				FVector(R.Range(40.0, 110.0), R.Range(30.0, 80.0), 0.6), R.Range(0.0, 180.0), false);
		}
		// The pit: dark, edged with timber, steps down at the far end.
		const double PY = GateMid;
		Span(Out, Colors::Dark, FVector(PitX0, PY - PitHalf, 3.0), FVector(PitX1, PY + PitHalf, 3.6), false);
		Span(Out, Colors::Wood, FVector(PitX0 - 10, PY - PitHalf - 12, 3), FVector(PitX1 + 10, PY - PitHalf, 7), false);
		Span(Out, Colors::Wood, FVector(PitX0 - 10, PY + PitHalf, 3), FVector(PitX1 + 10, PY + PitHalf + 12, 7), false);
		Span(Out, Colors::Wood, FVector(PitX0 - 10, PY - PitHalf, 3), FVector(PitX0, PY + PitHalf, 7), false);
		Span(Out, Colors::Wood, FVector(PitX1, PY - PitHalf, 3), FVector(PitX1 + 10, PY + PitHalf, 7), false);
		for (int32 i = 0; i < 3; ++i)
		{
			Span(Out, Colors::Wood, FVector(PitX1 - 30 - i * 26, PY - PitHalf + 4, 3.7), FVector(PitX1 - 12 - i * 26, PY + PitHalf - 4, 4.4), false);
		}
	}

	/** The workshop: an open-fronted shed across the back, a board roof sloping to the back wall, the bench, tools, a lamp. */
	inline void BuildShed(TArray<FShape>& Out)
	{
		const double Back = Depth, Eave = 300.0, High = 400.0;
		// Back wall (against the house behind) and the two ends.
		Span(Out, Colors::Plaster, FVector(Back - 30, LeftY, 0), FVector(Back, RightY, High));
		Span(Out, Colors::Plaster, FVector(ShedX, LeftY, 0), FVector(Back, LeftY + 25, Eave));
		Span(Out, Colors::Plaster, FVector(ShedX, RightY - 25, 0), FVector(Back, RightY, Eave));
		Span(Out, Colors::Dark, FVector(ShedX + 20, LeftY + 25, 0), FVector(Back - 30, RightY - 25, 2.5), false);
		// Posts and the beam along the open front.
		for (const double Y : { LeftY + 40.0, -60.0, 500.0, RightY - 40.0 })
		{
			Add(Out, MarketModel::Cube, Colors::Wood, FVector(ShedX, Y, Eave * 0.5), FVector(18, 18, Eave));
		}
		Span(Out, Colors::Wood, FVector(ShedX - 10, LeftY + 20, Eave - 22), FVector(ShedX + 10, RightY - 20, Eave), false);
		// The roof: tarred boards from the eave up to the back wall.
		const double Run = Back - ShedX + 60.0;
		const double Rise = High - Eave;
		const double Slope = FMath::RadiansToDegrees(FMath::Atan2(Rise, Run));
		const double Long = FMath::Sqrt(Run * Run + Rise * Rise);
		Add(Out, MarketModel::Cube, Colors::Roof, FVector(ShedX - 60.0 + Run * 0.5, (LeftY + RightY) * 0.5, (Eave + High) * 0.5 + 6.0),
			FVector(Long, RightY - LeftY + 40.0, 10.0), 0.0, true, Slope);
		// The workbench along the back wall, a vice, tools on a board above, a lamp hanging over it.
		const double BenchX = Back - 30.0 - 70.0;
		Span(Out, Colors::Wood, FVector(BenchX, -600, 88), FVector(Back - 30, 200, 96));
		for (const double Y : { -590.0, -200.0, 190.0 })
		{
			Span(Out, Colors::Wood, FVector(BenchX + 4, Y - 5, 0), FVector(BenchX + 14, Y + 5, 88), false);
			Span(Out, Colors::Wood, FVector(Back - 44, Y - 5, 0), FVector(Back - 34, Y + 5, 88), false);
		}
		Span(Out, Colors::Iron, FVector(BenchX - 6, -480, 96), FVector(BenchX + 20, -450, 118), false);
		Span(Out, Colors::Wood, FVector(Back - 33, -560, 140), FVector(Back - 30, 140, 240), false);
		for (int32 i = 0; i < 12; ++i)
		{
			const double Y = -530.0 + i * 55.0;
			const double Len = 30.0 + (i % 4) * 9.0;
			Add(Out, MarketModel::Cube, i % 3 == 0 ? Colors::Wood : Colors::Steel, FVector(Back - 36, Y, 210 - Len * 0.5), FVector(2.5, 4, Len), 0.0, false, 0.0, (i % 2) * 8.0 - 4.0);
		}
		Add(Out, MarketModel::Cylinder, Colors::Iron, FVector(BenchX + 20, -200, 245), FVector(1.5, 1.5, 90), 0.0, false);
		Add(Out, MarketModel::Cone, Colors::Iron, FVector(BenchX + 20, -200, 195), FVector(36, 36, 18), 0.0, false);
		Add(Out, MarketModel::Sphere, Colors::Globe, FVector(BenchX + 20, -200, 188), FVector(12, 12, 12), 0.0, false);
		// A car engine on a stand by the bench, rags and a can on the bench.
		const FVector EngineAt(BenchX - 150, 420, 0);
		Span(Out, Colors::Iron, EngineAt + FVector(-30, -30, 0), EngineAt + FVector(30, 30, 50));
		Span(Out, Colors::DrumGrey, EngineAt + FVector(-55, -30, 50), EngineAt + FVector(55, 30, 110));
		Span(Out, Colors::Iron, EngineAt + FVector(-45, -22, 110), EngineAt + FVector(45, 22, 122), false);
		Add(Out, MarketModel::Cylinder, Colors::Iron, EngineAt + FVector(62, 0, 80), FVector(30, 30, 8), 0.0, false, 90.0);
		Add(Out, MarketModel::Sphere, Colors::Rag, FVector(BenchX + 30, -60, 100), FVector(30, 24, 8), 30.0, false);
		Add(Out, MarketModel::Cylinder, Colors::PumpRed, FVector(BenchX + 34, 80, 112), FVector(18, 18, 32), 0.0, false);
	}

	/** The lifting gantry over the pit's end: two A-frame legs, the beam, a chain block hanging. */
	inline void BuildGantry(TArray<FShape>& Out)
	{
		const double X = PitX1 + 140.0, Top = 330.0;
		for (const double Y : { GateMid - 260.0, GateMid + 260.0 })
		{
			for (const double Lean : { -12.0, 12.0 })
			{
				Add(Out, MarketModel::Cube, Colors::Wood, FVector(X + Lean * 3.0, Y, Top * 0.5), FVector(14, 14, Top + 10.0), 0.0, true, Lean);
			}
		}
		Span(Out, Colors::Wood, FVector(X - 10, GateMid - 280, Top - 6), FVector(X + 10, GateMid + 280, Top + 14), false);
		Add(Out, MarketModel::Cube, Colors::Iron, FVector(X, GateMid, Top - 30), FVector(20, 14, 34), 0.0, false);
		Add(Out, MarketModel::Cylinder, Colors::Iron, FVector(X, GateMid, Top - 100), FVector(2, 2, 110), 0.0, false);
		Add(Out, MarketModel::Cube, Colors::Iron, FVector(X, GateMid, Top - 160), FVector(10, 4, 14), 0.0, false);
	}

	/** By the walls: the petrol pump, oil drums, tyres, a car door and a mudguard, a bare car chassis with its body off. */
	inline void BuildClutter(TArray<FShape>& Out, FRandom& R)
	{
		// The hand petrol pump inside the gate on the right: a red column, a glass globe, the hose and a crank.
		const FVector Pump(160.0, GateMid + GateHalf + 110.0, 0.0);
		Add(Out, MarketModel::Cube, Colors::Setts, Pump + FVector(0, 0, 6), FVector(60, 60, 12));
		Add(Out, MarketModel::Cylinder, Colors::PumpRed, Pump + FVector(0, 0, 110), FVector(38, 38, 200));
		Add(Out, MarketModel::Cylinder, Colors::Globe, Pump + FVector(0, 0, 230), FVector(30, 30, 44), 0.0, false);
		Add(Out, MarketModel::Sphere, Colors::Globe, Pump + FVector(0, 0, 262), FVector(36, 36, 30), 0.0, false);
		Add(Out, MarketModel::Cylinder, Colors::Brass, Pump + FVector(-22, 0, 140), FVector(6, 6, 30), 0.0, false, 0.0, 90.0);
		Add(Out, MarketModel::Cylinder, Colors::Rubber, Pump + FVector(-24, -18, 90), FVector(4, 4, 110), 0.0, false, 0.0, 10.0);
		// Oil drums along the right wall.
		const FLinearColor DrumColors[3] = { Colors::DrumRed, Colors::DrumGreen, Colors::DrumGrey };
		for (int32 i = 0; i < 5; ++i)
		{
			Drum(Out, FVector(500.0 + i * 66.0 + R.Range(-5.0, 5.0), RightY - 40.0 - (i % 2) * 8.0, 0.0), DrumColors[i % 3]);
		}
		Add(Out, MarketModel::Cylinder, Colors::DrumGrey, FVector(880, RightY - 120, 30), FVector(58, 58, 88), 30.0, false, 90.0);
		// Tyres stacked by the left wall, one leaning.
		for (int32 Stack = 0; Stack < 3; ++Stack)
		{
			const int32 N = 3 + Stack % 3;
			for (int32 k = 0; k < N; ++k)
			{
				Tyre(Out, FVector(420.0 + Stack * 90.0 + R.Range(-4.0, 4.0), LeftY + 60.0 + R.Range(-4.0, 4.0), 7.0 + k * 14.0), 0.0, 0.0, k == 0);
			}
		}
		Tyre(Out, FVector(720.0, LeftY + 30.0, 38.0), 0.0, 78.0);
		// A bare rolling chassis by the left wall, its body off: frame, wheels, engine, radiator, the bulkhead, wheel and seat.
		const FVector Old(1220.0, LeftY + 190.0, 0.0);
		for (const double Y : { -38.0, 38.0 })
		{
			Span(Out, Colors::Iron, Old + FVector(-210, Y - 4, 40), Old + FVector(190, Y + 4, 54));
		}
		for (const double X : { -140.0, 140.0 })
		{
			Add(Out, MarketModel::Cylinder, Colors::Iron, Old + FVector(X, 0, 36), FVector(8, 8, 150), 0.0, false, 0.0, 90.0);
			for (const double Y : { -76.0, 76.0 })
			{
				Tyre(Out, Old + FVector(X, Y, 36), 0.0, 90.0, true);
				Add(Out, MarketModel::Cylinder, Colors::OldCar, Old + FVector(X, Y, 36), FVector(48, 48, 16), 0.0, false, 0.0, 90.0);
			}
		}
		Span(Out, Colors::DrumGrey, Old + FVector(60, -24, 54), Old + FVector(150, 24, 110));
		Span(Out, Colors::Iron, Old + FVector(70, -18, 110), Old + FVector(140, 18, 120), false);
		Span(Out, Colors::Steel, Old + FVector(166, -30, 46), Old + FVector(180, 30, 126));
		Span(Out, Colors::Dark, Old + FVector(179, -24, 52), Old + FVector(181, 24, 120), false);
		Span(Out, Colors::OldCar, Old + FVector(30, -60, 54), Old + FVector(36, 60, 130));
		Add(Out, MarketModel::Cylinder, Colors::Iron, Old + FVector(10, -26, 112), FVector(4, 4, 70), 0.0, false, 50.0);
		Add(Out, MarketModel::Cylinder, Colors::Rubber, Old + FVector(-14, -26, 132), FVector(40, 40, 3), 0.0, false, 50.0);
		Span(Out, Colors::Leather, Old + FVector(-70, -55, 54), Old + FVector(-20, 55, 78));
		Span(Out, Colors::Leather, Old + FVector(-80, -55, 78), Old + FVector(-68, 55, 130));
		Add(Out, MarketModel::Cylinder, Colors::DrumGrey, Old + FVector(-190, 0, 70), FVector(40, 40, 100), 0.0, false, 0.0, 90.0);
		Add(Out, MarketModel::Cube, Colors::OldCar, FVector(1560, LeftY + 16, 70), FVector(90, 4, 120), 0.0, false, 0.0, -10.0);
		Add(Out, MarketModel::Cube, Colors::Iron, FVector(1430, LeftY + 20, 40), FVector(110, 8, 30), 20.0, false, 0.0, -60.0);
	}

	inline void Build(TArray<FShape>& Out)
	{
		FRandom R(1932);
		BuildGround(Out, R);
		BuildGate(Out);
		BuildShed(Out);
		BuildGantry(Out);
		BuildClutter(Out, R);
	}

	inline void Signs(TArray<FSign>& Out)
	{
		Out.Add({ TEXT("AUT\u00d3JAV\u00cdT\u00d3"), FVector(-8.0, GateMid, 327.0), 180.0, 30.0, Colors::Lettering });
		Out.Add({ TEXT("BENZIN \u00b7 OLAJ \u00b7 GUMI"), FVector(-2.0, GateMid + GateHalf + 290.0, 200.0), 180.0, 22.0, Colors::WallLettering });
	}

	/** Laci, Bözsi néni's nephew, the mechanic: young, blue bib overalls over a grey shirt, a red neckerchief, a flat cap, boots. */
	inline PersonKit::FOutfit Laci()
	{
		PersonKit::FOutfit O;
		O.Height = 177.f;
		O.Build = 0.95f;
		O.Hat = PersonKit::EHat::FlatCap;
		O.Top = PersonKit::ETop::Waistcoat;
		O.bTie = false;
		O.bScarf = true;
		O.bBoots = true;
		O.Scarf = FLinearColor(0.45f, 0.07f, 0.05f);
		O.Waistcoat = FLinearColor(0.10f, 0.15f, 0.26f);
		O.Trousers = FLinearColor(0.10f, 0.15f, 0.26f);
		O.Shirt = FLinearColor(0.50f, 0.50f, 0.48f);
		O.HatColor = FLinearColor(0.14f, 0.14f, 0.15f);
		O.Hair = FLinearColor(0.16f, 0.10f, 0.05f);
		O.Shoes = FLinearColor(0.06f, 0.04f, 0.03f);
		return O;
	}
}
