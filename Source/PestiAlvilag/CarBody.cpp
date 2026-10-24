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

	FLinearColor PaintColor(EPaint Slot, const FLinearColor& BodyColor)
	{
		switch (Slot)
		{
		case EPaint::Tyre: return FLinearColor(0.01f, 0.01f, 0.01f);
		case EPaint::Chrome: return FLinearColor(0.6f, 0.6f, 0.62f);
		case EPaint::Glass: return FLinearColor(0.02f, 0.03f, 0.05f);
		case EPaint::Headlamp: return FLinearColor(1.f, 0.9f, 0.6f);
		case EPaint::TailLamp: return FLinearColor(0.7f, 0.02f, 0.02f);
		default: return BodyColor;
		}
	}

	void Build(AActor* Owner, USceneComponent* Parent, TArray<TObjectPtr<UStaticMeshComponent>>& OutParts, TArray<EPaint>& OutPaints)
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
		static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

		for (const FPartSpec& Spec : PartSpecs)
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
