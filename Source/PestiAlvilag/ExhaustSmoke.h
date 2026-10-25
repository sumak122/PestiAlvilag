// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExhaustSmoke.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;
class UTexture;

/**
 * All the cars' exhaust smoke: little see-through grey puffs that drift
 * back from the tail pipe, swell, rise and fade away. One of these is made
 * the first time a car puffs, and it draws every puff in the game as one
 * mesh, so lots of smoke costs very little.
 */
UCLASS()
class PESTIALVILAG_API AExhaustSmoke : public AActor
{
	GENERATED_BODY()

public:
	AExhaustSmoke();

	/** The world's smoke, made on first use. */
	static AExhaustSmoke* Get(UWorld* World);

	/** A puff at At (world), moving off at Velocity (cm/s), Size across (cm) when it leaves the pipe. */
	void Puff(const FVector& At, const FVector& Velocity, float Size, bool bDark);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	struct FSmokePuff
	{
		FVector At = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		float Size = 0.f;
		float Age = 0.f;
		float Life = 0.f;
	};

	/** Light everyday smoke, and the dark puffs of a backfire. */
	struct FPool
	{
		TArray<FSmokePuff> Puffs;
		int32 Next = 0;
		bool bAny = false;
	};

	void UpdatePool(int32 Section, FPool& Pool, float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, Category = "Smoke")
	TObjectPtr<UProceduralMeshComponent> Mesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> SeeThroughMaterial;

	UPROPERTY()
	TObjectPtr<UTexture> WhiteTexture;

	FPool Pools[2];
	TArray<FVector> Vertices;
	TArray<FVector> Normals;
};
