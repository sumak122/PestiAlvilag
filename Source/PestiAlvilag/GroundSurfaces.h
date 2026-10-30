// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SurfaceModel.h"

class UMaterialInstanceDynamic;

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
}
