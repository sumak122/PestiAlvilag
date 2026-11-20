// Fill out your copyright notice in the Description page of Project Settings.

#include "MissionKulcs.h"

#include "Cafe.h"
#include "CarModel.h"
#include "CarShowModel.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "LaciYard.h"
#include "LaciYardModel.h"
#include "PersonKit.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiCar.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "ShapeSet.h"
#include "StoryPerson.h"
#include "StreetFights.h"

namespace KulcsPlan
{
	constexpr int32 TalkStep = 0;
	constexpr int32 TavernStep = 1;
	constexpr int32 DrinkStep = 2;
	constexpr int32 BackStep = 3;

	// Bauer Károly's tavern on Baross tér: the middle of its front on the
	// house wall, and the way out to the street.
	const FVector2D TavernAt(85880.0, -31729.0);
	constexpr double TavernYaw = -87.1;
	// The table stands this far out from the front, and this far along it (cm, + to the right looking out).
	constexpr double TableOut = 230.0;
	constexpr double TableAlong = 120.0;
	// The importer's limousine at the pavement's edge, a little along.
	constexpr double CarOut = 850.0;
	constexpr double CarAlong = 300.0;
	constexpr double CarLift = 120.0;
	// A seated person's middle over the floor (as at the café tables).
	constexpr double SeatedMiddle = PersonModel::MiddleHeight - PersonKit::SeatDrop + 2.0;

	constexpr float TalkRadius = 250.f;
	constexpr float TavernRadius = 1500.f;
	// Across the table from Kálmán, where the player stands to buy rounds (table frame).
	const FVector Across(125.0, 0.0, 0.0);
	constexpr double AcrossNear = 130.0;
	constexpr double BehindNear = 110.0;

	// Four rounds put him to sleep: beer, wine, then two pálinkas (fillér, before the hírnév cut).
	constexpr int32 RoundCount = 4;
	constexpr int32 RoundPrices[RoundCount] = { 30, 60, 40, 40 };
	// Seconds before the next round can be ordered, and from the last round to his sleep.
	constexpr double RoundGap = 4.0;
	constexpr double NodOffAfter = 7.0;
	// How long he takes to slump over the table (s), how far he leans (degrees, - is forward) and rises meanwhile (cm).
	constexpr float SlumpSeconds = 1.6f;
	constexpr double SlumpPitch = -38.0;
	constexpr double SlumpLift = 8.0;
	// His sway per round drunk (degrees) and how fast it goes (radians a second).
	constexpr double SwayPerRound = 2.5;
	constexpr float SwaySpeed = 1.4f;
	// The fight with him counts this far from where it began (cm).
	constexpr double FightRange = 5000.0;

	constexpr int32 HintKey = 39;
	constexpr int32 NoteKey = 82;
	constexpr float HintSeconds = 6.f;
	constexpr float NoteSeconds = 4.f;
	constexpr double NoteEvery = 3.5;

	const TCHAR* const LaciName = TEXT("Laci");
	const TCHAR* const KalmanName = TEXT("K\u00e1lm\u00e1n");
}

AMissionKulcs::AMissionKulcs()
{
	// Must match PestiStart::Kulcs in the game mode.
	MissionId = TEXT("Kulcs");
	Reward = 800;
	TitleHu = TEXT("A kulcs");
	TitleEn = TEXT("The key");
	Number = 0;
	Random.Initialize(1932);
}

void AMissionKulcs::SetUpMission()
{
	for (TActorIterator<ALaciYard> It(GetWorld()); It; ++It)
	{
		Laci = It->GetLaci();
		break;
	}
	const double YardGround = GroundZ(FVector2D(ALaciYard::ToCity(LaciYardModel::Spots::Laci, 0.0)), 0.0);
	const FVector LaciAt = Laci ? Laci->GetActorLocation() : ALaciYard::ToCity(LaciYardModel::Spots::Laci, YardGround);

	// The table's frame before the tavern.
	TableYaw = KulcsPlan::TavernYaw;
	const FVector2D Out(FMath::Cos(FMath::DegreesToRadians(TableYaw)), FMath::Sin(FMath::DegreesToRadians(TableYaw)));
	const FVector2D Right(-Out.Y, Out.X);
	const FVector2D TableFlat = KulcsPlan::TavernAt + Out * KulcsPlan::TableOut + Right * KulcsPlan::TableAlong;
	TableAt = FVector(TableFlat, GroundZ(TableFlat, YardGround));
	const FTransform Frame(FRotator(0.0, TableYaw, 0.0), TableAt);

	FMissionStep Talk;
	Talk.GoalHu = TEXT("Menj Lacihoz, Szerdahelyi utca");
	Talk.GoalEn = TEXT("Go to Laci on Szerdahelyi utca");
	Talk.Target = LaciAt;
	Talk.Radius = KulcsPlan::TalkRadius;
	Talk.bPressE = true;
	Talk.Place = TEXT("Szerdahelyi utca");
	Talk.SceneSeconds = 16.f;
	Talk.SceneWith = Laci.Get();
	Steps.Add(Talk);
	bStartsAtRing = true;
	StartLetter = TEXT("L");

	FMissionStep Tavern;
	Tavern.GoalHu = TEXT("Menj a Bauer-kocsm\u00e1hoz, Baross t\u00e9r");
	Tavern.GoalEn = TEXT("Go to Bauer's tavern on Baross t\u00e9r");
	Tavern.Target = TableAt;
	Tavern.Radius = KulcsPlan::TavernRadius;
	Tavern.bOnFoot = false;
	Tavern.Place = TEXT("Baross t\u00e9r");
	Steps.Add(Tavern);

	FMissionStep Drink;
	Drink.GoalHu = TEXT("\u00c1llj K\u00e1lm\u00e1nnal szemben, \u00e9s fizess neki egy k\u00f6rt (E)");
	Drink.GoalEn = TEXT("Stand across the table from K\u00e1lm\u00e1n and stand him a round (E)");
	Drink.Target = Frame.TransformPosition(KulcsPlan::Across);
	Drink.Radius = KulcsPlan::AcrossNear;
	Drink.bPressE = true;
	Drink.bManual = true;
	Drink.bGuide = false;
	Drink.Place = TEXT("Baross t\u00e9r");
	Steps.Add(Drink);

	FMissionStep Back;
	Back.GoalHu = TEXT("Vidd a kulcsot Lacinak");
	Back.GoalEn = TEXT("Take the key to Laci");
	Back.Target = LaciAt;
	Back.Radius = KulcsPlan::TalkRadius;
	Back.bPressE = true;
	Back.Place = TEXT("Szerdahelyi utca");
	Back.SceneSeconds = 12.f;
	Back.SceneWith = Laci.Get();
	Steps.Add(Back);
}

void AMissionKulcs::SetTheTable()
{
	if (Kalman || TableSet)
	{
		return;
	}
	const FTransform Frame(FRotator(0.0, TableYaw, 0.0), TableAt);

	// The table, chairs, beer and the coat with the key.
	LayTable();

	// Kálmán in his chair, looking out at the street over his beer.
	const FVector Chair = Frame.TransformPosition(CarShowModel::Table::Chair);
	Kalman = SpawnForMission<ACafeGuest>(Chair + FVector(0.0, 0.0, KulcsPlan::SeatedMiddle), TableYaw + CarShowModel::Table::ChairYaw);
	if (Kalman)
	{
		Kalman->SetOutfit(CarShowModel::ChauffeurAtTable());
	}

	// Mr. Vogel's black limousine, waiting for its chauffeur (locked; looks only).
	const FVector2D Out(FMath::Cos(FMath::DegreesToRadians(TableYaw)), FMath::Sin(FMath::DegreesToRadians(TableYaw)));
	const FVector2D Right(-Out.Y, Out.X);
	const FVector2D CarFlat = KulcsPlan::TavernAt + Out * KulcsPlan::CarOut + Right * KulcsPlan::CarAlong;
	const FTransform Place(FRotator(0.0, TableYaw + 90.0, 0.0), FVector(CarFlat, GroundZ(CarFlat, TableAt.Z) + KulcsPlan::CarLift));
	Limousine = GetWorld()->SpawnActorDeferred<APestiCar>(APestiCar::StaticClass(), Place, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (Limousine)
	{
		CarModel::FStyle Look;
		Look.Body = CarModel::EBody::Limousine;
		Look.PaintColor = FLinearColor(0.02f, 0.02f, 0.025f);
		Look.GuardColor = FLinearColor(0.02f, 0.02f, 0.025f);
		Limousine->BodyColor = Look.PaintColor;
		Limousine->SetStyle(Look);
		Limousine->SetLocked(true);
		Limousine->FinishSpawning(Place);
		AddMissionActor(Limousine.Get());
	}
}

void AMissionKulcs::LayTable()
{
	// A shape set cannot be cleared, so a new one is laid for each round's glasses.
	if (TableSet)
	{
		TableSet->Destroy();
		TableSet = nullptr;
	}
	TableSet = SpawnForMission<AShapeSet>(FVector::ZeroVector, 0.0);
	if (TableSet)
	{
		TArray<MarketModel::FShape> Shapes;
		CarShowModel::BuildTable(Shapes, TableAt, TableYaw, Rounds);
		TableSet->AddShapes(Shapes);
	}
}

void AMissionKulcs::SayLater(float Seconds, const FString& Who, const FString& LineHu, const FString& LineEn)
{
	// Seconds after the line before it (or after now, if none is waiting).
	const double Now = GetWorld()->GetTimeSeconds();
	const double Last = Lines.Num() > 0 ? Lines.Last().At : Now;
	Lines.Add({ FMath::Max(Last, Now) + Seconds, Who, LineHu, LineEn });
}

void AMissionKulcs::Hint(const FString& Text) const
{
	APestiHUD::Show(this, KulcsPlan::HintKey, KulcsPlan::HintSeconds, FColor(255, 210, 60), Text);
}

void AMissionKulcs::OnStepStarted(int32 Step)
{
	if (Step == KulcsPlan::TavernStep)
	{
		SetTheTable();
		return;
	}
	if (Step == KulcsPlan::DrinkStep)
	{
		const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
		ArrestsAtStart = Game ? Game->GetArrests() : 0;
		Rounds = 0;
		NextRoundAt = 0.0;
		SleepsAt = -1.0;
		Asleep = 0.f;
		bAcross = false;
		Hint(TEXT("Tipp: ha el\u0151bb ny\u00falsz a kab\u00e1tj\u00e1ba, mint hogy elalszik, vereked\u00e9s lesz  (Tip: reach into his coat before he sleeps and there'll be a fight)"));
	}
}

void AMissionKulcs::OnStepDone(int32 Step)
{
	const APawn* Player = GetPlayer();
	if (Step == KulcsPlan::TalkStep)
	{
		if (Laci && Player)
		{
			Laci->FaceTowards(Player->GetActorLocation());
		}
		Say(KulcsPlan::LaciName,
			TEXT("K\u00e1lm\u00e1n, a sof\u0151r. Hallottad, hol iszik: a Bauer-kocsma el\u0151tt, a Baross t\u00e9ren."),
			TEXT("K\u00e1lm\u00e1n, the chauffeur. You heard where he drinks: before Bauer's tavern on Baross t\u00e9r."), 6.f);
		SayLater(6.f, KulcsPlan::LaciName,
			TEXT("\u00c1llj vele szemben, \u00e9s fizesd a k\u00f6r\u00f6ket. S\u00f6r, bor, p\u00e1linka, am\u00edg el nem alszik. Vigy\u00e9l p\u00e1r peng\u0151t."),
			TEXT("Stand across from him and pay for the rounds. Beer, wine, p\u00e1linka, till he falls asleep. Bring a few peng\u0151."));
		SayLater(6.f, KulcsPlan::LaciName,
			TEXT("Akkor a kulcs mag\u00e1t\u00f3l kics\u00faszik a zseb\u00e9b\u0151l. De ha el\u0151bb ny\u00falsz bele, abb\u00f3l buny\u00f3 lesz."),
			TEXT("Then the key slips out of his pocket by itself. But reach in any sooner and it'll come to blows."));
		return;
	}
	if (Step == KulcsPlan::DrinkStep)
	{
		if (!bFighting)
		{
			Hint(TEXT("Megvan a kulcs. K\u00e1lm\u00e1n csak horkol tov\u00e1bb  (You have the key. K\u00e1lm\u00e1n just snores on)"));
		}
		return;
	}
	if (Step == KulcsPlan::BackStep)
	{
		Lines.Reset();
		if (Laci && Player)
		{
			Laci->FaceTowards(Player->GetActorLocation());
		}
		// Mr. Vogel's car stays at the kerb; its chauffeur will be along.
		if (IsValid(Limousine.Get()))
		{
			KeepAfterMission(Limousine.Get());
		}
		Say(KulcsPlan::LaciName,
			TEXT("Mercedes-Benz, Stuttgart. Ez az, ez nyitja a fekete sz\u00e9ps\u00e9get. J\u00f3l csin\u00e1ltad."),
			TEXT("Mercedes-Benz, Stuttgart. That's the one, it opens the black beauty. Well done."), 6.f);
		SayLater(6.f, KulcsPlan::LaciName,
			TEXT("Most m\u00e1r csak az \u00e9jjeli\u0151r \u00e1ll k\u00f6zt\u00fcnk \u00e9s az aut\u00f3 k\u00f6z\u00f6tt. Arra m\u00e9g kital\u00e1lok valamit."),
			TEXT("Now only the night watchman stands between us and the car. I'll think of something for him."));
	}
}

void AMissionKulcs::TickStep(int32 Step, float DeltaSeconds)
{
	// Lines said a little later, in turn.
	const double Now = GetWorld()->GetTimeSeconds();
	if (Lines.Num() > 0 && Now >= Lines[0].At)
	{
		Say(Lines[0].Who, Lines[0].Hu, Lines[0].En, 7.f);
		Lines.RemoveAt(0);
	}
	if (Step == KulcsPlan::DrinkStep)
	{
		TickDrinks(DeltaSeconds);
	}
}

void AMissionKulcs::TickDrinks(float DeltaSeconds)
{
	const double Now = GetWorld()->GetTimeSeconds();
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (Game && Game->GetArrests() > ArrestsAtStart)
	{
		Fail(TEXT("Elkaptak a rend\u0151r\u00f6k"), TEXT("The police caught you"));
		return;
	}

	if (bFighting)
	{
		// Beaten, the key is his no longer; run off, and it is gone with him.
		if (AStreetFights::IsBeatenNear(this, FightAt, KulcsPlan::FightRange))
		{
			Say(KulcsPlan::KalmanName, TEXT("J\u00f3l van, j\u00f3l van! Vidd a nyavaly\u00e1s kulcsot!"), TEXT("All right, all right! Take the blasted key!"), 5.f);
			FinishStep();
		}
		else if (!AStreetFights::IsFightNear(this, FightAt, KulcsPlan::FightRange))
		{
			Fail(TEXT("K\u00e1lm\u00e1n elfutott a kulccsal"), TEXT("K\u00e1lm\u00e1n ran off with the key"));
		}
		return;
	}
	if (!Kalman || !Game)
	{
		return;
	}

	// He sways more with every round, and after the last one slumps over the table.
	const FTransform Frame(FRotator(0.0, TableYaw, 0.0), TableAt);
	if (SleepsAt >= 0.0 && Now >= SleepsAt && Asleep <= 0.f)
	{
		Say(KulcsPlan::KalmanName, TEXT("(horkol)"), TEXT("(snores)"), 5.f);
		Steps[KulcsPlan::DrinkStep].GoalHu = TEXT("K\u00e1lm\u00e1n alszik. \u00c1llj a sz\u00e9ke m\u00f6g\u00e9, \u00e9s vedd ki a kulcsot (E)");
		Steps[KulcsPlan::DrinkStep].GoalEn = TEXT("K\u00e1lm\u00e1n is asleep. Stand behind his chair and take the key (E)");
		SetStepTarget(Frame.TransformPosition(CarShowModel::Table::Behind));
		Asleep = KINDA_SMALL_NUMBER;
	}
	if (Asleep > 0.f)
	{
		Asleep = FMath::Min(1.f, Asleep + DeltaSeconds / KulcsPlan::SlumpSeconds);
	}
	SwayClock += DeltaSeconds * KulcsPlan::SwaySpeed;
	const double Slump = FMath::SmoothStep(0.f, 1.f, Asleep);
	const double Sway = (1.0 - Slump) * KulcsPlan::SwayPerRound * Rounds * FMath::Sin(SwayClock);
	const double Nod = (1.0 - Slump) * 0.5 * KulcsPlan::SwayPerRound * Rounds * FMath::Sin(SwayClock * 0.7f);
	const FVector Seat = Frame.TransformPosition(CarShowModel::Table::Chair) + FVector(0.0, 0.0, KulcsPlan::SeatedMiddle + Slump * KulcsPlan::SlumpLift);
	Kalman->SetActorLocationAndRotation(Seat, FRotator(Slump * KulcsPlan::SlumpPitch - FMath::Abs(Nod), TableYaw + CarShowModel::Table::ChairYaw, Sway));

	APestiPerson* Person = Cast<APestiPerson>(GetPlayer());
	const APlayerController* PC = Person ? Cast<APlayerController>(Person->GetController()) : nullptr;
	if (!Person || !PC)
	{
		return;
	}
	const bool bPressed = PC->WasInputKeyJustPressed(EKeys::E);

	// The hand in his coat: fine once he sleeps, a fight while he is awake.
	const FVector Behind = Frame.TransformPosition(CarShowModel::Table::Behind);
	if (FVector::Dist2D(Person->GetActorLocation(), Behind) < KulcsPlan::BehindNear && bPressed)
	{
		if (Asleep >= 1.f)
		{
			FinishStep();
		}
		else
		{
			Caught();
		}
		return;
	}

	// Across the table: a greeting, then rounds while there are rounds to buy.
	const FVector AcrossAt = Frame.TransformPosition(KulcsPlan::Across);
	const bool bThere = FVector::Dist2D(Person->GetActorLocation(), AcrossAt) < KulcsPlan::AcrossNear;
	if (!bThere)
	{
		bAcross = false;
		return;
	}
	if (!bAcross)
	{
		bAcross = true;
		if (Rounds == 0)
		{
			Say(KulcsPlan::KalmanName, TEXT("Mit b\u00e1mul? Ez az \u00e9n asztalom. Hacsak nem fizet valamit."),
				TEXT("What are you staring at? This is my table. Unless you're buying."), 5.f);
		}
	}
	if (Rounds >= KulcsPlan::RoundCount)
	{
		return;
	}
	const int32 Price = APestiAlvilagGameModeBase::PriceFor(this, KulcsPlan::RoundPrices[Rounds]);
	if (bPressed && Now >= NextRoundAt)
	{
		if (Game->GetMoney() < Price)
		{
			Hint(TEXT("Nincs el\u00e9g p\u00e9nzed a k\u00f6rre  (You haven't the money for the round)"));
			return;
		}
		Game->AddMoney(-Price);
		++Rounds;
		NextRoundAt = Now + KulcsPlan::RoundGap;
		LayTable();
		switch (Rounds)
		{
		case 1:
			Say(KulcsPlan::KalmanName, TEXT("Egy s\u00f6r? H\u00e1t, ha maga fizet, ki vagyok \u00e9n, hogy nemet mondjak? Eg\u00e9szs\u00e9g\u00e9re!"),
				TEXT("A beer? Well, if you're paying, who am I to say no? Your health!"), 5.f);
			break;
		case 2:
			Say(KulcsPlan::KalmanName, TEXT("Tudja, a gazd\u00e1m, Vogel \u00far, egy poh\u00e1r vizet se adna. Maga m\u00e1s. Maga \u00fariember."),
				TEXT("You know, my master, Mr. Vogel, wouldn't give me a glass of water. You're different. You're a gentleman."), 5.f);
			break;
		case 3:
			Say(KulcsPlan::KalmanName, TEXT("Barackp\u00e1linka... ez azt\u00e1n! Mondtam m\u00e1r, hogy maga rendes ember? Mert az."),
				TEXT("Apricot brandy... now that's something! Did I say you're a decent fellow? Because you are."), 5.f);
			break;
		default:
			Say(KulcsPlan::KalmanName, TEXT("M\u00e9g egy... utols\u00f3... a kulcsot... j\u00f3zanul kell... visszavinnem..."),
				TEXT("One more... last one... the key... got to take it back... sober..."), 5.f);
			SleepsAt = Now + KulcsPlan::NodOffAfter;
			break;
		}
		return;
	}
	if (Now >= NextRoundAt && Now - LastNote > KulcsPlan::NoteEvery)
	{
		LastNote = Now;
		static const TCHAR* const DrinkHu[KulcsPlan::RoundCount] = { TEXT("egy kors\u00f3 s\u00f6r"), TEXT("egy poh\u00e1r bor"), TEXT("egy p\u00e1linka"), TEXT("m\u00e9g egy p\u00e1linka") };
		static const TCHAR* const DrinkEn[KulcsPlan::RoundCount] = { TEXT("a mug of beer"), TEXT("a glass of wine"), TEXT("a p\u00e1linka"), TEXT("another p\u00e1linka") };
		const FString Money = APestiAlvilagGameModeBase::FormatMoney(Price);
		APestiHUD::Show(this, KulcsPlan::NoteKey, KulcsPlan::NoteSeconds, FColor(255, 210, 60),
			FString(TEXT("E: ")) + DrinkHu[Rounds] + TEXT(", ") + Money + TEXT("  (E: ") + DrinkEn[Rounds] + TEXT(", ") + Money + TEXT(")"));
	}
}

void AMissionKulcs::Caught()
{
	APestiPerson* Person = Cast<APestiPerson>(GetPlayer());
	if (!Kalman || !Person)
	{
		return;
	}
	const FVector Chair = Kalman->GetActorLocation() - FVector(0.0, 0.0, KulcsPlan::SeatedMiddle);
	const FVector ToPlayer = Person->GetActorLocation() - Chair;
	const double FaceYaw = FMath::RadiansToDegrees(FMath::Atan2(ToPlayer.Y, ToPlayer.X));
	Kalman->SetActorRotation(FRotator(0.0, FaceYaw, 0.0));

	// Up he gets, in his waistcoat, fists ready.
	PersonKit::FOutfit Dress = CarShowModel::ChauffeurAtTable();
	Dress.bSeated = false;
	AStreetFights* Fights = AStreetFights::Get(this);
	if (!Fights || !Fights->Begin(Person, Dress, Chair, FaceYaw))
	{
		// The player is busy (mid-punch, mid-jump): he tries again next frame.
		return;
	}
	Fights->SetFightsBack();
	bFighting = true;
	FightAt = Chair;
	Say(KulcsPlan::KalmanName, TEXT("H\u00e9! Mit keresel a zsebemben, te tolvaj?!"), TEXT("Hey! What are you doing in my pocket, you thief?!"), 4.f);
	Hint(TEXT("Verd meg K\u00e1lm\u00e1nt, \u00e9s ti\u00e9d a kulcs  (Beat K\u00e1lm\u00e1n and the key is yours)"));
	Kalman->Destroy();
	Kalman = nullptr;
}

bool AMissionKulcs::GetRetrySpot(FVector& OutAt, double& OutYaw) const
{
	const FVector Gate = LaciYardModel::Spots::Gate + FVector(300.0, 0.0, 0.0);
	const FVector2D Flat(ALaciYard::ToCity(Gate, 0.0));
	OutAt = FVector(Flat, GroundZ(Flat, 0.0) + PersonModel::MiddleHeight + 10.0);
	OutYaw = LaciYardModel::CityYaw;
	return true;
}

void AMissionKulcs::OnFailed()
{
	Lines.Reset();
}
