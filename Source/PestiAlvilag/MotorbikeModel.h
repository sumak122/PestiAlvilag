// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BicycleModel.h"
#include "PersonKit.h"
#include "PoliceModel.h"

/**
 * A Budapest-built Méray motorcycle of the early thirties, with a sidecar
 * on its left (the kerb side: Pest kept left in 1932): rigid black frame,
 * saddle tank with gold lines, a finned single cylinder, fishtail exhaust,
 * sprung saddle, girder fork with a big headlamp, and a lacquered torpedo
 * sidecar on its own wheel. Built in the floor frame of the whole outfit
 * (X forward, Y right, Z up from the road, the middle of the outfit at
 * X = Y = 0). Without the sidecar (bSidecar false) it is the solo machine.
 */
namespace MotorbikeModel
{
	using PersonKit::FPiece;

	enum EPart : int32 { Body, Steer, WheelFront, WheelRear, WheelSide, NumParts };

	/** Where the machine runs and where the sidecar sits, across the outfit (cm). */
	inline double BikeY(bool bSidecar) { return bSidecar ? 30.0 : 0.0; }
	constexpr double SideY = -34.0;
	constexpr double WheelRadius = 33.0;
	constexpr double SideWheelRadius = 30.0;
	inline FVector RearAxle(bool bSidecar) { return FVector(-72.0, BikeY(bSidecar), WheelRadius); }
	inline FVector FrontAxle(bool bSidecar) { return FVector(74.0, BikeY(bSidecar), WheelRadius); }
	inline FVector SideAxle() { return FVector(-8.0, SideY - 35.0, SideWheelRadius); }
	inline FVector HeadBottom(bool bSidecar) { return FVector(50.0, BikeY(bSidecar), 72.0); }
	inline FVector HeadTop(bool bSidecar) { return FVector(44.0, BikeY(bSidecar), 92.0); }
	/** The steering turns about the head tube's line (unit vector, up the tube). */
	inline FVector SteerAxis(bool bSidecar) { return (HeadTop(bSidecar) - HeadBottom(bSidecar)).GetSafeNormal(); }
	constexpr double SaddleX = -26.0;
	constexpr double SaddleTop = 88.0;
	/** The rider's middle, seated on the saddle; and the sidecar passenger's. */
	inline FVector RiderSeat(bool bSidecar) { return FVector(SaddleX + 2.0, BikeY(bSidecar), SaddleTop + 9.0); }
	inline FVector SidecarSeat() { return FVector(-30.0, SideY, 66.0); }
	/** The handlebar grips. */
	inline FVector Grip(bool bSidecar, double Side) { return FVector(32.0, BikeY(bSidecar) + 31.0 * Side, 100.0); }
	/** Half the outfit's size, for its box (cm). */
	inline FVector OutfitHalf(bool bSidecar) { return bSidecar ? FVector(112.0, 75.0, 55.0) : FVector(112.0, 36.0, 55.0); }

	inline FLinearColor Enamel() { return FLinearColor(0.015f, 0.015f, 0.017f); }
	inline FLinearColor Nickel() { return FLinearColor(0.55f, 0.55f, 0.57f); }
	inline FLinearColor Alloy() { return FLinearColor(0.36f, 0.35f, 0.33f); }
	inline FLinearColor CastIron() { return FLinearColor(0.07f, 0.07f, 0.075f); }
	inline FLinearColor Tyre() { return FLinearColor(0.04f, 0.038f, 0.036f); }
	inline FLinearColor Leather() { return FLinearColor(0.15f, 0.06f, 0.025f); }
	inline FLinearColor GoldLine() { return FLinearColor(0.62f, 0.44f, 0.12f); }
	inline FLinearColor Cushion() { return FLinearColor(0.20f, 0.03f, 0.03f); }
	inline FLinearColor Glass() { return FLinearColor(0.30f, 0.35f, 0.37f); }
	inline FLinearColor Lens() { return FLinearColor(0.85f, 0.82f, 0.68f); }
	inline FLinearColor PlateWhite() { return FLinearColor(0.80f, 0.79f, 0.74f); }
	/** The police machine's tank and guards: the police cars' blue, white lines and tips. */
	inline FLinearColor PoliceBlue() { return FLinearColor(0.02f, 0.04f, 0.12f); }
	inline FLinearColor PoliceWhite() { return FLinearColor(0.82f, 0.82f, 0.80f); }

	/** The sidecar's lacquer, by number: black, maroon, bottle green, dove grey. */
	constexpr int32 NumPaints = 4;
	inline FLinearColor Paint(int32 Index)
	{
		switch (((Index % NumPaints) + NumPaints) % NumPaints)
		{
		case 1: return FLinearColor(0.20f, 0.025f, 0.03f);
		case 2: return FLinearColor(0.025f, 0.09f, 0.045f);
		case 3: return FLinearColor(0.30f, 0.31f, 0.30f);
		default: return FLinearColor(0.02f, 0.02f, 0.022f);
		}
	}

	/** A motorbike constable: the uniform (PoliceModel) under a brown leather crash helmet with goggles. */
	inline PersonKit::FOutfit PoliceRider(FRandomStream& Random)
	{
		PersonKit::FOutfit O = PoliceModel::Outfit(Random);
		O.Hat = PersonKit::EHat::Helmet;
		O.HatColor = FLinearColor(0.20f, 0.11f, 0.05f);
		O.HatBand = FLinearColor(0.09f, 0.05f, 0.025f);
		return O;
	}

	/** A spoked wheel round its hub (its middle, the Pivot), turning about Y: drum hub, 36 spokes, nickel rim, fat tyre. */
	inline void BuildWheel(FPiece& W, double Radius)
	{
		using namespace PersonKit;
		Loft(W, Alloy(), {
			{ FVector(0.0, -6.0, 0.0), FVector(7.0, 0.0, 0.0), FVector(0.0, 0.0, 7.0) },
			{ FVector(0.0, 3.0, 0.0), FVector(7.0, 0.0, 0.0), FVector(0.0, 0.0, 7.0) },
		}, 14, true, true);
		Loft(W, Nickel(), {
			{ FVector(0.0, 3.0, 0.0), FVector(3.5, 0.0, 0.0), FVector(0.0, 0.0, 3.5) },
			{ FVector(0.0, 7.0, 0.0), FVector(3.5, 0.0, 0.0), FVector(0.0, 0.0, 3.5) },
		}, 10, true, true);
		const double RimR = Radius - 6.0;
		const int32 Spokes = 36;
		for (int32 s = 0; s < Spokes; ++s)
		{
			const double A = 2.0 * PI * s / Spokes;
			const double Lead = (s % 2 == 0 ? 1.0 : -1.0) * 0.35;
			const double Y = (s % 4 < 2) ? -4.5 : 4.5;
			const FVector Hub(FMath::Cos(A + Lead) * 5.0, Y, FMath::Sin(A + Lead) * 5.0);
			const FVector Rim(FMath::Cos(A) * (RimR - 0.8), 0.0, FMath::Sin(A) * (RimR - 0.8));
			Rod(W, Nickel(), Hub, Rim, 0.3, 3);
		}
		BicycleModel::Arc(W, Nickel(), FVector::ZeroVector, RimR, 0.0, 2.0 * PI, 1.4, 2.4, 6);
		BicycleModel::Arc(W, Tyre(), FVector::ZeroVector, Radius - 3.2, 0.0, 2.0 * PI, 3.2, 4.4, 10);
	}

	/** The rigid frame, engine, gearbox, tank, saddle, exhaust, rear guard and carrier. */
	inline void BuildMachine(FPiece& B, bool bSidecar, bool bPolice = false)
	{
		using namespace PersonKit;
		const double Y = BikeY(bSidecar);
		const FVector Rear = RearAxle(bSidecar);
		const FVector HeadLow = HeadBottom(bSidecar);
		const FVector HeadHigh = HeadTop(bSidecar);

		// The frame: head tube, top tube, down tube under the engine, seat tube and the rear stays.
		const FVector SeatLug(SaddleX + 4.0, Y, 80.0);
		const FVector Cradle(-12.0, Y, 20.0);
		const FVector Front(26.0, Y, 18.0);
		Rod(B, Enamel(), HeadLow, HeadHigh, 3.0, 10);
		Rod(B, Enamel(), HeadHigh - FVector(0.0, 0.0, 4.0), SeatLug, 1.8, 8);
		Rod(B, Enamel(), HeadLow, Front, 2.0, 8);
		Rod(B, Enamel(), Front, Cradle, 1.8, 8);
		Rod(B, Enamel(), SeatLug, Cradle, 1.8, 8);
		for (const double Side : { -1.0, 1.0 })
		{
			const FVector Drop = Rear + FVector(0.0, 6.0 * Side, 0.0);
			Rod(B, Enamel(), Cradle + FVector(0.0, 4.0 * Side, 0.0), Drop, 1.3, 6);
			Rod(B, Enamel(), SeatLug + FVector(0.0, 3.0 * Side, 0.0), Drop, 1.2, 6);
		}

		// The saddle tank over the top tube, black with a gold line, its filler cap.
		TArray<FRing> Tank;
		const double TankRows[][4] = { { -16.0, 7.0, 6.0, 82.0 }, { -8.0, 11.0, 8.5, 83.5 }, { 12.0, 12.5, 9.5, 86.0 },
			{ 32.0, 11.5, 9.0, 88.5 }, { 42.0, 7.0, 6.5, 90.0 } };
		for (const auto& Row : TankRows)
		{
			Tank.Add({ FVector(Row[0], Y, Row[3]), FVector(0.0, Row[1], 0.0), FVector(0.0, 0.0, Row[2]) });
		}
		const FLinearColor TankPaint = bPolice ? PoliceBlue() : Enamel();
		const FLinearColor LinePaint = bPolice ? PoliceWhite() : GoldLine();
		Loft(B, TankPaint, Tank, 14, true, true);
		for (const double Side : { -1.0, 1.0 })
		{
			BicycleModel::Curve(B, LinePaint, FVector(-10.0, Y + 11.4 * Side, 84.0), FVector(12.0, Y + 13.0 * Side, 86.5),
				FVector(36.0, Y + 11.0 * Side, 89.0), 0.45, 5);
			BicycleModel::Curve(B, LinePaint, FVector(-6.0, Y + 11.6 * Side, 79.5), FVector(12.0, Y + 13.0 * Side, 80.5),
				FVector(32.0, Y + 11.4 * Side, 83.0), 0.45, 5);
			// Knee grips of rubber.
			Block(B, Tyre(), FVector(-6.0, Y + 10.5 * Side, 82.0), FVector(5.0, 1.2, 4.0));
		}
		Loft(B, Nickel(), {
			{ FVector(16.0, Y, 95.0), FVector(3.2, 0.0, 0.0), FVector(0.0, 3.2, 0.0) },
			{ FVector(16.0, Y, 97.5), FVector(3.2, 0.0, 0.0), FVector(0.0, 3.2, 0.0) },
		}, 10, false, true);

		// The engine: crankcase of alloy, the finned iron barrel leaning forward, its head and the plug.
		Blob(B, Alloy(), FVector(4.0, Y, 30.0), FVector(17.0, 9.0, 13.0), 14, 8);
		const FVector BarrelFoot(8.0, Y, 40.0);
		const FVector BarrelUp = FVector(0.22, 0.0, 1.0).GetSafeNormal();
		for (int32 Fin = 0; Fin < 8; ++Fin)
		{
			const FVector At = BarrelFoot + BarrelUp * (Fin * 2.8);
			const double R = Fin < 6 ? 9.5 : 8.0;
			Loft(B, CastIron(), {
				{ At, FVector(R, 0.0, 0.0), FVector(0.0, R * 0.85, 0.0) },
				{ At + BarrelUp * 1.1, FVector(R, 0.0, 0.0), FVector(0.0, R * 0.85, 0.0) },
			}, 12, true, true);
		}
		Rod(B, CastIron(), BarrelFoot, BarrelFoot + BarrelUp * 22.0, 6.0, 10);
		Blob(B, Alloy(), BarrelFoot + BarrelUp * 24.0, FVector(10.0, 8.5, 4.5), 12, 4);
		Rod(B, Nickel(), BarrelFoot + BarrelUp * 27.0 + FVector(-3.0, 0.0, 0.0), BarrelFoot + BarrelUp * 31.0 + FVector(-4.0, 0.0, 0.0), 0.8);
		// The gearbox behind, the primary chain case on the left, the magneto in front.
		Block(B, Alloy(), FVector(-16.0, Y, 34.0), FVector(8.0, 7.0, 9.0));
		Loft(B, Enamel(), {
			{ FVector(-6.0, Y - 9.0, 33.0), FVector(16.0, 0.0, 0.0), FVector(0.0, 0.0, 8.0) },
			{ FVector(-6.0, Y - 12.0, 33.0), FVector(16.0, 0.0, 0.0), FVector(0.0, 0.0, 8.0) },
		}, 12, true, true);
		Loft(B, Enamel(), {
			{ FVector(20.0, Y - 4.0, 42.0), FVector(0.0, 0.0, 4.0), FVector(4.0, 0.0, 0.0) },
			{ FVector(20.0, Y + 4.0, 42.0), FVector(0.0, 0.0, 4.0), FVector(4.0, 0.0, 0.0) },
		}, 10, true, true);
		// The rear chain on the right, the kick-start lever, the footrests.
		Rod(B, CastIron(), FVector(-16.0, Y + 8.0, 30.0), Rear + FVector(0.0, 8.0, 6.0), 0.9, 4);
		Rod(B, CastIron(), FVector(-16.0, Y + 8.0, 24.0), Rear + FVector(0.0, 8.0, -6.0), 0.9, 4);
		Rod(B, Nickel(), FVector(-18.0, Y + 11.0, 38.0), FVector(-34.0, Y + 12.0, 52.0), 1.0);
		for (const double Side : { -1.0, 1.0 })
		{
			Rod(B, Tyre(), FVector(2.0, Y + 8.0 * Side, 22.0), FVector(2.0, Y + 22.0 * Side, 22.0), 1.6);
		}

		// The exhaust: down from the port, back along the right side, the fishtail at the end.
		const FVector Port = BarrelFoot + BarrelUp * 18.0 + FVector(8.0, 4.0, 0.0);
		BicycleModel::Curve(B, Nickel(), Port, FVector(34.0, Y + 10.0, 30.0), FVector(16.0, Y + 13.0, 20.0), 2.0, 5);
		Rod(B, Nickel(), FVector(16.0, Y + 13.0, 20.0), FVector(-70.0, Y + 15.0, 26.0), 2.0, 10);
		Loft(B, Nickel(), {
			{ FVector(-70.0, Y + 15.0, 26.0), FVector(0.0, 2.2, 0.0), FVector(0.0, 0.0, 2.2) },
			{ FVector(-90.0, Y + 15.0, 27.5), FVector(0.0, 2.6, 0.0), FVector(0.0, 0.0, 5.5) },
		}, 10, false, true);

		// The sprung saddle on its two coil springs, the toolbox, the rear guard, carrier and plate.
		Blob(B, Leather(), FVector(SaddleX, Y, SaddleTop - 2.5), FVector(14.0, 10.5, 3.5), 12, 5);
		Blob(B, Leather(), FVector(SaddleX + 13.0, Y, SaddleTop - 3.0), FVector(5.0, 4.0, 2.5), 8, 4);
		for (const double Side : { -1.0, 1.0 })
		{
			const FVector Top(SaddleX - 10.0, Y + 6.0 * Side, SaddleTop - 5.0);
			const FVector Foot(SaddleX - 14.0, Y + 6.0 * Side, 72.0);
			for (int32 Coil = 0; Coil < 5; ++Coil)
			{
				const FVector At = Foot + (Top - Foot) * (Coil / 4.0);
				Loft(B, Nickel(), {
					{ At, FVector(2.2, 0.0, 0.0), FVector(0.0, 2.2, 0.0) },
					{ At + FVector(0.0, 0.0, 0.8), FVector(2.2, 0.0, 0.0), FVector(0.0, 2.2, 0.0) },
				}, 8, false, false);
			}
			Rod(B, Enamel(), Foot, Rear + FVector(10.0, 6.0 * Side, 20.0), 0.9);
		}
		Block(B, Enamel(), FVector(-50.0, Y + 9.0, 52.0), FVector(8.0, 3.0, 5.0));
		BicycleModel::Arc(B, bPolice ? PoliceBlue() : Enamel(), FVector(Rear.X, Y, Rear.Z), WheelRadius + 4.0, 0.35, PI + 0.25, 0.8, 6.5, 6);
		if (bPolice)
		{
			// A white tip to the rear guard, leather bags each side of the carrier for the forms and the handcuffs.
			BicycleModel::Arc(B, PoliceWhite(), FVector(Rear.X, Y, Rear.Z), WheelRadius + 4.1, PI - 0.05, PI + 0.27, 0.9, 6.7, 6);
			for (const double Side : { -1.0, 1.0 })
			{
				Block(B, Leather(), FVector(-80.0, Y + 13.0 * Side, 58.0), FVector(12.0, 4.0, 9.0));
				Block(B, Leather(), FVector(-80.0, Y + 13.5 * Side, 65.5), FVector(12.5, 4.3, 1.5));
				Rod(B, Nickel(), FVector(-80.0, Y + 17.2 * Side, 61.0), FVector(-80.0, Y + 17.2 * Side, 65.0), 0.5);
			}
		}
		for (const double Side : { -1.0, 1.0 })
		{
			Rod(B, Enamel(), FVector(-50.0, Y + 6.0 * Side, 72.0), FVector(-96.0, Y + 6.0 * Side, 66.0), 0.9);
		}
		Rod(B, Enamel(), FVector(-96.0, Y - 6.0, 66.0), FVector(-96.0, Y + 6.0, 66.0), 0.9);
		Block(B, Enamel(), FVector(-107.4, Y, 46.0), FVector(0.4, 9.6, 6.6));
		Block(B, PlateWhite(), FVector(-108.0, Y, 46.0), FVector(0.4, 9.0, 6.0));
		Blob(B, FLinearColor(0.45f, 0.03f, 0.02f), FVector(-108.0, Y, 54.5), FVector(1.5, 2.0, 2.0), 8, 4);
	}

	/** The girder fork, handlebars, headlamp and front guard: they turn about the head tube, round HeadBottom. */
	inline void BuildSteer(FPiece& S, bool bSidecar, bool bPolice = false)
	{
		using namespace PersonKit;
		const double Y = BikeY(bSidecar);
		const FVector Axle = FrontAxle(bSidecar);
		const FVector HeadLow = HeadBottom(bSidecar);
		const FVector HeadHigh = HeadTop(bSidecar);
		S.Pivot = HeadLow;
		for (const double Side : { -1.0, 1.0 })
		{
			const FVector Out(0.0, 8.0 * Side, 0.0);
			// Two blades each side of the girder, braced, the spring on the front.
			Rod(S, Enamel(), HeadHigh + Out, Axle + Out + FVector(-2.0, 0.0, 0.0), 1.3, 6);
			Rod(S, Enamel(), HeadLow + Out + FVector(6.0, 0.0, 0.0), Axle + Out + FVector(4.0, 0.0, 6.0), 1.1, 6);
		}
		Rod(S, Enamel(), HeadHigh + FVector(0.0, -8.0, 0.0), HeadHigh + FVector(0.0, 8.0, 0.0), 1.5, 6);
		Rod(S, Enamel(), HeadLow + FVector(4.0, -8.0, 0.0), HeadLow + FVector(4.0, 8.0, 0.0), 1.5, 6);
		Loft(S, Nickel(), {
			{ HeadHigh + FVector(5.0, 0.0, -2.0), FVector(2.4, 0.0, 0.0), FVector(0.0, 2.4, 0.0) },
			{ HeadHigh + FVector(5.0, 0.0, 10.0), FVector(2.4, 0.0, 0.0), FVector(0.0, 2.4, 0.0) },
		}, 8, true, true);

		// The handlebars, swept back to the grips, the levers.
		const FVector Clamp = HeadHigh + FVector(2.0, 0.0, 4.0);
		for (const double Side : { -1.0, 1.0 })
		{
			const FVector Hand = Grip(bSidecar, Side);
			BicycleModel::Curve(S, Nickel(), Clamp, FVector(Clamp.X + 2.0, Y + 22.0 * Side, Clamp.Z + 1.0), Hand + FVector(6.0, 0.0, 0.0), 1.1, 6);
			Rod(S, Tyre(), Hand + FVector(6.0, 0.0, 0.0), Hand + FVector(-6.0, 3.0 * Side, 0.0), 1.6, 8);
			Rod(S, Nickel(), Hand + FVector(8.0, -3.0 * Side, 0.5), Hand + FVector(0.0, 2.0 * Side, -3.0), 0.5, 4);
		}

		// The headlamp on its brackets: drum, nickel rim, the glass.
		const FVector Lamp = HeadLow + FVector(26.0, 0.0, 14.0);
		Rod(S, Enamel(), HeadHigh + FVector(4.0, -8.0, -6.0), Lamp + FVector(-6.0, -9.0, 0.0), 0.8);
		Rod(S, Enamel(), HeadHigh + FVector(4.0, 8.0, -6.0), Lamp + FVector(-6.0, 9.0, 0.0), 0.8);
		Loft(S, Enamel(), {
			{ Lamp + FVector(-9.0, 0.0, 0.0), FVector(0.0, 7.0, 0.0), FVector(0.0, 0.0, 7.0) },
			{ Lamp + FVector(2.0, 0.0, 0.0), FVector(0.0, 10.5, 0.0), FVector(0.0, 0.0, 10.5) },
		}, 16, true, false);
		Loft(S, Nickel(), {
			{ Lamp + FVector(2.0, 0.0, 0.0), FVector(0.0, 11.0, 0.0), FVector(0.0, 0.0, 11.0) },
			{ Lamp + FVector(4.0, 0.0, 0.0), FVector(0.0, 11.0, 0.0), FVector(0.0, 0.0, 11.0) },
		}, 16, false, false);
		Loft(S, Lens(), {
			{ Lamp + FVector(2.5, 0.0, 0.0), FVector(0.0, 9.5, 0.0), FVector(0.0, 0.0, 9.5) },
			{ Lamp + FVector(3.5, 0.0, 0.0), FVector(0.0, 9.5, 0.0), FVector(0.0, 0.0, 9.5) },
		}, 16, false, true);
		// The bulb horn on the right bar.
		Rod(S, Nickel(), Grip(bSidecar, 1.0) + FVector(10.0, -6.0, 2.0), Grip(bSidecar, 1.0) + FVector(28.0, -6.0, 0.0), 1.2);
		Loft(S, Nickel(), {
			{ Grip(bSidecar, 1.0) + FVector(28.0, -6.0, 0.0), FVector(0.0, 1.2, 0.0), FVector(0.0, 0.0, 1.2) },
			{ Grip(bSidecar, 1.0) + FVector(36.0, -6.0, 0.0), FVector(0.0, 4.0, 0.0), FVector(0.0, 0.0, 4.0) },
		}, 10, false, false);
		Blob(S, Tyre(), Grip(bSidecar, 1.0) + FVector(6.0, -6.0, 3.0), FVector(3.5, 3.5, 3.5), 8, 5);

		// The front guard and its stays, the front number plate on it.
		BicycleModel::Arc(S, bPolice ? PoliceBlue() : Enamel(), FVector(Axle.X, Y, Axle.Z), WheelRadius + 4.0, -0.15, PI * 0.82, 0.8, 6.5, 6);
		if (bPolice)
		{
			// The white tip of the front guard, seen coming at night.
			BicycleModel::Arc(S, PoliceWhite(), FVector(Axle.X, Y, Axle.Z), WheelRadius + 4.1, -0.17, 0.25, 0.9, 6.7, 6);
		}
		Block(S, PlateWhite(), FVector(Axle.X, Y, Axle.Z + WheelRadius + 9.0), FVector(9.0, 0.5, 4.0));
		Block(S, Enamel(), FVector(Axle.X, Y, Axle.Z + WheelRadius + 9.0), FVector(9.6, 0.4, 4.6));
	}

	/** The torpedo sidecar on its chassis, its seat, little windscreen and lamp, the third wheel's guard. */
	inline void BuildSidecar(FPiece& B, const FLinearColor& Lacquer)
	{
		using namespace PersonKit;
		const double Y = BikeY(true);
		// The chassis: two rails under the boat, tied to the machine, the axle out to the third wheel.
		const FVector Wheel = SideAxle();
		for (const double X : { -50.0, 40.0 })
		{
			Rod(B, Enamel(), FVector(X, Y - 2.0, 24.0), FVector(X, SideY + 20.0, 28.0), 1.6, 6);
		}
		Rod(B, Enamel(), FVector(-70.0, SideY + 6.0, 28.0), FVector(60.0, SideY + 6.0, 28.0), 1.6, 6);
		Rod(B, Enamel(), FVector(Wheel.X, SideY + 6.0, 28.0), Wheel + FVector(0.0, 6.0, 0.0), 1.8, 6);
		Rod(B, Enamel(), FVector(-34.0, SideY + 6.0, 28.0), Wheel + FVector(0.0, 6.0, 1.0), 1.4, 6);
		Rod(B, Enamel(), FVector(16.0, SideY + 6.0, 28.0), Wheel + FVector(0.0, 6.0, 1.0), 1.4, 6);
		Rod(B, Enamel(), FVector(-10.0, Y - 2.0, 80.0), FVector(-10.0, SideY + 24.0, 64.0), 1.2, 6);
		// C-springs the boat rides on.
		for (const double X : { -58.0, 46.0 })
		{
			BicycleModel::Arc(B, Enamel(), FVector(X, SideY + 6.0, 34.0), 5.0, PI * 0.5, PI * 1.6, 1.0, 2.5, 4);
		}

		// The boat: a lacquered torpedo, pointed both ends, deepest under the seat.
		TArray<FRing> Boat;
		const double Rows[][4] = { { -92.0, 4.0, 5.0, 58.0 }, { -84.0, 17.0, 15.0, 57.0 }, { -66.0, 25.0, 20.0, 56.0 },
			{ -36.0, 29.0, 22.0, 55.0 }, { 0.0, 29.0, 21.0, 55.0 }, { 34.0, 26.0, 18.0, 54.0 }, { 58.0, 19.0, 13.0, 52.0 },
			{ 76.0, 10.0, 7.5, 50.0 }, { 86.0, 3.0, 3.0, 49.0 } };
		for (const auto& Row : Rows)
		{
			Boat.Add({ FVector(Row[0], SideY, Row[3]), FVector(0.0, Row[1], 0.0), FVector(0.0, 0.0, Row[2]) });
		}
		Loft(B, Lacquer, Boat, 18, true, true);
		// A gold coachline round each side.
		for (const double Side : { -1.0, 1.0 })
		{
			BicycleModel::Curve(B, GoldLine(), FVector(-80.0, SideY + 17.6 * Side, 61.0), FVector(-30.0, SideY + 31.0 * Side, 61.0),
				FVector(20.0, SideY + 28.4 * Side, 60.0), 0.5, 7);
			BicycleModel::Curve(B, GoldLine(), FVector(20.0, SideY + 28.4 * Side, 60.0), FVector(55.0, SideY + 22.5 * Side, 58.5),
				FVector(78.0, SideY + 9.8 * Side, 55.5), 0.5, 5);
		}
		// The open seat: the dark well, its padded rim, the high cushioned back.
		Blob(B, FLinearColor(0.03f, 0.02f, 0.02f), FVector(-34.0, SideY, 75.5), FVector(30.0, 20.0, 1.5), 16, 3);
		Loft(B, Leather(), {
			{ FVector(-34.0, SideY, 75.0), FVector(31.0, 0.0, 0.0), FVector(0.0, 21.5, 0.0) },
			{ FVector(-34.0, SideY, 78.0), FVector(31.0, 0.0, 0.0), FVector(0.0, 21.5, 0.0) },
		}, 20, false, true);
		Blob(B, Cushion(), FVector(-58.0, SideY, 84.0), FVector(6.0, 18.0, 13.0), 12, 6, -0.7);
		Blob(B, Cushion(), FVector(-36.0, SideY, 77.0), FVector(18.0, 17.0, 3.0), 12, 4);
		// The windscreen in a nickel frame, the little sidecar lamp out on the nose.
		Block(B, Glass(), FVector(4.0, SideY, 86.0), FVector(0.4, 17.0, 8.0));
		for (const double Side : { -1.0, 1.0 })
		{
			Rod(B, Nickel(), FVector(4.0, SideY + 17.5 * Side, 76.0), FVector(4.0, SideY + 17.5 * Side, 94.5), 0.6);
		}
		Rod(B, Nickel(), FVector(4.0, SideY - 17.5, 94.5), FVector(4.0, SideY + 17.5, 94.5), 0.6);
		Rod(B, Nickel(), FVector(56.0, SideY - 14.0, 62.0), FVector(58.0, SideY - 18.0, 70.0), 0.7);
		Loft(B, Nickel(), {
			{ FVector(54.0, SideY - 18.0, 72.0), FVector(0.0, 3.5, 0.0), FVector(0.0, 0.0, 3.5) },
			{ FVector(62.0, SideY - 18.0, 72.0), FVector(0.0, 4.2, 0.0), FVector(0.0, 0.0, 4.2) },
		}, 12, true, true);
		Loft(B, Lens(), {
			{ FVector(62.0, SideY - 18.0, 72.0), FVector(0.0, 3.6, 0.0), FVector(0.0, 0.0, 3.6) },
			{ FVector(62.6, SideY - 18.0, 72.0), FVector(0.0, 3.6, 0.0), FVector(0.0, 0.0, 3.6) },
		}, 12, false, true);
		// A step on the side, the third wheel's guard.
		Block(B, Tyre(), FVector(-8.0, SideY + 26.0, 30.0), FVector(10.0, 6.0, 0.8));
		BicycleModel::Arc(B, Lacquer, FVector(Wheel.X, Wheel.Y, Wheel.Z), SideWheelRadius + 4.0, 0.05, PI - 0.05, 0.7, 6.0, 6);
	}

	/** Where each part's pivot sits in the outfit (the wheels are built round their hubs at zero). */
	inline FVector PartAt(int32 Part, bool bSidecar)
	{
		switch (Part)
		{
		case Steer: return HeadBottom(bSidecar);
		case WheelFront: return FrontAxle(bSidecar);
		case WheelRear: return RearAxle(bSidecar);
		case WheelSide: return SideAxle();
		default: return FVector::ZeroVector;
		}
	}

	/** The whole outfit's parts, each about its own pivot (PartAt). The solo machine has no WheelSide; bPolice paints it as the police's. */
	inline void Build(FPiece (&Parts)[NumParts], bool bSidecar, int32 PaintIndex, bool bPolice = false)
	{
		BuildMachine(Parts[Body], bSidecar, bPolice);
		if (bSidecar)
		{
			BuildSidecar(Parts[Body], Paint(PaintIndex));
		}
		BuildSteer(Parts[Steer], bSidecar, bPolice);
		BuildWheel(Parts[WheelFront], WheelRadius);
		BuildWheel(Parts[WheelRear], WheelRadius);
		if (bSidecar)
		{
			BuildWheel(Parts[WheelSide], SideWheelRadius);
		}
	}
}
