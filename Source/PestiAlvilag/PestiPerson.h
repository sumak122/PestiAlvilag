// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CarBody.h"
#include "GameFramework/Character.h"
#include "PestiPerson.generated.h"

class UCameraComponent;
class UMaterialInterface;
class USpringArmComponent;
class UStaticMeshComponent;

/**
 * The player on foot, built from engine shapes so it needs no art: a man in
 * a long coat and hat, with a GTA style camera behind him.
 *
 * Keys: W/A/S/D or left stick to walk, Shift or left stick click to run,
 * Space or gamepad A to jump, mouse or right stick to look around,
 * F or gamepad B to get into the car next to you.
 */
UCLASS()
class PESTIALVILAG_API APestiPerson : public ACharacter
{
	GENERATED_BODY()

public:
	APestiPerson();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Person")
	float WalkSpeed = 170.f;

	UPROPERTY(EditAnywhere, Category = "Person")
	float RunSpeed = 550.f;

	/** How close a car must be to get in (cm, centre to centre). */
	UPROPERTY(EditAnywhere, Category = "Person")
	float EnterCarRange = 400.f;

	/** Degrees the camera turns per unit of mouse movement. */
	UPROPERTY(EditAnywhere, Category = "Camera")
	float MouseLookScale = 2.5f;

	/** Degrees per second the camera turns with the gamepad's right stick. */
	UPROPERTY(EditAnywhere, Category = "Camera")
	float StickLookRate = 150.f;

	/** Called by a car the player leaves: appear at At if there is room. */
	bool TryStepOutAt(const FVector& At, double Yaw);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Person")
	TArray<TObjectPtr<UStaticMeshComponent>> BodyParts;

	UPROPERTY(VisibleAnywhere, Category = "Person")
	TObjectPtr<USpringArmComponent> CameraArm;

	UPROPERTY(VisibleAnywhere, Category = "Person")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	virtual void BeginPlay() override;

private:
	void Look(APlayerController* PC, float DeltaSeconds);
	void Walk(const APlayerController* PC);
	void SwingLimbs(float DeltaSeconds);
	void TryEnterCar(APlayerController* PC);
	void SetInWorld(bool bInWorld);

	TArray<CarBody::EPaint> BodyPaints;
	float StridePhase = 0.f;
	bool bCameraAimed = false;
	double OutOfCarTime = -1.0;
};
