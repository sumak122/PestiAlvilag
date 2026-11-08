// Fill out your copyright notice in the Description page of Project Settings.

#include "RaceRival.h"

#include "CarVisuals.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "PestiPerson.h"

namespace RaceRivalTune
{
	// The car's box rides this high over the road (as the traffic's).
	constexpr double RideHeight = 90.0;
	// Picking up speed and braking (cm/s each second).
	constexpr float Acceleration = 520.f;
	constexpr float Braking = 1400.f;
	// Through a right-angle corner, and slower for a sharper one (cm/s).
	constexpr float CornerSpeed = 650.f;
	constexpr float HairpinSpeed = 380.f;
	// How far ahead it looks for corners (cm).
	constexpr double LookAhead = 5000.0;
	// Holding back when this far ahead of the player, pushing on when this far behind (cm).
	constexpr double FarAhead = 5000.0;
	constexpr double FarBehind = 7000.0;
	constexpr float HoldBack = 0.82f;
	constexpr float PushOn = 1.12f;
	// Stuck on something this long, it drives on through (s); at this crawl at least (cm/s).
	constexpr float MaxBlocked = 1.5f;
	constexpr float Crawl = 300.f;
	// How far ahead on the line it aims its nose (cm).
	constexpr double AimAhead = 350.0;
}

ARaceRival::ARaceRival()
{
	PrimaryActorTick.bCanEverTick = true;

	// Same box as a traffic car (ATrafficCar), so it bumps the same way.
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetBoxExtent(FVector(210.f, 85.f, 70.f));
	CollisionBox->SetCollisionProfileName(TEXT("Pawn"));
	RootComponent = CollisionBox;

	Visuals = CreateDefaultSubobject<UCarVisuals>(TEXT("Visuals"));
}

void ARaceRival::Setup(const TArray<FVector>& InLine, const TArray<double>& InAlong, float InTopSpeed, double InSide, const CarModel::FStyle& InStyle, double StartAlong)
{
	Line = InLine;
	Cumulative = InAlong;
	Total = Cumulative.Num() > 0 ? Cumulative.Last() : 0.0;
	TopSpeed = InTopSpeed;
	Side = InSide;
	Along = StartAlong;
	PlayerAlong = StartAlong;
	Speed = 0.f;
	bGoing = false;

	Visuals->Show(CollisionBox, InStyle);
	Visuals->ShowStreetDriver(FMath::RandRange(0, UCarVisuals::NumDriverLooks - 1));
	Visuals->SetStreetEngine(true);
	Visuals->SetHeadlights(true);

	FVector Dir = FVector::ForwardVector;
	FVector At = PointAt(Line, Cumulative, Along, &Dir);
	At += FVector(-Dir.Y, Dir.X, 0.0) * Side;
	At.Z = GroundAt(At) + RaceRivalTune::RideHeight;
	SetActorLocationAndRotation(At, FRotator(0.0, Dir.Rotation().Yaw, 0.0));
}

FVector ARaceRival::PointAt(const TArray<FVector>& Points, const TArray<double>& Lengths, double S, FVector* OutDir)
{
	if (Points.Num() == 0 || Lengths.Num() != Points.Num())
	{
		return FVector::ZeroVector;
	}
	if (Points.Num() == 1)
	{
		return Points[0];
	}
	const int32 Last = Points.Num() - 1;
	int32 Seg = 0;
	while (Seg < Last - 1 && Lengths[Seg + 1] < S)
	{
		++Seg;
	}
	const double SegLength = FMath::Max(Lengths[Seg + 1] - Lengths[Seg], 1.0);
	const double F = FMath::Clamp((S - Lengths[Seg]) / SegLength, 0.0, 1.0);
	const FVector A = Points[Seg];
	const FVector B = Points[Seg + 1];
	if (OutDir)
	{
		*OutDir = (B - A).GetSafeNormal2D();
		if (OutDir->IsNearlyZero())
		{
			*OutDir = FVector::ForwardVector;
		}
	}
	return A + (B - A) * F;
}

double ARaceRival::GroundAt(const FVector& At) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RaceRivalGround), false, this);
	if (GetWorld()->LineTraceSingleByObjectType(Hit, FVector(At.X, At.Y, At.Z + 400.0), FVector(At.X, At.Y, At.Z - 1500.0),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		return Hit.ImpactPoint.Z;
	}
	return At.Z - RaceRivalTune::RideHeight;
}

float ARaceRival::CornerLimit() const
{
	float Limit = TopSpeed;
	const int32 Count = Line.Num();
	// Every bend of the line coming up within sight: slow enough now to make it.
	for (int32 i = 1; i + 1 < Count; ++i)
	{
		const double Ahead = Cumulative[i] - Along;
		if (Ahead < -50.0)
		{
			continue;
		}
		if (Ahead > RaceRivalTune::LookAhead)
		{
			break;
		}
		const FVector In = (Line[i] - Line[i - 1]).GetSafeNormal2D();
		const FVector Out = (Line[i + 1] - Line[i]).GetSafeNormal2D();
		const double Cos = FMath::Clamp(FVector::DotProduct(In, Out), -1.0, 1.0);
		const float Turn = static_cast<float>(FMath::RadiansToDegrees(FMath::Acos(Cos)));
		if (Turn < 12.f)
		{
			continue;
		}
		const float Bend = Turn <= 90.f
			? FMath::Lerp(TopSpeed, RaceRivalTune::CornerSpeed, (Turn - 12.f) / 78.f)
			: FMath::Lerp(RaceRivalTune::CornerSpeed, RaceRivalTune::HairpinSpeed, FMath::Clamp((Turn - 90.f) / 90.f, 0.f, 1.f));
		const float Room = static_cast<float>(FMath::Max(Ahead, 0.0));
		Limit = FMath::Min(Limit, FMath::Sqrt(FMath::Square(Bend) + 2.f * RaceRivalTune::Braking * Room));
	}
	return Limit;
}

void ARaceRival::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bGoing || Line.Num() < 2 || HasFinished())
	{
		Speed = 0.f;
		return;
	}

	// How fast to go: the corners ahead, and where the player is.
	float Wanted = CornerLimit();
	if (Along - PlayerAlong > RaceRivalTune::FarAhead)
	{
		Wanted *= RaceRivalTune::HoldBack;
	}
	else if (PlayerAlong - Along > RaceRivalTune::FarBehind)
	{
		Wanted = FMath::Min(Wanted * RaceRivalTune::PushOn, TopSpeed * RaceRivalTune::PushOn);
	}
	const float Rate = Wanted < Speed ? RaceRivalTune::Braking : RaceRivalTune::Acceleration;
	Speed = FMath::FInterpConstantTo(Speed, Wanted, DeltaSeconds, Rate);

	// The next spot on the line, a little to its side, nose aimed a bit further on.
	const double Step = static_cast<double>(Speed) * DeltaSeconds;
	const double NextAlong = FMath::Min(Along + Step, Total);
	FVector Dir = FVector::ForwardVector;
	FVector Next = PointAt(Line, Cumulative, NextAlong, &Dir);
	FVector AimDir = FVector::ForwardVector;
	const FVector Aim = PointAt(Line, Cumulative, FMath::Min(NextAlong + RaceRivalTune::AimAhead, Total), &AimDir);
	Next += FVector(-Dir.Y, Dir.X, 0.0) * Side;
	const FVector Location = GetActorLocation();
	Next.Z = GroundAt(FVector(Next.X, Next.Y, Location.Z)) + RaceRivalTune::RideHeight;
	const FVector Facing = (Aim + FVector(-AimDir.Y, AimDir.X, 0.0) * Side - Next).GetSafeNormal2D();
	const double AimYaw = Facing.IsNearlyZero() ? GetActorRotation().Yaw : Facing.Rotation().Yaw;
	const float NewYaw = FMath::FixedTurn(static_cast<float>(GetActorRotation().Yaw), static_cast<float>(AimYaw), 220.f * DeltaSeconds);
	SetActorRotation(FRotator(0.0, NewYaw, 0.0));

	FHitResult Hit;
	SetActorLocation(Next, true, &Hit);
	if (!Hit.bBlockingHit)
	{
		Along = NextAlong;
		BlockedTime = 0.f;
		return;
	}

	// Bumped into something: it loses its speed.
	BlockedTime += DeltaSeconds;
	Speed *= 0.5f;
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const AActor* Blocker = Hit.GetActor();
	const bool bPlayer = Blocker && Blocker == PlayerPawn;
	if (bPlayer)
	{
		// Running into him on foot hurts him; his car it just shoves.
		if (APestiPerson* Person = Cast<APestiPerson>(PlayerPawn))
		{
			Person->HitByVehicle(Speed * 2.f, GetActorForwardVector());
		}
		return;
	}
	if (BlockedTime > RaceRivalTune::MaxBlocked)
	{
		// Wedged on a wall, a cart or some car: on through it, at a crawl at least.
		Speed = FMath::Max(Speed, RaceRivalTune::Crawl);
		SetActorLocation(Next, false);
		Along = NextAlong;
	}
}
