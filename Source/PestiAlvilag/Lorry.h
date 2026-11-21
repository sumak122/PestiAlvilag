// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PestiCar.h"
#include "LorryModel.h"
#include "Lorry.generated.h"

class APedestrian;
class USceneComponent;
class UMaterialInterface;
class UProceduralMeshComponent;

/**
 * A Rába lorry (LorryModel) as a vehicle the player drives: a heavy, slow
 * APestiCar that draws itself with the lorry's looks instead of a car body
 * (UCarVisuals::DrawNothing), its load on the platform. The front wheels
 * turn as it steers. A traffic lorry the player takes becomes one
 * (ATrafficManager::StealCar); parked ones stand by the kerb
 * (ALorryParking), and taking one is a theft: the carrier runs out shouting.
 */
UCLASS()
class PESTIALVILAG_API ALorry : public APestiCar
{
	GENERATED_BODY()

public:
	ALorry();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Its paint (LorryModel::Paint) and load. Before spawning. */
	void SetLook(int32 InPaint, LorryModel::ELoad InLoad);

	/** A carrier's lorry parked in the street: the first drive in it is a theft. */
	void SetOwned(bool bInOwned) { bOwned = bInOwned; }
	/** Has been driven by the player at some time. */
	bool WasTaken() const { return bTaken; }

	/** Paints "M. KIR. POSTA" on both sides of a post van's box: text on Body, whose floor is FloorAt below its middle. */
	static void AddPostLettering(AActor* Van, USceneComponent* Body, double FloorAt);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> Parts;

	/** The player in the cab while he drives. */
	UPROPERTY(Transient)
	TObjectPtr<APedestrian> WheelMan;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

private:
	/** The carrier runs out shouting, the deed is noted, the police told if they saw. */
	void Stolen();
	/** The Royal Post's van taken: the police called at once, the mail sack's money his. */
	void PostTaken();
	/** The player seated at the wheel while he drives, hidden otherwise. */
	void PlaceWheelMan(bool bDriven);

	int32 PaintIndex = 0;
	LorryModel::ELoad Load = LorryModel::ELoad::Crates;
	bool bOwned = false;
	bool bTaken = false;
	/** The ground under the box's middle, in the box's frame (cm). */
	double FloorZ = -90.0;
	float WheelTurn = 0.f;
	float SteerAngle = 0.f;
	float LastYaw = 0.f;
};
