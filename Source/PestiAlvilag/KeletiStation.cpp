// Fill out your copyright notice in the Description page of Project Settings.

#include "KeletiStation.h"

#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace Keleti
{
	// One mesh section (and colour) per material. The parts up to RoofOut
	// are solid; the roof and its trusses are out of reach and need no
	// collision. The roof glass is drawn facing the sky only: from inside
	// the hall it is not there at all, so the sky shows through and the
	// sun and the sky light fall on the platforms as through clear glass.
	// The arch and the far end have their iron bars and no panes at all.
	enum EPart : int32
	{
		Stone,
		Rustic,
		Trim,
		Glass,
		Door,
		Slate,
		Bronze,
		Iron,
		Paving,
		Ballast,
		Steel,
		CarGreen,
		CarRoof,
		Black,
		Red,
		Lawn,
		Canvas,
		Wood,
		RoofOut,
		RoofIn,
		Truss,
		Pane,
		PartCount
	};
	constexpr int32 FirstOverheadPart = RoofOut;
	constexpr int32 FirstGlazingPart = Pane;

	// Sizes (cm). X runs from the main front into the hall, Y across it
	// (+Y towards Kerepesi út), Z up from the pavement.
	constexpr double Base = -100.0;            // walls start below the pavement so slopes don't show a gap

	// The main front: the arch wall between two towers that stand forward of it.
	constexpr double TowerFrontX = -200.0;
	constexpr double TowerBackX = 400.0;
	constexpr double ArchWallDepth = 300.0;
	constexpr double FrontHalf = 2300.0;       // the towers reach out to here
	constexpr double TowerInner = 1300.0;      // and in to here, where the arch wall begins
	constexpr double TowerRusticTop = 1100.0;
	constexpr double ColumnTop = 2400.0;
	constexpr double TowerTop = 2850.0;
	constexpr double ArchRadius = 1250.0;
	constexpr double ArchSpring = 1300.0;
	constexpr double ArchRing = 110.0;         // the moulded band round the arch
	constexpr double ColonnadeTop = 650.0;
	constexpr double BandTop = 790.0;          // the entablature over the colonnade
	constexpr double FrontEntablature = 2650.0;
	constexpr double FrontEntablatureTop = 2800.0;
	constexpr double AtticHalf = 850.0;
	constexpr double AtticTop = 3250.0;

	// The wings either side of the hall.
	constexpr double WingFrontX = 400.0;
	constexpr double EndX = 18510.0;
	constexpr double HallHalf = 2150.0;        // the hall's inside walls
	constexpr double RusticTop = 800.0;
	constexpr double UpperSill = 950.0;
	constexpr double UpperSpring = 1450.0;
	constexpr double EntablatureZ = 1700.0;
	constexpr double CorniceTop = 1950.0;
	constexpr double MansardTop = 2500.0;
	constexpr double BayWidth = 450.0;

	// The hall.
	constexpr double HallWallTop = 2500.0;
	constexpr double RidgeZ = 3600.0;
	constexpr double TrussSpring = 2300.0;
	constexpr double TrussApex = 3200.0;
	constexpr int32 HallBays = 18;
	constexpr double EndScreenBottom = 900.0;  // the glass screen over the tracks at the far end
	constexpr double GlassFrom = 0.12;         // the glazed strip of each roof slope, from eaves (0) to ridge (1)
	constexpr double GlassTo = 0.94;

	// Tracks and platforms.
	constexpr double FloorTop = 3.0;
	constexpr double BallastTop = 12.0;
	constexpr double RailTop = 30.0;
	constexpr double PlatformTop = 40.0;
	constexpr double PlatformStart = 1700.0;
	constexpr double PlatformEnd = 17300.0;
	constexpr double BufferX = 1900.0;
	constexpr double TrackEnd = 26000.0;
	constexpr double Gauge = 144.0;
	constexpr double TrackY[] = { 1075.0, 625.0, -625.0, -1075.0 };
	constexpr double PlatformY[][2] = { { 1300.0, HallHalf }, { -400.0, 400.0 }, { -HallHalf, -1300.0 } };
	constexpr double CarriageLength = 2000.0;
	constexpr double CarriagePitch = 2070.0;

	// The Baross statue on its lawn in front of the station.
	const FVector2D StatueAt(-3466.0, 0.0);

	// Lettering (placed by the actor).
	const FVector FriezeTextAt(-62.0, 0.0, 2725.0);
	const FVector AtticTextAt(-42.0, 0.0, 3030.0);
	const FVector RestaurantTextAt(77.0, -3590.0, 480.0);
	const FVector StatueTextAt(-3466.0 - 173.0, 0.0, 500.0);

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

		// The same polygon seen from both sides (glass, canvas).
		void TwoSided(EPart Part, std::initializer_list<FVector> Points, const FVector& Hint)
		{
			Poly(Part, Points, Hint);
			Poly(Part, Points, -Hint);
		}

		// An axis-aligned box without its bottom.
		void Box(EPart Part, const FVector& Min, const FVector& Max)
		{
			const FVector C[8] = {
				FVector(Min.X, Min.Y, Min.Z), FVector(Max.X, Min.Y, Min.Z), FVector(Max.X, Max.Y, Min.Z), FVector(Min.X, Max.Y, Min.Z),
				FVector(Min.X, Min.Y, Max.Z), FVector(Max.X, Min.Y, Max.Z), FVector(Max.X, Max.Y, Max.Z), FVector(Min.X, Max.Y, Max.Z) };
			Poly(Part, { C[4], C[5], C[6], C[7] }, FVector::UpVector);
			Poly(Part, { C[1], C[2], C[6], C[5] }, FVector::ForwardVector);
			Poly(Part, { C[0], C[3], C[7], C[4] }, FVector::BackwardVector);
			Poly(Part, { C[2], C[3], C[7], C[6] }, FVector::RightVector);
			Poly(Part, { C[0], C[1], C[5], C[4] }, FVector::LeftVector);
			// Anything lifted off the ground (cornices, ledges, sills) can be seen from below.
			if (Min.Z > 0.5)
			{
				Poly(Part, { C[0], C[3], C[2], C[1] }, FVector::DownVector);
			}
		}

		// A round shaft narrowing from R0 to R1, with a lid when bTop.
		void Frustum(EPart Part, const FVector2D& Center, double R0, double R1, double Z0, double Z1, int32 Sides, bool bTop = true)
		{
			for (int32 i = 0; i < Sides; ++i)
			{
				const double A0 = 2.0 * PI * i / Sides;
				const double A1 = 2.0 * PI * (i + 1) / Sides;
				const FVector D0(FMath::Cos(A0), FMath::Sin(A0), 0.0);
				const FVector D1(FMath::Cos(A1), FMath::Sin(A1), 0.0);
				const FVector C0(Center, Z0);
				const FVector C1(Center, Z1);
				Poly(Part, { C0 + D0 * R0, C0 + D1 * R0, C1 + D1 * R1, C1 + D0 * R1 }, D0 + D1);
				if (bTop && R1 > 0.0)
				{
					Poly(Part, { C1, C1 + D0 * R1, C1 + D1 * R1 }, FVector::UpVector);
				}
			}
		}

		void Cylinder(EPart Part, const FVector2D& Center, double R, double Z0, double Z1, int32 Sides)
		{
			Frustum(Part, Center, R, R, Z0, Z1, Sides);
		}

		// A round bar from P0 to P1 in any direction, closed at both ends:
		// wheels, boilers, buffers, and (with four sides) iron girders.
		void Tube(EPart Part, const FVector& P0, const FVector& P1, double R, int32 Sides)
		{
			const FVector Axis = (P1 - P0).GetSafeNormal();
			const FVector Helper = FMath::Abs(Axis.Z) > 0.9 ? FVector::ForwardVector : FVector::UpVector;
			const FVector U = FVector::CrossProduct(Axis, Helper).GetSafeNormal();
			const FVector V = FVector::CrossProduct(Axis, U);
			const double Turn = Sides == 4 ? PI / 4.0 : 0.0;
			for (int32 i = 0; i < Sides; ++i)
			{
				const double A0 = Turn + 2.0 * PI * i / Sides;
				const double A1 = Turn + 2.0 * PI * (i + 1) / Sides;
				const FVector D0 = U * FMath::Cos(A0) + V * FMath::Sin(A0);
				const FVector D1 = U * FMath::Cos(A1) + V * FMath::Sin(A1);
				Poly(Part, { P0 + D0 * R, P0 + D1 * R, P1 + D1 * R, P1 + D0 * R }, D0 + D1);
				Poly(Part, { P0, P0 + D0 * R, P0 + D1 * R }, -Axis);
				Poly(Part, { P1, P1 + D0 * R, P1 + D1 * R }, Axis);
			}
		}

		// A ring of a square dome: the rectangle HX0 by HY0 (half sizes) at
		// Z0 narrowing to HX1 by HY1 at Z1.
		void SquareRing(EPart Part, const FVector2D& Center, double HX0, double HY0, double HX1, double HY1, double Z0, double Z1)
		{
			const FVector C0(Center, Z0), C1(Center, Z1);
			const FVector Lo[4] = { C0 + FVector(-HX0, -HY0, 0.0), C0 + FVector(HX0, -HY0, 0.0), C0 + FVector(HX0, HY0, 0.0), C0 + FVector(-HX0, HY0, 0.0) };
			const FVector Hi[4] = { C1 + FVector(-HX1, -HY1, 0.0), C1 + FVector(HX1, -HY1, 0.0), C1 + FVector(HX1, HY1, 0.0), C1 + FVector(-HX1, HY1, 0.0) };
			const FVector Outs[4] = { FVector(0.0, -1.0, 0.5), FVector(1.0, 0.0, 0.5), FVector(0.0, 1.0, 0.5), FVector(-1.0, 0.0, 0.5) };
			for (int32 i = 0; i < 4; ++i)
			{
				const int32 j = (i + 1) % 4;
				Poly(Part, { Lo[i], Lo[j], Hi[j], Hi[i] }, Outs[i]);
			}
		}

		// A gable: a triangular prism running along X (bAlongX) or Y, its
		// ridge Rise above Min.Z.
		void Gable(EPart Part, const FVector& Min, const FVector& Max, double Rise, bool bAlongX)
		{
			const double Z0 = Min.Z;
			const double Z1 = Min.Z + Rise;
			if (bAlongX)
			{
				const double Mid = (Min.Y + Max.Y) / 2.0;
				for (const double X : { Min.X, Max.X })
				{
					Poly(Part, { FVector(X, Min.Y, Z0), FVector(X, Max.Y, Z0), FVector(X, Mid, Z1) }, FVector(X == Max.X ? 1.0 : -1.0, 0.0, 0.0));
				}
				Poly(Part, { FVector(Min.X, Min.Y, Z0), FVector(Max.X, Min.Y, Z0), FVector(Max.X, Mid, Z1), FVector(Min.X, Mid, Z1) }, FVector(0.0, -1.0, 1.0));
				Poly(Part, { FVector(Min.X, Max.Y, Z0), FVector(Max.X, Max.Y, Z0), FVector(Max.X, Mid, Z1), FVector(Min.X, Mid, Z1) }, FVector(0.0, 1.0, 1.0));
			}
			else
			{
				const double Mid = (Min.X + Max.X) / 2.0;
				for (const double Y : { Min.Y, Max.Y })
				{
					Poly(Part, { FVector(Min.X, Y, Z0), FVector(Max.X, Y, Z0), FVector(Mid, Y, Z1) }, FVector(0.0, Y == Max.Y ? 1.0 : -1.0, 0.0));
				}
				Poly(Part, { FVector(Min.X, Min.Y, Z0), FVector(Min.X, Max.Y, Z0), FVector(Mid, Max.Y, Z1), FVector(Mid, Min.Y, Z1) }, FVector(-1.0, 0.0, 1.0));
				Poly(Part, { FVector(Max.X, Min.Y, Z0), FVector(Max.X, Max.Y, Z0), FVector(Mid, Max.Y, Z1), FVector(Mid, Min.Y, Z1) }, FVector(1.0, 0.0, 1.0));
			}
		}

		// A box turned to run along the level line from P0 to P1,
		// HalfDepth to either side of it, from Z0 to Z1.
		void LineBox(EPart Part, const FVector& P0, const FVector& P1, double HalfDepth, double Z0, double Z1)
		{
			const FVector Along = FVector(P1.X - P0.X, P1.Y - P0.Y, 0.0).GetSafeNormal();
			const FVector Across(-Along.Y, Along.X, 0.0);
			const FVector Lo(0.0, 0.0, Z0), Hi(0.0, 0.0, Z1);
			const FVector A = FVector(P0.X, P0.Y, 0.0) - Across * HalfDepth, B = FVector(P1.X, P1.Y, 0.0) - Across * HalfDepth;
			const FVector C = FVector(P1.X, P1.Y, 0.0) + Across * HalfDepth, D = FVector(P0.X, P0.Y, 0.0) + Across * HalfDepth;
			Poly(Part, { A + Hi, B + Hi, C + Hi, D + Hi }, FVector::UpVector);
			Poly(Part, { A + Lo, B + Lo, B + Hi, A + Hi }, -Across);
			Poly(Part, { D + Lo, C + Lo, C + Hi, D + Hi }, Across);
			Poly(Part, { A + Lo, D + Lo, D + Hi, A + Hi }, -Along);
			Poly(Part, { B + Lo, C + Lo, C + Hi, B + Hi }, Along);
		}

		// A balustrade from P0 to P1: plinth, a baluster every 35 cm, a
		// post every few metres, and a rail.
		void Balustrade(EPart Part, const FVector& P0, const FVector& P1, double Height, bool bPosts = true)
		{
			const FVector Along = (P1 - P0).GetSafeNormal();
			const double Length = FVector::Dist(P0, P1);
			auto Slab = [&](double S0, double S1, double Z0, double Z1, double HalfDepth)
			{
				LineBox(Part, P0 + Along * S0, P0 + Along * S1, HalfDepth, P0.Z + Z0, P0.Z + Z1);
			};
			Slab(0.0, Length, 0.0, 18.0, 22.0);
			Slab(0.0, Length, Height - 14.0, Height, 20.0);
			const int32 Posts = bPosts ? FMath::Max(1, FMath::RoundToInt(Length / 320.0)) : 0;
			for (int32 i = 0; i <= Posts && bPosts; ++i)
			{
				const double S = FMath::Clamp(Length * i / Posts, 18.0, Length - 18.0);
				Slab(S - 18.0, S + 18.0, 18.0, Height - 14.0, 18.0);
			}
			for (double S = 35.0; S < Length - 20.0; S += 35.0)
			{
				Slab(S - 6.0, S + 6.0, 18.0, Height - 14.0, 6.0);
			}
		}

		// A standing figure on a plinth, Scale 1 being a little over life size.
		void Figure(EPart Part, const FVector& At, double Scale, bool bArmRaised)
		{
			const double S = Scale;
			Box(Part, At + FVector(-40.0, -40.0, 0.0) * S, At + FVector(40.0, 40.0, 50.0) * S);
			Frustum(Part, FVector2D(At), 30.0 * S, 18.0 * S, At.Z + 50.0 * S, At.Z + 200.0 * S, 8);
			Cylinder(Part, FVector2D(At), 13.0 * S, At.Z + 200.0 * S, At.Z + 232.0 * S, 8);
			Box(Part, At + FVector(-8.0, -28.0, 120.0) * S, At + FVector(8.0, -16.0, 190.0) * S);
			if (bArmRaised)
			{
				Box(Part, At + FVector(-8.0, 16.0, 180.0) * S, At + FVector(8.0, 28.0, 270.0) * S);
			}
			else
			{
				Box(Part, At + FVector(-8.0, 16.0, 120.0) * S, At + FVector(8.0, 28.0, 190.0) * S);
			}
		}
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
			B.Poly(Part, { At(S0, Z1, 0.0), At(S1, Z1, 0.0), At(S1, Z1, D), At(S0, Z1, D) }, FVector::UpVector);
			B.Poly(Part, { At(S0, Z0, 0.0), At(S1, Z0, 0.0), At(S1, Z0, D), At(S0, Z0, D) }, FVector::DownVector);
			B.Poly(Part, { At(S0, Z0, 0.0), At(S0, Z1, 0.0), At(S0, Z1, D), At(S0, Z0, D) }, -Along);
			B.Poly(Part, { At(S1, Z0, 0.0), At(S1, Z1, 0.0), At(S1, Z1, D), At(S1, Z0, D) }, Along);
		}

		// A round-headed opening: straight sides up to Spring, then a half
		// circle. A door has no frame along the bottom.
		void Arch(FBuilder& B, EPart FramePart, EPart PanePart, double S0, double S1, double Z0, double Spring, double Border, double Depth, bool bDoor) const
		{
			constexpr int32 Segments = 10;
			const double Mid = (S0 + S1) / 2.0;
			const double R = (S1 - S0) / 2.0;
			const double Inner = R - Border;
			const double Bottom = bDoor ? Z0 : Z0 + Border;
			if (!bDoor)
			{
				Block(B, FramePart, S0, S1, Z0, Z0 + Border, Depth);
			}
			Block(B, FramePart, S0, S0 + Border, Bottom, Spring, Depth);
			Block(B, FramePart, S1 - Border, S1, Bottom, Spring, Depth);
			Panel(B, PanePart, S0 + Border, S1 - Border, Bottom, Spring, 1.0);
			for (int32 i = 0; i < Segments; ++i)
			{
				const double A0 = PI * i / Segments;
				const double A1 = PI * (i + 1) / Segments;
				const FVector2D D0(FMath::Cos(A0), FMath::Sin(A0));
				const FVector2D D1(FMath::Cos(A1), FMath::Sin(A1));
				B.Poly(PanePart, { At(Mid, Spring, 1.0), At(Mid + D0.X * Inner, Spring + D0.Y * Inner, 1.0), At(Mid + D1.X * Inner, Spring + D1.Y * Inner, 1.0) }, Out);
				B.Poly(FramePart, { At(Mid + D0.X * Inner, Spring + D0.Y * Inner, Depth), At(Mid + D0.X * R, Spring + D0.Y * R, Depth),
					At(Mid + D1.X * R, Spring + D1.Y * R, Depth), At(Mid + D1.X * Inner, Spring + D1.Y * Inner, Depth) }, Out);
				B.Poly(FramePart, { At(Mid + D0.X * R, Spring + D0.Y * R, 0.0), At(Mid + D1.X * R, Spring + D1.Y * R, 0.0),
					At(Mid + D1.X * R, Spring + D1.Y * R, Depth), At(Mid + D0.X * R, Spring + D0.Y * R, Depth) }, FVector(0.0, 0.0, 1.0) + Along * ((D0.X + D1.X) / 2.0));
			}
			// Keystone.
			Block(B, FramePart, Mid - 18.0, Mid + 18.0, Spring + Inner - 6.0, Spring + R + 14.0, Depth + 6.0);
		}

		// Dark joints between the courses of a rusticated ground floor.
		void Rustication(FBuilder& B, double S0, double S1, double Top) const
		{
			for (double Z = 70.0; Z < Top - 30.0; Z += 70.0)
			{
				Panel(B, Slate, S0, S1, Z - 4.0, Z, 0.5);
			}
		}
	};

	// One stretch of a wing's outside wall: a rusticated ground floor of
	// arched windows, an upper floor of tall arched windows over little
	// balconies, columns between the bays, the entablature and the cornice.
	void Elevation(FBuilder& B, const FFace& F, double S0, double S1)
	{
		F.Block(B, Trim, S0, S1, Base, 40.0, 10.0);
		F.Rustication(B, S0, S1, RusticTop);
		F.Block(B, Trim, S0, S1, RusticTop, RusticTop + 60.0, 22.0);
		F.Block(B, Trim, S0, S1, EntablatureZ, EntablatureZ + 70.0, 14.0);
		F.Block(B, Trim, S0, S1, CorniceTop - 130.0, CorniceTop - 60.0, 26.0);
		F.Block(B, Trim, S0, S1, CorniceTop - 60.0, CorniceTop, 50.0);
		const double Length = S1 - S0;
		if (Length < 300.0)
		{
			return;
		}
		const int32 Bays = FMath::Max(1, FMath::RoundToInt(Length / BayWidth));
		const double Step = Length / Bays;
		for (int32 k = 0; k < Bays; ++k)
		{
			const double Mid = S0 + Step * (k + 0.5);
			F.Arch(B, Trim, Glass, Mid - 95.0, Mid + 95.0, 130.0, 560.0, 16.0, 10.0, false);
			F.Block(B, Trim, Mid - 125.0, Mid + 125.0, RusticTop + 60.0, UpperSill, 12.0);
			F.Arch(B, Trim, Glass, Mid - 105.0, Mid + 105.0, UpperSill, UpperSpring, 16.0, 10.0, false);
		}
		for (int32 k = 0; k <= Bays; ++k)
		{
			const double S = FMath::Clamp(S0 + Step * k, S0 + 45.0, S1 - 45.0);
			F.Block(B, Trim, S - 42.0, S + 42.0, RusticTop + 60.0, RusticTop + 120.0, 50.0);
			B.Cylinder(Stone, FVector2D(F.At(S, 0.0, 30.0)), 28.0, RusticTop + 120.0, EntablatureZ - 50.0, 10);
			F.Block(B, Trim, S - 42.0, S + 42.0, EntablatureZ - 50.0, EntablatureZ, 50.0);
		}
	}

	// A mansard roof over a rectangle: steep sides stepping in by Inset, a flat top.
	void Mansard(FBuilder& B, const FVector2D& Min, const FVector2D& Max, double Z0, double Z1, double Inset)
	{
		const FVector2D Center = (Min + Max) / 2.0;
		const double HX = (Max.X - Min.X) / 2.0;
		const double HY = (Max.Y - Min.Y) / 2.0;
		B.SquareRing(Slate, Center, HX, HY, HX - Inset, HY - Inset, Z0, Z1);
		const double TX = HX - Inset, TY = HY - Inset;
		B.Poly(Slate, { FVector(Center.X - TX, Center.Y - TY, Z1), FVector(Center.X + TX, Center.Y - TY, Z1),
			FVector(Center.X + TX, Center.Y + TY, Z1), FVector(Center.X - TX, Center.Y + TY, Z1) }, FVector::UpVector);
	}

	// A pavilion's tall curved roof, as on the corners of the wings: rings
	// curving in to a flat top with an iron crest and a flagpole.
	void Dome(FBuilder& B, const FVector2D& Min, const FVector2D& Max, double Z0)
	{
		const FVector2D Center = (Min + Max) / 2.0;
		const double HX = (Max.X - Min.X) / 2.0;
		const double HY = (Max.Y - Min.Y) / 2.0;
		const double Rings[][3] = { { 1.0, 0.94, 300.0 }, { 0.94, 0.80, 300.0 }, { 0.80, 0.62, 200.0 }, { 0.62, 0.52, 90.0 } };
		double Z = Z0;
		for (const auto& Ring : Rings)
		{
			B.SquareRing(Slate, Center, HX * Ring[0], HY * Ring[0], HX * Ring[1], HY * Ring[1], Z, Z + Ring[2]);
			Z += Ring[2];
		}
		const double TX = HX * 0.52, TY = HY * 0.52;
		const FVector Corners[4] = {
			FVector(Center.X - TX, Center.Y - TY, Z), FVector(Center.X + TX, Center.Y - TY, Z),
			FVector(Center.X + TX, Center.Y + TY, Z), FVector(Center.X - TX, Center.Y + TY, Z) };
		B.Poly(Slate, { Corners[0], Corners[1], Corners[2], Corners[3] }, FVector::UpVector);
		for (int32 i = 0; i < 4; ++i)
		{
			const FVector Inward = (FVector(Center, Z) - Corners[i]).GetSafeNormal() * 25.0;
			const FVector Next = (FVector(Center, Z) - Corners[(i + 1) % 4]).GetSafeNormal() * 25.0;
			B.Balustrade(Iron, Corners[i] + Inward, Corners[(i + 1) % 4] + Next, 60.0, false);
		}
		B.Cylinder(Iron, Center, 7.0, Z, Z + 450.0, 6);
	}

	// The station restaurant on the front of the departure wing: a canvas
	// awning over the ground floor and a terrace behind a hedge.
	void Restaurant(FBuilder& B, const FFace& F, double S0, double S1)
	{
		const double A = S0 + 120.0;
		const double E = S1 - 120.0;
		const FVector Slope = F.Out + FVector::UpVector;
		B.TwoSided(Canvas, { F.At(A, 700.0, 0.0), F.At(E, 700.0, 0.0), F.At(E, 520.0, 320.0), F.At(A, 520.0, 320.0) }, Slope);
		B.TwoSided(Canvas, { F.At(A, 440.0, 320.0), F.At(E, 440.0, 320.0), F.At(E, 520.0, 320.0), F.At(A, 520.0, 320.0) }, F.Out);
		for (double S = A; S <= E + 1.0; S += (E - A) / 4.0)
		{
			B.Cylinder(Iron, FVector2D(F.At(S, 0.0, 310.0)), 5.0, 0.0, 520.0, 6);
		}
		// The hedge in planters, open in the middle for the way in.
		const double Gap = (A + E) / 2.0;
		B.LineBox(Wood, F.At(A, 0.0, 340.0), F.At(Gap - 120.0, 0.0, 340.0), 25.0, 0.0, 45.0);
		B.LineBox(Wood, F.At(Gap + 120.0, 0.0, 340.0), F.At(E, 0.0, 340.0), 25.0, 0.0, 45.0);
		B.LineBox(Lawn, F.At(A, 0.0, 340.0), F.At(Gap - 120.0, 0.0, 340.0), 22.0, 45.0, 110.0);
		B.LineBox(Lawn, F.At(Gap + 120.0, 0.0, 340.0), F.At(E, 0.0, 340.0), 22.0, 45.0, 110.0);
	}

	void BuildWings(FBuilder& B)
	{
		struct FStretch
		{
			double X0;
			double X1;
			double Out;   // how far out from the middle of the hall the wall stands
		};
		struct FWing
		{
			double Side;
			double BaseOut;
			FStretch Front;
			FStretch Middle;
			FStretch End;
		};
		// Measured off the station's outline in OpenStreetMap: each wing
		// has a pavilion at both ends and one in the middle.
		const FWing Wings[] = {
			{ 1.0, 3770.0, { WingFrontX, 2220.0, 4780.0 }, { 8100.0, 11150.0, 4550.0 }, { 16720.0, EndX, 4780.0 } },
			{ -1.0, 4350.0, { WingFrontX, 2220.0, 4880.0 }, { 7300.0, 11150.0, 4860.0 }, { 16720.0, EndX, 4830.0 } } };

		for (const FWing& Wing : Wings)
		{
			const double Side = Wing.Side;
			const FStretch Stretches[] = {
				Wing.Front, { Wing.Front.X1, Wing.Middle.X0, Wing.BaseOut }, Wing.Middle, { Wing.Middle.X1, Wing.End.X0, Wing.BaseOut }, Wing.End };
			constexpr int32 StretchCount = 5;
			for (int32 i = 0; i < StretchCount; ++i)
			{
				const FStretch& Here = Stretches[i];
				const bool bPavilion = i % 2 == 0;
				const double Y0 = FMath::Min(Side * HallHalf, Side * Here.Out);
				const double Y1 = FMath::Max(Side * HallHalf, Side * Here.Out);
				B.Box(Rustic, FVector(Here.X0, Y0, Base), FVector(Here.X1, Y1, RusticTop));
				B.Box(Stone, FVector(Here.X0, Y0, RusticTop), FVector(Here.X1, Y1, CorniceTop));
				const FFace Outside{ FVector(Here.X0, Side * Here.Out, 0.0), FVector(1.0, 0.0, 0.0), FVector(0.0, Side, 0.0) };
				Elevation(B, Outside, 0.0, Here.X1 - Here.X0);

				// Where a pavilion stands out of the wing, the short wall at its side.
				if (i + 1 < StretchCount)
				{
					const FStretch& Next = Stretches[i + 1];
					const double Low = FMath::Min(Here.Out, Next.Out);
					const double High = FMath::Max(Here.Out, Next.Out);
					const FFace Flank{ FVector(Here.X1, Side * Low, 0.0), FVector(0.0, Side, 0.0), FVector(Here.Out > Next.Out ? 1.0 : -1.0, 0.0, 0.0) };
					Elevation(B, Flank, 0.0, High - Low);
				}

				const FVector2D RoofMin(Here.X0, Y0);
				const FVector2D RoofMax(Here.X1, Y1);
				if (bPavilion)
				{
					Dome(B, RoofMin, RoofMax, CorniceTop);
				}
				else
				{
					Mansard(B, RoofMin, RoofMax, CorniceTop, MansardTop, 250.0);
				}
			}

			// The front of the corner pavilion on Baross tér, and the far end.
			const FFace West{ FVector(WingFrontX, Side * FrontHalf, 0.0), FVector(0.0, Side, 0.0), FVector(-1.0, 0.0, 0.0) };
			Elevation(B, West, 0.0, Wing.Front.Out - FrontHalf);
			if (Side < 0.0)
			{
				Restaurant(B, West, 0.0, Wing.Front.Out - FrontHalf);
			}
			const FFace East{ FVector(EndX, Side * HallHalf, 0.0), FVector(0.0, Side, 0.0), FVector(1.0, 0.0, 0.0) };
			Elevation(B, East, 0.0, Wing.End.Out - HallHalf);

			// The hall wall rises above the wings' cornice to carry the roof.
			B.Box(Stone, FVector(WingFrontX, FMath::Min(Side * HallHalf, Side * (HallHalf + 80.0)), CorniceTop - 10.0),
				FVector(EndX, FMath::Max(Side * HallHalf, Side * (HallHalf + 80.0)), HallWallTop));
		}
	}

	// The point at angle A on a circle of radius R round the arch's centre, in the plane X.
	FVector ArchPoint(double X, double A, double R)
	{
		return FVector(X, FMath::Cos(A) * R, ArchSpring + FMath::Sin(A) * R);
	}

	// Where the ray at angle A from the arch's centre leaves the wall
	// panel round the arch (out to the towers, up to the entablature).
	FVector PanelEdge(double X, double A)
	{
		const double C = FMath::Cos(A);
		const double S = FMath::Sin(A);
		double T = 1.0e9;
		if (FMath::Abs(C) > 1.0e-6)
		{
			T = FMath::Min(T, TowerInner / FMath::Abs(C));
		}
		if (S > 1.0e-6)
		{
			T = FMath::Min(T, (FrontEntablature - ArchSpring) / S);
		}
		return FVector(X, C * T, ArchSpring + S * T);
	}

	void BuildFront(FBuilder& B)
	{
		// --- The two towers either side of the arch.
		for (const double Side : { 1.0, -1.0 })
		{
			const double Y0 = FMath::Min(Side * TowerInner, Side * FrontHalf);
			const double Y1 = FMath::Max(Side * TowerInner, Side * FrontHalf);
			B.Box(Rustic, FVector(TowerFrontX, Y0, Base), FVector(TowerBackX, Y1, TowerRusticTop));
			B.Box(Stone, FVector(TowerFrontX, Y0, TowerRusticTop), FVector(TowerBackX, Y1, TowerTop));

			const FFace F{ FVector(TowerFrontX, Side * FrontHalf, 0.0), FVector(0.0, -Side, 0.0), FVector(-1.0, 0.0, 0.0) };
			const double Width = FrontHalf - TowerInner;
			const double Mid = Width / 2.0;
			F.Block(B, Trim, 0.0, Width, Base, 40.0, 12.0);
			F.Rustication(B, 0.0, Width, TowerRusticTop);
			F.Arch(B, Trim, Glass, Mid - 110.0, Mid + 110.0, 200.0, 760.0, 18.0, 12.0, false);
			F.Block(B, Trim, 0.0, Width, TowerRusticTop, TowerRusticTop + 60.0, 26.0);

			// Paired columns either side of a niche with a statue.
			for (const double Offset : { -400.0, -290.0, 290.0, 400.0 })
			{
				const double S = Mid + Offset;
				F.Block(B, Trim, S - 45.0, S + 45.0, TowerRusticTop + 60.0, TowerRusticTop + 130.0, 75.0);
				B.Cylinder(Stone, FVector2D(F.At(S, 0.0, 38.0)), 32.0, TowerRusticTop + 130.0, ColumnTop, 12);
				F.Block(B, Trim, S - 45.0, S + 45.0, ColumnTop, ColumnTop + 80.0, 75.0);
			}
			F.Arch(B, Trim, Door, Mid - 150.0, Mid + 150.0, 1350.0, 2000.0, 16.0, 10.0, false);
			F.Block(B, Trim, Mid - 80.0, Mid + 80.0, 1330.0, 1380.0, 60.0);
			B.Figure(Trim, F.At(Mid, 1380.0, 35.0), 1.3, Side > 0.0);

			// Entablature, cornice and a balustrade along the top.
			F.Block(B, Trim, 0.0, Width, ColumnTop + 80.0, FrontEntablature, 20.0);
			F.Block(B, Trim, 0.0, Width, FrontEntablature, FrontEntablature + 70.0, 50.0);
			B.Balustrade(Trim, FVector(TowerFrontX + 30.0, Side * (FrontHalf - 30.0), TowerTop), FVector(TowerFrontX + 30.0, Side * (TowerInner + 30.0), TowerTop), 100.0);
			B.Balustrade(Trim, FVector(TowerFrontX + 30.0, Side * (FrontHalf - 30.0), TowerTop), FVector(TowerBackX - 30.0, Side * (FrontHalf - 30.0), TowerTop), 100.0);
			for (const double S : { 50.0, Width - 50.0 })
			{
				const FVector P = F.At(S, TowerTop, -20.0);
				B.Box(Trim, P + FVector(-60.0, -60.0, 0.0), P + FVector(60.0, 60.0, 150.0));
				B.Frustum(Trim, FVector2D(P), 35.0, 20.0, P.Z + 150.0, P.Z + 230.0, 8);
			}

			// The tower's other face, towards the wing.
			const FFace Flank{ FVector(TowerFrontX, Side * FrontHalf, 0.0), FVector(1.0, 0.0, 0.0), FVector(0.0, Side, 0.0) };
			const double Depth = TowerBackX - TowerFrontX;
			Flank.Block(B, Trim, 0.0, Depth, Base, 40.0, 12.0);
			Flank.Rustication(B, 0.0, Depth, TowerRusticTop);
			Flank.Block(B, Trim, 0.0, Depth, TowerRusticTop, TowerRusticTop + 60.0, 26.0);
			Flank.Block(B, Trim, 0.0, Depth, ColumnTop + 80.0, FrontEntablature, 20.0);
			Flank.Block(B, Trim, 0.0, Depth, FrontEntablature, FrontEntablature + 70.0, 50.0);
		}

		// --- Under the arch: a colonnade, open to walk through into the hall.
		for (const double Y : { -1000.0, -600.0, -200.0, 200.0, 600.0, 1000.0 })
		{
			B.Box(Trim, FVector(55.0, Y - 50.0, Base), FVector(145.0, Y + 50.0, 50.0));
			B.Cylinder(Stone, FVector2D(100.0, Y), 36.0, 50.0, ColonnadeTop - 50.0, 12);
			B.Box(Trim, FVector(50.0, Y - 55.0, ColonnadeTop - 50.0), FVector(150.0, Y + 55.0, ColonnadeTop));
		}
		B.Box(Trim, FVector(-40.0, -TowerInner, ColonnadeTop), FVector(ArchWallDepth, TowerInner, BandTop));
		for (const double Side : { 1.0, -1.0 })
		{
			const double Y0 = FMath::Min(Side * ArchRadius, Side * TowerInner);
			const double Y1 = FMath::Max(Side * ArchRadius, Side * TowerInner);
			B.Box(Stone, FVector(0.0, Y0, BandTop), FVector(ArchWallDepth, Y1, ArchSpring));
		}

		// A row of tall windows below the arch, then the arch itself, its
		// iron glazing bars fanning out from the middle. The glass is left
		// clear: you see into the hall, and the sun shines in.
		constexpr double GlassX = 150.0;
		for (int32 k = 1; k < 10; ++k)
		{
			const double Y = -ArchRadius + 2.0 * ArchRadius * k / 10.0;
			B.Tube(Iron, FVector(GlassX, Y, BandTop), FVector(GlassX, Y, ArchSpring), 9.0, 4);
		}
		B.Tube(Iron, FVector(GlassX, -ArchRadius, (BandTop + ArchSpring) / 2.0), FVector(GlassX, ArchRadius, (BandTop + ArchSpring) / 2.0), 7.0, 4);
		B.Box(Trim, FVector(GlassX - 40.0, -ArchRadius, ArchSpring - 15.0), FVector(GlassX + 40.0, ArchRadius, ArchSpring + 15.0));

		constexpr int32 FanSegments = 24;
		const FVector Hub(GlassX, 0.0, ArchSpring);
		for (int32 i = 0; i < FanSegments; ++i)
		{
			const double A0 = PI * i / FanSegments;
			const double A1 = PI * (i + 1) / FanSegments;
			for (const double R : { 450.0, 900.0 })
			{
				B.Tube(Iron, ArchPoint(GlassX, A0, R), ArchPoint(GlassX, A1, R), 7.0, 4);
			}
			if (i % 2 == 1)
			{
				B.Tube(Iron, ArchPoint(GlassX, A1, 120.0), ArchPoint(GlassX, A1, ArchRadius), 8.0, 4);
			}
		}
		B.Tube(Iron, Hub + FVector(-25.0, 0.0, 0.0), Hub + FVector(25.0, 0.0, 0.0), 110.0, 16);

		// The wall round the arch, front and back, the arch's soffit, and the
		// moulded band round it. The panel is cut along rays from the arch's
		// centre; the rays through the panel's top corners split it cleanly.
		TArray<double> Angles;
		for (int32 i = 0; i <= FanSegments; ++i)
		{
			Angles.Add(PI * i / FanSegments);
		}
		const double CornerAngle = FMath::Atan2(FrontEntablature - ArchSpring, TowerInner);
		Angles.Add(CornerAngle);
		Angles.Add(PI - CornerAngle);
		Angles.Sort();
		for (int32 i = 0; i + 1 < Angles.Num(); ++i)
		{
			const double A0 = Angles[i];
			const double A1 = Angles[i + 1];
			if (A1 - A0 < 1.0e-4)
			{
				continue;
			}
			B.Poly(Stone, { ArchPoint(0.0, A0, ArchRadius), ArchPoint(0.0, A1, ArchRadius), PanelEdge(0.0, A1), PanelEdge(0.0, A0) }, FVector::BackwardVector);
			B.Poly(Stone, { ArchPoint(ArchWallDepth, A0, ArchRadius), ArchPoint(ArchWallDepth, A1, ArchRadius), PanelEdge(ArchWallDepth, A1), PanelEdge(ArchWallDepth, A0) }, FVector::ForwardVector);
			const FVector Inward = -FVector(0.0, FMath::Cos((A0 + A1) / 2.0), FMath::Sin((A0 + A1) / 2.0));
			B.Poly(Stone, { ArchPoint(0.0, A0, ArchRadius), ArchPoint(0.0, A1, ArchRadius), ArchPoint(ArchWallDepth, A1, ArchRadius), ArchPoint(ArchWallDepth, A0, ArchRadius) }, Inward);
			B.Poly(Trim, { ArchPoint(-40.0, A0, ArchRadius), ArchPoint(-40.0, A1, ArchRadius), ArchPoint(-40.0, A1, ArchRadius + ArchRing), ArchPoint(-40.0, A0, ArchRadius + ArchRing) }, FVector::BackwardVector);
			B.Poly(Trim, { ArchPoint(-40.0, A0, ArchRadius + ArchRing), ArchPoint(-40.0, A1, ArchRadius + ArchRing), ArchPoint(0.0, A1, ArchRadius + ArchRing), ArchPoint(0.0, A0, ArchRadius + ArchRing) }, -Inward);
			B.Poly(Trim, { ArchPoint(-40.0, A0, ArchRadius), ArchPoint(-40.0, A1, ArchRadius), ArchPoint(0.0, A1, ArchRadius), ArchPoint(0.0, A0, ArchRadius) }, Inward);
		}
		B.Box(Trim, FVector(-90.0, -70.0, ArchSpring + ArchRadius - 30.0), FVector(0.0, 70.0, FrontEntablature));
		for (const double Y : { -1020.0, 1020.0 })
		{
			B.Tube(Trim, FVector(-25.0, Y, 2380.0), FVector(0.0, Y, 2380.0), 95.0, 16);
		}

		// Entablature with the station's name, the attic with the railway's
		// initials and a sculpture group on top, balustrades either side.
		B.Box(Trim, FVector(-60.0, -TowerInner, FrontEntablature), FVector(ArchWallDepth, TowerInner, FrontEntablatureTop));
		B.Box(Trim, FVector(-90.0, -TowerInner, FrontEntablatureTop - 40.0), FVector(ArchWallDepth, TowerInner, FrontEntablatureTop));
		B.Box(Stone, FVector(-40.0, -AtticHalf, FrontEntablatureTop), FVector(ArchWallDepth, AtticHalf, AtticTop));
		B.Box(Trim, FVector(-70.0, -AtticHalf - 30.0, AtticTop), FVector(ArchWallDepth + 20.0, AtticHalf + 30.0, AtticTop + 70.0));
		for (const double Side : { 1.0, -1.0 })
		{
			B.Box(Trim, FVector(-60.0, FMath::Min(Side * (AtticHalf - 70.0), Side * AtticHalf), FrontEntablatureTop), FVector(0.0, FMath::Max(Side * (AtticHalf - 70.0), Side * AtticHalf), AtticTop));
			B.Balustrade(Trim, FVector(-20.0, Side * AtticHalf, FrontEntablatureTop), FVector(-20.0, Side * TowerInner, FrontEntablatureTop), 100.0);
			B.Figure(Trim, FVector(130.0, Side * 420.0, AtticTop + 70.0), 1.2, false);
			B.Figure(Trim, FVector(130.0, Side * 700.0, AtticTop + 70.0), 0.95, Side < 0.0);
		}
		B.Box(Trim, FVector(20.0, -160.0, AtticTop + 70.0), FVector(240.0, 160.0, AtticTop + 190.0));
		B.Figure(Trim, FVector(130.0, 0.0, AtticTop + 190.0), 1.6, true);

		// The roof's gable behind the front, seen over the attic and from inside.
		B.TwoSided(Stone, { FVector(ArchWallDepth, -HallHalf, HallWallTop), FVector(ArchWallDepth, HallHalf, HallWallTop), FVector(ArchWallDepth, 0.0, RidgeZ) }, FVector::BackwardVector);
	}

	// A point on the hall roof: T runs from the eaves (0) to the ridge (1).
	FVector RoofPoint(double Side, double T, double X)
	{
		return FVector(X, Side * HallHalf * (1.0 - T), HallWallTop + (RidgeZ - HallWallTop) * T);
	}

	// One iron roof truss across the hall: a shallow arch from wall to wall,
	// hangers up to the rafters, and the rafters under the roof.
	void RoofTruss(FBuilder& B, double X)
	{
		const double Rise = TrussApex - TrussSpring;
		const double R = (HallHalf * HallHalf + Rise * Rise) / (2.0 * Rise);
		const double CenterZ = TrussApex - R;
		constexpr int32 Segments = 12;
		FVector Prev = FVector::ZeroVector;
		for (int32 i = 0; i <= Segments; ++i)
		{
			const double Y = -HallHalf + 2.0 * HallHalf * i / Segments;
			const FVector P(X, Y, CenterZ + FMath::Sqrt(FMath::Max(0.0, R * R - Y * Y)));
			if (i > 0)
			{
				B.Tube(Truss, Prev, P, 16.0, 4);
			}
			if (i > 0 && i < Segments)
			{
				const double RoofZ = HallWallTop + (RidgeZ - HallWallTop) * (1.0 - FMath::Abs(Y) / HallHalf) - 12.0;
				B.Tube(Truss, P, FVector(X, Y, RoofZ), 6.0, 4);
			}
			Prev = P;
		}
		for (const double Side : { 1.0, -1.0 })
		{
			B.Tube(Truss, RoofPoint(Side, 0.0, X) - FVector(0.0, 0.0, 14.0), RoofPoint(Side, 1.0, X) - FVector(0.0, 0.0, 14.0), 12.0, 4);
			B.Tube(Truss, FVector(X, Side * (HallHalf - 10.0), TrussSpring - 150.0), FVector(X, Side * (HallHalf - 10.0), HallWallTop), 16.0, 4);
		}
	}

	void BuildHall(FBuilder& B)
	{
		// The paved floor, from the porch under the arch to the far end.
		B.Box(Paving, FVector(TowerFrontX, -HallHalf, Base), FVector(EndX, HallHalf, FloorTop));

		// The side walls inside: tall arched doorways between paired
		// pilasters, pairs of arched windows above.
		const double Length = EndX - WingFrontX;
		const double Bay = Length / HallBays;
		for (const double Side : { 1.0, -1.0 })
		{
			const FFace F{ FVector(WingFrontX, Side * HallHalf, 0.0), FVector(1.0, 0.0, 0.0), FVector(0.0, -Side, 0.0) };
			F.Block(B, Trim, 0.0, Length, FloorTop, 70.0, 8.0);
			F.Block(B, Trim, 0.0, Length, 1560.0, 1680.0, 22.0);
			F.Block(B, Trim, 0.0, Length, 2330.0, 2420.0, 40.0);
			for (int32 k = 0; k <= HallBays; ++k)
			{
				const double S = FMath::Clamp(Bay * k, 80.0, Length - 80.0);
				F.Block(B, Stone, S - 75.0, S - 15.0, 70.0, 1500.0, 25.0);
				F.Block(B, Stone, S + 15.0, S + 75.0, 70.0, 1500.0, 25.0);
				F.Block(B, Trim, S - 85.0, S + 85.0, 1500.0, 1560.0, 35.0);
				F.Block(B, Stone, S - 40.0, S + 40.0, 1680.0, 2330.0, 15.0);
			}
			for (int32 k = 0; k < HallBays; ++k)
			{
				const double Mid = Bay * (k + 0.5);
				F.Arch(B, Trim, Door, Mid - 330.0, Mid + 330.0, 0.0, 1150.0, 24.0, 12.0, true);
				F.Arch(B, Trim, Glass, Mid - 260.0, Mid - 60.0, 1800.0, 2100.0, 14.0, 8.0, false);
				F.Arch(B, Trim, Glass, Mid + 60.0, Mid + 260.0, 1800.0, 2100.0, 14.0, 8.0, false);
			}
		}

		// The roof: slate near the eaves and along the ridge, a long strip of
		// glass between, on iron trusses with purlins along the hall.
		for (const double Side : { 1.0, -1.0 })
		{
			const FVector Up(0.0, Side, 1.0);
			const double Bands[][2] = { { 0.0, GlassFrom }, { GlassTo, 1.0 } };
			for (const auto& Band : Bands)
			{
				B.Poly(RoofOut, { RoofPoint(Side, Band[0], ArchWallDepth), RoofPoint(Side, Band[0], EndX), RoofPoint(Side, Band[1], EndX), RoofPoint(Side, Band[1], ArchWallDepth) }, Up);
				const FVector Below(0.0, 0.0, -4.0);
				B.Poly(RoofIn, { RoofPoint(Side, Band[0], ArchWallDepth) + Below, RoofPoint(Side, Band[0], EndX) + Below, RoofPoint(Side, Band[1], EndX) + Below, RoofPoint(Side, Band[1], ArchWallDepth) + Below }, -Up);
			}
			B.Poly(Pane, { RoofPoint(Side, GlassFrom, ArchWallDepth), RoofPoint(Side, GlassFrom, EndX), RoofPoint(Side, GlassTo, EndX), RoofPoint(Side, GlassTo, ArchWallDepth) }, Up);
			const FVector Lift(0.0, 0.0, 6.0);
			for (double X = ArchWallDepth + 250.0; X < EndX; X += 250.0)
			{
				B.Tube(Truss, RoofPoint(Side, GlassFrom, X) + Lift, RoofPoint(Side, GlassTo, X) + Lift, 5.0, 4);
			}
			for (const double T : { GlassFrom, 0.4, 0.68, GlassTo })
			{
				B.Tube(Truss, RoofPoint(Side, T, WingFrontX) - FVector(0.0, 0.0, 22.0), RoofPoint(Side, T, EndX) - FVector(0.0, 0.0, 22.0), 10.0, 4);
			}
		}
		for (int32 k = 0; k <= HallBays; ++k)
		{
			RoofTruss(B, FMath::Min(WingFrontX + Bay * k, EndX - 40.0));
		}

		// The far end: open over the tracks below a glazed iron screen (clear glass).
		B.Box(Iron, FVector(EndX - 30.0, -HallHalf, EndScreenBottom - 70.0), FVector(EndX + 30.0, HallHalf, EndScreenBottom));
		for (double Y = -HallHalf + 300.0; Y < HallHalf - 100.0; Y += 300.0)
		{
			B.Tube(Truss, FVector(EndX + 10.0, Y, EndScreenBottom), RoofPoint(Y > 0.0 ? 1.0 : -1.0, 1.0 - FMath::Abs(Y) / HallHalf, EndX + 10.0), 8.0, 4);
		}
	}

	// A platform lamp: an iron post with a lantern.
	void PlatformLamp(FBuilder& B, const FVector& At)
	{
		B.Box(Iron, At + FVector(-16.0, -16.0, 0.0), At + FVector(16.0, 16.0, 45.0));
		B.Cylinder(Iron, FVector2D(At), 6.0, At.Z + 45.0, At.Z + 360.0, 8);
		B.Box(Canvas, At + FVector(-14.0, -14.0, 360.0), At + FVector(14.0, 14.0, 400.0));
		B.Frustum(Iron, FVector2D(At), 24.0, 4.0, At.Z + 400.0, At.Z + 425.0, 8);
	}

	// A wooden bench along the platform, its seat facing FacingY (+1 or -1).
	void Bench(FBuilder& B, const FVector& At, double FacingY)
	{
		B.Box(Wood, At + FVector(-90.0, -22.0, 42.0), At + FVector(90.0, 22.0, 48.0));
		const double BackY = -FacingY * 24.0;
		B.Box(Wood, At + FVector(-90.0, FMath::Min(BackY - 3.0, BackY + 3.0), 52.0), At + FVector(90.0, FMath::Max(BackY - 3.0, BackY + 3.0), 95.0));
		for (const double X : { -78.0, 78.0 })
		{
			B.Box(Iron, At + FVector(X - 3.0, -22.0, 0.0), At + FVector(X + 3.0, 22.0, 42.0));
		}
	}

	void BuildTracks(FBuilder& B)
	{
		for (const double Y : TrackY)
		{
			B.Box(Ballast, FVector(BufferX - 120.0, Y - 220.0, Base), FVector(TrackEnd, Y + 220.0, BallastTop));
			for (double X = BufferX; X < TrackEnd; X += 100.0)
			{
				B.Box(Wood, FVector(X, Y - 130.0, BallastTop), FVector(X + 25.0, Y + 130.0, BallastTop + 6.0));
			}
			for (const double Rail : { -Gauge / 2.0, Gauge / 2.0 })
			{
				B.Box(Steel, FVector(BufferX - 60.0, Y + Rail - 4.0, BallastTop + 6.0), FVector(TrackEnd, Y + Rail + 4.0, RailTop));
			}
			// The buffer stop at the head of the track.
			B.Box(Red, FVector(BufferX - 110.0, Y - 150.0, BallastTop), FVector(BufferX - 50.0, Y + 150.0, 140.0));
			for (const double Offset : { -100.0, 100.0 })
			{
				B.Tube(Steel, FVector(BufferX - 50.0, Y + Offset, 110.0), FVector(BufferX, Y + Offset, 110.0), 17.0, 8);
			}
		}

		for (const auto& Edges : PlatformY)
		{
			const double Y0 = Edges[0];
			const double Y1 = Edges[1];
			B.Box(Paving, FVector(PlatformStart, Y0, Base), FVector(PlatformEnd, Y1, PlatformTop));
			// Light stone edging along the tracks.
			if (Y0 > -HallHalf + 1.0)
			{
				B.Box(Trim, FVector(PlatformStart, Y0, PlatformTop - 6.0), FVector(PlatformEnd, Y0 + 35.0, PlatformTop + 1.0));
			}
			if (Y1 < HallHalf - 1.0)
			{
				B.Box(Trim, FVector(PlatformStart, Y1 - 35.0, PlatformTop - 6.0), FVector(PlatformEnd, Y1, PlatformTop + 1.0));
			}
			// Lamps down the middle; benches against the wall, or back to back on the island.
			const double Mid = (Y0 + Y1) / 2.0;
			const bool bIsland = Y0 > -HallHalf + 1.0 && Y1 < HallHalf - 1.0;
			for (double X = PlatformStart + 800.0; X < PlatformEnd - 400.0; X += 1600.0)
			{
				PlatformLamp(B, FVector(X, Mid, PlatformTop));
				const double BenchX = X + 800.0;
				if (BenchX > PlatformEnd - 300.0)
				{
					continue;
				}
				if (bIsland)
				{
					Bench(B, FVector(BenchX, Mid + 40.0, PlatformTop), 1.0);
					Bench(B, FVector(BenchX, Mid - 40.0, PlatformTop), -1.0);
				}
				else
				{
					const double WallSide = Y1 >= HallHalf - 1.0 ? 1.0 : -1.0;
					Bench(B, FVector(BenchX, WallSide * (HallHalf - 70.0), PlatformTop), -WallSide);
				}
			}
		}
	}

	// A dark green passenger carriage of the M.Á.V. on the track at Y.
	void Carriage(FBuilder& B, double X0, double Y)
	{
		const double X1 = X0 + CarriageLength;
		constexpr double Half = 145.0;
		for (const double BogieX : { X0 + 320.0, X1 - 320.0 })
		{
			B.Box(Black, FVector(BogieX - 140.0, Y - 95.0, 60.0), FVector(BogieX + 140.0, Y + 95.0, 100.0));
			for (const double AxleX : { BogieX - 95.0, BogieX + 95.0 })
			{
				for (const double Side : { -1.0, 1.0 })
				{
					B.Tube(Black, FVector(AxleX, Y + Side * (Gauge / 2.0 - 4.0), RailTop + 46.0), FVector(AxleX, Y + Side * (Gauge / 2.0 + 8.0), RailTop + 46.0), 46.0, 12);
				}
			}
		}
		B.Box(Black, FVector(X0 + 40.0, Y - 130.0, 100.0), FVector(X1 - 40.0, Y + 130.0, 140.0));
		B.Box(CarGreen, FVector(X0, Y - Half, 140.0), FVector(X1, Y + Half, 400.0));
		B.Box(CarRoof, FVector(X0 - 10.0, Y - Half - 8.0, 400.0), FVector(X1 + 10.0, Y + Half + 8.0, 410.0));
		B.Gable(CarRoof, FVector(X0 - 10.0, Y - Half - 8.0, 410.0), FVector(X1 + 10.0, Y + Half + 8.0, 410.0), 45.0, true);
		for (const double Side : { -1.0, 1.0 })
		{
			const double Face = Y + Side * (Half + 1.0);
			const FVector Outward(0.0, Side, 0.0);
			auto Flat = [&B, Face, &Outward](EPart Part, double FromX, double ToX, double Low, double High)
			{
				B.Poly(Part, { FVector(FromX, Face, Low), FVector(ToX, Face, Low), FVector(ToX, Face, High), FVector(FromX, Face, High) }, Outward);
			};
			Flat(Trim, X0 + 20.0, X1 - 20.0, 228.0, 234.0);
			for (double X = X0 + 260.0; X + 115.0 < X1 - 200.0; X += 170.0)
			{
				Flat(Glass, X, X + 115.0, 250.0, 350.0);
			}
			Flat(Door, X0 + 50.0, X0 + 140.0, 150.0, 380.0);
			Flat(Door, X1 - 140.0, X1 - 50.0, 150.0, 380.0);
			for (const double StepX : { X0 + 45.0, X1 - 145.0 })
			{
				B.Box(Black, FVector(StepX, FMath::Min(Face, Face + Side * 25.0), 70.0), FVector(StepX + 100.0, FMath::Max(Face, Face + Side * 25.0), 82.0));
			}
		}
		// Buffers and the gangway bellows at both ends.
		for (const double EndAt : { X0, X1 })
		{
			const double Dir = EndAt == X0 ? -1.0 : 1.0;
			for (const double Offset : { -100.0, 100.0 })
			{
				B.Tube(Steel, FVector(EndAt, Y + Offset, 120.0), FVector(EndAt + Dir * 35.0, Y + Offset, 120.0), 16.0, 8);
			}
			B.Box(Black, FVector(FMath::Min(EndAt, EndAt + Dir * 30.0), Y - 60.0, 150.0), FVector(FMath::Max(EndAt, EndAt + Dir * 30.0), Y + 60.0, 370.0));
		}
	}

	// A steam engine and its tender on the track at Y, its front at FrontX.
	// Dir +1: the engine reaches east from FrontX (it faces west); -1 the other way.
	void Locomotive(FBuilder& B, double FrontX, double Y, double Dir)
	{
		auto Along = [FrontX, Dir](double S) { return FrontX + Dir * S; };
		auto Span = [&B, &Along, Y](EPart Part, double S0, double S1, double Y0, double Y1, double Z0, double Z1)
		{
			B.Box(Part, FVector(FMath::Min(Along(S0), Along(S1)), Y + Y0, Z0), FVector(FMath::Max(Along(S0), Along(S1)), Y + Y1, Z1));
		};
		auto Wheel = [&B, &Along, Y](EPart Part, double S, double R)
		{
			for (const double Side : { -1.0, 1.0 })
			{
				B.Tube(Part, FVector(Along(S), Y + Side * (Gauge / 2.0 - 4.0), RailTop + R), FVector(Along(S), Y + Side * (Gauge / 2.0 + 10.0), RailTop + R), R, 14);
			}
		};

		// Buffer beam, frame, running boards.
		Span(Red, 0.0, 25.0, -140.0, 140.0, 80.0, 135.0);
		for (const double Offset : { -100.0, 100.0 })
		{
			B.Tube(Steel, FVector(Along(0.0), Y + Offset, 110.0), FVector(Along(-35.0), Y + Offset, 110.0), 16.0, 8);
		}
		Span(Black, 25.0, 1380.0, -95.0, 95.0, 80.0, 150.0);
		Span(Black, 60.0, 1020.0, -150.0, 150.0, 175.0, 185.0);

		// Smokebox, boiler, chimney and domes.
		B.Tube(Black, FVector(Along(60.0), Y, 275.0), FVector(Along(230.0), Y, 275.0), 95.0, 16);
		B.Tube(Black, FVector(Along(230.0), Y, 275.0), FVector(Along(1020.0), Y, 275.0), 85.0, 16);
		B.Tube(Steel, FVector(Along(52.0), Y, 275.0), FVector(Along(60.0), Y, 275.0), 60.0, 16);
		B.Cylinder(Black, FVector2D(Along(150.0), Y), 26.0, 340.0, 440.0, 10);
		B.Cylinder(Black, FVector2D(Along(150.0), Y), 33.0, 430.0, 445.0, 10);
		B.Cylinder(Black, FVector2D(Along(560.0), Y), 38.0, 340.0, 395.0, 10);
		B.Cylinder(Black, FVector2D(Along(780.0), Y), 32.0, 340.0, 385.0, 10);
		for (const double Side : { -1.0, 1.0 })
		{
			B.Tube(Black, FVector(Along(140.0), Y + Side * 115.0, 120.0), FVector(Along(330.0), Y + Side * 115.0, 120.0), 32.0, 10);
		}

		// Red wheels: a small leading pair and four coupled pairs with rods.
		Wheel(Red, 200.0, 45.0);
		for (const double S : { 420.0, 600.0, 780.0, 960.0 })
		{
			Wheel(Red, S, 75.0);
		}
		Span(Steel, 420.0, 960.0, Gauge / 2.0 + 12.0, Gauge / 2.0 + 18.0, RailTop + 70.0, RailTop + 80.0);
		Span(Steel, 420.0, 960.0, -Gauge / 2.0 - 18.0, -Gauge / 2.0 - 12.0, RailTop + 70.0, RailTop + 80.0);

		// The cab with its windows.
		Span(Black, 1020.0, 1330.0, -150.0, 150.0, 185.0, 440.0);
		Span(Black, 1000.0, 1350.0, -165.0, 165.0, 440.0, 455.0);
		for (const double Side : { -1.0, 1.0 })
		{
			const double Face = Y + Side * 151.0;
			B.Poly(Glass, { FVector(Along(1080.0), Face, 320.0), FVector(Along(1260.0), Face, 320.0), FVector(Along(1260.0), Face, 410.0), FVector(Along(1080.0), Face, 410.0) }, FVector(0.0, Side, 0.0));
			const double Front = Along(1019.0);
			B.Poly(Glass, { FVector(Front, Y + Side * 45.0, 340.0), FVector(Front, Y + Side * 125.0, 340.0), FVector(Front, Y + Side * 125.0, 410.0), FVector(Front, Y + Side * 45.0, 410.0) }, FVector(-Dir, 0.0, 0.0));
		}

		// The tender with its coal.
		Span(Black, 1360.0, 2050.0, -95.0, 95.0, 80.0, 130.0);
		Span(Black, 1360.0, 2050.0, -140.0, 140.0, 130.0, 360.0);
		Span(Slate, 1380.0, 1800.0, -125.0, 125.0, 360.0, 390.0);
		for (const double S : { 1500.0, 1700.0, 1900.0 })
		{
			Wheel(Black, S, 45.0);
		}
		for (const double Offset : { -100.0, 100.0 })
		{
			B.Tube(Steel, FVector(Along(2050.0), Y + Offset, 120.0), FVector(Along(2085.0), Y + Offset, 120.0), 16.0, 8);
		}
	}

	void BuildTrains(FBuilder& B)
	{
		// The arrival train the player came on, at the platform on the
		// arrival side, its engine up against the buffers.
		Locomotive(B, BufferX + 60.0, TrackY[0], 1.0);
		for (int32 k = 0; k < 5; ++k)
		{
			Carriage(B, BufferX + 60.0 + 2120.0 + CarriagePitch * k, TrackY[0]);
		}
		// A departure train on the far side, its engine at the outer end.
		const double DepartureX = 2400.0;
		for (int32 k = 0; k < 5; ++k)
		{
			Carriage(B, DepartureX + CarriagePitch * k, TrackY[3]);
		}
		Locomotive(B, DepartureX + CarriagePitch * 4.0 + CarriageLength + 70.0 + 2050.0, TrackY[3], -1.0);
		// A few carriages waiting by the island platform.
		for (int32 k = 0; k < 3; ++k)
		{
			Carriage(B, 6000.0 + CarriagePitch * k, TrackY[2]);
		}
	}

	// Baross Gábor, the railway minister, on a tall pedestal with two
	// figures on its steps, in a round lawn with an iron railing.
	void BuildStatue(FBuilder& B)
	{
		const FVector2D C = StatueAt;
		B.Frustum(Trim, C, 1060.0, 1060.0, Base, 15.0, 32);
		B.Frustum(Lawn, C, 1020.0, 1020.0, Base, 18.0, 32);
		constexpr int32 Posts = 48;
		FVector2D Prev = C + FVector2D(990.0, 0.0);
		for (int32 i = 1; i <= Posts; ++i)
		{
			const double A = 2.0 * PI * i / Posts;
			const FVector2D P = C + FVector2D(FMath::Cos(A), FMath::Sin(A)) * 990.0;
			B.Box(Iron, FVector(P.X - 3.0, P.Y - 3.0, 15.0), FVector(P.X + 3.0, P.Y + 3.0, 100.0));
			B.Tube(Iron, FVector(Prev, 92.0), FVector(P, 92.0), 3.0, 4);
			B.Tube(Iron, FVector(Prev, 40.0), FVector(P, 40.0), 2.0, 4);
			Prev = P;
		}
		B.Box(Trim, FVector(C.X - 450.0, C.Y - 520.0, 0.0), FVector(C.X + 450.0, C.Y + 520.0, 40.0));
		B.Box(Trim, FVector(C.X - 360.0, C.Y - 420.0, 40.0), FVector(C.X + 360.0, C.Y + 420.0, 80.0));
		B.Box(Trim, FVector(C.X - 280.0, C.Y - 320.0, 80.0), FVector(C.X + 280.0, C.Y + 320.0, 130.0));
		B.Box(Stone, FVector(C.X - 170.0, C.Y - 170.0, 130.0), FVector(C.X + 170.0, C.Y + 170.0, 560.0));
		B.Box(Trim, FVector(C.X - 200.0, C.Y - 200.0, 560.0), FVector(C.X + 200.0, C.Y + 200.0, 620.0));
		B.Box(Stone, FVector(C.X - 150.0, C.Y - 150.0, 620.0), FVector(C.X + 150.0, C.Y + 150.0, 680.0));
		B.Box(Bronze, FVector(C.X - 178.0, C.Y - 110.0, 240.0), FVector(C.X - 170.0, C.Y + 110.0, 430.0));
		B.Figure(Bronze, FVector(C, 680.0), 1.6, false);
		B.Figure(Bronze, FVector(C.X - 200.0, C.Y - 300.0, 130.0), 1.1, false);
		B.Figure(Bronze, FVector(C.X - 200.0, C.Y + 300.0, 130.0), 1.15, true);
	}

	void BuildStation(FBuilder& B)
	{
		BuildFront(B);
		BuildWings(B);
		BuildHall(B);
		BuildTracks(B);
		BuildTrains(B);
		BuildStatue(B);
	}
}

AKeletiStation::AKeletiStation()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	Mesh->bUseComplexAsSimpleCollision = true;
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	RootComponent = Mesh;

	Overhead = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Overhead"));
	Overhead->SetupAttachment(Mesh);
	Overhead->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Glazing = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Glazing"));
	Glazing->SetupAttachment(Mesh);
	Glazing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Glazing->SetCastShadow(false);
	Glazing->bAffectDynamicIndirectLighting = false;
	Glazing->bAffectDistanceFieldLighting = false;
	Glazing->bVisibleInRayTracing = false;

	// Lettering faces the square (the front looks along -X).
	auto MakeText = [this](const TCHAR* ComponentName, const FVector& At, float Size, const FColor& Ink, const TCHAR* Words)
	{
		UTextRenderComponent* Label = CreateDefaultSubobject<UTextRenderComponent>(ComponentName);
		Label->SetupAttachment(Mesh);
		Label->SetRelativeLocationAndRotation(At, FRotator(0.0, 180.0, 0.0));
		Label->SetHorizontalAlignment(EHTA_Center);
		Label->SetVerticalAlignment(EVRTA_TextCenter);
		Label->SetWorldSize(Size);
		Label->SetTextRenderColor(Ink);
		Label->SetText(FText::FromString(Words));
		return Label;
	};
	FriezeText = MakeText(TEXT("FriezeText"), Keleti::FriezeTextAt, 80.f, FColor(45, 40, 34), TEXT("KELETI P\u00c1LYAUDVAR"));
	AtticText = MakeText(TEXT("AtticText"), Keleti::AtticTextAt, 190.f, FColor(45, 40, 34), TEXT("M. \u00c1. V."));
	RestaurantText = MakeText(TEXT("RestaurantText"), Keleti::RestaurantTextAt, 45.f, FColor(30, 30, 28), TEXT("\u00c9TTEREM \u00b7 RESTAURANT"));
	StatueText = MakeText(TEXT("StatueText"), Keleti::StatueTextAt, 40.f, FColor(35, 33, 30), TEXT("BAROSS"));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = BasicShapeMaterial.Object;
}

FVector2D AKeletiStation::FrontAt()
{
	// The middle of the main front, from the station's outline in OpenStreetMap.
	return FVector2D(92766.0, -43178.0);
}

FVector2D AKeletiStation::ToCity(const FVector2D& Local)
{
	// The station stands square to the map: its +X is east.
	return FrontAt() + Local;
}

void AKeletiStation::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
}

void AKeletiStation::Build()
{
	using namespace Keleti;

	FBuilder B;
	BuildStation(B);

	const FLinearColor Colors[PartCount] = {
		FLinearColor(0.50f, 0.46f, 0.40f),    // Stone
		FLinearColor(0.38f, 0.35f, 0.30f),    // Rustic
		FLinearColor(0.60f, 0.56f, 0.49f),    // Trim
		FLinearColor(0.025f, 0.03f, 0.035f),  // Glass
		FLinearColor(0.10f, 0.07f, 0.05f),    // Door
		FLinearColor(0.12f, 0.13f, 0.14f),    // Slate
		FLinearColor(0.10f, 0.11f, 0.09f),    // Bronze
		FLinearColor(0.06f, 0.08f, 0.07f),    // Iron
		FLinearColor(0.36f, 0.35f, 0.33f),    // Paving
		FLinearColor(0.21f, 0.19f, 0.17f),    // Ballast
		FLinearColor(0.32f, 0.32f, 0.34f),    // Steel
		FLinearColor(0.05f, 0.12f, 0.07f),    // CarGreen
		FLinearColor(0.22f, 0.22f, 0.22f),    // CarRoof
		FLinearColor(0.03f, 0.03f, 0.03f),    // Black
		FLinearColor(0.35f, 0.04f, 0.03f),    // Red
		FLinearColor(0.10f, 0.22f, 0.06f),    // Lawn
		FLinearColor(0.72f, 0.68f, 0.56f),    // Canvas
		FLinearColor(0.20f, 0.12f, 0.06f),    // Wood
		FLinearColor(0.12f, 0.13f, 0.14f),    // RoofOut
		FLinearColor(0.10f, 0.09f, 0.08f),    // RoofIn
		FLinearColor(0.07f, 0.09f, 0.08f),    // Truss
		FLinearColor(0.45f, 0.52f, 0.55f) };  // Pane

	Mesh->ClearAllMeshSections();
	Overhead->ClearAllMeshSections();
	Glazing->ClearAllMeshSections();
	int32 NumTriangles = 0;
	for (int32 i = 0; i < PartCount; ++i)
	{
		UProceduralMeshComponent* Target = Mesh;
		int32 Section = i;
		if (i >= FirstGlazingPart)
		{
			Target = Glazing;
			Section = i - FirstGlazingPart;
		}
		else if (i >= FirstOverheadPart)
		{
			Target = Overhead;
			Section = i - FirstOverheadPart;
		}
		const FPart& Part = B.Parts[i];
		Target->CreateMeshSection_LinearColor(Section, Part.Vertices, Part.Triangles, Part.Normals,
			TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), Target == Mesh);
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Material->SetVectorParameterValue(TEXT("Color"), Colors[i]);
			Target->SetMaterial(Section, Material);
		}
		NumTriangles += Part.Triangles.Num() / 3;
	}
	UE_LOG(LogTemp, Log, TEXT("KeletiStation: %d triangles"), NumTriangles);
}
