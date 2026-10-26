// Fill out your copyright notice in the Description page of Project Settings.

#include "StoryPerson.h"

#include "CarBody.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "PersonModel.h"
#include "ProceduralMeshComponent.h"

namespace StoryPerson
{
	constexpr float CapsuleRadius = 35.f;
	constexpr float CapsuleHalfHeight = 90.f;
	constexpr float MaxSwing = 30.f;
	constexpr float StrideLength = 120.f;
	constexpr float TurnRate = 360.f;
}

AStoryPerson::AStoryPerson()
{
	PrimaryActorTick.bCanEverTick = true;

	BaseMaterial = CarBody::LoadPaintMaterial();

	// The player bumps into him rather than walking through.
	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->InitCapsuleSize(StoryPerson::CapsuleRadius, StoryPerson::CapsuleHalfHeight);
	Capsule->SetCollisionProfileName(TEXT("Pawn"));
	RootComponent = Capsule;
}

void AStoryPerson::BeginPlay()
{
	Super::BeginPlay();

	Outfit.Coat = CoatColor;
	PersonModel::Dress(this, Capsule, BaseMaterial, Outfit, Pieces);
}

void AStoryPerson::SetCoatColor(const FLinearColor& Color)
{
	CoatColor = Color;
	Outfit.Coat = Color;
	if (HasActorBegunPlay())
	{
		PersonModel::Dress(this, Capsule, BaseMaterial, Outfit, Pieces);
	}
}

void AStoryPerson::SetOutfit(const PersonKit::FOutfit& InOutfit)
{
	Outfit = InOutfit;
	CoatColor = Outfit.Coat;
	if (HasActorBegunPlay())
	{
		PersonModel::Dress(this, Capsule, BaseMaterial, Outfit, Pieces);
	}
}

USceneComponent* AStoryPerson::Limb(int32 Index) const
{
	return Pieces.IsValidIndex(Index) ? Pieces[Index].Get() : nullptr;
}

void AStoryPerson::Follow(AActor* Target)
{
	FollowTarget = Target;
}

void AStoryPerson::FaceTowards(const FVector& Point)
{
	const FVector Direction = (Point - GetActorLocation()).GetSafeNormal2D();
	if (!Direction.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.0, Direction.Rotation().Yaw, 0.0));
	}
}

void AStoryPerson::SetInWorld(bool bInWorld)
{
	SetActorHiddenInGame(!bInWorld);
	SetActorEnableCollision(bInWorld);
	SetActorTickEnabled(bInWorld);
}

void AStoryPerson::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	float Speed = 0.f;
	if (FollowTarget)
	{
		const FVector Here = GetActorLocation();
		const FVector Offset = FollowTarget->GetActorLocation() - Here;
		const double Gap = Offset.Size2D();
		if (Gap > FollowDistance)
		{
			// Catch up faster when left far behind.
			Speed = WalkSpeed * FMath::Clamp(static_cast<float>(Gap / FollowDistance) - 0.5f, 1.f, 3.f);
			const FVector Direction = Offset.GetSafeNormal2D();
			FVector Next = Here + Direction * FMath::Min(static_cast<double>(Speed * DeltaSeconds), Gap - FollowDistance);

			// Keep his feet on the ground.
			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(StoryPersonGround), false, this);
			Params.AddIgnoredActor(FollowTarget);
			if (GetWorld()->LineTraceSingleByChannel(Hit, Next + FVector(0.0, 0.0, 150.0), Next - FVector(0.0, 0.0, 500.0), ECC_Visibility, Params))
			{
				Next.Z = Hit.ImpactPoint.Z + StoryPerson::CapsuleHalfHeight;
			}
			SetActorLocation(Next);
		}
		const float WantYaw = static_cast<float>(Offset.GetSafeNormal2D().Rotation().Yaw);
		const float Yaw = FMath::FixedTurn(static_cast<float>(GetActorRotation().Yaw), WantYaw, StoryPerson::TurnRate * DeltaSeconds);
		SetActorRotation(FRotator(0.0, Yaw, 0.0));
	}
	SwingLimbs(Speed, DeltaSeconds);
}

void AStoryPerson::SwingLimbs(float Speed, float DeltaSeconds)
{
	StridePhase = Speed > 0.f ? StridePhase + DeltaSeconds * Speed / StoryPerson::StrideLength * PI : 0.f;
	const float Reach = StoryPerson::MaxSwing * FMath::Clamp(Speed / 300.f, 0.f, 1.f);
	const float Swing = FMath::Sin(StridePhase) * Reach;
	PersonModel::StrideLegs(Pieces, StridePhase, Reach, FMath::Clamp(Speed / 150.f, 0.f, 1.6f));
	const float Arms[] = { -Swing, Swing };  // left arm, right arm
	for (int32 i = 0; i < 2; ++i)
	{
		if (USceneComponent* Part = Limb(PersonKit::ArmLeft + i))
		{
			Part->SetRelativeRotation(FRotator(Arms[i], 0.f, 0.f));
		}
	}
}
