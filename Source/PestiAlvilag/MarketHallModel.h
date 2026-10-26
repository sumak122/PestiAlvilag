// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * The Rákóczi tér market hall (Vásárcsarnok No. II, Rozinay István and
 * Klunzinger Pál, 1897) as it stood in the early 1930s, as polygons in
 * mesh parts (one colour each). AMarketHall puts it in the game.
 *
 * The plan, from the 1944 aerial photo (Fortepan 109125) and the
 * descriptions: a 71 by 75.5 m block. Two five-aisled halls cross in a
 * Greek cross, rising in steps: a low outer aisle with a lean-to roof, an
 * inner aisle a step higher, and the tall nave with a saddle roof whose top
 * is glazed. The steps between them are glazed too. Over the crossing a
 * raised square with windows on all four sides. In the four corners stand
 * three-storey brick pavilions with flats above, rising like towers.
 * The main front faces Rákóczi tér: a stepped gable over the great arched
 * gateway wide enough for a horse cart, a big lunette window over it.
 *
 * Header-only (like MarketModel and PersonKit) so it can be built and
 * looked at outside the engine. The frame: origin the middle of the main
 * front on the pavement, +X into the hall (east in the city), +Y along the
 * front towards Déri Miksa utca (south), Z up.
 */
namespace MarketHall
{
	// One mesh section (and colour) per part. The parts up to Slate are
	// solid; the roofs and their iron are out of reach and need no
	// collision. Glass casts no shadow so the daylight falls into the hall.
	enum EPart : int32
	{
		Brick,
		Trim,
		Plinth,
		Iron,
		Glass,
		Wood,
		Floor,
		Sign,
		Canvas,
		Slate,
		RoofIn,
		Beam,
		RoofGlass,
		Pane,
		PartCount
	};
	constexpr int32 FirstOverheadPart = Slate;
	constexpr int32 FirstGlazingPart = RoofGlass;
	constexpr int32 FirstSeeThroughPart = Pane;

	inline FLinearColor PartColor(int32 Part)
	{
		static const FLinearColor Colors[PartCount] = {
			FLinearColor(0.36f, 0.12f, 0.07f),   // Brick: dark red
			FLinearColor(0.70f, 0.60f, 0.44f),   // Trim: sandstone
			FLinearColor(0.30f, 0.29f, 0.27f),   // Plinth: grey stone
			FLinearColor(0.07f, 0.11f, 0.09f),   // Iron: painted dark green
			FLinearColor(0.05f, 0.06f, 0.07f),   // Glass: dark window panes
			FLinearColor(0.22f, 0.12f, 0.06f),   // Wood
			FLinearColor(0.42f, 0.38f, 0.32f),   // Floor: worn stone tiles
			FLinearColor(0.08f, 0.16f, 0.12f),   // Sign: the name board
			FLinearColor(0.62f, 0.58f, 0.48f),   // Canvas
			FLinearColor(0.16f, 0.17f, 0.19f),   // Slate
			FLinearColor(0.26f, 0.21f, 0.16f),   // RoofIn: boarded underside
			FLinearColor(0.07f, 0.11f, 0.09f),   // Beam: iron under the roofs
			FLinearColor(0.32f, 0.36f, 0.37f),   // RoofGlass, from above
			FLinearColor(0.62f, 0.68f, 0.70f, 0.22f) }; // Pane: see-through, its alpha how much it hides
		return Colors[Part];
	}

	// Sizes (cm). See the frame above.
	constexpr double Base = -100.0;          // walls start below the pavement so slopes don't show a gap
	constexpr double Depth = 7110.0;         // the main front to the Víg utca side of the pavilions
	constexpr double HalfFront = 3775.0;     // the main front reaches out to here either side
	constexpr double PavilionX = 1550.0;     // the corner pavilions' size along X
	constexpr double LongHalf = 2000.0;      // half the width of the front-to-back hall; the pavilions reach from here to HalfFront
	constexpr double CrossX = Depth / 2.0;   // the middle of the hall across
	constexpr double CrossHalf = CrossX - PavilionX;       // half the width of the hall across
	// Between the pavilions the other three fronts stand back from the street (OpenStreetMap outline).
	constexpr double BackFront = Depth - 115.0;           // the Víg utca front
	constexpr double SideFront = HalfFront - 160.0;       // the Vásár utca and Déri Miksa utca fronts

	// The steps of a hall's cross section, from the middle out.
	constexpr double NaveHalf = 625.0;       // the nave is 12.5 m wide
	constexpr double InnerHalf = 1225.0;     // the inner aisles reach out to here, the outer ones to the hall's side
	constexpr double OuterEaves = 900.0;
	constexpr double OuterTop = 1100.0;      // the outer lean-to at the inner aisle
	constexpr double InnerTop = 1350.0;      // the inner lean-to at its outer edge, over a glazed step
	constexpr double InnerHigh = 1500.0;     // and at the nave
	constexpr double NaveEaves = 1750.0;     // the nave's glazed step ends here
	constexpr double Ridge = 2125.0;
	constexpr double GlassFrom = 0.55;       // the glazed top of each nave roof slope, from eaves (0) to ridge (1)

	// The raised square over the crossing.
	constexpr double CentreHalf = InnerHalf;
	constexpr double CentreBand = 2150.0;    // its window band starts above every roof round it
	constexpr double CentreTop = 2550.0;
	constexpr double CentreRoofTop = 3050.0;

	// The ends of the halls (the four fronts).
	constexpr double EndWall = 60.0;
	constexpr double OuterFrontTop = 1150.0;
	constexpr double InnerFrontTop = 1600.0;
	constexpr double NaveFrontTop = 1950.0;
	constexpr double GableTop = 2450.0;
	constexpr double TurretTop = 2250.0;

	// The corner pavilions: a ground floor of shops and two floors of flats.
	constexpr double PavilionTop = 1350.0;
	constexpr double PavilionRoofTop = 2150.0;
	constexpr double FirstFloor = 500.0;
	constexpr double SecondFloor = 900.0;

	// The name over the gateway (placed by the actor), on the board Front puts up.
	const FVector NameTextAt(-12.0, 0.0, 873.0);

	// The iron columns stand this far apart along the halls.
	constexpr double ColumnStep = 600.0;

	struct FPart
	{
		TArray<FVector> Vertices;
		TArray<FVector> Normals;
		TArray<int32> Triangles;
	};

	struct FBuilder
	{
		FPart Parts[PartCount];

		// A flat convex polygon facing roughly towards Hint.
		void Poly(EPart Part, std::initializer_list<FVector> Points, const FVector& Hint)
		{
			FPart& P = Parts[Part];
			const TArray<FVector> Corners(Points);
			FVector Normal = FVector::ZeroVector;
			for (int32 i = 0; i < Corners.Num(); ++i)
			{
				const FVector& A = Corners[i];
				const FVector& B = Corners[(i + 1) % Corners.Num()];
				Normal += FVector((A.Y - B.Y) * (A.Z + B.Z), (A.Z - B.Z) * (A.X + B.X), (A.X - B.X) * (A.Y + B.Y));
			}
			Normal = Normal.GetSafeNormal();
			if (FVector::DotProduct(Normal, Hint) < 0.0)
			{
				Normal = -Normal;
			}
			const int32 First = P.Vertices.Num();
			for (const FVector& Corner : Corners)
			{
				P.Vertices.Add(Corner);
				P.Normals.Add(Normal);
			}
			for (int32 i = 1; i + 1 < Corners.Num(); ++i)
			{
				// Unreal treats a triangle as facing (C - A) ^ (B - A).
				const int32 A = First, B = First + i, C = First + i + 1;
				const FVector Facing = FVector::CrossProduct(P.Vertices[C] - P.Vertices[A], P.Vertices[B] - P.Vertices[A]);
				P.Triangles.Add(A);
				if (FVector::DotProduct(Facing, Normal) >= 0.0)
				{
					P.Triangles.Add(B);
					P.Triangles.Add(C);
				}
				else
				{
					P.Triangles.Add(C);
					P.Triangles.Add(B);
				}
			}
		}

		// The same polygon seen from both sides.
		void TwoSided(EPart Part, std::initializer_list<FVector> Points, const FVector& Hint)
		{
			Poly(Part, Points, Hint);
			Poly(Part, Points, -Hint);
		}

		// A roof slope: Out above, In below.
		void Roof(EPart Out, EPart In, std::initializer_list<FVector> Points, const FVector& Up)
		{
			Poly(Out, Points, Up);
			Poly(In, Points, -Up);
		}

		// An axis-aligned box without its bottom.
		void Box(EPart Part, const FVector& Min, const FVector& Max)
		{
			const FVector C[8] = {
				FVector(Min.X, Min.Y, Min.Z), FVector(Max.X, Min.Y, Min.Z), FVector(Max.X, Max.Y, Min.Z), FVector(Min.X, Max.Y, Min.Z),
				FVector(Min.X, Min.Y, Max.Z), FVector(Max.X, Min.Y, Max.Z), FVector(Max.X, Max.Y, Max.Z), FVector(Min.X, Max.Y, Max.Z) };
			Poly(Part, { C[4], C[5], C[6], C[7] }, FVector(0.0, 0.0, 1.0));
			Poly(Part, { C[1], C[2], C[6], C[5] }, FVector(1.0, 0.0, 0.0));
			Poly(Part, { C[0], C[3], C[7], C[4] }, FVector(-1.0, 0.0, 0.0));
			Poly(Part, { C[2], C[3], C[7], C[6] }, FVector(0.0, 1.0, 0.0));
			Poly(Part, { C[0], C[1], C[5], C[4] }, FVector(0.0, -1.0, 0.0));
			// Anything lifted off the ground can be seen from below.
			if (Min.Z > 0.5)
			{
				Poly(Part, { C[0], C[3], C[2], C[1] }, FVector(0.0, 0.0, -1.0));
			}
		}

		// A round shaft narrowing from R0 to R1, with a lid.
		void Frustum(EPart Part, double CX, double CY, double R0, double R1, double Z0, double Z1, int32 Sides)
		{
			const double Turn = Sides == 4 ? PI / 4.0 : 0.0;
			for (int32 i = 0; i < Sides; ++i)
			{
				const double A0 = Turn + 2.0 * PI * i / Sides;
				const double A1 = Turn + 2.0 * PI * (i + 1) / Sides;
				const FVector D0(FMath::Cos(A0), FMath::Sin(A0), 0.0);
				const FVector D1(FMath::Cos(A1), FMath::Sin(A1), 0.0);
				const FVector C0(CX, CY, Z0);
				const FVector C1(CX, CY, Z1);
				Poly(Part, { C0 + D0 * R0, C0 + D1 * R0, C1 + D1 * R1, C1 + D0 * R1 }, D0 + D1);
				if (R1 > 0.0)
				{
					Poly(Part, { C1, C1 + D0 * R1, C1 + D1 * R1 }, FVector(0.0, 0.0, 1.0));
				}
			}
		}

		// A box turned to run along the line from P0 to P1 (which may slope),
		// HalfDepth to either side and HalfHeight above and below it.
		void Bar(EPart Part, const FVector& P0, const FVector& P1, double HalfDepth, double HalfHeight)
		{
			const FVector Along = (P1 - P0).GetSafeNormal();
			const FVector Side = FVector::CrossProduct(Along, FMath::Abs(Along.Z) > 0.9 ? FVector(1.0, 0.0, 0.0) : FVector(0.0, 0.0, 1.0)).GetSafeNormal();
			const FVector Up = FVector::CrossProduct(Side, Along).GetSafeNormal();
			const FVector S = Side * HalfDepth, U = Up * HalfHeight;
			Poly(Part, { P0 + S + U, P1 + S + U, P1 - S + U, P0 - S + U }, Up);
			Poly(Part, { P0 + S - U, P1 + S - U, P1 - S - U, P0 - S - U }, -Up);
			Poly(Part, { P0 + S - U, P1 + S - U, P1 + S + U, P0 + S + U }, Side);
			Poly(Part, { P0 - S - U, P1 - S - U, P1 - S + U, P0 - S + U }, -Side);
			Poly(Part, { P0 + S - U, P0 - S - U, P0 - S + U, P0 + S + U }, -Along);
			Poly(Part, { P1 + S - U, P1 - S - U, P1 - S + U, P1 + S + U }, Along);
		}

		// A ring of a hipped roof: the rectangle HX0 by HY0 (half sizes) at Z0
		// narrowing to HX1 by HY1 at Z1, outside and (when In isn't Out) under it.
		void HipRing(EPart Out, EPart In, double CX, double CY, double HX0, double HY0, double HX1, double HY1, double Z0, double Z1)
		{
			const FVector Lo[4] = { FVector(CX - HX0, CY - HY0, Z0), FVector(CX + HX0, CY - HY0, Z0), FVector(CX + HX0, CY + HY0, Z0), FVector(CX - HX0, CY + HY0, Z0) };
			const FVector Hi[4] = { FVector(CX - HX1, CY - HY1, Z1), FVector(CX + HX1, CY - HY1, Z1), FVector(CX + HX1, CY + HY1, Z1), FVector(CX - HX1, CY + HY1, Z1) };
			const FVector Outs[4] = { FVector(0.0, -1.0, 0.5), FVector(1.0, 0.0, 0.5), FVector(0.0, 1.0, 0.5), FVector(-1.0, 0.0, 0.5) };
			for (int32 i = 0; i < 4; ++i)
			{
				const int32 j = (i + 1) % 4;
				Poly(Out, { Lo[i], Lo[j], Hi[j], Hi[i] }, Outs[i]);
				if (In != Out)
				{
					Poly(In, { Lo[i], Lo[j], Hi[j], Hi[i] }, -Outs[i]);
				}
			}
		}

		// An axis-aligned box between two corners given in any order.
		void BoxBetween(EPart Part, const FVector& P0, const FVector& P1)
		{
			Box(Part, FVector(FMath::Min(P0.X, P1.X), FMath::Min(P0.Y, P1.Y), FMath::Min(P0.Z, P1.Z)),
				FVector(FMath::Max(P0.X, P1.X), FMath::Max(P0.Y, P1.Y), FMath::Max(P0.Z, P1.Z)));
		}

		void Lid(EPart Part, double CX, double CY, double HX, double HY, double Z)
		{
			Poly(Part, { FVector(CX - HX, CY - HY, Z), FVector(CX + HX, CY - HY, Z), FVector(CX + HX, CY + HY, Z), FVector(CX - HX, CY + HY, Z) }, FVector(0.0, 0.0, 1.0));
		}
	};

	/** An opening in a wall: a door or window, square-headed or with a round head over Spring. */
	struct FOpening
	{
		double Mid;
		double Half;
		double Bottom;
		double Spring;
		bool bArch;
		bool bGlazed;
		double Top() const { return bArch ? Spring + Half : Spring; }
	};

	// Draws on one wall: S runs along it from Origin, Z up, D out of it.
	struct FFace
	{
		FVector Origin;
		FVector Along;
		FVector Out;

		FVector At(double S, double Z, double D) const
		{
			return Origin + Along * S + Out * D + FVector(0.0, 0.0, Z);
		}

		void Panel(FBuilder& B, EPart Part, double S0, double S1, double Z0, double Z1, double D) const
		{
			B.Poly(Part, { At(S0, Z0, D), At(S1, Z0, D), At(S1, Z1, D), At(S0, Z1, D) }, Out);
		}

		// A block standing out of the wall (no back face).
		void Block(FBuilder& B, EPart Part, double S0, double S1, double Z0, double Z1, double D) const
		{
			Panel(B, Part, S0, S1, Z0, Z1, D);
			B.Poly(Part, { At(S0, Z1, 0.0), At(S1, Z1, 0.0), At(S1, Z1, D), At(S0, Z1, D) }, FVector(0.0, 0.0, 1.0));
			if (Z0 > 0.5)
			{
				B.Poly(Part, { At(S0, Z0, 0.0), At(S1, Z0, 0.0), At(S1, Z0, D), At(S0, Z0, D) }, FVector(0.0, 0.0, -1.0));
			}
			B.Poly(Part, { At(S0, Z0, 0.0), At(S0, Z1, 0.0), At(S0, Z1, D), At(S0, Z0, D) }, -Along);
			B.Poly(Part, { At(S1, Z0, 0.0), At(S1, Z1, 0.0), At(S1, Z1, D), At(S1, Z0, D) }, Along);
		}

		// A band along the wall broken where it would cross an opening.
		void Band(FBuilder& B, EPart Part, double S0, double S1, double Z0, double Z1, double D, const TArray<FOpening>& Openings) const
		{
			double From = S0;
			for (const FOpening& O : Openings)
			{
				if (O.Bottom < Z1 && O.Top() > Z0)
				{
					if (O.Mid - O.Half > From)
					{
						Block(B, Part, From, O.Mid - O.Half, Z0, Z1, D);
					}
					From = O.Mid + O.Half;
				}
			}
			if (S1 > From)
			{
				Block(B, Part, From, S1, Z0, Z1, D);
			}
		}

		// One side of a wall with its openings cut out, at depth D.
		void Skin(FBuilder& B, EPart Part, double S0, double S1, double Z0, double Z1, double D, const TArray<FOpening>& Openings, const FVector& Facing) const
		{
			constexpr int32 Segments = 10;
			auto Quad = [&](double A0, double A1, double Lo, double Hi)
			{
				if (A1 - A0 > 0.5 && Hi - Lo > 0.5)
				{
					B.Poly(Part, { At(A0, Lo, D), At(A1, Lo, D), At(A1, Hi, D), At(A0, Hi, D) }, Facing);
				}
			};
			// Openings over each other (a door under a window) share a column of
			// the wall: the parts between them are filled up to the next one.
			double From = S0;
			int32 i = 0;
			while (i < Openings.Num())
			{
				int32 End = i + 1;
				double UL = Openings[i].Mid - Openings[i].Half, UR = Openings[i].Mid + Openings[i].Half;
				while (End < Openings.Num() && Openings[End].Mid - Openings[End].Half < UR)
				{
					UL = FMath::Min(UL, Openings[End].Mid - Openings[End].Half);
					UR = FMath::Max(UR, Openings[End].Mid + Openings[End].Half);
					++End;
				}
				Quad(From, UL, Z0, Z1);
				double Z = Z0;
				for (int32 k = i; k < End; ++k)
				{
					const FOpening& O = Openings[k];
					const double L = O.Mid - O.Half, R = O.Mid + O.Half;
					const double Ceiling = k + 1 < End ? Openings[k + 1].Bottom : Z1;
					Quad(UL, UR, Z, O.Bottom);
					Quad(UL, L, O.Bottom, O.bArch ? O.Spring : Ceiling);
					Quad(R, UR, O.Bottom, O.bArch ? O.Spring : Ceiling);
					if (!O.bArch)
					{
						Quad(L, R, O.Spring, Ceiling);
					}
					else
					{
						Quad(UL, L, O.Spring, Ceiling);
						Quad(R, UR, O.Spring, Ceiling);
						for (int32 j = 0; j < Segments; ++j)
						{
							const double A0 = PI * (1.0 - double(j) / Segments);
							const double A1 = PI * (1.0 - double(j + 1) / Segments);
							const double SA = O.Mid + FMath::Cos(A0) * O.Half, ZA = O.Spring + FMath::Sin(A0) * O.Half;
							const double SB = O.Mid + FMath::Cos(A1) * O.Half, ZB = O.Spring + FMath::Sin(A1) * O.Half;
							B.Poly(Part, { At(SA, ZA, D), At(SB, ZB, D), At(SB, Ceiling, D), At(SA, Ceiling, D) }, Facing);
						}
					}
					Z = Ceiling;
				}
				From = UR;
				i = End;
			}
			Quad(From, S1, Z0, Z1);
		}

		// A solid wall from S0 to S1, Z0 to Z1, Thick deep behind the face,
		// with its openings cut through, their reveals, panes and frames.
		void Wall(FBuilder& B, EPart Part, double S0, double S1, double Z0, double Z1, double Thick, const TArray<FOpening>& Openings, bool bEnds = false) const
		{
			constexpr int32 Segments = 10;
			Skin(B, Part, S0, S1, Z0, Z1, 0.0, Openings, Out);
			Skin(B, Part, S0, S1, Z0, Z1, -Thick, Openings, -Out);
			B.Poly(Part, { At(S0, Z1, 0.0), At(S1, Z1, 0.0), At(S1, Z1, -Thick), At(S0, Z1, -Thick) }, FVector(0.0, 0.0, 1.0));
			if (bEnds)
			{
				B.Poly(Part, { At(S0, Z0, 0.0), At(S0, Z1, 0.0), At(S0, Z1, -Thick), At(S0, Z0, -Thick) }, -Along);
				B.Poly(Part, { At(S1, Z0, 0.0), At(S1, Z1, 0.0), At(S1, Z1, -Thick), At(S1, Z0, -Thick) }, Along);
			}
			for (const FOpening& O : Openings)
			{
				const double L = O.Mid - O.Half, R = O.Mid + O.Half;
				auto Reveal = [&](double SA, double ZA, double SB, double ZB, const FVector& Facing)
				{
					B.Poly(Part, { At(SA, ZA, 0.0), At(SB, ZB, 0.0), At(SB, ZB, -Thick), At(SA, ZA, -Thick) }, Facing);
				};
				Reveal(L, O.Bottom, L, O.Spring, Along);
				Reveal(R, O.Bottom, R, O.Spring, -Along);
				if (O.Bottom > Z0 + 0.5)
				{
					Reveal(L, O.Bottom, R, O.Bottom, FVector(0.0, 0.0, 1.0));
				}
				if (!O.bArch)
				{
					Reveal(L, O.Spring, R, O.Spring, FVector(0.0, 0.0, -1.0));
				}
				else
				{
					for (int32 i = 0; i < Segments; ++i)
					{
						const double A0 = PI * (1.0 - double(i) / Segments);
						const double A1 = PI * (1.0 - double(i + 1) / Segments);
						const double Am = (A0 + A1) / 2.0;
						Reveal(O.Mid + FMath::Cos(A0) * O.Half, O.Spring + FMath::Sin(A0) * O.Half,
							O.Mid + FMath::Cos(A1) * O.Half, O.Spring + FMath::Sin(A1) * O.Half,
							FVector(0.0, 0.0, -FMath::Sin(Am)) - Along * FMath::Cos(Am));
					}
				}
				if (O.bGlazed)
				{
					Glaze(B, O, -Thick * 0.5);
				}
			}
		}

		// A see-through pane in an opening with iron glazing bars.
		void Glaze(FBuilder& B, const FOpening& O, double D) const
		{
			constexpr int32 Segments = 10;
			const double L = O.Mid - O.Half, R = O.Mid + O.Half;
			B.TwoSided(Pane, { At(L, O.Bottom, D), At(R, O.Bottom, D), At(R, O.Spring, D), At(L, O.Spring, D) }, Out);
			const int32 Bars = FMath::Max(1, int32(O.Half * 2.0 / 70.0));
			for (int32 k = 1; k < Bars; ++k)
			{
				const double S = L + (R - L) * k / Bars;
				B.Bar(Iron, At(S, O.Bottom, D), At(S, O.Spring, D), 2.5, 2.5);
			}
			for (double Z = O.Bottom + 90.0; Z < O.Spring - 30.0; Z += 90.0)
			{
				B.Bar(Iron, At(L, Z, D), At(R, Z, D), 2.5, 2.5);
			}
			if (O.bArch)
			{
				const FVector Hub = At(O.Mid, O.Spring, D);
				B.Bar(Iron, At(L, O.Spring, D), At(R, O.Spring, D), 3.5, 3.5);
				for (int32 i = 0; i < Segments; ++i)
				{
					const double A0 = PI * double(i) / Segments;
					const double A1 = PI * double(i + 1) / Segments;
					const FVector P0 = At(O.Mid + FMath::Cos(A0) * O.Half, O.Spring + FMath::Sin(A0) * O.Half, D);
					const FVector P1 = At(O.Mid + FMath::Cos(A1) * O.Half, O.Spring + FMath::Sin(A1) * O.Half, D);
					B.TwoSided(Pane, { Hub, P0, P1 }, Out);
					if (i % 2 == 1)
					{
						B.Bar(Iron, Hub, P1, 2.5, 2.5);
					}
				}
				// A half ring halfway out, as in a fanlight.
				for (int32 i = 0; i < Segments; ++i)
				{
					const double A0 = PI * double(i) / Segments;
					const double A1 = PI * double(i + 1) / Segments;
					const double H = O.Half * 0.5;
					B.Bar(Iron, At(O.Mid + FMath::Cos(A0) * H, O.Spring + FMath::Sin(A0) * H, D),
						At(O.Mid + FMath::Cos(A1) * H, O.Spring + FMath::Sin(A1) * H, D), 2.5, 2.5);
				}
			}
		}

		// A stone ring round an arched opening, and jambs down its sides.
		void Surround(FBuilder& B, const FOpening& O, double Width, double D) const
		{
			constexpr int32 Segments = 10;
			const double L = O.Mid - O.Half, R = O.Mid + O.Half;
			Block(B, Trim, L - Width, L, O.Bottom, O.Spring, D);
			Block(B, Trim, R, R + Width, O.Bottom, O.Spring, D);
			if (!O.bArch)
			{
				Block(B, Trim, L - Width, R + Width, O.Spring, O.Spring + Width, D);
				return;
			}
			for (int32 i = 0; i < Segments; ++i)
			{
				const double A0 = PI * double(i) / Segments;
				const double A1 = PI * double(i + 1) / Segments;
				const double RI = O.Half, RO = O.Half + Width;
				const FVector I0 = At(O.Mid + FMath::Cos(A0) * RI, O.Spring + FMath::Sin(A0) * RI, D);
				const FVector I1 = At(O.Mid + FMath::Cos(A1) * RI, O.Spring + FMath::Sin(A1) * RI, D);
				const FVector O0 = At(O.Mid + FMath::Cos(A0) * RO, O.Spring + FMath::Sin(A0) * RO, D);
				const FVector O1 = At(O.Mid + FMath::Cos(A1) * RO, O.Spring + FMath::Sin(A1) * RO, D);
				B.Poly(Trim, { I0, O0, O1, I1 }, Out);
				const FVector Back0 = O0 - Out * D, Back1 = O1 - Out * D;
				B.Poly(Trim, { O0, Back0, Back1, O1 }, O0 + O1 - I0 - I1);
				const FVector BackI0 = I0 - Out * D, BackI1 = I1 - Out * D;
				B.Poly(Trim, { I0, BackI0, BackI1, I1 }, I0 + I1 - O0 - O1);
			}
			// Keystone.
			Block(B, Trim, O.Mid - 22.0, O.Mid + 22.0, O.Spring + O.Half - 4.0, O.Spring + O.Half + Width + 18.0, D + 6.0);
		}

		// A dark glazed window (the flats, the shops) in a stone frame, set into a solid wall.
		void FlatWindow(FBuilder& B, double Mid, double Half, double Bottom, double Top, bool bArch) const
		{
			const double Spring = bArch ? Top - Half : Top;
			const FOpening O{ Mid, Half, Bottom, Spring, bArch, false };
			Panel(B, Glass, Mid - Half, Mid + Half, Bottom, Spring, 1.0);
			if (bArch)
			{
				constexpr int32 Segments = 8;
				for (int32 i = 0; i < Segments; ++i)
				{
					const double A0 = PI * double(i) / Segments;
					const double A1 = PI * double(i + 1) / Segments;
					B.Poly(Glass, { At(Mid, Spring, 1.0), At(Mid + FMath::Cos(A0) * Half, Spring + FMath::Sin(A0) * Half, 1.0),
						At(Mid + FMath::Cos(A1) * Half, Spring + FMath::Sin(A1) * Half, 1.0) }, Out);
				}
			}
			// Mullion and transom.
			Block(B, Wood, Mid - 3.0, Mid + 3.0, Bottom, Spring, 3.0);
			Block(B, Wood, Mid - Half, Mid + Half, Spring - (Spring - Bottom) * 0.3 - 3.0, Spring - (Spring - Bottom) * 0.3 + 3.0, 3.0);
			Surround(B, O, 16.0, 8.0);
			// Sill.
			Block(B, Trim, Mid - Half - 26.0, Mid + Half + 26.0, Bottom - 14.0, Bottom, 14.0);
		}
	};

	/** One of the two halls: Along runs its length, Across from its middle line out. */
	struct FHall
	{
		bool bAlongY;
		double Half;

		FVector P(double A, double C, double Z) const
		{
			return bAlongY ? FVector(CrossX + C, A, Z) : FVector(A, C, Z);
		}
		FVector AcrossDir(double Side) const { return bAlongY ? FVector(Side, 0.0, 0.0) : FVector(0.0, Side, 0.0); }
		FVector AlongDir() const { return bAlongY ? FVector(0.0, 1.0, 0.0) : FVector(1.0, 0.0, 0.0); }
	};

	// A glazed step between two roof levels on the line C, from A0 to A1:
	// bottom Z0 (at A0) to Z0b (at A1), top Z1.
	inline void GlazedStep(FBuilder& B, const FHall& H, double A0, double A1, double C, double Z0, double Z0b, double Z1)
	{
		const FVector Side = H.AcrossDir(C > 0.0 ? 1.0 : -1.0);
		B.TwoSided(Pane, { H.P(A0, C, Z0), H.P(A1, C, Z0b), H.P(A1, C, Z1), H.P(A0, C, Z1) }, Side);
		B.Bar(Beam, H.P(A0, C, FMath::Max(Z0, Z0b) - 12.0), H.P(A1, C, FMath::Max(Z0, Z0b) - 12.0), 10.0, 14.0);
		for (double A = A0 + 75.0; A < A1 - 10.0; A += 150.0)
		{
			const double Z = Z0 + (Z0b - Z0) * (A - A0) / (A1 - A0);
			B.Bar(Beam, H.P(A, C, Z), H.P(A, C, Z1), 2.5, 2.5);
		}
		B.Bar(Beam, H.P(A0, C, Z1 - 4.0), H.P(A1, C, Z1 - 4.0), 6.0, 6.0);
	}

	// The outer aisles' lean-to roofs of a hall from A0 to A1.
	inline void OuterRoofs(FBuilder& B, const FHall& H, double A0, double A1)
	{
		for (const double Side : { -1.0, 1.0 })
		{
			const FVector Up = H.AcrossDir(Side) * 0.3 + FVector(0.0, 0.0, 1.0);
			B.Roof(Slate, RoofIn, { H.P(A0, Side * (H.Half + 40.0), OuterEaves - 15.0), H.P(A1, Side * (H.Half + 40.0), OuterEaves - 15.0),
				H.P(A1, Side * InnerHalf, OuterTop), H.P(A0, Side * InnerHalf, OuterTop) }, Up);
		}
	}

	// The inner aisles, the nave and the glazed steps between them, from A0 to A1.
	// Low0/Low1: where the inner step starts at A0 and A1 (lower than OuterTop
	// where the hall runs out over the other hall's outer aisle).
	inline void InnerRoofs(FBuilder& B, const FHall& H, double A0, double A1, double Low0 = OuterTop, double Low1 = OuterTop)
	{
		for (const double Side : { -1.0, 1.0 })
		{
			const FVector Out = H.AcrossDir(Side);
			GlazedStep(B, H, A0, A1, Side * InnerHalf, Low0, Low1, InnerTop);
			B.Roof(Slate, RoofIn, { H.P(A0, Side * InnerHalf, InnerTop), H.P(A1, Side * InnerHalf, InnerTop),
				H.P(A1, Side * NaveHalf, InnerHigh), H.P(A0, Side * NaveHalf, InnerHigh) }, Out * 0.25 + FVector(0.0, 0.0, 1.0));
			GlazedStep(B, H, A0, A1, Side * NaveHalf, InnerHigh, InnerHigh, NaveEaves);
			// The nave roof: slate below, glass above.
			const FVector Up = Out * 0.6 + FVector(0.0, 0.0, 1.0);
			const double CG = Side * NaveHalf * (1.0 - GlassFrom);
			const double ZG = NaveEaves + (Ridge - NaveEaves) * GlassFrom;
			B.Roof(Slate, RoofIn, { H.P(A0, Side * (NaveHalf + 30.0), NaveEaves - 18.0), H.P(A1, Side * (NaveHalf + 30.0), NaveEaves - 18.0),
				H.P(A1, CG, ZG), H.P(A0, CG, ZG) }, Up);
			B.Poly(RoofGlass, { H.P(A0, CG, ZG), H.P(A1, CG, ZG), H.P(A1, 0.0, Ridge), H.P(A0, 0.0, Ridge) }, Up);
			B.Poly(Pane, { H.P(A0, CG, ZG), H.P(A1, CG, ZG), H.P(A1, 0.0, Ridge), H.P(A0, 0.0, Ridge) }, -Up);
			// Glazing bars down the glass, and the purlin under its lower edge.
			for (double A = A0 + 100.0; A < A1 - 20.0; A += 200.0)
			{
				B.Bar(Beam, H.P(A, CG, ZG + 3.0), H.P(A, 0.0, Ridge + 3.0), 3.0, 3.0);
			}
			B.Bar(Beam, H.P(A0, CG, ZG - 10.0), H.P(A1, CG, ZG - 10.0), 8.0, 10.0);
		}
		B.Bar(Slate, H.P(A0, 0.0, Ridge + 6.0), H.P(A1, 0.0, Ridge + 6.0), 14.0, 8.0);
		// Iron trusses across the nave every other column.
		for (double A = A0 + ColumnStep; A < A1 - 50.0; A += ColumnStep * 2.0)
		{
			for (const double Side : { -1.0, 1.0 })
			{
				B.Bar(Beam, H.P(A, Side * NaveHalf, NaveEaves - 25.0), H.P(A, 0.0, Ridge - 25.0), 5.0, 9.0);
				B.Bar(Beam, H.P(A, Side * NaveHalf, NaveEaves - 25.0), H.P(A, Side * NaveHalf * 0.5, NaveEaves + 60.0), 3.0, 3.0);
			}
			B.Bar(Beam, H.P(A, -NaveHalf, NaveEaves - 25.0), H.P(A, NaveHalf, NaveEaves - 25.0), 4.0, 4.0);
		}
	}

	// An iron column with a foot and a capital.
	inline void Column(FBuilder& B, double X, double Y, double Top)
	{
		B.Box(Plinth, FVector(X - 26.0, Y - 26.0, 0.0), FVector(X + 26.0, Y + 26.0, 35.0));
		B.Frustum(Iron, X, Y, 20.0, 14.0, 35.0, 70.0, 8);
		B.Frustum(Iron, X, Y, 14.0, 12.0, 70.0, Top - 40.0, 8);
		B.Frustum(Iron, X, Y, 13.0, 26.0, Top - 40.0, Top - 10.0, 8);
		B.Box(Iron, FVector(X - 28.0, Y - 28.0, Top - 10.0), FVector(X + 28.0, Y + 28.0, Top));
	}

	// The columns of a hall along its lines at NaveHalf and InnerHalf, at the given places.
	inline void Columns(FBuilder& B, const FHall& H, std::initializer_list<double> Places)
	{
		for (const double A : Places)
		{
			for (const double C : { -InnerHalf, -NaveHalf, NaveHalf, InnerHalf })
			{
				const FVector At = H.P(A, C, 0.0);
				Column(B, At.X, At.Y, FMath::Abs(C) > NaveHalf + 1.0 ? OuterTop - 12.0 : InnerHigh - 12.0);
			}
		}
	}

	// One front, at the end of a hall: Face.Along runs across the hall.
	// bMain: the main front on Rákóczi tér, with the gateway for carts.
	inline void Front(FBuilder& B, const FFace& F, double Half, bool bMain)
	{
		// Openings per zone, from the left.
		const double OuterMid = (Half + InnerHalf) / 2.0;
		const double InnerMid = (InnerHalf + NaveHalf) / 2.0;
		const double GateHalf = bMain ? 300.0 : 220.0;
		const double GateSpring = bMain ? 450.0 : 380.0;
		const TArray<FOpening> Outer[2] = {
			{ { -OuterMid, 150.0, 250.0, 700.0, true, true } },
			{ { OuterMid, 150.0, 250.0, 700.0, true, true } } };
		const TArray<FOpening> Inner[2] = {
			{ { -InnerMid, 120.0, 0.0, 300.0, true, false }, },
			{ { InnerMid, 120.0, 0.0, 300.0, true, false }, } };
		const FOpening InnerWindows[2] = { { -InnerMid, 140.0, 760.0, 1240.0, true, true }, { InnerMid, 140.0, 760.0, 1240.0, true, true } };
		const FOpening Gate{ 0.0, GateHalf, 0.0, GateSpring, true, false };
		const FOpening Lunette{ 0.0, 450.0, 1000.0, 1350.0, true, true };

		for (int32 k = 0; k < 2; ++k)
		{
			const double Sgn = k == 0 ? -1.0 : 1.0;
			const double S0 = k == 0 ? -Half : InnerHalf, S1 = k == 0 ? -InnerHalf : Half;
			const double I0 = k == 0 ? -InnerHalf : NaveHalf, I1 = k == 0 ? -NaveHalf : InnerHalf;
			F.Wall(B, Brick, S0, S1, Base, OuterFrontTop, EndWall, Outer[k]);
			TArray<FOpening> InnerAll = Inner[k];
			InnerAll.Add(InnerWindows[k]);
			F.Wall(B, Brick, I0, I1, Base, InnerFrontTop, EndWall, InnerAll);
			for (const FOpening& O : Outer[k])
			{
				F.Surround(B, O, 22.0, 8.0);
				F.Block(B, Trim, O.Mid - O.Half - 30.0, O.Mid + O.Half + 30.0, O.Bottom - 16.0, O.Bottom, 14.0);
			}
			F.Surround(B, InnerAll[0], 22.0, 8.0);
			F.Surround(B, InnerWindows[k], 22.0, 8.0);
			F.Block(B, Trim, InnerWindows[k].Mid - 170.0, InnerWindows[k].Mid + 170.0, 744.0, 760.0, 14.0);
			// Plinth, string course and cornices.
			F.Band(B, Plinth, S0, S1, 0.0, 90.0, 8.0, Outer[k]);
			F.Band(B, Plinth, I0, I1, 0.0, 90.0, 8.0, InnerAll);
			F.Band(B, Trim, S0, S1, 540.0, 575.0, 12.0, TArray<FOpening>());
			F.Band(B, Trim, I0, I1, 540.0, 575.0, 12.0, InnerAll);
			F.Block(B, Trim, S0, S1, OuterFrontTop - 60.0, OuterFrontTop - 35.0, 14.0);
			F.Block(B, Trim, S0 - (k == 0 ? 20.0 : 0.0), S1 + (k == 1 ? 20.0 : 0.0), OuterFrontTop - 35.0, OuterFrontTop + 10.0, 26.0);
			F.Block(B, Trim, I0, I1, InnerFrontTop - 60.0, InnerFrontTop - 35.0, 14.0);
			F.Block(B, Trim, I0, I1, InnerFrontTop - 35.0, InnerFrontTop + 10.0, 26.0);
			// Brick pilasters with stone caps at the corners of the steps.
			for (const double S : { Sgn * Half, Sgn * InnerHalf })
			{
				const double Top = FMath::Abs(S) > InnerHalf + 1.0 ? OuterFrontTop : InnerFrontTop;
				const double PS = FMath::Abs(S) > InnerHalf + 1.0 ? S - Sgn * 40.0 : S;
				F.Block(B, Brick, PS - 40.0, PS + 40.0, 0.0, Top + 120.0, 24.0);
				F.Block(B, Trim, PS - 46.0, PS + 46.0, Top + 120.0, Top + 140.0, 30.0);
				const FVector Cap = F.At(PS, 0.0, 12.0);
				B.Frustum(Trim, Cap.X, Cap.Y, 40.0, 0.0, Top + 140.0, Top + 230.0, 4);
			}
		}

		// The nave front: the gateway, the name board, the lunette and the gable.
		TArray<FOpening> Nave = { Gate, Lunette };
		F.Wall(B, Brick, -NaveHalf, NaveHalf, Base, NaveFrontTop, EndWall, Nave);
		F.Surround(B, Gate, 36.0, 10.0);
		F.Surround(B, Lunette, 28.0, 8.0);
		F.Band(B, Plinth, -NaveHalf, NaveHalf, 0.0, 90.0, 8.0, Nave);
		F.Band(B, Trim, -NaveHalf, NaveHalf, 540.0, 575.0, 12.0, Nave);
		F.Block(B, Trim, -Lunette.Half - 50.0, Lunette.Half + 50.0, Lunette.Bottom - 18.0, Lunette.Bottom, 16.0);
		const double BoardBottom = Gate.Top() + 40.0 + 36.0;
		if (bMain)
		{
			F.Block(B, Sign, -360.0, 360.0, BoardBottom, BoardBottom + 95.0, 10.0);
			F.Block(B, Trim, -372.0, 372.0, BoardBottom - 12.0, BoardBottom, 14.0);
			F.Block(B, Trim, -372.0, 372.0, BoardBottom + 95.0, BoardBottom + 107.0, 14.0);
		}
		F.Block(B, Trim, -NaveHalf - 20.0, NaveHalf + 20.0, NaveFrontTop - 40.0, NaveFrontTop, 24.0);
		// The gable, with a stone coping and a round window.
		const double Thick = EndWall;
		B.Poly(Brick, { F.At(-NaveHalf, NaveFrontTop, 0.0), F.At(NaveHalf, NaveFrontTop, 0.0), F.At(0.0, GableTop, 0.0) }, F.Out);
		B.Poly(Brick, { F.At(-NaveHalf, NaveFrontTop, -Thick), F.At(NaveHalf, NaveFrontTop, -Thick), F.At(0.0, GableTop, -Thick) }, -F.Out);
		for (const double Sgn : { -1.0, 1.0 })
		{
			const FVector E0 = F.At(Sgn * (NaveHalf + 30.0), NaveFrontTop - 8.0, 14.0);
			const FVector E1 = F.At(0.0, GableTop + 22.0, 14.0);
			B.Bar(Trim, E0 - F.Out * 30.0, E1 - F.Out * 30.0, 44.0, 14.0);
		}
		const FVector Oculus = F.At(0.0, NaveFrontTop + 200.0, 2.0);
		for (int32 i = 0; i < 12; ++i)
		{
			const double A0 = 2.0 * PI * i / 12.0, A1 = 2.0 * PI * (i + 1) / 12.0;
			const FVector D0 = F.Along * FMath::Cos(A0) + FVector(0.0, 0.0, FMath::Sin(A0));
			const FVector D1 = F.Along * FMath::Cos(A1) + FVector(0.0, 0.0, FMath::Sin(A1));
			B.Poly(Glass, { Oculus, Oculus + D0 * 75.0, Oculus + D1 * 75.0 }, F.Out);
			B.Poly(Trim, { Oculus + D0 * 75.0 + F.Out * 6.0, Oculus + D0 * 100.0 + F.Out * 6.0, Oculus + D1 * 100.0 + F.Out * 6.0, Oculus + D1 * 75.0 + F.Out * 6.0 }, F.Out);
		}
		// Turrets either side of the gable, and a finial on top.
		for (const double Sgn : { -1.0, 1.0 })
		{
			const double S = Sgn * (NaveHalf + 10.0);
			B.BoxBetween(Brick, F.At(S - 55.0, 0.0, 34.0), F.At(S + 55.0, TurretTop, -EndWall));
			F.Block(B, Trim, S - 62.0, S + 62.0, 540.0, 575.0, 40.0);
			F.Block(B, Trim, S - 62.0, S + 62.0, TurretTop - 160.0, TurretTop - 140.0, 40.0);
			F.Block(B, Trim, S - 64.0, S + 64.0, TurretTop, TurretTop + 24.0, 42.0);
			const FVector Cap = F.At(S, 0.0, -13.0);
			B.Frustum(Slate, Cap.X, Cap.Y, 62.0, 0.0, TurretTop + 24.0, TurretTop + 260.0, 4);
			B.Frustum(Iron, Cap.X, Cap.Y, 3.0, 2.0, TurretTop + 250.0, TurretTop + 360.0, 4);
		}
		const FVector Top = F.At(0.0, 0.0, 0.0);
		B.Frustum(Trim, Top.X - F.Out.X * 30.0, Top.Y - F.Out.Y * 30.0, 30.0, 18.0, GableTop + 10.0, GableTop + 90.0, 8);
		B.Frustum(Iron, Top.X - F.Out.X * 30.0, Top.Y - F.Out.Y * 30.0, 4.0, 2.0, GableTop + 90.0, GableTop + 260.0, 4);

		// Inside the gateway: the leaves of the iron gate folded back against the reveals.
		for (const double Sgn : { -1.0, 1.0 })
		{
			const double S = Sgn * (GateHalf - 4.0);
			B.TwoSided(Iron, { F.At(S, 0.0, -EndWall - 2.0), F.At(S, GateSpring, -EndWall - 2.0),
				F.At(S, GateSpring, -EndWall - 2.0 - GateHalf * 0.9), F.At(S, 0.0, -EndWall - 2.0 - GateHalf * 0.9) }, F.Along * -Sgn);
		}
	}

	// A corner pavilion between (X0, Y0) and (X1, Y1). StreetX / StreetY:
	// which of its sides face the street (-1 the low side, +1 the high side).
	inline void CornerPavilion(FBuilder& B, double X0, double Y0, double X1, double Y1, double StreetX, double StreetY)
	{
		B.Box(Brick, FVector(X0, Y0, Base), FVector(X1, Y1, PavilionTop));
		const double CX = (X0 + X1) / 2.0, CY = (Y0 + Y1) / 2.0;
		const double HX = (X1 - X0) / 2.0, HY = (Y1 - Y0) / 2.0;
		struct FSide { FFace Face; double Length; bool bStreet; };
		const FSide Sides[4] = {
			{ { FVector(CX, Y0, 0.0), FVector(1.0, 0.0, 0.0), FVector(0.0, -1.0, 0.0) }, HX, StreetY < 0.0 },
			{ { FVector(CX, Y1, 0.0), FVector(-1.0, 0.0, 0.0), FVector(0.0, 1.0, 0.0) }, HX, StreetY > 0.0 },
			{ { FVector(X0, CY, 0.0), FVector(0.0, -1.0, 0.0), FVector(-1.0, 0.0, 0.0) }, HY, StreetX < 0.0 },
			{ { FVector(X1, CY, 0.0), FVector(0.0, 1.0, 0.0), FVector(1.0, 0.0, 0.0) }, HY, StreetX > 0.0 } };
		for (const FSide& Side : Sides)
		{
			const FFace& F = Side.Face;
			const double L = Side.Length;
			// Plinth, string courses, cornice; stone quoins at the corners.
			F.Block(B, Trim, -L - 20.0, L + 20.0, PavilionTop - 70.0, PavilionTop, 30.0);
			F.Block(B, Trim, -L, L, PavilionTop - 110.0, PavilionTop - 70.0, 12.0);
			for (double Z = 0.0; Z < PavilionTop - 120.0; Z += 70.0)
			{
				const double W = (int32(Z / 70.0) % 2 == 0) ? 70.0 : 45.0;
				F.Block(B, Trim, -L, -L + W, Z, Z + 62.0, 6.0);
				F.Block(B, Trim, L - W, L, Z, Z + 62.0, 6.0);
			}
			if (!Side.bStreet)
			{
				// Towards the hall: the top floor's windows look out over the aisle roofs.
				for (const double S : { -L * 0.5, 0.0, L * 0.5 })
				{
					F.FlatWindow(B, S, 55.0, SecondFloor + 120.0, SecondFloor + 300.0, false);
				}
				F.Panel(B, Wood, -70.0, 70.0, 0.0, 280.0, 2.0);
				F.Block(B, Trim, -86.0, 86.0, 280.0, 300.0, 10.0);
				continue;
			}
			F.Block(B, Plinth, -L, L, 0.0, 90.0, 8.0);
			F.Block(B, Trim, -L, L, FirstFloor - 20.0, FirstFloor + 20.0, 14.0);
			F.Block(B, Trim, -L, L, SecondFloor - 10.0, SecondFloor + 20.0, 12.0);
			const int32 Bays = 4;
			for (int32 k = 0; k < Bays; ++k)
			{
				const double S = -L + 2.0 * L * (k + 0.5) / Bays;
				// Shops on the ground floor, the middle two with doors.
				if (k == 1 || k == 2)
				{
					F.Panel(B, Wood, S - 70.0, S + 70.0, 0.0, 260.0, 3.0);
					F.Panel(B, Glass, S - 60.0, S + 60.0, 280.0, 400.0, 3.0);
					F.Block(B, Trim, S - 86.0, S + 86.0, 260.0, 280.0, 12.0);
					const FOpening O{ S, 70.0, 0.0, 400.0, false, false };
					F.Surround(B, O, 16.0, 8.0);
				}
				else
				{
					F.FlatWindow(B, S, 80.0, 110.0, 420.0, true);
				}
				F.FlatWindow(B, S, 60.0, FirstFloor + 90.0, SecondFloor - 50.0, false);
				F.Block(B, Trim, S - 78.0, S + 78.0, SecondFloor - 50.0, SecondFloor - 28.0, 16.0);
				F.FlatWindow(B, S, 55.0, SecondFloor + 110.0, PavilionTop - 140.0, false);
			}
		}
		// A steep hipped roof like a tower's, with a stone crest and a finial.
		B.HipRing(Slate, Slate, CX, CY, HX + 30.0, HY + 30.0, HX * 0.45, HY * 0.45, PavilionTop, PavilionRoofTop - 150.0);
		B.HipRing(Slate, Slate, CX, CY, HX * 0.45, HY * 0.45, HX * 0.2, HY * 0.2, PavilionRoofTop - 150.0, PavilionRoofTop);
		B.Lid(Slate, CX, CY, HX * 0.2, HY * 0.2, PavilionRoofTop);
		B.Frustum(Iron, CX, CY, 20.0, 4.0, PavilionRoofTop, PavilionRoofTop + 120.0, 6);
		B.Frustum(Iron, CX, CY, 3.0, 2.0, PavilionRoofTop + 120.0, PavilionRoofTop + 320.0, 4);
		// A dormer in the middle of each street side.
		auto Dormer = [&](const FVector& At, const FVector& Out, const FVector& Along)
		{
			const FVector Up(0.0, 0.0, 1.0);
			const FVector Foot = At + Up * PavilionTop;
			B.Bar(Brick, Foot + Up * 115.0, Foot - Out * 110.0 + Up * 115.0, 70.0, 115.0);
			const FFace F{ Foot + Out * 0.0, Along, Out };
			F.Panel(B, Glass, -40.0, 40.0, 40.0, 170.0, 1.0);
			F.Block(B, Trim, -52.0, 52.0, 170.0, 186.0, 8.0);
			B.Poly(Slate, { Foot + Along * 85.0 + Up * 225.0 + Out * 10.0, Foot - Along * 85.0 + Up * 225.0 + Out * 10.0, Foot + Up * 300.0 - Out * 60.0 }, Up + Out);
			B.Poly(Slate, { Foot + Along * 85.0 + Up * 225.0 + Out * 10.0, Foot + Up * 300.0 - Out * 60.0, Foot + Up * 260.0 - Out * 150.0 + Along * 85.0 }, Up + Along);
			B.Poly(Slate, { Foot - Along * 85.0 + Up * 225.0 + Out * 10.0, Foot + Up * 300.0 - Out * 60.0, Foot + Up * 260.0 - Out * 150.0 - Along * 85.0 }, Up - Along);
		};
		if (StreetY != 0.0)
		{
			Dormer(FVector(CX, StreetY < 0.0 ? Y0 : Y1, 0.0) + FVector(0.0, StreetY < 0.0 ? 60.0 : -60.0, 0.0), FVector(0.0, StreetY, 0.0), FVector(1.0, 0.0, 0.0));
		}
		if (StreetX != 0.0)
		{
			Dormer(FVector(StreetX < 0.0 ? X0 : X1, CY, 0.0) + FVector(StreetX < 0.0 ? 60.0 : -60.0, 0.0, 0.0), FVector(StreetX, 0.0, 0.0), FVector(0.0, 1.0, 0.0));
		}
	}

	// The raised square over the crossing: its four walls stand on the
	// halls' cross sections and carry a band of windows above every roof.
	inline void Centre(FBuilder& B)
	{
		const double CX = CrossX;
		const double H = CentreHalf;
		const double Thick = 40.0;
		const FFace Faces[4] = {
			{ FVector(CX - H, 0.0, 0.0), FVector(0.0, 1.0, 0.0), FVector(-1.0, 0.0, 0.0) },
			{ FVector(CX + H, 0.0, 0.0), FVector(0.0, -1.0, 0.0), FVector(1.0, 0.0, 0.0) },
			{ FVector(CX, -H, 0.0), FVector(-1.0, 0.0, 0.0), FVector(0.0, -1.0, 0.0) },
			{ FVector(CX, H, 0.0), FVector(1.0, 0.0, 0.0), FVector(0.0, 1.0, 0.0) } };
		TArray<FOpening> Windows;
		const int32 Count = 7;
		for (int32 k = 0; k < Count; ++k)
		{
			const double S = -H + 2.0 * H * (k + 0.5) / Count;
			Windows.Add({ S, 105.0, CentreBand + 30.0, CentreTop - 160.0, true, true });
		}
		for (const FFace& F : Faces)
		{
			// Below the window band the wall comes down onto the hall's roofs.
			auto Lower = [&](double S0, double Z0, double S1, double Z1)
			{
				B.Poly(Brick, { F.At(S0, Z0, 0.0), F.At(S1, Z1, 0.0), F.At(S1, CentreBand, 0.0), F.At(S0, CentreBand, 0.0) }, F.Out);
				B.Poly(Brick, { F.At(S0, Z0, -Thick), F.At(S1, Z1, -Thick), F.At(S1, CentreBand, -Thick), F.At(S0, CentreBand, -Thick) }, -F.Out);
				B.Poly(Brick, { F.At(S0, Z0, 0.0), F.At(S1, Z1, 0.0), F.At(S1, Z1, -Thick), F.At(S0, Z0, -Thick) }, FVector(0.0, 0.0, -1.0));
			};
			Lower(-H, InnerTop, -NaveHalf, InnerHigh);
			Lower(-NaveHalf, NaveEaves, 0.0, Ridge);
			Lower(0.0, Ridge, NaveHalf, NaveEaves);
			Lower(NaveHalf, InnerHigh, H, InnerTop);
			for (const double S : { -NaveHalf, NaveHalf })
			{
				B.Poly(Brick, { F.At(S, InnerHigh, 0.0), F.At(S, NaveEaves, 0.0), F.At(S, NaveEaves, -Thick), F.At(S, InnerHigh, -Thick) }, -F.Along * (S < 0.0 ? 1.0 : -1.0));
			}
			F.Wall(B, Brick, -H, H, CentreBand, CentreTop, Thick, Windows);
			for (const FOpening& O : Windows)
			{
				F.Surround(B, O, 16.0, 6.0);
			}
			F.Block(B, Trim, -H - 30.0, H + 30.0, CentreBand - 10.0, CentreBand + 18.0, 12.0);
			F.Block(B, Trim, -H - 30.0, H + 30.0, CentreTop - 50.0, CentreTop, 30.0);
			for (const double S : { -H, H })
			{
				F.Block(B, Trim, S - 30.0, S + 30.0, CentreBand + 18.0, CentreTop - 50.0, 14.0);
			}
		}
		// Hipped roof, a glazed lantern and a flagpole.
		B.HipRing(Slate, RoofIn, CX, 0.0, H + 50.0, H + 50.0, 420.0, 420.0, CentreTop, CentreRoofTop);
		B.Poly(RoofIn, { FVector(CX - 420.0, -420.0, CentreRoofTop), FVector(CX + 420.0, -420.0, CentreRoofTop), FVector(CX + 420.0, 420.0, CentreRoofTop), FVector(CX - 420.0, 420.0, CentreRoofTop) }, FVector(0.0, 0.0, -1.0));
		B.Lid(Slate, CX, 0.0, 420.0, 420.0, CentreRoofTop);
		B.Box(Iron, FVector(CX - 260.0, -260.0, CentreRoofTop), FVector(CX + 260.0, 260.0, CentreRoofTop + 20.0));
		for (int32 i = 0; i < 4; ++i)
		{
			const double A = PI / 2.0 * i;
			const FVector D(FMath::Cos(A), FMath::Sin(A), 0.0), Sd(-FMath::Sin(A), FMath::Cos(A), 0.0);
			const FVector C(CX, 0.0, 0.0);
			B.TwoSided(RoofGlass, { C + D * 250.0 - Sd * 250.0 + FVector(0.0, 0.0, CentreRoofTop + 20.0), C + D * 250.0 + Sd * 250.0 + FVector(0.0, 0.0, CentreRoofTop + 20.0),
				C + D * 250.0 + Sd * 250.0 + FVector(0.0, 0.0, CentreRoofTop + 170.0), C + D * 250.0 - Sd * 250.0 + FVector(0.0, 0.0, CentreRoofTop + 170.0) }, D);
		}
		B.HipRing(Slate, Slate, CX, 0.0, 280.0, 280.0, 0.0, 0.0, CentreRoofTop + 170.0, CentreRoofTop + 330.0);
		B.Frustum(Iron, CX, 0.0, 5.0, 3.0, CentreRoofTop + 330.0, CentreRoofTop + 900.0, 6);
	}

	// A covered stall inside: a counter towards the aisle, a back frame with
	// a shelf, and the seller's name board on top. +X towards the aisle.
	struct FStall
	{
		FVector At;
		double Yaw;
	};

	// Stalls stand in rows along the outer and inner aisles, backs to the walls of the aisles.
	inline void StallSpots(TArray<FStall>& Out)
	{
		const double Pitch = 300.0;
		// The hall from the front to the back: rows along the outer aisles, backs to the pavilions.
		const double Row = (LongHalf + InnerHalf) / 2.0 + 40.0;
		for (double A = 400.0; A <= PavilionX - 150.0; A += Pitch)
		{
			for (const double X : { A, BackFront - A })
			{
				Out.Add({ FVector(X, -Row, 0.0), 90.0 });
				Out.Add({ FVector(X, Row, 0.0), -90.0 });
			}
		}
		// The hall across: rows in its outer aisles, backs to the pavilions.
		const double RowX = (CrossHalf + InnerHalf) / 2.0 + 20.0;
		for (const double Sgn : { -1.0, 1.0 })
		{
			for (double Y = InnerHalf + 300.0; Y < SideFront - 250.0; Y += Pitch)
			{
				Out.Add({ FVector(CrossX - RowX, Sgn * Y, 0.0), 0.0 });
				Out.Add({ FVector(CrossX + RowX, Sgn * Y, 0.0), 180.0 });
			}
		}
		// Islands of stalls in the inner aisles, facing the nave.
		const double Mid = (InnerHalf + NaveHalf) / 2.0;
		for (const double Sgn : { -1.0, 1.0 })
		{
			const double Facing = Sgn > 0.0 ? -90.0 : 90.0;
			for (double A = 700.0; A < PavilionX; A += Pitch)
			{
				Out.Add({ FVector(A, Sgn * Mid, 0.0), Facing });
				Out.Add({ FVector(BackFront - A, Sgn * Mid, 0.0), Facing });
			}
			for (double Y = InnerHalf + 500.0; Y < SideFront - 350.0; Y += Pitch)
			{
				Out.Add({ FVector(CrossX - Mid, Sgn * Y, 0.0), 0.0 });
				Out.Add({ FVector(CrossX + Mid, Sgn * Y, 0.0), 180.0 });
			}
		}
	}

	// The booth round a stall (its goods come from MarketModel).
	inline void Booth(FBuilder& B, const FStall& S)
	{
		const double A = FMath::DegreesToRadians(S.Yaw);
		const FVector Fwd(FMath::Cos(A), FMath::Sin(A), 0.0), Side(-FMath::Sin(A), FMath::Cos(A), 0.0);
		auto P = [&](double X, double Y, double Z) { return S.At + Fwd * X + Side * Y + FVector(0.0, 0.0, Z); };
		const double HW = 145.0;
		// Corner posts, the back boards with a shelf, the board on top.
		for (const double Y : { -HW, HW })
		{
			B.Bar(Wood, P(-110.0, Y, 0.0), P(-110.0, Y, 300.0), 5.0, 5.0);
			B.Bar(Wood, P(55.0, Y, 0.0), P(55.0, Y, 300.0), 5.0, 5.0);
			B.Bar(Wood, P(-110.0, Y, 295.0), P(55.0, Y, 295.0), 4.0, 4.0);
		}
		B.TwoSided(Wood, { P(-112.0, -HW, 0.0), P(-112.0, HW, 0.0), P(-112.0, HW, 220.0), P(-112.0, -HW, 220.0) }, Fwd);
		B.Bar(Wood, P(-95.0, -HW, 150.0), P(-95.0, HW, 150.0), 15.0, 2.0);
		B.Bar(Sign, P(58.0, -HW, 270.0), P(58.0, HW, 270.0), 3.0, 22.0);
		B.Bar(Wood, P(58.0, -HW - 5.0, 296.0), P(58.0, HW + 5.0, 296.0), 4.0, 4.0);
	}

	inline void Build(FBuilder& B)
	{
		const FHall Long{ false, LongHalf };   // from the main front to Víg utca
		const FHall Cross{ true, CrossHalf };  // from Vásár utca to Déri Miksa utca

		// The floor.
		B.Box(Floor, FVector(0.0, -SideFront, Base), FVector(BackFront, SideFront, 3.0));

		// The halls' roofs. The long hall stops where it meets the hall across:
		// its outer aisles at that hall's side, the rest at the raised square.
		const double LongInnerEnd = CrossX - CentreHalf;
		OuterRoofs(B, Long, 0.0, PavilionX);
		OuterRoofs(B, Long, Depth - PavilionX, BackFront);
		InnerRoofs(B, Long, 0.0, PavilionX, OuterTop, OuterTop);
		InnerRoofs(B, Long, PavilionX, LongInnerEnd, OuterEaves, OuterTop);
		InnerRoofs(B, Long, Depth - LongInnerEnd, Depth - PavilionX, OuterTop, OuterEaves);
		InnerRoofs(B, Long, Depth - PavilionX, BackFront, OuterTop, OuterTop);
		OuterRoofs(B, Cross, -SideFront, -InnerHalf);
		OuterRoofs(B, Cross, InnerHalf, SideFront);
		InnerRoofs(B, Cross, -SideFront, -CentreHalf);
		InnerRoofs(B, Cross, CentreHalf, SideFront);
		// Where the long hall's outer aisle ends against the hall across.
		for (const double X : { PavilionX, Depth - PavilionX })
		{
			for (const double Sgn : { -1.0, 1.0 })
			{
				B.TwoSided(Brick, { FVector(X, Sgn * LongHalf, OuterEaves - 15.0), FVector(X, Sgn * InnerHalf, OuterTop), FVector(X, Sgn * InnerHalf, OuterEaves - 15.0) }, FVector(1.0, 0.0, 0.0));
			}
		}
		Centre(B);

		// The columns.
		Columns(B, Long, { ColumnStep, ColumnStep * 2.0, PavilionX, LongInnerEnd, Depth - LongInnerEnd, Depth - PavilionX, BackFront - ColumnStep * 2.0, BackFront - ColumnStep });
		Columns(B, Cross, { -InnerHalf - ColumnStep * 3.0, -InnerHalf - ColumnStep * 2.0, -InnerHalf - ColumnStep, -InnerHalf,
			InnerHalf, InnerHalf + ColumnStep, InnerHalf + ColumnStep * 2.0, InnerHalf + ColumnStep * 3.0 });

		// The four fronts.
		Front(B, { FVector(0.0, 0.0, 0.0), FVector(0.0, 1.0, 0.0), FVector(-1.0, 0.0, 0.0) }, LongHalf, true);
		Front(B, { FVector(BackFront, 0.0, 0.0), FVector(0.0, -1.0, 0.0), FVector(1.0, 0.0, 0.0) }, LongHalf, false);
		Front(B, { FVector(CrossX, -SideFront, 0.0), FVector(-1.0, 0.0, 0.0), FVector(0.0, -1.0, 0.0) }, CrossHalf, false);
		Front(B, { FVector(CrossX, SideFront, 0.0), FVector(1.0, 0.0, 0.0), FVector(0.0, 1.0, 0.0) }, CrossHalf, false);

		// The corner pavilions.
		CornerPavilion(B, 0.0, -HalfFront, PavilionX, -LongHalf, -1.0, -1.0);
		CornerPavilion(B, 0.0, LongHalf, PavilionX, HalfFront, -1.0, 1.0);
		CornerPavilion(B, Depth - PavilionX, -HalfFront, Depth, -LongHalf, 1.0, -1.0);
		CornerPavilion(B, Depth - PavilionX, LongHalf, Depth, HalfFront, 1.0, 1.0);

		// The stalls' booths.
		TArray<FStall> Stalls;
		StallSpots(Stalls);
		for (const FStall& S : Stalls)
		{
			Booth(B, S);
		}
	}
}
