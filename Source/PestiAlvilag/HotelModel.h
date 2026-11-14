// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CityDetailsModel.h"

/**
 * The two grand hotels by Blaha Lujza tér: the Nemzeti Szálloda on the corner
 * of József körút, by the Népszínház, and the Metropol on Rákóczi út. Rich
 * plastered fronts of the 1890s-1910s: a rusticated ground floor with tall
 * shop and café windows, windows with pediments and balconies with iron
 * rails on the floors above, a heavy cornice; on the main front the hotel
 * door under a glass canopy with lamps, and the hotel's name on the attic;
 * the Nemzeti's cut corner carries a small dome. The detail is simplified;
 * ACityDetails dresses each street wall of the hotel's map block with one run.
 *
 * Header-only so it can be built and looked at outside the engine. A run's
 * frame: its start at the foot, +X out to the street, the wall along +Y for
 * Length, up to Height.
 */
namespace HotelModel
{
	using MarketModel::FShape;
	using BozsiHouseModel::FSign;
	using BozsiHouseModel::FLamp;
	using BozsiHouseModel::Add;

	enum class EHotel : uint8 { Nemzeti, Metropol };
	enum class EPart : uint8 { Plain, Main, Corner };

	namespace Colors
	{
		const FLinearColor NemzetiWall(0.72f, 0.60f, 0.42f);
		const FLinearColor MetropolWall(0.66f, 0.65f, 0.60f);
		const FLinearColor Base(0.48f, 0.45f, 0.40f);
		const FLinearColor Trim(0.85f, 0.81f, 0.70f);
		const FLinearColor Glass(0.06f, 0.07f, 0.09f);
		const FLinearColor ShopGlass(0.10f, 0.12f, 0.13f);
		const FLinearColor Door(0.24f, 0.13f, 0.07f);
		const FLinearColor Gilt(0.85f, 0.64f, 0.24f);
		const FLinearColor Dome(0.24f, 0.36f, 0.32f);
		const FLinearColor Canopy(0.30f, 0.33f, 0.35f);
		const FLinearColor Plate(0.10f, 0.12f, 0.11f);
	}

	// Sizes (cm).
	constexpr double StoreyHeight = 380.0;
	constexpr double GroundTop = 460.0;
	constexpr double BayWidth = 340.0;
	constexpr double Skin = 14.0;

	inline const FLinearColor& WallColor(EHotel Which)
	{
		return Which == EHotel::Nemzeti ? Colors::NemzetiWall : Colors::MetropolWall;
	}

	/** Upper storeys above the ground floor that fit under Height. */
	inline int32 UpperFloors(double Height)
	{
		return FMath::Max(1, FMath::FloorToInt((Height - GroundTop - 60.0) / StoreyHeight));
	}

	/** A window at Y on an upper floor, its sill at Z: frame, glass, sill, a pediment or a straight hood. */
	inline void Window(TArray<FShape>& Out, double Y, double Z, bool bPediment)
	{
		using namespace Colors;
		Add(Out, MarketModel::Cube, Trim, FVector(Skin + 3, Y, Z + 115), FVector(6, 130, 240), 0.0, false);
		Add(Out, MarketModel::Cube, Glass, FVector(Skin + 6, Y, Z + 113), FVector(4, 100, 216), 0.0, false);
		Add(Out, MarketModel::Cube, Trim, FVector(Skin + 9, Y, Z - 6), FVector(18, 150, 12), 0.0, false);
		if (bPediment)
		{
			// A low triangle of two tilted slabs.
			for (const double Side : { -1.0, 1.0 })
			{
				Add(Out, MarketModel::Cube, Trim, FVector(Skin + 10, Y + Side * 40.0, Z + 262), FVector(20, 92, 10), 0.0, false, 0.0, Side * 18.0);
			}
		}
		else
		{
			Add(Out, MarketModel::Cube, Trim, FVector(Skin + 10, Y, Z + 248), FVector(20, 156, 14), 0.0, false);
		}
	}

	/** An iron-railed balcony on a stone slab at Y, floor Z, Wide across. */
	inline void Balcony(TArray<FShape>& Out, double Y, double Z, double Wide)
	{
		using namespace Colors;
		Add(Out, MarketModel::Cube, Trim, FVector(Skin + 50, Y, Z - 8), FVector(100, Wide, 16));
		Add(Out, MarketModel::Cube, CityDetailsModel::Colors::Iron, FVector(Skin + 98, Y, Z + 90), FVector(3, Wide, 4), 0.0, false);
		Add(Out, MarketModel::Cube, CityDetailsModel::Colors::Iron, FVector(Skin + 98, Y, Z + 20), FVector(3, Wide, 3), 0.0, false);
		for (double Bar = -Wide * 0.5 + 6.0; Bar <= Wide * 0.5 - 6.0; Bar += 16.0)
		{
			Add(Out, MarketModel::Cube, CityDetailsModel::Colors::Iron, FVector(Skin + 98, Y + Bar, Z + 50), FVector(2, 2, 80), 0.0, false);
		}
		for (const double Side : { -1.0, 1.0 })
		{
			Add(Out, MarketModel::Cube, CityDetailsModel::Colors::Iron, FVector(Skin + 50, Y + Side * (Wide * 0.5 - 2.0), Z + 90), FVector(100, 3, 4), 0.0, false);
		}
	}

	/** The wall from Y0 to Y1: plaster, the rusticated ground floor, string courses, the cornice and a low attic. */
	inline void Wall(TArray<FShape>& Out, EHotel Which, double Y0, double Y1, double Height)
	{
		using namespace Colors;
		const double Mid = (Y0 + Y1) * 0.5;
		const double Long = Y1 - Y0;
		Add(Out, MarketModel::Cube, WallColor(Which), FVector(Skin * 0.5, Mid, Height * 0.5), FVector(Skin, Long, Height));
		Add(Out, MarketModel::Cube, Base, FVector(Skin * 0.5 + 4, Mid, GroundTop * 0.5), FVector(Skin + 8, Long, GroundTop), 0.0, false);
		for (double Z = 100.0; Z < GroundTop; Z += 100.0)
		{
			Add(Out, MarketModel::Cube, WallColor(Which), FVector(Skin + 8, Mid, Z), FVector(3, Long, 5), 0.0, false);
		}
		Add(Out, MarketModel::Cube, Trim, FVector(Skin + 12, Mid, GroundTop), FVector(24, Long, 26), 0.0, false);
		Add(Out, MarketModel::Cube, Trim, FVector(Skin + 6, Mid, GroundTop + StoreyHeight - 20), FVector(12, Long, 14), 0.0, false);
		Add(Out, MarketModel::Cube, Trim, FVector(Skin + 20, Mid, Height - 25), FVector(52, Long + 40, 50), 0.0, false);
		Add(Out, MarketModel::Cube, WallColor(Which), FVector(Skin * 0.5, Mid, Height + 45), FVector(Skin, Long, 90));
		Add(Out, MarketModel::Cube, Trim, FVector(Skin * 0.5 + 4, Mid, Height + 94), FVector(Skin + 16, Long + 10, 8), 0.0, false);
	}

	/** Windows, balconies and ground-floor glass for the bays between Y0 and Y1. */
	inline void Bays(TArray<FShape>& Out, double Y0, double Y1, double Height)
	{
		using namespace Colors;
		const double Long = Y1 - Y0;
		if (Long < 250.0)
		{
			return;
		}
		const int32 Count = FMath::Max(1, FMath::RoundToInt(Long / BayWidth));
		const double Bay = Long / Count;
		const int32 Floors = UpperFloors(Height);
		for (int32 k = 0; k < Count; ++k)
		{
			const double Y = Y0 + (k + 0.5) * Bay;
			// A tall shop or café window in a stone frame.
			Add(Out, MarketModel::Cube, Trim, FVector(Skin + 10, Y, 200), FVector(6, Bay - 70, 330), 0.0, false);
			Add(Out, MarketModel::Cube, ShopGlass, FVector(Skin + 13, Y, 198), FVector(4, Bay - 100, 300), 0.0, false);
			for (int32 Floor = 0; Floor < Floors; ++Floor)
			{
				const double Z = GroundTop + Floor * StoreyHeight + 70.0;
				Window(Out, Y, Z, Floor == 0);
				if (Floor == 0 && k % 2 == 0)
				{
					Balcony(Out, Y, Z - 10.0, 190.0);
				}
			}
		}
	}

	/** One plain run. */
	inline void Run(EHotel Which, double Length, double Height, TArray<FShape>& Out)
	{
		Wall(Out, Which, 0.0, Length, Height);
		Bays(Out, 0.0, Length, Height);
	}

	/** The hotel's name for its attic plate. */
	inline FString Name(EHotel Which)
	{
		return Which == EHotel::Nemzeti ? FString(TEXT("NEMZETI SZ\u00C1LLODA")) : FString(TEXT("METROPOL SZ\u00C1LL\u00D3"));
	}

	/** The main front: the run with the hotel door in the middle under a glass canopy, a long balcony over it, the name on a raised attic. */
	inline void MainFront(EHotel Which, double Length, double Height, TArray<FShape>& Out, TArray<FLamp>& Lamps, TArray<FSign>& Signs)
	{
		using namespace Colors;
		const double Mid = Length * 0.5;
		const double DoorBay = 520.0;
		Wall(Out, Which, 0.0, Length, Height);
		Bays(Out, 0.0, Mid - DoorBay * 0.5, Height);
		Bays(Out, Mid + DoorBay * 0.5, Length, Height);
		// The door: a stone surround, a glazed double door, a fanlight.
		Add(Out, MarketModel::Cube, Trim, FVector(Skin + 12, Mid, 210), FVector(20, 300, 420), 0.0, false);
		Add(Out, MarketModel::Cube, Door, FVector(Skin + 23, Mid, 150), FVector(4, 220, 300), 0.0, false);
		Add(Out, MarketModel::Cube, Glass, FVector(Skin + 26, Mid, 175), FVector(3, 160, 200), 0.0, false);
		Add(Out, MarketModel::Cube, Glass, FVector(Skin + 23, Mid, 350), FVector(4, 220, 80), 0.0, false);
		Add(Out, MarketModel::Cube, Trim, FVector(Skin + 40, Mid, 12), FVector(60, 320, 24));
		// The glass canopy on iron arms, a lamp each side.
		Add(Out, MarketModel::Cube, Canopy, FVector(Skin + 160, Mid, 410), FVector(300, 420, 8), 0.0, false, 6.0);
		Add(Out, MarketModel::Cube, CityDetailsModel::Colors::Iron, FVector(Skin + 160, Mid, 404), FVector(304, 424, 4), 0.0, false, 6.0);
		for (const double Y : { Mid - 190.0, Mid + 190.0 })
		{
			Add(Out, MarketModel::Cube, CityDetailsModel::Colors::Iron, FVector(Skin + 150, Y, 450), FVector(290, 5, 5), 0.0, false, -20.0);
			Add(Out, MarketModel::Cube, CityDetailsModel::Colors::Iron, FVector(Skin + 300, Y, 380), FVector(28, 28, 44), 0.0, false);
			Lamps.Add({ FVector(Skin + 300, Y, 375), 650.f });
		}
		// Upper floors of the door bay: a long balcony and twin windows.
		const int32 Floors = UpperFloors(Height);
		Balcony(Out, Mid, GroundTop + 60.0, DoorBay - 40.0);
		for (int32 Floor = 0; Floor < Floors; ++Floor)
		{
			for (const double Y : { Mid - 120.0, Mid + 120.0 })
			{
				Window(Out, Y, GroundTop + Floor * StoreyHeight + 70.0, Floor % 2 == 0);
			}
		}
		// The raised attic with the name, and a gilt crest on it.
		Add(Out, MarketModel::Cube, WallColor(Which), FVector(Skin * 0.5 + 10, Mid, Height + 150), FVector(Skin + 20, 1400, 300));
		Add(Out, MarketModel::Cube, Trim, FVector(Skin * 0.5 + 16, Mid, Height + 305), FVector(Skin + 32, 1430, 14), 0.0, false);
		Add(Out, MarketModel::Cylinder, Trim, FVector(Skin * 0.5 + 10, Mid, Height + 300), FVector(400, 400, Skin + 20), 0.0, false, 90.0);
		Add(Out, MarketModel::Sphere, Gilt, FVector(Skin + 20, Mid, Height + 430), FVector(50, 50, 50), 0.0, false);
		Add(Out, MarketModel::Cube, Plate, FVector(Skin + 22, Mid, Height + 150), FVector(4, 1250, 110), 0.0, false);
		Signs.Add({ Name(Which), FVector(Skin + 26, Mid, Height + 140), 0.0, 60.0, Gilt });
	}

	/** The cut corner: balconies on every floor, and for the Nemzeti a small dome with a lantern. */
	inline void Corner(EHotel Which, double Length, double Height, TArray<FShape>& Out)
	{
		using namespace Colors;
		const double Mid = Length * 0.5;
		Wall(Out, Which, 0.0, Length, Height);
		Add(Out, MarketModel::Cube, Door, FVector(Skin + 12, Mid, 150), FVector(4, FMath::Min(160.0, Length - 60.0), 300), 0.0, false);
		const int32 Floors = UpperFloors(Height);
		for (int32 Floor = 0; Floor < Floors; ++Floor)
		{
			const double Z = GroundTop + Floor * StoreyHeight + 70.0;
			Window(Out, Mid, Z, true);
			Balcony(Out, Mid, Z - 10.0, FMath::Min(220.0, Length - 20.0));
		}
		if (Which == EHotel::Nemzeti)
		{
			const double Round = FMath::Max(Length, 300.0);
			Add(Out, MarketModel::Cylinder, WallColor(Which), FVector(-Round * 0.5 + Skin, Mid, Height + 180), FVector(Round, Round, 360));
			Add(Out, MarketModel::Cylinder, Trim, FVector(-Round * 0.5 + Skin, Mid, Height + 365), FVector(Round + 30, Round + 30, 16), 0.0, false);
			Add(Out, MarketModel::Sphere, Dome, FVector(-Round * 0.5 + Skin, Mid, Height + 370), FVector(Round, Round, Round * 0.9), 0.0, false);
			Add(Out, MarketModel::Cylinder, Dome, FVector(-Round * 0.5 + Skin, Mid, Height + 370 + Round * 0.45 + 40), FVector(70, 70, 90), 0.0, false);
			Add(Out, MarketModel::Sphere, Gilt, FVector(-Round * 0.5 + Skin, Mid, Height + 370 + Round * 0.45 + 110), FVector(36, 36, 36), 0.0, false);
		}
	}
}
