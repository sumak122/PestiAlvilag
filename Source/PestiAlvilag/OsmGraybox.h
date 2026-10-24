// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OsmGraybox.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;

/**
 * Builds a gray-box block-out of real streets from the JSON that
 * Tools/OsmGraybox/osm_to_graybox.py writes: a ground plane, roads, squares,
 * parks, tram and rail tracks, and extruded buildings.
 *
 * Drop one into a level. It builds itself the first time, and again when
 * the JSON file is newer than the last build (save the level afterwards).
 */
UCLASS()
class PESTIALVILAG_API AOsmGraybox : public AActor
{
	GENERATED_BODY()

public:
	AOsmGraybox();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Reloads the JSON file and rebuilds every block. Happens by itself when the file changes. */
	UFUNCTION(CallInEditor, Category = "Graybox")
	void Rebuild();

	/** JSON file, relative to the project folder (kept out of Content so the editor does not try to import it). */
	UPROPERTY(EditAnywhere, Category = "Graybox")
	FString DataFile = TEXT("Data/OsmGraybox/blaha_test_area.json");

	/** Tick this if everything looks inside out (you can see through walls from outside). */
	UPROPERTY(EditAnywhere, Category = "Graybox")
	bool bFlipFaces = false;

	UPROPERTY(EditAnywhere, Category = "Graybox")
	bool bCreateCollision = true;

	UPROPERTY(EditAnywhere, Category = "Graybox|Colors")
	FLinearColor GroundColor = FLinearColor(0.30f, 0.29f, 0.27f);

	UPROPERTY(EditAnywhere, Category = "Graybox|Colors")
	FLinearColor RoadColor = FLinearColor(0.06f, 0.06f, 0.06f);

	UPROPERTY(EditAnywhere, Category = "Graybox|Colors")
	FLinearColor PlazaColor = FLinearColor(0.45f, 0.42f, 0.38f);

	UPROPERTY(EditAnywhere, Category = "Graybox|Colors")
	FLinearColor ParkColor = FLinearColor(0.10f, 0.25f, 0.06f);

	UPROPERTY(EditAnywhere, Category = "Graybox|Colors")
	FLinearColor TramColor = FLinearColor(0.35f, 0.10f, 0.08f);

	UPROPERTY(EditAnywhere, Category = "Graybox|Colors")
	FLinearColor RailColor = FLinearColor(0.22f, 0.15f, 0.10f);

	UPROPERTY(EditAnywhere, Category = "Graybox|Colors")
	FLinearColor WallColor = FLinearColor(0.62f, 0.55f, 0.45f);

	UPROPERTY(EditAnywhere, Category = "Graybox|Colors")
	FLinearColor RoofColor = FLinearColor(0.30f, 0.20f, 0.17f);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Graybox")
	TObjectPtr<UProceduralMeshComponent> Mesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

private:
	bool Build();
	FString DataPath() const;

	/** When the data file was last built from; a newer file triggers a rebuild. */
	UPROPERTY()
	FDateTime BuiltFromFileTime;
};
