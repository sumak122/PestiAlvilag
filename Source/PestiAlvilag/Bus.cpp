// Fill out your copyright notice in the Description page of Project Settings.

#include "Bus.h"

#include "BusModel.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineSound.h"
#include "Kismet/GameplayStatics.h"
#include "PersonModel.h"
#include "ProceduralMeshComponent.h"
#include "TrafficManager.h"

namespace CityBus
{
	// Box half size: 9.3 m long, 2.5 m wide, 3 m tall (the ground is BusModel::Ground below the middle).
	const FVector BoxExtent(465.f, 125.f, 150.f);
	// People getting on and off stand this far out from the side (cm).
	constexpr double StepOut = 60.0;
	// The seats are this much lower than the tram's benches, which
	// PersonModel::SeatedMiddle fits (cm).
	constexpr double SeatLower = 2.0;
	// Buses are heard within this distance of the camera (cm), fall silent
	// again beyond the second, and only this many at once.
	constexpr double HearRange = 4500.0;
	constexpr double ForgetRange = 5500.0;
	constexpr int32 MaxHeard = 3;
	int32 Heard = 0;
	constexpr float EngineFalloff = 4000.f;
	constexpr float EngineVolume = 0.9f;
	// The colour of each BusModel part: dark blue and white.
	const FLinearColor Colors[BusModel::PartCount] = {
		FLinearColor(0.012f, 0.025f, 0.10f),  // Blue
		FLinearColor(0.80f, 0.80f, 0.77f),    // White
		FLinearColor(0.62f, 0.62f, 0.64f),    // Chrome
		FLinearColor(0.03f, 0.03f, 0.03f),    // Dark
		FLinearColor(0.22f, 0.10f, 0.05f),    // Leather
		FLinearColor(1.00f, 0.85f, 0.55f),    // Lamp
		// See-through: colour and how much it hides (alpha).
		FLinearColor(0.60f, 0.66f, 0.66f, 0.18f) };  // Glass

	/** The body with each line's number on its roof box: made once per number. */
	const BusModel::FBuilder& Model(int32 Number)
	{
		static TMap<int32, TSharedPtr<BusModel::FBuilder>> Built;
		TSharedPtr<BusModel::FBuilder>& Found = Built.FindOrAdd(Number);
		if (!Found.IsValid())
		{
			Found = MakeShared<BusModel::FBuilder>();
			BusModel::Build(*Found, Number);
		}
		return *Found;
	}

	/** One wheel, the same for all. */
	const BusModel::FBuilder& WheelModel()
	{
		static const BusModel::FBuilder Built = []()
		{
			BusModel::FBuilder B;
			BusModel::BuildWheel(B);
			return B;
		}();
		return Built;
	}
}

ABus::ABus()
{
	HalfSize = CityBus::BoxExtent;
	CollisionBox->SetBoxExtent(HalfSize);
	LampSection = BusModel::Lamp;
	InsideLightZ = BusModel::RoofEdge + 10.0;
	// Cars stuck in front of a bus usually move on: wait longer than a tram before pushing through.
	MaxHeldUpTime = 15.f;
	MaxSpeed = 650.f;
	Acceleration = 150.f;
	BrakeDeceleration = 320.f;
	DwellTime = 7.f;
	StopGap = 950.f;

	for (int32 i = 0; i < 4; ++i)
	{
		UProceduralMeshComponent* Wheel = CreateDefaultSubobject<UProceduralMeshComponent>(*FString::Printf(TEXT("Wheel%d"), i));
		Wheel->SetupAttachment(Body);
		Wheel->SetRelativeLocation(BusModel::Hub(i));
		Wheel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Wheel->SetCanEverAffectNavigation(false);
		Wheels.Add(Wheel);
	}
}

void ABus::BuildBody()
{
	if (BuiltNumber == RouteNumber)
	{
		return;
	}
	BuiltNumber = RouteNumber;
	const BusModel::FBuilder& B = CityBus::Model(RouteNumber);
	Body->ClearAllMeshSections();
	for (int32 i = 0; i < BusModel::PartCount; ++i)
	{
		const BusModel::FPart& Part = B.Parts[i];
		Body->CreateMeshSection_LinearColor(i, Part.Vertices, Part.Triangles, Part.Normals,
			TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
		if (!bPainted)
		{
			PaintSection(Body, i, CityBus::Colors[i], i == BusModel::Glass);
		}
		else if (i == BusModel::Glass && !SeeThroughMaterial)
		{
			Body->ClearMeshSection(i);
		}
	}
	if (bPainted)
	{
		return;
	}
	const BusModel::FBuilder& WheelParts = CityBus::WheelModel();
	for (UProceduralMeshComponent* Wheel : Wheels)
	{
		if (!Wheel)
		{
			continue;
		}
		Wheel->ClearAllMeshSections();
		for (int32 i = 0; i < BusModel::PartCount; ++i)
		{
			const BusModel::FPart& Part = WheelParts.Parts[i];
			if (Part.Vertices.Num() == 0)
			{
				continue;
			}
			Wheel->CreateMeshSection_LinearColor(i, Part.Vertices, Part.Triangles, Part.Normals,
				TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
			PaintSection(Wheel, i, CityBus::Colors[i], false);
		}
	}
	bPainted = true;
}

void ABus::StartOnRoute(ATrafficManager* InManager, int32 InRouteIndex, float InDistance)
{
	if (InManager)
	{
		// The line's number on the roof box.
		RouteNumber = FCString::Atoi(*InManager->GetTramRoute(InRouteIndex).Ref);
		BuildBody();
	}
	Super::StartOnRoute(InManager, InRouteIndex, InDistance);
	LastDistance = InDistance;
}

void ABus::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Turn the wheels by how far the bus went (not when it jumps back to the start of its route).
	const float Along = GetDistance();
	const float Moved = Along - LastDistance;
	LastDistance = Along;
	if (Moved <= 0.f || Moved > 1000.f || !WasRecentlyRendered(0.5f))
	{
		return;
	}
	WheelTurn = FMath::Fmod(WheelTurn + FMath::RadiansToDegrees(Moved / BusModel::WheelRadius), 360.0);
	for (UProceduralMeshComponent* Wheel : Wheels)
	{
		if (Wheel)
		{
			Wheel->SetRelativeRotation(FRotator(-WheelTurn, 0.0, 0.0));
		}
	}
}

int32 ABus::GetNumSeats() const
{
	return BusModel::NumSeats;
}

void ABus::GetSeat(int32 Index, FVector& OutAt, double& OutYaw) const
{
	const BusModel::FSeat Place = BusModel::Seat(Index, PersonModel::SeatedMiddle - CityBus::SeatLower);
	OutAt = Place.At;
	OutYaw = Place.Yaw;
}

int32 ABus::GetFreeSeat() const
{
	// The middle rows first, by the aisle, so the player sits in view.
	for (const int32 SeatRow : { 3, 4, 2, 5, 1, 6, 0 })
	{
		for (const int32 InRow : { 0, 2, 1, 3 })
		{
			const int32 Index = SeatRow * BusModel::SeatsPerRow + InRow;
			if (Index < BusModel::NumSeats && IsSeatFree(Index))
			{
				return Index;
			}
		}
	}
	return INDEX_NONE;
}

double ABus::GetGroundBelow() const
{
	return BusModel::Ground;
}

FVector ABus::GetDoorNear(const FVector& /*Point*/) const
{
	// One door, on the left (kerb) side behind the front wheel.
	const FVector Step((BusModel::DoorFrom + BusModel::DoorTo) * 0.5, -(BusModel::HalfWidth + CityBus::StepOut), BusModel::Ground);
	return GetActorTransform().TransformPosition(Step);
}

void ABus::UpdateSound()
{
	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const double ToCamera = Camera && IsInService() ? FVector::Dist(Camera->GetCameraLocation(), GetActorLocation()) : CityBus::ForgetRange + 1.0;
	if (bSoundHeard && ToCamera > CityBus::ForgetRange)
	{
		StopSound();
	}
	else if (!bSoundHeard && ToCamera < CityBus::HearRange && CityBus::Heard < CityBus::MaxHeard)
	{
		if (!Engine)
		{
			Engine = NewObject<UEngineSound>(this);
			Engine->SetupAttachment(CollisionBox);
			Engine->bAllowSpatialization = true;
			Engine->bOverrideAttenuation = true;
			Engine->AttenuationOverrides.bAttenuate = true;
			Engine->AttenuationOverrides.bSpatialize = true;
			Engine->AttenuationOverrides.AttenuationShapeExtents = FVector(HalfSize.X, 0.0, 0.0);
			Engine->AttenuationOverrides.FalloffDistance = CityBus::EngineFalloff;
			Engine->RegisterComponent();
			AddInstanceComponent(Engine);
		}
		Engine->SetVolumeMultiplier(CityBus::EngineVolume);
		Engine->Start();
		bSoundHeard = true;
		++CityBus::Heard;
	}
	if (bSoundHeard && Engine)
	{
		// Idling at the stops, working hard pulling away, humming along.
		const float Share = FMath::Clamp(CurrentSpeed / FMath::Max(MaxSpeed, 1.f), 0.f, 1.f);
		Engine->SetEngine(0.08f + 0.45f * Share + 0.2f * SoundPull, FMath::Max(SoundPull, 0.15f * Share));
	}
}

void ABus::StopSound()
{
	if (!bSoundHeard)
	{
		return;
	}
	if (Engine)
	{
		Engine->Stop();
	}
	bSoundHeard = false;
	CityBus::Heard = FMath::Max(0, CityBus::Heard - 1);
}
