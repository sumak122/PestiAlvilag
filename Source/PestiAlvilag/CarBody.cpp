// Fill out your copyright notice in the Description page of Project Settings.

#include "CarBody.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace CarBody
{
	enum class EShape : uint8
	{
		Cube,
		Cylinder,
		Sphere,
	};

	struct FPartSpec
	{
		const TCHAR* Name;
		EShape Shape;
		EPaint Paint;
		FVector Location;
		FRotator Rotation;
		FVector Scale;
	};

	// Engine shapes are 100 cm across. X is forward, Y right, Z up.
	const FRotator Upright = FRotator(0.f, 0.f, 0.f);
	const FRotator WheelAxle = FRotator(0.f, 0.f, 90.f);   // cylinder axis along Y
	const FRotator SpareAxle = FRotator(90.f, 0.f, 0.f);   // cylinder axis along X
	const FVector WheelScale = FVector(0.7f, 0.7f, 0.25f);

	const FPartSpec PartSpecs[] = {
		// Body: low chassis, long bonnet in front, tall cabin behind.
		{ TEXT("Chassis"), EShape::Cube, EPaint::Body, FVector(0.f, 0.f, -40.f), Upright, FVector(4.2f, 1.7f, 0.4f) },
		{ TEXT("Bonnet"), EShape::Cube, EPaint::Body, FVector(95.f, 0.f, -5.f), Upright, FVector(1.9f, 1.3f, 0.3f) },
		{ TEXT("Cabin"), EShape::Cube, EPaint::Body, FVector(-75.f, 0.f, 25.f), Upright, FVector(1.9f, 1.55f, 0.9f) },
		{ TEXT("Windows"), EShape::Cube, EPaint::Glass, FVector(-75.f, 0.f, 42.f), Upright, FVector(1.92f, 1.57f, 0.35f) },
		// Front: chrome grille and two round headlamps.
		{ TEXT("Grille"), EShape::Cube, EPaint::Chrome, FVector(192.f, 0.f, -5.f), Upright, FVector(0.08f, 0.9f, 0.5f) },
		{ TEXT("HeadlampLeft"), EShape::Sphere, EPaint::Headlamp, FVector(190.f, -55.f, 12.f), Upright, FVector(0.28f) },
		{ TEXT("HeadlampRight"), EShape::Sphere, EPaint::Headlamp, FVector(190.f, 55.f, 12.f), Upright, FVector(0.28f) },
		// Rear: spare wheel and red tail lamps.
		{ TEXT("SpareWheel"), EShape::Cylinder, EPaint::Tyre, FVector(-216.f, 0.f, -10.f), SpareAxle, FVector(0.6f, 0.6f, 0.12f) },
		{ TEXT("TailLampLeft"), EShape::Cube, EPaint::TailLamp, FVector(-212.f, -65.f, -30.f), Upright, FVector(0.05f, 0.18f, 0.12f) },
		{ TEXT("TailLampRight"), EShape::Cube, EPaint::TailLamp, FVector(-212.f, 65.f, -30.f), Upright, FVector(0.05f, 0.18f, 0.12f) },
		// Wheels.
		{ TEXT("WheelFrontLeft"), EShape::Cylinder, EPaint::Tyre, FVector(130.f, -78.f, -55.f), WheelAxle, WheelScale },
		{ TEXT("WheelFrontRight"), EShape::Cylinder, EPaint::Tyre, FVector(130.f, 78.f, -55.f), WheelAxle, WheelScale },
		{ TEXT("WheelRearLeft"), EShape::Cylinder, EPaint::Tyre, FVector(-130.f, -78.f, -55.f), WheelAxle, WheelScale },
		{ TEXT("WheelRearRight"), EShape::Cylinder, EPaint::Tyre, FVector(-130.f, 78.f, -55.f), WheelAxle, WheelScale },
	};

	// Taxi markings: a chequered band round the cabin at waist height, a
	// roof lamp, and the meter flag on the left (driver's side when keeping left).
	const FPartSpec TaxiSpecs[] = {
		{ TEXT("TaxiBandLeft"), EShape::Cube, EPaint::Accent, FVector(-75.f, -78.5f, 8.f), Upright, FVector(1.9f, 0.02f, 0.16f) },
		{ TEXT("TaxiBandRight"), EShape::Cube, EPaint::Accent, FVector(-75.f, 78.5f, 8.f), Upright, FVector(1.9f, 0.02f, 0.16f) },
		{ TEXT("TaxiCheckL0"), EShape::Cube, EPaint::Tyre, FVector(-160.5f, -79.8f, 12.0f), Upright, FVector(0.19f, 0.01f, 0.08f) },
		{ TEXT("TaxiCheckL1"), EShape::Cube, EPaint::Tyre, FVector(-141.5f, -79.8f, 4.0f), Upright, FVector(0.19f, 0.01f, 0.08f) },
		{ TEXT("TaxiCheckL2"), EShape::Cube, EPaint::Tyre, FVector(-122.5f, -79.8f, 12.0f), Upright, FVector(0.19f, 0.01f, 0.08f) },
		{ TEXT("TaxiCheckL3"), EShape::Cube, EPaint::Tyre, FVector(-103.5f, -79.8f, 4.0f), Upright, FVector(0.19f, 0.01f, 0.08f) },
		{ TEXT("TaxiCheckL4"), EShape::Cube, EPaint::Tyre, FVector(-84.5f, -79.8f, 12.0f), Upright, FVector(0.19f, 0.01f, 0.08f) },
		{ TEXT("TaxiCheckL5"), EShape::Cube, EPaint::Tyre, FVector(-65.5f, -79.8f, 4.0f), Upright, FVector(0.19f, 0.01f, 0.08f) },
		{ TEXT("TaxiCheckL6"), EShape::Cube, EPaint::Tyre, FVector(-46.5f, -79.8f, 12.0f), Upright, FVector(0.19f, 0.01f, 0.08f) },
		{ TEXT("TaxiCheckL7"), EShape::Cube, EPaint::Tyre, FVector(-27.5f, -79.8f, 4.0f), Upright, FVector(0.19f, 0.01f, 0.08f) },
		{ TEXT("TaxiCheckL8"), EShape::Cube, EPaint::Tyre, FVector(-8.5f, -79.8f, 12.0f), Upright, FVector(0.19f, 0.01f, 0.08f) },
		{ TEXT("TaxiCheckL9"), EShape::Cube, EPaint::Tyre, FVector(10.5f, -79.8f, 4.0f), Upright, FVector(0.19f, 0.01f, 0.08f) },
		{ TEXT("TaxiCheckR0"), EShape::Cube, EPaint::Tyre, FVector(-160.5f, 79.8f, 12.0f), Upright, FVector(0.19f, 0.01f, 0.08f) },
		{ TEXT("TaxiCheckR1"), EShape::Cube, EPaint::Tyre, FVector(-141.5f, 79.8f, 4.0f), Upright, FVector(0.19f, 0.01f, 0.08f) },
		{ TEXT("TaxiCheckR2"), EShape::Cube, EPaint::Tyre, FVector(-122.5f, 79.8f, 12.0f), Upright, FVector(0.19f, 0.01f, 0.08f) },
		{ TEXT("TaxiCheckR3"), EShape::Cube, EPaint::Tyre, FVector(-103.5f, 79.8f, 4.0f), Upright, FVector(0.19f, 0.01f, 0.08f) },
		{ TEXT("TaxiCheckR4"), EShape::Cube, EPaint::Tyre, FVector(-84.5f, 79.8f, 12.0f), Upright, FVector(0.19f, 0.01f, 0.08f) },
		{ TEXT("TaxiCheckR5"), EShape::Cube, EPaint::Tyre, FVector(-65.5f, 79.8f, 4.0f), Upright, FVector(0.19f, 0.01f, 0.08f) },
		{ TEXT("TaxiCheckR6"), EShape::Cube, EPaint::Tyre, FVector(-46.5f, 79.8f, 12.0f), Upright, FVector(0.19f, 0.01f, 0.08f) },
		{ TEXT("TaxiCheckR7"), EShape::Cube, EPaint::Tyre, FVector(-27.5f, 79.8f, 4.0f), Upright, FVector(0.19f, 0.01f, 0.08f) },
		{ TEXT("TaxiCheckR8"), EShape::Cube, EPaint::Tyre, FVector(-8.5f, 79.8f, 12.0f), Upright, FVector(0.19f, 0.01f, 0.08f) },
		{ TEXT("TaxiCheckR9"), EShape::Cube, EPaint::Tyre, FVector(10.5f, 79.8f, 4.0f), Upright, FVector(0.19f, 0.01f, 0.08f) },
		{ TEXT("TaxiSign"), EShape::Cube, EPaint::Accent, FVector(-40.f, 0.f, 79.f), Upright, FVector(0.15f, 0.55f, 0.18f) },
		{ TEXT("TaxiMeterPole"), EShape::Cube, EPaint::Chrome, FVector(100.f, -72.f, 20.f), Upright, FVector(0.03f, 0.03f, 0.4f) },
		{ TEXT("TaxiMeterFlag"), EShape::Cube, EPaint::TailLamp, FVector(100.f, -72.f, 42.f), Upright, FVector(0.02f, 0.14f, 0.08f) },
	};

	// A two-axle Budapest tram of the period, 10 m long: yellow body, a band
	// of windows, grey roof with a raised clerestory and a bow collector.
	// Ground is 165 cm below the centre. Both ends look alike, as the cars
	// were driven from either end.
	const FVector TramWheelScale = FVector(0.8f, 0.8f, 0.15f);
	const FPartSpec TramSpecs[] = {
		{ TEXT("Underframe"), EShape::Cube, EPaint::Tyre, FVector(0.f, 0.f, -110.f), Upright, FVector(9.4f, 1.9f, 0.3f) },
		{ TEXT("LowerBody"), EShape::Cube, EPaint::Body, FVector(0.f, 0.f, -40.f), Upright, FVector(10.f, 2.2f, 1.1f) },
		{ TEXT("Windows"), EShape::Cube, EPaint::Glass, FVector(0.f, 0.f, 55.f), Upright, FVector(9.8f, 2.22f, 0.8f) },
		{ TEXT("Roof"), EShape::Cube, EPaint::Chrome, FVector(0.f, 0.f, 105.f), Upright, FVector(10.f, 2.2f, 0.2f) },
		{ TEXT("Clerestory"), EShape::Cube, EPaint::Chrome, FVector(0.f, 0.f, 125.f), Upright, FVector(7.f, 1.2f, 0.2f) },
		{ TEXT("CollectorArm"), EShape::Cube, EPaint::Tyre, FVector(0.f, 0.f, 160.f), Upright, FVector(0.05f, 0.05f, 0.5f) },
		{ TEXT("CollectorBow"), EShape::Cube, EPaint::Chrome, FVector(0.f, 0.f, 185.f), Upright, FVector(0.1f, 1.4f, 0.05f) },
		{ TEXT("HeadlampFront"), EShape::Sphere, EPaint::Headlamp, FVector(502.f, 0.f, -30.f), Upright, FVector(0.25f) },
		{ TEXT("HeadlampRear"), EShape::Sphere, EPaint::Headlamp, FVector(-502.f, 0.f, -30.f), Upright, FVector(0.25f) },
		{ TEXT("WheelFrontLeft"), EShape::Cylinder, EPaint::Tyre, FVector(250.f, -80.f, -125.f), WheelAxle, TramWheelScale },
		{ TEXT("WheelFrontRight"), EShape::Cylinder, EPaint::Tyre, FVector(250.f, 80.f, -125.f), WheelAxle, TramWheelScale },
		{ TEXT("WheelRearLeft"), EShape::Cylinder, EPaint::Tyre, FVector(-250.f, -80.f, -125.f), WheelAxle, TramWheelScale },
		{ TEXT("WheelRearRight"), EShape::Cylinder, EPaint::Tyre, FVector(-250.f, 80.f, -125.f), WheelAxle, TramWheelScale },
	};

	// A Pest man of 1932: long coat, dark trousers, hat. The nose shows which
	// way he faces. Ground is 90 cm below the centre.
	const FPartSpec PersonSpecs[] = {
		{ TEXT("LegLeft"), EShape::Cube, EPaint::Trousers, FVector(0.f, -11.f, -48.f), Upright, FVector(0.16f, 0.15f, 0.84f) },
		{ TEXT("LegRight"), EShape::Cube, EPaint::Trousers, FVector(0.f, 11.f, -48.f), Upright, FVector(0.16f, 0.15f, 0.84f) },
		{ TEXT("ArmLeft"), EShape::Cube, EPaint::Body, FVector(0.f, -29.f, 22.f), Upright, FVector(0.12f, 0.11f, 0.62f) },
		{ TEXT("ArmRight"), EShape::Cube, EPaint::Body, FVector(0.f, 29.f, 22.f), Upright, FVector(0.12f, 0.11f, 0.62f) },
		{ TEXT("Coat"), EShape::Cube, EPaint::Body, FVector(0.f, 0.f, 12.f), Upright, FVector(0.3f, 0.46f, 0.84f) },
		{ TEXT("Collar"), EShape::Cube, EPaint::Body, FVector(0.f, 0.f, 54.f), Upright, FVector(0.22f, 0.3f, 0.06f) },
		{ TEXT("Head"), EShape::Sphere, EPaint::Skin, FVector(0.f, 0.f, 70.f), Upright, FVector(0.22f, 0.2f, 0.25f) },
		{ TEXT("Nose"), EShape::Cube, EPaint::Skin, FVector(11.f, 0.f, 69.f), Upright, FVector(0.05f, 0.04f, 0.06f) },
		{ TEXT("HatBrim"), EShape::Cylinder, EPaint::Tyre, FVector(0.f, 0.f, 81.f), Upright, FVector(0.36f, 0.36f, 0.03f) },
		{ TEXT("HatCrown"), EShape::Cylinder, EPaint::Tyre, FVector(0.f, 0.f, 88.f), Upright, FVector(0.22f, 0.22f, 0.13f) },
	};

	FLinearColor PaintColor(EPaint Slot, const FLinearColor& BodyColor)
	{
		switch (Slot)
		{
		case EPaint::Tyre: return FLinearColor(0.01f, 0.01f, 0.01f);
		case EPaint::Chrome: return FLinearColor(0.6f, 0.6f, 0.62f);
		case EPaint::Glass: return FLinearColor(0.02f, 0.03f, 0.05f);
		case EPaint::Headlamp: return FLinearColor(1.f, 0.9f, 0.6f);
		case EPaint::TailLamp: return FLinearColor(0.7f, 0.02f, 0.02f);
		case EPaint::Skin: return FLinearColor(0.75f, 0.5f, 0.38f);
		case EPaint::Trousers: return FLinearColor(0.03f, 0.03f, 0.04f);
		case EPaint::Accent: return FLinearColor(0.85f, 0.62f, 0.05f);
		default: return BodyColor;
		}
	}

	void BuildParts(TConstArrayView<FPartSpec> Specs, AActor* Owner, USceneComponent* Parent,
		TArray<TObjectPtr<UStaticMeshComponent>>& OutParts, TArray<EPaint>& OutPaints)
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
		static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

		for (const FPartSpec& Spec : Specs)
		{
			UStaticMesh* Shape = Spec.Shape == EShape::Cube ? CubeMesh.Object
				: Spec.Shape == EShape::Cylinder ? CylinderMesh.Object
				: SphereMesh.Object;

			UStaticMeshComponent* Part = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Spec.Name);
			Part->SetupAttachment(Parent);
			Part->SetStaticMesh(Shape);
			Part->SetRelativeLocationAndRotation(Spec.Location, Spec.Rotation);
			Part->SetRelativeScale3D(Spec.Scale);
			Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			OutParts.Add(Part);
			OutPaints.Add(Spec.Paint);
		}
	}

	void Build(AActor* Owner, USceneComponent* Parent, TArray<TObjectPtr<UStaticMeshComponent>>& OutParts, TArray<EPaint>& OutPaints)
	{
		BuildParts(PartSpecs, Owner, Parent, OutParts, OutPaints);
	}

	void BuildTaxi(AActor* Owner, USceneComponent* Parent, TArray<TObjectPtr<UStaticMeshComponent>>& OutParts, TArray<EPaint>& OutPaints)
	{
		BuildParts(TaxiSpecs, Owner, Parent, OutParts, OutPaints);
	}

	void BuildTram(AActor* Owner, USceneComponent* Parent, TArray<TObjectPtr<UStaticMeshComponent>>& OutParts, TArray<EPaint>& OutPaints)
	{
		BuildParts(TramSpecs, Owner, Parent, OutParts, OutPaints);
	}

	void BuildPerson(AActor* Owner, USceneComponent* Parent, TArray<TObjectPtr<UStaticMeshComponent>>& OutParts, TArray<EPaint>& OutPaints)
	{
		BuildParts(PersonSpecs, Owner, Parent, OutParts, OutPaints);
	}

	UMaterialInterface* LoadPaintMaterial()
	{
		static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		return BasicShapeMaterial.Object;
	}

	void Paint(AActor* Owner, UMaterialInterface* BaseMaterial, const TArray<TObjectPtr<UStaticMeshComponent>>& Parts,
		const TArray<EPaint>& Paints, const FLinearColor& BodyColor)
	{
		if (!BaseMaterial)
		{
			return;
		}
		for (int32 i = 0; i < Parts.Num() && i < Paints.Num(); ++i)
		{
			if (UStaticMeshComponent* Part = Parts[i])
			{
				UMaterialInstanceDynamic* Paintwork = UMaterialInstanceDynamic::Create(BaseMaterial, Owner);
				Paintwork->SetVectorParameterValue(TEXT("Color"), PaintColor(Paints[i], BodyColor));
				Part->SetMaterial(0, Paintwork);
			}
		}
	}
}
