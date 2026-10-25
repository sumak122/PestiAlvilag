// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StoryPerson.h"
#include "ParkKeeper.generated.h"

class USceneComponent;
class UStaticMeshComponent;

/**
 * A park keeper in a grey work coat with a long wooden rake. He
 * walks from one leaf pile to the next, stops beside it and rakes for a
 * while, then moves on, round and round his part of the park.
 */
UCLASS()
class PESTIALVILAG_API AParkKeeper : public AStoryPerson
{
	GENERATED_BODY()

public:
	AParkKeeper();

	virtual void Tick(float DeltaSeconds) override;

	/** The leaf piles he tends (ground points), in the order he visits them. */
	void SetWorkSpots(const TArray<FVector>& InSpots);

	/** How long he rakes at each pile (s). */
	UPROPERTY(EditAnywhere, Category = "Keeper")
	float RakeTime = 18.f;

protected:
	/** Where his hands hold the rake; the rake swings from here. */
	UPROPERTY(VisibleAnywhere, Category = "Keeper")
	TObjectPtr<USceneComponent> RakeGrip;

	UPROPERTY(VisibleAnywhere, Category = "Keeper")
	TObjectPtr<UStaticMeshComponent> RakeHandle;

	UPROPERTY(VisibleAnywhere, Category = "Keeper")
	TObjectPtr<UStaticMeshComponent> RakeHead;

	UPROPERTY(VisibleAnywhere, Category = "Keeper")
	TObjectPtr<UStaticMeshComponent> RakeTeeth;

	virtual void BeginPlay() override;

private:
	void PoseLimbs(float Legs, float Arms);

	TArray<FVector> Spots;
	int32 Next = 0;
	float Raking = 0.f;
	float Phase = 0.f;
};
