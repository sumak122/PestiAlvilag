// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * The player's street name (hírnév) and what goes with it: the five ranks
 * and what each gives, the greetings of passers-by, Bözsi néni's lines as
 * she warms to him, the newspaper clippings on his wall (one for each act 1
 * mission) and the employer's letters (one after each mission, and one more
 * for all thirty lighters). The points live in the game mode (AddRenown),
 * the wall and the letters at the home door (AHomeDoor), painted by PestiHud.
 */
namespace StreetName
{
	/** The most points there are. */
	constexpr int32 MaxPoints = 100;

	/** What earns or costs points. */
	constexpr int32 ForMission = 8;
	constexpr int32 ForBoxingWin = 4;
	constexpr int32 ForLighter = 1;
	constexpr int32 ForTaxiFare = 1;
	constexpr int32 ForCartRun = 1;
	constexpr int32 ForDeliveryRound = 1;
	constexpr int32 ForRaceWin = 3;
	constexpr int32 ForKnockingDown = -1;
	constexpr int32 ForRunningOver = -4;
	constexpr int32 ForBeatingUp = -1;
	// Knocking a constable down in a fist fight: the underworld hears of it.
	constexpr int32 ForBeatingPolice = 2;
	// Beating a tough who came up to pick a fight with the player.
	constexpr int32 ForBeatingChallenger = 2;

	struct FRank
	{
		/** Points from which he has this rank. */
		int32 From;
		const TCHAR* Hu;
		const TCHAR* En;
		/** What the rank gives, said when he reaches it. */
		const TCHAR* GiftHu;
		const TCHAR* GiftEn;
		/** Shops, cafés and cabbies take this much off (per cent). */
		int32 PriceCut;
	};

	inline const FRank Ranks[] = {
		{ 0, TEXT("Senki"), TEXT("Nobody"), TEXT(""), TEXT(""), 0 },
		{ 10, TEXT("Ismer\u0151s arc"), TEXT("A known face"),
			TEXT("Az utc\u00e1n m\u00e1r k\u00f6sz\u00f6nnek mag\u00e1nak"), TEXT("People in the street greet you now"), 0 },
		{ 30, TEXT("K\u00f6rny\u00e9kbeli"), TEXT("A local"),
			TEXT("Az \u00e9telek \u00e9s a taxi 10%-kal olcs\u00f3bbak"), TEXT("Food and taxi rides cost 10% less"), 10 },
		{ 55, TEXT("Tekint\u00e9ly"), TEXT("A man of standing"),
			TEXT("Tisztelettel sz\u00f3lnak mag\u00e1hoz az utc\u00e1n"), TEXT("People speak to you with respect"), 10 },
		{ 85, TEXT("A negyed embere"), TEXT("The district's man"),
			TEXT("Az \u00e9telek \u00e9s a taxi 20%-kal olcs\u00f3bbak"), TEXT("Food and taxi rides cost 20% less"), 20 },
	};
	constexpr int32 NumRanks = static_cast<int32>(UE_ARRAY_COUNT(Ranks));

	inline int32 RankOf(int32 Points)
	{
		int32 Rank = 0;
		for (int32 i = 0; i < NumRanks; ++i)
		{
			if (Points >= Ranks[i].From)
			{
				Rank = i;
			}
		}
		return Rank;
	}

	/** How far he is from his rank to the next, 0 to 1 (1 at the top). */
	inline float ToNextRank(int32 Points)
	{
		const int32 Rank = RankOf(Points);
		if (Rank + 1 >= NumRanks)
		{
			return 1.f;
		}
		const int32 From = Ranks[Rank].From;
		const int32 To = Ranks[Rank + 1].From;
		return FMath::Clamp(static_cast<float>(Points - From) / static_cast<float>(To - From), 0.f, 1.f);
	}

	struct FLine
	{
		const TCHAR* Hu;
		const TCHAR* En;
	};

	/** What passers-by say when he walks by, for ranks 1 to 4 (three each). */
	inline const FLine Greetings[4][3] = {
		{
			{ TEXT("J\u00f3 napot!"), TEXT("Good day!") },
			{ TEXT("Mag\u00e1t m\u00e1r l\u00e1ttam valahol."), TEXT("I've seen you somewhere before.") },
			{ TEXT("Szervusz, fiatalember."), TEXT("Hello there, young man.") },
		},
		{
			{ TEXT("Al\u00e1szolg\u00e1ja, fiatal\u00far!"), TEXT("Your servant, young sir!") },
			{ TEXT("Na, mi \u00fajs\u00e1g a k\u00f6rny\u00e9ken?"), TEXT("Well, what's new round here?") },
			{ TEXT("J\u00f3 l\u00e1tni, fiatal\u00far."), TEXT("Good to see you, young sir.") },
		},
		{
			{ TEXT("Tiszteletem, uram!"), TEXT("My respects, sir!") },
			{ TEXT("Ha b\u00e1rmire sz\u00fcks\u00e9ge van, csak sz\u00f3ljon."), TEXT("If you need anything, just say.") },
			{ TEXT("Mag\u00e1r\u00f3l besz\u00e9l az eg\u00e9sz k\u00e1v\u00e9h\u00e1z."), TEXT("The whole caf\u00e9 is talking about you.") },
		},
		{
			{ TEXT("A negyed embere! Tiszteletem!"), TEXT("The district's man! My respects!") },
			{ TEXT("Uram, megtisztel, hogy erre j\u00e1r."), TEXT("Sir, you honour us by coming this way.") },
			{ TEXT("Kalapot le, uraim, \u0151 az!"), TEXT("Hats off, gentlemen, it's him!") },
		},
	};

	/** Where each act 1 mission's deed was done, for "I heard what you did ..." (Hungarian, then English). */
	inline const FLine DeedPlaces[] = {
		{ TEXT("a Keletin\u00e9l"), TEXT("at Keleti") },
		{ TEXT("a Teleki t\u00e9ren"), TEXT("on Teleki t\u00e9r") },
		{ TEXT("a v\u00e1s\u00e1rcsarnokban"), TEXT("in the market hall") },
		{ TEXT("a zsibv\u00e1s\u00e1rn\u00e1l"), TEXT("at the flea market") },
		{ TEXT("a Garay utc\u00e1ban"), TEXT("on Garay utca") },
		{ TEXT("Varga \u00farn\u00e1l"), TEXT("for Mr Varga") },
		{ TEXT("a Kiss J\u00f3zsef utc\u00e1ban"), TEXT("on Kiss J\u00f3zsef utca") },
		{ TEXT("a N\u00e9psz\u00ednh\u00e1z utc\u00e1ban"), TEXT("on N\u00e9psz\u00ednh\u00e1z utca") },
	};

	/**
	 * Bözsi néni at her stall, as she warms to him: by the act 1 missions
	 * he has done (the first line before mission 3, the last after mission 8).
	 */
	struct FBozsiLine
	{
		int32 FromMissions;
		const TCHAR* Hu;
		const TCHAR* En;
	};
	inline const FBozsiLine BozsiLines[] = {
		{ 0, TEXT("Mit akar? Nem l\u00e1tja, hogy dolgozom?"), TEXT("What do you want? Can't you see I'm working?") },
		{ 3, TEXT("Maga az? Na j\u00f3... k\u00f6sz\u00f6n\u00f6m m\u00e9g egyszer a p\u00e9nzt\u00e1rc\u00e1t."), TEXT("Oh, it's you. Well... thank you again for the purse.") },
		{ 5, TEXT("Hallom, a Garay utc\u00e1ban is megfordult. Vigy\u00e1zzon mag\u00e1ra."), TEXT("I hear you were on Garay utca too. Look after yourself.") },
		{ 7, TEXT("Egyen valamit, csont \u00e9s b\u0151r. Nem az \u00e9n dolgom, de akkor is."), TEXT("Eat something, you're skin and bone. Not my business, but still.") },
		{ 8, TEXT("Fiatal\u00far! Csendes a h\u00e1z maga n\u00e9lk\u00fcl. Ezt ne mondja el senkinek."), TEXT("Young sir! The house is quiet without you. Don't tell anyone I said so.") },
	};

	/** The clipping on his wall for each act 1 mission, by its number less one. */
	inline const FLine Clippings[] = {
		{ TEXT("Zs\u00fafolt esti gyors a Keletiben"), TEXT("Crowded evening express at Keleti") },
		{ TEXT("Szalad\u00f3 ifj\u00fa a 28-as villamoson"), TEXT("A lad racing the no. 28 tram") },
		{ TEXT("Elfogt\u00e1k a csarnok zsebtolvaj\u00e1t"), TEXT("Market hall pickpocket caught") },
		{ TEXT("L\u00e1d\u00e1k hullottak a N\u00e9psz\u00ednh\u00e1z utc\u00e1n"), TEXT("Crates rained on N\u00e9psz\u00ednh\u00e1z utca") },
		{ TEXT("Vereked\u00e9s a Garay utc\u00e1ban"), TEXT("Brawl on Garay utca") },
		{ TEXT("Rekordid\u0151: kifut\u00f3fi\u00fa a Keletin\u00e9l"), TEXT("Record time: errand boy at Keleti") },
		{ TEXT("F\u00e9nyes nappal lopt\u00e1k el egy orvos aut\u00f3j\u00e1t"), TEXT("Doctor's motor car stolen in broad daylight") },
		{ TEXT("\u00c9jjeli razzia a N\u00e9psz\u00ednh\u00e1z utc\u00e1ban"), TEXT("Night raid on N\u00e9psz\u00ednh\u00e1z utca") },
	};
	constexpr int32 NumClippings = static_cast<int32>(UE_ARRAY_COUNT(Clippings));

	/**
	 * The employer's letters: one after each act 1 mission (0 to 7), and the
	 * last when all the lighters are found (8). Typed, signed only with a
	 * burnt match; money in the envelope. In Hu, \n breaks a line and an
	 * empty line parts paragraphs.
	 */
	struct FLetter
	{
		const TCHAR* Hu;
		const TCHAR* En;
		int32 Filler;
	};
	inline const FLetter Letters[] = {
		{ TEXT("Uram,\n\nmeg\u00e9rkezett. B\u00f6zsi asszony nem fogja szeretni, de nem is kell.\n\nEgyel\u0151re csak figyeljen, \u00e9s tanulja meg az utc\u00e1kat."),
			TEXT("Sir,\n\nyou have arrived. Mrs B\u00f6zsi will not like you, nor need she.\n\nFor now, only watch, and learn the streets."), 100 },
		{ TEXT("Uram,\n\nPista b\u00e1csi r\u00e9gi ismer\u0151s\u00f6m. Amit t\u0151le hozott, azt ne k\u00e9rdezze.\n\nGyors volt. Ezt \u00e9rt\u00e9kelem."),
			TEXT("Sir,\n\nold Pista is an acquaintance of mine. Do not ask what you carried for him.\n\nYou were quick. I value that."), 100 },
		{ TEXT("Uram,\n\na tolvajt nem kellett volna ennyire megkergetnie. Vagy m\u00e9gis.\n\nB\u00f6zsi asszony most m\u00e1r k\u00f6sz\u00f6n mag\u00e1nak, igaz?"),
			TEXT("Sir,\n\nyou need not have chased the thief so hard. Or perhaps you did.\n\nMrs B\u00f6zsi says good day to you now, does she not?"), 150 },
		{ TEXT("Uram,\n\na l\u00e1d\u00e1k\u00e9rt k\u00e1r, a l\u00f3\u00e9rt nem.\n\nAkik Pista b\u00e1csit zaklatt\u00e1k, nem a maguk szak\u00e1ll\u00e1ra j\u00e1rtak. Ezt m\u00e9g megtudjuk."),
			TEXT("Sir,\n\na pity about the crates, never mind the horse.\n\nThe men who troubled old Pista were not working for themselves. We shall find out for whom."), 150 },
		{ TEXT("Uram,\n\nl\u00e1ttam a Garay utc\u00e1ban. \u00dcgyes volt, de hangos.\n\nA szomsz\u00e9ds\u00e1g most m\u00e1r tudja a nev\u00e9t. Ez hasznos lehet, \u00e9s vesz\u00e9lyes is."),
			TEXT("Sir,\n\nI saw you on Garay utca. Neatly done, but loud.\n\nThe neighbourhood knows your name now. That can be useful, and dangerous too."), 200 },
		{ TEXT("Uram,\n\nVarga \u00far dics\u00e9ri. Mazsola kev\u00e9sb\u00e9.\n\nA gyors l\u00e1b\u00fa fi\u00faknak sok bar\u00e1tjuk \u00e9s sok irigy\u00fck van. V\u00e1logassa meg \u0151ket."),
			TEXT("Sir,\n\nMr Varga praises you. Mazsola less so.\n\nQuick-footed lads have many friends and many envious ones. Choose them with care."), 200 },
		{ TEXT("Uram,\n\na doktor aut\u00f3ja sz\u00e9p munka volt. Laci nem k\u00e9rdez, \u00e9s maga se k\u00e9rdezze \u0151t.\n\nA doktor viszont k\u00e9rdezni fog. Ker\u00fclje egy darabig a Kiss J\u00f3zsef utc\u00e1t."),
			TEXT("Sir,\n\nthe doctor's car was fine work. Laci asks no questions, and nor should you ask him.\n\nThe doctor, though, will ask. Keep away from Kiss J\u00f3zsef utca for a while."), 250 },
		{ TEXT("Uram,\n\na bunda \u00fcgy\u00e9t lez\u00e1rtnak tekintem. B\u00f6zsi asszonynak nem esett baja, \u00e9s ezt mag\u00e1nak k\u00f6sz\u00f6nheti.\n\nA padl\u00e1sszoba az \u00f6n\u00e9, a lakb\u00e9rt rendeztem. Hamarosan nagyobb munk\u00e1t kap."),
			TEXT("Sir,\n\nI consider the matter of the fur coat closed. Mrs B\u00f6zsi came to no harm, and she owes that to you.\n\nThe attic room is yours; I have paid the rent. Soon you will have bigger work."), 300 },
		{ TEXT("Uram,\n\nmegtal\u00e1lta mind a harmincat. Tudtam, hogy t\u00fcrelmes.\n\nAz aranyat tartsa meg: aki ismeri, tudni fogja, ki\u00e9.\n\nHa elj\u00f6n az ideje, keresse a rakparton a Kapit\u00e1nyt."),
			TEXT("Sir,\n\nyou have found all thirty. I knew you were patient.\n\nKeep the golden one: whoever knows it will know whose it is.\n\nWhen the time comes, look for the Captain on the quay."), 500 },
	};
	constexpr int32 NumLetters = static_cast<int32>(UE_ARRAY_COUNT(Letters));
	/** The letter that comes with all the lighters. */
	constexpr int32 LighterLetter = NumLetters - 1;

	/** Bözsi néni's postcard, on his wall after the raid (mission 8). */
	inline const FLine Postcard = { TEXT("Kedves Fiatal\u00far!\nA h\u00e1z csendes maga n\u00e9lk\u00fcl.\nEz nem panasz.\n>B\u00f6zsi"), TEXT("Dear young sir! The house is quiet without you. That is not a complaint. B\u00f6zsi") };

	// ---------------------------------------------------------------------
	// Deeds the papers and the gossips get hold of (GameMode::NoteDeed): the
	// biggest of a day is on the kiosks' news boards next morning, and now and
	// then a passer-by tells another about one of the last two days.

	enum class EDeed : uint8
	{
		CarStolen, BikeStolen, CartStolen, RunOver, ConstableBeaten, PasserbyBeaten, ToughBeaten,
		BoxSmashed, PoliceRammed, Arrested, BoxingWin, RaceWin, Count
	};

	struct FDeedNews
	{
		/** How much the papers care: the biggest of the day makes the headline. */
		int32 Weight;
		/** The headline (Hungarian in capitals); {HOL}/{WHERE} is the place. */
		const TCHAR* HeadHu;
		const TCHAR* HeadEn;
		/** A passer-by telling another; {MIKOR}/{WHEN} is today or yesterday. */
		const TCHAR* GossipHu;
		const TCHAR* GossipEn;
	};
	inline const FDeedNews DeedNews[] = {
		{ 7, TEXT("AUT\u00d3T LOPTAK {HOL}"), TEXT("MOTOR CAR STOLEN {WHERE}"),
			TEXT("Hallotta? {MIKOR} valaki elk\u00f6t\u00f6tt egy aut\u00f3t {HOL}!"), TEXT("Did you hear? {WHEN} someone made off with a motor car {WHERE}!") },
		{ 2, TEXT("BICIKLITOLVAJ {HOL}"), TEXT("BICYCLE THIEF {WHERE}"),
			TEXT("{MIKOR} {HOL} egy fick\u00f3 ellopott egy biciklit, k\u00e9pzelje."), TEXT("{WHEN} {WHERE} some fellow stole a bicycle, imagine.") },
		{ 4, TEXT("ELHAJTOTT\u00c1K A KOCSIT {HOL}"), TEXT("CART DRIVEN OFF {WHERE}"),
			TEXT("Szeg\u00e9ny fuvaros... {MIKOR} elhajtott\u00e1k a lov\u00e1t {HOL}."), TEXT("Poor carter... {WHEN} they drove his horse off {WHERE}.") },
		{ 8, TEXT("G\u00c1ZOL\u00c1S {HOL}"), TEXT("RUN DOWN {WHERE}"),
			TEXT("Hallotta? {MIKOR} valakit elg\u00e1zoltak {HOL}! Sz\u00f6rny\u0171."), TEXT("Did you hear? {WHEN} someone was run down {WHERE}! Terrible.") },
		{ 9, TEXT("REND\u0150RT VERTEK {HOL}"), TEXT("CONSTABLE BEATEN {WHERE}"),
			TEXT("{MIKOR} {HOL} valaki lev\u00e1gott egy rend\u0151rt. Na de ilyet!"), TEXT("{WHEN} {WHERE} someone floored a constable. Well I never!") },
		{ 3, TEXT("UTCAI VEREKED\u00c9S {HOL}"), TEXT("STREET BRAWL {WHERE}"),
			TEXT("{MIKOR} megint verekedtek {HOL}. Hova jut ez a v\u00e1ros?"), TEXT("{WHEN} there was fighting {WHERE} again. What is this city coming to?") },
		{ 3, TEXT("RENDRE TAN\u00cdTOTT\u00c1K A K\u00d6TEKED\u0150T"), TEXT("TOUGH PUT IN HIS PLACE"),
			TEXT("{MIKOR} {HOL} valaki helyre tette azt a k\u00f6teked\u0151t. Ide\u00e9je volt."), TEXT("{WHEN} {WHERE} somebody put that tough in his place. About time.") },
		{ 5, TEXT("BET\u00d6RT\u00c9K A REND\u0150RTELEFONT {HOL}"), TEXT("POLICE TELEPHONE SMASHED {WHERE}"),
			TEXT("{MIKOR} {HOL} bez\u00fazt\u00e1k a rend\u0151rtelefont. Tiszta \u00fcveg minden."), TEXT("{WHEN} {WHERE} they smashed the police telephone. Glass everywhere.") },
		{ 6, TEXT("REND\u0150RAUT\u00d3T T\u00d6RTEK {HOL}"), TEXT("POLICE CAR RAMMED {WHERE}"),
			TEXT("{MIKOR} valaki nekihajtott a rend\u0151raut\u00f3nak {HOL}!"), TEXT("{WHEN} someone drove into the police car {WHERE}!") },
		{ 7, TEXT("ELFOGT\u00c1K A GAR\u00c1ZD\u00c1T"), TEXT("TROUBLEMAKER ARRESTED"),
			TEXT("{MIKOR} elkapt\u00e1k azt a gar\u00e1zd\u00e1t {HOL}. De m\u00e1r kint is van, mondj\u00e1k."), TEXT("{WHEN} they caught that troublemaker {WHERE}. They say he's out already.") },
		{ 4, TEXT("\u00daJ N\u00c9V A GRUNDON"), TEXT("A NEW NAME ON THE GRUND"),
			TEXT("{MIKOR} Bandi grundj\u00e1n megint nyert az a fiatal\u00far. Kem\u00e9ny \u00f6kle van."), TEXT("{WHEN} that young gent won again at Bandi's grund. Hard fists, he has.") },
		{ 5, TEXT("\u00c9JSZAKAI AUT\u00d3VERSENY"), TEXT("NIGHT MOTOR RACE"),
			TEXT("Hallotta az \u00e9jjel a motorz\u00fag\u00e1st? {MIKOR} versenyeztek a k\u00f6rny\u00e9ken."), TEXT("Did you hear the engines at night? {WHEN} they raced round here.") },
	};
	static_assert(static_cast<int32>(UE_ARRAY_COUNT(DeedNews)) == static_cast<int32>(EDeed::Count), "one news line for each deed");

	/** The news board on a quiet day (the morning paper's ordinary news). */
	inline const FLine QuietHeadlines[] = {
		{ TEXT("A FRADI GY\u0150Z\u00d6TT AZ \u00dcLL\u0150I \u00daTON"), TEXT("FERENCV\u00c1ROS WON AT \u00dcLL\u0150I \u00daT") },
		{ TEXT("HAVAZ\u00c1S V\u00c1RHAT\u00d3"), TEXT("SNOW EXPECTED") },
		{ TEXT("\u00daJ VILLAMOSKOCSIK A K\u00d6R\u00daTON"), TEXT("NEW TRAMCARS ON THE K\u00d6R\u00daT") },
		{ TEXT("OLCS\u00d3BB A KENY\u00c9R"), TEXT("BREAD IS CHEAPER") },
		{ TEXT("TELT H\u00c1Z A N\u00c9POPER\u00c1BAN"), TEXT("FULL HOUSE AT THE N\u00c9POPERA") },
		{ TEXT("NAGY FORGALOM A KELETIBEN"), TEXT("BUSY DAYS AT KELETI") },
	};
	constexpr int32 NumQuietHeadlines = static_cast<int32>(UE_ARRAY_COUNT(QuietHeadlines));
	/** The paper on the boards. */
	inline const TCHAR* const NewsPaperName = TEXT("PESTI H\u00cdRLAP");

	/** "a Kiss József utcában" for "Kiss József utca": where a deed was done, in Hungarian; empty Street gives "a negyedben". */
	inline FString InStreetHu(const FString& Street)
	{
		if (Street.IsEmpty())
		{
			return TEXT("a negyedben");
		}
		struct FEnding
		{
			const TCHAR* From;
			const TCHAR* To;
		};
		const FEnding Endings[] = {
			{ TEXT(" utca"), TEXT(" utc\u00e1ban") },
			{ TEXT(" k\u00f6r\u00fat"), TEXT(" k\u00f6r\u00faton") },
			{ TEXT(" \u00fat"), TEXT(" \u00faton") },
			{ TEXT(" t\u00e9r"), TEXT(" t\u00e9ren") },
			{ TEXT(" tere"), TEXT(" ter\u00e9n") },
			{ TEXT(" k\u00f6z"), TEXT(" k\u00f6zben") },
			{ TEXT(" sor"), TEXT(" soron") },
			{ TEXT(" s\u00e9t\u00e1ny"), TEXT(" s\u00e9t\u00e1nyon") },
		};
		FString In = Street + TEXT(" k\u00f6rny\u00e9k\u00e9n");
		for (const FEnding& Ending : Endings)
		{
			if (Street.EndsWith(Ending.From, ESearchCase::CaseSensitive))
			{
				In = Street.LeftChop(FCString::Strlen(Ending.From)) + Ending.To;
				break;
			}
		}
		// "az" before a vowel.
		const FString Vowels = TEXT("AEIOU\u00c1\u00c9\u00cd\u00d3\u00d6\u0150\u00da\u00dc\u0170aeiou\u00e1\u00e9\u00ed\u00f3\u00f6\u0151\u00fa\u00fc\u0171");
		int32 Found = INDEX_NONE;
		return (Vowels.FindChar(Street[0], Found) ? TEXT("az ") : TEXT("a ")) + In;
	}

	/** "on Kiss József utca"; empty Street gives "in the district". */
	inline FString OnStreetEn(const FString& Street)
	{
		return Street.IsEmpty() ? FString(TEXT("in the district")) : TEXT("on ") + Street;
	}

	/** Breaks a headline into board lines of at most Width letters (at most MaxLines). */
	inline TArray<FString> BoardLines(const FString& Headline, int32 Width = 15, int32 MaxLines = 3)
	{
		TArray<FString> Words;
		Headline.ParseIntoArray(Words, TEXT(" "));
		TArray<FString> Lines;
		FString Current;
		for (const FString& Word : Words)
		{
			if (!Current.IsEmpty() && Current.Len() + 1 + Word.Len() > Width)
			{
				Lines.Add(Current);
				Current.Reset();
			}
			Current = Current.IsEmpty() ? Word : Current + TEXT(" ") + Word;
		}
		if (!Current.IsEmpty())
		{
			Lines.Add(Current);
		}
		if (Lines.Num() > MaxLines)
		{
			Lines.SetNum(MaxLines);
		}
		return Lines;
	}
}
