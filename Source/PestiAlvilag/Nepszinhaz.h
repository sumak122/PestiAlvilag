// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Nepszinhaz.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;

/**
 * The Népszínház (from 1908 the Nemzeti Színház) on Blaha Lujza tér, the
 * first hero building: Fellner and Helmer's theatre of 1875, pulled down in
 * 1965. Built from code after period photos: a rusticated ground floor with
 * arched doors, giant columns under a sculpted pediment, corner pavilions
 * with statues on the balustrade, side porches, the domed roof over the
 * auditorium, and the rounded back towards the Körút. The sides and the
 * back are dressed like the front.
 *
 * The actor's front faces its +X; the game mode places it on the square
 * with the front towards the west, as it stood.
 */
UCLASS()
class PESTIALVILAG_API ANepszinhaz : public AActor
{
	GENERATED_BODY()

public:
	ANepszinhaz();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, Category = "Theatre|Colors")
	FLinearColor PlasterColor = FLinearColor(0.50f, 0.45f, 0.37f);

	UPROPERTY(EditAnywhere, Category = "Theatre|Colors")
	FLinearColor RusticColor = FLinearColor(0.40f, 0.36f, 0.30f);

	UPROPERTY(EditAnywhere, Category = "Theatre|Colors")
	FLinearColor TrimColor = FLinearColor(0.60f, 0.56f, 0.48f);

	UPROPERTY(EditAnywhere, Category = "Theatre|Colors")
	FLinearColor GlassColor = FLinearColor(0.025f, 0.03f, 0.035f);

	UPROPERTY(EditAnywhere, Category = "Theatre|Colors")
	FLinearColor DoorColor = FLinearColor(0.12f, 0.07f, 0.04f);

	UPROPERTY(EditAnywhere, Category = "Theatre|Colors")
	FLinearColor RoofColor = FLinearColor(0.10f, 0.11f, 0.12f);

	UPROPERTY(EditAnywhere, Category = "Theatre|Colors")
	FLinearColor StatueColor = FLinearColor(0.42f, 0.40f, 0.35f);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Theatre")
	TObjectPtr<UProceduralMeshComponent> Mesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

private:
	void Build();
};
