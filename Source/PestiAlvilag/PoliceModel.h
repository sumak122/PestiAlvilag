// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PersonKit.h"

/**
 * The constables of the royal Hungarian state police on the beat in 1932
 * Pest, and the police telephone boxes they ring the station from.
 *
 * A constable is a person from the dress-up kit (PersonKit) in a dark
 * belted tunic buttoned up to a stand collar, trousers and boots, with the
 * rest of the uniform put on over it here: a tall shako with a short peak,
 * a brass badge and the red, white and green cockade, brass buttons and
 * buckle, and a sabre on the left hip. Patrols walk in pairs.
 *
 * The telephone box is a narrow cast-iron kiosk, glazed above a solid
 * dado, with a hipped roof, a round lamp on top and RENDŐRSÉG on a band
 * under the eaves; its door stands open on the pavement side and the
 * wall telephone (a wooden case with a crank, a horn mouthpiece and two
 * bells) hangs inside on the back wall.
 *
 * Header-only like the other models, so the offline preview builds the same
 * shapes. Centimetres, X forward, Y right, Z up; the constable's pieces are
 * in his body's frame (feet at Z = 0), the box's with its floor at Z = 0
 * and its door towards +X.
 */
namespace PoliceModel
{
	using namespace PersonKit;

	const FLinearColor Tunic(0.045f, 0.055f, 0.050f);
	const FLinearColor Shako(0.025f, 0.025f, 0.028f);
	const FLinearColor Leather(0.02f, 0.017f, 0.015f);
	const FLinearColor Brass(0.62f, 0.45f, 0.16f);
	const FLinearColor Steel(0.35f, 0.36f, 0.38f);
	const FLinearColor Bakelite(0.03f, 0.03f, 0.03f);
	const FLinearColor CockadeRed(0.55f, 0.04f, 0.04f);
	const FLinearColor CockadeWhite(0.85f, 0.85f, 0.82f);
	const FLinearColor CockadeGreen(0.05f, 0.30f, 0.10f);

	const FLinearColor BoxIron(0.06f, 0.11f, 0.08f);
	const FLinearColor BoxRoof(0.04f, 0.06f, 0.05f);
	const FLinearColor BoxSign(0.85f, 0.83f, 0.74f);
	const FLinearColor BoxGlass(0.42f, 0.50f, 0.52f);
	const FLinearColor BoxFloor(0.20f, 0.19f, 0.17f);
	const FLinearColor Wood(0.30f, 0.16f, 0.07f);
	const FLinearColor Lamp(0.95f, 0.90f, 0.75f);

	/** The box's footprint (half the width and depth) and its height to the eaves (cm). */
	constexpr double BoxHalf = 58.0;
	constexpr double BoxEaves = 236.0;
	/** Where a constable stands to use the telephone, in the box's frame, facing -X. */
	const FVector PhoneStand(4.0, 0.0, 0.0);
	/** The sign band under the eaves: its middle height and how far out from the box's middle. */
	constexpr double SignZ = 224.0;
	constexpr double SignOut = BoxHalf + 1.5;

	/** A constable: the kit's tunic, trousers and boots; the shako and the rest come from BuildUniform. */
	inline FOutfit Outfit(FRandomStream& Random)
	{
		FOutfit O;
		O.Height = Random.FRandRange(174.f, 188.f);
		O.Build = Random.FRandRange(1.0f, 1.12f);
		O.Hat = EHat::None;
		O.Top = ETop::Jacket;
		O.Coat = Tunic;
		// Buttoned up to the collar: the shirt's opening in the colour of the tunic.
		O.Shirt = Tunic;
		O.bTie = false;
		O.Trousers = Tunic;
		O.bBoots = true;
		O.Shoes = Leather;
		O.bMoustache = Random.FRand() < 0.6f;
		O.Hair = Random.FRand() < 0.5f ? FLinearColor(0.03f, 0.025f, 0.02f) : FLinearColor(0.10f, 0.07f, 0.05f);
		const FLinearColor Skins[] = { FLinearColor(0.72f, 0.50f, 0.38f), FLinearColor(0.78f, 0.58f, 0.46f), FLinearColor(0.65f, 0.45f, 0.33f) };
		O.Skin = Skins[Random.RandRange(0, 2)];
		return O;
	}

	/** The shako, collar, brass and sabre, for the body piece of a constable dressed in Outfit. */
	inline void BuildUniform(const FOutfit& O, FPiece& B)
	{
		const double H = O.Height / 178.0;
		const double G = O.Build;
		auto Z = [H](double Cm) { return Cm * H; };
		const FVector HeadAt(1.5, 0.0, Z(165.5));
		const FVector Crown = HeadAt + FVector(-0.5, 0.0, 7.5);

		// The shako: a stiff drum, a little narrower at the top and leaning
		// back, on a leather band, with a flat top.
		const double Base = Crown.Z - 4.0;
		Loft(B, Shako, {
			Flat(Crown.X, Base, 10.6, 9.6),
			Flat(Crown.X - 0.6, Base + 9.0, 10.3, 9.3),
			Flat(Crown.X - 1.6, Base + 17.0, 9.7, 8.7),
		}, 16, true, true);
		Loft(B, Leather, { Flat(Crown.X, Base - 0.3, 10.9, 9.9), Flat(Crown.X - 0.1, Base + 2.4, 10.8, 9.8) }, 16, true, true);
		// The top's rim, and the short peak over the eyes.
		Loft(B, Leather, { Flat(Crown.X - 1.6, Base + 16.2, 9.9, 8.9), Flat(Crown.X - 1.6, Base + 17.3, 9.9, 8.9) }, 16, true, true);
		Loft(B, Leather, {
			{ FVector(Crown.X + 9.6, 0.0, Base + 0.6), FVector(0.0, 0.0, 0.35), FVector(0.0, 8.5, 0.0) },
			{ FVector(Crown.X + 13.5, 0.0, Base - 1.6), FVector(0.0, 0.0, 0.35), FVector(0.0, 7.0, 0.0) },
			{ FVector(Crown.X + 15.5, 0.0, Base - 2.8), FVector(0.0, 0.0, 0.35), FVector(0.0, 4.5, 0.0) },
		}, 8, true, true);
		// The chin strap, brass, along the band.
		Loft(B, Brass, { Flat(Crown.X + 0.1, Base + 2.6, 10.85, 9.85), Flat(Crown.X + 0.1, Base + 3.2, 10.85, 9.85) }, 16, true, true);
		// The badge on the front, the cockade above it.
		const double FrontX = Crown.X + 10.0;
		Blob(B, Brass, FVector(FrontX - 0.8, 0.0, Base + 8.0), FVector(1.0, 3.4, 4.2), 10, 5);
		Blob(B, CockadeGreen, FVector(FrontX - 1.8, 0.0, Base + 14.0), FVector(0.8, 2.2, 2.2), 10, 4);
		Blob(B, CockadeWhite, FVector(FrontX - 1.4, 0.0, Base + 14.0), FVector(0.8, 1.5, 1.5), 10, 4);
		Blob(B, CockadeRed, FVector(FrontX - 1.0, 0.0, Base + 14.0), FVector(0.8, 0.8, 0.8), 8, 3);

		// A stand collar round the neck.
		Loft(B, Tunic, { Flat(0.3, Z(145.0), 7.4, 7.6), Flat(0.6, Z(151.0), 6.6, 6.6) }, 12, false, true);
		Loft(B, Brass, { Flat(0.6, Z(150.6), 6.7, 6.7), Flat(0.6, Z(151.2), 6.7, 6.7) }, 12, false, false);

		// Two rows of brass buttons down the front, above the belt.
		for (int32 i = 0; i < 3; ++i)
		{
			const double Bz = Z(134.0 - 9.0 * i);
			const double Front = (i == 0 ? 11.8 : 12.6) * G + 0.6;
			for (const double Side : { -4.5, 4.5 })
			{
				Blob(B, Brass, FVector(Front, Side * G, Bz), FVector(0.8, 1.1, 1.1), 6, 3);
			}
		}
		// The belt, over the tunic where it is widest, and its brass buckle.
		const double BeltX = FMath::Max(13.8 * G, 12.6 * G * G) + 0.9;
		const double BeltY = FMath::Max(17.4 * G, 16.6 * G * G) + 0.9;
		Loft(B, Leather, { Flat(0.5, Z(99.0), BeltX, BeltY), Flat(0.5, Z(105.0), BeltX, BeltY) }, 16, true, true);
		Block(B, Brass, FVector(BeltX + 0.9, 0.0, Z(102.0)), FVector(0.5, 3.0, 2.6));
		// A strap from the right shoulder down to the belt on the left (cross-belt).
		Loft(B, Leather, {
			{ FVector(6.0, 13.0 * G, Z(141.0)), FVector(0.5, 0.0, 0.0), FVector(0.0, 2.2, 0.0) },
			{ FVector(12.8 * G + 0.9, 0.0, Z(122.0)), FVector(0.5, 0.0, 0.0), FVector(0.0, 2.2, 0.0) },
			{ FVector(BeltX - 0.6, -11.0 * G, Z(104.0)), FVector(0.5, 0.0, 0.0), FVector(0.0, 2.2, 0.0) },
		}, 4, true, true, PI * 0.25);

		// The sabre, in its scabbard on the left hip, sloping back, the hilt forward.
		const FVector Hilt(5.0, -17.5 * G, Z(100.0));
		const FVector Tip(-16.0, -20.0 * G, Z(28.0));
		Rod(B, Steel, Hilt, Tip, 1.5, 6);
		Rod(B, Steel, Tip, Tip + (Tip - Hilt).GetSafeNormal() * 2.5, 1.9, 6);
		Rod(B, Brass, Hilt, Hilt + FVector(4.5, 0.0, 9.0), 1.3, 6);
		Rod(B, Brass, Hilt + FVector(4.5, 0.0, 9.0), Hilt + FVector(9.5, 0.0, 5.0), 0.6, 4);
		Blob(B, Brass, Hilt + FVector(1.0, 0.0, 1.0), FVector(2.6, 1.0, 1.8), 8, 3);
		// The frog that hangs it from the belt.
		Rod(B, Leather, Hilt + FVector(-2.0, 1.0, 2.0), FVector(2.0, -16.0 * G, Z(104.0)), 0.9, 4);
	}

	/** The shoulder, elbow and hand of an arm (ArmLeft or ArmRight) of a man dressed in O, in his body's frame, as PersonKit builds them. */
	inline void ArmJoints(const FOutfit& O, EPiece Arm, FVector& OutElbow, FVector& OutHand)
	{
		const double H = O.Height / 178.0;
		const double Side = Arm == ArmLeft ? -1.0 : 1.0;
		const FVector S(0.0, 19.5 * O.Build * Side, 138.0 * H);
		OutElbow = FVector(S.X + 1.1, S.Y + 2.7 * Side, S.Z - ElbowDrop * H);
		OutHand = FVector(S.X + 2.5, S.Y + 5.0 * Side, S.Z - 56.5 * H);
	}

	/** A short wooden truncheon in the right hand, for the right forearm (its pivot at the elbow). */
	inline void BuildTruncheon(const FOutfit& O, FPiece& Forearm)
	{
		FVector Elbow, Hand;
		ArmJoints(O, ArmRight, Elbow, Hand);
		Forearm.Pivot = Elbow;
		// Gripped at the end, the rest of it along the line of the forearm and on past the fist.
		const FVector Along = (Hand - Elbow).GetSafeNormal();
		Rod(Forearm, Wood, Hand - Along * 3.0, Hand + Along * 38.0 + FVector(4.0, 0.0, 0.0), 1.8, 6);
		Rod(Forearm, Leather, Hand - Along * 4.5, Hand - Along * 2.5, 2.1, 6);
	}

	/** The telephone's earpiece held in the left hand, for the left forearm (its pivot at the elbow). */
	inline void BuildEarpiece(const FOutfit& O, FPiece& Forearm)
	{
		FVector Elbow, Hand;
		ArmJoints(O, ArmLeft, Elbow, Hand);
		Forearm.Pivot = Elbow;
		const FVector Along = (Hand - Elbow).GetSafeNormal();
		Rod(Forearm, Bakelite, Hand - Along * 2.0, Hand + Along * 7.0, 1.6, 6);
		Loft(Forearm, Bakelite, { Flat(Hand.X, Hand.Z + 7.0 * Along.Z, 1.6, 1.6, Hand.Y), Flat(Hand.X, Hand.Z + 10.0 * Along.Z, 3.4, 3.4, Hand.Y) }, 10, false, true);
	}

	/** The telephone box (see the namespace comment). */
	inline void BuildBox(FPiece& P)
	{
		const double Hf = BoxHalf;
		const double Post = 4.0;
		// A stone step, the floor, and the four corner posts.
		Block(P, FLinearColor(0.42f, 0.40f, 0.36f), FVector(0.0, 0.0, 5.0), FVector(Hf + 6.0, Hf + 6.0, 5.0));
		Block(P, BoxFloor, FVector(0.0, 0.0, 10.5), FVector(Hf, Hf, 0.5));
		for (const double Sx : { -1.0, 1.0 })
		{
			for (const double Sy : { -1.0, 1.0 })
			{
				const FVector At(Sx * (Hf - Post), Sy * (Hf - Post), 0.0);
				Block(P, BoxIron, At + FVector(0.0, 0.0, 10.0 + (BoxEaves - 10.0) * 0.5), FVector(Post, Post, (BoxEaves - 10.0) * 0.5));
				// A little capital where the post meets the frieze.
				Block(P, BoxIron, At + FVector(0.0, 0.0, BoxEaves - 26.0), FVector(Post + 1.2, Post + 1.2, 1.2));
			}
		}
		// Three walls: a solid dado up to 100 cm with a raised panel, then
		// glazing bars and panes up to the frieze. The front (+X) is the doorway.
		const double Dado = 100.0;
		const double Frieze = BoxEaves - 24.0;
		auto Wall = [&](const FVector& Mid, const FVector& Along, const FVector& Out)
		{
			const double Len = Hf - Post;
			const FVector Half = Along * Len + Out * 1.5;
			auto Abs = [](const FVector& V) { return FVector(FMath::Abs(V.X), FMath::Abs(V.Y), FMath::Abs(V.Z)); };
			Block(P, BoxIron, Mid + FVector(0.0, 0.0, 10.0 + (Dado - 10.0) * 0.5), Abs(Half) + FVector(0.0, 0.0, (Dado - 10.0) * 0.5));
			Block(P, BoxIron, Mid + Out * 2.0 + FVector(0.0, 0.0, 55.0), Abs(Along * (Len - 10.0) + Out * 0.8) + FVector(0.0, 0.0, 30.0));
			// Glass, thin and set back, then the bars over it.
			Block(P, BoxGlass, Mid + FVector(0.0, 0.0, (Dado + Frieze) * 0.5), Abs(Along * Len + Out * 0.4) + FVector(0.0, 0.0, (Frieze - Dado) * 0.5));
			Block(P, BoxIron, Mid + Out * 0.6 + FVector(0.0, 0.0, Dado + 1.5), Abs(Along * Len + Out * 1.2) + FVector(0.0, 0.0, 2.5));
			for (const double S : { -1.0 / 3.0, 1.0 / 3.0 })
			{
				Block(P, BoxIron, Mid + Along * (S * Len) + Out * 0.6 + FVector(0.0, 0.0, (Dado + Frieze) * 0.5), Abs(Along * 1.2 + Out * 1.2) + FVector(0.0, 0.0, (Frieze - Dado) * 0.5));
			}
			Block(P, BoxIron, Mid + Out * 0.6 + FVector(0.0, 0.0, Dado + (Frieze - Dado) * 0.62), Abs(Along * Len + Out * 1.2) + FVector(0.0, 0.0, 1.0));
		};
		Wall(FVector(-Hf + 1.5, 0.0, 0.0), FVector(0.0, 1.0, 0.0), FVector(-1.0, 0.0, 0.0));
		Wall(FVector(0.0, -Hf + 1.5, 0.0), FVector(1.0, 0.0, 0.0), FVector(0.0, -1.0, 0.0));
		Wall(FVector(0.0, Hf - 1.5, 0.0), FVector(1.0, 0.0, 0.0), FVector(0.0, 1.0, 0.0));
		// The frieze all round, with the sign band on it (the words are put on in the game).
		Block(P, BoxIron, FVector(0.0, 0.0, BoxEaves - 12.0), FVector(Hf, Hf, 12.0));
		for (const FVector& Out : { FVector(1.0, 0.0, 0.0), FVector(0.0, 1.0, 0.0), FVector(-1.0, 0.0, 0.0), FVector(0.0, -1.0, 0.0) })
		{
			const FVector Along(-Out.Y, Out.X, 0.0);
			auto Abs = [](const FVector& V) { return FVector(FMath::Abs(V.X), FMath::Abs(V.Y), FMath::Abs(V.Z)); };
			Block(P, BoxSign, Out * (Hf + 0.4) + FVector(0.0, 0.0, SignZ), Abs(Along * (Hf - 6.0) + Out * 0.6) + FVector(0.0, 0.0, 7.0));
		}
		// The roof: an overhanging cornice, a low hipped roof and the lamp.
		Block(P, BoxIron, FVector(0.0, 0.0, BoxEaves + 2.0), FVector(Hf + 7.0, Hf + 7.0, 2.0));
		Loft(P, BoxRoof, {
			{ FVector(0.0, 0.0, BoxEaves + 4.0), FVector(Hf + 6.0, 0.0, 0.0) * FMath::Sqrt(2.0), FVector(0.0, Hf + 6.0, 0.0) * FMath::Sqrt(2.0) },
			{ FVector(0.0, 0.0, BoxEaves + 30.0), FVector(8.0, 0.0, 0.0), FVector(0.0, 8.0, 0.0) },
		}, 4, true, true, PI * 0.25);
		Rod(P, BoxIron, FVector(0.0, 0.0, BoxEaves + 28.0), FVector(0.0, 0.0, BoxEaves + 40.0), 3.0, 8);
		Blob(P, Lamp, FVector(0.0, 0.0, BoxEaves + 49.0), FVector(10.0, 10.0, 11.0), 12, 6);
		Rod(P, BoxIron, FVector(0.0, 0.0, BoxEaves + 59.0), FVector(0.0, 0.0, BoxEaves + 64.0), 2.5, 8);
		Blob(P, BoxIron, FVector(0.0, 0.0, BoxEaves + 65.0), FVector(3.0, 3.0, 3.0), 8, 4);

		// The door, standing open against the right-hand side of the doorway.
		{
			// Closed it would run from the hinge along -Y; open, it is swung out by 75 degrees.
			const FVector HingeAt(Hf - Post, Hf - Post, 0.0);
			const double DoorW = 2.0 * (Hf - Post) - 4.0;
			const double Swing = FMath::DegreesToRadians(75.0);
			const FVector DirAlong(FMath::Sin(Swing), -FMath::Cos(Swing), 0.0);
			const FVector DirOut(-DirAlong.Y, DirAlong.X, 0.0);
			const FVector DoorMid = HingeAt + DirAlong * (DoorW * 0.5);
			auto Panel = [&](double Z0, double Z1, const FLinearColor& C, double Thick, double Inset)
			{
				// A thin slab along DirAlong: lofted as a four-sided ring at each end.
				const double Half = DoorW * 0.5 - Inset;
				const FVector A = DoorMid - DirAlong * Half;
				const FVector Bv = DoorMid + DirAlong * Half;
				TArray<FRing> Rings;
				for (const FVector& End : { A, Bv })
				{
					Rings.Add({ End + FVector(0.0, 0.0, (Z0 + Z1) * 0.5), DirOut * Thick * FMath::Sqrt(2.0), FVector(0.0, 0.0, (Z1 - Z0) * 0.5) * FMath::Sqrt(2.0) });
				}
				Loft(P, C, Rings, 4, true, true, PI * 0.25);
			};
			Panel(12.0, Dado, BoxIron, 1.5, 0.0);
			Panel(Dado, Frieze, BoxGlass, 0.4, 0.0);
			Panel(Dado, Dado + 3.0, BoxIron, 1.6, 0.0);
			Panel(Frieze - 3.0, Frieze, BoxIron, 1.6, 0.0);
			Panel(Dado + (Frieze - Dado) * 0.62 - 1.0, Dado + (Frieze - Dado) * 0.62 + 1.0, BoxIron, 1.6, 0.0);
			// Its handle.
			Blob(P, Brass, DoorMid + DirAlong * (DoorW * 0.42) - DirOut * 2.5 + FVector(0.0, 0.0, 105.0), FVector(1.6, 1.6, 1.6), 6, 3);
		}

		// The telephone on the back wall: a wooden case, two bells on top,
		// the horn mouthpiece, the crank on the right and the earpiece on its hook.
		const double WallX = -Hf + 3.0;
		Block(P, Wood, FVector(WallX + 7.0, 0.0, 150.0), FVector(7.0, 13.0, 20.0));
		for (const double Sy : { -5.5, 5.5 })
		{
			Blob(P, Brass, FVector(WallX + 9.0, Sy, 174.0), FVector(4.5, 4.5, 3.5), 10, 4, 0.0);
		}
		Rod(P, Bakelite, FVector(WallX + 14.0, 0.0, 158.0), FVector(WallX + 22.0, 0.0, 160.0), 1.0, 6);
		Loft(P, Bakelite, { Flat(WallX + 22.0, 160.0, 1.2, 1.2), Flat(WallX + 25.0, 160.0, 3.6, 3.6) }, 10, false, true);
		Rod(P, Steel, FVector(WallX + 10.0, 14.0, 145.0), FVector(WallX + 10.0, 18.0, 145.0), 0.8, 6);
		Rod(P, Steel, FVector(WallX + 10.0, 18.0, 145.0), FVector(WallX + 15.0, 18.0, 140.0), 0.6, 4);
		Rod(P, Bakelite, FVector(WallX + 15.0, 18.0, 140.0), FVector(WallX + 15.0, 21.0, 140.0), 1.2, 6);
		Rod(P, Bakelite, FVector(WallX + 10.0, -16.0, 158.0), FVector(WallX + 10.0, -16.0, 142.0), 1.6, 6);
		Loft(P, Bakelite, { Flat(WallX + 10.0, 141.0, 1.6, 1.6), Flat(WallX + 10.0, 138.0, 3.4, 3.4) }, 10, false, true);
	}
}
