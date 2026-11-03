// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"

/**
 * The city plan the minimap and the big map (M) are drawn from: one picture
 * baked in code from the graybox map data (Data/OsmGraybox), with dark
 * ground, cream streets and squares, brown blocks, green parks and thin red
 * tram lines, after a 1930s printed city plan. Also the street names, to
 * say under the minimap where the player is (the 1932 name where it differs
 * from today's).
 */
namespace PestiMap
{
	struct FStreet
	{
		FString Name;
		TArray<FVector2D> Points;
		FBox2D Bounds = FBox2D(ForceInit);
	};

	struct FPlan
	{
		/** The picture, for Slate. */
		FSlateBrush Brush;
		/** The world spot (cm) at the picture's top left corner, and how many cm its side covers. */
		FVector2D Corner = FVector2D::ZeroVector;
		double Side = 1.0;
		TArray<FStreet> Streets;

		/** The name of the street nearest At, or the district's name when no street is near. */
		FString StreetAt(const FVector2D& At) const;
	};

	/** The plan, baked the first time it is asked for; null when the map data is missing. */
	const FPlan* Get();
}
