// Fill out your copyright notice in the Description page of Project Settings.

#include "WalkingPerson.h"

#include "PersonModel.h"
#include "Engine/World.h"

namespace WalkingPerson
{
	// The actor's location is the person's middle, this far above the ground (cm).
	constexpr double Middle = 90.0;
	// Close enough to a point to go on to the next (cm).
	constexpr double Reached = 15.0;
	constexpr float GroundEvery = 0.2f;
	// Whatever happens, nobody walks about for longer than this (s).
	constexpr float GoneAfter = 60.f;
	// Turning towards where they walk (degrees a second).
	constexpr float TurnRate = 360.f;
	// The ground is looked for from this far above the middle down (cm): below awnings and shelter roofs.
	constexpr double LookFrom = 50.0;
}

AWalkingPerson::AWalkingPerson()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AWalkingPerson::SetPath(const TArray<FVector2D>& InPath, float InSpeed, bool bInVanish)
{
	Path = InPath;
	Next = 0;
	Speed = InSpeed;
	bVanish = bInVanish;
	Clock = 0.f;
	FacingYaw = static_cast<float>(GetActorRotation().Yaw);
}

void AWalkingPerson::SetGoal(const FVector2D& Goal)
{
	if (Path.Num() > 0)
	{
		Path.Last() = Goal;
	}
}

void AWalkingPerson::StandFacing(double Yaw)
{
	Path.Reset();
	Next = 0;
	FacingYaw = static_cast<float>(Yaw);
	SetActorRotation(FRotator(0.0, Yaw, 0.0));
	Rest();
}

void AWalkingPerson::Rest()
{
	PersonModel::PoseLegs(Pieces, 0.f, 0.f);
	for (const int32 Arm : { PersonKit::ArmLeft, PersonKit::ArmRight })
	{
		if (USceneComponent* Part = Limb(Arm))
		{
			Part->SetRelativeRotation(FRotator::ZeroRotator);
		}
	}
}

void AWalkingPerson::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Clock += DeltaSeconds;
	if (Next >= Path.Num())
	{
		// Standing: settle onto the ground once, wherever they were put.
		if (!bGroundKnown)
		{
			const FVector Here = GetActorLocation();
			UpdateGround(FVector2D(Here), Here.Z);
			if (bGroundKnown)
			{
				SetActorLocation(FVector(Here.X, Here.Y, GroundZ + WalkingPerson::Middle * GetActorScale3D().Z));
			}
		}
		if (Clock > WalkingPerson::GoneAfter && bVanish)
		{
			Destroy();
		}
		return;
	}

	const FVector Here = GetActorLocation();
	FVector2D At(Here);
	double Step = Speed * DeltaSeconds;
	while (Next < Path.Num())
	{
		const FVector2D ToNext = Path[Next] - At;
		const double Left = ToNext.Size();
		if (Left > Step + WalkingPerson::Reached)
		{
			At += ToNext / Left * Step;
			FacingYaw = FMath::FixedTurn(FacingYaw, static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(ToNext.Y, ToNext.X))), WalkingPerson::TurnRate * DeltaSeconds);
			break;
		}
		At = Path[Next];
		Step = FMath::Max(0.0, Step - Left);
		++Next;
	}

	// Keep to the ground (the road or the pavement, never a car roof).
	GroundClock -= DeltaSeconds;
	if (!bGroundKnown || GroundClock <= 0.f)
	{
		GroundClock = WalkingPerson::GroundEvery;
		UpdateGround(At, Here.Z);
	}
	const double Ground = bGroundKnown ? GroundZ : Here.Z - WalkingPerson::Middle;
	WalkTo(FVector(At, Ground + WalkingPerson::Middle * GetActorScale3D().Z), FacingYaw, Speed, DeltaSeconds);

	if (Next >= Path.Num())
	{
		if (bVanish)
		{
			Destroy();
			return;
		}
		Rest();
	}
}

void AWalkingPerson::UpdateGround(const FVector2D& At, double MiddleZ)
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WalkingPerson), false, this);
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByObjectType(Hit, FVector(At, MiddleZ + WalkingPerson::LookFrom), FVector(At, MiddleZ - 1000.0),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		GroundZ = Hit.ImpactPoint.Z;
		bGroundKnown = true;
	}
}
