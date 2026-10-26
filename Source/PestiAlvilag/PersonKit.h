// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * A dress-up kit for the people of 1932 Pest: one smooth, rounded body
 * (a man or a woman, any height and build) and a wardrobe of pieces to put
 * on it: hats, coats and jackets, a dress or trousers, shoes, a tie, a
 * scarf, a sash, a moustache, and something to carry. An outfit lists the pieces
 * and their colours; Build turns it into meshes, so every walker can be
 * dressed differently without anyone modelling him by hand.
 *
 * The person comes in nine pieces that move: the body (with the head and
 * everything worn on it), each arm, which hangs from the shoulder, each leg
 * in two: the thigh, which hangs from the hip, and the shin and foot, which
 * hang from the knee (ShinOf, KneeBend) so the knee can bend; and the two
 * halves of a coat's or dress's skirts, which hang from the hips and follow
 * the thighs part of the way (SkirtFollow). Pieces with nothing on them
 * (the skirts of a man in a waistcoat) are empty. Each piece is in centimetres
 * with its pivot at the origin, X forward, Y to the right, Z up; the feet
 * stand at Z = 0 in the body's frame.
 */
namespace PersonKit
{
	enum class EHat : uint8 { None, Bowler, Fedora, FlatCap, Boater, Cloche, Hood };
	enum class ETop : uint8 { Shirt, Waistcoat, Jacket, Overcoat, Dress };
	enum class EHeld : uint8 { None, Briefcase, Umbrella, Newspaper, Basket };

	struct FOutfit
	{
		bool bWoman = false;
		/** From the soles to the top of the head (cm). */
		float Height = 178.f;
		/** 1 is an average build; up to 1.25 for a stout one. */
		float Build = 1.f;

		EHat Hat = EHat::Fedora;
		ETop Top = ETop::Overcoat;
		bool bTie = true;
		bool bScarf = false;
		bool bMoustache = false;
		bool bApron = false;
		/** A cloth sash wound round the waist. */
		bool bSash = false;
		bool bBoots = false;
		EHeld Held = EHeld::None;
		/**
		 * Sitting on a chair: thighs forward, shins down, a lap instead of
		 * hanging skirts. The feet then rest SeatDrop above the body's
		 * ground, so set a seated person down by that much.
		 */
		bool bSeated = false;

		FLinearColor Skin = FLinearColor(0.72f, 0.50f, 0.38f);
		FLinearColor Hair = FLinearColor(0.10f, 0.07f, 0.05f);
		FLinearColor HatColor = FLinearColor(0.06f, 0.06f, 0.06f);
		FLinearColor HatBand = FLinearColor(0.02f, 0.02f, 0.02f);
		FLinearColor Coat = FLinearColor(0.12f, 0.10f, 0.08f);
		FLinearColor Shirt = FLinearColor(0.85f, 0.84f, 0.80f);
		FLinearColor Tie = FLinearColor(0.30f, 0.06f, 0.05f);
		FLinearColor Waistcoat = FLinearColor(0.08f, 0.08f, 0.09f);
		FLinearColor Trousers = FLinearColor(0.06f, 0.06f, 0.07f);
		FLinearColor Shoes = FLinearColor(0.03f, 0.02f, 0.02f);
		FLinearColor Scarf = FLinearColor(0.35f, 0.30f, 0.20f);
		FLinearColor Apron = FLinearColor(0.90f, 0.90f, 0.88f);
		FLinearColor Sash = FLinearColor(0.45f, 0.06f, 0.05f);
		FLinearColor HeldColor = FLinearColor(0.25f, 0.13f, 0.06f);
	};

	/** How far a seated person's feet are above the standing person's ground (cm). */
	constexpr double SeatDrop = 44.0;

	enum EPiece : int32 { LegLeft, LegRight, ArmLeft, ArmRight, Body, ShinLeft, ShinRight, SkirtLeft, SkirtRight, NumPieces };

	/**
	 * Below the hips, a long coat's or dress's skirts hang in two flaps,
	 * hinged inside the rigid top of the skirts so they never come away from
	 * the body, and each flap follows its thigh by this share of the thigh's
	 * swing: the hem parts as he strides, as cloth would.
	 */
	constexpr float SkirtFollow = 0.5f;

	/** The piece a piece hangs from: a shin from its thigh, anything else from the body (NumPieces). */
	inline EPiece ParentOf(int32 Piece)
	{
		return Piece == ShinLeft ? LegLeft : Piece == ShinRight ? LegRight : NumPieces;
	}

	/** The shin under a thigh (LegLeft or LegRight). */
	inline EPiece ShinOf(int32 Leg)
	{
		return Leg == LegLeft ? ShinLeft : ShinRight;
	}

	/** The half of the skirts over a thigh (LegLeft or LegRight). */
	inline EPiece SkirtOf(int32 Leg)
	{
		return Leg == LegLeft ? SkirtLeft : SkirtRight;
	}

	/**
	 * How far the knee bends (degrees, negative: the foot goes back) at this
	 * point of a stride whose thigh swings by Sin(Phase): most while the leg
	 * swings forward, a little while it bears the weight. Amount is 0 when
	 * standing and 1 at a brisk walk; above that, at a run, it bends more.
	 */
	inline float KneeBend(float Phase, float Amount)
	{
		const float Swinging = FMath::Max(0.f, static_cast<float>(FMath::Cos(Phase)));
		return -(6.f + 42.f * Swinging * Swinging) * Amount;
	}

	/** One colour of one piece: a smooth-shaded triangle list. */
	struct FSection
	{
		FLinearColor Color;
		TArray<FVector> Vertices;
		TArray<FVector> Normals;
		TArray<int32> Triangles;
	};

	struct FPiece
	{
		/** Where the piece hangs from, in the body's frame (shoulder, hip, or the feet for the body). */
		FVector Pivot = FVector::ZeroVector;
		TArray<FSection> Sections;

		FSection& Section(const FLinearColor& Color)
		{
			for (int32 i = 0; i < Sections.Num(); ++i)
			{
				if (Sections[i].Color == Color)
				{
					return Sections[i];
				}
			}
			FSection Fresh;
			Fresh.Color = Color;
			return Sections[Sections.Add(Fresh)];
		}
	};

	struct FPerson
	{
		FPiece Pieces[NumPieces];
	};

	/** One cross-section of a lofted shape: its centre and two half-axes. */
	struct FRing
	{
		FVector Center;
		FVector U;
		FVector V;
	};

	inline FRing Flat(double X, double Z, double RadiusX, double RadiusY, double Y = 0.0)
	{
		return { FVector(X, Y, Z), FVector(RadiusX, 0.0, 0.0), FVector(0.0, RadiusY, 0.0) };
	}

	/**
	 * Skins a row of rings into a smooth tube, with optional caps at either
	 * end. The rings may go round either way: the faces are turned to face
	 * outward from the way the row runs.
	 */
	inline void Loft(FPiece& Piece, const FLinearColor& Color, const TArray<FRing>& Rings, int32 Sides, bool bCapStart, bool bCapEnd, double Phase = 0.0)
	{
		if (Rings.Num() < 2)
		{
			return;
		}
		FSection& S = Piece.Section(Color);
		const int32 First = S.Vertices.Num();
		const FVector Offset = -Piece.Pivot;
		const FVector Travel = Rings[Rings.Num() - 1].Center - Rings[0].Center;
		const bool bFlip = FVector::DotProduct(FVector::CrossProduct(Rings[0].U, Rings[0].V), Travel) < 0.0;
		for (int32 r = 0; r < Rings.Num(); ++r)
		{
			for (int32 i = 0; i < Sides; ++i)
			{
				const double A = Phase + 2.0 * PI * i / Sides;
				S.Vertices.Add(Rings[r].Center + Rings[r].U * FMath::Cos(A) + Rings[r].V * FMath::Sin(A) + Offset);
				S.Normals.Add(FVector::ZeroVector);
			}
		}
		auto Tri = [&S, bFlip](int32 A, int32 B, int32 C)
		{
			if (bFlip)
			{
				Swap(B, C);
			}
			// Unreal draws a triangle facing (C - A) x (B - A).
			S.Triangles.Add(A);
			S.Triangles.Add(B);
			S.Triangles.Add(C);
			const FVector N = FVector::CrossProduct(S.Vertices[C] - S.Vertices[A], S.Vertices[B] - S.Vertices[A]);
			S.Normals[A] += N;
			S.Normals[B] += N;
			S.Normals[C] += N;
		};
		for (int32 r = 0; r + 1 < Rings.Num(); ++r)
		{
			for (int32 i = 0; i < Sides; ++i)
			{
				const int32 A = First + r * Sides + i;
				const int32 B = First + r * Sides + (i + 1) % Sides;
				const int32 C = A + Sides;
				const int32 D = B + Sides;
				Tri(A, C, B);
				Tri(B, C, D);
			}
		}
		for (int32 i = First; i < S.Vertices.Num(); ++i)
		{
			S.Normals[i] = S.Normals[i].GetSafeNormal();
		}

		// Flat caps, with their own vertices so the edge stays crisp.
		auto Cap = [&S, &Rings, Sides, Phase, Offset](int32 Ring, bool bAlongUV)
		{
			const FRing& R = Rings[Ring];
			const FVector Axis = FVector::CrossProduct(R.U, R.V).GetSafeNormal() * (bAlongUV ? 1.0 : -1.0);
			const int32 Middle = S.Vertices.Add(R.Center + Offset);
			S.Normals.Add(Axis);
			for (int32 i = 0; i < Sides; ++i)
			{
				const double A = Phase + 2.0 * PI * i / Sides;
				S.Vertices.Add(R.Center + R.U * FMath::Cos(A) + R.V * FMath::Sin(A) + Offset);
				S.Normals.Add(Axis);
			}
			for (int32 i = 0; i < Sides; ++i)
			{
				const int32 A = Middle + 1 + i;
				const int32 B = Middle + 1 + (i + 1) % Sides;
				S.Triangles.Add(Middle);
				S.Triangles.Add(bAlongUV ? B : A);
				S.Triangles.Add(bAlongUV ? A : B);
			}
		};
		if (bCapStart)
		{
			Cap(0, bFlip);
		}
		if (bCapEnd)
		{
			Cap(Rings.Num() - 1, !bFlip);
		}
	}

	/** A smooth egg shape: centre and half-sizes along X, Y and Z. */
	inline void Blob(FPiece& Piece, const FLinearColor& Color, const FVector& Center, const FVector& Radius, int32 Sides = 10, int32 Bands = 7, double FromZ = -1.0)
	{
		TArray<FRing> Rings;
		const double Start = FMath::Clamp(FromZ, -1.0, 1.0);
		for (int32 b = 0; b <= Bands; ++b)
		{
			// From Start (bottom) to the top pole, spaced by angle.
			const double T0 = FMath::Asin(Start);
			const double T = T0 + (PI * 0.5 - T0) * b / Bands;
			const double Ring = FMath::Max(FMath::Cos(T), 0.02);
			Rings.Add({ Center + FVector(0.0, 0.0, Radius.Z * FMath::Sin(T)), FVector(Radius.X * Ring, 0.0, 0.0), FVector(0.0, Radius.Y * Ring, 0.0) });
		}
		Loft(Piece, Color, Rings, Sides, Start > -0.99, false);
	}

	/** A box from a loft of four sides: centre, half-sizes. */
	inline void Block(FPiece& Piece, const FLinearColor& Color, const FVector& Center, const FVector& Half)
	{
		const double R = FMath::Sqrt(2.0);
		TArray<FRing> Rings = {
			{ Center - FVector(0.0, 0.0, Half.Z), FVector(Half.X * R, 0.0, 0.0), FVector(0.0, Half.Y * R, 0.0) },
			{ Center + FVector(0.0, 0.0, Half.Z), FVector(Half.X * R, 0.0, 0.0), FVector(0.0, Half.Y * R, 0.0) },
		};
		Loft(Piece, Color, Rings, 4, true, true, PI * 0.25);
	}

	/** A rod from A to B (a cane, a handle, a strap). */
	inline void Rod(FPiece& Piece, const FLinearColor& Color, const FVector& A, const FVector& B, double Radius, int32 Sides = 6)
	{
		const FVector Along = (B - A).GetSafeNormal();
		const FVector Side = FMath::Abs(Along.Z) < 0.9 ? FVector::CrossProduct(Along, FVector::UpVector).GetSafeNormal() : FVector::RightVector;
		const FVector Other = FVector::CrossProduct(Side, Along);
		TArray<FRing> Rings = { { A, Side * Radius, Other * Radius }, { B, Side * Radius, Other * Radius } };
		Loft(Piece, Color, Rings, Sides, true, true);
	}

	/**
	 * Dresses a person. Sizes are for a man of 178 cm; everything is scaled
	 * to the outfit's height, and the girth to its build.
	 */
	inline void Build(const FOutfit& O, FPerson& Out)
	{
		const double H = O.Height / 178.0;
		const double G = O.Build;
		const bool W = O.bWoman;
		auto Z = [H](double Cm) { return Cm * H; };

		const FVector ShoulderAt(0.0, 19.5 * G * (W ? 0.86 : 1.0), Z(138.0));
		const FVector HipAt(0.0, 9.5 * G, Z(92.0));
		Out.Pieces[Body].Pivot = FVector::ZeroVector;
		Out.Pieces[LegLeft].Pivot = FVector(HipAt.X, -HipAt.Y, HipAt.Z);
		Out.Pieces[LegRight].Pivot = HipAt;
		Out.Pieces[ArmLeft].Pivot = FVector(ShoulderAt.X, -ShoulderAt.Y, ShoulderAt.Z);
		Out.Pieces[ArmRight].Pivot = ShoulderAt;

		FPiece& B = Out.Pieces[Body];
		const bool bCoat = O.Top == ETop::Jacket || O.Top == ETop::Overcoat;
		const FLinearColor& TorsoColor = O.Top == ETop::Shirt ? O.Shirt : O.Top == ETop::Waistcoat ? O.Waistcoat : O.Coat;
		const double Thick = O.Top == ETop::Overcoat ? 1.08 : 1.0;

		// The torso: hips, waist, chest and shoulders, up to the neck.
		{
			const double Wx = W ? 0.92 : 1.0;
			TArray<FRing> Rings = {
				Flat(0.0, Z(86.0), 12.0 * G * Thick, (W ? 16.5 : 15.5) * G * Thick),
				Flat(0.0, Z(95.0), 13.0 * G * Thick, (W ? 18.0 : 16.5) * G * Thick),
				Flat(0.5, Z(105.0), (W ? 10.5 : 11.5) * G * G * Thick, (W ? 13.5 : 15.5) * G * G * Thick),
				Flat(1.5, Z(121.0), 12.5 * G * Wx * Thick, (W ? 15.5 : 18.0) * G * Thick),
				Flat(1.0, Z(134.0), 11.5 * Wx * Thick, (W ? 16.0 : 19.0) * G * Thick),
				Flat(0.0, Z(141.0), 9.5 * Wx * Thick, (W ? 14.5 : 18.0) * G * Thick),
				Flat(0.0, Z(145.0), 7.5 * Thick, (W ? 10.5 : 13.0) * Thick),
				Flat(0.0, Z(147.5), 6.0 * Thick, (W ? 7.5 : 8.5) * Thick),
			};
			Loft(B, TorsoColor, Rings, 14, true, true);
		}
		// Neck and head.
		Loft(B, O.Skin, { Flat(0.5, Z(145.0), 5.5, 5.5), Flat(1.0, Z(152.0), 5.0, 5.0), Flat(1.5, Z(156.0), 4.0, 3.8) }, 8, false, false);
		const FVector HeadAt(1.5, 0.0, Z(165.5));
		const double Hr = W ? 0.93 : 1.0;
		Blob(B, O.Skin, HeadAt, FVector(10.0, 8.6, 11.5) * Hr, 12, 8);
		Blob(B, O.Skin, HeadAt + FVector(2.0, 0.0, -5.5 * Hr), FVector(8.0, 7.0, 6.0) * Hr, 10, 5);
		// Nose, ears, eyes, brows and mouth.
		Rod(B, O.Skin, HeadAt + FVector(9.2, 0.0, 0.5) * Hr, HeadAt + FVector(12.3, 0.0, -2.8) * Hr, 1.4, 5);
		for (const double Side : { -1.0, 1.0 })
		{
			Blob(B, O.Skin, HeadAt + FVector(0.0, 8.4 * Side, 0.0) * Hr, FVector(2.0, 1.2, 3.2) * Hr, 6, 4);
			Blob(B, FLinearColor(0.03f, 0.03f, 0.03f), HeadAt + FVector(8.7, 3.3 * Side, 2.0) * Hr, FVector(0.9, 1.1, 0.9), 6, 3);
			Blob(B, O.Hair, HeadAt + FVector(9.0, 3.5 * Side, 4.6) * Hr, FVector(0.8, 2.3, 0.6), 6, 3);
		}
		Blob(B, FLinearColor(0.35f, 0.15f, 0.12f), HeadAt + FVector(10.2, 0.0, -6.0) * Hr, FVector(0.6, 2.4, 0.5), 6, 3);
		if (O.bMoustache && !W)
		{
			Blob(B, O.Hair, HeadAt + FVector(10.8, 0.0, -3.9), FVector(1.0, 3.6, 0.9), 8, 3);
		}
		// Hair: a cap over the top and back, longer at the back for a woman, with a bun.
		Blob(B, O.Hair, HeadAt + FVector(-1.0, 0.0, 1.0), FVector(10.4, 9.2, 11.2) * Hr, 12, 6, W ? -0.55 : -0.2);
		if (W)
		{
			Blob(B, O.Hair, HeadAt + FVector(-9.0, 0.0, -2.0), FVector(4.0, 5.0, 4.0), 8, 4);
		}

		// Shirt front, collar and tie in the open neck of a jacket or waistcoat.
		if (O.Top != ETop::Shirt && O.Top != ETop::Dress)
		{
			const double Front = 12.5 * G * Thick + 0.6;
			Loft(B, O.Shirt, {
				{ FVector(Front - 3.5, 0.0, Z(146.0)), FVector(0.6, 0.0, 0.0), FVector(0.0, 6.0, 0.0) },
				{ FVector(Front - 0.4, 0.0, Z(134.0)), FVector(0.6, 0.0, 0.0), FVector(0.0, 4.5, 0.0) },
				{ FVector(Front, 0.0, Z(123.0)), FVector(0.6, 0.0, 0.0), FVector(0.0, 0.5, 0.0) },
			}, 4, true, true, PI * 0.25);
		}
		if (O.bTie && !W && O.Top != ETop::Dress)
		{
			const double Front = 12.5 * G * Thick + 1.4;
			Loft(B, O.Tie, {
				{ FVector(Front - 3.4, 0.0, Z(146.0)), FVector(0.5, 0.0, 0.0), FVector(0.0, 1.2, 0.0) },
				{ FVector(Front - 0.2, 0.0, Z(132.0)), FVector(0.5, 0.0, 0.0), FVector(0.0, 2.6, 0.0) },
				{ FVector(Front, 0.0, Z(124.0)), FVector(0.5, 0.0, 0.0), FVector(0.0, 0.3, 0.0) },
			}, 4, true, true, PI * 0.25);
		}
		// Coat buttons down the front.
		if (bCoat || O.Top == ETop::Waistcoat)
		{
			const int32 Buttons = O.Top == ETop::Overcoat ? 4 : 3;
			for (int32 i = 0; i < Buttons; ++i)
			{
				const double Bz = Z(120.0 - 9.0 * i);
				Blob(B, FLinearColor(0.02f, 0.02f, 0.02f), FVector(12.0 * G * Thick + 0.4, 0.0, Bz), FVector(0.6, 1.0, 1.0), 6, 3);
			}
		}

		// Below the waist: the skirts of a jacket or overcoat, or a dress.
		const bool bLongSkirts = O.Top == ETop::Overcoat || O.Top == ETop::Dress || W;
		if (O.bSeated && O.Top != ETop::Shirt && O.Top != ETop::Waistcoat)
		{
			// Sitting: the skirts come down to the seat, and the long ones lie over the lap.
			const FLinearColor& SkirtColor = bCoat ? O.Coat : O.Top == ETop::Dress ? O.Coat : O.Trousers;
			Loft(B, SkirtColor, {
				Flat(0.5, Z(104.0), 12.5 * G * G, 16.5 * G * G),
				Flat(0.0, Z(93.0), 15.0 * G, 19.0 * G),
			}, 14, true, true);
			if (bLongSkirts)
			{
				const double LapEnd = 47.0 * H;
				Block(B, SkirtColor, FVector(LapEnd * 0.5 - 4.0, 0.0, Z(92.0) - 1.0), FVector(LapEnd * 0.5 + 4.0, 19.0 * G, 10.0));
			}
		}
		else if (bCoat || O.Top == ETop::Dress || W)
		{
			// The hips and the top of the skirts, rigid with the body and
			// roomy enough for the thighs to swing inside; a jacket ends here,
			// longer skirts go on below in two flaps (see the legs).
			const FLinearColor& HipColor = bCoat || O.Top == ETop::Dress ? O.Coat : O.Trousers;
			const bool bJacket = O.Top == ETop::Jacket;
			const bool bOvercoat = O.Top == ETop::Overcoat;
			Loft(B, HipColor, {
				Flat(0.5, Z(104.0), (bCoat ? 12.5 : 11.0) * G * G, (bCoat ? 16.4 : 14.0) * G * G),
				Flat(0.0, Z(92.0), (bJacket ? 14.0 : 14.6) * G, (bJacket ? 18.0 : 18.6) * G),
				bJacket ? Flat(-0.5, Z(76.0), 14.8 * G, 18.6 * G)
					: Flat(-0.5, Z(bOvercoat ? 74.0 : 72.0), (bOvercoat ? 16.6 : 16.0) * G, (bOvercoat ? 20.4 : 20.0) * G),
			}, 14, true, true);
		}
		if (O.Top == ETop::Overcoat)
		{
			// A wide collar turned down over the shoulders.
			Loft(B, O.Coat, {
				Flat(0.0, Z(140.0), 11.5 * G, 20.5 * G),
				Flat(0.0, Z(146.0), 8.0, 11.5),
			}, 14, false, true);
		}
		if (O.bApron && !O.bSeated)
		{
			Loft(B, O.Apron, {
				{ FVector(13.5 * G, 0.0, Z(104.0)), FVector(0.4, 0.0, 0.0), FVector(0.0, 13.0 * G, 0.0) },
				{ FVector(16.5 * G, 0.0, Z(50.0)), FVector(0.4, 0.0, 0.0), FVector(0.0, 15.0 * G, 0.0) },
			}, 4, true, true, PI * 0.25);
		}
		if (O.bSash && !O.bSeated)
		{
			// Wound round the waist over the coat, its end hanging at the left front.
			Loft(B, O.Sash, {
				Flat(0.5, Z(97.0), 13.8 * G * Thick + 0.8, 17.4 * G * Thick + 0.8),
				Flat(0.5, Z(109.0), 12.6 * G * G * Thick + 0.8, 16.6 * G * G * Thick + 0.8),
			}, 14, true, true);
			Loft(B, O.Sash, {
				{ FVector(13.8 * G * Thick + 1.2, -6.0, Z(99.0)), FVector(0.6, 0.0, 0.0), FVector(0.0, 3.5, 0.0) },
				{ FVector(14.8 * G * Thick + 1.6, -7.5, Z(72.0)), FVector(0.6, 0.0, 0.0), FVector(0.0, 4.0, 0.0) },
			}, 4, true, true, PI * 0.25);
		}
		if (O.bScarf)
		{
			Loft(B, O.Scarf, { Flat(0.5, Z(146.0), 8.0, 8.5), Flat(1.0, Z(152.0), 7.0, 7.0) }, 10, true, true);
			Loft(B, O.Scarf, {
				{ FVector(9.5, 4.0, Z(147.0)), FVector(0.8, 0.0, 0.0), FVector(0.0, 3.5, 0.0) },
				{ FVector(13.5 * G * Thick + 1.5, 5.5, Z(118.0)), FVector(0.8, 0.0, 0.0), FVector(0.0, 3.5, 0.0) },
			}, 4, true, true, PI * 0.25);
		}

		// Hats.
		const FVector Crown = HeadAt + FVector(-0.5, 0.0, 7.5 * Hr);
		switch (O.Hat)
		{
		case EHat::Bowler:
			Blob(B, O.HatColor, Crown, FVector(10.8, 9.8, 9.5), 14, 6, 0.0);
			Loft(B, O.HatBand, { Flat(Crown.X, Crown.Z, 10.9, 9.9), Flat(Crown.X, Crown.Z + 2.0, 10.7, 9.7) }, 14, true, true);
			Loft(B, O.HatColor, { Flat(Crown.X + 0.5, Crown.Z - 1.0, 14.5, 13.0), Flat(Crown.X + 0.5, Crown.Z, 14.5, 13.0) }, 16, true, true);
			break;
		case EHat::Fedora:
			Loft(B, O.HatColor, {
				Flat(Crown.X, Crown.Z, 10.6, 9.6),
				Flat(Crown.X, Crown.Z + 6.0, 10.0, 9.0),
				Flat(Crown.X - 0.5, Crown.Z + 10.0, 8.5, 7.2),
				Flat(Crown.X - 0.5, Crown.Z + 11.0, 6.5, 4.0),
			}, 14, false, true);
			Loft(B, O.HatBand, { Flat(Crown.X, Crown.Z + 0.6, 10.8, 9.8), Flat(Crown.X, Crown.Z + 3.2, 10.6, 9.6) }, 14, true, true);
			Loft(B, O.HatColor, { Flat(Crown.X + 1.0, Crown.Z - 0.6, 16.5, 15.0), Flat(Crown.X + 1.0, Crown.Z + 0.4, 16.5, 15.0) }, 16, true, true);
			break;
		case EHat::FlatCap:
			Blob(B, O.HatColor, Crown + FVector(1.5, 0.0, -1.5), FVector(12.0, 10.5, 5.5), 14, 4, 0.0);
			Loft(B, O.HatColor, {
				{ Crown + FVector(10.0, 0.0, -1.6), FVector(0.0, 0.0, 0.4), FVector(0.0, 7.5, 0.0) },
				{ Crown + FVector(15.5, 0.0, -2.6), FVector(0.0, 0.0, 0.4), FVector(0.0, 6.0, 0.0) },
			}, 8, true, true);
			break;
		case EHat::Boater:
			Loft(B, O.HatColor, { Flat(Crown.X, Crown.Z, 10.5, 9.5), Flat(Crown.X, Crown.Z + 6.0, 10.5, 9.5) }, 14, false, true);
			Loft(B, O.HatBand, { Flat(Crown.X, Crown.Z + 0.5, 10.7, 9.7), Flat(Crown.X, Crown.Z + 3.0, 10.7, 9.7) }, 14, true, true);
			Loft(B, O.HatColor, { Flat(Crown.X, Crown.Z - 0.5, 17.5, 16.0), Flat(Crown.X, Crown.Z + 0.3, 17.5, 16.0) }, 16, true, true);
			break;
		case EHat::Cloche:
			Loft(B, O.HatColor, {
				Flat(Crown.X + 0.5, Crown.Z - 6.0, 12.5, 11.0),
				Flat(Crown.X, Crown.Z - 3.0, 11.5, 10.2),
				Flat(Crown.X, Crown.Z + 3.0, 10.8, 9.6),
				Flat(Crown.X - 0.5, Crown.Z + 7.5, 7.5, 6.5),
				Flat(Crown.X - 0.5, Crown.Z + 9.0, 3.0, 2.5),
			}, 14, true, true);
			Loft(B, O.HatBand, { Flat(Crown.X, Crown.Z - 2.5, 11.7, 10.4), Flat(Crown.X, Crown.Z - 0.5, 11.5, 10.2) }, 14, true, true);
			break;
		case EHat::Hood:
		{
			// A loose hood: a cowl round the head, open at the face, its edge
			// drawn to a peak over the brow, and a short cape over the shoulders.
			auto Cowl = [&HeadAt, Hr](double X, double Up, double RadiusY, double RadiusZ)
			{
				return FRing{ HeadAt + FVector(X, 0.0, Up) * Hr, FVector(0.0, RadiusY * Hr, 0.0), FVector(0.0, 0.0, RadiusZ * Hr) };
			};
			Loft(B, O.HatColor, {
				Cowl(8.5, 1.0, 9.2, 11.5),
				Cowl(2.0, 2.0, 11.0, 13.0),
				Cowl(-6.0, 1.0, 10.8, 12.5),
				Cowl(-12.0, -1.0, 7.5, 9.5),
				Cowl(-14.5, -2.0, 3.0, 4.0),
			}, 14, false, true);
			Loft(B, O.HatColor, {
				{ HeadAt + FVector(2.0, 0.0, 11.5) * Hr, FVector(0.0, 7.0, 0.0), FVector(0.0, 0.0, 3.0) },
				{ HeadAt + FVector(8.0, 0.0, 11.0) * Hr, FVector(0.0, 4.0, 0.0), FVector(0.0, 0.0, 2.0) },
				{ HeadAt + FVector(12.0, 0.0, 8.0) * Hr, FVector(0.0, 0.6, 0.0), FVector(0.0, 0.0, 0.6) },
			}, 10, false, true);
			Loft(B, O.HatColor, {
				Flat(-0.5, Z(134.0), 12.8 * G * Thick, 21.0 * G * Thick),
				Flat(-1.0, Z(144.0), 10.5 * Thick, 14.5 * Thick),
				Flat(-2.5, Z(155.0), 8.6, 9.8),
			}, 14, false, false);
			break;
		}
		default:
			break;
		}

		// Legs: trousers (or stockings under a skirt), from the hip to the
		// knee, then the shin and shoe from the knee. The skirts of a coat or
		// dress below the hips hang in halves beside them (SkirtFollow), so
		// they part as he strides instead of the legs poking through them.
		// The flaps of long skirts: rows of stops down from inside the rigid
		// top (height, half depth, half width of both flaps together, how far
		// forward), and the height of the hinge.
		struct FStop
		{
			double Z, Depth, Width, X;
		};
		TArray<FStop> Skirt;
		double Hinge = 0.0;
		FLinearColor SkirtColor = O.Coat;
		if (!O.bSeated)
		{
			if (O.Top == ETop::Overcoat)
			{
				Skirt = { FStop{ Z(81.0), 15.4 * G, 18.4 * G, -0.3 }, FStop{ Z(74.0), 15.8 * G, 18.6 * G, -0.5 }, FStop{ Z(50.0), 18.4 * G, 21.2 * G, -1.5 } };
				Hinge = Z(77.0);
			}
			else if (O.Top == ETop::Dress || W)
			{
				Skirt = { FStop{ Z(79.0), 15.0 * G, 17.8 * G, -0.3 }, FStop{ Z(72.0), 15.3 * G, 18.0 * G, -0.5 }, FStop{ Z(38.0), 19.0 * G, 21.6 * G, -1.0 } };
				Hinge = Z(75.0);
				SkirtColor = O.Top == ETop::Dress ? O.Coat : O.Trousers;
			}
		}
		for (const EPiece Leg : { LegLeft, LegRight })
		{
			FPiece& L = Out.Pieces[Leg];
			FPiece& Shin = Out.Pieces[ShinOf(Leg)];
			FPiece& Coat = Out.Pieces[SkirtOf(Leg)];
			const double Side = Leg == LegLeft ? -1.0 : 1.0;
			// Hinged inside the top of the skirts, over this leg.
			Coat.Pivot = FVector(0.0, 9.0 * Side * G, Hinge);
			const FVector Hip = L.Pivot;
			const double Lg = W ? 0.85 : 1.0;
			const FLinearColor LegColor = W ? O.Skin * 0.8f : O.Trousers;
			const double Sz = W ? 0.85 : 1.0;
			if (O.bSeated)
			{
				// Thigh forward to the knee, shin down to the floor.
				const double Knee = Hip.X + 44.0 * H;
				const double KneeZ = Hip.Z - 3.0;
				const double Floor = SeatDrop;
				Shin.Pivot = FVector(Knee, Hip.Y, KneeZ);
				auto Across = [&Hip](double X, double Zc, double Rz, double Ry)
				{
					return FRing{ FVector(X, Hip.Y, Zc), FVector(0.0, 0.0, Rz), FVector(0.0, Ry, 0.0) };
				};
				Loft(L, LegColor, {
					Across(Hip.X - 2.0, Hip.Z, 9.0 * G * Lg, 8.5 * G * Lg),
					Across(Hip.X + 20.0 * H, Hip.Z - 1.5, 7.5 * G * Lg, 7.5 * G * Lg),
					Across(Knee, KneeZ, 6.2 * Lg, 6.2 * Lg),
				}, 10, true, false);
				Blob(L, LegColor, FVector(Knee, Hip.Y, KneeZ), FVector(6.4, 6.4, 6.4) * Lg, 10, 5);
				Loft(Shin, LegColor, {
					Flat(Knee, KneeZ, 6.2 * Lg, 6.0 * Lg, Hip.Y),
					Flat(Knee - 0.5, Floor + 10.0, 5.6 * Lg, 5.2 * Lg, Hip.Y),
				}, 10, false, true);
				const FVector Foot(Knee - 2.0, Hip.Y + 0.5 * Side, Floor);
				Loft(Shin, O.Shoes, {
					{ Foot + FVector(-6.0 * Sz, 0.0, 4.5), FVector(0.0, 4.0 * Sz, 0.0), FVector(0.0, 0.0, 3.5) },
					{ Foot + FVector(0.0, 0.0, 5.0), FVector(0.0, 4.8 * Sz, 0.0), FVector(0.0, 0.0, 4.5) },
					{ Foot + FVector(8.0 * Sz, 0.0, 3.8), FVector(0.0, 4.8 * Sz, 0.0), FVector(0.0, 0.0, 3.6) },
					{ Foot + FVector(16.0 * Sz, 0.0, 2.8), FVector(0.0, 4.0 * Sz, 0.0), FVector(0.0, 0.0, 2.6) },
					{ Foot + FVector(19.5 * Sz, 0.0, 2.4), FVector(0.0, 2.2 * Sz, 0.0), FVector(0.0, 0.0, 1.8) },
				}, 10, true, true, PI * 0.5);
				continue;
			}
			const double KneeZ = Z(50.0);
			const FVector Knee(Hip.X + 0.8, Hip.Y, KneeZ);
			Shin.Pivot = Knee;
			// The thigh, round at the knee so the bend shows no gap.
			Loft(L, LegColor, {
				Flat(Hip.X, Hip.Z + 4.0, 9.0 * G * Lg, 8.5 * G * Lg, Hip.Y),
				Flat(Hip.X + 0.5, Z(70.0), 7.5 * G * Lg, 7.0 * G * Lg, Hip.Y),
				Flat(Knee.X, KneeZ, 6.3 * Lg, 6.1 * Lg, Hip.Y),
			}, 10, true, false);
			Blob(L, LegColor, Knee, FVector(6.4, 6.2, 6.4) * Lg, 10, 5);
			// The shin, from the knee down.
			Loft(Shin, LegColor, {
				Flat(Knee.X, KneeZ, 6.2 * Lg, 6.0 * Lg, Hip.Y),
				Flat(Hip.X, Z(30.0), 6.3 * Lg, 6.0 * Lg, Hip.Y),
				Flat(Hip.X - 0.5, Z(10.0), 5.6 * Lg, 5.2 * Lg, Hip.Y),
			}, 10, true, true);
			// The shoe (or a boot), lofted from heel to toe.
			const FVector Foot(Hip.X, Hip.Y + 0.5 * Side, 0.0);
			Loft(Shin, O.Shoes, {
				{ Foot + FVector(-6.0 * Sz, 0.0, 4.5), FVector(0.0, 4.0 * Sz, 0.0), FVector(0.0, 0.0, 3.5) },
				{ Foot + FVector(0.0, 0.0, 5.0), FVector(0.0, 4.8 * Sz, 0.0), FVector(0.0, 0.0, 4.5) },
				{ Foot + FVector(8.0 * Sz, 0.0, 3.8), FVector(0.0, 4.8 * Sz, 0.0), FVector(0.0, 0.0, 3.6) },
				{ Foot + FVector(16.0 * Sz, 0.0, 2.8), FVector(0.0, 4.0 * Sz, 0.0), FVector(0.0, 0.0, 2.6) },
				{ Foot + FVector(19.5 * Sz, 0.0, 2.4), FVector(0.0, 2.2 * Sz, 0.0), FVector(0.0, 0.0, 1.8) },
			}, 10, true, true, PI * 0.5);
			if (O.bBoots)
			{
				Loft(Shin, O.Shoes, { Flat(Foot.X - 0.5, 5.0, 6.0, 5.8, Hip.Y), Flat(Foot.X - 0.5, Z(28.0), 6.6, 6.4, Hip.Y) }, 10, false, true);
			}
			// This leg's half of the skirts: an oval from the middle seam out
			// to the side, widening to the hem, overlapping the other half a little.
			if (Skirt.Num() > 1)
			{
				TArray<FRing> Rings;
				for (const FStop& Stop : Skirt)
				{
					Rings.Add({ FVector(Stop.X, Side * Stop.Width * 0.45, Stop.Z), FVector(Stop.Depth, 0.0, 0.0), FVector(0.0, Stop.Width * 0.62, 0.0) });
				}
				Loft(Coat, SkirtColor, Rings, 14, false, true);
			}
		}

		// Arms: sleeves of the coat (or shirt) and hands, from the shoulder.
		for (const EPiece Arm : { ArmLeft, ArmRight })
		{
			FPiece& A = Out.Pieces[Arm];
			const FVector S = A.Pivot;
			const double Side = Arm == ArmLeft ? -1.0 : 1.0;
			const double Ag = (W ? 0.85 : 1.0) * (O.Top == ETop::Overcoat ? 1.12 : 1.0);
			const FLinearColor& Sleeve = bCoat || O.Top == ETop::Dress ? O.Coat : O.Shirt;
			// The arm hangs a little out from the body, past the hips.
			// A rounded shoulder, and a sleeve hanging a little out from the body, past the hips.
			Blob(A, Sleeve, FVector(S.X, S.Y, S.Z), FVector(6.2 * Ag * G, 5.8 * Ag * G, 4.0), 10, 3, 0.0);
			Loft(A, Sleeve, {
				Flat(S.X, S.Z, 6.2 * Ag * G, 5.8 * Ag * G, S.Y),
				Flat(S.X + 0.5, S.Z - 6.0, 5.6 * Ag * G, 5.2 * Ag * G, S.Y + 0.5 * Side),
				Flat(S.X + 1.0, S.Z - 26.0 * H, 4.6 * Ag, 4.4 * Ag, S.Y + 2.5 * Side),
				Flat(S.X + 2.0, S.Z - 52.0 * H, 4.2 * Ag, 4.0 * Ag, S.Y + 4.5 * Side),
			}, 10, false, true);
			const double Hs = W ? 0.85 : 1.0;
			const FVector Hand = FVector(S.X + 2.5, S.Y + 5.0 * Side, S.Z - 56.5 * H);
			Blob(A, O.Skin, Hand, FVector(3.6, 2.4, 5.6) * Hs, 8, 5);
			Blob(A, O.Skin, Hand + FVector(2.8, -1.4 * Side, 1.5) * Hs, FVector(1.2, 1.2, 2.6) * Hs, 6, 3);

			// Something in the right hand.
			if (Arm == ArmRight)
			{
				switch (O.Held)
				{
				case EHeld::Briefcase:
					Block(A, O.HeldColor, Hand + FVector(0.0, 0.0, -18.0), FVector(18.0, 4.5, 13.0));
					Rod(A, FLinearColor(0.15f, 0.08f, 0.03f), Hand + FVector(-4.0, 0.0, -4.0), Hand + FVector(4.0, 0.0, -4.0), 1.0);
					break;
				case EHeld::Umbrella:
					Rod(A, FLinearColor(0.02f, 0.02f, 0.02f), Hand + FVector(6.0, 0.0, -3.0), Hand + FVector(22.0, 0.0, -72.0), 1.8, 8);
					Rod(A, O.HeldColor, Hand + FVector(0.0, 0.0, 2.0), Hand + FVector(6.0, 0.0, -3.0), 1.0);
					break;
				case EHeld::Newspaper:
					Block(A, FLinearColor(0.75f, 0.73f, 0.66f), Hand + FVector(2.0, 3.0, -8.0), FVector(8.0, 0.8, 14.0));
					break;
				case EHeld::Basket:
					Blob(A, O.HeldColor, Hand + FVector(0.0, 0.0, -24.0), FVector(16.0, 11.0, 10.0), 12, 4, -0.3);
					Rod(A, O.HeldColor, Hand + FVector(-10.0, 0.0, -18.0), Hand + FVector(0.0, 0.0, -1.0), 1.0);
					Rod(A, O.HeldColor, Hand + FVector(10.0, 0.0, -18.0), Hand + FVector(0.0, 0.0, -1.0), 1.0);
					break;
				default:
					break;
				}
			}
		}
	}
}
