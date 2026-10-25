// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FallingLeaves.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;

/**
 * Autumn leaves in the wind around the camera. Most come off the park trees
 * that still have leaves, flutter down with the wind, lie a while and are
 * blown along the ground by the gusts; a few blow through the streets too.
 *
 * Each leaf is a small flat card, drawn as instances of the engine cube in
 * a few colours and moved by this actor every frame, so it needs no art and
 * no particle system.
 */
UCLASS()
class PESTIALVILAG_API AFallingLeaves : public AActor
{
	GENERATED_BODY()

public:
	AFallingLeaves();

	virtual void Tick(float DeltaSeconds) override;

	/** The trees leaves fall from: ground point (XYZ) and tree size (W). */
	void SetTrees(const TArray<FVector4>& InTrees);

	/** Leaves in the air or on the ground near the camera at one time. */
	UPROPERTY(EditAnywhere, Category = "Leaves")
	int32 LeafCount = 320;

	/** Leaves come down within this distance of the camera (cm). */
	UPROPERTY(EditAnywhere, Category = "Leaves")
	float Range = 4500.f;

	/** Average wind speed (cm/s); the gusts go up to about twice that. */
	UPROPERTY(EditAnywhere, Category = "Leaves")
	float WindSpeed = 160.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY(VisibleAnywhere, Category = "Leaves")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Colours;

private:
	struct FLeaf
	{
		FVector Location = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		double GroundZ = 0.0;
		float Phase = 0.f;
		float Flutter = 1.f;
		float Spin = 0.f;
		float Yaw = 0.f;
		float LieTime = 0.f;
		bool bLanded = false;
	};

	/** Starts a leaf off again somewhere near Around; bScatter at the start, anywhere on its way down. */
	void Release(FLeaf& Leaf, const FVector& Around, bool bScatter);
	bool CameraLocation(FVector& OutLocation) const;

	TArray<FVector4> Trees;
	/** The trees near the camera, found again when it has moved a way. */
	TArray<int32> NearTrees;
	FVector NearTreesFrom = FVector(1e9);
	TArray<FLeaf> Leaves;
	TArray<TArray<FTransform>> Frames;
	FRandomStream Random;
	float Time = 0.f;
	bool bStarted = false;
};
