// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PersonKit.h"

/**
 * The cars of 1932 Pest, made from boxes, bars, cylinders and side-on
 * outlines: five kinds of body (a four-door saloon, an open tourer, a small
 * two-seat coupé, a long limousine and a delivery van), each with swept
 * mudguards over the wheels, running boards, a chrome radiator and bumpers,
 * drum headlamps, see-through windows with a driver behind the wheel, and
 * a tail pipe. A style picks the body, the paint, the mudguard colour and
 * the kind of wheel, so the streets fill with different cars.
 *
 * The wheels are separate meshes, each with its middle at the hub, so the
 * game can roll them and turn the front ones.
 *
 * Header-only (like TramModel) so the offline preview builds the same
 * shapes. Frame: X forward, Y right, Z up; the ground is 90 cm below the
 * origin (the middle of the car's collision box).
 */
namespace CarModel
{
	/** One mesh section (and colour) each. */
	enum EPart : int32
	{
		Paint,
		/** Mudguards, running boards and the chassis: black, or the paint. */
		Guards,
		/** Tyres, and the driver's coat and hat. */
		Rubber,
		Chrome,
		/** Window glass: see-through. */
		Glass,
		Lamp,
		TailLamp,
		Leather,
		Skin,
		/** The taxi's yellow, the van's sign panel. */
		Accent,
		/** The wheel's spokes or disc. */
		Spokes,
		PartCount
	};

	/** Balilla: the little two-door Fiat 508 Balilla (1932), upright, with a trunk on the back. Opel: the boxy two-door Opel 1.2 Liter (1931). Steyr: Dr. Halász's big four-door Steyr saloon (mission 7), also rare in traffic. Ssk: the black Mercedes-Benz SSK two-seater of the car show (never in traffic). */
	enum class EBody : uint8 { Saloon, Tourer, Coupe, Limousine, Van, Balilla, Opel, Steyr, Ssk, Count };
	enum class EWheel : uint8 { Artillery, Wire, Disc, Count };

	struct FStyle
	{
		EBody Body = EBody::Saloon;
		EWheel Wheel = EWheel::Artillery;
		FLinearColor PaintColor = FLinearColor(0.03f, 0.07f, 0.05f);
		FLinearColor GuardColor = FLinearColor(0.012f, 0.012f, 0.012f);
		FLinearColor SpokeColor = FLinearColor(0.55f, 0.48f, 0.33f);
		FLinearColor AccentColor = FLinearColor(0.85f, 0.62f, 0.05f);
		/** Taxi markings: a chequered band, the roof lamp and the meter flag. */
		bool bTaxi = false;
		/** The police's blue glass lamp by the windscreen (the Accent colour). */
		bool bPolice = false;
	};

	struct FPart
	{
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
	};

	constexpr double Ground = -90.0;
	constexpr double WheelRadius = 36.0;
	constexpr double WheelWidth = 13.0;
	constexpr double HubZ = Ground + WheelRadius;
	constexpr double WheelY = 74.0;
	/** The top of the lower body, where the windows start. */
	constexpr double Waist = 18.0;
	constexpr double BodyHalfWidth = 72.0;

	/** Where things are on one kind of body. */
	struct FLayout
	{
		/** Front and rear axle (cm forward of the middle). */
		double FrontAxle = 130.0;
		double RearAxle = -130.0;
		/** The front of the radiator, and the back of the body. */
		double Nose = 186.0;
		double Tail = -196.0;
		/** The windscreen's foot. */
		double Cowl = 22.0;
		double RoofTop = 80.0;
		/** The tail pipe's end, where the smoke comes out. */
		FVector Exhaust = FVector(-214.0, 40.0, -62.0);
		/** The front edge of the driver's seat cushion, and its top. */
		double SeatX = -40.0;
		double SeatTop = Waist - 46.0;
		/** Where a back-seat passenger's hips are, and the middle of the back doors. */
		double RearSeatX = -100.0;
		double RearDoorX = -80.0;
		/** How far out the wheels' middles are (half the track), and the body's half width. */
		double TrackY = WheelY;
		double HalfWidth = BodyHalfWidth;
		/** Where the driver and a back-seat passenger sit across the car (DriverY, PassengerY; nearer the middle in a narrow car). */
		double DriverSide = -36.0;
		double PassengerSide = 30.0;
	};

	/** The driver sits on the left (FLayout::DriverSide). How far the arms reach forward to the wheel, and the thighs tilt up (degrees). */
	constexpr double DriverArmPitch = 70.0;
	constexpr double DriverLegPitch = 25.0;
	/** Taller people are made this tall in the car, so their hats stay under the roof (cm). */
	constexpr double DriverMaxHeight = 174.0;
	/** A back-seat passenger sits on the right (FLayout::PassengerSide), hands in the lap. */
	constexpr double PassengerArmPitch = 40.0;
	constexpr double PassengerLegPitch = 10.0;

	inline FLayout Layout(EBody Body)
	{
		FLayout L;
		switch (Body)
		{
		case EBody::Coupe:
			L.FrontAxle = 116.0;
			L.RearAxle = -116.0;
			L.Nose = 170.0;
			L.Tail = -186.0;
			L.RoofTop = 78.0;
			L.Exhaust = FVector(-200.0, 38.0, -62.0);
			break;
		case EBody::Limousine:
			L.FrontAxle = 142.0;
			L.RearAxle = -138.0;
			L.Nose = 198.0;
			L.Tail = -186.0;
			L.Cowl = 32.0;
			L.RoofTop = 82.0;
			L.SeatX = -30.0;
			L.RearDoorX = -67.0;
			L.Exhaust = FVector(-226.0, 40.0, -62.0);
			break;
		case EBody::Van:
			L.Tail = -208.0;
			L.RoofTop = 88.0;
			L.Exhaust = FVector(-222.0, 44.0, -62.0);
			L.SeatX = -36.0;
			break;
		case EBody::Tourer:
			L.RoofTop = 52.0;
			break;
		case EBody::Balilla:
			// 3.2 m long and 1.39 m wide on a 2.25 m wheelbase, 1.19 m track, 1.6 m tall.
			// The radiator stands right over the front axle; the trunk hangs off the back.
			L.FrontAxle = 113.0;
			L.RearAxle = -112.0;
			L.Nose = 122.0;
			L.Tail = -148.0;
			L.Cowl = 24.0;
			L.RoofTop = 68.0;
			L.TrackY = 60.0;
			L.HalfWidth = 58.0;
			L.Exhaust = FVector(-176.0, 30.0, -62.0);
			L.SeatX = -42.0;
			L.RearSeatX = -100.0;
			L.RearDoorX = -24.0;
			// Sitting low, so a hat stays under the low roof.
			L.SeatTop = Waist - 54.0;
			L.DriverSide = -27.0;
			L.PassengerSide = 25.0;
			break;
		case EBody::Opel:
			// 3.35 m long and 1.42 m wide on a 2.29 m wheelbase, 1.6 m tall.
			L.FrontAxle = 114.0;
			L.RearAxle = -115.0;
			L.Nose = 150.0;
			L.Tail = -156.0;
			L.Cowl = 38.0;
			L.RoofTop = 72.0;
			L.TrackY = 61.0;
			L.HalfWidth = 60.0;
			L.Exhaust = FVector(-170.0, 30.0, -62.0);
			L.SeatX = -38.0;
			L.RearSeatX = -100.0;
			L.RearDoorX = -10.0;
			L.SeatTop = Waist - 50.0;
			L.DriverSide = -28.0;
			L.PassengerSide = 26.0;
			break;
		case EBody::Steyr:
			// The Steyr XII / XXX saloon: a long bonnet, a tall four-door body
			// and a trunk on a rack behind, about 4.4 m long over the bumpers.
			// Everything sits 25 cm further forward than the long trunk would
			// put it, so bumper to bumper the car is centred on its collision
			// box (PestiCar::BoxExtent), like the other cars.
			L.FrontAxle = 175.0;
			L.RearAxle = -107.0;
			L.Nose = 185.0;
			L.Tail = -175.0;
			L.Cowl = 55.0;
			L.RoofTop = 80.0;
			L.SeatX = -7.0;
			L.RearSeatX = -87.0;
			L.RearDoorX = -43.0;
			L.Exhaust = FVector(-213.0, 40.0, -62.0);
			break;
		case EBody::Ssk:
			// The Mercedes-Benz SSK "Black Prince" roadster: the radiator just
			// behind the front axle, a long bonnet, a low two-seater far back
			// and a long pointed tail between the rear wings; centred on the
			// collision box like the others, a little longer than it.
			L.FrontAxle = 168.0;
			L.RearAxle = -102.0;
			L.Nose = 160.0;
			L.Tail = -205.0;
			L.Cowl = 0.0;
			L.RoofTop = 28.0;
			L.SeatX = -78.0;
			L.SeatTop = Waist - 52.0;
			L.RearSeatX = -78.0;
			L.RearDoorX = -60.0;
			L.HalfWidth = 44.0;
			L.TrackY = 72.0;
			L.DriverSide = -20.0;
			L.PassengerSide = 20.0;
			L.Exhaust = FVector(-44.0, 52.0, -70.0);
			break;
		default:
			break;
		}
		return L;
	}

	/** The four hubs, in the order front left, front right, rear left, rear right. */
	inline FVector WheelHub(EBody Body, int32 Index)
	{
		const FLayout L = Layout(Body);
		return FVector(Index < 2 ? L.FrontAxle : L.RearAxle, (Index % 2 == 0) ? -L.TrackY : L.TrackY, HubZ);
	}

	struct FBuilder
	{
		FPart Parts[PartCount];

		void Quad(EPart Part, const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& Normal)
		{
			FPart& P = Parts[Part];
			const int32 First = P.Vertices.Num();
			for (const FVector& V : { A, B, C, D })
			{
				P.Vertices.Add(V);
				P.Normals.Add(Normal);
			}
			// As in TramModel: Unreal shows a triangle from the side it runs
			// clockwise, so wind against Normal's run.
			const FVector Facing = FVector::CrossProduct(C - A, D - B);
			if (FVector::DotProduct(Facing, Normal) < 0.0)
			{
				for (const int32 I : { 0, 1, 2, 0, 2, 3 })
				{
					P.Triangles.Add(First + I);
				}
			}
			else
			{
				for (const int32 I : { 0, 2, 1, 0, 3, 2 })
				{
					P.Triangles.Add(First + I);
				}
			}
		}

		/** A quad with its own normal at each corner, for smooth shading over a curved skin. */
		void SmoothQuad(EPart Part, const FVector& A, const FVector& B, const FVector& C, const FVector& D,
			const FVector& NA, const FVector& NB, const FVector& NC, const FVector& ND)
		{
			FPart& P = Parts[Part];
			const int32 First = P.Vertices.Num();
			P.Vertices.Append({ A, B, C, D });
			P.Normals.Append({ NA, NB, NC, ND });
			const FVector Facing = FVector::CrossProduct(C - A, D - B);
			if (FVector::DotProduct(Facing, NA + NB + NC + ND) < 0.0)
			{
				P.Triangles.Append({ First, First + 1, First + 2, First, First + 2, First + 3 });
			}
			else
			{
				P.Triangles.Append({ First, First + 2, First + 1, First, First + 3, First + 2 });
			}
		}

		/** A pane seen from both sides. */
		void Pane(EPart Part, const FVector& A, const FVector& B, const FVector& C, const FVector& D)
		{
			const FVector Normal = FVector::CrossProduct(C - A, D - B).GetSafeNormal();
			Quad(Part, A, B, C, D, Normal);
			Quad(Part, A, B, C, D, -Normal);
		}

		void Box(EPart Part, const FVector& Min, const FVector& Max)
		{
			const FVector C[8] = {
				FVector(Min.X, Min.Y, Min.Z), FVector(Max.X, Min.Y, Min.Z), FVector(Max.X, Max.Y, Min.Z), FVector(Min.X, Max.Y, Min.Z),
				FVector(Min.X, Min.Y, Max.Z), FVector(Max.X, Min.Y, Max.Z), FVector(Max.X, Max.Y, Max.Z), FVector(Min.X, Max.Y, Max.Z) };
			Quad(Part, C[4], C[5], C[6], C[7], FVector(0.0, 0.0, 1.0));
			Quad(Part, C[0], C[3], C[2], C[1], FVector(0.0, 0.0, -1.0));
			Quad(Part, C[0], C[1], C[5], C[4], FVector(0.0, -1.0, 0.0));
			Quad(Part, C[3], C[7], C[6], C[2], FVector(0.0, 1.0, 0.0));
			Quad(Part, C[0], C[4], C[7], C[3], FVector(-1.0, 0.0, 0.0));
			Quad(Part, C[1], C[2], C[6], C[5], FVector(1.0, 0.0, 0.0));
		}

		/** A box mirrored to both sides of the car (Y and -Y). */
		void BothSides(EPart Part, const FVector& Min, const FVector& Max)
		{
			Box(Part, Min, Max);
			Box(Part, FVector(Min.X, -Max.Y, Min.Z), FVector(Max.X, -Min.Y, Max.Z));
		}

		/** A square bar from A to B, Thick across. */
		void Bar(EPart Part, const FVector& A, const FVector& B, double Thick)
		{
			const FVector Along = (B - A).GetSafeNormal();
			FVector Side = FVector::CrossProduct(Along, FVector(0.0, 0.0, 1.0));
			if (Side.SizeSquared() < 1e-6)
			{
				Side = FVector(0.0, 1.0, 0.0);
			}
			Side = Side.GetSafeNormal();
			const FVector Up = FVector::CrossProduct(Side, Along).GetSafeNormal();
			const FVector S = Side * (Thick * 0.5);
			const FVector U = Up * (Thick * 0.5);
			const FVector A0 = A - S - U, A1 = A + S - U, A2 = A + S + U, A3 = A - S + U;
			const FVector B0 = B - S - U, B1 = B + S - U, B2 = B + S + U, B3 = B - S + U;
			Quad(Part, A0, A1, B1, B0, -Up);
			Quad(Part, A1, A2, B2, B1, Side);
			Quad(Part, A2, A3, B3, B2, Up);
			Quad(Part, A3, A0, B0, B3, -Side);
			Quad(Part, A0, A3, A2, A1, -Along);
			Quad(Part, B0, B1, B2, B3, Along);
		}

		/** A bar on each side: A and B given for the right (+Y) side. */
		void BarBothSides(EPart Part, const FVector& A, const FVector& B, double Thick)
		{
			Bar(Part, A, B, Thick);
			Bar(Part, FVector(A.X, -A.Y, A.Z), FVector(B.X, -B.Y, B.Z), Thick);
		}

		/**
		 * A side-on outline (X, Z), which must bulge outwards everywhere,
		 * pushed out sideways from Y0 to Y1.
		 */
		void SideOutline(EPart Part, const TArray<FVector2D>& Outline, double Y0, double Y1)
		{
			Outline3(Part, Outline, Y0, Y1, 1);
		}

		/** A front-on outline (Y, Z), which must bulge outwards everywhere, run along X from X0 to X1. */
		void FrontOutline(EPart Part, const TArray<FVector2D>& Outline, double X0, double X1)
		{
			Outline3(Part, Outline, X0, X1, 0);
		}

		void Outline3(EPart Part, const TArray<FVector2D>& Outline, double From, double To, int32 Axis)
		{
			const int32 N = Outline.Num();
			if (N < 3)
			{
				return;
			}
			auto At = [Axis](const FVector2D& P, double W)
			{
				return Axis == 1 ? FVector(P.X, W, P.Y) : FVector(W, P.X, P.Y);
			};
			auto Out2 = [Axis](const FVector2D& D)
			{
				return Axis == 1 ? FVector(D.X, 0.0, D.Y) : FVector(0.0, D.X, D.Y);
			};
			const FVector Along = Axis == 1 ? FVector(0.0, 1.0, 0.0) : FVector(1.0, 0.0, 0.0);
			FVector2D Middle(0.0, 0.0);
			for (const FVector2D& P : Outline)
			{
				Middle += P;
			}
			Middle /= static_cast<double>(N);
			for (int32 i = 0; i < N; ++i)
			{
				const FVector2D& P0 = Outline[i];
				const FVector2D& P1 = Outline[(i + 1) % N];
				const FVector2D Edge = P1 - P0;
				FVector2D Out(Edge.Y, -Edge.X);
				if (FVector2D::DotProduct(Out, (P0 + P1) * 0.5 - Middle) < 0.0)
				{
					Out = -Out;
				}
				Quad(Part, At(P0, From), At(P1, From), At(P1, To), At(P0, To), Out2(Out.GetSafeNormal()));
				// The two flat ends, as fans of quads to the middle.
				Quad(Part, At(Middle, To), At(Middle, To), At(P0, To), At(P1, To), Along);
				Quad(Part, At(Middle, From), At(Middle, From), At(P1, From), At(P0, From), -Along);
			}
		}

		/**
		 * A mudguard: a strip Thick deep along a side-on path (X, Z), from
		 * Y0 to Y1, mirrored to both sides of the car (Y0 and Y1 on the right).
		 */
		void Guard(EPart Part, const TArray<FVector2D>& Path, double Y0, double Y1, double Thick)
		{
			for (const double Sign : { 1.0, -1.0 })
			{
				const double A = Y0 * Sign;
				const double B = Y1 * Sign;
				const int32 N = Path.Num();
				TArray<FVector2D> Outer;
				TArray<FVector2D> Inner;
				TArray<FVector2D> Normal;
				for (int32 i = 0; i < N; ++i)
				{
					const FVector2D Tangent = (Path[FMath::Min(i + 1, N - 1)] - Path[FMath::Max(i - 1, 0)]).GetSafeNormal();
					const FVector2D Out(Tangent.Y, -Tangent.X);
					Normal.Add(Out);
					Outer.Add(Path[i] + Out * (Thick * 0.5));
					Inner.Add(Path[i] - Out * (Thick * 0.5));
				}
				auto P = [](const FVector2D& V, double Y) { return FVector(V.X, Y, V.Y); };
				for (int32 i = 0; i + 1 < N; ++i)
				{
					const FVector2D Mid = (Normal[i] + Normal[i + 1]).GetSafeNormal();
					const FVector Up(Mid.X, 0.0, Mid.Y);
					Quad(Part, P(Outer[i], A), P(Outer[i + 1], A), P(Outer[i + 1], B), P(Outer[i], B), Up);
					Quad(Part, P(Inner[i], A), P(Inner[i + 1], A), P(Inner[i + 1], B), P(Inner[i], B), -Up);
					Quad(Part, P(Outer[i], A), P(Outer[i + 1], A), P(Inner[i + 1], A), P(Inner[i], A), FVector(0.0, A < B ? -1.0 : 1.0, 0.0));
					Quad(Part, P(Outer[i], B), P(Outer[i + 1], B), P(Inner[i + 1], B), P(Inner[i], B), FVector(0.0, A < B ? 1.0 : -1.0, 0.0));
				}
				for (const int32 End : { 0, N - 1 })
				{
					const FVector2D Tangent = (Path[End == 0 ? 0 : N - 1] - Path[End == 0 ? 1 : N - 2]).GetSafeNormal();
					Quad(Part, P(Outer[End], A), P(Inner[End], A), P(Inner[End], B), P(Outer[End], B), FVector(Tangent.X, 0.0, Tangent.Y));
				}
			}
		}

		/** A round bar, wheel or lamp: Axis 0 along X, 1 along Y, 2 along Z. */
		void Cylinder(EPart Part, const FVector& Centre, double Radius, double Length, int32 Axis, int32 Sides = 12)
		{
			Ring(Part, Centre, Radius, 0.0, Length, Axis, Sides);
		}

		/** A tube: a cylinder with a round hole through it (InnerRadius 0 for none). */
		void Ring(EPart Part, const FVector& Centre, double Radius, double InnerRadius, double Length, int32 Axis, int32 Sides = 12)
		{
			auto Point = [&](double Angle, double R, double S)
			{
				const double U = R * FMath::Cos(Angle);
				const double V = R * FMath::Sin(Angle);
				const double W = Length * 0.5 * S;
				return Axis == 0 ? Centre + FVector(W, U, V) : Axis == 1 ? Centre + FVector(U, W, V) : Centre + FVector(U, V, W);
			};
			const FVector Along = Axis == 0 ? FVector(1.0, 0.0, 0.0) : Axis == 1 ? FVector(0.0, 1.0, 0.0) : FVector(0.0, 0.0, 1.0);
			for (int32 i = 0; i < Sides; ++i)
			{
				const double A0 = 2.0 * PI * i / Sides;
				const double A1 = 2.0 * PI * (i + 1) / Sides;
				const FVector Out = (Point((A0 + A1) * 0.5, 1.0, 0.0) - Centre).GetSafeNormal();
				Quad(Part, Point(A0, Radius, -1.0), Point(A1, Radius, -1.0), Point(A1, Radius, 1.0), Point(A0, Radius, 1.0), Out);
				if (InnerRadius > 0.0)
				{
					Quad(Part, Point(A0, InnerRadius, -1.0), Point(A1, InnerRadius, -1.0), Point(A1, InnerRadius, 1.0), Point(A0, InnerRadius, 1.0), -Out);
				}
				Quad(Part, Point(A0, InnerRadius, 1.0), Point(A1, InnerRadius, 1.0), Point(A1, Radius, 1.0), Point(A0, Radius, 1.0), Along);
				Quad(Part, Point(A1, InnerRadius, -1.0), Point(A0, InnerRadius, -1.0), Point(A0, Radius, -1.0), Point(A1, Radius, -1.0), -Along);
			}
		}

		/** A low, rounded ball (a head), Radius across, a little taller than wide. */
		void Ball(EPart Part, const FVector& Centre, double Radius, double Tall = 1.15)
		{
			const int32 Around = 8;
			const int32 Rings = 6;
			auto Point = [&](int32 i, int32 j)
			{
				const double Lat = PI * (static_cast<double>(j) / Rings - 0.5);
				const double Lon = 2.0 * PI * i / Around;
				return Centre + FVector(FMath::Cos(Lat) * FMath::Cos(Lon) * Radius, FMath::Cos(Lat) * FMath::Sin(Lon) * Radius, FMath::Sin(Lat) * Radius * Tall);
			};
			for (int32 j = 0; j < Rings; ++j)
			{
				for (int32 i = 0; i < Around; ++i)
				{
					const FVector A = Point(i, j), B = Point(i + 1, j), C = Point(i + 1, j + 1), D = Point(i, j + 1);
					const FVector Out = ((A + B + C + D) * 0.25 - Centre).GetSafeNormal();
					Quad(Part, A, B, C, D, Out);
				}
			}
		}
	};

	/**
	 * A roof with well rounded edges: a slab from X0 to X1 and HalfW each side,
	 * from ZBase up to ZTop, its top edges rounded over by EdgeRadius and its
	 * corners rounded by CornerRadius seen from above (CornerRadius > EdgeRadius).
	 */
	inline void RoundedTop(FBuilder& B, EPart Part, double X0, double X1, double HalfW, double ZBase, double ZTop, double EdgeRadius, double CornerRadius)
	{
		const int32 CornerSteps = 4;
		const int32 EdgeSteps = 4;
		const FVector2D Centres[4] = { FVector2D(X1 - CornerRadius, HalfW - CornerRadius), FVector2D(X0 + CornerRadius, HalfW - CornerRadius),
			FVector2D(X0 + CornerRadius, -HalfW + CornerRadius), FVector2D(X1 - CornerRadius, -HalfW + CornerRadius) };
		const int32 Around = 4 * (CornerSteps + 1);
		// Point i round the edge (each corner has CornerSteps + 1 of them), Inset in from it, at height Z.
		auto Angle = [&](int32 i) { return HALF_PI * (i / (CornerSteps + 1) + static_cast<double>(i % (CornerSteps + 1)) / CornerSteps); };
		auto Point = [&](int32 i, double Inset, double Z)
		{
			const FVector2D C = Centres[(i % Around) / (CornerSteps + 1)];
			const double A = Angle(i % Around);
			return FVector(C.X + (CornerRadius - Inset) * FMath::Cos(A), C.Y + (CornerRadius - Inset) * FMath::Sin(A), Z);
		};
		// Which way the strip from point i to i + 1 faces, tilted up by Lift.
		auto Facing = [&](int32 i, double Lift)
		{
			const double A = (Angle(i % Around) + Angle((i + 1) % Around) + ((i + 1) % Around == 0 ? 2.0 * PI : 0.0)) * 0.5;
			return FVector(FMath::Cos(A) * FMath::Cos(Lift), FMath::Sin(A) * FMath::Cos(Lift), FMath::Sin(Lift));
		};
		const double WallTop = ZTop - EdgeRadius;
		for (int32 i = 0; i < Around; ++i)
		{
			B.Quad(Part, Point(i, 0.0, ZBase), Point(i + 1, 0.0, ZBase), Point(i + 1, 0.0, WallTop), Point(i, 0.0, WallTop), Facing(i, 0.0));
			for (int32 k = 0; k < EdgeSteps; ++k)
			{
				const double Lift0 = HALF_PI * k / EdgeSteps;
				const double Lift1 = HALF_PI * (k + 1) / EdgeSteps;
				const double In0 = EdgeRadius * (1.0 - FMath::Cos(Lift0));
				const double In1 = EdgeRadius * (1.0 - FMath::Cos(Lift1));
				const double Z0 = WallTop + EdgeRadius * FMath::Sin(Lift0);
				const double Z1 = WallTop + EdgeRadius * FMath::Sin(Lift1);
				B.Quad(Part, Point(i, In0, Z0), Point(i + 1, In0, Z0), Point(i + 1, In1, Z1), Point(i, In1, Z1), Facing(i, (Lift0 + Lift1) * 0.5));
			}
		}
		// The flat top and the ceiling under it, as fans from the middle.
		const FVector Mid((X0 + X1) * 0.5, 0.0, ZTop);
		const FVector Under((X0 + X1) * 0.5, 0.0, ZBase);
		for (int32 i = 0; i < Around; i += 2)
		{
			B.Quad(Part, Mid, Point(i, EdgeRadius, ZTop), Point(i + 1, EdgeRadius, ZTop), Point(i + 2, EdgeRadius, ZTop), FVector::UpVector);
			B.Quad(Part, Under, Point(i, 0.0, ZBase), Point(i + 1, 0.0, ZBase), Point(i + 2, 0.0, ZBase), -FVector::UpVector);
		}
	}

	/** Points on a circle round (X, Z), from angle From to To (degrees, 0 is forward, 90 up). */
	inline void Arc(TArray<FVector2D>& Out, const FVector2D& Centre, double Radius, double From, double To, int32 Steps)
	{
		for (int32 i = 0; i <= Steps; ++i)
		{
			const double A = FMath::DegreesToRadians(From + (To - From) * i / Steps);
			Out.Add(Centre + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Radius);
		}
	}

	/** The front end: radiator, bonnet, headlamps, bumper, starting handle. */
	inline void BuildFront(FBuilder& B, const FLayout& L, double BonnetHalf)
	{
		const double Shell = L.Nose - 10.0;
		// Bonnet: a rounded top, louvres down each side.
		const double Top = Waist - 2.0;
		B.FrontOutline(Paint, { FVector2D(-BonnetHalf, -40.0), FVector2D(BonnetHalf, -40.0), FVector2D(BonnetHalf, Top - 8.0),
			FVector2D(BonnetHalf - 8.0, Top), FVector2D(-BonnetHalf + 8.0, Top), FVector2D(-BonnetHalf, Top - 8.0) }, L.Cowl - 2.0, Shell);
		B.Bar(Chrome, FVector(L.Cowl, 0.0, Top + 0.5), FVector(Shell, 0.0, Top + 0.5), 1.5);
		for (double X = L.Cowl + 30.0; X < Shell - 25.0; X += 11.0)
		{
			B.BothSides(Guards, FVector(X, BonnetHalf, -16.0), FVector(X + 4.0, BonnetHalf + 0.6, 6.0));
		}
		// Radiator: chrome shell, dark core with upright slats, cap on top.
		B.Box(Chrome, FVector(Shell, -BonnetHalf + 4.0, -44.0), FVector(L.Nose, BonnetHalf - 4.0, Top + 4.0));
		B.Box(Guards, FVector(L.Nose, -BonnetHalf + 9.0, -38.0), FVector(L.Nose + 0.6, BonnetHalf - 9.0, Top - 2.0));
		for (double Y = -BonnetHalf + 12.0; Y < BonnetHalf - 10.0; Y += 5.0)
		{
			B.Box(Chrome, FVector(L.Nose + 0.6, Y - 0.6, -38.0), FVector(L.Nose + 1.2, Y + 0.6, Top - 2.0));
		}
		B.Cylinder(Chrome, FVector(Shell + 5.0, 0.0, Top + 7.0), 2.6, 6.0, 2, 8);
		B.Cylinder(Chrome, FVector(Shell + 5.0, 0.0, Top + 11.0), 1.2, 3.0, 2, 6);
		// Headlamps on a bar between the mudguards, with the lenses forward.
		const double LampX = L.Nose - 2.0;
		B.Bar(Chrome, FVector(LampX, -L.TrackY + 8.0, -6.0), FVector(LampX, L.TrackY - 8.0, -6.0), 3.0);
		for (const double S : { -1.0, 1.0 })
		{
			const FVector Drum(LampX + 2.0, S * (BonnetHalf + 20.0), 10.0);
			B.Bar(Chrome, Drum - FVector(0.0, 0.0, 16.0), Drum - FVector(0.0, 0.0, 6.0), 2.5);
			B.Cylinder(Chrome, Drum, 11.0, 14.0, 0, 12);
			B.Cylinder(Lamp, Drum + FVector(7.4, 0.0, 0.0), 9.5, 1.0, 0, 12);
		}
		// Bumper: two chrome blades on irons.
		const double BumperX = L.Nose + 18.0;
		B.Bar(Chrome, FVector(BumperX, -(L.HalfWidth + 8.0), -58.0), FVector(BumperX, L.HalfWidth + 8.0, -58.0), 5.0);
		B.Bar(Chrome, FVector(BumperX - 0.5, -(L.HalfWidth + 4.0), -50.0), FVector(BumperX - 0.5, L.HalfWidth + 4.0, -50.0), 3.0);
		B.BarBothSides(Guards, FVector(L.Nose - 8.0, 40.0, -54.0), FVector(BumperX, 40.0, -54.0), 3.0);
		// Starting handle under the radiator.
		B.Bar(Chrome, FVector(L.Nose, 0.0, -48.0), FVector(L.Nose + 9.0, 0.0, -48.0), 1.5);
		B.Bar(Chrome, FVector(L.Nose + 9.0, 0.0, -48.0), FVector(L.Nose + 9.0, 0.0, -56.0), 1.5);
	}

	/** Mudguards, running boards, chassis, rear bumper, tail lamps and the tail pipe. */
	inline void BuildRunningGear(FBuilder& B, const FLayout& L, double Tail)
	{
		const double R = WheelRadius + 7.0;
		const double BoardZ = -49.0;
		// Front mudguard: from in front of the wheel, over it, down onto the running board.
		TArray<FVector2D> Front;
		Arc(Front, FVector2D(L.FrontAxle, HubZ), R, 2.0, 150.0, 12);
		Front.Add(FVector2D(L.FrontAxle - 54.0, BoardZ + 3.0));
		Front.Add(FVector2D(L.FrontAxle - 66.0, BoardZ));
		B.Guard(Guards, Front, L.TrackY - 12.0, L.TrackY + 12.0, 3.0);
		// Rear mudguard.
		TArray<FVector2D> Rear;
		Rear.Add(FVector2D(L.RearAxle + 52.0, BoardZ));
		Arc(Rear, FVector2D(L.RearAxle, HubZ), R, 30.0, 178.0, 10);
		Rear.Add(FVector2D(L.RearAxle - R - 4.0, HubZ - 6.0));
		B.Guard(Guards, Rear, L.TrackY - 12.0, L.TrackY + 12.0, 3.0);
		// Running boards between them, with rubber treads.
		B.BothSides(Guards, FVector(L.RearAxle + 50.0, L.HalfWidth - 2.0, BoardZ - 2.0), FVector(L.FrontAxle - 64.0, L.TrackY + 10.0, BoardZ + 1.5));
		for (double X = L.RearAxle + 58.0; X < L.FrontAxle - 70.0; X += 12.0)
		{
			B.BothSides(Rubber, FVector(X, L.HalfWidth + 2.0, BoardZ + 1.5), FVector(X + 6.0, L.TrackY + 8.0, BoardZ + 2.2));
		}
		// Chassis rails and axles, dark under the body.
		B.Box(Guards, FVector(Tail + 4.0, -46.0, -62.0), FVector(L.Nose - 6.0, 46.0, -44.0));
		B.Bar(Guards, FVector(L.FrontAxle, -L.TrackY + 8.0, HubZ), FVector(L.FrontAxle, L.TrackY - 8.0, HubZ), 5.0);
		B.Bar(Guards, FVector(L.RearAxle, -L.TrackY + 8.0, HubZ), FVector(L.RearAxle, L.TrackY - 8.0, HubZ), 7.0);
		B.Cylinder(Guards, FVector(L.RearAxle, 0.0, HubZ), 11.0, 18.0, 1, 10);
		// Rear bumper and the red tail lamps on the mudguards.
		const double BumperX = Tail - 14.0;
		B.Bar(Chrome, FVector(BumperX, -(L.HalfWidth + 6.0), -58.0), FVector(BumperX, L.HalfWidth + 6.0, -58.0), 5.0);
		B.BarBothSides(Guards, FVector(Tail + 6.0, 40.0, -56.0), FVector(BumperX, 40.0, -56.0), 3.0);
		for (const double S : { -1.0, 1.0 })
		{
			const FVector Drum(L.RearAxle - R - 2.0, S * (L.TrackY + 2.0), HubZ + 22.0);
			B.Bar(Chrome, Drum - FVector(-4.0, 0.0, 10.0), Drum, 1.5);
			B.Cylinder(Chrome, Drum, 5.0, 6.0, 0, 10);
			B.Cylinder(TailLamp, Drum - FVector(3.2, 0.0, 0.0), 4.2, 1.0, 0, 10);
		}
		// Tail pipe out of the back, on the right.
		const FVector Pipe = L.Exhaust;
		B.Cylinder(Guards, FVector(L.RearAxle, Pipe.Y, Pipe.Z), 3.0, 30.0, 0, 8);
		B.Bar(Guards, FVector(L.RearAxle, Pipe.Y, Pipe.Z), FVector(Pipe.X + 2.0, Pipe.Y, Pipe.Z), 5.0);
		B.Cylinder(Rubber, FVector(Pipe.X + 1.0, Pipe.Y, Pipe.Z), 2.0, 2.4, 0, 8);
	}

	/** A spare wheel on the back, standing upright and facing backwards. */
	inline void BuildRearSpare(FBuilder& B, double X, double Z)
	{
		B.Ring(Rubber, FVector(X, 0.0, Z), WheelRadius - 2.0, 24.0, 12.0, 0, 16);
		B.Cylinder(Paint, FVector(X, 0.0, Z), 24.0, 9.0, 0, 16);
		B.Cylinder(Chrome, FVector(X - 5.0, 0.0, Z), 9.0, 2.0, 0, 12);
		B.Bar(Guards, FVector(X + 6.0, 0.0, Z - 18.0), FVector(X + 18.0, 0.0, Z - 18.0), 4.0);
	}

	/** The driver's seat and the steering wheel in front of it, where the driver (BuildSeatedDriver) holds it. */
	inline void BuildDriverSeat(FBuilder& B, const FLayout& L)
	{
		const double X = L.SeatX;
		const double Top = L.SeatTop;
		const double SeatHalf = L.HalfWidth - 10.0;
		B.Box(Leather, FVector(X - 12.0, -SeatHalf, FMath::Max(Top - 10.0, -43.0)), FVector(X + 20.0, SeatHalf, Top));
		B.Box(Leather, FVector(X - 16.0, -SeatHalf, Top), FVector(X - 8.0, SeatHalf, Waist + 12.0));
		// The wheel stands up in front of the chest, on a column slanting down into the dash.
		const FVector Wheel(X + 54.0, L.DriverSide, Top + 35.0);
		B.Ring(Rubber, Wheel, 19.0, 16.5, 2.5, 0, 16);
		B.Bar(Rubber, Wheel - FVector(0.0, 16.5, 0.0), Wheel + FVector(0.0, 16.5, 0.0), 1.6);
		B.Bar(Rubber, Wheel, Wheel + FVector(30.0, 0.0, -26.0), 3.0);
	}

	/**
	 * A closed body: the lower body to the waist, then a glass house with
	 * pillars and a roof. Pillars lists the X of the door pillars between
	 * the windscreen and the back.
	 */
	inline void BuildClosedBody(FBuilder& B, const FLayout& L, const TArray<FVector2D>& Lower, double GlassBack, double RoofBack,
		const TArray<double>& Pillars, bool bRoundedRoof = false)
	{
		const double W = L.HalfWidth;
		const double GlassTop = L.RoofTop - 10.0;
		B.SideOutline(Paint, Lower, -W, W);
		// The glass house, slightly narrower than the body.
		const double Screen = L.Cowl - 2.0;
		const double ScreenTop = Screen - 14.0;
		// Its foot and top tuck into the body and the roof, so no glass face lies on
		// a painted one (they would flicker against each other).
		B.SideOutline(Glass, { FVector2D(Screen + 0.5, Waist - 2.0), FVector2D(ScreenTop - 0.5, GlassTop + 2.0), FVector2D(RoofBack + 4.0, GlassTop + 2.0),
			FVector2D(GlassBack, Waist - 2.0) }, -(W - 5.0), W - 5.0);
		if (bRoundedRoof)
		{
			// Rounded over all round, with a little peak over the windscreen.
			RoundedTop(B, Paint, RoofBack - 2.0, ScreenTop + 6.0, W - 2.0, GlassTop - 1.0, L.RoofTop, 8.0, 16.0);
		}
		else
		{
			// Roof, gently rounded at the back.
			B.SideOutline(Paint, { FVector2D(ScreenTop + 3.0, GlassTop - 1.0), FVector2D(ScreenTop - 1.0, L.RoofTop - 2.0), FVector2D(ScreenTop - 20.0, L.RoofTop),
				FVector2D(RoofBack + 10.0, L.RoofTop), FVector2D(RoofBack - 4.0, L.RoofTop - 5.0), FVector2D(RoofBack, GlassTop - 1.0) }, -(W - 2.0), W - 2.0);
		}
		// Pillars: the windscreen's, each door's, and broad ones round the small back window.
		B.BarBothSides(Paint, FVector(Screen, W - 4.0, Waist), FVector(ScreenTop, W - 4.0, GlassTop), 5.0);
		B.Bar(Chrome, FVector(Screen + 1.2, 0.0, Waist), FVector(ScreenTop + 1.2, 0.0, GlassTop), 1.5);
		for (const double X : Pillars)
		{
			B.BarBothSides(Paint, FVector(X, W - 4.0, Waist), FVector(X, W - 4.0, GlassTop), 6.0);
		}
		for (const double Y : { W - 5.0, W - 14.0, W - 23.0 })
		{
			B.BarBothSides(Paint, FVector(GlassBack, Y, Waist), FVector(RoofBack + 4.0, Y, GlassTop), 11.0);
		}
		B.Bar(Paint, FVector(GlassBack + 0.5, -W + 4.0, Waist + 2.0), FVector(GlassBack + 0.5, W - 4.0, Waist + 2.0), 6.0);
		// Chrome along the waist, door seams and handles below it.
		B.BarBothSides(Chrome, FVector(L.Cowl, W + 0.6, Waist - 1.0), FVector(Lower.Num() > 2 ? Lower[2].X : GlassBack, W + 0.6, Waist - 1.0), 1.6);
		for (const double X : Pillars)
		{
			B.BothSides(Guards, FVector(X - 0.6, W, -38.0), FVector(X + 0.6, W + 0.4, Waist - 2.0));
			B.BothSides(Chrome, FVector(X + 4.0, W, Waist - 9.0), FVector(X + 11.0, W + 1.4, Waist - 7.0));
		}
		B.BothSides(Guards, FVector(Screen - 0.6, W, -38.0), FVector(Screen + 0.6, W + 0.4, Waist - 2.0));
		// Inside: the driver's seat and wheel.
		BuildDriverSeat(B, L);
	}

	/** The taxi's chequered band below the waist (from BandBack to the windscreen), the roof lamp and the meter flag by the driver. */
	inline void BuildTaxiMarks(FBuilder& B, const FLayout& L, double BandBack)
	{
		for (const double S : { -1.0, 1.0 })
		{
			const double Y = S * (L.HalfWidth + 0.3);
			B.Box(Accent, FVector(BandBack, Y - 0.3, 2.0), FVector(L.Cowl - 4.0, Y + 0.3, 14.0));
			int32 Index = 0;
			for (double X = BandBack; X + 11.0 <= L.Cowl - 4.0; X += 11.0, ++Index)
			{
				const double Z = Index % 2 == 0 ? 8.0 : 2.0;
				B.Box(Rubber, FVector(X, Y - 0.5, Z), FVector(X + 11.0, Y + 0.5, Z + 6.0));
			}
		}
		B.Box(Accent, FVector(-30.0, -24.0, L.RoofTop - 1.0), FVector(-14.0, 24.0, L.RoofTop + 11.0));
		B.Bar(Chrome, FVector(L.Cowl + 6.0, -44.0, Waist - 4.0), FVector(L.Cowl + 6.0, -44.0, Waist + 22.0), 2.0);
		B.Box(TailLamp, FVector(L.Cowl + 5.0, -52.0, Waist + 18.0), FVector(L.Cowl + 7.0, -44.0, Waist + 26.0));
	}

	inline void BuildSaloon(FBuilder& B, const FLayout& L, const FStyle& Style)
	{
		BuildFront(B, L, 38.0);
		const double Back = L.Tail;
		BuildClosedBody(B, L, { FVector2D(L.Cowl, -44.0), FVector2D(L.Cowl, Waist), FVector2D(Back + 18.0, Waist), FVector2D(Back + 4.0, Waist - 14.0),
			FVector2D(Back, Waist - 42.0), FVector2D(Back + 6.0, -44.0) }, Back + 20.0, Back + 46.0, { -45.0, -112.0 });
		B.Box(Leather, FVector(-128.0, -62.0, Waist - 4.0), FVector(-120.0, 62.0, Waist + 16.0));
		BuildRunningGear(B, L, Back);
		BuildRearSpare(B, Back - 9.0, Waist - 32.0);
		if (Style.bTaxi)
		{
			BuildTaxiMarks(B, L, Back + 14.0);
		}
		if (Style.bPolice)
		{
			// A blue glass lamp on a stalk by the windscreen, right of the driver.
			const FVector Blue(L.Cowl + 4.0, 44.0, Waist + 14.0);
			B.Bar(Chrome, FVector(L.Cowl + 2.0, 44.0, Waist - 3.0), Blue - FVector(0.0, 0.0, 4.0), 2.0);
			B.Cylinder(Chrome, Blue, 6.5, 8.0, 0, 12);
			B.Cylinder(Accent, Blue + FVector(4.2, 0.0, 0.0), 5.5, 1.0, 0, 12);
			B.Ball(Accent, Blue + FVector(0.0, 0.0, 6.0), 3.0, 1.0);
		}
	}

	inline void BuildCoupe(FBuilder& B, const FLayout& L)
	{
		BuildFront(B, L, 35.0);
		const double Back = L.Tail;
		// Short glass house for two, then a sloping boot.
		BuildClosedBody(B, L, { FVector2D(L.Cowl, -44.0), FVector2D(L.Cowl, Waist), FVector2D(-92.0, Waist), FVector2D(-140.0, Waist - 8.0),
			FVector2D(Back + 4.0, Waist - 30.0), FVector2D(Back, Waist - 50.0), FVector2D(Back + 6.0, -44.0) }, -92.0, -72.0, { });
		// Boot lid seam and handle.
		B.Bar(Guards, FVector(-112.0, -40.0, Waist - 2.0), FVector(-112.0, 40.0, Waist - 2.0), 1.0);
		B.Bar(Chrome, FVector(Back + 1.0, -8.0, Waist - 36.0), FVector(Back + 1.0, 8.0, Waist - 36.0), 2.0);
		BuildRunningGear(B, L, Back);
		BuildRearSpare(B, Back - 6.0, Waist - 46.0);
	}

	inline void BuildLimousine(FBuilder& B, const FLayout& L, const FStyle& Style)
	{
		BuildFront(B, L, 40.0);
		const double Back = L.Tail;
		BuildClosedBody(B, L, { FVector2D(L.Cowl, -44.0), FVector2D(L.Cowl, Waist), FVector2D(Back + 14.0, Waist), FVector2D(Back + 2.0, Waist - 12.0),
			FVector2D(Back, Waist - 44.0), FVector2D(Back + 4.0, -44.0) }, Back + 16.0, Back + 40.0, { -36.0, -98.0 });
		B.Box(Leather, FVector(-128.0, -62.0, Waist - 4.0), FVector(-120.0, 62.0, Waist + 18.0));
		// A trunk on a rack at the back.
		B.Box(Guards, FVector(Back - 28.0, -60.0, Waist - 46.0), FVector(Back - 2.0, 60.0, Waist - 4.0));
		B.Box(Chrome, FVector(Back - 29.0, -61.0, Waist - 26.0), FVector(Back - 1.0, 61.0, Waist - 24.0));
		B.Box(Guards, FVector(Back - 30.0, -58.0, Waist - 48.0), FVector(Back, 58.0, Waist - 46.0));
		BuildRunningGear(B, L, Back - 24.0);
		// Spare wheels in the front mudguards, behind the front wheels.
		for (const double S : { -1.0, 1.0 })
		{
			const FVector At(L.FrontAxle - 74.0, S * (WheelY + 6.0), -24.0);
			B.Ring(Rubber, At, WheelRadius - 2.0, 24.0, 12.0, 1, 16);
			B.Cylinder(Paint, At, 24.0, 9.0, 1, 16);
			B.Cylinder(Chrome, At + FVector(0.0, S * 5.0, 0.0), 9.0, 2.0, 1, 12);
		}
		if (Style.bTaxi)
		{
			BuildTaxiMarks(B, L, Back + 10.0);
		}
	}

	inline void BuildVan(FBuilder& B, const FLayout& L)
	{
		BuildFront(B, L, 38.0);
		const double CabBack = -56.0;
		// The cab, with the driver.
		BuildClosedBody(B, L, { FVector2D(L.Cowl, -44.0), FVector2D(L.Cowl, Waist), FVector2D(CabBack, Waist), FVector2D(CabBack, -44.0) },
			CabBack + 2.0, CabBack + 2.0, { });
		// The box behind: tall, with a sign panel down each side and doors at the back.
		// Below the mudguards' tops it is narrower, between the rear wheels, so
		// the wheels and mudguards stand clear under the full-width box.
		const double BoxSill = HubZ + WheelRadius + 13.0;
		B.Box(Paint, FVector(L.Tail, -(WheelY - 12.0), -44.0), FVector(CabBack, WheelY - 12.0, BoxSill));
		B.Box(Paint, FVector(L.Tail, -78.0, BoxSill), FVector(CabBack, 78.0, L.RoofTop));
		B.Box(Paint, FVector(L.Tail - 2.0, -80.0, L.RoofTop), FVector(CabBack + 4.0, 80.0, L.RoofTop + 4.0));
		B.BothSides(Accent, FVector(L.Tail + 14.0, 78.0, BoxSill + 8.0), FVector(CabBack - 14.0, 78.6, L.RoofTop - 14.0));
		B.BothSides(Paint, FVector(L.Tail + 18.0, 78.4, BoxSill + 12.0), FVector(CabBack - 18.0, 78.8, BoxSill + 16.0));
		B.Box(Guards, FVector(L.Tail - 0.6, -0.6, -40.0), FVector(L.Tail, 0.6, L.RoofTop - 4.0));
		B.Box(Chrome, FVector(L.Tail - 1.6, 3.0, 14.0), FVector(L.Tail, 10.0, 17.0));
		for (const double Z : { BoxSill + 6.0, L.RoofTop - 18.0 })
		{
			B.Box(Chrome, FVector(L.Tail - 1.4, -70.0, Z), FVector(L.Tail, -60.0, Z + 6.0));
			B.Box(Chrome, FVector(L.Tail - 1.4, 60.0, Z), FVector(L.Tail, 70.0, Z + 6.0));
		}
		BuildRunningGear(B, L, L.Tail);
	}

	/** Over the rear wheels' mudguards, where a small car's body sides stop (cm, car frame). */
	inline double SmallGuardTop()
	{
		return HubZ + WheelRadius + 9.0;
	}

	/**
	 * A narrow small car's body sides from the mudguards' tops up to the
	 * waist, from the windscreen to the tail (the waist runs back to
	 * TailTop, then rounds down): BuildClosedBody's outline.
	 */
	inline TArray<FVector2D> SmallBodyUpper(const FLayout& L, double TailTop)
	{
		return { FVector2D(L.Cowl, SmallGuardTop()), FVector2D(L.Cowl, Waist), FVector2D(TailTop, Waist), FVector2D(L.Tail + 2.0, Waist - 12.0),
			FVector2D(L.Tail, SmallGuardTop()) };
	}

	/**
	 * Below that, in two pieces so the rear wheels stay clear: the body down
	 * to the floor ahead of the rear wheels, and down again behind them.
	 */
	inline void BuildSmallBodyLower(FBuilder& B, const FLayout& L)
	{
		const double W = L.HalfWidth;
		const double ArchFront = L.RearAxle + WheelRadius + 8.0;
		const double ArchBack = L.RearAxle - WheelRadius - 8.0;
		const double GuardTop = SmallGuardTop();
		B.SideOutline(Paint, { FVector2D(L.Cowl, -44.0), FVector2D(L.Cowl, GuardTop), FVector2D(ArchFront, GuardTop), FVector2D(ArchFront, -44.0) }, -W, W);
		if (ArchBack > L.Tail + 4.0)
		{
			B.SideOutline(Paint, { FVector2D(ArchBack, -40.0), FVector2D(ArchBack, GuardTop), FVector2D(L.Tail, GuardTop), FVector2D(L.Tail + 3.0, -40.0) }, -W, W);
		}
		// The floor between the rear wheel arches, dark.
		B.Box(Guards, FVector(ArchBack, -W + 14.0, -44.0), FVector(ArchFront, W - 14.0, GuardTop));
	}

	/**
	 * A radiator's front, seen from ahead (Y, Z): HalfW wide, straight sides
	 * up to Shoulder, an arched top up to Crown, and a small V at the foot.
	 */
	inline TArray<FVector2D> ArchedRadiator(double HalfW, double Foot, double Shoulder, double Crown, double Vee)
	{
		TArray<FVector2D> Out;
		Out.Add(FVector2D(0.0, Foot - Vee));
		Out.Add(FVector2D(HalfW * 0.85, Foot));
		for (int32 i = 0; i <= 8; ++i)
		{
			const double A = PI * i / 8.0;
			Out.Add(FVector2D(HalfW * FMath::Cos(A), Shoulder + (Crown - Shoulder) * FMath::Sin(A)));
		}
		Out.Add(FVector2D(-HalfW * 0.85, Foot));
		return Out;
	}

	/**
	 * The Balilla's front: a tall narrow radiator with an arched top in a
	 * chrome shell, fine upright slats and the badge slanting across them,
	 * big chrome headlamp bowls beside it, a narrow bonnet with a panel of
	 * louvres and a dark moulding, and one curved bumper blade well ahead.
	 */
	inline void BuildBalillaFront(FBuilder& B, const FLayout& L)
	{
		const double BonnetHalf = 27.0;
		const double Top = Waist - 2.0;
		const double Shell = L.Nose - 8.0;
		B.FrontOutline(Paint, { FVector2D(-BonnetHalf, -40.0), FVector2D(BonnetHalf, -40.0), FVector2D(BonnetHalf, Top - 6.0),
			FVector2D(BonnetHalf - 6.0, Top), FVector2D(-BonnetHalf + 6.0, Top), FVector2D(-BonnetHalf, Top - 6.0) }, L.Cowl - 2.0, Shell + 1.0);
		B.Bar(Chrome, FVector(L.Cowl, 0.0, Top + 0.4), FVector(Shell, 0.0, Top + 0.4), 1.2);
		// The moulding along the bonnet's sides, and the panel of fine louvres.
		B.BothSides(Guards, FVector(L.Cowl - 2.0, BonnetHalf, Top - 6.5), FVector(Shell, BonnetHalf + 0.4, Top - 5.5));
		for (double X = L.Cowl + 24.0; X < Shell - 10.0; X += 4.0)
		{
			B.BothSides(Guards, FVector(X, BonnetHalf, -26.0), FVector(X + 1.6, BonnetHalf + 0.6, 4.0));
		}
		// Radiator: the chrome shell, the dark core inside its edge, slats, badge and cap.
		const double Crown = Top + 6.0;
		B.FrontOutline(Chrome, ArchedRadiator(24.0, -36.0, 8.0, Crown, 4.0), Shell, L.Nose);
		B.FrontOutline(Guards, ArchedRadiator(20.5, -32.0, 8.0, Crown - 3.5, 3.0), L.Nose, L.Nose + 0.6);
		for (double Y = -17.5; Y <= 17.6; Y += 2.5)
		{
			const double SlatTop = 8.0 + (Crown - 3.5 - 8.0) * FMath::Sqrt(FMath::Max(0.0, 1.0 - FMath::Square(Y / 20.5))) - 1.5;
			B.Box(Chrome, FVector(L.Nose + 0.6, Y - 0.35, -29.0), FVector(L.Nose + 1.0, Y + 0.35, SlatTop));
		}
		B.Bar(Chrome, FVector(L.Nose + 1.4, -11.0, -5.0), FVector(L.Nose + 1.4, 11.0, 1.0), 2.0);
		B.Cylinder(Chrome, FVector(Shell + 3.0, 0.0, Crown + 2.0), 2.4, 4.0, 2, 8);
		B.Cylinder(Chrome, FVector(Shell + 3.0, 0.0, Crown + 5.0), 1.0, 3.0, 2, 6);
		// The dark apron under the radiator.
		B.Box(Guards, FVector(L.Nose - 14.0, -BonnetHalf + 2.0, -48.0), FVector(L.Nose - 2.0, BonnetHalf - 2.0, -36.0));
		// Headlamps: big chrome bowls on stalks either side of the radiator, on a tie bar.
		B.Bar(Chrome, FVector(L.Nose - 3.0, -44.0, -16.0), FVector(L.Nose - 3.0, 44.0, -16.0), 2.4);
		for (const double S : { -1.0, 1.0 })
		{
			const FVector Bowl(L.Nose - 1.0, S * 42.0, 2.0);
			B.Bar(Chrome, FVector(L.Nose - 3.0, S * 42.0, -16.0), Bowl - FVector(2.0, 0.0, 8.0), 2.2);
			B.Ball(Chrome, Bowl - FVector(3.0, 0.0, 0.0), 10.0, 1.0);
			B.Cylinder(Chrome, Bowl + FVector(2.0, 0.0, 0.0), 10.5, 3.0, 0, 14);
			B.Cylinder(Lamp, Bowl + FVector(3.8, 0.0, 0.0), 9.2, 1.0, 0, 14);
		}
		// Bumper: one blade, its ends curving back, on two irons.
		const double BumperX = L.FrontAxle + 46.0;
		const double BumperHalf = L.HalfWidth - 6.0;
		B.Bar(Chrome, FVector(BumperX, -BumperHalf, -60.0), FVector(BumperX, BumperHalf, -60.0), 5.0);
		B.BarBothSides(Chrome, FVector(BumperX, BumperHalf, -60.0), FVector(BumperX - 6.0, BumperHalf + 12.0, -60.0), 5.0);
		B.BarBothSides(Guards, FVector(L.Nose - 10.0, 26.0, -55.0), FVector(BumperX, 26.0, -55.0), 3.0);
		// Starting handle under the radiator.
		B.Bar(Chrome, FVector(L.Nose, 0.0, -40.0), FVector(L.Nose + 8.0, 0.0, -40.0), 1.5);
	}

	/**
	 * The Fiat 508 Balilla two-door berlina of 1932: small, narrow and tall.
	 * The radiator over the front axle, a narrow louvred bonnet, upright
	 * windscreen, a long door and a quarter window each side under a roof
	 * rounded all round, an upright back with a small window, a black trunk
	 * with leather straps on a platform, twin lamps on each rear wing, disc
	 * wheels, and the spare wheel in the left front wing before the door.
	 */
	inline void BuildBalilla(FBuilder& B, const FLayout& L)
	{
		BuildBalillaFront(B, L);
		const double Back = L.Tail;
		const double W = L.HalfWidth;
		BuildClosedBody(B, L, SmallBodyUpper(L, Back + 8.0), Back + 12.0, Back + 14.0, { -72.0 }, true);
		BuildSmallBodyLower(B, L);
		// The back window is small: paint below and above it.
		const double GlassTop = L.RoofTop - 10.0;
		B.Bar(Paint, FVector(Back + 12.5, -(W - 4.0), Waist + 7.0), FVector(Back + 12.5, W - 4.0, Waist + 7.0), 12.0);
		B.Bar(Paint, FVector(Back + 17.0, -(W - 4.0), GlassTop - 3.0), FVector(Back + 17.0, W - 4.0, GlassTop - 3.0), 6.0);
		// The dark moulding along the body, carrying on from the bonnet's.
		B.BothSides(Guards, FVector(Back + 6.0, W, Waist - 6.5), FVector(L.Cowl, W + 0.4, Waist - 5.5));
		B.Box(Leather, FVector(Back + 34.0, -50.0, Waist - 4.0), FVector(Back + 42.0, 50.0, Waist + 12.0));
		BuildRunningGear(B, L, Back - 16.0);
		// An amber lamp over each red one on the rear wings.
		for (const double S : { -1.0, 1.0 })
		{
			const FVector Drum(L.RearAxle - WheelRadius - 9.0, S * (L.TrackY + 2.0), HubZ + 31.0);
			B.Cylinder(Chrome, Drum, 5.0, 6.0, 0, 10);
			B.Cylinder(Accent, Drum - FVector(3.2, 0.0, 0.0), 4.2, 1.0, 0, 10);
		}
		// The trunk: black, rounded at the top, two leather straps and a chrome latch,
		// on a platform, with the body's apron below.
		const double TrunkFront = Back - 1.0;
		const double TrunkBack = Back - 27.0;
		B.SideOutline(Guards, { FVector2D(TrunkFront, -46.0), FVector2D(TrunkFront, 10.0), FVector2D(TrunkFront - 4.0, 14.0), FVector2D(TrunkBack + 4.0, 14.0),
			FVector2D(TrunkBack, 10.0), FVector2D(TrunkBack, -46.0) }, -44.0, 44.0);
		for (const double Y : { -28.0, 28.0 })
		{
			B.Box(Leather, FVector(TrunkBack - 0.6, Y - 2.5, -45.0), FVector(TrunkBack + 0.4, Y + 2.5, 10.0));
			B.Box(Leather, FVector(TrunkBack + 2.0, Y - 2.5, 13.6), FVector(TrunkFront - 2.0, Y + 2.5, 14.6));
			B.Box(Chrome, FVector(TrunkBack - 1.2, Y - 3.0, -16.0), FVector(TrunkBack - 0.4, Y + 3.0, -12.0));
		}
		B.Box(Chrome, FVector(TrunkBack - 1.4, -3.0, -2.0), FVector(TrunkBack - 0.4, 3.0, 8.0));
		B.Box(Guards, FVector(TrunkBack - 2.0, -46.0, -50.0), FVector(Back, 46.0, -46.0));
		B.Box(Paint, FVector(TrunkBack + 2.0, -(W - 10.0), -58.0), FVector(Back + 2.0, W - 10.0, -50.0));
		// The spare wheel standing in the left front wing, before the door.
		const FVector Spare(L.Cowl + 22.0, -(L.TrackY + 6.0), -14.0);
		B.Ring(Rubber, Spare, WheelRadius - 3.0, 22.0, 11.0, 1, 18);
		B.Cylinder(Paint, Spare, 22.5, 7.0, 1, 18);
		B.Cylinder(Chrome, Spare + FVector(0.0, -4.5, 0.0), 9.0, 2.0, 1, 12);
		B.Box(Guards, FVector(Spare.X - 3.0, -(L.TrackY + 1.0), Spare.Z - 2.0), FVector(Spare.X + 3.0, -(W - 1.0), Spare.Z + 2.0));
	}

	/**
	 * The Opel 1.2 Liter two-door Limousine of 1932: a narrow upright box.
	 * A flat chrome radiator, a sun visor over the windscreen, a door and a
	 * quarter window each side, a flat back with the spare wheel, wire wheels.
	 */
	inline void BuildOpel(FBuilder& B, const FLayout& L)
	{
		BuildFront(B, L, 30.0);
		const double Back = L.Tail;
		BuildClosedBody(B, L, SmallBodyUpper(L, Back + 3.0), Back + 6.0, Back + 8.0, { -42.0 });
		BuildSmallBodyLower(B, L);
		// The sun visor over the windscreen.
		const double VisorX = L.Cowl - 16.0;
		B.Box(Paint, FVector(VisorX - 4.0, -(L.HalfWidth - 3.0), L.RoofTop - 10.0), FVector(VisorX + 13.0, L.HalfWidth - 3.0, L.RoofTop - 8.0));
		B.Box(Leather, FVector(Back + 30.0, -52.0, Waist - 4.0), FVector(Back + 38.0, 52.0, Waist + 14.0));
		BuildRunningGear(B, L, Back);
		BuildRearSpare(B, Back - 9.0, Waist - 34.0);
	}

	/** The Steyr's front: a tall narrow radiator, the bonnet as wide, big headlamps close by it, a double bumper with overriders. */
	inline void BuildSteyrFront(FBuilder& B, const FLayout& L)
	{
		const double BonnetHalf = 33.0;
		const double Shell = L.Nose - 10.0;
		const double Top = Waist - 1.0;
		B.FrontOutline(Paint, { FVector2D(-BonnetHalf, -40.0), FVector2D(BonnetHalf, -40.0), FVector2D(BonnetHalf, Top - 8.0),
			FVector2D(BonnetHalf - 8.0, Top), FVector2D(-BonnetHalf + 8.0, Top), FVector2D(-BonnetHalf, Top - 8.0) }, L.Cowl - 2.0, Shell);
		B.Bar(Chrome, FVector(L.Cowl, 0.0, Top + 0.5), FVector(Shell, 0.0, Top + 0.5), 1.5);
		for (double X = L.Cowl + 34.0; X < Shell - 20.0; X += 6.0)
		{
			B.BothSides(Guards, FVector(X, BonnetHalf, -20.0), FVector(X + 2.0, BonnetHalf + 0.6, 4.0));
		}
		// Radiator: a tall chrome shell, the dark core with close upright slats, the round badge on top, the cap and its mascot.
		const double Half = BonnetHalf - 1.0;
		B.Box(Chrome, FVector(Shell, -Half, -50.0), FVector(L.Nose, Half, Top + 5.0));
		B.Box(Guards, FVector(L.Nose, -Half + 5.0, -44.0), FVector(L.Nose + 0.6, Half - 5.0, Top - 1.0));
		for (double Y = -Half + 7.0; Y < Half - 6.0; Y += 3.5)
		{
			B.Box(Chrome, FVector(L.Nose + 0.6, Y - 0.6, -44.0), FVector(L.Nose + 1.2, Y + 0.6, Top - 1.0));
		}
		B.Cylinder(Chrome, FVector(L.Nose + 1.6, 0.0, Top - 7.0), 5.5, 1.6, 0, 14);
		B.Cylinder(Guards, FVector(L.Nose + 2.5, 0.0, Top - 7.0), 3.6, 0.6, 0, 14);
		B.Cylinder(Chrome, FVector(Shell + 5.0, 0.0, Top + 8.0), 2.6, 6.0, 2, 8);
		B.Bar(Chrome, FVector(Shell + 5.0, 0.0, Top + 10.0), FVector(Shell + 6.0, 0.0, Top + 19.0), 2.0);
		B.Ball(Chrome, FVector(Shell + 6.0, 0.0, Top + 21.0), 2.6, 1.3);
		// Big headlamps on a bar just outside the radiator.
		const double LampX = L.Nose - 2.0;
		B.Bar(Chrome, FVector(LampX, -L.TrackY + 8.0, -12.0), FVector(LampX, L.TrackY - 8.0, -12.0), 3.0);
		for (const double S : { -1.0, 1.0 })
		{
			const FVector Drum(LampX + 3.0, S * (Half + 17.0), 4.0);
			B.Bar(Chrome, Drum - FVector(0.0, 0.0, 16.0), Drum - FVector(0.0, 0.0, 8.0), 2.5);
			B.Ball(Chrome, Drum - FVector(4.0, 0.0, 0.0), 13.0, 1.0);
			B.Cylinder(Chrome, Drum + FVector(3.0, 0.0, 0.0), 13.5, 3.0, 0, 16);
			B.Cylinder(Lamp, Drum + FVector(4.8, 0.0, 0.0), 12.0, 1.0, 0, 16);
		}
		// The double bumper on its irons, with two overriders.
		const double BumperX = L.Nose + 20.0;
		B.Bar(Chrome, FVector(BumperX, -(L.HalfWidth + 6.0), -60.0), FVector(BumperX, L.HalfWidth + 6.0, -60.0), 4.0);
		B.Bar(Chrome, FVector(BumperX, -(L.HalfWidth + 6.0), -51.0), FVector(BumperX, L.HalfWidth + 6.0, -51.0), 4.0);
		B.BarBothSides(Chrome, FVector(BumperX + 1.0, 32.0, -66.0), FVector(BumperX + 1.0, 32.0, -44.0), 3.4);
		B.BarBothSides(Guards, FVector(L.Nose - 8.0, 40.0, -56.0), FVector(BumperX, 40.0, -56.0), 3.0);
	}

	/**
	 * Dr. Halász's Steyr (the XII / XXX saloon of about 1930): a tall upright
	 * radiator with a round badge and a mascot on the cap, big headlamps, a
	 * long louvred bonnet, a spare wheel standing in each front wing, four
	 * doors under a roof rounded all round, a black belt below the windows,
	 * a sun visor, a spotlight by the windscreen, a double bumper with
	 * overriders, and a brown trunk on a braced rack at the back.
	 */
	inline void BuildSteyr(FBuilder& B, const FLayout& L)
	{
		BuildSteyrFront(B, L);
		const double Back = L.Tail;
		const double W = L.HalfWidth;
		BuildClosedBody(B, L, { FVector2D(L.Cowl, -44.0), FVector2D(L.Cowl, Waist), FVector2D(Back + 12.0, Waist), FVector2D(Back + 2.0, Waist - 12.0),
			FVector2D(Back, Waist - 44.0), FVector2D(Back + 4.0, -44.0) }, Back + 10.0, Back + 14.0, { -6.0, -75.0 }, true);
		// The small back window: paint below and above it; the broad panel behind the
		// quarter windows.
		const double GlassTop = L.RoofTop - 10.0;
		const double QuarterBack = -139.0;
		B.BothSides(Paint, FVector(Back + 2.0, W - 6.0, Waist - 1.0), FVector(QuarterBack, W, GlassTop + 1.0));
		B.Bar(Paint, FVector(Back + 10.5, -(W - 4.0), Waist + 9.0), FVector(Back + 10.5, W - 4.0, Waist + 9.0), 16.0);
		B.Bar(Paint, FVector(Back + 14.0, -(W - 4.0), GlassTop - 4.0), FVector(Back + 14.0, W - 4.0, GlassTop - 4.0), 8.0);
		B.BothSides(Paint, FVector(Back + 8.0, W - 42.0, Waist), FVector(Back + 14.0, W - 20.0, GlassTop));
		// The black belt under the windows, from the back to the windscreen.
		B.BothSides(Guards, FVector(Back + 4.0, W, Waist - 7.0), FVector(L.Cowl, W + 0.6, Waist - 1.0));
		B.Box(Guards, FVector(Back - 0.6, -(W - 6.0), Waist - 7.0), FVector(Back + 4.0, W - 6.0, Waist - 1.0));
		// The sun visor over the windscreen, and the spotlight on the right pillar.
		const double VisorX = L.Cowl - 16.0;
		B.Box(Paint, FVector(VisorX - 2.0, -(W - 2.0), L.RoofTop - 11.0), FVector(VisorX + 16.0, W - 2.0, L.RoofTop - 9.0));
		const FVector Spot(L.Cowl + 4.0, W + 6.0, Waist + 16.0);
		B.Bar(Chrome, FVector(L.Cowl - 1.0, W - 2.0, Waist + 10.0), Spot - FVector(2.0, 0.0, 0.0), 1.6);
		B.Cylinder(Chrome, Spot, 6.5, 9.0, 0, 12);
		B.Cylinder(Lamp, Spot + FVector(4.8, 0.0, 0.0), 5.5, 1.0, 0, 12);
		// Seat backs inside.
		B.Box(Leather, FVector(Back + 40.0, -(W - 10.0), Waist - 4.0), FVector(Back + 48.0, W - 10.0, Waist + 18.0));
		B.Box(Leather, FVector(L.SeatX - 76.0, -(W - 10.0), L.SeatTop), FVector(L.SeatX - 70.0, W - 10.0, Waist + 12.0));
		// The trunk on its rack, braced up to the body's sides.
		const double TrunkFront = Back - 3.0;
		const double TrunkBack = Back - 33.0;
		B.SideOutline(Leather, { FVector2D(TrunkFront, -50.0), FVector2D(TrunkFront, 8.0), FVector2D(TrunkFront - 3.0, 12.0), FVector2D(TrunkBack + 3.0, 12.0),
			FVector2D(TrunkBack, 8.0), FVector2D(TrunkBack, -50.0) }, -(W - 6.0), W - 6.0);
		B.Box(Guards, FVector(TrunkBack - 0.6, -(W - 5.0), -22.0), FVector(TrunkFront + 0.6, W - 5.0, -19.0));
		for (const double Y : { -34.0, 34.0 })
		{
			B.Box(Chrome, FVector(TrunkBack - 1.2, Y - 4.0, -6.0), FVector(TrunkBack - 0.2, Y + 4.0, 0.0));
		}
		B.Box(Guards, FVector(TrunkBack - 2.0, -(W - 4.0), -54.0), FVector(Back, W - 4.0, -50.0));
		B.BarBothSides(Chrome, FVector(TrunkBack + 2.0, W - 5.0, -52.0), FVector(Back + 2.0, W - 2.0, Waist - 20.0), 1.4);
		BuildRunningGear(B, L, Back - 28.0);
		// The spare wheels, standing in the front wings before the doors.
		for (const double S : { -1.0, 1.0 })
		{
			const FVector At(L.Cowl + 38.0, S * (L.TrackY + 6.0), -26.0);
			B.Ring(Rubber, At, WheelRadius - 1.0, 24.0, 13.0, 1, 18);
			B.Cylinder(Paint, At, 24.5, 8.0, 1, 18);
			B.Cylinder(Guards, At + FVector(0.0, S * 1.0, 0.0), 19.0, 8.4, 1, 18);
			B.Cylinder(Chrome, At + FVector(0.0, S * 5.0, 0.0), 9.0, 2.0, 1, 12);
			B.Box(Guards, FVector(At.X - 3.0, S > 0.0 ? W - 1.0 : -(L.TrackY + 1.0), At.Z - 2.0), FVector(At.X + 3.0, S > 0.0 ? L.TrackY + 1.0 : -(W - 1.0), At.Z + 2.0));
		}
	}

	/**
	 * A smooth skin through rings of points: Rings[i][j] is point j of ring
	 * i, every ring has the same count and runs round the same way. Normals
	 * are smoothed over the neighbours, so a glossy body shades softly; a
	 * ring shrunk to one point closes that end.
	 */
	inline void Loft(FBuilder& B, EPart Part, const TArray<TArray<FVector>>& Rings)
	{
		const int32 NumRings = Rings.Num();
		if (NumRings < 2)
		{
			return;
		}
		const int32 N = Rings[0].Num();
		TArray<FVector> Mids;
		for (const TArray<FVector>& R : Rings)
		{
			FVector Sum = FVector::ZeroVector;
			for (const FVector& P : R)
			{
				Sum += P;
			}
			Mids.Add(Sum / static_cast<double>(FMath::Max(N, 1)));
		}
		auto NormalAt = [&](int32 i, int32 j)
		{
			const FVector& P = Rings[i][j];
			const FVector Round = Rings[i][(j + 1) % N] - Rings[i][(j + N - 1) % N];
			const FVector Along = Rings[FMath::Min(i + 1, NumRings - 1)][j] - Rings[FMath::Max(i - 1, 0)][j];
			FVector Out = FVector::CrossProduct(Round, Along).GetSafeNormal();
			const FVector Away = P - Mids[i];
			if (Away.SizeSquared() < 0.01 || Out.IsNearlyZero())
			{
				// A closed tip: face straight out along the loft.
				const FVector Tip = (Mids[i] - Mids[i == 0 ? 1 : i - 1]).GetSafeNormal();
				return i == 0 ? -Tip : Tip;
			}
			return FVector::DotProduct(Out, Away) < 0.0 ? -Out : Out;
		};
		for (int32 i = 0; i + 1 < NumRings; ++i)
		{
			for (int32 j = 0; j < N; ++j)
			{
				const int32 k = (j + 1) % N;
				B.SmoothQuad(Part, Rings[i][j], Rings[i][k], Rings[i + 1][k], Rings[i + 1][j],
					NormalAt(i, j), NormalAt(i, k), NormalAt(i + 1, k), NormalAt(i + 1, j));
			}
		}
	}

	/**
	 * One rounded cross-section ring of Count points round Centre, in the
	 * plane of Side (across) and Up: HalfW to each side, Above over the
	 * centre and Below under it. Square 2 is an ellipse; more is boxier.
	 */
	inline TArray<FVector> RoundRing(const FVector& Centre, const FVector& Side, const FVector& Up, double HalfW, double Above, double Below, double Square, int32 Count = 16)
	{
		TArray<FVector> Out;
		const double Power = 2.0 / Square;
		for (int32 j = 0; j < Count; ++j)
		{
			const double A = 2.0 * PI * j / Count;
			const double C = FMath::Cos(A);
			const double S = FMath::Sin(A);
			const double U = (C < 0.0 ? -1.0 : 1.0) * FMath::Pow(FMath::Abs(C), Power) * HalfW;
			const double V = (S < 0.0 ? -1.0 : 1.0) * FMath::Pow(FMath::Abs(S), Power) * (S > 0.0 ? Above : Below);
			Out.Add(Centre + Side * U + Up * V);
		}
		return Out;
	}

	/**
	 * The black Mercedes-Benz SSK of the car show, after the one-off
	 * "Black Prince" roadster: a pointed (vee) radiator behind the front
	 * axle with the big headlamps on a bar before it; a long louvred bonnet
	 * with three flexible chrome exhaust pipes out of its right side; long
	 * swooping front wings; a low open two-seater with two small screens;
	 * a pointed tail between two great pointed rear wings that hide the
	 * back wheels; copper-spoked wire wheels.
	 */
	inline void BuildSsk(FBuilder& B, const FLayout& L)
	{
		const double W = L.HalfWidth;
		const double Floor = -42.0;
		const double Sill = -66.0;
		const double BonnetHalf = 25.0;
		const double Shell = L.Nose - 8.0;
		const double Top = 8.0;
		const double DeckFront = L.SeatX - 26.0;

		// The bonnet, long and narrow, with louvres along its top and a vent panel down each side.
		B.FrontOutline(Paint, { FVector2D(-BonnetHalf, -46.0), FVector2D(BonnetHalf, -46.0), FVector2D(BonnetHalf, Top - 6.0),
			FVector2D(BonnetHalf - 6.0, Top), FVector2D(-BonnetHalf + 6.0, Top), FVector2D(-BonnetHalf, Top - 6.0) }, L.Cowl - 2.0, Shell);
		B.Bar(Chrome, FVector(L.Cowl, 0.0, Top + 0.4), FVector(Shell, 0.0, Top + 0.4), 1.2);
		for (double X = L.Cowl + 30.0; X < Shell - 10.0; X += 5.0)
		{
			for (const double S : { -1.0, 1.0 })
			{
				B.Box(Guards, FVector(X, S > 0.0 ? 4.0 : -BonnetHalf + 7.0, Top - 0.2), FVector(X + 1.8, S > 0.0 ? BonnetHalf - 7.0 : -4.0, Top + 0.5));
			}
		}
		for (double X = L.Cowl + 45.0; X < Shell - 20.0; X += 4.0)
		{
			B.BothSides(Guards, FVector(X, BonnetHalf, -32.0), FVector(X + 2.0, BonnetHalf + 0.6, -4.0));
		}
		// The radiator: a tall chrome shell drawn forward to a vee, the dark core behind upright slats.
		const double Vee = L.Nose + 7.0;
		const double Foot = -42.0;
		const double Crown = Top + 3.0;
		B.Box(Chrome, FVector(Shell, -BonnetHalf, Foot), FVector(L.Nose - 4.0, BonnetHalf, Crown));
		for (const double S : { -1.0, 1.0 })
		{
			const FVector Edge(L.Nose - 4.0, S * BonnetHalf, 0.0);
			const FVector Ridge(Vee, 0.0, 0.0);
			B.Pane(Guards, FVector(Edge.X + 0.3, S * (BonnetHalf - 3.0), Foot + 4.0), FVector(Ridge.X - 1.0, 0.0, Foot + 4.0),
				FVector(Ridge.X - 1.0, 0.0, Crown - 4.0), FVector(Edge.X + 0.3, S * (BonnetHalf - 3.0), Crown - 4.0));
			for (double T = 0.08; T < 0.97; T += 0.075)
			{
				const double SlatX = FMath::Lerp(Edge.X, Ridge.X, T) + 0.4;
				const double SlatY = S * FMath::Lerp(BonnetHalf - 3.0, 0.0, T);
				B.Bar(Chrome, FVector(SlatX, SlatY, Foot + 4.0), FVector(SlatX, SlatY, Crown - 4.0), 0.6);
			}
			for (double Z = Foot + 8.0; Z < Crown - 4.0; Z += 4.0)
			{
				B.Bar(Chrome, FVector(Edge.X + 0.6, S * (BonnetHalf - 3.0), Z), FVector(Ridge.X - 0.4, 0.0, Z), 0.4);
			}
			B.Bar(Chrome, FVector(Edge.X, S * BonnetHalf, Foot), FVector(Edge.X, S * BonnetHalf, Crown), 2.6);
			B.Bar(Chrome, FVector(Edge.X, S * BonnetHalf, Crown), FVector(Ridge.X, 0.0, Crown), 2.6);
			B.Bar(Chrome, FVector(Edge.X, S * BonnetHalf, Foot), FVector(Ridge.X, 0.0, Foot), 2.6);
		}
		B.Bar(Chrome, FVector(Vee, 0.0, Foot), FVector(Vee, 0.0, Crown), 2.6);
		// The cap, and the star in its ring on the shell's crown.
		B.Cylinder(Chrome, FVector(L.Nose - 4.0, 0.0, Crown + 2.5), 3.5, 5.0, 2, 10);
		const FVector Star(Vee + 1.5, 0.0, Crown - 6.0);
		B.Ring(Chrome, Star, 3.6, 2.8, 0.8, 0, 14);
		for (int32 i = 0; i < 3; ++i)
		{
			const double A = FMath::DegreesToRadians(90.0 + 120.0 * i);
			B.Bar(Chrome, Star, Star + FVector(0.0, FMath::Cos(A) * 3.0, FMath::Sin(A) * 3.0), 0.6);
		}
		// Below the radiator: the dark apron between the dumb irons.
		B.Box(Guards, FVector(Shell - 10.0, -BonnetHalf + 2.0, -62.0), FVector(L.Nose + 4.0, BonnetHalf - 2.0, Foot));
		B.BarBothSides(Guards, FVector(Shell - 20.0, 22.0, -56.0), FVector(L.Nose + 12.0, 22.0, -52.0), 6.0);

		// The great headlamps on a bar across the front, just outside the radiator.
		const double LampX = L.Nose + 12.0;
		B.Bar(Chrome, FVector(LampX - 5.0, -46.0, -22.0), FVector(LampX - 5.0, 46.0, -22.0), 2.4);
		for (const double S : { -1.0, 1.0 })
		{
			const FVector Drum(LampX, S * 40.0, -6.0);
			B.Bar(Chrome, Drum - FVector(5.0, 0.0, 16.0), Drum - FVector(4.0, 0.0, 9.0), 2.4);
			B.Ball(Chrome, Drum - FVector(8.0, 0.0, 0.0), 11.0, 1.0);
			B.Cylinder(Chrome, Drum + FVector(2.5, 0.0, 0.0), 13.0, 3.0, 0, 18);
			B.Cylinder(Lamp, Drum + FVector(4.2, 0.0, 0.0), 11.5, 1.0, 0, 18);
		}

		// The front wings: from the point ahead of the wheel, over it, sweeping long and low back to the body side.
		{
			TArray<FVector2D> Path;
			const FVector2D Hub(L.FrontAxle, HubZ);
			Arc(Path, Hub, WheelRadius + 9.0, -14.0, 112.0, 14);
			for (const FVector2D& P : { FVector2D(132.0, -18.0), FVector2D(114.0, -27.0), FVector2D(96.0, -38.0), FVector2D(78.0, -48.0),
				FVector2D(60.0, -56.0), FVector2D(42.0, -62.0), FVector2D(24.0, -65.0), FVector2D(8.0, -66.0) })
			{
				Path.Add(P);
			}
			const int32 Count = Path.Num();
			for (const double S : { -1.0, 1.0 })
			{
				TArray<TArray<FVector>> Rings;
				TArray<TArray<FVector>> Lip;
				for (int32 i = 0; i < Count; ++i)
				{
					const FVector2D Tangent = (Path[FMath::Min(i + 1, Count - 1)] - Path[FMath::Max(i - 1, 0)]).GetSafeNormal();
					// The path runs over the wheel anticlockwise, so this side of it is outwards.
					const FVector Up = FVector(Tangent.Y, 0.0, -Tangent.X);
					const double T = static_cast<double>(i) / (Count - 1);
					const double Ends = FMath::Min(1.0, FMath::Min(T * 7.0 + 0.25, (1.0 - T) * 4.0 + 0.15));
					const double Half = 17.0 * Ends;
					const double Centre = FMath::Lerp(L.TrackY + 1.0, W + 7.0, FMath::Clamp((T - 0.55) / 0.45, 0.0, 1.0));
					Rings.Add(RoundRing(FVector(Path[i].X, S * Centre, Path[i].Y), FVector(0.0, S, 0.0), Up, Half, 8.0 * Ends + 1.0, 2.5, 2.6));
					// The deep rounded lip down its outer edge, which makes the wing look full from the front.
					Lip.Add(RoundRing(FVector(Path[i].X, S * (Centre + Half - 3.0 * Ends), Path[i].Y) - Up * (3.0 * Ends), FVector(0.0, S, 0.0), Up,
						3.5 * Ends + 0.3, 5.0 * Ends + 0.3, 8.0 * Ends + 0.3, 2.2, 12));
				}
				Loft(B, Guards, Rings);
				Loft(B, Guards, Lip);
			}
		}

		// The rear wings: great pointed teardrops, skirted down over the back wheels, their tips past the tail.
		const double WingFront = -36.0;
		const double WingPeak = L.RearAxle + 4.0;
		const double WingTip = L.Tail - 14.0;
		for (const double S : { -1.0, 1.0 })
		{
			TArray<TArray<FVector>> Rings;
			const int32 Count = 26;
			for (int32 i = 0; i <= Count; ++i)
			{
				const double X = FMath::Lerp(WingFront, WingTip, static_cast<double>(i) / Count);
				double Crest;
				if (X > WingPeak)
				{
					const double T = (X - WingPeak) / (WingFront - WingPeak);
					Crest = -3.0 - 50.0 * T * T;
				}
				else
				{
					const double T = (WingPeak - X) / (WingPeak - WingTip);
					Crest = -3.0 - 58.0 * FMath::Pow(T, 1.5);
				}
				const double Front = FMath::Clamp((WingFront - X) / 16.0, 0.0, 1.0);
				const double Back = FMath::Clamp((X - WingTip) / 70.0, 0.0, 1.0);
				const double Size = FMath::Sqrt(Front) * FMath::Pow(Back, 0.6);
				const double Bottom = FMath::Min(Sill + 4.0 * FMath::Pow(1.0 - Back, 2.0), Crest - 2.0);
				const double Mid = (Crest + Bottom) * 0.5;
				const double Half = 16.0 * Size + 0.5;
				const double Centre = L.TrackY + 1.0 - 4.0 * (1.0 - Back);
				Rings.Add(RoundRing(FVector(X, S * Centre, Mid), FVector(0.0, S, 0.0), FVector(0.0, 0.0, 1.0), Half,
					(Crest - Mid) * FMath::Max(Size, 0.15), (Mid - Bottom) * FMath::Max(Size, 0.15), 3.0, 18));
			}
			Loft(B, Guards, Rings);
			// The star on the wheel spat.
			const FVector Badge(L.RearAxle, S * (L.TrackY + 17.4), HubZ + 1.0);
			B.Ring(Chrome, Badge, 4.0, 3.2, 0.8, 1, 14);
			for (int32 i = 0; i < 3; ++i)
			{
				const double A = FMath::DegreesToRadians(90.0 + 120.0 * i);
				B.Bar(Chrome, Badge, Badge + FVector(FMath::Cos(A) * 3.4, 0.0, FMath::Sin(A) * 3.4), 0.6);
			}
		}

		// The body: a low tub from the scuttle back, the open cockpit in it, a narrow sill joining the wings.
		B.Box(Paint, FVector(DeckFront, -W, Sill), FVector(L.Cowl, W, Floor));
		// The doors: rounded sides bulging out, their top line dipping in a long curve between the scuttle and the tail.
		for (const double S : { -1.0, 1.0 })
		{
			TArray<TArray<FVector>> Rings;
			const int32 Count = 14;
			for (int32 i = 0; i <= Count; ++i)
			{
				const double T = static_cast<double>(i) / Count;
				const double X = FMath::Lerp(L.Cowl + 1.0, DeckFront - 1.0, T);
				const double Crest = Top + 2.0 - 11.0 * FMath::Pow(FMath::Sin(PI * FMath::Clamp(T * 1.1 - 0.05, 0.0, 1.0)), 1.6);
				const double Mid = (Crest + Sill) * 0.5;
				Rings.Add(RoundRing(FVector(X, S * (W - 3.0), Mid), FVector(0.0, S, 0.0), FVector(0.0, 0.0, 1.0), 4.5, Crest - Mid, Mid - Sill, 2.3, 16));
			}
			Loft(B, Paint, Rings);
		}
		B.Box(Paint, FVector(L.Cowl - 16.0, -W, Floor), FVector(L.Cowl, W, Top + 2.0));
		B.Box(Chrome, FVector(L.Cowl - 17.0, -W + 8.0, Top - 12.0), FVector(L.Cowl - 16.0, W - 8.0, Top - 2.0));
		B.Bar(Leather, FVector(L.Cowl - 16.0, -W + 2.0, Top + 3.0), FVector(L.Cowl - 16.0, W - 2.0, Top + 3.0), 3.5);
		B.BothSides(Paint, FVector(WingFront - 6.0, W - 2.0, Sill - 1.0), FVector(12.0, W + 10.0, Sill + 2.0));
		for (const double S : { -1.0, 1.0 })
		{
			// The slanting vent on each side of the scuttle.
			B.Pane(Guards, FVector(L.Cowl - 4.0, S * (W + 1.7), -10.0), FVector(L.Cowl - 12.0, S * (W + 1.7), -10.0),
				FVector(L.Cowl - 30.0, S * (W + 1.7), -36.0), FVector(L.Cowl - 22.0, S * (W + 1.7), -36.0));
		}

		// The tail: a smooth hump behind the seats, narrowing and sloping down to a point between the wings.
		{
			TArray<TArray<FVector>> Rings;
			const int32 Count = 18;
			for (int32 i = 0; i <= Count; ++i)
			{
				const double T = static_cast<double>(i) / Count;
				const double X = FMath::Lerp(DeckFront + 2.0, L.Tail, T);
				const double Half = W * FMath::Pow(FMath::Max(1.0 - T * T, 0.0), 0.55) + 0.3;
				const double Crest = FMath::Lerp(Top + 6.0, -50.0, FMath::Pow(T, 1.4));
				const double Bottom = FMath::Lerp(Sill, -58.0, T);
				const double Mid = FMath::Max((Crest + Bottom) * 0.5, Bottom + 0.5);
				const double Shrink = FMath::Max(1.0 - T * T * T, 0.08);
				Rings.Add(RoundRing(FVector(X, 0.0, Mid), FVector(0.0, 1.0, 0.0), FVector(0.0, 0.0, 1.0), Half,
					(Crest - Mid) * Shrink + 0.3, (Mid - Bottom) * Shrink + 0.3, 3.2, 18));
			}
			Loft(B, Paint, Rings);
		}
		// The padded leather roll behind the seats.
		B.Cylinder(Leather, FVector(DeckFront + 2.0, 0.0, Top + 6.0), 6.0, 2.0 * W - 10.0, 1, 12);
		// The number plate and the little lamps under the tail's point.
		const FVector Plate(L.Tail + 10.0, 0.0, -60.0);
		B.Box(Guards, Plate + FVector(-1.0, -14.0, -7.0), Plate + FVector(1.0, 14.0, 7.0));
		B.Box(Chrome, Plate + FVector(-1.5, -14.5, -7.5), Plate + FVector(-1.1, 14.5, 7.5));
		for (const double S : { -1.0, 1.0 })
		{
			B.Cylinder(Chrome, Plate + FVector(1.0, S * 5.0, 11.0), 3.0, 3.0, 0, 10);
			B.Cylinder(TailLamp, Plate + FVector(-0.8, S * 5.0, 11.0), 2.4, 1.0, 0, 10);
		}

		// The three flexible exhaust pipes out of the bonnet's right side, curling down into the wing, ribbed in chrome.
		for (int32 i = 0; i < 3; ++i)
		{
			const double X = L.Cowl + 52.0 + i * 28.0;
			const FVector P0(X, BonnetHalf, -6.0);
			const FVector P1(X + 2.0, BonnetHalf + 14.0, -8.0);
			const FVector P2(X - 8.0, BonnetHalf + 22.0, -34.0);
			const FVector P3(X - 26.0 - i * 4.0, W + 10.0, -56.0);
			FVector Last = P0;
			for (int32 Step = 1; Step <= 12; ++Step)
			{
				const double T = Step / 12.0;
				const double U = 1.0 - T;
				const FVector P = P0 * (U * U * U) + P1 * (3.0 * U * U * T) + P2 * (3.0 * U * T * T) + P3 * (T * T * T);
				B.Bar(Chrome, Last, P, 6.0);
				B.Bar(Rubber, (Last + P) * 0.5 - (P - Last) * 0.08, (Last + P) * 0.5 + (P - Last) * 0.08, 6.8);
				Last = P;
			}
			B.Cylinder(Chrome, FVector(X, BonnetHalf + 0.4, -6.0), 4.6, 2.0, 1, 10);
		}
		// Under the right side, the one pipe they join runs back to the rear wing.
		B.Cylinder(Guards, FVector((WingFront - 6.0 + 30.0) * 0.5, W + 8.0, -70.0), 4.0, 30.0 - WingFront + 6.0, 0, 10);

		// The small vee screen in two panes, the seat and wheel, and the axles and rails under it all.
		const double Screen = L.Cowl - 12.0;
		for (const double S : { -1.0, 1.0 })
		{
			B.Pane(Glass, FVector(Screen + 4.0, S * 2.0, Top + 3.0), FVector(Screen, S * (W - 8.0), Top + 3.0),
				FVector(Screen - 5.0, S * (W - 8.0), L.RoofTop), FVector(Screen - 1.0, S * 2.0, L.RoofTop));
			B.Bar(Chrome, FVector(Screen, S * (W - 8.0), Top + 2.0), FVector(Screen - 5.0, S * (W - 8.0), L.RoofTop), 1.4);
			B.Bar(Chrome, FVector(Screen - 5.0, S * (W - 8.0), L.RoofTop), FVector(Screen - 1.0, S * 2.0, L.RoofTop), 1.4);
			B.Bar(Chrome, FVector(Screen + 4.0, S * 2.0, Top + 2.0), FVector(Screen - 1.0, S * 2.0, L.RoofTop), 1.4);
		}
		// The seat sits low, its back hardly over the body side; the wheel stands where BuildSeatedDriver holds it.
		B.Box(Leather, FVector(L.SeatX - 12.0, -W + 8.0, Floor), FVector(L.SeatX + 20.0, W - 8.0, L.SeatTop));
		B.Box(Leather, FVector(L.SeatX - 18.0, -W + 6.0, L.SeatTop), FVector(L.SeatX - 10.0, W - 6.0, Top + 6.0));
		const FVector Wheel(L.SeatX + 54.0, L.DriverSide, L.SeatTop + 35.0);
		B.Ring(Rubber, Wheel, 19.0, 16.5, 2.5, 0, 16);
		B.Bar(Rubber, Wheel - FVector(0.0, 16.5, 0.0), Wheel + FVector(0.0, 16.5, 0.0), 1.6);
		B.Bar(Rubber, Wheel, Wheel + FVector(30.0, 0.0, -26.0), 3.0);
		B.Box(Guards, FVector(L.Tail + 30.0, -34.0, -66.0), FVector(L.Nose - 10.0, 34.0, -50.0));
		B.Bar(Guards, FVector(L.FrontAxle, -L.TrackY + 8.0, HubZ), FVector(L.FrontAxle, L.TrackY - 8.0, HubZ), 5.0);
		B.Bar(Guards, FVector(L.RearAxle, -L.TrackY + 8.0, HubZ), FVector(L.RearAxle, L.TrackY - 8.0, HubZ), 7.0);
		B.Cylinder(Guards, FVector(L.RearAxle, 0.0, HubZ), 11.0, 18.0, 1, 10);
	}

	inline void BuildTourer(FBuilder& B, const FLayout& L)
	{
		BuildFront(B, L, 38.0);
		const double Back = L.Tail;
		const double W = BodyHalfWidth;
		const double Floor = -40.0;
		// An open tub: the body to the floor, thin sides up to the waist, a rear deck.
		B.SideOutline(Paint, { FVector2D(L.Cowl, -44.0), FVector2D(L.Cowl, Floor), FVector2D(Back + 4.0, Floor), FVector2D(Back, Waist - 42.0),
			FVector2D(Back + 6.0, -44.0) }, -W, W);
		B.BothSides(Paint, FVector(Back + 28.0, W - 4.0, Floor), FVector(L.Cowl, W, Waist));
		B.SideOutline(Paint, { FVector2D(Back + 28.0, Floor), FVector2D(Back + 28.0, Waist), FVector2D(Back + 16.0, Waist), FVector2D(Back + 3.0, Waist - 14.0),
			FVector2D(Back + 4.0, Floor) }, -W, W);
		B.Box(Paint, FVector(L.Cowl - 8.0, -W, Floor), FVector(L.Cowl, W, Waist));
		B.BarBothSides(Leather, FVector(Back + 28.0, W - 2.0, Waist + 1.0), FVector(L.Cowl - 8.0, W - 2.0, Waist + 1.0), 4.0);
		// Windscreen in a chrome frame.
		const double Screen = L.Cowl - 4.0;
		B.Pane(Glass, FVector(Screen, -W + 8.0, Waist + 2.0), FVector(Screen, W - 8.0, Waist + 2.0), FVector(Screen - 6.0, W - 8.0, Waist + 36.0), FVector(Screen - 6.0, -W + 8.0, Waist + 36.0));
		B.BarBothSides(Chrome, FVector(Screen, W - 7.0, Waist), FVector(Screen - 6.0, W - 7.0, Waist + 37.0), 2.5);
		B.Bar(Chrome, FVector(Screen - 6.0, -W + 7.0, Waist + 37.0), FVector(Screen - 6.0, W - 7.0, Waist + 37.0), 2.5);
		B.Bar(Chrome, FVector(Screen, -W + 7.0, Waist + 1.0), FVector(Screen, W - 7.0, Waist + 1.0), 2.5);
		// The hood, folded down behind the back seat.
		B.Cylinder(Guards, FVector(Back + 22.0, 0.0, Waist + 8.0), 9.0, 2.0 * W - 6.0, 1, 10);
		B.Box(Guards, FVector(Back + 12.0, -W + 3.0, Waist), FVector(Back + 32.0, W - 3.0, Waist + 8.0));
		B.BarBothSides(Chrome, FVector(Back + 40.0, W - 1.0, Waist - 4.0), FVector(Back + 24.0, W - 1.0, Waist + 10.0), 2.0);
		// Seats, front and back, with the driver.
		BuildDriverSeat(B, L);
		B.Box(Leather, FVector(Back + 30.0, -62.0, Floor), FVector(Back + 64.0, 62.0, L.SeatTop));
		B.Box(Leather, FVector(Back + 28.0, -62.0, L.SeatTop), FVector(Back + 36.0, 62.0, Waist + 14.0));
		B.BothSides(Chrome, FVector(-44.0, W, Waist - 9.0), FVector(-37.0, W + 1.4, Waist - 7.0));
		B.BothSides(Guards, FVector(-48.6, W, -38.0), FVector(-47.4, W + 0.4, Waist - 2.0));
		BuildRunningGear(B, L, Back);
		BuildRearSpare(B, Back - 9.0, Waist - 32.0);
	}

	inline void Build(FBuilder& B, const FStyle& Style)
	{
		const FLayout L = Layout(Style.Body);
		switch (Style.Body)
		{
		case EBody::Tourer: BuildTourer(B, L); break;
		case EBody::Coupe: BuildCoupe(B, L); break;
		case EBody::Limousine: BuildLimousine(B, L, Style); break;
		case EBody::Van: BuildVan(B, L); break;
		case EBody::Balilla: BuildBalilla(B, L); break;
		case EBody::Opel: BuildOpel(B, L); break;
		case EBody::Steyr: BuildSteyr(B, L); break;
		case EBody::Ssk: BuildSsk(B, L); break;
		default: BuildSaloon(B, L, Style); break;
		}
	}

	/** One wheel, with its hub at the origin and its axle along Y. */
	inline void BuildWheel(FBuilder& B, EWheel Wheel)
	{
		const double Half = WheelWidth * 0.5;
		const double Rim = WheelRadius - 9.0;
		B.Ring(Rubber, FVector::ZeroVector, WheelRadius, Rim, WheelWidth, 1, 18);
		// Tread blocks, so you can see the tyre turn even from the side.
		for (int32 i = 0; i < 18; ++i)
		{
			const double A = 2.0 * PI * (i + 0.5) / 18.0;
			const FVector Out(FMath::Cos(A), 0.0, FMath::Sin(A));
			B.Bar(Rubber, Out * (WheelRadius - 0.5) - FVector(0.0, Half - 1.0, 0.0), Out * (WheelRadius - 0.5) + FVector(0.0, Half - 1.0, 0.0), 2.6);
		}
		B.Ring(Spokes, FVector::ZeroVector, Rim + 0.5, Rim - 3.0, WheelWidth - 4.0, 1, 18);
		switch (Wheel)
		{
		case EWheel::Wire:
			// Many thin spokes, crossing from either side of the hub.
			for (int32 i = 0; i < 24; ++i)
			{
				const double A = 2.0 * PI * i / 24.0;
				const double S = i % 2 == 0 ? 1.0 : -1.0;
				const FVector Out(FMath::Cos(A), 0.0, FMath::Sin(A));
				B.Bar(Spokes, FVector(0.0, S * 3.5, 0.0) + Out * 5.0, Out * (Rim - 2.0), 0.9);
			}
			break;
		case EWheel::Disc:
			// A pressed steel disc with a ring of bolt holes.
			B.Cylinder(Spokes, FVector::ZeroVector, Rim - 2.0, 4.0, 1, 18);
			for (int32 i = 0; i < 6; ++i)
			{
				const double A = 2.0 * PI * i / 6.0;
				const FVector Out(FMath::Cos(A), 0.0, FMath::Sin(A));
				B.Cylinder(Rubber, Out * 15.0, 2.2, 4.6, 1, 6);
			}
			break;
		default:
			// Ten stout wooden spokes.
			for (int32 i = 0; i < 10; ++i)
			{
				const double A = 2.0 * PI * i / 10.0;
				const FVector Out(FMath::Cos(A), 0.0, FMath::Sin(A));
				B.Bar(Spokes, Out * 6.0, Out * (Rim - 2.0), 3.4);
			}
			break;
		}
		B.Cylinder(Chrome, FVector::ZeroVector, 8.0, WheelWidth + 4.0, 1, 12);
		B.Cylinder(Chrome, FVector::ZeroVector, 4.0, WheelWidth + 7.0, 1, 8);
	}

	/** The colour of a part in this style; Glass carries how much it hides in alpha. */
	inline FLinearColor PartColor(EPart Part, const FStyle& Style)
	{
		switch (Part)
		{
		case Paint: return Style.PaintColor;
		case Guards: return Style.GuardColor;
		case Rubber: return FLinearColor(0.012f, 0.012f, 0.012f);
		case Chrome: return FLinearColor(0.62f, 0.62f, 0.64f);
		// Dark: the glass is unlit, so a light tint glares white when the eye adjusts to shade.
		case Glass: return FLinearColor(0.05f, 0.06f, 0.065f, 0.35f);
		case Lamp: return FLinearColor(1.0f, 0.92f, 0.65f);
		case TailLamp: return FLinearColor(0.7f, 0.02f, 0.02f);
		case Leather: return FLinearColor(0.18f, 0.07f, 0.04f);
		case Skin: return FLinearColor(0.72f, 0.50f, 0.38f);
		case Accent: return Style.AccentColor;
		case Spokes: return Style.SpokeColor;
		default: return FLinearColor::White;
		}
	}

	/** Paints of the period: blacks, deep greens, maroons, navy, a few creams and tans. */
	inline FLinearColor RandomPaint(FRandomStream& Random)
	{
		const FLinearColor Paints[] = {
			FLinearColor(0.010f, 0.010f, 0.012f),
			FLinearColor(0.015f, 0.015f, 0.018f),
			FLinearColor(0.03f, 0.07f, 0.05f),
			FLinearColor(0.02f, 0.05f, 0.03f),
			FLinearColor(0.20f, 0.03f, 0.03f),
			FLinearColor(0.12f, 0.02f, 0.03f),
			FLinearColor(0.02f, 0.03f, 0.10f),
			FLinearColor(0.04f, 0.05f, 0.12f),
			FLinearColor(0.45f, 0.40f, 0.28f),
			FLinearColor(0.22f, 0.16f, 0.09f),
			FLinearColor(0.30f, 0.30f, 0.30f),
			FLinearColor(0.35f, 0.22f, 0.06f),
		};
		return Paints[Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Paints)) - 1)];
	}

	/** Any car of the streets of Pest, with this paint. */
	inline FStyle RandomStyle(FRandomStream& Random, const FLinearColor& InPaint)
	{
		FStyle Style;
		Style.PaintColor = InPaint;
		// The best sellers of the early thirties first (the Fiat Balilla and the
		// Opel 1.2), then saloons, tourers and coupés; fewer vans and limousines,
		// and now and then (3 in 100) a big Steyr saloon like Dr. Halász's.
		const int32 Pick = Random.RandRange(0, 99);
		Style.Body = Pick < 22 ? EBody::Balilla : Pick < 40 ? EBody::Opel : Pick < 59 ? EBody::Saloon : Pick < 62 ? EBody::Steyr
			: Pick < 74 ? EBody::Tourer : Pick < 84 ? EBody::Coupe : Pick < 94 ? EBody::Van : EBody::Limousine;
		Style.Wheel = static_cast<EWheel>(Random.RandRange(0, static_cast<int32>(EWheel::Count) - 1));
		if (Style.Body == EBody::Steyr)
		{
			// Like the doctor's: disc wheels with rims in the body's paint, black wings.
			Style.Wheel = EWheel::Disc;
			Style.GuardColor = FLinearColor(0.008f, 0.008f, 0.009f);
			Style.SpokeColor = InPaint;
			return Style;
		}
		if (Style.Body == EBody::Balilla)
		{
			// Pressed steel disc wheels, black wings, rims in red, black or the body's paint.
			Style.Wheel = EWheel::Disc;
			const FLinearColor RimPaints[] = { FLinearColor(0.30f, 0.03f, 0.02f), FLinearColor(0.02f, 0.02f, 0.02f), InPaint };
			Style.GuardColor = FLinearColor(0.012f, 0.012f, 0.012f);
			Style.SpokeColor = RimPaints[Random.RandRange(0, 2)];
			return Style;
		}
		// Black mudguards on most; some are painted to match.
		const bool bMatching = Random.FRand() < 0.3f;
		Style.GuardColor = bMatching ? InPaint : FLinearColor(0.012f, 0.012f, 0.012f);
		const FLinearColor SpokePaints[] = {
			FLinearColor(0.55f, 0.48f, 0.33f),
			FLinearColor(0.30f, 0.03f, 0.02f),
			FLinearColor(0.02f, 0.02f, 0.02f),
			FLinearColor(0.60f, 0.60f, 0.62f),
			InPaint,
		};
		Style.SpokeColor = SpokePaints[Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(SpokePaints)) - 1)];
		if (Style.Body == EBody::Van)
		{
			// The tradesman's sign panel: cream, or white.
			Style.AccentColor = Random.FRand() < 0.5f ? FLinearColor(0.70f, 0.62f, 0.42f) : FLinearColor(0.78f, 0.78f, 0.74f);
		}
		return Style;
	}

	/** Dr. Halász's Steyr: deep green with black wings, disc wheels with green rims (mission 7). */
	inline FStyle SteyrStyle()
	{
		FStyle Style;
		Style.Body = EBody::Steyr;
		Style.PaintColor = FLinearColor(0.012f, 0.075f, 0.040f);
		Style.GuardColor = FLinearColor(0.008f, 0.008f, 0.009f);
		Style.Wheel = EWheel::Disc;
		Style.SpokeColor = FLinearColor(0.012f, 0.075f, 0.040f);
		return Style;
	}

	/** The car show's Mercedes-Benz SSK: black all over, copper-spoked wire wheels. */
	inline FStyle SskStyle()
	{
		FStyle Style;
		Style.Body = EBody::Ssk;
		Style.PaintColor = FLinearColor(0.006f, 0.006f, 0.007f);
		Style.GuardColor = FLinearColor(0.006f, 0.006f, 0.007f);
		Style.Wheel = EWheel::Wire;
		Style.SpokeColor = FLinearColor(0.32f, 0.12f, 0.045f);
		return Style;
	}

	/** The company taxi: a black saloon with the taxi markings. */
	inline FStyle TaxiStyle()
	{
		FStyle Style;
		Style.PaintColor = FLinearColor(0.01f, 0.01f, 0.01f);
		Style.Wheel = EWheel::Disc;
		Style.SpokeColor = FLinearColor(0.85f, 0.62f, 0.05f);
		Style.bTaxi = true;
		return Style;
	}

	/** The company's best car, for its regulars: a deep red limousine with a gold chequered band and gold wire wheels. */
	inline FStyle DeluxeTaxiStyle()
	{
		FStyle Style;
		Style.Body = EBody::Limousine;
		Style.PaintColor = FLinearColor(0.10f, 0.012f, 0.02f);
		Style.GuardColor = FLinearColor(0.008f, 0.008f, 0.009f);
		Style.Wheel = EWheel::Wire;
		Style.SpokeColor = FLinearColor(0.80f, 0.55f, 0.12f);
		Style.AccentColor = FLinearColor(0.80f, 0.55f, 0.12f);
		Style.bTaxi = true;
		return Style;
	}

	/**
	 * Someone sitting in the car, as one shape in the car's frame: a person
	 * from PersonKit, seated with the hips at Hip (the middle between them),
	 * the arms reaching forward by ArmPitch and the thighs tilted up by
	 * LegPitch (degrees). Pieces of the same colour share a section.
	 */
	inline void BuildSeatedPerson(const PersonKit::FOutfit& InOutfit, const FVector& Hip, double ArmPitch, double LegPitch, TArray<PersonKit::FSection>& Out)
	{
		PersonKit::FOutfit Outfit = InOutfit;
		Outfit.bSeated = true;
		Outfit.Held = PersonKit::EHeld::None;
		Outfit.Height = FMath::Min(Outfit.Height, static_cast<float>(DriverMaxHeight));
		PersonKit::FPerson Person;
		PersonKit::Build(Outfit, Person);
		const FVector Joint = Person.Pieces[PersonKit::LegRight].Pivot;
		const FVector Offset(Hip.X - Joint.X, Hip.Y, Hip.Z - Joint.Z);
		Out.Reset();
		for (int32 p = 0; p < PersonKit::NumPieces; ++p)
		{
			const PersonKit::FPiece& Piece = Person.Pieces[p];
			// A shin turns with its thigh, about the hip.
			const PersonKit::EPiece Hangs = PersonKit::ParentOf(p);
			const int32 Swung = Hangs != PersonKit::NumPieces ? static_cast<int32>(Hangs) : p;
			const FVector Pivot = Person.Pieces[Swung].Pivot;
			const FVector Below = Piece.Pivot - Pivot;
			const bool bArm = Swung == PersonKit::ArmLeft || Swung == PersonKit::ArmRight;
			const bool bLeg = Swung == PersonKit::LegLeft || Swung == PersonKit::LegRight;
			const FRotator Tilt(bArm ? ArmPitch : bLeg ? LegPitch : 0.0, 0.0, 0.0);
			for (const PersonKit::FSection& From : Piece.Sections)
			{
				PersonKit::FSection* Into = Out.FindByPredicate([&From](const PersonKit::FSection& Each) { return Each.Color.Equals(From.Color); });
				if (!Into)
				{
					Into = &Out.AddDefaulted_GetRef();
					Into->Color = From.Color;
				}
				const int32 First = Into->Vertices.Num();
				for (int32 i = 0; i < From.Vertices.Num(); ++i)
				{
					Into->Vertices.Add(Offset + Pivot + Tilt.RotateVector(Below + From.Vertices[i]));
					Into->Normals.Add(Tilt.RotateVector(From.Normals[i]));
				}
				for (const int32 Corner : From.Triangles)
				{
					Into->Triangles.Add(First + Corner);
				}
			}
		}
	}

	/** The driver at the wheel, the hips on the cushion (a thigh's thickness above it) and the hands on the wheel. */
	inline void BuildSeatedDriver(const PersonKit::FOutfit& Outfit, const FLayout& L, TArray<PersonKit::FSection>& Out)
	{
		BuildSeatedPerson(Outfit, FVector(L.SeatX + 2.0, L.DriverSide, L.SeatTop + 9.0), DriverArmPitch, DriverLegPitch, Out);
	}

	/** A passenger on the back seat. */
	inline void BuildBackSeatPassenger(const PersonKit::FOutfit& Outfit, const FLayout& L, TArray<PersonKit::FSection>& Out)
	{
		BuildSeatedPerson(Outfit, FVector(L.RearSeatX, L.PassengerSide, L.SeatTop + 9.0), PassengerArmPitch, PassengerLegPitch, Out);
	}

	/** Where someone stands to get in at a back door, on the left (Side -1) or right (1), on the ground. */
	inline FVector RearDoor(EBody Body, double Side)
	{
		return FVector(Layout(Body).RearDoorX, Side * (Layout(Body).HalfWidth + 45.0), Ground);
	}

	/** A key telling styles with the same shapes apart (the colours don't matter). */
	inline int32 ShapeKey(const FStyle& Style)
	{
		return static_cast<int32>(Style.Body) * 4 + (Style.bTaxi ? 1 : 0) + (Style.bPolice ? 2 : 0);
	}
}
