// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CityAmbience.generated.h"

class UCityAmbienceSound;

/**
 * The background sound of 1932 Józsefváros, so the streets are never dead
 * quiet: one stereo sound (UCityAmbienceSound) that follows the camera. Each
 * frame it works out how near the listener is to the park, the markets, the
 * busy Rákóczi út, Keleti and the church, which side those lie on, and the
 * time of day, and passes that to the sound.
 */
UCLASS()
class PESTIALVILAG_API ACityAmbience : public AActor
{
	GENERATED_BODY()

public:
	ACityAmbience();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UCityAmbienceSound> Sound;

	bool bStarted = false;
};
