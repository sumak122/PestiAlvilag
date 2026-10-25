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

/**
 * The company's best car, lent to its regulars once they have driven 20
 * fares: a long red limousine with a gold chequered band and gold wire
 * wheels, faster than the other taxis. It works as a taxi like any other.
 */
UCLASS()
class PESTIALVILAG_API APestiDeluxeTaxi : public APestiTaxi
{
	GENERATED_BODY()

public:
	APestiDeluxeTaxi();
};
