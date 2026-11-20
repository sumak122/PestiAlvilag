// Fill out your copyright notice in the Description page of Project Settings.

#include "Motorbike.h"

#include "Camera/PlayerCameraManager.h"
#include "CarBody.h"
#include "CarModel.h"
#include "CarVisuals.h"
#include "CarjackedDriver.h"
#include "Components/BoxComponent.h"
#include "DailyLifeModel.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MotorbikeModel.h"
#include "Pedestrians.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PolicePatrols.h"
#include "ProceduralMeshComponent.h"
#include "SpotSound.h"
#include "StreetName.h"
#include "Witnesses.h"

namespace MotorbikeRun
{
	// Its own top speed and pull (cm/s, cm/s each second), before the small car's shares (CarHandling).
	constexpr float TopSpeed = 2500.f;
	constexpr float Pull = 1100.f;
	constexpr float TurnRate = 95.f;
	// The fork turns this far at the fastest turning (degrees).
	constexpr float MaxSteer = 24.f;
	// A honk lasts this long (s), heard this far (cm).
	constexpr float HonkSeconds = 0.9f;
	constexpr double HornReach = 5000.0;
	// The owner comes out of a doorway this far behind and to the kerb side (cm); the police see it from this far.
	constexpr double OwnerBack = 500.0;
	constexpr double OwnerKerb = 260.0;
	constexpr double TheftSeen = 2500.0;
}

AMotorbike::AMotorbike()
{
	MaxSpeed = MotorbikeRun::TopSpeed;
	Acceleration = MotorbikeRun::Pull;
	MaxTurnRate = MotorbikeRun::TurnRate;
	// It handles like the smallest car there is, a Balilla.
	CarModel::FStyle Look;
	Look.Body = CarModel::EBody::Balilla;
	Look.PaintColor = MotorbikeModel::Enamel();
	SetStyle(Look);
	BaseMaterial = CarBody::LoadPaintMaterial();
}

void AMotorbike::SetLook(bool bInSidecar, int32 InPaint)
{
	bSidecar = bInSidecar;
	PaintIndex = InPaint;
}

void AMotorbike::BeginPlay()
{
	Super::BeginPlay();
	using namespace MotorbikeModel;

	// The car's own looks go; the box takes the outfit's length and width.
	Visuals->DrawNothing();
	const FVector Half = OutfitHalf(bSidecar);
	const double BoxZ = CollisionBox->GetUnscaledBoxExtent().Z;
	CollisionBox->SetBoxExtent(FVector(Half.X, Half.Y, BoxZ));
	FloorZ = -(BoxZ + HoverHeight);
	LastYaw = static_cast<float>(GetActorRotation().Yaw);

	// The machine, the steering, the wheels, each about its own pivot; the front wheel rides on the steering.
	PersonKit::FPiece Shapes[NumParts];
	Build(Shapes, bSidecar, PaintIndex);
	for (int32 p = 0; p < NumParts; ++p)
	{
		UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this);
		if (p == WheelFront && Parts.IsValidIndex(Steer) && Parts[Steer])
		{
			Mesh->SetupAttachment(Parts[Steer]);
			Mesh->SetRelativeLocation(PartAt(WheelFront, bSidecar) - PartAt(Steer, bSidecar));
		}
		else
		{
			Mesh->SetupAttachment(CollisionBox);
			Mesh->SetRelativeLocation(FVector(0.0, 0.0, FloorZ) + PartAt(p, bSidecar));
		}
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
}

void AMotorbike::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(Rider))
	{
		Rider->Destroy();
	}
	Rider = nullptr;
	Super::EndPlay(EndPlayReason);
}

void AMotorbike::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	using namespace MotorbikeModel;

	APlayerController* PC = Cast<APlayerController>(GetController());
	const bool bDriven = PC != nullptr;
	if (bDriven && !bTaken)
	{
		bTaken = true;
		if (bOwned)
		{
			bOwned = false;
			Stolen();
		}
	}

	// The wheels roll as far as it goes.
	WheelTurn = FMath::Fmod(WheelTurn + ForwardSpeed * DeltaSeconds, static_cast<float>(2.0 * PI * WheelRadius));
	const float Spin = -FMath::RadiansToDegrees(WheelTurn / static_cast<float>(WheelRadius));
	const float SideSpin = -FMath::RadiansToDegrees(WheelTurn / static_cast<float>(SideWheelRadius));
	for (const int32 w : { static_cast<int32>(WheelFront), static_cast<int32>(WheelRear), static_cast<int32>(WheelSide) })
	{
		if (Parts.IsValidIndex(w) && Parts[w])
		{
			Parts[w]->SetRelativeRotation(FRotator(w == WheelSide ? SideSpin : Spin, 0.f, 0.f));
		}
	}

	// The fork follows the way it turns.
	const float NowYaw = static_cast<float>(GetActorRotation().Yaw);
	const float YawRate = DeltaSeconds > 0.f ? FMath::FindDeltaAngleDegrees(LastYaw, NowYaw) / DeltaSeconds : 0.f;
	LastYaw = NowYaw;
	const float Moving = FMath::Clamp(FMath::Abs(ForwardSpeed) / 300.f, 0.2f, 1.f);
	const float WantSteer = FMath::Clamp(YawRate / (MotorbikeRun::TurnRate * Moving), -1.f, 1.f) * MotorbikeRun::MaxSteer
		* (ForwardSpeed < 0.f ? -1.f : 1.f);
	SteerAngle = FMath::FInterpTo(SteerAngle, WantSteer, DeltaSeconds, 8.f);
	if (Parts.IsValidIndex(Steer) && Parts[Steer])
	{
		Parts[Steer]->SetRelativeRotation(FQuat(SteerAxis(bSidecar), FMath::DegreesToRadians(SteerAngle)));
	}

	const bool bHonk = bDriven && (PC->WasInputKeyJustPressed(EKeys::B) || PC->WasInputKeyJustPressed(EKeys::Gamepad_DPad_Down));
	UpdateHorn(DeltaSeconds, bHonk);
	PlaceRider(bDriven);
}

void AMotorbike::UpdateHorn(float DeltaSeconds, bool bPressed)
{
	HornTime = FMath::Max(0.f, HornTime - DeltaSeconds);
	if (bPressed)
	{
		HornTime = MotorbikeRun::HonkSeconds;
		if (!Horn)
		{
			Horn = NewObject<USpotSound>(this);
			Horn->SetupAttachment(CollisionBox);
			Horn->RegisterComponent();
			Horn->SetHeard(0.f, 1.f);
			Horn->Start();
		}
		// Each honk starts the sound afresh.
		Horn->SetSpot(CitySynth::ESpot::MotorHorn, ++Honks);
	}
	if (!Horn)
	{
		return;
	}
	float Gain = 0.f;
	float Far = 1.f;
	if (HornTime > 0.f)
	{
		if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			const double Distance = FVector::Dist(Camera->GetCameraLocation(), GetActorLocation());
			Far = static_cast<float>(FMath::Clamp(Distance / MotorbikeRun::HornReach, 0.0, 1.0));
			Gain = 1.f - Far;
		}
	}
	Horn->SetHeard(Gain, Far);
}

void AMotorbike::PlaceRider(bool bDriven)
{
	if (!bDriven)
	{
		if (IsValid(Rider) && !Rider->IsHidden())
		{
			Rider->SetActorHiddenInGame(true);
		}
		return;
	}
	if (!IsValid(Rider))
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Rider = GetWorld()->SpawnActor<APedestrian>(APedestrian::StaticClass(), GetActorLocation(), GetActorRotation(), Params);
		if (!Rider)
		{
			return;
		}
		Rider->SetOutfit(PersonModel::PlayerOutfit());
	}
	if (Rider->IsHidden())
	{
		Rider->SetActorHiddenInGame(false);
	}
	// Astride the saddle, feet on the rests, leaning a little to the bars.
	DailyLifeModel::FLimbs Pose;
	DailyLifeModel::Seated(Pose, 0.f);
	Pose.Legs[0] = 55.f;
	Pose.Legs[1] = 55.f;
	Pose.Knees[0] = -75.f;
	Pose.Knees[1] = -75.f;
	Pose.Arms[0] = FRotator(62.f, 0.f, 14.f);
	Pose.Arms[1] = FRotator(62.f, 0.f, -14.f);
	Pose.Elbows[0] = 18.f;
	Pose.Elbows[1] = 18.f;
	Pose.Pitch = 14.f;
	const FTransform Frame = GetActorTransform();
	Rider->Strike(Frame.TransformPosition(FVector(0.0, 0.0, FloorZ) + MotorbikeModel::RiderSeat(bSidecar)), Frame.Rotator().Yaw, Pose);
}

void AMotorbike::Stolen()
{
	const FVector Location = GetActorLocation();
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector Danger = Player ? Player->GetActorLocation() : Location;

	// The owner comes running out of a doorway behind, on the kerb side, shouting after it.
	const FVector Back = -GetActorForwardVector().GetSafeNormal2D();
	const FVector Kerb = -GetActorRightVector().GetSafeNormal2D();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector OwnerAt = Location + Back * MotorbikeRun::OwnerBack + Kerb * MotorbikeRun::OwnerKerb;
	if (ACarjackedDriver* Robbed = GetWorld()->SpawnActor<ACarjackedDriver>(ACarjackedDriver::StaticClass(), OwnerAt, GetActorRotation(), Params))
	{
		FRandomStream Dice(FMath::Rand());
		PersonKit::FOutfit Clothes = PersonModel::RandomOutfit(Dice);
		Clothes.Held = PersonKit::EHeld::None;
		Robbed->SetOutfit(Clothes);
		Robbed->AfterThief(Danger, true, Clothes);
	}
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->Say(TEXT("Tulajdonos"), TEXT("H\u00e9! A motorom! Tolvaj!"), TEXT("Hey! My motorbike! Thief!"), 3.f);
	}

	APestiAlvilagGameModeBase::Deed(this, StreetName::EDeed::CarStolen, Location);
	if (APolicePatrols::Witnessed(this, Location, MotorbikeRun::TheftSeen))
	{
		if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
		{
			Game->ReportCrime(1);
		}
	}
	else
	{
		AWitnesses::Report(this, Location, 1);
	}
}
