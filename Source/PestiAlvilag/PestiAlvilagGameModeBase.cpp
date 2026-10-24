// Fill out your copyright notice in the Description page of Project Settings.


#include "PestiAlvilagGameModeBase.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Nepszinhaz.h"
#include "PestiCar.h"
#include "PestiPerson.h"
#include "PestiTaxi.h"
#include "TaxiCompany.h"
#include "TrafficManager.h"

namespace PestiStart
{
	// The Népszínház stood on the Corvin side of Blaha Lujza tér, its front
	// towards the west (cm, X east, Y south).
	const FVector2D TheatreAt(-3600.f, 200.f);
	constexpr float TheatreYaw = 180.f;

	// The player starts on the square in front of it, looking at its front.
	const FVector2D PersonAt(-9800.f, 1500.f);
	const FVector2D CarAt(-8600.f, 2600.f);
	constexpr float Yaw = -23.f;

	// The taxi company's kiosk west of the square, two of its taxis waiting
	// on the open ground beside it.
	const FVector2D TaxiCompanyAt(-12000.f, 3100.f);
	const FVector2D TaxiAt[] = { FVector2D(-10400.f, 2500.f), FVector2D(-11100.f, 2500.f) };

	// Below this speed next to a police car counts as being caught (cm/s).
	constexpr float CaughtSpeed = 300.f;
	constexpr int32 MaxWantedLevel = 3;
}

APestiAlvilagGameModeBase::APestiAlvilagGameModeBase()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = APestiPerson::StaticClass();
}

APestiAlvilagGameModeBase* APestiAlvilagGameModeBase::Get(const UObject* WorldContextObject)
{
	return Cast<APestiAlvilagGameModeBase>(UGameplayStatics::GetGameMode(WorldContextObject));
}

void APestiAlvilagGameModeBase::StartPlay()
{
	Super::StartPlay();

	// Put up the Népszínház unless the level already has one.
	if (!TActorIterator<ANepszinhaz>(GetWorld()))
	{
		const FVector Location(PestiStart::TheatreAt, GroundZ(PestiStart::TheatreAt));
		GetWorld()->SpawnActor<ANepszinhaz>(ANepszinhaz::StaticClass(), Location, FRotator(0.f, PestiStart::TheatreYaw, 0.f));
	}

	// Put up the taxi company and its taxis unless the level already has one.
	if (!TActorIterator<ATaxiCompany>(GetWorld()))
	{
		const FVector Location(PestiStart::TaxiCompanyAt, GroundZ(PestiStart::TaxiCompanyAt));
		GetWorld()->SpawnActor<ATaxiCompany>(ATaxiCompany::StaticClass(), Location, FRotator::ZeroRotator);

		FActorSpawnParameters TaxiParams;
		TaxiParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		for (const FVector2D& At : PestiStart::TaxiAt)
		{
			const FVector TaxiLocation(At, GroundZ(At) + 150.0);
			GetWorld()->SpawnActor<APestiTaxi>(APestiTaxi::StaticClass(), TaxiLocation, FRotator::ZeroRotator, TaxiParams);
		}
	}

	// Start traffic unless the level already has its own manager.
	if (!TActorIterator<ATrafficManager>(GetWorld()))
	{
		GetWorld()->SpawnActor<ATrafficManager>();
	}
}

APawn* APestiAlvilagGameModeBase::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	// Start on foot in front of the Népszínház, whatever PlayerStart the level has.
	const FRotator Facing(0.f, PestiStart::Yaw, 0.f);
	const FVector PersonLocation(PestiStart::PersonAt, GroundZ(PestiStart::PersonAt) + 100.0);
	APawn* Pawn = SpawnDefaultPawnAtTransform(NewPlayer, FTransform(Facing, PersonLocation));

	// The player's car, parked a few steps away (it settles onto the ground itself).
	FActorSpawnParameters CarParams;
	CarParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	const FVector CarLocation(PestiStart::CarAt, GroundZ(PestiStart::CarAt) + 150.0);
	GetWorld()->SpawnActor<APestiCar>(APestiCar::StaticClass(), CarLocation, Facing, CarParams);

	return Pawn;
}

double APestiAlvilagGameModeBase::GroundZ(const FVector2D& At) const
{
	FHitResult Hit;
	const FVector Top(At, 10000.0);
	const FVector Bottom(At, -10000.0);
	return GetWorld()->LineTraceSingleByChannel(Hit, Top, Bottom, ECC_Visibility) ? Hit.ImpactPoint.Z : 0.0;
}

void APestiAlvilagGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC && PC->WasInputKeyJustPressed(EKeys::P))
	{
		if (WantedLevel > 0)
		{
			ClearWanted(TEXT("Police called off (P)"));
		}
		else
		{
			ReportCrime(1);
		}
	}

	UpdateWanted(DeltaSeconds);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(5, 0.f, FColor(255, 220, 120), FString::Printf(TEXT("P\u00e9nz (money): %s"), *FormatMoney(Money)));
	}

	if (GEngine && WantedLevel > 0)
	{
		GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Red,
			FString::Printf(TEXT("WANTED %s   lose them: %d s%s"), *FString::ChrN(WantedLevel, TEXT('*')),
				FMath::CeilToInt(FMath::Max(0.f, EscapeTime - TimeUnseen)),
				ArrestProgress > 0.f ? *FString::Printf(TEXT("   ARREST %d%%"), FMath::RoundToInt(100.f * ArrestProgress / ArrestTime)) : TEXT("")));
	}
}

void APestiAlvilagGameModeBase::ReportCrime(int32 CrimeLevel)
{
	if (WantedLevel == 0 && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(3, 4.f, FColor::Red, TEXT("The police are after you!"));
	}
	WantedLevel = FMath::Clamp(FMath::Max(WantedLevel, CrimeLevel), 0, PestiStart::MaxWantedLevel);
	TimeUnseen = 0.f;
}

void APestiAlvilagGameModeBase::NotifySeenByPolice()
{
	TimeUnseen = 0.f;
}

void APestiAlvilagGameModeBase::NotifyPoliceNear()
{
	LastPoliceNearTime = GetWorld()->GetTimeSeconds();
}

void APestiAlvilagGameModeBase::UpdateWanted(float DeltaSeconds)
{
	if (WantedLevel == 0)
	{
		return;
	}

	TimeUnseen += DeltaSeconds;
	if (TimeUnseen > EscapeTime)
	{
		ClearWanted(TEXT("You lost the police!"));
		return;
	}

	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const bool bPoliceNear = GetWorld()->GetTimeSeconds() - LastPoliceNearTime < 0.2;
	if (Player && bPoliceNear && Player->GetVelocity().Size2D() < PestiStart::CaughtSpeed)
	{
		ArrestProgress += DeltaSeconds;
		if (ArrestProgress > ArrestTime)
		{
			if (APestiCar* Car = Cast<APestiCar>(UGameplayStatics::GetPlayerPawn(this, 0)))
			{
				Car->ForwardSpeed = 0.f;
			}
			ClearWanted(TEXT("BUSTED! The police caught you."));
		}
	}
	else
	{
		ArrestProgress = FMath::Max(0.f, ArrestProgress - DeltaSeconds);
	}
}

void APestiAlvilagGameModeBase::ClearWanted(const TCHAR* Message)
{
	WantedLevel = 0;
	TimeUnseen = 0.f;
	ArrestProgress = 0.f;
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Red, TEXT(""));
		GEngine->AddOnScreenDebugMessage(3, 4.f, FColor::Yellow, Message);
	}
}

void APestiAlvilagGameModeBase::AddMoney(int32 Filler)
{
	Money = FMath::Max(0, Money + Filler);
}

FString APestiAlvilagGameModeBase::FormatMoney(int32 Filler)
{
	const int32 Sign = Filler < 0 ? -1 : 1;
	const int32 Abs = FMath::Abs(Filler);
	return FString::Printf(TEXT("%s%d,%02d peng\u0151"), Sign < 0 ? TEXT("-") : TEXT(""), Abs / 100, Abs % 100);
}
