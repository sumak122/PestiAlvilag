// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CityDetailsModel.h"

/**
 * The Kerepesi cemetery (Fiumei úti sírkert) as far as it lies inside the
 * map: its high plastered wall with pillars and a stone coping, the main
 * gate on Fiumei út between two tall stone pylons with lanterns and its
 * iron gates standing open, two smaller side gates, and the graves in rows
 * between the alleys (headstones, stone and iron crosses, obelisks, kerbed
 * family graves, a few marble figures; flowers and grave lanterns on some).
 * The alleys' gravel and the trees along them are AStreetFurniture's
 * (street_furniture.json "kerepesi"); ACityDetails puts up the rest from
 * city_details.json "cemetery".
 *
 * Header-only so it can be built and looked at outside the engine. Frames:
 * a wall run from its start at the foot, +X out to the street, Y along the
 * run; a gate from its middle, +X out to the street; a grave from the foot
 * of its headstone, +X the way the stone faces, the grave lying along +X.
 */
namespace CemeteryModel
{
	using MarketModel::FShape;
	using BozsiHouseModel::FLamp;
	using BozsiHouseModel::Add;
	using BozsiHouseModel::Span;
	using CityDetailsModel::Rod;

	/** The kinds of grave, in city_details.py's numbering. */
	enum class EGrave : uint8 { Headstone, StoneCross, IronCross, Obelisk, FamilyPlot, Figure };

	namespace Colors
	{
		const FLinearColor WallPlaster(0.55f, 0.49f, 0.38f);
		const FLinearColor WallStain(0.42f, 0.38f, 0.30f);
		const FLinearColor Coping(0.50f, 0.48f, 0.44f);
		const FLinearColor PylonStone(0.58f, 0.55f, 0.48f);
		const FLinearColor StoneGrey(0.46f, 0.45f, 0.42f);
		const FLinearColor StoneDark(0.17f, 0.17f, 0.18f);
		const FLinearColor Marble(0.78f, 0.77f, 0.73f);
		const FLinearColor Moss(0.30f, 0.33f, 0.22f);
		const FLinearColor Iron(0.035f, 0.04f, 0.04f);
		const FLinearColor Gilt(0.85f, 0.64f, 0.24f);
		const FLinearColor Lantern(0.95f, 0.85f, 0.55f);
		const FLinearColor Earth(0.20f, 0.16f, 0.11f);
		const FLinearColor Ivy(0.10f, 0.20f, 0.07f);
		const FLinearColor Gravel(0.50f, 0.47f, 0.41f);
		const FLinearColor FlowerRed(0.62f, 0.07f, 0.06f);
		const FLinearColor FlowerWhite(0.86f, 0.85f, 0.80f);
		const FLinearColor Candle(0.60f, 0.10f, 0.06f);
	}

	// Sizes (cm).
	constexpr double WallHeight = 300.0;
	constexpr double WallThick = 50.0;
	constexpr double PillarGap = 560.0;
	constexpr double PillarWidth = 64.0;
	constexpr double MainPylonHeight = 700.0;
	constexpr double SidePylonHeight = 400.0;

	/** A wall run Length long: plastered panels between pillars, a stone coping, stains and ivy here and there. */
	inline void WallRun(double Length, uint32 Seed, TArray<FShape>& Out)
	{
		using namespace Colors;
		const double Half = WallThick * 0.5;
		Span(Out, WallPlaster, FVector(-Half, 0, 0), FVector(Half, Length, WallHeight));
		Span(Out, WallStain, FVector(-Half - 1, 0, 0), FVector(Half + 1, Length, 40), false);
		Span(Out, Coping, FVector(-Half - 8, 0, WallHeight), FVector(Half + 8, Length, WallHeight + 16), false);
		const int32 Panels = FMath::Max(1, FMath::RoundToInt(Length / PillarGap));
		const double Panel = Length / Panels;
		MarketModel::FRandom Random(Seed);
		for (int32 k = 0; k <= Panels; ++k)
		{
			const double Y = FMath::Clamp(Panel * k, PillarWidth * 0.5, Length - PillarWidth * 0.5);
			Span(Out, WallPlaster, FVector(-Half - 10, Y - PillarWidth * 0.5, 0), FVector(Half + 10, Y + PillarWidth * 0.5, WallHeight + 30), false);
			Span(Out, Coping, FVector(-Half - 16, Y - PillarWidth * 0.5 - 6, WallHeight + 30), FVector(Half + 16, Y + PillarWidth * 0.5 + 6, WallHeight + 46), false);
			Add(Out, MarketModel::Cone, Coping, FVector(0, Y, WallHeight + 66), FVector(WallThick + 20, PillarWidth + 4, 40), 0.0, false);
			if (k < Panels)
			{
				// A sunk panel on the street side, a damp stain under the coping, ivy over the top now and then.
				const double Y0 = Panel * k + PillarWidth * 0.5 + 30.0;
				const double Y1 = Panel * (k + 1) - PillarWidth * 0.5 - 30.0;
				if (Y1 > Y0 + 50.0)
				{
					Span(Out, WallStain, FVector(Half, Y0, 80), FVector(Half + 2, Y1, WallHeight - 50), false);
					Span(Out, WallPlaster, FVector(Half + 2, Y0 + 12, 92), FVector(Half + 3, Y1 - 12, WallHeight - 62), false);
				}
				if (Random.Unit() < 0.25)
				{
					const double IvyY = Panel * (k + Random.Range(0.2, 0.8));
					const double IvyWide = Random.Range(120.0, 260.0);
					Span(Out, Ivy, FVector(-Half - 12, IvyY - IvyWide * 0.5, WallHeight - Random.Range(60.0, 160.0)), FVector(Half + 12, IvyY + IvyWide * 0.5, WallHeight + 28), false);
				}
			}
		}
	}

	/** A pylon of stone at Y, Height tall, with a cornice, a cap and (on the main gate) a cross on top and a lantern. */
	inline void Pylon(TArray<FShape>& Out, TArray<FLamp>& Lamps, double Y, double Width, double Height, bool bMain)
	{
		using namespace Colors;
		const double Half = Width * 0.5;
		Span(Out, StoneGrey, FVector(-Half - 10, Y - Half - 10, 0), FVector(Half + 10, Y + Half + 10, 60));
		Span(Out, PylonStone, FVector(-Half, Y - Half, 60), FVector(Half, Y + Half, Height));
		Span(Out, StoneGrey, FVector(Half, Y - Half + 16, Height * 0.45), FVector(Half + 3, Y + Half - 16, Height * 0.85), false);
		Span(Out, PylonStone, FVector(-Half - 14, Y - Half - 14, Height), FVector(Half + 14, Y + Half + 14, Height + 30), false);
		Add(Out, MarketModel::Cone, StoneGrey, FVector(0, Y, Height + 30 + Width * 0.3), FVector(Width + 20, Width + 20, Width * 0.6), 0.0, false);
		if (bMain)
		{
			// A stone cross on top, a lantern on an iron arm to the street.
			const double Top = Height + 30 + Width * 0.6;
			Span(Out, StoneGrey, FVector(-10, Y - 10, Top), FVector(10, Y + 10, Top + 150), false);
			Span(Out, StoneGrey, FVector(-10, Y - 50, Top + 90), FVector(10, Y + 50, Top + 110), false);
			Rod(Out, Iron, FVector(Half, Y, Height - 120), FVector(Half + 70, Y, Height - 120), 5.0);
			Rod(Out, Iron, FVector(Half + 70, Y, Height - 120), FVector(Half + 70, Y, Height - 150), 3.0);
			Add(Out, MarketModel::Cylinder, Lantern, FVector(Half + 70, Y, Height - 175), FVector(26, 26, 40), 0.0, false);
			Add(Out, MarketModel::Cone, Iron, FVector(Half + 70, Y, Height - 145), FVector(36, 36, 18), 0.0, false);
			Lamps.Add({ FVector(Half + 90, Y, Height - 190), 900.f });
		}
	}

	/** One leaf of an iron gate hinged at Y, Width wide, Height tall, swung Open degrees in towards the cemetery. */
	inline void GateLeaf(TArray<FShape>& Out, double Y, double Side, double Width, double Height, double Open)
	{
		using namespace Colors;
		const FRotator Turn(0.0, -Side * Open, 0.0);
		auto P = [&](double Along, double Z) { return FVector(0, Y, 0) + Turn.RotateVector(FVector(0, Side * Along, Z)); };
		Rod(Out, Iron, P(0, 20), P(Width, 20), 6.0);
		Rod(Out, Iron, P(0, Height * 0.5), P(Width, Height * 0.5), 5.0);
		Rod(Out, Iron, P(0, Height), P(Width, Height), 6.0);
		const int32 Bars = FMath::Max(3, FMath::RoundToInt(Width / 14.0));
		for (int32 k = 0; k <= Bars; ++k)
		{
			const double Along = Width * k / Bars;
			// The bars rise to a curve over the middle of the gate, with spear points.
			const double Rise = Height + 40.0 * FMath::Sin(PI * Along / Width) * (Side > 0 ? 1.0 : 1.0);
			Rod(Out, Iron, P(Along, 10), P(Along, Rise), 3.0);
			Add(Out, MarketModel::Cone, Gilt, P(Along, Rise + 8), FVector(6, 6, 16), 0.0, false);
		}
	}

	/** A gate Width wide in the wall, from its middle: the main one with tall pylons and a name, or a side one. */
	inline void Gate(bool bMain, double Width, TArray<FShape>& Out, TArray<FLamp>& Lamps)
	{
		using namespace Colors;
		const double Half = Width * 0.5;
		const double PylonWidth = bMain ? 150.0 : 80.0;
		const double Height = bMain ? MainPylonHeight : SidePylonHeight;
		const double Opening = bMain ? 520.0 : Width - 2.0 * PylonWidth - 40.0;
		const double PylonY = Opening * 0.5 + PylonWidth * 0.5;
		for (const double S : { -1.0, 1.0 })
		{
			Pylon(Out, Lamps, S * PylonY, PylonWidth, Height, bMain);
			if (bMain)
			{
				// A low wall with a wicket gate out to the wall's end either side.
				const double Y0 = S * (PylonY + PylonWidth * 0.5);
				const double Y1 = S * Half;
				Span(Out, WallPlaster, FVector(-WallThick * 0.5, FMath::Min(Y0, Y1), 0), FVector(WallThick * 0.5, FMath::Max(Y0, Y1), 60));
				Span(Out, Coping, FVector(-WallThick * 0.5 - 8, FMath::Min(Y0, Y1), 60), FVector(WallThick * 0.5 + 8, FMath::Max(Y0, Y1), 74), false);
				for (double Y = FMath::Min(Y0, Y1) + 10.0; Y < FMath::Max(Y0, Y1) - 5.0; Y += 14.0)
				{
					Rod(Out, Iron, FVector(0, Y, 74), FVector(0, Y, WallHeight), 3.0);
				}
				Rod(Out, Iron, FVector(0, FMath::Min(Y0, Y1), WallHeight - 30), FVector(0, FMath::Max(Y0, Y1), WallHeight - 30), 5.0);
				Rod(Out, Iron, FVector(0, FMath::Min(Y0, Y1), 120), FVector(0, FMath::Max(Y0, Y1), 120), 5.0);
			}
			// The leaves stand open, swung in.
			GateLeaf(Out, S * Opening * 0.5, -S, Opening * 0.5, bMain ? 380.0 : 260.0, 75.0);
		}
		if (bMain)
		{
			// An iron arch over the opening with a gilt cross.
			for (int32 k = 0; k < 10; ++k)
			{
				const double A0 = PI * k / 10.0;
				const double A1 = PI * (k + 1) / 10.0;
				const double R = Opening * 0.5;
				Rod(Out, Iron, FVector(0, -R * FMath::Cos(A0), 450 + R * 0.45 * FMath::Sin(A0)), FVector(0, -R * FMath::Cos(A1), 450 + R * 0.45 * FMath::Sin(A1)), 6.0);
			}
			Span(Out, Gilt, FVector(-3, -4, 570), FVector(3, 4, 660), false);
			Span(Out, Gilt, FVector(-3, -26, 625), FVector(3, 26, 633), false);
			// A gravel apron in front and behind.
			Span(Out, Gravel, FVector(-500, -Opening * 0.5, 0), FVector(400, Opening * 0.5, 2), false);
		}
	}

	/** One grave from the foot of its headstone: Kind as above, Seed picks the stone and the bits. */
	inline void Grave(EGrave Kind, uint32 Seed, TArray<FShape>& Out)
	{
		using namespace Colors;
		MarketModel::FRandom Random(Seed + 17u);
		const FLinearColor Stones[] = { StoneGrey, StoneGrey, StoneDark, Marble, Moss };
		const FLinearColor& Stone = Stones[Random.Index(5)];
		const double Tall = Random.Range(0.85, 1.2);
		const bool bMound = Kind != EGrave::FamilyPlot;
		if (bMound)
		{
			// The grave itself: a low mound of earth with grass going over it, or with a stone kerb.
			Add(Out, MarketModel::Sphere, Earth, FVector(95, 0, 2), FVector(170, 80, 22), 0.0, false);
			if (Random.Unit() < 0.4)
			{
				Span(Out, Stone, FVector(10, -46, 0), FVector(185, -38, 14), false);
				Span(Out, Stone, FVector(10, 38, 0), FVector(185, 46, 14), false);
				Span(Out, Stone, FVector(177, -46, 0), FVector(185, 46, 14), false);
			}
		}
		switch (Kind)
		{
		case EGrave::Headstone:
		{
			const double H = 90.0 * Tall;
			Span(Out, Stone, FVector(-14, -36, 0), FVector(14, 36, 14));
			Span(Out, Stone, FVector(-8, -30, 14), FVector(8, 30, H));
			Add(Out, MarketModel::Cylinder, Stone, FVector(0, 0, H), FVector(60, 60, 16), 0.0, false, 90.0);
			Span(Out, Gilt, FVector(8, -14, H - 30), FVector(9, 14, H - 26), false);
			break;
		}
		case EGrave::StoneCross:
		{
			const double H = 120.0 * Tall;
			Span(Out, Stone, FVector(-18, -26, 0), FVector(18, 26, 30));
			Span(Out, Stone, FVector(-14, -20, 30), FVector(14, 20, 46));
			Span(Out, Stone, FVector(-6, -6, 46), FVector(6, 6, H));
			Span(Out, Stone, FVector(-6, -26, H - 40), FVector(6, 26, H - 28));
			break;
		}
		case EGrave::IronCross:
		{
			const double H = 110.0 * Tall;
			Rod(Out, Iron, FVector(0, 0, 0), FVector(0, 0, H), 4.0);
			Rod(Out, Iron, FVector(0, -24, H - 32), FVector(0, 24, H - 32), 4.0);
			Add(Out, MarketModel::Cylinder, Iron, FVector(0, 0, H - 32), FVector(22, 22, 2), 0.0, false, 90.0);
			Add(Out, MarketModel::Cylinder, Marble, FVector(2, 0, H - 32), FVector(14, 14, 2), 0.0, false, 90.0);
			break;
		}
		case EGrave::Obelisk:
		{
			const double H = 210.0 * Tall;
			Span(Out, Stone, FVector(-30, -30, 0), FVector(30, 30, 30));
			Span(Out, Stone, FVector(-22, -22, 30), FVector(22, 22, 70));
			Add(Out, MarketModel::Cube, Stone, FVector(0, 0, 70 + (H - 70) * 0.5), FVector(30, 30, H - 70), 0.0, false);
			Add(Out, MarketModel::Cone, Stone, FVector(0, 0, H + 12), FVector(32, 32, 24), 0.0, false);
			Span(Out, Gilt, FVector(22, -12, 46), FVector(23, 12, 56), false);
			break;
		}
		case EGrave::FamilyPlot:
		{
			// A kerbed plot with gravel, a wide headstone, sometimes a low iron railing.
			Span(Out, Stone, FVector(0, -80, 0), FVector(200, -70, 20), false);
			Span(Out, Stone, FVector(0, 70, 0), FVector(200, 80, 20), false);
			Span(Out, Stone, FVector(190, -80, 0), FVector(200, 80, 20), false);
			Span(Out, Gravel, FVector(10, -70, 0), FVector(190, 70, 8), false);
			const double H = 120.0 * Tall;
			Span(Out, Stone, FVector(-20, -86, 0), FVector(20, 86, 24));
			Span(Out, Stone, FVector(-12, -74, 24), FVector(12, 74, H));
			Span(Out, Stone, FVector(-16, -80, H), FVector(16, 80, H + 12), false);
			Span(Out, StoneDark, FVector(12, -50, H - 80), FVector(13, 50, H - 20), false);
			Span(Out, Gilt, FVector(13, -36, H - 44), FVector(14, 36, H - 40), false);
			if (Random.Unit() < 0.5)
			{
				for (const double Y : { -86.0, 86.0 })
				{
					Rod(Out, Iron, FVector(0, Y, 20), FVector(206, Y, 20), 2.5);
					Rod(Out, Iron, FVector(0, Y, 60), FVector(206, Y, 60), 2.5);
					for (double X = 10.0; X <= 206.0; X += 28.0)
					{
						Rod(Out, Iron, FVector(X, Y, 0), FVector(X, Y, 66), 2.5);
					}
				}
			}
			break;
		}
		case EGrave::Figure:
		default:
		{
			// A marble figure, head bowed, on a tall plinth.
			Span(Out, Stone, FVector(-34, -34, 0), FVector(34, 34, 40));
			Span(Out, Marble, FVector(-26, -26, 40), FVector(26, 26, 120));
			Add(Out, MarketModel::Cone, Marble, FVector(0, 0, 178), FVector(46, 40, 116), 0.0, false);
			Add(Out, MarketModel::Cylinder, Marble, FVector(2, 0, 216), FVector(24, 28, 36), 0.0, false);
			Add(Out, MarketModel::Sphere, Marble, FVector(6, 0, 244), FVector(20, 18, 22), 0.0, false);
			Add(Out, MarketModel::Cube, Marble, FVector(-8, -18, 220), FVector(10, 26, 50), 0.0, false, 0.0, -20.0);
			Add(Out, MarketModel::Cube, Marble, FVector(-8, 18, 220), FVector(10, 26, 50), 0.0, false, 0.0, 20.0);
			break;
		}
		}

		// Now and then flowers or a red grave lantern in front of the stone.
		const double Bits = Random.Unit();
		if (Bits < 0.25)
		{
			Add(Out, MarketModel::Cylinder, StoneDark, FVector(30, 18, 8), FVector(16, 16, 16), 0.0, false);
			Add(Out, MarketModel::Sphere, Random.Unit() < 0.5 ? FlowerRed : FlowerWhite, FVector(30, 18, 22), FVector(26, 26, 18), 0.0, false);
		}
		else if (Bits < 0.4)
		{
			Add(Out, MarketModel::Cylinder, Candle, FVector(28, -16, 12), FVector(10, 10, 20), 0.0, false);
			Add(Out, MarketModel::Cone, Iron, FVector(28, -16, 26), FVector(12, 12, 8), 0.0, false);
		}
	}
}
