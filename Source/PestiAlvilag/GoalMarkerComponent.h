// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GoalMarkerComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
class UTexture;

/**
 * The mark over a goal, in an art deco dress and kept small: a stepped gold
 * gem about 2.5 m up that slowly turns, a little fan of rays over it facing
 * the camera, a faint gold thread down to the ground and a sunburst of short
 * gold rays on the ground round the spot. It glows (unlit, see-through) so
 * it reads in the dark too. It lives in the world, not on its owner.
 */
UCLASS(ClassGroup = (Custom))
class PESTIALVILAG_API UGoalMarkerComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UGoalMarkerComponent();

	/** Stands the marker on the ground at (or just under) Target. */
	void ShowAt(const FVector& Target);
	void Hide();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void Build();
	UMaterialInstanceDynamic* Glow(const FLinearColor& Color, float Opacity);
	void AddBar(USceneComponent* Parent, UMaterialInstanceDynamic* Paint, const FVector& At, const FVector& Size, const FRotator& Turn);

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> SeeThroughMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UTexture> WhiteTexture;

	/** The gem, which turns, and the fan of rays, which faces the camera. */
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> Gem;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> Fan;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Bars;

	float Time = 0.f;
};
