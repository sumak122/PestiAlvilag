// Fill out your copyright notice in the Description page of Project Settings.

#include "Tram.h"

#include "CarBody.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TrafficManager.h"

namespace Tram
{
	// Box half size: 10 m long, 2.2 m wide, 3.3 m tall.
	const FVector BoxExtent(500.f, 110.f, 165.f);
	// The body leans along the track between these points ahead and behind (cm).
	constexpr float AxleSpread = 300.f;
	// How far ahead the tram watches the rails (cm).
	constexpr float LookAhead = 3000.f;
	// Seconds held up by an AI car before pushing on regardless.
	constexpr float MaxHeldUp = 6.f;
	// Seconds out of sight between leaving the map and coming back in.
	constexpr float MinTurnaround = 6.f;
	constexpr float MaxTurnaround = 15.f;
	// Room the tram needs at the start of its route to come back in (cm).
	constexpr float StartRoom = 1500.f;
	// Budapest trams were yellow.
	const FLinearColor Livery(0.85f, 0.55f, 0.03f);
}

ATram::ATram()
{
	PrimaryActorTick.bCanEverTick = true;

	BaseMaterial = CarBody::LoadPaintMaterial();

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetBoxExtent(Tram::BoxExtent);
	CollisionBox->SetCollisionProfileName(TEXT("Pawn"));
	RootComponent = CollisionBox;

	CarBody::BuildTram(this, CollisionBox, BodyParts, BodyPaints);
}

void ATram::StartOnRoute(ATrafficManager* InManager, int32 InRouteIndex, float InDistance)
{
	Manager = InManager;
	RouteIndex = InRouteIndex;
	Distance = InDistance;
	CurrentSpeed = 0.f;

	const FTramRoute& Route = Manager->GetTramRoute(RouteIndex);
	NextStop = 0;
	while (Route.Stops.IsValidIndex(NextStop) && Route.Stops[NextStop] <= Distance)
	{
		++NextStop;
	}

	// The map is flat, so find the rails' height once.
	const FVector Start = Route.LocationAt(0.f);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TramGround), false, this);
	GroundZ = GetWorld()->LineTraceSingleByChannel(Hit, Start + FVector(0.f, 0.f, 200.f), Start - FVector(0.f, 0.f, 1000.f), ECC_Visibility, Params)
		? Hit.ImpactPoint.Z
		: 0.0;

	CarBody::Paint(this, BaseMaterial, BodyParts, BodyPaints, Tram::Livery);
	PlaceOnTrack();
}

void ATram::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Manager || RouteIndex == INDEX_NONE)
	{
		return;
	}
	const FTramRoute& Route = Manager->GetTramRoute(RouteIndex);

	// Off the map: wait, then come back in at the start when there is room.
	if (OutOfServiceLeft > 0.f)
	{
		OutOfServiceLeft -= DeltaSeconds;
		if (OutOfServiceLeft <= 0.f)
		{
			if (!IsStartClear())
			{
				OutOfServiceLeft = 1.f;
				return;
			}
			Distance = 0.f;
			NextStop = 0;
			CurrentSpeed = Route.Speed * 0.5f;
			PlaceOnTrack();
			SetInService(true);
		}
		return;
	}

	// Standing at a stop.
	if (DwellLeft > 0.f)
	{
		DwellLeft -= DeltaSeconds;
		if (DwellLeft > 0.f)
		{
			return;
		}
		++NextStop;
	}

	float WantedSpeed = FMath::Min(MaxSpeed, Route.Speed);

	// Brake to stop at the next stop.
	if (Route.Stops.IsValidIndex(NextStop))
	{
		const float ToStop = Route.Stops[NextStop] - Distance;
		if (ToStop < 30.f && CurrentSpeed < 50.f)
		{
			CurrentSpeed = 0.f;
			DwellLeft = DwellTime;
			return;
		}
		WantedSpeed = FMath::Min(WantedSpeed, FMath::Sqrt(2.f * BrakeDeceleration * FMath::Max(0.f, ToStop)) + 20.f);
	}

	// Brake for anything on the rails ahead. Never for long for an AI car,
	// which may itself be waiting for the tram.
	const float ClearAll = Manager->ClearDistanceAhead(this, Tram::LookAhead);
	const bool bHeldUpByCar = ClearAll < StopGap + 200.f && Manager->ClearDistanceAhead(this, Tram::LookAhead, true) >= StopGap + 200.f;
	HeldUpTime = bHeldUpByCar ? HeldUpTime + DeltaSeconds : 0.f;
	const float Clear = HeldUpTime > Tram::MaxHeldUp ? Manager->ClearDistanceAhead(this, Tram::LookAhead, true) : ClearAll;
	WantedSpeed *= FMath::Clamp((Clear - StopGap) / (Tram::LookAhead - StopGap), 0.f, 1.f);

	const float Rate = WantedSpeed < CurrentSpeed ? BrakeDeceleration : Acceleration;
	CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, WantedSpeed, DeltaSeconds, Rate);
	Distance += CurrentSpeed * DeltaSeconds;

	if (Distance >= Route.Length)
	{
		// Gone off the edge of the map.
		SetInService(false);
		OutOfServiceLeft = FMath::FRandRange(Tram::MinTurnaround, Tram::MaxTurnaround);
		return;
	}
	PlaceOnTrack();
}

void ATram::PlaceOnTrack()
{
	const FTramRoute& Route = Manager->GetTramRoute(RouteIndex);
	FVector Location = Route.LocationAt(Distance);
	const FVector Ahead = Route.LocationAt(Distance + Tram::AxleSpread);
	const FVector Behind = Route.LocationAt(Distance - Tram::AxleSpread);
	Location.Z = GroundZ + Tram::BoxExtent.Z + 2.0;
	const FRotator Facing = (Ahead - Behind).GetSafeNormal2D().Rotation();
	SetActorLocationAndRotation(Location, FRotator(0.f, Facing.Yaw, 0.f));
}

void ATram::SetInService(bool bInService)
{
	SetActorHiddenInGame(!bInService);
	SetActorEnableCollision(bInService);
}

bool ATram::IsStartClear() const
{
	const FVector Start = Manager->GetTramRoute(RouteIndex).LocationAt(0.f);
	return Manager->IsAreaClear(Start, Tram::StartRoom, this);
}
