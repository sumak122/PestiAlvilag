// Fill out your copyright notice in the Description page of Project Settings.

#include "Tram.h"

#include "Camera/PlayerCameraManager.h"
#include "CarBody.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PersonModel.h"
#include "PestiPerson.h"
#include "ProceduralMeshComponent.h"
#include "TrafficManager.h"
#include "TramSound.h"
#include "UObject/ConstructorHelpers.h"

namespace Tram
{
	// Box half size: 10 m long, 2.2 m wide, 3.3 m tall.
	const FVector BoxExtent(500.f, 110.f, 165.f);
	// A moving tram hurts someone in front of it: within this much of its
	// front end, and this far outside its box (cm), going faster than this (cm/s).
	constexpr double FrontReach = 200.0;
	constexpr double HitReach = 50.0;
	constexpr float HarmlessSpeed = 150.f;
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
	// People getting on and off stand this far out from the side (cm).
	constexpr double StepOut = 60.0;
	// Trams are heard within this distance of the camera (cm), fall silent
	// again beyond the second, and only this many at once.
	constexpr double HearRange = 4500.0;
	constexpr double ForgetRange = 5500.0;
	constexpr int32 MaxHeard = 4;
	int32 Heard = 0;
	// The sound fades out to nothing over this distance (cm).
	constexpr float SoundFalloff = 4000.f;
	constexpr float SoundVolume = 0.8f;
	// The track bends this much under the tram (degrees between its axles)
	// before the wheels squeal, and squeal fully at the second.
	constexpr float SquealFrom = 4.f;
	constexpr float SquealFull = 14.f;
	// The driver rings at someone on the rails this far ahead (cm), at most every few seconds.
	constexpr double WarnReach = 1500.0;
	constexpr float BellPause = 4.f;
	// The colour of each TramModel part. Budapest trams were yellow.
	const FLinearColor Colors[TramModel::PartCount] = {
		FLinearColor(0.85f, 0.55f, 0.03f),    // Livery
		FLinearColor(0.25f, 0.14f, 0.07f),    // Wood
		FLinearColor(0.05f, 0.05f, 0.05f),    // Dark
		FLinearColor(0.22f, 0.22f, 0.22f),    // Roof
		FLinearColor(1.00f, 0.85f, 0.55f),    // Lamp
		FLinearColor(0.55f, 0.42f, 0.15f),    // Brass
		FLinearColor(0.15f, 0.07f, 0.04f),    // Leather
		// See-through: colour and how much it hides (alpha).
		FLinearColor(0.60f, 0.66f, 0.66f, 0.18f) };  // Glass
	// How many passengers sit on each tram (the rest is room for the player).
	constexpr int32 MinPassengers = 1;
	constexpr int32 MaxPassengers = 4;

	/** The body is the same for every tram: make it once. */
	const TramModel::FBuilder& Model()
	{
		static const TramModel::FBuilder Built = []()
		{
			TramModel::FBuilder B;
			TramModel::Build(B);
			return B;
		}();
		return Built;
	}
}

ATram::ATram()
{
	PrimaryActorTick.bCanEverTick = true;

	BaseMaterial = CarBody::LoadPaintMaterial();

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetBoxExtent(Tram::BoxExtent);
	CollisionBox->SetCollisionProfileName(TEXT("Pawn"));
	RootComponent = CollisionBox;

	Body = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(CollisionBox);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetCanEverAffectNavigation(false);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Translucent(TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
	SeeThroughMaterial = Translucent.Object;
	static ConstructorHelpers::FObjectFinder<UTexture> White(TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	WhiteTexture = White.Object;
}

void ATram::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildBody();
}

void ATram::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopSound();
	Super::EndPlay(EndPlayReason);
}

void ATram::BuildBody()
{
	const TramModel::FBuilder& B = Tram::Model();
	Body->ClearAllMeshSections();
	for (int32 i = 0; i < TramModel::PartCount; ++i)
	{
		const TramModel::FPart& Part = B.Parts[i];
		Body->CreateMeshSection_LinearColor(i, Part.Vertices, Part.Triangles, Part.Normals,
			TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
		if (i == TramModel::Glass)
		{
			if (!SeeThroughMaterial)
			{
				// No see-through material: leave the panes out, as clear glass.
				Body->ClearMeshSection(i);
				continue;
			}
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(SeeThroughMaterial, this);
			if (WhiteTexture)
			{
				Material->SetTextureParameterValue(TEXT("SlateUI"), WhiteTexture);
			}
			Material->SetScalarParameterValue(TEXT("OpacityFromTexture"), 0.f);
			Material->SetVectorParameterValue(TEXT("TintColorAndOpacity"), Tram::Colors[i]);
			Body->SetMaterial(i, Material);
		}
		else if (BaseMaterial)
		{
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Material->SetVectorParameterValue(TEXT("Color"), Tram::Colors[i]);
			Body->SetMaterial(i, Material);
		}
	}
}

void ATram::SeatPassengers()
{
	while (TakenSeats.Num() > 0)
	{
		RemovePassenger(TakenSeats.Num() - 1);
	}

	FRandomStream Random(FMath::Rand());
	const int32 Count = Random.RandRange(Tram::MinPassengers, Tram::MaxPassengers);
	while (TakenSeats.Num() < Count)
	{
		const int32 Index = Random.RandRange(0, TramModel::NumSeats - 1);
		if (TakenSeats.Contains(Index) || Index == ReservedSeat)
		{
			continue;
		}
		AddPassenger(Index, PersonModel::RandomOutfit(Random));
	}
}

void ATram::AddPassenger(int32 SeatIndex, const PersonKit::FOutfit& Outfit)
{
	const TramModel::FSeat Place = TramModel::Seat(SeatIndex, PersonModel::SeatedMiddle);
	USceneComponent* Seat = NewObject<USceneComponent>(this);
	Seat->SetupAttachment(CollisionBox);
	Seat->SetRelativeLocationAndRotation(Place.At, FRotator(0.0, Place.Yaw, 0.0));
	Seat->RegisterComponent();
	AddInstanceComponent(Seat);

	PersonKit::FOutfit Seated = Outfit;
	Seated.bSeated = true;
	// Hands free on a tram (no umbrella poking through the bench).
	Seated.Held = PersonKit::EHeld::None;
	TArray<TObjectPtr<UProceduralMeshComponent>> Pieces;
	PersonModel::Dress(this, Seat, BaseMaterial, Seated, Pieces);
	Pieces.SetNum(PersonKit::NumPieces);

	TakenSeats.Add(SeatIndex);
	PassengerSeats.Add(Seat);
	PassengerOutfits.Add(Outfit);
	PassengerPieces.Append(Pieces);
}

void ATram::RemovePassenger(int32 Which)
{
	if (!TakenSeats.IsValidIndex(Which))
	{
		return;
	}
	const int32 First = Which * PersonKit::NumPieces;
	for (int32 i = First; i < First + PersonKit::NumPieces && PassengerPieces.IsValidIndex(i); ++i)
	{
		if (PassengerPieces[i])
		{
			PassengerPieces[i]->DestroyComponent();
		}
	}
	PassengerPieces.RemoveAt(First, FMath::Min(static_cast<int32>(PersonKit::NumPieces), PassengerPieces.Num() - First));
	if (PassengerSeats.IsValidIndex(Which) && PassengerSeats[Which])
	{
		PassengerSeats[Which]->DestroyComponent();
	}
	PassengerSeats.RemoveAt(Which);
	PassengerOutfits.RemoveAt(Which);
	TakenSeats.RemoveAt(Which);
}

bool ATram::SeatPassenger(const PersonKit::FOutfit& Outfit)
{
	const int32 Index = GetFreeSeat();
	if (Index == INDEX_NONE)
	{
		return false;
	}
	AddPassenger(Index, Outfit);
	return true;
}

bool ATram::TakeOffPassenger(PersonKit::FOutfit& OutOutfit)
{
	if (TakenSeats.Num() == 0)
	{
		return false;
	}
	const int32 Which = FMath::RandRange(0, TakenSeats.Num() - 1);
	OutOutfit = PassengerOutfits[Which];
	RemovePassenger(Which);
	return true;
}

FVector ATram::GetDoorNear(const FVector& Point) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(Point);
	const double Along = Local.X >= 0.0 ? 1.0 : -1.0;
	const double Side = Local.Y >= 0.0 ? 1.0 : -1.0;
	const FVector Step(Along * (TramModel::DoorFrom + TramModel::DoorTo) * 0.5, Side * (TramModel::HalfWidth + Tram::StepOut), TramModel::Ground);
	return GetActorTransform().TransformPosition(Step);
}

int32 ATram::GetFreeSeat() const
{
	// Nearest the middle of the car first, so the player sits in view.
	for (const int32 Along : { 2, 3, 1, 4, 0, 5 })
	{
		for (const int32 Side : { 0, TramModel::SeatsPerSide })
		{
			if (!TakenSeats.Contains(Along + Side) && Along + Side != ReservedSeat)
			{
				return Along + Side;
			}
		}
	}
	return INDEX_NONE;
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

	SeatPassengers();
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
	UpdateSound();

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
		SoundPull = 0.f;
		SoundBrake = 0.f;
		if (DwellLeft > 0.f)
		{
			return;
		}
		++NextStop;
		// "Ding-ding": off we go.
		if (Sound && bSoundHeard)
		{
			Sound->Ring();
			BellWait = Tram::BellPause;
		}
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
			SoundPull = 0.f;
			SoundBrake = 0.f;
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
	SoundPull = WantedSpeed > CurrentSpeed + 5.f ? 1.f : 0.f;
	SoundBrake = WantedSpeed < CurrentSpeed - 5.f ? 1.f : 0.f;
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
	HitPlayerInTheWay();
	WarnPlayerAhead(DeltaSeconds);
}

void ATram::WarnPlayerAhead(float DeltaSeconds)
{
	BellWait -= DeltaSeconds;
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Sound || !bSoundHeard || BellWait > 0.f || !Player || CurrentSpeed < Tram::HarmlessSpeed)
	{
		return;
	}
	// Someone (on foot or in a car) standing on the rails in front.
	const FVector Local = GetActorTransform().InverseTransformPosition(Player->GetActorLocation());
	if (Local.X > Tram::BoxExtent.X && Local.X < Tram::BoxExtent.X + Tram::WarnReach && FMath::Abs(Local.Y) < Tram::BoxExtent.Y + Tram::HitReach)
	{
		Sound->Ring();
		BellWait = Tram::BellPause;
	}
}

void ATram::UpdateSound()
{
	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const double ToCamera = Camera && IsInService() ? FVector::Dist(Camera->GetCameraLocation(), GetActorLocation()) : Tram::ForgetRange + 1.0;
	if (bSoundHeard && ToCamera > Tram::ForgetRange)
	{
		StopSound();
	}
	else if (!bSoundHeard && ToCamera < Tram::HearRange && Tram::Heard < Tram::MaxHeard)
	{
		if (!Sound)
		{
			Sound = NewObject<UTramSound>(this);
			Sound->SetupAttachment(CollisionBox);
			Sound->bAllowSpatialization = true;
			Sound->bOverrideAttenuation = true;
			Sound->AttenuationOverrides.bAttenuate = true;
			Sound->AttenuationOverrides.bSpatialize = true;
			Sound->AttenuationOverrides.AttenuationShapeExtents = FVector(Tram::BoxExtent.X, 0.0, 0.0);
			Sound->AttenuationOverrides.FalloffDistance = Tram::SoundFalloff;
			Sound->RegisterComponent();
			AddInstanceComponent(Sound);
		}
		Sound->SetVolumeMultiplier(Tram::SoundVolume);
		Sound->Start();
		bSoundHeard = true;
		++Tram::Heard;
	}
	if (bSoundHeard && Sound)
	{
		const float Bend = FMath::Clamp((BendDegrees - Tram::SquealFrom) / (Tram::SquealFull - Tram::SquealFrom), 0.f, 1.f);
		Sound->SetMotion(CurrentSpeed / TramSynth::CruiseSpeed, SoundPull, SoundBrake, Bend);
	}
}

void ATram::StopSound()
{
	if (!bSoundHeard)
	{
		return;
	}
	if (Sound)
	{
		Sound->Stop();
	}
	bSoundHeard = false;
	Tram::Heard = FMath::Max(0, Tram::Heard - 1);
}

void ATram::HitPlayerInTheWay()
{
	APestiPerson* Person = Cast<APestiPerson>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Person || CurrentSpeed < Tram::HarmlessSpeed)
	{
		return;
	}
	// Only the front of the tram, where he would be in its way.
	const FVector Local = GetActorTransform().InverseTransformPosition(Person->GetActorLocation());
	if (Local.X < Tram::BoxExtent.X - Tram::FrontReach || Local.X > Tram::BoxExtent.X + Tram::HitReach
		|| FMath::Abs(Local.Y) > Tram::BoxExtent.Y + Tram::HitReach || FMath::Abs(Local.Z) > Tram::BoxExtent.Z + Tram::HitReach)
	{
		return;
	}
	// Knocked on ahead and out to the side he stands on.
	const FVector Push = GetActorForwardVector() + GetActorRightVector() * (Local.Y >= 0.0 ? 1.0 : -1.0);
	Person->HitByVehicle(CurrentSpeed, Push);
}

void ATram::PlaceOnTrack()
{
	const FTramRoute& Route = Manager->GetTramRoute(RouteIndex);
	FVector Location = Route.LocationAt(Distance);
	const FVector Ahead = Route.LocationAt(Distance + Tram::AxleSpread);
	const FVector Behind = Route.LocationAt(Distance - Tram::AxleSpread);
	Location.Z = GroundZ + Tram::BoxExtent.Z + 2.0;
	const FRotator Facing = (Ahead - Behind).GetSafeNormal2D().Rotation();
	const FVector FrontHalf = (Ahead - Location).GetSafeNormal2D();
	const FVector BackHalf = (Location - Behind).GetSafeNormal2D();
	BendDegrees = static_cast<float>(FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(FrontHalf, BackHalf), -1.0, 1.0))));
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
