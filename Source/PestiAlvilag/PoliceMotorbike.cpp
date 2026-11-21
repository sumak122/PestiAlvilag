// Fill out your copyright notice in the Description page of Project Settings.

#include "PoliceMotorbike.h"

#include "Camera/PlayerCameraManager.h"
#include "CarVisuals.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "DailyLifeModel.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MotorbikeModel.h"
#include "Pedestrians.h"
#include "PolicePatrols.h"
#include "ProceduralMeshComponent.h"

namespace PoliceMotorbikeRun
{
	// The box's middle is this far over the road (ATrafficManager's ride height), its half height (cm).
	constexpr double FloorZ = -90.0;
	constexpr double BoxHalfZ = 70.0;
	// The fork turns this far at its sharpest (degrees), at this turn rate (degrees/s).
	constexpr float MaxSteer = 24.f;
	constexpr float FullTurn = 150.f;
	// The rider is posed only while the camera is this near (cm).
	constexpr double PoseRange = 12000.0;
}

APoliceMotorbike::APoliceMotorbike()
{
	// Quicker and nimbler than the cars, one constable, only from two stars.
	ChaseSpeed = 2850.f;
	ChaseTurnRate = 200.f;
	NumOfficers = 1;
	MinStars = 2;
	const FVector Half = MotorbikeModel::OutfitHalf(false);
	CollisionBox->SetBoxExtent(FVector(Half.X, Half.Y + 6.0, PoliceMotorbikeRun::BoxHalfZ));
	// No roof sign or trafficators on a motorbike.
	for (USceneComponent* Part : { static_cast<USceneComponent*>(RoofSign.Get()), static_cast<USceneComponent*>(SignTextLeft.Get()),
		static_cast<USceneComponent*>(SignTextRight.Get()), static_cast<USceneComponent*>(TrafficatorLeft.Get()),
		static_cast<USceneComponent*>(TrafficatorRight.Get()) })
	{
		if (Part)
		{
			Part->SetHiddenInGame(true);
		}
	}
}

PersonKit::FOutfit APoliceMotorbike::OfficerOutfit(FRandomStream& Random) const
{
	return MotorbikeModel::PoliceRider(Random);
}

void APoliceMotorbike::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(Rider))
	{
		Rider->Destroy();
	}
	Rider = nullptr;
	Super::EndPlay(EndPlayReason);
}

void APoliceMotorbike::Dress()
{
	using namespace MotorbikeModel;
	bDressed = true;
	Visuals->DrawNothing();
	LastYaw = static_cast<float>(GetActorRotation().Yaw);

	PersonKit::FPiece Shapes[NumParts];
	Build(Shapes, false, 0, true);
	for (int32 p = 0; p < NumParts; ++p)
	{
		if (p == WheelSide)
		{
			continue;
		}
		UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this);
		if (p == WheelFront && Parts.IsValidIndex(Steer) && Parts[Steer])
		{
			Mesh->SetupAttachment(Parts[Steer]);
			Mesh->SetRelativeLocation(PartAt(WheelFront, false) - PartAt(Steer, false));
		}
		else
		{
			Mesh->SetupAttachment(CollisionBox);
			Mesh->SetRelativeLocation(FVector(0.0, 0.0, PoliceMotorbikeRun::FloorZ) + PartAt(p, false));
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
		// Keep the indices in step with MotorbikeModel::EPart.
		while (Parts.Num() < p)
		{
			Parts.Add(nullptr);
		}
		Parts.Add(Mesh);
	}
}

void APoliceMotorbike::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	using namespace MotorbikeModel;
	if (!Manager)
	{
		return;
	}
	if (!bDressed)
	{
		Dress();
	}

	// Thrown off by a crash in a chase: the constable comes on foot.
	if (IsChasing() && IsKnocked() && !bOfficersOut)
	{
		GetOut();
	}

	// The wheels roll, the fork follows the way it turns.
	const float RollSpeed = GetDrivingSpeed();
	WheelTurn = FMath::Fmod(WheelTurn + RollSpeed * DeltaSeconds, static_cast<float>(2.0 * PI * WheelRadius));
	const float Spin = -FMath::RadiansToDegrees(WheelTurn / static_cast<float>(WheelRadius));
	for (const int32 w : { static_cast<int32>(WheelFront), static_cast<int32>(WheelRear) })
	{
		if (Parts.IsValidIndex(w) && Parts[w])
		{
			Parts[w]->SetRelativeRotation(FRotator(Spin, 0.f, 0.f));
		}
	}
	const float NowYaw = static_cast<float>(GetActorRotation().Yaw);
	const float YawRate = DeltaSeconds > 0.f ? FMath::FindDeltaAngleDegrees(LastYaw, NowYaw) / DeltaSeconds : 0.f;
	LastYaw = NowYaw;
	const float WantSteer = FMath::Clamp(YawRate / PoliceMotorbikeRun::FullTurn, -1.f, 1.f) * PoliceMotorbikeRun::MaxSteer;
	SteerAngle = FMath::FInterpTo(SteerAngle, WantSteer, DeltaSeconds, 8.f);
	if (Parts.IsValidIndex(Steer) && Parts[Steer])
	{
		Parts[Steer]->SetRelativeRotation(FQuat(SteerAxis(false), FMath::DegreesToRadians(SteerAngle)));
	}

	PlaceRider(!bOfficersOut);
}

void APoliceMotorbike::PlaceRider(bool bRiding)
{
	const APlayerCameraManager* Viewer = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const bool bNear = Viewer && FVector::Dist(Viewer->GetCameraLocation(), GetActorLocation()) < PoliceMotorbikeRun::PoseRange;
	if (!bRiding || !bNear)
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
		Rider = GetWorld()->SpawnActor<APoliceman>(APoliceman::StaticClass(), GetActorLocation(), GetActorRotation(), Params);
		if (!Rider)
		{
			return;
		}
		FRandomStream Looks(static_cast<int32>(GetUniqueID()));
		Rider->PutOnUniform(MotorbikeModel::PoliceRider(Looks));
		Rider->SetActorEnableCollision(false);
	}
	if (Rider->IsHidden())
	{
		Rider->SetActorHiddenInGame(false);
	}
	// Astride the saddle, feet on the rests, leaning to the bars.
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
	Rider->Strike(Frame.TransformPosition(FVector(0.0, 0.0, PoliceMotorbikeRun::FloorZ) + MotorbikeModel::RiderSeat(false)),
		Frame.Rotator().Yaw, Pose);
}
