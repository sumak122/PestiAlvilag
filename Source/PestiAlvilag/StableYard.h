// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StableYard.generated.h"

class AStoryPerson;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

/**
 * The carters' yard on Auróra utca (StableYardModel), where Pista bácsi's
 * horse Csillag and his cart wait, looked after by Jóska the stable lad.
 * Built from basic shapes when play begins.
 *
 * The actor's origin is the middle of the yard's street front at pavement
 * level, its +X points into the yard; the game mode spawns it at
 * StableYardModel::CityAt turned by StableYardModel::CityYaw.
 */
UCLASS()
class PESTIALVILAG_API AStableYard : public AActor
{
	GENERATED_BODY()

public:
	AStableYard();

	/** Turns a point of the yard's own frame into the city, at Ground (cm). */
	static FVector ToCity(const FVector& Local, double Ground);

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

	/** Jóska, the stable lad. */
	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Joska;
};
