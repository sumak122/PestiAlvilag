// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * A Budapest city bus after the MÁVAG "Harcsa" (catfish): a bonneted bus
 * in dark blue and white. Its round nose carries the chrome whiskers that
 * gave it the name, curved bars sweeping up and out from a V, between two
 * headlamps, with white mudguards over the front wheels and a chrome
 * bumper. Behind the bonnet: a split windscreen under the destination
 * board, the route number in a lit box on the roof, six windows a side,
 * the door on the left (kerb) side behind the front wheel, rear wheels
 * under trimmed arches and a luggage rail on the back half of the roof.
 * Inside: a driver's seat on the right, and pairs of seats each side of an
 * aisle, so you can see people sitting.
 *
 * Header-only (like TramModel) so it can be built and looked at outside the
 * engine. Frame: X forward, Y right, Z up; the ground is 150 cm below the
 * origin (the middle of the bus's collision box). The wheels are built
 * separately (BuildWheel), so the game can turn them.
 */
namespace BusModel
{
	/** One mesh section (and colour) each. */
	enum EPart : int32
	{
		/** The dark blue lower body and bonnet. */
		Blue,
		/** The white window band, roof and mudguards. */
		White,
		Chrome,
		Dark,
		/** Seats. */
		Leather,
		/** Headlamps and the lit number box. */
		Lamp,
		/** Window glass: see-through. */
		Glass,
		PartCount
	};

	constexpr double Ground = -150.0;
	constexpr double WheelRadius = 50.0;
	constexpr double AxleZ = Ground + WheelRadius;
	constexpr double FrontAxle = 300.0;
	constexpr double RearAxle = -250.0;
	constexpr double TrackHalf = 98.0;
	constexpr double TyreWidth = 24.0;

	constexpr double HalfWidth = 120.0;
	constexpr double Rear = -470.0;
	/** The front wall of the saloon, with the windscreen; the bonnet is ahead of it. */
	constexpr double Front = 230.0;
	constexpr double BodyBottom = -95.0;
	constexpr double FloorTop = -70.0;
	constexpr double Waist = 5.0;           // the window sills
	constexpr double WindowTop = 80.0;
	constexpr double RoofEdge = 100.0;      // where the roof starts to round over
	constexpr double RoofTop = 132.0;
	constexpr double Wall = 4.0;
	constexpr double DoorFrom = 125.0;      // the door, on the left side
	constexpr double DoorTo = 205.0;
	constexpr double SaloonFrom = Rear + 25.0;
	constexpr double NoseTip = 440.0;
	constexpr int32 SideWindows = 6;

	struct FPart
	{
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
	};

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
			// Unreal shows a triangle from the side it runs clockwise, so wind
			// against Normal's run (see TramModel).
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

		/** A box mirrored to both sides of the bus (Y and -Y). */
		void BothSides(EPart Part, const FVector& Min, const FVector& Max)
		{
			Box(Part, Min, Max);
			Box(Part, FVector(Min.X, -Max.Y, Min.Z), FVector(Max.X, -Min.Y, Max.Z));
		}

		/** A round bar or wheel: Axis 0 along X, 1 along Y, 2 along Z. */
		void Cylinder(EPart Part, const FVector& Centre, double Radius, double Length, int32 Axis, int32 Sides = 10)
		{
			auto Point = [&](double A, double S)
			{
				const double U = Radius * FMath::Cos(A);
				const double V = Radius * FMath::Sin(A);
				const double W = Length * 0.5 * S;
				return Axis == 0 ? Centre + FVector(W, U, V) : Axis == 1 ? Centre + FVector(U, W, V) : Centre + FVector(U, V, W);
			};
			const FVector Along = Axis == 0 ? FVector(1.0, 0.0, 0.0) : Axis == 1 ? FVector(0.0, 1.0, 0.0) : FVector(0.0, 0.0, 1.0);
			for (int32 i = 0; i < Sides; ++i)
			{
				const double A0 = 2.0 * PI * i / Sides;
				const double A1 = 2.0 * PI * (i + 1) / Sides;
				const FVector Out = (Point((A0 + A1) * 0.5, 0.0) - Centre).GetSafeNormal();
				Quad(Part, Point(A0, -1.0), Point(A1, -1.0), Point(A1, 1.0), Point(A0, 1.0), Out);
				Quad(Part, Centre + Along * (Length * 0.5), Centre + Along * (Length * 0.5), Point(A0, 1.0), Point(A1, 1.0), Along);
				Quad(Part, Centre - Along * (Length * 0.5), Centre - Along * (Length * 0.5), Point(A1, -1.0), Point(A0, -1.0), -Along);
			}
		}

		/** A square bar of this thickness from A to B. */
		void Bar(EPart Part, const FVector& A, const FVector& B, double Thickness, const FVector& Up = FVector(0.0, 0.0, 1.0))
		{
			const FVector Along = (B - A).GetSafeNormal();
			FVector Side = FVector::CrossProduct(Up, Along).GetSafeNormal();
			if (Side.IsNearlyZero())
			{
				Side = FVector::CrossProduct(FVector(0.0, 1.0, 0.0), Along).GetSafeNormal();
			}
			const FVector Top = FVector::CrossProduct(Along, Side);
			const double H = Thickness * 0.5;
			const FVector Corner[4] = { Side * H + Top * H, -Side * H + Top * H, -Side * H - Top * H, Side * H - Top * H };
			for (int32 k = 0; k < 4; ++k)
			{
				const FVector& C0 = Corner[k];
				const FVector& C1 = Corner[(k + 1) % 4];
				Quad(Part, A + C0, A + C1, B + C1, B + C0, (C0 + C1).GetSafeNormal());
			}
			Quad(Part, B + Corner[0], B + Corner[1], B + Corner[2], B + Corner[3], Along);
			Quad(Part, A + Corner[3], A + Corner[2], A + Corner[1], A + Corner[0], -Along);
		}

		/**
		 * The skin between two cross-sections at X0 and X1 (points in Y, Z,
		 * round the outline, the same number in each). Faces point away
		 * from Middle (Y, Z), which lies inside both outlines.
		 */
		void Loft(EPart Part, double X0, const TArray<FVector2D>& Ring0, double X1, const TArray<FVector2D>& Ring1, const FVector2D& Middle)
		{
			const int32 N = Ring0.Num();
			for (int32 i = 0; i < N; ++i)
			{
				const int32 j = (i + 1) % N;
				const FVector A(X0, Ring0[i].X, Ring0[i].Y);
				const FVector B(X0, Ring0[j].X, Ring0[j].Y);
				const FVector C(X1, Ring1[j].X, Ring1[j].Y);
				const FVector D(X1, Ring1[i].X, Ring1[i].Y);
				FVector Normal = FVector::CrossProduct(C - A, D - B).GetSafeNormal();
				const FVector2D Mid = (Ring0[i] + Ring0[j] + Ring1[i] + Ring1[j]) * 0.25;
				const FVector Out(0.0, Mid.X - Middle.X, Mid.Y - Middle.Y);
				if (FVector::DotProduct(Normal, Out) < 0.0)
				{
					Normal = -Normal;
				}
				Quad(Part, A, B, C, D, Normal);
			}
		}

		/** Closes an outline at X with a fan, facing Facing (+1 forward, -1 back). */
		void Cap(EPart Part, double X, const TArray<FVector2D>& Ring, const FVector2D& Middle, double Facing)
		{
			const FVector Centre(X, Middle.X, Middle.Y);
			for (int32 i = 0; i < Ring.Num(); ++i)
			{
				const FVector2D& P0 = Ring[i];
				const FVector2D& P1 = Ring[(i + 1) % Ring.Num()];
				Quad(Part, Centre, Centre, FVector(X, P0.X, P0.Y), FVector(X, P1.X, P1.Y), FVector(Facing, 0.0, 0.0));
			}
		}
	};

	/**
	 * A rounded outline in Y, Z: half width W, from Bottom to Top, the top
	 * corners rounded by TopRound and the bottom ones by BottomRound.
	 */
	inline TArray<FVector2D> Rounded(double W, double Bottom, double Top, double TopRound, double BottomRound)
	{
		TArray<FVector2D> Ring;
		constexpr int32 Steps = 5;
		auto Corner = [&](double CY, double CZ, double R, double From)
		{
			for (int32 s = 0; s <= Steps; ++s)
			{
				const double A = From + (PI * 0.5) * s / Steps;
				Ring.Add(FVector2D(CY + R * FMath::Cos(A), CZ + R * FMath::Sin(A)));
			}
		};
		Corner(W - BottomRound, Bottom + BottomRound, BottomRound, -PI * 0.5);
		Corner(W - TopRound, Top - TopRound, TopRound, 0.0);
		Corner(-W + TopRound, Top - TopRound, TopRound, PI * 0.5);
		Corner(-W + BottomRound, Bottom + BottomRound, BottomRound, PI);
		return Ring;
	}

	/** A digit drawn with bars, like a painted sign: Size tall, on a face at X facing +X (or -X if Facing < 0). */
	inline void Digit(FBuilder& B, EPart Part, int32 Value, double X, double Y, double Z, double Size, double Facing)
	{
		// Seven strokes: top, upper right, lower right, bottom, lower left, upper left, middle.
		static const uint8 Strokes[10] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };
		const double Wd = Size * 0.55;
		const double H = Size * 0.5;
		const double T = Size * 0.14;
		const double FaceX = X + Facing * 0.6;
		// Seen from the front of the face, +Y is to the left when facing +X.
		const double R = -Facing;
		auto Stroke = [&](double Y0, double Z0, double Y1, double Z1)
		{
			B.Box(Part, FVector(FMath::Min(X, FaceX), Y + R * FMath::Min(Y0, Y1) - T * 0.5, Z + FMath::Min(Z0, Z1) - T * 0.5),
				FVector(FMath::Max(X, FaceX), Y + R * FMath::Max(Y0, Y1) + T * 0.5, Z + FMath::Max(Z0, Z1) + T * 0.5));
		};
		const uint8 Mask = Strokes[FMath::Clamp(Value, 0, 9)];
		const double L = -Wd * 0.5;
		const double Rt = Wd * 0.5;
		if (Mask & 0x01) { Stroke(L, H, Rt, H); }
		if (Mask & 0x02) { Stroke(Rt, 0.0, Rt, H); }
		if (Mask & 0x04) { Stroke(Rt, -H, Rt, 0.0); }
		if (Mask & 0x08) { Stroke(L, -H, Rt, -H); }
		if (Mask & 0x10) { Stroke(L, -H, L, 0.0); }
		if (Mask & 0x20) { Stroke(L, 0.0, L, H); }
		if (Mask & 0x40) { Stroke(L, 0.0, Rt, 0.0); }
	}

	/** A one- or two-digit route number centred at Y, Z on a face at X. */
	inline void RouteNumber(FBuilder& B, EPart Part, int32 Route, double X, double Y, double Z, double Size, double Facing)
	{
		if (Route >= 10)
		{
			const double Gap = Size * 0.4;
			// Facing +X, the first digit is on the viewer's left: +Y.
			Digit(B, Part, Route / 10, X, Y - Facing * Gap, Z, Size, Facing);
			Digit(B, Part, Route % 10, X, Y + Facing * Gap, Z, Size, Facing);
		}
		else
		{
			Digit(B, Part, Route, X, Y, Z, Size, Facing);
		}
	}

	constexpr double ArchRadius = WheelRadius + 12.0;

	/** Height of the arch over a wheel at X along the bus, or BodyBottom away from the wheels. */
	inline double ArchTop(double X)
	{
		for (const double Axle : { FrontAxle, RearAxle })
		{
			const double Dx = X - Axle;
			if (FMath::Abs(Dx) < ArchRadius)
			{
				return AxleZ + FMath::Sqrt(ArchRadius * ArchRadius - Dx * Dx);
			}
		}
		return BodyBottom;
	}

	inline void BuildBody(FBuilder& B)
	{
		const double W = HalfWidth;

		// Chassis and floor.
		B.Box(Dark, FVector(Rear + 30.0, -70.0, AxleZ - 8.0), FVector(NoseTip - 60.0, 70.0, BodyBottom + 10.0));
		B.Box(Dark, FVector(Rear + Wall, -W + Wall, BodyBottom), FVector(Front, W - Wall, FloorTop));
		for (const double Axle : { FrontAxle, RearAxle })
		{
			B.Cylinder(Dark, FVector(Axle, 0.0, AxleZ), 7.0, 2.0 * TrackHalf, 1, 6);
		}

		// The sides below the windows: whole panels, and narrow strips round
		// the rear wheel arch so it comes out round. The door opening on the
		// left side is left out. (The front wheels stand ahead of the body.)
		const double ArchFrom = RearAxle - ArchRadius;
		const double ArchTo = RearAxle + ArchRadius;
		B.BothSides(Blue, FVector(Rear, W - Wall, BodyBottom), FVector(ArchFrom, W, Waist));
		B.Box(Blue, FVector(ArchTo, W - Wall, BodyBottom), FVector(Front, W, Waist));
		B.Box(Blue, FVector(ArchTo, -W, BodyBottom), FVector(DoorFrom, -W + Wall, Waist));
		B.Box(Blue, FVector(DoorTo, -W, BodyBottom), FVector(Front, -W + Wall, Waist));
		constexpr int32 ArchStrips = 40;
		const double Strip = (ArchTo - ArchFrom) / ArchStrips;
		for (int32 k = 0; k < ArchStrips; ++k)
		{
			const double X = ArchFrom + Strip * k;
			const double Bottom = ArchTop(X + Strip * 0.5);
			B.BothSides(Blue, FVector(X, W - Wall, Bottom), FVector(X + Strip, W, Waist));
		}
		// A white trim round the arch.
		constexpr int32 TrimSteps = 16;
		for (int32 k = 0; k < TrimSteps; ++k)
		{
			const double A0 = PI * k / TrimSteps;
			const double A1 = PI * (k + 1) / TrimSteps;
			for (const double Y : { W + 1.0, -W - 1.0 })
			{
				const FVector P0(RearAxle + ArchRadius * FMath::Cos(A0), Y, AxleZ + ArchRadius * FMath::Sin(A0) + 1.5);
				const FVector P1(RearAxle + ArchRadius * FMath::Cos(A1), Y, AxleZ + ArchRadius * FMath::Sin(A1) + 1.5);
				B.Bar(White, P0, P1, 4.0, FVector(0.0, 1.0, 0.0));
			}
		}
		// The rear and front walls below the windows.
		B.Box(Blue, FVector(Rear, -W, BodyBottom), FVector(Rear + Wall, W, Waist));
		B.Box(Blue, FVector(Front - Wall, -W, BodyBottom), FVector(Front, W, Waist));
		// A white line along the side and a chrome strip at the sills.
		B.BothSides(White, FVector(Rear, W, -40.0), FVector(DoorFrom, W + 1.0, -34.0));
		B.Box(White, FVector(DoorTo, W, -40.0), FVector(Front, W + 1.0, -34.0));
		B.Box(White, FVector(DoorTo, -W - 1.0, -40.0), FVector(Front, -W, -34.0));
		B.BothSides(Chrome, FVector(Rear, W, Waist - 3.0), FVector(Front, W + 1.5, Waist));
		B.Box(Chrome, FVector(Rear - 1.5, -W, Waist - 3.0), FVector(Rear, W, Waist));

		// The window band: white posts, glass between; the door on the left
		// is two tall glazed leaves, and a step below it.
		const double WindowFrom = SaloonFrom;
		const double WindowTo = DoorFrom - 10.0;
		const double Pane = (WindowTo - WindowFrom) / SideWindows;
		for (int32 k = 0; k <= SideWindows; ++k)
		{
			const double X = WindowFrom + Pane * k;
			B.BothSides(White, FVector(X - 5.0, W - Wall, Waist), FVector(X + 5.0, W, WindowTop));
		}
		B.BothSides(White, FVector(Rear, W - Wall, Waist), FVector(WindowFrom - 5.0, W, WindowTop));
		B.Box(White, FVector(WindowTo + 5.0, W - Wall, Waist), FVector(Front, W, WindowTop));
		B.Box(Glass, FVector(WindowTo + 5.0, W - Wall * 0.5 - 0.5, Waist + 2.0), FVector(Front - 12.0, W - Wall * 0.5 + 0.5, WindowTop - 3.0));
		B.Box(White, FVector(WindowTo + 5.0, -W, Waist), FVector(DoorFrom, -W + Wall, WindowTop));
		B.Box(White, FVector(DoorTo, -W, Waist), FVector(Front, -W + Wall, WindowTop));
		B.BothSides(Glass, FVector(WindowFrom, W - Wall * 0.5 - 0.5, Waist + 2.0), FVector(WindowTo, W - Wall * 0.5 + 0.5, WindowTop - 3.0));
		B.Box(Glass, FVector(DoorTo + 4.0, -W + Wall * 0.5 - 0.5, Waist + 2.0), FVector(Front - 12.0, -W + Wall * 0.5 + 0.5, WindowTop - 3.0));
		const double DoorMid = (DoorFrom + DoorTo) * 0.5;
		for (const double X : { DoorFrom, DoorMid - 2.0, DoorMid + 2.0, DoorTo - 4.0 })
		{
			B.Box(Blue, FVector(X, -W - 1.0, FloorTop - 20.0), FVector(X + 4.0, -W + 2.0, WindowTop));
		}
		B.Box(Blue, FVector(DoorFrom, -W - 1.0, FloorTop - 20.0), FVector(DoorTo, -W + 2.0, -20.0));
		B.Box(Glass, FVector(DoorFrom + 4.0, -W + 0.0, -16.0), FVector(DoorTo - 4.0, -W + 1.0, WindowTop - 3.0));
		B.Box(Chrome, FVector(DoorFrom + 2.0, -W - 6.0, BodyBottom - 2.0), FVector(DoorTo - 2.0, -W + 6.0, BodyBottom + 3.0));
		// The rear: a small window high up, a lamp each side.
		B.Box(White, FVector(Rear, -W, Waist), FVector(Rear + Wall, -38.0, WindowTop));
		B.Box(White, FVector(Rear, 38.0, Waist), FVector(Rear + Wall, W, WindowTop));
		B.Box(White, FVector(Rear, -38.0, Waist), FVector(Rear + Wall, 38.0, 30.0));
		B.Box(Glass, FVector(Rear + 1.5, -38.0, 30.0), FVector(Rear + 2.5, 38.0, WindowTop - 3.0));
		for (const double Y : { -95.0, 95.0 })
		{
			B.Box(Lamp, FVector(Rear - 3.0, Y - 6.0, -60.0), FVector(Rear, Y + 6.0, -50.0));
		}
		B.Box(Chrome, FVector(Rear - 10.0, -W + 5.0, BodyBottom - 4.0), FVector(Rear - 2.0, W - 5.0, BodyBottom + 6.0));

		// The windscreen, split down the middle, under the destination board.
		B.Box(White, FVector(Front - Wall, -W, Waist), FVector(Front, -W + 12.0, WindowTop));
		B.Box(White, FVector(Front - Wall, W - 12.0, Waist), FVector(Front, W, WindowTop));
		B.Box(White, FVector(Front - Wall, -4.0, Waist), FVector(Front, 4.0, WindowTop));
		B.Box(Glass, FVector(Front - 2.5, -W + 12.0, Waist + 2.0), FVector(Front - 1.5, W - 12.0, WindowTop - 2.0));
		B.Box(Dark, FVector(Front - 1.0, -70.0, WindowTop + 2.0), FVector(Front + 3.0, 70.0, WindowTop + 18.0));
		B.Box(White, FVector(Front + 3.0, -64.0, WindowTop + 5.0), FVector(Front + 3.5, 64.0, WindowTop + 15.0));
		for (int32 k = 0; k < 9; ++k)
		{
			// The place names on the board, too small to read from the street.
			const double Y = -58.0 + k * 13.5;
			B.Box(Dark, FVector(Front + 3.5, Y, WindowTop + 7.0), FVector(Front + 4.0, Y + (k == 4 ? 4.0 : 10.0), WindowTop + 13.0));
		}

		// The upper sides and the roof, rounding over at the edges.
		B.BothSides(White, FVector(Rear, W - Wall, WindowTop), FVector(Front, W, RoofEdge));
		B.Box(White, FVector(Rear, -W, WindowTop), FVector(Rear + Wall, W, RoofEdge));
		B.Box(White, FVector(Front - Wall, -W, WindowTop), FVector(Front, W, RoofEdge));
		{
			TArray<FVector2D> Roof;
			constexpr int32 Steps = 6;
			const double R = RoofTop - RoofEdge;
			for (int32 s = 0; s <= Steps; ++s)
			{
				const double A = (PI * 0.5) * s / Steps;
				Roof.Add(FVector2D(W - R + R * FMath::Cos(A), RoofEdge + R * FMath::Sin(A)));
			}
			for (int32 s = 0; s <= Steps; ++s)
			{
				const double A = PI * 0.5 + (PI * 0.5) * s / Steps;
				Roof.Add(FVector2D(-W + R + R * FMath::Cos(A), RoofEdge + R * FMath::Sin(A)));
			}
			const FVector2D Middle(0.0, RoofEdge);
			// The roof sweeps down a little at the front and the back.
			TArray<FVector2D> Lower = Roof;
			for (FVector2D& P : Lower)
			{
				P.Y = RoofEdge + (P.Y - RoofEdge) * 0.6;
			}
			B.Loft(White, Rear, Lower, Rear + 30.0, Roof, Middle);
			B.Loft(White, Rear + 30.0, Roof, Front - 15.0, Roof, Middle);
			B.Loft(White, Front - 15.0, Roof, Front + 4.0, Lower, Middle);
			B.Cap(White, Rear, Lower, Middle, -1.0);
			B.Cap(White, Front + 4.0, Lower, Middle, 1.0);
			B.Box(White, FVector(Rear, -W, RoofEdge - 1.0), FVector(Front + 4.0, W, RoofEdge + 1.0));
		}

		// The route number box on a stalk at the front of the roof, lit,
		// with the number on both faces; a rain strip and the luggage rail.
		B.Box(Dark, FVector(Front - 30.0, -4.0, RoofTop - 2.0), FVector(Front - 22.0, 4.0, RoofTop + 14.0));
		B.Box(Dark, FVector(Front - 42.0, -30.0, RoofTop + 14.0), FVector(Front - 10.0, 30.0, RoofTop + 50.0));
		B.Box(Lamp, FVector(Front - 10.0, -26.0, RoofTop + 17.0), FVector(Front - 9.0, 26.0, RoofTop + 47.0));
		B.Box(Lamp, FVector(Front - 43.0, -26.0, RoofTop + 17.0), FVector(Front - 42.0, 26.0, RoofTop + 47.0));
		B.BothSides(Chrome, FVector(Rear + 20.0, W - 6.0, RoofEdge + 8.0), FVector(Front - 10.0, W - 4.0, RoofEdge + 10.0));
		for (int32 k = 0; k <= 6; ++k)
		{
			const double X = Rear + 30.0 + k * 40.0;
			B.BothSides(Chrome, FVector(X - 1.5, 76.0, RoofTop - 6.0), FVector(X + 1.5, 79.0, RoofTop + 12.0));
		}
		B.BothSides(Chrome, FVector(Rear + 28.0, 76.0, RoofTop + 10.0), FVector(Rear + 272.0, 79.0, RoofTop + 13.0));
		B.Box(Chrome, FVector(Rear + 28.0, -79.0, RoofTop + 10.0), FVector(Rear + 31.0, 79.0, RoofTop + 13.0));
		B.Box(Chrome, FVector(Rear + 269.0, -79.0, RoofTop + 10.0), FVector(Rear + 272.0, 79.0, RoofTop + 13.0));
	}

	/** One cross-section of the bonnet: a rounded outline at X. */
	struct FNoseSection
	{
		double X;
		double W;
		double Bottom;
		double Top;
		double Round;
	};

	/** The bonnet from the windscreen to the tip of the nose, which is broad and round. */
	constexpr int32 NumNoseSections = 6;
	inline const FNoseSection& NoseSection(int32 Index)
	{
		static const FNoseSection Sections[NumNoseSections] = {
			{ Front, 78.0, -95.0, 8.0, 30.0 },
			{ 330.0, 76.0, -95.0, 3.0, 32.0 },
			{ 392.0, 72.0, -94.0, -6.0, 36.0 },
			{ 418.0, 66.0, -92.0, -16.0, 36.0 },
			{ 432.0, 56.0, -90.0, -28.0, 30.0 },
			{ NoseTip, 42.0, -88.0, -38.0, 22.0 },
		};
		return Sections[Index];
	}

	/** How far forward the nose's skin is at Y, Z (the first X from the front inside it). */
	inline double NoseSurfaceX(double Y, double Z)
	{
		for (double X = NoseTip; X > Front; X -= 0.5)
		{
			for (int32 i = 0; i + 1 < NumNoseSections; ++i)
			{
				const FNoseSection& A = NoseSection(i);
				const FNoseSection& C = NoseSection(i + 1);
				if (X < A.X || X > C.X)
				{
					continue;
				}
				const double T = (X - A.X) / (C.X - A.X);
				const double W = A.W + (C.W - A.W) * T;
				const double Bottom = A.Bottom + (C.Bottom - A.Bottom) * T;
				const double Top = A.Top + (C.Top - A.Top) * T;
				const double R = A.Round + (C.Round - A.Round) * T;
				// Inside the rounded outline (top corners rounded by R)?
				const double Ay = FMath::Abs(Y);
				if (Ay > W || Z < Bottom || Z > Top)
				{
					continue;
				}
				const double Dy = Ay - (W - R);
				const double Dz = Z - (Top - R);
				if (Dy > 0.0 && Dz > 0.0 && Dy * Dy + Dz * Dz > R * R)
				{
					continue;
				}
				return X;
			}
		}
		return Front;
	}

	/** How many chrome whiskers the grille has on each side, and how many pieces each is made of. */
	constexpr int32 Whiskers = 9;
	constexpr int32 WhiskerSteps = 12;

	/** The bonnet and its catfish nose, the mudguards, lamps and bumper. */
	inline void BuildNose(FBuilder& B)
	{
		const FVector2D Middle(0.0, -55.0);
		TArray<TArray<FVector2D>> Rings;
		for (int32 i = 0; i < NumNoseSections; ++i)
		{
			const FNoseSection& S = NoseSection(i);
			Rings.Add(Rounded(S.W, S.Bottom, S.Top, S.Round, 6.0));
		}
		for (int32 i = 0; i + 1 < Rings.Num(); ++i)
		{
			B.Loft(Blue, NoseSection(i).X, Rings[i], NoseSection(i + 1).X, Rings[i + 1], Middle);
		}
		B.Cap(Blue, NoseTip, Rings.Last(), Middle, 1.0);
		// The bonnet's centre seam and the badge at the top of the whiskers.
		for (int32 i = 0; i + 2 < NumNoseSections; ++i)
		{
			B.Bar(Chrome, FVector(NoseSection(i).X, 0.0, NoseSection(i).Top + 1.0), FVector(NoseSection(i + 1).X, 0.0, NoseSection(i + 1).Top + 1.0), 2.5);
		}
		const double BadgeZ = NoseSection(NumNoseSections - 2).Top - 6.0;
		const double BadgeX = NoseSurfaceX(0.0, BadgeZ);
		B.Box(White, FVector(BadgeX - 4.0, -6.0, BadgeZ - 5.0), FVector(BadgeX + 1.5, 6.0, BadgeZ + 5.0));

		// The whiskers: thin chrome bars standing close together low in the
		// middle of the nose and fanning out like a fountain as they rise,
		// the outer ones bending out and down beside the lamps. Between them
		// the dark grille shows, laid on the nose's skin strip by strip.
		const auto WhiskerPoint = [](int32 k, double Sign, int32 s, double Lift)
		{
			const double Spread = (k + 0.5) / Whiskers;
			const double T = static_cast<double>(s) / WhiskerSteps;
			const double Z = -86.0 + 54.0 * T * (1.0 - 0.45 * Spread * Spread * Spread) - 14.0 * Spread * Spread * T * T * T;
			const double Y = Sign * (2.5 + 30.0 * Spread + 25.0 * Spread * T * T);
			return FVector(NoseSurfaceX(Y, Z) + Lift, Y, Z);
		};
		for (int32 k = 0; k < Whiskers; ++k)
		{
			for (const double Sign : { -1.0, 1.0 })
			{
				for (int32 s = 1; s <= WhiskerSteps; ++s)
				{
					B.Bar(Chrome, WhiskerPoint(k, Sign, s - 1, 1.0), WhiskerPoint(k, Sign, s, 1.0), 2.2, FVector(1.0, 0.0, 0.0));
					// The grille between this whisker and the next one in (or the other side's first).
					const int32 Inner = k == 0 ? 0 : k - 1;
					const double InnerSign = k == 0 ? -Sign : Sign;
					B.Quad(Dark, WhiskerPoint(Inner, InnerSign, s - 1, 0.4), WhiskerPoint(k, Sign, s - 1, 0.4), WhiskerPoint(k, Sign, s, 0.4), WhiskerPoint(Inner, InnerSign, s, 0.4), FVector(1.0, 0.0, 0.0));
				}
			}
		}
		// A straight bar up the middle to the badge.
		B.Bar(Chrome, FVector(NoseSurfaceX(0.0, -86.0) + 1.0, 0.0, -86.0), FVector(NoseSurfaceX(0.0, BadgeZ - 5.0) + 1.0, 0.0, BadgeZ - 5.0), 2.6, FVector(1.0, 0.0, 0.0));

		// Headlamps in chrome rims either side of the nose.
		for (const double Y : { -70.0, 70.0 })
		{
			const double LampX = NoseSurfaceX(Y * 0.9, -48.0);
			B.Cylinder(Chrome, FVector(LampX - 2.0, Y, -48.0), 13.0, 14.0, 0, 14);
			B.Cylinder(Lamp, FVector(LampX + 5.5, Y, -48.0), 10.5, 1.5, 0, 14);
		}

		// The front mudguards: white shells arching over the wheels and
		// running back into a step under the door.
		for (const double Sign : { -1.0, 1.0 })
		{
			const double Y0 = Sign * (TrackHalf - 20.0);
			const double Y1 = Sign * (TrackHalf + 20.0);
			constexpr int32 Steps = 14;
			const double R = WheelRadius + 10.0;
			FVector PrevIn, PrevOut, PrevInTop, PrevOutTop;
			for (int32 s = 0; s <= Steps; ++s)
			{
				// From behind the wheel (low) over the top to the front (low).
				const double A = PI * (0.95 - 0.95 * s / Steps) + 0.05;
				const double Swell = 4.0 * FMath::Sin(PI * s / Steps);
				const FVector Dir(FMath::Cos(A), 0.0, FMath::Sin(A));
				const FVector C(FrontAxle, 0.0, AxleZ);
				const FVector In = C + Dir * R + FVector(0.0, Y0, 0.0);
				const FVector Out = C + Dir * R + FVector(0.0, Y1, 0.0);
				const FVector InTop = In + Dir * (7.0 + Swell);
				const FVector OutTop = Out + Dir * (7.0 + Swell) + FVector(0.0, Sign * 3.0, 0.0);
				if (s > 0)
				{
					const FVector Up = Dir;
					B.Quad(White, PrevInTop, PrevOutTop, OutTop, InTop, Up);
					B.Quad(White, PrevIn, PrevOut, Out, In, -Up);
					B.Quad(White, PrevOut, PrevOutTop, OutTop, Out, FVector(0.0, Sign, 0.0));
					B.Quad(White, PrevIn, PrevInTop, InTop, In, FVector(0.0, -Sign, 0.0));
				}
				PrevIn = In;
				PrevOut = Out;
				PrevInTop = InTop;
				PrevOutTop = OutTop;
			}
			// The apron between the mudguard and the bonnet side.
			B.Box(White, FVector(FrontAxle - 50.0, Sign > 0 ? 60.0 : -TrackHalf, AxleZ + 40.0),
				FVector(FrontAxle + 45.0, Sign > 0 ? TrackHalf : -60.0, AxleZ + 50.0));
		}

		// The bumper: a chrome bar on two brackets, with two upright guards.
		B.Box(Chrome, FVector(NoseTip + 2.0, -108.0, -106.0), FVector(NoseTip + 9.0, 108.0, -94.0));
		for (const double Y : { -108.0, 108.0 })
		{
			B.Box(Chrome, FVector(NoseTip - 6.0, Y - 4.0, -106.0), FVector(NoseTip + 9.0, Y + 4.0, -94.0));
		}
		for (const double Y : { -36.0, 36.0 })
		{
			B.Box(Chrome, FVector(NoseTip - 25.0, Y - 3.0, -102.0), FVector(NoseTip + 2.0, Y + 3.0, -97.0));
			B.Box(Chrome, FVector(NoseTip + 9.0, Y - 3.5, -110.0), FVector(NoseTip + 12.0, Y + 3.5, -86.0));
		}
		// The number plate under the whiskers.
		B.Box(White, FVector(NoseTip - 4.0, -20.0, -93.0), FVector(NoseTip + 2.0, 20.0, -85.0));
		B.Box(Dark, FVector(NoseTip + 2.0, -17.0, -91.5), FVector(NoseTip + 2.5, 17.0, -86.5));
	}

	/** The driver's place (right) and the seats in pairs each side of the aisle. */
	inline void BuildInside(FBuilder& B)
	{
		const double W = HalfWidth;
		// Driver's seat and steering wheel, behind the right windscreen.
		B.Box(Leather, FVector(Front - 95.0, 45.0, FloorTop), FVector(Front - 55.0, 95.0, FloorTop + 45.0));
		B.Box(Leather, FVector(Front - 100.0, 45.0, FloorTop + 45.0), FVector(Front - 92.0, 95.0, FloorTop + 100.0));
		B.Cylinder(Dark, FVector(Front - 30.0, 70.0, Waist - 10.0), 20.0, 3.0, 0, 12);
		B.Bar(Dark, FVector(Front - 30.0, 70.0, Waist - 10.0), FVector(Front - 5.0, 70.0, FloorTop + 10.0), 5.0, FVector(0.0, 1.0, 0.0));
		// A rail behind the driver.
		B.Box(Chrome, FVector(Front - 112.0, 10.0, FloorTop), FVector(Front - 108.0, 14.0, WindowTop));

		for (double X = SaloonFrom + 15.0; X < DoorFrom - 40.0; X += 78.0)
		{
			for (const double Sign : { -1.0, 1.0 })
			{
				const double Y0 = Sign > 0 ? 25.0 : -W + Wall + 2.0;
				const double Y1 = Sign > 0 ? W - Wall - 2.0 : -25.0;
				B.Box(Leather, FVector(X + 5.0, Y0, FloorTop + 38.0), FVector(X + 45.0, Y1, FloorTop + 46.0));
				B.Box(Dark, FVector(X + 15.0, Y0 + 10.0, FloorTop), FVector(X + 35.0, Y1 - 10.0, FloorTop + 38.0));
				B.Box(Leather, FVector(X, Y0, FloorTop + 46.0), FVector(X + 6.0, Y1, FloorTop + 100.0));
				B.Box(Chrome, FVector(X - 1.0, Sign > 0 ? Y0 - 1.0 : Y1 - 3.0, FloorTop + 98.0), FVector(X + 5.0, Sign > 0 ? Y0 + 3.0 : Y1 + 1.0, FloorTop + 104.0));
			}
		}
		// Ceiling lamps.
		for (const double X : { -300.0, -100.0, 100.0 })
		{
			B.Box(Lamp, FVector(X - 10.0, -8.0, RoofEdge + 22.0), FVector(X + 10.0, 8.0, RoofEdge + 26.0));
		}
	}

	/** The route number on the roof box, front and back. */
	inline void BuildRouteNumber(FBuilder& B, int32 Route)
	{
		RouteNumber(B, Dark, Route, Front - 9.0, 0.0, RoofTop + 32.0, 22.0, 1.0);
		RouteNumber(B, Dark, Route, Front - 43.0, 0.0, RoofTop + 32.0, 22.0, -1.0);
	}

	inline void Build(FBuilder& B, int32 Route)
	{
		BuildBody(B);
		BuildNose(B);
		BuildInside(B);
		BuildRouteNumber(B, Route);
	}

	/** Where each wheel's hub is: front left, front right, rear left, rear right. */
	inline FVector Hub(int32 Index)
	{
		const double X = Index < 2 ? FrontAxle : RearAxle;
		const double Y = (Index % 2 == 0 ? -1.0 : 1.0) * TrackHalf;
		return FVector(X, Y, AxleZ);
	}

	/** One wheel round its hub (the origin), axle along Y: tyre, white disc, chrome cap. */
	inline void BuildWheel(FBuilder& B)
	{
		B.Cylinder(Dark, FVector::ZeroVector, WheelRadius, TyreWidth, 1, 18);
		for (const double Side : { -1.0, 1.0 })
		{
			B.Cylinder(White, FVector(0.0, Side * (TyreWidth * 0.5 + 0.5), 0.0), WheelRadius - 14.0, 1.5, 1, 18);
			B.Cylinder(Chrome, FVector(0.0, Side * (TyreWidth * 0.5 + 3.0), 0.0), 11.0, 5.0, 1, 10);
			for (int32 k = 0; k < 8; ++k)
			{
				const double A = 2.0 * PI * k / 8.0;
				B.Cylinder(Dark, FVector(17.0 * FMath::Cos(A), Side * (TyreWidth * 0.5 + 1.6), 17.0 * FMath::Sin(A)), 2.0, 1.0, 1, 6);
			}
		}
	}
}
