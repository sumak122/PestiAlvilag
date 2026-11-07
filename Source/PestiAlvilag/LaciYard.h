// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaciYard.generated.h"

class AStoryPerson;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

/**
 * Laci's motor workshop on Szerdahelyi utca (LaciYardModel), where Bözsi
 * néni's nephew mends cars and takes the doctor's Steyr (mission 7).
 * Built from basic shapes when play begins; Laci stands in it.
 *
 * The actor's origin is the middle of the yard's street front at pavement
 * level, its +X points into the yard; the game mode spawns it at
 * LaciYardModel::CityAt turned by LaciYardModel::CityYaw.
 */
UCLASS()
class PESTIALVILAG_API ALaciYard : public AActor
{
	GENERATED_BODY()

public:
	ALaciYard();

	/** Turns a point of the yard's own frame into the city, at Ground (cm). */
	static FVector ToCity(const FVector& Local, double Ground);

	/** Laci, the mechanic, or null before play begins. */
	AStoryPerson* GetLaci() const;

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

	/** Laci, the mechanic. */
	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Laci;
};
