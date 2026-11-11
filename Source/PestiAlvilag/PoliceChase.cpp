// Fill out your copyright notice in the Description page of Project Settings.

#include "PoliceChase.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "OsmGraybox.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiCar.h"
#include "PoliceModel.h"
#include "PolicePatrols.h"
#include "PoliceStationModel.h"
#include "ShapeSet.h"
#include "TrafficManager.h"

namespace PoliceChase
{
	// The district police station's front on Víg utca: the middle of the
	// house front facing the street (map cm), and the way out to the street.
	const FVector2D StationDoor(30169.5, 51534.0);
	constexpr double StationFacing = 160.43;
	constexpr float StationDrawDistance = 20000.f;

	// Road blocks: at most this many at once, a new one no sooner than this
	// after the last (s); placed this far ahead of the player (cm), never
	// nearer than MinAhead, and only where he can't see it go up.
	constexpr int32 MaxBlocks = 2;
	constexpr double BlockEvery = 14.0;
	const double TryAhead[] = { 22000.0, 17000.0, 27000.0 };
	constexpr double MinAhead = 12000.0;
	constexpr double SeenFrom = 20000.0;
	// The road's width: looked for this far each way, the pavements this wide
	// are left open, and a block spans at least and at most this (cm).
	constexpr double LookAcross = 2500.0;
	constexpr double KeepPavement = 320.0;
	constexpr double MinSpan = 450.0;
	constexpr double MaxSpan = 1800.0;
	constexpr double BarrierEvery = 265.0;
	constexpr double Stagger = 35.0;
	// Taken away once the player is this far off, or this long after he is no longer wanted (s).
	constexpr double ClearBeyond = 45000.0;
	constexpr double ClearPassed = 15000.0;
	constexpr double ClearWhenCalm = 6.0;
	constexpr double MaxAge = 150.0;
	constexpr float DrawDistance = 30000.f;

	// A car touching a barrier: how near (along the car, across the plank, cm),
	// and from what speed it dents the car and loses it speed (cm/s).
	constexpr double TouchAlong = 230.0;
	constexpr double TouchAcross = 90.0;
	constexpr float DentFrom = 700.f;
	constexpr float SpeedKept = 0.75f;
	constexpr float Gravity = 980.f;

	// The constables: how far they see, how near counts as beside them (cm), how often they look (s).
	constexpr double SeeRange = 4500.0;
	constexpr double NearRange = 700.0;
	constexpr float LookEvery = 0.25f;

	TArray<FVector> Points;
}

APoliceChase::APoliceChase()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

APoliceChase* APoliceChase::Get(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	TActorIterator<APoliceChase> It(const_cast<UWorld*>(World));
	return It ? *It : nullptr;
}

const TArray<FVector>& APoliceChase::BlockPoints()
{
	return PoliceChase::Points;
}

void APoliceChase::BeginPlay()
{
	Super::BeginPlay();
	PoliceChase::Points.Reset();
	Looks.Initialize(1932);
	BuildStation();
}

void APoliceChase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Taken out of a running game: its blocks and guards with it (at the
	// level's end they go anyway).
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		for (FBlock& Block : Blocks)
		{
			ClearBlock(Block);
		}
		for (const TWeakObjectPtr<APoliceman>& Man : Sentries)
		{
			if (APoliceman* Guard = Man.Get())
			{
				Guard->Destroy();
			}
		}
	}
	Blocks.Reset();
	PoliceChase::Points.Reset();
	Super::EndPlay(EndPlayReason);
}

bool APoliceChase::GroundAt(const FVector& Near, double& OutZ) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PoliceChaseGround), false, this);
	TArray<FHitResult> Hits;
	GetWorld()->LineTraceMultiByObjectType(Hits, Near + FVector(0.0, 0.0, 600.0), Near - FVector(0.0, 0.0, 1500.0),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params);
	for (const FHitResult& Hit : Hits)
	{
		if (Cast<AOsmGraybox>(Hit.GetActor()))
		{
			OutZ = Hit.ImpactPoint.Z;
			return true;
		}
	}
	return false;
}

ATrafficManager* APoliceChase::FindTraffic()
{
	if (!Traffic.IsValid())
	{
		TActorIterator<ATrafficManager> It(GetWorld());
		Traffic = It ? *It : nullptr;
	}
	return Traffic.Get();
}

void APoliceChase::BuildStation()
{
	const FVector2D Out = FVector2D(FMath::Cos(FMath::DegreesToRadians(PoliceChase::StationFacing)), FMath::Sin(FMath::DegreesToRadians(PoliceChase::StationFacing)));
	double Ground = 0.0;
	if (!GroundAt(FVector(PoliceChase::StationDoor + Out * 200.0, 0.0), Ground))
	{
		// The city is not there yet: try again next frame.
		return;
	}
	bStationBuilt = true;
	StationAt = FVector(PoliceChase::StationDoor, Ground);
	StationYaw = PoliceChase::StationFacing;
	const FTransform Frame(FRotator(0.0, StationYaw, 0.0), StationAt);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AShapeSet* Front = GetWorld()->SpawnActor<AShapeSet>(AShapeSet::StaticClass(), Frame, Params))
	{
		Front->SetDrawDistance(PoliceChase::StationDrawDistance);
		TArray<MarketModel::FShape> Shapes;
		PoliceStationModel::BuildStationFront(Shapes);
		Front->AddShapes(Shapes);
		TArray<BozsiHouseModel::FSign> Signs;
		PoliceStationModel::StationSigns(Signs);
		for (const BozsiHouseModel::FSign& Sign : Signs)
		{
			Front->AddSign(Sign.Text, Sign.At, Sign.Yaw, Sign.Height, Sign.Color);
		}
		TArray<BozsiHouseModel::FLamp> Lamps;
		PoliceStationModel::StationLamps(Lamps);
		for (const BozsiHouseModel::FLamp& Lamp : Lamps)
		{
			Front->AddLamp(Lamp.At, Lamp.Radius);
		}
	}

	// Two constables on guard either side of the door.
	for (const FVector& Spot : { PoliceStationModel::SentryLeft, PoliceStationModel::SentryRight })
	{
		const FVector At = Frame.TransformPosition(Spot) + FVector(0.0, 0.0, PersonModel::MiddleHeight);
		APoliceman* Guard = GetWorld()->SpawnActor<APoliceman>(APoliceman::StaticClass(), At, FRotator(0.0, StationYaw, 0.0), Params);
		if (!Guard)
		{
			continue;
		}
		Guard->PutOnUniform(PoliceModel::Outfit(Looks));
		Guard->Stand(At, StationYaw);
		Sentries.Add(Guard);
		SentryAt.Add(At);
	}
}

bool APoliceChase::GetLetOut(FVector& OutAt, double& OutYaw) const
{
	if (!bStationBuilt)
	{
		return false;
	}
	const FTransform Frame(FRotator(0.0, StationYaw, 0.0), StationAt);
	OutAt = Frame.TransformPosition(PoliceStationModel::LetOutAt);
	double Z = 0.0;
	if (GroundAt(OutAt, Z))
	{
		OutAt.Z = Z;
	}
	OutYaw = StationYaw;
	return true;
}

void APoliceChase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bStationBuilt)
	{
		BuildStation();
	}
	// The guards keep their pose (only drawn near the camera).
	for (int32 i = 0; i < Sentries.Num(); ++i)
	{
		if (APoliceman* Guard = Sentries[i].Get())
		{
			Guard->Stand(SentryAt[i], StationYaw);
		}
	}
	TickBlocks(DeltaSeconds);
}

void APoliceChase::TickBlocks(float DeltaSeconds)
{
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const double Now = GetWorld()->GetTimeSeconds();
	if (!Game || !Player)
	{
		return;
	}
	APestiCar* Car = Cast<APestiCar>(Player);
	const int32 Wanted = Game->GetWantedLevel();

	// A new block ahead while he drives at three stars.
	if (Wanted >= 3 && Car && Blocks.Num() < PoliceChase::MaxBlocks && Now >= NextBlockAt)
	{
		NextBlockAt = Now + (TryPlaceBlock(Player) ? PoliceChase::BlockEvery : 1.0);
	}

	SightClock -= DeltaSeconds;
	const bool bLook = SightClock <= 0.f;
	if (bLook)
	{
		SightClock = PoliceChase::LookEvery;
	}

	const FVector PlayerAt = Player->GetActorLocation();
	PoliceChase::Points.Reset();
	for (int32 b = Blocks.Num() - 1; b >= 0; --b)
	{
		FBlock& Block = Blocks[b];
		const double Away = FVector::Dist2D(PlayerAt, Block.Middle);
		const bool bPassed = FVector::DotProduct(PlayerAt - Block.Middle, Block.Facing) < 0.0 && Away > PoliceChase::ClearPassed;
		// No longer wanted: gone as soon as nobody sees it go.
		bool bInView = false;
		for (const FBarrier& Barrier : Block.Barriers)
		{
			bInView = bInView || (Barrier.Set.IsValid() && Barrier.Set->WasRecentlyRendered(0.5f));
		}
		const bool bCalm = Wanted == 0 && Now - Block.Born > PoliceChase::ClearWhenCalm && (!bInView || Away > PoliceChase::SeenFrom);
		const bool bOld = Now - Block.Born > PoliceChase::MaxAge && Away > PoliceChase::SeenFrom;
		if (Away > PoliceChase::ClearBeyond || bPassed || bCalm || bOld)
		{
			ClearBlock(Block);
			Blocks.RemoveAt(b);
			continue;
		}

		if (Car)
		{
			KnockBarriers(Block, Car);
		}
		FlyBarriers(Block, DeltaSeconds);
		for (const FBarrier& Barrier : Block.Barriers)
		{
			if (!Barrier.bKnocked && Barrier.Set.IsValid())
			{
				PoliceChase::Points.Add(Barrier.Set->GetActorLocation());
			}
		}

		// The constables stand by and watch the street.
		for (int32 i = 0; i < Block.Men.Num(); ++i)
		{
			APoliceman* Man = Block.Men[i].Get();
			if (!Man)
			{
				continue;
			}
			Man->Stand(Block.MenAt[i], Block.Facing.Rotation().Yaw);
			if (!bLook || Wanted == 0)
			{
				continue;
			}
			const double ToPlayer = FVector::Dist2D(Block.MenAt[i], PlayerAt);
			if (ToPlayer < PoliceChase::NearRange)
			{
				Game->NotifyPoliceNear();
			}
			if (ToPlayer < PoliceChase::SeeRange)
			{
				FCollisionQueryParams Params(SCENE_QUERY_STAT(RoadBlockSight), false, Man);
				Params.AddIgnoredActor(Player);
				FHitResult Hit;
				if (!GetWorld()->LineTraceSingleByObjectType(Hit, Block.MenAt[i] + FVector(0.0, 0.0, 70.0), PlayerAt + FVector(0.0, 0.0, 50.0),
					FCollisionObjectQueryParams(ECC_WorldStatic), Params))
				{
					Game->NotifySeenByPolice();
				}
			}
		}
	}
}

bool APoliceChase::TryPlaceBlock(const APawn* Player)
{
	ATrafficManager* Lanes = FindTraffic();
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!Lanes || !PC || !PC->PlayerCameraManager)
	{
		return false;
	}
	const FVector From = Player->GetActorLocation();
	const FVector Velocity = Player->GetVelocity();
	const FVector Heading = Velocity.Size2D() > 300.0 ? Velocity.GetSafeNormal2D() : Player->GetActorForwardVector().GetSafeNormal2D();
	const FVector Camera = PC->PlayerCameraManager->GetCameraLocation();

	for (const double Ahead : PoliceChase::TryAhead)
	{
		int32 Lane = INDEX_NONE;
		int32 Point = 0;
		if (!Lanes->FindLanePointNear(From + Heading * Ahead, Heading, Lane, Point))
		{
			continue;
		}
		const TArray<FVector>& Along = Lanes->GetLane(Lane).Points;
		if (!Along.IsValidIndex(Point) || Along.Num() < 2)
		{
			continue;
		}
		const FVector At = Along[Point];
		const FVector Offset = At - From;
		if (Offset.Size2D() < PoliceChase::MinAhead || FVector::DotProduct(Offset, Heading) <= 0.0)
		{
			continue;
		}
		// Only where he can't see it put up: behind a house, or far off.
		FCollisionQueryParams SightParams(SCENE_QUERY_STAT(RoadBlockHidden), false, this);
		SightParams.AddIgnoredActor(Player);
		FHitResult Hit;
		const bool bOutOfSight = GetWorld()->LineTraceSingleByObjectType(Hit, Camera, At + FVector(0.0, 0.0, 150.0),
			FCollisionObjectQueryParams(ECC_WorldStatic), SightParams);
		if (!bOutOfSight && FVector::Dist2D(Camera, At) < PoliceChase::SeenFrom)
		{
			continue;
		}

		// Along the street, facing the way he comes.
		const int32 Next = Point + 1 < Along.Num() ? Point + 1 : Point - 1;
		FVector Street = (Along[Next] - At).GetSafeNormal2D() * (Next > Point ? 1.0 : -1.0);
		if (FVector::DotProduct(Street, Heading) < 0.0)
		{
			Street = -Street;
		}
		const FVector Across(-Street.Y, Street.X, 0.0);

		// The road between the houses, the pavements left open.
		double Reach[2] = { PoliceChase::LookAcross * 0.4, PoliceChase::LookAcross * 0.4 };
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const FVector Dir = Side == 0 ? -Across : Across;
			FHitResult Wall;
			FCollisionQueryParams WallParams(SCENE_QUERY_STAT(RoadBlockWidth), false, this);
			if (GetWorld()->LineTraceSingleByObjectType(Wall, At + FVector(0.0, 0.0, 200.0), At + FVector(0.0, 0.0, 200.0) + Dir * PoliceChase::LookAcross,
				FCollisionObjectQueryParams(ECC_WorldStatic), WallParams) && Cast<AOsmGraybox>(Wall.GetActor()))
			{
				Reach[Side] = FVector::Dist2D(At, Wall.ImpactPoint);
			}
		}
		const FVector Middle = At + Across * ((Reach[1] - Reach[0]) * 0.5);
		const double Span = FMath::Clamp(Reach[0] + Reach[1] - 2.0 * PoliceChase::KeepPavement, PoliceChase::MinSpan, PoliceChase::MaxSpan);
		const int32 Count = FMath::Max(2, FMath::CeilToInt32(Span / PoliceChase::BarrierEvery));

		FBlock Block;
		Block.Middle = Middle;
		Block.Facing = -Street;
		Block.Born = GetWorld()->GetTimeSeconds();
		const double BarrierYaw = Block.Facing.Rotation().Yaw;
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		TArray<MarketModel::FShape> Shapes;
		PoliceStationModel::BuildBarrier(Shapes);
		TArray<BozsiHouseModel::FSign> Signs;
		PoliceStationModel::BarrierSigns(Signs);
		for (int32 i = 0; i < Count; ++i)
		{
			FVector Spot = Middle + Across * (-Span * 0.5 + (i + 0.5) * Span / Count) + Street * (i % 2 == 0 ? PoliceChase::Stagger : -PoliceChase::Stagger);
			double Z = At.Z;
			if (!GroundAt(Spot, Z))
			{
				Z = At.Z;
			}
			Spot.Z = Z;
			AShapeSet* Set = GetWorld()->SpawnActor<AShapeSet>(AShapeSet::StaticClass(), FTransform(FRotator(0.0, BarrierYaw, 0.0), Spot), Params);
			if (!Set)
			{
				continue;
			}
			Set->SetDrawDistance(PoliceChase::DrawDistance);
			Set->AddShapes(Shapes);
			for (const BozsiHouseModel::FSign& Sign : Signs)
			{
				Set->AddSign(Sign.Text, Sign.At, Sign.Yaw, Sign.Height, Sign.Color);
			}
			if (i % 2 == 0)
			{
				Set->AddLamp(PoliceStationModel::BarrierLampAt(), 250.f);
			}
			FBarrier Barrier;
			Barrier.Set = Set;
			Barrier.GroundZ = Z;
			Block.Barriers.Add(Barrier);
		}

		// A constable at each end, a step behind the barriers.
		for (const double Side : { -1.0, 1.0 })
		{
			FVector Spot = Middle + Across * (Side * (Span * 0.5 + 90.0)) - Block.Facing * 150.0;
			double Z = At.Z;
			GroundAt(Spot, Z);
			Spot.Z = Z + PersonModel::MiddleHeight;
			APoliceman* Man = GetWorld()->SpawnActor<APoliceman>(APoliceman::StaticClass(), Spot, Block.Facing.Rotation(), Params);
			if (!Man)
			{
				continue;
			}
			Man->PutOnUniform(PoliceModel::Outfit(Looks));
			Man->ShowTruncheon(true);
			Man->Stand(Spot, BarrierYaw);
			Block.Men.Add(Man);
			Block.MenAt.Add(Spot);
		}
		Blocks.Add(MoveTemp(Block));
		return true;
	}
	return false;
}

void APoliceChase::KnockBarriers(FBlock& Block, APestiCar* Car)
{
	const FVector CarAt = Car->GetActorLocation();
	const FVector CarForward = Car->GetActorForwardVector().GetSafeNormal2D();
	const FVector CarVelocity = Car->GetVelocity();
	const float Speed = static_cast<float>(CarVelocity.Size2D());
	if (Speed < 30.f)
	{
		return;
	}
	for (FBarrier& Barrier : Block.Barriers)
	{
		AShapeSet* Set = Barrier.Set.Get();
		if (!Set || Barrier.bKnocked)
		{
			continue;
		}
		// The barrier in the car's frame: near its nose or tail, within its width plus the plank.
		const FVector Offset = Set->GetActorLocation() - CarAt;
		const double Along = FMath::Abs(FVector::DotProduct(Offset, CarForward));
		const FVector Plank = Set->GetActorRightVector().GetSafeNormal2D();
		const double AcrossPlank = FMath::Abs(FVector::DotProduct(Offset - CarForward * FVector::DotProduct(Offset, CarForward), Plank));
		if (Along > PoliceChase::TouchAlong || AcrossPlank > PoliceStationModel::BarrierHalf + PoliceChase::TouchAcross)
		{
			continue;
		}
		// Over it goes, flying on with the car or just toppling, and the car loses speed.
		Barrier.bKnocked = true;
		Barrier.Velocity = CarVelocity * 0.9 + FVector(0.0, 0.0, 200.0 + Speed * 0.15);
		Barrier.Spin = FRotator(Looks.FRandRange(-260.f, 260.f), Looks.FRandRange(-180.f, 180.f), Looks.FRandRange(-320.f, 320.f)) * FMath::Min(1.f, Speed / 1500.f);
		Car->ForwardSpeed *= PoliceChase::SpeedKept;
		if (Speed > PoliceChase::DentFrom)
		{
			Car->TakeKnock(Set->GetActorLocation(), -CarVelocity.GetSafeNormal2D() * (Speed * 0.15f));
		}
	}
}

void APoliceChase::FlyBarriers(FBlock& Block, float DeltaSeconds)
{
	for (FBarrier& Barrier : Block.Barriers)
	{
		AShapeSet* Set = Barrier.Set.Get();
		if (!Set || !Barrier.bKnocked || Barrier.bLanded)
		{
			continue;
		}
		Barrier.Velocity.Z -= PoliceChase::Gravity * DeltaSeconds;
		FVector At = Set->GetActorLocation() + Barrier.Velocity * DeltaSeconds;
		FRotator Turn = Set->GetActorRotation() + Barrier.Spin * DeltaSeconds;
		if (At.Z <= Barrier.GroundZ && Barrier.Velocity.Z < 0.0)
		{
			// Down on the road, lying on its side.
			At.Z = Barrier.GroundZ;
			Turn = FRotator(0.0, Turn.Yaw, 90.0);
			Barrier.bLanded = true;
		}
		Set->SetActorLocationAndRotation(At, Turn);
	}
}

void APoliceChase::ClearBlock(FBlock& Block)
{
	for (FBarrier& Barrier : Block.Barriers)
	{
		if (AShapeSet* Set = Barrier.Set.Get())
		{
			Set->Destroy();
		}
	}
	for (const TWeakObjectPtr<APoliceman>& Man : Block.Men)
	{
		if (APoliceman* Constable = Man.Get())
		{
			Constable->Destroy();
		}
	}
	Block.Barriers.Reset();
	Block.Men.Reset();
}
