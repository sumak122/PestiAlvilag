// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "RouteGuideComponent.generated.h"

/**
 * Plans the way to a target along the streets and shows it as a line on the
 * minimap (APestiHUD::ShowRoute): gold for a mission or a fare, red for the
 * player's own mark. The route is planned once and followed; it is only
 * planned again when the player leaves it or the target moves.
 */
UCLASS()
class PESTIALVILAG_API URouteGuideComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	URouteGuideComponent();

	/** Keep guiding from From to To; call every frame while guiding. */
	void Guide(const FVector& From, const FVector& To);
	/** Stop guiding; the line leaves the minimap. */
	void Stop();

	/** True for the route to the player's own mark (drawn red). */
	bool bOwnMark = false;

private:
	void Plan(const FVector& From, const FVector& To);
	/** Index of the route segment nearest to At, and how far At is from it (2D). */
	int32 NearestSegment(const FVector& At, double& OutDistance) const;
	TArray<FVector> Route;
	FVector Goal = FVector::ZeroVector;
	double NextPlanTime = 0.0;
};
