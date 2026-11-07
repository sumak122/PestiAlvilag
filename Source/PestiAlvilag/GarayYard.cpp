// Fill out your copyright notice in the Description page of Project Settings.

#include "GarayYard.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "DayNight.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GarayModel.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "StoryPerson.h"
#include "UObject/ConstructorHelpers.h"

namespace GarayYardPlace
{
	// How far up and down to look for the ground under the yard (cm).
	constexpr double GroundLook = 10000.0;
	// Things in the yard are not drawn from further off than this (cm).
	constexpr float DrawDistance = 20000.f;

	// The shop's lamps light its walls this many times as bright as a lit
	// grey wall at night (see ADayNight::LampCandela), 1 m off: by day, so
	// the room isn't dark against the street, and at night, softer.
	constexpr float LampDay = 98.f;
	constexpr float LampNight = 14.f;
	constexpr float LampReach = 900.f;
	const FLinearColor LampColor(1.f, 0.85f, 0.62f);
	// How often the lamps follow the time of day, and the people look up (s).
	constexpr float LampUpdate = 0.25f;

	// After mission 5 ("A felmondás") Hermann is back in his shop, bandaged.
	const FName HermannBackAfter(TEXT("Felmondas"));
	// He knows the player again this close (cm), and shouts again once he has gone this far.
	constexpr double KnowsYouAt = 450.0;
	constexpr double ForgetsAt = 1400.0;
	// He turns to watch the player from this far (cm).
	constexpr double WatchesAt = 900.0;
	constexpr float ShoutSeconds = 6.f;
	// He is back only once the player is this far from the shop (cm).
	constexpr double AppearsBeyond = 4000.0;
}

AGarayYard::AGarayYard()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = GarayYardPlace::LampUpdate;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = BasicShapeMaterial.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	Shapes = { CubeMesh.Object, CylinderMesh.Object, SphereMesh.Object, ConeMesh.Object };
}

FTransform AGarayYard::Frame(const UWorld* World)
{
	if (World)
	{
		for (TActorIterator<AGarayYard> It(World); It; ++It)
		{
			return It->GetActorTransform();
		}
	}
	// Not built (yet): where it would stand, on the ground there.
	double Z = 0.0;
	if (World)
	{
		FHitResult Hit;
		const FVector Top(GarayModel::CityAt, GarayYardPlace::GroundLook);
		const FVector Bottom(GarayModel::CityAt, -GarayYardPlace::GroundLook);
		if (World->LineTraceSingleByChannel(Hit, Top, Bottom, ECC_WorldStatic))
		{
			Z = Hit.ImpactPoint.Z;
		}
	}
	return FTransform(FRotator(0.0, GarayModel::CityYaw, 0.0), FVector(GarayModel::CityAt, Z));
}

void AGarayYard::BeginPlay()
{
	Super::BeginPlay();

	TArray<MarketModel::FShape> Pieces;
	GarayModel::Build(Pieces, true);
	for (const MarketModel::FShape& Piece : Pieces)
	{
		AddShape(Piece.Shape, Piece.Color, Piece.At, Piece.Size, FRotator(Piece.Pitch, Piece.Yaw, Piece.Roll), Piece.bCollide);
	}

	// The painted names over the shop, on the yard wall and on the hut.
	TArray<GarayModel::FSign> Signs;
	GarayModel::Signs(Signs);
	for (const GarayModel::FSign& Sign : Signs)
	{
		UTextRenderComponent* Text = NewObject<UTextRenderComponent>(this, NAME_None, RF_Transient);
		Text->SetupAttachment(RootComponent);
		Text->SetRelativeLocationAndRotation(Sign.At, FRotator(0.0, Sign.Yaw, 0.0));
		Text->SetText(FText::FromString(Sign.Text));
		Text->SetHorizontalAlignment(EHTA_Center);
		Text->SetVerticalAlignment(EVRTA_TextCenter);
		Text->SetWorldSize(Sign.Height);
		Text->SetTextRenderColor(Sign.Color.ToFColor(true));
		Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Text->RegisterComponent();
	}

	for (const FVector& Lamp : GarayModel::Spots::ShopLamps)
	{
		UPointLightComponent* Light = NewObject<UPointLightComponent>(this, NAME_None, RF_Transient);
		Light->SetupAttachment(RootComponent);
		Light->SetRelativeLocation(Lamp - FVector(0.0, 0.0, 15.0));
		Light->SetMobility(EComponentMobility::Movable);
		// Shadows keep the light in the shop (and off the street outside).
		Light->SetCastShadows(true);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetAttenuationRadius(GarayYardPlace::LampReach);
		Light->SetLightColor(GarayYardPlace::LampColor);
		Light->RegisterComponent();
		ShopLights.Add(Light);
	}
	UpdateLamps();
}

void AGarayYard::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateLamps();
	TickShopPeople();
}

void AGarayYard::TickShopPeople()
{
	using namespace GarayYardPlace;
	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (!Game || !Game->IsMissionDone(HermannBackAfter))
	{
		return;
	}
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const APawn* Player = PC ? PC->GetPawn() : nullptr;
	if (!bShopPeopleUp)
	{
		// Not while he is near: the men he fought may still be lying in the shop for a while.
		const FVector ShopAt = GetActorTransform().TransformPosition(GarayModel::Spots::Hermann);
		if (!Player || FVector::Dist2D(Player->GetActorLocation(), ShopAt) < AppearsBeyond)
		{
			return;
		}
		// Old Hermann by his counter, his head bandaged, and Lajos by the window.
		bShopPeopleUp = true;
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FTransform Here = GetActorTransform();
		const double Turn = GetActorRotation().Yaw;
		const FVector HermannAt = Here.TransformPosition(GarayModel::Spots::Hermann) + FVector(0.0, 0.0, PersonModel::MiddleHeight);
		Hermann = GetWorld()->SpawnActor<AStoryPerson>(AStoryPerson::StaticClass(), HermannAt, FRotator(0.0, Turn + 135.0, 0.0), Params);
		if (Hermann)
		{
			PersonKit::FOutfit Dress = GarayModel::Hermann();
			Dress.bBandaged = true;
			Hermann->SetOutfit(Dress);
		}
		const FVector ClerkAt = Here.TransformPosition(GarayModel::Spots::ClerkOne) + FVector(0.0, 0.0, PersonModel::MiddleHeight);
		Clerk = GetWorld()->SpawnActor<AStoryPerson>(AStoryPerson::StaticClass(), ClerkAt, FRotator(0.0, Turn, 0.0), Params);
		if (Clerk)
		{
			Clerk->SetOutfit(GarayModel::Clerk(0));
		}
	}

	if (!Player || !Hermann)
	{
		return;
	}
	const FVector PlayerAt = Player->GetActorLocation();
	const double Gap = FVector::Dist2D(PlayerAt, Hermann->GetActorLocation());
	if (Gap < WatchesAt)
	{
		// Both keep an eye on him.
		Hermann->FaceTowards(PlayerAt);
		if (Clerk)
		{
			Clerk->FaceTowards(PlayerAt);
		}
	}
	if (Gap > ForgetsAt)
	{
		bShoutArmed = true;
	}
	if (!bShoutArmed || Gap > KnowsYouAt)
	{
		return;
	}
	bShoutArmed = false;
	APestiHUD* Hud = APestiHUD::Get(this);
	if (!Hud)
	{
		return;
	}
	if (Shouts == 0)
	{
		Hud->Say(TEXT("Friedrich Hermann"),
			TEXT("Maga az! Maga volt az! Lajos, hallja? Ez \u00fct\u00f6tt le a saj\u00e1t boltomban!"),
			TEXT("It's you! It was you! Lajos, do you hear? This is the one who knocked me down in my own shop!"), ShoutSeconds);
	}
	else if (Shouts % 2 == 1)
	{
		Hud->Say(TEXT("Lajos, a seg\u00e9d"),
			TEXT("Menjen innen, uram, am\u00edg sz\u00e9pen mondom. A f\u0151n\u00f6k \u00far feljelent\u00e9st tett."),
			TEXT("Leave, sir, while I'm asking nicely. The governor has gone to the police."), ShoutSeconds);
	}
	else
	{
		Hud->Say(TEXT("Friedrich Hermann"),
			TEXT("M\u00e1r megint itt van? Kifel\u00e9, miel\u0151tt a fiam haza\u00e9r!"),
			TEXT("You again? Out, before my son gets home!"), ShoutSeconds);
	}
	++Shouts;
}

void AGarayYard::UpdateLamps()
{
	const ADayNight* Clock = ADayNight::Get(GetWorld());
	if (!Clock)
	{
		return;
	}
	// Between day and night on a log scale, as the eye's exposure goes.
	const float Strength = FMath::Exp(FMath::Lerp(FMath::Loge(GarayYardPlace::LampDay), FMath::Loge(GarayYardPlace::LampNight), Clock->GetDarkness()));
	const float Candela = Clock->LampCandela(Strength);
	for (UPointLightComponent* Light : ShopLights)
	{
		if (Light)
		{
			Light->SetIntensity(Candela);
		}
	}
}

void AGarayYard::AddShape(int32 Shape, const FLinearColor& Color, const FVector& At, const FVector& Size, const FRotator& Turn, bool bCollide)
{
	if (!Shapes.IsValidIndex(Shape) || !Shapes[Shape])
	{
		return;
	}
	// One batch per shape, colour and collision; the instances are placed in the yard's frame.
	const FString Key = FString::Printf(TEXT("%d %s %d"), Shape, *Color.ToString(), bCollide ? 1 : 0);
	UInstancedStaticMeshComponent* Instances = nullptr;
	if (const int32* Found = BatchIndex.Find(Key))
	{
		Instances = Batches[*Found];
	}
	else
	{
		Instances = NewObject<UInstancedStaticMeshComponent>(this, NAME_None, RF_Transient);
		Instances->SetupAttachment(RootComponent);
		Instances->SetStaticMesh(Shapes[Shape]);
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Paint->SetVectorParameterValue(TEXT("Color"), Color);
			Instances->SetMaterial(0, Paint);
		}
		Instances->SetCollisionProfileName(bCollide ? TEXT("BlockAll") : TEXT("NoCollision"));
		Instances->SetCullDistances(0, GarayYardPlace::DrawDistance);
		Instances->SetCanEverAffectNavigation(false);
		Instances->RegisterComponent();
		BatchIndex.Add(Key, Batches.Add(Instances));
	}
	Instances->AddInstance(FTransform(Turn, At, Size / 100.0), false);
}
