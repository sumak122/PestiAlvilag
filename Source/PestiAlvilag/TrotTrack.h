// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrotTrack.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

/**
 * The Kerepesi út trotting track at the east end of the map
 * (TrotTrackModel): sand oval and rails, grandstand, betting windows, fence
 * and gate, stables, and the engine shed, water tower and old gallop stand
 * nearby. Built from basic shapes when play begins; the game mode spawns it
 * at TrotTrackModel::Middle.
 */
UCLASS()
class PESTIALVILAG_API ATrotTrack : public AActor
{
	GENERATED_BODY()

public:
	ATrotTrack();

protected:
	virtual void BeginPlay() override;

private:
	void AddShape(int32 Shape, const FLinearColor& Color, const FVector& At, const FVector& Size, const FRotator& Turn, bool bCollide);

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	/** The engine's cube, cylinder, sphere and cone (MarketModel::EShape order). */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> Shapes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Batches;

	TMap<FString, int32> BatchIndex;
};
