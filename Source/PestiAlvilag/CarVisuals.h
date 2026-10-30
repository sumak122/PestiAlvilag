// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CarModel.h"
#include "PersonKit.h"
#include "Components/ActorComponent.h"
#include "CarVisuals.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UProceduralMeshComponent;
class USceneComponent;
class USpotLightComponent;
class UTexture;
class UEngineSound;

/**
 * What a car looks like (see CarModel) and how it moves: the body, four
 * wheels that roll with the distance the car covers and front wheels that
 * turn into corners, a driver at the wheel, and faint puffs of smoke from
 * the tail pipe when the car pulls away hard, headlights at night and brake
 * lights whenever it slows down. It works out the car's movement from where the
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

	/** Sits this person at the wheel (the player, say). */
	void ShowDriver(const PersonKit::FOutfit& Outfit);

	/**
	 * Sits one of the town's drivers at the wheel: Look picks which one, and
	 * taxis and police cars get their own. The same look is made only once.
	 */
	void ShowStreetDriver(int32 Look);

	/** Nobody at the wheel. */
	void HideDriver();

	/** Sits this person on the back seat (a taxi's fare), or empties it. */
	void ShowPassenger(const PersonKit::FOutfit& Outfit);
	void HidePassenger();

	/** How many different street drivers there are (ShowStreetDriver's Look). */
	static constexpr int32 NumDriverLooks = 24;

	/** For a car someone drives: how far the wheel is turned (-1 left to 1 right), instead of guessing it from the turning. */
	void SetSteer(float InSteer)
	{
		DriverSteer = InSteer;
		bDriverSteers = true;
	}

	/** For a car someone drives: how hard the pedal is down (0 to 1); smoke when pulling away. */
	void SetThrottle(float InThrottle) { DriverThrottle = InThrottle; }

	/** A parked car with nobody in it is quiet: its wheels stay put and it makes no smoke. */
	void SetEngineRunning(bool bRunning) { bEngineRunning = bRunning; }

	/**
	 * For traffic: the engine can be heard when the camera comes near
	 * (only the nearest few cars at a time). The player's car runs its own.
	 */
	void SetStreetEngine(bool bOn) { bStreetEngine = bOn; }

	/**
	 * Headlights on or off by hand (the player's car). Until this is called
	 * they come on by themselves at dusk while the engine runs.
	 */
	void SetHeadlights(bool bOn)
	{
		bHeadlightsByHand = true;
		bHeadlightsWanted = bOn;
	}

	bool AreHeadlightsOn() const { return bHeadlights; }

	/** For a car someone drives: whether they are braking, instead of guessing it from the slowing down. */
	void SetBraking(bool bBraking) { DriverBrake = bBraking ? 1 : 0; }

	/** A bang from the tail pipe: a big dark puff. */
	void Backfire();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void Fill(UProceduralMeshComponent* Mesh, const CarModel::FBuilder& Shape);
	void Puff(float Size, bool bDark);
	void FillPerson(TObjectPtr<UProceduralMeshComponent>& Person, const TArray<PersonKit::FSection>& Sections);
	static void HidePerson(UProceduralMeshComponent* Person);
	void UpdateStreetEngine(double Distance, float Speed, float Pickup);
	void StopStreetEngine();
	void UpdateLights(float DeltaTime, float Speed, float Pickup, double Distance);
	void ShowBeams(bool bShow);

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> Body;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> Wheels;

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> Driver;

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> Passenger;

	UPROPERTY(Transient)
	TObjectPtr<UEngineSound> StreetEngine;

	/** The lamps' own paint, and how they look lit. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> LampPaint;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> TailPaint;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LampGlow;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> TailGlow;

	/** The headlights' beams, near the camera only. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USpotLightComponent>> Beams;

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
	bool bStreetEngine = false;
	bool bStreetEngineHeard = false;
	float PuffTimer = 0.f;

	/** Where the two headlamp lenses are, on the body. */
	FVector LampAt[2] = { FVector::ZeroVector, FVector::ZeroVector };
	bool bHeadlightsByHand = false;
	bool bHeadlightsWanted = false;
	bool bHeadlights = false;
	int32 DriverBrake = -1;
	float BrakeHold = 0.f;
	bool bHeldOnBrake = false;
	bool bBeamsShown = false;
	/** What the lamps show now: off (-1 unknown, 0 off, 1 lit, 2 braking) and at which darkness they were last tinted. */
	int32 LampState = -1;
	int32 TailState = -1;
	float TintDarkness = -1.f;
};
