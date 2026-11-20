// Fill out your copyright notice in the Description page of Project Settings.

#include "FireStation.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "DailyLifeModel.h"
#include "DayNight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FireStationModel.h"
#include "FireTruck.h"
#include "Kismet/GameplayStatics.h"
#include "OsmGraybox.h"
#include "Pedestrians.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "ShapeSet.h"

namespace FireStationRun
{
	constexpr float DrawDistance = 22000.f;
	// The firemen are posed only while the camera is this near (cm).
	constexpr double PoseRange = 9000.0;
	// A new engine is put out only with the player at least this far off (cm).
	constexpr double SpawnAway = 7000.0;
	// The engine counts as gone once it is this far from its spot (cm), or driven.
	constexpr double GoneFrom = 450.0;
	// An engine left standing this near its spot is back at the station (cm).
	constexpr double BackAt = 900.0;
	// The firemen run after it this long (s) at this speed (cm/s), no nearer than CatchUp (cm); then shake their fists this long.
	constexpr float ChaseTime = 4.f;
	constexpr float RunSpeed = 520.f;
	constexpr double CatchUp = 500.0;
	constexpr float ShakeTime = 4.f;
	constexpr float WalkSpeed = 150.f;
	// The morning hour a new engine is ready.
	constexpr float MorningHour = 6.f;
}

AFireStation::AFireStation()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AFireStation::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		for (APedestrian* Man : Firemen)
		{
			if (IsValid(Man))
			{
				Man->Destroy();
			}
		}
		if (IsValid(Front))
		{
			Front->Destroy();
		}
	}
	Firemen.Reset();
	Super::EndPlay(EndPlayReason);
}

bool AFireStation::GroundAt(const FVector2D& Near, double& OutZ) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FireStationGround), false, this);
	TArray<FHitResult> Hits;
	GetWorld()->LineTraceMultiByObjectType(Hits, FVector(Near, 20000.0), FVector(Near, -20000.0),
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

bool AFireStation::Build()
{
	const double FacingRad = FMath::DegreesToRadians(FireStationModel::CityFacing);
	const FVector2D Out(FMath::Cos(FacingRad), FMath::Sin(FacingRad));
	if (!GroundAt(FireStationModel::CityAt + Out * 200.0, Ground))
	{
		// The city is not there yet: try again next frame.
		return false;
	}
	Frame = FTransform(FRotator(0.0, FireStationModel::CityFacing, 0.0), FVector(FireStationModel::CityAt, Ground));

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Front = GetWorld()->SpawnActor<AShapeSet>(AShapeSet::StaticClass(), Frame, Params);
	if (Front)
	{
		Front->SetDrawDistance(FireStationRun::DrawDistance);
		TArray<MarketModel::FShape> Shapes;
		FireStationModel::BuildFront(Shapes);
		Front->AddShapes(Shapes);
		TArray<BozsiHouseModel::FSign> Signs;
		FireStationModel::Signs(Signs);
		for (const BozsiHouseModel::FSign& Sign : Signs)
		{
			Front->AddSign(Sign.Text, Sign.At, Sign.Yaw, Sign.Height, Sign.Color);
		}
		TArray<BozsiHouseModel::FLamp> Lamps;
		FireStationModel::Lamps(Lamps);
		for (const BozsiHouseModel::FLamp& Lamp : Lamps)
		{
			Front->AddLamp(Lamp.At, Lamp.Radius);
		}
	}

	// Two firemen on duty in their brass helmets.
	FRandomStream Dice(1932);
	for (int32 i = 0; i < 2; ++i)
	{
		const FVector Spot = i == 0 ? FireStationModel::PolisherAt : FireStationModel::SmokerAt;
		const FVector At = Frame.TransformPosition(Spot) + FVector(0.0, 0.0, PersonModel::MiddleHeight);
		APedestrian* Man = GetWorld()->SpawnActor<APedestrian>(APedestrian::StaticClass(), At, Frame.Rotator(), Params);
		if (!Man)
		{
			continue;
		}
		const PersonKit::FOutfit Look = DailyLifeModel::FiremanLook(Dice);
		Man->SetOutfit(Look);
		PersonKit::FPiece Helmet;
		DailyLifeModel::FireHelmet(Helmet);
		Man->AddProp(Helmet, PersonKit::Body, DailyLifeModel::HeadMiddle(Look), FRotator::ZeroRotator);
		Firemen.Add(Man);
	}
	bBuilt = true;
	return true;
}

void AFireStation::SpawnEngine()
{
	const FVector At = Frame.TransformPosition(FireStationModel::ParkAt) + FVector(0.0, 0.0, 120.0);
	const FRotator Facing(0.0, Frame.Rotator().Yaw + FireStationModel::ParkYaw, 0.0);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Parked = GetWorld()->SpawnActor<AFireTruck>(AFireTruck::StaticClass(), At, Facing, Params);
}

void AFireStation::Stolen()
{
	Thief = Parked.Get();
	Parked = nullptr;
	bWaitForMorning = true;
	Duty = EFireDuty::Chasing;
	DutyTime = 0.f;
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->Say(TEXT("T\u0171zolt\u00f3"), TEXT("\u00c1llj! A fecskend\u0151! Riassz\u00e1tok a rend\u0151rs\u00e9get!"), TEXT("Stop! The engine! Call the police!"), 3.5f);
	}
	if (APestiAlvilagGameModeBase* Mode = APestiAlvilagGameModeBase::Get(this))
	{
		Mode->ReportCrime(2);
	}
}

void AFireStation::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Clock += DeltaSeconds;
	if (!bBuilt && !Build())
	{
		return;
	}

	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const FVector Seen = Camera ? Camera->GetCameraLocation() : FVector::ZeroVector;
	const FVector ParkSpot = Frame.TransformPosition(FireStationModel::ParkAt);
	const double CameraAway = Camera ? FVector::Dist2D(Seen, ParkSpot) : 0.0;

	// The waiting engine driven off, or shoved off its spot: stolen.
	if (Parked && (!IsValid(Parked) || Parked->GetController() || FVector::Dist2D(Parked->GetActorLocation(), ParkSpot) > FireStationRun::GoneFrom))
	{
		if (IsValid(Parked))
		{
			Stolen();
		}
		else
		{
			Parked = nullptr;
		}
	}

	// The morning after a theft a new engine is ready.
	if (bWaitForMorning)
	{
		const ADayNight* Sky = ADayNight::Get(GetWorld());
		const float Hour = Sky ? Sky->GetHour() : 0.f;
		if (LastHour >= 0.f && LastHour < FireStationRun::MorningHour && Hour >= FireStationRun::MorningHour)
		{
			bWaitForMorning = false;
		}
		LastHour = Hour;
	}
	if (!Parked && !bWaitForMorning && Camera && CameraAway > FireStationRun::SpawnAway)
	{
		// An engine brought back and left on its spot is waiting there again.
		AFireTruck* Returned = nullptr;
		for (TActorIterator<AFireTruck> It(GetWorld()); It; ++It)
		{
			if (!It->GetController() && FVector::Dist2D(It->GetActorLocation(), ParkSpot) < FireStationRun::BackAt)
			{
				Returned = *It;
				break;
			}
		}
		if (Returned)
		{
			Parked = Returned;
		}
		else
		{
			SpawnEngine();
		}
	}

	TickFiremen(DeltaSeconds, CameraAway < FireStationRun::PoseRange);
}

void AFireStation::TickFiremen(float DeltaSeconds, bool bNear)
{
	using namespace DailyLifeModel;
	DutyTime += DeltaSeconds;
	const double Up = PersonModel::MiddleHeight;
	const AFireTruck* Gone = Thief.Get();
	for (int32 i = 0; i < Firemen.Num(); ++i)
	{
		APedestrian* Man = Firemen[i];
		if (!IsValid(Man))
		{
			continue;
		}
		const FVector Spot = i == 0 ? FireStationModel::PolisherAt : FireStationModel::SmokerAt;
		const double SpotYaw = i == 0 ? FireStationModel::PolisherYaw : FireStationModel::SmokerYaw;
		const FVector Home = Frame.TransformPosition(Spot) + FVector(0.0, 0.0, Up);
		const FVector Now = Man->GetActorLocation();
		if (Duty == EFireDuty::Chasing)
		{
			// After the engine, along the pavement.
			const FVector Target = Gone ? Gone->GetActorLocation() : Now;
			FVector Way = Target - Now;
			Way.Z = 0.0;
			if (Way.Size() > FireStationRun::CatchUp)
			{
				const FVector Dir = Way.GetSafeNormal();
				const FVector Next(Now.X + Dir.X * FireStationRun::RunSpeed * DeltaSeconds, Now.Y + Dir.Y * FireStationRun::RunSpeed * DeltaSeconds, Ground + Up);
				Man->WalkTo(Next, Dir.Rotation().Yaw, FireStationRun::RunSpeed, DeltaSeconds);
			}
			else
			{
				Man->ShakeFist(Now, Way.Rotation().Yaw, Clock + i * 0.4f);
			}
			continue;
		}
		if (Duty == EFireDuty::Shaking)
		{
			const FVector Target = Gone ? Gone->GetActorLocation() : Now + Man->GetActorForwardVector() * 100.0;
			Man->ShakeFist(Now, (Target - Now).Rotation().Yaw, Clock + i * 0.4f);
			continue;
		}
		if (Duty == EFireDuty::Back)
		{
			FVector Way = Home - Now;
			Way.Z = 0.0;
			if (Way.Size() > 30.0)
			{
				const FVector Dir = Way.GetSafeNormal();
				const double Step = FMath::Min(static_cast<double>(FireStationRun::WalkSpeed * DeltaSeconds), Way.Size());
				Man->WalkTo(FVector(Now.X + Dir.X * Step, Now.Y + Dir.Y * Step, Ground + Up), Dir.Rotation().Yaw, FireStationRun::WalkSpeed, DeltaSeconds);
				continue;
			}
		}
		// On duty: posed only when someone may see it.
		if (!bNear)
		{
			continue;
		}
		FLimbs Pose;
		if (i == 0 && Parked)
		{
			// Polishing the radiator's brass, the cloth going round.
			Pose.Arms[1] = FRotator(78.f + 8.f * FMath::Sin(Clock * 3.1f), 0.f, -10.f + 6.f * FMath::Cos(Clock * 3.1f));
			Pose.Elbows[1] = 50.f;
			Pose.Arms[0] = FRotator(10.f, 0.f, 5.f);
			Pose.Elbows[0] = 20.f;
		}
		else if (i == 1)
		{
			// Having a smoke, the other hand on his belt.
			const float Puff = FMath::Max(0.f, FMath::Sin(Clock * 0.5f));
			Pose.Arms[1] = FRotator(40.f * Puff, 0.f, -10.f);
			Pose.Elbows[1] = 30.f + 100.f * Puff;
			Pose.Arms[0] = FRotator(16.f, 0.f, 4.f);
			Pose.Elbows[0] = 100.f;
		}
		Man->Strike(Home, Frame.Rotator().Yaw + SpotYaw, Pose);
	}

	// Chasing, then shaking their fists, then back to their spots.
	if (Duty == EFireDuty::Chasing && DutyTime > FireStationRun::ChaseTime)
	{
		Duty = EFireDuty::Shaking;
		DutyTime = 0.f;
	}
	else if (Duty == EFireDuty::Shaking && DutyTime > FireStationRun::ShakeTime)
	{
		Duty = EFireDuty::Back;
		DutyTime = 0.f;
	}
	else if (Duty == EFireDuty::Back && DutyTime > 30.f)
	{
		Duty = EFireDuty::OnDuty;
	}
}
