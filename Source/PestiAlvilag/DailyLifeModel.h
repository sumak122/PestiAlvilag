// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PersonKit.h"

/**
 * The shapes and clothes of the city's daily life (ADailyLife): the
 * lamplighter and his long pole, the organ grinder's barrel organ with its
 * fortune-telling parrot, the knife grinder's barrow and stone, the chestnut
 * seller's stove, the courtyard's carpet beater, washing and football, the
 * wedding party (bride's veil, bouquet, the fiddler's fiddle), the mourners'
 * wreath, the fire brigade's motor engine and the firemen's brass helmets.
 *
 * Hand-held things are built round the grip (where the fist closes), their
 * long way along +Z. ADailyLife hangs them on the forearm at the hand
 * (HandOnForearm) turned by HoldUpright, so they stand the way they should
 * however the arm is raised. Things on the ground are built from the middle
 * of their foot, facing +X.
 */
namespace DailyLifeModel
{
	using PersonKit::FPiece;
	using PersonKit::FOutfit;

	// Colours.
	inline FLinearColor Wood() { return FLinearColor(0.30f, 0.20f, 0.11f); }
	inline FLinearColor DarkWood() { return FLinearColor(0.14f, 0.08f, 0.04f); }
	inline FLinearColor BrassColor() { return FLinearColor(0.62f, 0.44f, 0.14f); }
	inline FLinearColor IronColor() { return FLinearColor(0.05f, 0.05f, 0.055f); }
	inline FLinearColor FlameColor() { return FLinearColor(1.0f, 0.62f, 0.18f); }
	inline FLinearColor EmberColor() { return FLinearColor(0.95f, 0.30f, 0.05f); }
	inline FLinearColor FireRed() { return FLinearColor(0.42f, 0.025f, 0.02f); }
	inline FLinearColor Linen() { return FLinearColor(0.86f, 0.85f, 0.80f); }
	inline FLinearColor Mourning() { return FLinearColor(0.025f, 0.025f, 0.03f); }

	/** Where the fist closes, in the forearm's own frame (from PersonKit's arm). Side: -1 left, 1 right. */
	inline FVector HandOnForearm(double Side, double Height)
	{
		return FVector(1.4, 2.3 * Side, -27.5 * Height / 178.0);
	}

	/**
	 * The turn that keeps a held thing (built along +Z) upright, leaning
	 * forward by Lean degrees, in a hand whose arm and elbow together are
	 * pitched Raised degrees forward.
	 */
	inline FRotator HoldUpright(float Raised, float Lean = 0.f)
	{
		return FRotator(-Raised - Lean, 0.f, 0.f);
	}

	/** The head's middle in the body's frame (PersonKit puts it there). */
	inline FVector HeadMiddle(const FOutfit& O)
	{
		return FVector(1.5, 0.0, 165.5 * O.Height / 178.0);
	}

	/**
	 * A held pose: each arm's turn at the shoulder (left, right), each
	 * elbow's bend, each thigh's swing and knee's bend (as PersonModel
	 * PoseLegs), and the whole body tipped forward by Pitch.
	 */
	struct FLimbs
	{
		FRotator Arms[2] = { FRotator::ZeroRotator, FRotator::ZeroRotator };
		float Elbows[2] = { 0.f, 0.f };
		float Legs[2] = { 0.f, 0.f };
		float Knees[2] = { 0.f, 0.f };
		float Pitch = 0.f;
	};

	/** Walking with the pole upright in his right hand like a staff (the pole's hold: HoldUpright(PoleCarried)). */
	constexpr float PoleCarried = 85.f;
	inline void PoleCarry(FLimbs& L)
	{
		L.Arms[1] = FRotator(30.f, 0.f, -8.f);
		L.Elbows[1] = 55.f;
	}

	/** Reaching up the pole to the lantern, by Reach (0 down .. 1 at the top); the right arm's total raise for HoldUpright. */
	inline float PoleReach(FLimbs& L, float Reach)
	{
		const float R = FMath::SmoothStep(0.f, 1.f, Reach);
		L.Arms[1] = FRotator(FMath::Lerp(30.f, 150.f, R), 0.f, FMath::Lerp(-8.f, -4.f, R));
		L.Elbows[1] = FMath::Lerp(55.f, 15.f, R);
		L.Arms[0] = FRotator(FMath::Lerp(0.f, 128.f, R), 0.f, FMath::Lerp(0.f, -14.f, R));
		L.Elbows[0] = FMath::Lerp(0.f, 28.f, R);
		return L.Arms[1].Pitch + L.Elbows[1];
	}

	/** Turning the organ's crank (Phase in turns): the right hand goes round with it, the left rests on the box. */
	inline void Cranking(FLimbs& L, float Phase)
	{
		const float A = 2.f * PI * Phase;
		L.Arms[1] = FRotator(42.f + 16.f * FMath::Sin(A), 0.f, -14.f);
		L.Elbows[1] = 58.f - 22.f * FMath::Cos(A);
		L.Arms[0] = FRotator(38.f, 0.f, 8.f);
		L.Elbows[0] = 40.f;
	}

	/** At the grinding stone: both hands forward holding the knife to it, the left foot working the treadle (Phase in strokes). */
	inline float Grinding(FLimbs& L, float Phase)
	{
		const float A = 2.f * PI * Phase;
		L.Arms[0] = FRotator(50.f, 0.f, 14.f);
		L.Arms[1] = FRotator(52.f + 3.f * FMath::Sin(A * 3.f), 0.f, -16.f);
		L.Elbows[0] = 48.f;
		L.Elbows[1] = 46.f;
		L.Legs[0] = 18.f + 12.f * FMath::Sin(A);
		L.Knees[0] = -28.f - 14.f * FMath::Sin(A);
		L.Pitch = 10.f;
		return L.Arms[1].Pitch + L.Elbows[1];
	}

	/** Turning the chestnuts on the pan with the scoop (Phase in turns). */
	inline float Stirring(FLimbs& L, float Phase)
	{
		const float A = 2.f * PI * Phase;
		L.Arms[1] = FRotator(48.f + 8.f * FMath::Sin(A), 0.f, -10.f + 8.f * FMath::Cos(A));
		L.Elbows[1] = 42.f;
		L.Arms[0] = FRotator(14.f, 0.f, 18.f);
		L.Elbows[0] = 70.f;
		L.Pitch = 6.f;
		return L.Arms[1].Pitch + L.Elbows[1];
	}

	/** Beating the carpet (Phase in beats): the beater swung up and brought down hard; the right arm's raise. */
	inline float Beating(FLimbs& L, float Phase)
	{
		const float T = Phase - FMath::FloorToFloat(Phase);
		// Slowly up over most of the beat, then down fast.
		const float Up = T < 0.72f ? FMath::SmoothStep(0.f, 1.f, T / 0.72f) : 1.f - FMath::Square((T - 0.72f) / 0.28f);
		L.Arms[1] = FRotator(FMath::Lerp(70.f, 165.f, Up), 0.f, -10.f);
		L.Elbows[1] = FMath::Lerp(8.f, 40.f, Up);
		L.Arms[0] = FRotator(10.f, 0.f, 24.f);
		L.Elbows[0] = 95.f;
		L.Legs[0] = 10.f;
		L.Legs[1] = -6.f;
		return L.Arms[1].Pitch + L.Elbows[1];
	}

	/** Reaching up to the washing line with a sheet in both hands. */
	inline void Pegging(FLimbs& L, float Phase)
	{
		const float Wobble = 4.f * FMath::Sin(2.f * PI * Phase);
		L.Arms[0] = FRotator(162.f + Wobble, 0.f, 12.f);
		L.Arms[1] = FRotator(162.f - Wobble, 0.f, -12.f);
		L.Elbows[0] = 12.f;
		L.Elbows[1] = 12.f;
	}

	/** A kick (Swing 0..1 through it): the right leg back, then through, arms out for balance. */
	inline void Kicking(FLimbs& L, float Swing)
	{
		const float S = FMath::Sin(PI * FMath::Clamp(Swing, 0.f, 1.f));
		const float Through = FMath::Lerp(-35.f, 60.f, FMath::Clamp(Swing, 0.f, 1.f));
		L.Legs[1] = Through;
		L.Knees[1] = -60.f * (1.f - Swing) * S - 5.f;
		L.Legs[0] = -8.f;
		L.Arms[0] = FRotator(25.f * S, 0.f, 35.f * S);
		L.Arms[1] = FRotator(-20.f * S, 0.f, -30.f * S);
		L.Pitch = -6.f * S;
	}

	/** Waiting for the ball, a little crouched, hands on the knees now and then. */
	inline void Ready(FLimbs& L, float Crouch)
	{
		L.Legs[0] = 18.f * Crouch;
		L.Legs[1] = 14.f * Crouch;
		L.Knees[0] = -26.f * Crouch;
		L.Knees[1] = -22.f * Crouch;
		L.Arms[0] = FRotator(30.f * Crouch, 0.f, 8.f);
		L.Arms[1] = FRotator(30.f * Crouch, 0.f, -8.f);
		L.Elbows[0] = 20.f * Crouch;
		L.Elbows[1] = 20.f * Crouch;
		L.Pitch = 16.f * Crouch;
	}

	/** Playing the fiddle (Phase in bow strokes). */
	inline void Fiddling(FLimbs& L, float Phase)
	{
		const float A = FMath::Sin(2.f * PI * Phase);
		L.Arms[0] = FRotator(80.f, 0.f, 36.f);
		L.Elbows[0] = 80.f;
		L.Arms[1] = FRotator(58.f + 10.f * A, 0.f, -36.f - 14.f * A);
		L.Elbows[1] = 78.f + 24.f * A;
	}

	/** Holding something before the chest in both hands (a bouquet, a wreath); the right arm's raise. */
	inline float HoldingUp(FLimbs& L)
	{
		L.Arms[0] = FRotator(32.f, 0.f, 12.f);
		L.Arms[1] = FRotator(34.f, 0.f, -12.f);
		L.Elbows[0] = 66.f;
		L.Elbows[1] = 64.f;
		return L.Arms[1].Pitch + L.Elbows[1];
	}

	/** Sitting (on the engine's bench), hands on the knees, or one up on the rail by Grip (0..1). */
	inline void Seated(FLimbs& L, float Grip)
	{
		L.Legs[0] = 88.f;
		L.Legs[1] = 84.f;
		L.Knees[0] = -88.f;
		L.Knees[1] = -84.f;
		L.Arms[0] = FRotator(36.f, 0.f, 6.f);
		L.Elbows[0] = 30.f;
		L.Arms[1] = FRotator(FMath::Lerp(36.f, 110.f, Grip), 0.f, FMath::Lerp(-6.f, -30.f, Grip));
		L.Elbows[1] = FMath::Lerp(30.f, 50.f, Grip);
	}

	/** Pushing the shutter up with the hook pole, by Lift (0 bottom .. 1 top); the right arm's raise. */
	inline float PushingUp(FLimbs& L, float Lift)
	{
		const float R = FMath::Clamp(Lift, 0.f, 1.f);
		L.Arms[1] = FRotator(FMath::Lerp(60.f, 160.f, R), 0.f, -6.f);
		L.Arms[0] = FRotator(FMath::Lerp(50.f, 140.f, R), 0.f, 14.f);
		L.Elbows[1] = FMath::Lerp(60.f, 10.f, R);
		L.Elbows[0] = FMath::Lerp(60.f, 20.f, R);
		return L.Arms[1].Pitch + L.Elbows[1];
	}

	// Small builders.
	inline void Box(FPiece& P, const FLinearColor& Color, const FVector& From, const FVector& To)
	{
		PersonKit::Block(P, Color, (From + To) * 0.5, (To - From).GetAbs() * 0.5);
	}

	inline void Tube(FPiece& P, const FLinearColor& Color, const FVector& A, const FVector& B, double Radius, int32 Sides = 12)
	{
		PersonKit::Rod(P, Color, A, B, Radius, Sides);
	}

	/** A ring of rods round Center in the plane of U and V (each scaled to the radius). */
	inline void Ring(FPiece& P, const FLinearColor& Color, const FVector& Center, const FVector& U, const FVector& V, double Thick, int32 Steps = 16)
	{
		for (int32 i = 0; i < Steps; ++i)
		{
			const double A0 = 2.0 * PI * i / Steps;
			const double A1 = 2.0 * PI * (i + 1) / Steps;
			PersonKit::Rod(P, Color, Center + U * FMath::Cos(A0) + V * FMath::Sin(A0), Center + U * FMath::Cos(A1) + V * FMath::Sin(A1), Thick, 5);
		}
	}

	/** A spoked wheel turning about Y, its hub at Center: tyre, rim, spokes and hub. */
	inline void SpokedWheel(FPiece& P, const FLinearColor& Paint, const FVector& Center, double Radius, double Width, int32 Spokes)
	{
		const double Half = Width * 0.5;
		// The tyre: a short fat tube round the rim (two rings joined), the inside open.
		TArray<PersonKit::FRing> Tyre;
		Tyre.Add({ Center + FVector(0.0, -Half, 0.0), FVector(Radius, 0.0, 0.0), FVector(0.0, 0.0, Radius) });
		Tyre.Add({ Center + FVector(0.0, Half, 0.0), FVector(Radius, 0.0, 0.0), FVector(0.0, 0.0, Radius) });
		PersonKit::Loft(P, FLinearColor(0.025f, 0.025f, 0.025f), Tyre, 20, false, false);
		Ring(P, Paint, Center + FVector(0.0, -Half + 1.0, 0.0), FVector(Radius - 4.0, 0.0, 0.0), FVector(0.0, 0.0, Radius - 4.0), 2.2, 20);
		Ring(P, Paint, Center + FVector(0.0, Half - 1.0, 0.0), FVector(Radius - 4.0, 0.0, 0.0), FVector(0.0, 0.0, Radius - 4.0), 2.2, 20);
		for (int32 s = 0; s < Spokes; ++s)
		{
			const double A = 2.0 * PI * s / Spokes;
			const FVector Out(FMath::Cos(A) * (Radius - 4.0), 0.0, FMath::Sin(A) * (Radius - 4.0));
			PersonKit::Rod(P, Paint, Center, Center + Out, 1.8, 5);
		}
		Tube(P, Paint, Center + FVector(0.0, -Half - 2.0, 0.0), Center + FVector(0.0, Half + 2.0, 0.0), Radius * 0.18, 10);
	}

	// ---------------------------------------------------------------------
	// The lamplighter.

	/** How far up from his hand the lamplighter's pole reaches, and how far down. */
	constexpr double PoleUp = 360.0;
	constexpr double PoleDown = 35.0;

	/** A long pole with a brass hook at the top; with bWick a little flame in a brass cup beside it. */
	inline void Pole(FPiece& P, double Up, double Down, bool bWick)
	{
		Tube(P, Wood(), FVector(0.0, 0.0, -Down), FVector(0.0, 0.0, Up), 1.8, 6);
		Tube(P, BrassColor(), FVector(0.0, 0.0, Up - 4.0), FVector(0.0, 0.0, Up + 10.0), 2.4, 8);
		// The hook that pulls the lantern's chain and turns the gas tap.
		Tube(P, BrassColor(), FVector(0.0, 0.0, Up + 10.0), FVector(5.0, 0.0, Up + 16.0), 0.9, 5);
		Tube(P, BrassColor(), FVector(5.0, 0.0, Up + 16.0), FVector(7.0, 0.0, Up + 11.0), 0.9, 5);
		if (bWick)
		{
			Tube(P, BrassColor(), FVector(-4.0, 0.0, Up - 2.0), FVector(-4.0, 0.0, Up + 5.0), 1.8, 8);
			PersonKit::Blob(P, FlameColor(), FVector(-4.0, 0.0, Up + 8.0), FVector(1.4, 1.4, 3.2), 8, 4);
		}
	}

	inline FOutfit LamplighterLook(FRandomStream& Random)
	{
		FOutfit O;
		O.Height = Random.FRandRange(168.f, 180.f);
		O.Top = PersonKit::ETop::Jacket;
		O.Coat = FLinearColor(0.10f, 0.12f, 0.17f);
		O.Trousers = FLinearColor(0.07f, 0.08f, 0.10f);
		O.Hat = PersonKit::EHat::FlatCap;
		O.HatColor = FLinearColor(0.07f, 0.08f, 0.12f);
		O.HatBand = BrassColor();
		O.bTie = false;
		O.bScarf = true;
		O.Scarf = FLinearColor(0.30f, 0.26f, 0.20f);
		O.bMoustache = true;
		O.bBoots = true;
		O.bAged = Random.FRand() < 0.5f;
		return O;
	}

	// ---------------------------------------------------------------------
	// The organ grinder.

	/** Where the crank's axle is on the organ, from its foot, and how long the crank is. */
	inline FVector OrganCrankAt() { return FVector(-4.0, 38.0, 104.0); }
	constexpr double OrganCrankArm = 13.0;

	/** The barrel organ on its little two-wheeled cart, the parrot on its perch on top, from the foot under the box, facing +X. */
	inline void Organ(FPiece& P)
	{
		const FLinearColor Lacquer(0.28f, 0.05f, 0.04f);
		const FLinearColor Gilt(0.70f, 0.52f, 0.20f);
		const FLinearColor Picture(0.70f, 0.62f, 0.45f);
		const FLinearColor Cloth(0.16f, 0.22f, 0.12f);
		// The cart: two wheels, an axle, two shafts back to the handles, a leg at the front.
		SpokedWheel(P, DarkWood(), FVector(0.0, -40.0, 30.0), 30.0, 5.0, 8);
		SpokedWheel(P, DarkWood(), FVector(0.0, 40.0, 30.0), 30.0, 5.0, 8);
		Tube(P, IronColor(), FVector(0.0, -40.0, 30.0), FVector(0.0, 40.0, 30.0), 1.6, 6);
		for (const double Y : { -28.0, 28.0 })
		{
			Tube(P, Wood(), FVector(28.0, Y, 62.0), FVector(-70.0, Y, 78.0), 2.2, 6);
			Tube(P, Wood(), FVector(0.0, Y, 30.0), FVector(0.0, Y, 64.0), 2.2, 6);
		}
		Tube(P, Wood(), FVector(26.0, 0.0, 0.0), FVector(26.0, 0.0, 62.0), 2.2, 6);
		// The organ: a lacquered box with a gilt frame, a painted front and brass pipes behind fretwork.
		Box(P, Lacquer, FVector(-22.0, -34.0, 64.0), FVector(22.0, 34.0, 124.0));
		Box(P, Gilt, FVector(21.0, -32.0, 66.0), FVector(23.0, 32.0, 69.0));
		Box(P, Gilt, FVector(21.0, -32.0, 120.0), FVector(23.0, 32.0, 123.0));
		Box(P, Picture, FVector(22.0, -28.0, 72.0), FVector(23.5, 28.0, 90.0));
		for (int32 i = 0; i < 11; ++i)
		{
			const double Y = -24.0 + i * 4.8;
			const double Top = 116.0 - FMath::Abs(i - 5) * 2.4;
			Tube(P, BrassColor(), FVector(23.0, Y, 94.0), FVector(23.0, Y, Top), 1.6, 6);
		}
		Box(P, Gilt, FVector(-23.0, -35.0, 124.0), FVector(23.0, 35.0, 127.0));
		// A cloth over the lid, and the fortune cards in a little tray at the front.
		Box(P, Cloth, FVector(-20.0, -30.0, 127.0), FVector(20.0, 30.0, 129.0));
		Box(P, Wood(), FVector(8.0, -26.0, 129.0), FVector(20.0, -8.0, 134.0));
		for (int32 c = 0; c < 6; ++c)
		{
			Box(P, Linen(), FVector(10.0 + c * 1.6, -24.0, 130.0), FVector(10.8 + c * 1.6, -10.0, 138.0));
		}
		// The crank's axle boss on the right.
		Tube(P, BrassColor(), FVector(-4.0, 34.0, 104.0), FVector(-4.0, 38.0, 104.0), 3.0, 8);
		// The parrot: a green bird with a red head on a brass perch.
		Tube(P, BrassColor(), FVector(-6.0, 12.0, 129.0), FVector(-6.0, 12.0, 146.0), 0.9, 5);
		Tube(P, BrassColor(), FVector(-6.0, 4.0, 146.0), FVector(-6.0, 20.0, 146.0), 0.9, 5);
		const FLinearColor Green(0.08f, 0.38f, 0.10f);
		PersonKit::Blob(P, Green, FVector(-6.0, 12.0, 154.0), FVector(5.0, 4.5, 8.0), 8, 5);
		PersonKit::Blob(P, FLinearColor(0.55f, 0.07f, 0.04f), FVector(-4.5, 12.0, 164.0), FVector(3.8, 3.4, 3.8), 8, 4);
		Tube(P, FLinearColor(0.85f, 0.80f, 0.60f), FVector(-1.5, 12.0, 164.0), FVector(1.0, 12.0, 162.0), 1.0, 5);
		Tube(P, Green, FVector(-8.0, 12.0, 148.0), FVector(-15.0, 12.0, 136.0), 1.8, 5);
	}

	/** The crank, about its axle (pitch turns it): the arm along +Z and the handle out to the side. */
	inline void OrganCrank(FPiece& P)
	{
		Tube(P, IronColor(), FVector(0.0, 1.0, 0.0), FVector(0.0, 1.0, OrganCrankArm), 1.0, 6);
		Tube(P, DarkWood(), FVector(0.0, 1.0, OrganCrankArm), FVector(0.0, 11.0, OrganCrankArm), 1.6, 6);
	}

	inline FOutfit OrganGrinderLook(FRandomStream& Random)
	{
		FOutfit O;
		O.Height = Random.FRandRange(162.f, 172.f);
		O.Build = 1.08f;
		O.Top = PersonKit::ETop::Overcoat;
		O.Coat = FLinearColor(0.22f, 0.18f, 0.13f);
		O.Trousers = FLinearColor(0.12f, 0.10f, 0.08f);
		O.Hat = PersonKit::EHat::Bowler;
		O.HatColor = FLinearColor(0.10f, 0.09f, 0.08f);
		O.bTie = false;
		O.bScarf = true;
		O.Scarf = FLinearColor(0.45f, 0.10f, 0.08f);
		O.bMoustache = true;
		O.MoustacheKind = PersonKit::EMoustache::Walrus;
		O.Beard = Random.FRand() < 0.5f ? PersonKit::EBeard::Stubble : PersonKit::EBeard::None;
		O.bAged = true;
		O.Hair = FLinearColor(0.45f, 0.44f, 0.42f);
		return O;
	}

	// ---------------------------------------------------------------------
	// The knife grinder.

	/** The grinding stone's middle on the barrow, its radius, and the treadle's front end. */
	inline FVector StoneAt() { return FVector(8.0, 0.0, 96.0); }
	constexpr double StoneRadius = 22.0;

	/** The knife grinder's barrow: a big wheel at the front, two handles at the back, the frame, the treadle, the water can and the hand bell. */
	inline void GrinderBarrow(FPiece& P)
	{
		const FLinearColor Frame(0.20f, 0.24f, 0.18f);
		SpokedWheel(P, Frame, FVector(62.0, 0.0, 34.0), 34.0, 6.0, 10);
		for (const double Y : { -24.0, 24.0 })
		{
			// The handles run from the wheel's axle back to where he lifts it.
			Tube(P, Wood(), FVector(62.0, Y * 0.5, 34.0), FVector(-80.0, Y, 78.0), 2.4, 6);
			// Legs at the back, and the uprights holding the stone.
			Tube(P, Wood(), FVector(-40.0, Y, 66.0), FVector(-44.0, Y * 1.2, 0.0), 2.2, 6);
			Tube(P, Wood(), FVector(8.0, Y * 0.5, 48.0), FVector(8.0, Y * 0.5, 96.0), 2.2, 6);
		}
		Tube(P, IronColor(), FVector(8.0, -14.0, 96.0), FVector(8.0, 14.0, 96.0), 1.4, 6);
		// The flywheel and the treadle that turns it.
		Ring(P, IronColor(), FVector(8.0, -14.0, 70.0), FVector(18.0, 0.0, 0.0), FVector(0.0, 0.0, 18.0), 1.6, 14);
		Tube(P, IronColor(), FVector(8.0, -14.0, 70.0), FVector(8.0, -14.0, 96.0), 0.9, 5);
		Box(P, Wood(), FVector(-36.0, -10.0, 8.0), FVector(18.0, 10.0, 11.0));
		Tube(P, IronColor(), FVector(10.0, -14.0, 11.0), FVector(8.0, -14.0, 60.0), 0.8, 5);
		// The tin water can dripping onto the stone.
		Tube(P, FLinearColor(0.50f, 0.51f, 0.52f), FVector(14.0, 0.0, 128.0), FVector(14.0, 0.0, 142.0), 7.0, 10);
		Tube(P, FLinearColor(0.50f, 0.51f, 0.52f), FVector(14.0, 0.0, 128.0), FVector(10.0, 0.0, 120.0), 0.6, 4);
		Tube(P, Wood(), FVector(14.0, 0.0, 142.0), FVector(14.0, 0.0, 160.0), 1.0, 5);
		// The bell he rings in each courtyard, hung on a hook.
		Tube(P, IronColor(), FVector(-60.0, 24.0, 76.0), FVector(-60.0, 24.0, 96.0), 0.7, 5);
		PersonKit::Loft(P, BrassColor(), { PersonKit::Flat(-60.0, 84.0, 5.0, 5.0, 24.0), PersonKit::Flat(-60.0, 92.0, 3.0, 3.0, 24.0), PersonKit::Flat(-60.0, 96.0, 1.5, 1.5, 24.0) }, 10, true, true);
	}

	/** The grinding stone, about its axle (pitch spins it). */
	inline void GrinderStone(FPiece& P)
	{
		TArray<PersonKit::FRing> Disc;
		Disc.Add({ FVector(0.0, -4.0, 0.0), FVector(StoneRadius, 0.0, 0.0), FVector(0.0, 0.0, StoneRadius) });
		Disc.Add({ FVector(0.0, 4.0, 0.0), FVector(StoneRadius, 0.0, 0.0), FVector(0.0, 0.0, StoneRadius) });
		PersonKit::Loft(P, FLinearColor(0.42f, 0.40f, 0.36f), Disc, 18, true, true);
		// A worn notch so it can be seen to turn.
		Box(P, FLinearColor(0.28f, 0.27f, 0.25f), FVector(-3.0, -4.3, StoneRadius - 6.0), FVector(3.0, 4.3, StoneRadius - 1.0));
	}

	/** A kitchen knife, held by its handle, the blade up along +Z. */
	inline void Knife(FPiece& P)
	{
		Box(P, DarkWood(), FVector(-1.4, -1.0, -6.0), FVector(1.4, 1.0, 5.0));
		Box(P, FLinearColor(0.70f, 0.71f, 0.72f), FVector(-0.4, -1.8, 5.0), FVector(0.4, 1.8, 24.0));
	}

	/** A few sparks flying off the stone where the knife meets it (bright bits, shown while he grinds). */
	inline void Sparks(FPiece& P, int32 Seed)
	{
		FRandomStream Dice(Seed);
		for (int32 i = 0; i < 9; ++i)
		{
			const FVector From(Dice.FRandRange(-2.0, 2.0), Dice.FRandRange(-3.0, 3.0), 0.0);
			const FVector To = From + FVector(Dice.FRandRange(10.0, 28.0), Dice.FRandRange(-10.0, 10.0), Dice.FRandRange(-6.0, 10.0));
			Tube(P, FLinearColor(1.0f, 0.75f, 0.30f), From + (To - From) * 0.4, To, 0.35, 3);
		}
	}

	inline FOutfit KnifeGrinderLook(FRandomStream& Random)
	{
		FOutfit O;
		O.Height = Random.FRandRange(166.f, 176.f);
		O.Top = PersonKit::ETop::Waistcoat;
		O.Waistcoat = FLinearColor(0.16f, 0.14f, 0.11f);
		O.Coat = O.Waistcoat;
		O.Shirt = FLinearColor(0.70f, 0.68f, 0.62f);
		O.Trousers = FLinearColor(0.20f, 0.18f, 0.14f);
		O.Hat = PersonKit::EHat::FlatCap;
		O.HatColor = FLinearColor(0.20f, 0.17f, 0.12f);
		O.bTie = false;
		O.bApron = true;
		O.Apron = FLinearColor(0.20f, 0.12f, 0.07f);
		O.bMoustache = true;
		O.bBoots = true;
		O.Skin = FLinearColor(0.62f, 0.42f, 0.30f);
		return O;
	}

	// ---------------------------------------------------------------------
	// The chestnut seller.

	/** The chestnut seller's stove: an iron drum on legs with the glowing grate, the pan of chestnuts on top, its pipe and the paper bags. */
	inline void ChestnutStove(FPiece& P)
	{
		const FLinearColor Drum(0.07f, 0.065f, 0.06f);
		for (int32 l = 0; l < 3; ++l)
		{
			const double A = 2.0 * PI * l / 3.0 + 0.4;
			Tube(P, IronColor(), FVector(18.0 * FMath::Cos(A), 18.0 * FMath::Sin(A), 34.0), FVector(26.0 * FMath::Cos(A), 26.0 * FMath::Sin(A), 0.0), 1.6, 5);
		}
		PersonKit::Loft(P, Drum, { PersonKit::Flat(0.0, 30.0, 26.0, 26.0), PersonKit::Flat(0.0, 86.0, 26.0, 26.0) }, 16, true, true);
		Ring(P, Drum, FVector(0.0, 0.0, 50.0), FVector(26.6, 0.0, 0.0), FVector(0.0, 26.6, 0.0), 1.2, 16);
		Ring(P, Drum, FVector(0.0, 0.0, 70.0), FVector(26.6, 0.0, 0.0), FVector(0.0, 26.6, 0.0), 1.2, 16);
		// The grate's door open at the front, the coals glowing inside.
		Box(P, EmberColor(), FVector(18.0, -10.0, 42.0), FVector(26.2, 10.0, 56.0));
		Box(P, FlameColor(), FVector(20.0, -6.0, 44.0), FVector(26.4, 6.0, 50.0));
		// The pan: a wide shallow tray with holes, piled with chestnuts.
		PersonKit::Loft(P, IronColor(), { PersonKit::Flat(0.0, 86.0, 30.0, 30.0), PersonKit::Flat(0.0, 93.0, 34.0, 34.0) }, 18, true, false);
		FRandomStream Dice(77);
		for (int32 i = 0; i < 34; ++i)
		{
			const double R = 27.0 * FMath::Sqrt(Dice.FRand());
			const double A = Dice.FRand() * 2.0 * PI;
			const bool bSplit = Dice.FRand() < 0.35f;
			PersonKit::Blob(P, bSplit ? FLinearColor(0.55f, 0.40f, 0.22f) : FLinearColor(0.20f, 0.08f, 0.04f),
				FVector(R * FMath::Cos(A), R * FMath::Sin(A), 91.0 + Dice.FRandRange(0.0, 2.0)), FVector(2.4, 2.0, 2.0), 6, 3);
		}
		// The pipe up at the back, with a little hood.
		Tube(P, Drum, FVector(-20.0, 0.0, 80.0), FVector(-34.0, 0.0, 84.0), 4.5, 8);
		Tube(P, Drum, FVector(-34.0, 0.0, 80.0), FVector(-34.0, 0.0, 168.0), 4.5, 8);
		PersonKit::Loft(P, Drum, { PersonKit::Flat(-34.0, 168.0, 9.0, 9.0), PersonKit::Flat(-34.0, 176.0, 2.0, 2.0) }, 8, true, false);
		// A small shelf on the side with the paper cones (stoppers of newspaper).
		Box(P, Wood(), FVector(-14.0, 26.0, 70.0), FVector(14.0, 46.0, 73.0));
		Tube(P, IronColor(), FVector(0.0, 26.0, 72.0), FVector(0.0, 44.0, 40.0), 0.8, 4);
		for (int32 c = 0; c < 5; ++c)
		{
			const FVector At(-9.0 + c * 4.5, 36.0, 73.0);
			PersonKit::Loft(P, FLinearColor(0.78f, 0.74f, 0.62f), { PersonKit::Flat(At.X, At.Z, 0.4, 0.4, At.Y), PersonKit::Flat(At.X, At.Z + 13.0, 3.0, 3.0, At.Y) }, 6, false, true);
		}
	}

	/** His little iron scoop for turning the chestnuts, held by the handle, the scoop along +Z. */
	inline void Scoop(FPiece& P)
	{
		Tube(P, DarkWood(), FVector(0.0, 0.0, -6.0), FVector(0.0, 0.0, 8.0), 1.4, 6);
		Tube(P, IronColor(), FVector(0.0, 0.0, 8.0), FVector(0.0, 0.0, 30.0), 0.6, 4);
		Box(P, IronColor(), FVector(-0.4, -4.0, 30.0), FVector(0.4, 4.0, 39.0));
	}

	inline FOutfit ChestnutLook(FRandomStream& Random)
	{
		FOutfit O;
		O.Height = Random.FRandRange(164.f, 174.f);
		O.Build = 1.15f;
		O.Top = PersonKit::ETop::Overcoat;
		O.Coat = FLinearColor(0.16f, 0.15f, 0.13f);
		O.Hat = PersonKit::EHat::FlatCap;
		O.HatColor = FLinearColor(0.10f, 0.09f, 0.08f);
		O.bTie = false;
		O.bScarf = true;
		O.Scarf = FLinearColor(0.22f, 0.20f, 0.28f);
		O.bApron = true;
		O.Apron = FLinearColor(0.32f, 0.30f, 0.26f);
		O.bMoustache = true;
		O.Beard = PersonKit::EBeard::Stubble;
		O.Skin = FLinearColor(0.70f, 0.44f, 0.34f);
		return O;
	}

	// ---------------------------------------------------------------------
	// Courtyard life.

	/** A rattan carpet beater: the handle in the fist, the looped paddle up along +Z. */
	inline void CarpetBeater(FPiece& P)
	{
		const FLinearColor Cane(0.70f, 0.58f, 0.36f);
		Tube(P, Cane, FVector(0.0, 0.0, -9.0), FVector(0.0, 0.0, 30.0), 1.3, 6);
		Ring(P, Cane, FVector(0.0, 0.0, 46.0), FVector(0.0, 13.0, 0.0), FVector(0.0, 0.0, 16.0), 0.8, 16);
		Ring(P, Cane, FVector(0.0, -9.0, 38.0), FVector(0.0, 8.0, 0.0), FVector(0.0, 0.0, 9.0), 0.8, 12);
		Ring(P, Cane, FVector(0.0, 9.0, 38.0), FVector(0.0, 8.0, 0.0), FVector(0.0, 0.0, 9.0), 0.8, 12);
	}

	/** A puff of dust beaten out of a carpet (scaled up and faded by ADailyLife). */
	inline void DustPuff(FPiece& P)
	{
		const FLinearColor Dust(0.48f, 0.44f, 0.38f);
		PersonKit::Blob(P, Dust, FVector(0.0, 0.0, 0.0), FVector(12.0, 18.0, 10.0), 8, 5);
		PersonKit::Blob(P, Dust, FVector(4.0, 12.0, 6.0), FVector(8.0, 10.0, 8.0), 8, 4);
		PersonKit::Blob(P, Dust, FVector(2.0, -14.0, -3.0), FVector(8.0, 9.0, 7.0), 8, 4);
	}

	/** A leather football, laced. */
	constexpr double BallRadius = 11.0;
	inline void Football(FPiece& P)
	{
		const FLinearColor Leather(0.30f, 0.15f, 0.06f);
		PersonKit::Blob(P, Leather, FVector::ZeroVector, FVector(BallRadius, BallRadius, BallRadius), 12, 8);
		Ring(P, FLinearColor(0.18f, 0.08f, 0.03f), FVector::ZeroVector, FVector(BallRadius, 0.0, 0.0), FVector(0.0, 0.0, BallRadius), 0.6, 18);
		Box(P, FLinearColor(0.75f, 0.70f, 0.55f), FVector(-0.8, -3.0, BallRadius - 1.0), FVector(0.8, 3.0, BallRadius + 0.6));
	}

	/** A wet sheet being pegged on the line, held up by its top corners: Width across (Y), hanging down from its top edge at 0. */
	inline void Sheet(FPiece& P, double Width, double Drop, const FLinearColor& Color)
	{
		Box(P, Color, FVector(-0.6, -Width * 0.5, -Drop), FVector(0.6, Width * 0.5, 0.0));
	}

	/** The wicker washing basket on the ground, from its foot. */
	inline void WashBasket(FPiece& P)
	{
		const FLinearColor Wicker(0.52f, 0.40f, 0.22f);
		PersonKit::Loft(P, Wicker, { PersonKit::Flat(0.0, 0.0, 26.0, 18.0), PersonKit::Flat(0.0, 26.0, 32.0, 22.0) }, 14, true, false);
		Ring(P, Wicker, FVector(0.0, 0.0, 26.0), FVector(32.0, 0.0, 0.0), FVector(0.0, 22.0, 0.0), 1.6, 14);
		PersonKit::Blob(P, Linen(), FVector(0.0, 0.0, 22.0), FVector(26.0, 17.0, 8.0), 10, 4);
		PersonKit::Blob(P, FLinearColor(0.55f, 0.62f, 0.75f), FVector(8.0, -4.0, 27.0), FVector(12.0, 9.0, 4.0), 8, 3);
	}

	inline FOutfit KidLook(FRandomStream& Random, bool bGirl)
	{
		FOutfit O;
		O.bWoman = bGirl;
		O.Height = Random.FRandRange(118.f, 140.f);
		O.Build = 0.85f;
		O.Top = bGirl ? PersonKit::ETop::Dress : PersonKit::ETop::Jacket;
		const FLinearColor Coats[] = { FLinearColor(0.22f, 0.19f, 0.15f), FLinearColor(0.14f, 0.16f, 0.20f), FLinearColor(0.26f, 0.14f, 0.10f), FLinearColor(0.18f, 0.20f, 0.14f) };
		const FLinearColor Dresses[] = { FLinearColor(0.45f, 0.12f, 0.10f), FLinearColor(0.20f, 0.28f, 0.45f), FLinearColor(0.55f, 0.50f, 0.38f) };
		O.Coat = bGirl ? Dresses[Random.RandRange(0, 2)] : Coats[Random.RandRange(0, 3)];
		O.Trousers = FLinearColor(0.10f, 0.09f, 0.08f);
		O.Hat = bGirl ? PersonKit::EHat::None : (Random.FRand() < 0.6f ? PersonKit::EHat::FlatCap : PersonKit::EHat::None);
		O.HatColor = FLinearColor(0.14f, 0.12f, 0.10f);
		O.bTie = false;
		O.bMoustache = false;
		O.Beard = PersonKit::EBeard::None;
		O.Hair = Random.FRand() < 0.5f ? FLinearColor(0.32f, 0.20f, 0.10f) : FLinearColor(0.10f, 0.07f, 0.05f);
		return O;
	}

	inline FOutfit HousewifeLook(FRandomStream& Random)
	{
		FOutfit O;
		O.bWoman = true;
		O.Height = Random.FRandRange(156.f, 168.f);
		O.Build = Random.FRandRange(1.0f, 1.2f);
		O.Top = PersonKit::ETop::Dress;
		const FLinearColor Dresses[] = { FLinearColor(0.20f, 0.22f, 0.30f), FLinearColor(0.32f, 0.16f, 0.12f), FLinearColor(0.18f, 0.20f, 0.16f), FLinearColor(0.35f, 0.30f, 0.24f) };
		O.Coat = Dresses[Random.RandRange(0, 3)];
		O.Hat = PersonKit::EHat::None;
		O.bScarf = Random.FRand() < 0.5f;
		O.Scarf = FLinearColor(0.50f, 0.42f, 0.30f);
		O.bApron = true;
		O.Apron = Random.FRand() < 0.5f ? FLinearColor(0.80f, 0.78f, 0.72f) : FLinearColor(0.30f, 0.34f, 0.45f);
		O.bTie = false;
		O.Hair = Random.FRand() < 0.4f ? FLinearColor(0.40f, 0.38f, 0.36f) : FLinearColor(0.18f, 0.10f, 0.05f);
		O.Shoes = FLinearColor(0.08f, 0.04f, 0.02f);
		return O;
	}

	// ---------------------------------------------------------------------
	// The wedding party.

	/** The bride's veil: a short cap of white flowers on her head, the tulle falling down her back. Built about the head's middle. */
	inline void Veil(FPiece& P, double Height)
	{
		const double H = Height / 178.0;
		const FLinearColor Tulle(0.92f, 0.92f, 0.90f);
		PersonKit::Loft(P, Tulle, {
			{ FVector(-2.0, 0.0, 9.0), FVector(9.5, 0.0, 0.0), FVector(0.0, 10.0, 0.0) },
			{ FVector(-8.0, 0.0, -4.0), FVector(8.0, 0.0, 0.0), FVector(0.0, 14.0, 0.0) },
			{ FVector(-14.0, 0.0, -30.0 * H), FVector(5.0, 0.0, 0.0), FVector(0.0, 22.0, 0.0) },
			{ FVector(-19.0, 0.0, -75.0 * H), FVector(5.0, 0.0, 0.0), FVector(0.0, 26.0, 0.0) },
		}, 12, false, false, PI * 0.5);
		for (int32 f = 0; f < 7; ++f)
		{
			const double A = (f - 3) * 0.42;
			PersonKit::Blob(P, FLinearColor(0.95f, 0.93f, 0.85f), FVector(1.0 - 3.0 * FMath::Abs(FMath::Sin(A)), 9.5 * FMath::Sin(A), 9.5 + 2.0 * FMath::Cos(A)), FVector(2.0, 2.0, 2.0), 6, 3);
		}
	}

	/** The bride's bouquet: white and pink flowers on green, the stems in her fist, the flowers up along +Z. */
	inline void Bouquet(FPiece& P)
	{
		Tube(P, FLinearColor(0.12f, 0.30f, 0.10f), FVector(0.0, 0.0, -10.0), FVector(0.0, 0.0, 10.0), 2.2, 6);
		PersonKit::Blob(P, FLinearColor(0.10f, 0.26f, 0.08f), FVector(0.0, 0.0, 14.0), FVector(12.0, 12.0, 8.0), 10, 5);
		FRandomStream Dice(12);
		for (int32 f = 0; f < 12; ++f)
		{
			const double A = Dice.FRand() * 2.0 * PI;
			const double R = 9.0 * FMath::Sqrt(Dice.FRand());
			const bool bPink = f % 4 == 0;
			PersonKit::Blob(P, bPink ? FLinearColor(0.85f, 0.50f, 0.55f) : FLinearColor(0.95f, 0.94f, 0.88f),
				FVector(R * FMath::Cos(A), R * FMath::Sin(A), 18.0 + Dice.FRandRange(0.0, 3.0)), FVector(3.2, 3.2, 2.6), 6, 3);
		}
		Box(P, FLinearColor(0.95f, 0.95f, 0.92f), FVector(-0.3, -1.5, -14.0), FVector(0.3, 1.5, -2.0));
	}

	/** The fiddle, held under the chin: built from the chin rest, the body back along -X... see FiddleOnShoulder. Neck along +Y. */
	inline void Fiddle(FPiece& P)
	{
		const FLinearColor Varnish(0.42f, 0.16f, 0.05f);
		PersonKit::Loft(P, Varnish, {
			{ FVector(0.0, -2.0, -2.0), FVector(9.0, 0.0, 0.0), FVector(0.0, 0.0, 0.1) },
			{ FVector(0.0, -2.0, 2.0), FVector(9.0, 0.0, 0.0), FVector(0.0, 0.0, 0.1) },
		}, 4, true, true);
		PersonKit::Blob(P, Varnish, FVector(0.0, 6.0, 0.0), FVector(8.5, 9.5, 2.6), 10, 4);
		PersonKit::Blob(P, Varnish, FVector(0.0, 20.0, 0.0), FVector(7.0, 8.0, 2.6), 10, 4);
		Tube(P, FLinearColor(0.05f, 0.04f, 0.03f), FVector(0.0, 26.0, 2.5), FVector(0.0, 52.0, 2.0), 1.3, 6);
		PersonKit::Blob(P, DarkWood(), FVector(0.0, 54.0, 2.0), FVector(1.8, 3.0, 2.0), 6, 3);
	}

	/** The bow, its frog in the right fist, the stick along +Z. */
	inline void FiddleBow(FPiece& P)
	{
		Tube(P, DarkWood(), FVector(0.0, 0.0, -3.0), FVector(0.0, 0.0, 66.0), 0.6, 4);
		Box(P, IronColor(), FVector(-1.0, -1.0, -3.0), FVector(1.0, 1.0, 3.0));
		Tube(P, Linen(), FVector(1.5, 0.0, 1.0), FVector(1.5, 0.0, 64.0), 0.35, 3);
	}

	/** Where the fiddle sits on the fiddler's left shoulder, in his body's frame, and how it is turned. */
	inline FVector FiddleAt(double Height) { return FVector(10.0, -16.0, 142.0 * Height / 178.0); }
	inline FRotator FiddleTurn() { return FRotator(12.0, -38.0, -28.0); }

	inline FOutfit BrideLook()
	{
		FOutfit O;
		O.bWoman = true;
		O.Height = 162.f;
		O.Top = PersonKit::ETop::Dress;
		O.Coat = FLinearColor(0.90f, 0.89f, 0.86f);
		O.Hat = PersonKit::EHat::None;
		O.bTie = false;
		O.Hair = FLinearColor(0.26f, 0.15f, 0.07f);
		O.Shoes = FLinearColor(0.85f, 0.84f, 0.80f);
		O.FaceShape = 0;
		return O;
	}

	inline FOutfit GroomLook()
	{
		FOutfit O;
		O.Height = 178.f;
		O.Top = PersonKit::ETop::Jacket;
		O.Coat = FLinearColor(0.025f, 0.025f, 0.03f);
		O.Trousers = FLinearColor(0.03f, 0.03f, 0.035f);
		O.Hat = PersonKit::EHat::Bowler;
		O.HatColor = FLinearColor(0.02f, 0.02f, 0.02f);
		O.Shirt = FLinearColor(0.95f, 0.95f, 0.93f);
		O.Tie = FLinearColor(0.85f, 0.84f, 0.82f);
		O.bMoustache = true;
		O.MoustacheKind = PersonKit::EMoustache::Pencil;
		return O;
	}

	/** A wedding guest in Sunday best. */
	inline FOutfit GuestLook(FRandomStream& Random, bool bWoman)
	{
		FOutfit O;
		O.bWoman = bWoman;
		O.Height = bWoman ? Random.FRandRange(156.f, 168.f) : Random.FRandRange(166.f, 182.f);
		const FLinearColor WomenColors[] = { FLinearColor(0.45f, 0.30f, 0.45f), FLinearColor(0.25f, 0.40f, 0.55f), FLinearColor(0.55f, 0.20f, 0.18f), FLinearColor(0.50f, 0.45f, 0.30f), FLinearColor(0.20f, 0.38f, 0.28f) };
		const FLinearColor MenColors[] = { FLinearColor(0.06f, 0.06f, 0.07f), FLinearColor(0.15f, 0.15f, 0.17f), FLinearColor(0.10f, 0.08f, 0.06f), FLinearColor(0.08f, 0.09f, 0.14f) };
		if (bWoman)
		{
			O.Top = Random.FRand() < 0.6f ? PersonKit::ETop::Dress : PersonKit::ETop::Overcoat;
			O.Coat = WomenColors[Random.RandRange(0, 4)];
			O.Hat = PersonKit::EHat::Cloche;
			O.HatColor = O.Coat * 0.6f;
			O.bTie = false;
			O.Hair = Random.FRand() < 0.5f ? FLinearColor(0.20f, 0.12f, 0.06f) : FLinearColor(0.45f, 0.32f, 0.16f);
		}
		else
		{
			O.Top = PersonKit::ETop::Jacket;
			O.Coat = MenColors[Random.RandRange(0, 3)];
			O.Hat = Random.FRand() < 0.5f ? PersonKit::EHat::Bowler : PersonKit::EHat::Fedora;
			O.HatColor = FLinearColor(0.05f, 0.05f, 0.05f);
			O.Tie = Random.FRand() < 0.5f ? FLinearColor(0.50f, 0.08f, 0.06f) : FLinearColor(0.10f, 0.20f, 0.45f);
			O.bMoustache = Random.FRand() < 0.6f;
		}
		O.bAged = Random.FRand() < 0.3f;
		return O;
	}

	/** The gypsy fiddler leading the party: a dark suit with a waistcoat, no hat, black hair and moustache. */
	inline FOutfit FiddlerLook()
	{
		FOutfit O;
		O.Height = 172.f;
		O.Top = PersonKit::ETop::Waistcoat;
		O.Waistcoat = FLinearColor(0.05f, 0.05f, 0.06f);
		O.Coat = O.Waistcoat;
		O.Shirt = FLinearColor(0.92f, 0.90f, 0.86f);
		O.Hat = PersonKit::EHat::None;
		O.Hair = FLinearColor(0.03f, 0.025f, 0.02f);
		O.bMoustache = true;
		O.MoustacheKind = PersonKit::EMoustache::Handlebar;
		O.bTie = false;
		O.bSash = true;
		O.Sash = FLinearColor(0.50f, 0.06f, 0.05f);
		O.Skin = FLinearColor(0.58f, 0.40f, 0.28f);
		return O;
	}

	// ---------------------------------------------------------------------
	// The funeral.

	inline FOutfit MournerLook(FRandomStream& Random, bool bWoman)
	{
		FOutfit O;
		O.bWoman = bWoman;
		O.Height = bWoman ? Random.FRandRange(154.f, 168.f) : Random.FRandRange(164.f, 182.f);
		O.Top = PersonKit::ETop::Overcoat;
		O.Coat = Mourning();
		O.Trousers = Mourning();
		O.Hat = bWoman ? PersonKit::EHat::Cloche : (Random.FRand() < 0.5f ? PersonKit::EHat::Bowler : PersonKit::EHat::Fedora);
		O.HatColor = FLinearColor(0.02f, 0.02f, 0.02f);
		O.HatBand = O.HatColor;
		O.Tie = FLinearColor(0.02f, 0.02f, 0.02f);
		O.bTie = !bWoman;
		O.bAged = Random.FRand() < 0.5f;
		O.bMoustache = !bWoman && Random.FRand() < 0.6f;
		return O;
	}

	/** A funeral wreath of dark leaves and white flowers with a black ribbon, held up by its rim at the bottom (the grip), standing along +Z. */
	inline void Wreath(FPiece& P)
	{
		const FLinearColor Leaves(0.06f, 0.16f, 0.06f);
		const FVector Middle(0.0, 0.0, 30.0);
		for (int32 i = 0; i < 16; ++i)
		{
			const double A = 2.0 * PI * i / 16;
			const FVector At = Middle + FVector(0.0, 28.0 * FMath::Cos(A), 28.0 * FMath::Sin(A));
			PersonKit::Blob(P, Leaves, At, FVector(5.0, 7.0, 7.0), 8, 4);
			if (i % 3 == 0)
			{
				PersonKit::Blob(P, FLinearColor(0.92f, 0.91f, 0.86f), At + FVector(4.0, 0.0, 0.0), FVector(2.8, 3.4, 3.4), 6, 3);
			}
		}
		Box(P, FLinearColor(0.02f, 0.02f, 0.02f), FVector(3.0, -6.0, -6.0), FVector(4.0, -2.0, 6.0));
		Box(P, FLinearColor(0.02f, 0.02f, 0.02f), FVector(3.0, 2.0, -12.0), FVector(4.0, 6.0, 6.0));
	}

	// ---------------------------------------------------------------------
	// The fire brigade.

	enum EFirePart : int32 { FireBody, FireWheelFrontLeft, FireWheelFrontRight, FireWheelBackLeft, FireWheelBackRight, FireBell, NumFireParts };

	constexpr double FireWheelRadius = 46.0;
	constexpr double FireFrontAxle = 175.0;
	constexpr double FireBackAxle = -175.0;
	constexpr double FireTrack = 78.0;
	/** Half its length and width, for its box (cm), and how high its frame is. */
	inline FVector FireHalf() { return FVector(315.0, 100.0, 120.0); }

	/** Each fireman's seat on the bench (their middle, seated), and which side he faces (+1 right, -1 left). */
	constexpr int32 NumFiremen = 6;
	inline FVector FiremanSeat(int32 Index, double& OutSide)
	{
		OutSide = Index % 2 == 0 ? -1.0 : 1.0;
		const int32 Row = Index / 2;
		return FVector(-30.0 - Row * 80.0, OutSide * 30.0, 132.0);
	}
	/** The driver's middle, seated, and the officer beside him. */
	inline FVector FireDriverSeat() { return FVector(108.0, -30.0, 130.0); }
	inline FVector FireOfficerSeat() { return FVector(108.0, 32.0, 130.0); }
	/** Where the bell hangs from (its pivot). */
	inline FVector FireBellAt() { return FVector(152.0, 72.0, 168.0); }

	inline FVector FireWheelHub(int32 Part)
	{
		const bool bFront = Part == FireWheelFrontLeft || Part == FireWheelFrontRight;
		const bool bLeft = Part == FireWheelFrontLeft || Part == FireWheelBackLeft;
		return FVector(bFront ? FireFrontAxle : FireBackAxle, bLeft ? -FireTrack : FireTrack, FireWheelRadius);
	}

	/**
	 * The fire brigade's motor engine of the early 1930s, from the ground
	 * under its middle, facing +X: long red bonnet with the brass radiator,
	 * open driver's bench, the crew's bench down the middle where the firemen
	 * sit back to back facing out over the running boards, the hose reels,
	 * the ladder on top, brass lamps and the big brass bell. The wheels and
	 * the bell are parts of their own, about their pivots.
	 */
	inline void FireEngine(FPiece (&Parts)[NumFireParts])
	{
		const FLinearColor Red = FireRed();
		const FLinearColor Black(0.025f, 0.025f, 0.025f);
		const FLinearColor Gold(0.65f, 0.48f, 0.15f);
		const FLinearColor Leather(0.16f, 0.06f, 0.03f);
		const FLinearColor Hose(0.55f, 0.50f, 0.40f);
		FPiece& B = Parts[FireBody];
		// The chassis and the running boards.
		Box(B, Black, FVector(-300.0, -46.0, 50.0), FVector(260.0, 46.0, 66.0));
		for (const double S : { -1.0, 1.0 })
		{
			Box(B, Black, FVector(-250.0, S * 72.0, 56.0), FVector(120.0, S * 102.0, 60.0));
			Box(B, Gold, FVector(-250.0, S * 101.0, 58.0), FVector(120.0, S * 103.0, 60.0));
			// Mudguards over the wheels: a curve of short plates.
			for (const double Axle : { FireFrontAxle, FireBackAxle })
			{
				for (int32 k = 0; k < 7; ++k)
				{
					const double A0 = PI * k / 7.0;
					const double A1 = PI * (k + 1) / 7.0;
					const double R = FireWheelRadius + 8.0;
					const FVector C0(Axle + R * FMath::Cos(A0), 0.0, FireWheelRadius + R * FMath::Sin(A0));
					const FVector C1(Axle + R * FMath::Cos(A1), 0.0, FireWheelRadius + R * FMath::Sin(A1));
					const FVector Lo = C0.ComponentMin(C1);
					const FVector Hi = C0.ComponentMax(C1);
					Box(B, Black, FVector(Lo.X, S * 62.0, Lo.Z), FVector(Hi.X + 1.0, S * 100.0, Hi.Z + 2.0));
				}
			}
		}
		// The bonnet, the radiator and its brass shell, the starting handle.
		Box(B, Red, FVector(160.0, -44.0, 66.0), FVector(282.0, 44.0, 140.0));
		Box(B, Gold, FVector(160.0, -45.0, 136.0), FVector(282.0, 45.0, 139.0));
		Box(B, Gold, FVector(282.0, -42.0, 62.0), FVector(298.0, 42.0, 146.0));
		Box(B, FLinearColor(0.08f, 0.07f, 0.06f), FVector(296.0, -34.0, 72.0), FVector(299.0, 34.0, 138.0));
		Tube(B, IronColor(), FVector(298.0, 0.0, 74.0), FVector(312.0, 0.0, 74.0), 1.4, 5);
		// Headlamps on stalks, and a pair of side lamps by the screen.
		for (const double S : { -1.0, 1.0 })
		{
			Tube(B, IronColor(), FVector(272.0, S * 54.0, 100.0), FVector(272.0, S * 54.0, 118.0), 1.4, 5);
			PersonKit::Loft(B, Gold, { PersonKit::Flat(262.0, 128.0, 0.1, 13.0, S * 54.0) , PersonKit::Flat(262.0, 128.0, 0.1, 13.0, S * 54.0) }, 4, false, false);
			TArray<PersonKit::FRing> Lamp;
			Lamp.Add({ FVector(262.0, S * 54.0, 128.0), FVector(0.0, 12.0, 0.0), FVector(0.0, 0.0, 12.0) });
			Lamp.Add({ FVector(284.0, S * 54.0, 128.0), FVector(0.0, 13.0, 0.0), FVector(0.0, 0.0, 13.0) });
			PersonKit::Loft(B, Gold, Lamp, 12, true, false);
			Box(B, FLinearColor(0.95f, 0.90f, 0.70f), FVector(284.0, S * 54.0 - 10.0, 118.0), FVector(285.0, S * 54.0 + 10.0, 138.0));
		}
		// The scuttle and the open driver's bench, its leather seat and back, the wheel.
		Box(B, Red, FVector(140.0, -48.0, 66.0), FVector(160.0, 48.0, 150.0));
		Box(B, FLinearColor(0.20f, 0.20f, 0.22f), FVector(152.0, -46.0, 150.0), FVector(154.0, 46.0, 176.0));
		Box(B, Red, FVector(80.0, -60.0, 66.0), FVector(140.0, 60.0, 112.0));
		Box(B, Leather, FVector(90.0, -58.0, 112.0), FVector(136.0, 58.0, 122.0));
		Box(B, Leather, FVector(80.0, -58.0, 122.0), FVector(90.0, 58.0, 160.0));
		Tube(B, Black, FVector(146.0, -30.0, 130.0), FVector(128.0, -30.0, 150.0), 1.6, 5);
		Ring(B, Black, FVector(128.0, -30.0, 151.0), FVector(0.0, 16.0, 0.0), FVector(7.0, 0.0, 7.0), 1.2, 14);
		// The crew's body: a red deck with a gold line, the bench down the middle, rails along it.
		Box(B, Red, FVector(-300.0, -66.0, 66.0), FVector(80.0, 66.0, 112.0));
		Box(B, Gold, FVector(-300.0, -67.0, 98.0), FVector(80.0, 67.0, 101.0));
		Box(B, Leather, FVector(-240.0, -40.0, 112.0), FVector(20.0, 40.0, 124.0));
		Box(B, Red, FVector(-240.0, -10.0, 124.0), FVector(20.0, 10.0, 166.0));
		Box(B, Leather, FVector(-240.0, -12.0, 166.0), FVector(20.0, 12.0, 174.0));
		for (const double S : { -1.0, 1.0 })
		{
			Tube(B, Gold, FVector(-250.0, S * 64.0, 112.0), FVector(-250.0, S * 64.0, 150.0), 1.4, 6);
			Tube(B, Gold, FVector(70.0, S * 64.0, 112.0), FVector(70.0, S * 64.0, 150.0), 1.4, 6);
		}
		// The hose reel at the back: two drums of grey canvas hose between red cheeks.
		for (const double S : { -1.0, 1.0 })
		{
			TArray<PersonKit::FRing> Cheek;
			Cheek.Add({ FVector(-272.0, S * 62.0, 140.0), FVector(42.0, 0.0, 0.0), FVector(0.0, 0.0, 42.0) });
			Cheek.Add({ FVector(-272.0, S * 66.0, 140.0), FVector(42.0, 0.0, 0.0), FVector(0.0, 0.0, 42.0) });
			PersonKit::Loft(B, Red, Cheek, 18, true, true);
		}
		TArray<PersonKit::FRing> Reel;
		Reel.Add({ FVector(-272.0, -62.0, 140.0), FVector(34.0, 0.0, 0.0), FVector(0.0, 0.0, 34.0) });
		Reel.Add({ FVector(-272.0, 62.0, 140.0), FVector(34.0, 0.0, 0.0), FVector(0.0, 0.0, 34.0) });
		PersonKit::Loft(B, Hose, Reel, 18, false, false);
		for (int32 k = -4; k <= 4; ++k)
		{
			Ring(B, Hose * 0.8f, FVector(-272.0, k * 13.0, 140.0), FVector(34.5, 0.0, 0.0), FVector(0.0, 0.0, 34.5), 0.8, 18);
		}
		// The pump's brass dome and outlets behind the driver.
		PersonKit::Blob(B, Gold, FVector(50.0, 0.0, 112.0), FVector(18.0, 18.0, 26.0), 12, 6, 0.0);
		for (const double S : { -1.0, 1.0 })
		{
			Tube(B, Gold, FVector(50.0, S * 16.0, 118.0), FVector(50.0, S * 68.0, 104.0), 3.0, 8);
		}
		// The ladder on hooks over the bench, longer than the engine.
		for (const double S : { -1.0, 1.0 })
		{
			Tube(B, Wood(), FVector(-330.0, S * 24.0, 196.0), FVector(250.0, S * 24.0, 196.0), 2.4, 6);
			for (const double X : { -200.0, 40.0 })
			{
				Tube(B, IronColor(), FVector(X, S * 24.0, 174.0), FVector(X, S * 24.0, 194.0), 1.4, 5);
			}
			Tube(B, IronColor(), FVector(200.0, S * 24.0, 150.0), FVector(200.0, S * 24.0, 194.0), 1.4, 5);
		}
		for (double X = -320.0; X <= 240.0; X += 30.0)
		{
			Tube(B, Wood(), FVector(X, -24.0, 196.0), FVector(X, 24.0, 196.0), 1.3, 5);
		}
		// A coat of arms on the doors.
		for (const double S : { -1.0, 1.0 })
		{
			PersonKit::Loft(B, Gold, { PersonKit::Flat(-100.0, 78.0, 0.1, 0.1, S * 67.0), PersonKit::Flat(-100.0, 92.0, 9.0, 0.1, S * 67.0), PersonKit::Flat(-100.0, 96.0, 8.0, 0.1, S * 67.0) }, 8, false, false);
		}
		// The wheels: red artillery spokes, black tyres.
		for (int32 w = FireWheelFrontLeft; w <= FireWheelBackRight; ++w)
		{
			SpokedWheel(Parts[w], Red, FVector::ZeroVector, FireWheelRadius, 12.0, 12);
		}
		// The bell: a brass bell hanging under its bracket, clapper inside.
		FPiece& Bell = Parts[FireBell];
		Tube(Bell, IronColor(), FVector(0.0, -14.0, 0.0), FVector(0.0, 0.0, 0.0), 1.4, 5);
		PersonKit::Loft(Bell, Gold, {
			PersonKit::Flat(0.0, -26.0, 13.0, 13.0),
			PersonKit::Flat(0.0, -18.0, 10.0, 10.0),
			PersonKit::Flat(0.0, -8.0, 8.0, 8.0),
			PersonKit::Flat(0.0, -2.0, 4.0, 4.0),
		}, 14, false, true);
		PersonKit::Blob(Bell, IronColor(), FVector(0.0, 0.0, -24.0), FVector(2.5, 2.5, 2.5), 6, 3);
	}

	/** A fireman's brass helmet with its comb and the black leather neck flap, about the head's middle. */
	inline void FireHelmet(FPiece& P)
	{
		const FLinearColor Gold(0.70f, 0.52f, 0.18f);
		PersonKit::Blob(P, Gold, FVector(-0.5, 0.0, 2.0), FVector(11.8, 10.4, 12.4), 12, 6, -0.1);
		Box(P, Gold, FVector(-12.0, -1.2, 10.0), FVector(10.0, 1.2, 15.5));
		PersonKit::Loft(P, Gold, { PersonKit::Flat(1.0, 1.0, 13.0, 11.5), PersonKit::Flat(3.0, 0.0, 15.0, 12.5) }, 14, false, false);
		PersonKit::Loft(P, FLinearColor(0.03f, 0.03f, 0.03f), { PersonKit::Flat(-8.0, 0.0, 6.0, 11.0), PersonKit::Flat(-13.0, -12.0, 5.0, 12.0) }, 10, false, false, PI);
	}

	inline FOutfit FiremanLook(FRandomStream& Random)
	{
		FOutfit O;
		O.Height = Random.FRandRange(170.f, 182.f);
		O.Top = PersonKit::ETop::Jacket;
		O.Coat = FLinearColor(0.04f, 0.05f, 0.09f);
		O.Trousers = FLinearColor(0.04f, 0.05f, 0.09f);
		O.Hat = PersonKit::EHat::None;
		O.bTie = false;
		O.bSash = true;
		O.Sash = FLinearColor(0.22f, 0.13f, 0.05f);
		O.bBoots = true;
		O.bMoustache = Random.FRand() < 0.7f;
		O.Hair = FLinearColor(0.10f, 0.07f, 0.05f);
		return O;
	}

	// ---------------------------------------------------------------------
	// Shops in the morning.

	/** A shopkeeper in his long grey or tan dust coat. */
	inline FOutfit ShopkeeperLook(FRandomStream& Random)
	{
		FOutfit O;
		O.Height = Random.FRandRange(164.f, 178.f);
		O.Build = Random.FRandRange(1.0f, 1.2f);
		O.Top = PersonKit::ETop::Overcoat;
		O.Coat = Random.FRand() < 0.5f ? FLinearColor(0.45f, 0.43f, 0.38f) : FLinearColor(0.52f, 0.42f, 0.28f);
		O.Hat = Random.FRand() < 0.5f ? PersonKit::EHat::None : PersonKit::EHat::FlatCap;
		O.HatColor = FLinearColor(0.15f, 0.13f, 0.10f);
		O.bMoustache = Random.FRand() < 0.7f;
		O.bAged = Random.FRand() < 0.4f;
		return O;
	}

	/**
	 * A corrugated iron shutter Width across (Y) and Drop tall, hanging from
	 * its top edge at 0 (so scaling Z rolls it up), its face towards +X.
	 */
	inline void Shutter(FPiece& P, double Width, double Drop)
	{
		const FLinearColor Light(0.30f, 0.31f, 0.31f);
		const FLinearColor Dark(0.22f, 0.23f, 0.23f);
		int32 Band = 0;
		for (double Z = 0.0; Z < Drop; Z += 8.0, ++Band)
		{
			Box(P, Band % 2 ? Light : Dark, FVector(-0.75, -Width * 0.5, -FMath::Min(Z + 8.0, Drop)), FVector(0.75, Width * 0.5, -Z));
		}
		Box(P, IronColor(), FVector(0.0, -Width * 0.5, -Drop), FVector(2.5, Width * 0.5, -Drop + 6.0));
	}

	/** The roller box over the shop window the shutter rolls into. */
	inline void ShutterBox(FPiece& P, double Width)
	{
		Box(P, FLinearColor(0.20f, 0.21f, 0.21f), FVector(0.0, -Width * 0.5 - 2.0, 0.0), FVector(9.0, Width * 0.5 + 2.0, 14.0));
	}
}
