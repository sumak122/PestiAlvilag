// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CityDetailsModel.h"
#include "PersonKit.h"

/**
 * Two of Józsefváros's churches as they stood in 1932, in basic shapes
 * (ACityDetails puts them up in place of their plain map blocks, which
 * AOsmGraybox leaves out):
 * - the Szent Rókus chapel on Rákóczi út by Blaha Lujza tér (the 1700s):
 *   a yellow baroque front with white pilasters, one tower over the door
 *   with an onion helmet, St Rocco in a niche, and the low old hospital
 *   wings either side;
 * - the Jézus Szíve church on Mária utca (the Jesuits', 1890): a long nave
 *   of pale stone with buttresses and tall pointed windows, a rose window
 *   in the front gable, the round apse at the back, and one tall tower with
 *   a slate spire over the front.
 *
 * Header-only so it can be built and looked at outside the engine. A
 * church's frame: the middle of its front at the foot, +X out of the front
 * to the street, Y along the front, the church running back to -X.
 */
namespace ChurchModel
{
	using MarketModel::FShape;
	using BozsiHouseModel::Add;
	using BozsiHouseModel::FLamp;
	using CityDetailsModel::Rod;

	enum class EChurch : uint8 { Rokus, JezusSzive };

	namespace Colors
	{
		const FLinearColor Ochre(0.66f, 0.50f, 0.24f);
		const FLinearColor OchreDark(0.50f, 0.37f, 0.17f);
		const FLinearColor Trim(0.82f, 0.80f, 0.72f);
		const FLinearColor PaleStone(0.60f, 0.55f, 0.46f);
		const FLinearColor StoneTrim(0.74f, 0.70f, 0.61f);
		const FLinearColor Slate(0.10f, 0.11f, 0.12f);
		const FLinearColor Copper(0.20f, 0.36f, 0.30f);
		const FLinearColor Tile(0.36f, 0.16f, 0.10f);
		const FLinearColor Dark(0.03f, 0.03f, 0.035f);
		const FLinearColor Glass(0.06f, 0.07f, 0.09f);
		const FLinearColor Door(0.22f, 0.12f, 0.06f);
		const FLinearColor Gilt(0.85f, 0.64f, 0.24f);
		const FLinearColor ClockFace(0.90f, 0.88f, 0.80f);
		const FLinearColor Statue(0.70f, 0.68f, 0.62f);
	}

	/** A pitched roof over X0..X1 (along the ridge) and -Half..Half across, its eaves at Z, Rise high. */
	inline void RidgeRoof(TArray<FShape>& Out, const FLinearColor& Color, double X0, double X1, double Half, double Z, double Rise)
	{
		const double Slope = FMath::RadiansToDegrees(FMath::Atan2(Rise, Half));
		const double Run = FMath::Sqrt(Half * Half + Rise * Rise) + 30.0;
		for (const double Side : { -1.0, 1.0 })
		{
			Add(Out, MarketModel::Cube, Color, FVector((X0 + X1) * 0.5, Side * Half * 0.5, Z + Rise * 0.5),
				FVector(X1 - X0 + 40.0, Run, 14.0), 0.0, true, 0.0, Side * Slope);
		}
	}

	/** A gable end filling the triangle under such a roof, at X with Depth, as steps of stone. */
	inline void Gable(TArray<FShape>& Out, const FLinearColor& Color, double X, double Depth, double Half, double Z, double Rise)
	{
		constexpr int32 Steps = 10;
		for (int32 i = 0; i < Steps; ++i)
		{
			const double Width = 2.0 * Half * (1.0 - (i + 0.5) / Steps);
			Add(Out, MarketModel::Cube, Color, FVector(X, 0, Z + Rise * (i + 0.5) / Steps), FVector(Depth, Width, Rise / Steps + 1.0));
		}
	}

	/** A tall window with a round or pointed top on a wall, its foot's middle at At, facing Yaw (0 = +X). */
	inline void TallWindow(TArray<FShape>& Out, const FVector& At, double Yaw, double Width, double Height, const FLinearColor& Frame, bool bPointed)
	{
		const FRotator Turn(0.0, Yaw, 0.0);
		auto P = [&](double X, double Z) { return At + Turn.RotateVector(FVector(X, 0, Z)); };
		Add(Out, MarketModel::Cube, Frame, P(2, Height * 0.5), FVector(6, Width + 24, Height + 10), Yaw, false);
		Add(Out, MarketModel::Cube, Colors::Glass, P(5, Height * 0.5 - 6), FVector(4, Width, Height - 12), Yaw, false);
		if (bPointed)
		{
			Add(Out, MarketModel::Cone, Frame, P(3, Height + Width * 0.35), FVector(8, Width + 24, Width * 0.8), Yaw, false);
			Add(Out, MarketModel::Cone, Colors::Glass, P(6, Height + Width * 0.25), FVector(4, Width, Width * 0.6), Yaw, false);
		}
		else
		{
			Add(Out, MarketModel::Cylinder, Frame, P(2, Height), FVector(Width + 24, Width + 24, 6), Yaw, false, 90.0);
			Add(Out, MarketModel::Cylinder, Colors::Glass, P(5, Height - 6), FVector(Width, Width, 4), Yaw, false, 90.0);
		}
	}

	/** A clock face on a tower's front at X, Z (the hands at ten past two, fixed). */
	inline void TowerClock(TArray<FShape>& Out, double X, double Z, double Size)
	{
		Add(Out, MarketModel::Cylinder, Colors::Gilt, FVector(X, 0, Z), FVector(Size + 16, Size + 16, 4), 0.0, false, 90.0);
		Add(Out, MarketModel::Cylinder, Colors::ClockFace, FVector(X + 2, 0, Z), FVector(Size, Size, 4), 0.0, false, 90.0);
		Add(Out, MarketModel::Cube, Colors::Dark, FVector(X + 5, -Size * 0.12, Z + Size * 0.08), FVector(2, Size * 0.30, 5), 0.0, false, 0.0, -30.0);
		Add(Out, MarketModel::Cube, Colors::Dark, FVector(X + 5, -Size * 0.06, Z + Size * 0.20), FVector(2, 4, Size * 0.42), 0.0, false, 0.0, -20.0);
	}

	/** A cross on top, at Z. */
	inline void Cross(TArray<FShape>& Out, double X, double Z, double Size)
	{
		Add(Out, MarketModel::Cube, Colors::Gilt, FVector(X, 0, Z + Size * 0.5), FVector(Size * 0.08, Size * 0.08, Size), 0.0, false);
		Add(Out, MarketModel::Cube, Colors::Gilt, FVector(X, 0, Z + Size * 0.68), FVector(Size * 0.08, Size * 0.55, Size * 0.08), 0.0, false);
	}

	/** The Szent Rókus chapel with its hospital wings: Width along the front, Depth back from it. */
	inline void Rokus(double Width, double Depth, TArray<FShape>& Out, TArray<FLamp>& Lamps)
	{
		using namespace Colors;
		const double Half = Width * 0.5;
		constexpr double Nave = 640.0;      // half the chapel's width
		constexpr double Eaves = 1250.0;
		constexpr double Rise = 430.0;
		constexpr double WingTop = 980.0;
		const double Back = -Depth;

		// The chapel's body and roof, with its front gable.
		Add(Out, MarketModel::Cube, Ochre, FVector(Back * 0.5, 0, Eaves * 0.5), FVector(Depth, Nave * 2.0, Eaves));
		RidgeRoof(Out, Tile, Back, 0.0, Nave + 20.0, Eaves, Rise);
		Gable(Out, Ochre, 10.0, 20.0, Nave, Eaves + 40.0, Rise);
		// The back gable, and a ceiling under the roof so no sky shows through it from any side.
		Gable(Out, Ochre, Back + 10.0, 20.0, Nave, Eaves, Rise);
		Add(Out, MarketModel::Cube, Ochre, FVector(Back * 0.5, 0, Eaves + 5.0), FVector(Depth, Nave * 2.0, 10));
		// Baroque scrolls at the gable's shoulders, and the gable's cornice.
		for (const double Side : { -1.0, 1.0 })
		{
			Add(Out, MarketModel::Cylinder, Trim, FVector(26, Side * (Nave - 70.0), Eaves + 100.0), FVector(120, 120, 30), 0.0, false, 90.0);
			Add(Out, MarketModel::Cylinder, Ochre, FVector(36, Side * (Nave - 70.0), Eaves + 100.0), FVector(60, 60, 12), 0.0, false, 90.0);
			Add(Out, MarketModel::Cube, Trim, FVector(24, Side * (Nave - 180.0), Eaves + 230.0), FVector(16, 200, 40), 0.0, false, 0.0, Side * 28.0);
		}
		Add(Out, MarketModel::Cube, Trim, FVector(26, 0, Eaves), FVector(30, Nave * 2.0 + 60.0, 50), 0.0, false);
		Add(Out, MarketModel::Cube, Trim, FVector(10, 0, 30), FVector(40, Nave * 2.0 + 40.0, 60));
		// White pilasters up the front, their capitals under the cornice.
		for (const double Y : { -Nave + 40.0, -300.0, 300.0, Nave - 40.0 })
		{
			Add(Out, MarketModel::Cube, Trim, FVector(14, Y, Eaves * 0.5), FVector(24, 70, Eaves), 0.0, false);
			Add(Out, MarketModel::Cube, Trim, FVector(22, Y, Eaves - 50.0), FVector(30, 90, 30), 0.0, false);
		}
		// The door in a stone frame with a broken pediment, St Rocco in a niche over it.
		Add(Out, MarketModel::Cube, Trim, FVector(18, 0, 220), FVector(20, 300, 440), 0.0, false);
		Add(Out, MarketModel::Cube, Door, FVector(26, 0, 190), FVector(8, 200, 380), 0.0, false);
		Add(Out, MarketModel::Cylinder, Door, FVector(26, 0, 380), FVector(200, 200, 8), 0.0, false, 90.0);
		for (const double Side : { -1.0, 1.0 })
		{
			Add(Out, MarketModel::Cube, Trim, FVector(34, Side * 110.0, 520), FVector(26, 150, 26), 0.0, false, 0.0, Side * 18.0);
		}
		Add(Out, MarketModel::Cube, OchreDark, FVector(22, 0, 760), FVector(16, 150, 260), 0.0, false);
		Add(Out, MarketModel::Cylinder, OchreDark, FVector(22, 0, 890), FVector(150, 150, 16), 0.0, false, 90.0);
		Add(Out, MarketModel::Cylinder, Statue, FVector(42, 0, 730), FVector(52, 46, 170), 0.0, false);
		Add(Out, MarketModel::Sphere, Statue, FVector(44, 0, 840), FVector(30, 28, 34), 0.0, false);
		Add(Out, MarketModel::Cube, Statue, FVector(46, 0, 640), FVector(40, 90, 14), 0.0, false);
		// Tall round-topped windows either side of the door.
		for (const double Y : { -470.0, 470.0 })
		{
			TallWindow(Out, FVector(20, Y, 380), 0.0, 120.0, 420.0, Trim, false);
		}
		Lamps.Add({ FVector(80, 0, 470), 700.f });

		// The tower over the door: a square shaft, the belfry with its clock,
		// and the onion helmet with a lantern and the cross.
		constexpr double TowerHalf = 230.0;
		constexpr double TowerX = -250.0;   // its face just behind the front gable
		constexpr double BelfryTop = 2350.0;
		Add(Out, MarketModel::Cube, Ochre, FVector(TowerX, 0, (Eaves + BelfryTop) * 0.5), FVector(TowerHalf * 2.0, TowerHalf * 2.0, BelfryTop - Eaves + 200.0));
		for (const double Z : { Eaves + Rise + 20.0, BelfryTop })
		{
			Add(Out, MarketModel::Cube, Trim, FVector(TowerX, 0, Z), FVector(TowerHalf * 2.0 + 40.0, TowerHalf * 2.0 + 40.0, 36), 0.0, false);
		}
		for (int32 Face = 0; Face < 4; ++Face)
		{
			const double Yaw = Face * 90.0;
			const FRotator Turn(0.0, Yaw, 0.0);
			const FVector Middle = FVector(TowerX, 0, 0) + Turn.RotateVector(FVector(TowerHalf, 0, 0));
			// The belfry's arched opening, and corner pilasters.
			Add(Out, MarketModel::Cube, Dark, Middle + FVector(0, 0, 2060), FVector(6, 120, 220), Yaw, false);
			Add(Out, MarketModel::Cylinder, Dark, Middle + FVector(0, 0, 2170), FVector(120, 120, 6), Yaw, false, 90.0);
			for (const double Y : { -TowerHalf + 20.0, TowerHalf - 20.0 })
			{
				Add(Out, MarketModel::Cube, Trim, Middle + Turn.RotateVector(FVector(4, Y, (Eaves + Rise + BelfryTop) * 0.5)), FVector(18, 40, BelfryTop - Eaves - Rise), Yaw, false);
			}
		}
		TowerClock(Out, TowerX + TowerHalf + 2.0, Eaves + Rise + 160.0, 150.0);
		Add(Out, MarketModel::Cube, Copper, FVector(TowerX, 0, BelfryTop + 60.0), FVector(TowerHalf * 2.0, TowerHalf * 2.0, 90), 0.0, false);
		Add(Out, MarketModel::Sphere, Copper, FVector(TowerX, 0, BelfryTop + 230.0), FVector(440, 440, 330));
		Add(Out, MarketModel::Cylinder, Copper, FVector(TowerX, 0, BelfryTop + 440.0), FVector(110, 110, 150), 0.0, false);
		Add(Out, MarketModel::Sphere, Copper, FVector(TowerX, 0, BelfryTop + 540.0), FVector(150, 150, 110), 0.0, false);
		Add(Out, MarketModel::Cone, Copper, FVector(TowerX, 0, BelfryTop + 680.0), FVector(70, 70, 200), 0.0, false);
		Cross(Out, TowerX, BelfryTop + 770.0, 140.0);

		// The old hospital wings either side: two floors under a tiled roof.
		const double WingSpan = Half - Nave;
		if (WingSpan > 300.0)
		{
			for (const double Side : { -1.0, 1.0 })
			{
				const double Mid = Side * (Nave + WingSpan * 0.5);
				Add(Out, MarketModel::Cube, OchreDark, FVector(Back * 0.5, Mid, WingTop * 0.5), FVector(Depth, WingSpan, WingTop));
				Add(Out, MarketModel::Cube, Trim, FVector(8, Mid, WingTop - 20.0), FVector(30, WingSpan, 40), 0.0, false);
				Add(Out, MarketModel::Cube, Trim, FVector(6, Mid, 470), FVector(16, WingSpan, 20), 0.0, false);
				// A pitched roof with its ridge along the wing, a gable at the wing's
				// outer end, and a ceiling under it.
				const double HalfDeep = Depth * 0.5;
				constexpr double WingRise = 320.0;
				const double Slope = FMath::RadiansToDegrees(FMath::Atan2(WingRise, HalfDeep));
				const double Run = FMath::Sqrt(HalfDeep * HalfDeep + WingRise * WingRise) + 30.0;
				for (const double RoofSide : { 1.0, -1.0 })
				{
					Add(Out, MarketModel::Cube, Tile, FVector(Back * 0.5 + RoofSide * HalfDeep * 0.5, Mid, WingTop + WingRise * 0.5),
						FVector(Run, WingSpan + 20.0, 14), 0.0, true, -RoofSide * Slope);
				}
				Add(Out, MarketModel::Cube, OchreDark, FVector(Back * 0.5, Mid, WingTop + 5.0), FVector(Depth, WingSpan, 10));
				constexpr int32 Steps = 8;
				for (int32 i = 0; i < Steps; ++i)
				{
					Add(Out, MarketModel::Cube, OchreDark, FVector(Back * 0.5, Side * (Half - 10.0), WingTop + WingRise * (i + 0.5) / Steps),
						FVector(Depth * (1.0 - (i + 0.5) / Steps), 20, WingRise / Steps + 1.0));
				}
				const int32 Bays = FMath::Max(1, FMath::FloorToInt(WingSpan / 300.0));
				for (int32 k = 0; k < Bays; ++k)
				{
					const double Y = Side * (Nave + WingSpan * (k + 0.5) / Bays);
					for (const double Z : { 160.0, 600.0 })
					{
						Add(Out, MarketModel::Cube, Trim, FVector(4, Y, Z + 110.0), FVector(8, 130, 240), 0.0, false);
						Add(Out, MarketModel::Cube, Glass, FVector(7, Y, Z + 110.0), FVector(4, 100, 210), 0.0, false);
					}
				}
			}
		}
	}

	/** The Jézus Szíve church: the front is Width across, the church Depth long including its apse. */
	inline void JezusSzive(double Width, double Depth, TArray<FShape>& Out, TArray<FLamp>& Lamps)
	{
		using namespace Colors;
		const double Half = FMath::Min(Width * 0.5, 900.0);
		constexpr double Eaves = 1500.0;
		constexpr double Rise = 750.0;
		const double ApseRadius = Half * 0.75;
		const double NaveBack = -Depth + ApseRadius;

		// The nave, its slate roof and the front gable.
		Add(Out, MarketModel::Cube, PaleStone, FVector(NaveBack * 0.5, 0, Eaves * 0.5), FVector(-NaveBack, Half * 2.0, Eaves));
		RidgeRoof(Out, Slate, NaveBack, 0.0, Half + 30.0, Eaves, Rise);
		Gable(Out, PaleStone, 0.0, 40.0, Half, Eaves, Rise + 60.0);
		Gable(Out, PaleStone, NaveBack + 20.0, 40.0, Half, Eaves, Rise);
		Add(Out, MarketModel::Cube, PaleStone, FVector(NaveBack * 0.5, 0, Eaves + 5.0), FVector(-NaveBack, Half * 2.0, 10));
		Add(Out, MarketModel::Cube, StoneTrim, FVector(4, 0, Eaves), FVector(50, Half * 2.0 + 40.0, 40), 0.0, false);
		Add(Out, MarketModel::Cube, StoneTrim, FVector(2, 0, 40), FVector(50, Half * 2.0 + 40.0, 80));
		// The round apse with its cone roof.
		Add(Out, MarketModel::Cylinder, PaleStone, FVector(NaveBack, 0, (Eaves - 150.0) * 0.5), FVector(ApseRadius * 2.0, ApseRadius * 2.0, Eaves - 150.0));
		Add(Out, MarketModel::Cone, Slate, FVector(NaveBack, 0, Eaves - 150.0 + Rise * 0.4), FVector(ApseRadius * 2.0 + 60.0, ApseRadius * 2.0 + 60.0, Rise * 0.8), 0.0, false);

		// Buttresses and tall pointed windows down both sides.
		const int32 Bays = FMath::Max(2, FMath::FloorToInt(-NaveBack / 520.0));
		for (int32 k = 0; k <= Bays; ++k)
		{
			const double X = NaveBack + (-NaveBack - 100.0) * k / Bays;
			for (const double Side : { -1.0, 1.0 })
			{
				Add(Out, MarketModel::Cube, StoneTrim, FVector(X, Side * (Half + 40.0), Eaves * 0.42), FVector(80, 80, Eaves * 0.84));
				Add(Out, MarketModel::Cube, StoneTrim, FVector(X, Side * (Half + 25.0), Eaves * 0.86), FVector(70, 50, 140), 0.0, false, 0.0, Side * 30.0);
				if (k < Bays)
				{
					const double WX = NaveBack + (-NaveBack - 100.0) * (k + 0.5) / Bays;
					TallWindow(Out, FVector(WX, Side * Half, 480), Side * 90.0, 150.0, 640.0, StoneTrim, true);
				}
			}
		}

		// The front: three pointed portals, a rose window, small windows by the tower.
		for (const double Y : { -Half * 0.62, Half * 0.62 })
		{
			Add(Out, MarketModel::Cube, StoneTrim, FVector(20, Y, 230), FVector(30, 240, 460), 0.0, false);
			Add(Out, MarketModel::Cube, Door, FVector(36, Y, 170), FVector(6, 160, 340), 0.0, false);
			Add(Out, MarketModel::Cone, StoneTrim, FVector(22, Y, 540), FVector(30, 240, 180), 0.0, false);
			Add(Out, MarketModel::Cylinder, Glass, FVector(14, Y, 1050), FVector(220, 220, 6), 0.0, false, 90.0);
			Add(Out, MarketModel::Cylinder, StoneTrim, FVector(16, Y, 1050), FVector(80, 80, 8), 0.0, false, 90.0);
		}
		// The tower, standing out in front of the middle of the front.
		constexpr double TowerHalf = 300.0;
		constexpr double TowerX = -100.0;
		constexpr double ShaftTop = 3300.0;
		Add(Out, MarketModel::Cube, PaleStone, FVector(TowerX, 0, ShaftTop * 0.5), FVector(TowerHalf * 2.0, TowerHalf * 2.0, ShaftTop));
		for (const double Z : { 80.0, Eaves, Eaves + Rise, ShaftTop - 20.0 })
		{
			Add(Out, MarketModel::Cube, StoneTrim, FVector(TowerX, 0, Z), FVector(TowerHalf * 2.0 + 40.0, TowerHalf * 2.0 + 40.0, 40), 0.0, false);
		}
		// Corner buttresses up the tower.
		for (const double CX : { -1.0, 1.0 })
		{
			for (const double CY : { -1.0, 1.0 })
			{
				Add(Out, MarketModel::Cube, StoneTrim, FVector(TowerX + CX * (TowerHalf + 10.0), CY * (TowerHalf + 10.0), ShaftTop * 0.45), FVector(80, 80, ShaftTop * 0.9));
				Add(Out, MarketModel::Cone, Slate, FVector(TowerX + CX * (TowerHalf + 10.0), CY * (TowerHalf + 10.0), ShaftTop + 120.0), FVector(80, 80, 300), 0.0, false);
			}
		}
		// The main portal at the tower's foot, the window over it, the belfry and the clock.
		const double Face = TowerX + TowerHalf;
		Add(Out, MarketModel::Cube, StoneTrim, FVector(Face + 10.0, 0, 300), FVector(40, 320, 600), 0.0, false);
		Add(Out, MarketModel::Cube, Door, FVector(Face + 32.0, 0, 220), FVector(6, 200, 440), 0.0, false);
		Add(Out, MarketModel::Cone, StoneTrim, FVector(Face + 12.0, 0, 720), FVector(40, 320, 260), 0.0, false);
		Add(Out, MarketModel::Cube, StoneTrim, FVector(Face + 30.0, 0, 30), FVector(140, 420, 60));
		TallWindow(Out, FVector(Face, 0, 1000), 0.0, 140.0, 520.0, StoneTrim, true);
		TowerClock(Out, Face + 4.0, Eaves + Rise - 160.0, 200.0);
		for (int32 FaceIndex = 0; FaceIndex < 4; ++FaceIndex)
		{
			const double Yaw = FaceIndex * 90.0;
			const FRotator Turn(0.0, Yaw, 0.0);
			const FVector Middle = FVector(TowerX, 0, 0) + Turn.RotateVector(FVector(TowerHalf, 0, 0));
			for (const double Y : { -80.0, 80.0 })
			{
				const FVector Slot = Middle + Turn.RotateVector(FVector(2, Y, 0));
				Add(Out, MarketModel::Cube, Dark, Slot + FVector(0, 0, 2780), FVector(8, 90, 460), Yaw, false);
				Add(Out, MarketModel::Cone, Dark, Slot + FVector(0, 0, 3050), FVector(90, 90, 90), Yaw, false);
			}
		}
		// The slate spire and the cross.
		Add(Out, MarketModel::Cone, Slate, FVector(TowerX, 0, ShaftTop + 1100.0), FVector(TowerHalf * 1.7, TowerHalf * 1.7, 2200.0), 45.0, false);
		Add(Out, MarketModel::Sphere, Gilt, FVector(TowerX, 0, ShaftTop + 2210.0), FVector(40, 40, 40), 0.0, false);
		Cross(Out, TowerX, ShaftTop + 2220.0, 220.0);
		Lamps.Add({ FVector(Face + 120.0, 0, 560), 800.f });
	}

	// ---------------------------------------------------------------------
	// Life in front of the church: the steps, a flower and candle stall, and
	// the people ACityDetails stands about it.

	enum class EChurchFolk : uint8 { Priest, OldWoman, Beggar, FlowerSeller, Gentleman, Mother };

	/**
	 * Someone in front of the church: where (the church's frame), which way
	 * they face, who, the seat's height for one sitting (0 standing), and
	 * the height of the step they are on.
	 */
	struct FChurchFolk
	{
		FVector2D At;
		double Yaw;
		EChurchFolk Who;
		double SeatHeight;
		double Up = 0.0;
	};

	/** What one of the church folk wears (Seed tells the two old women apart). */
	inline PersonKit::FOutfit FolkOutfit(EChurchFolk Who, int32 Seed)
	{
		using namespace PersonKit;
		FOutfit O;
		const FLinearColor Black(0.03f, 0.03f, 0.035f);
		switch (Who)
		{
		case EChurchFolk::Priest:
			// A black cassock to the ankles, a white collar, bare head.
			O.Height = 174.f;
			O.Build = 1.12f;
			O.Hat = EHat::None;
			O.Top = ETop::Overcoat;
			O.Coat = Black;
			O.Trousers = Black;
			O.Shirt = FLinearColor(0.92f, 0.92f, 0.90f);
			O.bTie = false;
			O.Hair = FLinearColor(0.42f, 0.40f, 0.38f);
			break;
		case EChurchFolk::OldWoman:
			// Black from head to foot, a headscarf, a basket on one of them.
			O.bWoman = true;
			O.Height = Seed == 0 ? 154.f : 158.f;
			O.Build = Seed == 0 ? 1.15f : 1.05f;
			O.Hat = EHat::Hood;
			O.HatColor = Seed == 0 ? Black : FLinearColor(0.10f, 0.09f, 0.10f);
			O.Top = ETop::Dress;
			O.Coat = Black;
			O.bTie = false;
			O.Held = Seed == 0 ? EHeld::Basket : EHeld::None;
			O.HeldColor = FLinearColor(0.50f, 0.36f, 0.18f);
			O.Skin = FLinearColor(0.70f, 0.52f, 0.42f);
			break;
		case EChurchFolk::Beggar:
			// Worn brown jacket and cap, no tie, boots.
			O.Height = 170.f;
			O.Build = 0.95f;
			O.Hat = EHat::FlatCap;
			O.HatColor = FLinearColor(0.16f, 0.14f, 0.11f);
			O.Top = ETop::Jacket;
			O.Coat = FLinearColor(0.22f, 0.17f, 0.11f);
			O.Trousers = FLinearColor(0.14f, 0.13f, 0.12f);
			O.Shirt = FLinearColor(0.55f, 0.52f, 0.45f);
			O.bTie = false;
			O.bBoots = true;
			O.bMoustache = true;
			O.bScarf = true;
			O.Scarf = FLinearColor(0.30f, 0.12f, 0.08f);
			O.bSeated = true;
			break;
		case EChurchFolk::FlowerSeller:
			// A stout woman in a headscarf and a white apron.
			O.bWoman = true;
			O.Height = 160.f;
			O.Build = 1.22f;
			O.Hat = EHat::Hood;
			O.HatColor = FLinearColor(0.50f, 0.10f, 0.08f);
			O.Top = ETop::Dress;
			O.Coat = FLinearColor(0.16f, 0.20f, 0.30f);
			O.bTie = false;
			O.bApron = true;
			O.bSeated = true;
			break;
		case EChurchFolk::Gentleman:
			// Sunday best: a dark overcoat and a bowler.
			O.Height = 180.f;
			O.Build = 1.05f;
			O.Hat = EHat::Bowler;
			O.Top = ETop::Overcoat;
			O.Coat = FLinearColor(0.10f, 0.10f, 0.12f);
			O.bMoustache = true;
			O.Held = EHeld::Umbrella;
			O.HeldColor = Black;
			break;
		default:
			// A young mother in her good coat and cloche hat.
			O.bWoman = true;
			O.Height = 164.f;
			O.Hat = EHat::Cloche;
			O.HatColor = FLinearColor(0.30f, 0.12f, 0.16f);
			O.Top = ETop::Overcoat;
			O.Coat = FLinearColor(0.24f, 0.20f, 0.16f);
			O.Trousers = FLinearColor(0.18f, 0.14f, 0.12f);
			O.bTie = false;
			O.Held = EHeld::Basket;
			O.HeldColor = FLinearColor(0.50f, 0.36f, 0.18f);
			break;
		}
		return O;
	}

	/** What one of the church folk calls out when the player comes near on foot, and the name shown; false for those who keep quiet. */
	inline bool FolkCall(EChurchFolk Who, const TCHAR*& OutName, const TCHAR*& OutHu, const TCHAR*& OutEn)
	{
		switch (Who)
		{
		case EChurchFolk::Priest:
			OutName = TEXT("Pap");
			OutHu = TEXT("Dics\u00e9rtess\u00e9k a J\u00e9zus Krisztus, fiam!");
			OutEn = TEXT("Praised be Jesus Christ, my son!");
			return true;
		case EChurchFolk::Beggar:
			OutName = TEXT("Koldus");
			OutHu = TEXT("Egy fill\u00e9rt, j\u00f3 uram, az Isten \u00e1ldja meg!");
			OutEn = TEXT("A filler, good sir, God bless you!");
			return true;
		case EChurchFolk::FlowerSeller:
			OutName = TEXT("Vir\u00e1gos asszony");
			OutHu = TEXT("Vir\u00e1got, gyerty\u00e1t, tess\u00e9k! Olcs\u00f3n adom!");
			OutEn = TEXT("Flowers, candles, here you are! Going cheap!");
			return true;
		case EChurchFolk::OldWoman:
			OutName = TEXT("N\u00e9ni");
			OutHu = TEXT("...\u00e9s azt mondja a pl\u00e9b\u00e1nos \u00far, hogy b\u0171n az!");
			OutEn = TEXT("...and the parish priest says it's a sin!");
			return true;
		default:
			return false;
		}
	}

	/** The flower and candle woman's stall: a trestle table with buckets of flowers and rows of candles, a basket under it. Its front faces +X. */
	inline void FlowerStall(TArray<FShape>& Out, const FVector& At)
	{
		const FLinearColor Wood(0.34f, 0.24f, 0.14f);
		const FLinearColor Zinc(0.40f, 0.41f, 0.41f);
		const FLinearColor Wax(0.90f, 0.86f, 0.70f);
		const FLinearColor Cloth(0.80f, 0.78f, 0.70f);
		Add(Out, MarketModel::Cube, Cloth, At + FVector(0, 0, 76), FVector(70, 150, 4));
		Add(Out, MarketModel::Cube, Cloth, At + FVector(36, 0, 60), FVector(2, 150, 34), 0.0, false);
		for (const double Y : { -64.0, 64.0 })
		{
			for (const double X : { -28.0, 28.0 })
			{
				Add(Out, MarketModel::Cube, Wood, At + FVector(X, Y, 37), FVector(5, 5, 74), 0.0, false);
			}
		}
		// Buckets of flowers on the table: red, white and yellow.
		const FLinearColor Flowers[] = { FLinearColor(0.62f, 0.04f, 0.05f), FLinearColor(0.85f, 0.84f, 0.80f), FLinearColor(0.80f, 0.62f, 0.10f) };
		for (int32 i = 0; i < 3; ++i)
		{
			const FVector Bucket = At + FVector(-10, -48 + i * 34, 90);
			Add(Out, MarketModel::Cylinder, Zinc, Bucket, FVector(24, 24, 24), 0.0, false);
			Add(Out, MarketModel::Sphere, FLinearColor(0.10f, 0.22f, 0.06f), Bucket + FVector(0, 0, 20), FVector(30, 30, 22), 0.0, false);
			Add(Out, MarketModel::Sphere, Flowers[i], Bucket + FVector(2, 0, 30), FVector(26, 26, 14), 0.0, false);
		}
		// Candles for sale, standing and in a bundle.
		for (int32 i = 0; i < 6; ++i)
		{
			Add(Out, MarketModel::Cylinder, Wax, At + FVector(20, 30 + (i % 3) * 12, 86 + (i / 3) * 1.0), FVector(3, 3, 20), 0.0, false);
		}
		Add(Out, MarketModel::Cube, Wax, At + FVector(22, 54, 81), FVector(14, 10, 6), 0.0, false);
		// A basket on the ground and the seller's stool.
		Add(Out, MarketModel::Cylinder, FLinearColor(0.52f, 0.38f, 0.18f), At + FVector(10, 92, 16), FVector(40, 40, 32), 0.0, false);
		Add(Out, MarketModel::Cube, Wood, At + FVector(-70, 0, 22), FVector(34, 34, 44));
	}

	/** Two worn stone steps across the door, Width wide, its front at X. */
	inline void Steps(TArray<FShape>& Out, double X, double Width)
	{
		const FLinearColor Step(0.56f, 0.53f, 0.47f);
		Add(Out, MarketModel::Cube, Step, FVector(X - 20.0, 0, 7), FVector(60, Width, 14));
		Add(Out, MarketModel::Cube, Step, FVector(X - 45.0, 0, 21), FVector(40, Width - 40.0, 14));
	}

	/** The church's forecourt things and people. */
	inline void Forecourt(EChurch Which, TArray<FShape>& Out, TArray<FChurchFolk>& Folk)
	{
		// Where the door's threshold is, out from the front.
		const double Door = Which == EChurch::Rokus ? 30.0 : 270.0;
		Steps(Out, Door + 60.0, Which == EChurch::Rokus ? 360.0 : 440.0);
		// The Jézus Szíve's pavement is narrower: the folk keep closer to the door there.
		const double Reach = Which == EChurch::Rokus ? 1.0 : 0.65;
		const FVector2D Stall(Door + 230.0 * Reach, Which == EChurch::Rokus ? 520.0 : 440.0);
		FlowerStall(Out, FVector(Stall, 0.0));
		// The priest on the steps, seeing people out.
		Folk.Add({ FVector2D(Door + 40.0, 70.0), 0.0, EChurchFolk::Priest, 0.0, 14.0 });
		// The flower woman on her stool behind the stall.
		Folk.Add({ FVector2D(Stall.X - 70.0, Stall.Y), 0.0, EChurchFolk::FlowerSeller, 44.0 });
		// Two old women in black talking by the door.
		Folk.Add({ FVector2D(Door + 210.0 * Reach, -260.0), 70.0, EChurchFolk::OldWoman, 0.0 });
		Folk.Add({ FVector2D(Door + 210.0 * Reach + 50.0, -170.0), -110.0, EChurchFolk::OldWoman, 0.0 });
		// The beggar sitting on the top step by the door with his hat out.
		Folk.Add({ FVector2D(Door + 15.0, -150.0), 10.0, EChurchFolk::Beggar, 28.0 });
		// A gentleman and a mother coming to the door.
		Folk.Add({ FVector2D(Door + 330.0 * Reach, 120.0), 180.0, EChurchFolk::Gentleman, 0.0 });
		Folk.Add({ FVector2D(Door + 380.0 * Reach, -40.0), 165.0, EChurchFolk::Mother, 0.0 });
		// The beggar's hat on the step in front of him.
		Add(Out, MarketModel::Cylinder, FLinearColor(0.10f, 0.09f, 0.08f), FVector(Door + 65.0, -150.0, 16), FVector(28, 28, 6), 0.0, false);
	}

	inline void Church(EChurch Which, double Width, double Depth, TArray<FShape>& Out, TArray<FLamp>& Lamps)
	{
		if (Which == EChurch::Rokus)
		{
			Rokus(Width, Depth, Out, Lamps);
		}
		else
		{
			JezusSzive(Width, Depth, Out, Lamps);
		}
	}
}
