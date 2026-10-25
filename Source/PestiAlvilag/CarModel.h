// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

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

	enum class EBody : uint8 { Saloon, Tourer, Coupe, Limousine, Van, Count };
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
		bool bDriver = true;
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
	};

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
			L.Exhaust = FVector(-226.0, 40.0, -62.0);
			break;
		case EBody::Van:
			L.Tail = -208.0;
			L.RoofTop = 88.0;
			L.Exhaust = FVector(-222.0, 44.0, -62.0);
			break;
		case EBody::Tourer:
			L.RoofTop = 52.0;
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
		return FVector(Index < 2 ? L.FrontAxle : L.RearAxle, (Index % 2 == 0) ? -WheelY : WheelY, HubZ);
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
		B.Bar(Chrome, FVector(LampX, -WheelY + 8.0, -6.0), FVector(LampX, WheelY - 8.0, -6.0), 3.0);
		for (const double S : { -1.0, 1.0 })
		{
			const FVector Drum(LampX + 2.0, S * (BonnetHalf + 20.0), 10.0);
			B.Bar(Chrome, Drum - FVector(0.0, 0.0, 16.0), Drum - FVector(0.0, 0.0, 6.0), 2.5);
			B.Cylinder(Chrome, Drum, 11.0, 14.0, 0, 12);
			B.Cylinder(Lamp, Drum + FVector(7.4, 0.0, 0.0), 9.5, 1.0, 0, 12);
		}
		// Bumper: two chrome blades on irons.
		const double BumperX = L.Nose + 18.0;
		B.Bar(Chrome, FVector(BumperX, -80.0, -58.0), FVector(BumperX, 80.0, -58.0), 5.0);
		B.Bar(Chrome, FVector(BumperX - 0.5, -76.0, -50.0), FVector(BumperX - 0.5, 76.0, -50.0), 3.0);
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
		B.Guard(Guards, Front, WheelY - 12.0, WheelY + 12.0, 3.0);
		// Rear mudguard.
		TArray<FVector2D> Rear;
		Rear.Add(FVector2D(L.RearAxle + 52.0, BoardZ));
		Arc(Rear, FVector2D(L.RearAxle, HubZ), R, 30.0, 178.0, 10);
		Rear.Add(FVector2D(L.RearAxle - R - 4.0, HubZ - 6.0));
		B.Guard(Guards, Rear, WheelY - 12.0, WheelY + 12.0, 3.0);
		// Running boards between them, with rubber treads.
		B.BothSides(Guards, FVector(L.RearAxle + 50.0, BodyHalfWidth - 2.0, BoardZ - 2.0), FVector(L.FrontAxle - 64.0, WheelY + 10.0, BoardZ + 1.5));
		for (double X = L.RearAxle + 58.0; X < L.FrontAxle - 70.0; X += 12.0)
		{
			B.BothSides(Rubber, FVector(X, BodyHalfWidth + 2.0, BoardZ + 1.5), FVector(X + 6.0, WheelY + 8.0, BoardZ + 2.2));
		}
		// Chassis rails and axles, dark under the body.
		B.Box(Guards, FVector(Tail + 4.0, -46.0, -62.0), FVector(L.Nose - 6.0, 46.0, -44.0));
		B.Bar(Guards, FVector(L.FrontAxle, -WheelY + 8.0, HubZ), FVector(L.FrontAxle, WheelY - 8.0, HubZ), 5.0);
		B.Bar(Guards, FVector(L.RearAxle, -WheelY + 8.0, HubZ), FVector(L.RearAxle, WheelY - 8.0, HubZ), 7.0);
		B.Cylinder(Guards, FVector(L.RearAxle, 0.0, HubZ), 11.0, 18.0, 1, 10);
		// Rear bumper and the red tail lamps on the mudguards.
		const double BumperX = Tail - 14.0;
		B.Bar(Chrome, FVector(BumperX, -78.0, -58.0), FVector(BumperX, 78.0, -58.0), 5.0);
		B.BarBothSides(Guards, FVector(Tail + 6.0, 40.0, -56.0), FVector(BumperX, 40.0, -56.0), 3.0);
		for (const double S : { -1.0, 1.0 })
		{
			const FVector Drum(L.RearAxle - R - 2.0, S * (WheelY + 2.0), HubZ + 22.0);
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

	/** The driver at the wheel on the left, and the wheel in front of him. */
	inline void BuildDriver(FBuilder& B, double SeatX, double SeatTop, double WheelX)
	{
		const double Y = -36.0;
		// Seat, a little of which shows.
		B.Box(Leather, FVector(SeatX - 12.0, -62.0, SeatTop - 8.0), FVector(SeatX + 20.0, 62.0, SeatTop));
		B.Box(Leather, FVector(SeatX - 14.0, -62.0, SeatTop), FVector(SeatX - 6.0, 62.0, SeatTop + 38.0));
		// Coat, arms to the wheel, head and hat.
		B.Box(Rubber, FVector(SeatX - 6.0, Y - 15.0, SeatTop), FVector(SeatX + 12.0, Y + 15.0, SeatTop + 44.0));
		B.Bar(Rubber, FVector(SeatX + 4.0, Y - 13.0, SeatTop + 38.0), FVector(WheelX - 3.0, Y - 11.0, SeatTop + 26.0), 7.0);
		B.Bar(Rubber, FVector(SeatX + 4.0, Y + 13.0, SeatTop + 38.0), FVector(WheelX - 3.0, Y + 11.0, SeatTop + 26.0), 7.0);
		B.Box(Skin, FVector(SeatX - 1.0, Y - 4.0, SeatTop + 43.0), FVector(SeatX + 6.0, Y + 4.0, SeatTop + 48.0));
		B.Ball(Skin, FVector(SeatX + 3.0, Y, SeatTop + 55.0), 8.5);
		B.Cylinder(Rubber, FVector(SeatX + 3.0, Y, SeatTop + 62.0), 12.0, 1.4, 2, 12);
		B.Cylinder(Rubber, FVector(SeatX + 3.0, Y, SeatTop + 66.0), 8.0, 7.0, 2, 10);
		// Steering wheel on its column.
		B.Ring(Rubber, FVector(WheelX, Y, SeatTop + 27.0), 17.0, 14.5, 2.5, 0, 14);
		B.Bar(Rubber, FVector(WheelX, Y, SeatTop + 27.0), FVector(WheelX + 26.0, Y, SeatTop - 4.0), 3.0);
	}

	/**
	 * A closed body: the lower body to the waist, then a glass house with
	 * pillars and a roof. Pillars lists the X of the door pillars between
	 * the windscreen and the back.
	 */
	inline void BuildClosedBody(FBuilder& B, const FLayout& L, const TArray<FVector2D>& Lower, double GlassBack, double RoofBack,
		const TArray<double>& Pillars, double SeatX)
	{
		const double W = BodyHalfWidth;
		const double GlassTop = L.RoofTop - 10.0;
		B.SideOutline(Paint, Lower, -W, W);
		// The glass house, slightly narrower than the body.
		const double Screen = L.Cowl - 2.0;
		const double ScreenTop = Screen - 14.0;
		B.SideOutline(Glass, { FVector2D(Screen, Waist), FVector2D(ScreenTop, GlassTop), FVector2D(RoofBack + 4.0, GlassTop), FVector2D(GlassBack, Waist) }, -(W - 5.0), W - 5.0);
		// Roof, gently rounded at the back.
		B.SideOutline(Paint, { FVector2D(ScreenTop + 3.0, GlassTop - 1.0), FVector2D(ScreenTop - 1.0, L.RoofTop - 2.0), FVector2D(ScreenTop - 20.0, L.RoofTop),
			FVector2D(RoofBack + 10.0, L.RoofTop), FVector2D(RoofBack - 4.0, L.RoofTop - 5.0), FVector2D(RoofBack, GlassTop - 1.0) }, -(W - 2.0), W - 2.0);
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
		// Inside: the driver, and the back seat.
		BuildDriver(B, SeatX, Waist - 30.0, Screen - 22.0);
	}

	inline void BuildSaloon(FBuilder& B, const FLayout& L, const FStyle& Style)
	{
		BuildFront(B, L, 38.0);
		const double Back = L.Tail;
		BuildClosedBody(B, L, { FVector2D(L.Cowl, -44.0), FVector2D(L.Cowl, Waist), FVector2D(Back + 18.0, Waist), FVector2D(Back + 4.0, Waist - 14.0),
			FVector2D(Back, Waist - 42.0), FVector2D(Back + 6.0, -44.0) }, Back + 20.0, Back + 46.0, { -45.0, -112.0 }, -40.0);
		B.Box(Leather, FVector(-128.0, -62.0, Waist - 4.0), FVector(-120.0, 62.0, Waist + 16.0));
		BuildRunningGear(B, L, Back);
		BuildRearSpare(B, Back - 9.0, Waist - 32.0);
		if (Style.bTaxi)
		{
			// Chequered band below the waist, the roof lamp and the meter flag by the driver.
			for (const double S : { -1.0, 1.0 })
			{
				const double Y = S * (BodyHalfWidth + 0.3);
				B.Box(Accent, FVector(Back + 14.0, Y - 0.3, 2.0), FVector(L.Cowl - 4.0, Y + 0.3, 14.0));
				int32 Index = 0;
				for (double X = Back + 14.0; X + 11.0 <= L.Cowl - 4.0; X += 11.0, ++Index)
				{
					const double Z = Index % 2 == 0 ? 8.0 : 2.0;
					B.Box(Rubber, FVector(X, Y - 0.5, Z), FVector(X + 11.0, Y + 0.5, Z + 6.0));
				}
			}
			B.Box(Accent, FVector(-30.0, -24.0, L.RoofTop - 1.0), FVector(-14.0, 24.0, L.RoofTop + 11.0));
			B.Bar(Chrome, FVector(L.Cowl + 6.0, -44.0, Waist - 4.0), FVector(L.Cowl + 6.0, -44.0, Waist + 22.0), 2.0);
			B.Box(TailLamp, FVector(L.Cowl + 5.0, -52.0, Waist + 18.0), FVector(L.Cowl + 7.0, -44.0, Waist + 26.0));
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
			FVector2D(Back + 4.0, Waist - 30.0), FVector2D(Back, Waist - 50.0), FVector2D(Back + 6.0, -44.0) }, -92.0, -72.0, { }, -40.0);
		// Boot lid seam and handle.
		B.Bar(Guards, FVector(-112.0, -40.0, Waist - 2.0), FVector(-112.0, 40.0, Waist - 2.0), 1.0);
		B.Bar(Chrome, FVector(Back + 1.0, -8.0, Waist - 36.0), FVector(Back + 1.0, 8.0, Waist - 36.0), 2.0);
		BuildRunningGear(B, L, Back);
		BuildRearSpare(B, Back - 6.0, Waist - 46.0);
	}

	inline void BuildLimousine(FBuilder& B, const FLayout& L)
	{
		BuildFront(B, L, 40.0);
		const double Back = L.Tail;
		BuildClosedBody(B, L, { FVector2D(L.Cowl, -44.0), FVector2D(L.Cowl, Waist), FVector2D(Back + 14.0, Waist), FVector2D(Back + 2.0, Waist - 12.0),
			FVector2D(Back, Waist - 44.0), FVector2D(Back + 4.0, -44.0) }, Back + 16.0, Back + 40.0, { -36.0, -98.0 }, -30.0);
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
	}

	inline void BuildVan(FBuilder& B, const FLayout& L)
	{
		BuildFront(B, L, 38.0);
		const double CabBack = -34.0;
		// The cab, with the driver.
		BuildClosedBody(B, L, { FVector2D(L.Cowl, -44.0), FVector2D(L.Cowl, Waist), FVector2D(CabBack, Waist), FVector2D(CabBack, -44.0) },
			CabBack + 2.0, CabBack + 2.0, { }, -30.0);
		// The box behind: tall, with a sign panel down each side and doors at the back.
		B.Box(Paint, FVector(L.Tail, -78.0, -44.0), FVector(CabBack, 78.0, L.RoofTop));
		B.Box(Paint, FVector(L.Tail - 2.0, -80.0, L.RoofTop), FVector(CabBack + 4.0, 80.0, L.RoofTop + 4.0));
		B.BothSides(Accent, FVector(L.Tail + 14.0, 78.0, -22.0), FVector(CabBack - 14.0, 78.6, L.RoofTop - 14.0));
		B.BothSides(Paint, FVector(L.Tail + 18.0, 78.4, -18.0), FVector(CabBack - 18.0, 78.8, -14.0));
		B.Box(Guards, FVector(L.Tail - 0.6, -0.6, -40.0), FVector(L.Tail, 0.6, L.RoofTop - 4.0));
		B.Box(Chrome, FVector(L.Tail - 1.6, 3.0, 14.0), FVector(L.Tail, 10.0, 17.0));
		for (const double Z : { -30.0, L.RoofTop - 18.0 })
		{
			B.Box(Chrome, FVector(L.Tail - 1.4, -70.0, Z), FVector(L.Tail, -60.0, Z + 6.0));
			B.Box(Chrome, FVector(L.Tail - 1.4, 60.0, Z), FVector(L.Tail, 70.0, Z + 6.0));
		}
		BuildRunningGear(B, L, L.Tail);
	}

	inline void BuildTourer(FBuilder& B, const FLayout& L)
	{
		BuildFront(B, L, 38.0);
		const double Back = L.Tail;
		const double W = BodyHalfWidth;
		const double Floor = -18.0;
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
		BuildDriver(B, -40.0, Floor + 16.0, Screen - 24.0);
		B.Box(Leather, FVector(Back + 30.0, -62.0, Floor), FVector(Back + 64.0, 62.0, Floor + 16.0));
		B.Box(Leather, FVector(Back + 28.0, -62.0, Floor + 16.0), FVector(Back + 36.0, 62.0, Waist + 14.0));
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
		case EBody::Limousine: BuildLimousine(B, L); break;
		case EBody::Van: BuildVan(B, L); break;
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
		case Glass: return FLinearColor(0.30f, 0.36f, 0.38f, 0.45f);
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
		// Saloons most of all, then tourers and coupés; fewer vans and limousines.
		const int32 Pick = Random.RandRange(0, 99);
		Style.Body = Pick < 40 ? EBody::Saloon : Pick < 58 ? EBody::Tourer : Pick < 74 ? EBody::Coupe : Pick < 87 ? EBody::Van : EBody::Limousine;
		Style.Wheel = static_cast<EWheel>(Random.RandRange(0, static_cast<int32>(EWheel::Count) - 1));
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

	/** A key telling styles with the same shapes apart (the colours don't matter). */
	inline int32 ShapeKey(const FStyle& Style)
	{
		return static_cast<int32>(Style.Body) * 4 + (Style.bTaxi ? 1 : 0) + (Style.bPolice ? 2 : 0);
	}
}
