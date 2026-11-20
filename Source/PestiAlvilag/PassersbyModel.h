// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PersonKit.h"

/**
 * More of the people of Pest in 1932 for the walkers (PersonModel::RandomOutfit):
 * old women in headscarves, market women in shawls, old men with a cane,
 * schoolboys, soldiers on leave, priests, nuns and ladies in fur collars,
 * and brighter colours for the women's clothes. Header only, so the looks
 * can be drawn outside the game too.
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
}
