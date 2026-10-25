// Fill out your copyright notice in the Description page of Project Settings.

#include "CarVisuals.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "ExhaustSmoke.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace CarVisuals
{
	// How far the front wheels turn at full lock (degrees).
	constexpr float MaxSteer = 32.f;
	// Wheels further from the camera than this aren't worth turning (cm)...
	constexpr double WheelRange = 6000.0;
	// ...nor smoke worth making.
	constexpr double SmokeRange = 3500.0;
	// Moving further than this in a frame is a jump (spawned or moved), not driving (cm).
	constexpr float MaxTravel = 600.f;
	// Pulling away this hard (cm/s each second) counts as full throttle.
	constexpr float FullEffortAcceleration = 400.f;
	// Seconds between puffs: idling, and with the pedal down.
	constexpr float IdlePuffInterval = 0.45f;
	constexpr float BusyPuffInterval = 0.07f;
	// Smoke leaves the pipe backwards at this speed (cm/s).
	constexpr float PuffSpeed = 90.f;

	/** The bodies and wheels are the same for every car of a kind: make each once. */
	const CarModel::FBuilder& BodyShape(const CarModel::FStyle& Style)
	{
		static TMap<int32, CarModel::FBuilder> Shapes;
		const int32 Key = CarModel::ShapeKey(Style);
		if (const CarModel::FBuilder* Found = Shapes.Find(Key))
		{
			return *Found;
		}
		CarModel::FBuilder& Built = Shapes.Add(Key);
		CarModel::Build(Built, Style);
		return Built;
	}

	const CarModel::FBuilder& WheelShape(CarModel::EWheel Wheel)
	{
		static TMap<int32, CarModel::FBuilder> Shapes;
		const int32 Key = static_cast<int32>(Wheel);
		if (const CarModel::FBuilder* Found = Shapes.Find(Key))
		{
			return *Found;
		}
		CarModel::FBuilder& Built = Shapes.Add(Key);
		CarModel::BuildWheel(Built, Wheel);
		return Built;
	}
}

UCarVisuals::UCarVisuals()
{
	PrimaryComponentTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = BasicShapeMaterial.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Translucent(TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
	SeeThroughMaterial = Translucent.Object;
	static ConstructorHelpers::FObjectFinder<UTexture> White(TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	WhiteTexture = White.Object;
}

void UCarVisuals::Show(USceneComponent* Parent, const CarModel::FStyle& InStyle)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Parent)
	{
		return;
	}
	Style = InStyle;

	auto MakeMesh = [Owner, Parent]()
	{
		UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(Owner);
		Mesh->SetupAttachment(Parent);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->RegisterComponent();
		Owner->AddInstanceComponent(Mesh);
		return Mesh;
	};
	if (!Body)
	{
		Body = MakeMesh();
		for (int32 i = 0; i < 4; ++i)
		{
			Wheels.Add(MakeMesh());
		}
	}

	Fill(Body, CarVisuals::BodyShape(Style));
	for (int32 i = 0; i < Wheels.Num(); ++i)
	{
		Wheels[i]->SetRelativeLocationAndRotation(CarModel::WheelHub(Style.Body, i), FRotator::ZeroRotator);
		Fill(Wheels[i], CarVisuals::WheelShape(Style.Wheel));
	}
	bShown = true;
	bMoved = false;
}

void UCarVisuals::Fill(UProceduralMeshComponent* Mesh, const CarModel::FBuilder& Shape)
{
	Mesh->ClearAllMeshSections();
	for (int32 i = 0; i < CarModel::PartCount; ++i)
	{
		const CarModel::FPart& Part = Shape.Parts[i];
		if (Part.Triangles.Num() == 0)
		{
			continue;
		}
		const FLinearColor Color = CarModel::PartColor(static_cast<CarModel::EPart>(i), Style);
		if (i == CarModel::Glass)
		{
			if (!SeeThroughMaterial)
			{
				// No see-through material: leave the panes out, as open windows.
				continue;
			}
			Mesh->CreateMeshSection_LinearColor(i, Part.Vertices, Part.Triangles, Part.Normals,
				TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(SeeThroughMaterial, this);
			if (WhiteTexture)
			{
				Material->SetTextureParameterValue(TEXT("SlateUI"), WhiteTexture);
			}
			Material->SetScalarParameterValue(TEXT("OpacityFromTexture"), 0.f);
			Material->SetVectorParameterValue(TEXT("TintColorAndOpacity"), Color);
			Mesh->SetMaterial(i, Material);
			continue;
		}
		Mesh->CreateMeshSection_LinearColor(i, Part.Vertices, Part.Triangles, Part.Normals,
			TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Material->SetVectorParameterValue(TEXT("Color"), Color);
			Mesh->SetMaterial(i, Material);
		}
	}
}

void UCarVisuals::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!bShown || !Owner || DeltaTime <= 0.f)
	{
		return;
	}

	// How far the car went forward and turned since last frame.
	const FVector Location = Owner->GetActorLocation();
	const float Yaw = static_cast<float>(Owner->GetActorRotation().Yaw);
	if (!bMoved)
	{
		LastLocation = Location;
		LastYaw = Yaw;
		bMoved = true;
		return;
	}
	const float Travel = static_cast<float>(FVector::DotProduct(Location - LastLocation, Owner->GetActorForwardVector()));
	const float Turn = FRotator::NormalizeAxis(Yaw - LastYaw);
	LastLocation = Location;
	LastYaw = Yaw;
	if (FMath::Abs(Travel) > CarVisuals::MaxTravel)
	{
		return;
	}
	const float Speed = Travel / DeltaTime;
	const float Pickup = (Speed - LastSpeed) / DeltaTime;
	LastSpeed = Speed;

	// The wheels roll as far as the car goes. The front ones turn as far as
	// the turning needs (a car turns Travel / wheelbase * tan(angle)), or as
	// the driver turns the wheel.
	Spin = FMath::Fmod(Spin + FMath::RadiansToDegrees(Travel / static_cast<float>(CarModel::WheelRadius)), 360.f);
	float WantedSteer = SteerAngle;
	if (bDriverSteers)
	{
		WantedSteer = DriverSteer * CarVisuals::MaxSteer;
	}
	else if (FMath::Abs(Travel) > 0.5f)
	{
		const CarModel::FLayout Layout = CarModel::Layout(Style.Body);
		const float Wheelbase = static_cast<float>(Layout.FrontAxle - Layout.RearAxle);
		WantedSteer = FMath::RadiansToDegrees(FMath::Atan(Wheelbase * FMath::DegreesToRadians(Turn) / Travel));
	}
	WantedSteer = FMath::Clamp(WantedSteer, -CarVisuals::MaxSteer, CarVisuals::MaxSteer);
	SteerAngle = FMath::FInterpTo(SteerAngle, WantedSteer, DeltaTime, 10.f);

	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const double Distance = Camera ? FVector::Dist(Camera->GetCameraLocation(), Location) : 0.0;
	if (Distance < CarVisuals::WheelRange && Owner->WasRecentlyRendered(0.2f))
	{
		for (int32 i = 0; i < Wheels.Num(); ++i)
		{
			// Rolling forward turns the top of the wheel forward: pitch down.
			Wheels[i]->SetRelativeRotation(FRotator(-Spin, i < 2 ? SteerAngle : 0.f, 0.f));
		}
	}

	// Little puffs from the tail pipe: more, and bigger, when pulling away.
	if (bEngineRunning && Distance < CarVisuals::SmokeRange)
	{
		const float Effort = DriverThrottle >= 0.f
			? DriverThrottle
			: FMath::Clamp(Pickup / CarVisuals::FullEffortAcceleration, 0.f, 1.f);
		PuffTimer -= DeltaTime;
		if (PuffTimer <= 0.f)
		{
			PuffTimer = FMath::Lerp(CarVisuals::IdlePuffInterval, CarVisuals::BusyPuffInterval, Effort) * FMath::FRandRange(0.7f, 1.3f);
			Puff(FMath::Lerp(5.f, 10.f, Effort), false);
		}
	}
}

void UCarVisuals::Backfire()
{
	Puff(14.f, true);
	Puff(10.f, true);
}

void UCarVisuals::Puff(float Size, bool bDark)
{
	AActor* Owner = GetOwner();
	AExhaustSmoke* Smoke = AExhaustSmoke::Get(GetWorld());
	if (!Owner || !Smoke)
	{
		return;
	}
	const FVector Pipe = Owner->GetActorTransform().TransformPosition(CarModel::Layout(Style.Body).Exhaust);
	const FVector Back = -Owner->GetActorForwardVector() * CarVisuals::PuffSpeed;
	const FVector Wander(FMath::FRandRange(-20.f, 20.f), FMath::FRandRange(-20.f, 20.f), FMath::FRandRange(5.f, 25.f));
	Smoke->Puff(Pipe, Back + Wander, Size, bDark);
}
