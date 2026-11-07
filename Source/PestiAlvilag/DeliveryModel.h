// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PersonKit.h"

/**
 * Mission 6, "Kifutófiú": Varga Lajos's grocery (gyarmatáru) on Népszínház
 * utca, where the player becomes his errand boy, and the three places he
 * rides the parcels to, each dressed up as a little scene of its own:
 *
 *  - the Oppenheim café on Népszínház utca: the head waiter at the door,
 *    café tables and bentwood chairs on the pavement, a chalk menu board,
 *    a crate of soda siphons and a milk churn;
 *  - Keleti station's taxi stand: a lady traveller with her steamer trunk,
 *    suitcases and hat boxes, and a porter with his hand cart;
 *  - Népszínház utca 7: Dr. Halász's maid at the tenement gate, between two
 *    bay trees in green tubs, a doormat, and the doctor's brass plate and
 *    bell push on the wall.
 *
 * The people's outfits are here too, and Mazsola, the errand boys' leader,
 * who waits at the shop with his own shop bicycle.
 *
 * Each scene is laid out in its own frame: the origin on the wall foot (or
 * the kerb sign) in the middle, X out towards the street, Y along the wall
 * to the right, Z up, in centimetres. The city spots (X east, Y south) and
 * yaws come from the street furniture data. Header-only so it can be built
 * and looked at outside the engine.
 */
namespace DeliveryModel
{
	enum class EPlace : uint8 { Shop, Cafe, Station, Doctor, Count };

	/** Where each scene's frame sits in the city, and which way its X points (degrees). */
	inline FVector2D PlaceAt(EPlace Place)
	{
		switch (Place)
		{
		case EPlace::Cafe: return FVector2D(74990.0, 27101.0);
		case EPlace::Station: return FVector2D(101266.0, -37678.0);
		case EPlace::Doctor: return FVector2D(21760.0, 9062.0);
		default: return FVector2D(65514.0, 23668.0);
		}
	}
	inline double PlaceYaw(EPlace Place)
	{
		switch (Place)
		{
		case EPlace::Cafe: return 108.3;
		case EPlace::Station: return 90.0;
		case EPlace::Doctor: return 109.2;
		default: return 108.4;
		}
	}

	/** Who takes the parcel (or gives them out), in the scene's frame: where they stand and which way they face. */
	inline FVector PersonAt(EPlace Place)
	{
		switch (Place)
		{
		case EPlace::Cafe: return FVector(70.0, -60.0, 0.0);
		case EPlace::Station: return FVector(-120.0, 60.0, 0.0);
		case EPlace::Doctor: return FVector(55.0, 0.0, 0.0);
		default: return FVector(70.0, 0.0, 0.0);
		}
	}
	inline double PersonYaw(EPlace Place) { return Place == EPlace::Station ? 20.0 : 0.0; }

	/** Mazsola and his bicycle at the shop (the bicycle's yaw is along the wall). */
	inline FVector MazsolaAt() { return FVector(120.0, 230.0, 0.0); }
	inline double MazsolaYaw() { return -150.0; }
	inline FVector MazsolaBikeAt() { return FVector(70.0, 330.0, 0.0); }
	inline double MazsolaBikeYaw() { return 90.0; }
	/** The porter beside the lady at the station. */
	inline FVector PorterAt() { return FVector(-60.0, 210.0, 0.0); }
	inline double PorterYaw() { return -100.0; }

	/** A scene's point in the city (Z left at 0: the caller puts it on the ground). */
	inline FVector ToCity(EPlace Place, const FVector& Local)
	{
		const double Yaw = FMath::DegreesToRadians(PlaceYaw(Place));
		const double C = FMath::Cos(Yaw);
		const double S = FMath::Sin(Yaw);
		const FVector2D At = PlaceAt(Place);
		return FVector(At.X + Local.X * C - Local.Y * S, At.Y + Local.X * S + Local.Y * C, Local.Z);
	}

	// The people.
	inline PersonKit::FOutfit VargaOutfit()
	{
		PersonKit::FOutfit O;
		O.Height = 172.f;
		O.Build = 1.15f;
		O.Hat = PersonKit::EHat::None;
		O.Top = PersonKit::ETop::Overcoat;
		// A grocer's long dust coat over his waistcoat.
		O.Coat = FLinearColor(0.42f, 0.40f, 0.34f);
		O.bTie = true;
		O.Tie = FLinearColor(0.10f, 0.08f, 0.06f);
		O.bMoustache = true;
		O.Hair = FLinearColor(0.30f, 0.29f, 0.27f);
		O.Trousers = FLinearColor(0.10f, 0.09f, 0.09f);
		return O;
	}

	inline PersonKit::FOutfit MazsolaOutfit()
	{
		PersonKit::FOutfit O;
		O.Height = 160.f;
		O.Build = 0.88f;
		O.Hat = PersonKit::EHat::FlatCap;
		O.HatColor = FLinearColor(0.20f, 0.16f, 0.11f);
		O.Top = PersonKit::ETop::Jacket;
		O.Coat = FLinearColor(0.18f, 0.20f, 0.14f);
		O.bTie = false;
		O.bScarf = true;
		O.Scarf = FLinearColor(0.75f, 0.55f, 0.12f);
		O.Shirt = FLinearColor(0.70f, 0.66f, 0.58f);
		O.Trousers = FLinearColor(0.16f, 0.13f, 0.10f);
		O.Hair = FLinearColor(0.32f, 0.18f, 0.08f);
		O.Skin = FLinearColor(0.76f, 0.55f, 0.42f);
		return O;
	}

	inline PersonKit::FOutfit WaiterOutfit()
	{
		PersonKit::FOutfit O;
		O.Height = 176.f;
		O.Hat = PersonKit::EHat::None;
		O.Top = PersonKit::ETop::Waistcoat;
		O.Waistcoat = FLinearColor(0.03f, 0.03f, 0.035f);
		O.Shirt = FLinearColor(0.90f, 0.89f, 0.85f);
		O.bTie = true;
		O.Tie = FLinearColor(0.02f, 0.02f, 0.02f);
		O.bApron = true;
		O.Apron = FLinearColor(0.92f, 0.92f, 0.90f);
		O.bMoustache = true;
		O.Hair = FLinearColor(0.05f, 0.04f, 0.035f);
		return O;
	}

	inline PersonKit::FOutfit LadyOutfit()
	{
		PersonKit::FOutfit O;
		O.bWoman = true;
		O.Height = 166.f;
		O.Hat = PersonKit::EHat::Cloche;
		O.HatColor = FLinearColor(0.30f, 0.06f, 0.10f);
		O.HatBand = FLinearColor(0.08f, 0.02f, 0.03f);
		O.Top = PersonKit::ETop::Overcoat;
		O.Coat = FLinearColor(0.36f, 0.30f, 0.24f);
		O.bScarf = true;
		O.Scarf = FLinearColor(0.70f, 0.66f, 0.60f);
		O.Held = PersonKit::EHeld::Umbrella;
		O.HeldColor = FLinearColor(0.25f, 0.13f, 0.06f);
		O.Hair = FLinearColor(0.22f, 0.12f, 0.06f);
		O.Shoes = FLinearColor(0.12f, 0.05f, 0.03f);
		return O;
	}

	inline PersonKit::FOutfit PorterOutfit()
	{
		PersonKit::FOutfit O;
		O.Height = 175.f;
		O.Build = 1.1f;
		O.Hat = PersonKit::EHat::FlatCap;
		O.HatColor = FLinearColor(0.35f, 0.06f, 0.05f);
		O.Top = PersonKit::ETop::Waistcoat;
		O.Waistcoat = FLinearColor(0.10f, 0.12f, 0.20f);
		O.Shirt = FLinearColor(0.62f, 0.60f, 0.55f);
		O.bTie = false;
		O.bMoustache = true;
		O.bBoots = true;
		O.Trousers = FLinearColor(0.10f, 0.12f, 0.20f);
		return O;
	}

	inline PersonKit::FOutfit MaidOutfit()
	{
		PersonKit::FOutfit O;
		O.bWoman = true;
		O.Height = 160.f;
		O.Hat = PersonKit::EHat::None;
		O.Top = PersonKit::ETop::Dress;
		O.Coat = FLinearColor(0.04f, 0.04f, 0.05f);
		O.bApron = true;
		O.Apron = FLinearColor(0.93f, 0.93f, 0.91f);
		O.Hair = FLinearColor(0.45f, 0.30f, 0.12f);
		return O;
	}

	/** Whoever stands at a place: Varga at the shop, the waiter, the lady traveller, the maid. */
	inline PersonKit::FOutfit OutfitAt(EPlace Place)
	{
		switch (Place)
		{
		case EPlace::Cafe: return WaiterOutfit();
		case EPlace::Station: return LadyOutfit();
		case EPlace::Doctor: return MaidOutfit();
		default: return VargaOutfit();
		}
	}

	// Colours.
	inline FLinearColor Wood() { return FLinearColor(0.34f, 0.22f, 0.12f); }
	inline FLinearColor DarkWood() { return FLinearColor(0.12f, 0.07f, 0.04f); }
	inline FLinearColor Marble() { return FLinearColor(0.78f, 0.76f, 0.72f); }
	inline FLinearColor CastIron() { return FLinearColor(0.03f, 0.03f, 0.03f); }
	inline FLinearColor Chalkboard() { return FLinearColor(0.035f, 0.05f, 0.04f); }
	inline FLinearColor Chalk() { return FLinearColor(0.75f, 0.75f, 0.72f); }
	inline FLinearColor SiphonGlass() { return FLinearColor(0.20f, 0.42f, 0.46f); }
	inline FLinearColor Pewter() { return FLinearColor(0.50f, 0.50f, 0.50f); }
	inline FLinearColor Tin() { return FLinearColor(0.58f, 0.58f, 0.56f); }
	inline FLinearColor TrunkCanvas() { return FLinearColor(0.20f, 0.30f, 0.20f); }
	inline FLinearColor Leather() { return FLinearColor(0.28f, 0.13f, 0.05f); }
	inline FLinearColor Brass() { return FLinearColor(0.60f, 0.43f, 0.15f); }
	inline FLinearColor HatBoxPaper() { return FLinearColor(0.70f, 0.55f, 0.55f); }
	inline FLinearColor TubGreen() { return FLinearColor(0.06f, 0.20f, 0.10f); }
	inline FLinearColor BayLeaves() { return FLinearColor(0.08f, 0.17f, 0.06f); }
	inline FLinearColor Coir() { return FLinearColor(0.42f, 0.30f, 0.15f); }

	/** A round thing turned on a lathe at At: a profile of (height, radius) pairs, going up. */
	inline void Lathe(PersonKit::FPiece& P, const FLinearColor& Color, const FVector& At, const TArray<FVector2D>& Profile, int32 Sides,
		bool bCapBottom, bool bCapTop)
	{
		TArray<PersonKit::FRing> Rings;
		for (const FVector2D& Step : Profile)
		{
			Rings.Add({ At + FVector(0.0, 0.0, Step.X), FVector(Step.Y, 0.0, 0.0), FVector(0.0, Step.Y, 0.0) });
		}
		PersonKit::Loft(P, Color, Rings, Sides, bCapBottom, bCapTop);
	}

	/** Copies a prop built round its own origin into P, turned by Yaw (degrees) and moved to At. */
	inline void PutProp(PersonKit::FPiece& P, const PersonKit::FPiece& Prop, const FVector& At, double Yaw)
	{
		const double R = FMath::DegreesToRadians(Yaw);
		const double C = FMath::Cos(R);
		const double S = FMath::Sin(R);
		auto Turn = [C, S](const FVector& V) { return FVector(V.X * C - V.Y * S, V.X * S + V.Y * C, V.Z); };
		for (const PersonKit::FSection& From : Prop.Sections)
		{
			PersonKit::FSection& Into = P.Section(From.Color);
			const int32 First = Into.Vertices.Num();
			for (int32 i = 0; i < From.Vertices.Num(); ++i)
			{
				Into.Vertices.Add(Turn(From.Vertices[i]) + At - P.Pivot);
				Into.Normals.Add(Turn(From.Normals[i]));
			}
			for (const int32 Index : From.Triangles)
			{
				Into.Triangles.Add(First + Index);
			}
		}
	}

	// The props, each round its own origin on the ground, its front towards +X.

	/** A café table: a round marble top on a cast-iron foot. */
	inline void CafeTable(PersonKit::FPiece& P)
	{
		Lathe(P, CastIron(), FVector::ZeroVector, { FVector2D(0.0, 22.0), FVector2D(2.0, 20.0), FVector2D(4.0, 6.0), FVector2D(10.0, 3.0), FVector2D(68.0, 2.5), FVector2D(70.0, 8.0) }, 12, true, true);
		Lathe(P, Marble(), FVector::ZeroVector, { FVector2D(70.0, 30.0), FVector2D(73.0, 30.0) }, 18, true, true);
	}

	/** A bentwood café chair, facing +X: a round seat, four legs, a looped back. */
	inline void Chair(PersonKit::FPiece& P)
	{
		using namespace PersonKit;
		Lathe(P, DarkWood(), FVector(0.0, 0.0, 44.0), { FVector2D(0.0, 20.0), FVector2D(2.5, 20.0) }, 14, true, true);
		for (int32 k = 0; k < 4; ++k)
		{
			const double A = PI * 0.25 + k * PI * 0.5;
			const FVector Top(FMath::Cos(A) * 14.0, FMath::Sin(A) * 14.0, 44.0);
			Rod(P, DarkWood(), Top, FVector(FMath::Cos(A) * 19.0, FMath::Sin(A) * 19.0, 0.0), 1.3, 6);
		}
		Lathe(P, DarkWood(), FVector(0.0, 0.0, 22.0), { FVector2D(0.0, 15.5), FVector2D(1.2, 15.5) }, 14, false, false);
		// The back: two uprights curving into a hoop.
		for (const double Side : { -1.0, 1.0 })
		{
			Rod(P, DarkWood(), FVector(-14.0, 13.0 * Side, 46.0), FVector(-19.0, 15.0 * Side, 88.0), 1.2, 6);
		}
		for (int32 k = 0; k < 10; ++k)
		{
			const double A0 = PI * k / 10.0;
			const double A1 = PI * (k + 1) / 10.0;
			Rod(P, DarkWood(), FVector(-19.0 - FMath::Sin(A0) * 2.0, -15.0 * FMath::Cos(A0), 88.0 + FMath::Sin(A0) * 10.0),
				FVector(-19.0 - FMath::Sin(A1) * 2.0, -15.0 * FMath::Cos(A1), 88.0 + FMath::Sin(A1) * 10.0), 1.2, 6);
		}
	}

	/** A chalk menu board on an easel, the writing facing +X. */
	inline void MenuBoard(PersonKit::FPiece& P)
	{
		using namespace PersonKit;
		for (const double Side : { -1.0, 1.0 })
		{
			Rod(P, Wood(), FVector(8.0, 26.0 * Side, 0.0), FVector(-2.0, 24.0 * Side, 110.0), 1.6, 6);
		}
		Rod(P, Wood(), FVector(-30.0, 0.0, 0.0), FVector(-2.0, 0.0, 104.0), 1.4, 6);
		Block(P, Wood(), FVector(2.5, 0.0, 70.0), FVector(1.2, 27.0, 33.0));
		Block(P, Chalkboard(), FVector(3.3, 0.0, 70.0), FVector(0.8, 24.0, 30.0));
		// Lines of chalk writing: the day's offer.
		const double Lines[] = { 24.0, 26.0, 18.0, 22.0, 14.0 };
		for (int32 k = 0; k < 5; ++k)
		{
			Block(P, Chalk(), FVector(4.2, -2.0 + (k % 2) * 2.0, 90.0 - k * 9.0), FVector(0.2, Lines[k] * 0.8, k == 0 ? 1.6 : 0.9));
		}
	}

	/** A wooden crate of six soda siphons (szódásüveg), blue-green glass with pewter heads. */
	inline void SiphonCrate(PersonKit::FPiece& P)
	{
		using namespace PersonKit;
		Block(P, Wood(), FVector(0.0, 0.0, 1.0), FVector(17.0, 25.0, 1.0));
		for (const double Side : { -1.0, 1.0 })
		{
			Block(P, Wood(), FVector(16.0 * Side, 0.0, 9.0), FVector(1.0, 25.0, 8.0));
			Block(P, Wood(), FVector(0.0, 24.0 * Side, 9.0), FVector(17.0, 1.0, 8.0));
		}
		for (int32 i = 0; i < 6; ++i)
		{
			const FVector At(-8.0 + (i % 2) * 16.0, -16.0 + (i / 2) * 16.0, 2.0);
			Lathe(P, SiphonGlass(), At, { FVector2D(0.0, 5.0), FVector2D(22.0, 5.0), FVector2D(25.0, 3.0) }, 10, true, false);
			Lathe(P, Pewter(), At, { FVector2D(25.0, 3.0), FVector2D(31.0, 2.6), FVector2D(32.0, 1.0) }, 8, false, true);
			Rod(P, Pewter(), At + FVector(0.0, 0.0, 30.0), At + FVector(6.0, 0.0, 29.0), 0.8, 4);
		}
	}

	/** A tin milk churn. */
	inline void MilkChurn(PersonKit::FPiece& P)
	{
		Lathe(P, Tin(), FVector::ZeroVector, { FVector2D(0.0, 16.0), FVector2D(40.0, 16.0), FVector2D(50.0, 8.0), FVector2D(56.0, 7.0), FVector2D(58.0, 9.0), FVector2D(61.0, 0.5) }, 14, true, true);
		Lathe(P, Pewter(), FVector::ZeroVector, { FVector2D(12.0, 16.4), FVector2D(14.0, 16.4) }, 14, false, false);
	}

	/** A steamer trunk lying on its back, with leather corners and brass locks. */
	inline void Trunk(PersonKit::FPiece& P)
	{
		using namespace PersonKit;
		Block(P, TrunkCanvas(), FVector(0.0, 0.0, 28.0), FVector(28.0, 45.0, 28.0));
		for (const double Z : { 8.0, 48.0 })
		{
			Block(P, Wood(), FVector(0.0, 0.0, Z), FVector(28.6, 45.6, 2.0));
		}
		for (const double Y : { -30.0, 0.0, 30.0 })
		{
			Block(P, Wood(), FVector(0.0, Y, 28.0), FVector(28.6, 2.0, 28.6));
		}
		for (const double Y : { -18.0, 18.0 })
		{
			Block(P, Brass(), FVector(28.8, Y, 44.0), FVector(0.6, 2.5, 3.5));
		}
		Block(P, Leather(), FVector(0.0, 0.0, 57.0), FVector(3.0, 9.0, 1.0));
	}

	/** A leather suitcase standing on its end, its handle up. */
	inline void Suitcase(PersonKit::FPiece& P, double Scale = 1.0)
	{
		using namespace PersonKit;
		Block(P, Leather(), FVector(0.0, 0.0, 22.0 * Scale), FVector(9.0, 32.0 * Scale, 22.0 * Scale));
		Block(P, DarkWood(), FVector(0.0, 0.0, 22.0 * Scale), FVector(9.4, 32.4 * Scale, 1.0));
		for (const double Y : { -6.0, 6.0 })
		{
			Rod(P, DarkWood(), FVector(0.0, Y * Scale, 44.0 * Scale), FVector(0.0, Y * Scale, 48.0 * Scale), 0.8, 4);
		}
		Rod(P, DarkWood(), FVector(0.0, -6.0 * Scale, 48.0 * Scale), FVector(0.0, 6.0 * Scale, 48.0 * Scale), 1.0, 6);
		for (const double Y : { -22.0, 22.0 })
		{
			Block(P, Brass(), FVector(9.6, Y * Scale, 40.0 * Scale), FVector(0.4, 2.0, 2.0));
		}
	}

	/** A round hat box tied with a ribbon. */
	inline void HatBox(PersonKit::FPiece& P, double Radius = 20.0)
	{
		Lathe(P, HatBoxPaper(), FVector::ZeroVector, { FVector2D(0.0, Radius), FVector2D(20.0, Radius) }, 16, true, false);
		Lathe(P, HatBoxPaper() * 0.8f, FVector::ZeroVector, { FVector2D(19.0, Radius + 0.6), FVector2D(23.0, Radius + 0.6) }, 16, false, true);
		PersonKit::Block(P, FLinearColor(0.35f, 0.05f, 0.08f), FVector(0.0, 0.0, 11.6), FVector(Radius + 0.8, 1.2, 11.6));
	}

	/** A porter's two-wheeled hand cart, its handles towards -X. */
	inline void PorterCart(PersonKit::FPiece& P)
	{
		using namespace PersonKit;
		for (const double Side : { -1.0, 1.0 })
		{
			Rod(P, Wood(), FVector(40.0, 28.0 * Side, 30.0), FVector(-90.0, 24.0 * Side, 72.0), 2.2, 6);
			// A wheel: an iron tyre round wooden spokes.
			const FVector Hub(10.0, 34.0 * Side, 30.0);
			for (int32 k = 0; k < 12; ++k)
			{
				const double A0 = 2.0 * PI * k / 12.0;
				const double A1 = 2.0 * PI * (k + 1) / 12.0;
				Rod(P, CastIron(), Hub + FVector(FMath::Cos(A0) * 29.0, 0.0, FMath::Sin(A0) * 29.0), Hub + FVector(FMath::Cos(A1) * 29.0, 0.0, FMath::Sin(A1) * 29.0), 1.6, 5);
				if (k % 2 == 0)
				{
					Rod(P, Wood(), Hub, Hub + FVector(FMath::Cos(A0) * 28.0, 0.0, FMath::Sin(A0) * 28.0), 1.0, 4);
				}
			}
		}
		Rod(P, CastIron(), FVector(10.0, -36.0, 30.0), FVector(10.0, 36.0, 30.0), 1.4, 6);
		Block(P, Wood(), FVector(0.0, 0.0, 38.0), FVector(42.0, 28.0, 1.5));
		Rod(P, Wood(), FVector(-90.0, -24.0, 72.0), FVector(-90.0, 24.0, 72.0), 1.8, 6);
		Rod(P, CastIron(), FVector(-60.0, 0.0, 55.0), FVector(-60.0, 0.0, 0.0), 1.2, 5);
	}

	/** A bay tree in a green wooden tub. */
	inline void BayTree(PersonKit::FPiece& P)
	{
		using namespace PersonKit;
		Block(P, TubGreen(), FVector(0.0, 0.0, 22.0), FVector(20.0, 20.0, 22.0));
		for (const double Z : { 6.0, 38.0 })
		{
			Block(P, CastIron(), FVector(0.0, 0.0, Z), FVector(20.6, 20.6, 1.2));
		}
		Rod(P, DarkWood(), FVector(0.0, 0.0, 40.0), FVector(0.0, 0.0, 110.0), 2.0, 6);
		Blob(P, BayLeaves(), FVector(0.0, 0.0, 130.0), FVector(30.0, 30.0, 32.0), 12, 7);
	}

	/** A coir doormat. */
	inline void Doormat(PersonKit::FPiece& P)
	{
		PersonKit::Block(P, Coir(), FVector(0.0, 0.0, 0.6), FVector(30.0, 45.0, 0.6));
	}

	/** The doctor's brass plate and bell push on the wall, facing +X. */
	inline void Nameplate(PersonKit::FPiece& P)
	{
		using namespace PersonKit;
		Block(P, Brass(), FVector(0.6, 0.0, 150.0), FVector(0.6, 18.0, 11.0));
		Block(P, CastIron(), FVector(1.3, 0.0, 154.0), FVector(0.2, 13.0, 1.6));
		Block(P, CastIron(), FVector(1.3, 0.0, 147.0), FVector(0.2, 9.0, 1.0));
		Block(P, CastIron(), FVector(1.3, 0.0, 142.0), FVector(0.2, 11.0, 1.0));
		Lathe(P, Brass(), FVector(0.0, 26.0, 128.0), { FVector2D(-3.0, 0.1), FVector2D(-3.0, 3.0), FVector2D(3.0, 3.0), FVector2D(3.0, 0.1) }, 10, false, false);
		Blob(P, Marble(), FVector(2.0, 26.0, 128.0), FVector(1.5, 1.5, 1.5), 6, 3);
	}

	/** Lays out a place's props in its frame. */
	inline void BuildScene(PersonKit::FPiece& P, EPlace Kind)
	{
		auto Add = [&P](void (*Make)(PersonKit::FPiece&), const FVector& At, double Yaw)
		{
			PersonKit::FPiece Prop;
			Make(Prop);
			PutProp(P, Prop, At, Yaw);
		};
		switch (Kind)
		{
		case EPlace::Cafe:
			for (const double Y : { -170.0, 150.0 })
			{
				Add(CafeTable, FVector(170.0, Y, 0.0), 0.0);
				Add(Chair, FVector(170.0, Y - 50.0, 0.0), 90.0);
				Add(Chair, FVector(170.0, Y + 50.0, 0.0), -80.0);
				Add(Chair, FVector(215.0, Y + 8.0, 0.0), 175.0);
			}
			Add(MenuBoard, FVector(90.0, 60.0, 0.0), 10.0);
			Add(SiphonCrate, FVector(30.0, -150.0, 0.0), 0.0);
			Add(MilkChurn, FVector(25.0, -105.0, 0.0), 0.0);
			break;
		case EPlace::Station:
		{
			Add(Trunk, FVector(-140.0, -40.0, 0.0), 10.0);
			{
				PersonKit::FPiece Case;
				Suitcase(Case, 1.0);
				PutProp(P, Case, FVector(-100.0, -95.0, 0.0), 80.0);
			}
			{
				PersonKit::FPiece Case;
				Suitcase(Case, 0.8);
				PutProp(P, Case, FVector(-160.0, -130.0, 0.0), 60.0);
			}
			{
				PersonKit::FPiece Box;
				HatBox(Box, 20.0);
				PutProp(P, Box, FVector(-140.0, -40.0, 56.0), 0.0);
				PersonKit::FPiece Small;
				HatBox(Small, 15.0);
				PutProp(P, Small, FVector(-138.0, -42.0, 79.0), 30.0);
			}
			Add(PorterCart, FVector(-60.0, 300.0, 0.0), 120.0);
			break;
		}
		case EPlace::Doctor:
			for (const double Y : { -190.0, 190.0 })
			{
				Add(BayTree, FVector(30.0, Y, 0.0), 0.0);
			}
			Add(Doormat, FVector(32.0, 0.0, 0.0), 0.0);
			Add(Nameplate, FVector(0.0, 210.0, 0.0), 0.0);
			break;
		default:
			// The shop: a few sacks and crates of the day's goods are the shop's own; parcels on a crate by the door.
			{
				PersonKit::FPiece Crate;
				PersonKit::Block(Crate, Wood(), FVector(0.0, 0.0, 20.0), FVector(22.0, 30.0, 20.0));
				PutProp(P, Crate, FVector(40.0, -140.0, 0.0), 0.0);
			}
			break;
		}
	}
}
