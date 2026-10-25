// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CarModel.h"
#include "GameFramework/Pawn.h"
#include "PestiCar.generated.h"

class UBoxComponent;
class UCameraComponent;
class UCarVisuals;
class UEngineSound;
class USpringArmComponent;

/**
 * A simple arcade car for the feasibility prototype, built from engine shapes
 * so it needs no art. It drives on flat ground, slides along walls, and has a
 * chase camera plus a GTA 1 style overhead camera.
 *
 * Keys: W/S or triggers to drive and brake, A/D or left stick to steer,
 * Space or gamepad A for the handbrake, C or gamepad Y to switch camera,
 * mouse or right stick to look around, F or gamepad B to get out.
 *
 * Ramming traffic makes the police come after you (see the game mode).
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

	UPROPERTY(EditAnywhere, Category = "Car")
	FLinearColor BodyColor = FLinearColor(0.03f, 0.07f, 0.05f);

	/** Keeps the collision box off the flat road layers. */
	UPROPERTY(EditAnywhere, Category = "Car")
	float HoverHeight = 20.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Car")
	float ForwardSpeed = 0.f;

	/** Degrees the camera turns per unit of mouse movement. */
	UPROPERTY(EditAnywhere, Category = "Camera")
	float MouseLookScale = 2.5f;

	/** Degrees per second the camera turns with the gamepad's right stick. */
	UPROPERTY(EditAnywhere, Category = "Camera")
	float StickLookRate = 150.f;

	/** Seconds without looking around before the camera swings back behind the car while driving. */
	UPROPERTY(EditAnywhere, Category = "Camera")
	float LookResetDelay = 2.f;

	virtual FVector GetVelocity() const override;

	/** Puts a person behind the wheel; they get out again with F. */
	void TakeDriver(APawn* InDriver);

	/** The car's look; set it before the car is spawned (SpawnActorDeferred), or it is a saloon in BodyColor. */
	void SetStyle(const CarModel::FStyle& InStyle)
	{
		Style = InStyle;
		bStyleSet = true;
	}

	/** How many times the car has hit something hard enough to jolt a passenger. */
	int32 GetBumpCount() const { return BumpCount; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Car")
	TObjectPtr<UBoxComponent> CollisionBox;

	/** The body and wheels (see CarModel), turning wheels and exhaust smoke. */
	UPROPERTY(VisibleAnywhere, Category = "Car")
	TObjectPtr<UCarVisuals> Visuals;

	/** The engine's putter while someone drives. */
	UPROPERTY(VisibleAnywhere, Category = "Car")
	TObjectPtr<UEngineSound> EngineNoise;

	UPROPERTY(VisibleAnywhere, Category = "Car")
	TObjectPtr<USpringArmComponent> CameraArm;

	UPROPERTY(VisibleAnywhere, Category = "Car")
	TObjectPtr<UCameraComponent> ChaseCamera;

	CarModel::FStyle Style;
	bool bStyleSet = false;

	virtual void BeginPlay() override;

private:
	void UpdateSpeed(float Throttle, bool bHandbrake, float DeltaSeconds);
	void MoveWithSlide(const FVector& Delta);
	void SnapToGround();
	void ToggleCamera();
	void UpdateFreeLook(const APlayerController* PC, float DeltaSeconds);
	void LetDriverOut(APlayerController* PC);
	void UpdateEngineNoise(float Throttle);

	bool bOverheadCamera = false;
	// Free look: the camera's turn away from straight behind the car (degrees).
	float LookYaw = 0.f;
	float LookPitch = 0.f;
	float TimeSinceLook = 0.f;
	// The person who got in, waiting hidden until they get out.
	UPROPERTY()
	TObjectPtr<APawn> Driver;
	double DriverInTime = 0.0;
	int32 BumpCount = 0;
	float LastPedal = 0.f;
	/** The gear the engine is in, from 0. */
	int32 Gear = 0;
	double LastBumpTime = -1.0;
};
