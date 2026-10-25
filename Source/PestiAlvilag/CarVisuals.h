// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CarModel.h"
#include "Components/ActorComponent.h"
#include "CarVisuals.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;
class USceneComponent;
class UTexture;

/**
 * What a car looks like (see CarModel) and how it moves: the body, four
 * wheels that roll with the distance the car covers and front wheels that
 * turn into corners, and little puffs of smoke from the tail pipe (more
 * when the car pulls away). It works out the car's movement from where the
 * car was last frame, so it works the same for the player's car, traffic
 * and the police.
 */
UCLASS()
class PESTIALVILAG_API UCarVisuals : public UActorComponent
{
	GENERATED_BODY()

public:
	UCarVisuals();

	/** Builds (or rebuilds) the car under Parent, whose origin is the middle of the car's collision box. */
	void Show(USceneComponent* Parent, const CarModel::FStyle& InStyle);

	const CarModel::FStyle& GetStyle() const { return Style; }

	/** For a car someone drives: how far the wheel is turned (-1 left to 1 right), instead of guessing it from the turning. */
	void SetSteer(float InSteer)
	{
		DriverSteer = InSteer;
		bDriverSteers = true;
	}

	/** For a car someone drives: how hard the pedal is down (0 to 1); more smoke. */
	void SetThrottle(float InThrottle) { DriverThrottle = InThrottle; }

	/** A parked car with nobody in it is quiet: its wheels stay put and it makes no smoke. */
	void SetEngineRunning(bool bRunning) { bEngineRunning = bRunning; }

	/** A bang from the tail pipe: a big dark puff. */
	void Backfire();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void Fill(UProceduralMeshComponent* Mesh, const CarModel::FBuilder& Shape);
	void Puff(float Size, bool bDark);

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> Body;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> Wheels;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> SeeThroughMaterial;

	UPROPERTY()
	TObjectPtr<UTexture> WhiteTexture;

	CarModel::FStyle Style;
	bool bShown = false;

	FVector LastLocation = FVector::ZeroVector;
	float LastYaw = 0.f;
	float LastSpeed = 0.f;
	bool bMoved = false;

	/** Wheel roll (degrees) and front wheel turn (degrees, + is right). */
	float Spin = 0.f;
	float SteerAngle = 0.f;

	float DriverSteer = 0.f;
	bool bDriverSteers = false;
	float DriverThrottle = -1.f;

	bool bEngineRunning = true;
	float PuffTimer = 0.f;
};
