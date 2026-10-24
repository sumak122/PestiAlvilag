// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PestiCar.generated.h"

class UBoxComponent;
class UCameraComponent;
class UMaterialInterface;
class USpringArmComponent;
class UStaticMeshComponent;

/**
 * A simple arcade car for the feasibility prototype, built from engine shapes
 * so it needs no art. It drives on flat ground, slides along walls, and has a
 * chase camera plus a GTA 1 style overhead camera.
 *
 * Keys: W/S or triggers to drive and brake, A/D or left stick to steer,
 * Space or gamepad A for the handbrake, C or gamepad Y to switch camera.
 */
UCLASS()
class PESTIALVILAG_API APestiCar : public APawn
{
	GENERATED_BODY()

public:
	APestiCar();

	virtual void Tick(float DeltaSeconds) override;

	/** Top speed in cm/s (2800 is about 100 km/h). */
	UPROPERTY(EditAnywhere, Category = "Car")
	float MaxSpeed = 2800.f;

	UPROPERTY(EditAnywhere, Category = "Car")
	float MaxReverseSpeed = 800.f;

	/** cm/s gained per second at full throttle. */
	UPROPERTY(EditAnywhere, Category = "Car")
	float Acceleration = 900.f;

	UPROPERTY(EditAnywhere, Category = "Car")
	float BrakeDeceleration = 2600.f;

	/** Slowing down when no pedal is pressed. */
	UPROPERTY(EditAnywhere, Category = "Car")
	float RollingDrag = 350.f;

	UPROPERTY(EditAnywhere, Category = "Car")
	float HandbrakeDeceleration = 1800.f;

	/** Degrees per second of turning at good speed. */
	UPROPERTY(EditAnywhere, Category = "Car")
	float MaxTurnRate = 85.f;

	/** Below this speed the car turns more slowly, like a real car. */
	UPROPERTY(EditAnywhere, Category = "Car")
	float FullTurnSpeed = 500.f;

	/** Keeps the collision box off the flat road layers. */
	UPROPERTY(EditAnywhere, Category = "Car")
	float HoverHeight = 20.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Car")
	float ForwardSpeed = 0.f;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Car")
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere, Category = "Car")
	TObjectPtr<UStaticMeshComponent> Chassis;

	UPROPERTY(VisibleAnywhere, Category = "Car")
	TObjectPtr<UStaticMeshComponent> Cabin;

	UPROPERTY(VisibleAnywhere, Category = "Car")
	TArray<TObjectPtr<UStaticMeshComponent>> Wheels;

	UPROPERTY(VisibleAnywhere, Category = "Car")
	TObjectPtr<USpringArmComponent> CameraArm;

	UPROPERTY(VisibleAnywhere, Category = "Car")
	TObjectPtr<UCameraComponent> ChaseCamera;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	virtual void BeginPlay() override;

private:
	void UpdateSpeed(float Throttle, bool bHandbrake, float DeltaSeconds);
	void MoveWithSlide(const FVector& Delta);
	void SnapToGround();
	void ToggleCamera();

	bool bOverheadCamera = false;
};
