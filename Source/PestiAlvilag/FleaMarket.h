// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FleaMarket.generated.h"

class AStoryPerson;
class FJsonObject;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UProceduralMeshComponent;
class UStaticMesh;

/**
 * The Teleki tér market of 1932, after the Fortepan aerial photos: the
 * square laid with trodden earth instead of lawn, a grid of big open
 * market sheds on its east side with tables under them, and on the west
 * an open flea market of tables and blankets spread with second-hand
 * clothes, shoes, pots, junk, books, furniture, crockery and vegetables.
 * Sellers stand behind some stalls, shoppers look at the goods and a crowd
 * mills along the south side. Two stalls belong to people the story needs
 * (Pista bácsi, Bözsi néni): FindKeeper finds them.
 *
 * Laid out by the street furniture script (market); the street furniture
 * actor spawns it at the world origin and calls Setup.
 */
UCLASS()
class PESTIALVILAG_API AFleaMarket : public AActor
{
	GENERATED_BODY()

public:
	AFleaMarket();

	/** Builds the whole market from the furniture file's "market" object. */
	void Setup(const FJsonObject& Market);

	/** A stall keeper the story needs, by name (as in the furniture file), or null. */
	static AStoryPerson* FindKeeper(const UWorld* World, const FString& Name);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "Market")
	TObjectPtr<UProceduralMeshComponent> Ground;

	/** All the sheds' roofs, in one mesh. */
	UPROPERTY(VisibleAnywhere, Category = "Market")
	TObjectPtr<UProceduralMeshComponent> Roofs;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	/** Cube, cylinder, sphere and cone from the engine's basic shapes. */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> Shapes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Batches;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AStoryPerson>> People;

	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<AStoryPerson>> Keepers;

private:
	/** One shape, in world cm (Size is the whole size), batched by shape and colour. */
	void Add(int32 Shape, const FLinearColor& Color, const FVector& At, const FVector& Size,
		const FRotator& Turn = FRotator::ZeroRotator, bool bCollide = false);
	void BuildGround(const FJsonObject& Market);
	/** Adds a shed's pyramid roof to the roof mesh. */
	void BuildShedRoof(const FVector& Middle, double Yaw, double Width, double Depth);
	AStoryPerson* SpawnPerson(const FVector& Feet, double Yaw, FRandomStream& Random, const FString& Keeper = FString(), bool bShopper = false);

	double GroundZ = 0.0;
	TArray<FVector> RoofVertices;
	TArray<int32> RoofTriangles;
	TArray<FVector> RoofNormals;
	TMap<FString, int32> BatchIndex;
};
