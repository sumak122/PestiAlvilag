// Fill out your copyright notice in the Description page of Project Settings.

#include "PoliceCar.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PestiAlvilagGameModeBase.h"
#include "PersonModel.h"
#include "PestiCar.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "PoliceModel.h"
#include "PolicePatrols.h"
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

	// Pulling up to get out: within this of the player on foot, or of his car
	// going slower than this (cm, cm/s); out once down to this speed.
	constexpr float StopNearWalker = 2200.f;
	constexpr float StopNearCar = 1500.f;
	constexpr float StopCarSlowerThan = 250.f;
	constexpr float GetOutBelow = 60.f;
	// The constables: where the doors are (car's own cm), how fast they run (cm/s), how close lays hands on him.
	constexpr double DoorBack = -20.0;
	constexpr double DoorOut = 135.0;
	constexpr float OfficerRun = 560.f;
	constexpr double HandsOnWalker = 200.0;
	constexpr double HandsOnCar = 330.0;
	constexpr double BackIn = 70.0;
	// He got away: this far from the car on foot, or driving off this fast (cm, cm/s).
	constexpr double GotAwayFar = 4500.0;
	constexpr float DrivesOffFaster = 900.f;
	// After they got back in, no pulling up again for this long (s).
	constexpr double StopAgainAfter = 5.0;
	// The player's car knocks one down this close this fast (cm, cm/s), for this long (s).
	constexpr double CarHitRadius = 170.0;
	constexpr float CarHitSpeed = 300.f;
	constexpr float DownSeconds = 4.f;
	constexpr double LyingHeight = 18.0;
	constexpr double ShoutEvery = 4.0;

	struct FLine2 { const TCHAR* Hu; const TCHAR* En; };
	const FLine2 OutLines[] = {
		{ TEXT("\u00c1llj! Rend\u0151rs\u00e9g! Fel a kezekkel!"), TEXT("Stop! Police! Hands up!") },
		{ TEXT("Meg\u00e1llni! \u00d6n le van tart\u00f3ztatva!"), TEXT("Stand still! You are under arrest!") },
		{ TEXT("Fogd meg, Feri, ott van!"), TEXT("Grab him, Feri, there he is!") },
	};
	const FLine2 CarLine = { TEXT("Sz\u00e1lljon ki a kocsib\u00f3l! Rend\u0151rs\u00e9g!"), TEXT("Get out of the car! Police!") };
	const FLine2 DownLine = { TEXT("Jaj! Elg\u00e1zolt egy rend\u0151rt, az \u00e1tkozott!"), TEXT("Ow! The devil ran down a policeman!") };

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

	// The car was taken away (moved on by the traffic): its constables with it.
	if (bOfficersOut && FVector::Dist2D(GetActorLocation(), ParkedAt) > 500.0)
	{
		ClearOfficers();
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
		// Not wanted any more: the constables get back in first.
		if (bOfficersOut)
		{
			AActor::Tick(DeltaSeconds);
			if (SignPaint)
			{
				SignPaint->SetVectorParameterValue(TEXT("Color"), PoliceCar::SignWhite);
			}
			if (TickOfficers(Player, Game, DeltaSeconds, true))
			{
				ClearOfficers();
			}
			return;
		}
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

	// Pulled up with the constables out: they go for him, or back in if he got away.
	if (bOfficersOut)
	{
		CurrentSpeed = 0.f;
		const APestiCar* Car = Cast<APestiCar>(Player);
		const bool bGotAway = DistToPlayer > PoliceCar::GotAwayFar
			|| (Car && FMath::Abs(Car->ForwardSpeed) > PoliceCar::DrivesOffFaster && DistToPlayer > PoliceCar::StopNearCar);
		if (TickOfficers(Player, Game, DeltaSeconds, bGotAway))
		{
			ClearOfficers();
			NoStopUntil = GetWorld()->GetTimeSeconds() + PoliceCar::StopAgainAfter;
			ReplanTimer = 0.f;
		}
		return;
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
	// Near enough to him: pull up, and out they get.
	const bool bStopping = WantsToStop(Player, DistToPlayer);
	if (bStopping)
	{
		WantedSpeed = 0.f;
	}
	const float Rate = WantedSpeed < CurrentSpeed ? BrakeDeceleration * (bStopping ? 1.6f : 1.f) : Acceleration * 2.f;
	CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, WantedSpeed, DeltaSeconds, Rate);
	if (bStopping && CurrentSpeed < PoliceCar::GetOutBelow)
	{
		CurrentSpeed = 0.f;
		GetOut();
		return;
	}

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

void APoliceCar::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearOfficers();
	Super::EndPlay(EndPlayReason);
}

bool APoliceCar::WantsToStop(const APawn* Player, float DistToPlayer) const
{
	if (!bSeesPlayer || GetWorld()->GetTimeSeconds() < NoStopUntil)
	{
		return false;
	}
	if (const APestiCar* Car = Cast<APestiCar>(Player))
	{
		return DistToPlayer < PoliceCar::StopNearCar && FMath::Abs(Car->ForwardSpeed) < PoliceCar::StopCarSlowerThan;
	}
	const APestiPerson* Person = Cast<APestiPerson>(Player);
	return Person && !Person->IsRiding() && DistToPlayer < PoliceCar::StopNearWalker;
}

FVector2D APoliceCar::DoorSpot(int32 Side) const
{
	const double Out = Side == 0 ? -PoliceCar::DoorOut : PoliceCar::DoorOut;
	return FVector2D(GetActorTransform().TransformPosition(FVector(PoliceCar::DoorBack, Out, 0.0)));
}

bool APoliceCar::FindGround(const FVector2D& At, double Near, double& OutZ) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PoliceCarGround), false, this);
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByObjectType(Hit, FVector(At, Near + 150.0), FVector(At, Near - 400.0),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		OutZ = Hit.ImpactPoint.Z;
		return true;
	}
	return false;
}

void APoliceCar::GetOut()
{
	ClearOfficers();
	bOfficersOut = true;
	ParkedAt = GetActorLocation();
	OfficerLooks.Initialize(static_cast<int32>(GetUniqueID()) + static_cast<int32>(GetWorld()->GetTimeSeconds() * 10.0));
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const double CarGround = GetActorLocation().Z - 60.0;
	for (int32 Side = 0; Side < 2; ++Side)
	{
		FOfficer Officer;
		Officer.At = DoorSpot(Side);
		Officer.GroundZ = CarGround;
		double Z = 0.0;
		if (FindGround(Officer.At, CarGround + 60.0, Z))
		{
			Officer.GroundZ = Z;
		}
		Officer.Yaw = static_cast<float>(GetActorRotation().Yaw) + (Side == 0 ? -90.f : 90.f);
		APoliceman* Man = GetWorld()->SpawnActor<APoliceman>(APoliceman::StaticClass(),
			FVector(Officer.At, Officer.GroundZ + PersonModel::MiddleHeight), FRotator(0.0, Officer.Yaw, 0.0), Params);
		if (!Man)
		{
			continue;
		}
		Man->PutOnUniform(PoliceModel::Outfit(OfficerLooks));
		Man->ShowTruncheon(true);
		Officer.Man = Man;
		Officers.Add(Officer);
	}

	APestiHUD* Hud = APestiHUD::Get(this);
	const double Now = GetWorld()->GetTimeSeconds();
	if (Hud && !Hud->IsSaying() && Now - LastOfficerShout > PoliceCar::ShoutEvery)
	{
		LastOfficerShout = Now;
		const bool bInCar = Cast<APestiCar>(UGameplayStatics::GetPlayerPawn(this, 0)) != nullptr;
		const PoliceCar::FLine2& Line = bInCar ? PoliceCar::CarLine
			: PoliceCar::OutLines[OfficerLooks.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(PoliceCar::OutLines)) - 1)];
		Hud->Say(TEXT("Rend\u0151r"), Line.Hu, Line.En, 3.f);
	}
}

void APoliceCar::ClearOfficers()
{
	for (FOfficer& Officer : Officers)
	{
		if (APoliceman* Man = Officer.Man.Get())
		{
			Man->Destroy();
		}
	}
	Officers.Reset();
	bOfficersOut = false;
}

bool APoliceCar::TickOfficers(const APawn* Player, APestiAlvilagGameModeBase* Game, float DeltaSeconds, bool bBack)
{
	const FVector PlayerAt = Player ? Player->GetActorLocation() : GetActorLocation();
	const APestiCar* PlayerCar = Cast<APestiCar>(Player);
	const double HandsOn = PlayerCar ? PoliceCar::HandsOnCar : PoliceCar::HandsOnWalker;
	bool bAllIn = true;
	for (int32 i = 0; i < Officers.Num(); ++i)
	{
		FOfficer& Officer = Officers[i];
		APoliceman* Man = Officer.Man.Get();
		if (!Man || Officer.bIn)
		{
			continue;
		}
		bAllIn = false;

		// Knocked down by his car: on his back a while, then up and on.
		if (Officer.DownLeft > 0.f)
		{
			Officer.DownLeft -= DeltaSeconds;
			Man->Hold(FVector(Officer.At, Officer.GroundZ + PoliceCar::LyingHeight), FRotator(90.0, Officer.Yaw, 0.0), 10.f, -20.f, 20.f);
			continue;
		}
		if (PlayerCar && FMath::Abs(PlayerCar->ForwardSpeed) > PoliceCar::CarHitSpeed
			&& FVector2D::Distance(Officer.At, FVector2D(PlayerAt)) < PoliceCar::CarHitRadius)
		{
			Officer.DownLeft = PoliceCar::DownSeconds;
			if (Game)
			{
				Game->ReportCrime(2);
			}
			if (APestiHUD* Hud = APestiHUD::Get(this))
			{
				Hud->Say(TEXT("Rend\u0151r"), PoliceCar::DownLine.Hu, PoliceCar::DownLine.En, 3.f);
			}
			continue;
		}

		// Running for him, or back to his door.
		const FVector2D Goal = bBack ? DoorSpot(i % 2) : FVector2D(PlayerAt);
		const FVector2D To = Goal - Officer.At;
		const double Gap = To.Size();
		const double Stop = bBack ? PoliceCar::BackIn : HandsOn * 0.7;
		if (!bBack && Gap < HandsOn && Game)
		{
			Game->NotifyPoliceNear();
		}
		if (Gap <= Stop)
		{
			if (bBack)
			{
				// In he gets.
				Officer.bIn = true;
				Man->Destroy();
				continue;
			}
			// Got him: holds him there, facing him.
			Officer.Yaw = FMath::FixedTurn(Officer.Yaw, static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(To.Y, To.X))), 400.f * DeltaSeconds);
			Man->Stand(FVector(Officer.At, Officer.GroundZ + PersonModel::MiddleHeight), Officer.Yaw);
			continue;
		}
		const FVector2D Way = To / Gap;
		FVector2D Step = Way * FMath::Min(Gap - Stop * 0.9, static_cast<double>(PoliceCar::OfficerRun * DeltaSeconds));
		// Round walls rather than through them.
		const FVector Chest(Officer.At, Officer.GroundZ + 100.0);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(PoliceOfficerStep), false, this);
		Params.AddIgnoredActor(Man);
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByObjectType(Hit, Chest, Chest + FVector(Step * 4.0, 0.0), FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			const FVector2D Normal = FVector2D(Hit.ImpactNormal).GetSafeNormal();
			Step -= Normal * FVector2D::DotProduct(Step, Normal);
		}
		Officer.At += Step;
		double Z = 0.0;
		if (FindGround(Officer.At, Officer.GroundZ, Z) && FMath::Abs(Z - Officer.GroundZ) < 60.0)
		{
			Officer.GroundZ = Z;
		}
		Officer.Yaw = FMath::FixedTurn(Officer.Yaw, static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Way.Y, Way.X))), 500.f * DeltaSeconds);
		Man->RunTo(FVector(Officer.At, Officer.GroundZ + PersonModel::MiddleHeight), Officer.Yaw, PoliceCar::OfficerRun, DeltaSeconds);
	}
	return bAllIn;
}
