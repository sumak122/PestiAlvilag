// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Styling/SlateBrush.h"
#include "Lighters.generated.h"

class AStoryPerson;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class USceneComponent;
class UStaticMesh;
class UTexture;
class UTexture2D;

/**
 * The thirty old lighters hidden around Józsefváros (LighterModel): on
 * benches, outside gateways, by tram stop signs, at the foot of advertising
 * columns, on a café table, at the flea market and on a Keleti platform.
 * Each one not yet found lies at its place with a glint over it, and walking
 * up to it picks it up. The game remembers them in the save.
 *
 * L opens the collection cabinet (painted by the HUD): every lighter in its
 * velvet niche, the chosen one large with its story. Finding ten gives
 * three pengő, twenty another three, and all thirty the golden Lucifer.
 *
 * Once "Üzenet" is done, Pista bácsi at the flea market sells a tip for
 * 20 fillér: where the nearest lighter still to be found lies.
 *
 * The game mode spawns one.
 */
UCLASS()
class PESTIALVILAG_API ALighters : public AActor
{
	GENERATED_BODY()

public:
	ALighters();

	virtual void Tick(float DeltaSeconds) override;

	/** The collection cabinet is open, and which niche is chosen (0 to 29). */
	bool IsCabinetOpen() const { return bCabinetOpen; }
	int32 GetSelected() const { return Selected; }
	/** A lighter's picture for the cabinet (30 is the golden one); null if its file is missing. */
	const FSlateBrush* GetPicture(int32 Index) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Puts down the lighters still to find, once the ground under them is there. */
	void PlaceLighters();
	/** The point a lighter lies on, or false while the ground isn't built yet. */
	bool FindRestingPoint(int32 Index, FVector& Out) const;
	void Build(int32 Index, const FVector& At);
	void PickUp(int32 Index);
	void Glint(float DeltaSeconds, const FVector& PlayerAt);
	void LoadPictures();
	void OpenCabinet(bool bOpen);
	void TickCabinet();
	void TickTip(APawn* Player);
	/** Where a lighter lies on the map. */
	FVector2D SpotOnMap(int32 Index) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMesh>> Shapes;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> SeeThroughMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UTexture> WhiteTexture;

	/** The glints share one material, which pulses. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GlintPaint;

	/** A soft light over the nearest lighter, so it shows in the dark too. */
	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> GlintLight;

	/** Each lighter's place in the world (null once found or before it is put down). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> Holders;

	/** Each lighter's glint, which turns to face the camera. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> Glints;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTexture2D>> Pictures;

	TArray<FSlateBrush> PictureBrushes;
	/** Lighters already put down or found. */
	TArray<bool> Done;
	/** Paint by colour, shared by every lighter. */
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UMaterialInstanceDynamic>> Paints;

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Pista;

	bool bCabinetOpen = false;
	int32 Selected = 0;
	float GlintTime = 0.f;
	float PlaceTimer = 0.f;
	/** When the golden lighter turns up and when the telephone rings, after the last one (negative: not now). */
	double LuciferAt = -1.0;
	double RingAt = -1.0;
};
