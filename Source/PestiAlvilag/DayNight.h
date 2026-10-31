// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"
#include "DayNight.generated.h"

class ADirectionalLight;
class APostProcessVolume;
class ASkyLight;
class AExponentialHeightFog;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class UTexture;

/** Something that lights up at night: one material slot of a mesh, and how it glows. */
USTRUCT()
struct FNightGlow
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<UMeshComponent> Mesh;

	int32 Slot = 0;

	/** What the slot wears by day (the mesh's own material). */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> Day;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> Glow;

	FLinearColor Color = FLinearColor::White;
	/** How bright it glows, in steps of a lit grey wall at night. */
	float Strength = 1.f;
};

/** An unlit see-through material (glass, smoke) and its daytime tint. */
USTRUCT()
struct FSeeThrough
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<UMaterialInstanceDynamic> Material;

	FLinearColor Tint = FLinearColor::White;
};

/**
 * The clock, and the sun and moon that go with it. A whole day goes by in
 * 48 minutes of play: the sun climbs from the east, sets in the west, the
 * sky turns orange and then dark, and the moon takes over. At dusk the gas
 * lamps, house windows, shop windows and trams light up (see AddNightGlow
 * and AddLamp); the nearest lamps also light the pavement around them.
 *
 * It works with whatever sun, sky light and fog the level has, and adds a
 * sky if there is none. Brightness is worked out from the level's own sun,
 * so the daytime look stays as it was.
 */
UCLASS()
class PESTIALVILAG_API ADayNight : public AActor
{
	GENERATED_BODY()

public:
	ADayNight();

	/** The one in this world (made the first time it is asked for). */
	static ADayNight* Get(UWorld* World);

	/** The time of day in hours, 0 to 24. */
	float GetHour() const { return Hour; }

	/** "18:40" */
	FString GetClockText() const;

	/** 0 in daylight, 1 at night, in between at dusk and dawn. */
	float GetDarkness() const { return Darkness; }

	/** Whether the street lamps are lit. */
	bool AreLampsLit() const { return bLampsLit; }

	/** Moves the clock on an hour. */
	void SkipHour();

	/**
	 * Makes a material slot glow at night: from dusk it shows Color, Strength
	 * times as bright as a lit grey wall, and goes back to its own material
	 * in the morning.
	 */
	void AddNightGlow(UMeshComponent* Mesh, int32 Slot, const FLinearColor& Color, float Strength);

	/** A street lamp's lantern: the nearest ones light the ground at night. */
	void AddLamp(const FVector& At) { Lamps.Add(At); }

	/**
	 * An unlit see-through material (window glass, smoke): it gives off its
	 * own colour, so at night it would shine white. This darkens its Tint
	 * (the daytime colour and opacity) with the night, as if lit like the rest.
	 */
	void AddSeeThrough(UMaterialInstanceDynamic* Material, const FLinearColor& Tint);

	/** A glowing material (see AddNightGlow) for someone who switches it on and off themselves, like a car's lamps. */
	UMaterialInstanceDynamic* MakeGlowMaterial(UObject* Outer) const;

	/** Sets a glowing material from MakeGlowMaterial to Color, Strength times as bright as a lit grey wall just now. */
	void SetGlow(UMaterialInstanceDynamic* Glow, const FLinearColor& Color, float Strength) const;

	/** How strong a lamp has to be (candela) to light what it shines on Strength times as bright as a lit grey wall at night, 1 m away. */
	float LampCandela(float Strength) const;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	void FindSky();
	void UpdateSun(bool bForce);
	void UpdateGlows(bool bForce);
	void UpdateLampLights();
	void UpdateSeeThrough(bool bForce);
	void ShadeSeeThrough(const FSeeThrough& Glass) const;
	float SunElevation(float AtHour) const;

	/** How bright a lit grey wall looks now (the scale of SetGlow). */
	float GreyLuminance() const;

	UPROPERTY(Transient)
	TObjectPtr<ADirectionalLight> Sun;

	UPROPERTY(Transient)
	TObjectPtr<ASkyLight> Sky;

	UPROPERTY(Transient)
	TObjectPtr<AExponentialHeightFog> Fog;

	UPROPERTY(Transient)
	TObjectPtr<APostProcessVolume> NightExposure;

	UPROPERTY(Transient)
	TArray<FNightGlow> Glows;

	UPROPERTY(Transient)
	TArray<FSeeThrough> SeeThroughs;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPointLightComponent>> LampLights;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> GlowBase;

	UPROPERTY()
	TObjectPtr<UTexture> WhiteTexture;

	TArray<FVector> Lamps;

	float Hour = 16.f;
	float Darkness = 0.f;
	bool bLampsLit = false;

	/** The level's own sun (lux), sky light and fog, as they were at the start. */
	float DaySunIntensity = 10.f;
	FLinearColor DaySunColor = FLinearColor::White;
	float DaySkyIntensity = 1.f;
	FLinearColor DayFogColor = FLinearColor::White;

	FRotator LastSunRotation = FRotator::ZeroRotator;
	float LastGlowDarkness = -1.f;
	float LastShadeDarkness = -1.f;
	float LampTimer = 0.f;
	float GlowTimer = 0.f;
};
