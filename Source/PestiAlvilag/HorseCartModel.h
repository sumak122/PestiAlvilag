// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PersonKit.h"

/**
 * A heavy cart horse and the flat four-wheeled market cart it pulls (a
 * "stráfkocsi"), as Pista bácsi drives to the flea market.
 *
 * The horse is a big cold-blood: a deep barrel, a thick crested neck, a
 * plain long head with blinkers, feathered fetlocks over broad hooves. It
 * wears a padded collar with brass-tipped hames, a saddle pad with the shaft
 * loops, a belly band and breeching round the rump. It is built, like the
 * people, from the dress-up kit's lofts and blobs, in pieces that move: the
 * body, the neck with the head (from the base of the neck, so it can nod),
 * the tail (from its root, so it can swish), and each leg in two, the upper
 * part from the shoulder or hip and the lower from the knee or hock.
 *
 * The cart has a plank bed with low side boards, a raised bench and
 * footboard at the front, big rear wheels on a fixed axle and smaller front
 * wheels on a turntable. The front axle, the shafts and the swingletree turn
 * together with the horse about the turntable (Hinge): that is the forecart.
 * Its wheels are separate meshes, each with its middle at the hub, so they
 * can roll.
 *
 * Centimetres, X forward, Y to the right, Z up. The cart's origin is on the
 * ground under the middle of its bed; the forecart's pieces are in the
 * cart's frame, with their pivot at the hinge; the horse's origin is on the
 * ground under the middle of its body, HorseAt in front of the hinge.
 * Header-only so it can be built and looked at outside the engine.
 */
namespace HorseCartModel
{
	enum EHorsePart : int32
	{
		Body,
		Neck,
		Tail,
		UpperFrontLeft,
		UpperFrontRight,
		UpperBackLeft,
		UpperBackRight,
		LowerFrontLeft,
		LowerFrontRight,
		LowerBackLeft,
		LowerBackRight,
		NumHorseParts
	};

	enum ECartPart : int32 { Bed, Forecart, WheelFrontLeft, WheelFrontRight, WheelBackLeft, WheelBackRight, NumCartParts };

	enum class ECoat : uint8 { Bay, Chestnut, Grey, Black, Count };

	struct FHorse
	{
		PersonKit::FPiece Parts[NumHorseParts];
	};

	struct FCart
	{
		PersonKit::FPiece Parts[NumCartParts];
	};

	/** The leg a lower leg hangs from; the lower legs move with them. */
	inline EHorsePart UpperOf(int32 Lower) { return static_cast<EHorsePart>(Lower - (LowerFrontLeft - UpperFrontLeft)); }
	inline bool IsLower(int32 Part) { return Part >= LowerFrontLeft && Part <= LowerBackRight; }
	inline bool IsFront(int32 Part) { return Part == UpperFrontLeft || Part == UpperFrontRight || Part == LowerFrontLeft || Part == LowerFrontRight; }
	inline double SideOf(int32 Part) { return (Part - UpperFrontLeft) % 2 == 0 ? -1.0 : 1.0; }

	// The horse (cm, from its origin).
	constexpr double FrontLegX = 54.0;
	constexpr double BackLegX = -60.0;
	constexpr double FrontLegY = 17.0;
	constexpr double BackLegY = 18.0;
	constexpr double ShoulderZ = 122.0;
	constexpr double HipZ = 128.0;
	constexpr double KneeZ = 58.0;
	constexpr double HockZ = 60.0;
	constexpr double HockX = -72.0;
	/** About 165 cm at the withers. */
	constexpr double WithersZ = 165.0;

	inline FVector NeckBase() { return FVector(62.0, 0.0, 142.0); }
	inline FVector TailRoot() { return FVector(-90.0, 0.0, 150.0); }
	inline FVector LegTop(int32 Part)
	{
		return IsFront(Part) ? FVector(FrontLegX, FrontLegY * SideOf(Part), ShoulderZ) : FVector(BackLegX, BackLegY * SideOf(Part), HipZ);
	}
	inline FVector LegJoint(int32 Part)
	{
		return IsFront(Part) ? FVector(FrontLegX - 2.0, FrontLegY * SideOf(Part), KneeZ) : FVector(HockX, BackLegY * SideOf(Part), HockZ);
	}

	// The cart (cm, from its origin).
	constexpr double BedFront = 112.0;
	constexpr double BedBack = -150.0;
	constexpr double BedHalfWidth = 72.0;
	constexpr double BedTop = 95.0;
	constexpr double SideBoard = 28.0;
	constexpr double BackWheelX = -95.0;
	constexpr double BackWheelRadius = 52.0;
	constexpr double BackWheelY = 84.0;
	constexpr double FrontWheelRadius = 40.0;
	constexpr double FrontWheelY = 74.0;
	/** The turntable: the forecart and the horse turn about it. */
	constexpr double HingeX = 70.0;
	/** The horse's origin, this far in front of the hinge. */
	constexpr double HorseAt = 190.0;
	constexpr double BenchX = 96.0;
	constexpr double BenchTop = 132.0;
	/** Where the driver's hips are, on the bench. */
	inline FVector DriverHip() { return FVector(BenchX - 2.0, -18.0, BenchTop + 9.0); }
	/** The middle of the collar, and where the traces hook onto it (horse's frame). */
	inline FVector CollarCenter() { return FVector(74.0, 0.0, 154.0); }
	inline FVector TraceHook(double Side) { return CollarCenter() + FVector(4.0, 33.0 * Side, -6.0); }
	/** Where the bit is, for the reins (in the horse's frame, with the head still). */
	inline FVector Bit(double Side) { return FVector(134.0, 8.5 * Side, 165.0); }

	/** The middle of the whole rig, cart and horse (the actor's origin over it), and its half sizes. */
	constexpr double RigMiddleX = 128.0;
	inline FVector RigHalf() { return FVector(282.0, 88.0, 112.0); }

	/** Six crates on the bed, three by two (the middle of each crate's bottom). */
	constexpr int32 MaxCrates = 6;
	constexpr double CrateLength = 58.0;
	constexpr double CrateWidth = 44.0;
	constexpr double CrateHeight = 36.0;
	inline FVector CrateSpot(int32 Index)
	{
		return FVector(-112.0 + (Index / 2) * 64.0, (Index % 2 == 0 ? -1.0 : 1.0) * 30.0, BedTop);
	}

	struct FCoatColors
	{
		FLinearColor Hide;
		FLinearColor Points;
		FLinearColor Mane;
		FLinearColor Muzzle;
	};

	inline FCoatColors CoatColors(ECoat Coat)
	{
		switch (Coat)
		{
		case ECoat::Chestnut:
			return { FLinearColor(0.30f, 0.11f, 0.035f), FLinearColor(0.26f, 0.09f, 0.03f), FLinearColor(0.42f, 0.20f, 0.07f), FLinearColor(0.16f, 0.08f, 0.05f) };
		case ECoat::Grey:
			return { FLinearColor(0.50f, 0.49f, 0.47f), FLinearColor(0.34f, 0.33f, 0.32f), FLinearColor(0.62f, 0.61f, 0.58f), FLinearColor(0.10f, 0.09f, 0.09f) };
		case ECoat::Black:
			return { FLinearColor(0.025f, 0.022f, 0.022f), FLinearColor(0.018f, 0.016f, 0.016f), FLinearColor(0.012f, 0.011f, 0.011f), FLinearColor(0.04f, 0.035f, 0.035f) };
		default:
			return { FLinearColor(0.20f, 0.075f, 0.025f), FLinearColor(0.025f, 0.02f, 0.018f), FLinearColor(0.02f, 0.017f, 0.015f), FLinearColor(0.07f, 0.04f, 0.03f) };
		}
	}

	// Harness and cart colours.
	inline FLinearColor Leather() { return FLinearColor(0.045f, 0.022f, 0.012f); }
	inline FLinearColor CollarPad() { return FLinearColor(0.12f, 0.05f, 0.025f); }
	inline FLinearColor Brass() { return FLinearColor(0.55f, 0.38f, 0.12f); }
	inline FLinearColor Iron() { return FLinearColor(0.035f, 0.035f, 0.038f); }
	inline FLinearColor Hoof() { return FLinearColor(0.05f, 0.045f, 0.04f); }
	inline FLinearColor Feather() { return FLinearColor(0.55f, 0.52f, 0.47f); }
	inline FLinearColor Planks() { return FLinearColor(0.30f, 0.25f, 0.19f); }
	inline FLinearColor Frame() { return FLinearColor(0.20f, 0.13f, 0.075f); }
	inline FLinearColor BoardPaint() { return FLinearColor(0.07f, 0.16f, 0.09f); }
	inline FLinearColor WheelPaint() { return FLinearColor(0.32f, 0.05f, 0.03f); }
	inline FLinearColor CrateWood() { return FLinearColor(0.46f, 0.36f, 0.22f); }

	/** A ring across a path heading along Dir in the XZ plane: RY across, RV in the plane. */
	inline PersonKit::FRing Across(const FVector& Center, const FVector& Dir, double RY, double RV)
	{
		const FVector D = Dir.GetSafeNormal();
		return { Center, FVector(0.0, RY, 0.0), FVector(-D.Z, 0.0, D.X) * RV };
	}

	inline PersonKit::FRing AlongX(double X, double Z, double RY, double RZ, double Y = 0.0)
	{
		return { FVector(X, Y, Z), FVector(0.0, RY, 0.0), FVector(0.0, 0.0, RZ) };
	}

	/** A loop of rods round an ellipse in the plane of U and V (a collar, a strap round the body). */
	inline void Hoop(PersonKit::FPiece& P, const FLinearColor& Color, const FVector& Center, const FVector& U, const FVector& V, double Thick, int32 Steps = 16,
		double From = 0.0, double To = 2.0 * PI)
	{
		for (int32 i = 0; i < Steps; ++i)
		{
			const double A0 = From + (To - From) * i / Steps;
			const double A1 = From + (To - From) * (i + 1) / Steps;
			PersonKit::Rod(P, Color, Center + U * FMath::Cos(A0) + V * FMath::Sin(A0), Center + U * FMath::Cos(A1) + V * FMath::Sin(A1), Thick, 6);
		}
	}

	inline void BuildLeg(FHorse& Out, int32 Upper, const FCoatColors& C)
	{
		using namespace PersonKit;
		const double Side = SideOf(Upper);
		const bool bFront = IsFront(Upper);
		const FVector Top = LegTop(Upper);
		const FVector Joint = LegJoint(Upper);
		const double Y = Top.Y;

		FPiece& U = Out.Parts[Upper];
		U.Pivot = Top;
		if (bFront)
		{
			// The forearm, thick at the elbow and narrowing to the knee.
			Loft(U, C.Hide, {
				{ FVector(Top.X + 2.0, Y, Top.Z + 8.0), FVector(16.0, 0.0, 0.0), FVector(0.0, 13.0, 0.0) },
				{ FVector(Top.X + 1.0, Y, Top.Z - 10.0), FVector(14.0, 0.0, 0.0), FVector(0.0, 12.0, 0.0) },
				{ FVector(Top.X - 1.0, Y, 88.0), FVector(10.0, 0.0, 0.0), FVector(0.0, 9.0, 0.0) },
				{ FVector(Joint.X, Y, Joint.Z + 6.0), FVector(7.5, 0.0, 0.0), FVector(0.0, 7.0, 0.0) },
			}, 10, true, false);
		}
		else
		{
			// The gaskin, sloping back from the stifle to the hock.
			Loft(U, C.Hide, {
				{ FVector(Top.X + 6.0, Y, Top.Z + 4.0), FVector(20.0, 0.0, 0.0), FVector(0.0, 15.0, 0.0) },
				{ FVector(Top.X + 2.0, Y, 96.0), FVector(16.0, 0.0, 0.0), FVector(0.0, 13.0, 0.0) },
				{ FVector(HockX + 3.0, Y, Joint.Z + 8.0), FVector(8.5, 0.0, 0.0), FVector(0.0, 7.5, 0.0) },
			}, 10, true, false);
		}
		Blob(U, bFront ? C.Hide : C.Points, Joint + FVector(bFront ? 1.0 : -1.0, 0.0, 2.0), FVector(8.5, 7.5, 8.0), 8, 5);

		// The lower leg: cannon, fetlock with its feathering, pastern and the broad hoof.
		const int32 Lower = Upper + (LowerFrontLeft - UpperFrontLeft);
		FPiece& L = Out.Parts[Lower];
		L.Pivot = Joint;
		const double HoofX = bFront ? Joint.X + 2.0 : Joint.X + 6.0;
		Loft(L, C.Points, {
			{ FVector(Joint.X, Y, Joint.Z + 2.0), FVector(7.0, 0.0, 0.0), FVector(0.0, 6.5, 0.0) },
			{ FVector(HoofX - 1.0, Y, 24.0), FVector(6.0, 0.0, 0.0), FVector(0.0, 5.5, 0.0) },
		}, 8, false, false);
		Blob(L, C.Points, FVector(HoofX - 1.0, Y, 20.0), FVector(8.0, 7.5, 7.0), 8, 5);
		// Feathering: long pale hair round the fetlock, falling over the hoof.
		Loft(L, Feather(), {
			{ FVector(HoofX - 2.0, Y, 26.0), FVector(8.0, 0.0, 0.0), FVector(0.0, 7.5, 0.0) },
			{ FVector(HoofX - 1.0, Y, 14.0), FVector(9.5, 0.0, 0.0), FVector(0.0, 9.0, 0.0) },
			{ FVector(HoofX, Y, 6.0), FVector(10.5, 0.0, 0.0), FVector(0.0, 10.0, 0.0) },
		}, 10, false, false);
		Loft(L, Hoof(), {
			{ FVector(HoofX + 1.0, Y, 7.0), FVector(9.5, 0.0, 0.0), FVector(0.0, 9.0, 0.0) },
			{ FVector(HoofX + 2.0, Y, 0.0), FVector(11.0, 0.0, 0.0), FVector(0.0, 10.0, 0.0) },
		}, 10, false, true);
	}

	inline void BuildHorse(FHorse& Out, ECoat Coat, bool bBlaze = true)
	{
		using namespace PersonKit;
		const FCoatColors C = CoatColors(Coat);
		const FLinearColor White(0.72f, 0.70f, 0.66f);
		const FLinearColor Eye(0.02f, 0.015f, 0.012f);

		// The barrel, from the buttocks to the chest.
		FPiece& B = Out.Parts[Body];
		Loft(B, C.Hide, {
			AlongX(-92.0, 128.0, 14.0, 16.0),
			AlongX(-84.0, 130.0, 28.0, 30.0),
			AlongX(-66.0, 131.0, 35.0, 34.0),
			AlongX(-42.0, 130.0, 37.0, 36.0),
			AlongX(-10.0, 128.0, 39.0, 38.0),
			AlongX(24.0, 129.0, 39.0, 37.0),
			AlongX(50.0, 131.0, 34.0, 35.0),
			AlongX(70.0, 130.0, 26.0, 30.0),
			AlongX(82.0, 128.0, 15.0, 20.0),
		}, 16, true, true);
		// The croup over the hips, split down the middle as a heavy horse's is, and the withers.
		for (const double Side : { -1.0, 1.0 })
		{
			Blob(B, C.Hide, FVector(-60.0, 13.0 * Side, 150.0), FVector(28.0, 18.0, 14.0), 12, 6, -0.3);
			// The big shoulder and quarter muscles.
			Blob(B, C.Hide, FVector(52.0, 21.0 * Side, 126.0), FVector(24.0, 14.0, 28.0), 10, 6);
			Blob(B, C.Hide, FVector(-58.0, 23.0 * Side, 124.0), FVector(28.0, 13.0, 28.0), 10, 6);
		}
		Blob(B, C.Hide, FVector(44.0, 0.0, 158.0), FVector(24.0, 15.0, 11.0), 10, 5, -0.4);
		// The chest between the front legs.
		Blob(B, C.Hide, FVector(72.0, 0.0, 116.0), FVector(15.0, 24.0, 22.0), 10, 6);

		// Harness: the saddle pad with the shaft loops and the belly band.
		Block(B, Leather(), FVector(18.0, 0.0, 168.0), FVector(15.0, 20.0, 2.0));
		Hoop(B, Leather(), FVector(18.0, 0.0, 129.0), FVector(0.0, 41.0, 0.0), FVector(0.0, 0.0, 40.0), 1.8, 18);
		for (const double Side : { -1.0, 1.0 })
		{
			Block(B, Leather(), FVector(18.0, 39.5 * Side, 154.0), FVector(10.0, 1.5, 10.0));
			Hoop(B, Iron(), FVector(18.0, 43.0 * Side, 124.0), FVector(6.0, 0.0, 0.0), FVector(0.0, 0.0, 6.0), 1.3, 10);
			// Breeching round the quarters, held up by a strap over the croup.
			Rod(B, Leather(), FVector(-58.0, 37.5 * Side, 115.0), FVector(-92.0, 20.0 * Side, 115.0), 2.0);
			Rod(B, Leather(), FVector(-58.0, 37.5 * Side, 115.0), FVector(-56.0, 29.0 * Side, 157.0), 1.5);
			Rod(B, Leather(), FVector(-58.0, 37.5 * Side, 115.0), FVector(18.0, 41.0 * Side, 124.0), 1.5);
		}
		Hoop(B, Leather(), FVector(-93.0, 0.0, 115.0), FVector(0.0, 20.0, 0.0), FVector(-6.0, 0.0, 0.0), 2.0, 8, 0.0, PI);
		Rod(B, Leather(), FVector(-56.0, -29.0, 157.0), FVector(-56.0, 29.0, 157.0), 1.5);
		Rod(B, Leather(), FVector(-56.0, 0.0, 165.0), FVector(18.0, 0.0, 170.0), 1.7);

		// The collar: a thick padded ring round the base of the neck, the hames along it tipped with brass.
		const FVector CollarAt = CollarCenter();
		const FVector CollarU(0.0, 29.0, 0.0);
		const FVector CollarV = FVector(-0.45, 0.0, 1.0).GetSafeNormal() * 38.0;
		Hoop(B, CollarPad(), CollarAt, CollarU, CollarV, 7.5, 20);
		Hoop(B, Leather(), CollarAt + FVector(3.0, 0.0, 0.0), CollarU * 1.12, CollarV * 1.1, 2.8, 20);
		for (const double Side : { -1.0, 1.0 })
		{
			const FVector HameTop = CollarAt + CollarV * 1.1 + CollarU * 0.45 * Side + FVector(2.0, 0.0, 0.0);
			Rod(B, Brass(), HameTop, HameTop + FVector(-3.0, 1.0 * Side, 11.0), 1.7);
			Blob(B, Brass(), HameTop + FVector(-3.0, 1.0 * Side, 12.0), FVector(2.4, 2.4, 2.4), 6, 3);
			// Where the traces hook on, at the side.
			Blob(B, Brass(), TraceHook(Side), FVector(2.4, 2.4, 2.4), 6, 3);
		}

		// The neck, rising from the collar to the poll, and the long head going down to the muzzle.
		FPiece& N = Out.Parts[Neck];
		N.Pivot = NeckBase();
		const FVector Up(0.5, 0.0, 1.0);
		Loft(N, C.Hide, {
			Across(FVector(58.0, 0.0, 138.0), Up, 23.0, 30.0),
			Across(FVector(76.0, 0.0, 166.0), Up, 20.0, 25.0),
			Across(FVector(90.0, 0.0, 192.0), Up, 15.0, 18.0),
			Across(FVector(100.0, 0.0, 210.0), Up, 12.0, 13.5),
		}, 14, false, false);
		// The heavy crest along the top of the neck, and the mane on it.
		Blob(N, C.Hide, FVector(72.0, 0.0, 182.0), FVector(22.0, 11.0, 13.0), 10, 5);
		for (int32 k = 0; k < 7; ++k)
		{
			const double T = k / 6.0;
			const FVector At = FVector(56.0, 0.0, 174.0) + (FVector(100.0, 0.0, 222.0) - FVector(56.0, 0.0, 174.0)) * T;
			Blob(N, C.Mane, At + FVector(0.0, (k % 2 == 0 ? 3.0 : -3.0), 0.0), FVector(8.0, 6.0, 7.0), 8, 4);
		}
		const FVector Down = FVector(34.0, 0.0, -52.0);
		Loft(N, C.Hide, {
			Across(FVector(104.0, 0.0, 214.0), Down, 11.0, 12.0),
			Across(FVector(111.0, 0.0, 200.0), Down, 12.5, 17.0),
			Across(FVector(122.0, 0.0, 182.0), Down, 10.0, 12.0),
			Across(FVector(133.0, 0.0, 166.0), Down, 9.0, 10.5),
		}, 12, false, false);
		Blob(N, C.Muzzle, FVector(137.0, 0.0, 160.0), FVector(10.0, 9.5, 9.0), 10, 5);
		Blob(N, C.Hide, FVector(104.0, 0.0, 195.0), FVector(11.0, 11.0, 13.0), 10, 5);
		for (const double Side : { -1.0, 1.0 })
		{
			// Ears, eyes and nostrils.
			Loft(N, C.Hide, {
				{ FVector(101.0, 6.0 * Side, 221.0), FVector(3.0, 0.0, 0.0), FVector(0.0, 2.0, 0.0) },
				{ FVector(99.0, 8.0 * Side, 233.0), FVector(1.4, 0.0, 0.0), FVector(0.0, 1.0, 0.0) },
			}, 6, false, true);
			Blob(N, Eye, FVector(113.0, 11.0 * Side, 201.0), FVector(2.4, 1.2, 2.0), 6, 3);
			Blob(N, Eye, FVector(143.0, 5.0 * Side, 158.0), FVector(2.0, 1.4, 2.6), 6, 3);
			// The bridle: cheek straps, blinkers, the bit ring; the reins go from it.
			Rod(N, Leather(), FVector(102.0, 11.5 * Side, 217.0), FVector(132.0, 9.5 * Side, 167.0), 1.4);
			Block(N, Leather(), FVector(112.0, 13.0 * Side, 205.0), FVector(3.5, 0.6, 3.2));
			Hoop(N, Brass(), Bit(Side), FVector(2.5, 0.0, 0.0), FVector(0.0, 0.0, 2.5), 0.7, 8);
		}
		Hoop(N, Leather(), FVector(128.0, 0.0, 173.0), FVector(0.0, 11.0, 0.0), FVector(5.0, 0.0, 9.0), 1.3, 14);
		Hoop(N, Leather(), FVector(102.0, 0.0, 218.0), FVector(0.0, 12.0, 0.0), FVector(-2.0, 0.0, 3.0), 1.3, 10, 0.0, PI);
		Rod(N, Leather(), FVector(104.0, -11.0, 207.0), FVector(104.0, 11.0, 207.0), 1.2);
		Blob(N, C.Mane, FVector(106.0, 0.0, 218.0), FVector(5.0, 5.0, 4.0), 8, 4);
		if (bBlaze)
		{
			Rod(N, White, FVector(113.0, 0.0, 219.0), FVector(140.5, 0.0, 171.0), 3.2, 6);
		}

		// The tail, hanging long and thick from its root.
		FPiece& T = Out.Parts[Tail];
		T.Pivot = TailRoot();
		Loft(T, C.Mane, {
			{ FVector(-89.0, 0.0, 154.0), FVector(6.0, 0.0, 0.0), FVector(0.0, 6.0, 0.0) },
			{ FVector(-98.0, 0.0, 142.0), FVector(7.0, 0.0, 0.0), FVector(0.0, 7.5, 0.0) },
			{ FVector(-102.0, 0.0, 114.0), FVector(8.5, 0.0, 0.0), FVector(0.0, 9.0, 0.0) },
			{ FVector(-102.0, 0.0, 82.0), FVector(9.5, 0.0, 0.0), FVector(0.0, 10.0, 0.0) },
			{ FVector(-99.0, 0.0, 66.0), FVector(5.0, 0.0, 0.0), FVector(0.0, 6.0, 0.0) },
		}, 10, true, true);

		for (const int32 Upper : { UpperFrontLeft, UpperFrontRight, UpperBackLeft, UpperBackRight })
		{
			BuildLeg(Out, Upper, C);
		}
	}

	/** A wooden wheel round its hub (its middle), turning about Y: hub, spokes, felloe and iron tyre. */
	inline void BuildWheel(PersonKit::FPiece& W, double Radius)
	{
		using namespace PersonKit;
		const double Felloe = Radius - 5.0;
		const int32 Spokes = Radius > 45.0 ? 14 : 12;
		Loft(W, Frame(), {
			{ FVector(0.0, -9.0, 0.0), FVector(6.0, 0.0, 0.0), FVector(0.0, 0.0, 6.0) },
			{ FVector(0.0, 9.0, 0.0), FVector(8.0, 0.0, 0.0), FVector(0.0, 0.0, 8.0) },
		}, 10, true, true);
		Loft(W, Iron(), {
			{ FVector(0.0, -11.0, 0.0), FVector(4.0, 0.0, 0.0), FVector(0.0, 0.0, 4.0) },
			{ FVector(0.0, -9.0, 0.0), FVector(4.5, 0.0, 0.0), FVector(0.0, 0.0, 4.5) },
		}, 8, true, false);
		for (int32 s = 0; s < Spokes; ++s)
		{
			const double A = 2.0 * PI * s / Spokes;
			const FVector Out(FMath::Cos(A), 0.0, FMath::Sin(A));
			// Dished a little, as cart wheels are.
			Rod(W, WheelPaint(), Out * 7.0 + FVector(0.0, 2.0, 0.0), Out * (Felloe - 2.0), 1.8, 5);
		}
		Hoop(W, WheelPaint(), FVector::ZeroVector, FVector(Felloe, 0.0, 0.0), FVector(0.0, 0.0, Felloe), 3.4, 28);
		Hoop(W, Iron(), FVector::ZeroVector, FVector(Radius - 1.6, 0.0, 0.0), FVector(0.0, 0.0, Radius - 1.6), 2.0, 32);
	}

	/** One slatted crate, from the middle of its bottom: its long side along X, or along Y if bAcross. */
	inline void BuildCrate(PersonKit::FPiece& P, const FVector& At, bool bAcross = false)
	{
		using namespace PersonKit;
		const FLinearColor Wood = CrateWood();
		const FLinearColor Dark(0.30f, 0.22f, 0.13f);
		const double HX = CrateLength * 0.5;
		const double HY = CrateWidth * 0.5;
		auto Slat = [&P, &At, bAcross](const FLinearColor& Color, const FVector& Center, const FVector& Half)
		{
			Block(P, Color, At + (bAcross ? FVector(-Center.Y, Center.X, Center.Z) : Center), bAcross ? FVector(Half.Y, Half.X, Half.Z) : Half);
		};
		Slat(Dark, FVector(0.0, 0.0, 1.0), FVector(HX, HY, 1.0));
		for (int32 k = 0; k < 3; ++k)
		{
			const double Z = 6.0 + k * 11.5;
			for (const double Side : { -1.0, 1.0 })
			{
				Slat(Wood, FVector(0.0, (HY - 1.0) * Side, Z), FVector(HX, 1.0, 4.0));
				Slat(Wood, FVector((HX - 1.0) * Side, 0.0, Z), FVector(1.0, HY - 2.0, 4.0));
			}
		}
		for (const double SX : { -1.0, 1.0 })
		{
			for (const double SY : { -1.0, 1.0 })
			{
				Slat(Dark, FVector((HX - 2.5) * SX, (HY - 2.5) * SY, CrateHeight * 0.5), FVector(2.5, 2.5, CrateHeight * 0.5));
			}
		}
		// What is in it: straw, and the tops of jars and bundles.
		Slat(FLinearColor(0.55f, 0.46f, 0.22f), FVector(0.0, 0.0, CrateHeight - 6.0), FVector(HX - 2.5, HY - 2.5, 2.0));
	}

	inline void BuildCart(FCart& Out)
	{
		using namespace PersonKit;
		FPiece& B = Out.Parts[Bed];

		// The bed: two long beams under it, cross bearers, the planks.
		for (const double Side : { -1.0, 1.0 })
		{
			Block(B, Frame(), FVector((BedFront + BedBack) * 0.5, 48.0 * Side, BedTop - 14.0), FVector((BedFront - BedBack) * 0.5 + 6.0, 5.0, 5.0));
		}
		for (const double X : { BedBack + 8.0, BackWheelX, -30.0, 30.0, HingeX, BedFront - 6.0 })
		{
			Block(B, Frame(), FVector(X, 0.0, BedTop - 7.5), FVector(5.0, BedHalfWidth, 2.5));
		}
		for (int32 k = 0; k < 8; ++k)
		{
			const double Y = -BedHalfWidth + 9.0 + k * (2.0 * BedHalfWidth - 18.0) / 7.0;
			Block(B, Planks() * (k % 3 == 1 ? 0.88f : 1.f), FVector((BedFront + BedBack) * 0.5, Y, BedTop - 2.5), FVector((BedFront - BedBack) * 0.5, 8.6, 2.5));
		}
		// Low side boards, painted, with stakes; the tailboard hangs on chains.
		for (const double Side : { -1.0, 1.0 })
		{
			Block(B, BoardPaint(), FVector((BedFront + BedBack) * 0.5 - 6.0, (BedHalfWidth - 2.0) * Side, BedTop + SideBoard * 0.5),
				FVector((BedFront - BedBack) * 0.5 - 6.0, 2.0, SideBoard * 0.5));
			Block(B, Frame(), FVector((BedFront + BedBack) * 0.5 - 6.0, (BedHalfWidth - 1.5) * Side, BedTop + SideBoard + 1.5),
				FVector((BedFront - BedBack) * 0.5 - 6.0, 2.5, 1.5));
			for (const double X : { BedBack + 4.0, -95.0, -30.0, 30.0 })
			{
				Block(B, Iron(), FVector(X, (BedHalfWidth + 0.5) * Side, BedTop + SideBoard * 0.5 - 4.0), FVector(2.0, 1.0, SideBoard * 0.5 + 6.0));
			}
			Rod(B, Iron(), FVector(BedBack + 3.0, (BedHalfWidth - 4.0) * Side, BedTop + SideBoard - 2.0), FVector(BedBack - 1.0, (BedHalfWidth - 10.0) * Side, BedTop + 8.0), 0.8);
		}
		Block(B, BoardPaint(), FVector(BedBack + 2.0, 0.0, BedTop + SideBoard * 0.4), FVector(2.0, BedHalfWidth - 4.0, SideBoard * 0.4));

		// The bench at the front on its box, a back rail, the sloping footboard.
		Block(B, Frame(), FVector(BenchX, 0.0, (BedTop + BenchTop) * 0.5 - 2.0), FVector(16.0, BedHalfWidth - 6.0, (BenchTop - BedTop) * 0.5 - 2.0));
		Block(B, Planks(), FVector(BenchX, 0.0, BenchTop - 2.0), FVector(19.0, BedHalfWidth - 2.0, 2.0));
		Block(B, CollarPad(), FVector(BenchX - 2.0, 0.0, BenchTop + 1.5), FVector(15.0, BedHalfWidth - 10.0, 1.5));
		for (const double Side : { -1.0, 1.0 })
		{
			Rod(B, Iron(), FVector(BenchX - 16.0, (BedHalfWidth - 4.0) * Side, BenchTop), FVector(BenchX - 20.0, (BedHalfWidth - 4.0) * Side, BenchTop + 26.0), 1.2);
			// The brake lever by the driver's right hand, and its block on the back wheel.
			Rod(B, Iron(), FVector(BenchX + 2.0, BedHalfWidth + 3.0, BedTop - 8.0), FVector(BenchX - 6.0, BedHalfWidth + 3.0, BenchTop + 28.0), 1.4);
		}
		Rod(B, Iron(), FVector(BenchX - 20.0, -(BedHalfWidth - 4.0), BenchTop + 26.0), FVector(BenchX - 20.0, BedHalfWidth - 4.0, BenchTop + 26.0), 1.4);
		Block(B, Planks(), FVector(BedFront + 8.0, 0.0, BedTop + 6.0), FVector(3.0, BedHalfWidth - 8.0, 16.0));
		Block(B, Frame(), FVector(BedFront + 3.0, 0.0, BedTop - 1.0), FVector(8.0, BedHalfWidth - 8.0, 1.5));

		// The rear axle and its springs; the turntable ring under the front.
		Rod(B, Iron(), FVector(BackWheelX, -BackWheelY + 8.0, BackWheelRadius), FVector(BackWheelX, BackWheelY - 8.0, BackWheelRadius), 3.5, 8);
		for (const double Side : { -1.0, 1.0 })
		{
			for (int32 k = 0; k < 3; ++k)
			{
				const double Half = 42.0 - k * 9.0;
				Block(B, Iron(), FVector(BackWheelX, 48.0 * Side, BackWheelRadius + 6.0 + k * 2.2 + (42.0 - Half) * 0.3), FVector(Half, 3.0, 1.0));
			}
			Rod(B, Iron(), FVector(BackWheelX, 48.0 * Side, BackWheelRadius + 3.0), FVector(BackWheelX, 48.0 * Side, BedTop - 19.0), 2.0);
		}
		Hoop(B, Iron(), FVector(HingeX, 0.0, BedTop - 11.0), FVector(36.0, 0.0, 0.0), FVector(0.0, 36.0, 0.0), 2.0, 20);

		// The forecart: front axle, the bolster on the turntable, the shafts and the swingletree.
		FPiece& F = Out.Parts[Forecart];
		F.Pivot = FVector(HingeX, 0.0, 0.0);
		Rod(F, Iron(), FVector(HingeX, -FrontWheelY + 8.0, FrontWheelRadius), FVector(HingeX, FrontWheelY - 8.0, FrontWheelRadius), 3.2, 8);
		Block(F, Frame(), FVector(HingeX, 0.0, FrontWheelRadius + 8.0), FVector(7.0, FrontWheelY - 12.0, 6.0));
		Block(F, Frame(), FVector(HingeX, 0.0, BedTop - 16.0), FVector(6.0, 40.0, 3.0));
		Rod(F, Iron(), FVector(HingeX, 0.0, FrontWheelRadius + 14.0), FVector(HingeX, 0.0, BedTop - 13.0), 2.5);
		const double ShaftTipX = HingeX + HorseAt + 72.0;
		for (const double Side : { -1.0, 1.0 })
		{
			// The futchels that hold the shafts, then the shafts, bending out round the horse's belly up to the shaft loops.
			Rod(F, Frame(), FVector(HingeX + 4.0, 34.0 * Side, FrontWheelRadius + 8.0), FVector(HingeX + 70.0, 46.0 * Side, 74.0), 3.2);
			const FVector Loop = FVector(HingeX + HorseAt + 18.0, 43.0 * Side, 124.0);
			const FVector Bend = FVector(HingeX + HorseAt - 80.0, 50.0 * Side, 102.0);
			Rod(F, Frame(), FVector(HingeX + 70.0, 46.0 * Side, 74.0), Bend, 3.4);
			Rod(F, Frame(), Bend, Loop, 3.2);
			Rod(F, Frame(), Loop, FVector(ShaftTipX, 40.0 * Side, 132.0), 2.8);
			Blob(F, Iron(), FVector(ShaftTipX, 40.0 * Side, 132.0), FVector(3.5, 3.5, 3.5), 6, 3);
			// The breeching hooks onto the shafts here.
			Blob(F, Iron(), FVector(HingeX + HorseAt - 58.0, 47.0 * Side, 110.0), FVector(2.5, 2.5, 2.5), 6, 3);
			// The traces, from the collar back to the swingletree.
			Rod(F, Leather(), FVector(HingeX + HorseAt, 0.0, 0.0) + TraceHook(Side), FVector(HingeX + 112.0, 40.0 * Side, 78.0), 1.6, 4);
		}
		Rod(F, Frame(), FVector(HingeX + 70.0, -46.0, 74.0), FVector(HingeX + 70.0, 46.0, 74.0), 3.0);
		Rod(F, Frame(), FVector(HingeX + 112.0, -44.0, 78.0), FVector(HingeX + 112.0, 44.0, 78.0), 2.6);
		Rod(F, Iron(), FVector(HingeX + 70.0, 0.0, 74.0), FVector(HingeX + 112.0, 0.0, 78.0), 1.6);

		// The wheels, each round its hub.
		auto Wheel = [&Out](int32 Part, const FVector& Hub, double Radius)
		{
			FPiece& W = Out.Parts[Part];
			W.Pivot = Hub;
			FPiece Round;
			BuildWheel(Round, Radius);
			// Turned so the hub cap faces outward on both sides.
			const bool bLeft = Hub.Y < 0.0;
			for (const FSection& From : Round.Sections)
			{
				FSection& Into = W.Section(From.Color);
				const int32 First = Into.Vertices.Num();
				for (int32 i = 0; i < From.Vertices.Num(); ++i)
				{
					const FVector V = From.Vertices[i];
					const FVector N = From.Normals[i];
					Into.Vertices.Add(bLeft ? V : FVector(V.X, -V.Y, V.Z));
					Into.Normals.Add(bLeft ? N : FVector(N.X, -N.Y, N.Z));
				}
				for (int32 t = 0; t < From.Triangles.Num(); t += 3)
				{
					Into.Triangles.Add(First + From.Triangles[t]);
					Into.Triangles.Add(First + From.Triangles[t + (bLeft ? 1 : 2)]);
					Into.Triangles.Add(First + From.Triangles[t + (bLeft ? 2 : 1)]);
				}
			}
		};
		Wheel(WheelFrontLeft, FVector(HingeX, -FrontWheelY, FrontWheelRadius), FrontWheelRadius);
		Wheel(WheelFrontRight, FVector(HingeX, FrontWheelY, FrontWheelRadius), FrontWheelRadius);
		Wheel(WheelBackLeft, FVector(BackWheelX, -BackWheelY, BackWheelRadius), BackWheelRadius);
		Wheel(WheelBackRight, FVector(BackWheelX, BackWheelY, BackWheelRadius), BackWheelRadius);
	}

	/** The front wheels' hubs are on the forecart, so they turn with it. */
	inline bool OnForecart(int32 Part) { return Part == WheelFrontLeft || Part == WheelFrontRight; }

	enum class EGait : uint8 { Stand, Walk, Trot, Canter };

	/** How the horse holds itself at one moment of its stride (degrees, cm). */
	struct FHorsePose
	{
		float Upper[4] = { 0.f, 0.f, 0.f, 0.f };
		float Lower[4] = { 0.f, 0.f, 0.f, 0.f };
		/** Each hoof on the ground (for the clop as it lands). */
		bool Planted[4] = { true, true, true, true };
		float Lift = 0.f;
		float Pitch = 0.f;
		float NeckPitch = 0.f;
		float TailPitch = 0.f;
	};

	/**
	 * The legs at Phase (0..1) of one stride. Legs in the order front left,
	 * front right, back left, back right. A positive upper angle swings the
	 * leg forward; the knee folds the hoof back, the hock forward.
	 */
	inline FHorsePose Pose(EGait Gait, float Phase)
	{
		FHorsePose P;
		if (Gait == EGait::Stand)
		{
			return P;
		}
		// When each leg's stride starts, how long it stays on the ground, and how far it swings.
		const float WalkStart[4] = { 0.25f, 0.75f, 0.f, 0.5f };
		const float TrotStart[4] = { 0.f, 0.5f, 0.5f, 0.f };
		const float CanterStart[4] = { 0.30f, 0.45f, 0.f, 0.30f };
		const float* Start = Gait == EGait::Walk ? WalkStart : Gait == EGait::Trot ? TrotStart : CanterStart;
		const float Ground = Gait == EGait::Walk ? 0.62f : Gait == EGait::Trot ? 0.48f : 0.42f;
		const float Swing = Gait == EGait::Walk ? 17.f : Gait == EGait::Trot ? 23.f : 30.f;
		const float Fold = Gait == EGait::Walk ? 45.f : Gait == EGait::Trot ? 75.f : 85.f;
		for (int32 l = 0; l < 4; ++l)
		{
			float T = Phase - Start[l];
			T -= FMath::FloorToFloat(T);
			const bool bFront = l < 2;
			P.Planted[l] = T < Ground;
			if (T < Ground)
			{
				// On the ground: the body passes over the hoof.
				P.Upper[l] = Swing * (1.f - 2.f * T / Ground);
				P.Lower[l] = 0.f;
			}
			else
			{
				const float S = (T - Ground) / (1.f - Ground);
				P.Upper[l] = -Swing + 2.f * Swing * (0.5f - 0.5f * FMath::Cos(PI * S));
				const float Bend = FMath::Sin(PI * FMath::Min(S * 1.2f, 1.f)) * Fold;
				P.Lower[l] = bFront ? -Bend : Bend * 0.8f;
				if (!bFront)
				{
					P.Upper[l] += Bend * 0.25f;
				}
			}
			P.Upper[l] *= bFront ? 1.f : 0.85f;
		}
		const float Beats = Gait == EGait::Walk ? 2.f : Gait == EGait::Trot ? 2.f : 1.f;
		const float Bob = FMath::Sin(2.f * PI * Phase * Beats);
		P.Lift = Gait == EGait::Walk ? 1.2f * Bob : Gait == EGait::Trot ? 3.5f * Bob : 6.f * Bob;
		P.Pitch = Gait == EGait::Canter ? 3.f * FMath::Cos(2.f * PI * Phase) : 0.f;
		// The head nods at the walk, is steadier at the trot, swings at the canter.
		P.NeckPitch = Gait == EGait::Walk ? -4.f * Bob : Gait == EGait::Trot ? -1.5f * Bob : -6.f * FMath::Cos(2.f * PI * Phase);
		P.TailPitch = 4.f * Bob;
		return P;
	}
}
