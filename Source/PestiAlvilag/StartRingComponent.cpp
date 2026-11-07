// Fill out your copyright notice in the Description page of Project Settings.

#include "StartRingComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "PestiHud.h"
#include "TextureResource.h"
#include "UObject/ConstructorHelpers.h"

namespace StartRing
{
	// Gold and a darker brass, as on the screen panels, and the ink of the plaques.
	const FLinearColor Gold(0.90f, 0.58f, 0.16f);
	const FLinearColor Light(1.00f, 0.74f, 0.34f);
	const FLinearColor Ink(0.03f, 0.025f, 0.02f);

	// The ring on the ground (cm): the outer band, two thin ones inside it,
	// and eight short ticks round the outside.
	constexpr double Outer = 75.0;
	constexpr int32 Segments = 32;
	constexpr double OuterBand = 6.0;
	constexpr double InnerA = 61.0;
	constexpr double InnerB = 51.0;
	constexpr int32 Ticks = 8;
	constexpr double TickOut = 9.0;
	constexpr double TickLength = 14.0;

	// The column: soft glowing sheets, each one fainter than the one under
	// it, overlapping so the light fades smoothly upwards.
	constexpr int32 Sheets = 5;
	constexpr double SheetHeight = 70.0;
	constexpr double SheetStep = 26.0;
	constexpr double SheetWidth = 160.0;
	const float SheetOpacity[Sheets] = { 0.42f, 0.30f, 0.20f, 0.12f, 0.06f };
	constexpr float Breathe = 0.12f;   // share the light swells and fades
	constexpr float BreatheSpeed = 2.0f;

	// The letter's diamond over the column, bobbing a little.
	constexpr double BadgeHeight = 215.0;
	constexpr double BadgeSize = 34.0;
	constexpr double FrameSize = 40.0;
	constexpr float LetterSize = 30.f;
	constexpr float Bob = 4.f;
	constexpr float BobSpeed = 1.6f;

	// Inside the ring (cm from its middle).
	constexpr float Inside = 110.f;

	// How far over and under the spot the ground is looked for.
	constexpr double LookAbove = 200.0;
	constexpr double LookBelow = 1000.0;

	constexpr int32 TextureSize = 64;
}

UStartRingComponent::UStartRingComponent()
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

float UStartRingComponent::Radius()
{
	return StartRing::Inside;
}

UTexture2D* UStartRingComponent::MakeSoftTexture()
{
	using namespace StartRing;
	UTexture2D* Texture = UTexture2D::CreateTransient(TextureSize, TextureSize, PF_B8G8R8A8);
	if (!Texture)
	{
		return nullptr;
	}
	Texture->SRGB = true;
	Texture->LODGroup = TEXTUREGROUP_UI;
	Texture->Filter = TF_Bilinear;
	Texture->AddressX = TA_Clamp;
	Texture->AddressY = TA_Clamp;
	Texture->NeverStream = true;

	// White, its alpha fading to nothing at every edge (the same both ways,
	// so it looks right however the box's faces lay out the picture).
	TArray<FColor> Pixels;
	Pixels.SetNumUninitialized(TextureSize * TextureSize);
	for (int32 Y = 0; Y < TextureSize; ++Y)
	{
		for (int32 X = 0; X < TextureSize; ++X)
		{
			const float U = (X + 0.5f) / TextureSize * 2.f - 1.f;
			const float V = (Y + 0.5f) / TextureSize * 2.f - 1.f;
			const float Across = FMath::Clamp(1.f - U * U, 0.f, 1.f);
			const float Up = FMath::Clamp(1.f - V * V, 0.f, 1.f);
			Pixels[Y * TextureSize + X] = FColor(255, 255, 255, static_cast<uint8>(FMath::RoundToInt(255.f * Across * Across * Up)));
		}
	}
	FTexturePlatformData* Data = Texture->GetPlatformData();
	if (!Data || Data->Mips.Num() == 0)
	{
		return nullptr;
	}
	FTexture2DMipMap& Mip = Data->Mips[0];
	const int64 Bytes = static_cast<int64>(Pixels.Num()) * sizeof(FColor);
	Mip.BulkData.Lock(LOCK_READ_WRITE);
	void* Dest = Mip.BulkData.Realloc(Bytes);
	FMemory::Memcpy(Dest, Pixels.GetData(), Bytes);
	Mip.BulkData.Unlock();
	Texture->UpdateResource();
	return Texture;
}

UMaterialInstanceDynamic* UStartRingComponent::Glow(const FLinearColor& Color, float Opacity, UTexture* Picture)
{
	if (!SeeThroughMaterial)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(SeeThroughMaterial, this);
	UTexture* Shown = Picture ? Picture : WhiteTexture.Get();
	if (Shown)
	{
		Paint->SetTextureParameterValue(TEXT("SlateUI"), Shown);
	}
	FLinearColor Tint = Color;
	Tint.A = Opacity;
	Paint->SetVectorParameterValue(TEXT("TintColorAndOpacity"), Tint);
	return Paint;
}

void UStartRingComponent::AddBar(USceneComponent* Parent, UMaterialInstanceDynamic* Paint, const FVector& At, const FVector& Size, const FRotator& Turn)
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

void UStartRingComponent::Build()
{
	using namespace StartRing;
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	UMaterialInstanceDynamic* BandPaint = Glow(Gold, 0.85f);
	UMaterialInstanceDynamic* ThinPaint = Glow(Gold, 0.6f);
	UMaterialInstanceDynamic* FaintPaint = Glow(Gold, 0.4f);
	UMaterialInstanceDynamic* TickPaint = Glow(Gold, 0.75f);

	// The ring on the ground: a band of short pieces round the circle, two thin ones inside.
	const double Piece = 2.0 * UE_DOUBLE_PI / Segments;
	for (int32 i = 0; i < Segments; ++i)
	{
		const double A = (i + 0.5) * Piece;
		const double Degrees = FMath::RadiansToDegrees(A) + 90.0;
		const FVector Way(FMath::Cos(A), FMath::Sin(A), 0.0);
		const double Middle = Outer - OuterBand * 0.5;
		AddBar(this, BandPaint, Way * Middle + FVector(0.0, 0.0, 1.2), FVector(Piece * Outer + 0.6, OuterBand, 0.4), FRotator(0.0, Degrees, 0.0));
		AddBar(this, ThinPaint, Way * InnerA + FVector(0.0, 0.0, 1.2), FVector(Piece * InnerA + 0.4, 2.0, 0.4), FRotator(0.0, Degrees, 0.0));
		AddBar(this, FaintPaint, Way * InnerB + FVector(0.0, 0.0, 1.2), FVector(Piece * InnerB + 0.4, 1.5, 0.4), FRotator(0.0, Degrees, 0.0));
	}
	for (int32 i = 0; i < Ticks; ++i)
	{
		const double Degrees = i * 360.0 / Ticks;
		const double A = FMath::DegreesToRadians(Degrees);
		const double Middle = Outer + TickOut;
		AddBar(this, TickPaint, FVector(FMath::Cos(A) * Middle, FMath::Sin(A) * Middle, 1.3), FVector(TickLength, 5.0, 0.4), FRotator(0.0, Degrees, 0.0));
	}

	// The column: soft sheets of light, turned to the camera.
	if (!SoftTexture)
	{
		SoftTexture = MakeSoftTexture();
	}
	Column = NewObject<USceneComponent>(Owner, NAME_None, RF_Transient);
	Column->SetupAttachment(this);
	Column->RegisterComponent();
	for (int32 i = 0; i < Sheets; ++i)
	{
		UMaterialInstanceDynamic* SheetPaint = Glow(Light, SheetOpacity[i], SoftTexture ? SoftTexture.Get() : nullptr);
		AddBar(Column, SheetPaint, FVector(0.0, 0.0, SheetHeight * 0.5 + i * SheetStep), FVector(0.4, SheetWidth, SheetHeight), FRotator::ZeroRotator);
	}

	// The letter on a dark diamond in a gold frame.
	Badge = NewObject<USceneComponent>(Owner, NAME_None, RF_Transient);
	Badge->SetupAttachment(this);
	Badge->SetRelativeLocation(FVector(0.0, 0.0, BadgeHeight));
	Badge->RegisterComponent();
	AddBar(Badge, Glow(Gold, 0.95f), FVector(-1.0, 0.0, 0.0), FVector(0.4, FrameSize, FrameSize), FRotator(0.0, 0.0, 45.0));
	AddBar(Badge, Glow(Ink, 0.92f), FVector(0.0, 0.0, 0.0), FVector(0.6, BadgeSize, BadgeSize), FRotator(0.0, 0.0, 45.0));
	LetterText = NewObject<UTextRenderComponent>(Owner, NAME_None, RF_Transient);
	LetterText->SetupAttachment(Badge);
	LetterText->SetRelativeLocation(FVector(1.5, 0.0, 0.0));
	LetterText->SetHorizontalAlignment(EHTA_Center);
	LetterText->SetVerticalAlignment(EVRTA_TextCenter);
	LetterText->SetWorldSize(LetterSize);
	LetterText->SetTextRenderColor(FColor(242, 196, 106));
	LetterText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LetterText->SetCastShadow(false);
	LetterText->RegisterComponent();
}

void UStartRingComponent::ShowAt(const FVector& At, const FString& Letter)
{
	if (Bars.Num() == 0)
	{
		Build();
	}
	FVector Ground = At;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(StartRingGround), false, GetOwner());
	if (GetWorld() && GetWorld()->LineTraceSingleByObjectType(Hit, At + FVector(0.0, 0.0, StartRing::LookAbove),
		At - FVector(0.0, 0.0, StartRing::LookBelow), FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		Ground = Hit.ImpactPoint;
	}
	SetWorldLocationAndRotation(Ground, FRotator::ZeroRotator);
	ShownLetter = Letter;
	if (LetterText)
	{
		LetterText->SetText(FText::FromString(Letter));
	}
	SetVisibility(true, true);
	SetComponentTickEnabled(true);
	bShown = true;
}

void UStartRingComponent::Hide()
{
	SetVisibility(false, true);
	SetComponentTickEnabled(false);
	bShown = false;
}

void UStartRingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	using namespace StartRing;
	Time += DeltaTime;
	if (APestiHUD* Hud = bOnMap ? APestiHUD::Get(this) : nullptr)
	{
		Hud->MarkStart(GetComponentLocation(), ShownLetter);
	}

	// The light and the letter turn to the camera; the light breathes, the letter bobs.
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	double Yaw = 0.0;
	if (PC && PC->PlayerCameraManager)
	{
		Yaw = (PC->PlayerCameraManager->GetCameraLocation() - GetComponentLocation()).Rotation().Yaw;
	}
	if (Column)
	{
		const double Swell = 1.0 + Breathe * FMath::Sin(Time * BreatheSpeed);
		Column->SetRelativeRotation(FRotator(0.0, Yaw, 0.0));
		Column->SetRelativeScale3D(FVector(1.0, 1.0, Swell));
	}
	if (Badge)
	{
		Badge->SetRelativeLocationAndRotation(FVector(0.0, 0.0, BadgeHeight + Bob * FMath::Sin(Time * BobSpeed)), FRotator(0.0, Yaw, 0.0));
	}
}
