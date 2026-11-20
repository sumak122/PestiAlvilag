// Fill out your copyright notice in the Description page of Project Settings.

// The walkers' little moments (APedestrians, declared in Pedestrians.h):
// someone stopping for a moment alone (his watch, a cigarette, the paper,
// a shop window, a shoelace, crumbs for the pigeons), two going opposite
// ways who know each other stopping to talk (or one asking the other for a
// light, or a quarrel), pairs walking together (arm in arm, a mother with
// her child, two friends), old folk bent over their sticks, and the odd
// beggar holding out his cap. All kept rare.

#include "Pedestrians.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PassersbyModel.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "Pigeons.h"
#include "ProceduralMeshComponent.h"
#include "UmbrellaModel.h"

namespace PedestrianActs
{
	// How often someone near the player stops alone for a moment, and how
	// near (not right beside him, not too far to see).
	constexpr float IdleEveryLow = 8.f;
	constexpr float IdleEveryHigh = 16.f;
	constexpr double ActNear = 500.0;
	constexpr double ActFar = 3500.0;
	// How often two passing each other stop to talk, and the rarer scenes
	// (a light, a quarrel) between two men.
	constexpr float MeetEveryLow = 40.f;
	constexpr float MeetEveryHigh = 90.f;
	constexpr float RareEveryLow = 150.f;
	constexpr float RareEveryHigh = 330.f;
	// Two walking towards each other this far apart along their line stop when this near, and stand this far apart.
	constexpr double MeetSeekMin = 300.0;
	constexpr double MeetSeekMax = 1600.0;
	constexpr double MeetStop = 160.0;
	constexpr double TalkGap = 95.0;
	constexpr double LightGap = 80.0;
	constexpr double ArgueGap = 105.0;
	constexpr float MeetGiveUp = 25.f;
	constexpr float StepOver = 70.f;
	constexpr float TurnRate = 220.f;
	// A greeting at the start of a talk, and who speaks, in turns of this long.
	constexpr float GreetTime = 1.6f;
	constexpr float SpeakTurn = 3.4f;
	constexpr double SpeakNear = 900.0;
	// Walking together (Pedestrians::PairShare of the walkers come with company): how far apart they walk.
	constexpr float PairGap = 50.f;
	constexpr float PairLane = 40.f;
	constexpr float StrollPace = 118.f;
	// Old folk on a stick: slower and bent forward.
	constexpr float OldPace = 0.68f;
	constexpr float StoopPitch = 7.f;
	// Beggars: how many walkers are one, how near the player must come to be
	// asked, what a coin is, and how long before he asks the same man again.
	constexpr float BeggarShare = 0.012f;
	constexpr double BegAsk = 420.0;
	constexpr int32 BegCoin = 20;
	constexpr float BegPause = 120.f;
	constexpr double WallReach = 450.0;

	struct FSaying
	{
		const TCHAR* Hu;
		const TCHAR* En;
	};
	const FSaying Greetings[] = {
		{ TEXT("J\u00f3 napot, kedves!"), TEXT("Good day, my dear!") },
		{ TEXT("Szervusz, \u00f6regem! R\u00e9g l\u00e1ttalak."), TEXT("Hello, old chap! Long time no see.") },
		{ TEXT("Na, mi \u00fajs\u00e1g a J\u00f3zsefv\u00e1rosban?"), TEXT("Well, what's new in J\u00f3zsefv\u00e1ros?") },
		{ TEXT("Hogy van a kedves csal\u00e1d?"), TEXT("And how is the family?") },
	};
	const FSaying LadyGreetings[] = {
		{ TEXT("Kez\u00e9t cs\u00f3kolom, nagys\u00e1gos asszony!"), TEXT("My respects, madam!") },
		{ TEXT("Jaj, de r\u00e9g l\u00e1ttalak, dr\u00e1g\u00e1m!"), TEXT("Oh, it's been ages, darling!") },
	};
	const FSaying AskLight = { TEXT("Van egy kis t\u00fcze, uram?"), TEXT("Got a light, sir?") };
	const FSaying GiveLight = { TEXT("Parancsoljon."), TEXT("Here you are.") };
	const FSaying Quarrels[] = {
		{ TEXT("Maga csak ne kiab\u00e1ljon velem!"), TEXT("Don't you shout at me!") },
		{ TEXT("Az \u00e9n p\u00e9nzem volt, maga csal\u00f3!"), TEXT("That was my money, you cheat!") },
		{ TEXT("Menjen a fen\u00e9be!"), TEXT("Go to blazes!") },
		{ TEXT("M\u00e9g hogy \u00e9n? Na de k\u00e9rem!"), TEXT("Me? Well, I never!") },
	};
	const FSaying BegLines[] = {
		{ TEXT("Egy kis alamizsn\u00e1t, nagys\u00e1gos \u00far!"), TEXT("A little something, kind sir!") },
		{ TEXT("Egy fill\u00e9rt egy szeg\u00e9ny \u00f6regnek..."), TEXT("A penny for a poor old man...") },
	};
	const FSaying Thanks = { TEXT("Isten \u00e1ldja meg, uram!"), TEXT("God bless you, sir!") };

	template <int32 N>
	const FSaying& PickSaying(FRandomStream& Random, const FSaying (&Lines)[N])
	{
		return Lines[Random.RandRange(0, N - 1)];
	}

	/** The yaw from From to To (degrees). */
	float YawTo(const FVector2D& From, const FVector2D& To)
	{
		const FVector2D Way = To - From;
		return static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Way.Y, Way.X)));
	}

	bool IsPriestOrNun(const PersonKit::FOutfit& O)
	{
		return O.Hat == PersonKit::EHat::Veil || O.Hat == PersonKit::EHat::ClericHat;
	}
}

// ---------------------------------------------------------------------------
// APedestrian: the things in his hands, and an arm linked with someone.

void APedestrian::ShowActProp(EActProp Prop, bool bShow)
{
	const int32 Index = static_cast<int32>(Prop);
	if (ActProps.Num() < static_cast<int32>(EActProp::Num))
	{
		ActProps.SetNum(static_cast<int32>(EActProp::Num));
	}
	// Where it goes: in the right or left hand, or before the chest.
	int32 OnPiece = PersonKit::ForearmRight;
	FVector At = DailyLifeModel::HandOnForearm(1.0, Outfit.Height);
	FRotator Turn = FRotator::ZeroRotator;
	switch (Prop)
	{
	case EActProp::Watch:
	case EActProp::CrumbBag:
		OnPiece = PersonKit::ForearmLeft;
		At = DailyLifeModel::HandOnForearm(-1.0, Outfit.Height);
		break;
	case EActProp::Paper:
		OnPiece = PersonKit::Body;
		At = FVector(34.0, 0.0, 112.0 * Outfit.Height / 178.0);
		Turn = FRotator(-12.f, 0.f, 0.f);
		break;
	default:
		break;
	}
	USceneComponent* Holder = Pieces.IsValidIndex(OnPiece) ? Pieces[OnPiece].Get() : nullptr;
	UProceduralMeshComponent* Mesh = ActProps[Index];
	// Dressed again since it was made (an umbrella taken out of his hand): made again on the new piece.
	if (Mesh && Mesh->GetAttachParent() != Holder)
	{
		Mesh->DestroyComponent();
		Mesh = nullptr;
		ActProps[Index] = nullptr;
	}
	if (!bShow)
	{
		if (Mesh)
		{
			Mesh->SetVisibility(false);
		}
		return;
	}
	if (!Mesh)
	{
		PersonKit::FPiece Shape;
		switch (Prop)
		{
		case EActProp::Cigarette:
			PassersbyModel::Cigarette(Shape);
			break;
		case EActProp::Watch:
			PassersbyModel::PocketWatch(Shape);
			break;
		case EActProp::Paper:
			PassersbyModel::OpenPaper(Shape);
			break;
		case EActProp::Cap:
			PassersbyModel::HeldOutCap(Shape);
			break;
		default:
			PassersbyModel::CrumbBag(Shape);
			break;
		}
		Mesh = AddProp(Shape, OnPiece, At, Turn);
		if (Mesh)
		{
			Mesh->SetCastShadow(false);
		}
		ActProps[Index] = Mesh;
	}
	if (Mesh)
	{
		Mesh->SetVisibility(true);
	}
}

void APedestrian::HideActProps()
{
	for (UProceduralMeshComponent* Mesh : ActProps)
	{
		if (Mesh)
		{
			Mesh->SetVisibility(false);
		}
	}
}

void APedestrian::LinkArm(int32 Arm, const FRotator& Turn, float Elbow)
{
	if (USceneComponent* Part = Limb(PersonKit::ArmLeft + Arm))
	{
		Part->SetRelativeRotation(Turn);
	}
	PersonModel::PoseElbows(Pieces, Arm == 0 ? Elbow : 0.f, Arm == 1 ? Elbow : 0.f);
	// WalkTo straightens the elbow again next frame.
	bArmsFolded = true;
}

// ---------------------------------------------------------------------------
// APedestrians: who stops, and what he does.

APedestrians::FWalker* APedestrians::FindWalker(const APedestrian* Person)
{
	if (!Person)
	{
		return nullptr;
	}
	for (FWalker& Walker : Walkers)
	{
		if (Walker.Person == Person)
		{
			return &Walker;
		}
	}
	return nullptr;
}

bool APedestrians::IsFree(const FWalker& Walker) const
{
	return IsValid(Walker.Person) && !Walker.bStander && Walker.bPlaced && Walker.React == EReact::None
		&& Walker.Gawk == INDEX_NONE && Walker.Shelter == INDEX_NONE && !Walker.bIndoors && Walker.Act == 0
		&& !Walker.Partner.IsValid() && !Walker.bWaiting && Walker.Stolen == 0
		&& Lines.IsValidIndex(Walker.Line) && !Lines[Walker.Line].bCrossing && !Walker.Person->HasUmbrellaUp();
}

bool APedestrians::WallBeside(const FVector2D& At, double GroundZ, const FVector2D& Heading, float& OutYaw) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PedestrianWall), false, this);
	const FVector From(At, GroundZ + 150.0);
	double Best = PedestrianActs::WallReach;
	bool bFound = false;
	for (const double Side : { -1.0, 1.0 })
	{
		const FVector2D Across = FVector2D(-Heading.Y, Heading.X) * Side;
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, From, From + FVector(Across * PedestrianActs::WallReach, 0.0), ECC_Visibility, Params)
			&& Hit.Distance < Best && FMath::Abs(Hit.ImpactNormal.Z) < 0.3)
		{
			Best = Hit.Distance;
			OutYaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Across.Y, Across.X)));
			bFound = true;
		}
	}
	return bFound;
}

void APedestrians::StartAct(FWalker& Walker, EAct Act, float Seconds, float Yaw)
{
	Walker.Act = static_cast<uint8>(Act);
	Walker.ActLeft = Seconds;
	Walker.ActTime = 0.f;
	Walker.ActYaw = Yaw;
	using EProp = APedestrian::EActProp;
	APedestrian* Person = Walker.Person;
	switch (Act)
	{
	case EAct::Watch:
		Person->ShowActProp(EProp::Watch, true);
		break;
	case EAct::Smoke:
		Person->ShowActProp(EProp::Cigarette, true);
		break;
	case EAct::Paper:
		Person->ShowActProp(EProp::Paper, true);
		break;
	case EAct::Feed:
		Person->ShowActProp(EProp::CrumbBag, true);
		break;
	case EAct::Beg:
		Person->ShowActProp(EProp::Cap, true);
		Walker.bAsked = false;
		break;
	default:
		break;
	}
}

void APedestrians::EndAct(FWalker& Walker)
{
	if (Walker.Act == 0)
	{
		return;
	}
	APedestrian* Other = Walker.ActWith.Get();
	Walker.Act = 0;
	Walker.bMeeting = false;
	Walker.ActWith = nullptr;
	if (IsValid(Walker.Person))
	{
		Walker.Person->HideActProps();
	}
	if (FWalker* With = FindWalker(Other))
	{
		if (With->ActWith.Get() == Walker.Person)
		{
			EndAct(*With);
		}
	}
}

void APedestrians::TickActs(double Now, bool bPlayerAbout)
{
	using namespace PedestrianActs;
	if (NextIdleAct <= 0.0)
	{
		NextIdleAct = Now + Random.FRandRange(IdleEveryLow, IdleEveryHigh);
		NextMeeting = Now + Random.FRandRange(MeetEveryLow, MeetEveryHigh) * 0.5f;
		NextRareMeeting = Now + Random.FRandRange(RareEveryLow, RareEveryHigh);
	}
	if (!bPlayerAbout || Walkers.Num() == 0)
	{
		return;
	}
	const FVector2D Here(PlayerAt);
	auto InSight = [&Here](const FWalker& Walker)
	{
		const double Distance = FVector2D::Distance(Walker.Shown, Here);
		return Distance > ActNear && Distance < ActFar;
	};

	// Someone stops alone for a moment.
	if (Now >= NextIdleAct)
	{
		NextIdleAct = Now + Random.FRandRange(IdleEveryLow, IdleEveryHigh);
		const int32 Start = Random.RandRange(0, Walkers.Num() - 1);
		for (int32 n = 0; n < Walkers.Num(); ++n)
		{
			FWalker& Walker = Walkers[(Start + n) % Walkers.Num()];
			if (!IsFree(Walker) || !InSight(Walker) || Walker.bTipsy)
			{
				continue;
			}
			const PersonKit::FOutfit& Dress = Walker.Person->GetOutfit();
			const bool bHandsFree = Dress.Held == PersonKit::EHeld::None;
			FVector2D Tangent;
			PointOn(Lines[Walker.Line], Walker.Along, Tangent);
			const FVector2D Heading = Tangent * Walker.Dir;
			float WallYaw = 0.f;
			const bool bWall = WallBeside(Walker.Shown, Walker.GroundZ, Heading, WallYaw);
			if (Walker.bBeggar)
			{
				// Back to the wall, facing the passers-by.
				if (bWall)
				{
					StartAct(Walker, EAct::Beg, Random.FRandRange(45.f, 80.f), WallYaw + 180.f);
					break;
				}
				continue;
			}
			// What he might do, as he is.
			TArray<EAct, TInlineAllocator<6>> Could;
			Could.Add(EAct::Watch);
			if (bHandsFree && (!Dress.bWoman || Random.FRand() < 0.15f) && !PedestrianActs::IsPriestOrNun(Dress))
			{
				Could.Add(EAct::Smoke);
			}
			if (bHandsFree)
			{
				Could.Add(EAct::Paper);
			}
			if (bWall)
			{
				Could.Add(EAct::Window);
			}
			if (bHandsFree && !Walker.bStooped && !Dress.bLong && Dress.Height > 150.f)
			{
				Could.Add(EAct::Shoe);
			}
			FVector Flock;
			if (bHandsFree && APigeons::FlockNear(this, Walker.Shown, 1800.0, Flock))
			{
				// By a flock, crumbs most of all.
				Could.Add(EAct::Feed);
				Could.Add(EAct::Feed);
			}
			const EAct Act = Could[Random.RandRange(0, Could.Num() - 1)];
			float Yaw = Walker.ShownYaw;
			float Seconds = 6.f;
			switch (Act)
			{
			case EAct::Smoke:
				Seconds = Random.FRandRange(14.f, 22.f);
				break;
			case EAct::Paper:
				Seconds = Random.FRandRange(12.f, 18.f);
				break;
			case EAct::Window:
				Seconds = Random.FRandRange(8.f, 14.f);
				Yaw = WallYaw;
				break;
			case EAct::Shoe:
				Seconds = Random.FRandRange(4.f, 6.f);
				break;
			case EAct::Feed:
				Seconds = Random.FRandRange(15.f, 25.f);
				Yaw = YawTo(Walker.Shown, FVector2D(Flock));
				break;
			default:
				Seconds = Random.FRandRange(4.f, 7.f);
				break;
			}
			StartAct(Walker, Act, Seconds, Yaw);
			break;
		}
	}

	// Two going opposite ways along the same pavement know each other.
	if (Now >= NextMeeting)
	{
		NextMeeting = Now + Random.FRandRange(MeetEveryLow, MeetEveryHigh);
		const int32 Start = Random.RandRange(0, Walkers.Num() - 1);
		for (int32 n = 0; n < Walkers.Num(); ++n)
		{
			const int32 i = (Start + n) % Walkers.Num();
			FWalker& One = Walkers[i];
			if (!IsFree(One) || !InSight(One) || One.bBeggar || One.bTipsy)
			{
				continue;
			}
			int32 Found = INDEX_NONE;
			for (int32 j = 0; j < Walkers.Num(); ++j)
			{
				const FWalker& Other = Walkers[j];
				if (j == i || !IsFree(Other) || Other.bBeggar || Other.bTipsy || Other.Line != One.Line || Other.Dir == One.Dir)
				{
					continue;
				}
				// Ahead of him, coming his way.
				const double Ahead = (Other.Along - One.Along) * One.Dir;
				if (Ahead > MeetSeekMin && Ahead < MeetSeekMax)
				{
					Found = j;
					break;
				}
			}
			if (Found == INDEX_NONE)
			{
				continue;
			}
			FWalker& Other = Walkers[Found];
			const bool bTwoMen = !One.Person->GetOutfit().bWoman && !Other.Person->GetOutfit().bWoman
				&& !IsPriestOrNun(One.Person->GetOutfit()) && !IsPriestOrNun(Other.Person->GetOutfit());
			EAct Act = EAct::Chat;
			if (bTwoMen && Now >= NextRareMeeting)
			{
				NextRareMeeting = Now + Random.FRandRange(RareEveryLow, RareEveryHigh);
				Act = Random.FRand() < 0.5f ? EAct::Light : EAct::Argue;
			}
			for (FWalker* Each : { &One, &Other })
			{
				Each->Act = static_cast<uint8>(Act);
				Each->bMeeting = true;
				Each->ActLeft = MeetGiveUp;
				Each->ActTime = 0.f;
			}
			One.ActWith = Other.Person;
			Other.ActWith = One.Person;
			One.bActLead = true;
			Other.bActLead = false;
			break;
		}
	}
}

bool APedestrians::TickAct(FWalker& Walker, double Now, float DeltaSeconds)
{
	using namespace PedestrianActs;
	if (Walker.Act == 0)
	{
		return false;
	}
	if (Walker.React != EReact::None || !Walker.bPlaced || Walker.Gawk != INDEX_NONE || Walker.Shelter != INDEX_NONE || Walker.bIndoors)
	{
		EndAct(Walker);
		return false;
	}
	const EAct Act = static_cast<EAct>(Walker.Act);
	FWalker* With = nullptr;
	if (Act == EAct::Chat || Act == EAct::Light || Act == EAct::Argue)
	{
		With = FindWalker(Walker.ActWith.Get());
		if (!With || With->ActWith.Get() != Walker.Person || With->Act != Walker.Act)
		{
			EndAct(Walker);
			return false;
		}
	}
	Walker.ActLeft -= DeltaSeconds;
	if (Walker.ActLeft <= 0.f)
	{
		EndAct(Walker);
		return false;
	}

	// Still walking up to the one he knows: once near, both stop.
	if (Walker.bMeeting)
	{
		if (FVector2D::Distance(Walker.Shown, With->Shown) > MeetStop)
		{
			return false;
		}
		const float Seconds = Act == EAct::Chat ? Random.FRandRange(9.f, 16.f) : Act == EAct::Light ? 12.f : Random.FRandRange(9.f, 13.f);
		for (FWalker* Each : { &Walker, With })
		{
			Each->bMeeting = false;
			Each->ActTime = 0.f;
			Each->ActLeft = Seconds;
		}
		const bool bNear = FVector2D::Distance(Walker.Shown, FVector2D(PlayerAt)) < SpeakNear;
		APestiHUD* Hud = APestiHUD::Get(this);
		const bool bQuiet = !Hud || Hud->IsSaying();
		if (Act == EAct::Light)
		{
			// The one asking smokes; the other gives him a light and lights one himself.
			FWalker& Asker = Walker.bActLead ? *With : Walker;
			Asker.Person->ShowActProp(APedestrian::EActProp::Cigarette, true);
			if (bNear && !bQuiet)
			{
				Shout(AskLight.Hu, AskLight.En);
			}
		}
		else if (Act == EAct::Chat && bNear && !bQuiet)
		{
			const bool bLadies = Walker.Person->GetOutfit().bWoman || With->Person->GetOutfit().bWoman;
			const FSaying& Line = bLadies ? PickSaying(Random, LadyGreetings) : PickSaying(Random, Greetings);
			Shout(Line.Hu, Line.En);
		}
		return false;
	}

	Walker.ActTime += DeltaSeconds;
	APedestrian* Person = Walker.Person;
	const PersonKit::FOutfit& Dress = Person->GetOutfit();
	const double Size = Person->GetActorScale3D().Z;
	double Drop = 0.0;
	DailyLifeModel::FLimbs Limbs;
	const float T = Walker.ActTime;
	const float Own = T + Walker.Nosy * 7.f;

	// Facing: his own way, or the one he talks with, stepping to a fair distance first.
	if (With)
	{
		const double Gap = Act == EAct::Light ? LightGap : Act == EAct::Argue ? ArgueGap : TalkGap;
		const FVector2D Between = With->Shown - Walker.Shown;
		const double Apart = Between.Size();
		if (Apart > 1.0)
		{
			const FVector2D Toward = Between / Apart;
			const double Want = (Apart - Gap) * 0.5;
			const double Step = FMath::Min(FMath::Abs(Want), static_cast<double>(StepOver * DeltaSeconds));
			Walker.Shown += Toward * Step * FMath::Sign(Want);
			Walker.ActYaw = YawTo(Walker.Shown, With->Shown);
		}
	}
	Walker.ShownYaw = FMath::FixedTurn(Walker.ShownYaw, Walker.ActYaw, TurnRate * DeltaSeconds);

	switch (Act)
	{
	case EAct::Watch:
		PassersbyModel::LookAtWatch(Limbs);
		break;
	case EAct::Smoke:
	{
		// A draw now and then, the hand down between.
		const float Cycle = FMath::Fmod(Own, 5.f);
		PassersbyModel::Smoking(Limbs, Cycle < 1.6f ? FMath::Sin(Cycle / 1.6f * PI) : 0.f);
		break;
	}
	case EAct::Paper:
		PassersbyModel::Reading(Limbs);
		break;
	case EAct::Window:
		PassersbyModel::WindowShopping(Limbs);
		break;
	case EAct::Shoe:
		Drop = PassersbyModel::TyingShoe(Limbs, T) * Dress.Height / 178.0;
		break;
	case EAct::Feed:
		PassersbyModel::FeedingPigeons(Limbs, Own * 0.45f);
		break;
	case EAct::Chat:
		if (T < GreetTime)
		{
			// Hats raised (or hands shaken, between two men), a lady nods.
			if (Dress.bWoman)
			{
				PassersbyModel::Listening(Limbs, T);
			}
			else if (!With->Person->GetOutfit().bWoman && (Walker.Nosy + With->Nosy) < 1.f)
			{
				PassersbyModel::ShakingHands(Limbs, T);
			}
			else
			{
				const float Raise = FMath::Sin(FMath::Clamp(T / GreetTime, 0.f, 1.f) * PI);
				Limbs.Arms[1] = FRotator(125.f * Raise, 0.f, 20.f * Raise);
				Limbs.Elbows[1] = 125.f * Raise;
			}
			break;
		}
		{
			// In turns: one talks with his hands, the other listens.
			const bool bLeadSpeaks = FMath::FloorToInt((T - GreetTime) / SpeakTurn) % 2 == 0;
			if (bLeadSpeaks == Walker.bActLead)
			{
				PassersbyModel::Talking(Limbs, Own);
			}
			else
			{
				PassersbyModel::Listening(Limbs, Own);
			}
		}
		break;
	case EAct::Light:
	{
		const bool bGiver = Walker.bActLead;
		if (T < 3.f)
		{
			if (bGiver)
			{
				PassersbyModel::GivingALight(Limbs);
			}
			else
			{
				PassersbyModel::Smoking(Limbs, 0.9f);
			}
			if (bGiver && T - DeltaSeconds < 2.f && T >= 2.f && FVector2D::Distance(Walker.Shown, FVector2D(PlayerAt)) < SpeakNear)
			{
				Shout(GiveLight.Hu, GiveLight.En);
			}
		}
		else
		{
			if (bGiver && T - DeltaSeconds < 3.f)
			{
				Person->ShowActProp(APedestrian::EActProp::Cigarette, true);
			}
			const float Cycle = FMath::Fmod(Own, 4.5f);
			PassersbyModel::Smoking(Limbs, Cycle < 1.5f ? FMath::Sin(Cycle / 1.5f * PI) : 0.f);
		}
		break;
	}
	case EAct::Argue:
		PassersbyModel::Arguing(Limbs, Own);
		if (FVector2D::Distance(Walker.Shown, FVector2D(PlayerAt)) < SpeakNear * 1.5 && Random.FRand() < DeltaSeconds * 0.4f)
		{
			const FSaying& Line = PickSaying(Random, Quarrels);
			Shout(Line.Hu, Line.En);
		}
		break;
	case EAct::Beg:
		PassersbyModel::Begging(Limbs, Own);
		TickBeggar(Walker, Now);
		break;
	default:
		break;
	}
	Person->Strike(FVector(Walker.Shown, Walker.GroundZ + MiddleUp(Walker) - Drop * Size), Walker.ShownYaw, Limbs);
	return true;
}

void APedestrians::TickBeggar(FWalker& Walker, double Now)
{
	using namespace PedestrianActs;
	APestiPerson* Player = Cast<APestiPerson>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player || Player->IsHidden() || Player->IsRiding()
		|| FVector2D::Distance(Walker.Shown, FVector2D(Player->GetActorLocation())) > BegAsk)
	{
		return;
	}
	APestiHUD* Hud = APestiHUD::Get(this);
	if (!Hud)
	{
		return;
	}
	// He asks once as the player comes up; after a coin, not again for a while.
	if (Now < Walker.BegAgain)
	{
		return;
	}
	if (!Walker.bAsked && !Hud->IsSaying())
	{
		const FSaying& Line = PickSaying(Random, BegLines);
		Hud->Say(TEXT("Koldus"), Line.Hu, Line.En, 3.f);
		Walker.bAsked = true;
	}
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (!Game || Game->GetMoney() < BegCoin)
	{
		return;
	}
	Hud->Prompt(TEXT("E"), TEXT("Adni neki 20 fill\u00e9rt"), TEXT("Give him 20 fill\u00e9r"), false);
	const APlayerController* PC = Cast<APlayerController>(Player->GetController());
	if (PC && PC->WasInputKeyJustPressed(EKeys::E))
	{
		Game->AddMoney(-BegCoin);
		Hud->Say(TEXT("Koldus"), Thanks.Hu, Thanks.En, 3.f);
		Walker.BegAgain = Now + BegPause;
	}
}

// ---------------------------------------------------------------------------
// Walking together.

bool APedestrians::KeepBeside(FWalker& Walker)
{
	FWalker* Leader = FindWalker(Walker.Partner.Get());
	if (!Leader || Leader->Partner.Get() != Walker.Person || !Leader->bPlaced || !Walker.bPlaced
		|| Leader->React != EReact::None || Walker.React != EReact::None || Leader->Act != 0 || Walker.Act != 0
		|| Leader->Gawk != INDEX_NONE || Walker.Gawk != INDEX_NONE || Leader->Shelter != INDEX_NONE || Walker.Shelter != INDEX_NONE
		|| Leader->bIndoors || Walker.bIndoors)
	{
		SplitPair(Walker);
		return false;
	}
	Walker.Line = Leader->Line;
	Walker.Along = Leader->Along;
	Walker.Dir = Leader->Dir;
	Walker.Speed = Leader->Speed;
	Walker.Lane = Leader->Lane + PedestrianActs::PairGap;
	Walker.bWaiting = Leader->bWaiting;
	Walker.bRushing = Leader->bRushing;
	return true;
}

void APedestrians::SplitPair(FWalker& Walker)
{
	FWalker* Other = FindWalker(Walker.Partner.Get());
	Walker.Partner = nullptr;
	Walker.Pair = 0;
	Walker.bFollower = false;
	if (Other && Other->Partner.Get() == Walker.Person)
	{
		Other->Partner = nullptr;
		Other->Pair = 0;
		Other->bFollower = false;
	}
}

void APedestrians::PoseWalking(FWalker& Walker)
{
	APedestrian* Person = Walker.Person;
	if (Walker.bStooped)
	{
		Person->SetActorRotation(FRotator(-PedestrianActs::StoopPitch, Walker.ShownYaw, 0.f));
	}
	if (!Walker.Partner.IsValid() || Person->HasUmbrellaUp())
	{
		return;
	}
	// The follower walks on the leader's right: the leader's right arm, the follower's left.
	float Elbow = 0.f;
	switch (static_cast<EPair>(Walker.Pair))
	{
	case EPair::Couple:
	{
		const FRotator Turn = Walker.bFollower ? PassersbyModel::HookedArm(Elbow) : PassersbyModel::CrookedArm(Elbow);
		Person->LinkArm(Walker.bFollower ? 0 : 1, Turn, Elbow);
		break;
	}
	case EPair::MotherChild:
	{
		const FRotator Turn = Walker.bFollower ? PassersbyModel::ChildHand(Elbow) : PassersbyModel::MotherHand(Elbow);
		Person->LinkArm(Walker.bFollower ? 0 : 1, Turn, Elbow);
		break;
	}
	default:
		break;
	}
}

void APedestrians::AddCompanion(int32 Leader)
{
	if (!Walkers.IsValidIndex(Leader) || !IsValid(Walkers[Leader].Person))
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APedestrian* Person = GetWorld()->SpawnActor<APedestrian>(APedestrian::StaticClass(), Walkers[Leader].Person->GetActorLocation(),
		FRotator::ZeroRotator, Params);
	if (!Person)
	{
		return;
	}
	FWalker Follower;
	Follower.Person = Person;
	Follower.Nosy = Random.FRand();
	Follower.UmbrellaPatience = Random.FRand();
	Follower.ShelterClock = Random.FRand() * 2.f;
	const int32 Added = Walkers.Add(Follower);
	PairUp(Leader, Added);
}

void APedestrians::RecruitCompanion(int32 Leader)
{
	if (!Walkers.IsValidIndex(Leader))
	{
		return;
	}
	// Someone walking alone, out of sight and out of the way.
	const FVector2D Here(PlayerAt);
	for (int32 i = 0; i < Walkers.Num(); ++i)
	{
		const FWalker& Other = Walkers[i];
		if (i == Leader || !IsFree(Other) || Other.bBeggar || Other.bTipsy
			|| Other.Person->WasRecentlyRendered(1.f) || FVector2D::Distance(Other.Shown, Here) < AppearBeyond)
		{
			continue;
		}
		PairUp(Leader, i);
		return;
	}
}

void APedestrians::PairUp(int32 Leader, int32 Follower)
{
	using namespace PedestrianActs;
	FWalker& Lead = Walkers[Leader];
	FWalker& Side = Walkers[Follower];
	const PersonKit::FOutfit& LeadDress = Lead.Person->GetOutfit();
	if (Lead.bBeggar || LeadDress.Hat == PersonKit::EHat::ClericHat || LeadDress.bSeated)
	{
		return;
	}
	// Who walks with him: his wife, her child, or a friend of the same kind.
	EPair Kind = EPair::Friends;
	PersonKit::FOutfit Dress;
	if (LeadDress.Hat == PersonKit::EHat::Veil)
	{
		Dress = PassersbyModel::Nun(Random);
	}
	else if (LeadDress.bWoman && Random.FRand() < 0.45f)
	{
		Kind = EPair::MotherChild;
		Dress = DailyLifeModel::KidLook(Random, Random.FRand() < 0.5f);
	}
	else
	{
		const bool bCouple = !LeadDress.bWoman && Random.FRand() < 0.55f;
		Kind = bCouple ? EPair::Couple : EPair::Friends;
		const bool bWantWoman = bCouple || LeadDress.bWoman;
		Dress = PersonModel::RandomOutfit(Random, true);
		for (int32 Try = 0; Try < 10 && (Dress.bWoman != bWantWoman || IsPriestOrNun(Dress)); ++Try)
		{
			Dress = PersonModel::RandomOutfit(Random, true);
		}
		if (Dress.bWoman != bWantWoman || IsPriestOrNun(Dress))
		{
			return;
		}
		if (LeadDress.bAged && bCouple)
		{
			// An old man's wife is old too.
			Dress = PassersbyModel::OldWoman(Random);
		}
	}
	Side.Person->SetOutfit(Dress);
	Side.Partner = Lead.Person;
	Side.bFollower = true;
	Side.Pair = static_cast<uint8>(Kind);
	Lead.Partner = Side.Person;
	Lead.bFollower = false;
	Lead.Pair = static_cast<uint8>(Kind);
	// Strolling together, the leader nearer the line, the other beside him.
	Lead.Speed = FMath::Min(Lead.Speed, StrollPace);
	Lead.Lane = PairLane;
	Side.Line = Lead.Line;
	Side.Along = Lead.Along;
	Side.Dir = Lead.Dir;
	Side.Speed = Lead.Speed;
	Side.Lane = PairLane + PairGap;
	Side.GroundZ = Lead.GroundZ;
	Side.bGroundKnown = Lead.bGroundKnown;
	Side.bPlaced = false;
	Side.bStooped = Dress.Held == PersonKit::EHeld::Cane;
	Side.bBeggar = false;
	Side.bTipsy = false;
	Side.bHasUmbrella = Lead.bHasUmbrella;
	Side.UmbrellaColor = UmbrellaModel::ClothColor(Random.FRand(), Dress.bWoman);
	Side.RainShy = Lead.RainShy;
}

void APedestrians::SetUpWalker(FWalker& Walker)
{
	using namespace PedestrianActs;
	const PersonKit::FOutfit& Dress = Walker.Person->GetOutfit();
	Walker.bBeggar = false;
	Walker.bStooped = Dress.Held == PersonKit::EHeld::Cane;
	if (Walker.bStooped || (Dress.bAged && Dress.Hat == PersonKit::EHat::Kerchief))
	{
		Walker.Speed *= OldPace;
	}
	if (!Dress.bWoman && Random.FRand() < BeggarShare)
	{
		Walker.Person->SetOutfit(PassersbyModel::Beggar(Random));
		Walker.bBeggar = true;
		Walker.bStooped = false;
		Walker.Speed = FMath::Min(Walker.Speed, 110.f);
	}
}
