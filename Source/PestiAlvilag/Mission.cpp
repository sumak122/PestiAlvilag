// Fill out your copyright notice in the Description page of Project Settings.

#include "Mission.h"

#include "CarBody.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GoalMarkerComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "PestiText.h"
#include "RouteGuideComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace Mission
{
	// Closer than this the marker alone is enough; no arrows.
	constexpr double RouteMinDistance = 4000.0;
}

AMission::AMission()
{
	PrimaryActorTick.bCanEverTick = true;

	BaseMaterial = CarBody::LoadPaintMaterial();

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Marker = CreateDefaultSubobject<UGoalMarkerComponent>(TEXT("Marker"));
	Marker->SetupAttachment(Root);

	Guide = CreateDefaultSubobject<URouteGuideComponent>(TEXT("Guide"));
	Guide->SetupAttachment(Root);
}

void AMission::BeginPlay()
{
	Super::BeginPlay();

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
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->Say(Who, LineHu, LineEn, Seconds);
	}
}

void AMission::Announce(const FString& LineHu, const FString& LineEn, float Seconds) const
{
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->Announce(LineHu, LineEn, Seconds);
	}
}

void AMission::StartStep(int32 Step)
{
	CurrentStep = Step;
	Guide->Stop();
	const FMissionStep& Next = Steps[Step];
	Marker->ShowAt(Next.Target);
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

	// On the goal panel's right: the key to press, getting out, or how far and which way.
	FString Far;
	FString Way;
	if (bThere && Step.bPressE)
	{
		Far = TEXT("E");
		Way = TEXT("press");
	}
	else if (Distance >= Step.Radius)
	{
		Far = PestiText::Metres(Distance);
		Way = PestiText::CompassHu(At, Step.Target);
	}
	else if (!bOnFoot)
	{
		Far = TEXT("Sz\u00e1llj ki!");
		Way = TEXT("get out");
	}
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->SetGoal(Step.GoalHu, Step.GoalEn, Far, Way);
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
	Marker->Hide();
	Guide->Stop();
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->ClearGoal();
	}

	if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
	{
		Game->AddMoney(Reward);
		Game->MarkMissionDone(MissionId);
		Announce(TEXT("K\u00dcLDET\u00c9S TELJES\u00cdTVE"),
			FString::Printf(TEXT("Mission passed  \u00b7  %s (%s)  \u00b7  +%s"), *TitleHu, *TitleEn, *APestiAlvilagGameModeBase::FormatMoney(Reward)), 6.f);
		Game->StartNextMission();
	}

	// Leave the people where they are for a while, then tidy up.
	SetActorTickEnabled(false);
	SetLifeSpan(20.f);
}
