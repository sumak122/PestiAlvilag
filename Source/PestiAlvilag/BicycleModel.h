// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PersonKit.h"

/**
 * A shop's delivery bicycle of the early 1930s, as the errand boys
 * (kifutófiúk) of Pest rode them: a heavy black roadster with a small
 * front wheel under a big wicker basket on an iron carrier fixed to the
 * frame, a painted shop plate in the frame, upright swept-back bars with a
 * bell, a sprung leather saddle, mudguards, a chain guard and a carbide
 * lamp.
 *
 * Pieces that move: the frame (with the carrier and basket, which do not
 * turn), the steering (fork, bars, bell, lamp), each wheel round its hub,
 * and the cranks with the pedals round the bottom bracket. The steering
 * turns about SteerAxis() through HeadBottom(). Two kinds (EKind): the shop
 * bicycle above, and an ordinary roadster with two big wheels and the
 * basket on a rack on its fork.
 *
 * The rider is a dress-up kit person (PersonKit) with the hips on the
 * saddle: RiderPose gives the lean of the body and the angles of thighs,
 * shins, upper arms and forearms that put the feet on the pedals and the
 * hands on the grips, for any turn of the cranks.
 *
 * Centimetres, X forward, Y to the right, Z up; the origin is on the ground
 * under the bottom bracket. Header-only so it can be built and looked at
 * outside the engine.
 */
namespace BicycleModel
{
	enum EPart : int32 { Frame, Steer, WheelFront, WheelRear, Cranks, NumParts };

	struct FBicycle
	{
		PersonKit::FPiece Parts[NumParts];
	};

	// Where things are (cm, from the origin).
	inline FVector BottomBracket() { return FVector(0.0, 0.0, 28.0); }
	constexpr double CrankLength = 17.0;
	constexpr double RearRadius = 35.0;
	inline FVector RearAxle() { return FVector(-62.0, 0.0, RearRadius); }
	inline FVector SeatCluster() { return FVector(-14.0, 0.0, 82.0); }
	inline FVector SaddleTop() { return FVector(-19.0, 0.0, 90.0); }

	/**
	 * The two kinds: the shop bicycle with a small front wheel under a basket
	 * fixed to the frame, and an ordinary roadster with two big wheels and a
	 * basket on a rack over the front wheel, which turns with the bars.
	 */
	enum class EKind : uint8 { ShopBike, Roadster, Count };

	inline bool IsRoadster(EKind Kind) { return Kind == EKind::Roadster; }
	inline double FrontRadius(EKind Kind) { return IsRoadster(Kind) ? RearRadius : 26.0; }
	inline FVector FrontAxle(EKind Kind) { return IsRoadster(Kind) ? FVector(82.0, 0.0, RearRadius) : FVector(80.0, 0.0, 26.0); }
	inline FVector HeadBottom(EKind Kind) { return IsRoadster(Kind) ? FVector(64.0, 0.0, 76.0) : FVector(64.0, 0.0, 56.0); }
	inline FVector HeadTop(EKind Kind) { return IsRoadster(Kind) ? FVector(58.0, 0.0, 94.0) : FVector(59.0, 0.0, 76.0); }
	/** The steering turns about this line, through HeadBottom (a unit vector, tilted back). */
	inline FVector SteerAxis(EKind Kind) { return (HeadTop(Kind) - HeadBottom(Kind)).GetSafeNormal(); }
	/** How far back from upright the steering axis leans (degrees). */
	inline double SteerTilt(EKind Kind) { return FMath::RadiansToDegrees(FMath::Asin(-SteerAxis(Kind).X)); }
	inline FVector Grip(EKind Kind, double Side) { return FVector(38.0, 25.0 * Side, IsRoadster(Kind) ? 95.0 : 90.0); }
	/** The carrier platform the basket stands on (middle of its top), and the basket's half sizes. */
	inline FVector CarrierTop(EKind Kind) { return IsRoadster(Kind) ? FVector(93.0, 0.0, 79.0) : FVector(96.0, 0.0, 62.0); }
	inline FVector BasketHalf() { return FVector(24.0, 22.0, 11.0); }
	/** True when the basket (and the parcels in it) belong to the Steer part and turn with the bars. */
	inline bool BasketTurns(EKind Kind) { return IsRoadster(Kind); }
	/** The middle of the shop plate in the frame. */
	inline FVector PlateMiddle(EKind Kind) { return IsRoadster(Kind) ? FVector(26.0, 0.0, 67.0) : FVector(28.0, 0.0, 58.5); }
	/** The whole bicycle's middle and half sizes, for a collision box (fits both kinds). */
	inline FVector BoxMiddle() { return FVector(12.0, 0.0, 52.0); }
	inline FVector BoxHalf() { return FVector(110.0, 30.0, 52.0); }

	/** The pedal spindle of one side (Side +1 right, -1 left) at a turn of the cranks: 0 is the right crank straight up, 90 forward. */
	inline FVector Pedal(double CrankDegrees, double Side)
	{
		const double A = FMath::DegreesToRadians(CrankDegrees + (Side > 0.0 ? 0.0 : 180.0));
		return BottomBracket() + FVector(FMath::Sin(A) * CrankLength, 15.0 * Side, FMath::Cos(A) * CrankLength);
	}

	// Colours.
	inline FLinearColor Enamel() { return FLinearColor(0.018f, 0.018f, 0.02f); }
	inline FLinearColor Nickel() { return FLinearColor(0.52f, 0.52f, 0.54f); }
	inline FLinearColor Tyre() { return FLinearColor(0.045f, 0.042f, 0.04f); }
	inline FLinearColor Saddle() { return FLinearColor(0.16f, 0.07f, 0.03f); }
	inline FLinearColor Wicker() { return FLinearColor(0.50f, 0.36f, 0.19f); }
	inline FLinearColor WickerDark() { return FLinearColor(0.36f, 0.25f, 0.12f); }
	inline FLinearColor PlateCream() { return FLinearColor(0.78f, 0.70f, 0.52f); }
	inline FLinearColor PlateRed() { return FLinearColor(0.40f, 0.05f, 0.04f); }
	inline FLinearColor Brass() { return FLinearColor(0.55f, 0.38f, 0.12f); }
	inline FLinearColor Paper() { return FLinearColor(0.55f, 0.42f, 0.27f); }
	inline FLinearColor Twine() { return FLinearColor(0.80f, 0.74f, 0.60f); }

	/**
	 * A bent tube round Center in the XZ plane (at Center.Y), from angle From
	 * to To (radians, 0 is +X, PI/2 is up), with a cross-section RadialHalf
	 * across the bend and WideHalf along Y. Cut into short lofts so each
	 * faces the right way.
	 */
	inline void Arc(PersonKit::FPiece& P, const FLinearColor& Color, const FVector& Center, double Radius, double From, double To,
		double RadialHalf, double WideHalf, int32 Sides = 6)
	{
		const int32 Chunks = FMath::Max(1, static_cast<int32>(FMath::Abs(To - From) / (PI * 0.4)) + 1);
		const int32 Steps = 6;
		for (int32 c = 0; c < Chunks; ++c)
		{
			TArray<PersonKit::FRing> Rings;
			for (int32 s = 0; s <= Steps; ++s)
			{
				const double A = From + (To - From) * (c * Steps + s) / (Chunks * Steps);
				const FVector Radial(FMath::Cos(A), 0.0, FMath::Sin(A));
				Rings.Add({ Center + Radial * Radius, Radial * RadialHalf, FVector(0.0, WideHalf, 0.0) });
			}
			PersonKit::Loft(P, Color, Rings, Sides, false, false, Sides == 4 ? PI * 0.25 : 0.0);
		}
	}

	/** A spoked wheel round its hub (its middle), turning about Y: hub, 32 spokes, nickel rim and black tyre. */
	inline void BuildWheel(PersonKit::FPiece& W, double Radius)
	{
		using namespace PersonKit;
		Loft(W, Nickel(), {
			{ FVector(0.0, -5.5, 0.0), FVector(1.6, 0.0, 0.0), FVector(0.0, 0.0, 1.6) },
			{ FVector(0.0, 5.5, 0.0), FVector(1.6, 0.0, 0.0), FVector(0.0, 0.0, 1.6) },
		}, 8, true, true);
		for (const double Y : { -3.5, 3.5 })
		{
			Loft(W, Nickel(), {
				{ FVector(0.0, Y - 0.4, 0.0), FVector(3.0, 0.0, 0.0), FVector(0.0, 0.0, 3.0) },
				{ FVector(0.0, Y + 0.4, 0.0), FVector(3.0, 0.0, 0.0), FVector(0.0, 0.0, 3.0) },
			}, 10, true, true);
		}
		const double RimR = Radius - 3.4;
		const int32 Spokes = 32;
		for (int32 s = 0; s < Spokes; ++s)
		{
			// Laced across: each spoke leaves the flange a little round from where it meets the rim.
			const double A = 2.0 * PI * s / Spokes;
			const double Lead = (s % 2 == 0 ? 1.0 : -1.0) * 0.5;
			const double Y = (s % 4 < 2) ? -3.5 : 3.5;
			const FVector Hub(FMath::Cos(A + Lead) * 2.8, Y, FMath::Sin(A + Lead) * 2.8);
			const FVector Rim(FMath::Cos(A) * (RimR - 0.6), 0.0, FMath::Sin(A) * (RimR - 0.6));
			Rod(W, Nickel(), Hub, Rim, 0.22, 3);
		}
		Arc(W, Nickel(), FVector::ZeroVector, RimR, 0.0, 2.0 * PI, 1.1, 1.3, 6);
		Arc(W, Tyre(), FVector::ZeroVector, Radius - 1.7, 0.0, 2.0 * PI, 1.8, 2.0, 8);
	}

	/** A parcel in brown paper tied with string, from the middle of its bottom (for the basket, or held). */
	inline void BuildParcel(PersonKit::FPiece& P, const FVector& At, const FVector& Half)
	{
		using namespace PersonKit;
		Block(P, Paper(), At + FVector(0.0, 0.0, Half.Z), Half);
		Block(P, Twine(), At + FVector(0.0, 0.0, Half.Z), FVector(Half.X + 0.2, 0.35, Half.Z + 0.2));
		Block(P, Twine(), At + FVector(0.0, 0.0, Half.Z), FVector(0.35, Half.Y + 0.2, Half.Z + 0.2));
		Blob(P, Twine(), At + FVector(0.0, 0.0, Half.Z * 2.0 + 0.4), FVector(1.4, 1.4, 0.8), 6, 2);
	}

	/** Up to three parcels in the basket (the middle of each one's bottom, and its half sizes). */
	constexpr int32 MaxParcels = 3;
	inline FVector ParcelSpot(EKind Kind, int32 Index)
	{
		const FVector Floor = CarrierTop(Kind) + FVector(0.0, 0.0, 1.5);
		return Index == 0 ? Floor + FVector(-8.0, -9.0, 0.0) : Index == 1 ? Floor + FVector(6.0, 9.0, 0.0) : Floor + FVector(-2.0, -2.0, 15.0);
	}
	inline FVector ParcelHalf(int32 Index)
	{
		return Index == 0 ? FVector(13.0, 10.0, 7.5) : Index == 1 ? FVector(10.0, 9.0, 9.0) : FVector(11.0, 8.0, 5.0);
	}

	/** The carrier and the wicker basket on it (in the frame, or on the fork of a roadster). */
	inline void BuildBasket(PersonKit::FPiece& F, EKind Kind)
	{
		using namespace PersonKit;
		// The carrier: an iron frame from the head tube out over the front wheel, braced down to the axle.
		const FVector C = CarrierTop(Kind);
		const FVector BH = BasketHalf();
		Block(F, Enamel(), C + FVector(0.0, 0.0, -0.8), FVector(BH.X + 1.0, BH.Y + 1.0, 0.8));
		const FVector Axle = FrontAxle(Kind);
		for (const double Side : { -1.0, 1.0 })
		{
			if (IsRoadster(Kind))
			{
				// A rack on the fork: back corners on the crown, front corners braced to the axle.
				const FVector Crown = HeadBottom(Kind) - SteerAxis(Kind) * 6.0;
				Rod(F, Enamel(), Crown + FVector(1.0, 5.0 * Side, 0.0), C + FVector(-BH.X + 3.0, 12.0 * Side, -1.5), 0.9, 6);
				Rod(F, Enamel(), HeadTop(Kind) + FVector(1.5, 0.0, -3.0), C + FVector(-BH.X, 10.0 * Side, -1.0), 0.8, 5);
			}
			else
			{
				Rod(F, Enamel(), HeadTop(Kind) + FVector(1.0, 0.0, -4.0), C + FVector(-BH.X, 14.0 * Side, -1.0), 1.0, 6);
				Rod(F, Enamel(), HeadBottom(Kind) + FVector(2.0, 0.0, -2.0), C + FVector(-BH.X + 4.0, 14.0 * Side, -1.5), 1.0, 6);
				Rod(F, Enamel(), C + FVector(-BH.X + 2.0, (BH.Y - 1.0) * Side, -1.5), Axle + FVector(-6.0, 7.5 * Side, 0.0), 0.8, 5);
			}
			Rod(F, Enamel(), C + FVector(BH.X - 2.0, (BH.Y - 1.0) * Side, -1.5), Axle + FVector(6.0, 7.5 * Side, 0.0), 0.8, 5);
		}

		// The basket: woven sides, a rim, upright stakes and a floor.
		const FVector Floor = C + FVector(0.0, 0.0, 0.8);
		Block(F, WickerDark(), Floor, FVector(BH.X, BH.Y, 0.8));
		for (const double Side : { -1.0, 1.0 })
		{
			for (int32 k = 0; k < 5; ++k)
			{
				const double Z = Floor.Z + 2.5 + k * 4.3;
				const FLinearColor& Color = k % 2 == 0 ? Wicker() : WickerDark();
				Block(F, Color, FVector(Floor.X, (BH.Y - 0.6) * Side, Z), FVector(BH.X, 0.9, 2.2));
				Block(F, Color, FVector(Floor.X + (BH.X - 0.6) * Side, 0.0, Z), FVector(0.9, BH.Y, 2.2));
			}
			Rod(F, Wicker(), Floor + FVector(-BH.X, (BH.Y - 0.6) * Side, BH.Z * 2.0), Floor + FVector(BH.X, (BH.Y - 0.6) * Side, BH.Z * 2.0), 1.5, 6);
			Rod(F, Wicker(), Floor + FVector((BH.X - 0.6) * Side, -BH.Y, BH.Z * 2.0), Floor + FVector((BH.X - 0.6) * Side, BH.Y, BH.Z * 2.0), 1.5, 6);
			for (int32 k = -2; k <= 2; ++k)
			{
				Rod(F, WickerDark(), Floor + FVector(k * BH.X * 0.45, (BH.Y - 0.2) * Side, 0.0), Floor + FVector(k * BH.X * 0.45, (BH.Y - 0.2) * Side, BH.Z * 2.0), 0.7, 4);
				Rod(F, WickerDark(), Floor + FVector((BH.X - 0.2) * Side, k * BH.Y * 0.45, 0.0), Floor + FVector((BH.X - 0.2) * Side, k * BH.Y * 0.45, BH.Z * 2.0), 0.7, 4);
			}
		}
	}

	inline void BuildFrame(PersonKit::FPiece& F, EKind Kind)
	{
		using namespace PersonKit;
		const FVector BB = BottomBracket();
		const FVector Rear = RearAxle();
		const FVector Seat = SeatCluster();
		const double Tube = 1.7;

		// The main triangle; the shop bicycle has a second, lower top tube.
		Rod(F, Enamel(), BB, Seat + FVector(-0.8, 0.0, 4.0), Tube, 8);
		Rod(F, Enamel(), Seat, HeadTop(Kind) + FVector(0.0, 0.0, -2.0), Tube, 8);
		Rod(F, Enamel(), BB + FVector(-0.5, 0.0, 0.0), HeadBottom(Kind), Tube * 1.15, 8);
		if (!IsRoadster(Kind))
		{
			Rod(F, Enamel(), FVector(-11.0, 0.0, 66.0), FVector(62.0, 0.0, 64.0), Tube * 0.85, 8);
		}
		Rod(F, Enamel(), HeadBottom(Kind) - SteerAxis(Kind) * 2.0, HeadTop(Kind) + SteerAxis(Kind) * 1.5, 2.4, 10);
		Loft(F, Enamel(), {
			{ BB + FVector(0.0, -7.5, 0.0), FVector(2.6, 0.0, 0.0), FVector(0.0, 0.0, 2.6) },
			{ BB + FVector(0.0, 7.5, 0.0), FVector(2.6, 0.0, 0.0), FVector(0.0, 0.0, 2.6) },
		}, 10, true, true);
		// The stays to the rear axle, a pair each.
		for (const double Side : { -1.0, 1.0 })
		{
			const FVector Drop = Rear + FVector(0.0, 6.5 * Side, 0.0);
			Rod(F, Enamel(), BB + FVector(-2.0, 4.0 * Side, 0.0), Drop, 1.1, 6);
			Rod(F, Enamel(), Seat + FVector(-1.0, 2.5 * Side, -1.0), Drop, 1.0, 6);
		}
		Rod(F, Nickel(), Rear + FVector(0.0, -8.0, 0.0), Rear + FVector(0.0, 8.0, 0.0), 0.8, 6);

		// The shop plate in the frame, cream with a red border, on both sides.
		const FVector Plate = PlateMiddle(Kind);
		Block(F, PlateRed(), Plate, FVector(17.0, 0.45, 5.6));
		Block(F, PlateCream(), Plate, FVector(15.6, 0.6, 4.4));
		Block(F, PlateRed(), Plate, FVector(11.0, 0.7, 0.7));

		// The seat post and the saddle: a leather top on two coil springs.
		const FVector S = SaddleTop();
		Rod(F, Nickel(), Seat, S + FVector(1.0, 0.0, -9.0), 1.25, 8);
		Rod(F, Nickel(), S + FVector(-8.0, 0.0, -8.0), S + FVector(10.0, 0.0, -6.0), 0.7, 5);
		for (const double Side : { -1.0, 1.0 })
		{
			for (int32 k = 0; k < 5; ++k)
			{
				Loft(F, Nickel(), {
					{ S + FVector(-8.0, 5.5 * Side, -8.0 + k * 1.4), FVector(1.8, 0.0, 0.0), FVector(0.0, 1.8, 0.0) },
					{ S + FVector(-8.0, 5.5 * Side, -7.4 + k * 1.4), FVector(1.8, 0.0, 0.0), FVector(0.0, 1.8, 0.0) },
				}, 8, false, false);
			}
		}
		Loft(F, Saddle(), {
			{ S + FVector(-11.0, 0.0, -2.2), FVector(0.0, 10.5, 0.0), FVector(0.0, 0.0, 2.2) },
			{ S + FVector(-6.0, 0.0, -1.4), FVector(0.0, 11.0, 0.0), FVector(0.0, 0.0, 1.4) },
			{ S + FVector(2.0, 0.0, -1.6), FVector(0.0, 7.0, 0.0), FVector(0.0, 0.0, 1.6) },
			{ S + FVector(10.0, 0.0, -1.8), FVector(0.0, 3.2, 0.0), FVector(0.0, 0.0, 1.8) },
			{ S + FVector(13.0, 0.0, -2.0), FVector(0.0, 2.0, 0.0), FVector(0.0, 0.0, 1.6) },
		}, 12, true, true);
		// A leather tool bag behind the saddle.
		Block(F, Saddle(), S + FVector(-14.0, 0.0, -7.0), FVector(3.5, 6.0, 3.5));

		// The rear mudguard, the chain guard, a little rear rack and the kickstand.
		Arc(F, Enamel(), Rear, RearRadius + 3.0, PI * 0.08, PI * 0.95, 0.5, 3.4, 4);
		Rod(F, Enamel(), Rear + FVector(-RearRadius - 2.0, 0.0, 6.0), Rear + FVector(-RearRadius + 4.0, 0.0, RearRadius - 2.0), 0.6, 4);
		Block(F, PlateRed(), Rear + FVector(-RearRadius - 2.6, 0.0, 8.0), FVector(0.4, 2.0, 1.4));
		{
			// A flat guard plate over the chain, from the chain wheel back to the sprocket.
			const FVector A = BB + FVector(0.0, 10.6, 5.0);
			const FVector B = Rear + FVector(0.0, 10.6, 5.0);
			Block(F, Enamel(), (A + B) * 0.5 + FVector(0.0, 0.0, 2.0), FVector((B - A).Size() * 0.5 - 2.0, 0.4, 5.5));
			Arc(F, Enamel(), BB + FVector(0.0, 10.6, 0.0), 11.5, -PI * 0.1, PI * 0.95, 0.8, 0.5, 4);
		}
		Rod(F, Enamel(), BB + FVector(-12.0, -6.0, -2.0), Rear + FVector(-6.0, -6.0, -RearRadius + 12.0), 0.9, 5);
		for (const double Side : { -1.0, 1.0 })
		{
			Rod(F, Enamel(), Rear + FVector(-4.0, 8.0 * Side, 0.0), Rear + FVector(-20.0, 8.0 * Side, RearRadius + 6.0), 0.7, 5);
			Rod(F, Enamel(), Seat + FVector(-6.0, 4.0 * Side, -6.0), Rear + FVector(-14.0, 8.0 * Side, RearRadius + 6.0), 0.6, 5);
		}
		Block(F, Enamel(), Rear + FVector(-10.0, 0.0, RearRadius + 6.5), FVector(14.0, 9.0, 0.6));

		if (!BasketTurns(Kind))
		{
			BuildBasket(F, Kind);
		}
	}

	inline void BuildSteer(PersonKit::FPiece& P, EKind Kind)
	{
		using namespace PersonKit;
		P.Pivot = HeadBottom(Kind);
		const FVector Axle = FrontAxle(Kind);
		const FVector Crown = HeadBottom(Kind) - SteerAxis(Kind) * 6.0;
		Rod(P, Enamel(), HeadBottom(Kind) + SteerAxis(Kind) * 2.0, Crown, 1.5, 8);
		Rod(P, Enamel(), Crown + FVector(0.0, -6.0, 0.0), Crown + FVector(0.0, 6.0, 0.0), 1.6, 6);
		for (const double Side : { -1.0, 1.0 })
		{
			// The blades, bent forward at the foot to the axle.
			const FVector Top = Crown + FVector(0.0, 5.5 * Side, 0.0);
			const FVector Bend = Axle + FVector(-3.5, 5.5 * Side, 9.0);
			Rod(P, Enamel(), Top, Bend, 1.15, 6);
			Rod(P, Enamel(), Bend, Axle + FVector(0.0, 5.5 * Side, 0.0), 1.0, 6);
		}
		Rod(P, Nickel(), Axle + FVector(0.0, -7.0, 0.0), Axle + FVector(0.0, 7.0, 0.0), 0.7, 6);
		// The front mudguard.
		Arc(P, Enamel(), Axle, FrontRadius(Kind) + 2.5, PI * 0.1, PI * 0.9, 0.5, 3.0, 4);

		// The stem and the swept-back bars with white grips and a bell.
		const FVector StemTop = HeadTop(Kind) + SteerAxis(Kind) * 7.0;
		const FVector Clamp = StemTop + FVector(4.0, 0.0, 1.0);
		Rod(P, Nickel(), HeadTop(Kind) - SteerAxis(Kind) * 1.0, StemTop, 1.1, 8);
		Rod(P, Nickel(), StemTop, Clamp, 1.1, 8);
		const FLinearColor GripColor(0.70f, 0.66f, 0.56f);
		for (const double Side : { -1.0, 1.0 })
		{
			const FVector G = Grip(Kind, Side);
			const FVector Out = Clamp + FVector(0.0, 13.0 * Side, 0.0);
			const FVector Turn = FVector(G.X + 9.0, G.Y, G.Z + 0.5);
			Rod(P, Nickel(), Clamp, Out, 1.0, 6);
			Rod(P, Nickel(), Out, Turn, 1.0, 6);
			Rod(P, Nickel(), Turn, G + FVector(4.0, 0.0, 0.0), 1.0, 6);
			Rod(P, GripColor, G + FVector(5.0, 0.0, 0.0), G + FVector(-6.0, 0.0, -0.4), 1.6, 8);
			// The brake lever under the right grip, pulling a rod down to the front brake.
			if (Side > 0.0)
			{
				Rod(P, Nickel(), Turn + FVector(-2.0, -1.0, -1.5), G + FVector(-2.0, -1.0, -4.0), 0.5, 4);
			}
		}
		Rod(P, Nickel(), Clamp + FVector(0.0, 9.0, 0.0), Crown + FVector(4.0, 2.0, 0.0), 0.35, 4);
		const FVector Bell = Clamp + FVector(-3.0, -9.0, 2.6);
		Blob(P, Nickel(), Bell, FVector(2.6, 2.6, 1.8), 10, 4, -0.2);
		Blob(P, Nickel(), Bell + FVector(0.0, 0.0, 1.6), FVector(0.6, 0.6, 0.6), 6, 2);

		// The carbide lamp: on a bracket at the head, in front of the basket's foot, or on the front of a roadster's rack.
		const FVector LampFoot = IsRoadster(Kind) ? CarrierTop(Kind) + FVector(BasketHalf().X + 0.5, 0.0, -1.0) : HeadBottom(Kind) + FVector(1.5, 0.0, 1.0);
		const FVector Lamp = IsRoadster(Kind) ? LampFoot + FVector(4.0, 0.0, -5.0) : HeadBottom(Kind) + FVector(8.0, 0.0, 2.0);
		Rod(P, Enamel(), LampFoot, Lamp + FVector(-3.0, 0.0, 0.0), 0.6, 4);
		Loft(P, Brass(), {
			{ Lamp + FVector(-3.5, 0.0, 0.0), FVector(0.0, 3.4, 0.0), FVector(0.0, 0.0, 3.6) },
			{ Lamp + FVector(2.0, 0.0, 0.0), FVector(0.0, 3.6, 0.0), FVector(0.0, 0.0, 3.8) },
			{ Lamp + FVector(3.0, 0.0, 0.0), FVector(0.0, 4.2, 0.0), FVector(0.0, 0.0, 4.4) },
		}, 12, true, false);
		Loft(P, FLinearColor(0.80f, 0.78f, 0.66f), {
			{ Lamp + FVector(3.0, 0.0, 0.0), FVector(0.0, 3.7, 0.0), FVector(0.0, 0.0, 3.9) },
			{ Lamp + FVector(3.3, 0.0, 0.0), FVector(0.0, 3.7, 0.0), FVector(0.0, 0.0, 3.9) },
		}, 12, false, true);
		Blob(P, Brass(), Lamp + FVector(-1.0, 0.0, 3.8), FVector(1.4, 1.4, 1.4), 6, 3);
		if (BasketTurns(Kind))
		{
			BuildBasket(P, Kind);
		}
	}

inline void BuildCranks(PersonKit::FPiece& P)
	{
		using namespace PersonKit;
		const FVector BB = BottomBracket();
		P.Pivot = BB;
		// The chain wheel, and a ring of the chain round it and back to the sprocket.
		Arc(P, Nickel(), BB + FVector(0.0, 9.0, 0.0), 9.0, 0.0, 2.0 * PI, 0.6, 0.35, 4);
		for (int32 s = 0; s < 5; ++s)
		{
			const double A = 2.0 * PI * s / 5.0;
			Rod(P, Nickel(), BB + FVector(0.0, 9.0, 0.0), BB + FVector(FMath::Cos(A) * 8.6, 9.0, FMath::Sin(A) * 8.6), 0.6, 4);
		}
		for (const double Side : { -1.0, 1.0 })
		{
			// Crank 0 points up on the right, down on the left.
			const FVector Spindle(0.0, 15.0 * Side, CrankLength * Side);
			Rod(P, Nickel(), BB + FVector(0.0, (Side > 0.0 ? 10.5 : 9.0) * Side, 0.0), BB + FVector(0.0, 11.0 * Side, 0.0) + FVector(0.0, 0.0, CrankLength * Side), 0.9, 6);
			// A rubber block pedal.
			Block(P, Tyre(), BB + Spindle + FVector(0.0, 1.5 * Side, 0.0), FVector(5.0, 4.5, 1.2));
		}
	}

	/** Builds the whole bicycle of a kind; Parcels are put in its basket. */
	inline void Build(FBicycle& Out, EKind Kind, int32 Parcels = 0)
	{
		BuildFrame(Out.Parts[Frame], Kind);
		BuildSteer(Out.Parts[Steer], Kind);
		PersonKit::FPiece& Basket = Out.Parts[BasketTurns(Kind) ? Steer : Frame];
		for (int32 i = 0; i < FMath::Min(Parcels, MaxParcels); ++i)
		{
			BuildParcel(Basket, ParcelSpot(Kind, i), ParcelHalf(i));
		}
		// Each wheel is built round its hub, which is then its pivot.
		BuildWheel(Out.Parts[WheelFront], FrontRadius(Kind));
		Out.Parts[WheelFront].Pivot = FrontAxle(Kind);
		BuildWheel(Out.Parts[WheelRear], RearRadius);
		Out.Parts[WheelRear].Pivot = RearAxle();
		BuildCranks(Out.Parts[Cranks]);
	}

	// The rider.

	/** Where the rider's hip joints are (between them), sitting on the saddle. */
	inline FVector RiderHip() { return SaddleTop() + FVector(-1.0, 0.0, 8.0); }
	/** How far the rider leans forward from the hips (degrees, as a pitch: below zero is forward). */
	constexpr double RiderLean = -35.0;
	/** A long coat's skirt flaps follow the thighs by this share on the saddle, so they drape over the knees (PersonKit::SkirtFollow is for walking). */
	constexpr double RiderSkirtFollow = 0.85;

	/** Angles for a kit person on the bicycle (degrees, as pitches about each piece's pivot, relative to its parent). */
	struct FRiderPose
	{
		/** The body, about the hips. */
		double Lean = RiderLean;
		/** Left, right. */
		double Thigh[2] = { 0.0, 0.0 };
		double Shin[2] = { 0.0, 0.0 };
		double Arm[2] = { 0.0, 0.0 };
		double Forearm[2] = { 0.0, 0.0 };
	};

	/**
	 * Two bones from Top reaching for Target in the XZ plane, the joint bent
	 * forward (a knee) or back (an elbow). Gives each bone's pitch from
	 * hanging straight down (above zero swings the far end forward).
	 */
	inline void Reach2(const FVector& Top, const FVector& Target, double First, double Second, bool bJointForward, double& OutFirst, double& OutSecond)
	{
		const double DX = Target.X - Top.X;
		const double DZ = Target.Z - Top.Z;
		const double Dist = FMath::Clamp(FMath::Sqrt(DX * DX + DZ * DZ), FMath::Abs(First - Second) + 0.5, First + Second - 0.1);
		const double Line = FMath::Atan2(DX, -DZ);
		const double CosBend = FMath::Clamp((First * First + Dist * Dist - Second * Second) / (2.0 * First * Dist), -1.0, 1.0);
		const double Bend = FMath::Acos(CosBend);
		const double A1 = bJointForward ? Line + Bend : Line - Bend;
		const FVector Joint(Top.X + FMath::Sin(A1) * First, 0.0, Top.Z - FMath::Cos(A1) * First);
		const double A2 = FMath::Atan2(Target.X - Joint.X, -(Target.Z - Joint.Z));
		OutFirst = FMath::RadiansToDegrees(A1);
		OutSecond = FMath::RadiansToDegrees(A2 - A1);
	}

	/**
	 * The rider's pose with the cranks turned to CrankDegrees, for a kit
	 * person of Height cm. Thighs and upper arms are relative to the leaning
	 * body; shins and forearms to their thighs and upper arms.
	 */
	inline FRiderPose RiderPose(EKind Kind, double CrankDegrees, double Height)
	{
		const double H = Height / 178.0;
		FRiderPose P;
		const FVector Hip = RiderHip();
		const double LeanRad = FMath::DegreesToRadians(-P.Lean);
		// The kit's hip, knee, ankle, shoulder and hand heights.
		const double Thigh = (92.0 - 50.0) * H;
		const double ShinLen = (50.0 - 9.0) * H;
		const double Torso = (138.0 - 92.0) * H;
		const double Upper = PersonKit::ElbowDrop * H;
		const double Lower = (56.5 - PersonKit::ElbowDrop) * H;
		const FVector Shoulder = Hip + FVector(FMath::Sin(LeanRad) * Torso, 0.0, FMath::Cos(LeanRad) * Torso);
		for (int32 s = 0; s < 2; ++s)
		{
			const double Side = s == 0 ? -1.0 : 1.0;
			// The ankle behind and above the pedal, with the ball of the foot on it.
			const FVector Ankle = Pedal(CrankDegrees, Side) + FVector(-9.0, 0.0, 10.0);
			double A = 0.0;
			double B = 0.0;
			Reach2(Hip, Ankle, Thigh, ShinLen, true, A, B);
			P.Thigh[s] = A - P.Lean;
			P.Shin[s] = B;
			Reach2(Shoulder, Grip(Kind, Side), Upper, Lower, false, A, B);
			P.Arm[s] = A - P.Lean;
			P.Forearm[s] = B;
		}
		return P;
	}
}
