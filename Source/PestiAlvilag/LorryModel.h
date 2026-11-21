// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DailyLifeModel.h"
#include "PersonKit.h"

/**
 * A Győr-built Rába lorry of the early thirties (about 3 tons): long
 * bonnet with louvres, a tall radiator in a nickel shell with the RÁBA
 * badge, a closed wooden cab with a flat roof, steel disc wheels with twin
 * wheels at the back, and a wooden platform with drop sides and stakes.
 * The load on the platform varies (ELoad). Built in the floor frame (X
 * forward, Y right, Z up from the road, the middle of the lorry at
 * X = Y = 0); the wheels are parts of their own, built round their hubs.
 */
namespace LorryModel
{
	using PersonKit::FPiece;
	using DailyLifeModel::Box;
	using DailyLifeModel::Tube;
	using DailyLifeModel::Ring;

	enum EPart : int32 { Body, WheelFrontLeft, WheelFrontRight, WheelBackLeft, WheelBackRight, NumParts };

	/** What it carries on the platform; Post is the Royal Hungarian Post's van, a closed box body instead of the platform. */
	enum class ELoad : uint8 { Empty, Barrels, Crates, Sacks, Tilt, Post, Count };

	constexpr double WheelRadius = 46.0;
	constexpr double FrontAxle = 190.0;
	constexpr double BackAxle = -150.0;
	constexpr double FrontTrack = 76.0;
	constexpr double BackTrack = 74.0;
	/** The back twins stand this far apart on their hub. */
	constexpr double TwinGap = 18.0;
	/** The platform's floor, its ends and half width (cm). */
	constexpr double BedZ = 112.0;
	constexpr double BedBack = -310.0;
	constexpr double BedFront = 48.0;
	constexpr double BedHalfY = 100.0;
	constexpr double SideHeight = 48.0;

	/** Half its length, width and height, for its box (cm). */
	inline FVector LorryHalf() { return FVector(318.0, 104.0, 125.0); }
	/** The driver's middle, seated in the cab (on the right: Pest drove on the left in 1932), and his mate's. */
	inline FVector DriverSeat() { return FVector(88.0, 34.0, 152.0); }
	inline FVector MateSeat() { return FVector(88.0, -34.0, 152.0); }

	inline FVector WheelHub(int32 Part)
	{
		const bool bFront = Part == WheelFrontLeft || Part == WheelFrontRight;
		const bool bLeft = Part == WheelFrontLeft || Part == WheelBackLeft;
		const double Track = bFront ? FrontTrack : BackTrack;
		return FVector(bFront ? FrontAxle : BackAxle, bLeft ? -Track : Track, WheelRadius);
	}

	inline FLinearColor Chassis() { return FLinearColor(0.02f, 0.02f, 0.022f); }
	inline FLinearColor Nickel() { return FLinearColor(0.55f, 0.55f, 0.57f); }
	inline FLinearColor Planks() { return FLinearColor(0.34f, 0.23f, 0.12f); }
	inline FLinearColor DarkPlanks() { return FLinearColor(0.22f, 0.14f, 0.07f); }
	inline FLinearColor Canvas() { return FLinearColor(0.32f, 0.31f, 0.22f); }
	inline FLinearColor Glass() { return FLinearColor(0.16f, 0.19f, 0.21f); }
	inline FLinearColor Tyre() { return FLinearColor(0.03f, 0.03f, 0.03f); }
	inline FLinearColor SeatLeather() { return FLinearColor(0.13f, 0.06f, 0.03f); }

	/** The cab and bonnet colours a lorry can have: Rába green, oxblood, slate blue, brown. */
	inline FLinearColor Paint(int32 Index)
	{
		switch (((Index % 4) + 4) % 4)
		{
		case 1: return FLinearColor(0.20f, 0.035f, 0.025f);
		case 2: return FLinearColor(0.05f, 0.08f, 0.12f);
		case 3: return FLinearColor(0.17f, 0.09f, 0.04f);
		default: return FLinearColor(0.05f, 0.10f, 0.05f);
		}
	}

	/** A curved mudguard over a wheel at (X, Z), from YIn to YOut, as short plates round a half circle. */
	inline void Guard(FPiece& B, const FLinearColor& Color, double X, double Z, double R, double YIn, double YOut, double From = 0.0, double To = PI)
	{
		constexpr int32 Steps = 8;
		for (int32 k = 0; k < Steps; ++k)
		{
			const double A0 = From + (To - From) * k / Steps;
			const double A1 = From + (To - From) * (k + 1) / Steps;
			const FVector C0(X + R * FMath::Cos(A0), 0.0, Z + R * FMath::Sin(A0));
			const FVector C1(X + R * FMath::Cos(A1), 0.0, Z + R * FMath::Sin(A1));
			const FVector Lo = C0.ComponentMin(C1);
			const FVector Hi = C0.ComponentMax(C1);
			Box(B, Color, FVector(Lo.X - 0.5, FMath::Min(YIn, YOut), Lo.Z - 0.5), FVector(Hi.X + 0.5, FMath::Max(YIn, YOut), Hi.Z + 1.5));
		}
	}

	/** A steel disc wheel turning about Y, its hub at Center: fat tyre, dished disc with holes, nickel hub cap. */
	inline void DiscWheel(FPiece& W, const FLinearColor& Disc, const FVector& Center, double Width, double Out)
	{
		const double Half = Width * 0.5;
		TArray<PersonKit::FRing> Rubber;
		Rubber.Add({ Center + FVector(0.0, -Half, 0.0), FVector(WheelRadius - 3.0, 0.0, 0.0), FVector(0.0, 0.0, WheelRadius - 3.0) });
		Rubber.Add({ Center + FVector(0.0, -Half + 3.0, 0.0), FVector(WheelRadius, 0.0, 0.0), FVector(0.0, 0.0, WheelRadius) });
		Rubber.Add({ Center + FVector(0.0, Half - 3.0, 0.0), FVector(WheelRadius, 0.0, 0.0), FVector(0.0, 0.0, WheelRadius) });
		Rubber.Add({ Center + FVector(0.0, Half, 0.0), FVector(WheelRadius - 3.0, 0.0, 0.0), FVector(0.0, 0.0, WheelRadius - 3.0) });
		PersonKit::Loft(W, Tyre(), Rubber, 22, false, false);
		// The disc on the outer face, dished in to the hub; its rim and ring of holes.
		for (const double Side : { -1.0, 1.0 })
		{
			const double Face = Side * (Half - 2.5);
			PersonKit::Loft(W, Disc, {
				{ Center + FVector(0.0, Face, 0.0), FVector(WheelRadius - 9.0, 0.0, 0.0), FVector(0.0, 0.0, WheelRadius - 9.0) },
				{ Center + FVector(0.0, Face + Side * 2.0, 0.0), FVector(WheelRadius - 20.0, 0.0, 0.0), FVector(0.0, 0.0, WheelRadius - 20.0) },
				{ Center + FVector(0.0, Face + Side * 2.5, 0.0), FVector(0.5, 0.0, 0.0), FVector(0.0, 0.0, 0.5) },
			}, 18, false, false);
		}
		Ring(W, Disc, Center + FVector(0.0, Out * (Half - 2.0), 0.0), FVector(WheelRadius - 9.0, 0.0, 0.0), FVector(0.0, 0.0, WheelRadius - 9.0), 2.0, 22);
		for (int32 h = 0; h < 6; ++h)
		{
			const double A = 2.0 * PI * h / 6.0;
			PersonKit::Blob(W, Chassis(), Center + FVector(FMath::Cos(A) * 22.0, Out * (Half + 0.3), FMath::Sin(A) * 22.0), FVector(4.0, 0.6, 4.0), 8, 3);
		}
		Tube(W, Nickel(), Center + FVector(0.0, Out * (Half - 1.0), 0.0), Center + FVector(0.0, Out * (Half + 7.0), 0.0), 7.0, 12);
		PersonKit::Blob(W, Nickel(), Center + FVector(0.0, Out * (Half + 7.0), 0.0), FVector(6.0, 2.0, 6.0), 10, 4);
	}

	/** One barrel lying on its side along Y, its middle at At. */
	inline void Barrel(FPiece& B, const FVector& At, bool bStanding)
	{
		const FLinearColor Oak(0.36f, 0.22f, 0.10f);
		const FLinearColor Hoop(0.07f, 0.07f, 0.07f);
		const FVector Axis = bStanding ? FVector(0.0, 0.0, 1.0) : FVector(0.0, 1.0, 0.0);
		const FVector U = bStanding ? FVector(1.0, 0.0, 0.0) : FVector(1.0, 0.0, 0.0);
		const FVector V = bStanding ? FVector(0.0, 1.0, 0.0) : FVector(0.0, 0.0, 1.0);
		TArray<PersonKit::FRing> Staves;
		const double Rows[][2] = { { -38.0, 23.0 }, { -20.0, 28.0 }, { 0.0, 30.0 }, { 20.0, 28.0 }, { 38.0, 23.0 } };
		for (const auto& Row : Rows)
		{
			Staves.Add({ At + Axis * Row[0], U * Row[1], V * Row[1] });
		}
		PersonKit::Loft(B, Oak, Staves, 16, true, true);
		for (const double H : { -30.0, -12.0, 12.0, 30.0 })
		{
			const double R = 30.0 - FMath::Abs(H) * 0.18 + 0.6;
			Ring(B, Hoop, At + Axis * H, U * R, V * R, 1.0, 16);
		}
	}

	/** The load on the platform. */
	inline void BuildLoad(FPiece& B, ELoad Load)
	{
		switch (Load)
		{
		case ELoad::Barrels:
			// Beer barrels from Dreher's brewery, standing in two rows, one more lying across the back.
			for (int32 i = 0; i < 4; ++i)
			{
				for (const double Y : { -48.0, 48.0 })
				{
					Barrel(B, FVector(BedFront - 50.0 - i * 66.0, Y, BedZ + 40.0), true);
				}
			}
			Barrel(B, FVector(BedBack + 40.0, 0.0, BedZ + 30.0), false);
			break;
		case ELoad::Crates:
		{
			// Wooden crates stacked two high, some with stencilled ends.
			const FLinearColor Pine(0.50f, 0.38f, 0.21f);
			const FLinearColor Old(0.38f, 0.28f, 0.15f);
			int32 n = 0;
			for (int32 i = 0; i < 5; ++i)
			{
				for (const double Y : { -50.0, 0.0, 50.0 })
				{
					const int32 High = (i + (Y > 0.0 ? 1 : 0)) % 3 == 0 ? 1 : 2;
					for (int32 h = 0; h < High; ++h)
					{
						const FVector Lo(BedFront - 66.0 - i * 64.0, Y - 23.0, BedZ + h * 42.0);
						const FLinearColor Wood = (n++ % 3 == 0) ? Old : Pine;
						Box(B, Wood, Lo, Lo + FVector(58.0, 46.0, 40.0));
						Box(B, Wood * 0.7f, Lo + FVector(-0.5, -0.5, 18.0), Lo + FVector(58.5, 46.5, 22.0));
					}
				}
			}
			break;
		}
		case ELoad::Sacks:
		{
			// Flour or potato sacks laid in a heap.
			const FLinearColor Jute(0.52f, 0.43f, 0.28f);
			for (int32 Layer = 0; Layer < 3; ++Layer)
			{
				for (int32 i = 0; i < 6 - Layer * 2; ++i)
				{
					for (const double Y : { -45.0, 0.0, 45.0 })
					{
						if (Layer == 2 && Y != 0.0)
						{
							continue;
						}
						const double X = BedFront - 45.0 - (i + Layer) * 52.0;
						const FLinearColor C = ((i + Layer) % 2 == 0) ? Jute : Jute * 0.85f;
						PersonKit::Blob(B, C, FVector(X, Y + (i % 2 ? 4.0 : -4.0), BedZ + 14.0 + Layer * 22.0), FVector(26.0, 22.0, 14.0), 10, 6);
					}
				}
			}
			break;
		}
		case ELoad::Tilt:
		{
			// A canvas tilt: straight sides over the drop sides, a rounded top over hoops, a flap rolled up at the back.
			const double Mid = (BedFront + BedBack) * 0.5;
			const double HalfLen = (BedFront - BedBack) * 0.5 + 2.0;
			const double Eave = BedZ + SideHeight + 46.0;
			const double Rise = 52.0;
			for (const double S : { -1.0, 1.0 })
			{
				Box(B, Canvas(), FVector(BedBack - 2.0, S * (BedHalfY + 1.0) - 1.0, BedZ + 10.0), FVector(BedFront + 2.0, S * (BedHalfY + 1.0) + 1.0, Eave));
				// The ropes lacing it down to the platform.
				for (double X = BedBack + 30.0; X < BedFront; X += 60.0)
				{
					Tube(B, FLinearColor(0.45f, 0.40f, 0.28f), FVector(X, S * (BedHalfY + 2.2), BedZ + 14.0), FVector(X + 8.0, S * (BedHalfY + 2.2), BedZ - 6.0), 0.6, 4);
				}
			}
			TArray<PersonKit::FRing> Arch;
			constexpr int32 Steps = 12;
			for (int32 k = 0; k <= Steps; ++k)
			{
				const double A = PI * k / Steps;
				const FVector Normal(0.0, FMath::Cos(A) / (BedHalfY + 1.0), FMath::Sin(A) / Rise);
				Arch.Add({ FVector(Mid, FMath::Cos(A) * (BedHalfY + 1.0), Eave + FMath::Sin(A) * Rise), FVector(HalfLen, 0.0, 0.0), Normal.GetSafeNormal() * 1.0 });
			}
			PersonKit::Loft(B, Canvas(), Arch, 4, true, true, PI * 0.25);
			// The hoops showing through as ridges, and the back flap rolled up under the arch.
			for (int32 h = 0; h < 5; ++h)
			{
				const double X = BedBack + 10.0 + h * (BedFront - BedBack - 20.0) / 4.0;
				Ring(B, Canvas() * 0.75f, FVector(X, 0.0, Eave), FVector(0.0, BedHalfY + 2.2, 0.0), FVector(0.0, 0.0, Rise + 1.2), 1.3, 20);
			}
			Tube(B, Canvas() * 0.85f, FVector(BedBack - 4.0, -BedHalfY, Eave - 4.0), FVector(BedBack - 4.0, BedHalfY, Eave - 4.0), 6.0, 10);
			// The front end closed, behind the headboard.
			PersonKit::Loft(B, Canvas() * 0.9f, {
				{ FVector(BedFront + 1.0, 0.0, Eave), FVector(0.0, BedHalfY + 1.0, 0.0), FVector(0.0, 0.0, Rise) },
				{ FVector(BedFront + 2.0, 0.0, Eave), FVector(0.0, BedHalfY + 1.0, 0.0), FVector(0.0, 0.0, Rise) },
			}, 20, true, true);
			Box(B, Canvas() * 0.9f, FVector(BedFront, -BedHalfY, BedZ + SideHeight), FVector(BedFront + 2.0, BedHalfY, Eave));
			break;
		}
		default:
			break;
		}
	}

	/** The Royal Hungarian Post's green and the yellow of its lines and horn. */
	inline FLinearColor PostGreen() { return FLinearColor(0.035f, 0.075f, 0.04f); }
	inline FLinearColor PostYellow() { return FLinearColor(0.72f, 0.52f, 0.08f); }
	/** Where the post's name is painted on the box's side (+1 right, -1 left), and how tall (cm). */
	inline FVector PostLetteringAt(double Side) { return FVector((BedBack + BedFront) * 0.5 + 24.0, Side * (BedHalfY - 8.0 + 1.5), 128.0); }
	constexpr double PostLetterHeight = 22.0;

	/** The post horn with its looped tube and a crown over it, on a side at (X, Y, Z), facing out by Out (+1 right, -1 left). */
	inline void PostHorn(FPiece& B, double X, double Y, double Z, double Out)
	{
		const FLinearColor Gold = PostYellow();
		const double F = Y + Out * 0.8;
		// The loop of the tube, the bell flaring out to one side, the mouthpiece to the other.
		Ring(B, Gold, FVector(X, F, Z), FVector(16.0, 0.0, 0.0), FVector(0.0, 0.0, 11.0), 1.6, 20);
		PersonKit::Loft(B, Gold, {
			{ FVector(X + 14.0, F, Z - 4.0), FVector(0.0, 0.0, 2.0), FVector(0.0, Out * 0.5, 0.0) },
			{ FVector(X + 26.0, F, Z - 4.0), FVector(0.0, 0.0, 4.0), FVector(0.0, Out * 0.5, 0.0) },
			{ FVector(X + 32.0, F, Z - 4.0), FVector(0.0, 0.0, 9.0), FVector(0.0, Out * 0.5, 0.0) },
		}, 8, false, false);
		Box(B, Gold, FVector(X - 26.0, FMath::Min(F, F + Out) - 0.2, Z - 5.0), FVector(X - 14.0, FMath::Max(F, F + Out) + 0.2, Z - 3.0));
		// A cord with tassels hanging from the loop.
		Box(B, Gold, FVector(X - 1.0, FMath::Min(F, F + Out) - 0.2, Z - 22.0), FVector(X + 1.0, FMath::Max(F, F + Out) + 0.2, Z - 11.0));
		PersonKit::Blob(B, Gold, FVector(X, F, Z - 24.0), FVector(2.5, 1.0, 3.5), 8, 4);
		// The Holy Crown over it, its cross a little askew.
		PersonKit::Blob(B, Gold, FVector(X, F, Z + 20.0), FVector(8.0, 1.2, 6.0), 10, 5, 0.0);
		Box(B, Gold, FVector(X - 8.5, FMath::Min(F, F + Out) - 0.2, Z + 17.0), FVector(X + 8.5, FMath::Max(F, F + Out) + 0.2, Z + 20.0));
		Box(B, Gold, FVector(X + 0.2, FMath::Min(F, F + Out) - 0.2, Z + 26.0), FVector(X + 1.4, FMath::Max(F, F + Out) + 0.2, Z + 33.0));
		Box(B, Gold, FVector(X - 2.0, FMath::Min(F, F + Out) - 0.2, Z + 29.5), FVector(X + 3.6, FMath::Max(F, F + Out) + 0.2, Z + 30.7));
	}

	/** The post van's closed box body on the chassis: green panels, a yellow line, the horn and crown, double doors at the back, a roof rail. */
	inline void BuildPostBox(FPiece& B)
	{
		const FLinearColor Green = PostGreen();
		const FLinearColor Gold = PostYellow();
		const FLinearColor Black = Chassis();
		const double Back = BedBack + 30.0;
		const double Front = BedFront - 6.0;
		const double HalfY = BedHalfY - 8.0;
		const double Base = 100.0;
		const double Top = 252.0;
		// The floor bearers, the box, its roof a little wider, curved at the edges.
		for (double X = Back + 10.0; X < Front; X += 60.0)
		{
			Box(B, Black, FVector(X, -HalfY, 90.0), FVector(X + 8.0, HalfY, Base));
		}
		Box(B, Green, FVector(Back, -HalfY, Base), FVector(Front, HalfY, Top));
		PersonKit::Loft(B, Green * 0.85f, {
			{ FVector(Back - 3.0, 0.0, Top), FVector(0.0, HalfY + 3.0, 0.0), FVector(0.0, 0.0, 6.0) },
			{ FVector(Front + 3.0, 0.0, Top), FVector(0.0, HalfY + 3.0, 0.0), FVector(0.0, 0.0, 6.0) },
		}, 16, true, true);
		Box(B, Black, FVector(Back - 3.0, -HalfY - 3.0, Top - 1.0), FVector(Front + 3.0, HalfY + 3.0, Top + 0.5));
		// A rail round the roof for parcels.
		for (const double S : { -1.0, 1.0 })
		{
			Tube(B, Black, FVector(Back + 4.0, S * (HalfY - 4.0), Top + 12.0), FVector(Front - 4.0, S * (HalfY - 4.0), Top + 12.0), 1.0, 5);
			for (double X = Back + 4.0; X <= Front - 3.9; X += (Front - Back - 8.0) / 4.0)
			{
				Tube(B, Black, FVector(X, S * (HalfY - 4.0), Top + 4.0), FVector(X, S * (HalfY - 4.0), Top + 12.0), 0.8, 4);
			}
		}
		for (const double S : { -1.0, 1.0 })
		{
			const double Y = S * HalfY;
			// Raised panels with dark beading, the yellow line round the waist.
			for (int32 k = 0; k < 3; ++k)
			{
				const double X0 = Back + 8.0 + k * (Front - Back - 16.0) / 3.0;
				const double X1 = X0 + (Front - Back - 16.0) / 3.0 - 6.0;
				Box(B, Green * 0.6f, FVector(X0, Y - 0.3 * S, Base + 10.0), FVector(X1, Y + 0.8 * S, Base + 12.0));
				Box(B, Green * 0.6f, FVector(X0, Y - 0.3 * S, Top - 14.0), FVector(X1, Y + 0.8 * S, Top - 12.0));
				Box(B, Green * 0.6f, FVector(X0, Y - 0.3 * S, Base + 10.0), FVector(X0 + 2.0, Y + 0.8 * S, Top - 12.0));
				Box(B, Green * 0.6f, FVector(X1 - 2.0, Y - 0.3 * S, Base + 10.0), FVector(X1, Y + 0.8 * S, Top - 12.0));
			}
			Box(B, Gold, FVector(Back + 2.0, FMath::Min(Y, Y + S * 1.2), Base + 62.0), FVector(Front - 2.0, FMath::Max(Y, Y + S * 1.2), Base + 65.0));
			PostHorn(B, (Back + Front) * 0.5, Y, Base + 100.0, S);
		}
		// The back: two doors with a handle and hinges, the step, the lamp and plate.
		Box(B, Green * 0.9f, FVector(Back - 1.5, -HalfY + 4.0, Base + 6.0), FVector(Back, HalfY - 4.0, Top - 6.0));
		Box(B, Black, FVector(Back - 2.0, -1.0, Base + 6.0), FVector(Back - 1.0, 1.0, Top - 6.0));
		Box(B, Gold, FVector(Back - 2.0, -HalfY + 4.0, Base + 62.0), FVector(Back - 1.0, HalfY - 4.0, Base + 65.0));
		Tube(B, Nickel(), FVector(Back - 4.0, 6.0, Base + 70.0), FVector(Back - 4.0, 18.0, Base + 70.0), 1.0, 5);
		for (const double S : { -1.0, 1.0 })
		{
			for (const double Z : { Base + 30.0, Top - 30.0 })
			{
				Box(B, Black, FVector(Back - 2.5, S * (HalfY - 18.0), Z), FVector(Back - 1.5, S * (HalfY - 2.0), Z + 3.0));
			}
		}
		Box(B, Black, FVector(Back - 24.0, -50.0, 72.0), FVector(Back, 50.0, 76.0));
		Box(B, FLinearColor(0.80f, 0.79f, 0.74f), FVector(Back - 3.0, 30.0, 80.0), FVector(Back - 2.0, 74.0, 94.0));
		PersonKit::Blob(B, FLinearColor(0.55f, 0.05f, 0.03f), FVector(Back - 3.0, -60.0, 90.0), FVector(2.0, 4.0, 4.0), 8, 4);
		// Low guards over the twins, tucked under the box.
		for (const double S : { -1.0, 1.0 })
		{
			Guard(B, Black, BackAxle, WheelRadius + 2.0, WheelRadius + 9.0, S * (BackTrack - 22.0), S * (BackTrack + 22.0), 0.25, PI - 0.25);
		}
	}

	/** The lorry: chassis, bonnet, radiator, cab, platform and load; the wheels. */
	inline void Build(FPiece (&Parts)[NumParts], int32 PaintIndex, ELoad Load)
	{
		const FLinearColor Coat = Load == ELoad::Post ? PostGreen() : Paint(PaintIndex);
		const FLinearColor Black = Chassis();
		FPiece& B = Parts[Body];

		// The chassis rails, the front cross member and the spring hangers.
		for (const double S : { -1.0, 1.0 })
		{
			Box(B, Black, FVector(-300.0, S * 38.0 - 4.0, 56.0), FVector(300.0, S * 38.0 + 4.0, 72.0));
		}
		Box(B, Black, FVector(292.0, -46.0, 52.0), FVector(302.0, 46.0, 66.0));
		Box(B, Black, FVector(-310.0, -60.0, 54.0), FVector(-300.0, 60.0, 70.0));
		// The axles under the springs.
		Tube(B, Black, FVector(FrontAxle, -FrontTrack + 8.0, WheelRadius), FVector(FrontAxle, FrontTrack - 8.0, WheelRadius), 4.0, 8);
		Tube(B, Black, FVector(BackAxle, -BackTrack + 10.0, WheelRadius), FVector(BackAxle, BackTrack - 10.0, WheelRadius), 5.0, 8);
		PersonKit::Blob(B, Black, FVector(BackAxle, 0.0, WheelRadius), FVector(16.0, 18.0, 16.0), 10, 6);
		for (const double S : { -1.0, 1.0 })
		{
			for (const double X : { FrontAxle, BackAxle })
			{
				Box(B, Black, FVector(X - 45.0, S * 38.0 - 3.0, WheelRadius + 4.0), FVector(X + 45.0, S * 38.0 + 3.0, WheelRadius + 9.0));
			}
		}

		// The front bumper and the starting handle.
		Box(B, Black, FVector(302.0, -78.0, 46.0), FVector(310.0, 78.0, 54.0));
		Tube(B, Black, FVector(296.0, 0.0, 64.0), FVector(314.0, 0.0, 64.0), 1.4, 5);
		Tube(B, Black, FVector(314.0, 0.0, 64.0), FVector(314.0, 0.0, 52.0), 1.4, 5);

		// The radiator: a tall nickel shell, the dark honeycomb, the RÁBA badge, the filler cap.
		Box(B, Nickel(), FVector(282.0, -38.0, 64.0), FVector(296.0, 38.0, 170.0));
		Box(B, FLinearColor(0.05f, 0.05f, 0.05f), FVector(294.0, -31.0, 72.0), FVector(297.0, 31.0, 160.0));
		for (double Z = 76.0; Z < 158.0; Z += 6.0)
		{
			Box(B, FLinearColor(0.11f, 0.11f, 0.11f), FVector(296.5, -31.0, Z), FVector(297.2, 31.0, Z + 1.2));
		}
		Box(B, Coat, FVector(296.0, -16.0, 154.0), FVector(298.5, 16.0, 165.0));
		Box(B, FLinearColor(0.75f, 0.62f, 0.30f), FVector(298.4, -12.0, 156.0), FVector(298.9, 12.0, 163.0));
		Tube(B, Nickel(), FVector(289.0, 0.0, 170.0), FVector(289.0, 0.0, 176.0), 4.0, 10);

		// The bonnet: sides with louvres, the hinge down the middle, a little slope down to the radiator.
		PersonKit::Loft(B, Coat, {
			{ FVector(282.0, 0.0, 116.0), FVector(0.0, 40.0, 0.0), FVector(0.0, 0.0, 50.0) },
			{ FVector(170.0, 0.0, 118.0), FVector(0.0, 44.0, 0.0), FVector(0.0, 0.0, 52.0) },
			{ FVector(152.0, 0.0, 120.0), FVector(0.0, 46.0, 0.0), FVector(0.0, 0.0, 54.0) },
		}, 4, false, false, PI * 0.25);
		Box(B, Coat, FVector(152.0, -32.0, 162.0), FVector(282.0, 32.0, 168.0));
		Tube(B, Nickel(), FVector(152.0, 0.0, 168.5), FVector(282.0, 0.0, 168.5), 0.9, 5);
		for (const double S : { -1.0, 1.0 })
		{
			for (int32 k = 0; k < 9; ++k)
			{
				const double X = 180.0 + k * 10.0;
				Box(B, Coat * 0.45f, FVector(X, S * 37.5 - 0.6, 128.0), FVector(X + 2.0, S * 37.5 + 0.6, 152.0));
			}
		}

		// Front mudguards sweeping down into the running boards; headlamps on a bar between them.
		for (const double S : { -1.0, 1.0 })
		{
			Guard(B, Black, FrontAxle, WheelRadius, WheelRadius + 8.0, S * (FrontTrack - 14.0), S * (FrontTrack + 16.0), 0.15, PI);
			Box(B, Black, FVector(130.0, S * (FrontTrack - 14.0), 56.0), FVector(FrontAxle - WheelRadius - 6.0, S * (FrontTrack + 16.0), 60.0));
			Box(B, Black, FVector(-10.0, S * 66.0, 56.0), FVector(130.0, S * 94.0, 60.0));
			// Headlamp.
			TArray<PersonKit::FRing> Lamp;
			Lamp.Add({ FVector(262.0, S * 60.0, 132.0), FVector(0.0, 9.0, 0.0), FVector(0.0, 0.0, 9.0) });
			Lamp.Add({ FVector(278.0, S * 60.0, 132.0), FVector(0.0, 12.0, 0.0), FVector(0.0, 0.0, 12.0) });
			PersonKit::Loft(B, Black, Lamp, 14, true, false);
			Box(B, FLinearColor(0.92f, 0.88f, 0.70f), FVector(277.5, S * 60.0 - 10.0, 122.0), FVector(278.5, S * 60.0 + 10.0, 142.0));
			Ring(B, Nickel(), FVector(278.4, S * 60.0, 132.0), FVector(0.0, 12.0, 0.0), FVector(0.0, 0.0, 12.0), 1.0, 16);
		}
		Tube(B, Black, FVector(266.0, -60.0, 118.0), FVector(266.0, 60.0, 118.0), 1.4, 6);
		Tube(B, Black, FVector(266.0, -36.0, 118.0), FVector(282.0, -36.0, 100.0), 1.2, 5);
		Tube(B, Black, FVector(266.0, 36.0, 118.0), FVector(282.0, 36.0, 100.0), 1.2, 5);
		// The front number plate.
		Box(B, FLinearColor(0.80f, 0.79f, 0.74f), FVector(310.0, -22.0, 56.0), FVector(311.0, 22.0, 68.0));

		// The cab: the scuttle, a closed wooden cab with doors and windows, a flat roof with a peak.
		const double CabBack = 40.0;
		const double CabFront = 152.0;
		const double CabHalf = 78.0;
		const double Floor = 76.0;
		const double Waist = 160.0;
		const double Roof = 238.0;
		Box(B, Coat, FVector(CabFront - 14.0, -CabHalf + 18.0, Floor), FVector(CabFront, CabHalf - 18.0, 176.0));
		// Lower panels all round, the back panel full height.
		for (const double S : { -1.0, 1.0 })
		{
			Box(B, Coat, FVector(CabBack, S * CabHalf - 2.0, Floor), FVector(CabFront - 14.0, S * CabHalf + 2.0, Waist));
			Box(B, FLinearColor(0.75f, 0.62f, 0.30f), FVector(CabBack, S * CabHalf + 1.5, Waist - 6.0), FVector(CabFront - 14.0, S * CabHalf + 2.3, Waist - 4.0));
			// The door's edge and its handle.
			Box(B, Coat * 0.5f, FVector(CabFront - 20.0, S * CabHalf + 1.8, Floor + 4.0), FVector(CabFront - 18.0, S * CabHalf + 2.6, Waist));
			Box(B, Coat * 0.5f, FVector(CabBack + 10.0, S * CabHalf + 1.8, Floor + 4.0), FVector(CabBack + 12.0, S * CabHalf + 2.6, Waist));
			Tube(B, Nickel(), FVector(CabBack + 18.0, S * (CabHalf + 3.0), Waist - 16.0), FVector(CabBack + 30.0, S * (CabHalf + 3.0), Waist - 16.0), 0.9, 5);
			// The pillars and the side window.
			Box(B, Coat, FVector(CabBack, S * CabHalf - 2.0, Waist), FVector(CabBack + 8.0, S * CabHalf + 2.0, Roof));
			Box(B, Coat, FVector(CabFront - 22.0, S * CabHalf - 2.0, Waist), FVector(CabFront - 14.0, S * CabHalf + 2.0, Roof));
			Box(B, Glass(), FVector(CabBack + 8.0, S * CabHalf - 0.6, Waist), FVector(CabFront - 22.0, S * CabHalf + 0.6, Roof - 8.0));
			Box(B, Coat, FVector(CabBack + 8.0, S * CabHalf - 2.0, Roof - 8.0), FVector(CabFront - 22.0, S * CabHalf + 2.0, Roof));
			// The windscreen pillars, slightly raked.
			Box(B, Coat, FVector(CabFront - 16.0, S * (CabHalf - 4.0) - 3.0, 176.0), FVector(CabFront - 10.0, S * (CabHalf - 4.0) + 3.0, Roof));
		}
		Box(B, Coat, FVector(CabBack - 4.0, -CabHalf, Floor), FVector(CabBack, CabHalf, Roof));
		Box(B, Glass(), FVector(CabBack - 4.6, -30.0, 186.0), FVector(CabBack - 4.0, 30.0, 220.0));
		Box(B, Glass(), FVector(CabFront - 14.0, -CabHalf + 4.0, 176.0), FVector(CabFront - 12.6, CabHalf - 4.0, Roof - 6.0));
		Box(B, Coat, FVector(CabFront - 15.0, -2.0, 176.0), FVector(CabFront - 11.0, 2.0, Roof));
		// The roof with its peak over the windscreen, and a little lamp on each side of it.
		Box(B, Black, FVector(CabBack - 6.0, -CabHalf - 4.0, Roof), FVector(CabFront + 6.0, CabHalf + 4.0, Roof + 5.0));
		for (const double S : { -1.0, 1.0 })
		{
			PersonKit::Blob(B, Nickel(), FVector(CabFront - 10.0, S * (CabHalf + 4.0), 176.0), FVector(4.0, 4.0, 5.0), 8, 4);
		}
		// Inside: the bench, the steering wheel on the right, the gear lever.
		Box(B, SeatLeather(), FVector(CabBack, -CabHalf + 4.0, 118.0), FVector(CabBack + 48.0, CabHalf - 4.0, 130.0));
		Box(B, SeatLeather(), FVector(CabBack, -CabHalf + 4.0, 130.0), FVector(CabBack + 10.0, CabHalf - 4.0, 188.0));
		Tube(B, Black, FVector(CabFront - 16.0, 34.0, 150.0), FVector(CabFront - 40.0, 34.0, 176.0), 1.6, 5);
		Ring(B, Black, FVector(CabFront - 41.0, 34.0, 177.0), FVector(0.0, 18.0, 0.0), FVector(8.0, 0.0, 9.0), 1.3, 16);
		Tube(B, Black, FVector(CabFront - 30.0, 0.0, Floor), FVector(CabFront - 44.0, 0.0, 140.0), 1.0, 5);
		PersonKit::Blob(B, Black, FVector(CabFront - 44.0, 0.0, 141.0), FVector(3.0, 3.0, 3.0), 6, 3);
		// The step under each door.
		for (const double S : { -1.0, 1.0 })
		{
			Box(B, Black, FVector(CabBack + 20.0, S * 80.0, 82.0), FVector(CabBack + 56.0, S * 94.0, 84.0));
			Tube(B, Black, FVector(CabBack + 38.0, S * 80.0, 83.0), FVector(CabBack + 38.0, S * 78.0, 96.0), 1.0, 4);
		}

		if (Load == ELoad::Post)
		{
			BuildPostBox(B);
		}
		else
		{
			// The platform: cross bearers, the plank floor, drop sides with stakes, the tailboard.
			for (double X = BedBack + 10.0; X < BedFront; X += 60.0)
			{
				Box(B, DarkPlanks(), FVector(X, -BedHalfY, 96.0), FVector(X + 8.0, BedHalfY, 104.0));
			}
			Box(B, Planks(), FVector(BedBack, -BedHalfY, 104.0), FVector(BedFront, BedHalfY, BedZ));
			for (const double S : { -1.0, 1.0 })
			{
				const double Y = S * BedHalfY;
				// Three boards with gaps, and the iron edge.
				for (int32 k = 0; k < 3; ++k)
				{
					const double Z = BedZ + 1.0 + k * 16.0;
					Box(B, (k % 2) ? Planks() : Planks() * 0.88f, FVector(BedBack, Y - 2.5, Z), FVector(BedFront, Y + 2.5, Z + 14.0));
				}
				Box(B, Black, FVector(BedBack, Y - 3.0, BedZ + SideHeight - 1.0), FVector(BedFront, Y + 3.0, BedZ + SideHeight + 1.0));
				// The stakes and their iron pockets, the side hinges.
				for (double X = BedBack + 4.0; X < BedFront; X += (BedFront - BedBack - 8.0) / 4.0)
				{
					Box(B, DarkPlanks(), FVector(X, Y + S * 2.5, BedZ - 8.0), FVector(X + 6.0, Y + S * 7.5, BedZ + SideHeight + 4.0));
					Box(B, Black, FVector(X - 1.0, Y + S * 2.0, BedZ - 10.0), FVector(X + 7.0, Y + S * 8.0, BedZ - 2.0));
				}
				// Rear guards over the twins, flat in the way of the day.
				Guard(B, Black, BackAxle, WheelRadius + 2.0, WheelRadius + 10.0, S * (BackTrack - 22.0), S * (BackTrack + 22.0), 0.25, PI - 0.25);
			}
			// The headboard, taller, with a ladder rack over the cab roof.
			Box(B, DarkPlanks(), FVector(BedFront - 6.0, -BedHalfY, BedZ), FVector(BedFront, BedHalfY, BedZ + SideHeight + 50.0));
			for (int32 k = 0; k < 4; ++k)
			{
				Box(B, Planks(), FVector(BedFront - 7.0, -BedHalfY, BedZ + 4.0 + k * 24.0), FVector(BedFront - 5.5, BedHalfY, BedZ + 20.0 + k * 24.0));
			}
			// The tailboard, its chains, the back lamp and number plate.
			Box(B, Planks(), FVector(BedBack - 3.0, -BedHalfY, BedZ), FVector(BedBack + 2.0, BedHalfY, BedZ + SideHeight));
			Box(B, Black, FVector(BedBack - 3.5, -BedHalfY, BedZ + 20.0), FVector(BedBack - 2.5, BedHalfY, BedZ + 23.0));
			for (const double S : { -1.0, 1.0 })
			{
				Tube(B, Black, FVector(BedBack - 3.0, S * (BedHalfY - 4.0), BedZ + SideHeight), FVector(BedBack - 2.0, S * (BedHalfY - 4.0), BedZ + SideHeight - 30.0), 0.6, 4);
			}
			Box(B, FLinearColor(0.80f, 0.79f, 0.74f), FVector(BedBack - 6.0, 30.0, 70.0), FVector(BedBack - 5.0, 74.0, 84.0));
			PersonKit::Blob(B, FLinearColor(0.55f, 0.05f, 0.03f), FVector(BedBack - 6.0, -56.0, 80.0), FVector(2.0, 4.0, 4.0), 8, 4);
			Box(B, Black, FVector(BedBack - 4.0, -76.0, 66.0), FVector(BedBack + 4.0, 76.0, 72.0));

			BuildLoad(B, Load);
		}

		// The wheels: the front ones single, the back ones twins.
		const FLinearColor Disc = Coat * 0.8f;
		DiscWheel(Parts[WheelFrontLeft], Disc, FVector::ZeroVector, 16.0, -1.0);
		DiscWheel(Parts[WheelFrontRight], Disc, FVector::ZeroVector, 16.0, 1.0);
		for (const int32 w : { static_cast<int32>(WheelBackLeft), static_cast<int32>(WheelBackRight) })
		{
			const double Out = w == WheelBackLeft ? -1.0 : 1.0;
			DiscWheel(Parts[w], Disc, FVector(0.0, Out * TwinGap * 0.5, 0.0), 16.0, Out);
			DiscWheel(Parts[w], Disc, FVector(0.0, -Out * TwinGap * 0.5, 0.0), 16.0, -Out);
		}
	}
}
