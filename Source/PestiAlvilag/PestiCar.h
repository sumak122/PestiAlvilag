// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CarModel.h"
#include "GameFramework/Pawn.h"
#include "PestiCar.generated.h"

class UBoxComponent;
class UCameraComponent;
class UCarVisuals;
class UCrashSound;
class UEngineSound;
class USpringArmComponent;
class AWeather;

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
 *
 * Each kind of car drives differently (CarHandling: weight, power, top speed,
 * steering, grip). The car's weight shifts: braking into a bend tucks the
 * nose in and lets the back step out, pulling hard makes it run wide. Wet
 * streets grip and brake less. Crashes trade speed between the cars by their
 * weight, and a hit off the middle spins them (CarHandling::Collide).
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

	/** How fast the car slides sideways (cm/s, + is to its right): the tyres pull this back to nothing. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Car")
	float SideSpeed = 0.f;

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

	/**
	 * A parked car can be locked. Getting in then means smashing the driver's
	 * window (quick and loud: police near enough hear it) or picking the lock
	 * (slow and quiet); see APestiPerson::TickBreakIn.
	 */
	void SetLocked(bool bInLocked) { bLocked = bInLocked; }
	bool IsLocked() const { return bLocked; }
	/** The lock is beaten: with a smashed window (glass flies, a crime if the police hear), or quietly picked. */
	void BreakIn(bool bSmash);

	/**
	 * An old car's engine is started by hand at the front (APestiPerson::TickCrank):
	 * whether it can be cranked now (old, engine off, not already turning),
	 * where the person cranking it stands (on the ground plane, facing the
	 * radiator: the car's yaw turned round), and a go at the crank, which
	 * catches or not after a moment.
	 */
	bool CanCrank() const;
	FVector GetCrankSpot() const;
	void Crank();
	bool IsCranking() const { return CrankUntil > 0.0; }

	/** The dents, lost parts and wear of the same car as it was in traffic (stolen from it). */
	void TakeLooksFrom(UCarVisuals* From);

	/** The car's look; set it before the car is spawned (SpawnActorDeferred), or it is a saloon in BodyColor. */
	void SetStyle(const CarModel::FStyle& InStyle)
	{
		Style = InStyle;
		bStyleSet = true;
	}

	/** How many times the car has hit something hard enough to jolt a passenger. */
	int32 GetBumpCount() const { return BumpCount; }

	/** How battered the car is, from 0 (whole) to 1 (wrecked: the engine has died). */
	float GetWear() const;
	bool IsWrecked() const;
	/** Any dent, lost part or wear at all. */
	bool IsDamaged() const;
	/** Laci's garage: mends everything and sprays the body in Paint. */
	void Repaint(const FLinearColor& Paint);
	/** The body's paint now. */
	FLinearColor GetPaint() const;

	/**
	 * Something not moved by the car's own sweep hit it at At, shoving it by
	 * Push (cm/s): a police car ramming it, a road barrier it smashed. Dents,
	 * sparks and the crash sound, and the shove to its speed.
	 */
	void TakeKnock(const FVector& At, const FVector& Push);

	/**
	 * Shoved by another car sliding into it: its speed changes by Push (cm/s),
	 * its spin by Spin (degrees a second, + turns right). DentSpeed above 0
	 * dents it at At and plays the crash, as hard as they met (cm/s).
	 * Works on a parked car too: it slides on its locked wheels to a stop.
	 */
	void Shove(const FVector& Push, float Spin, const FVector& At, float DentSpeed);

	/** How heavy it is (kg), by its kind of car. */
	float GetMass() const { return Mass; }

	/** Stops the car and puts the player who drives it out on foot, whatever its speed (missions that fail). False when he could not get out. */
	bool ForceDriverOut();

	/** A fare on the back seat (taxis), or nobody. */
	void ShowBackSeatPassenger(const PersonKit::FOutfit& Outfit);
	void HideBackSeatPassenger();
	/** Where someone stands to get in or out at a back door, on the side facing Towards (on the ground). */
	FVector GetRearDoor(const FVector& Towards) const;

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
	/** Sets the top speed, pull, brakes, steering, grip and weight for its kind of car (CarHandling). */
	void ApplyHandling();
	/** Nobody driving: still slides on after a shove, on locked wheels, and spins to a stop. */
	void RollDriverless(float DeltaSeconds);
	/** The tyres squeal (or hiss on wet setts) while sliding, and the camera widens with speed. */
	void UpdateSkidAndView(bool bHandbrake, float DeltaSeconds);
	void MoveWithSlide(const FVector& Delta);
	void SnapToGround();
	void ToggleCamera();
	void UpdateFreeLook(const APlayerController* PC, float DeltaSeconds);
	void LetDriverOut(APlayerController* PC);
	void UpdateEngineNoise(float Throttle);
	/** Sparks, glass, dust and the crash sounds for a hit at At, Away from what was hit; Into is the speed straight into it, Along the speed along it (cm/s). */
	void CrashFeedback(const FVector& At, const FVector& Away, float Into, float Along, int32 Knocked, bool bSoft);
	void PlayCrashSound(uint8 Kind, float Strength, float Delay);

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

	/** Locked, until broken into or driven. */
	bool bLocked = false;
	/** An old car (CarModel::IsOld) is started with the crank at the front and stalls now and then: whether it runs, */
	bool bOldEngineOn = false;
	/** the crank turning the engine over until this time and whether it will catch, */
	double CrankUntil = -1.0;
	bool bCrankCatches = false;
	/** and when the last driver got out, or it was cranked into life with nobody in it (a warm engine catches more easily; left alone it idles a while). */
	double DriverOutTime = -1000.0;
	/** The old engine stops, with a cough. */
	void StallEngine();

	/** The headlights, switched with H while driving. */
	bool bHeadlightsOn = false;
	/** The gear the engine is in, from 0. */
	int32 Gear = 0;
	double LastBumpTime = -1.0;
	/** Crash sounds and the scrape, made the first time something is hit. */
	UPROPERTY(Transient)
	TObjectPtr<UCrashSound> CrashNoise;
	double LastCrashTime = -1.0;
	double LastScrapeSparks = -1.0;
	/** Scraping along a wall until this time, this hard (0..1). */
	double ScrapeUntil = -1.0;
	float ScrapeAmount = 0.f;
	/** How fast the car is turning now (degrees a second): follows the wheel with a little lag. */
	float TurnRate = 0.f;
	/** Spin from a crash (degrees a second, + to the right): the tyres slowly stop it. */
	float SpinRate = 0.f;

	/** Its kind of car's handling (see ApplyHandling). */
	float Mass = 1100.f;
	float GripShare = 1.f;
	float HandbrakeGripShare = 1.f;
	float TurnWeight = 7.f;
	float ShiftTime = 0.25f;
	bool bHandlingSet = false;
	/** The pull dips for this long still (s) while the gear changes up. */
	float ShiftPause = 0.f;
	/** Weight on the front wheels (+, braking) or the back ones (-, pulling), -1..1. */
	float WeightShift = 0.f;
	/** How wet the street is, 0..1 (AWeather). */
	float RoadWet = 0.f;
	TWeakObjectPtr<AWeather> WeatherFound;
	/** The tyre squeal now, 0..1, and the chase camera's view angle. */
	float SkidAmount = 0.f;
	float ChaseView = 90.f;
};
