// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PersonKit.h"
#include "MarketModel.h"

/**
 * The two music bars of Dzsoki's side quests and the people who play in them.
 *
 * - The Feher Galamb tavern (BuildTavern): a low Jozsefvaros room with dark
 *   panelling, white tablecloths, a counter with a barrel, a green tiled stove
 *   and a little stage in the back corner, where Dzsoki's band plays.
 * - The Rozsa coffee house (BuildCafeHall): a tall bright hall on Jozsef korut
 *   with mirrors in gilt frames, columns, marble tables, red plush benches,
 *   chandeliers and a stage with a brass rail, where Gyuszi's band plays.
 *
 * Both rooms are built in their own frame: the street door in the middle of
 * the front wall at X = 0, the room running back along +X, the floor at Z = 0.
 * The stage spots (TavernSpot, CafeSpot) say where each player stands or sits.
 *
 * The instruments (FPiece, built like PersonKit pieces): the cimbalom on its
 * legs (built from the middle of its feet, the player at -X), its hammers
 * (grip at the origin, along +Z), the double bass (from its spike, upright) and
 * a clarinet (grip at the origin, along +Z). The fiddle is DailyLifeModel's.
 */
namespace MusicBarModel
{
	using PersonKit::FPiece;
	using PersonKit::FOutfit;
	using MarketModel::FShape;

	// Colours.
	inline FLinearColor Walnut() { return FLinearColor(0.20f, 0.10f, 0.045f); }
	inline FLinearColor DarkPanel() { return FLinearColor(0.11f, 0.06f, 0.03f); }
	inline FLinearColor Lime() { return FLinearColor(0.78f, 0.74f, 0.64f); }
	inline FLinearColor Cloth() { return FLinearColor(0.88f, 0.87f, 0.83f); }
	inline FLinearColor Gilt() { return FLinearColor(0.70f, 0.52f, 0.18f); }
	inline FLinearColor Plush() { return FLinearColor(0.38f, 0.04f, 0.05f); }
	inline FLinearColor Mirror() { return FLinearColor(0.55f, 0.60f, 0.64f); }
	inline FLinearColor Marble() { return FLinearColor(0.86f, 0.85f, 0.82f); }
	inline FLinearColor Brass() { return FLinearColor(0.66f, 0.48f, 0.16f); }
	inline FLinearColor Steel() { return FLinearColor(0.55f, 0.55f, 0.56f); }
	inline FLinearColor BlackWood() { return FLinearColor(0.025f, 0.02f, 0.018f); }
	inline FLinearColor LampGlow() { return FLinearColor(1.0f, 0.86f, 0.55f); }

	/**
	 * A flat-sided slab: the outline (points round the edge, seen from above,
	 * either way round) raised from Z0 to Z1, with its top, bottom and sides.
	 */
	inline void Prism(FPiece& Piece, const FLinearColor& Color, const TArray<FVector2D>& Outline, double Z0, double Z1)
	{
		PersonKit::FSection& S = Piece.Section(Color);
		const int32 N = Outline.Num();
		// Which way round the outline runs (positive area: anticlockwise from above).
		double Area = 0.0;
		for (int32 i = 0; i < N; ++i)
		{
			const FVector2D& A = Outline[i];
			const FVector2D& B = Outline[(i + 1) % N];
			Area += A.X * B.Y - B.X * A.Y;
		}
		const bool bCcw = Area > 0.0;
		auto Quad = [&S](const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& Normal)
		{
			const int32 First = S.Vertices.Num();
			S.Vertices.Append({ A, B, C, D });
			S.Normals.Append({ Normal, Normal, Normal, Normal });
			// A B C D go round anticlockwise seen from the normal's side; Unreal's front faces are clockwise.
			S.Triangles.Append({ First, First + 2, First + 1, First, First + 3, First + 2 });
		};
		for (int32 i = 0; i < N; ++i)
		{
			const FVector2D& A = Outline[bCcw ? i : N - 1 - i];
			const FVector2D& B = Outline[bCcw ? (i + 1) % N : (2 * N - 2 - i) % N];
			const FVector Out = FVector(B.Y - A.Y, A.X - B.X, 0.0).GetSafeNormal();
			Quad(FVector(A.X, A.Y, Z0), FVector(B.X, B.Y, Z0), FVector(B.X, B.Y, Z1), FVector(A.X, A.Y, Z1), Out);
		}
		// Top and bottom as fans from the first point (the outlines used here are convex).
		for (int32 i = 1; i + 1 < N; ++i)
		{
			const int32 First = S.Vertices.Num();
			const FVector2D& A = Outline[0];
			const FVector2D& B = Outline[i];
			const FVector2D& C = Outline[i + 1];
			S.Vertices.Append({ FVector(A.X, A.Y, Z1), FVector(B.X, B.Y, Z1), FVector(C.X, C.Y, Z1) });
			S.Normals.Append({ FVector::UpVector, FVector::UpVector, FVector::UpVector });
			if (bCcw) { S.Triangles.Append({ First, First + 2, First + 1 }); }
			else { S.Triangles.Append({ First, First + 1, First + 2 }); }
			const int32 Low = S.Vertices.Num();
			S.Vertices.Append({ FVector(A.X, A.Y, Z0), FVector(B.X, B.Y, Z0), FVector(C.X, C.Y, Z0) });
			S.Normals.Append({ FVector(0.0, 0.0, -1.0), FVector(0.0, 0.0, -1.0), FVector(0.0, 0.0, -1.0) });
			if (bCcw) { S.Triangles.Append({ Low, Low + 1, Low + 2 }); }
			else { S.Triangles.Append({ Low, Low + 2, Low + 1 }); }
		}
	}

	// ---------------------------------------------------------------------
	// The instruments.

	/** How high the cimbalom's strings are, and how far in front of the player's feet its middle stands. */
	constexpr double CimbalomTop = 90.0;
	constexpr double CimbalomReach = 58.0;

	/**
	 * The concert cimbalom (the Schunda kind): a trapezoid box, its long side
	 * towards the player (-X), on four turned legs with a pedal lyre, its
	 * strings in courses across, two rows of bridges and the maker's rose.
	 */
	inline void Cimbalom(FPiece& P)
	{
		const double Near = -34.0, Far = 34.0, LongHalf = 70.0, ShortHalf = 46.0;
		const double Z0 = CimbalomTop - 14.0, Z1 = CimbalomTop - 1.5;
		Prism(P, Walnut(), { FVector2D(Near, -LongHalf), FVector2D(Near, LongHalf), FVector2D(Far, ShortHalf), FVector2D(Far, -ShortHalf) }, Z0, Z1);
		// The soundboard, a shade paler, and the dark rim round it.
		Prism(P, FLinearColor(0.50f, 0.33f, 0.15f), { FVector2D(Near + 4.0, -LongHalf + 6.0), FVector2D(Near + 4.0, LongHalf - 6.0), FVector2D(Far - 4.0, ShortHalf - 4.0), FVector2D(Far - 4.0, -ShortHalf + 4.0) }, Z1, Z1 + 0.4);
		// Strings: courses across, from the near side to the far.
		for (int32 i = 0; i < 15; ++i)
		{
			const double X = Near + 6.0 + i * 4.3;
			const double T = (X - Near) / (Far - Near);
			const double Half = FMath::Lerp(LongHalf, ShortHalf, T) - 7.0;
			PersonKit::Rod(P, Steel(), FVector(X, -Half, Z1 + 1.4), FVector(X, Half, Z1 + 1.4), 0.25, 3);
		}
		// Two rows of bridges (little dark studs) and the rose.
		for (int32 Row = 0; Row < 2; ++Row)
		{
			const double Y = Row == 0 ? -20.0 : 26.0;
			for (int32 i = 0; i < 9; ++i)
			{
				PersonKit::Block(P, BlackWood(), FVector(Near + 9.0 + i * 6.8, Y, Z1 + 1.0), FVector(0.8, 0.8, 1.0));
			}
		}
		PersonKit::Loft(P, Gilt(), { PersonKit::Flat(4.0, Z1 + 0.5, 5.0, 5.0, 4.0), PersonKit::Flat(4.0, Z1 + 0.7, 4.2, 4.2, 4.0) }, 12, false, false);
		// Tuning pins along the right end.
		for (int32 i = 0; i < 8; ++i)
		{
			const double X = Near + 6.0 + i * 8.0;
			const double Half = FMath::Lerp(LongHalf, ShortHalf, (X - Near) / (Far - Near)) - 3.0;
			PersonKit::Rod(P, Steel(), FVector(X, Half, Z1), FVector(X, Half, Z1 + 3.0), 0.5, 4);
		}
		// Legs, splayed a little, and the pedal lyre between the front ones.
		const FVector2D Feet[4] = { FVector2D(Near + 6.0, -LongHalf + 14.0), FVector2D(Near + 6.0, LongHalf - 14.0), FVector2D(Far - 6.0, ShortHalf - 10.0), FVector2D(Far - 6.0, -ShortHalf + 10.0) };
		for (const FVector2D& F : Feet)
		{
			const FVector Top(F.X, F.Y, Z0);
			const FVector Foot(F.X * 1.1, F.Y * 1.08, 0.0);
			PersonKit::Rod(P, Walnut(), Foot, Top, 2.2, 8);
			PersonKit::Blob(P, Walnut(), Top + FVector(0.0, 0.0, -10.0), FVector(3.0, 3.0, 4.0), 8, 4);
		}
		PersonKit::Rod(P, Walnut(), FVector(Near + 2.0, -10.0, 4.0), FVector(Near + 2.0, -10.0, Z0), 1.4, 6);
		PersonKit::Rod(P, Walnut(), FVector(Near + 2.0, 10.0, 4.0), FVector(Near + 2.0, 10.0, Z0), 1.4, 6);
		PersonKit::Block(P, Brass(), FVector(Near - 4.0, 0.0, 4.0), FVector(8.0, 12.0, 1.0));
	}

	/** A cimbalom hammer: a thin cane, the grip at the origin, the cotton-wrapped head at +Z. */
	inline void Hammer(FPiece& P)
	{
		PersonKit::Rod(P, Walnut(), FVector(0.0, 0.0, -3.0), FVector(0.0, 0.0, 22.0), 0.35, 4);
		PersonKit::Blob(P, Cloth(), FVector(0.0, 0.0, 23.5), FVector(0.9, 0.9, 2.0), 6, 3);
	}

	/** The double bass from its spike, upright, the front of its belly at +X, the neck up +Z. */
	inline void Bass(FPiece& P)
	{
		const FLinearColor Varnish(0.36f, 0.14f, 0.04f);
		PersonKit::Rod(P, Steel(), FVector(0.0, 0.0, 0.0), FVector(0.0, 0.0, 10.0), 0.8, 4);
		PersonKit::Blob(P, Varnish, FVector(0.0, 0.0, 38.0), FVector(11.0, 32.0, 28.0), 12, 6);
		PersonKit::Blob(P, Varnish, FVector(0.0, 0.0, 82.0), FVector(10.0, 24.0, 24.0), 12, 6);
		PersonKit::Block(P, Varnish, FVector(0.0, 0.0, 60.0), FVector(9.0, 20.0, 8.0));
		PersonKit::Rod(P, BlackWood(), FVector(6.0, 0.0, 70.0), FVector(5.0, 0.0, 160.0), 2.2, 6);
		PersonKit::Block(P, BlackWood(), FVector(9.0, 0.0, 40.0), FVector(1.0, 7.0, 4.0));
		PersonKit::Blob(P, Varnish, FVector(4.0, 0.0, 166.0), FVector(3.0, 3.0, 6.0), 8, 4);
		for (int32 i = 0; i < 4; ++i)
		{
			const double Y = -3.0 + i * 2.0;
			PersonKit::Rod(P, Steel(), FVector(10.5, Y, 36.0), FVector(7.5, Y * 0.5, 158.0), 0.2, 3);
		}
	}

	/** The bass bow: the frog in the right fist, the stick along +Z. */
	inline void BassBow(FPiece& P)
	{
		PersonKit::Rod(P, BlackWood(), FVector(0.0, 0.0, -3.0), FVector(0.0, 0.0, 58.0), 0.7, 4);
		PersonKit::Block(P, BlackWood(), FVector(0.0, 0.0, 0.0), FVector(1.2, 1.2, 3.5));
		PersonKit::Rod(P, Cloth(), FVector(1.8, 0.0, 1.0), FVector(1.8, 0.0, 56.0), 0.35, 3);
	}

	/** The clarinet, held in the right fist, the bell at -Z, the mouthpiece at +Z. */
	inline void Clarinet(FPiece& P)
	{
		PersonKit::Rod(P, BlackWood(), FVector(0.0, 0.0, -38.0), FVector(0.0, 0.0, 26.0), 1.4, 8);
		PersonKit::Loft(P, BlackWood(), { PersonKit::Flat(0.0, -38.0, 1.5, 1.5), PersonKit::Flat(0.0, -46.0, 3.6, 3.6) }, 10, false, true);
		for (int32 i = 0; i < 5; ++i)
		{
			PersonKit::Loft(P, Steel(), { PersonKit::Flat(0.0, -30.0 + i * 12.0, 1.6, 1.6), PersonKit::Flat(0.0, -29.4 + i * 12.0, 1.6, 1.6) }, 8, false, false);
		}
	}

	// ---------------------------------------------------------------------
	// The people.

	/** Dzsoki: the cimbalmos and leader of the house band. A big, heavy man near fifty, a black suit, a gold watch chain on his waistcoat. */
	inline FOutfit DzsokiLook()
	{
		FOutfit O;
		O.Height = 176.f;
		O.Build = 1.3f;
		O.Belly = 11.f;
		O.Top = PersonKit::ETop::Waistcoat;
		O.Waistcoat = FLinearColor(0.035f, 0.03f, 0.035f);
		O.Coat = O.Waistcoat;
		O.Trousers = FLinearColor(0.03f, 0.03f, 0.035f);
		O.Shirt = FLinearColor(0.94f, 0.93f, 0.90f);
		O.bTie = true;
		O.Tie = FLinearColor(0.42f, 0.05f, 0.06f);
		O.Hat = PersonKit::EHat::None;
		O.Hair = FLinearColor(0.05f, 0.04f, 0.035f);
		O.bMoustache = true;
		O.MoustacheKind = PersonKit::EMoustache::Walrus;
		O.FaceShape = 1;
		O.Skin = FLinearColor(0.56f, 0.38f, 0.26f);
		return O;
	}

	/** Dzsoki out in the street: the same with his black jacket and a bowler. */
	inline FOutfit DzsokiStreetLook()
	{
		FOutfit O = DzsokiLook();
		O.Top = PersonKit::ETop::Jacket;
		O.Coat = FLinearColor(0.04f, 0.035f, 0.04f);
		O.Hat = PersonKit::EHat::Bowler;
		return O;
	}

	/** Rózsi, Dzsoki's daughter and the band's singer: a dark green evening dress, black hair up. */
	inline FOutfit RozsiLook()
	{
		FOutfit O;
		O.bWoman = true;
		O.Height = 164.f;
		O.Top = PersonKit::ETop::Dress;
		O.Coat = FLinearColor(0.04f, 0.18f, 0.10f);
		O.Hat = PersonKit::EHat::None;
		O.bTie = false;
		O.Hair = FLinearColor(0.03f, 0.025f, 0.02f);
		O.Shoes = FLinearColor(0.05f, 0.03f, 0.02f);
		O.FaceShape = 3;
		O.Skin = FLinearColor(0.60f, 0.42f, 0.30f);
		return O;
	}

	/** Berki Gyuszi: the young first violin of the Rózsa's band, slim, a black dinner jacket, a pencil moustache. */
	inline FOutfit GyusziLook()
	{
		FOutfit O;
		O.Height = 181.f;
		O.Build = 0.94f;
		O.Top = PersonKit::ETop::Jacket;
		O.Coat = FLinearColor(0.02f, 0.02f, 0.025f);
		O.Trousers = O.Coat;
		O.Shirt = FLinearColor(0.96f, 0.96f, 0.94f);
		O.bTie = true;
		O.Tie = FLinearColor(0.02f, 0.02f, 0.02f);
		O.Hat = PersonKit::EHat::None;
		O.Hair = FLinearColor(0.02f, 0.018f, 0.016f);
		O.bMoustache = true;
		O.MoustacheKind = PersonKit::EMoustache::Pencil;
		O.FaceShape = 2;
		O.Skin = FLinearColor(0.58f, 0.40f, 0.28f);
		return O;
	}

	/**
	 * One of the bandsmen (Seed picks the face and age). The tavern band plays
	 * in waistcoats, the coffee house band (bSmart) in black dinner jackets.
	 */
	inline FOutfit BandsmanLook(int32 Seed, bool bSmart)
	{
		FOutfit O;
		O.Height = 166.f + float((Seed * 37) % 15);
		O.Build = 0.96f + 0.04f * float(Seed % 4);
		O.Top = bSmart ? PersonKit::ETop::Jacket : PersonKit::ETop::Waistcoat;
		O.Waistcoat = FLinearColor(0.04f, 0.035f, 0.04f);
		O.Coat = bSmart ? FLinearColor(0.02f, 0.02f, 0.025f) : O.Waistcoat;
		O.Trousers = FLinearColor(0.03f, 0.03f, 0.035f);
		O.Shirt = FLinearColor(0.94f, 0.93f, 0.90f);
		O.bTie = true;
		O.Tie = bSmart ? FLinearColor(0.02f, 0.02f, 0.02f) : FLinearColor(0.10f, 0.08f, 0.20f);
		O.Hat = PersonKit::EHat::None;
		O.Hair = Seed % 5 == 3 ? FLinearColor(0.35f, 0.34f, 0.33f) : FLinearColor(0.04f, 0.035f, 0.03f);
		O.bAged = Seed % 5 == 3;
		O.bMoustache = Seed % 3 != 1;
		O.FaceShape = Seed % 4;
		O.Skin = FLinearColor(0.56f + 0.02f * float(Seed % 3), 0.39f, 0.27f);
		return O;
	}

	// ---------------------------------------------------------------------
	// Playing poses (a held pose like DailyLifeModel::FLimbs: arm turns, elbows, legs, knees).

	struct FPlay
	{
		FRotator Arms[2] = { FRotator::ZeroRotator, FRotator::ZeroRotator };
		float Elbows[2] = { 0.f, 0.f };
		float Legs[2] = { 0.f, 0.f };
		float Knees[2] = { 0.f, 0.f };
		float Pitch = 0.f;
	};

	/** At the cimbalom: both hands over the strings, the hammers bouncing in turn (Phase in beats). */
	inline void Hammering(FPlay& L, float Phase)
	{
		const float A = FMath::Sin(2.f * PI * Phase);
		const float B = FMath::Sin(2.f * PI * (Phase + 0.5f));
		L.Arms[0] = FRotator(22.f + 5.f * A, 0.f, -8.f);
		L.Arms[1] = FRotator(22.f + 5.f * B, 0.f, 8.f);
		L.Elbows[0] = 38.f + 14.f * A;
		L.Elbows[1] = 38.f + 14.f * B;
		L.Pitch = 12.f;
	}

	/** The turn of a hammer in the fist (it points forward and a little down at the strings). */
	inline FRotator HammerTurn(float Raised) { return FRotator(-Raised - 100.f, 0.f, 0.f); }

	/** Singing: the left hand at her heart, the right open towards the room, swaying with Phase. */
	inline void Singing(FPlay& L, float Phase)
	{
		const float A = FMath::Sin(2.f * PI * Phase);
		L.Arms[0] = FRotator(30.f, 0.f, -14.f);
		L.Elbows[0] = 105.f;
		L.Arms[1] = FRotator(48.f + 8.f * A, 0.f, -22.f);
		L.Elbows[1] = 28.f;
		L.Legs[0] = 6.f;
		L.Legs[1] = -4.f;
	}

	/**
	 * Playing the fiddle (DailyLifeModel::Fiddle, built from the chin rest
	 * with the neck along +Y) under the chin: it sits on the left shoulder at
	 * FiddleAt, turned by FiddleTurn so the scroll points forward and to the
	 * left; the left hand is on the neck, the right draws the bow across.
	 */
	inline FVector FiddleAt(double Height) { return FVector(8.0, -12.0, 141.0 * Height / 178.0); }
	inline FRotator FiddleTurn() { return FRotator(0.0, -142.0, -12.0); }
	/** The bow's turn in the right fist (DailyLifeModel::FiddleBow), lying across the strings, for an arm raised Raised degrees. */
	inline FRotator BowTurn(float Raised) { return FRotator(-Raised, 0.f, -90.f); }
	inline void Fiddling(FPlay& L, float Phase)
	{
		const float A = FMath::Sin(2.f * PI * Phase);
		L.Arms[0] = FRotator(75.f, 0.f, 10.f);
		L.Elbows[0] = 40.f;
		L.Arms[1] = FRotator(50.f + 6.f * A, 0.f, -20.f - 14.f * A);
		L.Elbows[1] = 50.f + 10.f * A;
	}

	/** Bowing the bass: the left hand high on the neck, the right drawing the bow across. */
	inline void BassBowing(FPlay& L, float Phase)
	{
		const float A = FMath::Sin(2.f * PI * Phase);
		L.Arms[0] = FRotator(78.f, 0.f, 18.f);
		L.Elbows[0] = 70.f;
		L.Arms[1] = FRotator(30.f, 0.f, -8.f - 12.f * A);
		L.Elbows[1] = 62.f + 10.f * A;
	}

	/** Playing the clarinet: both hands at the chest, the bell down in front. */
	inline void Piping(FPlay& L, float Phase)
	{
		const float A = FMath::Sin(2.f * PI * Phase);
		L.Arms[0] = FRotator(34.f, 0.f, 18.f);
		L.Elbows[0] = 96.f;
		L.Arms[1] = FRotator(28.f, 0.f, -18.f);
		L.Elbows[1] = 78.f;
		L.Pitch = -3.f + 3.f * A;
	}

	// ---------------------------------------------------------------------
	// The rooms.

	enum class EPlayer : uint8 { Cimbalom, Fiddle, Bass, Clarinet, Singer, Second };

	struct FSpot
	{
		EPlayer Player;
		FVector At;
		double Yaw;
	};

	/** Where the tavern's band stands: Dzsoki at the cimbalom, the first fiddle, the bass, Rózsi at the front. */
	inline TArray<FSpot> TavernSpots()
	{
		return {
			{ EPlayer::Cimbalom, FVector(800.0, -340.0, 25.0), 180.0 },
			{ EPlayer::Fiddle, FVector(730.0, -470.0, 25.0), 160.0 },
			{ EPlayer::Bass, FVector(840.0, -500.0, 25.0), 150.0 },
			{ EPlayer::Singer, FVector(705.0, -200.0, 25.0), 200.0 },
		};
	}

	/** Where the coffee house band stands: Gyuszi in front, then the second fiddle, clarinet, cimbalom and bass. */
	inline TArray<FSpot> CafeSpots()
	{
		return {
			{ EPlayer::Fiddle, FVector(1130.0, 60.0, 40.0), 180.0 },
			{ EPlayer::Second, FVector(1230.0, -120.0, 40.0), 170.0 },
			{ EPlayer::Clarinet, FVector(1260.0, 240.0, 40.0), 195.0 },
			{ EPlayer::Cimbalom, FVector(1300.0, 60.0, 40.0), 180.0 },
			{ EPlayer::Bass, FVector(1330.0, -300.0, 40.0), 160.0 },
		};
	}

	/** A bentwood chair (Thonet No. 14) at At, its back towards -X turned by Yaw. */
	inline void Chair(MarketModel::FBuilder& B, const FVector& At, double Yaw)
	{
		const FRotator R(0.0, Yaw, 0.0);
		const FLinearColor Wood = BlackWood();
		B.Add(MarketModel::Cylinder, Wood, At + FVector(0.0, 0.0, 45.0), FVector(40.0, 40.0, 4.0));
		for (int32 i = 0; i < 4; ++i)
		{
			const FVector Leg = R.RotateVector(FVector(i < 2 ? 13.0 : -13.0, i % 2 ? 13.0 : -13.0, 22.0));
			B.Add(MarketModel::Cylinder, Wood, At + Leg, FVector(3.0, 3.0, 44.0));
		}
		const FVector Back = At + R.RotateVector(FVector(-18.0, 0.0, 70.0));
		B.Add(MarketModel::Cube, Wood, Back, FVector(3.0, 34.0, 44.0), 0.0, Yaw, 0.0);
		B.Add(MarketModel::Cylinder, Wood, Back + FVector(0.0, 0.0, 22.0), FVector(4.0, 36.0, 4.0), 0.0, Yaw, 90.0);
	}

	/** A hanging lamp: a flex from the ceiling and a green enamel shade with a bulb. */
	inline void HangingLamp(MarketModel::FBuilder& B, const FVector& At, double Ceiling)
	{
		B.Add(MarketModel::Cylinder, BlackWood(), FVector(At.X, At.Y, (Ceiling + At.Z) * 0.5), FVector(1.0, 1.0, Ceiling - At.Z));
		B.Add(MarketModel::Cone, FLinearColor(0.08f, 0.22f, 0.12f), At, FVector(42.0, 42.0, 18.0));
		B.Add(MarketModel::Sphere, LampGlow(), At - FVector(0.0, 0.0, 9.0), FVector(11.0, 11.0, 11.0));
	}

	/**
	 * The Fehér Galamb tavern: 9 m deep, 11 m wide, 3.4 m high. bClosedIn
	 * adds the front wall and the ceiling (left off for a look from outside).
	 * Lamps lists where its lights hang.
	 */
	constexpr double TavernDepth = 900.0;
	constexpr double TavernHalfWidth = 550.0;
	constexpr double TavernHeight = 340.0;

	/** Where the tavern's name is painted on the cloth behind the band (facing -X, into the room). */
	inline FVector TavernSignAt() { return FVector(TavernDepth - 8.0, -340.0, 240.0); }

	inline void BuildTavern(TArray<FShape>& Out, const FVector& Origin, double Yaw, bool bClosedIn, TArray<FVector>* Lamps = nullptr)
	{
		using namespace MarketModel;
		FBuilder B{ Out, Origin, Yaw };
		FRandom Random(1932);
		const double D = TavernDepth, W = TavernHalfWidth, H = TavernHeight;
		const FLinearColor Floor(0.17f, 0.10f, 0.05f);
		// Floor boards (strips along X), walls with dark panelling below and lime above.
		for (int32 i = 0; i < 11; ++i)
		{
			const double Y = -W + 50.0 + i * 100.0;
			B.Add(Cube, Floor * (0.9f + 0.2f * float(Random.Unit())), FVector(D * 0.5, Y, -1.0), FVector(D, 98.0, 2.0), 0.0, 0.0, 0.0, true);
		}
		B.Add(Cube, Lime(), FVector(D + 10.0, 0.0, H * 0.5), FVector(20.0, 2.0 * W + 40.0, H), 0.0, 0.0, 0.0, true);
		B.Add(Cube, Lime(), FVector(D * 0.5, -W - 10.0, H * 0.5), FVector(D, 20.0, H), 0.0, 0.0, 0.0, true);
		B.Add(Cube, Lime(), FVector(D * 0.5, W + 10.0, H * 0.5), FVector(D, 20.0, H), 0.0, 0.0, 0.0, true);
		B.Add(Cube, DarkPanel(), FVector(D - 1.0, 0.0, 60.0), FVector(4.0, 2.0 * W, 120.0));
		B.Add(Cube, DarkPanel(), FVector(D * 0.5, -W + 1.0, 60.0), FVector(D, 4.0, 120.0));
		B.Add(Cube, DarkPanel(), FVector(D * 0.5, W - 1.0, 60.0), FVector(D, 4.0, 120.0));
		B.Add(Cube, Walnut(), FVector(D - 3.0, 0.0, 122.0), FVector(8.0, 2.0 * W, 5.0));
		B.Add(Cube, Walnut(), FVector(D * 0.5, -W + 3.0, 122.0), FVector(D, 8.0, 5.0));
		B.Add(Cube, Walnut(), FVector(D * 0.5, W - 3.0, 122.0), FVector(D, 8.0, 5.0));
		if (bClosedIn)
		{
			// Front wall with the door in the middle and a window either side.
			B.Add(Cube, Lime(), FVector(-10.0, -W * 0.5 - 50.0, H * 0.5), FVector(20.0, W - 100.0 + 40.0, H), 0.0, 0.0, 0.0, true);
			B.Add(Cube, Lime(), FVector(-10.0, W * 0.5 + 50.0, H * 0.5), FVector(20.0, W - 100.0 + 40.0, H), 0.0, 0.0, 0.0, true);
			B.Add(Cube, Lime(), FVector(-10.0, 0.0, 285.0), FVector(20.0, 120.0, 110.0), 0.0, 0.0, 0.0, true);
			B.Add(Cube, Lime(), FVector(D * 0.5, 0.0, H + 5.0), FVector(D + 40.0, 2.0 * W + 40.0, 10.0));
		}
		// Ceiling beams.
		for (int32 i = 0; i < 4; ++i)
		{
			B.Add(Cube, DarkPanel(), FVector(120.0 + i * 220.0, 0.0, H - 12.0), FVector(24.0, 2.0 * W, 24.0));
		}

		// The stage: a low platform in the back left corner with a rail at its edge.
		B.Add(Cube, Walnut(), FVector(780.0, -340.0, 12.5), FVector(240.0, 420.0, 25.0), 0.0, 0.0, 0.0, true);
		B.Add(Cube, DarkPanel(), FVector(660.0, -340.0, 12.5), FVector(4.0, 420.0, 25.0));
		// A red cloth hung behind the band (the tavern's name goes on it as lettering, see TavernSignAt).
		B.Add(Cube, FLinearColor(0.32f, 0.05f, 0.05f), FVector(D - 4.0, -340.0, 210.0), FVector(3.0, 380.0, 150.0));

		// The counter along the right wall, bottles on shelves behind it and a barrel on its cradle.
		const double CounterY = W - 120.0;
		B.Add(Cube, DarkPanel(), FVector(380.0, CounterY, 54.0), FVector(420.0, 60.0, 108.0), 0.0, 0.0, 0.0, true);
		B.Add(Cube, Walnut(), FVector(380.0, CounterY, 110.0), FVector(430.0, 70.0, 5.0));
		B.Add(Cube, Brass(), FVector(380.0, CounterY - 34.0, 14.0), FVector(420.0, 4.0, 4.0));
		for (int32 Shelf = 0; Shelf < 3; ++Shelf)
		{
			const double Z = 140.0 + Shelf * 40.0;
			B.Add(Cube, Walnut(), FVector(380.0, W - 18.0, Z), FVector(400.0, 30.0, 3.0));
			for (int32 i = 0; i < 14; ++i)
			{
				const FLinearColor Glass = i % 3 == 0 ? FLinearColor(0.06f, 0.16f, 0.06f) : i % 3 == 1 ? FLinearColor(0.25f, 0.12f, 0.03f) : FLinearColor(0.55f, 0.55f, 0.50f);
				const double Tall = 22.0 + 8.0 * Random.Unit();
				B.Add(Cylinder, Glass, FVector(200.0 + i * 26.0 + 6.0 * Random.Unit(), W - 18.0, Z + 1.5 + Tall * 0.5), FVector(7.0, 7.0, Tall));
			}
		}
		B.Add(Cylinder, Walnut(), FVector(380.0, CounterY + 4.0, 140.0), FVector(52.0, 52.0, 66.0), 90.0, 90.0, 0.0);
		B.Add(Cylinder, FLinearColor(0.08f, 0.08f, 0.08f), FVector(380.0, CounterY - 12.0, 140.0), FVector(54.0, 54.0, 4.0), 90.0, 90.0, 0.0);
		B.Add(Cylinder, FLinearColor(0.08f, 0.08f, 0.08f), FVector(380.0, CounterY + 20.0, 140.0), FVector(54.0, 54.0, 4.0), 90.0, 90.0, 0.0);
		B.Add(Cylinder, Brass(), FVector(380.0, CounterY - 32.0, 128.0), FVector(4.0, 4.0, 10.0), 90.0, 90.0, 0.0);
		// The green tiled stove in the front left corner.
		B.Add(Cube, FLinearColor(0.10f, 0.26f, 0.18f), FVector(70.0, -W + 60.0, 95.0), FVector(90.0, 90.0, 190.0), 0.0, 0.0, 0.0, true);
		B.Add(Cube, FLinearColor(0.08f, 0.20f, 0.14f), FVector(70.0, -W + 60.0, 196.0), FVector(100.0, 100.0, 12.0));
		B.Add(Cube, BlackWood(), FVector(117.0, -W + 60.0, 50.0), FVector(3.0, 30.0, 26.0));
		// Pictures on the walls (the band in earlier days, a racehorse, the old king's town).
		for (int32 i = 0; i < 4; ++i)
		{
			const double X = 140.0 + i * 160.0;
			B.Add(Cube, Gilt(), FVector(X, -W + 3.0, 190.0), FVector(52.0, 3.0, 40.0));
			B.Add(Cube, FLinearColor(0.35f, 0.30f, 0.22f) * (0.6f + 0.15f * float(i)), FVector(X, -W + 5.0, 190.0), FVector(44.0, 3.0, 32.0));
		}
		B.Add(Cube, FLinearColor(0.85f, 0.85f, 0.80f), FVector(D - 4.0, 280.0, 230.0), FVector(3.0, 34.0, 34.0));
		B.Add(Cylinder, BlackWood(), FVector(D - 6.0, 280.0, 230.0), FVector(2.0, 2.0, 14.0), 0.0, 0.0, 30.0);

		// Six tables with white cloths and chairs round them.
		const FVector2D Tables[6] = { FVector2D(150.0, -330.0), FVector2D(150.0, -60.0), FVector2D(150.0, 210.0), FVector2D(380.0, -330.0), FVector2D(390.0, -60.0), FVector2D(560.0, 120.0) };
		for (int32 t = 0; t < 6; ++t)
		{
			const FVector At(Tables[t].X, Tables[t].Y, 0.0);
			B.Add(Cube, Walnut(), At + FVector(0.0, 0.0, 37.0), FVector(70.0, 70.0, 74.0), 0.0, 0.0, 0.0, true);
			B.Add(Cube, Cloth(), At + FVector(0.0, 0.0, 76.0), FVector(92.0, 92.0, 3.0));
			B.Add(Cube, FLinearColor(0.50f, 0.07f, 0.06f), At + FVector(0.0, 0.0, 77.8), FVector(92.5, 16.0, 0.6));
			B.Add(Cylinder, FLinearColor(0.60f, 0.62f, 0.55f), At + FVector(10.0, 12.0, 88.0), FVector(8.0, 8.0, 22.0));
			B.Add(Cylinder, FLinearColor(0.75f, 0.70f, 0.40f), At + FVector(-14.0, -8.0, 83.0), FVector(7.0, 7.0, 12.0));
			for (int32 c = 0; c < 4; ++c)
			{
				const double Turn = c * 90.0 + 15.0 * (Random.Unit() - 0.5);
				const FVector Off = FRotator(0.0, Turn, 0.0).RotateVector(FVector(-62.0, 0.0, 0.0));
				Chair(B, At + Off, Turn);
			}
		}
		// The lamps.
		const FVector LampSpots[5] = { FVector(150.0, -200.0, 230.0), FVector(150.0, 210.0, 230.0), FVector(390.0, -200.0, 230.0), FVector(560.0, 120.0, 230.0), FVector(620.0, -470.0, 262.0) };
		for (const FVector& L : LampSpots)
		{
			HangingLamp(B, L, H - 24.0);
			if (Lamps) { Lamps->Add(L); }
		}
	}

	/** The Rózsa coffee house hall: 14 m deep, 16 m wide, 4.8 m high. */
	constexpr double CafeDepth = 1400.0;
	constexpr double CafeHalfWidth = 800.0;
	constexpr double CafeHeight = 480.0;

	inline void BuildCafeHall(TArray<FShape>& Out, const FVector& Origin, double Yaw, bool bClosedIn, TArray<FVector>* Lamps = nullptr)
	{
		using namespace MarketModel;
		FBuilder B{ Out, Origin, Yaw };
		FRandom Random(1911);
		const double D = CafeDepth, W = CafeHalfWidth, H = CafeHeight;
		// A floor of black and white stone squares.
		const double Tile = 100.0;
		for (int32 i = 0; i < 14; ++i)
		{
			for (int32 j = 0; j < 16; ++j)
			{
				const bool bDark = (i + j) % 2 == 0;
				B.Add(Cube, bDark ? FLinearColor(0.05f, 0.05f, 0.05f) : FLinearColor(0.82f, 0.80f, 0.76f),
					FVector(Tile * (i + 0.5), -W + Tile * (j + 0.5), -1.0), FVector(Tile, Tile, 2.0), 0.0, 0.0, 0.0, true);
			}
		}
		const FLinearColor Plaster(0.80f, 0.72f, 0.56f);
		B.Add(Cube, Plaster, FVector(D + 10.0, 0.0, H * 0.5), FVector(20.0, 2.0 * W + 40.0, H), 0.0, 0.0, 0.0, true);
		B.Add(Cube, Plaster, FVector(D * 0.5, -W - 10.0, H * 0.5), FVector(D, 20.0, H), 0.0, 0.0, 0.0, true);
		B.Add(Cube, Plaster, FVector(D * 0.5, W + 10.0, H * 0.5), FVector(D, 20.0, H), 0.0, 0.0, 0.0, true);
		if (bClosedIn)
		{
			B.Add(Cube, Plaster, FVector(-10.0, -W * 0.5 - 60.0, H * 0.5), FVector(20.0, W - 120.0 + 40.0, H), 0.0, 0.0, 0.0, true);
			B.Add(Cube, Plaster, FVector(-10.0, W * 0.5 + 60.0, H * 0.5), FVector(20.0, W - 120.0 + 40.0, H), 0.0, 0.0, 0.0, true);
			B.Add(Cube, Plaster, FVector(-10.0, 0.0, 370.0), FVector(20.0, 140.0, 220.0), 0.0, 0.0, 0.0, true);
			B.Add(Cube, Plaster, FVector(D * 0.5, 0.0, H + 5.0), FVector(D + 40.0, 2.0 * W + 40.0, 10.0));
		}
		// A gilt cornice round the top.
		B.Add(Cube, Gilt(), FVector(D - 4.0, 0.0, H - 20.0), FVector(10.0, 2.0 * W, 18.0));
		B.Add(Cube, Gilt(), FVector(D * 0.5, -W + 4.0, H - 20.0), FVector(D, 10.0, 18.0));
		B.Add(Cube, Gilt(), FVector(D * 0.5, W - 4.0, H - 20.0), FVector(D, 10.0, 18.0));
		// Mirrors in gilt frames down both long walls, red plush benches under them.
		for (int32 i = 0; i < 5; ++i)
		{
			const double X = 150.0 + i * 230.0;
			for (int32 Side = -1; Side <= 1; Side += 2)
			{
				const double Y = Side * (W - 3.0);
				B.Add(Cube, Gilt(), FVector(X, Y, 250.0), FVector(170.0, 4.0, 210.0));
				B.Add(Cube, Mirror(), FVector(X, Y - Side * 2.0, 250.0), FVector(156.0, 4.0, 196.0));
			}
		}
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const double Y = Side * (W - 30.0);
			B.Add(Cube, Plush(), FVector(560.0, Y, 24.0), FVector(1060.0, 56.0, 48.0), 0.0, 0.0, 0.0, true);
			B.Add(Cube, Plush(), FVector(560.0, Y + Side * 22.0, 80.0), FVector(1060.0, 12.0, 70.0));
			B.Add(Cube, Walnut(), FVector(560.0, Y + Side * 22.0, 118.0), FVector(1060.0, 16.0, 6.0));
		}
		// Four columns with gilt capitals.
		const FVector2D Columns[4] = { FVector2D(380.0, -330.0), FVector2D(380.0, 330.0), FVector2D(820.0, -330.0), FVector2D(820.0, 330.0) };
		for (const FVector2D& C : Columns)
		{
			B.Add(Cylinder, Marble(), FVector(C.X, C.Y, H * 0.5), FVector(52.0, 52.0, H), 0.0, 0.0, 0.0, true);
			B.Add(Cube, FLinearColor(0.30f, 0.06f, 0.06f), FVector(C.X, C.Y, 40.0), FVector(66.0, 66.0, 80.0));
			B.Add(Cylinder, Gilt(), FVector(C.X, C.Y, H - 50.0), FVector(70.0, 70.0, 36.0));
			B.Add(Cube, Gilt(), FVector(C.X, C.Y, H - 26.0), FVector(80.0, 80.0, 14.0));
		}
		// The stage at the back, with a brass rail, a velvet curtain behind and the band's name in gilt above.
		B.Add(Cube, Walnut(), FVector(1250.0, 0.0, 20.0), FVector(300.0, 900.0, 40.0), 0.0, 0.0, 0.0, true);
		for (int32 i = 0; i < 9; ++i)
		{
			B.Add(Cylinder, Brass(), FVector(1100.0, -400.0 + i * 100.0, 64.0), FVector(4.0, 4.0, 48.0));
		}
		B.Add(Cylinder, Brass(), FVector(1100.0, 0.0, 88.0), FVector(5.0, 5.0, 810.0), 90.0, 90.0, 0.0);
		for (int32 i = 0; i < 18; ++i)
		{
			B.Add(Cylinder, FLinearColor(0.26f, 0.03f, 0.04f), FVector(D - 18.0, -440.0 + i * 52.0, 230.0), FVector(58.0, 40.0, 380.0));
		}
		B.Add(Cube, Gilt(), FVector(D - 36.0, 0.0, 420.0), FVector(8.0, 920.0, 26.0));
		// Marble tables on iron feet with chairs, a cup or a glass on each.
		for (int32 Row = 0; Row < 3; ++Row)
		{
			for (int32 Col = 0; Col < 5; ++Col)
			{
				const double X = 170.0 + Row * 290.0 + (Col % 2) * 60.0;
				const double Y = -560.0 + Col * 280.0;
				if (FMath::Abs(Y) < 120.0 && Row == 0) { continue; } // keep the way from the door clear
				const FVector At(X, Y, 0.0);
				B.Add(Cylinder, BlackWood(), At + FVector(0.0, 0.0, 36.0), FVector(8.0, 8.0, 72.0), 0.0, 0.0, 0.0, true);
				B.Add(Cylinder, BlackWood(), At + FVector(0.0, 0.0, 2.0), FVector(46.0, 46.0, 4.0));
				B.Add(Cylinder, Marble(), At + FVector(0.0, 0.0, 74.0), FVector(70.0, 70.0, 4.0));
				B.Add(Cylinder, FLinearColor(0.92f, 0.92f, 0.90f), At + FVector(10.0, -8.0, 80.0), FVector(9.0, 9.0, 8.0));
				B.Add(Cylinder, FLinearColor(0.70f, 0.62f, 0.40f), At + FVector(-12.0, 10.0, 81.0), FVector(6.0, 6.0, 10.0));
				for (int32 c = 0; c < 3; ++c)
				{
					const double Turn = c * 120.0 + 40.0 * (Random.Unit() - 0.5);
					Chair(B, At + FRotator(0.0, Turn, 0.0).RotateVector(FVector(-55.0, 0.0, 0.0)), Turn);
				}
			}
		}
		// Palms in brass tubs by the stage.
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const FVector Tub(1060.0, Side * 560.0, 0.0);
			B.Add(Cylinder, Brass(), Tub + FVector(0.0, 0.0, 30.0), FVector(60.0, 60.0, 60.0));
			for (int32 i = 0; i < 7; ++i)
			{
				const double Turn = i * 51.0;
				B.Add(Cube, FLinearColor(0.08f, 0.22f, 0.07f), Tub + FRotator(0.0, Turn, 0.0).RotateVector(FVector(40.0, 0.0, 150.0)), FVector(90.0, 22.0, 3.0), -30.0, Turn, 0.0);
			}
			B.Add(Cylinder, FLinearColor(0.25f, 0.17f, 0.08f), Tub + FVector(0.0, 0.0, 100.0), FVector(8.0, 8.0, 90.0));
		}
		// Chandeliers: a gilt ring of globes under a brass stem.
		const FVector2D Chandeliers[4] = { FVector2D(260.0, -260.0), FVector2D(260.0, 260.0), FVector2D(700.0, 0.0), FVector2D(1000.0, 0.0) };
		for (const FVector2D& C : Chandeliers)
		{
			const FVector At(C.X, C.Y, H - 130.0);
			B.Add(Cylinder, Brass(), FVector(C.X, C.Y, H - 65.0), FVector(3.0, 3.0, 130.0));
			B.Add(Sphere, Gilt(), At, FVector(26.0, 26.0, 20.0));
			for (int32 i = 0; i < 6; ++i)
			{
				const double Turn = i * 60.0;
				const FVector Globe = At + FRotator(0.0, Turn, 0.0).RotateVector(FVector(52.0, 0.0, 14.0));
				B.Add(Cylinder, Gilt(), At + FRotator(0.0, Turn, 0.0).RotateVector(FVector(26.0, 0.0, 4.0)), FVector(3.0, 3.0, 54.0), 80.0, Turn, 0.0);
				B.Add(Sphere, LampGlow(), Globe, FVector(16.0, 16.0, 16.0));
			}
			B.Add(Sphere, LampGlow(), At + FVector(0.0, 0.0, -14.0), FVector(22.0, 22.0, 22.0));
			if (Lamps) { Lamps->Add(At); }
		}
	}
}
