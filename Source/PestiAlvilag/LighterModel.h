// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * The collectible lighters of 1932 as lists of basic shapes (the engine's
 * cube, cylinder, sphere and cone, each 100 cm across), and the thirty to be
 * found in Józsefváros with their names and one line about each: trench
 * lighters hammered from cartridges, Viennese pocket lighters, slim art deco
 * enamel ones, bakelite table lighters, lever lighters and homemade tin ones.
 * The thirty-first, the golden one, is the reward for finding them all.
 * Also where each of the thirty is hidden.
 *
 * Header-only (like MarketModel) so it can be built and looked at outside
 * the engine. A lighter's frame: real size in cm, +X its front, Z up from
 * the surface it stands on.
 */
namespace LighterModel
{
	enum EShape : int32 { Cube, Cylinder, Sphere, Cone };
	enum class EKind : uint8 { Trench, Pocket, Deco, Table, Lever, Tin, Golden };

	/** One shape: where its middle is, its whole size, and its turn (degrees, as FRotator). */
	struct FPart
	{
		int32 Shape;
		FLinearColor Color;
		FVector At;
		FVector Size;
		double Pitch = 0.0;
		double Yaw = 0.0;
		double Roll = 0.0;
	};

	/** The colours a lighter is made in: its metal, its second colour (enamel, bakelite, a coin), and trim. */
	struct FPaint
	{
		FLinearColor Metal;
		FLinearColor Accent;
		FLinearColor Trim;
	};

	/** One of the lighters to find: what it is, what it is called and its line. */
	struct FLighter
	{
		EKind Kind;
		FPaint Paint;
		const TCHAR* NameHu;
		const TCHAR* NameEn;
		const TCHAR* LineHu;
		const TCHAR* LineEn;
	};

	namespace Colors
	{
		const FLinearColor Brass(0.55f, 0.40f, 0.15f);
		const FLinearColor DarkBrass(0.36f, 0.25f, 0.09f);
		const FLinearColor Copper(0.55f, 0.25f, 0.12f);
		const FLinearColor Chrome(0.66f, 0.66f, 0.64f);
		const FLinearColor Nickel(0.50f, 0.50f, 0.46f);
		const FLinearColor Silver(0.74f, 0.74f, 0.72f);
		const FLinearColor Gunmetal(0.18f, 0.18f, 0.19f);
		const FLinearColor Gold(0.80f, 0.55f, 0.12f);
		const FLinearColor Tin(0.42f, 0.42f, 0.40f);
		const FLinearColor Rust(0.36f, 0.20f, 0.10f);
		const FLinearColor Black(0.03f, 0.03f, 0.03f);
		const FLinearColor Dark(0.02f, 0.02f, 0.02f);
		const FLinearColor Flint(0.25f, 0.25f, 0.26f);
		const FLinearColor Jade(0.04f, 0.35f, 0.24f);
		const FLinearColor Lapis(0.05f, 0.12f, 0.45f);
		const FLinearColor Coral(0.72f, 0.22f, 0.12f);
		const FLinearColor Ivory(0.78f, 0.72f, 0.58f);
		const FLinearColor Red(0.50f, 0.05f, 0.04f);
		const FLinearColor BrownBakelite(0.25f, 0.10f, 0.04f);
		const FLinearColor GreenBakelite(0.10f, 0.24f, 0.14f);
		const FLinearColor Onyx(0.06f, 0.06f, 0.07f);
		const FLinearColor ParkGreen(0.05f, 0.20f, 0.09f);
		const FLinearColor Flame(1.0f, 0.45f, 0.08f);
		const FLinearColor FlameCore(1.0f, 0.85f, 0.35f);
	}

	/** The parts of one lighter, in its own frame. */
	inline void Build(EKind Kind, const FPaint& Paint, TArray<FPart>& Out)
	{
		using namespace Colors;
		const FLinearColor& M = Paint.Metal;
		const FLinearColor& A = Paint.Accent;
		const FLinearColor& T = Paint.Trim;
		// The striker wheel, its axle across the lighter (along X).
		auto Wheel = [&Out](const FVector& At, double Across)
		{
			Out.Add({ Cylinder, Flint, At, FVector(0.95, 0.95, Across), 90.0, 0.0, 0.0 });
		};
		switch (Kind)
		{
		case EKind::Trench:
			// A cartridge case: a round brass body, a cap on a chain-less
			// hinge, the wheel beside it, and something soldered on the front.
			Out.Add({ Cylinder, M, FVector(0, 0, 2.5), FVector(1.9, 1.9, 5.0) });
			Out.Add({ Cylinder, T, FVector(0, 0, 0.25), FVector(2.05, 2.05, 0.5) });
			Out.Add({ Cylinder, T, FVector(0, 0, 4.7), FVector(2.0, 2.0, 0.4) });
			Out.Add({ Cylinder, M, FVector(0, -0.35, 5.6), FVector(1.1, 1.1, 1.4) });
			Out.Add({ Sphere, M, FVector(0, -0.35, 6.3), FVector(1.1, 1.1, 0.6) });
			Wheel(FVector(0, 0.55, 5.45), 0.45);
			Out.Add({ Cylinder, A, FVector(0.95, 0, 2.6), FVector(1.3, 1.3, 0.12), 90.0, 0.0, 0.0 });
			break;

		case EKind::Pocket:
		case EKind::Golden:
			// The Viennese pocket lighter: a flat box, a chimney with slits
			// over the wick, the wheel, and the cap that snuffs it.
			Out.Add({ Cube, M, FVector(0, 0, 2.0), FVector(1.2, 3.4, 4.0) });
			Out.Add({ Cube, T, FVector(0, 0, 3.4), FVector(1.25, 3.45, 0.15) });
			Out.Add({ Cube, M, FVector(0, -0.75, 4.75), FVector(1.0, 1.1, 1.5) });
			for (double Z : { 4.4, 4.8, 5.2 })
			{
				Out.Add({ Cube, Dark, FVector(0.52, -0.75, Z), FVector(0.04, 0.7, 0.14) });
				Out.Add({ Cube, Dark, FVector(-0.52, -0.75, Z), FVector(0.04, 0.7, 0.14) });
			}
			Wheel(FVector(0, 0.15, 4.55), 0.6);
			Out.Add({ Cube, M, FVector(0, 0.95, 4.45), FVector(1.0, 1.0, 0.9) });
			Out.Add({ Cylinder, A, FVector(0.62, 0, 1.8), FVector(1.5, 1.5, 0.06), 90.0, 0.0, 0.0 });
			if (Kind == EKind::Golden)
			{
				// Lit, and it never goes out.
				Out.Add({ Cone, Flame, FVector(0, -0.75, 6.4), FVector(0.75, 0.75, 1.9) });
				Out.Add({ Cone, FlameCore, FVector(0.05, -0.75, 6.0), FVector(0.4, 0.4, 1.0) });
			}
			break;

		case EKind::Deco:
			// Slim and flat, an enamel panel on the front with a stepped
			// gold sunrise, the lid above a thin dark line.
			Out.Add({ Cube, M, FVector(0, 0, 2.35), FVector(0.8, 2.8, 4.7) });
			Out.Add({ Cube, Dark, FVector(0, 0, 4.75), FVector(0.82, 2.82, 0.1) });
			Out.Add({ Cube, M, FVector(0, 0, 5.25), FVector(0.8, 2.8, 0.9) });
			Out.Add({ Cube, A, FVector(0.41, 0, 2.4), FVector(0.06, 2.3, 3.7) });
			Out.Add({ Cube, T, FVector(0.45, 0, 1.6), FVector(0.04, 0.12, 2.0) });
			Out.Add({ Cube, T, FVector(0.45, 0, 2.7), FVector(0.04, 1.8, 0.14) });
			Out.Add({ Cube, T, FVector(0.45, 0, 3.15), FVector(0.04, 1.25, 0.14) });
			Out.Add({ Cube, T, FVector(0.45, 0, 3.6), FVector(0.04, 0.7, 0.14) });
			Out.Add({ Cube, T, FVector(0.45, 0, 0.65), FVector(0.04, 2.3, 0.14) });
			break;

		case EKind::Table:
			// The desk lighter: a round base, a fat bakelite body, a metal
			// neck and the wick cap on top.
			Out.Add({ Cylinder, M, FVector(0, 0, 0.4), FVector(4.6, 4.6, 0.8) });
			Out.Add({ Sphere, A, FVector(0, 0, 2.6), FVector(4.2, 4.2, 3.8) });
			Out.Add({ Cylinder, T, FVector(0, 0, 2.6), FVector(4.3, 4.3, 0.25) });
			Out.Add({ Cylinder, M, FVector(0, 0, 5.0), FVector(1.3, 1.3, 1.6) });
			Out.Add({ Cylinder, M, FVector(0, 0, 6.05), FVector(1.7, 1.7, 0.6) });
			Out.Add({ Sphere, M, FVector(0, 0, 6.4), FVector(1.7, 1.7, 0.5) });
			Wheel(FVector(0, 0.95, 5.5), 0.45);
			break;

		case EKind::Lever:
			// Press the arm and the cap flies up: a box body, a pivot at
			// one end, the arm along the top, the cap at the other end.
			Out.Add({ Cube, M, FVector(0, 0, 1.9), FVector(1.3, 3.0, 3.8) });
			Out.Add({ Cube, A, FVector(0.66, 0, 1.9), FVector(0.04, 2.4, 2.8) });
			Out.Add({ Cylinder, T, FVector(0, -1.3, 4.05), FVector(0.6, 0.6, 1.1), 90.0, 0.0, 0.0 });
			Out.Add({ Cube, M, FVector(0, 0.15, 4.3), FVector(0.5, 3.2, 0.25), 0.0, 0.0, -6.0 });
			Out.Add({ Cylinder, M, FVector(0, 1.1, 4.2), FVector(1.05, 1.05, 0.9) });
			Out.Add({ Cylinder, Dark, FVector(0, 1.1, 3.95), FVector(0.5, 0.5, 0.5) });
			Wheel(FVector(0, 0.0, 4.0), 0.55);
			break;

		case EKind::Tin:
			// Homemade from a tin: a box a little out of true, a crooked
			// chimney, rivets, and a band of wire round it.
			Out.Add({ Cube, M, FVector(0, 0, 2.1), FVector(1.3, 2.6, 4.2), 0.0, 0.0, 2.5 });
			Out.Add({ Cube, M, FVector(0, -0.5, 4.85), FVector(1.0, 1.0, 1.3), 0.0, 4.0, -5.0 });
			Out.Add({ Cube, Dark, FVector(0.5, -0.5, 4.9), FVector(0.05, 0.6, 0.5), 0.0, 4.0, -5.0 });
			Wheel(FVector(0, 0.55, 4.55), 0.55);
			for (double Y : { -0.9, 0.9 })
			{
				Out.Add({ Sphere, T, FVector(0.66, Y, 3.6), FVector(0.22, 0.22, 0.22) });
				Out.Add({ Sphere, T, FVector(0.66, Y, 0.6), FVector(0.22, 0.22, 0.22) });
			}
			Out.Add({ Cube, A, FVector(0, 0, 2.0), FVector(1.36, 2.66, 0.2), 0.0, 0.0, 2.5 });
			break;
		}
	}

	/** The thirty to find, in the order the collection cabinet shows them. */
	inline const TArray<FLighter>& All()
	{
		using namespace Colors;
		static const TArray<FLighter> List = {
			{ EKind::Trench, { Brass, Copper, DarkBrass }, TEXT("Isonz\u00f3i gy\u00fajt\u00f3"), TEXT("Isonzo lighter"),
				TEXT("Egy puskat\u00f6lt\u00e9nyb\u0151l kalap\u00e1lt\u00e1k az Isonz\u00f3n\u00e1l, 1917-ben."), TEXT("Hammered from a rifle cartridge on the Isonzo, 1917.") },
			{ EKind::Trench, { Copper, Brass, DarkBrass }, TEXT("Krajc\u00e1ros gy\u00fajt\u00f3"), TEXT("Kreuzer lighter"),
				TEXT("Az oldal\u00e1ra egy Ferenc J\u00f3zsef-krajc\u00e1rt forrasztottak, szerencs\u00e9re."), TEXT("A Franz Joseph kreuzer soldered on the side, for luck.") },
			{ EKind::Trench, { DarkBrass, DarkBrass, Brass }, TEXT("Doberd\u00f3i gy\u00fajt\u00f3"), TEXT("Doberdo lighter"),
				TEXT("Gazd\u00e1ja belekarcolta a nev\u00e9t, de m\u00e1r lekopott."), TEXT("Its owner scratched his name on it, but it has worn away.") },
			{ EKind::Trench, { Nickel, Red, Gunmetal }, TEXT("Husz\u00e1r gy\u00fajt\u00f3"), TEXT("Hussar's lighter"),
				TEXT("Egy husz\u00e1r\u00e9 volt; m\u00e9g most is l\u00f3szaga van."), TEXT("A hussar's; it still smells of horse.") },
			{ EKind::Trench, { Silver, Gold, Nickel }, TEXT("Tiszti gy\u00fajt\u00f3"), TEXT("Officer's lighter"),
				TEXT("Egy tiszt\u00e9, ez\u00fcst\u00f6z\u00f6tt, a Ludovika Akad\u00e9mi\u00e1r\u00f3l."), TEXT("An officer's, silver-plated, from the Ludovika Academy.") },
			{ EKind::Pocket, { Chrome, Chrome, Gunmetal }, TEXT("B\u00e9csi zsebgy\u00fajt\u00f3"), TEXT("Viennese pocket lighter"),
				TEXT("B\u00e9csben k\u00e9sz\u00fclt; a K\u00f6r\u00faton minden \u00fariembernek van ilyen."), TEXT("Made in Vienna; every gentleman on the K\u00f6r\u00fat has one.") },
			{ EKind::Pocket, { Gunmetal, Nickel, Black }, TEXT("Sof\u0151r gy\u00fajt\u00f3ja"), TEXT("The driver's lighter"),
				TEXT("Egy taxisof\u0151r felejtette a kocsi fell\u00e9p\u0151j\u00e9n."), TEXT("A taxi driver left it on his running board.") },
			{ EKind::Pocket, { Brass, Brass, DarkBrass }, TEXT("Kalauz gy\u00fajt\u00f3"), TEXT("Conductor's lighter"),
				TEXT("Egy villamoskalauz\u00e9 volt; sim\u00e1ra koptatta a h\u00fcvelykujja."), TEXT("A tram conductor's, worn smooth by his thumb.") },
			{ EKind::Pocket, { Copper, Brass, DarkBrass }, TEXT("Post\u00e1s gy\u00fajt\u00f3"), TEXT("Postman's lighter"),
				TEXT("Bev\u00e9sve: \u201eMagyar Kir\u00e1lyi Posta\u201d."), TEXT("Engraved: \"Royal Hungarian Post\".") },
			{ EKind::Pocket, { Black, Black, Gunmetal }, TEXT("\u00c9jszakai gy\u00fajt\u00f3"), TEXT("Night lighter"),
				TEXT("Feket\u00e9re festett\u00e9k, hogy ne csillogjon a s\u00f6t\u00e9tben. Vajon ki\u00e9 volt?"), TEXT("Painted black so it would not shine in the dark. Whose was it?") },
			{ EKind::Pocket, { Silver, Silver, Nickel }, TEXT("Monogramos gy\u00fajt\u00f3"), TEXT("Monogrammed lighter"),
				TEXT("Monogram: \u201eF. L.\u201d Nem a ti\u00e9d, de most m\u00e1r az."), TEXT("Monogram \"F. L.\" Not yours, but now it is.") },
			{ EKind::Deco, { Silver, Jade, Gold }, TEXT("J\u00e1de gy\u00fajt\u00f3"), TEXT("Jade lighter"),
				TEXT("Z\u00f6ld zom\u00e1nc \u00e9s ez\u00fcst, egy R\u00e1k\u00f3czi \u00fati \u00e9kszer\u00e9szt\u0151l."), TEXT("Green enamel and silver, from a jeweller on R\u00e1k\u00f3czi \u00fat.") },
			{ EKind::Deco, { Silver, Lapis, Gold }, TEXT("K\u00e9k Duna"), TEXT("Blue Danube"),
				TEXT("K\u00e9k, mint a Duna a plak\u00e1tokon, de sosem a val\u00f3s\u00e1gban."), TEXT("Blue as the Danube on the posters, never in real life.") },
			{ EKind::Deco, { Gold, Coral, Silver }, TEXT("Kabar\u00e9 gy\u00fajt\u00f3"), TEXT("Cabaret lighter"),
				TEXT("Egy ter\u00e9zv\u00e1rosi kabar\u00e9 t\u00e1ncosn\u0151j\u00e9\u00e9 volt."), TEXT("A dancer's, from a cabaret in Ter\u00e9zv\u00e1ros.") },
			{ EKind::Deco, { Black, Black, Gold }, TEXT("Fekete-arany"), TEXT("Black and gold"),
				TEXT("Fekete lakk \u00e9s arany, a P\u00e1risi Nagy \u00c1ruh\u00e1z legjava."), TEXT("Black lacquer and gold, the best the P\u00e1risi department store had.") },
			{ EKind::Deco, { Silver, Ivory, Gold }, TEXT("Elef\u00e1ntcsont gy\u00fajt\u00f3"), TEXT("Ivory lighter"),
				TEXT("Elef\u00e1ntcsontfeh\u00e9r zom\u00e1nc; egy h\u00f6lgy\u00e9, egy sz\u00ednh\u00e1zi p\u00e1holyb\u00f3l."), TEXT("Ivory-white enamel; a lady's, from a theatre box.") },
			{ EKind::Deco, { Gunmetal, Red, Gold }, TEXT("Mozis gy\u00fajt\u00f3"), TEXT("Cinema lighter"),
				TEXT("A Corvin mozi s\u00f6t\u00e9tj\u00e9ben ejtett\u00e9k el."), TEXT("Dropped in the dark of the Corvin cinema.") },
			{ EKind::Table, { Brass, BrownBakelite, Gold }, TEXT("K\u00e1v\u00e9h\u00e1zi gy\u00fajt\u00f3"), TEXT("Coffee house lighter"),
				TEXT("Egy k\u00e1v\u00e9h\u00e1zi asztalr\u00f3l v\u00e1ndorolt el. Senki sem vallja be."), TEXT("It wandered off a coffee house table. Nobody admits to it.") },
			{ EKind::Table, { Chrome, GreenBakelite, Chrome }, TEXT("\u00dcgyv\u00e9di gy\u00fajt\u00f3"), TEXT("Lawyer's lighter"),
				TEXT("Egy \u00fcgyv\u00e9d asztal\u00e1r\u00f3l; sok rossz szivart \u00e9s m\u00e9g rosszabb szerz\u0151d\u00e9st l\u00e1tott."), TEXT("From a lawyer's desk; it has seen many bad cigars and worse contracts.") },
			{ EKind::Table, { Brass, Red, Brass }, TEXT("Piros gy\u00fajt\u00f3"), TEXT("Red lighter"),
				TEXT("Piros, mint a t\u0171zolt\u00f3aut\u00f3. B\u00f6zsi n\u00e9ni egy peng\u0151\u00e9rt eladn\u00e1."), TEXT("Red as a fire engine. B\u00f6zsi n\u00e9ni would sell it for a peng\u0151.") },
			{ EKind::Table, { Chrome, Chrome, Gunmetal }, TEXT("Sz\u00e1llodai gy\u00fajt\u00f3"), TEXT("Hotel lighter"),
				TEXT("A Nemzeti Sz\u00e1ll\u00f3 hall\u00e1b\u00f3l k\u00f6lcs\u00f6n\u00f6zt\u00e9k, \u00f6r\u00f6kre."), TEXT("Borrowed for good from the lobby of the Hotel Nemzeti.") },
			{ EKind::Table, { Gold, Onyx, Gold }, TEXT("\u00d3nix gy\u00fajt\u00f3"), TEXT("Onyx lighter"),
				TEXT("Neh\u00e9z \u00f3nix; ajt\u00f3t lehetne kit\u00e1masztani vele."), TEXT("Heavy onyx; you could prop a door open with it.") },
			{ EKind::Lever, { Chrome, Gunmetal, Gunmetal }, TEXT("Karos gy\u00fajt\u00f3"), TEXT("Lever lighter"),
				TEXT("Megnyomod a kart, \u00e9s felpattan a kupak. \u00dcgyes."), TEXT("Press the lever and the cap flies up. Clever.") },
			{ EKind::Lever, { Brass, Lapis, DarkBrass }, TEXT("Amerikai gy\u00fajt\u00f3"), TEXT("American lighter"),
				TEXT("Egy matr\u00f3z zseb\u00e9ben j\u00f6tt Amerik\u00e1b\u00f3l."), TEXT("It came from America in a sailor's pocket.") },
			{ EKind::Lever, { Gunmetal, Black, Nickel }, TEXT("Detekt\u00edv gy\u00fajt\u00f3ja"), TEXT("The detective's lighter"),
				TEXT("Egy detekt\u00edv\u00e9. Vissza fogja k\u00e9rni."), TEXT("A detective's. He will want it back.") },
			{ EKind::Lever, { Nickel, Nickel, Gunmetal }, TEXT("Vasutas gy\u00fajt\u00f3"), TEXT("Railwayman's lighter"),
				TEXT("A s\u00ednek mellett tal\u00e1lt\u00e1k; z\u00f6r\u00f6g, mint a vonat."), TEXT("Found by the rails; it rattles like a train.") },
			{ EKind::Tin, { Tin, Tin, Nickel }, TEXT("B\u00e1doggy\u00fajt\u00f3"), TEXT("Tin lighter"),
				TEXT("Szard\u00ednias dobozb\u00f3l k\u00e9sz\u00fclt otthon. T\u00f6bbnyire m\u0171k\u00f6dik."), TEXT("Homemade from a sardine tin. It works, mostly.") },
			{ EKind::Tin, { Rust, Rust, Tin }, TEXT("Rozsd\u00e1s gy\u00fajt\u00f3"), TEXT("Rusty lighter"),
				TEXT("Rozsd\u00e1s \u00e9s horpadt; egy utcasepr\u0151 kincse."), TEXT("Rusty and dented; a street sweeper's treasure.") },
			{ EKind::Tin, { ParkGreen, ParkGreen, Tin }, TEXT("Z\u00f6ld b\u00e1dog"), TEXT("Green tin"),
				TEXT("Parkpad-fest\u00e9k marad\u00e9k\u00e1val festett\u00e9k be."), TEXT("Painted with leftover park bench paint.") },
			{ EKind::Tin, { Tin, Copper, Copper }, TEXT("Dr\u00f3tos gy\u00fajt\u00f3"), TEXT("Tinker's lighter"),
				TEXT("Egy dr\u00f3tost\u00f3t fogta \u00f6ssze r\u00e9zdr\u00f3ttal."), TEXT("A travelling tinker held it together with copper wire.") },
		};
		return List;
	}

	/** The reward for finding all thirty. */
	inline const FLighter& Golden()
	{
		using namespace Colors;
		static const FLighter Reward = { EKind::Golden, { Gold, Gold, DarkBrass }, TEXT("Lucifer"), TEXT("Lucifer"),
			TEXT("Arany \u00f6ngy\u00fajt\u00f3, a l\u00e1ngja sosem alszik ki. Nem eml\u00e9kszel, hogy vetted volna."),
			TEXT("A golden lighter whose flame never goes out. You don't remember buying it.") };
		return Reward;
	}

	/** What a lighter's hiding place is, for Pista bácsi's tips. */
	enum class EWhere : uint8 { Bench, ParkBench, Gateway, TramStop, Column, Market, Cafe, Keleti, TaxiStand };

	/**
	 * Where a lighter lies: a map point (cm, X east, Y south), its height
	 * above whatever is there (a bench seat 47.5, a market table 80; 0 on
	 * the ground, on a column's plinth or on a café table, which are solid;
	 * a Keleti platform's top is 40 over the station's floor),
	 * which way it faces, and the tram stop it is nearest to. The one at
	 * Keleti is given in the station's own frame (AKeletiStation::ToCity).
	 * Picked from the street furniture file (Data/OsmGraybox), spread across
	 * the map.
	 */
	struct FSpot
	{
		double X;
		double Y;
		double Height;
		double Yaw;
		EWhere Where;
		const TCHAR* Near;
	};

	/** The thirty hiding places; the lighter of the same number lies at each. */
	inline const TArray<FSpot>& Spots()
	{
		static const TArray<FSpot> List = {
			{ 101266.0, -37643.0, 0.0, 90.0, EWhere::TaxiStand, nullptr },
			{ -29979.0, 5190.0, 0.0, 66.1, EWhere::Column, TEXT("Klauz\u00e1l utca") },
			{ -24635.0, -61366.0, 0.0, 144.0, EWhere::TramStop, TEXT("Dob utca") },
			{ 3667.0, 64455.0, 0.0, -174.9, EWhere::TramStop, TEXT("J\u00f3zsef utca") },
			{ 57540.0, 21353.0, 0.0, 20.0, EWhere::Cafe, nullptr },
			{ 8620.0, 4836.0, 0.0, 108.2, EWhere::Gateway, TEXT("J\u00f3zsef k\u00f6r\u00fat") },
			{ 76688.0, 29734.0, 0.0, -73.2, EWhere::Gateway, TEXT("Nagy Fuvaros utca") },
			{ -5850.0, 37225.0, 47.5, 133.0, EWhere::Bench, TEXT("B\u00e9rkocsis utca") },
			{ -7223.0, -32724.0, 0.0, -26.1, EWhere::TramStop, TEXT("Barcsay utca") },
			{ 73808.0, 71758.0, 0.0, 83.2, EWhere::Column, TEXT("Teleki t\u00e9r") },
			{ 65906.0, 50064.0, 47.5, 61.6, EWhere::Bench, TEXT("Nagy Fuvaros utca") },
			{ 18177.0, -71243.0, 0.0, 52.5, EWhere::Column, TEXT("Dob utca") },
			{ 74010.0, 5198.0, 47.5, 249.3, EWhere::ParkBench, nullptr },
			{ 45968.0, -52898.0, 47.5, 234.3, EWhere::Bench, TEXT("Husz\u00e1r utca") },
			{ 3600.0, 1720.0, 40.0, 200.0, EWhere::Keleti, nullptr },
			{ 34334.0, -21402.0, 0.0, 68.6, EWhere::TramStop, TEXT("Luther utca") },
			{ 64049.0, -13543.0, 47.5, 106.7, EWhere::ParkBench, nullptr },
			{ 48989.0, 3650.0, 47.5, 162.7, EWhere::ParkBench, nullptr },
			{ 15883.0, 42733.0, 47.5, 247.4, EWhere::Bench, TEXT("B\u00e9rkocsis utca") },
			{ 138832.0, 52672.0, 0.0, -78.8, EWhere::TramStop, TEXT("Fiumei \u00fat") },
			{ 97083.0, 30814.0, 80.0, -120.6, EWhere::Market, nullptr },
			{ 42641.0, 18085.0, 0.0, -70.9, EWhere::Gateway, TEXT("Nagy Fuvaros utca") },
			{ 28881.0, 13866.0, 0.0, -72.2, EWhere::Gateway, TEXT("V\u00edg utca") },
			{ 133729.0, -69997.0, 47.5, 235.6, EWhere::Bench, TEXT("Baross t\u00e9r") },
			{ 10785.0, -51858.0, 47.5, -21.7, EWhere::Bench, TEXT("Barcsay utca") },
			{ 106975.0, 33358.0, 80.0, -120.6, EWhere::Market, nullptr },
			{ 39152.0, 55504.0, 47.5, -17.7, EWhere::Bench, TEXT("Kis Sal\u00e9trom utca") },
			{ 18710.0, 8045.0, 0.0, 109.2, EWhere::Gateway, TEXT("V\u00edg utca") },
			{ 122306.0, 65603.0, 47.5, 148.1, EWhere::Bench, TEXT("Magdolna utca") },
			{ 132421.0, -34922.0, 0.0, 88.3, EWhere::Column, TEXT("Baross t\u00e9r") },
		};
		return List;
	}

	/** A hiding place in a few words, Hungarian and English. */
	inline const TCHAR* WhereHu(EWhere Where)
	{
		switch (Where)
		{
		case EWhere::Bench: return TEXT("egy padon");
		case EWhere::ParkBench: return TEXT("egy padon a Tisza K\u00e1lm\u00e1n t\u00e9ren");
		case EWhere::Gateway: return TEXT("egy kapu el\u0151tt");
		case EWhere::TramStop: return TEXT("egy villamosmeg\u00e1ll\u00f3 t\u00e1bl\u00e1j\u00e1n\u00e1l");
		case EWhere::Column: return TEXT("egy hirdet\u0151oszlop t\u00f6v\u00e9ben");
		case EWhere::Market: return TEXT("a zsibv\u00e1s\u00e1ron, egy asztalon");
		case EWhere::Cafe: return TEXT("a k\u00e1v\u00e9h\u00e1z terasz\u00e1n, egy asztalon");
		case EWhere::Keleti: return TEXT("a Keleti p\u00e1lyaudvar peronj\u00e1n");
		case EWhere::TaxiStand: return TEXT("a Baross t\u00e9ri taxi\u00e1llom\u00e1sn\u00e1l");
		}
		return TEXT("valahol");
	}

	inline const TCHAR* WhereEn(EWhere Where)
	{
		switch (Where)
		{
		case EWhere::Bench: return TEXT("on a bench");
		case EWhere::ParkBench: return TEXT("on a bench in Tisza K\u00e1lm\u00e1n t\u00e9r");
		case EWhere::Gateway: return TEXT("outside a gateway");
		case EWhere::TramStop: return TEXT("by a tram stop sign");
		case EWhere::Column: return TEXT("at the foot of an advertising column");
		case EWhere::Market: return TEXT("on a table at the flea market");
		case EWhere::Cafe: return TEXT("on a table on the caf\u00e9 terrace");
		case EWhere::Keleti: return TEXT("on the platform at Keleti station");
		case EWhere::TaxiStand: return TEXT("by the taxi stand on Baross t\u00e9r");
		}
		return TEXT("somewhere");
	}
}
