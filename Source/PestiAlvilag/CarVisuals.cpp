// Fill out your copyright notice in the Description page of Project Settings.

#include "CarVisuals.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "EngineSound.h"
#include "ExhaustSmoke.h"
#include "PersonModel.h"
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
	// Smoke only comes when the car pulls away hard: the pedal down this far
	// (or picking up speed this fast), below this speed (cm/s).
	constexpr float PuffThrottle = 0.6f;
	constexpr float PuffAcceleration = 300.f;
	constexpr float PuffTopSpeed = 1000.f;
	// Seconds between puffs while it does.
	constexpr float PuffInterval = 0.12f;
	// Smoke leaves the pipe backwards at this speed (cm/s).
	constexpr float PuffSpeed = 90.f;
	// Traffic engines are heard within this distance of the camera (cm),
	// fall silent again beyond the second, and only this many at once.
	constexpr double HearRange = 2500.0;
	constexpr double ForgetRange = 3000.0;
	constexpr int32 MaxHeardEngines = 6;
	int32 HeardEngines = 0;
	// Their sound fades out to nothing over this distance, and is quieter than the player's car.
	constexpr float EngineFalloff = 2200.f;
	constexpr float EngineVolume = 0.6f;
	// Traffic at this speed is flat out (cm/s).
	constexpr float StreetTopSpeed = 1400.f;

	// A driver further than this isn't drawn (cm).
	constexpr float DriverRange = 6000.f;

	// Taxi drivers in black jackets and caps, policemen in navy.
	const FLinearColor TaxiDriverCloth(0.03f, 0.03f, 0.035f);
	const FLinearColor PoliceCloth(0.02f, 0.025f, 0.06f);

	/** One of the town's drivers. */
	PersonKit::FOutfit StreetDriver(const CarModel::FStyle& Style, int32 Look)
	{
		FRandomStream Random(Look * 7919 + 13);
		PersonKit::FOutfit Outfit = PersonModel::RandomOutfit(Random);
		if (Style.bTaxi || Style.bPolice)
		{
			// The company and the force hire men.
			for (int32 Try = 0; Try < 8 && Outfit.bWoman; ++Try)
			{
				Outfit = PersonModel::RandomOutfit(Random);
			}
			Outfit.bWoman = false;
			Outfit.Hat = PersonKit::EHat::FlatCap;
			Outfit.Top = PersonKit::ETop::Jacket;
			Outfit.bScarf = false;
			Outfit.bTie = true;
			const FLinearColor& Cloth = Style.bPolice ? PoliceCloth : TaxiDriverCloth;
			Outfit.HatColor = Cloth;
			Outfit.HatBand = Style.bPolice ? FLinearColor(0.55f, 0.45f, 0.15f) : Cloth;
			Outfit.Coat = Cloth;
			Outfit.Trousers = Cloth;
		}
		return Outfit;
	}

	/** The drivers are the same for every car of a kind with the same look: make each once. */
	const TArray<PersonKit::FSection>& StreetDriverShape(const CarModel::FStyle& Style, int32 Look)
	{
		static TMap<int32, TArray<PersonKit::FSection>> Shapes;
		const int32 Kind = Style.bPolice ? 2 : Style.bTaxi ? 1 : 0;
		const int32 Key = (Look * 3 + Kind) * static_cast<int32>(CarModel::EBody::Count) + static_cast<int32>(Style.Body);
		if (const TArray<PersonKit::FSection>* Found = Shapes.Find(Key))
		{
			return *Found;
		}
		TArray<PersonKit::FSection>& Built = Shapes.Add(Key);
		CarModel::BuildSeatedDriver(StreetDriver(Style, Look), CarModel::Layout(Style.Body), Built);
		return Built;
	}

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

void UCarVisuals::ShowDriver(const PersonKit::FOutfit& Outfit)
{
	TArray<PersonKit::FSection> Sections;
	CarModel::BuildSeatedDriver(Outfit, CarModel::Layout(Style.Body), Sections);
	FillDriver(Sections);
}

void UCarVisuals::ShowStreetDriver(int32 Look)
{
	FillDriver(CarVisuals::StreetDriverShape(Style, FMath::Clamp(Look, 0, NumDriverLooks - 1)));
}

void UCarVisuals::HideDriver()
{
	if (Driver)
	{
		Driver->ClearAllMeshSections();
		Driver->SetVisibility(false);
	}
}

void UCarVisuals::FillDriver(const TArray<PersonKit::FSection>& Sections)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Body)
	{
		return;
	}
	if (!Driver)
	{
		Driver = NewObject<UProceduralMeshComponent>(Owner);
		Driver->SetupAttachment(Body);
		Driver->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Driver->SetCanEverAffectNavigation(false);
		Driver->SetCastShadow(false);
		Driver->SetCachedMaxDrawDistance(CarVisuals::DriverRange);
		Driver->RegisterComponent();
		Owner->AddInstanceComponent(Driver);
	}
	Driver->ClearAllMeshSections();
	for (int32 i = 0; i < Sections.Num(); ++i)
	{
		const PersonKit::FSection& Section = Sections[i];
		Driver->CreateMeshSection_LinearColor(i, Section.Vertices, Section.Triangles, Section.Normals,
			TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Material->SetVectorParameterValue(TEXT("Color"), Section.Color);
			Driver->SetMaterial(i, Material);
		}
	}
	Driver->SetVisibility(true);
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

	if (bStreetEngine)
	{
		UpdateStreetEngine(Camera ? Distance : CarVisuals::ForgetRange + 1.0, Speed, Pickup);
	}

	// Faint puffs from the tail pipe, only while pulling away hard.
	PuffTimer -= DeltaTime;
	const bool bPullingAway = FMath::Abs(Speed) < CarVisuals::PuffTopSpeed && (DriverThrottle >= 0.f
		? DriverThrottle > CarVisuals::PuffThrottle && Pickup > 0.f
		: Pickup > CarVisuals::PuffAcceleration);
	if (bEngineRunning && bPullingAway && Distance < CarVisuals::SmokeRange && PuffTimer <= 0.f)
	{
		const float Effort = FMath::Clamp(Pickup / CarVisuals::FullEffortAcceleration, 0.f, 1.f);
		PuffTimer = CarVisuals::PuffInterval * FMath::FRandRange(0.7f, 1.3f);
		Puff(FMath::Lerp(6.f, 10.f, Effort), false);
	}
}

void UCarVisuals::UpdateStreetEngine(double Distance, float Speed, float Pickup)
{
	if (bStreetEngineHeard && Distance > CarVisuals::ForgetRange)
	{
		StopStreetEngine();
	}
	else if (!bStreetEngineHeard && Distance < CarVisuals::HearRange && CarVisuals::HeardEngines < CarVisuals::MaxHeardEngines)
	{
		AActor* Owner = GetOwner();
		if (!StreetEngine && Owner && Body)
		{
			StreetEngine = NewObject<UEngineSound>(Owner);
			StreetEngine->SetupAttachment(Body);
			StreetEngine->bAllowSpatialization = true;
			StreetEngine->bOverrideAttenuation = true;
			StreetEngine->AttenuationOverrides.bAttenuate = true;
			StreetEngine->AttenuationOverrides.bSpatialize = true;
			StreetEngine->AttenuationOverrides.AttenuationShapeExtents = FVector(300.0, 0.0, 0.0);
			StreetEngine->AttenuationOverrides.FalloffDistance = CarVisuals::EngineFalloff;
			StreetEngine->RegisterComponent();
			Owner->AddInstanceComponent(StreetEngine);
		}
		if (StreetEngine)
		{
			StreetEngine->SetVolumeMultiplier(CarVisuals::EngineVolume);
			StreetEngine->Start();
			bStreetEngineHeard = true;
			++CarVisuals::HeardEngines;
		}
	}
	if (bStreetEngineHeard && StreetEngine)
	{
		const float Share = FMath::Clamp(FMath::Abs(Speed) / CarVisuals::StreetTopSpeed, 0.f, 1.f);
		const float Load = FMath::Clamp(Pickup / CarVisuals::FullEffortAcceleration, 0.f, 1.f);
		StreetEngine->SetEngine(0.1f + 0.5f * Share + 0.15f * Load, Load);
	}
}

void UCarVisuals::StopStreetEngine()
{
	if (!bStreetEngineHeard)
	{
		return;
	}
	if (StreetEngine)
	{
		StreetEngine->Stop();
	}
	bStreetEngineHeard = false;
	CarVisuals::HeardEngines = FMath::Max(0, CarVisuals::HeardEngines - 1);
}

void UCarVisuals::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopStreetEngine();
	Super::EndPlay(EndPlayReason);
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
