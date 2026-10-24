// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CarBody.h"
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
 * Space or gamepad A for the handbrake, C or gamepad Y to switch camera,
 * P to make the police come after you (for testing).
 *
 * It also keeps the player's wanted level: ramming traffic makes the police
 * come after you; staying out of their sight long enough loses them, and
 * staying slow next to one gets you arrested.
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

	/** Seconds out of police sight that lose them. */
	UPROPERTY(EditAnywhere, Category = "Police")
	float EscapeTime = 20.f;

	/** Seconds slow beside a police car before being arrested. */
	UPROPERTY(EditAnywhere, Category = "Police")
	float ArrestTime = 3.f;

	/** 0 = not wanted; higher means more trouble. */
	int32 GetWantedLevel() const { return WantedLevel; }
	void ReportCrime(int32 CrimeLevel);
	/** Police call these every frame they see the player or are right beside them. */
	void NotifySeenByPolice();
	void NotifyPoliceNear();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Car")
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere, Category = "Car")
	TArray<TObjectPtr<UStaticMeshComponent>> BodyParts;

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
	void UpdateWanted(float DeltaSeconds);
	void ClearWanted(const TCHAR* Message);

	bool bOverheadCamera = false;
	int32 WantedLevel = 0;
	float TimeUnseen = 0.f;
	float ArrestProgress = 0.f;
	double LastPoliceNearTime = -1.0;
	TArray<CarBody::EPaint> BodyPaints;
};
