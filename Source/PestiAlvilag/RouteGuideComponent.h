// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "RouteGuideComponent.generated.h"

class UMaterialInterface;
class UTextRenderComponent;

/**
 * Shows the way to a target without a line on the road: a see-through arrow
 * hangs over each of the next crossings where the route turns, pointing the
 * way to go. The route is planned once along the streets and followed; it is
 * only planned again when the player leaves it, so the arrows stay put.
 */
UCLASS()
class PESTIALVILAG_API URouteGuideComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	URouteGuideComponent();

	/** Keep guiding from From to To; call every frame while guiding. */
	void Guide(const FVector& From, const FVector& To);
	/** Stop guiding and hide the arrows. */
	void Stop();

protected:
	virtual void BeginPlay() override;

private:
	void Plan(const FVector& From, const FVector& To);
	/** Index of the route segment nearest to At, and how far At is from it (2D). */
	int32 NearestSegment(const FVector& At, double& OutDistance) const;
	void PlaceArrows();
	void HideArrows();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextRenderComponent>> Arrows;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> SeeThroughMaterial;

	TArray<FVector> Route;
	FVector Goal = FVector::ZeroVector;
	double NextPlanTime = 0.0;
};
