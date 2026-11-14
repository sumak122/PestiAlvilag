// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PersonKit.h"

/**
 * Cats and dogs of the courtyards and streets, built with the dress-up kit's
 * lofts and blobs, in pieces that move: the body, the head (hanging from the
 * neck), the tail (from its root) and four legs (from shoulder and hip).
 *
 * Two named cats live in Laci's yard: Lafi, white with a brown tabby cap and
 * back, and Bütyök, a stout, sleepy brown tabby. Street cats and two kinds of
 * street dog (a rough-coated mutt and a black puli) come in a few colours.
 *
 * Centimetres, X forward, Y to the right, Z up, the origin on the ground
 * under the middle of the animal. Pose gives each piece's turn about its
 * pivot (and the body's own turn and lift) for sitting, lying, standing,
 * walking and running. Header-only so it can be built and looked at outside the engine.
 */
namespace AnimalModel
{
	enum EPart : int32 { Body, Head, Tail, FrontLeft, FrontRight, HindLeft, HindRight, NumParts };

	enum class EKind : uint8 { Lafi, Butyok, TabbyCat, BlackCat, GingerCat, Mutt, Puli, Count };

	enum class EPose : uint8 { Stand, Walk, Run, Sit, Lie, Lap, Count };

	struct FAnimal
	{
		PersonKit::FPiece Parts[NumParts];
	};

	/** How one kind is shaped and coloured. */
	struct FShape
	{
		bool bDog = false;
		/** Body length (rump to chest), height of the back, girth (cm). */
		double Length = 40.0;
		double Back = 24.0;
		double Girth = 1.0;
		/** Head size, muzzle length, and the ears: pointed (cats) or hanging (dogs). */
		double HeadSize = 1.0;
		double Muzzle = 3.0;
		bool bHangingEars = false;
		/** Tail length and thickness. */
		double TailLength = 26.0;
		double TailThick = 2.0;
		/** A coat of cords down to the ground (the puli). */
		bool bCorded = false;
		FLinearColor Coat;
		/** A tabby: darker stripes in Stripe. */
		bool bStriped = false;
		FLinearColor Stripe;
		/** White chest, belly, legs and face blaze (Lafi), or only a little on chin and chest. */
		FLinearColor White;
		bool bWhiteBelly = false;
		bool bWhiteChin = false;
		FLinearColor Eye;
		FLinearColor Nose;
		/** Half-closed eyes (Bütyök looks at everything as if it had disappointed him). */
		bool bSleepy = false;
	};

	inline FShape ShapeOf(EKind Kind)
	{
		FShape S;
		const FLinearColor Tabby(0.16f, 0.11f, 0.065f);
		const FLinearColor TabbyStripe(0.045f, 0.035f, 0.025f);
		const FLinearColor White(0.80f, 0.78f, 0.74f);
		S.White = White;
		S.Eye = FLinearColor(0.42f, 0.45f, 0.10f);
		S.Nose = FLinearColor(0.70f, 0.38f, 0.38f);
		switch (Kind)
		{
		case EKind::Lafi:
			S.bStriped = true;
			// Slim and long-legged; the tabby cap over ears and eyes, a white blaze down the face.
			S.Length = 40.0;
			S.Back = 25.0;
			S.Girth = 0.9;
			S.Coat = FLinearColor(0.20f, 0.15f, 0.10f);
			S.Stripe = TabbyStripe;
			S.bWhiteBelly = true;
			S.Eye = FLinearColor(0.48f, 0.52f, 0.16f);
			break;
		case EKind::Butyok:
			S.bStriped = true;
			// Stout, round, striped all over, a little white under the chin, a bushy tail.
			S.Length = 42.0;
			S.Back = 24.0;
			S.Girth = 1.25;
			S.HeadSize = 1.08;
			S.Coat = Tabby;
			S.Stripe = TabbyStripe;
			S.bWhiteChin = true;
			S.TailThick = 3.0;
			S.bSleepy = true;
			S.Nose = FLinearColor(0.40f, 0.22f, 0.18f);
			break;
		case EKind::TabbyCat:
			S.bStriped = true;
			S.Coat = FLinearColor(0.22f, 0.20f, 0.17f);
			S.Stripe = FLinearColor(0.06f, 0.055f, 0.05f);
			break;
		case EKind::BlackCat:
			S.Coat = FLinearColor(0.025f, 0.022f, 0.022f);
			S.Stripe = S.Coat;
			S.bWhiteChin = true;
			S.Eye = FLinearColor(0.60f, 0.45f, 0.05f);
			S.Nose = FLinearColor(0.05f, 0.04f, 0.04f);
			break;
		case EKind::GingerCat:
			S.bStriped = true;
			S.Coat = FLinearColor(0.50f, 0.22f, 0.06f);
			S.Stripe = FLinearColor(0.30f, 0.11f, 0.03f);
			S.bWhiteBelly = true;
			S.Eye = FLinearColor(0.55f, 0.40f, 0.06f);
			break;
		case EKind::Mutt:
			// A rough-coated street dog, brown with a grey muzzle, ears hanging.
			S.bDog = true;
			S.Length = 62.0;
			S.Back = 52.0;
			S.Girth = 1.3;
			S.HeadSize = 1.5;
			S.Muzzle = 9.0;
			S.bHangingEars = true;
			S.TailLength = 34.0;
			S.TailThick = 2.6;
			S.Coat = FLinearColor(0.20f, 0.13f, 0.07f);
			S.Stripe = S.Coat;
			S.bWhiteChin = true;
			S.White = FLinearColor(0.42f, 0.38f, 0.33f);
			S.Eye = FLinearColor(0.20f, 0.10f, 0.03f);
			S.Nose = FLinearColor(0.02f, 0.02f, 0.02f);
			break;
		case EKind::Puli:
			// The Hungarian herding dog: black cords hanging to the ground, eyes hidden.
			S.bDog = true;
			S.Length = 50.0;
			S.Back = 42.0;
			S.Girth = 1.45;
			S.HeadSize = 1.45;
			S.Muzzle = 5.0;
			S.bHangingEars = true;
			S.TailLength = 24.0;
			S.TailThick = 4.0;
			S.bCorded = true;
			S.Coat = FLinearColor(0.03f, 0.028f, 0.027f);
			S.Stripe = S.Coat;
			S.Eye = FLinearColor(0.10f, 0.06f, 0.03f);
			S.Nose = FLinearColor(0.01f, 0.01f, 0.01f);
			break;
		default:
			break;
		}
		return S;
	}

	/** The body's measures for a shape, used to place the pivots. */
	struct FFrame
	{
		double Rump;      // X of the rump
		double Chest;     // X of the chest
		double BodyZ;     // the middle of the body above the ground
		double Radius;    // the body's half height
		double Leg;       // shoulder to ground
		FVector Neck;     // the head's pivot
		FVector TailRoot; // the tail's pivot
		FVector Shoulder; // the right front leg's pivot (left is mirrored)
		FVector Hip;      // the right hind leg's pivot
	};

	inline FFrame FrameOf(const FShape& S)
	{
		FFrame F;
		F.Radius = 0.21 * S.Length * S.Girth * (S.bDog ? 0.8 : 1.0);
		F.BodyZ = S.Back - F.Radius;
		F.Rump = -0.5 * S.Length;
		F.Chest = 0.5 * S.Length;
		F.Leg = F.BodyZ;
		F.Neck = FVector(F.Chest - 0.15 * S.Length, 0.0, F.BodyZ + 0.6 * F.Radius);
		F.TailRoot = FVector(F.Rump + 1.0, 0.0, F.BodyZ + 0.45 * F.Radius);
		F.Shoulder = FVector(F.Chest - 0.2 * S.Length, 0.45 * F.Radius, F.BodyZ - 0.1 * F.Radius);
		F.Hip = FVector(F.Rump + 0.17 * S.Length, 0.5 * F.Radius, F.BodyZ - 0.1 * F.Radius);
		return F;
	}

	inline void Build(FAnimal& Out, EKind Kind)
	{
		using namespace PersonKit;
		const FShape S = ShapeOf(Kind);
		const FFrame F = FrameOf(S);
		const double R = F.Radius;
		const FLinearColor Dark(0.015f, 0.012f, 0.01f);
		const FLinearColor Pink(0.75f, 0.45f, 0.45f);
		auto AlongX = [](double X, double Z, double RadiusY, double RadiusZ)
		{
			return FRing{ FVector(X, 0.0, Z), FVector(0.0, RadiusY, 0.0), FVector(0.0, 0.0, RadiusZ) };
		};
		const FLinearColor Belly = S.bWhiteBelly ? S.White : S.Coat;

		// The body: a long egg, deeper at the chest, a little higher at the rump.
		FPiece& B = Out.Parts[Body];
		Loft(B, Belly, {
			AlongX(F.Rump, F.BodyZ + 0.1 * R, 0.55 * R, 0.6 * R),
			AlongX(F.Rump + 0.15 * S.Length, F.BodyZ, 0.95 * R, 0.95 * R),
			AlongX(0.0, F.BodyZ - 0.05 * R, 0.95 * R, R),
			AlongX(F.Chest - 0.2 * S.Length, F.BodyZ, 0.9 * R, 1.05 * R),
			AlongX(F.Chest, F.BodyZ + 0.1 * R, 0.6 * R, 0.8 * R),
		}, 14, true, true);
		// The coat over the back and sides (all over unless the belly is white).
		if (S.bWhiteBelly)
		{
			Loft(B, S.Coat, {
				{ FVector(F.Rump - 0.4, 0.0, F.BodyZ + 0.25 * R), FVector(0.0, 0.58 * R, 0.0), FVector(0.0, 0.0, 0.5 * R) },
				{ FVector(F.Rump + 0.15 * S.Length, 0.0, F.BodyZ + 0.2 * R), FVector(0.0, 0.98 * R, 0.0), FVector(0.0, 0.0, 0.8 * R) },
				{ FVector(0.0, 0.0, F.BodyZ + 0.2 * R), FVector(0.0, 0.98 * R, 0.0), FVector(0.0, 0.0, 0.82 * R) },
				{ FVector(F.Chest - 0.25 * S.Length, 0.0, F.BodyZ + 0.3 * R), FVector(0.0, 0.88 * R, 0.0), FVector(0.0, 0.0, 0.72 * R) },
			}, 14, true, true);
		}
		// Tabby stripes: thin dark bands over the back, down the sides.
		if (S.bStriped)
		{
			for (int32 k = 0; k < 7; ++k)
			{
				const double X = F.Rump + (0.12 + 0.11 * k) * S.Length;
				// A thin band over the back and down the sides, just proud of the coat.
				Blob(B, S.Stripe, FVector(X, 0.0, F.BodyZ + (S.bWhiteBelly ? 0.2 * R : 0.0)), FVector(0.06 * S.Length, 1.0 * R, (S.bWhiteBelly ? 0.84 : 1.04) * R), 14, 6, S.bWhiteBelly ? 0.2 : -0.3);
			}
		}
		if (S.bCorded)
		{
			// The puli's cords: hanging all round the body down to near the ground.
			for (int32 k = 0; k < 22; ++k)
			{
				const double X = F.Rump + (k % 11 + 0.5) / 11.0 * S.Length;
				for (const double Side : { -1.0, 1.0 })
				{
					const double Y = Side * (0.75 + 0.2 * (k / 11)) * R;
					Rod(B, S.Coat, FVector(X, Y, F.BodyZ + 0.6 * R), FVector(X + 1.5, Y * 1.25, 4.0 + 3.0 * (k % 3)), 2.0 + 0.4 * (k % 2), 5);
				}
			}
		}

		// The head with its neck, from the neck's pivot.
		FPiece& H = Out.Parts[Head];
		H.Pivot = F.Neck;
		const double HS = (S.bDog ? 6.0 : 6.8) * S.HeadSize;
		const FVector HeadAt = F.Neck + FVector(0.55 * HS, 0.0, 1.3 * HS);
		Loft(H, S.Coat, {
			AlongX(F.Neck.X - 0.6 * HS, F.Neck.Z - 0.5 * HS, 0.7 * R, 0.6 * R),
			AlongX(HeadAt.X - 0.4 * HS, HeadAt.Z - 0.4 * HS, 0.65 * HS, 0.7 * HS),
		}, 12, false, false);
		Blob(H, S.Coat, HeadAt, FVector(1.0 * HS, 0.95 * HS, 0.85 * HS), 14, 8);
		const FVector MuzzleAt = HeadAt + FVector(0.8 * HS + 0.4 * S.Muzzle, 0.0, -0.3 * HS);
		const FLinearColor Face = S.bWhiteBelly || S.bWhiteChin ? S.White : S.Coat;
		if (S.bDog)
		{
			Loft(H, S.Coat, {
				AlongX(HeadAt.X + 0.6 * HS, HeadAt.Z - 0.15 * HS, 0.55 * HS, 0.5 * HS),
				AlongX(MuzzleAt.X + 0.4 * S.Muzzle, MuzzleAt.Z, 0.32 * HS, 0.32 * HS),
			}, 10, false, true);
			Blob(H, Face, MuzzleAt + FVector(0.2 * S.Muzzle, 0.0, -0.25 * HS), FVector(0.4 * S.Muzzle, 0.3 * HS, 0.18 * HS), 8, 4);
			Blob(H, S.Nose, FVector(MuzzleAt.X + 0.4 * S.Muzzle + 0.4, 0.0, MuzzleAt.Z + 0.12 * HS), FVector(0.12 * HS, 0.16 * HS, 0.11 * HS), 8, 4);
		}
		else
		{
			Blob(H, Face, MuzzleAt, FVector(0.45 * HS, 0.55 * HS, 0.4 * HS), 10, 5);
			Blob(H, S.Nose, MuzzleAt + FVector(0.42 * HS, 0.0, 0.25 * HS), FVector(0.1 * HS, 0.14 * HS, 0.08 * HS), 6, 3);
		}
		if (S.bWhiteBelly)
		{
			// Lafi's white blaze, from the nose up between the eyes, and the white chin and chest.
			Blob(H, S.White, HeadAt + FVector(0.78 * HS, 0.0, 0.05 * HS), FVector(0.28 * HS, 0.22 * HS, 0.55 * HS), 8, 4);
			Blob(B, S.White, FVector(F.Chest - 0.02 * S.Length, 0.0, F.BodyZ + 0.1 * R), FVector(0.35 * R, 0.62 * R, 0.85 * R), 10, 5);
		}
		else if (S.bWhiteChin)
		{
			Blob(H, S.White, MuzzleAt + FVector(-0.1 * HS, 0.0, -0.32 * HS), FVector(0.3 * HS, 0.32 * HS, 0.14 * HS), 8, 4);
			Blob(B, S.White, FVector(F.Chest - 0.04 * S.Length, 0.0, F.BodyZ + 0.25 * R), FVector(0.2 * R, 0.22 * R, 0.25 * R), 8, 4);
		}
		if (S.bStriped)
		{
			// The "M" on a tabby's brow: three dark marks.
			for (const double Y : { -0.25, 0.0, 0.25 })
			{
				Block(H, S.Stripe, HeadAt + FVector(0.6 * HS, Y * HS, 0.5 * HS), FVector(0.12 * HS, 0.05 * HS, 0.22 * HS));
			}
		}
		// Eyes: open and round, or sleepy slits; a puli's are lost in its cords.
		if (!S.bCorded)
		{
			for (const double Side : { -1.0, 1.0 })
			{
				const FVector EyeAt = HeadAt + FVector(0.82 * HS, 0.38 * HS * Side, 0.12 * HS);
				const double Open = S.bSleepy ? 0.35 : 1.0;
				Blob(H, S.Eye, EyeAt, FVector(0.1 * HS, 0.17 * HS, 0.17 * HS * Open), 8, 4);
				Blob(H, Dark, EyeAt + FVector(0.06 * HS, 0.0, 0.0), FVector(0.06 * HS, S.bDog ? 0.09 * HS : 0.04 * HS, 0.14 * HS * Open), 6, 3);
				if (S.bSleepy)
				{
					// Heavy lids over the top of the eye.
					Blob(H, S.Coat, EyeAt + FVector(0.02 * HS, 0.0, 0.08 * HS), FVector(0.12 * HS, 0.2 * HS, 0.09 * HS), 8, 4);
				}
			}
		}
		// Ears: upright triangles with pink inside, or hanging flaps.
		for (const double Side : { -1.0, 1.0 })
		{
			if (S.bHangingEars)
			{
				const FVector Root = HeadAt + FVector(0.1 * HS, 0.75 * HS * Side, 0.55 * HS);
				Loft(H, S.bCorded ? S.Coat : !S.bStriped ? FLinearColor(S.Coat * 0.7f) : S.Stripe, {
					{ Root, FVector(0.45 * HS, 0.0, 0.0), FVector(0.0, 0.12 * HS * Side, 0.0) },
					{ Root + FVector(0.1 * HS, 0.35 * HS * Side, -0.8 * HS), FVector(0.35 * HS, 0.0, 0.0), FVector(0.0, 0.1 * HS * Side, 0.0) },
				}, 8, true, true);
			}
			else
			{
				const FVector Root = HeadAt + FVector(-0.05 * HS, 0.55 * HS * Side, 0.62 * HS);
				const FVector Tip = Root + FVector(0.05 * HS, 0.18 * HS * Side, 0.85 * HS);
				Loft(H, S.Coat, {
					{ Root, FVector(0.35 * HS, 0.0, 0.0), FVector(0.0, 0.14 * HS, 0.0) },
					{ Tip, FVector(0.03 * HS, 0.0, 0.0), FVector(0.0, 0.03 * HS, 0.0) },
				}, 8, true, true);
				Loft(H, Pink, {
					{ Root + FVector(0.13 * HS, 0.0, 0.05 * HS), FVector(0.22 * HS, 0.0, 0.0), FVector(0.0, 0.08 * HS, 0.0) },
					{ Tip + FVector(0.08 * HS, 0.0, -0.18 * HS), FVector(0.03 * HS, 0.0, 0.0), FVector(0.0, 0.02 * HS, 0.0) },
				}, 6, true, true);
			}
		}
		if (S.bCorded)
		{
			// Cords falling from the crown over the face.
			for (int32 k = 0; k < 9; ++k)
			{
				const double A = -1.2 + k * 0.3;
				const FVector From = HeadAt + FVector(0.3 * HS, FMath::Sin(A) * 0.7 * HS, 0.7 * HS);
				Rod(H, S.Coat, From, From + FVector(0.55 * HS, FMath::Sin(A) * 0.3 * HS, -1.1 * HS), 0.28 * HS, 5);
			}
		}

		// The tail, from its root, back and a little down, curving up at the end (a dog's curls up).
		FPiece& T = Out.Parts[Tail];
		T.Pivot = F.TailRoot;
		TArray<FRing> TailRings;
		const int32 Joints = 6;
		for (int32 j = 0; j <= Joints; ++j)
		{
			const double U = static_cast<double>(j) / Joints;
			const double X = F.TailRoot.X - U * S.TailLength * (S.bDog ? 0.7 : 0.95);
			const double Z = F.TailRoot.Z + (S.bDog ? U * 0.55 * S.TailLength : -U * 0.35 * S.TailLength + U * U * 0.2 * S.TailLength);
			const double Thick = S.TailThick * (1.0 - 0.35 * U) * (S.bCorded ? 1.6 : 1.0);
			TailRings.Add({ FVector(X, 0.0, Z), FVector(0.0, Thick, 0.0), FVector(0.0, 0.0, Thick) });
		}
		Loft(T, S.Coat, TailRings, 8, true, true);
		if (S.bStriped)
		{
			for (int32 k = 1; k < Joints; k += 1)
			{
				const FRing& Ring = TailRings[k];
				Loft(T, S.Stripe, {
					{ Ring.Center + FVector(0.6, 0.0, 0.0), Ring.U * 1.08, Ring.V * 1.08 },
					{ Ring.Center - FVector(0.6, 0.0, 0.0), Ring.U * 1.08, Ring.V * 1.08 },
				}, 8, false, false);
			}
		}

		// The legs, each from its shoulder or hip down to the paw.
		for (int32 L = FrontLeft; L <= HindRight; ++L)
		{
			const bool bFront = L == FrontLeft || L == FrontRight;
			const double Side = (L == FrontLeft || L == HindLeft) ? -1.0 : 1.0;
			FPiece& P = Out.Parts[L];
			const FVector Top = (bFront ? F.Shoulder : F.Hip) * FVector(1.0, Side, 1.0);
			P.Pivot = Top;
			const double W = (S.bDog ? 0.1 : 0.1) * S.Length * (0.75 + 0.25 * S.Girth);
			const FLinearColor LegColor = S.bWhiteBelly ? S.White : S.Coat;
			const double Paw = 1.3 * W * 0.45;
			if (bFront)
			{
				Loft(P, LegColor, {
					{ Top, FVector(0.0, 0.6 * W, 0.0), FVector(0.75 * W, 0.0, 0.0) },
					{ FVector(Top.X + 0.5, Top.Y, 0.5 * Top.Z), FVector(0.0, 0.42 * W, 0.0), FVector(0.45 * W, 0.0, 0.0) },
					{ FVector(Top.X + 0.8, Top.Y, Paw), FVector(0.0, 0.38 * W, 0.0), FVector(0.4 * W, 0.0, 0.0) },
				}, 8, false, false);
			}
			else
			{
				// The hind leg: a thick thigh, the hock bent back, then down to the paw.
				const FVector Hock(Top.X - 0.22 * Top.Z, Top.Y, 0.32 * Top.Z);
				Loft(P, S.Coat, {
					{ Top + FVector(0.0, 0.0, 0.1 * W), FVector(0.0, 0.7 * W, 0.0), FVector(1.1 * W, 0.0, 0.0) },
					{ FVector(Top.X - 0.05 * Top.Z, Top.Y, 0.6 * Top.Z), FVector(0.0, 0.55 * W, 0.0), FVector(0.75 * W, 0.0, 0.0) },
					{ Hock, FVector(0.0, 0.38 * W, 0.0), FVector(0.4 * W, 0.0, 0.0) },
				}, 8, true, false);
				Loft(P, LegColor, {
					{ Hock, FVector(0.0, 0.38 * W, 0.0), FVector(0.4 * W, 0.0, 0.0) },
					{ FVector(Hock.X + 0.1 * Top.Z, Top.Y, Paw), FVector(0.0, 0.36 * W, 0.0), FVector(0.38 * W, 0.0, 0.0) },
				}, 8, false, false);
			}
			const double PawX = bFront ? Top.X + 0.8 + 0.25 * W : Top.X - 0.12 * Top.Z + 0.25 * W;
			Blob(P, LegColor, FVector(PawX, Top.Y, Paw), FVector(0.65 * W, 0.42 * W, Paw), 8, 4, -0.2);
			if (S.bStriped && !S.bWhiteBelly)
			{
				for (int32 k = 1; k <= 3; ++k)
				{
					const double Z = Top.Z * (0.15 + 0.17 * k);
					Block(P, S.Stripe, FVector(Top.X + (bFront ? 0.6 : -0.1 * Top.Z), Top.Y, Z), FVector(0.35 * W, 0.43 * W, 0.22));
				}
			}
		}
	}

	/** A turn of one piece about its pivot (degrees, as an FRotator's pitch, yaw and roll). */
	struct FTurn
	{
		double Pitch = 0.0;
		double Yaw = 0.0;
		double Roll = 0.0;
	};

	/** The whole pose: the body's turn about the hip and how far it drops, and each piece's own turn. */
	struct FPose
	{
		FTurn BodyTurn;
		double Drop = 0.0;
		FTurn Parts[NumParts];
	};

	/** The body turns about this point (the middle of the hips). */
	inline FVector BodyPivot(EKind Kind)
	{
		const FFrame F = FrameOf(ShapeOf(Kind));
		return FVector(F.Hip.X, 0.0, F.Hip.Z);
	}

	/** A pose; Phase (0..1) runs the walk, and turns the head a little in the others. */
	inline FPose Pose(EKind Kind, EPose Which, double Phase)
	{
		const FShape S = ShapeOf(Kind);
		const FFrame F = FrameOf(S);
		FPose P;
		const double Swing = FMath::Sin(2.0 * PI * Phase);
		switch (Which)
		{
		case EPose::Walk:
		{
			// Diagonal pairs: front left with hind right.
			const double Step = 24.0;
			P.Parts[FrontLeft].Pitch = Step * Swing;
			P.Parts[HindRight].Pitch = Step * Swing;
			P.Parts[FrontRight].Pitch = -Step * Swing;
			P.Parts[HindLeft].Pitch = -Step * Swing;
			P.Parts[Head].Pitch = 2.0 * FMath::Sin(4.0 * PI * Phase);
			P.Parts[Tail].Yaw = 8.0 * Swing;
			P.Drop = 0.5 * (1.0 - FMath::Cos(4.0 * PI * Phase));
			break;
		}
		case EPose::Run:
		{
			// A bounding run: the front legs together, then the hind ones, the body rocking.
			const double Reach = 42.0;
			const double Hind = FMath::Sin(2.0 * PI * Phase + 1.6);
			P.Parts[FrontLeft].Pitch = Reach * Swing;
			P.Parts[FrontRight].Pitch = Reach * Swing * 0.85;
			P.Parts[HindLeft].Pitch = Reach * Hind;
			P.Parts[HindRight].Pitch = Reach * Hind * 0.85;
			P.BodyTurn.Pitch = 7.0 * FMath::Sin(2.0 * PI * Phase + 0.8);
			P.Parts[Head].Pitch = -6.0 - P.BodyTurn.Pitch;
			// The tail streams out behind, a little up.
			P.Parts[Tail].Pitch = S.bDog ? 10.0 : -18.0;
			P.Parts[Tail].Yaw = 5.0 * Swing;
			P.Drop = 0.08 * F.Leg * (1.0 - FMath::Cos(4.0 * PI * Phase));
			break;
		}
		case EPose::Sit:
		{
			// Rump down, front up on straight legs, hind legs folded forward, tail round the side.
			P.BodyTurn.Pitch = S.bDog ? 32.0 : 34.0;
			P.Drop = F.Hip.Z - 0.3 * F.Leg;
			P.Parts[FrontLeft].Pitch = -P.BodyTurn.Pitch;
			P.Parts[FrontRight].Pitch = -P.BodyTurn.Pitch;
			P.Parts[HindLeft].Pitch = -P.BodyTurn.Pitch + 75.0;
			P.Parts[HindRight].Pitch = -P.BodyTurn.Pitch + 75.0;
			P.Parts[Head].Pitch = -P.BodyTurn.Pitch + 4.0;
			P.Parts[Head].Yaw = 10.0 * Swing;
			P.Parts[Tail].Pitch = -P.BodyTurn.Pitch + 25.0;
			P.Parts[Tail].Yaw = 70.0;
			break;
		}
		case EPose::Lie:
		{
			// On the belly, front legs stretched out ahead, hind legs tucked under, head up.
			P.Drop = F.BodyZ - 0.85 * F.Radius;
			P.Parts[FrontLeft].Pitch = 80.0;
			P.Parts[FrontRight].Pitch = 76.0;
			P.Parts[FrontLeft].Yaw = 6.0;
			P.Parts[FrontRight].Yaw = -6.0;
			P.Parts[HindLeft].Pitch = 85.0;
			P.Parts[HindRight].Pitch = 85.0;
			P.Parts[HindLeft].Yaw = -25.0;
			P.Parts[HindRight].Yaw = 25.0;
			P.Parts[Head].Pitch = -6.0;
			P.Parts[Head].Yaw = 8.0 * Swing;
			P.Parts[Tail].Pitch = 15.0;
			P.Parts[Tail].Yaw = 35.0;
			break;
		}
		case EPose::Lap:
		{
			// Crouched over a puddle, lapping: front low, front legs splayed forward, hind legs folded, head down and bobbing.
			P.BodyTurn.Pitch = -14.0;
			P.Drop = 0.28 * F.Leg;
			P.Parts[FrontLeft].Pitch = 38.0;
			P.Parts[FrontRight].Pitch = 34.0;
			P.Parts[FrontLeft].Yaw = 8.0;
			P.Parts[FrontRight].Yaw = -8.0;
			P.Parts[HindLeft].Pitch = 62.0;
			P.Parts[HindRight].Pitch = 62.0;
			P.Parts[Head].Pitch = -38.0 - 6.0 * FMath::Max(0.0, FMath::Sin(8.0 * PI * Phase));
			P.Parts[Tail].Pitch = 20.0;
			P.Parts[Tail].Yaw = 25.0 * Swing;
			break;
		}
		default:
			P.Parts[Head].Yaw = 12.0 * Swing;
			P.Parts[Tail].Yaw = 10.0 * Swing;
			break;
		}
		return P;
	}
}
