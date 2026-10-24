// Fill out your copyright notice in the Description page of Project Settings.


#include "PestiAlvilagGameModeBase.h"

#include "PestiCar.h"

APestiAlvilagGameModeBase::APestiAlvilagGameModeBase()
{
	DefaultPawnClass = APestiCar::StaticClass();
}

