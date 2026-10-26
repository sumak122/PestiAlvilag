// Fill out your copyright notice in the Description page of Project Settings.

#include "CarjackedDriver.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
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
}

ACarjackedDriver::ACarjackedDriver()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ACarjackedDriver::RunFrom(const FVector& Danger)
{
	const FVector2D Away = FVector2D(GetActorLocation() - Danger);
	Heading = Away.IsNearlyZero() ? FVector2D(GetActorForwardVector()) : Away.GetSafeNormal();
	if (GEngine)
	{
		APestiHUD::Show(this, CarjackedDriver::ShoutKey, CarjackedDriver::ShoutSeconds, FColor::Orange,
			TEXT("\u201eH\u00e9! Az aut\u00f3m! Rend\u0151r!\u201d   (Hey! My car! Police!)"));
	}
}

void ACarjackedDriver::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Clock += DeltaSeconds;
	if (Clock > CarjackedDriver::GoneAfter)
	{
		Destroy();
		return;
	}
	const float Speed = Clock < CarjackedDriver::RunFor ? CarjackedDriver::RunSpeed : CarjackedDriver::WalkSpeed;
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
