// Fill out your copyright notice in the Description page of Project Settings.

#include "PoliceCar.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiPerson.h"
#include "TrafficManager.h"
#include "UObject/ConstructorHelpers.h"

namespace PoliceCar
{
	// Within this distance of the player, with a clear view, drive straight at them (cm).
	constexpr float DirectChaseRange = 3500.f;
	// A path point counts as reached this close (cm).
	constexpr float ReachRadius = 400.f;
	// Seconds between working out a new way to the player.
	constexpr float ReplanInterval = 1.5f;
	constexpr float SightInterval = 0.25f;
	// Close enough to the player to be arresting them (cm).
	constexpr float ArrestRange = 800.f;
	// Slowest fraction of chase speed in a sharp corner.
	constexpr float CornerSpeedFraction = 0.3f;
	// Seconds stuck on another car before shoving through it.
	constexpr float MaxBlockedTime = 1.f;

	const FLinearColor SignWhite(0.9f, 0.9f, 0.85f);
	const FLinearColor SignRed(0.9f, 0.02f, 0.02f);
	// "RENDŐRSÉG" (police), written with escapes to keep the source plain ASCII.
	const TCHAR* SignWord = TEXT("RENDŐRSÉG");
}

APoliceCar::APoliceCar()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	// On the saloon's roof (see CarModel: the roof runs from about x -150 to x 0, top at z 80).
	RoofSign = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RoofSign"));
	RoofSign->SetupAttachment(RootComponent);
	RoofSign->SetStaticMesh(CubeMesh.Object);
	RoofSign->SetRelativeLocation(FVector(-75.f, 0.f, 91.f));
	RoofSign->SetRelativeScale3D(FVector(0.9f, 0.9f, 0.22f));
	RoofSign->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	auto MakeSignText = [&](const TCHAR* PartName, float Side)
	{
		UTextRenderComponent* Text = CreateDefaultSubobject<UTextRenderComponent>(PartName);
		Text->SetupAttachment(RootComponent);
		Text->SetRelativeLocation(FVector(-75.f, Side * 46.f, 91.f));
		Text->SetRelativeRotation(FRotator(0.f, Side * 90.f, 0.f));
		Text->SetText(FText::FromString(PoliceCar::SignWord));
		Text->SetHorizontalAlignment(EHTA_Center);
		Text->SetVerticalAlignment(EVRTA_TextCenter);
		Text->SetWorldSize(13.f);
		Text->SetTextRenderColor(FColor(10, 10, 30));
		Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Text;
	};
	SignTextLeft = MakeSignText(TEXT("SignTextLeft"), -1.f);
	SignTextRight = MakeSignText(TEXT("SignTextRight"), 1.f);
}

void APoliceCar::BeginPlay()
{
	Super::BeginPlay();

	if (BaseMaterial)
	{
		SignPaint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		SignPaint->SetVectorParameterValue(TEXT("Color"), PoliceCar::SignWhite);
		RoofSign->SetMaterial(0, SignPaint);
	}
}

CarModel::FStyle APoliceCar::PickStyle(const FLinearColor& Paint) const
{
	// A plain saloon in the force's dark blue, on steel disc wheels.
	CarModel::FStyle Look;
	Look.PaintColor = Paint;
	Look.Wheel = CarModel::EWheel::Disc;
	Look.SpokeColor = Paint;
	Look.bPolice = true;
	Look.AccentColor = FLinearColor(0.05f, 0.25f, 1.0f);
	return Look;
}

void APoliceCar::Tick(float DeltaSeconds)
{
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (!Player || !Game || !Manager)
	{
		Super::Tick(DeltaSeconds);
		return;
	}

	const bool bWanted = Game->GetWantedLevel() > 0;
	// Called by telephone from a police box, every car in the district comes.
	const float Range = Game->IsPoliceCalledIn() ? CalledInRange : ResponseRange;
	if (!bChasing && bWanted && FVector::Dist2D(GetActorLocation(), Player->GetActorLocation()) < Range)
	{
		StartChase();
	}
	else if (bChasing && !bWanted)
	{
		StopChase();
	}

	if (!bChasing)
	{
		Super::Tick(DeltaSeconds);
		return;
	}
	// Skip the lane driving in ATrafficCar::Tick while chasing.
	AActor::Tick(DeltaSeconds);
	ChaseTick(Player, Game, DeltaSeconds);
}

void APoliceCar::StartChase()
{
	bChasing = true;
	bSeesPlayer = false;
	ReplanTimer = 0.f;
	SightTimer = 0.f;
	ChasePath.Reset();
	ReleaseAllJunctions();
}

void APoliceCar::StopChase()
{
	bChasing = false;
	ChasePath.Reset();
	if (SignPaint)
	{
		SignPaint->SetVectorParameterValue(TEXT("Color"), PoliceCar::SignWhite);
	}
	int32 Lane = INDEX_NONE;
	int32 Point = 0;
	if (Manager->FindLanePointNear(GetActorLocation(), GetActorForwardVector().GetSafeNormal2D(), Lane, Point))
	{
		ResumeOnLane(Lane, Point);
	}
}

void APoliceCar::ChaseTick(APawn* Player, APestiAlvilagGameModeBase* Game, float DeltaSeconds)
{
	const FVector Here = GetActorLocation();
	const FVector PlayerAt = Player->GetActorLocation();
	const float DistToPlayer = static_cast<float>(FVector::Dist2D(Here, PlayerAt));

	SightTimer -= DeltaSeconds;
	if (SightTimer <= 0.f)
	{
		SightTimer = PoliceCar::SightInterval;
		bSeesPlayer = CanSee(Player);
	}
	if (bSeesPlayer)
	{
		Game->NotifySeenByPolice();
	}
	if (DistToPlayer < PoliceCar::ArrestRange)
	{
		Game->NotifyPoliceNear();
	}

	// Flash the roof sign.
	FlashTimer += DeltaSeconds;
	if (SignPaint)
	{
		const bool bRed = FMath::Fmod(FlashTimer, 0.6f) < 0.3f;
		SignPaint->SetVectorParameterValue(TEXT("Color"), bRed ? PoliceCar::SignRed : PoliceCar::SignWhite);
	}

	// Work out the way: straight at the player when close and in view,
	// otherwise along the streets.
	FVector Target = PlayerAt;
	const bool bDirect = bSeesPlayer && DistToPlayer < PoliceCar::DirectChaseRange;
	if (bDirect)
	{
		ChasePath.Reset();
	}
	else
	{
		ReplanTimer -= DeltaSeconds;
		if (ReplanTimer <= 0.f || ChasePathIndex >= ChasePath.Num())
		{
			ReplanTimer = PoliceCar::ReplanInterval;
			Manager->FindPath(Here, PlayerAt, ChasePath);
			ChasePath.Add(PlayerAt);
			ChasePathIndex = 0;
			// Don't turn back for a point that is already behind us.
			while (ChasePathIndex + 1 < ChasePath.Num()
				&& FVector::Dist2D(Here, ChasePath[ChasePathIndex + 1]) < FVector::Dist2D(ChasePath[ChasePathIndex], ChasePath[ChasePathIndex + 1]))
			{
				++ChasePathIndex;
			}
		}
		while (ChasePathIndex < ChasePath.Num() - 1 && FVector::Dist2D(Here, ChasePath[ChasePathIndex]) < PoliceCar::ReachRadius)
		{
			++ChasePathIndex;
		}
		if (ChasePath.IsValidIndex(ChasePathIndex))
		{
			Target = ChasePath[ChasePathIndex];
		}
	}

	const FVector ToTarget = (Target - Here).GetSafeNormal2D();
	const float CurrentYaw = static_cast<float>(GetActorRotation().Yaw);
	const float WantedYaw = static_cast<float>(ToTarget.Rotation().Yaw);
	const float TurnNeeded = FMath::Abs(FRotator::NormalizeAxis(WantedYaw - CurrentYaw));
	SetActorRotation(FRotator(0.f, FMath::FixedTurn(CurrentYaw, WantedYaw, ChaseTurnRate * DeltaSeconds), 0.f));

	float WantedSpeed = ChaseSpeed * FMath::Lerp(1.f, PoliceCar::CornerSpeedFraction, FMath::Clamp(TurnNeeded / 90.f, 0.f, 1.f));
	if (bDirect)
	{
		// Close in, then keep pace to box the player in.
		const float PlayerSpeed = static_cast<float>(Player->GetVelocity().Size2D());
		WantedSpeed = FMath::Min(WantedSpeed, PlayerSpeed + FMath::Max(0.f, DistToPlayer - 500.f) * 1.5f);
	}
	const float Rate = WantedSpeed < CurrentSpeed ? BrakeDeceleration : Acceleration * 2.f;
	CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, WantedSpeed, DeltaSeconds, Rate);

	MoveChasing(GetActorForwardVector().GetSafeNormal2D() * CurrentSpeed * DeltaSeconds, Player, DeltaSeconds);
}

bool APoliceCar::CanSee(const APawn* Player) const
{
	const FVector Eye = GetActorLocation() + FVector(0.f, 0.f, 100.f);
	const FVector PlayerAt = Player->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	if (FVector::Dist2D(Eye, PlayerAt) > SightRange)
	{
		return false;
	}
	// Only buildings block the view, not other cars.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PoliceSight), false, this);
	Params.AddIgnoredActor(Player);
	FCollisionObjectQueryParams Objects(ECC_WorldStatic);
	FHitResult Hit;
	return !GetWorld()->LineTraceSingleByObjectType(Hit, Eye, PlayerAt, Objects, Params);
}

void APoliceCar::MoveChasing(const FVector& Delta, const APawn* Player, float DeltaSeconds)
{
	FHitResult Hit;
	AddActorWorldOffset(Delta, true, &Hit);
	if (Hit.bStartPenetrating)
	{
		AddActorWorldOffset(FVector(Hit.Normal.X, Hit.Normal.Y, 0.f) * (Hit.PenetrationDepth + 1.f), false);
		return;
	}
	if (!Hit.bBlockingHit)
	{
		BlockedTime = 0.f;
		return;
	}

	const AActor* Blocker = Hit.GetActor();
	if (Blocker == Player)
	{
		// Bumping the player: hold position against them. Running into
		// him on foot hurts him.
		if (APestiPerson* Person = Cast<APestiPerson>(UGameplayStatics::GetPlayerPawn(this, 0)))
		{
			Person->HitByVehicle(CurrentSpeed, GetActorForwardVector());
		}
		CurrentSpeed *= 0.5f;
		return;
	}

	// Scrape along walls; shove through other traffic after a moment.
	BlockedTime += DeltaSeconds;
	if (Blocker && Blocker->IsA<ATrafficCar>() && BlockedTime > PoliceCar::MaxBlockedTime)
	{
		AddActorWorldOffset(Delta * (1.f - Hit.Time), false);
		return;
	}
	const FVector WallNormal = FVector(Hit.Normal.X, Hit.Normal.Y, 0.f).GetSafeNormal();
	CurrentSpeed *= 1.f - 0.5f * static_cast<float>(FMath::Abs(FVector::DotProduct(GetActorForwardVector(), WallNormal)));
	AddActorWorldOffset(FVector::VectorPlaneProject(Delta * (1.f - Hit.Time), WallNormal), true);
}
