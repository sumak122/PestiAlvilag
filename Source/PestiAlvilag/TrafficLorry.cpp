// Fill out your copyright notice in the Description page of Project Settings.

#include "TrafficLorry.h"

#include "Camera/PlayerCameraManager.h"
#include "CarVisuals.h"
#include "Kismet/GameplayStatics.h"
#include "SpotSound.h"
#include "Lorry.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

namespace TrafficLorryRun
{
	// The box's middle is this far over the road (ATrafficManager's ride height), its half height (cm).
	constexpr double FloorZ = -90.0;
	constexpr double BoxHalfZ = 70.0;
	// The front wheels turn this far at their sharpest (degrees), at this turn rate (degrees/s).
	constexpr float MaxSteer = 30.f;
	constexpr float FullTurn = 60.f;
	// The ambulance rushes, its bell ringing, for this long (s), every so often; heard this far (cm).
	constexpr float RingLeast = 15.f;
	constexpr float RingMost = 35.f;
	constexpr float QuietLeast = 50.f;
	constexpr float QuietMost = 120.f;
	constexpr double BellReach = 9000.0;
}

ATrafficLorry::ATrafficLorry()
{
	// Heavy: slow to pull away and to stop, wide in the turns, a longer gap.
	TurnRate = 80.f;
	Acceleration = 280.f;
	BrakeDeceleration = 1000.f;
	StopGap = 900.f;
	const FVector Half = LorryModel::LorryHalf();
	CollisionBox->SetBoxExtent(FVector(Half.X, Half.Y, TrafficLorryRun::BoxHalfZ));
	// No trafficator arms on a lorry's cab.
	for (UStaticMeshComponent* Arm : { TrafficatorLeft.Get(), TrafficatorRight.Get() })
	{
		if (Arm)
		{
			Arm->SetHiddenInGame(true);
		}
	}
}

void ATrafficLorry::BeginPlay()
{
	Super::BeginPlay();
	// Its paint and load: most carry something, a few run empty.
	FRandomStream Dice(static_cast<int32>(GetUniqueID()) * 7 + 3);
	PaintIndex = Dice.RandRange(0, 3);
	const int32 Pick = Dice.RandRange(0, 9);
	Load = Pick < 3 ? LorryModel::ELoad::Crates : Pick < 5 ? LorryModel::ELoad::Barrels : Pick < 7 ? LorryModel::ELoad::Sacks
		: Pick < 9 ? LorryModel::ELoad::Tilt : LorryModel::ELoad::Empty;
	LastYaw = static_cast<float>(GetActorRotation().Yaw);
}

void ATrafficLorry::Dress()
{
	using namespace LorryModel;
	bDressed = true;
	Visuals->DrawNothing();

	PersonKit::FPiece Shapes[NumParts];
	Build(Shapes, PaintIndex, Load);
	for (int32 p = 0; p < NumParts; ++p)
	{
		UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this);
		Mesh->SetupAttachment(CollisionBox);
		Mesh->SetRelativeLocation(FVector(0.0, 0.0, TrafficLorryRun::FloorZ) + (p == Body ? FVector::ZeroVector : WheelHub(p)));
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCanEverAffectNavigation(false);
		// It moves: kept out of the ray-traced lighting and the distance fields.
		Mesh->bVisibleInRayTracing = false;
		Mesh->bAffectDistanceFieldLighting = false;
		const PersonKit::FPiece& Shape = Shapes[p];
		for (int32 s = 0; s < Shape.Sections.Num(); ++s)
		{
			const PersonKit::FSection& Section = Shape.Sections[s];
			Mesh->CreateMeshSection_LinearColor(s, Section.Vertices, Section.Triangles, Section.Normals,
				TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
			if (BaseMaterial)
			{
				UMaterialInstanceDynamic* Coat = UMaterialInstanceDynamic::Create(BaseMaterial, this);
				Coat->SetVectorParameterValue(TEXT("Color"), Section.Color);
				Mesh->SetMaterial(s, Coat);
			}
		}
		Mesh->RegisterComponent();
		AddInstanceComponent(Mesh);
		Parts.Add(Mesh);
	}
	ALorry::AddLettering(this, CollisionBox, TrafficLorryRun::FloorZ, Load);
}

void ATrafficLorry::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	using namespace LorryModel;
	if (!Manager)
	{
		return;
	}
	if (!bDressed)
	{
		Dress();
	}

	// The wheels roll, the front ones turn the way it turns.
	const float RollSpeed = GetDrivingSpeed();
	WheelTurn = FMath::Fmod(WheelTurn + RollSpeed * DeltaSeconds, static_cast<float>(2.0 * PI * WheelRadius));
	const float Spin = -FMath::RadiansToDegrees(WheelTurn / static_cast<float>(WheelRadius));
	const float NowYaw = static_cast<float>(GetActorRotation().Yaw);
	const float YawRate = DeltaSeconds > 0.f ? FMath::FindDeltaAngleDegrees(LastYaw, NowYaw) / DeltaSeconds : 0.f;
	LastYaw = NowYaw;
	const float WantSteer = FMath::Clamp(YawRate / TrafficLorryRun::FullTurn, -1.f, 1.f) * TrafficLorryRun::MaxSteer;
	SteerAngle = FMath::FInterpTo(SteerAngle, WantSteer, DeltaSeconds, 8.f);
	for (int32 w = WheelFrontLeft; w <= WheelBackRight; ++w)
	{
		if (Parts.IsValidIndex(w) && Parts[w])
		{
			const bool bFront = w == WheelFrontLeft || w == WheelFrontRight;
			Parts[w]->SetRelativeRotation(FRotator(Spin, bFront ? SteerAngle : 0.f, 0.f));
		}
	}
	if (Load == ELoad::Ambulance)
	{
		TickBell(DeltaSeconds);
	}
}

void ATrafficLorry::TickBell(float DeltaSeconds)
{
	BellLeft -= DeltaSeconds;
	if (BellLeft <= 0.f)
	{
		bRinging = !bRinging;
		BellLeft = bRinging ? FMath::FRandRange(TrafficLorryRun::RingLeast, TrafficLorryRun::RingMost)
			: FMath::FRandRange(TrafficLorryRun::QuietLeast, TrafficLorryRun::QuietMost);
	}
	if (bRinging && !Bell)
	{
		Bell = NewObject<USpotSound>(this);
		Bell->SetupAttachment(CollisionBox);
		Bell->RegisterComponent();
		Bell->SetSpot(CitySynth::ESpot::FireBell, FMath::Rand());
		Bell->SetHeard(0.f, 1.f);
		Bell->Start();
	}
	if (Bell)
	{
		float Gain = 0.f;
		float Far = 1.f;
		if (bRinging)
		{
			if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
			{
				const double Distance = FVector::Dist(Camera->GetCameraLocation(), GetActorLocation());
				Far = static_cast<float>(FMath::Clamp(Distance / TrafficLorryRun::BellReach, 0.0, 1.0));
				Gain = Distance < TrafficLorryRun::BellReach ? 1.f - Far : 0.f;
			}
		}
		Bell->SetHeard(Gain, Far);
	}
}
