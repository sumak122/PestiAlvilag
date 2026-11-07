// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weather.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;
class URainSound;
class UTexture;

/** What the test page can set the weather to; Natural lets it come and go by itself. */
enum class ETestWeather : uint8 { Natural, Clear, Rain, Fog, Count };

/**
 * The weather over Pest: spells of clear sky, grey cloud, rain and fog that
 * come and go every few minutes, fog more often early and late in the day.
 * Cloud dims the sun and greys the haze, fog thickens it (both through
 * ADayNight::SetWeather); rain falls as thin streaks all round the camera,
 * one mesh like AExhaustSmoke, with its hiss and patter (URainSound).
 */
UCLASS()
class PESTIALVILAG_API AWeather : public AActor
{
	GENERATED_BODY()

public:
	AWeather();

	/** The world's weather, made on first use. */
	static AWeather* Get(UWorld* World);

	/** 0 dry to 1 pouring. */
	float GetRain() const { return Rain; }

	/** For the test page: the weather it is set to, and stepping on to the next. */
	ETestWeather GetTestWeather() const { return Forced; }
	void SetTestWeather(ETestWeather To);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	enum class ESpell : uint8 { Clear, Cloudy, Rain, Fog };

	void NextSpell();
	void Aim(ESpell Spell);
	void UpdateDrops(float DeltaSeconds, const FVector& Eye);

	UPROPERTY(VisibleAnywhere, Category = "Weather")
	TObjectPtr<UProceduralMeshComponent> Mesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> SeeThroughMaterial;

	UPROPERTY()
	TObjectPtr<UTexture> WhiteTexture;

	UPROPERTY(Transient)
	TObjectPtr<URainSound> RainNoise;

	/** The weather now, and what it is heading for. */
	float Overcast = 0.f;
	float Mist = 0.f;
	float Rain = 0.f;
	float AimOvercast = 0.f;
	float AimMist = 0.f;
	float AimRain = 0.f;
	/** Seconds left of this spell. */
	float SpellLeft = 0.f;
	ETestWeather Forced = ETestWeather::Natural;

	/** The raindrops, in the world, and the box round the camera they fall through. */
	TArray<FVector> Drops;
	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	bool bDropsShown = false;
	FVector LastEye = FVector::ZeroVector;
	FRandomStream Random;
};
