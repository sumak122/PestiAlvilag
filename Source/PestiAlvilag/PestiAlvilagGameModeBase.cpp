// Fill out your copyright notice in the Description page of Project Settings.


#include "PestiAlvilagGameModeBase.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Cafe.h"
#include "HomeDoor.h"
#include "KeletiEagle.h"
#include "KeletiStation.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "MissionPrologue.h"
#include "MissionUzenet.h"
#include "Nepszinhaz.h"
#include "Pedestrians.h"
#include "PestiCar.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "PestiSaveGame.h"
#include "PestiTaxi.h"
#include "StreetFurniture.h"
#include "TaxiCompany.h"
#include "TaxiRides.h"
#include "TramRides.h"
#include "FoodMenus.h"
#include "TrafficManager.h"

namespace PestiStart
{
	// The Népszínház stood on the Corvin side of Blaha Lujza tér, its front
	// towards the west (cm, X east, Y south).
	const FVector2D TheatreAt(-3600.f, 200.f);
	constexpr float TheatreYaw = 180.f;

	// The player's car is parked on the square in front of it.
	const FVector2D CarAt(-8600.f, 2600.f);
	constexpr float Yaw = -23.f;

	// Bözsi néni's street door on the north side of Népszínház utca, between
	// Kun utca and Teleki tér, facing the street.
	const FVector2D HomeDoorAt(58392.f, 21268.f);
	constexpr float HomeDoorYaw = 108.6f;
	const FName Prologue(TEXT("Prologue"));
	const FName Uzenet(TEXT("Uzenet"));

	// The next story mission starts a little after the last one is passed.
	constexpr float NextMissionDelay = 8.f;

	// Fainting: the picture goes dark, and he comes round at home (or back
	// at the station before he has a home), a pengő poorer for the doctor.
	constexpr float FaintFade = 1.5f;
	constexpr float FaintDark = 2.5f;
	constexpr int32 DoctorFee = 100;

	// The taxi company's kiosk west of the square, two of its taxis waiting
	// on the open ground beside it.
	const FVector2D TaxiCompanyAt(-12000.f, 3100.f);
	const FVector2D TaxiAt[] = { FVector2D(-10400.f, 2500.f), FVector2D(-11100.f, 2500.f) };
	// The special taxi for regulars waits closest to the kiosk.
	const FVector2D DeluxeTaxiAt(-11800.f, 2500.f);

	// Below this speed next to a police car counts as being caught (cm/s).
	constexpr float CaughtSpeed = 300.f;
	constexpr int32 MaxWantedLevel = 3;
}

APestiAlvilagGameModeBase::APestiAlvilagGameModeBase()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = APestiPerson::StaticClass();
	HUDClass = APestiHUD::StaticClass();
}

void APestiAlvilagGameModeBase::SpawnDeluxeTaxi()
{
	if (TActorIterator<APestiDeluxeTaxi>(GetWorld()))
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	const FVector Location(PestiStart::DeluxeTaxiAt, GroundZ(PestiStart::DeluxeTaxiAt) + 150.0);
	GetWorld()->SpawnActor<APestiDeluxeTaxi>(APestiDeluxeTaxi::StaticClass(), Location, FRotator::ZeroRotator, Params);
}

APestiAlvilagGameModeBase* APestiAlvilagGameModeBase::Get(const UObject* WorldContextObject)
{
	return Cast<APestiAlvilagGameModeBase>(UGameplayStatics::GetGameMode(WorldContextObject));
}

void APestiAlvilagGameModeBase::StartPlay()
{
	Super::StartPlay();

	SpawnLandmarks();

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

	// Bözsi néni's door, the player's home.
	if (!TActorIterator<AHomeDoor>(GetWorld()))
	{
		const FVector Location(PestiStart::HomeDoorAt, GroundZ(PestiStart::HomeDoorAt));
		GetWorld()->SpawnActor<AHomeDoor>(AHomeDoor::StaticClass(), Location, FRotator(0.f, PestiStart::HomeDoorYaw, 0.f));
	}

	// The coffee house beside it, with its terrace on the pavement.
	if (!TActorIterator<ACafe>(GetWorld()))
	{
		const FVector Location(PestiStart::HomeDoorAt, GroundZ(PestiStart::HomeDoorAt));
		GetWorld()->SpawnActor<ACafe>(ACafe::StaticClass(), Location, FRotator(0.f, PestiStart::HomeDoorYaw, 0.f));
	}

	// Lamps, benches, advertising columns and the park on Tisza Kálmán tér.
	if (!TActorIterator<AStreetFurniture>(GetWorld()))
	{
		GetWorld()->SpawnActor<AStreetFurniture>();
	}

	// People walking the pavements.
	if (!TActorIterator<APedestrians>(GetWorld()))
	{
		GetWorld()->SpawnActor<APedestrians>();
	}

	// Taxis to ride in as a paying passenger.
	if (!TActorIterator<ATaxiRides>(GetWorld()))
	{
		GetWorld()->SpawnActor<ATaxiRides>();
	}

	// Food to buy at the café, the station restaurant and the street sellers.
	if (!TActorIterator<AFoodMenus>(GetWorld()))
	{
		GetWorld()->SpawnActor<AFoodMenus>();
	}

	// Trams to ride as a passenger.
	if (!TActorIterator<ATramRides>(GetWorld()))
	{
		GetWorld()->SpawnActor<ATramRides>();
	}

	// Start traffic unless the level already has its own manager.
	if (!TActorIterator<ATrafficManager>(GetWorld()))
	{
		GetWorld()->SpawnActor<ATrafficManager>();
	}

	// A new game begins with the prologue, a saved one with the first
	// mission not yet passed (traffic first: the prologue finds its route on the lanes).
	LoadSaveOnce();
	SpawnNextMission();
}

void APestiAlvilagGameModeBase::StartNextMission()
{
	NextMissionAt = GetWorld()->GetTimeSeconds() + PestiStart::NextMissionDelay;
}

void APestiAlvilagGameModeBase::SpawnNextMission()
{
	if (!IsMissionDone(PestiStart::Prologue))
	{
		GetWorld()->SpawnActor<AMissionPrologue>();
	}
	else if (!IsMissionDone(PestiStart::Uzenet))
	{
		GetWorld()->SpawnActor<AMissionUzenet>();
	}
}

APawn* APestiAlvilagGameModeBase::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	// A new game starts on the platform at Keleti station for the
	// prologue, a saved one at the home door, whatever PlayerStart the
	// level has. The station must stand first: the player starts inside it.
	SpawnLandmarks();
	LoadSaveOnce();
	FVector2D StartAt = AMissionPrologue::StartAt();
	double StartYaw = AMissionPrologue::StartYaw();
	if (IsMissionDone(PestiStart::Prologue))
	{
		StartAt = HomeStartAt();
		StartYaw = PestiStart::HomeDoorYaw;
	}
	const FVector PersonLocation(StartAt, GroundZ(StartAt) + 100.0);
	APawn* Pawn = SpawnDefaultPawnAtTransform(NewPlayer, FTransform(FRotator(0.0, StartYaw, 0.0), PersonLocation));

	// The player's car, parked in front of the Népszínház (it settles onto the ground itself).
	const FRotator Facing(0.f, PestiStart::Yaw, 0.f);
	FActorSpawnParameters CarParams;
	CarParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	const FVector CarLocation(PestiStart::CarAt, GroundZ(PestiStart::CarAt) + 150.0);
	GetWorld()->SpawnActor<APestiCar>(APestiCar::StaticClass(), CarLocation, Facing, CarParams);

	return Pawn;
}

void APestiAlvilagGameModeBase::SpawnLandmarks()
{
	// Put up the Népszínház unless the level already has one.
	if (!TActorIterator<ANepszinhaz>(GetWorld()))
	{
		const FVector Location(PestiStart::TheatreAt, GroundZ(PestiStart::TheatreAt));
		GetWorld()->SpawnActor<ANepszinhaz>(ANepszinhaz::StaticClass(), Location, FRotator(0.f, PestiStart::TheatreYaw, 0.f));
	}

	// And Keleti station, its front on Baross tér.
	if (!TActorIterator<AKeletiStation>(GetWorld()))
	{
		// Its ground height is taken on the square just in front of it.
		const FVector2D At = AKeletiStation::FrontAt();
		const double Z = GroundZ(At - FVector2D(800.0, 0.0));
		GetWorld()->SpawnActor<AKeletiStation>(AKeletiStation::StaticClass(), FVector(At, Z), FRotator::ZeroRotator);
	}

	// And the eagle that circles over it.
	if (!TActorIterator<AKeletiEagle>(GetWorld()))
	{
		const FVector2D At = AKeletiStation::FrontAt();
		const double Z = GroundZ(At - FVector2D(800.0, 0.0));
		GetWorld()->SpawnActor<AKeletiEagle>(AKeletiEagle::StaticClass(), FVector(At, Z), FRotator::ZeroRotator);
	}
}

FVector2D APestiAlvilagGameModeBase::HomeStartAt()
{
	const FVector2D Out = FVector2D(FRotator(0.f, PestiStart::HomeDoorYaw, 0.f).Vector()) * 150.0;
	return PestiStart::HomeDoorAt + Out;
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

	if (PC && PC->WasInputKeyJustPressed(EKeys::F9))
	{
		StartNewGame();
		return;
	}

	UpdateWanted(DeltaSeconds);
	UpdateFaint();

	if (NextMissionAt > 0.0 && GetWorld()->GetTimeSeconds() >= NextMissionAt)
	{
		NextMissionAt = -1.0;
		SpawnNextMission();
	}
}

void APestiAlvilagGameModeBase::HurtPlayer(float Amount)
{
	if (FaintUntil < 0.0)
	{
		Health = FMath::Max(0.f, Health - Amount);
	}
}

void APestiAlvilagGameModeBase::HealPlayer(float Amount)
{
	Health = FMath::Min(MaxHealth, Health + Amount);
}

void APestiAlvilagGameModeBase::UpdateFaint()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const double Now = GetWorld()->GetTimeSeconds();
	if (FaintUntil < 0.0)
	{
		if (Health > 0.f || !PC)
		{
			return;
		}
		// Down he goes: the picture darkens and the police lose interest.
		FaintUntil = Now + PestiStart::FaintDark;
		PC->SetIgnoreMoveInput(true);
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, PestiStart::FaintFade, FLinearColor::Black, false, true);
		}
		WantedLevel = 0;
		TimeUnseen = 0.f;
		ArrestProgress = 0.f;
		return;
	}
	if (Now < FaintUntil)
	{
		return;
	}

	// He comes round at home, or on the platform before he has a home.
	FaintUntil = -1.0;
	Health = MaxHealth;
	const bool bHome = IsMissionDone(PestiStart::Prologue);
	const FVector2D WakeAt = bHome ? HomeStartAt() : AMissionPrologue::StartAt();
	const double WakeYaw = bHome ? PestiStart::HomeDoorYaw : AMissionPrologue::StartYaw();
	if (PC)
	{
		if (APestiPerson* Person = Cast<APestiPerson>(PC->GetPawn()))
		{
			Person->SetActorLocationAndRotation(FVector(WakeAt, GroundZ(WakeAt) + 100.0), FRotator(0.0, WakeYaw, 0.0), false, nullptr, ETeleportType::TeleportPhysics);
			PC->SetControlRotation(FRotator(-10.0, WakeYaw, 0.0));
		}
		PC->SetIgnoreMoveInput(false);
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, PestiStart::FaintFade, FLinearColor::Black);
		}
	}
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		if (bHome)
		{
			AddMoney(-PestiStart::DoctorFee);
			Hud->Announce(TEXT("EL\u00c1JULT\u00c1L"), TEXT("You fainted. B\u00f6zsi n\u00e9ni fetched the doctor: \u22121 peng\u0151"), 6.f);
		}
		else
		{
			Hud->Announce(TEXT("EL\u00c1JULT\u00c1L"), TEXT("You fainted. A porter helped you back to your feet."), 6.f);
		}
	}
}

void APestiAlvilagGameModeBase::ReportCrime(int32 CrimeLevel)
{
	if (WantedLevel == 0)
	{
		APestiHUD::Show(this, 3, 4.f, FColor::Red, TEXT("The police are after you!"));
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
	APestiHUD::Show(this, 3, 4.f, FColor::Yellow, Message);
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

void APestiAlvilagGameModeBase::LoadSaveOnce()
{
	if (bSaveRead)
	{
		return;
	}
	bSaveRead = true;
	if (const UPestiSaveGame* Save = Cast<UPestiSaveGame>(UGameplayStatics::LoadGameFromSlot(UPestiSaveGame::SlotName(), 0)))
	{
		Money = Save->Money;
		CompletedMissions = Save->CompletedMissions;
		TaxiJobs = Save->TaxiJobs;
		TriedDishes = Save->TriedDishes;
	}
}

void APestiAlvilagGameModeBase::MarkMissionDone(FName MissionId)
{
	CompletedMissions.AddUnique(MissionId);
}

bool APestiAlvilagGameModeBase::CanSaveAtHome() const
{
	return IsMissionDone(PestiStart::Prologue);
}

bool APestiAlvilagGameModeBase::SaveToDisk() const
{
	UPestiSaveGame* Save = Cast<UPestiSaveGame>(UGameplayStatics::CreateSaveGameObject(UPestiSaveGame::StaticClass()));
	if (!Save)
	{
		return false;
	}
	Save->Money = Money;
	Save->CompletedMissions = CompletedMissions;
	Save->TaxiJobs = TaxiJobs;
	Save->TriedDishes = TriedDishes;
	return UGameplayStatics::SaveGameToSlot(Save, UPestiSaveGame::SlotName(), 0);
}

void APestiAlvilagGameModeBase::StartNewGame()
{
	UGameplayStatics::DeleteGameInSlot(UPestiSaveGame::SlotName(), 0);
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
}
