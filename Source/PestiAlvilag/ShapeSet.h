// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MarketModel.h"
#include "ShapeSet.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UPointLightComponent;
class UStaticMesh;

/**
 * Basic shapes (MarketModel::FShape), painted lettering and small lamps put
 * together in the actor's own frame: a house, a shop front, a kiosk, a
 * thing someone carries. Whoever spawns it fills it (AddShapes, AddSign,
 * AddLamp); the lamps glow as it gets dark. SetKey names it, so that the
 * one in the world can be found again (Find).
 */
UCLASS()
class PESTIALVILAG_API AShapeSet : public AActor
{
	GENERATED_BODY()

public:
	AShapeSet();

	virtual void Tick(float DeltaSeconds) override;

	void AddShapes(const TArray<MarketModel::FShape>& Shapes);
	void AddSign(const FString& Text, const FVector& At, double Yaw, double Height, const FLinearColor& Color);
	/** A small warm light at At, reaching Radius (cm), lit from dusk to dawn. */
	void AddLamp(const FVector& At, float Radius);
	/** Things further off than this are not drawn (cm). */
	void SetDrawDistance(float Distance);

	void SetKey(FName InKey) { Key = InKey; }
	/** The set with this key in World, or null. */
	static AShapeSet* Find(const UWorld* World, FName InKey);

private:
	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	/** The engine's cube, cylinder, sphere and cone (MarketModel::EShape order). */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> Meshes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Batches;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPointLightComponent>> Lamps;

	TMap<FString, int32> BatchIndex;
	FName Key;
	float DrawDistance = 20000.f;
	float LampCheckIn = 0.f;
};
