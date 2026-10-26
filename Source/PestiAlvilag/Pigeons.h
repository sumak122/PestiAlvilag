// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PigeonModel.h"
#include "Pigeons.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UProceduralMeshComponent;
class USceneComponent;

/**
 * Flocks of town pigeons (PigeonModel.h) on the squares: Blaha Lujza tér,
 * Tisza Kálmán tér, the Teleki tér market, Baross tér and the forecourt of
 * Keleti. On the ground they strut about, bobbing their heads, and peck.
 * When the player runs at them, comes too close, or drives up in a car,
 * the whole flock takes off (not all at once), climbs and circles high over
 * the square, and once all is quiet again comes back down to land.
 */
UCLASS()
class PESTIALVILAG_API APigeons : public AActor
{
	GENERATED_BODY()

public:
	APigeons();

	virtual void Tick(float DeltaSeconds) override;

	/** Flocks further than this from the camera are hidden and left still (cm). */
	UPROPERTY(EditAnywhere, Category = "Pigeons")
	float ShowWithin = 9000.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

private:
	enum class EState : uint8 { Ground, Startled, Flying, Landing };

	struct FBird
	{
		USceneComponent* Root = nullptr;
		UProceduralMeshComponent* Parts[PigeonModel::NumParts] = {};
		EState State = EState::Ground;
		FVector At = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		float Yaw = 0.f;
		/** On the ground: where he struts to, and how long he pecks for. */
		FVector Target = FVector::ZeroVector;
		float Busy = 0.f;
		bool bPecking = false;
		float Delay = 0.f;
		/** The wing beat, the head bob. */
		float Phase = 0.f;
		/** How long he stays up before coming down (s), and which way round he circles. */
		float Airtime = 12.f;
		float Circling = 1.f;
		float Up = 0.f;
	};

	struct FFlock
	{
		FVector Middle = FVector::ZeroVector;
		/** Places on the open ground to strut about and land on. */
		TArray<FVector> Spots;
		TArray<int32> Birds;
		bool bShown = true;
		/** Seconds since anything frightened them. */
		float Calm = 100.f;
	};

	void AddFlock(const FVector2D& Middle, int32 Count);
	bool GroundAt(const FVector2D& At, FVector& OutGround) const;
	void Startle(FFlock& Flock, const FVector& From);
	void MoveBird(FBird& Bird, const FFlock& Flock, float DeltaSeconds);
	void PoseBird(FBird& Bird);
	UMaterialInstanceDynamic* Paint(const FLinearColor& Color);

	TArray<PigeonModel::FPigeon> Plumages;
	TMap<FString, UMaterialInstanceDynamic*> Paints;
	TArray<FBird> Birds;
	TArray<FFlock> Flocks;

	FVector LastPlayerAt = FVector::ZeroVector;
	TWeakObjectPtr<AActor> LastPlayer;
	FVector PlayerVelocity = FVector::ZeroVector;
	FRandomStream Random;
};
