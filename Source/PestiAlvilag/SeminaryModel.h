// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CityDetailsModel.h"

/**
 * The Országos Rabbiképző Intézet (the Rabbinical Seminary, Freund and
 * Kolbenheyer, 1877) on the corner of Scheiber Sándor utca, a few steps from
 * the attic home: a two-storey building of bare red brick with pale stone
 * trim, round-arched windows in pairs, a band of small arches under the
 * cornice, and on the cut corner the main door under a round arch with a
 * triple window over it and a gabled attic carrying the institute's name.
 * The detail is simplified; ACityDetails dresses each street wall of its
 * map block with one run.
 *
 * Header-only so it can be built and looked at outside the engine. A run's
 * frame: its start at the foot, +X out to the street, the wall along +Y for
 * Length.
 */
namespace SeminaryModel
{
	using MarketModel::FShape;
	using BozsiHouseModel::FSign;
	using BozsiHouseModel::FLamp;
	using BozsiHouseModel::Add;

	namespace Colors
	{
		const FLinearColor Brick(0.42f, 0.20f, 0.13f);
		const FLinearColor Trim(0.72f, 0.64f, 0.48f);
		const FLinearColor Plinth(0.38f, 0.37f, 0.35f);
		const FLinearColor Glass(0.06f, 0.07f, 0.09f);
		const FLinearColor Door(0.20f, 0.11f, 0.06f);
		const FLinearColor Gilt(0.85f, 0.64f, 0.24f);
		const FLinearColor Plate(0.10f, 0.12f, 0.11f);
	}

	// Sizes (cm): the map block is two storeys, 760 high.
	constexpr double WallTop = 760.0;
	constexpr double FloorTwo = 380.0;
	constexpr double BayWidth = 330.0;
	constexpr double Skin = 14.0;

	/** A round-arched window on the wall at Y, its sill at Z. */
	inline void ArchWindow(TArray<FShape>& Out, double Y, double Z, double Width, double Height)
	{
		using namespace Colors;
		Add(Out, MarketModel::Cube, Trim, FVector(Skin + 3, Y, Z + Height * 0.5), FVector(6, Width + 26, Height + 8), 0.0, false);
		Add(Out, MarketModel::Cube, Glass, FVector(Skin + 6, Y, Z + Height * 0.5 - 4), FVector(4, Width, Height - 8), 0.0, false);
		Add(Out, MarketModel::Cylinder, Trim, FVector(Skin + 3, Y, Z + Height), FVector(Width + 26, Width + 26, 6), 0.0, false, 90.0);
		Add(Out, MarketModel::Cylinder, Glass, FVector(Skin + 6, Y, Z + Height - 4), FVector(Width, Width, 4), 0.0, false, 90.0);
		Add(Out, MarketModel::Cube, Trim, FVector(Skin + 8, Y, Z - 6), FVector(16, Width + 40, 12), 0.0, false);
	}

	/** The plain brick skin of a run from Y0 to Y1: plinth, string course, the arch band and cornice. */
	inline void Skin2(TArray<FShape>& Out, double Y0, double Y1)
	{
		using namespace Colors;
		const double Mid = (Y0 + Y1) * 0.5;
		const double Long = Y1 - Y0;
		Add(Out, MarketModel::Cube, Brick, FVector(Skin * 0.5, Mid, WallTop * 0.5), FVector(Skin, Long, WallTop));
		Add(Out, MarketModel::Cube, Plinth, FVector(Skin * 0.5 + 6, Mid, 45), FVector(Skin + 12, Long, 90), 0.0, false);
		Add(Out, MarketModel::Cube, Trim, FVector(Skin + 6, Mid, FloorTwo), FVector(12, Long, 20), 0.0, false);
		// The small arches under the cornice.
		const int32 Arches = FMath::Max(1, FMath::RoundToInt(Long / 60.0));
		for (int32 k = 0; k < Arches; ++k)
		{
			const double AY = Y0 + Long * (k + 0.5) / Arches;
			Add(Out, MarketModel::Cylinder, Trim, FVector(Skin + 3, AY, WallTop - 62), FVector(40, 40, 6), 0.0, false, 90.0);
		}
		Add(Out, MarketModel::Cube, Brick, FVector(Skin + 4, Mid, WallTop - 52), FVector(8, Long, 22), 0.0, false);
		Add(Out, MarketModel::Cube, Trim, FVector(Skin + 10, Mid, WallTop - 14), FVector(30, Long + 20, 28), 0.0, false);
	}

	/** One plain run: windows in pairs between brick pilasters, both floors. */
	inline void Run(double Length, TArray<FShape>& Out)
	{
		using namespace Colors;
		Skin2(Out, 0.0, Length);
		const int32 Bays = FMath::Max(1, FMath::FloorToInt(Length / BayWidth));
		const double Bay = Length / Bays;
		for (int32 k = 0; k <= Bays; ++k)
		{
			Add(Out, MarketModel::Cube, Brick, FVector(Skin + 10, k * Bay, WallTop * 0.5), FVector(20, 46, WallTop - 40), 0.0, false);
		}
		for (int32 k = 0; k < Bays; ++k)
		{
			const double Y = (k + 0.5) * Bay;
			const bool bPair = Bay > 260.0;
			for (const double Z : { 130.0, FloorTwo + 70.0 })
			{
				if (bPair)
				{
					ArchWindow(Out, Y - 48.0, Z, 70.0, 170.0);
					ArchWindow(Out, Y + 48.0, Z, 70.0, 170.0);
				}
				else
				{
					ArchWindow(Out, Y, Z, 80.0, 170.0);
				}
			}
		}
	}

	/** The cut corner with the main door, the triple window, the gabled attic with the name, a lamp each side. */
	inline void Corner(double Length, TArray<FShape>& Out, TArray<FLamp>& Lamps, TArray<FSign>& Signs)
	{
		using namespace Colors;
		const double Mid = Length * 0.5;
		// Stands a little forward of the wall, stone quoins at its sides.
		Add(Out, MarketModel::Cube, Brick, FVector(Skin + 15, Mid, WallTop * 0.5), FVector(Skin + 30, Length, WallTop));
		Add(Out, MarketModel::Cube, Plinth, FVector(Skin + 21, Mid, 45), FVector(Skin + 42, Length, 90), 0.0, false);
		for (const double Y : { 25.0, Length - 25.0 })
		{
			Add(Out, MarketModel::Cube, Trim, FVector(Skin + 32, Y, WallTop * 0.5), FVector(20, 50, WallTop), 0.0, false);
		}
		Add(Out, MarketModel::Cube, Trim, FVector(Skin + 36, Mid, FloorTwo), FVector(12, Length, 20), 0.0, false);
		// The door: a deep round arch in stone, the double door, three steps.
		const double DoorW = FMath::Min(220.0, Length * 0.4);
		Add(Out, MarketModel::Cube, Trim, FVector(Skin + 36, Mid, 160), FVector(16, DoorW + 60, 320), 0.0, false);
		Add(Out, MarketModel::Cylinder, Trim, FVector(Skin + 36, Mid, 320), FVector(DoorW + 60, DoorW + 60, 16), 0.0, false, 90.0);
		Add(Out, MarketModel::Cube, Door, FVector(Skin + 46, Mid, 150), FVector(4, DoorW, 300), 0.0, false);
		Add(Out, MarketModel::Cylinder, Glass, FVector(Skin + 46, Mid, 300), FVector(DoorW, DoorW, 4), 0.0, false, 90.0);
		for (int32 Step = 0; Step < 3; ++Step)
		{
			Add(Out, MarketModel::Cube, Plinth, FVector(Skin + 60 + Step * 25, Mid, 8 + (2 - Step) * 16), FVector(50, DoorW + 80, 16));
		}
		// The triple window over the door.
		for (const double Y : { -110.0, 0.0, 110.0 })
		{
			ArchWindow(Out, Mid + Y, FloorTwo + 60.0, 80.0, Y == 0.0 ? 210.0 : 180.0);
		}
		// The attic: a stepped gable with the name on a dark plate, and pinnacles at its sides.
		const double AtticW = FMath::Min(Length - 80.0, 560.0);
		for (int32 Step = 0; Step < 4; ++Step)
		{
			const double W = AtticW * (1.0 - Step * 0.22);
			Add(Out, MarketModel::Cube, Brick, FVector(Skin + 15, Mid, WallTop + 50 + Step * 90), FVector(Skin + 30, W, 90));
			Add(Out, MarketModel::Cube, Trim, FVector(Skin + 15, Mid, WallTop + 98 + Step * 90), FVector(Skin + 46, W + 16, 10), 0.0, false);
		}
		Add(Out, MarketModel::Cube, Plate, FVector(Skin + 33, Mid, WallTop + 60), FVector(4, AtticW - 60, 70), 0.0, false);
		Signs.Add({ TEXT("ORSZÁGOS RABBIKÉPZŐ INTÉZET"), FVector(Skin + 36, Mid, WallTop + 50), 0.0, 22.0, Gilt });
		for (const double Y : { Mid - AtticW * 0.5, Mid + AtticW * 0.5 })
		{
			Add(Out, MarketModel::Cube, Trim, FVector(Skin + 20, Y, WallTop + 90), FVector(40, 40, 180), 0.0, false);
			Add(Out, MarketModel::Cone, Trim, FVector(Skin + 20, Y, WallTop + 230), FVector(44, 44, 100), 0.0, false);
		}
		Add(Out, MarketModel::Cube, Trim, FVector(Skin + 15, Mid, WallTop + 420), FVector(30, 60, 30), 0.0, false);
		for (const double Y : { Mid - DoorW * 0.5 - 70.0, Mid + DoorW * 0.5 + 70.0 })
		{
			Add(Out, MarketModel::Cube, CityDetailsModel::Colors::Iron, FVector(Skin + 70, Y, 260), FVector(50, 6, 6), 0.0, false);
			Add(Out, MarketModel::Cube, CityDetailsModel::Colors::Iron, FVector(Skin + 95, Y, 240), FVector(24, 24, 40), 0.0, false);
			Lamps.Add({ FVector(Skin + 95, Y, 235), 500.f });
		}
	}
}
