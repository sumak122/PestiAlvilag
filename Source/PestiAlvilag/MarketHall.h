// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MarketHall.generated.h"

class AStoryPerson;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UProceduralMeshComponent;
class UStaticMesh;
class UTextRenderComponent;
class UTexture;

/**
 * The Rákóczi tér market hall, the third hero building (MarketHallModel):
 * the brick hall of 1897 with its four corner pavilions, the great gateway
 * on Rákóczi tér and the raised square over the crossing. Open to walk in:
 * iron columns, glazed roofs and rows of booths heaped with vegetables and
 * fruit, sellers behind most of them and shoppers in the aisles.
 *
 * The actor's origin is the middle of the main front at pavement level;
 * its +X runs from the front into the hall, +Y along the front towards
 * Déri Miksa utca. The game mode spawns it at FrontAt turned by Yaw.
 */
UCLASS()
class PESTIALVILAG_API AMarketHall : public AActor
{
	GENERATED_BODY()

public:
	AMarketHall();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Where the game puts the hall: the middle of its main front (cm, X east, Y south). */
	static FVector2D FrontAt();

	/** Which way the hall's +X points in the city (degrees). */
	static double Yaw();

	/** A point given in the hall's own frame, in the city. */
	static FVector2D ToCity(const FVector2D& Local);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Walls, floor, columns, booths: solid, walkable. */
	UPROPERTY(VisibleAnywhere, Category = "Market hall")
	TObjectPtr<UProceduralMeshComponent> Mesh;

	/** The roofs and the iron under them: out of reach, so no collision. */
	UPROPERTY(VisibleAnywhere, Category = "Market hall")
	TObjectPtr<UProceduralMeshComponent> Overhead;

	/** The glass: roof glazing and the see-through windows. Casts no shadow. */
	UPROPERTY(VisibleAnywhere, Category = "Market hall")
	TObjectPtr<UProceduralMeshComponent> Glazing;

	/** VÁSÁRCSARNOK on the board over the gateway. */
	UPROPERTY(VisibleAnywhere, Category = "Market hall")
	TObjectPtr<UTextRenderComponent> NameText;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> SeeThroughMaterial;

	UPROPERTY()
	TObjectPtr<UTexture> WhiteTexture;

	/** The engine's cube, cylinder, sphere and cone, for the goods on the stalls. */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> Shapes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Batches;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AStoryPerson>> People;

private:
	void Build();
	/** The goods on every stall, and the sellers and shoppers. */
	void Fill();
	void AddShape(int32 Shape, const FLinearColor& Color, const FVector& At, const FVector& Size, const FRotator& Turn, bool bCollide);
	AStoryPerson* SpawnPerson(const FVector& LocalFeet, double LocalYaw, FRandomStream& Random, bool bSeller);

	TMap<FString, int32> BatchIndex;
};
