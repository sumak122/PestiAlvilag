// Fill out your copyright notice in the Description page of Project Settings.

#include "Witnesses.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Pedestrians.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "PolicePatrols.h"
#include "StreetFights.h"
#include "StreetName.h"
#include "TrafficManager.h"

namespace Witness
{
	// How often someone runs off to tell (a share of the times nobody from
	// the police saw it), at the lowest street name; at the best one only
	// this share of that. Never more often than once in this long (s).
	constexpr float RunChance = 0.3f;
	constexpr float BestNameShare = 0.1f;
	constexpr double RunEvery = 150.0;
	// A passer-by this near what happened (cm) and at least this far from the player.
	constexpr double SeenFrom = 2500.0;
	constexpr double NotNearPlayer = 400.0;
	// The box he runs to is at most this far (cm).
	constexpr double BoxReach = 9000.0;
	// Running and walking (cm/s), and how long he gives it before giving up (s).
	constexpr float RunSpeed = 430.f;
	constexpr float WalkSpeed = 130.f;
	constexpr float RunGiveUp = 90.f;
	// At the telephone: cranking for this long, then talking; the call is through after this long (s).
	constexpr float CrankSeconds = 1.5f;
	constexpr float CallSeconds = 6.f;
	constexpr float CurseSeconds = 3.f;
	// Walking off: this far (cm); gone once out of sight this long (s), or after this long at most.
	constexpr double LeaveDistance = 3000.0;
	constexpr float GoneUnseen = 1.5f;
	constexpr float LeaveAtMost = 40.f;
	// The player stops him this close (cm); paying him off costs this (fillér).
	constexpr double CatchRange = 180.0;
	constexpr int32 PayOff = 100;
	// His middle is this high over the ground (cm, at scale 1), the ! this far over that.
	constexpr double MiddleUp = 90.0;
	constexpr double MarkUp = 125.0;
	constexpr double Arrived = 40.0;
	// Leaving from this close to the box, he goes out of its door first (cm).
	constexpr double NearBox = 400.0;
	constexpr float TurnRate = 540.f;
	constexpr float GroundEvery = 0.25f;
	const FColor MarkColor(200, 32, 24);
	constexpr float MarkSize = 46.f;

	struct FLine2 { const TCHAR* Hu; const TCHAR* En; };
	const FLine2 OffLine = { TEXT("Ezt jelentem a rendőrségnek!"), TEXT("I'm reporting this to the police!") };
	const FLine2 CallLine = { TEXT("Halló, rendőrség? Itt egy gazember garázdálkodik!"), TEXT("Hello, police? There's a scoundrel running riot here!") };
	const FLine2 PaidLine = { TEXT("Jól van, jól van... nem láttam semmit."), TEXT("All right, all right... I saw nothing.") };
	const FLine2 BrokeLine = { TEXT("Ennyi pénze sincs? Akkor is szólok!"), TEXT("Not even that much on you? I'm telling them anyway!") };
	const FLine2 SmashedLine = { TEXT("Összetörték a fülkét! Na megállj..."), TEXT("They've smashed the box! Just you wait...") };
}

AWitnesses::AWitnesses()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Mark = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Mark"));
	Mark->SetupAttachment(Root);
	Mark->SetText(FText::FromString(TEXT("!")));
	Mark->SetTextRenderColor(Witness::MarkColor);
	Mark->SetWorldSize(Witness::MarkSize);
	Mark->SetHorizontalAlignment(EHTA_Center);
	Mark->SetVerticalAlignment(EVRTA_TextCenter);
	Mark->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mark->SetCastShadow(false);
	Mark->SetVisibility(false);

	Random.Initialize(4711);
}

void AWitnesses::Report(const UObject* WorldContextObject, const FVector& Where, int32 Stars)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}
	AWitnesses* Found = nullptr;
	for (TActorIterator<AWitnesses> It(World); It; ++It)
	{
		Found = *It;
		break;
	}
	if (!Found)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Found = World->SpawnActor<AWitnesses>(AWitnesses::StaticClass(), FTransform::Identity, Params);
		if (!Found)
		{
			return;
		}
	}
	// One at a time, and seldom: less often the better the player's street name.
	const double Now = World->GetTimeSeconds();
	if (Found->Stage != EStage::None || Found->bPending || Now - Found->LastRun < Witness::RunEvery)
	{
		return;
	}
	float NameShare = 0.f;
	if (const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(World))
	{
		NameShare = static_cast<float>(Game->GetRenownRank()) / static_cast<float>(FMath::Max(1, StreetName::NumRanks - 1));
	}
	const float Chance = Witness::RunChance * FMath::Lerp(1.f, Witness::BestNameShare, FMath::Clamp(NameShare, 0.f, 1.f));
	if (Found->Random.FRand() >= Chance)
	{
		return;
	}
	// Picked on the next tick, away from whatever called this.
	Found->bPending = true;
	Found->PendingAt = Where;
	Found->PendingStars = FMath::Max(1, Stars);
}

bool AWitnesses::Start(const FVector& Where, int32 Stars)
{
	// The nearest box that still works.
	APoliceBox* Nearest = nullptr;
	double NearestDistance = Witness::BoxReach;
	for (TActorIterator<APoliceBox> It(GetWorld()); It; ++It)
	{
		const double Distance = FVector::Dist2D(It->GetActorLocation(), Where);
		if (!It->IsSmashed() && Distance < NearestDistance)
		{
			NearestDistance = Distance;
			Nearest = *It;
		}
	}
	if (!Nearest)
	{
		return false;
	}
	PersonKit::FOutfit Dress;
	FVector Ground;
	float StartYaw = 0.f;
	if (!APedestrians::TakeWitness(this, Where, Witness::SeenFrom, Witness::NotNearPlayer, Dress, Ground, StartYaw))
	{
		return false;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Person = GetWorld()->SpawnActor<APedestrian>(APedestrian::StaticClass(), Ground + FVector(0.0, 0.0, Witness::MiddleUp),
		FRotator(0.0, StartYaw, 0.0), Params);
	if (!Person)
	{
		return false;
	}
	Person->SetOutfit(Dress);

	Box = Nearest;
	Spot = FVector2D(Ground);
	Facing = StartYaw;
	GroundZ = Ground.Z;
	GroundClock = 0.f;
	Clock = 0.f;
	UnseenClock = 0.f;
	CrimeStars = Stars;

	// Along the streets to the box's door, then in to the telephone.
	if (!Traffic.IsValid())
	{
		TActorIterator<ATrafficManager> FoundTraffic(GetWorld());
		Traffic = FoundTraffic ? *FoundTraffic : nullptr;
	}
	Path.Reset();
	Next = 0;
	const FVector2D Door = Nearest->DoorStep();
	TArray<FVector> Streets;
	if (FVector2D::Distance(Spot, Door) > 600.0 && Traffic.IsValid()
		&& Traffic->FindPath(FVector(Spot, GroundZ), FVector(Door, GroundZ), Streets))
	{
		for (const FVector& Point : Streets)
		{
			Path.Add(FVector2D(Point));
		}
		// Skip a first corner that lies behind him.
		if (Path.Num() > 1 && FVector2D::Distance(Spot, Path[1]) < FVector2D::Distance(Path[0], Path[1]))
		{
			Next = 1;
		}
	}
	Path.Add(Door);
	Path.Add(Nearest->PhoneSpot());
	Stage = EStage::Running;
	LastRun = GetWorld()->GetTimeSeconds();
	Say(Witness::OffLine.Hu, Witness::OffLine.En);
	return true;
}

void AWitnesses::Finish()
{
	if (Person)
	{
		Person->Destroy();
		Person = nullptr;
	}
	Mark->SetVisibility(false);
	Stage = EStage::None;
	Path.Reset();
	Box = nullptr;
}

FVector AWitnesses::Middle() const
{
	const double Scale = Person ? Person->GetActorScale3D().Z : 1.0;
	return FVector(Spot, GroundZ + Witness::MiddleUp * Scale);
}

void AWitnesses::Say(const TCHAR* Hu, const TCHAR* En)
{
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->Say(TEXT("Szemtanú"), Hu, En, 3.f);
	}
}

bool AWitnesses::FollowPath(float Speed, float DeltaSeconds)
{
	while (Next < Path.Num() - 1 && FVector2D::Distance(Spot, Path[Next]) < Witness::Arrived * 3.0)
	{
		++Next;
	}
	if (!Path.IsValidIndex(Next))
	{
		return true;
	}
	const FVector2D To = Path[Next] - Spot;
	const double Left = To.Size();
	if (Left > 1.0)
	{
		const double Step = FMath::Min(Left, static_cast<double>(Speed * DeltaSeconds));
		Spot += To / Left * Step;
		if (Left > 5.0)
		{
			const float WantYaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(To.Y, To.X)));
			Facing = FMath::FixedTurn(Facing, WantYaw, Witness::TurnRate * DeltaSeconds);
		}
	}
	return Next == Path.Num() - 1 && Left < Witness::Arrived;
}

void AWitnesses::UpdateGround(float DeltaSeconds)
{
	GroundClock -= DeltaSeconds;
	if (GroundClock > 0.f)
	{
		return;
	}
	GroundClock = Witness::GroundEvery;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WitnessGround), false, this);
	if (Person)
	{
		Params.AddIgnoredActor(Person);
	}
	if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Params.AddIgnoredActor(Player);
	}
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, FVector(Spot, GroundZ + 60.0), FVector(Spot, GroundZ - 200.0), ECC_Visibility, Params))
	{
		GroundZ = Hit.ImpactPoint.Z;
	}
}

bool AWitnesses::TickPlayer()
{
	APestiPerson* Player = Cast<APestiPerson>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player || Player->IsHidden() || Player->IsRiding() || FVector::Dist2D(Player->GetActorLocation(), Middle()) > Witness::CatchRange)
	{
		return false;
	}
	// A click: he has to fight for it.
	if (AStreetFights::WantsToPunch(Player))
	{
		AStreetFights* Fights = AStreetFights::Get(this);
		if (Fights && Person && Fights->Begin(Player, Person->GetOutfit(), FVector(Spot, GroundZ), Facing))
		{
			Finish();
			return true;
		}
	}
	APestiHUD* Hud = APestiHUD::Get(this);
	if (Hud)
	{
		Hud->Prompt(TEXT("E"), TEXT("Lefizetni (1 P)"), TEXT("Pay him off (1 P)"), false);
	}
	const APlayerController* PC = Cast<APlayerController>(Player->GetController());
	if (!PC || !PC->WasInputKeyJustPressed(EKeys::E))
	{
		return false;
	}
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (!Game || Game->GetMoney() < Witness::PayOff)
	{
		Say(Witness::BrokeLine.Hu, Witness::BrokeLine.En);
		return false;
	}
	Game->AddMoney(-Witness::PayOff);
	Say(Witness::PaidLine.Hu, Witness::PaidLine.En);
	// Off he goes the other way, quietly.
	Leave((Spot - FVector2D(Player->GetActorLocation())).GetSafeNormal());
	return true;
}

void AWitnesses::Leave(const FVector2D& Way)
{
	// Out of the box's door first (if he is at one), then off along the streets, this way or away from the box.
	Path.Reset();
	Next = 0;
	FVector2D From = Spot;
	FVector2D Ahead = Way;
	if (const APoliceBox* Target = Box.Get())
	{
		const FVector2D Door = Target->DoorStep();
		if (FVector2D::Distance(Spot, Door) < Witness::NearBox)
		{
			Path.Add(Door);
			From = Door;
			if (Ahead.IsNearlyZero())
			{
				Ahead = (Door - FVector2D(Target->GetActorLocation())).GetSafeNormal();
			}
		}
	}
	if (Ahead.IsNearlyZero())
	{
		Ahead = FVector2D(FMath::Cos(FMath::DegreesToRadians(Facing)), FMath::Sin(FMath::DegreesToRadians(Facing)));
	}
	const FVector2D Goal = From + Ahead * Witness::LeaveDistance;
	TArray<FVector> Streets;
	if (Traffic.IsValid() && Traffic->FindPath(FVector(From, GroundZ), FVector(Goal, GroundZ), Streets))
	{
		for (const FVector& Point : Streets)
		{
			Path.Add(FVector2D(Point));
		}
	}
	else
	{
		Path.Add(Goal);
	}
	Stage = EStage::Leaving;
	Clock = 0.f;
	UnseenClock = 0.f;
}

void AWitnesses::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bPending)
	{
		bPending = false;
		if (Stage == EStage::None)
		{
			Start(PendingAt, PendingStars);
		}
	}
	if (Stage == EStage::None)
	{
		return;
	}
	if (!Person)
	{
		Finish();
		return;
	}
	Clock += DeltaSeconds;
	UpdateGround(DeltaSeconds);

	if (Stage != EStage::Leaving && TickPlayer())
	{
		if (Stage == EStage::None)
		{
			return;
		}
	}

	switch (Stage)
	{
	case EStage::Running:
	{
		const bool bThere = FollowPath(Witness::RunSpeed, DeltaSeconds);
		Person->WalkTo(Middle(), Facing, Witness::RunSpeed, DeltaSeconds);
		if (Clock > Witness::RunGiveUp)
		{
			Leave(FVector2D(FMath::Cos(FMath::DegreesToRadians(Facing)), FMath::Sin(FMath::DegreesToRadians(Facing))));
			break;
		}
		if (bThere)
		{
			APoliceBox* Target = Box.Get();
			Clock = 0.f;
			if (!Target || Target->IsSmashed())
			{
				Stage = EStage::Cursing;
				Say(Witness::SmashedLine.Hu, Witness::SmashedLine.En);
			}
			else
			{
				Stage = EStage::Phoning;
				Spot = Target->PhoneSpot();
				Facing = static_cast<float>(Target->PhoneYaw());
			}
		}
		break;
	}
	case EStage::Phoning:
	{
		Person->PhoneAt(Middle(), Facing, Clock < Witness::CrankSeconds, Clock);
		if (Clock >= Witness::CrankSeconds && Clock - DeltaSeconds < Witness::CrankSeconds)
		{
			Say(Witness::CallLine.Hu, Witness::CallLine.En);
		}
		if (Clock >= Witness::CallSeconds)
		{
			// Through to the station: the player is wanted.
			if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
			{
				Game->ReportCrime(CrimeStars);
			}
			Leave(FVector2D::ZeroVector);
		}
		break;
	}
	case EStage::Cursing:
	{
		Person->ShakeFist(Middle(), Facing, Clock);
		if (Clock >= Witness::CurseSeconds)
		{
			Leave(FVector2D::ZeroVector);
		}
		break;
	}
	case EStage::Leaving:
	{
		// Off along the street; gone once nobody sees him.
		FollowPath(Witness::WalkSpeed, DeltaSeconds);
		Person->WalkTo(Middle(), Facing, Witness::WalkSpeed, DeltaSeconds);
		UnseenClock = Person->WasRecentlyRendered(0.2f) ? 0.f : UnseenClock + DeltaSeconds;
		if (UnseenClock > Witness::GoneUnseen || Clock > Witness::LeaveAtMost)
		{
			Finish();
			return;
		}
		break;
	}
	default:
		break;
	}

	// The red ! over him while he is on his way to tell, on the minimap too.
	const bool bMarked = Stage == EStage::Running || Stage == EStage::Phoning;
	Mark->SetVisibility(bMarked);
	if (bMarked)
	{
		const double Scale = Person->GetActorScale3D().Z;
		const FVector MarkAt = Middle() + FVector(0.0, 0.0, Witness::MarkUp * Scale);
		FRotator MarkFacing = FRotator::ZeroRotator;
		if (const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			if (PC->PlayerCameraManager)
			{
				MarkFacing.Yaw = (PC->PlayerCameraManager->GetCameraLocation() - MarkAt).Rotation().Yaw;
			}
		}
		Mark->SetWorldLocationAndRotation(MarkAt, MarkFacing);
		if (APestiHUD* Hud = APestiHUD::Get(this))
		{
			Hud->MarkPlace(MarkAt, TEXT("!"));
		}
	}
}
