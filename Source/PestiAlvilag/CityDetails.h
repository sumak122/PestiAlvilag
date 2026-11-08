// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CityDetails.generated.h"

class AShapeSet;
class FJsonObject;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

/**
 * The city's smaller details from Data/OsmGraybox/city_details.json
 * (Tools/OsmGraybox/city_details.py), put up as shape sets
 * (CityDetailsModel): the Corvin department store's front on Blaha Lujza
 * ter, the open gateways and the courtyards behind them, the painted
 * adverts on the fire walls, the newspaper kiosks and the street clocks.
 * The clocks' hands follow the game's time (ADayNight). It also sends the
 * maps their little pictures of places (APestiHUD::SetMapIcons).
 *
 * The houses the gateways run through are cut by AOsmGraybox from the same
 * file; AStreetFurniture leaves those gates standing open (IsOpenGate).
 */
UCLASS()
class PESTIALVILAG_API ACityDetails : public AActor
{
	GENERATED_BODY()

public:
	ACityDetails();

	virtual void Tick(float DeltaSeconds) override;

	/** True when the tenement gate whose foot is at Foot stands open onto a courtyard. */
	static bool IsOpenGate(const FVector2D& Foot);

protected:
	virtual void BeginPlay() override;

private:
	static TSharedPtr<FJsonObject> LoadDetails();
	double GroundAt(const FVector2D& At) const;
	AShapeSet* PutUp(const FVector& At, double SetYaw, float DrawDistance) const;
	void BuildCorvin();
	void BuildCourtyards(const FJsonObject& Root);
	void BuildAdverts(const FJsonObject& Root);
	void BuildKiosks(const FJsonObject& Root);
	void BuildClocks(const FJsonObject& Root);
	void ReadMapIcons(const FJsonObject& Root);
	void SetHands(float Hour);

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	/** All the clocks' hands, as cubes placed in the city. */
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> Hands;

	/** Where each clock stands. */
	TArray<FTransform> ClockSpots;
	float HandsCheckIn = 0.f;
	float ShownHour = -100.f;

	/** The maps' little pictures, sent once the HUD is there. */
	TArray<FVector2D> IconSpots;
	TArray<uint8> IconKinds;
	bool bIconsSent = false;
};
