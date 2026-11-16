// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkidMarks.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UProceduralMeshComponent;
class UTexture;

/**
 * Black tyre marks left on the setts where a car slid or locked its wheels.
 * Each tyre draws its own unbroken line of thin dark strips just above the
 * ground. One of these is made the first time a tyre leaves a mark, and it
 * draws all the marks in the game as one mesh, in a few batches by age: the
 * older a batch, the fainter, until the oldest is wiped for new marks
 * (about a minute after it was made).
 */
UCLASS()
class PESTIALVILAG_API ASkidMarks : public AActor
{
	GENERATED_BODY()

public:
	ASkidMarks();

	/** The world's tyre marks, made on first use. */
	static ASkidMarks* Get(UWorld* World);

	/**
	 * A tyre at At (world, on the ground) leaves a mark, Strength 0..1 (how
	 * wide and dark). Tyre tells the tyres apart, so each draws its own line;
	 * a tyre that has not marked for a moment starts a new line.
	 */
	void Mark(int32 Tyre, const FVector& At, float Strength);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	/** Where a tyre's line got to, and when. */
	struct FTrail
	{
		FVector Last = FVector::ZeroVector;
		double Time = -1000.0;
	};

	/** One batch of marks: its strips, and whether the mesh needs them again. */
	struct FBatch
	{
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		int32 Strips = 0;
		bool bDirty = false;
	};

	/** The newest batch is full or old enough: the oldest one is wiped and takes new marks, and the rest fade a step. */
	void NextBatch(double Now);
	void FadeBatches();

	UPROPERTY(VisibleAnywhere, Category = "Marks")
	TObjectPtr<UProceduralMeshComponent> Mesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> SeeThroughMaterial;

	UPROPERTY()
	TObjectPtr<UTexture> WhiteTexture;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> BatchMaterials;

	TMap<int32, FTrail> Trails;
	TArray<FBatch> Batches;
	/** The batch taking new marks, and when it started. */
	int32 Newest = 0;
	double NewestSince = 0.0;
};
