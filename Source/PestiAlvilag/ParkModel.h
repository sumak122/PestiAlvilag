// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PersonKit.h"

/**
 * The shapes of the newer things in Tisza Kálmán tér: the statue east of
 * the middle and the big playground in the south-east quarter (sandpits,
 * swings, seesaws, a roundabout, a climbing frame, a slide and a drinking
 * fountain), and how its children and nannies are dressed. Header-only and
 * free of engine objects, so the offline preview builds the same shapes.
 *
 * Every object is in cm with its middle on the ground at the origin,
 * X forward, Y to the right, Z up. Shapes are the engine's basic shapes:
 * a 100 cm cube, cylinder (100 across, 100 tall), sphere and cone, all
 * centred on their middle.
 */
namespace ParkModel
{
	enum EShape : int32 { Cube, Cylinder, Sphere, Cone };

	struct FPart
	{
		int32 Shape;
		FLinearColor Color;
		/** Middle of the shape (cm). */
		FVector At;
		/** Whole size (cm). */
		FVector Size;
		double Pitch = 0.0;
		double Roll = 0.0;
		bool bCollide = false;
	};

	/** The things to play on, in the order the furniture script numbers them. */
	enum EPlay : int32 { Sandpit, Swings, Seesaw, Roundabout, ClimbingFrame, Slide, Fountain, NumPlay };

	/** Who is in the playground, as the furniture script numbers them. */
	enum EPlayer : int32 { ChildStanding, ChildSeated, GrownUpSeated, GrownUpStanding };

	// Sizes the furniture script also uses to seat the children (cm).
	constexpr double SwingSeatHeight = 50.0;
	constexpr double SwingSeatApart = 95.0;
	constexpr double SeesawEnd = 170.0;
	constexpr double SeesawTilt = 8.0;
	constexpr double SeesawPivotHeight = 45.0;
	constexpr double RoundaboutRadius = 150.0;
	constexpr double RoundaboutTop = 44.0;

	namespace Colors
	{
		const FLinearColor StoneDark(0.40f, 0.38f, 0.34f);
		const FLinearColor StoneLight(0.58f, 0.55f, 0.49f);
		/** Old bronze, darkened, with a little green. */
		const FLinearColor Bronze(0.08f, 0.10f, 0.08f);
		const FLinearColor Timber(0.32f, 0.20f, 0.10f);
		const FLinearColor Plank(0.42f, 0.30f, 0.16f);
		const FLinearColor Sand(0.70f, 0.62f, 0.45f);
		const FLinearColor Rope(0.45f, 0.38f, 0.25f);
		const FLinearColor Iron(0.035f, 0.05f, 0.04f);
		const FLinearColor ParkGreen(0.08f, 0.22f, 0.10f);
		const FLinearColor FadedRed(0.42f, 0.09f, 0.05f);
		const FLinearColor Bucket(0.55f, 0.10f, 0.04f);
	}

	/** Pitch and roll that turn a shape's Z axis along Dir (any length). */
	inline void Along(const FVector& Dir, double& OutPitch, double& OutRoll)
	{
		const double Length = FMath::Sqrt(Dir.X * Dir.X + Dir.Y * Dir.Y + Dir.Z * Dir.Z);
		if (Length <= 0.0)
		{
			OutPitch = OutRoll = 0.0;
			return;
		}
		const FVector D(Dir.X / Length, Dir.Y / Length, Dir.Z / Length);
		OutRoll = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(D.Y, -1.0, 1.0)));
		OutPitch = FMath::RadiansToDegrees(FMath::Atan2(-D.X, D.Z));
	}

	/** A round bar of this thickness from A to B. */
	inline FPart Bar(const FVector& A, const FVector& B, double Thick, const FLinearColor& Color, bool bCollide = false)
	{
		const FVector D(B.X - A.X, B.Y - A.Y, B.Z - A.Z);
		FPart Part{ Cylinder, Color, FVector((A.X + B.X) * 0.5, (A.Y + B.Y) * 0.5, (A.Z + B.Z) * 0.5),
			FVector(Thick, Thick, FMath::Sqrt(D.X * D.X + D.Y * D.Y + D.Z * D.Z)) };
		Along(D, Part.Pitch, Part.Roll);
		Part.bCollide = bCollide;
		return Part;
	}

	inline FPart Box(const FVector& At, const FVector& Size, const FLinearColor& Color, bool bCollide = false)
	{
		return { Cube, Color, At, Size, 0.0, 0.0, bCollide };
	}

	/**
	 * The statue: a stepped stone pedestal with a bronze plaque, and on it,
	 * larger than life, a statesman of the last century in a frock coat,
	 * bearded, one hand at his side and the other holding a scroll.
	 */
	inline TArray<FPart> Statue()
	{
		using namespace Colors;
		TArray<FPart> P;
		P.Add(Box(FVector(0, 0, 15), FVector(460, 460, 30), StoneDark, true));
		P.Add(Box(FVector(0, 0, 45), FVector(380, 380, 30), StoneDark, true));
		P.Add(Box(FVector(0, 0, 85), FVector(230, 230, 50), StoneLight, true));
		P.Add(Box(FVector(0, 0, 230), FVector(180, 180, 240), StoneLight, true));
		P.Add(Box(FVector(0, 0, 362), FVector(210, 210, 24), StoneLight, true));
		P.Add(Box(FVector(0, 0, 384), FVector(160, 160, 20), StoneDark, true));
		P.Add(Box(FVector(91.5, 0, 250), FVector(3, 110, 60), Bronze));
		for (int32 i = 0; i < 4; ++i)
		{
			const double X = (i % 2 ? 1.0 : -1.0) * 205.0;
			const double Y = (i / 2 ? 1.0 : -1.0) * 205.0;
			P.Add({ Cylinder, StoneDark, FVector(X, Y, 60), FVector(24, 24, 60), 0.0, 0.0, true });
			P.Add({ Sphere, StoneDark, FVector(X, Y, 92), FVector(26, 26, 18) });
		}

		// The figure, drawn for a man of 180 cm and set up larger than life.
		constexpr double Scale = 1.6;
		constexpr double Top = 394.0;
		TArray<FPart> Figure;
		Figure.Add(Box(FVector(0, 0, 3), FVector(70, 70, 6), Bronze));
		for (double Side : { -1.0, 1.0 })
		{
			Figure.Add(Box(FVector(6, Side * 11, 10), FVector(26, 10, 8), Bronze));
			Figure.Add({ Cylinder, Bronze, FVector(0, Side * 11, 52), FVector(15, 15, 80) });
		}
		Figure.Add({ Cylinder, Bronze, FVector(-2, 0, 84), FVector(34, 44, 48) });
		Figure.Add({ Cylinder, Bronze, FVector(0, 0, 128), FVector(30, 42, 50) });
		Figure.Add({ Sphere, Bronze, FVector(0, 0, 150), FVector(32, 50, 18) });
		Figure.Add({ Cylinder, Bronze, FVector(0, 0, 160), FVector(12, 12, 10) });
		Figure.Add({ Sphere, Bronze, FVector(1, 0, 172), FVector(21, 19, 25) });
		Figure.Add({ Sphere, Bronze, FVector(7, 0, 164), FVector(12, 15, 14) });
		// The right arm at his side; the left forward, holding a scroll.
		Figure.Add(Bar(FVector(0, 24, 148), FVector(2, 26, 94), 11, Bronze));
		Figure.Add({ Sphere, Bronze, FVector(2, 26, 90), FVector(9, 9, 10) });
		Figure.Add(Bar(FVector(0, -24, 148), FVector(4, -25, 112), 11, Bronze));
		Figure.Add(Bar(FVector(4, -25, 112), FVector(30, -16, 118), 10, Bronze));
		Figure.Add({ Sphere, Bronze, FVector(32, -14, 118), FVector(9, 9, 9) });
		Figure.Add(Bar(FVector(34, -26, 119), FVector(34, -2, 117), 6, Bronze));
		for (FPart& Part : Figure)
		{
			Part.At = FVector(Part.At.X * Scale, Part.At.Y * Scale, Top + Part.At.Z * Scale);
			Part.Size = FVector(Part.Size.X * Scale, Part.Size.Y * Scale, Part.Size.Z * Scale);
			P.Add(Part);
		}
		return P;
	}

	/** A square sandpit in a frame of planks, with a heap and a child's bucket. */
	inline TArray<FPart> MakeSandpit()
	{
		using namespace Colors;
		TArray<FPart> P;
		P.Add(Box(FVector(0, 0, 7), FVector(510, 510, 14), Sand));
		for (double Side : { -1.0, 1.0 })
		{
			P.Add(Box(FVector(0, Side * 265, 15), FVector(550, 20, 30), Plank, true));
			P.Add(Box(FVector(Side * 265, 0, 15), FVector(20, 510, 30), Plank, true));
		}
		P.Add({ Sphere, Sand, FVector(-60, 40, 14), FVector(130, 100, 44) });
		P.Add({ Cylinder, Bucket, FVector(80, -60, 22), FVector(16, 16, 16) });
		P.Add(Box(FVector(95, -35, 15), FVector(26, 6, 2), Iron));
		return P;
	}

	/** A timber frame with two swings on ropes. */
	inline TArray<FPart> MakeSwings()
	{
		using namespace Colors;
		TArray<FPart> P;
		constexpr double Height = 270.0;
		for (double Side : { -1.0, 1.0 })
		{
			const FVector TopEnd(0, Side * 250, Height);
			P.Add(Bar(FVector(-110, Side * 250, 0), TopEnd, 14, Timber, true));
			P.Add(Bar(FVector(110, Side * 250, 0), TopEnd, 14, Timber, true));
			P.Add(Bar(FVector(-50, Side * 250, 120), FVector(50, Side * 250, 120), 8, Timber));
		}
		P.Add(Bar(FVector(0, -265, Height), FVector(0, 265, Height), 16, Timber));
		for (double Seat : { -SwingSeatApart, SwingSeatApart })
		{
			for (double Off : { -22.0, 22.0 })
			{
				P.Add(Bar(FVector(0, Seat + Off, Height - 6), FVector(0, Seat + Off, SwingSeatHeight), 2.5, Rope));
			}
			P.Add(Box(FVector(0, Seat, SwingSeatHeight - 2), FVector(24, 52, 4), Plank));
		}
		return P;
	}

	/** A painted board on a timber stand, with a handle at each end. */
	inline TArray<FPart> MakeSeesaw()
	{
		using namespace Colors;
		TArray<FPart> P;
		P.Add(Box(FVector(0, 0, 20), FVector(30, 50, 40), Timber, true));
		P.Add(Bar(FVector(0, -30, SeesawPivotHeight - 4), FVector(0, 30, SeesawPivotHeight - 4), 8, Iron));
		const double Slope = FMath::Tan(FMath::DegreesToRadians(SeesawTilt));
		P.Add({ Cube, FadedRed, FVector(0, 0, SeesawPivotHeight), FVector(420, 28, 5), SeesawTilt, 0.0, true });
		for (double End : { -1.0, 1.0 })
		{
			const double X = End * (SeesawEnd - 40.0);
			const double Z = SeesawPivotHeight + X * Slope;
			P.Add(Bar(FVector(X, 0, Z), FVector(X, 0, Z + 30), 4, Iron));
			P.Add(Bar(FVector(X, -16, Z + 30), FVector(X, 16, Z + 30), 4, Iron));
		}
		return P;
	}

	/** A low round platform the children push round and jump on. */
	inline TArray<FPart> MakeRoundabout()
	{
		using namespace Colors;
		TArray<FPart> P;
		P.Add({ Cylinder, Iron, FVector(0, 0, 20), FVector(24, 24, 40), 0.0, 0.0, true });
		P.Add({ Cylinder, ParkGreen, FVector(0, 0, RoundaboutTop - 4), FVector(RoundaboutRadius * 2, RoundaboutRadius * 2, 8), 0.0, 0.0, true });
		P.Add(Bar(FVector(0, 0, RoundaboutTop), FVector(0, 0, RoundaboutTop + 55), 10, Iron));
		const double Rail = RoundaboutRadius - 10.0;
		for (int32 i = 0; i < 4; ++i)
		{
			const double A = PI * 0.5 * i;
			const FVector Out(FMath::Cos(A) * Rail, FMath::Sin(A) * Rail, 0.0);
			P.Add(Bar(FVector(0, 0, RoundaboutTop + 55), FVector(Out.X, Out.Y, RoundaboutTop + 55), 5, Iron));
			P.Add(Bar(FVector(Out.X, Out.Y, RoundaboutTop), FVector(Out.X, Out.Y, RoundaboutTop + 55), 5, Iron));
		}
		return P;
	}

	/** A cube of iron bars to climb, painted park green. */
	inline TArray<FPart> MakeClimbingFrame()
	{
		using namespace Colors;
		TArray<FPart> P;
		const double Lines[] = { -120.0, 0.0, 120.0 };
		for (double X : Lines)
		{
			for (double Y : Lines)
			{
				P.Add(Bar(FVector(X, Y, 0), FVector(X, Y, 210), 6, ParkGreen, true));
			}
		}
		for (double Z : { 70.0, 140.0, 210.0 })
		{
			for (double Line : Lines)
			{
				P.Add(Bar(FVector(-120, Line, Z), FVector(120, Line, Z), 5, ParkGreen));
				P.Add(Bar(FVector(Line, -120, Z), FVector(Line, 120, Z), 5, ParkGreen));
			}
		}
		return P;
	}

	/** A wooden slide: a ladder up to a little platform and the chute down, forward. */
	inline TArray<FPart> MakeSlide()
	{
		using namespace Colors;
		TArray<FPart> P;
		constexpr double Platform = 180.0;
		for (double Side : { -1.0, 1.0 })
		{
			P.Add(Bar(FVector(-235, Side * 25, 0), FVector(-195, Side * 25, Platform + 60), 7, Timber, true));
			P.Add(Bar(FVector(-140, Side * 30, 0), FVector(-140, Side * 30, Platform), 8, Timber, true));
		}
		for (int32 i = 1; i * 30.0 < Platform; ++i)
		{
			const double Z = i * 30.0;
			const double X = -235 + 40.0 * Z / (Platform + 60.0);
			P.Add(Bar(FVector(X, -25, Z), FVector(X, 25, Z), 5, Timber));
		}
		P.Add(Box(FVector(-170, 0, Platform), FVector(70, 66, 6), Plank, true));
		const FVector From(-135, 0, Platform);
		const FVector To(225, 0, 18);
		const FVector Mid((From.X + To.X) * 0.5, 0.0, (From.Z + To.Z) * 0.5);
		const double Length = FMath::Sqrt((To.X - From.X) * (To.X - From.X) + (To.Z - From.Z) * (To.Z - From.Z));
		const double Pitch = FMath::RadiansToDegrees(FMath::Atan2(From.Z - To.Z, To.X - From.X));
		P.Add({ Cube, Plank, Mid, FVector(Length, 50, 4), -Pitch, 0.0, true });
		for (double Side : { -1.0, 1.0 })
		{
			P.Add({ Cube, Timber, FVector(Mid.X, Side * 27, Mid.Z + 8), FVector(Length, 4, 18), -Pitch, 0.0 });
		}
		P.Add(Bar(FVector(200, 0, 0), FVector(200, 0, 26), 10, Timber));
		return P;
	}

	/** A stone drinking fountain with a basin and a brass spout. */
	inline TArray<FPart> MakeFountain()
	{
		using namespace Colors;
		TArray<FPart> P;
		P.Add({ Cylinder, StoneLight, FVector(0, 0, 45), FVector(40, 40, 90), 0.0, 0.0, true });
		P.Add({ Cylinder, StoneLight, FVector(0, 0, 94), FVector(64, 64, 10) });
		P.Add({ Cylinder, Iron, FVector(0, 0, 99.5), FVector(52, 52, 1) });
		P.Add(Bar(FVector(0, 0, 99), FVector(12, 0, 112), 4, FLinearColor(0.45f, 0.33f, 0.10f)));
		P.Add(Box(FVector(42, 0, 5), FVector(36, 50, 10), StoneDark, true));
		return P;
	}

	inline TArray<FPart> Play(int32 Kind)
	{
		switch (Kind)
		{
		case Sandpit: return MakeSandpit();
		case Swings: return MakeSwings();
		case Seesaw: return MakeSeesaw();
		case Roundabout: return MakeRoundabout();
		case ClimbingFrame: return MakeClimbingFrame();
		case Slide: return MakeSlide();
		case Fountain: return MakeFountain();
		default: return {};
		}
	}

	/** A child of five to ten, dressed for an autumn afternoon. */
	inline PersonKit::FOutfit ChildOutfit(FRandomStream& Random)
	{
		using namespace PersonKit;
		static const FLinearColor Cloth[] = {
			FLinearColor(0.10f, 0.12f, 0.22f), FLinearColor(0.30f, 0.08f, 0.06f), FLinearColor(0.12f, 0.20f, 0.10f),
			FLinearColor(0.35f, 0.28f, 0.18f), FLinearColor(0.20f, 0.20f, 0.22f), FLinearColor(0.45f, 0.30f, 0.32f),
		};
		const int32 Count = static_cast<int32>(UE_ARRAY_COUNT(Cloth));
		FOutfit O;
		O.bWoman = Random.FRand() < 0.5f;
		O.Height = Random.FRandRange(105.f, 138.f);
		O.Build = 0.55f + 0.2f * (O.Height - 105.f) / 33.f;
		O.bTie = false;
		O.Held = EHeld::None;
		O.Coat = Cloth[Random.RandRange(0, Count - 1)];
		O.Trousers = Cloth[Random.RandRange(0, Count - 1)] * 0.7f;
		O.Shirt = FLinearColor(0.80f, 0.78f, 0.72f);
		O.Hair = Random.FRand() < 0.4f ? FLinearColor(0.42f, 0.30f, 0.15f) : FLinearColor(0.12f, 0.08f, 0.05f);
		if (O.bWoman)
		{
			O.Top = ETop::Dress;
			O.Hat = Random.FRand() < 0.3f ? EHat::Cloche : EHat::None;
			O.HatColor = Cloth[Random.RandRange(0, Count - 1)];
		}
		else
		{
			O.Top = Random.FRand() < 0.6f ? ETop::Jacket : ETop::Shirt;
			O.Hat = Random.FRand() < 0.5f ? EHat::FlatCap : EHat::None;
			O.HatColor = FLinearColor(0.20f, 0.18f, 0.15f);
		}
		return O;
	}

	/** A mother or a nanny on a bench: a dark coat or a nanny's white apron. */
	inline PersonKit::FOutfit GrownUpOutfit(FRandomStream& Random)
	{
		using namespace PersonKit;
		FOutfit O;
		O.bWoman = true;
		O.Height = Random.FRandRange(156.f, 170.f);
		O.Build = Random.FRandRange(0.95f, 1.15f);
		O.bTie = false;
		O.Top = Random.FRand() < 0.5f ? ETop::Overcoat : ETop::Dress;
		O.Coat = Random.FRand() < 0.5f ? FLinearColor(0.10f, 0.09f, 0.10f) : FLinearColor(0.22f, 0.16f, 0.12f);
		O.bApron = O.Top == ETop::Dress && Random.FRand() < 0.6f;
		O.Hat = O.bApron ? EHat::None : EHat::Cloche;
		O.HatColor = FLinearColor(0.15f, 0.10f, 0.10f);
		O.Hair = FLinearColor(0.15f, 0.10f, 0.06f);
		return O;
	}

	/**
	 * How high above the ground a person's actor goes (its middle,
	 * PersonModel's MiddleHeight above the feet when standing) to sit on a
	 * seat this high.
	 */
	inline double SeatedMiddle(const PersonKit::FOutfit& Outfit, double SeatHeight, double MiddleHeight)
	{
		// The underside of the thighs: the hips less the thigh's thickness.
		const double Underside = 92.0 * Outfit.Height / 178.0 - 9.0 * Outfit.Build * (Outfit.bWoman ? 0.85 : 1.0);
		return SeatHeight - Underside + MiddleHeight;
	}
}
