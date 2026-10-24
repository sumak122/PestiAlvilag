// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PestiCar.h"
#include "PestiTaxi.generated.h"

/**
 * A company taxi: the player's saloon in black, with a yellow and black
 * chequered band, a roof lamp and the meter flag. Drives like any other car;
 * the taxi company gives fares to whoever drives one on a shift.
 */
UCLASS()
class PESTIALVILAG_API APestiTaxi : public APestiCar
{
	GENERATED_BODY()

public:
	APestiTaxi();
};
