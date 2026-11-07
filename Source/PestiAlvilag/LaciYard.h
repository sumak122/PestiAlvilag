// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaciYard.generated.h"

class APestiCar;
class AStoryPerson;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

/**
 * Laci's motor workshop on Szerdahelyi utca (LaciYardModel), where Bözsi
 * néni's nephew mends cars and takes the doctor's Steyr (mission 7).
 * Built from basic shapes when play begins; Laci stands in it.
 *
 * The actor's origin is the middle of the yard's street front at pavement
 * level, its +X points into the yard; the game mode spawns it at
 * LaciYardModel::CityAt turned by LaciYardModel::CityYaw.
 */
UCLASS()
class PESTIALVILAG_API ALaciYard : public AActor
{
	GENERATED_BODY()

public:
	ALaciYard();

	/** Turns a point of the yard's own frame into the city, at Ground (cm). */
	static FVector ToCity(const FVector& Local, double Ground);

	/** Laci, the mechanic, or null before play begins. */
	AStoryPerson* GetLaci() const;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	/**
	 * The garage (after mission 7): a car stopped over the pit is mended and
	 * sprayed a new colour behind a short fade to black, for a fee, and the
	 * police lose it; Laci says why not when he won't.
	 */
	void TickGarage(float DeltaSeconds);
	void StartWork(APestiCar* Car);
	void FinishWork();
	void LaciSays(const FString& Hu, const FString& En) const;

	void AddShape(int32 Shape, const FLinearColor& Color, const FVector& At, const FVector& Size, const FRotator& Turn, bool bCollide);

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	/** The engine's cube, cylinder, sphere and cone (MarketModel::EShape order). */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> Shapes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Batches;

	TMap<FString, int32> BatchIndex;

	/** Laci, the mechanic. */
	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Laci;

	/** The car being mended, while the screen is black. */
	UPROPERTY(Transient)
	TObjectPtr<APestiCar> InWork;

	/** How long the car has stood still over the pit (s). */
	float StoodFor = 0.f;
	/** Laci has had his say (or done the work): nothing more until the car leaves the pit. */
	bool bWaitForLeave = false;
	/** While working: when the car is done (the screen still black), and when the screen clears. */
	double DoneAt = -1.0;
	double ClearAt = -1.0;
};
