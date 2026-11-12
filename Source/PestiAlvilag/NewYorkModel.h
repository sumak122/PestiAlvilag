// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CityDetailsModel.h"

/**
 * The New York palace on Erzsébet körút (1894, Hauszmann, Korb and Giergl),
 * with the New York café on its ground floor, as it stood in 1932: dark,
 * sooty stone, the café's tall arched windows under a gilt name, the
 * middle bays of the körút front standing forward with a round gable, the
 * café door between two bronze fauns holding lamps, balconies, pilasters,
 * a deep cornice with urns, and the corner tower on Dohány utca with its
 * slate dome and gilt lantern.
 *
 * ACityDetails dresses the plain map block's three street walls with it
 * (city_details.py "newyork": the körút front, the cut corner, the Dohány
 * utca front). Each piece is built in its own frame: +X out of the wall to
 * the street, Y along the wall from 0 to Length, Z up from the pavement;
 * Place turns it into the set's frame.
 *
 * Header-only so it can be built and looked at outside the engine.
 */
namespace NewYorkModel
{
	using MarketModel::FShape;
	using BozsiHouseModel::FSign;
	using BozsiHouseModel::FLamp;
	using BozsiHouseModel::Add;
	using BozsiHouseModel::Span;
	using CityDetailsModel::Rod;

	/** Which of the three street walls a piece is. */
	enum class EPiece : uint8 { Korut, Corner, Dohany };

	/** One street wall in the set's frame: where it starts at the foot, which way is out, how long. */
	struct FPiece
	{
		EPiece Part;
		FVector2D Start;
		double Yaw;
		double Length;
	};

	namespace Colors
	{
		const FLinearColor Stone(0.42f, 0.38f, 0.32f);        // sandstone gone grey with coal smoke
		const FLinearColor StoneDark(0.30f, 0.27f, 0.23f);
		const FLinearColor Trim(0.55f, 0.51f, 0.43f);
		const FLinearColor Plinth(0.18f, 0.17f, 0.16f);
		const FLinearColor Glass(0.04f, 0.05f, 0.06f);
		const FLinearColor CafeGlass(0.36f, 0.25f, 0.12f);    // the café's lit windows
		const FLinearColor Bronze(0.16f, 0.11f, 0.06f);
		const FLinearColor Gilt(0.85f, 0.64f, 0.24f);
		const FLinearColor Iron(0.035f, 0.04f, 0.04f);
		const FLinearColor Slate(0.09f, 0.10f, 0.11f);
		const FLinearColor Globe(0.95f, 0.86f, 0.60f);
		const FLinearColor Door(0.20f, 0.11f, 0.05f);
	}

	// Heights (cm) of the front's bands.
	constexpr double PlinthTop = 60.0;
	constexpr double ArchSpring = 560.0;     // where the café windows' round tops start
	constexpr double BandAt = 800.0;         // the band over the café
	constexpr double BandTop = 850.0;
	constexpr double FloorHeight = 320.0;
	constexpr int32 UpperFloors = 4;
	constexpr double CorniceAt = BandTop + FloorHeight * UpperFloors;   // 2130
	constexpr double CorniceTop = CorniceAt + 70.0;
	constexpr double AtticTop = CorniceTop + 130.0;
	constexpr double EdgeMargin = 40.0;
	constexpr double PierWidth = 70.0;
	/** How far the middle bays of the körút front stand forward. */
	constexpr double BaysOut = 45.0;
	/** How many café bays the Dohány utca front has, from the corner. */
	constexpr int32 DohanyCafeBays = 4;

	/** A piece's shapes, lamps and lettering moved into the set's frame. */
	inline void Place(const TArray<FShape>& Local, const TArray<FLamp>& LocalLamps, const TArray<FSign>& LocalSigns,
		const FPiece& Piece, TArray<FShape>& Out, TArray<FLamp>& Lamps, TArray<FSign>& Signs)
	{
		const FRotator Turn(0.0, Piece.Yaw, 0.0);
		const FVector Origin(Piece.Start, 0.0);
		for (const FShape& One : Local)
		{
			FShape Moved = One;
			Moved.At = Origin + Turn.RotateVector(One.At);
			// The turn is about Z, the outermost of a rotator's turns, so it simply adds to the yaw.
			Moved.Yaw = One.Yaw + Piece.Yaw;
			Out.Add(Moved);
		}
		for (const FLamp& One : LocalLamps)
		{
			Lamps.Add({ Origin + Turn.RotateVector(One.At), One.Radius });
		}
		for (const FSign& One : LocalSigns)
		{
			Signs.Add({ One.Text, Origin + Turn.RotateVector(One.At), One.Yaw + Piece.Yaw, One.Height, One.Color });
		}
	}

	/** A window with a round top on the wall at X, its glass Width wide from Z0 up to where the round top starts. */
	inline void ArchWindow(TArray<FShape>& Out, double X, double Y, double Width, double Z0, double Spring, const FLinearColor& Pane)
	{
		using namespace Colors;
		// The stone frame round it, then the glass, the round top of each (a disc standing on the wall).
		Span(Out, Trim, FVector(X, Y - Width * 0.5 - 18, Z0 - 10), FVector(X + 6, Y + Width * 0.5 + 18, Spring), false);
		Add(Out, MarketModel::Cylinder, Trim, FVector(X + 3, Y, Spring), FVector(Width + 36, Width + 36, 6), 0.0, false, 90.0);
		Span(Out, Pane, FVector(X + 6, Y - Width * 0.5, Z0), FVector(X + 9, Y + Width * 0.5, Spring), false);
		Add(Out, MarketModel::Cylinder, Pane, FVector(X + 6.5, Y, Spring), FVector(Width, Width, 3), 0.0, false, 90.0);
		// A keystone at the top.
		Span(Out, Trim, FVector(X, Y - 22, Spring + Width * 0.5 - 6), FVector(X + 16, Y + 22, Spring + Width * 0.5 + 40), false);
	}

	/** One upper floor window: frame, glass, sill, and a hood or pediment by floor. */
	inline void UpperWindow(TArray<FShape>& Out, double X, double Y, int32 Floor, bool bBalcony)
	{
		using namespace Colors;
		const double Z0 = BandTop + Floor * FloorHeight;
		const double Width = Floor == UpperFloors - 1 ? 130.0 : 150.0;
		const double Sill = Z0 + 55.0;
		const double Top = Sill + (Floor == UpperFloors - 1 ? 170.0 : 205.0);
		Span(Out, Trim, FVector(X, Y - Width * 0.5 - 16, Sill), FVector(X + 6, Y + Width * 0.5 + 16, Top + 14), false);
		Span(Out, Glass, FVector(X + 6, Y - Width * 0.5, Sill + 8), FVector(X + 9, Y + Width * 0.5, Top), false);
		Span(Out, Trim, FVector(X + 8, Y - 3, Sill + 8), FVector(X + 10, Y + 3, Top), false);
		Span(Out, Trim, FVector(X, Y - Width * 0.5 - 26, Sill - 12), FVector(X + 18, Y + Width * 0.5 + 26, Sill), false);
		if (Floor == 0 || Floor == 2)
		{
			// A three-cornered pediment on a ledge.
			Span(Out, Trim, FVector(X, Y - Width * 0.5 - 30, Top + 14), FVector(X + 20, Y + Width * 0.5 + 30, Top + 28), false);
			Add(Out, MarketModel::Cone, Trim, FVector(X + 8, Y, Top + 58), FVector(10, Width + 70, 64), 0.0, false);
		}
		else if (Floor == 1)
		{
			// A straight hood on two little brackets.
			Span(Out, Trim, FVector(X, Y - Width * 0.5 - 24, Top + 14), FVector(X + 24, Y + Width * 0.5 + 24, Top + 30), false);
			for (const double S : { -1.0, 1.0 })
			{
				Span(Out, Trim, FVector(X, Y + S * (Width * 0.5 + 10) - 6, Top - 20), FVector(X + 18, Y + S * (Width * 0.5 + 10) + 6, Top + 14), false);
			}
		}
		if (bBalcony)
		{
			// A stone balcony slab with an iron railing.
			const double Half = Width * 0.5 + 45.0;
			Span(Out, Trim, FVector(X, Y - Half, Z0 + 30), FVector(X + 85, Y + Half, Z0 + 48));
			Span(Out, StoneDark, FVector(X, Y - Half + 10, Z0 + 10), FVector(X + 60, Y + Half - 10, Z0 + 30), false);
			Span(Out, Iron, FVector(X + 78, Y - Half, Z0 + 125), FVector(X + 84, Y + Half, Z0 + 131), false);
			for (double R = -Half; R <= Half + 1.0; R += 20.0)
			{
				Span(Out, Iron, FVector(X + 79, Y + R - 1.5, Z0 + 48), FVector(X + 83, Y + R + 1.5, Z0 + 125), false);
			}
			for (const double S : { -1.0, 1.0 })
			{
				Span(Out, Iron, FVector(X, Y + S * Half - 1.5, Z0 + 125), FVector(X + 84, Y + S * Half + 1.5, Z0 + 131), false);
			}
		}
	}

	/** A bronze faun on a pedestal holding up a lamp globe, at Y in front of the wall. */
	inline void Faun(TArray<FShape>& Out, TArray<FLamp>& Lamps, double X, double Y, double Lean)
	{
		using namespace Colors;
		Span(Out, StoneDark, FVector(X - 30, Y - 30, 0), FVector(X + 30, Y + 30, 120));
		Span(Out, Trim, FVector(X - 34, Y - 34, 120), FVector(X + 34, Y + 34, 132), false);
		// Goat legs, body, head with little horns, one arm up with the lamp.
		Add(Out, MarketModel::Cylinder, Bronze, FVector(X, Y - 9, 172), FVector(16, 16, 80), 0.0, false, 0.0, -8.0);
		Add(Out, MarketModel::Cylinder, Bronze, FVector(X, Y + 9, 172), FVector(16, 16, 80), 0.0, false, 0.0, 8.0);
		Add(Out, MarketModel::Cylinder, Bronze, FVector(X, Y, 250), FVector(30, 36, 86), 0.0, false, 0.0, Lean * 0.4);
		Add(Out, MarketModel::Sphere, Bronze, FVector(X + 2, Y + Lean * 0.6, 312), FVector(24, 22, 28), 0.0, false);
		for (const double S : { -1.0, 1.0 })
		{
			Add(Out, MarketModel::Cone, Bronze, FVector(X - 2, Y + Lean * 0.6 + S * 8, 330), FVector(5, 5, 14), 0.0, false, 0.0, S * 25.0);
		}
		const FVector Hand(X + 10, Y + Lean * 2.2, 380);
		Rod(Out, Bronze, FVector(X, Y + Lean * 0.3, 285), Hand, 9.0);
		Rod(Out, Bronze, FVector(X, Y - Lean * 0.3, 280), FVector(X + 20, Y - Lean * 0.8, 230), 9.0);
		Rod(Out, Bronze, Hand, Hand + FVector(0, 0, 30), 4.0);
		Add(Out, MarketModel::Sphere, Globe, Hand + FVector(0, 0, 52), FVector(34, 34, 34), 0.0, false);
		Lamps.Add({ Hand + FVector(30, 0, 50), 650.f });
	}

	/**
	 * A long street front: café arches on the ground floor (all along the
	 * körút, DohanyCafeBays from the corner on Dohány utca, shop windows past
	 * them), four upper floors, the cornice, attic and urns. On the körút the
	 * middle three bays stand forward with the café door, the fauns, the
	 * gable and the gilt name.
	 */
	inline void Front(EPiece Part, double Length, TArray<FShape>& Out, TArray<FLamp>& Lamps, TArray<FSign>& Signs)
	{
		using namespace Colors;
		const bool bMain = Part == EPiece::Korut;
		const int32 Bays = FMath::Max(3, FMath::RoundToInt((Length - 2.0 * EdgeMargin) / 430.0));
		const double Bay = (Length - 2.0 * EdgeMargin) / Bays;
		const int32 MidBay = Bays / 2;
		auto BayY = [&](int32 k) { return EdgeMargin + Bay * (k + 0.5); };
		// Bays counted from the corner (the körút runs to the corner, Dohány utca from it).
		auto FromCorner = [&](int32 k) { return bMain ? Bays - 1 - k : k; };
		auto Forth = [&](int32 k) { return bMain && FMath::Abs(k - MidBay) <= 1 ? BaysOut : 0.0; };

		// The face over the plain block's wall, the dark plinth, the band and its fascia.
		Span(Out, Stone, FVector(0, 0, 0), FVector(4, Length, AtticTop), false);
		Span(Out, Plinth, FVector(0, 0, 0), FVector(30, Length, PlinthTop));
		Span(Out, Trim, FVector(0, 0, BandAt), FVector(40, Length, BandTop), false);
		Span(Out, StoneDark, FVector(0, 0, BandAt - 60), FVector(26, Length, BandAt), false);

		for (int32 k = 0; k < Bays; ++k)
		{
			const double Y = BayY(k);
			const double X = Forth(k);
			const double Y0 = EdgeMargin + Bay * k;
			const double Y1 = Y0 + Bay;
			const bool bCafe = bMain || FromCorner(k) < DohanyCafeBays;
			const bool bDoor = bMain && k == MidBay;
			if (X > 0.0)
			{
				// The forward bays' own face, from the pavement to the attic.
				Span(Out, Stone, FVector(0, Y0, 0), FVector(X + 4, Y1, AtticTop), false);
				Span(Out, Plinth, FVector(0, Y0, 0), FVector(X + 30, Y1, PlinthTop));
				Span(Out, Trim, FVector(0, Y0, BandAt), FVector(X + 40, Y1, BandTop), false);
				Span(Out, StoneDark, FVector(0, Y0, BandAt - 60), FVector(X + 26, Y1, BandAt), false);
			}

			// Ground floor.
			const double Opening = Bay - PierWidth - 40.0;
			if (bDoor)
			{
				// The café door: a bronze and glass double door under a fanlight, a glass canopy on iron arms.
				Span(Out, Bronze, FVector(X, Y - 130, PlinthTop - 60), FVector(X + 8, Y + 130, 470), false);
				for (const double S : { -1.0, 1.0 })
				{
					Span(Out, CafeGlass, FVector(X + 8, Y + S * 64 - 52, 10), FVector(X + 10, Y + S * 64 + 52, 400), false);
					Span(Out, Bronze, FVector(X + 10, Y + S * 64 - 3, 10), FVector(X + 12, Y + S * 64 + 3, 400), false);
				}
				ArchWindow(Out, X, Y, Opening, 480.0, ArchSpring + 40.0, CafeGlass);
				Span(Out, Iron, FVector(X, Y - 200, 470), FVector(X + 230, Y + 200, 482), false);
				Span(Out, CafeGlass, FVector(X + 10, Y - 190, 482), FVector(X + 220, Y + 190, 486), false);
				Span(Out, Gilt, FVector(X + 228, Y - 200, 466), FVector(X + 232, Y + 200, 482), false);
				for (const double S : { -1.0, 1.0 })
				{
					Rod(Out, Iron, FVector(X, Y + S * 190, 640), FVector(X + 220, Y + S * 190, 482), 4.0);
				}
				Signs.Add({ TEXT("K\u00C1V\u00C9H\u00C1Z"), FVector(X + 233, Y, 474), 0.0, 10.0, Gilt });
				Lamps.Add({ FVector(X + 150, Y, 440), 700.f });
				Faun(Out, Lamps, X + 70, Y - Bay * 0.5 + 10, -10.0);
				Faun(Out, Lamps, X + 70, Y + Bay * 0.5 - 10, 10.0);
			}
			else if (bCafe)
			{
				// A tall arched café window over the ground floor and the gallery, a bronze bar where the gallery floor is.
				Span(Out, StoneDark, FVector(X, Y - Opening * 0.5, PlinthTop), FVector(X + 8, Y + Opening * 0.5, 80), false);
				ArchWindow(Out, X, Y, Opening, 80.0, ArchSpring, CafeGlass);
				Span(Out, Bronze, FVector(X + 9, Y - Opening * 0.5, 430), FVector(X + 12, Y + Opening * 0.5, 442), false);
				for (const double T : { -0.25, 0.25 })
				{
					Span(Out, Bronze, FVector(X + 9, Y + T * Opening - 2, 80), FVector(X + 12, Y + T * Opening + 2, ArchSpring), false);
				}
			}
			else if (k == (Bays + DohanyCafeBays) / 2)
			{
				// The house gate on Dohány utca: a tall arched wooden gate.
				ArchWindow(Out, X, Y, 230.0, 0.0, 420.0, Door);
				Span(Out, Iron, FVector(X + 9, Y - 1.5, 0), FVector(X + 11, Y + 1.5, 420), false);
			}
			else
			{
				// A shop window with a dark fascia over it.
				Span(Out, Bronze, FVector(X, Y - Opening * 0.5, 80), FVector(X + 8, Y + Opening * 0.5, 470), false);
				Span(Out, Glass, FVector(X + 8, Y - Opening * 0.5 + 10, 90), FVector(X + 10, Y + Opening * 0.5 - 10, 440), false);
				Span(Out, Trim, FVector(X, Y - Opening * 0.5 - 10, 560), FVector(X + 14, Y + Opening * 0.5 + 10, 680), false);
			}

			// Upper floors: balconies on the first floor every other bay, all along the forward bays.
			for (int32 Floor = 0; Floor < UpperFloors; ++Floor)
			{
				const bool bBalcony = (Floor == 0 && (k % 2 == 1 || X > 0.0)) || (Floor == 2 && X > 0.0);
				UpperWindow(Out, X + 4, Y, Floor, bBalcony);
			}
		}

		// Rusticated piers between the café bays, pilasters up the upper floors.
		for (int32 k = 0; k <= Bays; ++k)
		{
			const double Y = EdgeMargin + Bay * k;
			const double X = FMath::Max(k > 0 ? Forth(k - 1) : 0.0, k < Bays ? Forth(k) : 0.0);
			const double Half = PierWidth * 0.5;
			for (int32 Course = 0; Course < 6; ++Course)
			{
				const double Z0 = PlinthTop + Course * (BandAt - 60.0 - PlinthTop) / 6.0;
				const double Z1 = Z0 + (BandAt - 60.0 - PlinthTop) / 6.0 - 6.0;
				Span(Out, Course % 2 ? Stone : StoneDark, FVector(X, Y - Half, Z0), FVector(X + 26, Y + Half, Z1), Course == 0);
			}
			Span(Out, Trim, FVector(X, Y - 24, BandTop), FVector(X + 18, Y + 24, CorniceAt - 50), false);
			Span(Out, Trim, FVector(X, Y - 32, CorniceAt - 50), FVector(X + 26, Y + 32, CorniceAt), false);
		}

		// The cornice, the attic and urns over the pilasters.
		Span(Out, StoneDark, FVector(0, 0, CorniceAt), FVector(40, Length, CorniceAt + 22), false);
		Span(Out, Trim, FVector(0, -10, CorniceAt + 22), FVector(75, Length + 10, CorniceTop), false);
		Span(Out, Stone, FVector(0, 0, CorniceTop), FVector(22, Length, AtticTop), false);
		Span(Out, Trim, FVector(0, 0, AtticTop - 12), FVector(28, Length, AtticTop), false);
		for (int32 k = 0; k <= Bays; ++k)
		{
			if (bMain && FMath::Abs(k - MidBay - 0.5) < 1.0)
			{
				continue;
			}
			const double Y = EdgeMargin + Bay * k;
			Add(Out, MarketModel::Cylinder, Trim, FVector(14, Y, AtticTop + 10), FVector(34, 34, 20), 0.0, false);
			Add(Out, MarketModel::Sphere, Stone, FVector(14, Y, AtticTop + 52), FVector(44, 44, 64), 0.0, false);
			Add(Out, MarketModel::Cone, Trim, FVector(14, Y, AtticTop + 100), FVector(14, 14, 30), 0.0, false);
		}

		if (bMain)
		{
			// The forward bays' cornice and their round gable with the name and the year.
			const double G0 = EdgeMargin + Bay * (MidBay - 1);
			const double G1 = EdgeMargin + Bay * (MidBay + 2);
			const double GY = (G0 + G1) * 0.5;
			Span(Out, Trim, FVector(BaysOut, G0 - 10, CorniceAt + 22), FVector(BaysOut + 75, G1 + 10, CorniceTop), false);
			Span(Out, Stone, FVector(BaysOut, G0, CorniceTop), FVector(BaysOut + 22, G1, AtticTop + 60), false);
			Add(Out, MarketModel::Cylinder, Stone, FVector(BaysOut + 12, GY, AtticTop + 60), FVector(560, G1 - G0, 10), 0.0, false, 90.0);
			Add(Out, MarketModel::Cylinder, Trim, FVector(BaysOut + 8, GY, AtticTop + 60), FVector(600, G1 - G0 + 40, 10), 0.0, false, 90.0);
			Add(Out, MarketModel::Cylinder, Gilt, FVector(BaysOut + 18, GY, AtticTop + 170), FVector(110, 160, 4), 0.0, false, 90.0);
			Add(Out, MarketModel::Cylinder, StoneDark, FVector(BaysOut + 20, GY, AtticTop + 170), FVector(90, 140, 4), 0.0, false, 90.0);
			Signs.Add({ TEXT("NEW-YORK"), FVector(BaysOut + 24, GY, AtticTop + 70), 0.0, 70.0, Gilt });
			Signs.Add({ TEXT("1894"), FVector(BaysOut + 24, GY, AtticTop + 170), 0.0, 34.0, Gilt });
			Add(Out, MarketModel::Sphere, Gilt, FVector(BaysOut + 12, GY, AtticTop + 345), FVector(40, 40, 40), 0.0, false);
			for (const double Y : { G0, G1 })
			{
				Add(Out, MarketModel::Cylinder, Trim, FVector(BaysOut + 14, Y, AtticTop + 10), FVector(40, 40, 20), 0.0, false);
				Add(Out, MarketModel::Sphere, Stone, FVector(BaysOut + 14, Y, AtticTop + 60), FVector(54, 54, 80), 0.0, false);
			}
			// The café's gilt name along the band, either side of the door.
			Signs.Add({ TEXT("NEW YORK K\u00C1V\u00C9H\u00C1Z"), FVector(BaysOut + 42, GY, BandAt + 25), 0.0, 38.0, Gilt });
			const double Third = (Length - G1) * 0.5 + G1;
			Signs.Add({ TEXT("NEW YORK"), FVector(42, (G0 - EdgeMargin) * 0.5 + EdgeMargin, BandAt + 25), 0.0, 34.0, Gilt });
			Signs.Add({ TEXT("NEW YORK"), FVector(42, Third, BandAt + 25), 0.0, 34.0, Gilt });
			// Light from the café windows on the pavement.
			Lamps.Add({ FVector(120, (G0 - EdgeMargin) * 0.5 + EdgeMargin, 500), 800.f });
			Lamps.Add({ FVector(120, Third, 500), 800.f });
		}
		else
		{
			const double CafeEnd = EdgeMargin + Bay * FMath::Min(DohanyCafeBays, Bays);
			Signs.Add({ TEXT("NEW YORK K\u00C1V\u00C9H\u00C1Z"), FVector(42, CafeEnd * 0.5, BandAt + 25), 0.0, 32.0, Gilt });
			Lamps.Add({ FVector(120, CafeEnd * 0.5, 500), 800.f });
		}
	}

	/** The corner tower over the cut corner (Length along it): it stands proud of both fronts and ends in a slate dome. */
	inline void Tower(double Length, TArray<FShape>& Out, TArray<FLamp>& Lamps, TArray<FSign>& Signs)
	{
		using namespace Colors;
		const double Mid = Length * 0.5;
		const double Half = Mid + 150.0;
		constexpr double Proud = 80.0;
		constexpr double Back = -560.0;
		constexpr double ShaftTop = CorniceTop + 40.0;
		constexpr double StageTop = ShaftTop + 720.0;
		constexpr double DrumTop = StageTop + 260.0;

		// The shaft up to the main cornice, the plinth, the band.
		Span(Out, Stone, FVector(Back, Mid - Half, PlinthTop), FVector(Proud, Mid + Half, ShaftTop), false);
		Span(Out, Plinth, FVector(Back, Mid - Half - 4, 0), FVector(Proud + 26, Mid + Half + 4, PlinthTop));
		Span(Out, Stone, FVector(Back, Mid - Half, 0), FVector(Proud, Mid + Half, BandAt), true);
		Span(Out, Trim, FVector(Back, Mid - Half - 6, BandAt), FVector(Proud + 40, Mid + Half + 6, BandTop), false);
		ArchWindow(Out, Proud, Mid, Length - 110.0, 80.0, ArchSpring, CafeGlass);
		Span(Out, Bronze, FVector(Proud + 9, Mid - (Length - 110.0) * 0.5, 430), FVector(Proud + 12, Mid + (Length - 110.0) * 0.5, 442), false);
		for (int32 Floor = 0; Floor < UpperFloors; ++Floor)
		{
			UpperWindow(Out, Proud + 4, Mid, Floor, Floor != UpperFloors - 1);
		}
		// Quoins up its corners.
		for (const double S : { -1.0, 1.0 })
		{
			for (int32 Course = 0; Course < 14; ++Course)
			{
				const double Z0 = BandTop + Course * (CorniceAt - BandTop) / 14.0;
				const double Wide = Course % 2 ? 40.0 : 60.0;
				Span(Out, Trim, FVector(Proud - 10, Mid + S * Half - (S > 0 ? Wide : 0.0), Z0), FVector(Proud + 12, Mid + S * Half + (S < 0 ? Wide : 0.0), Z0 + (CorniceAt - BandTop) / 14.0 - 5.0), false);
			}
		}
		Span(Out, StoneDark, FVector(Back, Mid - Half - 10, CorniceAt), FVector(Proud + 40, Mid + Half + 10, CorniceAt + 22), false);
		Span(Out, Trim, FVector(Back, Mid - Half - 20, CorniceAt + 22), FVector(Proud + 75, Mid + Half + 20, ShaftTop), false);

		// The stage above the roofs: corner piers, two arched openings to the street, its own cornice.
		const double Stage = Half - 40.0;
		const double StageMidX = (Back + Proud) * 0.5 - 20.0;
		const double StageDepth = Proud - Back - 80.0;
		Span(Out, Stone, FVector(StageMidX - StageDepth * 0.5 + 10, Mid - Stage + 10, ShaftTop), FVector(StageMidX + StageDepth * 0.5 - 4, Mid + Stage - 10, StageTop), false);
		for (const double SX : { -1.0, 1.0 })
		{
			for (const double SY : { -1.0, 1.0 })
			{
				const FVector Corner(StageMidX + SX * (StageDepth * 0.5 - 35), Mid + SY * (Stage - 35), (ShaftTop + StageTop) * 0.5);
				Add(Out, MarketModel::Cube, Stone, Corner, FVector(70, 70, StageTop - ShaftTop), 0.0, false);
			}
		}
		Span(Out, Stone, FVector(StageMidX - StageDepth * 0.5, Mid - Stage, StageTop - 170), FVector(StageMidX + StageDepth * 0.5, Mid + Stage, StageTop), false);
		// Two tall arched openings to the street, a column between them.
		for (const double S : { -1.0, 1.0 })
		{
			ArchWindow(Out, StageMidX + StageDepth * 0.5, Mid + S * Stage * 0.45, Stage * 0.62, ShaftTop + 90.0, StageTop - 170.0 - Stage * 0.31 - 30.0, Glass);
		}
		Add(Out, MarketModel::Cylinder, Trim, FVector(StageMidX + StageDepth * 0.5 + 20, Mid, (ShaftTop + StageTop - 170) * 0.5), FVector(36, 36, StageTop - 170 - ShaftTop), 0.0, false);
		Span(Out, Iron, FVector(StageMidX + StageDepth * 0.5, Mid - Stage, ShaftTop + 100), FVector(StageMidX + StageDepth * 0.5 + 4, Mid + Stage, ShaftTop + 106), false);
		Span(Out, Trim, FVector(StageMidX - StageDepth * 0.5 - 20, Mid - Stage - 20, StageTop), FVector(StageMidX + StageDepth * 0.5 + 20, Mid + Stage + 20, StageTop + 40), false);

		// The drum, the slate dome, the gilt lantern and its spike.
		const double Drum = FMath::Min(StageDepth, 2.0 * Stage) - 20.0;
		Add(Out, MarketModel::Cylinder, Stone, FVector(StageMidX, Mid, (StageTop + 40 + DrumTop) * 0.5), FVector(Drum, Drum, DrumTop - StageTop - 40), 0.0, false);
		for (int32 i = 0; i < 8; ++i)
		{
			const double A = FMath::DegreesToRadians(i * 45.0 + 22.5);
			Add(Out, MarketModel::Cylinder, Glass, FVector(StageMidX + FMath::Cos(A) * Drum * 0.5, Mid + FMath::Sin(A) * Drum * 0.5, DrumTop - 110), FVector(50, 50, 4), i * 45.0 + 22.5, false, 90.0);
		}
		Add(Out, MarketModel::Cylinder, Trim, FVector(StageMidX, Mid, DrumTop + 10), FVector(Drum + 30, Drum + 30, 20), 0.0, false);
		Add(Out, MarketModel::Sphere, Slate, FVector(StageMidX, Mid, DrumTop + 20), FVector(Drum, Drum, 1150), 0.0, false);
		for (int32 i = 0; i < 8; ++i)
		{
			// Gilt ribs down the dome.
			const double A = FMath::DegreesToRadians(i * 45.0);
			const FVector Dir(FMath::Cos(A), FMath::Sin(A), 0.0);
			const FVector Low = FVector(StageMidX, Mid, DrumTop + 30) + Dir * (Drum * 0.5);
			const FVector Midway = FVector(StageMidX, Mid, DrumTop + 420) + Dir * (Drum * 0.37);
			const FVector High = FVector(StageMidX, Mid, DrumTop + 560) + Dir * (Drum * 0.12);
			Rod(Out, Gilt, Low, Midway, 6.0);
			Rod(Out, Gilt, Midway, High, 6.0);
		}
		const double LanternAt = DrumTop + 570.0;
		Add(Out, MarketModel::Cylinder, Trim, FVector(StageMidX, Mid, LanternAt + 80), FVector(150, 150, 160), 0.0, false);
		Add(Out, MarketModel::Cylinder, Glass, FVector(StageMidX, Mid, LanternAt + 80), FVector(156, 156, 90), 0.0, false);
		Add(Out, MarketModel::Cone, Slate, FVector(StageMidX, Mid, LanternAt + 220), FVector(170, 170, 120), 0.0, false);
		Add(Out, MarketModel::Sphere, Gilt, FVector(StageMidX, Mid, LanternAt + 300), FVector(46, 46, 46), 0.0, false);
		Rod(Out, Gilt, FVector(StageMidX, Mid, LanternAt + 300), FVector(StageMidX, Mid, LanternAt + 470), 6.0);

		Signs.Add({ TEXT("NEW YORK"), FVector(Proud + 42, Mid, BandAt + 25), 0.0, 30.0, Gilt });
		Lamps.Add({ FVector(Proud + 130, Mid, 500), 800.f });
	}

	/** The whole palace front: each street wall built in its frame and placed in the set's frame. */
	inline void Palace(const TArray<FPiece>& Pieces, TArray<FShape>& Out, TArray<FLamp>& Lamps, TArray<FSign>& Signs)
	{
		for (const FPiece& Piece : Pieces)
		{
			TArray<FShape> LocalShapes;
			TArray<FLamp> LocalLamps;
			TArray<FSign> LocalSigns;
			if (Piece.Part == EPiece::Corner)
			{
				Tower(Piece.Length, LocalShapes, LocalLamps, LocalSigns);
			}
			else
			{
				Front(Piece.Part, Piece.Length, LocalShapes, LocalLamps, LocalSigns);
			}
			Place(LocalShapes, LocalLamps, LocalSigns, Piece, Out, Lamps, Signs);
		}
	}
}
