// Fill out your copyright notice in the Description page of Project Settings.

#include "BandPlayer.h"

#include "Components/CapsuleComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PersonModel.h"
#include "ProceduralMeshComponent.h"

namespace BandPlayer
{
	/** Beats per second of the music (BandSynth: slow and fast parts, here somewhere between). */
	constexpr float BeatsPerSecond = 2.4f;
	/** The clarinet leans this far forward in the hands (degrees). */
	constexpr float ClarinetLean = -35.f;
	/** The bass stands at his left front, tipped back to him (body frame, cm and degrees). */
	const FVector BassAt(32.0, -14.0, -PersonModel::MiddleHeight);
	const FRotator BassTurn(-12.0, -15.0, 0.0);
	const FRotator BassBowTurn(0.0, 0.0, 80.0);
}

ABandPlayer::ABandPlayer()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ABandPlayer::BeginPlay()
{
	Super::BeginPlay();
	using namespace MusicBarModel;

	Beat = FMath::Fmod(static_cast<float>(GetActorLocation().X * 0.013 + GetActorLocation().Y * 0.007), 4.f);
	const double Tall = Outfit.Height;
	USceneComponent* Body = Pieces.IsValidIndex(PersonKit::Body) ? Pieces[PersonKit::Body].Get() : nullptr;
	USceneComponent* LeftArm = Pieces.IsValidIndex(PersonKit::ForearmLeft) ? Pieces[PersonKit::ForearmLeft].Get() : nullptr;
	USceneComponent* RightArm = Pieces.IsValidIndex(PersonKit::ForearmRight) ? Pieces[PersonKit::ForearmRight].Get() : nullptr;
	PersonKit::FPiece Shape;
	switch (Part)
	{
	case EPlayer::Cimbalom:
	{
		Cimbalom(Shape);
		// It stands on the floor in front of him and doesn't lean when he does.
		UProceduralMeshComponent* Box = AddProp(Shape, Capsule, FVector::ZeroVector, FRotator::ZeroRotator);
		if (Box)
		{
			Box->SetUsingAbsoluteLocation(true);
			Box->SetUsingAbsoluteRotation(true);
			const FRotator Facing(0.0, GetActorRotation().Yaw, 0.0);
			Box->SetWorldLocationAndRotation(GetActorLocation() + Facing.RotateVector(FVector(CimbalomReach, 0.0, -PersonModel::MiddleHeight)), Facing);
		}
		PersonKit::FPiece Stick;
		Hammer(Stick);
		LeftHand = AddProp(Stick, LeftArm, HandAt(-1.0, Tall), FRotator::ZeroRotator);
		RightHand = AddProp(Stick, RightArm, HandAt(1.0, Tall), FRotator::ZeroRotator);
		break;
	}
	case EPlayer::Fiddle:
	case EPlayer::Second:
	{
		Fiddle(Shape);
		AddProp(Shape, Body, FiddleAt(Tall), FiddleTurn());
		PersonKit::FPiece Stick;
		FiddleBow(Stick);
		RightHand = AddProp(Stick, RightArm, HandAt(1.0, Tall), FRotator::ZeroRotator);
		break;
	}
	case EPlayer::Bass:
	{
		MusicBarModel::Bass(Shape);
		AddProp(Shape, Capsule, BandPlayer::BassAt, BandPlayer::BassTurn);
		PersonKit::FPiece Stick;
		BassBow(Stick);
		RightHand = AddProp(Stick, RightArm, HandAt(1.0, Tall), BandPlayer::BassBowTurn);
		break;
	}
	case EPlayer::Clarinet:
	{
		MusicBarModel::Clarinet(Shape);
		RightHand = AddProp(Shape, RightArm, HandAt(1.0, Tall), FRotator::ZeroRotator);
		break;
	}
	default:
		break;
	}
}

UProceduralMeshComponent* ABandPlayer::AddProp(const PersonKit::FPiece& Shape, USceneComponent* Parent, const FVector& Location, const FRotator& Rotation)
{
	if (!Parent)
	{
		return nullptr;
	}
	UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this);
	Mesh->SetupAttachment(Parent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);
	// It moves with him all the time: kept out of the ray-traced lighting and the distance fields.
	Mesh->bVisibleInRayTracing = false;
	Mesh->bAffectDistanceFieldLighting = false;
	Mesh->SetCastShadow(false);
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
	Mesh->SetRelativeLocationAndRotation(Location, Rotation);
	Mesh->RegisterComponent();
	AddInstanceComponent(Mesh);
	Props.Add(Mesh);
	return Mesh;
}

void ABandPlayer::Tick(float DeltaSeconds)
{
	// Standing on his spot, not walking like a story person.
	AActor::Tick(DeltaSeconds);
	using namespace MusicBarModel;

	if (bPlaying)
	{
		Beat += DeltaSeconds * BandPlayer::BeatsPerSecond;
		if (Beat > 1000.f)
		{
			Beat -= 1000.f;
		}
	}
	FPlay Play;
	switch (Part)
	{
	case EPlayer::Cimbalom: Hammering(Play, Beat); break;
	case EPlayer::Fiddle: Fiddling(Play, Beat * 0.5f); break;
	case EPlayer::Second: Fiddling(Play, Beat * 0.5f + 0.25f); break;
	case EPlayer::Bass: BassBowing(Play, Beat * 0.5f); break;
	case EPlayer::Clarinet: Piping(Play, Beat * 0.25f); break;
	case EPlayer::Singer: Singing(Play, Beat * 0.125f); break;
	default: break;
	}
	Pose(Play);

	// What is in his hands turns with his forearms.
	const float RaisedLeft = static_cast<float>(Play.Arms[0].Pitch) + Play.Elbows[0];
	const float RaisedRight = static_cast<float>(Play.Arms[1].Pitch) + Play.Elbows[1];
	if (Part == EPlayer::Cimbalom)
	{
		if (LeftHand)
		{
			LeftHand->SetRelativeRotation(HammerTurn(RaisedLeft));
		}
		if (RightHand)
		{
			RightHand->SetRelativeRotation(HammerTurn(RaisedRight));
		}
	}
	else if ((Part == EPlayer::Fiddle || Part == EPlayer::Second) && RightHand)
	{
		RightHand->SetRelativeRotation(BowTurn(RaisedRight));
	}
	else if (Part == EPlayer::Clarinet && RightHand)
	{
		RightHand->SetRelativeRotation(HoldUpright(RaisedRight, BandPlayer::ClarinetLean));
	}
}

void ABandPlayer::Pose(const MusicBarModel::FPlay& Play)
{
	for (int32 i = 0; i < 2; ++i)
	{
		if (USceneComponent* Arm = Limb(PersonKit::ArmLeft + i))
		{
			Arm->SetRelativeRotation(Play.Arms[i]);
		}
	}
	PersonModel::PoseElbows(Pieces, Play.Elbows[0], Play.Elbows[1]);
	PersonModel::PoseLegs(Pieces, Play.Legs[0], Play.Legs[1], Play.Knees[0], Play.Knees[1]);
	// Bent over his instrument: the whole of him tips forward about his middle.
	const FRotator Now = GetActorRotation();
	if (!FMath::IsNearlyEqual(Now.Pitch, static_cast<double>(-Play.Pitch), 0.05))
	{
		SetActorRotation(FRotator(-Play.Pitch, Now.Yaw, 0.0));
	}
}
