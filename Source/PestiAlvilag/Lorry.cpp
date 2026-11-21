// Fill out your copyright notice in the Description page of Project Settings.

#include "Lorry.h"

#include "CarBody.h"
#include "CarModel.h"
#include "CarVisuals.h"
#include "CarjackedDriver.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "DailyLifeModel.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Pedestrians.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PolicePatrols.h"
#include "ProceduralMeshComponent.h"
#include "StreetName.h"
#include "Witnesses.h"

namespace LorryRun
{
	// Its own top speed, pull and turning (cm/s, cm/s each second, degrees/s), before the van's shares (CarHandling).
	constexpr float TopSpeed = 2000.f;
	constexpr float Pull = 560.f;
	constexpr float TurnRate = 70.f;
	// The front wheels turn this far at their sharpest (degrees).
	constexpr float MaxSteer = 30.f;
	// The carrier comes out of a doorway this far behind and to the kerb side (cm); the police see it from this far.
	constexpr double CarrierBack = 700.0;
	constexpr double CarrierKerb = 300.0;
	constexpr double TheftSeen = 2500.0;
	// The mail sack in a post van holds this much (pengő, as fillér).
	constexpr int32 SackLeast = 300;
	constexpr int32 SackMost = 800;
}

ALorry::ALorry()
{
	MaxSpeed = LorryRun::TopSpeed;
	Acceleration = LorryRun::Pull;
	MaxTurnRate = LorryRun::TurnRate;
	// The heaviest kind of car there is: it handles like a loaded van.
	CarModel::FStyle Look;
	Look.Body = CarModel::EBody::Van;
	Look.PaintColor = LorryModel::Paint(0);
	SetStyle(Look);
	BaseMaterial = CarBody::LoadPaintMaterial();
}

void ALorry::SetLook(int32 InPaint, LorryModel::ELoad InLoad)
{
	PaintIndex = InPaint;
	Load = InLoad;
}

void ALorry::BeginPlay()
{
	Super::BeginPlay();
	using namespace LorryModel;
	LastYaw = static_cast<float>(GetActorRotation().Yaw);

	// The car's own looks go; the box takes the lorry's length and width.
	Visuals->DrawNothing();
	const FVector Half = LorryHalf();
	const double BoxZ = CollisionBox->GetUnscaledBoxExtent().Z;
	CollisionBox->SetBoxExtent(FVector(Half.X, Half.Y, BoxZ));
	FloorZ = -(BoxZ + HoverHeight);

	// The body with its load, and the wheels, each about its own hub.
	PersonKit::FPiece Shapes[NumParts];
	Build(Shapes, PaintIndex, Load);
	for (int32 p = 0; p < NumParts; ++p)
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
				UMaterialInstanceDynamic* Coat = UMaterialInstanceDynamic::Create(BaseMaterial, this);
				Coat->SetVectorParameterValue(TEXT("Color"), Section.Color);
				Mesh->SetMaterial(s, Coat);
			}
		}
		const FVector Pivot = p == Body ? FVector::ZeroVector : WheelHub(p);
		Mesh->SetRelativeLocation(FVector(0.0, 0.0, FloorZ) + Pivot);
		Mesh->RegisterComponent();
		AddInstanceComponent(Mesh);
		Parts.Add(Mesh);
	}
	if (Load == ELoad::Post)
	{
		AddPostLettering(this, CollisionBox, FloorZ);
	}
}

void ALorry::AddPostLettering(AActor* Van, USceneComponent* Body, double FloorAt)
{
	if (!Van || !Body)
	{
		return;
	}
	for (const double Side : { -1.0, 1.0 })
	{
		UTextRenderComponent* Text = NewObject<UTextRenderComponent>(Van, NAME_None, RF_Transient);
		Text->SetupAttachment(Body);
		Text->SetRelativeLocationAndRotation(FVector(0.0, 0.0, FloorAt) + LorryModel::PostLetteringAt(Side), FRotator(0.0, Side * 90.0, 0.0));
		Text->SetText(FText::FromString(TEXT("M. KIR. POSTA")));
		Text->SetHorizontalAlignment(EHTA_Center);
		Text->SetVerticalAlignment(EVRTA_TextCenter);
		Text->SetWorldSize(LorryModel::PostLetterHeight);
		Text->SetTextRenderColor(LorryModel::PostYellow().ToFColor(true));
		Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Text->RegisterComponent();
		Van->AddInstanceComponent(Text);
	}
}

void ALorry::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(WheelMan))
	{
		WheelMan->Destroy();
	}
	WheelMan = nullptr;
	Super::EndPlay(EndPlayReason);
}

void ALorry::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	using namespace LorryModel;

	APlayerController* PC = Cast<APlayerController>(GetController());
	const bool bDriven = PC != nullptr;
	if (bDriven && !bTaken)
	{
		bTaken = true;
		if (Load == LorryModel::ELoad::Post)
		{
			PostTaken();
		}
		else if (bOwned)
		{
			bOwned = false;
			Stolen();
		}
	}

	// The wheels roll as far as it goes; the front ones turn the way it turns.
	WheelTurn = FMath::Fmod(WheelTurn + ForwardSpeed * DeltaSeconds, static_cast<float>(2.0 * PI * WheelRadius));
	const float Spin = -FMath::RadiansToDegrees(WheelTurn / static_cast<float>(WheelRadius));
	const float NowYaw = static_cast<float>(GetActorRotation().Yaw);
	const float YawRate = DeltaSeconds > 0.f ? FMath::FindDeltaAngleDegrees(LastYaw, NowYaw) / DeltaSeconds : 0.f;
	LastYaw = NowYaw;
	const float Moving = FMath::Clamp(FMath::Abs(ForwardSpeed) / 300.f, 0.2f, 1.f);
	const float WantSteer = FMath::Clamp(YawRate / (LorryRun::TurnRate * Moving), -1.f, 1.f) * LorryRun::MaxSteer
		* (ForwardSpeed < 0.f ? -1.f : 1.f);
	SteerAngle = FMath::FInterpTo(SteerAngle, WantSteer, DeltaSeconds, 8.f);
	for (int32 w = WheelFrontLeft; w <= WheelBackRight; ++w)
	{
		if (Parts.IsValidIndex(w) && Parts[w])
		{
			const bool bFront = w == WheelFrontLeft || w == WheelFrontRight;
			Parts[w]->SetRelativeRotation(FRotator(Spin, bFront ? SteerAngle : 0.f, 0.f));
		}
	}

	PlaceWheelMan(bDriven);
}

void ALorry::PlaceWheelMan(bool bDriven)
{
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
		WheelMan->SetActorEnableCollision(false);
	}
	if (WheelMan->IsHidden())
	{
		WheelMan->SetActorHiddenInGame(false);
	}
	// Seated on the bench, the hands on the wheel.
	DailyLifeModel::FLimbs Pose;
	DailyLifeModel::Seated(Pose, 0.f);
	Pose.Arms[0] = FRotator(50.f, 0.f, 8.f);
	Pose.Arms[1] = FRotator(50.f, 0.f, -8.f);
	Pose.Elbows[0] = 35.f;
	Pose.Elbows[1] = 35.f;
	const FTransform Frame = GetActorTransform();
	WheelMan->Strike(Frame.TransformPosition(FVector(0.0, 0.0, FloorZ) + LorryModel::DriverSeat()), Frame.Rotator().Yaw, Pose);
}

void ALorry::Stolen()
{
	const FVector Location = GetActorLocation();
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector Danger = Player ? Player->GetActorLocation() : Location;

	// The carrier comes running out of a doorway behind, on the kerb side, shouting after it.
	const FVector Back = -GetActorForwardVector().GetSafeNormal2D();
	const FVector Kerb = -GetActorRightVector().GetSafeNormal2D();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector CarrierAt = Location + Back * LorryRun::CarrierBack + Kerb * LorryRun::CarrierKerb;
	if (ACarjackedDriver* Robbed = GetWorld()->SpawnActor<ACarjackedDriver>(ACarjackedDriver::StaticClass(), CarrierAt, GetActorRotation(), Params))
	{
		FRandomStream Dice(FMath::Rand());
		PersonKit::FOutfit Clothes = PersonModel::RandomOutfit(Dice);
		Clothes.Held = PersonKit::EHeld::None;
		Robbed->SetOutfit(Clothes);
		Robbed->AfterThief(Danger, true, Clothes);
	}
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->Say(TEXT("Fuvaros"), TEXT("H\u00e9! A teheraut\u00f3m! Tolvaj!"), TEXT("Hey! My lorry! Thief!"), 3.f);
	}

	APestiAlvilagGameModeBase::Deed(this, StreetName::EDeed::CarStolen, Location);
	if (APolicePatrols::Witnessed(this, Location, LorryRun::TheftSeen))
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

void ALorry::PostTaken()
{
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	const int32 Sack = FMath::RandRange(LorryRun::SackLeast, LorryRun::SackMost);
	if (Game)
	{
		Game->AddMoney(Sack);
	}
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->Say(TEXT("Postakocsi"), FString::Printf(TEXT("Egy postazs\u00e1k a padl\u00f3n: %s."), *APestiAlvilagGameModeBase::FormatMoney(Sack)),
			FString::Printf(TEXT("A mail sack on the floor: %s."), *APestiAlvilagGameModeBase::FormatMoney(Sack)), 3.f);
	}
	// The Royal Post's own van (the theft itself is noted where it was taken): every patrol hears of it.
	if (Game)
	{
		Game->ReportCrime(2);
	}
}
