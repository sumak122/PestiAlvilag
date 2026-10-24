// Fill out your copyright notice in the Description page of Project Settings.

#include "TrafficCar.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TrafficManager.h"
#include "UObject/ConstructorHelpers.h"

namespace TrafficCar
{
	// How far ahead a car looks for others (cm).
	constexpr float LookAhead = 2500.f;
	// The car moves on to the next point once it is this close (cm).
	constexpr float ReachRadius = 300.f;
	// Slowest fraction of lane speed in a sharp corner.
	constexpr float CornerSpeedFraction = 0.35f;
	// Cars ask for a junction this far before it (cm)...
	constexpr float JunctionApproach = 1500.f;
	// ...stop this far before it if it is busy...
	constexpr float JunctionStopLine = 700.f;
	// ...and free it once this far past (cm).
	constexpr float JunctionExit = 900.f;
	// Seconds stuck against another car or a wall before driving through it.
	constexpr float MaxBlockedTime = 3.f;
	// Signal a turn from this far before the junction (cm)...
	constexpr float SignalDistance = 2500.f;
	// ...if the road bends more than this (degrees).
	constexpr float SignalAngle = 25.f;
	const FLinearColor TrafficatorColor(1.f, 0.35f, 0.f);
}

ATrafficCar::ATrafficCar()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = BasicShapeMaterial.Object;

	// Same size and layout as the player's car (see APestiCar).
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetBoxExtent(FVector(210.f, 85.f, 70.f));
	CollisionBox->SetCollisionProfileName(TEXT("Pawn"));
	RootComponent = CollisionBox;

	struct FPart
	{
		const TCHAR* Name;
		bool bWheel;
		FVector Location;
		FVector Scale;
	};
	const FPart Parts[] = {
		{ TEXT("Chassis"), false, FVector(0.f, 0.f, -35.f), FVector(4.2f, 1.7f, 0.6f) },
		{ TEXT("Cabin"), false, FVector(-30.f, 0.f, 25.f), FVector(2.0f, 1.5f, 0.6f) },
		{ TEXT("Wheel0"), true, FVector(130.f, 78.f, -55.f), FVector(0.7f, 0.7f, 0.25f) },
		{ TEXT("Wheel1"), true, FVector(130.f, -78.f, -55.f), FVector(0.7f, 0.7f, 0.25f) },
		{ TEXT("Wheel2"), true, FVector(-130.f, 78.f, -55.f), FVector(0.7f, 0.7f, 0.25f) },
		{ TEXT("Wheel3"), true, FVector(-130.f, -78.f, -55.f), FVector(0.7f, 0.7f, 0.25f) },
	};
	for (const FPart& Spec : Parts)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Spec.Name);
		Part->SetupAttachment(CollisionBox);
		Part->SetStaticMesh(Spec.bWheel ? CylinderMesh.Object : CubeMesh.Object);
		Part->SetRelativeLocationAndRotation(Spec.Location, Spec.bWheel ? FRotator(0.f, 0.f, 90.f) : FRotator::ZeroRotator);
		Part->SetRelativeScale3D(Spec.Scale);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BodyParts.Add(Part);
	}

	// Trafficator arms stick out sideways from the front of the cabin; the
	// car's left is -Y. Hidden until the car signals.
	auto MakeTrafficator = [&](const TCHAR* PartName, float Side)
	{
		UStaticMeshComponent* Arm = CreateDefaultSubobject<UStaticMeshComponent>(PartName);
		Arm->SetupAttachment(CollisionBox);
		Arm->SetStaticMesh(CubeMesh.Object);
		Arm->SetRelativeLocation(FVector(55.f, Side * 90.f, 30.f));
		Arm->SetRelativeScale3D(FVector(0.05f, 0.3f, 0.06f));
		Arm->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Arm->SetVisibility(false);
		return Arm;
	};
	TrafficatorLeft = MakeTrafficator(TEXT("TrafficatorLeft"), -1.f);
	TrafficatorRight = MakeTrafficator(TEXT("TrafficatorRight"), 1.f);
}

void ATrafficCar::StartOnLane(ATrafficManager* InManager, int32 InLaneIndex, int32 InPointIndex, const FLinearColor& Paint)
{
	Manager = InManager;
	LaneIndex = InLaneIndex;
	NextLaneIndex = Manager->PickNextLane(LaneIndex);
	PointIndex = InPointIndex;
	CurrentSpeed = 0.f;

	if (!BaseMaterial)
	{
		return;
	}
	for (int32 i = 0; i < BodyParts.Num(); ++i)
	{
		UStaticMeshComponent* Part = BodyParts[i];
		if (!Part)
		{
			continue;
		}
		// The first two parts are the body; the rest are tyres.
		UMaterialInstanceDynamic* Paintwork = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Paintwork->SetVectorParameterValue(TEXT("Color"), i < 2 ? Paint : FLinearColor(0.01f, 0.01f, 0.01f));
		Part->SetMaterial(0, Paintwork);
	}

	for (UStaticMeshComponent* Arm : { TrafficatorLeft.Get(), TrafficatorRight.Get() })
	{
		UMaterialInstanceDynamic* Paintwork = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Paintwork->SetVectorParameterValue(TEXT("Color"), TrafficCar::TrafficatorColor);
		Arm->SetMaterial(0, Paintwork);
	}
}

void ATrafficCar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Manager || LaneIndex == INDEX_NONE)
	{
		return;
	}

	FVector Location = GetActorLocation();

	// Advance along the lane, switching lanes at the end of each street piece.
	const FTrafficLane* Lane = &Manager->GetLane(LaneIndex);
	FVector Target = Lane->Points[PointIndex];
	for (int32 Guard = 0; Guard < 4 && FVector::Dist2D(Location, Target) < TrafficCar::ReachRadius; ++Guard)
	{
		if (++PointIndex >= Lane->Points.Num())
		{
			LaneIndex = NextLaneIndex;
			NextLaneIndex = Manager->PickNextLane(LaneIndex);
			Lane = &Manager->GetLane(LaneIndex);
			PointIndex = 0;
		}
		Target = Lane->Points[PointIndex];
	}

	// Steer towards the target point at a limited turn rate.
	const FVector ToTarget = (Target - Location).GetSafeNormal2D();
	const float CurrentYaw = static_cast<float>(GetActorRotation().Yaw);
	const float WantedYaw = static_cast<float>(ToTarget.Rotation().Yaw);
	const float TurnNeeded = FMath::Abs(FRotator::NormalizeAxis(WantedYaw - CurrentYaw));
	const float NewYaw = FMath::FixedTurn(CurrentYaw, WantedYaw, TurnRate * DeltaSeconds);
	SetActorRotation(FRotator(0.f, NewYaw, 0.f));

	// Slow for corners and for anything in front.
	float WantedSpeed = Lane->Speed * FMath::Lerp(1.f, TrafficCar::CornerSpeedFraction, FMath::Clamp(TurnNeeded / 90.f, 0.f, 1.f));
	const float Clear = Manager->ClearDistanceAhead(this, TrafficCar::LookAhead);
	WantedSpeed *= FMath::Clamp((Clear - StopGap) / (TrafficCar::LookAhead - StopGap), 0.f, 1.f);

	// Take turns at junctions.
	const float DistToJunction = static_cast<float>(FVector::Dist2D(Location, Lane->Points.Last()));
	WantedSpeed *= JunctionSpeedLimit(DistToJunction, Lane->ToNode, DeltaSeconds);
	ReleasePassedJunctions(Location, Lane->ToNode);
	UpdateTrafficators(DistToJunction);

	const float Rate = WantedSpeed < CurrentSpeed ? BrakeDeceleration : Acceleration;
	CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, WantedSpeed, DeltaSeconds, Rate);

	// Move with collision so cars bump into the player and each other instead
	// of passing through. If stuck on another AI car or a wall for a while,
	// drive through it so traffic cannot jam for good; never through the player.
	const FVector NewLocation = Location + GetActorForwardVector().GetSafeNormal2D() * CurrentSpeed * DeltaSeconds;
	FHitResult Hit;
	SetActorLocation(NewLocation, true, &Hit);
	if (!Hit.bBlockingHit)
	{
		BlockedTime = 0.f;
		return;
	}
	CurrentSpeed = 0.f;
	BlockedTime += DeltaSeconds;
	const AActor* Blocker = Hit.GetActor();
	const bool bBlockedByPlayer = Blocker && Blocker == UGameplayStatics::GetPlayerPawn(this, 0);
	if (!bBlockedByPlayer && BlockedTime > TrafficCar::MaxBlockedTime)
	{
		SetActorLocation(NewLocation, false);
	}
}

float ATrafficCar::JunctionSpeedLimit(float DistToJunction, int32 Node, float DeltaSeconds)
{
	if (HeldNodes.Contains(Node) || DistToJunction > TrafficCar::JunctionApproach)
	{
		return 1.f;
	}

	const bool bForce = JunctionWaitTime > MaxJunctionWait;
	if (Manager->TryClaimNode(Node, this, bForce))
	{
		HeldNodes.Add(Node);
		JunctionWaitTime = 0.f;
		return 1.f;
	}

	JunctionWaitTime += DeltaSeconds;
	return FMath::Clamp((DistToJunction - TrafficCar::JunctionStopLine)
		/ (TrafficCar::JunctionApproach - TrafficCar::JunctionStopLine), 0.f, 1.f);
}

void ATrafficCar::ReleasePassedJunctions(const FVector& Location, int32 NextNode)
{
	for (int32 i = HeldNodes.Num() - 1; i >= 0; --i)
	{
		const int32 Node = HeldNodes[i];
		if (Node != NextNode && FVector::Dist2D(Location, Manager->GetNodeLocation(Node)) > TrafficCar::JunctionExit)
		{
			Manager->ReleaseNode(Node, this);
			HeldNodes.RemoveAt(i);
		}
	}
}

void ATrafficCar::UpdateTrafficators(float DistToJunction)
{
	bool bLeft = false;
	bool bRight = false;
	const FTrafficLane& Lane = Manager->GetLane(LaneIndex);
	if (DistToJunction < TrafficCar::SignalDistance && Manager->IsJunction(Lane.ToNode) && NextLaneIndex != INDEX_NONE)
	{
		// Compare the heading at the end of this lane with the start of the next.
		const FTrafficLane& Next = Manager->GetLane(NextLaneIndex);
		const int32 Last = Lane.Points.Num() - 1;
		const float InYaw = static_cast<float>((Lane.Points[Last] - Lane.Points[Last - 1]).Rotation().Yaw);
		const float OutYaw = static_cast<float>((Next.Points[1] - Next.Points[0]).Rotation().Yaw);
		const float Turn = FRotator::NormalizeAxis(OutYaw - InYaw);
		// Unreal's yaw grows clockwise seen from above, so positive is a right turn.
		bLeft = Turn < -TrafficCar::SignalAngle;
		bRight = Turn > TrafficCar::SignalAngle;
	}
	TrafficatorLeft->SetVisibility(bLeft);
	TrafficatorRight->SetVisibility(bRight);
}
