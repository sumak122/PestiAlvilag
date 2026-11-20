// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DailyLifeModel.h"
#include "PersonKit.h"

/**
 * More of the people of Pest in 1932 for the walkers (PersonModel::RandomOutfit):
 * old women in headscarves, market women in shawls, old men with a cane,
 * schoolboys, soldiers on leave, priests, nuns and ladies in fur collars,
 * and brighter colours for the women's clothes; and what walkers do when
 * they stop (APedestrians acts): the poses (DailyLifeModel::FLimbs) and the
 * little things in their hands. Header only, so it can all be drawn outside
 * the game too.
 */
namespace PassersbyModel
{
	using PersonKit::FOutfit;
	using PersonKit::EHat;
	using PersonKit::ETop;
	using PersonKit::EHeld;

	template <SIZE_T N>
	inline const FLinearColor& PickOf(FRandomStream& Random, const FLinearColor (&Colors)[N])
	{
		return Colors[Random.RandRange(0, static_cast<int32>(N) - 1)];
	}

	inline FLinearColor SkinOf(FRandomStream& Random)
	{
		const FLinearColor Skins[] = { FLinearColor(0.72f, 0.50f, 0.38f), FLinearColor(0.78f, 0.58f, 0.46f), FLinearColor(0.65f, 0.45f, 0.33f) };
		return PickOf(Random, Skins);
	}

	/** Grey or white hair of an old face. */
	inline FLinearColor GreyHair(FRandomStream& Random)
	{
		const FLinearColor Greys[] = { FLinearColor(0.40f, 0.40f, 0.38f), FLinearColor(0.62f, 0.61f, 0.58f), FLinearColor(0.30f, 0.28f, 0.26f) };
		return PickOf(Random, Greys);
	}

	/** The brighter cloths of 1932: rose, mustard, teal, plum, bottle green, cornflower, camel, cream. */
	inline FLinearColor BrightCloth(FRandomStream& Random)
	{
		const FLinearColor Brights[] = {
			FLinearColor(0.55f, 0.16f, 0.20f),
			FLinearColor(0.52f, 0.38f, 0.08f),
			FLinearColor(0.06f, 0.28f, 0.30f),
			FLinearColor(0.30f, 0.09f, 0.26f),
			FLinearColor(0.05f, 0.24f, 0.11f),
			FLinearColor(0.22f, 0.32f, 0.58f),
			FLinearColor(0.45f, 0.32f, 0.18f),
			FLinearColor(0.68f, 0.62f, 0.48f),
			FLinearColor(0.60f, 0.25f, 0.10f),
		};
		return PickOf(Random, Brights);
	}

	/** Headscarf colours: red, navy, black, cream, blue print, brown. */
	inline FLinearColor KerchiefCloth(FRandomStream& Random)
	{
		const FLinearColor Kerchiefs[] = {
			FLinearColor(0.50f, 0.07f, 0.05f),
			FLinearColor(0.06f, 0.08f, 0.22f),
			FLinearColor(0.03f, 0.03f, 0.03f),
			FLinearColor(0.70f, 0.66f, 0.55f),
			FLinearColor(0.15f, 0.25f, 0.50f),
			FLinearColor(0.22f, 0.13f, 0.07f),
		};
		return PickOf(Random, Kerchiefs);
	}

	inline FLinearColor DarkCloth(FRandomStream& Random)
	{
		const FLinearColor Darks[] = {
			FLinearColor(0.04f, 0.04f, 0.04f),
			FLinearColor(0.08f, 0.06f, 0.05f),
			FLinearColor(0.06f, 0.07f, 0.10f),
			FLinearColor(0.10f, 0.10f, 0.10f),
			FLinearColor(0.12f, 0.09f, 0.06f),
		};
		return PickOf(Random, Darks);
	}

	/** An old woman: headscarf, a long dark dress, a shawl, a string bag or basket. */
	inline FOutfit OldWoman(FRandomStream& Random)
	{
		FOutfit O;
		O.bWoman = true;
		O.bAged = true;
		O.Skin = SkinOf(Random);
		O.Hair = GreyHair(Random);
		O.Height = Random.FRandRange(148.f, 160.f);
		O.Build = Random.FRandRange(1.0f, 1.24f);
		O.Top = ETop::Dress;
		O.bLong = true;
		O.Coat = DarkCloth(Random);
		O.Hat = EHat::Kerchief;
		O.HatColor = Random.FRand() < 0.5f ? FLinearColor(0.03f, 0.03f, 0.03f) : KerchiefCloth(Random);
		O.bShawl = Random.FRand() < 0.7f;
		O.Shawl = Random.FRand() < 0.5f ? FLinearColor(0.05f, 0.05f, 0.05f) : FLinearColor(0.20f, 0.14f, 0.09f);
		O.bTie = false;
		O.Shoes = FLinearColor(0.04f, 0.03f, 0.02f);
		O.HeldColor = FLinearColor(0.42f, 0.30f, 0.16f);
		const float Held = Random.FRand();
		O.Held = Held < 0.45f ? EHeld::NetBag : Held < 0.75f ? EHeld::Basket : Held < 0.9f ? EHeld::Cane : EHeld::None;
		return O;
	}

	/** A market woman from the country: bright headscarf, shawl, apron, a big basket; stout. */
	inline FOutfit MarketWoman(FRandomStream& Random)
	{
		FOutfit O;
		O.bWoman = true;
		O.Skin = FLinearColor(0.70f, 0.47f, 0.33f);
		O.Hair = FLinearColor(0.10f, 0.07f, 0.05f);
		O.Height = Random.FRandRange(152.f, 166.f);
		O.Build = Random.FRandRange(1.12f, 1.25f);
		O.Top = ETop::Dress;
		O.bLong = Random.FRand() < 0.5f;
		O.Coat = Random.FRand() < 0.5f ? DarkCloth(Random) : BrightCloth(Random);
		O.Hat = EHat::Kerchief;
		// A bright one, never the black of the old widows.
		const FLinearColor Brights[] = { FLinearColor(0.50f, 0.07f, 0.05f), FLinearColor(0.15f, 0.25f, 0.50f), FLinearColor(0.70f, 0.66f, 0.55f), FLinearColor(0.55f, 0.40f, 0.08f) };
		O.HatColor = PickOf(Random, Brights);
		O.bShawl = true;
		O.Shawl = Random.FRand() < 0.5f ? FLinearColor(0.38f, 0.10f, 0.08f) : FLinearColor(0.12f, 0.16f, 0.10f);
		O.bApron = true;
		O.Apron = Random.FRand() < 0.5f ? FLinearColor(0.80f, 0.78f, 0.70f) : FLinearColor(0.18f, 0.22f, 0.40f);
		O.bTie = false;
		O.bBoots = true;
		O.Shoes = FLinearColor(0.05f, 0.03f, 0.02f);
		O.Held = EHeld::Basket;
		O.HeldColor = FLinearColor(0.50f, 0.38f, 0.20f);
		return O;
	}

	/** An old gentleman: grey whiskers, a dark overcoat, a hat and a walking stick. */
	inline FOutfit OldMan(FRandomStream& Random)
	{
		FOutfit O;
		O.bAged = true;
		O.Skin = SkinOf(Random);
		O.Hair = GreyHair(Random);
		O.Height = Random.FRandRange(160.f, 176.f);
		O.Build = Random.FRandRange(0.9f, 1.15f);
		O.bMoustache = Random.FRand() < 0.6f;
		O.Beard = Random.FRand() < 0.2f ? PersonKit::EBeard::Full : PersonKit::EBeard::None;
		O.Top = ETop::Overcoat;
		O.Coat = DarkCloth(Random);
		O.Hat = Random.FRand() < 0.6f ? EHat::Bowler : EHat::Fedora;
		O.HatColor = DarkCloth(Random);
		O.bScarf = Random.FRand() < 0.4f;
		O.Scarf = FLinearColor(0.30f, 0.28f, 0.24f);
		O.Held = EHeld::Cane;
		O.HeldColor = FLinearColor(0.30f, 0.17f, 0.08f);
		return O;
	}

	/** A schoolboy or student: a peaked school cap, a short jacket, books in hand. */
	inline FOutfit Student(FRandomStream& Random)
	{
		FOutfit O;
		O.Skin = SkinOf(Random);
		const FLinearColor Hairs[] = { FLinearColor(0.10f, 0.07f, 0.05f), FLinearColor(0.25f, 0.16f, 0.08f), FLinearColor(0.45f, 0.35f, 0.20f) };
		O.Hair = PickOf(Random, Hairs);
		O.Height = Random.FRandRange(158.f, 176.f);
		O.Build = Random.FRandRange(0.86f, 0.98f);
		O.FaceShape = Random.FRand() < 0.5f ? 0 : 3;
		O.Top = ETop::Jacket;
		O.Coat = Random.FRand() < 0.6f ? FLinearColor(0.05f, 0.07f, 0.16f) : FLinearColor(0.14f, 0.14f, 0.15f);
		O.Trousers = O.Coat;
		O.Hat = EHat::PeakedCap;
		O.HatColor = FLinearColor(0.04f, 0.06f, 0.14f);
		O.HatBand = Random.FRand() < 0.5f ? FLinearColor(0.70f, 0.66f, 0.55f) : FLinearColor(0.30f, 0.06f, 0.05f);
		O.Buttons = FLinearColor(0.62f, 0.50f, 0.22f);
		O.bTie = Random.FRand() < 0.5f;
		O.Held = EHeld::Book;
		const FLinearColor Covers[] = { FLinearColor(0.30f, 0.08f, 0.06f), FLinearColor(0.08f, 0.14f, 0.08f), FLinearColor(0.10f, 0.08f, 0.06f) };
		O.HeldColor = PickOf(Random, Covers);
		return O;
	}

	/** A soldier on leave (honvéd): khaki-brown tunic with a stand collar and brass buttons, belt, peaked cap, boots. */
	inline FOutfit Soldier(FRandomStream& Random)
	{
		FOutfit O;
		O.Skin = SkinOf(Random);
		O.Hair = FLinearColor(0.10f, 0.07f, 0.05f);
		O.Height = Random.FRandRange(168.f, 184.f);
		O.Build = Random.FRandRange(0.98f, 1.1f);
		O.bMoustache = Random.FRand() < 0.5f;
		O.MoustacheKind = PersonKit::EMoustache::Brush;
		const FLinearColor Khaki(0.22f, 0.20f, 0.12f);
		O.Top = ETop::Jacket;
		O.Coat = Khaki;
		O.Shirt = Khaki;
		O.Trousers = Khaki;
		O.bTie = false;
		O.bStandCollar = true;
		O.bBelt = true;
		O.Belt = FLinearColor(0.14f, 0.08f, 0.04f);
		O.Buttons = FLinearColor(0.62f, 0.50f, 0.22f);
		O.bBoots = true;
		O.Shoes = FLinearColor(0.05f, 0.03f, 0.02f);
		O.Hat = EHat::PeakedCap;
		O.HatColor = Khaki;
		O.HatBand = FLinearColor(0.18f, 0.16f, 0.10f);
		return O;
	}

	/** A priest in his black cassock to the ankles, white collar, wide black hat. */
	inline FOutfit Priest(FRandomStream& Random)
	{
		FOutfit O;
		O.Skin = SkinOf(Random);
		O.Hair = Random.FRand() < 0.5f ? GreyHair(Random) : FLinearColor(0.10f, 0.07f, 0.05f);
		O.bAged = Random.FRand() < 0.4f;
		O.Height = Random.FRandRange(166.f, 182.f);
		O.Build = Random.FRandRange(0.95f, 1.2f);
		const FLinearColor Black(0.025f, 0.025f, 0.025f);
		O.Top = ETop::Overcoat;
		O.bLong = true;
		O.Coat = Black;
		O.Shirt = Black;
		O.Trousers = Black;
		O.bTie = false;
		O.bClerical = true;
		O.Hat = EHat::ClericHat;
		O.HatColor = Black;
		O.Held = Random.FRand() < 0.4f ? EHeld::Book : EHeld::None;
		O.HeldColor = Black;
		return O;
	}

	/** A nun in a black habit, white wimple and black veil. */
	inline FOutfit Nun(FRandomStream& Random)
	{
		FOutfit O;
		O.bWoman = true;
		O.Skin = SkinOf(Random);
		O.Height = Random.FRandRange(152.f, 166.f);
		O.Build = Random.FRandRange(0.95f, 1.15f);
		const FLinearColor Black(0.025f, 0.025f, 0.03f);
		O.Top = ETop::Dress;
		O.bLong = true;
		O.Coat = Black;
		O.Hat = EHat::Veil;
		O.HatColor = Black;
		O.bTie = false;
		O.Shoes = Black;
		O.Held = Random.FRand() < 0.5f ? EHeld::Book : EHeld::Basket;
		O.HeldColor = O.Held == EHeld::Book ? Black : FLinearColor(0.45f, 0.34f, 0.18f);
		return O;
	}

	/** A lady out shopping: a bright coat with a fur collar, a wide-brimmed hat, a handbag or umbrella. */
	inline FOutfit Lady(FRandomStream& Random)
	{
		FOutfit O;
		O.bWoman = true;
		O.Skin = FLinearColor(0.80f, 0.60f, 0.48f);
		const FLinearColor Hairs[] = { FLinearColor(0.03f, 0.025f, 0.02f), FLinearColor(0.25f, 0.16f, 0.08f), FLinearColor(0.60f, 0.45f, 0.22f), FLinearColor(0.35f, 0.10f, 0.05f) };
		O.Hair = PickOf(Random, Hairs);
		O.Height = Random.FRandRange(158.f, 172.f);
		O.Build = Random.FRandRange(0.9f, 1.05f);
		O.Top = ETop::Overcoat;
		O.Coat = BrightCloth(Random);
		O.Trousers = DarkCloth(Random);
		O.bFurCollar = true;
		const FLinearColor Furs[] = { FLinearColor(0.12f, 0.08f, 0.05f), FLinearColor(0.03f, 0.03f, 0.03f), FLinearColor(0.35f, 0.28f, 0.20f), FLinearColor(0.50f, 0.48f, 0.45f) };
		O.Fur = PickOf(Random, Furs);
		O.Hat = Random.FRand() < 0.6f ? EHat::WideBrim : EHat::Cloche;
		O.HatColor = Random.FRand() < 0.5f ? DarkCloth(Random) : BrightCloth(Random);
		O.HatBand = BrightCloth(Random);
		O.bTie = false;
		O.Shoes = FLinearColor(0.06f, 0.03f, 0.02f);
		const float Held = Random.FRand();
		O.Held = Held < 0.35f ? EHeld::Briefcase : Held < 0.55f ? EHeld::Umbrella : EHeld::None;
		O.HeldColor = FLinearColor(0.08f, 0.05f, 0.03f);
		return O;
	}

	/**
	 * Now and then one of the people above instead of an everyday walker; the
	 * rarer ones (nun, priest, the stick, the string bag) only on foot. False
	 * leaves the everyday walker to RandomOutfit.
	 */
	inline bool SpecialLook(FRandomStream& Random, bool bOnFoot, FOutfit& Out)
	{
		const float Roll = Random.FRand();
		if (!bOnFoot)
		{
			if (Roll < 0.03f) { Out = Soldier(Random); return true; }
			if (Roll < 0.07f) { Out = Student(Random); return true; }
			if (Roll < 0.12f) { Out = Lady(Random); return true; }
			return false;
		}
		if (Roll < 0.012f) { Out = Nun(Random); return true; }
		if (Roll < 0.03f) { Out = Priest(Random); return true; }
		if (Roll < 0.06f) { Out = Soldier(Random); return true; }
		if (Roll < 0.11f) { Out = Student(Random); return true; }
		if (Roll < 0.18f) { Out = OldMan(Random); return true; }
		if (Roll < 0.27f) { Out = OldWoman(Random); return true; }
		if (Roll < 0.31f) { Out = MarketWoman(Random); return true; }
		if (Roll < 0.37f) { Out = Lady(Random); return true; }
		return false;
	}

	/** A beggar: an old patched coat, no tie, a few days' stubble, a battered cap held out. */
	inline FOutfit Beggar(FRandomStream& Random)
	{
		FOutfit O;
		O.Skin = SkinOf(Random);
		O.Hair = GreyHair(Random);
		O.bAged = true;
		O.Height = Random.FRandRange(160.f, 174.f);
		O.Build = Random.FRandRange(0.86f, 0.95f);
		O.Beard = Random.FRand() < 0.6f ? PersonKit::EBeard::Stubble : PersonKit::EBeard::Full;
		O.Top = ETop::Overcoat;
		O.Coat = FLinearColor(0.16f, 0.14f, 0.10f);
		O.Shirt = FLinearColor(0.45f, 0.42f, 0.36f);
		O.Trousers = FLinearColor(0.12f, 0.11f, 0.09f);
		O.bTie = false;
		O.bScarf = true;
		O.Scarf = FLinearColor(0.22f, 0.16f, 0.12f);
		O.Hat = EHat::None;
		O.Shoes = FLinearColor(0.10f, 0.07f, 0.05f);
		return O;
	}

	// ---------------------------------------------------------------------
	// What a walker does when he stops for a moment, alone or with another.

	using DailyLifeModel::FLimbs;

	/** Looking at his pocket watch, held up before his chest in the left hand. */
	inline void LookAtWatch(FLimbs& L)
	{
		L.Arms[0] = FRotator(34.f, 0.f, -14.f);
		L.Elbows[0] = 92.f;
		L.Arms[1] = FRotator(18.f, 0.f, 10.f);
		L.Elbows[1] = 64.f;
		L.Pitch = 3.f;
	}

	/** Smoking: the right hand up to the mouth by Draw (0 down .. 1 at the lips), the left hand at rest. */
	inline void Smoking(FLimbs& L, float Draw)
	{
		const float D = FMath::SmoothStep(0.f, 1.f, Draw);
		L.Arms[1] = FRotator(FMath::Lerp(14.f, 58.f, D), 0.f, FMath::Lerp(-6.f, 50.f, D));
		L.Elbows[1] = FMath::Lerp(62.f, 132.f, D);
		L.Arms[0] = FRotator(4.f, 0.f, 4.f);
		L.Elbows[0] = 8.f;
	}

	/** Reading the paper held open before him with both hands. */
	inline void Reading(FLimbs& L)
	{
		L.Arms[0] = FRotator(40.f, 0.f, 6.f);
		L.Arms[1] = FRotator(40.f, 0.f, -6.f);
		L.Elbows[0] = 62.f;
		L.Elbows[1] = 62.f;
		L.Pitch = 3.f;
	}

	/** Looking in a shop window: hands clasped behind his back, leaning in a little. */
	inline void WindowShopping(FLimbs& L)
	{
		L.Arms[0] = FRotator(-24.f, 0.f, -10.f);
		L.Arms[1] = FRotator(-24.f, 0.f, 10.f);
		L.Elbows[0] = 58.f;
		L.Elbows[1] = 58.f;
		L.Pitch = 7.f;
	}

	/**
	 * Crouched down to tie a shoelace, the left knee forward and the right
	 * nearly on the ground (Phase in seconds: the fingers at work). Returns
	 * how far his middle comes down (cm, for a man of 178 cm).
	 */
	inline float TyingShoe(FLimbs& L, float Phase)
	{
		L.Pitch = 22.f;
		L.Legs[0] = 92.f;
		L.Knees[0] = -122.f;
		L.Legs[1] = 52.f;
		L.Knees[1] = -128.f;
		const float Fiddle = 5.f * FMath::Sin(Phase * 9.f);
		L.Arms[0] = FRotator(52.f + Fiddle, 0.f, -6.f);
		L.Arms[1] = FRotator(52.f - Fiddle, 0.f, 6.f);
		L.Elbows[0] = 34.f;
		L.Elbows[1] = 34.f;
		return 44.f;
	}

	/** Throwing crumbs to the pigeons from a paper bag in the left hand (Phase in throws). */
	inline void FeedingPigeons(FLimbs& L, float Phase)
	{
		const float T = FMath::Sin(2.f * PI * Phase);
		const float Toss = FMath::Max(0.f, T);
		L.Arms[1] = FRotator(26.f + 34.f * Toss, 0.f, -12.f);
		L.Elbows[1] = 46.f - 30.f * Toss;
		L.Arms[0] = FRotator(22.f, 0.f, -8.f);
		L.Elbows[0] = 72.f;
		L.Pitch = 6.f;
	}

	/** Talking with his hands (Phase in seconds): the right hand up and turning as he speaks. */
	inline void Talking(FLimbs& L, float Phase)
	{
		L.Arms[1] = FRotator(26.f + 12.f * FMath::Sin(Phase * 2.3f), 0.f, -10.f + 6.f * FMath::Sin(Phase * 1.7f));
		L.Elbows[1] = 82.f + 18.f * FMath::Sin(Phase * 3.1f);
		L.Arms[0] = FRotator(6.f, 0.f, 4.f);
		L.Elbows[0] = 20.f;
		L.Pitch = 2.f;
	}

	/** Listening: hands together before him, nodding a little (the pitch). */
	inline void Listening(FLimbs& L, float Phase)
	{
		L.Arms[0] = FRotator(4.f, 0.f, -22.f);
		L.Arms[1] = FRotator(4.f, 0.f, 22.f);
		L.Elbows[0] = 50.f;
		L.Elbows[1] = 50.f;
		L.Pitch = 2.f + 1.5f * FMath::Max(0.f, static_cast<float>(FMath::Sin(Phase * 2.f)));
	}

	/** Shaking hands with someone in front of him (Phase in seconds: the shake). */
	inline void ShakingHands(FLimbs& L, float Phase)
	{
		L.Arms[1] = FRotator(42.f + 6.f * FMath::Sin(Phase * 14.f), 0.f, 8.f);
		L.Elbows[1] = 24.f;
		L.Pitch = 4.f;
	}

	/** Arguing (Phase in seconds): both arms thrown about, leaning in. */
	inline void Arguing(FLimbs& L, float Phase)
	{
		L.Arms[1] = FRotator(58.f + 30.f * FMath::Sin(Phase * 3.3f), 0.f, -22.f);
		L.Elbows[1] = 50.f + 30.f * FMath::Cos(Phase * 2.9f);
		L.Arms[0] = FRotator(36.f + 26.f * FMath::Sin(Phase * 2.6f + 1.f), 0.f, 18.f);
		L.Elbows[0] = 70.f + 20.f * FMath::Sin(Phase * 3.7f);
		L.Pitch = 7.f + 3.f * FMath::Sin(Phase * 1.9f);
	}

	/** Giving a light: a match held up to the other's face, the left hand cupped round it. */
	inline void GivingALight(FLimbs& L)
	{
		L.Arms[1] = FRotator(58.f, 0.f, 10.f);
		L.Elbows[1] = 62.f;
		L.Arms[0] = FRotator(56.f, 0.f, -12.f);
		L.Elbows[0] = 70.f;
		L.Pitch = 5.f;
	}

	/** Begging: the cap held out in the right hand, stooped. */
	inline void Begging(FLimbs& L, float Phase)
	{
		L.Arms[1] = FRotator(50.f + 4.f * FMath::Sin(Phase * 1.3f), 0.f, -4.f);
		L.Elbows[1] = 26.f;
		L.Arms[0] = FRotator(12.f, 0.f, 4.f);
		L.Elbows[0] = 40.f;
		L.Pitch = 9.f;
	}

	/** Walking arm in arm: the man's arm crooked (Arm on the woman's side), and hers hooked through it. */
	inline FRotator CrookedArm(float& OutElbow)
	{
		OutElbow = 74.f;
		return FRotator(6.f, 0.f, -14.f);
	}
	inline FRotator HookedArm(float& OutElbow)
	{
		OutElbow = 58.f;
		return FRotator(18.f, 0.f, 16.f);
	}

	/** Hand in hand: the mother's right arm a little out and down to the child, the child's left up to her. */
	inline FRotator MotherHand(float& OutElbow)
	{
		OutElbow = 10.f;
		return FRotator(8.f, 0.f, -16.f);
	}
	inline FRotator ChildHand(float& OutElbow)
	{
		OutElbow = 6.f;
		return FRotator(34.f, 0.f, 22.f);
	}

	// The little things in their hands, each built round where the fist
	// closes (DailyLifeModel::HandOnForearm), pointing along +X.

	/** A lit cigarette sticking out of the fingers. */
	inline void Cigarette(PersonKit::FPiece& P)
	{
		PersonKit::Rod(P, FLinearColor(0.92f, 0.90f, 0.84f), FVector(0.5, 0.0, 1.5), FVector(8.0, 0.0, 1.5), 0.6, 6);
		PersonKit::Rod(P, DailyLifeModel::EmberColor(), FVector(8.0, 0.0, 1.5), FVector(8.8, 0.0, 1.5), 0.65, 6);
	}

	/** A gold pocket watch on its chain, its face up. */
	inline void PocketWatch(PersonKit::FPiece& P)
	{
		const FLinearColor Gold(0.70f, 0.52f, 0.18f);
		PersonKit::Loft(P, Gold, { PersonKit::Flat(2.0, 1.5, 3.2, 3.2), PersonKit::Flat(2.0, 2.6, 3.2, 3.2) }, 12, true, true);
		PersonKit::Loft(P, FLinearColor(0.90f, 0.88f, 0.80f), { PersonKit::Flat(2.0, 2.6, 2.7, 2.7), PersonKit::Flat(2.0, 2.7, 2.7, 2.7) }, 12, false, true);
		PersonKit::Rod(P, Gold, FVector(-0.6, 0.0, 2.0), FVector(-6.0, -4.0, -4.0), 0.25, 4);
	}

	/** The newspaper opened wide, held up before the chest: built round the middle of its bottom edge, the pages facing -X (towards him). */
	inline void OpenPaper(PersonKit::FPiece& P)
	{
		const FLinearColor Paper(0.80f, 0.78f, 0.70f);
		const FLinearColor Print(0.42f, 0.41f, 0.38f);
		PersonKit::Block(P, Paper, FVector(0.0, 0.0, 19.0), FVector(0.3, 28.0, 19.0));
		for (const double Side : { -1.0, 1.0 })
		{
			for (int32 Row = 0; Row < 4; ++Row)
			{
				PersonKit::Block(P, Print, FVector(0.4, Side * 14.0, 32.0 - Row * 8.0), FVector(0.05, 11.0, Row == 0 ? 3.0 : 2.4));
			}
		}
	}

	/** A battered cap held upside down to take the coins. */
	inline void HeldOutCap(PersonKit::FPiece& P)
	{
		const FLinearColor Cloth(0.18f, 0.16f, 0.13f);
		PersonKit::Blob(P, Cloth, FVector(6.0, 0.0, 2.0), FVector(9.0, 8.0, -5.0), 10, 4, 0.0);
		PersonKit::Loft(P, Cloth, { PersonKit::Flat(6.0, 2.0, 9.4, 8.4), PersonKit::Flat(6.0, 2.8, 9.4, 8.4) }, 12, false, false);
	}

	/** A small paper bag of crumbs. */
	inline void CrumbBag(PersonKit::FPiece& P)
	{
		PersonKit::Block(P, FLinearColor(0.68f, 0.58f, 0.40f), FVector(2.0, 0.0, 3.0), FVector(3.5, 3.0, 5.0));
	}
}
