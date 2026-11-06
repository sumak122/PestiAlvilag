// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestGrounds.generated.h"

class AChaseThief;
class APawn;
class APestiPerson;
class APlayerController;
class ARuffian;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UPointLightComponent;
class UStaticMesh;

/** Which of the two test grounds. */
enum class ETestGround : uint8
{
	Fight,
	Vehicles,
};

/**
 * The test grounds (TestGroundsModel), far out west of the city, for trying
 * out fights and vehicles without playing a mission: the fight yard, where
 * the number keys bring in the thief, Bika, three ruffians or Hermann's
 * shop people and 0 clears them away and heals you; and the vehicle yard
 * with every kind of car and the horse cart, where 0 brings fresh ones.
 *
 * Reached from the Esc menu (FPestiMenu, PRÓBATEREP): it is built the first
 * time it is needed, and remembers where in the city you came from.
 */
UCLASS()
class PESTIALVILAG_API ATestGrounds : public AActor
{
	GENERATED_BODY()

public:
	ATestGrounds();

	/** Takes the player, on foot, to one of the grounds (building them the first time). */
	static void Go(UWorld* World, ETestGround Where);
	/** Takes him back to where he was in the city when he first came out. */
	static void GoBack(UWorld* World);
	/** True while the player is out on the grounds. */
	static bool IsPlayerHere(const UWorld* World);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	static ATestGrounds* Find(const UWorld* World);
	/** Where a spot in the grounds' frame is in the world. */
	FVector ToWorld(const FVector& Local) const;
	/** The same, turned back: a world spot in the grounds' frame. */
	FVector ToLocal(const FVector& World) const;
	bool IsOnGrounds(const FVector& World) const;
	bool IsInFightYard(const FVector& World) const;

	void Arrive(APestiPerson* Person, APlayerController* PC, ETestGround Where);
	void ReadKeys(APestiPerson* Person, APlayerController* PC);
	/** The fighters of set 1 to 4 (see the class comment), after the last ones are cleared away. */
	void SpawnFoes(APestiPerson* Person, int32 Set);
	void ClearFoes(APestiPerson* Person);
	/** Who goes for him, who waits round him, whom he faces: as in the missions' group fights. */
	void TickFight(APestiPerson* Person);
	/** Fresh vehicles in the yard; the one he is driving stays. */
	void SpawnVehicles();
	void UpdateLamps();
	void AddShape(int32 Shape, const FLinearColor& Color, const FVector& At, const FVector& Size, const FRotator& Turn, bool bCollide);

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> Shapes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Batches;

	TMap<FString, int32> BatchIndex;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPointLightComponent>> Lamps;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ARuffian>> Foes;

	UPROPERTY(Transient)
	TObjectPtr<AChaseThief> Thief;

	UPROPERTY(Transient)
	TArray<TObjectPtr<APawn>> Vehicles;

	/** Where he was in the city when he first came out, to take him back. */
	FVector CityAt = FVector::ZeroVector;
	double CityYaw = 0.0;
	bool bHasCitySpot = false;

	/** The vehicles are put out on the first tick, once the ground under them is there. */
	bool bVehiclesDue = true;
	/** When the last fighter went down (to say so once), or below 0. */
	double WonAt = -1.0;
	double NextLampUpdate = 0.0;
};
