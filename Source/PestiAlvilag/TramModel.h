// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * The tram's body, made from boxes and cylinders: a yellow two-axle
 * Budapest car of the early 1930s with an open platform at each end (the
 * steps and a grab pole each side, the controller at the end window), a
 * saloon between with six windows a side, a clerestory roof and a bow
 * collector. Inside: a plank floor, a wooden bench along each side, leather
 * straps on brass rails and ceiling lamps, so you can see in and sit down.
 *
 * Header-only (like PersonKit) so it can be built and looked at outside
 * the engine. Frame: X forward, Y right, Z up; the ground is 165 cm below
 * the origin (the middle of the tram's collision box).
 */
namespace TramModel
{
	/** One mesh section (and colour) each. */
	enum EPart : int32
	{
		Livery,
		Wood,
		Dark,
		Roof,
		Lamp,
		Brass,
		Leather,
		/** Window glass: see-through. */
		Glass,
		PartCount
	};

	constexpr double Ground = -165.0;
	constexpr double FloorTop = -95.0;
	constexpr double Waist = 0.0;          // the window sills
	constexpr double WindowTop = 85.0;
	constexpr double RoofBottom = 110.0;
	constexpr double HalfLength = 495.0;
	constexpr double HalfWidth = 110.0;
	constexpr double Wall = 4.0;
	constexpr double SaloonEnd = 330.0;    // the bulkheads between saloon and platforms
	constexpr double DoorFrom = 360.0;     // the platform steps, both sides, both ends
	constexpr double DoorTo = 445.0;
	constexpr double SeatTop = FloorTop + 48.0;
	constexpr double SeatFront = 60.0;     // bench edge, from the middle of the car
	constexpr int32 SaloonWindows = 6;

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
			// Facing is the way A, B, C, D run round (right hand rule, from the
			// diagonals so a quad with two corners the same still works).
			// Unreal shows a triangle from the side it runs clockwise, so wind
			// against Normal's run.
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

		/** A box mirrored to both sides of the car (Y and -Y). */
		void BothSides(EPart Part, const FVector& Min, const FVector& Max)
		{
			Box(Part, Min, Max);
			Box(Part, FVector(Min.X, -Max.Y, Min.Z), FVector(Max.X, -Min.Y, Max.Z));
		}

		/** A box mirrored to both ends of the car (X and -X). */
		void BothEnds(EPart Part, const FVector& Min, const FVector& Max)
		{
			Box(Part, Min, Max);
			Box(Part, FVector(-Max.X, Min.Y, Min.Z), FVector(-Min.X, Max.Y, Max.Z));
		}

		/** Both sides and both ends. */
		void FourWays(EPart Part, const FVector& Min, const FVector& Max)
		{
			BothSides(Part, Min, Max);
			BothSides(Part, FVector(-Max.X, Min.Y, Min.Z), FVector(-Min.X, Max.Y, Max.Z));
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
				// The ends, as fans of quads to the middle.
				Quad(Part, Centre + Along * (Length * 0.5), Centre + Along * (Length * 0.5), Point(A0, 1.0), Point(A1, 1.0), Along);
				Quad(Part, Centre - Along * (Length * 0.5), Centre - Along * (Length * 0.5), Point(A1, -1.0), Point(A0, -1.0), -Along);
			}
		}
	};

	inline void BuildShell(FBuilder& B)
	{
		const double H = HalfLength;
		const double W = HalfWidth;

		// Running gear and the floor.
		B.Box(Dark, FVector(-H + 35.0, -W + 15.0, -135.0), FVector(H - 35.0, W - 15.0, FloorTop - 6.0));
		for (const double X : { -250.0, 250.0 })
		{
			for (const double Y : { -80.0, 80.0 })
			{
				B.Cylinder(Dark, FVector(X, Y, Ground + 40.0), 40.0, 9.0, 1, 14);
			}
			B.Cylinder(Dark, FVector(X, 0.0, Ground + 40.0), 7.0, 170.0, 1, 6);
		}
		B.Box(Wood, FVector(-H + Wall, -W + Wall, FloorTop - 6.0), FVector(H - Wall, W - Wall, FloorTop));
		// The lifeguard tray and the coupler at each end.
		B.BothEnds(Dark, FVector(H - 10.0, -70.0, Ground + 18.0), FVector(H + 15.0, 70.0, Ground + 26.0));
		B.BothEnds(Dark, FVector(H, -10.0, FloorTop - 40.0), FVector(H + 25.0, 10.0, FloorTop - 25.0));
		// The step under each platform opening.
		B.FourWays(Dark, FVector(DoorFrom, W, FloorTop - 42.0), FVector(DoorTo, W + 22.0, FloorTop - 36.0));

		// The sides below the windows: open at the platform steps.
		for (const double Side : { 1.0, -1.0 })
		{
			const double Y0 = Side > 0 ? W - Wall : -W;
			const double Y1 = Side > 0 ? W : -W + Wall;
			B.Box(Livery, FVector(-H, Y0, FloorTop - 6.0), FVector(-DoorTo, Y1, Waist));
			B.Box(Livery, FVector(-DoorFrom, Y0, FloorTop - 6.0), FVector(DoorFrom, Y1, Waist));
			B.Box(Livery, FVector(DoorTo, Y0, FloorTop - 6.0), FVector(H, Y1, Waist));
		}
		// The dash at each end, with its headlamp.
		B.BothEnds(Livery, FVector(H - Wall, -W + Wall, FloorTop - 6.0), FVector(H, W - Wall, Waist));
		B.BothEnds(Lamp, FVector(H, -12.0, FloorTop + 30.0), FVector(H + 6.0, 12.0, FloorTop + 54.0));
		B.BothEnds(Dark, FVector(H - 1.0, -W, Waist - 5.0), FVector(H + 3.0, W, Waist + 3.0));

		// The saloon windows: wooden pillars, a sill, a sash rail; glass in
		// the lower sash, the top lights open.
		const double Pane = 2.0 * SaloonEnd / SaloonWindows;
		for (int32 k = 0; k <= SaloonWindows; ++k)
		{
			const double X = -SaloonEnd + Pane * k;
			B.BothSides(Livery, FVector(X - 6.0, W - Wall, Waist), FVector(X + 6.0, W, WindowTop));
		}
		B.BothSides(Wood, FVector(-SaloonEnd, W - Wall - 3.0, Waist - 3.0), FVector(SaloonEnd, W + 1.0, Waist + 3.0));
		B.BothSides(Wood, FVector(-SaloonEnd, W - Wall, 55.0), FVector(SaloonEnd, W, 60.0));
		B.BothSides(Glass, FVector(-SaloonEnd, W - Wall * 0.5 - 0.5, Waist + 3.0), FVector(SaloonEnd, W - Wall * 0.5 + 0.5, 55.0));

		// The platforms: corner posts, a post each side of the steps, grab
		// poles, and the three end windows with the middle one glazed.
		B.FourWays(Livery, FVector(H - 12.0, W - Wall, Waist), FVector(H, W, WindowTop));
		B.FourWays(Livery, FVector(DoorTo, W - Wall, Waist), FVector(DoorTo + 8.0, W, WindowTop));
		B.FourWays(Livery, FVector(SaloonEnd, W - Wall, Waist), FVector(DoorFrom, W, WindowTop));
		for (const double X : { DoorFrom + 4.0, DoorTo - 4.0 })
		{
			for (const double Y : { W - 8.0, -W + 8.0 })
			{
				B.Cylinder(Brass, FVector(X, Y, (FloorTop + WindowTop) * 0.5), 2.5, WindowTop - FloorTop, 2, 6);
				B.Cylinder(Brass, FVector(-X, Y, (FloorTop + WindowTop) * 0.5), 2.5, WindowTop - FloorTop, 2, 6);
			}
		}
		for (const double Y : { -38.0, 38.0 })
		{
			B.BothEnds(Livery, FVector(H - Wall, Y - 5.0, Waist), FVector(H, Y + 5.0, WindowTop));
		}
		B.BothEnds(Glass, FVector(H - 2.5, -33.0, Waist + 2.0), FVector(H - 1.5, 33.0, WindowTop - 2.0));
		// The controller and the brake handle at each end window.
		B.BothEnds(Dark, FVector(H - 40.0, -22.0, FloorTop), FVector(H - 18.0, 0.0, FloorTop + 95.0));
		B.BothEnds(Brass, FVector(H - 42.0, -14.0, FloorTop + 95.0), FVector(H - 30.0, -10.0, FloorTop + 100.0));
		B.BothEnds(Dark, FVector(H - 30.0, 20.0, FloorTop), FVector(H - 24.0, 26.0, FloorTop + 90.0));

		// The bulkheads between platform and saloon, with a doorway.
		for (const double Sign : { 1.0, -1.0 })
		{
			const double X0 = Sign > 0 ? SaloonEnd : -SaloonEnd - Wall;
			const double X1 = X0 + Wall;
			B.Box(Wood, FVector(X0, -W + Wall, FloorTop), FVector(X1, -36.0, WindowTop));
			B.Box(Wood, FVector(X0, 36.0, FloorTop), FVector(X1, W - Wall, WindowTop));
			B.Box(Wood, FVector(X0, -36.0, 75.0), FVector(X1, 36.0, WindowTop));
			B.Box(Wood, FVector(X0, -W + Wall, WindowTop), FVector(X1, W - Wall, RoofBottom));
		}

		// The letterboard over the windows, the roof and the clerestory with
		// its row of little lights, the destination box at each end and the
		// bow collector on its frame.
		B.BothSides(Livery, FVector(-H, W - Wall, WindowTop), FVector(H, W, RoofBottom));
		B.BothEnds(Livery, FVector(H - Wall, -W + Wall, WindowTop), FVector(H, W - Wall, RoofBottom));
		B.Box(Roof, FVector(-H - 12.0, -W - 6.0, RoofBottom), FVector(H + 12.0, W + 6.0, RoofBottom + 8.0));
		B.Box(Roof, FVector(-SaloonEnd, -58.0, RoofBottom + 8.0), FVector(SaloonEnd, 58.0, RoofBottom + 30.0));
		B.Box(Roof, FVector(-SaloonEnd - 10.0, -64.0, RoofBottom + 30.0), FVector(SaloonEnd + 10.0, 64.0, RoofBottom + 35.0));
		for (int32 k = 0; k < 10; ++k)
		{
			const double X = -SaloonEnd + 30.0 + k * (2.0 * SaloonEnd - 60.0) / 9.0;
			B.BothSides(Dark, FVector(X - 14.0, 58.0, RoofBottom + 13.0), FVector(X + 14.0, 59.0, RoofBottom + 25.0));
		}
		B.BothEnds(Dark, FVector(H - 30.0, -45.0, RoofBottom + 8.0), FVector(H - 4.0, 45.0, RoofBottom + 32.0));
		B.BothEnds(Lamp, FVector(H - 4.0, -38.0, RoofBottom + 12.0), FVector(H - 2.0, 38.0, RoofBottom + 28.0));
		B.Box(Dark, FVector(-40.0, -40.0, RoofBottom + 35.0), FVector(40.0, 40.0, RoofBottom + 42.0));
		for (const double Y : { -30.0, 30.0 })
		{
			B.Box(Dark, FVector(-4.0, Y - 3.0, RoofBottom + 42.0), FVector(4.0, Y + 3.0, RoofBottom + 95.0));
		}
		B.Box(Brass, FVector(-5.0, -75.0, RoofBottom + 95.0), FVector(5.0, 75.0, RoofBottom + 100.0));
	}

	inline void BuildInside(FBuilder& B)
	{
		const double W = HalfWidth;
		// A wooden bench along each side of the saloon: seat, front board and back.
		B.BothSides(Wood, FVector(-SaloonEnd + 8.0, SeatFront, SeatTop - 5.0), FVector(SaloonEnd - 8.0, W - Wall, SeatTop));
		B.BothSides(Wood, FVector(-SaloonEnd + 8.0, SeatFront + 2.0, FloorTop), FVector(SaloonEnd - 8.0, SeatFront + 6.0, SeatTop - 5.0));
		B.BothSides(Wood, FVector(-SaloonEnd + 8.0, W - Wall - 6.0, SeatTop), FVector(SaloonEnd - 8.0, W - Wall, Waist - 4.0));
		// Lining boards inside the saloon, under the windows.
		B.BothSides(Wood, FVector(-SaloonEnd, W - Wall - 1.0, Waist - 4.0), FVector(SaloonEnd, W - Wall, Waist));

		// Brass rails along the ceiling with leather straps hanging.
		for (const double Y : { -42.0, 42.0 })
		{
			B.Cylinder(Brass, FVector(0.0, Y, RoofBottom - 14.0), 2.0, 2.0 * SaloonEnd - 20.0, 0, 6);
			for (double X = -SaloonEnd + 35.0; X < SaloonEnd - 20.0; X += 45.0)
			{
				B.Box(Leather, FVector(X - 1.5, Y - 1.0, RoofBottom - 40.0), FVector(X + 1.5, Y + 1.0, RoofBottom - 14.0));
				B.Box(Leather, FVector(X - 6.0, Y - 1.5, RoofBottom - 44.0), FVector(X + 6.0, Y + 1.5, RoofBottom - 40.0));
			}
		}
		// Ceiling lamps.
		for (const double X : { -200.0, 0.0, 200.0 })
		{
			B.Box(Lamp, FVector(X - 10.0, -10.0, RoofBottom - 8.0), FVector(X + 10.0, 10.0, RoofBottom));
		}
	}

	inline void Build(FBuilder& B)
	{
		BuildShell(B);
		BuildInside(B);
	}

	/** A place on the benches, facing across the aisle. */
	struct FSeat
	{
		FVector At;
		double Yaw;
	};

	constexpr int32 SeatsPerSide = 6;
	constexpr int32 NumSeats = SeatsPerSide * 2;

	/**
	 * Seat Index: where a seated person's middle goes (PersonModel's
	 * seated build: the feet on the floor) and which way they face.
	 * SeatedMiddle is how high their middle is above the floor.
	 */
	inline FSeat Seat(int32 Index, double SeatedMiddle)
	{
		const double Side = Index < SeatsPerSide ? 1.0 : -1.0;
		const int32 Along = Index % SeatsPerSide;
		const double Spacing = (2.0 * SaloonEnd - 80.0) / (SeatsPerSide - 1);
		const double X = -SaloonEnd + 40.0 + Spacing * Along;
		// Hips over the middle of the bench seat.
		const double Y = Side * (SeatFront + (HalfWidth - Wall - SeatFront) * 0.45);
		return { FVector(X, Y, FloorTop + SeatedMiddle), Side > 0 ? -90.0 : 90.0 };
	}
}
