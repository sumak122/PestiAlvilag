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

	/** Balilla: the little two-door Fiat 508 Balilla (1932), upright, with a trunk on the back. Opel: the boxy two-door Opel 1.2 Liter (1931). */
	enum class EBody : uint8 { Saloon, Tourer, Coupe, Limousine, Van, Balilla, Opel, Count };
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
		// Opel 1.2), then saloons, tourers and coupés; fewer vans and limousines.
		const int32 Pick = Random.RandRange(0, 99);
		Style.Body = Pick < 22 ? EBody::Balilla : Pick < 40 ? EBody::Opel : Pick < 62 ? EBody::Saloon : Pick < 74 ? EBody::Tourer
			: Pick < 84 ? EBody::Coupe : Pick < 94 ? EBody::Van : EBody::Limousine;
		Style.Wheel = static_cast<EWheel>(Random.RandRange(0, static_cast<int32>(EWheel::Count) - 1));
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
