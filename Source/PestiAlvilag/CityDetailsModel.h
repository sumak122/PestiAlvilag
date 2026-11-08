// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BozsiHouseModel.h"
#include "MarketModel.h"
#include "Math/RotationMatrix.h"

/**
 * The city's smaller details, as basic shapes (MarketModel::FShape) with
 * painted lettering and small lamps (ACityDetails puts them up):
 * - the Corvin department store's front on Blaha Lujza ter (opened 1926):
 *   shop windows under a glass canopy, giant pilasters, a deep cornice and
 *   the gilt name on the attic, flags on the roof;
 * - newspaper and tobacco kiosks (trafik) by the tram stops;
 * - street clocks on a cast iron post, two faces; their hands are made
 *   apart (ClockHands) so they can follow the game's time;
 * - painted adverts on the bare fire walls above a lower neighbour;
 * - an open tenement gateway, and the courtyard behind it: open balconies
 *   (gangs) with iron railings round the walls, doors and windows, the
 *   pump in the middle, a carpet beating rack, a washing line, dustbins.
 *
 * Header-only so it can be built and looked at outside the engine. Each
 * thing's frame: +X out (to the street, or towards the one reading it),
 * Z up from the ground.
 */
namespace CityDetailsModel
{
	using MarketModel::FShape;
	using BozsiHouseModel::FSign;
	using BozsiHouseModel::FLamp;
	using BozsiHouseModel::Add;
	using BozsiHouseModel::Span;

	namespace Colors
	{
		const FLinearColor CorvinStone(0.66f, 0.61f, 0.52f);
		const FLinearColor CorvinPier(0.50f, 0.46f, 0.40f);
		const FLinearColor CorvinPlinth(0.22f, 0.21f, 0.20f);
		const FLinearColor CorvinSpandrel(0.48f, 0.42f, 0.34f);
		const FLinearColor Bronze(0.20f, 0.14f, 0.08f);
		const FLinearColor Gilt(0.85f, 0.64f, 0.24f);
		const FLinearColor Glass(0.05f, 0.07f, 0.09f);
		const FLinearColor ShowGlass(0.28f, 0.24f, 0.16f);
		const FLinearColor Frame(0.80f, 0.77f, 0.68f);
		const FLinearColor Iron(0.035f, 0.05f, 0.04f);
		const FLinearColor FlagRed(0.55f, 0.04f, 0.04f);
		const FLinearColor FlagWhite(0.85f, 0.85f, 0.82f);
		const FLinearColor FlagGreen(0.05f, 0.30f, 0.10f);
		const FLinearColor Paper(0.86f, 0.84f, 0.76f);
		const FLinearColor PaperPink(0.85f, 0.62f, 0.55f);
		const FLinearColor PaperYellow(0.84f, 0.74f, 0.42f);
		const FLinearColor Roof(0.12f, 0.12f, 0.12f);
		const FLinearColor ClockFace(0.92f, 0.90f, 0.82f);
		const FLinearColor Hand(0.02f, 0.02f, 0.02f);
		const FLinearColor Setts(0.30f, 0.29f, 0.27f);
		const FLinearColor StoneDark(0.40f, 0.37f, 0.33f);
		const FLinearColor Plaster(0.60f, 0.55f, 0.45f);
		const FLinearColor Door(0.24f, 0.15f, 0.08f);
		const FLinearColor DoorGreen(0.10f, 0.20f, 0.14f);
		const FLinearColor Wood(0.36f, 0.25f, 0.14f);
		const FLinearColor Zinc(0.32f, 0.33f, 0.33f);
		const FLinearColor Carpet(0.45f, 0.08f, 0.06f);
		const FLinearColor CarpetBand(0.70f, 0.55f, 0.25f);
		const FLinearColor Geranium(0.70f, 0.05f, 0.04f);
		const FLinearColor Leaves(0.12f, 0.20f, 0.05f);
		const FLinearColor LeavesGold(0.45f, 0.30f, 0.05f);
		const FLinearColor Bark(0.16f, 0.11f, 0.07f);
		const FLinearColor Clay(0.45f, 0.20f, 0.10f);
		const FLinearColor Lantern(0.95f, 0.85f, 0.55f);
	}

	/** A shape stretched between two points (a rod, a line, a pole). */
	inline void Rod(TArray<FShape>& Out, const FLinearColor& Color, const FVector& A, const FVector& B, double Thick, bool bCollide = false)
	{
		const FVector Along = B - A;
		const double Length = Along.Size();
		if (Length < 1.0)
		{
			return;
		}
		const FRotator Turn = FRotationMatrix::MakeFromZ(Along / Length).Rotator();
		Add(Out, MarketModel::Cylinder, Color, (A + B) * 0.5, FVector(Thick, Thick, Length), Turn.Yaw, bCollide, Turn.Pitch, Turn.Roll);
	}

	/** A box lying along the direction Yaw (degrees): Length along it, Width across, Height up, its middle at At. */
	inline void Beam(TArray<FShape>& Out, const FLinearColor& Color, const FVector& At, double Yaw, double Length, double Width, double Height, bool bCollide = false)
	{
		Add(Out, MarketModel::Cube, Color, At, FVector(Length, Width, Height), Yaw, bCollide);
	}

	// ---------------------------------------------------------------------
	// The Corvin department store.

	/** The middle of the store's front at its foot, on Blaha Lujza ter, and the way out (north, to the square). */
	const FVector2D CorvinAt(-9439.5, 3697.0);
	constexpr double CorvinYaw = -92.12;
	constexpr double CorvinLength = 6109.0;
	/** The map block it stands on, and its height (cm; OsmGraybox puts the block up this tall). */
	const TCHAR* const CorvinBuilding = TEXT("w261801166");
	constexpr double CorvinHeight = 2350.0;

	inline void Corvin(TArray<FShape>& Out, TArray<FLamp>& Lamps, TArray<FSign>& Signs)
	{
		using namespace Colors;
		constexpr int32 Bays = 10;
		const double Half = CorvinLength * 0.5;
		const double Bay = CorvinLength / Bays;
		constexpr double ShopTop = 560.0;      // top of the shop floor's fascia
		constexpr double CapitalAt = 2000.0;   // the pilasters' capitals
		constexpr double CorniceAt = 2150.0;
		constexpr double Top = CorvinHeight;

		// The face over the plain block's wall, and the dark plinth.
		Span(Out, CorvinStone, FVector(0, -Half, ShopTop), FVector(6, Half, Top), false);
		Span(Out, CorvinPlinth, FVector(0, -Half, 0), FVector(44, Half, 60));

		for (int32 k = 0; k <= Bays; ++k)
		{
			const double Y = -Half + Bay * k;
			const double Y0 = FMath::Max(Y - 40.0, -Half);
			const double Y1 = FMath::Min(Y + 40.0, Half);
			// The shop floor's piers, then the giant pilasters through three floors.
			Span(Out, CorvinPier, FVector(0, Y0, 60), FVector(40, Y1, ShopTop));
			Span(Out, CorvinStone, FVector(0, Y0, ShopTop), FVector(34, Y1, CapitalAt), false);
			Span(Out, Frame, FVector(0, Y0 - 8, CapitalAt), FVector(44, Y1 + 8, CapitalAt + 60), false);
			Span(Out, CorvinPier, FVector(30, Y0 + 12, ShopTop + 40), FVector(36, Y1 - 12, CapitalAt - 40), false);
		}
		for (int32 k = 0; k < Bays; ++k)
		{
			const double Y0 = -Half + Bay * k + 40.0;
			const double Y1 = -Half + Bay * (k + 1) - 40.0;
			const double Mid = (Y0 + Y1) * 0.5;
			const bool bDoor = k == Bays / 2 - 1 || k == Bays / 2;
			if (bDoor)
			{
				// The entrance: glazed double doors in bronze, set back, a lit fanlight over them.
				Span(Out, Bronze, FVector(-40, Y0, 60), FVector(-30, Y1, 440), false);
				for (const double S : { -1.0, 1.0 })
				{
					const double DoorMid = Mid + S * 110.0;
					Span(Out, Glass, FVector(-30, DoorMid - 95, 60), FVector(-26, DoorMid + 95, 330), false);
					Span(Out, Bronze, FVector(-26, DoorMid - 4, 60), FVector(-22, DoorMid + 4, 330), false);
					Span(Out, Bronze, FVector(-26, DoorMid - 95, 180), FVector(-22, DoorMid + 95, 190), false);
				}
				Span(Out, ShowGlass, FVector(-30, Y0 + 20, 350), FVector(-26, Y1 - 20, 430), false);
				Span(Out, CorvinPier, FVector(-40, Y0, 0), FVector(0, Y1, 8));
			}
			else
			{
				// A shop window from the plinth up to the fascia, in a bronze frame.
				Span(Out, Bronze, FVector(0, Y0, 60), FVector(10, Y1, 72), false);
				Span(Out, ShowGlass, FVector(4, Y0 + 6, 72), FVector(8, Y1 - 6, 430), false);
				Span(Out, Bronze, FVector(6, Mid - 3, 72), FVector(10, Mid + 3, 430), false);
				Span(Out, Bronze, FVector(6, Y0, 340), FVector(10, Y1, 346), false);
				Span(Out, Bronze, FVector(0, Y0, 430), FVector(12, Y1, 440), false);
			}
			// The fascia over the shop floor.
			Span(Out, Bronze, FVector(0, Y0, 440), FVector(14, Y1, ShopTop), false);
			// Three floors of wide windows with a mullion each, spandrels between them.
			for (int32 Floor = 0; Floor < 3; ++Floor)
			{
				const double Z0 = 650.0 + Floor * 450.0;
				const double Z1 = Z0 + 330.0;
				Span(Out, Frame, FVector(6, Y0 + 30, Z0 - 10), FVector(12, Y1 - 30, Z1 + 8), false);
				Span(Out, Glass, FVector(12, Y0 + 40, Z0), FVector(14, Y1 - 40, Z1), false);
				for (const double T : { -0.5, 0.0, 0.5 })
				{
					Span(Out, Frame, FVector(12, Mid + T * (Y1 - Y0 - 80) * 0.66 - 4, Z0), FVector(16, Mid + T * (Y1 - Y0 - 80) * 0.66 + 4, Z1), false);
				}
				Span(Out, Frame, FVector(12, Y0 + 40, Z0 + 230), FVector(16, Y1 - 40, Z0 + 238), false);
				if (Floor < 2)
				{
					Span(Out, CorvinSpandrel, FVector(6, Y0 + 30, Z1 + 8), FVector(10, Y1 - 30, Z1 + 112), false);
					Span(Out, CorvinStone, FVector(10, Mid - 120, Z1 + 30), FVector(13, Mid + 120, Z1 + 90), false);
				}
			}
		}

		// The glass canopy on iron brackets over the shop windows, with lamps under it.
		Span(Out, Iron, FVector(0, -Half + Bay * 2, 470), FVector(190, Half - Bay * 2, 486), false);
		Span(Out, ShowGlass, FVector(10, -Half + Bay * 2 + 10, 486), FVector(180, Half - Bay * 2 - 10, 490), false);
		Span(Out, Gilt, FVector(188, -Half + Bay * 2, 470), FVector(192, Half - Bay * 2, 486), false);
		for (int32 k = 2; k <= Bays - 2; ++k)
		{
			const double Y = -Half + Bay * k;
			Rod(Out, Iron, FVector(40, Y, 640), FVector(185, Y, 490), 4.0);
			if (k < Bays - 2)
			{
				Add(Out, MarketModel::Sphere, Lantern, FVector(140, Y + Bay * 0.5, 450), FVector(26, 26, 26), 0.0, false);
				Lamps.Add({ FVector(140, Y + Bay * 0.5, 430), 700.f });
			}
		}

		// The entablature, the deep cornice, the attic with the name.
		Span(Out, Frame, FVector(0, -Half, CapitalAt + 60), FVector(30, Half, CorniceAt), false);
		Span(Out, CorvinStone, FVector(0, -Half - 20, CorniceAt), FVector(80, Half + 20, CorniceAt + 50), false);
		Span(Out, Frame, FVector(0, -Half - 10, CorniceAt - 20), FVector(60, Half + 10, CorniceAt), false);
		Span(Out, CorvinStone, FVector(0, -Half, CorniceAt + 50), FVector(24, Half, Top), false);
		Span(Out, Frame, FVector(0, -Half, Top - 12), FVector(30, Half, Top + 6), false);
		Signs.Add({ TEXT("CORVIN ÁRUHÁZ"), FVector(26, 0, CorniceAt + 110), 0.0, 150.0, Gilt });
		Signs.Add({ TEXT("CORVIN"), FVector(16, 0, 505), 0.0, 56.0, Gilt });
		Signs.Add({ TEXT("DIVAT  ·  FEHÉRNEMŰ  ·  JÁTÉK  ·  HÁZ TARTÁS"), FVector(194, 0, 478), 0.0, 14.0, Gilt });

		// Two flags on poles on the roof, red, white and green.
		for (const double Y : { -Half + Bay * 1.5, Half - Bay * 1.5 })
		{
			Rod(Out, Iron, FVector(-150, Y, Top), FVector(-150, Y, Top + 750), 8.0);
			Add(Out, MarketModel::Sphere, Gilt, FVector(-150, Y, Top + 755), FVector(16, 16, 16), 0.0, false);
			const FLinearColor Stripes[] = { FlagRed, FlagWhite, FlagGreen };
			for (int32 s = 0; s < 3; ++s)
			{
				Span(Out, Stripes[s], FVector(-152, Y + 4, Top + 720 - 60 * (s + 1)), FVector(-148, Y + 260, Top + 720 - 60 * s), false);
			}
		}
	}

	// ---------------------------------------------------------------------
	// The newspaper and tobacco kiosk: the counter faces +X.

	inline void Kiosk(int32 Variant, TArray<FShape>& Out, TArray<FLamp>& Lamps, TArray<FSign>& Signs)
	{
		using namespace Colors;
		const FLinearColor Paints[] = { FLinearColor(0.08f, 0.22f, 0.14f), FLinearColor(0.32f, 0.07f, 0.05f), FLinearColor(0.28f, 0.19f, 0.10f) };
		const FLinearColor Body = Paints[((Variant % 3) + 3) % 3];
		constexpr double HX = 80.0, HY = 110.0, Waist = 100.0, Eave = 235.0;
		Span(Out, StoneDark, FVector(-HX - 6, -HY - 6, 0), FVector(HX + 6, HY + 6, 12));
		// The panelled lower half all round, with raised panels.
		Span(Out, Body, FVector(-HX, -HY, 12), FVector(HX, HY, Waist));
		for (const double Y : { -HY * 0.5, HY * 0.5 })
		{
			Span(Out, Body * 1.4f, FVector(HX, Y - 40, 28), FVector(HX + 2, Y + 40, Waist - 14), false);
		}
		// The upper half: back and sides closed, the front an open counter window.
		Span(Out, Body, FVector(-HX, -HY, Waist), FVector(-HX + 8, HY, Eave));
		for (const double S : { -1.0, 1.0 })
		{
			Span(Out, Body, FVector(-HX, S > 0 ? HY - 8 : -HY, Waist), FVector(HX, S > 0 ? HY : -HY + 8, Eave));
			Span(Out, Body, FVector(HX - 8, S > 0 ? HY - 26 : -HY, Waist), FVector(HX, S > 0 ? HY : -HY + 26, Eave), false);
		}
		// The counter board, the papers laid out on it and pegged up round the window.
		Span(Out, Wood, FVector(HX - 10, -HY + 8, Waist), FVector(HX + 22, HY - 8, Waist + 5));
		const FLinearColor Papers[] = { Paper, PaperPink, PaperYellow, Paper };
		for (int32 i = 0; i < 4; ++i)
		{
			const double Y = -HY + 40 + i * 48.0;
			Add(Out, MarketModel::Cube, Papers[i], FVector(HX + 6, Y, Waist + 7), FVector(26, 34, 4), (i - 1.5) * 6.0, false);
		}
		for (int32 i = 0; i < 5; ++i)
		{
			const double Y = -HY + 30 + i * 40.0;
			Span(Out, Papers[(i + 1) % 4], FVector(HX - 6, Y - 16, Eave - 60), FVector(HX - 4, Y + 16, Eave - 14), false);
		}
		for (const double S : { -1.0, 1.0 })
		{
			for (int32 i = 0; i < 3; ++i)
			{
				const double X = -HX + 30 + i * 46.0;
				Span(Out, Papers[(i + 2) % 4], FVector(X - 18, S * (HY + 1), Waist + 30), FVector(X + 18, S * (HY + 3), Waist + 90), false);
			}
		}
		// Inside: shelves of cigarette boxes and the seller's stool.
		for (const double Z : { 150.0, 185.0 })
		{
			Span(Out, Wood, FVector(-HX + 8, -HY + 10, Z), FVector(-HX + 30, HY - 10, Z + 4), false);
			for (int32 i = 0; i < 6; ++i)
			{
				Span(Out, Papers[i % 4] * 0.8f, FVector(-HX + 12, -HY + 20 + i * 32, Z + 4), FVector(-HX + 26, -HY + 40 + i * 32, Z + 18), false);
			}
		}
		// The fascia and the roof over it, standing out over the counter.
		Span(Out, Body, FVector(-HX - 4, -HY - 4, Eave), FVector(HX + 4, HY + 4, Eave + 40));
		Span(Out, Roof, FVector(-HX - 20, -HY - 22, Eave + 40), FVector(HX + 42, HY + 22, Eave + 52), false);
		Add(Out, MarketModel::Cube, Roof, FVector(0, 0, Eave + 66), FVector(HX * 2 + 10, HY * 2 - 20, 24), 0.0, false);
		Add(Out, MarketModel::Sphere, Gilt, FVector(0, 0, Eave + 84), FVector(16, 16, 16), 0.0, false);
		Signs.Add({ TEXT("DOHÁNY · ÚJSÁG"), FVector(HX + 5, 0, Eave + 20), 0.0, 22.0, Gilt });
		Signs.Add({ TEXT("TRAFIK"), FVector(-HX - 5, 0, Eave + 20), 180.0, 24.0, Gilt });
		Add(Out, MarketModel::Sphere, Lantern, FVector(HX + 30, 0, Eave + 30), FVector(16, 16, 16), 0.0, false);
		Lamps.Add({ FVector(HX + 30, 0, Eave + 20), 400.f });
	}

	// ---------------------------------------------------------------------
	// The street clock.

	/** One face of a clock: its middle, which way it looks along X (+1 or -1), its radius (cm). */
	struct FClockFace
	{
		FVector Centre;
		double Facing;
		double Radius;
	};

	constexpr double ClockHeadZ = 400.0;
	constexpr double ClockFaceRadius = 40.0;
	constexpr double ClockFaceOut = 15.0;

	inline void ClockFaces(TArray<FClockFace>& Faces)
	{
		Faces.Add({ FVector(ClockFaceOut + 1.0, 0, ClockHeadZ), 1.0, ClockFaceRadius });
		Faces.Add({ FVector(-ClockFaceOut - 1.0, 0, ClockHeadZ), -1.0, ClockFaceRadius });
	}

	inline void Clock(TArray<FShape>& Out)
	{
		using namespace Colors;
		// The base, the fluted post with its collars, the bracket under the head.
		Add(Out, MarketModel::Cylinder, Iron, FVector(0, 0, 20), FVector(56, 56, 40));
		Add(Out, MarketModel::Cylinder, Iron, FVector(0, 0, 55), FVector(36, 36, 30));
		Add(Out, MarketModel::Cylinder, Iron, FVector(0, 0, 200), FVector(18, 18, 300));
		for (const double Z : { 75.0, 140.0, 320.0 })
		{
			Add(Out, MarketModel::Cylinder, Iron, FVector(0, 0, Z), FVector(26, 26, 8), 0.0, false);
		}
		Add(Out, MarketModel::Sphere, Iron, FVector(0, 0, 350), FVector(30, 30, 30), 0.0, false);
		// The drum with two faces, a ring round each, twelve marks.
		Add(Out, MarketModel::Cylinder, Iron, FVector(0, 0, ClockHeadZ), FVector(ClockFaceRadius * 2 + 14, ClockFaceRadius * 2 + 14, ClockFaceOut * 2 - 2), 0.0, false, 90.0);
		TArray<FClockFace> Faces;
		ClockFaces(Faces);
		for (const FClockFace& F : Faces)
		{
			Add(Out, MarketModel::Cylinder, ClockFace, F.Centre - FVector(F.Facing * 1.0, 0, 0), FVector(F.Radius * 2, F.Radius * 2, 2), 0.0, false, 90.0);
			for (int32 i = 0; i < 12; ++i)
			{
				const double A = FMath::DegreesToRadians(i * 30.0);
				const FVector Dir(0.0, -F.Facing * FMath::Sin(A), FMath::Cos(A));
				const double Long = i % 3 == 0 ? 9.0 : 5.0;
				Add(Out, MarketModel::Cube, Hand, F.Centre + Dir * (F.Radius - 4 - Long * 0.5), FVector(1, 3, Long), 0.0, false, 0.0, -F.Facing * i * 30.0);
			}
		}
		// The crown: a little dome and finial.
		Add(Out, MarketModel::Sphere, Iron, FVector(0, 0, ClockHeadZ + ClockFaceRadius + 4), FVector(40, 40, 30), 0.0, false);
		Add(Out, MarketModel::Cone, Gilt, FVector(0, 0, ClockHeadZ + ClockFaceRadius + 30), FVector(10, 10, 26), 0.0, false);
	}

	/** The hands of one face showing Hour (0 to 24), in the clock's frame. */
	inline void ClockHands(const FClockFace& F, double Hour, TArray<FShape>& Out)
	{
		const double Minutes = FMath::Fmod(Hour, 1.0) * 60.0;
		const double HourAngle = FMath::Fmod(Hour, 12.0) * 30.0;
		const double MinuteAngle = Minutes * 6.0;
		const double Lengths[] = { F.Radius * 0.55, F.Radius * 0.85 };
		const double Widths[] = { 4.0, 2.6 };
		const double Angles[] = { HourAngle, MinuteAngle };
		for (int32 i = 0; i < 2; ++i)
		{
			const double A = FMath::DegreesToRadians(Angles[i]);
			const FVector Dir(0.0, -F.Facing * FMath::Sin(A), FMath::Cos(A));
			const FVector At = F.Centre + FVector(F.Facing * (1.0 + i), 0, 0) + Dir * (Lengths[i] * 0.5 - 5.0);
			Add(Out, MarketModel::Cube, Colors::Hand, At, FVector(1, Widths[i], Lengths[i]), 0.0, false, 0.0, -F.Facing * Angles[i]);
		}
	}

	// ---------------------------------------------------------------------
	// Painted adverts on the fire walls. The field's frame: the wall at X = 0,
	// +X out, the field Width wide round Y = 0, from Z0 to Z1.

	struct FAdvert
	{
		const TCHAR* Big;
		const TCHAR* Small;
		FLinearColor Ground;
		FLinearColor Ink;
		FLinearColor Band;
	};

	constexpr int32 NumAdverts = 8;

	inline FAdvert Advert(int32 Kind)
	{
		// Faded paint: the colours a few summers after the painters left.
		static const FAdvert Adverts[NumAdverts] = {
			{ TEXT("TUNGSRAM"), TEXT("VILLANYKÖRTE"), FLinearColor(0.10f, 0.17f, 0.34f), FLinearColor(0.86f, 0.82f, 0.70f), FLinearColor(0.70f, 0.55f, 0.20f) },
			{ TEXT("PALMA"), TEXT("KAUCSUKSAROK"), FLinearColor(0.78f, 0.72f, 0.58f), FLinearColor(0.50f, 0.07f, 0.05f), FLinearColor(0.15f, 0.25f, 0.12f) },
			{ TEXT("STÜHMER"), TEXT("CSOKOLÁDÉ"), FLinearColor(0.72f, 0.58f, 0.22f), FLinearColor(0.24f, 0.12f, 0.06f), FLinearColor(0.42f, 0.10f, 0.06f) },
			{ TEXT("DREHER"), TEXT("SÖR"), FLinearColor(0.42f, 0.09f, 0.07f), FLinearColor(0.88f, 0.82f, 0.66f), FLinearColor(0.74f, 0.60f, 0.26f) },
			{ TEXT("UNICUM"), TEXT("ZWACK"), FLinearColor(0.07f, 0.07f, 0.06f), FLinearColor(0.80f, 0.64f, 0.28f), FLinearColor(0.50f, 0.08f, 0.06f) },
			{ TEXT("TÖRLEY"), TEXT("PEZSGŐ"), FLinearColor(0.80f, 0.76f, 0.62f), FLinearColor(0.08f, 0.24f, 0.14f), FLinearColor(0.68f, 0.52f, 0.20f) },
			{ TEXT("SCHICHT"), TEXT("SZAPPAN"), FLinearColor(0.82f, 0.80f, 0.72f), FLinearColor(0.10f, 0.20f, 0.45f), FLinearColor(0.50f, 0.08f, 0.06f) },
			{ TEXT("MEINL"), TEXT("KÁVÉ"), FLinearColor(0.50f, 0.10f, 0.07f), FLinearColor(0.88f, 0.84f, 0.72f), FLinearColor(0.10f, 0.08f, 0.06f) },
		};
		return Adverts[((Kind % NumAdverts) + NumAdverts) % NumAdverts];
	}

	inline void WallAdvert(int32 Kind, double Width, double Z0, double Z1, TArray<FShape>& Out, TArray<FSign>& Signs)
	{
		const FAdvert Ad = Advert(Kind);
		const double Half = Width * 0.5;
		const double High = Z1 - Z0;
		Span(Out, Ad.Ground, FVector(0, -Half, Z0), FVector(3, Half, Z1), false);
		// A painted band round the field, a little in.
		const double B = FMath::Clamp(High * 0.04, 15.0, 40.0);
		const double In = B * 1.5;
		Span(Out, Ad.Band, FVector(3, -Half + In, Z0 + In), FVector(4, Half - In, Z0 + In + B), false);
		Span(Out, Ad.Band, FVector(3, -Half + In, Z1 - In - B), FVector(4, Half - In, Z1 - In), false);
		Span(Out, Ad.Band, FVector(3, -Half + In, Z0 + In), FVector(4, -Half + In + B, Z1 - In), false);
		Span(Out, Ad.Band, FVector(3, Half - In - B, Z0 + In), FVector(4, Half - In, Z1 - In), false);
		// The name as big as fits, the line under it smaller; letters are about 0.65 of their height wide.
		const double Inner = Width - 2.0 * (In + B) - 60.0;
		const double BigHeight = FMath::Min(High * 0.36, Inner / (0.65 * FCString::Strlen(Ad.Big)));
		const double SmallHeight = FMath::Min(BigHeight * 0.45, Inner / (0.65 * FCString::Strlen(Ad.Small)));
		Signs.Add({ Ad.Big, FVector(5, 0, Z0 + High * 0.60), 0.0, BigHeight, Ad.Ink });
		Signs.Add({ Ad.Small, FVector(5, 0, Z0 + High * 0.28), 0.0, SmallHeight, Ad.Band });
	}

	// ---------------------------------------------------------------------
	// The open gateway: its frame at the gate's foot on the street wall, +X out
	// to the street, the gateway running back to X = -Depth.

	constexpr double GatewayTop = 340.0;

	inline void Gateway(double Width, double Depth, TArray<FShape>& Out, TArray<FLamp>& Lamps)
	{
		using namespace Colors;
		const double Half = Width * 0.5 - 34.0;
		Span(Out, Setts, FVector(-Depth - 30, -Half, -2), FVector(0, Half, 3), false);
		for (const double S : { -1.0, 1.0 })
		{
			// Stone dado and wheel guard stones along both walls, plaster above.
			Span(Out, StoneDark, FVector(-Depth, S * Half - (S > 0 ? 6 : 0), 0), FVector(0, S * Half + (S > 0 ? 0 : 6), 110));
			Span(Out, Plaster, FVector(-Depth, S * Half - (S > 0 ? 3 : 0), 110), FVector(0, S * Half + (S > 0 ? 0 : 3), GatewayTop), false);
			for (double X = -150.0; X > -Depth + 60.0; X -= 380.0)
			{
				Add(Out, MarketModel::Cylinder, StoneDark, FVector(X, S * (Half - 14), 25), FVector(20, 20, 50));
			}
		}
		// The vault's ribs across, and the gateway lamp in the middle.
		for (double X = -60.0; X > -Depth; X -= 400.0)
		{
			Span(Out, Plaster, FVector(X - 15, -Half, GatewayTop - 30), FVector(X + 15, Half, GatewayTop), false);
		}
		Span(Out, Plaster, FVector(-Depth, -Half, GatewayTop - 4), FVector(0, Half, GatewayTop), false);
		Rod(Out, Iron, FVector(-Depth * 0.5, 0, GatewayTop), FVector(-Depth * 0.5, 0, GatewayTop - 40), 2.0);
		Add(Out, MarketModel::Sphere, Lantern, FVector(-Depth * 0.5, 0, GatewayTop - 50), FVector(22, 22, 26), 0.0, false);
		Lamps.Add({ FVector(-Depth * 0.5, 0, GatewayTop - 60), 700.f });
	}

	// ---------------------------------------------------------------------
	// The courtyard, in a frame without a turn (X east, Y south, as the city).

	/** One courtyard wall, from P to Q, its house's height, and the way into the yard (degrees). */
	struct FYardWall
	{
		FVector2D P;
		FVector2D Q;
		double Height;
		double Inward;
	};

	/** Something standing in the yard: pump, rack, line, bench, bins, tree. */
	struct FYardThing
	{
		FString Kind;
		FVector2D At;
		double Yaw;
		double Length;
	};

	constexpr double GangDeep = 110.0;
	constexpr double FloorHigh = 380.0;
	constexpr double FirstGang = 450.0;

	inline void YardWall(const FYardWall& W, const FVector2D& ExitAt, double ExitHalf, uint32 Seed, TArray<FShape>& Out)
	{
		using namespace Colors;
		const FVector2D Run = W.Q - W.P;
		const double Length = Run.Size();
		if (Length < 250.0)
		{
			return;
		}
		const FVector2D D = Run / Length;
		const FVector2D N(FMath::Cos(FMath::DegreesToRadians(W.Inward)), FMath::Sin(FMath::DegreesToRadians(W.Inward)));
		const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X));
		auto At = [&](double Along, double Out_, double Z) { const FVector2D P = W.P + D * Along + N * Out_; return FVector(P.X, P.Y, Z); };

		MarketModel::FRandom R(Seed ^ 0x9e3779b9u);
		// Floors: the ground floor, then a gang at each upper floor while the house is tall enough.
		for (int32 Floor = 0; ; ++Floor)
		{
			const double Z = Floor == 0 ? 0.0 : FirstGang + (Floor - 1) * FloorHigh;
			if (Floor > 0 && (Z > W.Height - 300.0 || Length < 300.0 || W.Height < 800.0))
			{
				break;
			}
			if (Floor > 0)
			{
				// The stone slab on brackets, the iron railing on its edge.
				Beam(Out, StoneDark, At(Length * 0.5, GangDeep * 0.5, Z - 7), Yaw, Length - 10.0, GangDeep, 14.0);
				Beam(Out, Iron, At(Length * 0.5, GangDeep - 3, Z + 100), Yaw, Length - 10.0, 4.0, 4.0);
				Beam(Out, Iron, At(Length * 0.5, GangDeep - 3, Z + 15), Yaw, Length - 10.0, 3.0, 3.0);
				for (double S = 20.0; S < Length - 10.0; S += 16.0)
				{
					Beam(Out, Iron, At(S, GangDeep - 3, Z + 57), Yaw, 1.5, 1.5, 84.0);
				}
				for (double S = 60.0; S < Length - 30.0; S += 240.0)
				{
					Add(Out, MarketModel::Cube, StoneDark, At(S, 35.0, Z - 34), FVector(10, 70, 40), Yaw, false, 0.0, 0.0);
				}
				// A geranium pot here and there on the railing.
				for (double S = 120.0; S < Length - 80.0; S += 300.0 + R.Range(0.0, 400.0))
				{
					Add(Out, MarketModel::Cylinder, Clay, At(S, GangDeep + 8, Z + 110), FVector(20, 20, 18), 0.0, false);
					Add(Out, MarketModel::Sphere, R.Range(0.0, 1.0) < 0.6 ? Geranium : Leaves, At(S, GangDeep + 8, Z + 128), FVector(30, 30, 22), 0.0, false);
				}
			}
			// Flat doors and windows painted along the wall, one door to two windows.
			int32 Count = 0;
			for (double S = 120.0; S < Length - 100.0; S += 170.0, ++Count)
			{
				const FVector Foot = At(S, 0.0, Z);
				if (Floor == 0 && FVector2D::Distance(FVector2D(Foot.X, Foot.Y), ExitAt) < ExitHalf + 160.0)
				{
					continue;
				}
				if (Count % 3 == 0)
				{
					Beam(Out, Frame, At(S, 2.0, Z + 115), Yaw, 104.0, 4.0, 234.0);
					Beam(Out, Floor == 0 ? Door : DoorGreen, At(S, 4.0, Z + 112), Yaw, 90.0, 4.0, 222.0);
					Beam(Out, Glass, At(S, 6.0, Z + 180), Yaw, 60.0, 2.0, 60.0);
				}
				else
				{
					Beam(Out, Frame, At(S, 2.0, Z + 165), Yaw, 104.0, 4.0, 150.0);
					Beam(Out, Glass, At(S, 4.0, Z + 165), Yaw, 90.0, 4.0, 136.0);
					Beam(Out, Frame, At(S, 6.0, Z + 165), Yaw, 90.0, 2.0, 5.0);
				}
			}
		}
	}

	inline void YardThing(const FYardThing& T, TArray<FShape>& Out)
	{
		using namespace Colors;
		const FVector Base(T.At.X, T.At.Y, 0.0);
		const FRotator Turn(0.0, T.Yaw, 0.0);
		auto At = [&](double X, double Y, double Z) { return Base + Turn.RotateVector(FVector(X, Y, Z)); };
		if (T.Kind == TEXT("pump"))
		{
			// The yard's cast iron pump on a stone drain plate.
			Add(Out, MarketModel::Cube, StoneDark, At(0, 0, 4), FVector(120, 120, 8), T.Yaw);
			Add(Out, MarketModel::Cylinder, Iron, At(0, 0, 70), FVector(26, 26, 124));
			Add(Out, MarketModel::Sphere, Iron, At(0, 0, 138), FVector(30, 30, 26), 0.0, false);
			Add(Out, MarketModel::Cone, Iron, At(0, 0, 158), FVector(12, 12, 18), 0.0, false);
			Rod(Out, Iron, At(10, 0, 95), At(42, 0, 82), 7.0);
			Rod(Out, Iron, At(-10, 0, 120), At(-60, 0, 150), 4.0);
			Add(Out, MarketModel::Cube, Zinc, At(48, 0, 30), FVector(34, 30, 24), T.Yaw, false);
		}
		else if (T.Kind == TEXT("rack"))
		{
			// The carpet beating rack (porolo) with a rug over it.
			for (const double Y : { -125.0, 125.0 })
			{
				Rod(Out, Iron, At(0, Y, 0), At(0, Y, 180), 6.0, true);
			}
			Rod(Out, Iron, At(0, -130, 176), At(0, 130, 176), 5.0);
			for (const double S : { -1.0, 1.0 })
			{
				Add(Out, MarketModel::Cube, Carpet, At(S * 12, 0, 128), FVector(3, 190, 96), T.Yaw, false, 0.0, 0.0);
				Add(Out, MarketModel::Cube, CarpetBand, At(S * 14, 0, 100), FVector(1, 170, 10), T.Yaw, false);
			}
		}
		else if (T.Kind == TEXT("line"))
		{
			// A washing line on two poles with the washing pegged on it.
			const double Half = T.Length * 0.5;
			for (const double S : { -Half, Half })
			{
				Rod(Out, Wood, At(S, 0, 0), At(S, 0, 225), 7.0, true);
			}
			Rod(Out, Paper * 0.7f, At(-Half, 0, 215), At(Half, 0, 215), 1.0);
			const FLinearColor Washing[] = { Paper, FLinearColor(0.55f, 0.62f, 0.75f), PaperPink, Paper, FLinearColor(0.75f, 0.72f, 0.60f) };
			int32 i = 0;
			for (double S = -Half + 60.0; S < Half - 50.0; S += 85.0, ++i)
			{
				const double Long = i % 3 == 1 ? 90.0 : 60.0;
				Add(Out, MarketModel::Cube, Washing[i % 5], At(S, 0, 213 - Long * 0.5), FVector(i % 2 ? 50 : 66, 1.5, Long), T.Yaw, false);
			}
		}
		else if (T.Kind == TEXT("bins"))
		{
			for (const double Y : { -40.0, 40.0 })
			{
				Add(Out, MarketModel::Cylinder, Zinc, At(0, Y, 40), FVector(56, 56, 80));
				Add(Out, MarketModel::Cylinder, Zinc * 0.8f, At(0, Y, 83), FVector(62, 62, 6), 0.0, false);
			}
		}
		else if (T.Kind == TEXT("bench"))
		{
			Add(Out, MarketModel::Cube, Wood, At(0, 0, 45), FVector(45, 160, 6), T.Yaw);
			Add(Out, MarketModel::Cube, Wood, At(-20, 0, 80), FVector(5, 160, 30), T.Yaw, false);
			for (const double Y : { -65.0, 65.0 })
			{
				Add(Out, MarketModel::Cube, Iron, At(0, Y, 22), FVector(40, 5, 44), T.Yaw, false);
			}
		}
		else if (T.Kind == TEXT("tree"))
		{
			Rod(Out, Bark, At(0, 0, 0), At(0, 0, 420), 30.0, true);
			Rod(Out, Bark, At(0, 0, 300), At(90, 40, 520), 14.0);
			Rod(Out, Bark, At(0, 0, 320), At(-80, -30, 540), 14.0);
			Add(Out, MarketModel::Sphere, LeavesGold, At(0, 0, 600), FVector(380, 380, 300), 0.0, false);
			Add(Out, MarketModel::Sphere, Leaves, At(100, 60, 530), FVector(240, 240, 200), 0.0, false);
			Add(Out, MarketModel::Sphere, LeavesGold, At(-90, -50, 560), FVector(240, 240, 210), 0.0, false);
		}
	}
}
