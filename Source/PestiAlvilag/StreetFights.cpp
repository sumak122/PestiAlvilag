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
	const FTransform Spot(FRotator(0.0, FaceYaw, 0.0), Ground + FVector(0.0, 0.0, StreetFight::MiddleUp));
	ARuffian* Man = GetWorld()->SpawnActorDeferred<ARuffian>(ARuffian::StaticClass(), Spot, this, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Man)
	{
		return false;
	}
	Man->Kind = ERuffian::Passerby;
	Man->StreetDress = Dress;
	Man->FinishSpawning(Spot);

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
	return true;
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

void AStreetFights::Say(const TCHAR* Hu, const TCHAR* En)
{
	const double Now = GetWorld()->GetTimeSeconds();
	APestiHUD* Hud = APestiHUD::Get(this);
	if (!Hud || Hud->IsSaying() || Now - LastShout < StreetFight::ShoutEvery)
	{
		return;
	}
	LastShout = Now;
	Hud->Say(TEXT("J\u00e1r\u00f3kel\u0151"), Hu, En, 3.f);
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
			Fights.RemoveAt(i);
			continue;
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
				if (Fight.Loot > 0)
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
		if (!Man->IsAttacking())
		{
			const StreetFight::FLine2& Line = StreetFight::FightLines[Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(StreetFight::FightLines)) - 1)];
			Say(Line.Hu, Line.En);
			Man->Attack(Player, 0.85f);
		}
		// Knocked out of the fight (a scene, a menu): back in, facing him.
		if (!Player->IsFighting() && Player->GetAct() != Brawl::EAct::Beaten && Apart < StreetFight::LetGoDistance)
		{
			Player->StartFight(Man);
		}
	}
}
