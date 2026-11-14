// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CityDetailsModel.h"

/**
 * The OTI palace (Országos Társadalombiztosító Intézet, built 1912-13 by
 * Komor and Jakab for the workers' insurance fund) on Fiumei út, facing the
 * Kerepesi cemetery's main gate: a long four-storey front of pale plaster on a
 * grey stone ground floor, windows in a steady rhythm between flat pilasters,
 * a band of green Zsolnay tiles under the cornice, rounded gables over the two
 * end wings, and in the middle the arched gate with the institute's name and
 * a clock tower with a green copper spire. The detail is simplified;
 * ACityDetails dresses each street wall of its map block with one run.
 *
 * Header-only so it can be built and looked at outside the engine. A run's
 * frame: its start at the foot, +X out to the street, the wall along +Y for
 * Length.
 */
namespace OtiModel
{
	using MarketModel::FShape;
	using BozsiHouseModel::FSign;
	using BozsiHouseModel::FLamp;
	using BozsiHouseModel::Add;

	namespace Colors
	{
		const FLinearColor Plaster(0.70f, 0.66f, 0.56f);
		const FLinearColor Stone(0.46f, 0.45f, 0.42f);
		const FLinearColor Trim(0.84f, 0.81f, 0.72f);
		const FLinearColor Tile(0.16f, 0.38f, 0.36f);
		const FLinearColor Copper(0.27f, 0.45f, 0.38f);
		const FLinearColor Glass(0.06f, 0.07f, 0.09f);
		const FLinearColor Door(0.22f, 0.13f, 0.07f);
		const FLinearColor Gilt(0.85f, 0.64f, 0.24f);
		const FLinearColor Plate(0.10f, 0.12f, 0.11f);
		const FLinearColor Face(0.92f, 0.90f, 0.82f);
		const FLinearColor Hand(0.08f, 0.08f, 0.08f);
	}

	// Sizes (cm): the map block is four storeys, 1520 high.
	constexpr double WallTop = 1520.0;
	constexpr double StoreyHeight = 380.0;
	constexpr double BaseTop = 380.0;
	constexpr double BayWidth = 360.0;
	constexpr double Skin = 14.0;
	constexpr double MiddleWidth = 2200.0;
	constexpr double MiddleOut = 45.0;
	constexpr double WingWidth = 1300.0;
	constexpr double WingOut = 30.0;
	constexpr double TowerSide = 800.0;
	constexpr double TowerTop = WallTop + 1000.0;

	/** A window on the wall at Y, its sill at Z, its face at Front; bArched gives it a round head. */
	inline void Window(TArray<FShape>& Out, double Y, double Z, double Width, double Height, double Front, bool bArched = false)
	{
		using namespace Colors;
		Add(Out, MarketModel::Cube, Trim, FVector(Front + 3, Y, Z + Height * 0.5), FVector(6, Width + 24, Height + 10), 0.0, false);
		Add(Out, MarketModel::Cube, Glass, FVector(Front + 6, Y, Z + Height * 0.5 - 2), FVector(4, Width, Height - 6), 0.0, false);
		Add(Out, MarketModel::Cube, Trim, FVector(Front + 8, Y, Z - 6), FVector(16, Width + 36, 12), 0.0, false);
		if (bArched)
		{
			Add(Out, MarketModel::Cylinder, Trim, FVector(Front + 3, Y, Z + Height), FVector(Width + 24, Width + 24, 6), 0.0, false, 90.0);
			Add(Out, MarketModel::Cylinder, Glass, FVector(Front + 6, Y, Z + Height - 2), FVector(Width, Width, 4), 0.0, false, 90.0);
		}
	}

	/** The windows of one bay at Y, all floors, faces at Front. */
	inline void BayWindows(TArray<FShape>& Out, double Y, double Front, double Width)
	{
		Window(Out, Y, 110.0, Width, 190.0, Front + 8.0, true);
		for (int32 Floor = 1; Floor < 4; ++Floor)
		{
			Window(Out, Y, Floor * StoreyHeight + 80.0, Width, Floor == 3 ? 200.0 : 230.0, Front);
		}
	}

	/** The wall skin from Y0 to Y1 standing Front deep: stone ground floor, string courses, tile band, cornice, parapet. */
	inline void Wall(TArray<FShape>& Out, double Y0, double Y1, double Front)
	{
		using namespace Colors;
		const double Mid = (Y0 + Y1) * 0.5;
		const double Long = Y1 - Y0;
		Add(Out, MarketModel::Cube, Plaster, FVector(Front * 0.5, Mid, WallTop * 0.5), FVector(Front, Long, WallTop));
		Add(Out, MarketModel::Cube, Stone, FVector(Front * 0.5 + 4, Mid, BaseTop * 0.5), FVector(Front + 8, Long, BaseTop), 0.0, false);
		// The joints of the stone ground floor.
		for (double Z = 90.0; Z < BaseTop; Z += 90.0)
		{
			Add(Out, MarketModel::Cube, Plaster, FVector(Front + 8, Mid, Z), FVector(3, Long, 4), 0.0, false);
		}
		Add(Out, MarketModel::Cube, Trim, FVector(Front + 10, Mid, BaseTop), FVector(20, Long, 22), 0.0, false);
		Add(Out, MarketModel::Cube, Trim, FVector(Front + 6, Mid, 3 * StoreyHeight), FVector(12, Long, 14), 0.0, false);
		Add(Out, MarketModel::Cube, Tile, FVector(Front + 3, Mid, WallTop - 70), FVector(6, Long, 40), 0.0, false);
		Add(Out, MarketModel::Cube, Trim, FVector(Front + 14, Mid, WallTop - 20), FVector(40, Long + 30, 40), 0.0, false);
		Add(Out, MarketModel::Cube, Plaster, FVector(Front * 0.5, Mid, WallTop + 40), FVector(Front, Long, 80));
		Add(Out, MarketModel::Cube, Trim, FVector(Front * 0.5 + 4, Mid, WallTop + 84), FVector(Front + 16, Long + 10, 8), 0.0, false);
	}

	/** The windows of a plain stretch from Y0 to Y1: windows in bays, a flat pilaster every third bay. */
	inline void Bays(TArray<FShape>& Out, double Y0, double Y1)
	{
		using namespace Colors;
		const double Long = Y1 - Y0;
		if (Long < 250.0)
		{
			return;
		}
		const int32 Count = FMath::Max(1, FMath::RoundToInt(Long / BayWidth));
		const double Bay = Long / Count;
		const double PilasterZ = (BaseTop + WallTop - 40.0) * 0.5;
		for (int32 k = 0; k <= Count; k += 3)
		{
			Add(Out, MarketModel::Cube, Plaster, FVector(Skin + 8, Y0 + k * Bay, PilasterZ), FVector(16, 50, WallTop - 40.0 - BaseTop), 0.0, false);
		}
		for (int32 k = 0; k < Count; ++k)
		{
			BayWindows(Out, Y0 + (k + 0.5) * Bay, Skin, FMath::Min(120.0, Bay - 120.0));
		}
	}

	/** One plain run. */
	inline void Run(double Length, TArray<FShape>& Out)
	{
		Wall(Out, 0.0, Length, Skin);
		Bays(Out, 0.0, Length);
	}

	/** A rounded gable over the wall from Y0 to Y1, its face at Front. */
	inline void RoundGable(TArray<FShape>& Out, double Y0, double Y1, double Front)
	{
		using namespace Colors;
		const double Mid = (Y0 + Y1) * 0.5;
		const double Wide = Y1 - Y0 - 80.0;
		Add(Out, MarketModel::Cylinder, Plaster, FVector(Front - 20, Mid, WallTop + 80), FVector(Wide * 0.8, Wide, 40), 0.0, false, 90.0);
		Add(Out, MarketModel::Cylinder, Trim, FVector(Front - 22, Mid, WallTop + 80), FVector(Wide * 0.8 + 30, Wide + 30, 36), 0.0, false, 90.0);
		Add(Out, MarketModel::Cylinder, Tile, FVector(Front + 1, Mid, WallTop + 140), FVector(Wide * 0.35, Wide * 0.35, 4), 0.0, false, 90.0);
		Window(Out, Mid, WallTop + 100, 90.0, 120.0, Front, true);
	}

	/** An end wing standing a little forward, from Y0 to Y1: quoin strips, a rounded gable. */
	inline void Wing(TArray<FShape>& Out, double Y0, double Y1)
	{
		using namespace Colors;
		const double Front = Skin + WingOut;
		Wall(Out, Y0, Y1, Front);
		for (const double Y : { Y0 + 30.0, Y1 - 30.0 })
		{
			Add(Out, MarketModel::Cube, Trim, FVector(Front + 4, Y, (BaseTop + WallTop) * 0.5), FVector(8, 60, WallTop - BaseTop - 40), 0.0, false);
		}
		const int32 WingBays = 3;
		const double Bay = (Y1 - Y0 - 120.0) / WingBays;
		for (int32 k = 0; k < WingBays; ++k)
		{
			BayWindows(Out, Y0 + 60.0 + (k + 0.5) * Bay, Front, 110.0);
		}
		RoundGable(Out, Y0 + 100.0, Y1 - 100.0, Front);
	}

	/** The clock on the tower's face at X, height Z. */
	inline void Clock(TArray<FShape>& Out, double X, double Y, double Z)
	{
		using namespace Colors;
		Add(Out, MarketModel::Cylinder, Gilt, FVector(X + 2, Y, Z), FVector(330, 330, 6), 0.0, false, 90.0);
		Add(Out, MarketModel::Cylinder, Face, FVector(X + 5, Y, Z), FVector(300, 300, 6), 0.0, false, 90.0);
		for (int32 Hour = 0; Hour < 12; ++Hour)
		{
			const double Angle = FMath::DegreesToRadians(Hour * 30.0);
			Add(Out, MarketModel::Cube, Hand, FVector(X + 9, Y + FMath::Sin(Angle) * 130.0, Z + FMath::Cos(Angle) * 130.0), FVector(2, 10, 10), 0.0, false);
		}
		// Ten past ten.
		Add(Out, MarketModel::Cube, Hand, FVector(X + 10, Y - 30, Z + 18), FVector(2, 10, 80), 0.0, false, 0.0, -60.0);
		Add(Out, MarketModel::Cube, Hand, FVector(X + 11, Y + 50, Z + 30), FVector(2, 8, 120), 0.0, false, 0.0, 60.0);
	}

	/** The middle: forward-standing, the arched gate with the name over it, three tall bays, the clock tower. */
	inline void Middle(double Mid, TArray<FShape>& Out, TArray<FLamp>& Lamps, TArray<FSign>& Signs)
	{
		using namespace Colors;
		const double Y0 = Mid - MiddleWidth * 0.5;
		const double Y1 = Mid + MiddleWidth * 0.5;
		const double Front = Skin + MiddleOut;
		Wall(Out, Y0, Y1, Front);
		for (const double Y : { Y0 + 40.0, Y1 - 40.0, Mid - 380.0, Mid + 380.0 })
		{
			Add(Out, MarketModel::Cube, Trim, FVector(Front + 6, Y, (BaseTop + WallTop) * 0.5), FVector(12, 50, WallTop - BaseTop - 40), 0.0, false);
		}
		// The gate: a wide round arch in stone, the door, two steps.
		const double GateW = 360.0;
		Add(Out, MarketModel::Cube, Stone, FVector(Front + 12, Mid, 190), FVector(24, GateW + 100, 380), 0.0, false);
		Add(Out, MarketModel::Cylinder, Stone, FVector(Front + 12, Mid, 380), FVector(GateW + 100, GateW + 100, 24), 0.0, false, 90.0);
		Add(Out, MarketModel::Cube, Door, FVector(Front + 26, Mid, 180), FVector(4, GateW, 360), 0.0, false);
		Add(Out, MarketModel::Cylinder, Glass, FVector(Front + 24, Mid, 360), FVector(GateW, GateW, 4), 0.0, false, 90.0);
		for (int32 Step = 0; Step < 2; ++Step)
		{
			Add(Out, MarketModel::Cube, Stone, FVector(Front + 40 + Step * 30, Mid, 8 + (1 - Step) * 16), FVector(60, GateW + 120, 16));
		}
		// The name on a dark plate over the gate.
		Add(Out, MarketModel::Cube, Plate, FVector(Front + 22, Mid, 665), FVector(4, 1100, 70), 0.0, false);
		Signs.Add({ TEXT("ORSZ\u00C1GOS T\u00C1RSADALOMBIZTOS\u00CDT\u00D3 INT\u00C9ZET"), FVector(Front + 25, Mid, 655), 0.0, 26.0, Gilt });
		// Side bays of the ground floor and the three tall bays above.
		for (const double Y : { Mid - 700.0, Mid + 700.0 })
		{
			Window(Out, Y, 110.0, 120.0, 190.0, Front + 8.0, true);
		}
		for (const double Y : { Mid - 700.0, Mid - 190.0, Mid + 190.0, Mid + 700.0 })
		{
			for (int32 Floor = 1; Floor < 4; ++Floor)
			{
				const bool bTall = Floor == 2 && FMath::Abs(Y - Mid) < 300.0;
				if (Floor == 1 && FMath::Abs(Y - Mid) < 300.0)
				{
					continue;
				}
				Window(Out, Y, bTall ? 740.0 : Floor * StoreyHeight + 80.0, 130.0, bTall ? 320.0 : (Floor == 3 ? 200.0 : 230.0), Front, bTall);
			}
		}
		// The tower, standing on the middle's wall, a clock on each of its three outer faces.
		const double TowerX = Front - TowerSide * 0.5;
		Add(Out, MarketModel::Cube, Plaster, FVector(TowerX, Mid, (WallTop + TowerTop) * 0.5), FVector(TowerSide, TowerSide, TowerTop - WallTop));
		Add(Out, MarketModel::Cube, Tile, FVector(TowerX, Mid, TowerTop - 60), FVector(TowerSide + 6, TowerSide + 6, 40), 0.0, false);
		Add(Out, MarketModel::Cube, Trim, FVector(TowerX, Mid, TowerTop - 15), FVector(TowerSide + 40, TowerSide + 40, 30), 0.0, false);
		for (const double Y : { Mid - TowerSide * 0.5 + 40.0, Mid + TowerSide * 0.5 - 40.0 })
		{
			Add(Out, MarketModel::Cube, Trim, FVector(Front + 3, Y, (WallTop + TowerTop) * 0.5), FVector(6, 40, TowerTop - WallTop), 0.0, false);
		}
		Clock(Out, Front, Mid, WallTop + 600.0);
		Window(Out, Mid, WallTop + 140.0, 70.0, 170.0, Front, true);
		// The copper spire: a steep cap, a little lantern, a gilt ball on a spike.
		Add(Out, MarketModel::Cone, Copper, FVector(TowerX, Mid, TowerTop + 450), FVector(TowerSide + 60, TowerSide + 60, 900), 0.0, false);
		Add(Out, MarketModel::Cylinder, Copper, FVector(TowerX, Mid, TowerTop + 950), FVector(160, 160, 200), 0.0, false);
		Add(Out, MarketModel::Cone, Copper, FVector(TowerX, Mid, TowerTop + 1200), FVector(200, 200, 300), 0.0, false);
		Add(Out, MarketModel::Sphere, Gilt, FVector(TowerX, Mid, TowerTop + 1380), FVector(50, 50, 50), 0.0, false);
		Add(Out, MarketModel::Cylinder, Hand, FVector(TowerX, Mid, TowerTop + 1460), FVector(8, 8, 160), 0.0, false);
		// Lamps on iron arms either side of the gate.
		for (const double Y : { Mid - GateW * 0.5 - 120.0, Mid + GateW * 0.5 + 120.0 })
		{
			Add(Out, MarketModel::Cube, CityDetailsModel::Colors::Iron, FVector(Front + 30, Y, 330), FVector(60, 6, 6), 0.0, false);
			Add(Out, MarketModel::Cube, CityDetailsModel::Colors::Iron, FVector(Front + 60, Y, 305), FVector(26, 26, 44), 0.0, false);
			Lamps.Add({ FVector(Front + 60, Y, 300), 600.f });
		}
	}

	/** The long front on Fiumei út: the plain run, the two end wings and the middle with the tower. */
	inline void MainFront(double Length, TArray<FShape>& Out, TArray<FLamp>& Lamps, TArray<FSign>& Signs)
	{
		if (Length < MiddleWidth + 2.0 * WingWidth + 1000.0)
		{
			Run(Length, Out);
			return;
		}
		// The plain stretches get their windows only between the wings and the middle, so none is cut by them.
		Wall(Out, 0.0, Length, Skin);
		Bays(Out, WingWidth, (Length - MiddleWidth) * 0.5);
		Bays(Out, (Length + MiddleWidth) * 0.5, Length - WingWidth);
		Wing(Out, 0.0, WingWidth);
		Wing(Out, Length - WingWidth, Length);
		Middle(Length * 0.5, Out, Lamps, Signs);
	}
}
