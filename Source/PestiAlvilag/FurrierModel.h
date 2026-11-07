// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BozsiHouseModel.h"
#include "MarketModel.h"
#include "PersonKit.h"

/**
 * The things of mission 8, "A bunda" (the fur coat):
 * - Friedmann's furrier's shop on Rakoczi ut: a dark green shop front with
 *   two lit windows of furs on dummies, a striped awning, and out on the
 *   pavement a table with a fur coat folded on it (the one the player
 *   grabs), a rack of coats being aired and a board offering summer storage
 *   for furs (which is how the inspector's wife's own coat comes to be there);
 * - the fur coat folded over the player's arms as he carries it;
 * - a public telephone kiosk on Tisza Kalman ter, where the employer rings;
 * - the people: Friedmann the furrier, and Kardosne, the police inspector's wife.
 *
 * Header-only, as basic shapes (MarketModel::FShape). The shop's frame: the
 * house wall along Y at X = 0, +X out across the pavement to the street,
 * Z up from the pavement. The kiosk's frame: its door faces +X.
 */
namespace FurrierModel
{
	using MarketModel::FShape;
	using MarketModel::FRandom;
	using BozsiHouseModel::FSign;
	using BozsiHouseModel::FLamp;
	using BozsiHouseModel::Add;
	using BozsiHouseModel::Span;

	namespace Colors
	{
		const FLinearColor Front(0.05f, 0.13f, 0.09f);
		const FLinearColor FrontDark(0.03f, 0.07f, 0.05f);
		const FLinearColor Gilt(0.72f, 0.52f, 0.17f);
		const FLinearColor Glass(0.10f, 0.12f, 0.13f);
		const FLinearColor Inside(0.30f, 0.22f, 0.14f);
		const FLinearColor Velvet(0.30f, 0.05f, 0.06f);
		const FLinearColor DummyCloth(0.62f, 0.55f, 0.45f);
		const FLinearColor Mink(0.20f, 0.12f, 0.07f);
		const FLinearColor Sable(0.10f, 0.07f, 0.05f);
		const FLinearColor Fox(0.55f, 0.25f, 0.08f);
		const FLinearColor Astrakhan(0.06f, 0.06f, 0.06f);
		const FLinearColor Beaver(0.32f, 0.22f, 0.14f);
		const FLinearColor Lining(0.55f, 0.42f, 0.25f);
		const FLinearColor StripeA(0.78f, 0.74f, 0.62f);
		const FLinearColor StripeB(0.10f, 0.26f, 0.16f);
		const FLinearColor Table(0.26f, 0.15f, 0.08f);
		const FLinearColor Cloth(0.60f, 0.58f, 0.52f);
		const FLinearColor Iron(0.05f, 0.05f, 0.055f);
		const FLinearColor Board(0.08f, 0.08f, 0.08f);
		const FLinearColor Chalk(0.86f, 0.84f, 0.78f);
		const FLinearColor Step(0.50f, 0.48f, 0.44f);
		const FLinearColor KioskGreen(0.08f, 0.20f, 0.13f);
		const FLinearColor KioskRoof(0.05f, 0.10f, 0.08f);
		const FLinearColor Sign(0.80f, 0.74f, 0.55f);
		const FLinearColor Lettering(0.06f, 0.05f, 0.04f);
		const FLinearColor Globe(0.95f, 0.88f, 0.70f);
		const FLinearColor Black(0.02f, 0.02f, 0.02f);
		const FLinearColor Brass(0.62f, 0.45f, 0.16f);
	}

	/** Where the shop stands: the foot of its front in the middle, on Rakoczi ut's south side, and the way out to the street (degrees). */
	const FVector2D CityAt(41134.0, -21532.0);
	constexpr double CityYaw = -110.9;

	/** The telephone kiosk on Tisza Kalman ter, by the park railings, its door to the street. */
	const FVector2D KioskAt(58498.0, 14439.0);
	constexpr double KioskYaw = 69.6;

	// Sizes (cm).
	constexpr double HalfWidth = 360.0;   // the shop front, Y -360..360
	constexpr double FrontDepth = 60.0;   // how far the shop front stands out from the wall
	constexpr double FrontTop = 400.0;
	constexpr double TableX = 200.0;      // the table on the pavement, its middle
	constexpr double TableY = -200.0;

	/** Where things are, in the shop's frame. */
	namespace Spots
	{
		/** The coat folded on the table (its middle, on the table top). */
		const FVector Coat(TableX, TableY, 92.0);
		/** Friedmann, behind the table with his back to his door. */
		const FVector Furrier(118.0, TableY + 20.0, 0.0);
		constexpr double FurrierYaw = 0.0;
		/** Where the player stands to take the coat, on the street side of the table. */
		const FVector Grab(TableX + 95.0, TableY, 0.0);
	}

	/** Friedmann Bela, the furrier: past fifty, grey waistcoat and sleeves, a tape measure round his neck, spectacles' worth of moustache, no hat. */
	inline PersonKit::FOutfit Furrier()
	{
		PersonKit::FOutfit O;
		O.Height = 170.f;
		O.Build = 1.12f;
		O.Hat = PersonKit::EHat::None;
		O.Top = PersonKit::ETop::Waistcoat;
		O.bTie = true;
		O.bMoustache = true;
		O.bScarf = true;
		O.Scarf = FLinearColor(0.80f, 0.72f, 0.40f);
		O.Waistcoat = FLinearColor(0.22f, 0.22f, 0.24f);
		O.Trousers = FLinearColor(0.12f, 0.12f, 0.13f);
		O.Shirt = FLinearColor(0.86f, 0.85f, 0.82f);
		O.Tie = FLinearColor(0.10f, 0.08f, 0.20f);
		O.Hair = FLinearColor(0.52f, 0.50f, 0.48f);
		return O;
	}

	/**
	 * Kardosne, the police inspector's wife: tall and well kept, a plum
	 * cloche, a camel coat with a fox collar, kid gloves, a handbag. With
	 * bInFur she wears the coat she has just bought back.
	 */
	inline PersonKit::FOutfit Lady(bool bInFur)
	{
		PersonKit::FOutfit O;
		O.bWoman = true;
		O.Height = 170.f;
		O.Hat = PersonKit::EHat::Cloche;
		O.HatColor = FLinearColor(0.24f, 0.05f, 0.16f);
		O.HatBand = FLinearColor(0.70f, 0.62f, 0.40f);
		O.Top = PersonKit::ETop::Overcoat;
		O.Coat = bInFur ? Colors::Mink : FLinearColor(0.50f, 0.38f, 0.24f);
		O.bScarf = true;
		O.Scarf = bInFur ? Colors::Sable : Colors::Fox;
		O.Held = PersonKit::EHeld::Briefcase;
		O.HeldColor = FLinearColor(0.08f, 0.05f, 0.04f);
		O.Hair = FLinearColor(0.30f, 0.18f, 0.08f);
		O.Shoes = FLinearColor(0.10f, 0.04f, 0.03f);
		O.Skin = FLinearColor(0.78f, 0.58f, 0.46f);
		return O;
	}

	/** A fur coat hanging (from its shoulders at At): a long bell of fur, a collar, sleeves. */
	inline void HangingCoat(TArray<FShape>& Out, const FVector& At, const FLinearColor& Fur, double Yaw)
	{
		const FRotator Turn(0.0, Yaw, 0.0);
		// The body flaring out to the hem, square shoulders, a collar, sleeves hanging at the sides.
		Add(Out, MarketModel::Cylinder, Fur, At - FVector(0, 0, 66), FVector(54, 40, 110), Yaw, false);
		Add(Out, MarketModel::Cylinder, Fur, At - FVector(0, 0, 104), FVector(68, 50, 34), Yaw, false);
		Add(Out, MarketModel::Cube, Fur, At - FVector(0, 0, 10), FVector(52, 30, 16), Yaw, false);
		Add(Out, MarketModel::Sphere, Fur, At + FVector(0, 0, 0), FVector(30, 34, 14), Yaw, false);
		for (const double Side : { -1.0, 1.0 })
		{
			Add(Out, MarketModel::Cylinder, Fur, At + Turn.RotateVector(FVector(0, Side * 30, -44)), FVector(17, 17, 62), Yaw, false);
		}
		Add(Out, MarketModel::Cylinder, Colors::Iron, At + FVector(0, 0, 8), FVector(2, 2, 16), 0.0, false);
	}

	/** The coat folded, lying on something at At (its middle, the underside). */
	inline void FoldedCoat(TArray<FShape>& Out, const FVector& At, double Yaw)
	{
		Add(Out, MarketModel::Cube, Colors::Mink, At + FVector(0, 0, 8), FVector(56, 44, 16), Yaw, false);
		Add(Out, MarketModel::Cube, Colors::Lining, At + FVector(0, 0, 16.5), FVector(40, 30, 1), Yaw, false);
		Add(Out, MarketModel::Cylinder, Colors::Sable, At + FRotator(0.0, Yaw, 0.0).RotateVector(FVector(-24, 0, 16)), FVector(16, 16, 46), Yaw, false, 0.0, 90.0);
	}

	/** The coat carried over the arms, in the holder's frame (the middle of the bundle at the origin, +X forward). */
	inline void CarriedCoat(TArray<FShape>& Out)
	{
		Add(Out, MarketModel::Cube, Colors::Mink, FVector(0, 0, 0), FVector(30, 50, 22), 0.0, false);
		Add(Out, MarketModel::Cube, Colors::Mink, FVector(2, 0, -26), FVector(26, 46, 36), 0.0, false, -8.0);
		Add(Out, MarketModel::Cylinder, Colors::Sable, FVector(-6, 0, 12), FVector(18, 18, 54), 0.0, false, 0.0, 90.0);
		Add(Out, MarketModel::Cube, Colors::Lining, FVector(15.5, 0, -18), FVector(1, 30, 20), 0.0, false);
	}

	/** A shop dummy in the window, wearing a fur, at At (its foot). */
	inline void Dummy(TArray<FShape>& Out, const FVector& At, const FLinearColor& Fur)
	{
		Add(Out, MarketModel::Cylinder, Colors::Iron, At + FVector(0, 0, 4), FVector(30, 30, 8), 0.0, false);
		Add(Out, MarketModel::Cylinder, Colors::Iron, At + FVector(0, 0, 40), FVector(3, 3, 70), 0.0, false);
		HangingCoat(Out, At + FVector(0, 0, 175), Fur, 0.0);
		Add(Out, MarketModel::Sphere, Colors::DummyCloth, At + FVector(0, 0, 192), FVector(20, 18, 24), 0.0, false);
	}

	/** The shop front, standing out from the house wall, the windows and what is in them. */
	inline void BuildFront(TArray<FShape>& Out, TArray<FLamp>& Lamps)
	{
		using namespace Colors;
		const double W = HalfWidth;
		const double D = FrontDepth;
		// The frame: pilasters, the fascia box with its gilt edges, a stall riser under each window.
		for (const double Y : { -W, -70.0, 70.0, W })
		{
			Span(Out, Front, FVector(0, Y - 18, 0), FVector(D + 4, Y + 18, 300));
		}
		Span(Out, Front, FVector(0, -W - 22, 290), FVector(D + 6, W + 22, FrontTop));
		Span(Out, Gilt, FVector(D + 6, -W - 22, 296), FVector(D + 8, W + 22, 300), false);
		Span(Out, Gilt, FVector(D + 6, -W - 22, FrontTop - 6), FVector(D + 8, W + 22, FrontTop - 2), false);
		Span(Out, FrontDark, FVector(D + 6, -W + 30, 312), FVector(D + 7, W - 30, FrontTop - 14), false);
		for (const double Side : { -1.0, 1.0 })
		{
			const double Y0 = Side < 0 ? -W + 18 : 70.0 + 18;
			const double Y1 = Side < 0 ? -70.0 - 18 : W - 18;
			const double Mid = (Y0 + Y1) * 0.5;
			Span(Out, FrontDark, FVector(0, Y0, 0), FVector(D, Y1, 60));
			Span(Out, Gilt, FVector(D, Mid - 50, 22), FVector(D + 1, Mid + 50, 44), false);
			// The display: a lit back, a velvet floor, two dummies in furs, a fox stole.
			Span(Out, Inside, FVector(0, Y0, 60), FVector(2, Y1, 290), false);
			Span(Out, Velvet, FVector(0, Y0, 58), FVector(D - 4, Y1, 62), false);
			Dummy(Out, FVector(22, Mid - 55, 62), Side < 0 ? Mink : Sable);
			Dummy(Out, FVector(22, Mid + 55, 62), Side < 0 ? Astrakhan : Beaver);
			Add(Out, MarketModel::Cylinder, Fox, FVector(D - 18, Mid, 70), FVector(14, 14, 100), 0.0, false, 0.0, 90.0);
			Add(Out, MarketModel::Sphere, Fox, FVector(D - 18, Mid - 56, 74), FVector(16, 20, 14), 0.0, false);
			// The window's glazing bars (the glass itself is left out, so the furs show); close enough to keep people out.
			for (double Y = Y0 + 48.0; Y < Y1 - 10.0; Y += 48.0)
			{
				Span(Out, FrontDark, FVector(D - 3, Y - 2, 60), FVector(D, Y + 2, 290), true);
			}
			Span(Out, FrontDark, FVector(D - 3, Y0, 230), FVector(D, Y1, 234), false);
			// Window lamps.
			Add(Out, MarketModel::Sphere, Globe, FVector(D * 0.5, Mid, 282), FVector(12, 12, 12), 0.0, false);
			Lamps.Add({ FVector(D * 0.5, Mid, 270), 380.f });
		}
		// The door between, set back at the wall, glazed, with a brass handle and a step.
		Span(Out, FrontDark, FVector(-2, -52, 0), FVector(4, 52, 260), false);
		Span(Out, Inside, FVector(4, -40, 110), FVector(5, 40, 245), false);
		Span(Out, Brass, FVector(5, 30, 110), FVector(9, 38, 118), false);
		Span(Out, Step, FVector(0, -70, 0), FVector(D, 70, 12), false);
		// The striped awning, rolled out from under the fascia over the pavement.
		const double Out0 = D + 6.0, Out1 = D + 230.0, Z0 = 290.0, Z1 = 235.0;
		const double Pitch = -FMath::RadiansToDegrees(FMath::Atan2(Z0 - Z1, Out1 - Out0));
		const double Len = FMath::Sqrt((Out1 - Out0) * (Out1 - Out0) + (Z0 - Z1) * (Z0 - Z1));
		int32 k = 0;
		for (double Y = -W - 20; Y < W + 20 - 1; Y += 37.0, ++k)
		{
			Add(Out, MarketModel::Cube, k % 2 ? StripeB : StripeA, FVector((Out0 + Out1) * 0.5, Y + 18.5, (Z0 + Z1) * 0.5), FVector(Len, 37, 3), 0.0, false, Pitch);
			Add(Out, MarketModel::Cube, k % 2 ? StripeB : StripeA, FVector(Out1, Y + 18.5, Z1 - 14), FVector(2, 37, 28), 0.0, false);
		}
		// Iron arms holding it.
		for (const double Y : { -W, W })
		{
			Add(Out, MarketModel::Cube, Iron, FVector((Out0 + Out1) * 0.5, Y, (Z0 + Z1) * 0.5 - 4), FVector(Len, 3, 3), 0.0, false, Pitch);
		}
	}

	/** Out on the pavement: the table with the folded coat, the rack of coats being aired, the storage board. */
	inline void BuildPavement(TArray<FShape>& Out)
	{
		using namespace Colors;
		// The table: a top with a cloth on it, four turned legs.
		Add(Out, MarketModel::Cube, Table, FVector(TableX, TableY, 86), FVector(70, 170, 6));
		Add(Out, MarketModel::Cube, Cloth, FVector(TableX, TableY, 89.5), FVector(76, 176, 1), 0.0, false);
		Add(Out, MarketModel::Cube, Cloth, FVector(TableX + 38, TableY, 72), FVector(1, 176, 36), 0.0, false);
		for (const double DX : { -28.0, 28.0 })
		{
			for (const double DY : { -76.0, 76.0 })
			{
				Add(Out, MarketModel::Cylinder, Table, FVector(TableX + DX, TableY + DY, 42), FVector(6, 6, 84), 0.0, true);
			}
		}
		// A second fur laid over the far end, and a tape measure and a ledger.
		Add(Out, MarketModel::Cube, Colors::Beaver, FVector(TableX - 5, TableY + 58, 95), FVector(50, 44, 10), 12.0, false);
		Add(Out, MarketModel::Cylinder, Colors::Sign, FVector(TableX + 18, TableY - 70, 91), FVector(10, 10, 3), 0.0, false);
		Add(Out, MarketModel::Cube, Colors::Black, FVector(TableX - 18, TableY - 68, 92), FVector(22, 30, 4), 8.0, false);

		// The rack: two iron posts and a rail, three coats airing.
		const double RackX = 175.0, RackY0 = 60.0, RackY1 = 300.0, RackTop = 185.0;
		for (const double Y : { RackY0, RackY1 })
		{
			Add(Out, MarketModel::Cylinder, Iron, FVector(RackX, Y, RackTop * 0.5), FVector(4, 4, RackTop), 0.0, true);
			Add(Out, MarketModel::Cube, Iron, FVector(RackX, Y, 3), FVector(50, 6, 6), 0.0, false);
		}
		Add(Out, MarketModel::Cylinder, Iron, FVector(RackX, (RackY0 + RackY1) * 0.5, RackTop), FVector(3, 3, RackY1 - RackY0 + 10), 0.0, false, 0.0, 90.0);
		const FLinearColor Furs[] = { Astrakhan, Fox, Beaver };
		for (int32 i = 0; i < 3; ++i)
		{
			HangingCoat(Out, FVector(RackX, RackY0 + 50 + i * 70, RackTop - 18), Furs[i], 90.0);
		}

		// The A-board: "furs stored over the summer".
		const double BoardX = 305.0, BoardY = -330.0;
		for (const double Lean : { -12.0, 12.0 })
		{
			Add(Out, MarketModel::Cube, Board, FVector(BoardX + Lean * 1.4, BoardY, 55), FVector(3, 60, 110), 0.0, false, Lean);
		}
		Add(Out, MarketModel::Cube, Table, FVector(BoardX - 17, BoardY, 55), FVector(1, 64, 114), 0.0, false, 12.0);
	}

	/** The public telephone kiosk: green iron and glass, a domed roof, the sign over the door; the door (+X) stands open. */
	inline void Kiosk(TArray<FShape>& Out, TArray<FLamp>& Lamps)
	{
		using namespace Colors;
		const double H = 50.0, Top = 240.0;
		Span(Out, Step, FVector(-H - 8, -H - 8, 0), FVector(H + 8, H + 8, 10));
		for (const double X : { -H, H })
		{
			for (const double Y : { -H, H })
			{
				Add(Out, MarketModel::Cube, KioskGreen, FVector(X, Y, Top * 0.5 + 5), FVector(8, 8, Top - 10));
			}
		}
		// Back and sides: a panel below, glass above.
		const auto Side = [&](const FVector& A, const FVector& B)
		{
			Span(Out, KioskGreen, FVector(A.X, A.Y, 10), FVector(B.X, B.Y, 100));
			Span(Out, Glass, FVector(A.X, A.Y, 100), FVector(B.X, B.Y, Top - 20), false);
			Span(Out, KioskGreen, FVector(A.X, A.Y, Top - 20), FVector(B.X, B.Y, Top), false);
			Span(Out, KioskGreen, FVector((A.X + B.X) * 0.5 - 2, (A.Y + B.Y) * 0.5 - 2, 100), FVector((A.X + B.X) * 0.5 + 2, (A.Y + B.Y) * 0.5 + 2, Top - 20), false);
		};
		Side(FVector(-H - 2, -H, 0), FVector(-H + 2, H, 0));
		Side(FVector(-H, -H - 2, 0), FVector(H, -H + 2, 0));
		Side(FVector(-H, H - 2, 0), FVector(H, H + 2, 0));
		Span(Out, Glass, FVector(-H - 2, -H, 100), FVector(-H + 2, H, Top - 20), true);
		// The door, swung open outwards.
		Add(Out, MarketModel::Cube, KioskGreen, FVector(H + 40, H + 3, 125), FVector(80, 4, 210), 0.0, false);
		Add(Out, MarketModel::Cube, Glass, FVector(H + 40, H + 6, 150), FVector(60, 1, 120), 0.0, false);
		// The roof: a cap and a little dome with a finial.
		Span(Out, KioskRoof, FVector(-H - 12, -H - 12, Top), FVector(H + 12, H + 12, Top + 12), false);
		Add(Out, MarketModel::Sphere, KioskRoof, FVector(0, 0, Top + 12), FVector(110, 110, 60), 0.0, false);
		Add(Out, MarketModel::Cone, Brass, FVector(0, 0, Top + 50), FVector(10, 10, 26), 0.0, false);
		// The sign boards on all four sides.
		for (int32 i = 0; i < 4; ++i)
		{
			const FRotator Turn(0.0, i * 90.0, 0.0);
			Add(Out, MarketModel::Cube, Sign, FVector(0, 0, Top - 10) + Turn.RotateVector(FVector(H + 4, 0, 0)), FVector(2, 80, 18), i * 90.0, false);
		}
		// Inside: the telephone on the back wall, a little shelf, a lamp in the roof.
		Span(Out, Black, FVector(-H + 2, -12, 135), FVector(-H + 14, 12, 170), false);
		Add(Out, MarketModel::Cylinder, Black, FVector(-H + 20, 14, 150), FVector(5, 5, 18), 0.0, false);
		Span(Out, Table, FVector(-H + 2, -30, 112), FVector(-H + 28, 30, 116), false);
		Add(Out, MarketModel::Sphere, Globe, FVector(0, 0, Top - 26), FVector(14, 14, 14), 0.0, false);
		Lamps.Add({ FVector(0, 0, Top - 40), 500.f });
	}

	inline void Build(TArray<FShape>& Out, TArray<FLamp>& Lamps)
	{
		BuildFront(Out, Lamps);
		BuildPavement(Out);
	}

	inline void Signs(TArray<FSign>& Out)
	{
		const double Mid = (HalfWidth + 70.0) * 0.5;
		const double Board = 305.0 + 17.0 * 1.4 + 3.0;
		Out.Add({ TEXT("FRIEDMANN B\u00c9LA  \u00b7  SZ\u0170CS"), FVector(FrontDepth + 8.0, 0.0, 345.0), 0.0, 36.0, Colors::Gilt });
		Out.Add({ TEXT("BUND\u00c1K"), FVector(FrontDepth + 1.5, -Mid, 33.0), 0.0, 14.0, Colors::Lettering });
		Out.Add({ TEXT("PR\u00c9MEK"), FVector(FrontDepth + 1.5, Mid, 33.0), 0.0, 14.0, Colors::Lettering });
		Out.Add({ TEXT("BUND\u00c1K"), FVector(Board, -330.0, 82.0), 0.0, 11.0, Colors::Chalk });
		Out.Add({ TEXT("NY\u00c1RI"), FVector(Board, -330.0, 66.0), 0.0, 11.0, Colors::Chalk });
		Out.Add({ TEXT("MEG\u0150RZ\u00c9SE"), FVector(Board, -330.0, 50.0), 0.0, 9.0, Colors::Chalk });
	}

	inline void KioskSigns(TArray<FSign>& Out)
	{
		for (int32 i = 0; i < 4; ++i)
		{
			const FRotator Turn(0.0, i * 90.0, 0.0);
			Out.Add({ TEXT("TELEFON"), FVector(0, 0, 230) + Turn.RotateVector(FVector(55.5, 0, 0)), i * 90.0, 12.0, Colors::Lettering });
		}
	}
}
