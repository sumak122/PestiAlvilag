// Fill out your copyright notice in the Description page of Project Settings.

#include "Nepszinhaz.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace Nepszinhaz
{
	// One mesh section (and colour) per material of the building.
	enum EPart : int32
	{
		Plaster,
		Rustic,
		Trim,
		Glass,
		Door,
		Roof,
		Statue,
		PartCount
	};

	// Sizes (cm). The front faces +X; Y runs across it; Z is up from the pavement.
	constexpr double Base = -100.0;            // walls start below the pavement so slopes don't show a gap
	constexpr double RusticTop = 650.0;        // top of the rusticated ground floor
	constexpr double NobileSill = 820.0;       // the tall first-floor windows
	constexpr double NobileTop = 1350.0;
	constexpr double MezzanineSill = 1520.0;   // the small second-floor windows
	constexpr double MezzanineTop = 1700.0;
	constexpr double CapitalTop = 1800.0;      // where the columns and pilasters end
	constexpr double CorniceBottom = 1960.0;
	constexpr double CorniceTop = 2060.0;
	constexpr double BalustradeHeight = 110.0;

	constexpr double Front = 2800.0;           // front wall
	constexpr double PorticoFront = 2900.0;    // the central part stands forward of it
	constexpr double Back = -2200.0;           // back wall; the rounded end stands out of its middle
	constexpr double ApseRadius = 1300.0;
	constexpr double HalfWidth = 2200.0;
	constexpr double PorticoHalfWidth = 1300.0;

	constexpr double JointSpacing = 65.0;      // rustication courses

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
		// ridge Rise above Z0.
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

		// A hipped roof over a rectangle, the ridge along its longer side.
		void Hip(EPart Part, const FVector2D& Min, const FVector2D& Max, double Z0, double Rise)
		{
			const bool bAlongX = Max.X - Min.X >= Max.Y - Min.Y;
			const double Half = (bAlongX ? Max.Y - Min.Y : Max.X - Min.X) / 2.0;
			const FVector2D Mid = (Min + Max) / 2.0;
			const double Z1 = Z0 + Rise;
			const FVector A(Min.X, Min.Y, Z0), B(Max.X, Min.Y, Z0), C(Max.X, Max.Y, Z0), D(Min.X, Max.Y, Z0);
			if (bAlongX)
			{
				const FVector R0(Min.X + Half, Mid.Y, Z1), R1(Max.X - Half, Mid.Y, Z1);
				Poly(Part, { A, B, R1, R0 }, FVector(0.0, -1.0, 1.0));
				Poly(Part, { D, C, R1, R0 }, FVector(0.0, 1.0, 1.0));
				Poly(Part, { A, D, R0 }, FVector(-1.0, 0.0, 1.0));
				Poly(Part, { B, C, R1 }, FVector(1.0, 0.0, 1.0));
			}
			else
			{
				const FVector R0(Mid.X, Min.Y + Half, Z1), R1(Mid.X, Max.Y - Half, Z1);
				Poly(Part, { A, D, R1, R0 }, FVector(-1.0, 0.0, 1.0));
				Poly(Part, { B, C, R1, R0 }, FVector(1.0, 0.0, 1.0));
				Poly(Part, { A, B, R0 }, FVector(0.0, -1.0, 1.0));
				Poly(Part, { D, C, R1 }, FVector(0.0, 1.0, 1.0));
			}
		}

		// A box turned to run along Along (level), centred on the line from
		// P0 to P1, HalfDepth to either side of it, from Z0 to Z1 above it.
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

		// A balustrade on a roof or balcony from P0 to P1: plinth, a
		// baluster every 35 cm, a post every few metres, and a rail.
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
		void Figure(const FVector& At, double Scale, bool bArmRaised)
		{
			const double S = Scale;
			Box(Statue, At + FVector(-40.0, -40.0, 0.0) * S, At + FVector(40.0, 40.0, 50.0) * S);
			Frustum(Statue, FVector2D(At), 30.0 * S, 18.0 * S, At.Z + 50.0 * S, At.Z + 200.0 * S, 8);
			Cylinder(Statue, FVector2D(At), 13.0 * S, At.Z + 200.0 * S, At.Z + 232.0 * S, 8);
			Box(Statue, At + FVector(-8.0, -28.0, 120.0) * S, At + FVector(8.0, -16.0, 190.0) * S);
			if (bArmRaised)
			{
				Box(Statue, At + FVector(-8.0, 16.0, 180.0) * S, At + FVector(8.0, 28.0, 270.0) * S);
			}
			else
			{
				Box(Statue, At + FVector(-8.0, 16.0, 120.0) * S, At + FVector(8.0, 28.0, 190.0) * S);
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

		// A framed window: a raised frame around a dark pane set near the wall.
		void Window(FBuilder& B, EPart FramePart, EPart PanePart, double S0, double S1, double Z0, double Z1, double Border, double Depth) const
		{
			Block(B, FramePart, S0, S1, Z1 - Border, Z1, Depth);
			Block(B, FramePart, S0, S1, Z0, Z0 + Border, Depth);
			Block(B, FramePart, S0, S0 + Border, Z0 + Border, Z1 - Border, Depth);
			Block(B, FramePart, S1 - Border, S1, Z0 + Border, Z1 - Border, Depth);
			Panel(B, PanePart, S0 + Border, S1 - Border, Z0 + Border, Z1 - Border, 1.0);
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

		// A triangular hood over a window, Rise high in the middle.
		void Hood(FBuilder& B, EPart Part, double S0, double S1, double Z0, double Rise, double Depth) const
		{
			const double Mid = (S0 + S1) / 2.0;
			Block(B, Part, S0, S1, Z0, Z0 + 14.0, Depth);
			const double Z1 = Z0 + 14.0;
			B.Poly(Part, { At(S0, Z1, Depth), At(S1, Z1, Depth), At(Mid, Z1 + Rise, Depth) }, Out);
			B.Poly(Part, { At(S0, Z1, 0.0), At(Mid, Z1 + Rise, 0.0), At(Mid, Z1 + Rise, Depth), At(S0, Z1, Depth) }, FVector(0.0, 0.0, 1.0) - Along);
			B.Poly(Part, { At(S1, Z1, 0.0), At(Mid, Z1 + Rise, 0.0), At(Mid, Z1 + Rise, Depth), At(S1, Z1, Depth) }, FVector(0.0, 0.0, 1.0) + Along);
		}

		// Dark joints between the courses of the rusticated ground floor;
		// windows and doors sit in front of them.
		void Rustication(FBuilder& B, double S0, double S1) const
		{
			for (double Z = JointSpacing; Z < RusticTop - 20.0; Z += JointSpacing)
			{
				Panel(B, Roof, S0, S1, Z - 4.0, Z, 0.5);
			}
		}

		// One bay of the upper floors: a tall first-floor window (with a
		// triangular hood, a straight one, or none) and a small window above.
		void UpperBay(FBuilder& B, double Mid, int32 HoodKind) const
		{
			const double N0 = Mid - 80.0, N1 = Mid + 80.0;
			Window(B, Trim, Glass, N0, N1, NobileSill, NobileTop, 16.0, 10.0);
			Block(B, Trim, N0 - 25.0, N1 + 25.0, NobileSill - 25.0, NobileSill, 22.0);
			if (HoodKind == 1)
			{
				Hood(B, Trim, N0 - 30.0, N1 + 30.0, NobileTop + 12.0, 70.0, 20.0);
			}
			else if (HoodKind == 2)
			{
				Block(B, Trim, N0 - 30.0, N1 + 30.0, NobileTop + 12.0, NobileTop + 40.0, 22.0);
			}
			Window(B, Trim, Glass, Mid - 65.0, Mid + 65.0, MezzanineSill, MezzanineTop, 14.0, 8.0);
		}

		// A pilaster from the ground-floor ledge up to the entablature.
		void Pilaster(FBuilder& B, double Mid, double HalfWidth_) const
		{
			Block(B, Plaster, Mid - HalfWidth_, Mid + HalfWidth_, RusticTop + 50.0, CapitalTop - 40.0, 14.0);
			Block(B, Trim, Mid - HalfWidth_ - 8.0, Mid + HalfWidth_ + 8.0, CapitalTop - 40.0, CapitalTop, 22.0);
		}

		// The bands every wall of the front building shares: the ledge over
		// the ground floor, the sill band, the entablature and the cornice.
		void Bands(FBuilder& B, double S0, double S1) const
		{
			Block(B, Trim, S0, S1, Base, 30.0, 12.0);
			Block(B, Trim, S0, S1, RusticTop, RusticTop + 50.0, 24.0);
			Block(B, Trim, S0, S1, CapitalTop, CapitalTop + 50.0, 16.0);
			Block(B, Trim, S0, S1, CorniceBottom - 20.0, CorniceBottom, 30.0);
			Block(B, Trim, S0, S1, CorniceBottom, CorniceTop, 55.0);
		}
	};
}

ANepszinhaz::ANepszinhaz()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	Mesh->bUseComplexAsSimpleCollision = true;
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	RootComponent = Mesh;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = BasicShapeMaterial.Object;
}

void ANepszinhaz::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
}

void ANepszinhaz::Build()
{
	using namespace Nepszinhaz;

	FBuilder B;

	// --- The masses: rusticated ground floor, plaster above.
	B.Box(Rustic, FVector(Back, -HalfWidth, Base), FVector(Front, HalfWidth, RusticTop));
	B.Box(Plaster, FVector(Back, -HalfWidth, RusticTop), FVector(Front, HalfWidth, CorniceTop));
	B.Box(Rustic, FVector(Front, -PorticoHalfWidth, Base), FVector(PorticoFront, PorticoHalfWidth, RusticTop));

	// --- The front (+X).
	const FFace FrontWall{ FVector(Front, -HalfWidth, 0.0), FVector(0.0, 1.0, 0.0), FVector(1.0, 0.0, 0.0) };
	const FFace PorticoWall{ FVector(PorticoFront, -HalfWidth, 0.0), FVector(0.0, 1.0, 0.0), FVector(1.0, 0.0, 0.0) };
	const double PorticoS0 = HalfWidth - PorticoHalfWidth;
	const double PorticoS1 = HalfWidth + PorticoHalfWidth;
	const double Width = 2.0 * HalfWidth;

	FrontWall.Bands(B, 0.0, Width);
	FrontWall.Rustication(B, 0.0, PorticoS0);
	FrontWall.Rustication(B, PorticoS1, Width);
	PorticoWall.Rustication(B, PorticoS0, PorticoS1);
	PorticoWall.Block(B, Trim, PorticoS0, PorticoS1, Base, 30.0, 12.0);

	// Corner pavilions: one bay each between pilasters, a grand window
	// under a triangular hood, an arched window below.
	for (const double Mid : { PorticoS0 / 2.0, Width - PorticoS0 / 2.0 })
	{
		FrontWall.Arch(B, Trim, Glass, Mid - 90.0, Mid + 90.0, 140.0, 450.0, 16.0, 10.0, false);
		FrontWall.Pilaster(B, Mid - PorticoS0 / 2.0 + 50.0, 50.0);
		FrontWall.Pilaster(B, Mid + PorticoS0 / 2.0 - 50.0, 50.0);
		FrontWall.UpperBay(B, Mid, 1);
	}

	// The portico: six giant columns on a balustraded terrace, three arched
	// doors and two arched windows below, five bays of windows between.
	constexpr int32 Columns = 6;
	const double ColumnStep = (2.0 * PorticoHalfWidth - 300.0) / (Columns - 1);
	for (int32 i = 0; i < Columns; ++i)
	{
		const double Y = -PorticoHalfWidth + 150.0 + ColumnStep * i;
		B.Box(Trim, FVector(Front - 20.0, Y - 75.0, RusticTop), FVector(PorticoFront + 30.0, Y + 75.0, RusticTop + 70.0));
		B.Cylinder(Trim, FVector2D(Front + 60.0, Y), 55.0, RusticTop + 70.0, CapitalTop - 60.0, 16);
		B.Box(Trim, FVector(Front - 20.0, Y - 80.0, CapitalTop - 60.0), FVector(PorticoFront + 40.0, Y + 80.0, CapitalTop));
	}
	for (int32 i = 0; i + 1 < Columns; ++i)
	{
		const double Y = -PorticoHalfWidth + 150.0 + ColumnStep * (i + 0.5);
		const double Mid = Y + HalfWidth;
		if (i >= 1 && i <= 3)
		{
			PorticoWall.Arch(B, Trim, Door, Mid - 130.0, Mid + 130.0, 0.0, 330.0, 20.0, 12.0, true);
		}
		else
		{
			PorticoWall.Arch(B, Trim, Glass, Mid - 90.0, Mid + 90.0, 140.0, 450.0, 16.0, 10.0, false);
		}
		FrontWall.UpperBay(B, Mid, 2);
	}
	B.Balustrade(Trim, FVector(PorticoFront - 30.0, -PorticoHalfWidth + 20.0, RusticTop + 50.0), FVector(PorticoFront - 30.0, PorticoHalfWidth - 20.0, RusticTop + 50.0), 95.0);
	// Steps up to the doors.
	for (int32 Step = 0; Step < 3; ++Step)
	{
		B.Box(Trim, FVector(PorticoFront, -1000.0 - 40.0 * Step, Base), FVector(PorticoFront + 240.0 - 80.0 * Step, 1000.0 + 40.0 * Step, 18.0 * (Step + 1)));
	}
	// Entablature and pediment over the columns, with figures in the
	// tympanum and a group on top.
	B.Box(Trim, FVector(Front, -PorticoHalfWidth - 40.0, CapitalTop), FVector(PorticoFront + 40.0, PorticoHalfWidth + 40.0, CorniceBottom));
	B.Box(Trim, FVector(Front, -PorticoHalfWidth - 80.0, CorniceBottom), FVector(PorticoFront + 70.0, PorticoHalfWidth + 80.0, CorniceTop));
	B.Gable(Trim, FVector(Front - 150.0, -PorticoHalfWidth - 80.0, CorniceTop), FVector(PorticoFront + 70.0, PorticoHalfWidth + 80.0, CorniceTop), 560.0, true);
	{
		const double X = PorticoFront + 71.0;
		const double Z0 = CorniceTop + 40.0;
		const double HalfBase = PorticoHalfWidth - 60.0;
		const double Rise = (560.0 - 40.0) * HalfBase / (PorticoHalfWidth + 80.0) - 30.0;
		B.Poly(Statue, { FVector(X, -HalfBase, Z0), FVector(X, HalfBase, Z0), FVector(X, 0.0, Z0 + Rise) }, FVector::ForwardVector);
		for (int32 i = -4; i <= 4; ++i)
		{
			const double Y = i * HalfBase / 5.0;
			const double Top = Z0 + Rise * (1.0 - FMath::Abs(Y) / HalfBase) * 0.85;
			B.Box(Statue, FVector(X, Y - 35.0, Z0), FVector(X + 18.0, Y + 35.0, FMath::Max(Z0 + 60.0, Top)));
		}
	}
	B.Figure(FVector(PorticoFront - 20.0, 0.0, CorniceTop + 520.0), 1.5, true);
	B.Figure(FVector(PorticoFront - 20.0, -PorticoHalfWidth - 20.0, CorniceTop), 1.1, false);
	B.Figure(FVector(PorticoFront - 20.0, PorticoHalfWidth + 20.0, CorniceTop), 1.1, false);

	// --- The sides (+Y and -Y): ten bays between pilasters, a side
	// entrance under a columned porch near the front.
	constexpr int32 SideBays = 10;
	constexpr int32 PorchBay = 8;
	const double SideLength = Front - Back;
	const double SideBay = SideLength / SideBays;
	for (const double Side : { 1.0, -1.0 })
	{
		const FFace Wall{ FVector(Back, Side * HalfWidth, 0.0), FVector(1.0, 0.0, 0.0), FVector(0.0, Side, 0.0) };
		Wall.Bands(B, 0.0, SideLength);
		Wall.Rustication(B, 0.0, SideLength);
		for (int32 k = 0; k <= SideBays; ++k)
		{
			Wall.Pilaster(B, FMath::Clamp(SideBay * k, 45.0, SideLength - 45.0), 40.0);
		}
		for (int32 k = 0; k < SideBays; ++k)
		{
			const double Mid = SideBay * (k + 0.5);
			if (k == PorchBay)
			{
				Wall.Arch(B, Trim, Door, Mid - 120.0, Mid + 120.0, 0.0, 320.0, 18.0, 12.0, true);
			}
			else
			{
				Wall.Arch(B, Trim, Glass, Mid - 80.0, Mid + 80.0, 140.0, 440.0, 14.0, 10.0, false);
			}
			Wall.UpperBay(B, Mid, k % 2 == 0 ? 1 : 2);
		}

		// The porch: two columns carrying a balustraded roof.
		const double PorchMid = SideBay * (PorchBay + 0.5);
		const double Out = Side * (HalfWidth + 330.0);
		for (const double S : { PorchMid - 200.0, PorchMid + 200.0 })
		{
			const double X = Back + S;
			B.Cylinder(Trim, FVector2D(X, Out), 30.0, Base, 520.0, 12);
			B.Box(Trim, FVector(X - 40.0, Out - 40.0, 520.0), FVector(X + 40.0, Out + 40.0, 560.0));
		}
		const double Near = Side * HalfWidth;
		const double Far = Side * (HalfWidth + 380.0);
		B.Box(Trim, FVector(Back + PorchMid - 260.0, FMath::Min(Near, Far), 560.0), FVector(Back + PorchMid + 260.0, FMath::Max(Near, Far), 640.0));
		B.Balustrade(Trim, FVector(Back + PorchMid - 240.0, Side * (HalfWidth + 360.0), 640.0), FVector(Back + PorchMid + 240.0, Side * (HalfWidth + 360.0), 640.0), 80.0);

		// Roof balustrade along the side, statues at the front corners and
		// where the pavilions meet the portico.
		B.Balustrade(Trim, FVector(Back, Side * (HalfWidth - 30.0), CorniceTop), FVector(Front - 20.0, Side * (HalfWidth - 30.0), CorniceTop), BalustradeHeight);
		B.Balustrade(Trim, FVector(Front - 30.0, Side * (HalfWidth - 20.0), CorniceTop), FVector(Front - 30.0, Side * (PorticoHalfWidth + 120.0), CorniceTop), BalustradeHeight);
		B.Figure(FVector(Front - 60.0, Side * (HalfWidth - 60.0), CorniceTop), 1.0, Side > 0.0);
	}

	// --- The roof: a hipped roof over the front building and, over the
	// auditorium, a raised square block with windows under a square dome
	// whose sides curve in to a flat top with a railing.
	B.Hip(Roof, FVector2D(Back + 60.0, -HalfWidth + 120.0), FVector2D(Front - 250.0, HalfWidth - 120.0), CorniceTop, 520.0);
	{
		const FVector2D DomeCenter((Back + Front) / 2.0 - 300.0, 0.0);
		constexpr double DrumHalfX = 1100.0;
		constexpr double DrumHalfY = 1250.0;
		constexpr double DrumBottom = CorniceTop + 200.0;
		constexpr double DrumTop = CorniceTop + 800.0;
		B.Box(Plaster, FVector(DomeCenter.X - DrumHalfX, DomeCenter.Y - DrumHalfY, DrumBottom), FVector(DomeCenter.X + DrumHalfX, DomeCenter.Y + DrumHalfY, DrumTop));
		const FFace DrumFaces[4] = {
			{ FVector(DomeCenter.X + DrumHalfX, DomeCenter.Y - DrumHalfY, 0.0), FVector(0.0, 1.0, 0.0), FVector(1.0, 0.0, 0.0) },
			{ FVector(DomeCenter.X - DrumHalfX, DomeCenter.Y + DrumHalfY, 0.0), FVector(0.0, -1.0, 0.0), FVector(-1.0, 0.0, 0.0) },
			{ FVector(DomeCenter.X + DrumHalfX, DomeCenter.Y + DrumHalfY, 0.0), FVector(-1.0, 0.0, 0.0), FVector(0.0, 1.0, 0.0) },
			{ FVector(DomeCenter.X - DrumHalfX, DomeCenter.Y - DrumHalfY, 0.0), FVector(1.0, 0.0, 0.0), FVector(0.0, -1.0, 0.0) } };
		for (int32 f = 0; f < 4; ++f)
		{
			const FFace& Face = DrumFaces[f];
			const double Length = 2.0 * (f < 2 ? DrumHalfY : DrumHalfX);
			constexpr int32 DrumBays = 5;
			Face.Block(B, Trim, 0.0, Length, DrumTop - 90.0, DrumTop, 40.0);
			Face.Block(B, Trim, 0.0, Length, DrumBottom, DrumBottom + 40.0, 16.0);
			for (int32 k = 0; k <= DrumBays; ++k)
			{
				const double S = FMath::Clamp(Length * k / DrumBays, 35.0, Length - 35.0);
				Face.Block(B, Plaster, S - 35.0, S + 35.0, DrumBottom + 40.0, DrumTop - 90.0, 12.0);
			}
			for (int32 k = 0; k < DrumBays; ++k)
			{
				const double Mid = Length * (k + 0.5) / DrumBays;
				Face.Arch(B, Trim, Glass, Mid - 70.0, Mid + 70.0, DrumBottom + 120.0, DrumBottom + 340.0, 12.0, 8.0, false);
			}
		}
		// The dome: rings of the square, each narrower, steeper at the
		// bottom and flatter towards the top.
		const double Rings[][3] = { { 1.03, 1.0, 120.0 }, { 1.0, 0.92, 140.0 }, { 0.92, 0.80, 120.0 }, { 0.80, 0.66, 90.0 }, { 0.66, 0.55, 50.0 } };
		double Z = DrumTop;
		for (const auto& Ring : Rings)
		{
			B.SquareRing(Roof, DomeCenter, DrumHalfX * Ring[0], DrumHalfY * Ring[0], DrumHalfX * Ring[1], DrumHalfY * Ring[1], Z, Z + Ring[2]);
			Z += Ring[2];
		}
		const double TopX = DrumHalfX * 0.55, TopY = DrumHalfY * 0.55;
		B.Box(Roof, FVector(DomeCenter.X - TopX, DomeCenter.Y - TopY, Z - 1.0), FVector(DomeCenter.X + TopX, DomeCenter.Y + TopY, Z + 10.0));
		const FVector Corners[4] = {
			FVector(DomeCenter.X - TopX + 20.0, DomeCenter.Y - TopY + 20.0, Z + 10.0), FVector(DomeCenter.X + TopX - 20.0, DomeCenter.Y - TopY + 20.0, Z + 10.0),
			FVector(DomeCenter.X + TopX - 20.0, DomeCenter.Y + TopY - 20.0, Z + 10.0), FVector(DomeCenter.X - TopX + 20.0, DomeCenter.Y + TopY - 20.0, Z + 10.0) };
		for (int32 i = 0; i < 4; ++i)
		{
			B.Balustrade(Trim, Corners[i], Corners[(i + 1) % 4], 90.0, false);
		}
	}

	// --- The back, towards the Körút: a bay at each corner, and between
	// them the rounded end, two floors under a balustraded terrace.
	{
		const FFace Wall{ FVector(Back, HalfWidth, 0.0), FVector(0.0, -1.0, 0.0), FVector(-1.0, 0.0, 0.0) };
		const double Corner = HalfWidth - ApseRadius;
		Wall.Bands(B, 0.0, Width);
		Wall.Rustication(B, 0.0, Width);
		for (const double S0 : { 0.0, Width - Corner })
		{
			const double Mid = S0 + Corner / 2.0;
			Wall.Pilaster(B, S0 + 45.0, 40.0);
			Wall.Pilaster(B, S0 + Corner - 45.0, 40.0);
			Wall.Arch(B, Trim, Glass, Mid - 80.0, Mid + 80.0, 140.0, 440.0, 14.0, 10.0, false);
			Wall.UpperBay(B, Mid, 1);
			B.Balustrade(Trim, FVector(Back + 30.0, HalfWidth - S0 - 20.0, CorniceTop), FVector(Back + 30.0, HalfWidth - S0 - Corner, CorniceTop), BalustradeHeight);
		}

		constexpr int32 ApseSides = 10;
		const FVector2D Center(Back, 0.0);
		FVector2D Points[ApseSides + 1];
		for (int32 i = 0; i <= ApseSides; ++i)
		{
			const double A = PI / 2.0 + PI * i / ApseSides;
			Points[i] = Center + FVector2D(FMath::Cos(A), FMath::Sin(A)) * ApseRadius;
		}
		for (int32 i = 0; i < ApseSides; ++i)
		{
			const FVector2D Chord = Points[i + 1] - Points[i];
			const double Length = FMath::Sqrt(Chord.X * Chord.X + Chord.Y * Chord.Y);
			const FVector Along(Chord.X / Length, Chord.Y / Length, 0.0);
			const FVector Out(Along.Y, -Along.X, 0.0);
			const FFace Segment{ FVector(Points[i], 0.0), Along, Out };
			const FVector P0(Points[i], 0.0), P1(Points[i + 1], 0.0);
			B.Poly(Rustic, { P0 + FVector(0.0, 0.0, Base), P1 + FVector(0.0, 0.0, Base), P1 + FVector(0.0, 0.0, RusticTop), P0 + FVector(0.0, 0.0, RusticTop) }, Out);
			B.Poly(Plaster, { P0 + FVector(0.0, 0.0, RusticTop), P1 + FVector(0.0, 0.0, RusticTop), P1 + FVector(0.0, 0.0, CorniceTop), P0 + FVector(0.0, 0.0, CorniceTop) }, Out);
			B.Poly(Plaster, { FVector(Center, CorniceTop), P0 + FVector(0.0, 0.0, CorniceTop), P1 + FVector(0.0, 0.0, CorniceTop) }, FVector::UpVector);
			Segment.Bands(B, 0.0, Length);
			Segment.Rustication(B, 0.0, Length);
			Segment.Pilaster(B, 35.0, 30.0);
			Segment.Arch(B, Trim, Glass, Length / 2.0 - 75.0, Length / 2.0 + 75.0, 140.0, 440.0, 14.0, 10.0, false);
			Segment.UpperBay(B, Length / 2.0, i % 2 == 0 ? 2 : 1);
			const FVector2D In0 = Center + (Points[i] - Center) * ((ApseRadius - 30.0) / ApseRadius);
			const FVector2D In1 = Center + (Points[i + 1] - Center) * ((ApseRadius - 30.0) / ApseRadius);
			B.Balustrade(Trim, FVector(In0, CorniceTop), FVector(In1, CorniceTop), BalustradeHeight, false);
		}
	}

	// --- Hand the parts to the mesh, one coloured section each.
	Mesh->ClearAllMeshSections();
	const FLinearColor Colors[PartCount] = { PlasterColor, RusticColor, TrimColor, GlassColor, DoorColor, RoofColor, StatueColor };
	int32 NumTriangles = 0;
	for (int32 i = 0; i < PartCount; ++i)
	{
		const FPart& Part = B.Parts[i];
		Mesh->CreateMeshSection_LinearColor(i, Part.Vertices, Part.Triangles, Part.Normals,
			TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), true);
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Material->SetVectorParameterValue(TEXT("Color"), Colors[i]);
			Mesh->SetMaterial(i, Material);
		}
		NumTriangles += Part.Triangles.Num() / 3;
	}
	UE_LOG(LogTemp, Log, TEXT("Nepszinhaz: %d triangles"), NumTriangles);
}
