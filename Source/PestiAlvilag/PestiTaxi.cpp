// Fill out your copyright notice in the Description page of Project Settings.

#include "PestiTaxi.h"

APestiTaxi::APestiTaxi()
{
	BodyColor = FLinearColor(0.01f, 0.01f, 0.01f);
	SetStyle(CarModel::TaxiStyle());
}

APestiDeluxeTaxi::APestiDeluxeTaxi()
{
	BodyColor = CarModel::DeluxeTaxiStyle().PaintColor;
	SetStyle(CarModel::DeluxeTaxiStyle());
	MaxSpeed = 3300.f;
	Acceleration = 1150.f;
}
