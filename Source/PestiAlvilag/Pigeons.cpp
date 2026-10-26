// Fill out your copyright notice in the Description page of Project Settings.

#include "Pigeons.h"

#include "Camera/PlayerCameraManager.h"
#include "CarBody.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "OsmGraybox.h"
#include "PestiCar.h"
#include "ProceduralMeshComponent.h"

namespace Pigeons
{
	struct FPlace
	{
		FVector2D Middle;
		int32 Count;
	};

	// The squares they keep to (cm, X east, Y south).
	const FPlace Places[] = {
		// Blaha Lujza tér, in front of the Népszínház.
		{ FVector2D(-6500.0, 1500.0), 14 },
		// Tisza Kálmán tér, on the paths of the park.
		{ FVector2D(60000.0, 500.0), 12 },
		// The Teleki tér market, among the stalls.
		{ FVector2D(99969.0, 28889.0), 10 },
		// Baross tér, round the statue.
		{ FVector2D(90531.0, -35706.0), 12 },
		// The forecourt of Keleti.
		{ FVector2D(91000.0, -41600.0), 14 },
	};

	constexpr double SpreadOver = 450.0;
	constexpr int32 SpotsPerFlock = 28;

	// On the ground: a slow strut, pecks now and then.
	constexpr float StrutSpeed = 32.f;
	constexpr double StrutStep = 110.0;
	constexpr float BobRate = 9.f;
	constexpr float BobAngle = 10.f;
	constexpr float PeckAngle = -55.f;
	constexpr float PeckRate = 7.f;

	// What frightens them: a car coming up, someone running at them, or anyone right beside them.
	constexpr double CarFrighten = 1300.0;
	constexpr double CarFrightenSpeed = 150.0;
	constexpr double RunFrighten = 520.0;
	constexpr double RunFrightenSpeed = 380.0;
	constexpr double CloseFrighten = 170.0;
	constexpr float MaxStartleDelay = 0.4f;

	// Up and away, then round and round the square, high up.
	constexpr float TakeOffSpeed = 320.f;
	constexpr float TakeOffClimb = 430.f;
	constexpr float FlySpeed = 700.f;
	constexpr double CircleRadius = 1400.0;
	constexpr double CircleHeight = 950.0;
	constexpr float Steer = 1.3f;
	constexpr float MinAirtime = 10.f;
	constexpr float MaxAirtime = 18.f;
	// They only come back down once nothing has frightened them for this long (s).
	constexpr float CalmBeforeLanding = 5.f;
	constexpr float LandSpeed = 420.f;
	constexpr double Touchdown = 25.0;

	// The wings beat this fast (beats a second) and this far (degrees).
	constexpr float FlapRate = 7.f;
	constexpr float FlapAngle = 50.f;
}

APigeons::APigeons()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	BaseMaterial = CarBody::LoadPaintMaterial();
}

UMaterialInstanceDynamic* APigeons::Paint(const FLinearColor& Color)
{
	const FString Key = Color.ToString();
	if (UMaterialInstanceDynamic** Found = Paints.Find(Key))
	{
		return *Found;
	}
	UMaterialInstanceDynamic* Made = BaseMaterial ? UMaterialInstanceDynamic::Create(BaseMaterial, this) : nullptr;
	if (Made)
	{
		Made->SetVectorParameterValue(TEXT("Color"), Color);
	}
	Paints.Add(Key, Made);
	return Made;
}

void APigeons::BeginPlay()
{
	Super::BeginPlay();

	Random.Initialize(1932);
	for (int32 k = 0; k < static_cast<int32>(PigeonModel::EPlumage::Count); ++k)
	{
		PigeonModel::Build(Plumages.AddDefaulted_GetRef(), static_cast<PigeonModel::EPlumage>(k));
	}
	for (const Pigeons::FPlace& Place : Pigeons::Places)
	{
		AddFlock(Place.Middle, Place.Count);
	}
}

bool APigeons::GroundAt(const FVector2D& At, FVector& OutGround) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PigeonGround), false, this);
	if (!GetWorld()->LineTraceSingleByChannel(Hit, FVector(At, 20000.0), FVector(At, -5000.0), ECC_Visibility, Params))
	{
		return false;
	}
	// Only the open ground: not a roof, a bench, a stall or a statue.
	if (!Cast<AOsmGraybox>(Hit.GetActor()) || Hit.ImpactNormal.Z < 0.9)
	{
		return false;
	}
	OutGround = Hit.ImpactPoint;
	return true;
}

void APigeons::AddFlock(const FVector2D& Middle, int32 Count)
{
	FFlock Flock;
	for (int32 Try = 0; Try < Pigeons::SpotsPerFlock * 4 && Flock.Spots.Num() < Pigeons::SpotsPerFlock; ++Try)
	{
		const FVector2D At = Middle + FVector2D(Random.FRandRange(-1.f, 1.f), Random.FRandRange(-1.f, 1.f)) * Pigeons::SpreadOver;
		FVector Ground;
		if (GroundAt(At, Ground))
		{
			Flock.Spots.Add(Ground);
		}
	}
	if (Flock.Spots.Num() < 3)
	{
		return;
	}
	FVector Sum = FVector::ZeroVector;
	for (const FVector& Spot : Flock.Spots)
	{
		Sum += Spot;
	}
	Flock.Middle = Sum / Flock.Spots.Num();

	for (int32 n = 0; n < Count; ++n)
	{
		FBird Bird;
		// Mostly blue-grey ones, some dark or chequered, now and then a pale one.
		const float Pick = Random.FRand();
		const int32 Plumage = static_cast<int32>(Pick < 0.5f ? PigeonModel::EPlumage::Blue : Pick < 0.7f ? PigeonModel::EPlumage::Chequer
			: Pick < 0.92f ? PigeonModel::EPlumage::Dark : PigeonModel::EPlumage::Pale);
		Bird.Root = NewObject<USceneComponent>(this);
		Bird.Root->SetupAttachment(RootComponent);
		Bird.Root->SetMobility(EComponentMobility::Movable);
		Bird.Root->RegisterComponent();
		AddInstanceComponent(Bird.Root);
		const PigeonModel::FPigeon& Model = Plumages[Plumage];
		for (int32 p = 0; p < PigeonModel::NumParts; ++p)
		{
			const PersonKit::FPiece& Piece = Model.Parts[p];
			UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this);
			Mesh->SetupAttachment(Bird.Root);
			Mesh->SetRelativeLocation(Piece.Pivot);
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Mesh->SetCanEverAffectNavigation(false);
			Mesh->SetCachedMaxDrawDistance(ShowWithin);
			for (int32 s = 0; s < Piece.Sections.Num(); ++s)
			{
				const PersonKit::FSection& Section = Piece.Sections[s];
				Mesh->CreateMeshSection_LinearColor(s, Section.Vertices, Section.Triangles, Section.Normals,
					TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
				Mesh->SetMaterial(s, Paint(Section.Color));
			}
			Mesh->RegisterComponent();
			AddInstanceComponent(Mesh);
			Bird.Parts[p] = Mesh;
		}
		Bird.At = Flock.Spots[Random.RandRange(0, Flock.Spots.Num() - 1)] + FVector(Random.FRandRange(-40.f, 40.f), Random.FRandRange(-40.f, 40.f), 0.0);
		Bird.Target = Bird.At;
		Bird.Yaw = Random.FRandRange(0.f, 360.f);
		Bird.Phase = Random.FRandRange(0.f, 2.f * PI);
		Bird.Busy = Random.FRandRange(0.f, 2.f);
		Bird.Circling = Random.FRand() < 0.5f ? 1.f : -1.f;
		Flock.Birds.Add(Birds.Add(Bird));
		PoseBird(Birds.Last());
	}
	Flocks.Add(Flock);
}

void APigeons::Startle(FFlock& Flock, const FVector& From)
{
	Flock.Calm = 0.f;
	for (const int32 Index : Flock.Birds)
	{
		FBird& Bird = Birds[Index];
		if (Bird.State == EState::Ground || Bird.State == EState::Landing)
		{
			// The nearest go first, the rest a moment later.
			const double Near = FVector::Dist2D(Bird.At, From);
			Bird.Delay = Bird.State == EState::Landing ? 0.f : FMath::Min(Pigeons::MaxStartleDelay, static_cast<float>(Near / 2500.0) + Random.FRandRange(0.f, 0.2f));
			Bird.State = EState::Startled;
			const FVector2D Away = FVector2D(Bird.At - From).GetSafeNormal();
			Bird.Velocity = FVector(Away.X, Away.Y, 0.0);
		}
	}
}

void APigeons::MoveBird(FBird& Bird, const FFlock& Flock, float DeltaSeconds)
{
	switch (Bird.State)
	{
	case EState::Ground:
	{
		Bird.Busy -= DeltaSeconds;
		const FVector ToTarget = Bird.Target - Bird.At;
		if (!Bird.bPecking && ToTarget.Size2D() > 4.0)
		{
			// Strut over, head bobbing.
			const FVector Step = ToTarget.GetSafeNormal2D() * FMath::Min(static_cast<double>(Pigeons::StrutSpeed * DeltaSeconds), ToTarget.Size2D());
			Bird.At += Step;
			Bird.Yaw = FMath::FixedTurn(Bird.Yaw, static_cast<float>(Step.Rotation().Yaw), 400.f * DeltaSeconds);
			Bird.Phase = FMath::Fmod(Bird.Phase + DeltaSeconds * Pigeons::BobRate, 200.f * PI);
		}
		else if (Bird.Busy <= 0.f)
		{
			// Peck here a while, or strut on to somewhere near.
			Bird.bPecking = Random.FRand() < 0.6f;
			Bird.Busy = Random.FRandRange(0.6f, 2.0f);
			if (!Bird.bPecking)
			{
				const FVector Spot = Flock.Spots[Random.RandRange(0, Flock.Spots.Num() - 1)];
				const FVector Toward = (Spot - Bird.At).GetClampedToMaxSize2D(Pigeons::StrutStep);
				Bird.Target = Bird.At + FVector(Toward.X, Toward.Y, 0.0);
				Bird.Target.Z = FMath::Lerp(Bird.At.Z, Spot.Z, FMath::Clamp(Toward.Size2D() / FMath::Max(FVector::Dist2D(Spot, Bird.At), 1.0), 0.0, 1.0));
			}
			else
			{
				Bird.Target = Bird.At;
			}
		}
		if (Bird.bPecking)
		{
			Bird.Phase += DeltaSeconds * Pigeons::PeckRate;
		}
		break;
	}
	case EState::Startled:
		Bird.Delay -= DeltaSeconds;
		if (Bird.Delay <= 0.f)
		{
			// Off the ground with a clatter: away from the fright and up.
			const FVector Away = (Bird.Velocity + FVector(Random.FRandRange(-0.4f, 0.4f), Random.FRandRange(-0.4f, 0.4f), 0.0)).GetSafeNormal2D();
			Bird.Velocity = Away * Pigeons::TakeOffSpeed + FVector(0.0, 0.0, Pigeons::TakeOffClimb);
			Bird.State = EState::Flying;
			Bird.Up = 0.f;
			Bird.Airtime = Random.FRandRange(Pigeons::MinAirtime, Pigeons::MaxAirtime);
		}
		break;
	case EState::Flying:
	{
		Bird.Up += DeltaSeconds;
		// Round the square, high over it.
		const FVector Out = FVector(Bird.At.X - Flock.Middle.X, Bird.At.Y - Flock.Middle.Y, 0.0);
		const double Radius = FMath::Max(Out.Size(), 1.0);
		const FVector Radial = Out / Radius;
		const FVector Round = FVector(-Radial.Y, Radial.X, 0.0) * Bird.Circling;
		const double Height = Flock.Middle.Z + Pigeons::CircleHeight + 120.0 * FMath::Sin(Bird.Phase * 0.05f);
		FVector Wanted = (Round + Radial * FMath::Clamp((Pigeons::CircleRadius - Radius) / Pigeons::CircleRadius, -1.0, 1.0)).GetSafeNormal() * Pigeons::FlySpeed;
		Wanted.Z = FMath::Clamp((Height - Bird.At.Z) * 0.8, -250.0, 350.0);
		Bird.Velocity = FMath::Lerp(Bird.Velocity, Wanted, FMath::Min(1.f, Pigeons::Steer * DeltaSeconds));
		Bird.At += Bird.Velocity * DeltaSeconds;
		Bird.Phase += DeltaSeconds * Pigeons::FlapRate * 2.f * PI;
		if (Bird.Up > Bird.Airtime && Flock.Calm > Pigeons::CalmBeforeLanding)
		{
			Bird.State = EState::Landing;
			Bird.Target = Flock.Spots[Random.RandRange(0, Flock.Spots.Num() - 1)] + FVector(Random.FRandRange(-40.f, 40.f), Random.FRandRange(-40.f, 40.f), 0.0);
		}
		break;
	}
	case EState::Landing:
	{
		// Glide down to a spot, slowing and beating hard at the last.
		const FVector ToSpot = Bird.Target - Bird.At;
		const double Gap = ToSpot.Size();
		if (Gap < Pigeons::Touchdown)
		{
			Bird.At = Bird.Target;
			Bird.Velocity = FVector::ZeroVector;
			Bird.State = EState::Ground;
			Bird.bPecking = false;
			Bird.Busy = Random.FRandRange(0.5f, 1.5f);
			break;
		}
		const FVector Wanted = ToSpot / Gap * FMath::Clamp(Gap * 0.6, 120.0, static_cast<double>(Pigeons::LandSpeed));
		Bird.Velocity = FMath::Lerp(Bird.Velocity, Wanted, FMath::Min(1.f, 2.f * DeltaSeconds));
		Bird.At += Bird.Velocity * DeltaSeconds;
		Bird.Phase += DeltaSeconds * Pigeons::FlapRate * (Gap < 300.0 ? 1.6f : 0.6f) * 2.f * PI;
		break;
	}
	}
}

void APigeons::PoseBird(FBird& Bird)
{
	const bool bAloft = Bird.State == EState::Flying || Bird.State == EState::Landing;
	float Pitch = 0.f;
	if (bAloft && Bird.Velocity.SizeSquared2D() > 1.0)
	{
		Bird.Yaw = static_cast<float>(Bird.Velocity.Rotation().Yaw);
		Pitch = FMath::Clamp(static_cast<float>(Bird.Velocity.Rotation().Pitch), -25.f, 30.f);
	}
	Bird.Root->SetWorldLocationAndRotation(Bird.At, FRotator(Pitch, Bird.Yaw, 0.f));

	using namespace PigeonModel;
	Bird.Parts[Folded]->SetVisibility(!bAloft);
	Bird.Parts[WingLeft]->SetVisibility(bAloft);
	Bird.Parts[WingRight]->SetVisibility(bAloft);
	if (bAloft)
	{
		// Up is a negative roll for the right wing, a positive one for the left.
		const float Beat = Pigeons::FlapAngle * FMath::Sin(Bird.Phase) + 10.f;
		Bird.Parts[WingLeft]->SetRelativeRotation(FRotator(0.f, 0.f, Beat));
		Bird.Parts[WingRight]->SetRelativeRotation(FRotator(0.f, 0.f, -Beat));
		Bird.Parts[Head]->SetRelativeRotation(FRotator(10.f, 0.f, 0.f));
	}
	else
	{
		const float Head = Bird.bPecking ? Pigeons::PeckAngle * FMath::Abs(FMath::Sin(Bird.Phase)) : Pigeons::BobAngle * FMath::Sin(Bird.Phase);
		Bird.Parts[PigeonModel::Head]->SetRelativeRotation(FRotator(Head, 0.f, 0.f));
	}
}

void APigeons::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (DeltaSeconds <= 0.f)
	{
		return;
	}

	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}
	const FVector Camera = PC->PlayerCameraManager->GetCameraLocation();

	// The player, on foot or in his car, and how fast he goes.
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	FVector PlayerAt = Camera;
	bool bInCar = false;
	if (Player)
	{
		PlayerAt = Player->GetActorLocation();
		if (LastPlayer.Get() != Player)
		{
			LastPlayer = Player;
			LastPlayerAt = PlayerAt;
		}
		PlayerVelocity = FMath::Lerp(PlayerVelocity, (PlayerAt - LastPlayerAt) / DeltaSeconds, 0.3);
		LastPlayerAt = PlayerAt;
		bInCar = Player->IsA<APestiCar>();
	}
	const double PlayerSpeed = PlayerVelocity.Size2D();

	for (FFlock& Flock : Flocks)
	{
		const bool bShow = FVector::Dist(Flock.Middle, Camera) < ShowWithin;
		if (bShow != Flock.bShown)
		{
			Flock.bShown = bShow;
			for (const int32 Index : Flock.Birds)
			{
				Birds[Index].Root->SetVisibility(bShow, true);
				if (bShow)
				{
					PoseBird(Birds[Index]);
				}
			}
		}
		if (!bShow)
		{
			continue;
		}

		Flock.Calm += DeltaSeconds;
		if (Player)
		{
			// Frightened by any of them being frightened: the whole flock goes up.
			bool bFright = false;
			for (const int32 Index : Flock.Birds)
			{
				const FBird& Bird = Birds[Index];
				if (Bird.State == EState::Flying && Bird.Up > 3.f)
				{
					continue;
				}
				const double Near = FVector::Dist(Bird.At, PlayerAt);
				bFright = bFright
					|| (bInCar && Near < Pigeons::CarFrighten && PlayerSpeed > Pigeons::CarFrightenSpeed)
					|| (!bInCar && Near < Pigeons::RunFrighten && PlayerSpeed > Pigeons::RunFrightenSpeed)
					|| (!bInCar && Near < Pigeons::CloseFrighten);
				if (bFright)
				{
					break;
				}
			}
			if (bFright)
			{
				Startle(Flock, PlayerAt);
			}
		}

		for (const int32 Index : Flock.Birds)
		{
			MoveBird(Birds[Index], Flock, DeltaSeconds);
			PoseBird(Birds[Index]);
		}
	}
}
