// Fill out your copyright notice in the Description page of Project Settings.

#include "PestiTaxi.h"

#include "Components/BoxComponent.h"

APestiTaxi::APestiTaxi()
{
	BodyColor = FLinearColor(0.01f, 0.01f, 0.01f);
	CarBody::BuildTaxi(this, CollisionBox, BodyParts, BodyPaints);
}
