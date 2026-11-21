// Fill out your copyright notice in the Description page of Project Settings.

#include "FireTruck.h"

#include "Camera/PlayerCameraManager.h"
#include "CarBody.h"
#include "CarModel.h"
#include "CarVisuals.h"
#include "Components/BoxComponent.h"
#include "DailyLifeModel.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FireEngine.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Pedestrians.h"
#include "PersonModel.h"
#include "ProceduralMeshComponent.h"
#include "SpotSound.h"
#include "TrafficManager.h"

namespace FireSteer
{
	// The front wheels turn this far at their sharpest (degrees).
	constexpr float MaxSteer = 28.f;
}

namespace FireTruckRun
{
	// Its own top speed and pull (cm/s, cm/s each second), before the heavy van's shares (CarHandling).
	constexpr float TopSpeed = 2300.f;
	constexpr float Pull = 700.f;
	// The bell is heard this far (cm).
	constexpr double BellReach = 9000.0;
	// The road block it carries ahead of itself while the bell rings (its own slot, apart from the brigade's engine).
	constexpr int32 BlockSlot = 4;
	const double BlockAhead[] = { 500.0, 1200.0, 2000.0 };
}

AFireTruck::AFireTruck()
{
	MaxSpeed = FireTruckRun::TopSpeed;
	Acceleration = FireTruckRun::Pull;
	// The heaviest kind of car there is: it handles like a loaded van.
	CarModel::FStyle Look;
	Look.Body = CarModel::EBody::Van;
	Look.PaintColor = DailyLifeModel::FireRed();
	SetStyle(Look);
	BaseMaterial = CarBody::LoadPaintMaterial();
}

void AFireTruck::BeginPlay()
{
	Super::BeginPlay();
	LastYaw = static_cast<float>(GetActorRotation().Yaw);
	using namespace DailyLifeModel;

	// The car's own looks go; the box takes the engine's length and width.
	Visuals->DrawNothing();
	const FVector Half = FireHalf();
	const double BoxZ = CollisionBox->GetUnscaledBoxExtent().Z;
	CollisionBox->SetBoxExtent(FVector(Half.X, Half.Y, BoxZ));
	FloorZ = -(BoxZ + HoverHeight);

	// The body, the wheels and the bell, each about its own pivot.
	PersonKit::FPiece Shapes[NumFireParts];
	FireEngine(Shapes);
	for (int32 p = 0; p < NumFireParts; ++p)
	{
		UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this);
		Mesh->SetupAttachment(CollisionBox);
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
				UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
				Paint->SetVectorParameterValue(TEXT("Color"), Section.Color);
				Mesh->SetMaterial(s, Paint);
			}
		}
		FVector Pivot = FVector::ZeroVector;
		if (p >= FireWheelFrontLeft && p <= FireWheelBackRight)
		{
			Pivot = FireWheelHub(p);
		}
		else if (p == FireBell)
		{
			Pivot = FireBellAt();
		}
		Mesh->SetRelativeLocation(FVector(0.0, 0.0, FloorZ) + Pivot);
		Mesh->RegisterComponent();
		AddInstanceComponent(Mesh);
		Parts.Add(Mesh);
	}
}

void AFireTruck::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bBlocking && Traffic.IsValid())
	{
		Traffic->SetRoadBlock(FireTruckRun::BlockSlot, TArray<FVector>());
	}
	if (IsValid(WheelMan))
	{
		WheelMan->Destroy();
	}
	WheelMan = nullptr;
	Super::EndPlay(EndPlayReason);
}

ATrafficManager* AFireTruck::FindTraffic()
{
	if (!Traffic.IsValid())
	{
		TActorIterator<ATrafficManager> It(GetWorld());
		Traffic = It ? *It : nullptr;
	}
	return Traffic.Get();
}

void AFireTruck::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = Cast<APlayerController>(GetController());
	const bool bDriven = PC != nullptr;
	if (bDriven && (PC->WasInputKeyJustPressed(EKeys::B) || PC->WasInputKeyJustPressed(EKeys::Gamepad_DPad_Down)))
	{
		bBellOn = !bBellOn;
	}
	if (!bDriven)
	{
		bBellOn = false;
	}

	// The wheels roll as far as it goes.
	using namespace DailyLifeModel;
	WheelTurn = FMath::Fmod(WheelTurn + ForwardSpeed * DeltaSeconds, static_cast<float>(2.0 * PI * FireWheelRadius));
	const float Spin = -FMath::RadiansToDegrees(WheelTurn / static_cast<float>(FireWheelRadius));
	// The front wheels turn the way it turns.
	const float NowYaw = static_cast<float>(GetActorRotation().Yaw);
	const float YawRate = DeltaSeconds > 0.f ? FMath::FindDeltaAngleDegrees(LastYaw, NowYaw) / DeltaSeconds : 0.f;
	LastYaw = NowYaw;
	const float Moving = FMath::Clamp(FMath::Abs(ForwardSpeed) / 300.f, 0.2f, 1.f);
	const float WantSteer = FMath::Clamp(YawRate / (MaxTurnRate * Moving), -1.f, 1.f) * FireSteer::MaxSteer * (ForwardSpeed < 0.f ? -1.f : 1.f);
	SteerAngle = FMath::FInterpTo(SteerAngle, WantSteer, DeltaSeconds, 8.f);
	for (int32 w = FireWheelFrontLeft; w <= FireWheelBackRight; ++w)
	{
		if (Parts.IsValidIndex(w) && Parts[w])
		{
			const bool bFront = w == FireWheelFrontLeft || w == FireWheelFrontRight;
			Parts[w]->SetRelativeRotation(FRotator(Spin, bFront ? SteerAngle : 0.f, 0.f));
		}
	}

	UpdateBell(DeltaSeconds);
	PlaceWheelMan(bDriven);
}

void AFireTruck::UpdateBell(float DeltaSeconds)
{
	using namespace DailyLifeModel;
	BellClock += DeltaSeconds;
	const float Swing = bBellOn ? 22.f * FMath::Sin(BellClock * 9.f) : 0.f;
	if (Parts.IsValidIndex(FireBell) && Parts[FireBell])
	{
		Parts[FireBell]->SetRelativeRotation(FRotator(0.f, 0.f, Swing));
	}

	// The sound, made the first time it rings: heard round the engine, louder near.
	if (bBellOn && !Bell)
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
		if (bBellOn)
		{
			if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
			{
				const double Distance = FVector::Dist(Camera->GetCameraLocation(), GetActorLocation());
				Far = static_cast<float>(FMath::Clamp(Distance / FireTruckRun::BellReach, 0.0, 1.0));
				Gain = Distance < FireTruckRun::BellReach ? 1.f - Far : 0.f;
			}
		}
		Bell->SetHeard(Gain, Far);
	}

	// The traffic ahead stops for it while it rings.
	if (bBellOn || bBlocking)
	{
		if (ATrafficManager* Manager = FindTraffic())
		{
			TArray<FVector> Points;
			if (bBellOn)
			{
				const FVector Ahead = GetActorForwardVector().GetSafeNormal2D() * (ForwardSpeed < 0.f ? -1.0 : 1.0);
				for (const double Distance : FireTruckRun::BlockAhead)
				{
					Points.Add(GetActorLocation() + Ahead * Distance);
				}
			}
			Manager->SetRoadBlock(FireTruckRun::BlockSlot, Points);
			bBlocking = bBellOn;
		}
	}
}

void AFireTruck::PlaceWheelMan(bool bDriven)
{
	using namespace DailyLifeModel;
	if (!bDriven)
	{
		if (IsValid(WheelMan) && !WheelMan->IsHidden())
		{
			WheelMan->SetActorHiddenInGame(true);
		}
		return;
	}
	if (!IsValid(WheelMan))
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		WheelMan = GetWorld()->SpawnActor<APedestrian>(APedestrian::StaticClass(), GetActorLocation(), GetActorRotation(), Params);
		if (!WheelMan)
		{
			return;
		}
		WheelMan->SetOutfit(PersonModel::PlayerOutfit());
	}
	if (WheelMan->IsHidden())
	{
		WheelMan->SetActorHiddenInGame(false);
	}
	// Seated on the bench, the hands on the wheel.
	FLimbs Pose;
	Seated(Pose, 0.f);
	Pose.Arms[0] = FRotator(60.f, 0.f, 10.f);
	Pose.Arms[1] = FRotator(60.f, 0.f, -10.f);
	Pose.Elbows[0] = 30.f;
	Pose.Elbows[1] = 30.f;
	const FTransform Frame = GetActorTransform();
	WheelMan->Strike(Frame.TransformPosition(FVector(0.0, 0.0, FloorZ) + FireDriverSeat()), Frame.Rotator().Yaw, Pose);
}

AFireTruck* AFireTruck::TakeFrom(AFireEngine* Engine)
{
	if (!IsValid(Engine) || !Engine->CanBeTaken())
	{
		return nullptr;
	}
	UWorld* World = Engine->GetWorld();
	const FVector At = Engine->GetActorLocation();
	const FRotator Facing(0.0, Engine->GetActorRotation().Yaw, 0.0);
	// The brigade's own engine goes (hidden, out of the way), its crew left standing in the road.
	Engine->HandOver();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<AFireTruck>(AFireTruck::StaticClass(), At + FVector(0.0, 0.0, 30.0), Facing, Params);
}
