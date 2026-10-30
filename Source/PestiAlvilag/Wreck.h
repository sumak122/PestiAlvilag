// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"

/**
 * Pieces of something a car has smashed (a market stall, a bench, a litter
 * bin): each a basic shape that flies and tumbles as a physics body,
 * bouncing off the ground and each other but never off the player or his
 * car, and later lies still where it fell.
 */
namespace Wreck
{
	// Pieces weigh about as much as wood of their size, within these bounds (kg).
	constexpr double KgPerCubicCm = 0.0006;
	constexpr float MinMass = 0.2f;
	constexpr float MaxMass = 25.f;

	/** One piece, made where Transform puts the shape (whole size = scale x 100 cm), thrown at Velocity, spinning at Spin (degrees a second). */
	inline UStaticMeshComponent* Throw(AActor* Owner, UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& Transform,
		const FVector& Velocity, const FVector& Spin, float DrawDistance)
	{
		UStaticMeshComponent* Piece = NewObject<UStaticMeshComponent>(Owner, NAME_None, RF_Transient);
		Piece->SetMobility(EComponentMobility::Movable);
		Piece->SetStaticMesh(Mesh);
		Piece->SetMaterial(0, Material);
		Piece->SetWorldTransform(Transform);
		const FVector Size = Transform.GetScale3D() * 100.0;
		Piece->SetCastShadow(Size.GetMax() > 40.0);
		Piece->SetCachedMaxDrawDistance(DrawDistance);
		Piece->SetCanEverAffectNavigation(false);
		Piece->SetCollisionObjectType(ECC_PhysicsBody);
		Piece->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Piece->SetCollisionResponseToAllChannels(ECR_Block);
		Piece->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Piece->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Ignore);
		Piece->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		Piece->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		Piece->RegisterComponent();
		Piece->SetSimulatePhysics(true);
		const float Mass = FMath::Clamp(static_cast<float>(Size.X * Size.Y * Size.Z * KgPerCubicCm), MinMass, MaxMass);
		Piece->SetMassOverrideInKg(NAME_None, Mass, true);
		Piece->SetLinearDamping(0.2f);
		Piece->SetAngularDamping(0.5f);
		Piece->SetPhysicsLinearVelocity(Velocity);
		Piece->SetPhysicsAngularVelocityInDegrees(Spin);
		return Piece;
	}

	/** Stops a piece where it lies, so it costs nothing more. */
	inline void Settle(UStaticMeshComponent* Piece)
	{
		if (IsValid(Piece))
		{
			Piece->SetSimulatePhysics(false);
			Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	/** How a piece flies off a car going at CarVelocity: on with it, out to either side, up, tumbling. */
	inline void Fling(const FVector& CarVelocity, FRandomStream& Random, FVector& OutVelocity, FVector& OutSpin)
	{
		const double Speed = CarVelocity.Size2D();
		const float Force = static_cast<float>(FMath::Min(Speed / 900.0, 1.3));
		const FVector Going(CarVelocity.X, CarVelocity.Y, 0.0);
		const FVector Across = FVector(-Going.Y, Going.X, 0.0).GetSafeNormal();
		OutVelocity = Going * Random.FRandRange(0.45f, 0.95f) + Across * (Speed * Random.FRandRange(-0.35f, 0.35f))
			+ FVector(0.0, 0.0, Random.FRandRange(120.f, 420.f) * Force);
		OutSpin = FVector(Random.FRandRange(-1.f, 1.f), Random.FRandRange(-1.f, 1.f), Random.FRandRange(-1.f, 1.f)) * (400.f * Force);
	}
}
