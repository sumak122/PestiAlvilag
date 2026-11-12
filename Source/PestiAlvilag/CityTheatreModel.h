// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CityDetailsModel.h"

/**
 * The Városi Színház (city theatre) on Tisza Kálmán tér, built in nine
 * months in 1911 as the Népopera (Komor, Jakab and Márkus) and called the
 * Városi Színház from 1917; today's Erkel Színház. The towers and domes of
 * its first plans were never built for want of money, so it went up as a
 * plain, big house: here a cream plastered front on the square, lower wings
 * either side, and the taller middle with five doors under a glass canopy,
 * three tall arched foyer windows, the gilt name and a round gable between
 * two short pylons with flags. Playbills on the wings.
 *
 * ACityDetails puts it on the map block's straight front (city_details.py
 * "city_theatre"). Its frame: the front's start at the foot, +X out to the
 * square, Y along the front from 0 to Length, Z up from the pavement.
 *
 * Header-only so it can be built and looked at outside the engine.
 */
namespace CityTheatreModel
{
	using MarketModel::FShape;
	using BozsiHouseModel::FSign;
	using BozsiHouseModel::FLamp;
	using BozsiHouseModel::Add;
	using BozsiHouseModel::Span;
	using CityDetailsModel::Rod;

	namespace Colors
	{
		const FLinearColor Plaster(0.66f, 0.59f, 0.45f);
		const FLinearColor PlasterDark(0.52f, 0.46f, 0.35f);
		const FLinearColor Trim(0.76f, 0.71f, 0.60f);
		const FLinearColor Plinth(0.22f, 0.20f, 0.18f);
		const FLinearColor Glass(0.05f, 0.06f, 0.07f);
		const FLinearColor FoyerGlass(0.38f, 0.29f, 0.15f);   // the lit foyer
		const FLinearColor Door(0.12f, 0.17f, 0.12f);
		const FLinearColor Iron(0.035f, 0.04f, 0.04f);
		const FLinearColor Gilt(0.85f, 0.64f, 0.24f);
		const FLinearColor Copper(0.20f, 0.36f, 0.30f);
		const FLinearColor Globe(0.95f, 0.86f, 0.60f);
		const FLinearColor Paper(0.86f, 0.82f, 0.68f);
		const FLinearColor PaperRed(0.62f, 0.14f, 0.10f);
		const FLinearColor Ink(0.08f, 0.07f, 0.06f);
		const FLinearColor FlagRed(0.55f, 0.04f, 0.04f);
		const FLinearColor FlagWhite(0.85f, 0.85f, 0.82f);
		const FLinearColor FlagGreen(0.05f, 0.30f, 0.10f);
	}

	// Sizes (cm).
	constexpr double WingWidth = 1150.0;
	constexpr double WingTop = 1520.0;         // the map block's height
	constexpr double MiddleTop = 2050.0;       // the middle's cornice
	constexpr double GableRise = 380.0;
	constexpr double PylonTop = 2500.0;
	constexpr double Forward = 100.0;          // how far the middle stands out
	constexpr double MiddleBack = -900.0;      // and how deep it runs into the block
	constexpr double PlinthTop = 50.0;
	constexpr double CanopyAt = 400.0;
	constexpr double CanopyDepth = 260.0;

	/** A tall window with a round top on the wall at X: Width wide, glass from Z0 up to Spring, then the half round. */
	inline void ArchWindow(TArray<FShape>& Out, double X, double Y, double Width, double Z0, double Spring, const FLinearColor& Frame, const FLinearColor& Pane)
	{
		Span(Out, Frame, FVector(X, Y - Width * 0.5 - 20, Z0 - 12), FVector(X + 6, Y + Width * 0.5 + 20, Spring), false);
		Add(Out, MarketModel::Cylinder, Frame, FVector(X + 3, Y, Spring), FVector(Width + 40, Width + 40, 6), 0.0, false, 90.0);
		Span(Out, Pane, FVector(X + 6, Y - Width * 0.5, Z0), FVector(X + 9, Y + Width * 0.5, Spring), false);
		Add(Out, MarketModel::Cylinder, Pane, FVector(X + 6.5, Y, Spring), FVector(Width, Width, 3), 0.0, false, 90.0);
	}

	/** A plain window in a flat frame with a sill. */
	inline void Window(TArray<FShape>& Out, double X, double Y, double Width, double Z0, double Height)
	{
		using namespace Colors;
		Span(Out, Trim, FVector(X, Y - Width * 0.5 - 14, Z0 - 8), FVector(X + 6, Y + Width * 0.5 + 14, Z0 + Height + 14), false);
		Span(Out, Glass, FVector(X + 6, Y - Width * 0.5, Z0), FVector(X + 9, Y + Width * 0.5, Z0 + Height), false);
		Span(Out, Trim, FVector(X + 8, Y - 3, Z0), FVector(X + 10, Y + 3, Z0 + Height), false);
		Span(Out, Trim, FVector(X, Y - Width * 0.5 - 22, Z0 - 20), FVector(X + 16, Y + Width * 0.5 + 22, Z0 - 8), false);
	}

	/** A playbill board on the wall at Y: a framed paper with the evening's show. */
	inline void Playbill(TArray<FShape>& Out, TArray<FSign>& Signs, double X, double Y, bool bRed)
	{
		using namespace Colors;
		Span(Out, Iron, FVector(X, Y - 75, 110), FVector(X + 6, Y + 75, 330), false);
		Span(Out, bRed ? PaperRed : Paper, FVector(X + 6, Y - 68, 117), FVector(X + 8, Y + 68, 323), false);
		const FLinearColor& Letters = bRed ? Paper : Ink;
		Signs.Add({ TEXT("VÁROSI SZÍNHÁZ"), FVector(X + 9, Y, 305), 0.0, 11.0, Letters });
		Signs.Add({ TEXT("MA ESTE"), FVector(X + 9, Y, 265), 0.0, 22.0, Letters });
		Signs.Add({ bRed ? TEXT("OPERETT") : TEXT("OPERA"), FVector(X + 9, Y, 220), 0.0, 26.0, Letters });
		Signs.Add({ TEXT("8 ÓRAKOR"), FVector(X + 9, Y, 175), 0.0, 16.0, Letters });
	}

	/** One of the lower wings, from Y0 to Y1 along the front. */
	inline void Wing(TArray<FShape>& Out, TArray<FSign>& Signs, double Y0, double Y1, bool bPlaybills)
	{
		using namespace Colors;
		Span(Out, Plaster, FVector(0, Y0, 0), FVector(4, Y1, WingTop), false);
		Span(Out, Plinth, FVector(0, Y0, 0), FVector(24, Y1, PlinthTop));
		// Banded ground floor, a string course, the cornice.
		for (int32 Band = 0; Band < 5; ++Band)
		{
			const double Z0 = PlinthTop + Band * 90.0;
			Span(Out, Band % 2 ? Plaster : PlasterDark, FVector(0, Y0, Z0), FVector(10, Y1, Z0 + 84), false);
		}
		Span(Out, Trim, FVector(0, Y0, 500), FVector(26, Y1, 530), false);
		Span(Out, Trim, FVector(0, Y0 - 10, WingTop - 70), FVector(50, Y1 + 10, WingTop - 30), false);
		Span(Out, PlasterDark, FVector(0, Y0, WingTop - 30), FVector(20, Y1, WingTop + 30), false);
		const int32 Bays = FMath::Max(2, FMath::RoundToInt((Y1 - Y0) / 330.0));
		const double Bay = (Y1 - Y0) / Bays;
		for (int32 k = 0; k < Bays; ++k)
		{
			const double Y = Y0 + Bay * (k + 0.5);
			if (bPlaybills && k % 2 == 1)
			{
				Playbill(Out, Signs, 10, Y, k % 4 == 1);
			}
			else
			{
				Window(Out, 10, Y, 120, 140, 260);
			}
			Window(Out, 4, Y, 130, 640, 280);
			Window(Out, 4, Y, 130, 1040, 260);
		}
	}

	/** The theatre's front, Length long. */
	inline void Theatre(double Length, TArray<FShape>& Out, TArray<FLamp>& Lamps, TArray<FSign>& Signs)
	{
		using namespace Colors;
		const double M0 = WingWidth;
		const double M1 = Length - WingWidth;
		const double Mid = Length * 0.5;
		const double X = Forward;

		Wing(Out, Signs, 0.0, M0, true);
		Wing(Out, Signs, M1, Length, true);

		// The middle: a solid block standing out of the wings and above them.
		Span(Out, Plaster, FVector(MiddleBack, M0, 0), FVector(X, M1, MiddleTop));
		Span(Out, Plinth, FVector(0, M0 - 4, 0), FVector(X + 24, M1 + 4, PlinthTop));
		// Five doors in a row, each with a fanlight, two steps up to them.
		Span(Out, PlasterDark, FVector(X, M0 + 80, PlinthTop), FVector(X + 8, M1 - 80, CanopyAt), false);
		Span(Out, Trim, FVector(X, M0 + 60, 0), FVector(X + 70, M1 - 60, 16));
		Span(Out, Trim, FVector(X, M0 + 80, 16), FVector(X + 35, M1 - 80, 32));
		constexpr int32 Doors = 5;
		const double DoorBay = (M1 - M0 - 200.0) / Doors;
		for (int32 k = 0; k < Doors; ++k)
		{
			const double Y = M0 + 100.0 + DoorBay * (k + 0.5);
			Span(Out, Trim, FVector(X + 8, Y - 115, 32), FVector(X + 14, Y + 115, 370), false);
			for (const double S : { -1.0, 1.0 })
			{
				Span(Out, Door, FVector(X + 14, Y + S * 52 - 50, 32), FVector(X + 18, Y + S * 52 + 50, 290), false);
				Span(Out, FoyerGlass, FVector(X + 18, Y + S * 52 - 34, 140), FVector(X + 20, Y + S * 52 + 34, 270), false);
			}
			Span(Out, FoyerGlass, FVector(X + 14, Y - 100, 300), FVector(X + 18, Y + 100, 360), false);
			Span(Out, Gilt, FVector(X + 18, Y - 2, 300), FVector(X + 20, Y + 2, 360), false);
		}

		// The glass canopy on iron arms over all the doors, lamps under it.
		Span(Out, Iron, FVector(X, M0 + 60, CanopyAt), FVector(X + CanopyDepth, M1 - 60, CanopyAt + 14), false);
		Span(Out, FoyerGlass, FVector(X + 10, M0 + 70, CanopyAt + 14), FVector(X + CanopyDepth - 10, M1 - 70, CanopyAt + 18), false);
		Span(Out, Gilt, FVector(X + CanopyDepth - 2, M0 + 60, CanopyAt - 30), FVector(X + CanopyDepth + 2, M1 - 60, CanopyAt + 14), false);
		Signs.Add({ TEXT("VÁROSI SZÍNHÁZ"), FVector(X + CanopyDepth + 3, Mid, CanopyAt - 12), 0.0, 16.0, Gilt });
		for (int32 k = 0; k <= 4; ++k)
		{
			const double Y = M0 + 60.0 + (M1 - M0 - 120.0) * k / 4.0;
			Rod(Out, Iron, FVector(X, Y, CanopyAt + 220), FVector(X + CanopyDepth - 10, Y, CanopyAt + 14), 4.0);
		}
		for (int32 k = 0; k < 3; ++k)
		{
			const double Y = M0 + (M1 - M0) * (k + 1) / 4.0;
			Add(Out, MarketModel::Sphere, Globe, FVector(X + CanopyDepth * 0.5, Y, CanopyAt - 30), FVector(28, 28, 28), 0.0, false);
			Lamps.Add({ FVector(X + CanopyDepth * 0.5, Y, CanopyAt - 60), 700.f });
		}

		// Three tall arched foyer windows, slim gilt-banded pilasters between them.
		const double Third = (M1 - M0) / 3.0;
		for (int32 k = 0; k < 3; ++k)
		{
			ArchWindow(Out, X, M0 + Third * (k + 0.5), Third - 260.0, 520.0, 1380.0, Trim, FoyerGlass);
			Span(Out, Trim, FVector(X + 9, M0 + Third * (k + 0.5) - 2, 520), FVector(X + 12, M0 + Third * (k + 0.5) + 2, 1380), false);
			Span(Out, Trim, FVector(X + 9, M0 + Third * (k + 0.5) - (Third - 260.0) * 0.5, 940), FVector(X + 12, M0 + Third * (k + 0.5) + (Third - 260.0) * 0.5, 948), false);
		}
		for (int32 k = 0; k <= 3; ++k)
		{
			const double Y = M0 + Third * k;
			Span(Out, Trim, FVector(X, Y - 40, CanopyAt + 20), FVector(X + 22, Y + 40, MiddleTop - 220), false);
			for (const double Z : { 900.0, 1300.0, 1700.0 })
			{
				Span(Out, Gilt, FVector(X + 22, Y - 30, Z), FVector(X + 25, Y + 30, Z + 12), false);
			}
		}

		// The name band, the cornice, the round gable with a lyre in a wreath.
		Span(Out, PlasterDark, FVector(X, M0, MiddleTop - 220), FVector(X + 14, M1, MiddleTop - 60), false);
		Signs.Add({ TEXT("VÁROSI SZÍNHÁZ"), FVector(X + 15, Mid, MiddleTop - 140), 0.0, 110.0, Gilt });
		Span(Out, Trim, FVector(MiddleBack, M0 - 10, MiddleTop - 60), FVector(X + 50, M1 + 10, MiddleTop), false);
		Add(Out, MarketModel::Cylinder, Plaster, FVector(X - 10, Mid, MiddleTop), FVector(2.0 * GableRise, M1 - M0 - 200.0, 20), 0.0, false, 90.0);
		Add(Out, MarketModel::Cylinder, Trim, FVector(X - 14, Mid, MiddleTop), FVector(2.0 * GableRise + 40.0, M1 - M0 - 160.0, 20), 0.0, false, 90.0);
		Add(Out, MarketModel::Cylinder, Gilt, FVector(X + 2, Mid, MiddleTop + 180), FVector(170, 170, 6), 0.0, false, 90.0);
		Add(Out, MarketModel::Cylinder, Plaster, FVector(X + 4, Mid, MiddleTop + 180), FVector(140, 140, 6), 0.0, false, 90.0);
		for (const double S : { -1.0, 1.0 })
		{
			Rod(Out, Gilt, FVector(X + 8, Mid + S * 34, MiddleTop + 120), FVector(X + 8, Mid + S * 24, MiddleTop + 240), 6.0);
		}
		Span(Out, Gilt, FVector(X + 6, Mid - 40, MiddleTop + 118), FVector(X + 10, Mid + 40, MiddleTop + 126), false);
		for (int32 k = -2; k <= 2; ++k)
		{
			Span(Out, Gilt, FVector(X + 8, Mid + k * 9.0 - 1, MiddleTop + 126), FVector(X + 10, Mid + k * 9.0 + 1, MiddleTop + 232), false);
		}

		// Two short pylons where the middle meets the wings, each with a copper cap and a flag.
		for (const double Y : { M0 + 90.0, M1 - 90.0 })
		{
			Span(Out, Plaster, FVector(MiddleBack + 300, Y - 110, MiddleTop - 60), FVector(X + 30, Y + 110, PylonTop));
			Span(Out, Trim, FVector(MiddleBack + 290, Y - 120, PylonTop - 40), FVector(X + 40, Y + 120, PylonTop));
			Add(Out, MarketModel::Cone, Copper, FVector((MiddleBack + 300 + X + 30) * 0.5, Y, PylonTop + 80), FVector(X + 30 - MiddleBack - 300 + 20, 240, 160), 0.0, false);
			const FVector Pole((MiddleBack + 300 + X + 30) * 0.5, Y, PylonTop + 150);
			Rod(Out, Iron, Pole, Pole + FVector(0, 0, 500), 7.0);
			Add(Out, MarketModel::Sphere, Gilt, Pole + FVector(0, 0, 505), FVector(14, 14, 14), 0.0, false);
			const FLinearColor Stripes[] = { FlagRed, FlagWhite, FlagGreen };
			for (int32 s = 0; s < 3; ++s)
			{
				Span(Out, Stripes[s], Pole + FVector(-2, 4, 470 - 50 * (s + 1)), Pole + FVector(2, 220, 470 - 50 * s), false);
			}
			// A globe lantern on a bracket by the doors.
			const double LampY = Y < Mid ? M0 + 60.0 : M1 - 60.0;
			Rod(Out, Iron, FVector(X, LampY, 330), FVector(X + 60, LampY, 330), 4.0);
			Add(Out, MarketModel::Sphere, Globe, FVector(X + 60, LampY, 300), FVector(32, 32, 32), 0.0, false);
		}
	}
}
