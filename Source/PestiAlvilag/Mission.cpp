// Fill out your copyright notice in the Description page of Project Settings.

#include "Mission.h"

#include "CarBody.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiPerson.h"
#include "PestiText.h"
#include "RouteGuideComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace Mission
{
	constexpr int32 GoalLine = 7;
	constexpr int32 SpeechLine = 8;
	constexpr int32 AnnounceLine = 9;
	const FColor GoalColor(255, 230, 160);
	const FColor SpeechColor(255, 255, 255);
	const FColor AnnounceColor(255, 200, 60);

	constexpr float BeaconHeight = 1500.f;
	// Closer than this the light alone is enough; no arrows.
	constexpr double RouteMinDistance = 4000.0;
}

AMission::AMission()
{
	PrimaryActorTick.bCanEverTick = true;

	BaseMaterial = CarBody::LoadPaintMaterial();

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// The light over the target; it moves from step to step on its own.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Beacon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beacon"));
	Beacon->SetupAttachment(Root);
	Beacon->SetUsingAbsoluteLocation(true);
	Beacon->SetUsingAbsoluteRotation(true);
	Beacon->SetUsingAbsoluteScale(true);
	Beacon->SetStaticMesh(CylinderMesh.Object);
	Beacon->SetWorldScale3D(FVector(0.4f, 0.4f, 30.f));
	Beacon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Beacon->SetCastShadow(false);
	Beacon->SetVisibility(false);

	Guide = CreateDefaultSubobject<URouteGuideComponent>(TEXT("Guide"));
	Guide->SetupAttachment(Root);
}

void AMission::BeginPlay()
{
	Super::BeginPlay();

	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* Light = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Light->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.85f, 0.3f));
		Beacon->SetMaterial(0, Light);
	}

	SetUpMission();
	if (Steps.Num() > 0)
	{
		StartStep(0);
	}
}

void AMission::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (AActor* Actor : MissionActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	MissionActors.Reset();

	Super::EndPlay(EndPlayReason);
}

void AMission::KeepAfterMission(AActor* Actor)
{
	MissionActors.Remove(Actor);
}

APawn* AMission::GetPlayer() const
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	return PC ? PC->GetPawn() : nullptr;
}

double AMission::GroundZ(const FVector2D& At, double Fallback) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MissionGround), false, this);
	const FVector Top(At, 10000.0);
	const FVector Bottom(At, -10000.0);
	return GetWorld()->LineTraceSingleByChannel(Hit, Top, Bottom, ECC_Visibility, Params) ? Hit.ImpactPoint.Z : Fallback;
}

void AMission::Say(const FString& Who, const FString& LineHu, const FString& LineEn, float Seconds) const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(Mission::SpeechLine, Seconds, Mission::SpeechColor,
			FString::Printf(TEXT("%s: \u201e%s\u201d  (%s)"), *Who, *LineHu, *LineEn));
	}
}

void AMission::Announce(const FString& LineHu, const FString& LineEn, float Seconds) const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(Mission::AnnounceLine, Seconds, Mission::AnnounceColor,
			FString::Printf(TEXT("%s   (%s)"), *LineHu, *LineEn), true, FVector2D(1.6f, 1.6f));
	}
}

void AMission::StartStep(int32 Step)
{
	CurrentStep = Step;
	Guide->Stop();
	const FMissionStep& Next = Steps[Step];
	Beacon->SetWorldLocation(FVector(Next.Target.X, Next.Target.Y, Next.Target.Z + Mission::BeaconHeight));
	Beacon->SetVisibility(true);
	OnStepStarted(Step);
}

void AMission::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Steps.IsValidIndex(CurrentStep))
	{
		return;
	}
	TickStep(CurrentStep, DeltaSeconds);

	APawn* Player = GetPlayer();
	if (!Player || !Steps.IsValidIndex(CurrentStep))
	{
		return;
	}
	const FMissionStep& Step = Steps[CurrentStep];
	const FVector At = Player->GetActorLocation();
	const double Distance = FVector::Dist2D(At, Step.Target);
	const bool bOnFoot = Player->IsA<APestiPerson>();
	const bool bThere = Distance < Step.Radius && (bOnFoot || !Step.bOnFoot);

	FString Goal = FString::Printf(TEXT("C\u00e9l: %s  (%s)"), *Step.GoalHu, *Step.GoalEn);
	if (bThere && Step.bPressE)
	{
		Goal += TEXT("   E");
	}
	else if (Distance >= Step.Radius)
	{
		Goal += FString::Printf(TEXT("   %s %s"), *PestiText::Metres(Distance), PestiText::Compass(At, Step.Target));
	}
	else if (!bOnFoot)
	{
		Goal += TEXT("   Sz\u00e1llj ki!  (Get out!)");
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(Mission::GoalLine, 0.f, Mission::GoalColor, Goal);
	}

	// Arrows over the turns on the way, while the target is still far off.
	if (Step.bGuide && Distance > Mission::RouteMinDistance)
	{
		Guide->Guide(At, Step.Target);
	}
	else
	{
		Guide->Stop();
	}

	const APlayerController* PC = Cast<APlayerController>(Player->GetController());
	if (bThere && (!Step.bPressE || (PC && PC->WasInputKeyJustPressed(EKeys::E))))
	{
		const int32 Done = CurrentStep;
		OnStepDone(Done);
		if (Done + 1 < Steps.Num())
		{
			StartStep(Done + 1);
		}
		else
		{
			Pass();
		}
	}
}

void AMission::Pass()
{
	CurrentStep = INDEX_NONE;
	Beacon->SetVisibility(false);
	Guide->Stop();
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(Mission::GoalLine, 0.f, Mission::GoalColor, TEXT(""));
	}

	if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
	{
		Game->AddMoney(Reward);
		Game->MarkMissionDone(MissionId);
		Announce(FString::Printf(TEXT("K\u00dcLDET\u00c9S TELJES\u00cdTVE: %s  +%s"), *TitleHu, *APestiAlvilagGameModeBase::FormatMoney(Reward)),
			FString::Printf(TEXT("Mission passed: %s"), *TitleEn), 6.f);
	}

	// Leave the people where they are for a while, then tidy up.
	SetActorTickEnabled(false);
	SetLifeSpan(20.f);
}
