// Fill out your copyright notice in the Description page of Project Settings.

#include "StreetFights.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "Pedestrians.h"
#include "PestiPerson.h"
#include "PolicePatrols.h"
#include "Ruffian.h"
#include "StreetName.h"
#include "Witnesses.h"

namespace StreetFight
{
	// The man stands on the ground; his middle is this high (cm), as the player's.
	constexpr double MiddleUp = 94.0;
	// Of the men, this share put their fists up; the rest (and every woman) run.
	constexpr float FightBackChance = 0.6f;
	// A runner far enough from the player to let him put his fists down (cm).
	constexpr double LetGoDistance = 400.0;
	// After his man is beaten, the player's fists come down after this long (s).
	constexpr double FistsDownAfter = 1.2;
	// The beaten are cleared once nobody has seen them for a while, after this long (s), or this far off (cm).
	constexpr double LieAtLeast = 8.0;
	constexpr double ClearBeyond = 6000.0;
	// A fighter gives up when the player is this far (cm).
	constexpr double GiveUpDistance = 2500.0;
	// A constable or police car this close sees the first punch (cm).
	constexpr double PoliceSee = 5000.0;
	constexpr double ShoutEvery = 4.0;
	// Passers-by gather round a fight: called this often (s), each call holding them this long (s).
	constexpr double GatherEvery = 1.0;
	constexpr float GatherHold = 4.f;
	// Round a man beaten down they stay a little longer (s).
	constexpr float BeatenHold = 10.f;

	struct FLine2 { const TCHAR* Hu; const TCHAR* En; };
	const FLine2 FightLines[] = {
		{ TEXT("Mi van, megbolondult? Na megkapja!"), TEXT("What, have you gone mad? You'll get it now!") },
		{ TEXT("Engem ne l\u00f6kd\u00f6ss\u00f6n! Gyere csak!"), TEXT("Don't you shove me! Come on then!") },
		{ TEXT("Ezt megb\u00e1nod, te csirkefog\u00f3!"), TEXT("You'll be sorry for that, you scoundrel!") },
	};
	const FLine2 RunLines[] = {
		{ TEXT("Seg\u00edts\u00e9g! Rend\u0151r! Megt\u00e1madtak!"), TEXT("Help! Police! I've been attacked!") },
		{ TEXT("Jaj, ne b\u00e1nts! Seg\u00edts\u00e9g!"), TEXT("Oh, don't hurt me! Help!") },
		{ TEXT("Ver\u00e9s! H\u00edvjanak rend\u0151rt!"), TEXT("A beating! Call the police!") },
	};
	const FLine2 BeatenLine = { TEXT("El\u00e9g... el\u00e9g, nem b\u00e1ntom..."), TEXT("Enough... enough, I won't touch you...") };
	const FLine2 WonLine = { TEXT("Na! Legk\u00f6zelebb gondold meg, kibe k\u00f6tsz bele!"), TEXT("There! Next time think who you pick on!") };

	// Constables: the player can't walk this far (cm) from one he fights before he gives up the fist fight and runs after him again.
	constexpr double ConstableLetGo = 700.0;
	// A beaten constable lies this long (s) before he is himself again, on the ground a while longer.
	constexpr double ConstableLies = 3.0;
	// Waiting his turn, a man keeps this far from the player (cm).
	constexpr double WaitRing = 330.0;
	// Punching reach, as at the passers-by: this far (cm), this much in front (dot), this much higher or lower (cm).
	constexpr double PunchReach = 230.0;
	constexpr double PunchFacing = 0.3;
	constexpr double PunchHeight = 200.0;
	const FLine2 StruckLines[] = {
		{ TEXT("Rend\u0151rre emel kezet?! Ezt megkeser\u00fcli!"), TEXT("Raising your hand to a policeman?! You'll pay for that!") },
		{ TEXT("Hat\u00f3s\u00e1gi k\u00f6zeg ellen?! Most azt\u00e1n v\u00e9ge!"), TEXT("Against an officer of the law?! Now you're done for!") },
	};
	const FLine2 JoinLine = { TEXT("El\u00e9g legyen! Most velem gy\u0171lik meg a baja!"), TEXT("That's enough! Now you'll deal with me!") };
	const FLine2 ConstableDownLine = { TEXT("Le\u00fct\u00f6tte a rend\u0151rt! J\u00e9zus M\u00e1ria!"), TEXT("He knocked the policeman down! Good heavens!") };
}

AStreetFights::AStreetFights()
{
	PrimaryActorTick.bCanEverTick = true;
	Random.Initialize(1932);
}

AStreetFights* AStreetFights::Get(const UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AStreetFights> It(World); It; ++It)
	{
		return *It;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<AStreetFights>(AStreetFights::StaticClass(), FTransform::Identity, Params);
}

bool AStreetFights::WantsToPunch(const APestiPerson* Player)
{
	if (!Player || Player->IsHidden() || Player->GetAct() != Brawl::EAct::None || Player->IsRiding()
		|| Player->IsCarrying() || Player->IsHoldingInFront() || Player->IsFreeRunning())
	{
		return false;
	}
	const APlayerController* PC = Cast<APlayerController>(Player->GetController());
	if (!PC || PC->IsMoveInputIgnored())
	{
		return false;
	}
	return PC->WasInputKeyJustPressed(EKeys::LeftMouseButton) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Left);
}

bool AStreetFights::Begin(APestiPerson* Player, const PersonKit::FOutfit& Dress, const FVector& Ground, double FaceYaw)
{
	if (!Player || Player->GetAct() != Brawl::EAct::None)
	{
		return false;
	}
	ARuffian* Man = SpawnFighter(false, Dress, Ground, FaceYaw);
	if (!Man)
	{
		return false;
	}

	if (Fights.Num() == 0)
	{
		if (const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
		{
			ArrestsSeen = Game->GetArrests();
		}
	}
	FFight Fight;
	Fight.Man = Man;
	Fight.Since = GetWorld()->GetTimeSeconds();
	Fight.bFightsBack = !Dress.bWoman && Random.FRand() < StreetFight::FightBackChance;
	Fights.Add(Fight);

	// Both square up, and the player's first punch is on its way.
	Man->StartFight(Player);
	Player->StartFight(Man);
	Player->ChainPunch(false);

	// Seen by the police: he is wanted.
	if (APolicePatrols::Witnessed(this, Ground, StreetFight::PoliceSee))
	{
		if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
		{
			Game->ReportCrime(1);
		}
	}
	else
	{
		// Nobody from the police saw it: now and then a passer-by runs to tell them.
		AWitnesses::Report(this, Ground, 1);
	}
	return true;
}

ARuffian* AStreetFights::SpawnFighter(bool bPolice, const PersonKit::FOutfit& Dress, const FVector& Ground, double FaceYaw)
{
	const FTransform Spot(FRotator(0.0, FaceYaw, 0.0), Ground + FVector(0.0, 0.0, StreetFight::MiddleUp));
	ARuffian* Man = GetWorld()->SpawnActorDeferred<ARuffian>(ARuffian::StaticClass(), Spot, this, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Man)
	{
		return nullptr;
	}
	Man->Kind = bPolice ? ERuffian::Constable : ERuffian::Passerby;
	Man->StreetDress = Dress;
	Man->FinishSpawning(Spot);
	return Man;
}

bool AStreetFights::BeginConstable(APestiPerson* Player, APoliceman* Constable, const FVector& Ground, double FaceYaw, bool bPlayerStarts)
{
	if (!Player || !Constable || Constable->IsHidden())
	{
		return false;
	}
	if (bPlayerStarts ? Player->GetAct() != Brawl::EAct::None : !CanJoinIn(Player))
	{
		return false;
	}
	for (const FFight& Other : Fights)
	{
		if (Other.Constable.Get() == Constable)
		{
			return false;
		}
	}
	// The fighter wears his uniform, truncheon and all.
	PersonKit::FOutfit Dress = Constable->GetOutfit();
	Dress.bUniform = true;
	Dress.bSeated = false;
	ARuffian* Man = SpawnFighter(true, Dress, Ground, FaceYaw);
	if (!Man)
	{
		return false;
	}

	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (Fights.Num() == 0 && Game)
	{
		ArrestsSeen = Game->GetArrests();
	}
	FFight Fight;
	Fight.Man = Man;
	Fight.Since = GetWorld()->GetTimeSeconds();
	Fight.bFightsBack = true;
	Fight.bConstable = true;
	Fight.Constable = Constable;
	Fight.LastAt = FVector2D(Ground);
	// He himself is out of sight (and out of the way) while his fighter stands in.
	Fight.bConstableHidCollision = Constable->GetActorEnableCollision();
	Constable->SetActorHiddenInGame(true);
	if (Fight.bConstableHidCollision)
	{
		Constable->SetActorEnableCollision(false);
	}
	Fights.Add(Fight);

	Man->StartFight(Player);
	if (bPlayerStarts)
	{
		// The player's first punch is on its way: hitting a policeman is two stars at once.
		Player->StartFight(Man);
		Player->ChainPunch(false);
		if (Game)
		{
			Game->ReportCrime(2);
		}
		const StreetFight::FLine2& Line = StreetFight::StruckLines[Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(StreetFight::StruckLines)) - 1)];
		Say(Line.Hu, Line.En, TEXT("Rend\u0151r"));
	}
	else
	{
		Say(StreetFight::JoinLine.Hu, StreetFight::JoinLine.En, TEXT("Rend\u0151r"));
	}
	return true;
}

EConstableFight AStreetFights::ConstableState(const UObject* WorldContextObject, const APoliceman* Constable, FVector2D& OutAt)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World || !Constable)
	{
		return EConstableFight::None;
	}
	for (TActorIterator<AStreetFights> It(World); It; ++It)
	{
		for (const FFight& Fight : It->Fights)
		{
			if (Fight.bConstable && Fight.Constable.Get() == Constable)
			{
				OutAt = Fight.LastAt;
				return EConstableFight::Fighting;
			}
		}
		for (int32 i = 0; i < It->Ended.Num(); ++i)
		{
			if (It->Ended[i].Constable.Get() == Constable)
			{
				const EConstableFight State = It->Ended[i].State;
				OutAt = It->Ended[i].At;
				It->Ended.RemoveAt(i);
				return State;
			}
		}
	}
	return EConstableFight::None;
}

bool AStreetFights::CanJoinIn(const APestiPerson* Player)
{
	return Player && !Player->IsHidden() && !Player->IsRiding() && Player->IsFighting();
}

bool AStreetFights::InPunchReach(const APestiPerson* Player, const FVector& At)
{
	if (!Player)
	{
		return false;
	}
	const FVector From = Player->GetActorLocation();
	const FVector2D Offset(At.X - From.X, At.Y - From.Y);
	if (Offset.SizeSquared() > FMath::Square(StreetFight::PunchReach) || FMath::Abs(At.Z - From.Z) > StreetFight::PunchHeight)
	{
		return false;
	}
	const FVector2D Forward = FVector2D(Player->GetActorForwardVector()).GetSafeNormal();
	return FVector2D::DotProduct(Offset.GetSafeNormal(), Forward) > StreetFight::PunchFacing;
}

bool AStreetFights::IsPoliceFighting(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return false;
	}
	for (TActorIterator<AStreetFights> It(World); It; ++It)
	{
		for (const FFight& Fight : It->Fights)
		{
			const ARuffian* Man = Fight.Man.Get();
			if (Fight.bConstable && Man && !Man->IsBeaten())
			{
				return true;
			}
		}
	}
	return false;
}

void AStreetFights::HandBack(FFight& Fight, APestiPerson* Player, bool bDown)
{
	if (ARuffian* Man = Fight.Man.Get())
	{
		Fight.LastAt = FVector2D(Man->GetActorLocation());
		if (Player && Player->GetFoe() == Man && Player->GetAct() != Brawl::EAct::None)
		{
			Player->EndFight();
		}
		Man->Destroy();
	}
	if (APoliceman* Cop = Fight.Constable.Get())
	{
		Cop->SetActorHiddenInGame(false);
		if (Fight.bConstableHidCollision)
		{
			Cop->SetActorEnableCollision(true);
		}
		FConstableEnd Over;
		Over.Constable = Cop;
		Over.State = bDown ? EConstableFight::Down : EConstableFight::Back;
		Over.At = Fight.LastAt;
		Ended.Add(Over);
	}
}

void AStreetFights::TakeTurns(FFight& Fight, ARuffian* Man, APestiPerson* Player)
{
	// The player busy with someone else: wait round him, fists up.
	const APestiPerson* Facing = Player->GetFoe();
	if (Facing && Facing != Man && Facing->IsFighting())
	{
		FVector Away = (Man->GetActorLocation() - Player->GetActorLocation()).GetSafeNormal2D();
		if (Away.IsNearlyZero())
		{
			Away = -Player->GetActorForwardVector().GetSafeNormal2D();
		}
		Man->Circle(Player, Player->GetActorLocation() + Away * StreetFight::WaitRing);
		return;
	}
	if (!Man->IsAttacking())
	{
		if (!Fight.bConstable)
		{
			const StreetFight::FLine2& Line = StreetFight::FightLines[Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(StreetFight::FightLines)) - 1)];
			Say(Line.Hu, Line.En);
		}
		Man->Attack(Player, Fight.bConstable ? 0.9f : 0.85f);
	}
}

void AStreetFights::SetLoot(int32 Filler)
{
	if (Fights.Num() > 0 && Filler > 0)
	{
		Fights.Last().Loot = Filler;
		Fights.Last().bFightsBack = true;
	}
}

bool AStreetFights::IsFightNear(const UObject* WorldContextObject, const FVector& At, double Radius)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return false;
	}
	for (TActorIterator<AStreetFights> It(World); It; ++It)
	{
		for (const FFight& Fight : It->Fights)
		{
			const ARuffian* Man = Fight.Man.Get();
			if (Man && !Fight.bRanOff && !Man->IsBeaten() && FVector::Dist2D(Man->GetActorLocation(), At) < Radius)
			{
				return true;
			}
		}
	}
	return false;
}

void AStreetFights::Say(const TCHAR* Hu, const TCHAR* En, const TCHAR* Speaker)
{
	const double Now = GetWorld()->GetTimeSeconds();
	APestiHUD* Hud = APestiHUD::Get(this);
	if (!Hud || Hud->IsSaying() || Now - LastShout < StreetFight::ShoutEvery)
	{
		return;
	}
	LastShout = Now;
	Hud->Say(Speaker ? Speaker : TEXT("J\u00e1r\u00f3kel\u0151"), Hu, En, 3.f);
}

void AStreetFights::SendOff(FFight& Fight, APestiPerson* Player)
{
	ARuffian* Man = Fight.Man.Get();
	if (Player && Man && Player->GetFoe() == Man && Player->GetAct() != Brawl::EAct::None)
	{
		Player->EndFight();
	}
	if (Man && !Man->IsBeaten())
	{
		Man->Flee(Player);
	}
	Fight.bRanOff = true;
}

void AStreetFights::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Ended constables whose owner never asked (gone with their car).
	for (int32 i = Ended.Num() - 1; i >= 0; --i)
	{
		if (!Ended[i].Constable.IsValid())
		{
			Ended.RemoveAt(i);
		}
	}
	if (Fights.Num() == 0)
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	APestiPerson* Player = Cast<APestiPerson>(UGameplayStatics::GetPlayerPawn(this, 0));
	// He was arrested: every fight is off.
	const int32 Arrests = Game ? Game->GetArrests() : 0;
	const bool bArrested = Arrests != ArrestsSeen;
	ArrestsSeen = Arrests;
	const bool bFainted = Game && Game->IsFainted();

	for (int32 i = Fights.Num() - 1; i >= 0; --i)
	{
		FFight& Fight = Fights[i];
		ARuffian* Man = Fight.Man.Get();
		if (!Man)
		{
			if (Fight.bConstable)
			{
				HandBack(Fight, Player, false);
			}
			Fights.RemoveAt(i);
			continue;
		}
		// A constable's fight: gone with his car meanwhile, his fighter goes too.
		if (Fight.bConstable)
		{
			if (!Fight.Constable.IsValid())
			{
				if (Player && Player->GetFoe() == Man && Player->GetAct() != Brawl::EAct::None)
				{
					Player->EndFight();
				}
				Man->Destroy();
				Fights.RemoveAt(i);
				continue;
			}
			Fight.LastAt = FVector2D(Man->GetActorLocation());
		}
		const double Apart = Player ? FVector::Dist2D(Player->GetActorLocation(), Man->GetActorLocation()) : 1e9;

		// Gone round the corner: away with him.
		if (Man->IsGone())
		{
			Man->Destroy();
			Fights.RemoveAt(i);
			continue;
		}

		// Beaten: it costs the player a little street name; he lies there till nobody sees.
		if (Man->IsBeaten())
		{
			if (Fight.BeatenAt < 0.0)
			{
				Fight.BeatenAt = Now;
				// The street stops to look at the man on the ground.
				APedestrians::Gather(this, Man->GetActorLocation(), StreetFight::BeatenHold, false);
				if (Fight.bConstable)
				{
					// A policeman knocked down: three stars, and the street talks about him.
					Say(StreetFight::ConstableDownLine.Hu, StreetFight::ConstableDownLine.En);
					if (Game)
					{
						Game->ReportCrime(3);
						Game->AddRenown(StreetName::ForBeatingPolice, TEXT("le\u00fct\u00f6tt egy rend\u0151rt"), TEXT("knocked down a policeman"));
					}
				}
				else if (Fight.Loot > 0)
				{
					// The pickpocket: the money comes back, and nobody minds him beaten.
					if (Game)
					{
						Game->AddMoney(Fight.Loot);
					}
					if (APestiHUD* Hud = APestiHUD::Get(this))
					{
						Hud->Say(TEXT("Zsebtolvaj"), TEXT("Itt van, itt van, vegye vissza!"), TEXT("Here, here, take it back!"), 3.f);
					}
					Fight.Loot = 0;
				}
				else
				{
					Say(StreetFight::BeatenLine.Hu, StreetFight::BeatenLine.En);
					if (Game)
					{
						Game->AddRenown(StreetName::ForBeatingUp, TEXT("megvert egy j\u00e1r\u00f3kel\u0151t"), TEXT("beat up a passer-by"));
					}
				}
			}
			if (Player && Player->GetFoe() == Man && Player->IsFighting() && Now - Fight.BeatenAt > StreetFight::FistsDownAfter)
			{
				Player->EndFight();
			}
			// A constable is soon himself again, lying there a while before he gets up.
			if (Fight.bConstable)
			{
				if (Now - Fight.BeatenAt > StreetFight::ConstableLies)
				{
					HandBack(Fight, Player, true);
					Fights.RemoveAt(i);
				}
				continue;
			}
			const bool bSeen = Man->WasRecentlyRendered(1.f);
			if (Apart > StreetFight::ClearBeyond || (!bSeen && Now - Fight.BeatenAt > StreetFight::LieAtLeast))
			{
				Man->Destroy();
				Fights.RemoveAt(i);
			}
			continue;
		}

		if (Fight.bRanOff)
		{
			// Running off: the player's fists come down once he is away.
			if (Player && Player->GetFoe() == Man && Player->GetAct() != Brawl::EAct::None && Apart > StreetFight::LetGoDistance)
			{
				Player->EndFight();
			}
			continue;
		}

		// A constable: the player out of it (taken, fainted, in a car, run off) and he is after him again on foot.
		if (Fight.bConstable)
		{
			if (!Player || bFainted || bArrested || Player->IsRiding() || Apart > StreetFight::ConstableLetGo)
			{
				HandBack(Fight, Player, false);
				Fights.RemoveAt(i);
				continue;
			}
			// He has the player in sight all the while.
			if (Game)
			{
				Game->NotifySeenByPolice();
			}
		}

		// The player out of it (fainted, arrested, in a car, gone off): the other man walks away.
		if (!Player || bFainted || bArrested || Player->IsRiding() || Apart > StreetFight::GiveUpDistance)
		{
			if (Fight.bFightsBack && bFainted)
			{
				Say(StreetFight::WonLine.Hu, StreetFight::WonLine.En);
			}
			SendOff(Fight, Player);
			continue;
		}

		// Passers-by stop and make a ring round the two of them.
		if (Now >= Fight.NextGather)
		{
			Fight.NextGather = Now + StreetFight::GatherEvery;
			APedestrians::Gather(this, (Player->GetActorLocation() + Man->GetActorLocation()) * 0.5, StreetFight::GatherHold, true);
		}

		// After the first punch has had its moment: fists up and in, or off he runs.
		if (Now - Fight.Since < 0.35)
		{
			continue;
		}
		if (!Fight.bFightsBack)
		{
			const StreetFight::FLine2& Line = StreetFight::RunLines[Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(StreetFight::RunLines)) - 1)];
			Say(Line.Hu, Line.En);
			SendOff(Fight, Player);
			continue;
		}
		// In at him, or waiting his turn while the player is busy with another.
		TakeTurns(Fight, Man, Player);
		// Knocked out of the fight (a scene, a menu): back in, facing him.
		if (!Player->IsFighting() && Player->GetAct() != Brawl::EAct::Beaten && Apart < StreetFight::LetGoDistance)
		{
			Player->StartFight(Man);
		}
	}
}
