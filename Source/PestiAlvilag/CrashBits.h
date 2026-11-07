// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CrashBits.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UProceduralMeshComponent;
class UTexture;

/**
 * What flies off in a car crash: glowing orange sparks that arc down and go
 * out, bits of broken glass and grit that tumble and lie on the road a while,
 * and a puff of dust (the exhaust smoke's puffs). One of these is made the
 * first time something crashes, and it draws every bit in the game as one
 * mesh, like AExhaustSmoke.
 */
UCLASS()
class PESTIALVILAG_API ACrashBits : public AActor
{
	GENERATED_BODY()

public:
	ACrashBits();

	/** The world's crash bits, made on first use. */
	static ACrashBits* Get(UWorld* World);

	/**
	 * A crash at At (world): Away is the way out from what was hit, Strength
	 * 0..1 how hard; with glass when a pane or lamp broke. The bits come to
	 * rest on the road at height Ground.
	 */
	void Burst(const FVector& At, const FVector& Away, float Strength, bool bGlass, double Ground);

	/** A few sparks only, for a wing scraping along a wall. */
	void Scrape(const FVector& At, const FVector& Away, const FVector& Along, double Ground);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	struct FBit
	{
		FVector At = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		FVector Axis = FVector::UpVector;
		float Angle = 0.f;
		float Spin = 0.f;
		float Size = 0.f;
		float Age = 0.f;
		float Life = 0.f;
		double Ground = 0.0;
	};

	/** Sparks, glass and grit each have their own pool and their own mesh section. */
	struct FPool
	{
		TArray<FBit> Bits;
		int32 Next = 0;
		bool bAny = false;
	};

	FBit& Take(FPool& Pool);
	void AddSparks(const FVector& At, const FVector& Away, const FVector& Along, int32 Count, float Speed, double Ground);
	void UpdateSparks(float DeltaSeconds, const FVector& Eye);
	void UpdatePieces(int32 Section, float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, Category = "Crash")
	TObjectPtr<UProceduralMeshComponent> Mesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> SeeThroughMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY()
	TObjectPtr<UTexture> WhiteTexture;

	/** The sparks' glow, kept as bright as the daylight needs. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SparkGlow;

	FPool Pools[3];
	TArray<FVector> Vertices;
	TArray<FVector> Normals;
};
