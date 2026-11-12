// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SurfaceModel.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;

/**
 * Materials for the ground: cobbles, big setts, paving slabs, gravel and
 * autumn grass (the pictures come from SurfaceModel). The pictures are laid
 * out by world position, not by the mesh, so a road made of many pieces
 * shows one unbroken pavement and nothing needs texture coordinates.
 *
 * The pictures are drawn once when first asked for (about half a second)
 * and shared by everyone after that. The material itself is put together
 * by the editor, so this only works when playing from the editor; anywhere
 * else Make gives nullptr and the ground keeps its plain colours.
 */
namespace GroundSurfaces
{
	/** A material showing Kind, or nullptr if it can't be made here. */
	UMaterialInstanceDynamic* Make(SurfaceModel::EKind Kind, UObject* Outer);

	/**
	 * Which ground a material made by Make shows (for the footsteps); false
	 * for any other material, or none.
	 */
	bool KindOf(const UMaterialInterface* Material, SurfaceModel::EKind& OutKind);

	/**
	 * How wet the ground is, 0 dry to 1 soaked (from AWeather): every ground
	 * material made by Make turns darker and shinier, and puddles spread in
	 * its hollows. Materials made later start as wet as this.
	 */
	void SetWet(float Wetness);

	/**
	 * The glow a street lamp throws on the ground, for a flat square
	 * LightPoolRadius cm from its middle to each side, with texture
	 * coordinates 0..1 across it. Set the vector "Light" to the brightness
	 * in its middle, and "FadeFrom" / "FadeLength" (cm) to where it fades
	 * in with the distance from the camera. nullptr where it can't be made.
	 */
	UMaterialInstanceDynamic* MakeLightPool(UObject* Outer);

	/** Half the width of a light pool's square (cm). */
	constexpr double LightPoolRadius = 1500.0;
}
