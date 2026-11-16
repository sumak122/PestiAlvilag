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
	constexpr int32 LiftStep = 2;
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
	constexpr float LiftRadius = 130.f;

	// Holding E this long gets the key out (s).
	constexpr float LiftSeconds = 3.f;
	// Seconds between his glances, the glance's length, how long his head
	// takes to turn, how far it turns (degrees) and how far turned he sees
	// the hand in his coat (0 to 1).
	constexpr float GlanceMin = 4.f;
	constexpr float GlanceMax = 8.f;
	constexpr float GlanceSeconds = 2.4f;
	constexpr float TurnSeconds = 0.9f;
	constexpr double TurnAngle = 70.0;
	constexpr float SeesAt = 0.7f;
	// The fight with him counts this far from where it began (cm).
	constexpr double FightRange = 5000.0;

	constexpr int32 HintKey = 39;
	constexpr int32 BarKey = 82;
	constexpr float HintSeconds = 6.f;
	constexpr int32 BarLength = 20;

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

	FMissionStep Lift;
	Lift.GoalHu = TEXT("\u00c1llj K\u00e1lm\u00e1n sz\u00e9ke m\u00f6g\u00e9, \u00e9s tartsd lenyomva az E-t, hogy kivedd a kulcsot");
	Lift.GoalEn = TEXT("Stand behind K\u00e1lm\u00e1n's chair and hold E to lift the key");
	Lift.Target = Frame.TransformPosition(CarShowModel::Table::Behind);
	Lift.Radius = KulcsPlan::LiftRadius;
	Lift.bPressE = true;
	Lift.bManual = true;
	Lift.bGuide = false;
	Lift.Place = TEXT("Baross t\u00e9r");
	Steps.Add(Lift);

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
	TableSet = SpawnForMission<AShapeSet>(FVector::ZeroVector, 0.0);
	if (TableSet)
	{
		TArray<MarketModel::FShape> Shapes;
		CarShowModel::BuildTable(Shapes, TableAt, TableYaw);
		TableSet->AddShapes(Shapes);
	}

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
	if (Step == KulcsPlan::LiftStep)
	{
		const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
		ArrestsAtStart = Game ? Game->GetArrests() : 0;
		Lifted = 0.f;
		Glancing = -1.f;
		GlanceIn = Random.FRandRange(KulcsPlan::GlanceMin, KulcsPlan::GlanceMax);
		Hint(TEXT("Tipp: ha K\u00e1lm\u00e1n h\u00e1trafordul, engedd el az E-t  (Tip: when K\u00e1lm\u00e1n looks round, let go of E)"));
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
			TEXT("A sof\u0151r, K\u00e1lm\u00e1n. Ilyenkor a Bauer-kocsma el\u0151tt iszik, a kab\u00e1tja a sz\u00e9ke t\u00e1ml\u00e1j\u00e1n l\u00f3g."),
			TEXT("The chauffeur, K\u00e1lm\u00e1n. This time of day he drinks before Bauer's tavern, his coat hanging on the back of his chair."), 6.f);
		SayLater(6.f, KulcsPlan::LaciName,
			TEXT("A kulcs a kab\u00e1tzsebben van. \u00c1llj m\u00f6g\u00e9, \u00e9s ny\u00falj bele, de csak ha az utc\u00e1t b\u00e1mulja."),
			TEXT("The key is in the coat pocket. Stand behind him and reach in, but only while he's staring at the street."));
		SayLater(6.f, KulcsPlan::LaciName,
			TEXT("Ha \u00e9szrevesz, nagy h\u0171h\u00f3 lesz. Az a fick\u00f3 nem keny\u00e9rre keni a pofonokat."),
			TEXT("If he spots you, there'll be a fuss. That fellow doesn't hand out slaps on bread."));
		return;
	}
	if (Step == KulcsPlan::LiftStep)
	{
		APestiHUD::Show(this, KulcsPlan::BarKey, 0.1f, FColor(255, 210, 60), FString());
		if (!bFighting)
		{
			Hint(TEXT("Megvan a kulcs. K\u00e1lm\u00e1n semmit sem vett \u00e9szre  (You have the key. K\u00e1lm\u00e1n noticed nothing)"));
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
	if (Step == KulcsPlan::LiftStep)
	{
		TickLift(DeltaSeconds);
	}
}

void AMissionKulcs::TickLift(float DeltaSeconds)
{
	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
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
	if (!Kalman)
	{
		return;
	}

	// His glances over his shoulder: turn, look a moment, turn back.
	const double BaseYaw = TableYaw + CarShowModel::Table::ChairYaw;
	float Turned = 0.f;
	if (Glancing < 0.f)
	{
		GlanceIn -= DeltaSeconds;
		if (GlanceIn <= 0.f)
		{
			Glancing = 0.f;
			GlanceSide = Random.FRand() < 0.5f ? -1.0 : 1.0;
		}
	}
	else
	{
		Glancing += DeltaSeconds;
		const float Back = KulcsPlan::GlanceSeconds - Glancing;
		Turned = FMath::Clamp(FMath::Min(Glancing, Back) / KulcsPlan::TurnSeconds, 0.f, 1.f);
		if (Glancing >= KulcsPlan::GlanceSeconds)
		{
			Glancing = -1.f;
			Turned = 0.f;
			GlanceIn = Random.FRandRange(KulcsPlan::GlanceMin, KulcsPlan::GlanceMax);
		}
	}
	const double Smooth = FMath::SmoothStep(0.f, 1.f, Turned);
	Kalman->SetActorRotation(FRotator(0.0, BaseYaw + GlanceSide * KulcsPlan::TurnAngle * Smooth, 0.0));

	// The player's hand in the pocket: on foot behind the chair, holding E.
	APestiPerson* Person = Cast<APestiPerson>(GetPlayer());
	const APlayerController* PC = Person ? Cast<APlayerController>(Person->GetController()) : nullptr;
	const bool bBehind = Person && FVector::Dist2D(Person->GetActorLocation(), Steps[KulcsPlan::LiftStep].Target) < KulcsPlan::LiftRadius;
	const bool bReaching = bBehind && PC && PC->IsInputKeyDown(EKeys::E);
	if (bReaching && Turned >= KulcsPlan::SeesAt)
	{
		Caught();
		return;
	}
	if (bReaching)
	{
		Lifted = FMath::Min(1.f, Lifted + DeltaSeconds / KulcsPlan::LiftSeconds);
	}
	if (bBehind && Lifted > 0.f)
	{
		const int32 Done = FMath::RoundToInt(Lifted * KulcsPlan::BarLength);
		const FString Bar = FString(TEXT("Kulcs  [")) + FString::ChrN(Done, TEXT('#')) + FString::ChrN(KulcsPlan::BarLength - Done, TEXT('-')) + TEXT("]  (Key)");
		APestiHUD::Show(this, KulcsPlan::BarKey, 0.5f, FColor(255, 210, 60), Bar);
	}
	if (Lifted >= 1.f)
	{
		FinishStep();
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
	APestiHUD::Show(this, KulcsPlan::BarKey, 0.1f, FColor(255, 210, 60), FString());
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
	APestiHUD::Show(this, KulcsPlan::BarKey, 0.1f, FColor(255, 210, 60), FString());
}
