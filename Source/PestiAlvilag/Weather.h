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
 * The streets get wet in the rain and dry slowly after it (shiny cobbles
 * and puddles, GroundSurfaces::SetWet).
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

	/** How wet the streets are, 0 dry to 1 soaked: soaks up in the rain, dries slowly after. */
	float GetWet() const { return Wet; }

	/** For the test page: the weather it is set to, and stepping on to the next. */
	ETestWeather GetTestWeather() const { return Forced; }
	void SetTestWeather(ETestWeather To);

	/**
	 * A roof nothing can bump into (so looking down for it finds nothing),
	 * like the market hall's or Keleti's: no rain is drawn under it.
	 */
	void AddShelter(UProceduralMeshComponent* Roof);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	enum class ESpell : uint8 { Clear, Cloudy, Rain, Fog };

	void NextSpell();
	void Aim(ESpell Spell);
	void UpdateDrops(float DeltaSeconds, const FVector& Eye);

	/** A sheltering roof, drawn down onto a grid of squares: the roof's top over each (far below where there is none). */
	struct FShelter
	{
		TWeakObjectPtr<UProceduralMeshComponent> Roof;
		bool bDrawn = false;
		FVector2D Corner = FVector2D::ZeroVector;
		int32 Across = 0;
		int32 Down = 0;
		TArray<float> Tops;
	};
	void DrawShelter(FShelter& Shelter) const;
	/** Looks down for roofs over a few more of the squares round the camera. */
	void LookForCover(const FVector& Eye);
	/** The top of whatever is over this spot (roof, or the ground itself): rain only shows above it. */
	float CoverOver(const FVector& At) const;

	TArray<FShelter> Shelters;
	/** What was found over each square of the map looked at so far, and which square is looked at next. */
	TMap<FIntPoint, float> CoverSquares;
	int32 CoverTurn = 0;

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
	float Wet = 0.f;
	/** The wetness last given to the ground (below 0: none yet). */
	float WetShown = -1.f;
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
