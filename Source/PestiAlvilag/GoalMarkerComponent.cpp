// Fill out your copyright notice in the Description page of Project Settings.

#include "GoalMarkerComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace GoalMarker
{
	// Gold and a darker brass, as on the screen panels.
	const FLinearColor Gold(0.90f, 0.58f, 0.16f);
	const FLinearColor Brass(0.55f, 0.34f, 0.09f);

	// The gem hangs this high over the ground and is built of five square
	// layers turned corner-on, widest in the middle (cm).
	constexpr double GemHeight = 250.0;
	constexpr double LayerHeight = 5.0;
	const double LayerWidths[] = { 7.0, 17.0, 27.0, 17.0, 7.0 };
	constexpr float GemTurn = 12.f;    // degrees a second
	constexpr float Bob = 4.f;         // cm up and down
	constexpr float BobSpeed = 1.6f;

	// The fan over the gem: nine rays in a half circle, long and short in turn.
	constexpr double FanOver = 12.0;
	constexpr int32 Rays = 9;
	constexpr double RayFrom = 13.0;
	constexpr double RayLong = 28.0;
	constexpr double RayShort = 22.0;

	// The sunburst on the ground: sixteen rays, every fourth a long one.
	constexpr int32 GroundRays = 16;

	// How far over and under Target the ground is looked for.
	constexpr double LookAbove = 200.0;
	constexpr double LookBelow = 1000.0;
}

UGoalMarkerComponent::UGoalMarkerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetUsingAbsoluteLocation(true);
	SetUsingAbsoluteRotation(true);
	SetUsingAbsoluteScale(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	CubeMesh = Cube.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Translucent(TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
	SeeThroughMaterial = Translucent.Object;
	static ConstructorHelpers::FObjectFinder<UTexture> White(TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	WhiteTexture = White.Object;
}

UMaterialInstanceDynamic* UGoalMarkerComponent::Glow(const FLinearColor& Color, float Opacity)
{
	if (!SeeThroughMaterial)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(SeeThroughMaterial, this);
	if (WhiteTexture)
	{
		Paint->SetTextureParameterValue(TEXT("SlateUI"), WhiteTexture);
	}
	FLinearColor Tint = Color;
	Tint.A = Opacity;
	Paint->SetVectorParameterValue(TEXT("TintColorAndOpacity"), Tint);
	return Paint;
}

void UGoalMarkerComponent::AddBar(USceneComponent* Parent, UMaterialInstanceDynamic* Paint, const FVector& At, const FVector& Size, const FRotator& Turn)
{
	AActor* Owner = GetOwner();
	if (!Owner || !CubeMesh)
	{
		return;
	}
	UStaticMeshComponent* Bar = NewObject<UStaticMeshComponent>(Owner, NAME_None, RF_Transient);
	Bar->SetupAttachment(Parent);
	Bar->SetStaticMesh(CubeMesh);
	Bar->SetRelativeLocationAndRotation(At, Turn);
	Bar->SetRelativeScale3D(Size / 100.0);
	Bar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bar->SetCanEverAffectNavigation(false);
	Bar->SetCastShadow(false);
	if (Paint)
	{
		Bar->SetMaterial(0, Paint);
	}
	Bar->RegisterComponent();
	Bars.Add(Bar);
}

void UGoalMarkerComponent::Build()
{
	using namespace GoalMarker;
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	UMaterialInstanceDynamic* GemGold = Glow(Gold, 0.95f);
	UMaterialInstanceDynamic* GemBrass = Glow(Brass, 0.95f);
	UMaterialInstanceDynamic* RayPaint = Glow(Gold, 0.55f);
	UMaterialInstanceDynamic* ThreadPaint = Glow(Gold, 0.22f);
	UMaterialInstanceDynamic* LongPaint = Glow(Gold, 0.5f);
	UMaterialInstanceDynamic* ShortPaint = Glow(Gold, 0.35f);

	// The stepped gem, its layers in gold and brass by turns.
	Gem = NewObject<USceneComponent>(Owner, NAME_None, RF_Transient);
	Gem->SetupAttachment(this);
	Gem->SetRelativeLocation(FVector(0.0, 0.0, GemHeight));
	Gem->RegisterComponent();
	const int32 Layers = static_cast<int32>(UE_ARRAY_COUNT(LayerWidths));
	for (int32 i = 0; i < Layers; ++i)
	{
		const double Z = (i - (Layers - 1) * 0.5) * LayerHeight;
		AddBar(Gem, i % 2 ? GemBrass : GemGold, FVector(0.0, 0.0, Z), FVector(LayerWidths[i], LayerWidths[i], LayerHeight), FRotator(0.0, 45.0, 0.0));
	}

	// The fan of rays over it, in a plane that turns to the camera.
	Fan = NewObject<USceneComponent>(Owner, NAME_None, RF_Transient);
	Fan->SetupAttachment(this);
	Fan->SetRelativeLocation(FVector(0.0, 0.0, GemHeight + FanOver));
	Fan->RegisterComponent();
	for (int32 i = 0; i < Rays; ++i)
	{
		const double Degrees = 20.0 + 140.0 * i / (Rays - 1);
		const double A = FMath::DegreesToRadians(Degrees);
		const double To = i % 2 ? RayShort : RayLong;
		const double Middle = (RayFrom + To) * 0.5;
		AddBar(Fan, RayPaint, FVector(0.0, FMath::Cos(A) * Middle, FMath::Sin(A) * Middle), FVector(0.3, 1.2, To - RayFrom), FRotator(0.0, 0.0, 90.0 - Degrees));
	}

	// The thread down to the ground.
	AddBar(this, ThreadPaint, FVector(0.0, 0.0, (GemHeight - 14.0) * 0.5), FVector(1.2, 1.2, GemHeight - 14.0), FRotator::ZeroRotator);

	// The sunburst on the ground.
	for (int32 i = 0; i < GroundRays; ++i)
	{
		const double Degrees = i * 360.0 / GroundRays;
		const bool bLong = i % 4 == 0;
		const double From = bLong ? 40.0 : 55.0;
		const double To = bLong ? 95.0 : 78.0;
		const double Middle = (From + To) * 0.5;
		const double A = FMath::DegreesToRadians(Degrees);
		AddBar(this, bLong ? LongPaint : ShortPaint, FVector(FMath::Cos(A) * Middle, FMath::Sin(A) * Middle, 1.0),
			FVector(To - From, bLong ? 3.0 : 2.0, 0.4), FRotator(0.0, Degrees, 0.0));
	}
}

void UGoalMarkerComponent::ShowAt(const FVector& Target)
{
	if (Bars.Num() == 0)
	{
		Build();
	}
	// On the ground under the target (a person's middle, say), or at the target.
	FVector Ground = Target;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GoalMarkerGround), false, GetOwner());
	if (GetWorld() && GetWorld()->LineTraceSingleByObjectType(Hit, Target + FVector(0.0, 0.0, GoalMarker::LookAbove),
		Target - FVector(0.0, 0.0, GoalMarker::LookBelow), FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		Ground = Hit.ImpactPoint;
	}
	SetWorldLocationAndRotation(Ground, FRotator::ZeroRotator);
	SetVisibility(true, true);
	SetComponentTickEnabled(true);
}

void UGoalMarkerComponent::Hide()
{
	SetVisibility(false, true);
	SetComponentTickEnabled(false);
}

void UGoalMarkerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	using namespace GoalMarker;
	Time += DeltaTime;
	const double Rise = Bob * FMath::Sin(Time * BobSpeed);
	if (Gem)
	{
		Gem->SetRelativeLocationAndRotation(FVector(0.0, 0.0, GemHeight + Rise), FRotator(0.0, Time * GemTurn, 0.0));
	}
	if (Fan)
	{
		const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		double Yaw = 0.0;
		if (PC && PC->PlayerCameraManager)
		{
			Yaw = (PC->PlayerCameraManager->GetCameraLocation() - Fan->GetComponentLocation()).Rotation().Yaw;
		}
		Fan->SetRelativeLocationAndRotation(FVector(0.0, 0.0, GemHeight + FanOver + Rise), FRotator(0.0, Yaw, 0.0));
	}
}
