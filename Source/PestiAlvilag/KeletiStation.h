// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KeletiStation.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;
class UTextRenderComponent;
class UTexture;

/**
 * Keleti pályaudvar, the second hero building: Rochlitz Gyula's station of
 * 1884 as it stood in the early 1930s, built from code after period photos.
 *
 * The main front on Baross tér: the great glazed arch with radiating
 * glazing bars over a low colonnade, framed by two towers with paired
 * columns and statues in niches, the M.Á.V. attic with its sculpture group
 * on top. Behind it the train hall, open to walk in: arcaded side walls,
 * iron roof trusses under a roof with glazed strips, four tracks between
 * three platforms, an arrival train of dark green carriages and a
 * departure train on the other side. Along both sides the two-storey
 * wings with domed corner pavilions, the station restaurant's awning on
 * the departure wing. In front, the Baross statue in its railed lawn.
 *
 * The actor's origin is the middle of the main front at pavement level;
 * its +X runs from the front into the hall (east in the city), +Y to the
 * arrival side on Kerepesi út (south).
 */
UCLASS()
class PESTIALVILAG_API AKeletiStation : public AActor
{
	GENERATED_BODY()

public:
	AKeletiStation();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Where the game puts the station: the middle of its main front (cm, X east, Y south). */
	static FVector2D FrontAt();

	/** A point given in the station's own frame, in the city. */
	static FVector2D ToCity(const FVector2D& Local);

protected:
	/** Walls, platforms, trains, the statue: solid, walkable. */
	UPROPERTY(VisibleAnywhere, Category = "Station")
	TObjectPtr<UProceduralMeshComponent> Mesh;

	/** The hall roof and its iron trusses: out of reach, so no collision. */
	UPROPERTY(VisibleAnywhere, Category = "Station")
	TObjectPtr<UProceduralMeshComponent> Overhead;

	/**
	 * The glass: the roof strips (opaque from above, a faint see-through
	 * tint from below) and the see-through panes of the front arch. Casts
	 * no shadow, so the sun reaches the platforms.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Station")
	TObjectPtr<UProceduralMeshComponent> Glazing;

	UPROPERTY(VisibleAnywhere, Category = "Station")
	TObjectPtr<UTextRenderComponent> FriezeText;

	UPROPERTY(VisibleAnywhere, Category = "Station")
	TObjectPtr<UTextRenderComponent> AtticText;

	UPROPERTY(VisibleAnywhere, Category = "Station")
	TObjectPtr<UTextRenderComponent> RestaurantText;

	UPROPERTY(VisibleAnywhere, Category = "Station")
	TObjectPtr<UTextRenderComponent> StatueText;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	/** A see-through engine material for the glass, tinted per pane kind. */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> SeeThroughMaterial;

	UPROPERTY()
	TObjectPtr<UTexture> WhiteTexture;

private:
	void Build();
};
