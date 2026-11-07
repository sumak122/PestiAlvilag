// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "HorseCartModel.h"
#include "HorseCart.generated.h"

class UBoxComponent;
class UCameraComponent;
class UHorseSound;
class UMaterialInterface;
class UProceduralMeshComponent;
class USceneComponent;
class USpringArmComponent;

/**
 * A horse and its flat market cart (HorseCartModel), driven from the bench.
 *
 * The horse has four paces, and W and S move it up and down one at a time
 * (standing, walk, trot, canter, gallop); held at a stand, S backs it slowly. A and
 * D turn it: the horse leads, swinging the shafts and the front wheels with
 * it about the turntable, and the cart follows. Space pulls it up ("Hó!"),
 * F gets down. The horse's legs step with its pace, its head nods, its
 * hooves clop on the cobbles and the iron tyres rumble.
 *
 * The cart carries up to six crates (SetCrates).
 */
UCLASS()
class PESTIALVILAG_API AHorseCart : public APawn
{
	GENERATED_BODY()

public:
	AHorseCart();

	virtual void Tick(float DeltaSeconds) override;
	virtual FVector GetVelocity() const override;

	/** Puts a person on the bench; they get down again with F. */
	virtual void TakeDriver(APawn* InDriver);

	/** One of the city's carriers' carts (ATrafficCart), not Pista bácsi's. */
	virtual bool IsCityCart() const { return false; }
	/** Where to climb up: by the bench (on the ground). */
	FVector GetBenchLocation() const;

	/** Shows this many crates on the bed (0 to six). */
	void SetCrates(int32 Count);
	int32 GetCrates() const { return Crates; }
	/** Where crate Index stands on the bed, in the world. */
	FVector GetCrateSpot(int32 Index) const;

	/** Someone standing at At is close enough to the bed to lift a crate onto it. */
	bool IsByBed(const FVector& At) const;
	/** Behind the tailboard, on the ground: where to bring a crate. */
	FVector GetTailLocation() const;
	/**
	 * A crate that fell off since the last call: where it landed (on the
	 * ground beside the cart). Driving into things at a trot or faster, or
	 * turning hard at a canter, throws one off.
	 */
	bool TakeFallenCrate(FVector& OutAt);

	/** How fast it goes: 0 standing, 1 walk, 2 trot, 3 canter, 4 gallop. */
	int32 GetPace() const { return Pace; }
	float GetSpeed() const { return Speed; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Cart")
	TObjectPtr<UBoxComponent> CollisionBox;

	/** The cart's frame: on the ground under the middle of the bed. */
	UPROPERTY(VisibleAnywhere, Category = "Cart")
	TObjectPtr<USceneComponent> Rig;

	/** The front axle, the shafts and the horse, turning about the turntable. */
	UPROPERTY(VisibleAnywhere, Category = "Cart")
	TObjectPtr<USceneComponent> ForecartPivot;

	/** The horse's frame: on the ground under the middle of its body. */
	UPROPERTY(VisibleAnywhere, Category = "Cart")
	TObjectPtr<USceneComponent> HorseRoot;

	UPROPERTY(VisibleAnywhere, Category = "Cart")
	TObjectPtr<UHorseSound> Sound;

	UPROPERTY(VisibleAnywhere, Category = "Cart")
	TObjectPtr<USpringArmComponent> CameraArm;

	UPROPERTY(VisibleAnywhere, Category = "Cart")
	TObjectPtr<UCameraComponent> ChaseCamera;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> HorseParts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> CartParts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> CrateMeshes;

	/** A carrier's load on the bed (churns, sacks, barrels), one mesh each. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> LoadMeshes;

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> DriverMesh;

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> Reins;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	virtual void BeginPlay() override;

	// How it is built; a city cart sets these before it is spawned (deferred).
	HorseCartModel::ECoat HorseCoat = HorseCartModel::ECoat::Bay;
	bool bBlaze = true;
	HorseCartModel::ELoad CartLoad = HorseCartModel::ELoad::None;
	/** Who sits on the bench when nobody drives it (bOwnDriverLook), else the player's look. */
	PersonKit::FOutfit DriverLook;
	bool bOwnDriverLook = false;
	/** Its pieces are not drawn beyond this (cm; 0 always). */
	float DrawDistance = 0.f;
	/** The sound is left to be started by whoever moves it (a city cart, only when near). */
	bool bSoundWhenNear = false;

	/** Seats this person on the bench (who is seen there while it is driven). */
	void DressDriver(const PersonKit::FOutfit& Look);

	UProceduralMeshComponent* MakeMesh(USceneComponent* Parent, const PersonKit::FPiece& Piece, const FVector& At);
	void Fill(UProceduralMeshComponent* Mesh, const TArray<PersonKit::FSection>& Sections);
	void Drive(APlayerController* PC, float DeltaSeconds);
	void Move(float DeltaSeconds);
	void Animate(float DeltaSeconds);
	void UpdateReins();
	void UpdateFreeLook(const APlayerController* PC, float DeltaSeconds);
	void LetDriverOut(APlayerController* PC);
	void SnapToGround();
	/** One crate off the bed and onto the road beside it. */
	void DropCrate();

	/** The pace asked for, and the speed (cm/s, below 0 backing). */
	int32 Pace = 0;
	float Speed = 0.f;
	/** How far the shafts are turned (degrees, right positive). */
	float SteerAngle = 0.f;
	/** How far into its stride the horse is (0..1), and which hooves were down. */
	float Stride = 0.f;
	bool WasPlanted[4] = { true, true, true, true };
	float Idle = 0.f;
	double NextSnort = 0.0;
	float WheelTurn = 0.f;
	int32 Crates = 0;
	/** How much the load has been thrown about by hard turns at a canter. */
	float Sway = 0.f;
	double LastSpill = -10.0;
	TArray<FVector> Fallen;

	float LookYaw = 0.f;
	float LookPitch = 0.f;
	float TimeSinceLook = 0.f;

	UPROPERTY()
	TObjectPtr<APawn> Driver;
	double DriverInTime = 0.0;
};
