// Fill out your copyright notice in the Description page of Project Settings.

#include "TrafficBike.h"

#include "BicycleModel.h"
#include "Camera/PlayerCameraManager.h"
#include "CarjackedDriver.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PolicePatrols.h"
#include "TrafficManager.h"
#include "Weather.h"

namespace TrafficBikeRide
{
	// Riders keep this far towards the kerb from the cars' lane, and this far from a wall (cm).
	constexpr double KerbOffset = 140.0;
	constexpr double WallRoom = 60.0;
	// In the rain: the most of the riders gone home at full rain, and how far
	// from the camera (out of its view) one goes or comes back (cm).
	constexpr float HomeInRain = 0.75f;
	constexpr double RainAwayBeyond = 3000.0;
	constexpr double RainBackOutOfView = 0.2;
	// The next point is taken once it is this close (cm).
	constexpr double ReachRadius = 150.0;
	// Each rider's easy pace (cm/s: 14 to 19 km/h; errand boys hurry).
	constexpr float CruiseLow = 380.f;
	constexpr float CruiseHigh = 520.f;
	constexpr float ErrandCruise = 580.f;
	constexpr float SpeedUp = 120.f;
	constexpr float SlowDown = 500.f;
	constexpr float TurnRate = 150.f;
	// The slowest share of the pace in a sharp bend, and before a junction.
	constexpr float CornerShare = 0.45f;
	constexpr float JunctionShare = 0.6f;
	constexpr float JunctionSlowing = 800.f;
	// Looks this far ahead, in a strip this wide each side, and stops this far behind (cm).
	constexpr float LookAhead = 1200.f;
	constexpr float Corridor = 110.f;
	constexpr float StopGap = 260.f;
	// Rings the bell at something in front within this (cm), every few seconds at most.
	constexpr float BellAt = 900.f;
	constexpr float BellEveryLow = 5.f;
	constexpr float BellEveryHigh = 9.f;
	// Held up this long (s), it squeezes past for this long.
	constexpr float MaxStuck = 6.f;
	constexpr float Squeeze = 3.f;
	constexpr float MaxLean = 18.f;
	constexpr float LeanRate = 5.f;
	constexpr float Gravity = 980.f;

	// Something this much faster than the bicycle knocks it over (cm/s, about 14 km/h); slower, it wobbles.
	constexpr float KnockSpeed = 380.f;
	constexpr float NudgeSpeed = 60.f;
	// How close counts as touching, beyond the other's box (cm along, across, up).
	constexpr double TouchAlong = 45.0;
	constexpr double TouchAcross = 35.0;
	constexpr double TouchUp = 120.0;
	constexpr float BumpRest = 1.5f;
	// A wobble lasts this long (s) and sways this far (degrees) this fast (radians a second).
	constexpr float WobbleSeconds = 1.2f;
	constexpr float WobbleRoll = 9.f;
	constexpr float WobbleRate = 16.f;
	// Knocked over: it goes down this far (degrees) this fast, lies this long, and is got up this fast.
	constexpr float FallRoll = 80.f;
	constexpr float FallRate = 260.f;
	constexpr float LieSeconds = 4.5f;
	constexpr float GetUpRate = 60.f;

	// The rider's words are shown when the player is this near (cm), for this long (s), as this note.
	constexpr double HearWords = 2500.0;
	constexpr float WordsSeconds = 3.f;
	constexpr int32 WordsKey = 7;
	// Pulled off: the rider gets up this far to the side (cm); a policeman this near sees it.
	constexpr double RunnerOut = 120.0;
	constexpr double TheftSeen = 3000.0;
	// Pieces are not drawn beyond this (cm); beyond FarAway they are only moved now and then (s).
	constexpr float DrawDistance = 9000.f;
	constexpr double FarAway = 10000.0;
	constexpr float FarStep = 0.25f;
}

ATrafficBike::ATrafficBike()
{
	// Riding in the traffic it does not block: cars pass close by, and bumps are worked out by hand.
	CollisionBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionBox->SetGenerateOverlapEvents(false);
	CameraArm->bDoCollisionTest = false;
	CameraArm->PrimaryComponentTick.bStartWithTickEnabled = false;
}

void ATrafficBike::PickRider(FRandomStream& Random)
{
	using PersonKit::EHat;
	using PersonKit::ETop;
	using BicycleModel::EKind;
	PersonKit::FOutfit Look = PersonModel::RandomOutfit(Random);
	const float Who = Random.FRand();
	const bool bLady = Who < 0.28f;
	for (int32 Try = 0; Try < 20 && Look.bWoman != bLady; ++Try)
	{
		Look = PersonModel::RandomOutfit(Random);
	}
	EKind Chosen = EKind::Gents;
	Cruise = Random.FRandRange(TrafficBikeRide::CruiseLow, TrafficBikeRide::CruiseHigh);
	if (bLady)
	{
		Chosen = EKind::Ladies;
	}
	else if (Who < 0.42f)
	{
		// An errand boy with parcels for the shop's customers.
		Look.Height = Random.FRandRange(152.f, 168.f);
		Look.Build = 0.95f;
		Look.Hat = EHat::FlatCap;
		Look.Top = ETop::Waistcoat;
		Look.bApron = true;
		Look.bTie = false;
		Look.bMoustache = false;
		Look.bBoots = false;
		Chosen = EKind::ShopBike;
		ParcelsCarried = Random.RandRange(1, BicycleModel::MaxParcels);
		Cruise = TrafficBikeRide::ErrandCruise;
	}
	else if (Who < 0.68f)
	{
		// A working man on his way.
		Look.Hat = EHat::FlatCap;
		Look.Top = ETop::Jacket;
		Look.bBoots = Random.FRand() < 0.6f;
		Chosen = Random.FRand() < 0.3f ? EKind::Roadster : EKind::Gents;
	}
	else if (Look.Hat == EHat::FlatCap || Look.Hat == EHat::None)
	{
		// A gentleman keeps his hat on.
		Look.Hat = EHat::Fedora;
	}
	Look.Held = PersonKit::EHeld::None;
	Look.bSeated = false;

	bShopBike = Chosen == EKind::ShopBike;
	SetKind(Chosen);
	SetPaint(BicycleModel::Paint(Random.RandRange(0, BicycleModel::NumPaints - 1)));
	SetRiderLook(Look);
	SetDrawDistance(TrafficBikeRide::DrawDistance);
	bCityBike = true;
}

void ATrafficBike::BeginPlay()
{
	Super::BeginPlay();
	SetParcels(ParcelsCarried);
	ShowRider(true);
	Lean = 0.f;
	SteerAngle = 0.f;
	StandPitch = static_cast<float>(BicycleModel::StandFolded);
	Animate(0.f);
}

void ATrafficBike::StartOnLane(ATrafficManager* InManager, int32 InLaneIndex, int32 InPointIndex)
{
	Manager = InManager;
	LaneIndex = InLaneIndex;
	NextLaneIndex = Manager->PickNextLane(LaneIndex);
	PointIndex = InPointIndex;
	Target = KerbSpot(PointIndex);
	Speed = Cruise * 0.5f;
	BellCooldown = FMath::FRandRange(0.f, TrafficBikeRide::BellEveryHigh);
	RainShy = FMath::FRand();
}

void ATrafficBike::Tick(float DeltaSeconds)
{
	if (!Manager)
	{
		// Taken by the player: an ordinary bicycle from now on.
		Super::Tick(DeltaSeconds);
		return;
	}
	APawn::Tick(DeltaSeconds);
	if (TickRain())
	{
		return;
	}

	BumpCooldown = FMath::Max(0.f, BumpCooldown - DeltaSeconds);
	BellCooldown = FMath::Max(0.f, BellCooldown - DeltaSeconds);
	Recover(DeltaSeconds);
	if (!IsDown())
	{
		FollowLane(DeltaSeconds);
	}
	CheckBumps();

	// Far from the camera the pieces are only moved now and then.
	const APlayerCameraManager* Viewer = UGameplayStatics::GetPlayerCameraManager(this, 0);
	FarClock += DeltaSeconds;
	const bool bFar = Viewer && FVector::Dist(Viewer->GetCameraLocation(), GetActorLocation()) > TrafficBikeRide::FarAway;
	if (!bFar || FarClock >= TrafficBikeRide::FarStep)
	{
		Animate(FarClock);
		FarClock = 0.f;
	}
	UpdateBell(DeltaSeconds);
}

bool ATrafficBike::TickRain()
{
	const AWeather* Sky = AWeather::Get(GetWorld());
	const bool bHome = RainShy < TrafficBikeRide::HomeInRain * (Sky ? Sky->GetRain() : 0.f);
	const APlayerCameraManager* Viewer = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!Viewer)
	{
		return bRainedOff;
	}
	const FVector2D FromCamera = FVector2D(GetActorLocation() - Viewer->GetCameraLocation());
	const bool bFar = FromCamera.Size() > TrafficBikeRide::RainAwayBeyond;
	if (bRainedOff)
	{
		const bool bOutOfView = FVector2D::DotProduct(FromCamera.GetSafeNormal(), FVector2D(Viewer->GetCameraRotation().Vector()).GetSafeNormal())
			< TrafficBikeRide::RainBackOutOfView;
		if (bHome || !bFar || !bOutOfView)
		{
			return true;
		}
		bRainedOff = false;
		SetActorHiddenInGame(false);
		SetActorEnableCollision(true);
		return false;
	}
	if (bHome && bFar && !IsDown() && !WasRecentlyRendered(1.f))
	{
		bRainedOff = true;
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
		return true;
	}
	return false;
}

bool ATrafficBike::IsDown() const
{
	return DownTime > 0.f || (WobbleTime <= 0.f && FMath::Abs(ExtraRoll) > 1.f);
}

FVector ATrafficBike::KerbSpot(int32 Point) const
{
	const FTrafficLane& Lane = Manager->GetLane(LaneIndex);
	const int32 Last = Lane.Points.Num() - 1;
	const int32 At = FMath::Clamp(Point, 0, Last);
	const FVector From = Lane.Points[At > 0 ? At - 1 : 0];
	const FVector To = Lane.Points[At > 0 ? At : FMath::Min(1, Last)];
	const FVector Along = (To - From).GetSafeNormal2D();
	// Traffic keeps left (as in 1932), so the kerb is to the left: X forward, Y right.
	const FVector Left(Along.Y, -Along.X, 0.0);
	FVector Spot = Lane.Points[At];
	const double RoadZ = GetActorLocation().Z - CollisionBox->GetScaledBoxExtent().Z - BicycleModel::BoxBottom;
	Spot.Z = RoadZ + 60.0;
	double Offset = TrafficBikeRide::KerbOffset;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TrafficBikeKerb), false, this);
	if (GetWorld()->LineTraceSingleByObjectType(Hit, Spot, Spot + Left * (Offset + TrafficBikeRide::WallRoom), FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		Offset = FMath::Max(0.0, static_cast<double>(Hit.Distance) - TrafficBikeRide::WallRoom);
	}
	return Spot + Left * Offset;
}

void ATrafficBike::FollowLane(float DeltaSeconds)
{
	using TrafficBikeRide::Corridor;
	const FTrafficLane* Lane = &Manager->GetLane(LaneIndex);
	const FVector Here = GetActorLocation();
	for (int32 Guard = 0; Guard < 4 && FVector::Dist2D(Here, Target) < TrafficBikeRide::ReachRadius; ++Guard)
	{
		if (++PointIndex >= Lane->Points.Num())
		{
			if (NextLaneIndex == INDEX_NONE)
			{
				PointIndex = Lane->Points.Num() - 1;
				break;
			}
			LaneIndex = NextLaneIndex;
			NextLaneIndex = Manager->PickNextLane(LaneIndex);
			Lane = &Manager->GetLane(LaneIndex);
			PointIndex = 0;
		}
		Target = KerbSpot(PointIndex);
	}

	// Steer for the point, leaning into the turn.
	const FVector ToTarget = (Target - Here).GetSafeNormal2D();
	const float FacingYaw = static_cast<float>(GetActorRotation().Yaw);
	const float WantedYaw = static_cast<float>(ToTarget.Rotation().Yaw);
	const float TurnNeeded = FRotator::NormalizeAxis(WantedYaw - FacingYaw);
	const float NewYaw = FMath::FixedTurn(FacingYaw, WantedYaw, TrafficBikeRide::TurnRate * DeltaSeconds);
	const float YawRate = DeltaSeconds > 0.f ? FRotator::NormalizeAxis(NewYaw - FacingYaw) / DeltaSeconds : 0.f;
	SteerAngle = FMath::FInterpTo(SteerAngle, FMath::Clamp(TurnNeeded * 0.5f, -25.f, 25.f), DeltaSeconds, 4.f);
	const float WantLean = FMath::RadiansToDegrees(FMath::Atan2(FMath::DegreesToRadians(YawRate) * Speed, TrafficBikeRide::Gravity));
	Lean = FMath::FInterpTo(Lean, FMath::Clamp(WantLean, -TrafficBikeRide::MaxLean, TrafficBikeRide::MaxLean), DeltaSeconds, TrafficBikeRide::LeanRate);

	// Its own pace, slower in bends and coming up to a junction, and stopping for what is in front.
	float Wanted = Cruise * FMath::Lerp(1.f, TrafficBikeRide::CornerShare, FMath::Clamp(FMath::Abs(TurnNeeded) / 90.f, 0.f, 1.f));
	const float ToJunction = static_cast<float>(FVector::Dist2D(Here, Lane->Points.Last()));
	if (Manager->IsJunction(Lane->ToNode) && ToJunction < TrafficBikeRide::JunctionSlowing)
	{
		Wanted *= FMath::Lerp(TrafficBikeRide::JunctionShare, 1.f, ToJunction / TrafficBikeRide::JunctionSlowing);
	}
	SqueezeTime = FMath::Max(0.f, SqueezeTime - DeltaSeconds);
	if (SqueezeTime <= 0.f)
	{
		const float Clear = Manager->ClearDistanceAhead(this, TrafficBikeRide::LookAhead, false, Corridor);
		Wanted *= FMath::Clamp((Clear - TrafficBikeRide::StopGap) / (TrafficBikeRide::LookAhead - TrafficBikeRide::StopGap), 0.f, 1.f);
		if (Clear < TrafficBikeRide::BellAt && BellCooldown <= 0.f)
		{
			RingBell();
			BellCooldown = FMath::FRandRange(TrafficBikeRide::BellEveryLow, TrafficBikeRide::BellEveryHigh);
		}
		StuckTime = Wanted < 20.f ? StuckTime + DeltaSeconds : 0.f;
		if (StuckTime > TrafficBikeRide::MaxStuck)
		{
			StuckTime = 0.f;
			SqueezeTime = TrafficBikeRide::Squeeze;
		}
	}
	Speed = FMath::FInterpConstantTo(Speed, Wanted, DeltaSeconds, Wanted < Speed ? TrafficBikeRide::SlowDown : TrafficBikeRide::SpeedUp);
	bPedalling = Speed > 20.f && Wanted >= Speed - 20.f;

	SetActorRotation(FRotator(0.f, NewYaw, 0.f));
	SetActorLocation(Here + GetActorForwardVector().GetSafeNormal2D() * Speed * DeltaSeconds, false);
	SnapToGround();
}

void ATrafficBike::CheckBumps()
{
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (BumpCooldown > 0.f || DownTime > 0.f || !Player || Player == this || Player->GetAttachParentActor())
	{
		return;
	}
	// The other's size: its box (a car, a bicycle, the cart) or its capsule (on foot).
	FVector Extent(40.0, 40.0, 90.0);
	if (const UBoxComponent* Box = Cast<UBoxComponent>(Player->GetRootComponent()))
	{
		Extent = Box->GetScaledBoxExtent();
	}
	else if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Player->GetRootComponent()))
	{
		Extent = FVector(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight());
	}
	const FVector Local = Player->GetActorTransform().InverseTransformPositionNoScale(GetActorLocation());
	if (FMath::Abs(Local.X) > Extent.X + TrafficBikeRide::TouchAlong || FMath::Abs(Local.Y) > Extent.Y + TrafficBikeRide::TouchAcross
		|| FMath::Abs(Local.Z) > Extent.Z + TrafficBikeRide::TouchUp)
	{
		return;
	}
	const float Impact = static_cast<float>((Player->GetVelocity() - GetVelocity()).Size2D());
	if (Impact > TrafficBikeRide::KnockSpeed)
	{
		KnockOver(Player->GetActorLocation());
	}
	else if (Impact > TrafficBikeRide::NudgeSpeed || Speed > 100.f)
	{
		Wobble();
	}
	BumpCooldown = TrafficBikeRide::BumpRest;
}

void ATrafficBike::Wobble()
{
	if (WobbleTime > 0.f)
	{
		return;
	}
	WobbleTime = TrafficBikeRide::WobbleSeconds;
	Speed *= 0.5f;
	RingBell();
	CallOut(TEXT("\u201eHopp\u00e1! Vigy\u00e1zzon m\u00e1r!\u201d   (Whoa! Watch it!)"));
}

void ATrafficBike::KnockOver(const FVector& From)
{
	// Over it goes, away from whatever hit it.
	FallSide = FVector::DotProduct(GetActorLocation() - From, GetActorRightVector()) >= 0.0 ? 1.f : -1.f;
	DownTime = TrafficBikeRide::LieSeconds;
	WobbleTime = 0.f;
	bPedalling = false;
	CallOut(FMath::RandBool()
		? TEXT("\u201eAz istenit! Nem l\u00e1t a szem\u00e9t\u0151l?!\u201d   (Damn it! Are you blind?!)")
		: TEXT("\u201eJ\u00e9zusm\u00e1ria! El\u00fct\u00f6tt ez a gazember!\u201d   (Good Lord! The scoundrel ran me down!)"));
}

void ATrafficBike::Recover(float DeltaSeconds)
{
	if (DownTime > 0.f)
	{
		// Lying on its side.
		DownTime -= DeltaSeconds;
		Speed = FMath::FInterpConstantTo(Speed, 0.f, DeltaSeconds, TrafficBikeRide::SlowDown * 2.f);
		ExtraRoll = FMath::FInterpConstantTo(ExtraRoll, FallSide * TrafficBikeRide::FallRoll, DeltaSeconds, TrafficBikeRide::FallRate);
		SetActorLocation(GetActorLocation() + GetActorForwardVector().GetSafeNormal2D() * Speed * DeltaSeconds, false);
		return;
	}
	if (WobbleTime > 0.f)
	{
		WobbleTime -= DeltaSeconds;
		const float Fade = FMath::Max(0.f, WobbleTime / TrafficBikeRide::WobbleSeconds);
		ExtraRoll = WobbleTime > 0.f ? TrafficBikeRide::WobbleRoll * Fade * FMath::Sin(WobbleTime * TrafficBikeRide::WobbleRate) : 0.f;
		return;
	}
	// Got up again, slowly.
	ExtraRoll = FMath::FInterpConstantTo(ExtraRoll, 0.f, DeltaSeconds, TrafficBikeRide::GetUpRate);
	if (IsDown())
	{
		Speed = 0.f;
		bPedalling = false;
	}
}

void ATrafficBike::CallOut(const FString& Text) const
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Player && FVector::Dist(Player->GetActorLocation(), GetActorLocation()) < TrafficBikeRide::HearWords)
	{
		APestiHUD::Show(this, TrafficBikeRide::WordsKey, TrafficBikeRide::WordsSeconds, FColor::Orange, Text);
	}
}

void ATrafficBike::TakeRider(APawn* InRider)
{
	if (Manager)
	{
		// The rider is pulled off (or gets up from where he fell) and runs, shouting.
		const FVector Here = GetActorLocation();
		const FVector Danger = InRider ? InRider->GetActorLocation() : Here;
		const FVector Right = GetActorRightVector();
		const double Side = FVector::DotProduct(Danger - Here, Right) > 0.0 ? -1.0 : 1.0;
		FActorSpawnParameters RunnerParams;
		RunnerParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ACarjackedDriver* Runner = GetWorld()->SpawnActor<ACarjackedDriver>(ACarjackedDriver::StaticClass(),
			Here + Right * (Side * TrafficBikeRide::RunnerOut), GetActorRotation(), RunnerParams))
		{
			Runner->SetOutfit(RiderLook);
			Runner->RunFrom(Danger, true);
		}
		if (APolicePatrols::Witnessed(this, Here, TrafficBikeRide::TheftSeen))
		{
			if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
			{
				Game->ReportCrime(1);
			}
		}

		// Out of the traffic: an ordinary bicycle for the player.
		Manager = nullptr;
		LaneIndex = INDEX_NONE;
		DownTime = 0.f;
		WobbleTime = 0.f;
		ExtraRoll = 0.f;
		Lean = 0.f;
		Speed = 0.f;
		CollisionBox->SetCollisionProfileName(TEXT("Pawn"));
		CameraArm->bDoCollisionTest = true;
		CameraArm->SetComponentTickEnabled(true);
		bOwnRiderLook = false;
		DressRider(PersonModel::PlayerOutfit());
	}
	Super::TakeRider(InRider);
}
