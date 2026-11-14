// Fill out your copyright notice in the Description page of Project Settings.

#include "CarjackedDriver.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiPerson.h"
#include "PolicePatrols.h"
#include "StreetFights.h"
#include "PestiHud.h"

namespace CarjackedDriver
{
	constexpr float RunSpeed = 480.f;
	constexpr float WalkSpeed = 150.f;
	constexpr float RunFor = 5.f;
	constexpr float GoneAfter = 30.f;
	constexpr float GroundEvery = 0.2f;
	constexpr double Middle = 90.0;
	// He looks this far ahead for a wall, and turns aside from one.
	constexpr double WallAhead = 120.0;
	constexpr float ShoutSeconds = 3.f;
	constexpr int32 ShoutKey = 7;
	// A robbed rider or carter: the share who run after the player, and who run to a constable
	// (one within FetchRange; not while he is already wanted); the rest run off.
	constexpr float ChaseShare = 0.35f;
	constexpr float FetchShare = 0.35f;
	constexpr double FetchRange = 5000.0;
	// The chase: given up after ChaseFor seconds or once this far behind; caught within CatchReach
	// (a fight if the player is on foot); a shout every ShoutEvery seconds.
	constexpr float ChaseSpeed = 520.f;
	constexpr float ChaseFor = 14.f;
	constexpr double LostBeyond = 2500.0;
	constexpr double CatchReach = 140.0;
	constexpr float ShoutEvery = 3.5f;
	// The run to a constable: given up after FetchFor seconds; where the constable is is looked up this often.
	constexpr float FetchFor = 30.f;
	constexpr float LookEvery = 1.f;
	constexpr double TellReach = 300.0;
	// Shaking his fist after the thief before he goes.
	constexpr float FistFor = 3.f;
}

ACarjackedDriver::ACarjackedDriver()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ACarjackedDriver::RunFrom(const FVector& Danger, bool bBicycle)
{
	const FVector2D Away = FVector2D(GetActorLocation() - Danger);
	Heading = Away.IsNearlyZero() ? FVector2D(GetActorForwardVector()) : Away.GetSafeNormal();
	if (GEngine)
	{
		APestiHUD::Show(this, CarjackedDriver::ShoutKey, CarjackedDriver::ShoutSeconds, FColor::Orange, bBicycle
			? TEXT("\u201eH\u00e9! A biciklim! Tolvaj!\u201d   (Hey! My bicycle! Thief!)")
			: TEXT("\u201eH\u00e9! Az aut\u00f3m! Rend\u0151r!\u201d   (Hey! My car! Police!)"));
	}
}

void ACarjackedDriver::AfterThief(const FVector& Danger, bool bBicycle, const PersonKit::FOutfit& Dress)
{
	bBike = bBicycle;
	Clothes = Dress;
	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	const bool bWanted = Game && Game->GetWantedLevel() > 0;
	const float Roll = FMath::FRand();
	if (Roll < CarjackedDriver::ChaseShare)
	{
		Mode = EMode::Chase;
		HeadFor(Danger);
		APestiHUD::Show(this, CarjackedDriver::ShoutKey, CarjackedDriver::ShoutSeconds, FColor::Orange,
			TEXT("\u201e\u00c1llj meg, te gazember! Add vissza!\u201d   (Stop, you scoundrel! Give it back!)"));
		return;
	}
	if (!bWanted && Roll < CarjackedDriver::ChaseShare + CarjackedDriver::FetchShare
		&& APolicePatrols::FindConstable(this, GetActorLocation(), CarjackedDriver::FetchRange, Constable))
	{
		Mode = EMode::Fetch;
		HeadFor(Constable);
		APestiHUD::Show(this, CarjackedDriver::ShoutKey, CarjackedDriver::ShoutSeconds, FColor::Orange,
			TEXT("\u201eMegl\u00e1tod, hozom a rend\u0151rt!\u201d   (You'll see, I'm fetching the police!)"));
		return;
	}
	if (bBicycle)
	{
		RunFrom(Danger, true);
		return;
	}
	// The carter has had his say already (ATrafficCart::TakeDriver): he only runs off.
	const FVector2D Away = FVector2D(GetActorLocation() - Danger);
	Heading = Away.IsNearlyZero() ? FVector2D(GetActorForwardVector()) : Away.GetSafeNormal();
}

void ACarjackedDriver::HeadFor(const FVector& To)
{
	const FVector2D Way = FVector2D(To - GetActorLocation());
	if (!Way.IsNearlyZero())
	{
		Heading = Way.GetSafeNormal();
	}
}

bool ACarjackedDriver::TickOwner(float DeltaSeconds)
{
	ModeClock += DeltaSeconds;
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector Here = GetActorLocation();
	const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(Heading.Y, Heading.X));
	switch (Mode)
	{
	case EMode::Chase:
	{
		const double Gap = Player ? FVector::Dist2D(Player->GetActorLocation(), Here) : TNumericLimits<double>::Max();
		if (!Player || ModeClock > CarjackedDriver::ChaseFor || Gap > CarjackedDriver::LostBeyond)
		{
			Mode = EMode::Fist;
			ModeClock = 0.f;
			return true;
		}
		HeadFor(Player->GetActorLocation());
		// Caught him on foot: a fist fight (the fight's man stands in for him).
		APestiPerson* OnFoot = Cast<APestiPerson>(Player);
		if (Gap < CarjackedDriver::CatchReach && OnFoot && !OnFoot->IsRiding() && !OnFoot->IsFighting() && !OnFoot->IsHidden())
		{
			AStreetFights* Fights = AStreetFights::Get(this);
			const FVector Feet = Here - FVector(0.0, 0.0, CarjackedDriver::Middle * GetActorScale3D().Z);
			if (Fights && Fights->Begin(OnFoot, Clothes, Feet, Yaw))
			{
				Fights->SetFightsBack();
				Destroy();
				return true;
			}
		}
		ShoutClock -= DeltaSeconds;
		if (ShoutClock <= 0.f && Gap < CarjackedDriver::LostBeyond * 0.6)
		{
			ShoutClock = CarjackedDriver::ShoutEvery;
			APestiHUD::Show(this, CarjackedDriver::ShoutKey, CarjackedDriver::ShoutSeconds, FColor::Orange, bBike
				? TEXT("\u201eTolvaj! Fogj\u00e1tok meg! A biciklim!\u201d   (Thief! Stop him! My bicycle!)")
				: TEXT("\u201eTolvaj! Fogj\u00e1tok meg! A lovam!\u201d   (Thief! Stop him! My horse!)"));
		}
		Step(CarjackedDriver::ChaseSpeed, DeltaSeconds);
		return true;
	}
	case EMode::Fetch:
	{
		LookClock -= DeltaSeconds;
		if (LookClock <= 0.f)
		{
			LookClock = CarjackedDriver::LookEvery;
			if (!APolicePatrols::FindConstable(this, Here, CarjackedDriver::FetchRange, Constable))
			{
				Mode = EMode::Fist;
				ModeClock = 0.f;
				return true;
			}
			HeadFor(Constable);
		}
		if (FVector::Dist2D(Constable, Here) < CarjackedDriver::TellReach)
		{
			APolicePatrols::TipOff(this, Constable);
			APestiHUD::Show(this, CarjackedDriver::ShoutKey, CarjackedDriver::ShoutSeconds, FColor::Orange,
				TEXT("\u201eBiztos \u00far! Arra ment a tolvaj, arra!\u201d   (Constable! The thief went that way!)"));
			Mode = EMode::Away;
			Clock = CarjackedDriver::RunFor;
			return false;
		}
		if (ModeClock > CarjackedDriver::FetchFor)
		{
			Mode = EMode::Fist;
			ModeClock = 0.f;
			return true;
		}
		Step(CarjackedDriver::RunSpeed, DeltaSeconds);
		return true;
	}
	case EMode::Fist:
	{
		if (Player)
		{
			HeadFor(Player->GetActorLocation());
		}
		ShakeFist(Here, FMath::RadiansToDegrees(FMath::Atan2(Heading.Y, Heading.X)), ModeClock);
		if (ModeClock > CarjackedDriver::FistFor)
		{
			// Off he goes, walking.
			Mode = EMode::Away;
			Heading = -Heading;
			Clock = CarjackedDriver::RunFor;
			return false;
		}
		return true;
	}
	default:
		return false;
	}
}

void ACarjackedDriver::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Mode != EMode::Away && TickOwner(DeltaSeconds))
	{
		return;
	}
	Clock += DeltaSeconds;
	if (Clock > CarjackedDriver::GoneAfter)
	{
		Destroy();
		return;
	}
	Step(Clock < CarjackedDriver::RunFor ? CarjackedDriver::RunSpeed : CarjackedDriver::WalkSpeed, DeltaSeconds);
}

void ACarjackedDriver::Step(float Speed, float DeltaSeconds)
{
	const FVector Here = GetActorLocation();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CarjackedDriver), false, this);
	FHitResult Hit;

	// A wall or a car in the way: turn aside.
	const FVector Knee = Here - FVector(0.0, 0.0, 40.0);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Knee, Knee + FVector(Heading * CarjackedDriver::WallAhead, 0.0), ECC_Visibility, Params))
	{
		Heading = FVector2D(-Heading.Y, Heading.X);
	}

	const FVector2D At = FVector2D(Here) + Heading * Speed * DeltaSeconds;
	GroundClock -= DeltaSeconds;
	if (!bGroundKnown || GroundClock <= 0.f)
	{
		GroundClock = CarjackedDriver::GroundEvery;
		if (GetWorld()->LineTraceSingleByChannel(Hit, FVector(At, Here.Z + 100.0), FVector(At, Here.Z - 1000.0), ECC_Visibility, Params))
		{
			GroundZ = Hit.ImpactPoint.Z;
			bGroundKnown = true;
		}
	}
	const double Ground = bGroundKnown ? GroundZ : Here.Z - CarjackedDriver::Middle;
	const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(Heading.Y, Heading.X));
	WalkTo(FVector(At, Ground + CarjackedDriver::Middle * GetActorScale3D().Z), Yaw, Speed, DeltaSeconds);
}
