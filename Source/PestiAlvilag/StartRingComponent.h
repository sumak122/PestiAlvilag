// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "StartRingComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;
class UTexture;
class UTexture2D;

/**
 * Where a story mission waits to begin: a soft column of gold light about
 * 1.7 m high standing in a stepped gold ring on the ground, and over it a
 * small dark diamond with the first letter of whoever gives the mission
 * (B for Bözsi néni). Walking into the ring starts the mission. It glows
 * (unlit, see-through) and turns its light and its letter to the camera.
 * While it shows, the minimap shows the letter there (APestiHUD::MarkStart).
 * It lives in the world, not on its owner.
 */
UCLASS(ClassGroup = (Custom))
class PESTIALVILAG_API UStartRingComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UStartRingComponent();

	/** Stands the ring on the ground at (or just under) At, with this letter over it. */
	void ShowAt(const FVector& At, const FString& Letter);
	void Hide();
	/** Off for a ring that is not a story mission's (it then leaves the minimap's start mark alone). */
	void SetOnMap(bool bInOnMap) { bOnMap = bInOnMap; }
	bool IsShown() const { return bShown; }
	/** The middle of the ring, on the ground. */
	FVector GetSpot() const { return GetComponentLocation(); }
	/** How near its middle counts as inside it (cm). */
	static float Radius();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void Build();
	UMaterialInstanceDynamic* Glow(const FLinearColor& Color, float Opacity, UTexture* Picture = nullptr);
	void AddBar(USceneComponent* Parent, UMaterialInstanceDynamic* Paint, const FVector& At, const FVector& Size, const FRotator& Turn);
	/** A soft spot of light, fading towards every edge, for the column. */
	UTexture2D* MakeSoftTexture();

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> SeeThroughMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UTexture> WhiteTexture;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> SoftTexture;

	/** The column of light and the letter's diamond, which both face the camera. */
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> Column;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> Badge;

	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> LetterText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Bars;

	FString ShownLetter;
	float Time = 0.f;
	bool bShown = false;
	bool bOnMap = true;
};
