// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GarayYard.generated.h"

class UInstancedStaticMeshComponent;
class UPointLightComponent;
class UMaterialInterface;
class UStaticMesh;

/**
 * Garay utca 13 (GarayModel): Friedrich Hermann's wholesale shop on the
 * street with his glass office at the back, and the timber yard behind the
 * gate that he lets to Schwimmer Mihály. Mission 5, "A felmondás", happens
 * here. Built from basic shapes when play begins; the shop's ceiling lamps
 * light it inside, a little dimmer at night so it doesn't glare.
 *
 * The actor's origin is the middle of the yard's street front at pavement
 * level, its +X points away from the street; the game mode spawns it at
 * GarayModel::CityAt turned by GarayModel::CityYaw.
 */
UCLASS()
class PESTIALVILAG_API AGarayYard : public AActor
{
	GENERATED_BODY()

public:
	AGarayYard();

	/** The yard in the world (or where it would stand), for turning its own points into the city's. */
	static FTransform Frame(const UWorld* World);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	/** The shop's lamps as bright as the time of day wants them. */
	void UpdateLamps();
	void AddShape(int32 Shape, const FLinearColor& Color, const FVector& At, const FVector& Size, const FRotator& Turn, bool bCollide);

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	/** The engine's cube, cylinder, sphere and cone (MarketModel::EShape order). */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> Shapes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Batches;

	TMap<FString, int32> BatchIndex;

	/** The light of each of the shop's ceiling lamps. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPointLightComponent>> ShopLights;
};
