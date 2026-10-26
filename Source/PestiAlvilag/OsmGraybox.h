// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OsmGraybox.generated.h"

class FJsonObject;
class UMaterialInterface;
class UProceduralMeshComponent;

/**
 * Builds a gray-box block-out of real streets from the JSON that
 * Tools/OsmGraybox/osm_to_graybox.py writes: a ground plane, roads, squares,
 * parks, tram and rail tracks, and extruded buildings in period plaster
 * colours. Walls facing a street get a Pest tenement facade: windows with
 * stone frames and sills, ledges between floors, a cornice on top, and shops
 * and a gate on the ground floor.
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
	virtual void BeginPlay() override;

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

	/** The tram track bed: granite setts. */
	UPROPERTY(EditAnywhere, Category = "Graybox|Colors")
	FLinearColor TramColor = FLinearColor(0.20f, 0.19f, 0.17f);

	/** The row of bigger stones along each side of the tram track bed. */
	UPROPERTY(EditAnywhere, Category = "Graybox|Colors")
	FLinearColor TrackEdgeColor = FLinearColor(0.34f, 0.32f, 0.29f);

	/** The groove beside each tram rail head, where the wheel flange runs. */
	UPROPERTY(EditAnywhere, Category = "Graybox|Colors")
	FLinearColor RailGrooveColor = FLinearColor(0.02f, 0.02f, 0.02f);

	/** Tram rail heads, polished by the wheels. */
	UPROPERTY(EditAnywhere, Category = "Graybox|Colors")
	FLinearColor RailHeadColor = FLinearColor(0.50f, 0.50f, 0.52f);

	UPROPERTY(EditAnywhere, Category = "Graybox|Colors")
	FLinearColor RailColor = FLinearColor(0.22f, 0.15f, 0.10f);

	/** Plaster colours; each building gets one of these. */
	UPROPERTY(EditAnywhere, Category = "Graybox|Colors")
	TArray<FLinearColor> WallColors = {
		FLinearColor(0.62f, 0.55f, 0.42f),  // cream
		FLinearColor(0.60f, 0.45f, 0.24f),  // ochre
		FLinearColor(0.50f, 0.49f, 0.46f),  // grey
		FLinearColor(0.68f, 0.60f, 0.36f),  // pale yellow
		FLinearColor(0.60f, 0.43f, 0.34f),  // salmon
		FLinearColor(0.46f, 0.47f, 0.38f),  // grey-green
		FLinearColor(0.36f, 0.34f, 0.31f),  // sooty grey
	};

	UPROPERTY(EditAnywhere, Category = "Graybox|Colors")
	FLinearColor RoofColor = FLinearColor(0.30f, 0.20f, 0.17f);

	/** Window frames, ledges and cornices. */
	UPROPERTY(EditAnywhere, Category = "Graybox|Colors")
	FLinearColor StoneColor = FLinearColor(0.72f, 0.68f, 0.60f);

	UPROPERTY(EditAnywhere, Category = "Graybox|Colors")
	FLinearColor GlassColor = FLinearColor(0.02f, 0.025f, 0.03f);

	/** Gates and shop front frames. */
	UPROPERTY(EditAnywhere, Category = "Graybox|Colors")
	FLinearColor WoodColor = FLinearColor(0.12f, 0.06f, 0.03f);

	/** Shop sign boards. */
	UPROPERTY(EditAnywhere, Category = "Graybox|Colors")
	FLinearColor SignColor = FLinearColor(0.03f, 0.10f, 0.06f);

	/** Dress street-facing walls with windows, ledges, cornices, shops and gates. */
	UPROPERTY(EditAnywhere, Category = "Graybox|Facades")
	bool bBuildFacades = true;

	/** Facade detail further away than this is not drawn (cm); the plain walls still are. */
	UPROPERTY(EditAnywhere, Category = "Graybox|Facades")
	float FacadeDrawDistance = 45000.f;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Graybox")
	TObjectPtr<UProceduralMeshComponent> Mesh;

	/** Facade detail in a grid of tiles, so far-away tiles can be skipped when drawing. Made at game start only; no collision. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> FacadeTiles;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

private:
	bool Build();
	void BuildFacades();
	void BuildFacades(const FJsonObject& Root);
	bool HasFacades() const;
	TSharedPtr<FJsonObject> LoadData() const;
	FString DataPath() const;

	/** When the data file was last built from; a newer file triggers a rebuild. */
	UPROPERTY()
	FDateTime BuiltFromFileTime;
};
