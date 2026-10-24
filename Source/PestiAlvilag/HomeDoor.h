// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HomeDoor.generated.h"

class UMaterialInterface;
class UStaticMeshComponent;

/**
 * The street door of Bözsi néni's tenement on Népszínház utca, the player's
 * home. Once the prologue is done, pressing E here on foot sleeps and saves
 * the game. The actor stands on the pavement at the foot of the wall, its
 * +X pointing out to the street.
 */
UCLASS()
class PESTIALVILAG_API AHomeDoor : public AActor
{
	GENERATED_BODY()

public:
	AHomeDoor();

	virtual void Tick(float DeltaSeconds) override;

	/** How close the player must stand to use the door (cm). */
	UPROPERTY(EditAnywhere, Category = "Home")
	float UseRange = 250.f;

	/** Where someone coming out of the door stands, and which way they face. */
	FVector GetStandSpot() const;
	double GetStandYaw() const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Home")
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	virtual void BeginPlay() override;

private:
	double FadeDoneTime = -1.0;
};
