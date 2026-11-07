// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChimneySmoke.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;
class UTexture;

/**
 * Coal smoke from the chimneys over the roofs, and the steam of the two
 * engines standing at Keleti. Brick chimney stacks are put up on the flat
 * roofs round the camera as it goes (the map has none of its own); about
 * half of them smoke, more in the morning and evening, and the smoke drifts
 * off with the wind, swelling and thinning. The engines breathe soft steam
 * from their chimneys and now and then blow a white cloud from their
 * cylinders. All the smoke is one mesh, like AExhaustSmoke.
 */
UCLASS()
class PESTIALVILAG_API AChimneySmoke : public AActor
{
	GENERATED_BODY()

public:
	AChimneySmoke();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	struct FStack
	{
		FVector Top = FVector::ZeroVector;
		/** 0 for a cold chimney; otherwise how much it smokes (puffs a second). */
		float Rate = 0.f;
		float Timer = 0.f;
	};

	struct FPuff
	{
		FVector At = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		float Size = 0.f;
		float Grow = 1.f;
		float Age = 0.f;
		float Life = 0.f;
	};

	/** Coal smoke and white steam each have a pool and a mesh section. */
	struct FPool
	{
		TArray<FPuff> Puffs;
		int32 Next = 0;
		bool bAny = false;
	};

	void Scan(const FVector& Eye, double Street);
	bool IsRoof(const FVector2D& At, double Street, double& OutZ) const;
	void BuildStacks();
	void FindSteam();
	void Emit(FPool& Pool, const FVector& At, const FVector& Velocity, float Size, float Grow, float Life);
	void UpdatePool(int32 Section, FPool& Pool, float DeltaSeconds);
	UMaterialInterface* SeeThrough(const FLinearColor& Tint);

	/** The smoke and steam. */
	UPROPERTY(VisibleAnywhere, Category = "Smoke")
	TObjectPtr<UProceduralMeshComponent> Mesh;

	/** The brick stacks on the roofs. */
	UPROPERTY(VisibleAnywhere, Category = "Smoke")
	TObjectPtr<UProceduralMeshComponent> Stacks;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> SeeThroughMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY()
	TObjectPtr<UTexture> WhiteTexture;

	TArray<FStack> Chimneys;
	TSet<FIntPoint> Scanned;
	TArray<FVector> SteamChimneys;
	TArray<FVector> SteamCylinders;
	TArray<float> CylinderTimers;
	bool bSteamFound = false;
	bool bStacksDirty = false;
	float ScanTimer = 0.f;
	float StackTimer = 0.f;
	float SteamTimer = 0.f;
	FPool Pools[2];
	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	FRandomStream Random;
};
