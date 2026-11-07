// Fill out your copyright notice in the Description page of Project Settings.

#include "Mission.h"

#include "CarBody.h"
#include "Cutscene.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GoalMarkerComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "PestiText.h"
#include "RouteGuideComponent.h"
#include "StartRingComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace Mission
{
	// Closer than this the marker alone is enough; no route line on the minimap.
	constexpr double RouteMinDistance = 4000.0;

	// After failing: the offer to try again comes after this long and
	// stays this long; trying again starts the mission this soon, the
	// picture coming back over this long (s).
	constexpr double RetryAfter = 3.0;
	constexpr double RetryOffer = 20.0;
	constexpr float RetryStart = 0.5f;
	constexpr float RetryFade = 1.0f;

	// The ring stands this far in front of whoever gives the mission (cm),
	// and counts only once the player has been this much further out.
	constexpr double RingAhead = 170.0;
	constexpr double RingArmGap = 80.0;

	// The opening scenes seen in this play (a mission tried again skips its own).
	TWeakObjectPtr<UWorld> SeenWorld;
	TSet<FName> SeenOpenings;
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

	Ring = CreateDefaultSubobject<UStartRingComponent>(TEXT("Ring"));
	Ring->SetupAttachment(Root);
}

void AMission::BeginPlay()
{
	Super::BeginPlay();

	SetUpMission();
	if (Steps.Num() == 0)
	{
		return;
	}
	if (!bStartsAtRing)
	{
		StartStep(0);
		return;
	}
	// Wait in a ring in front of whoever gives the mission.
	const FMissionStep& First = Steps[0];
	FVector Spot = First.Target;
	if (const AActor* Giver = First.SceneWith.Get())
	{
		Spot = Giver->GetActorLocation() + Giver->GetActorForwardVector().GetSafeNormal2D() * Mission::RingAhead;
	}
	Ring->ShowAt(Spot, StartLetter);
	bAtRing = true;
	bRingArmed = false;
}

bool AMission::GetStartSpot(FVector& OutAt, FVector& OutAway) const
{
	if (!bAtRing || !Ring || Steps.Num() == 0)
	{
		return false;
	}
	OutAt = Ring->GetSpot();
	OutAway = FVector::ForwardVector;
	if (const AActor* Giver = Steps[0].SceneWith.Get())
	{
		const FVector Out = (OutAt - Giver->GetActorLocation()).GetSafeNormal2D();
		if (!Out.IsNearlyZero())
		{
			OutAway = Out;
		}
	}
	return true;
}

void AMission::TickRing()
{
	const APawn* Player = GetPlayer();
	if (!Player)
	{
		return;
	}
	const double Distance = FVector::Dist2D(Player->GetActorLocation(), Ring->GetSpot());
	if (Distance > UStartRingComponent::Radius() + Mission::RingArmGap)
	{
		bRingArmed = true;
		return;
	}
	if (!bRingArmed || Distance > UStartRingComponent::Radius() || !Player->IsA<APestiPerson>())
	{
		return;
	}
	// In he walks: the first step (talking to the giver) is done there and then.
	bAtRing = false;
	Ring->Hide();
	StartStep(0);
	FinishStep();
}

void AMission::PlayStepScene(int32 Step)
{
	if (!Steps.IsValidIndex(Step) || Steps[Step].SceneSeconds <= 0.f || !GetPlayer() || !GetPlayer()->IsA<APestiPerson>())
	{
		return;
	}
	const FMissionStep& Done = Steps[Step];
	// The opening scene, with the mission's name; once only in a play.
	const bool bOpening = Step == 0 && bStartsAtRing;
	if (bOpening)
	{
		if (Mission::SeenWorld.Get() != GetWorld())
		{
			Mission::SeenWorld = GetWorld();
			Mission::SeenOpenings.Reset();
		}
		if (Mission::SeenOpenings.Contains(MissionId))
		{
			return;
		}
		Mission::SeenOpenings.Add(MissionId);
	}
	ACutscene::PlayTalk(GetWorld(), Done.SceneWith.Get(), Done.Target, Done.SceneSeconds,
		bOpening ? Number : 0, bOpening ? TitleHu : FString(), bOpening ? TitleEn : FString());
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
	CloseLetter();

	Super::EndPlay(EndPlayReason);
}

void AMission::AddMissionActor(AActor* Actor)
{
	if (Actor)
	{
		MissionActors.Add(Actor);
	}
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

	TickLetter();
	if (bAtRing)
	{
		TickRing();
		return;
	}
	if (bFailed)
	{
		TickFailed();
		return;
	}
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

	// The way there as a line on the minimap, while the target is still far off.
	if (Step.bGuide && Distance > Mission::RouteMinDistance)
	{
		Guide->Guide(At, Step.Target);
	}
	else
	{
		Guide->Stop();
	}

	const APlayerController* PC = Cast<APlayerController>(Player->GetController());
	if (!Step.bManual && bThere && (!Step.bPressE || (PC && PC->WasInputKeyJustPressed(EKeys::E))))
	{
		FinishStep();
	}
}

void AMission::FinishStep()
{
	if (!Steps.IsValidIndex(CurrentStep) || bFailed)
	{
		return;
	}
	const int32 Done = CurrentStep;
	OnStepDone(Done);
	PlayStepScene(Done);
	if (Done + 1 < Steps.Num())
	{
		StartStep(Done + 1);
	}
	else
	{
		Pass();
	}
}

void AMission::SetStepTarget(const FVector& At)
{
	if (Steps.IsValidIndex(CurrentStep))
	{
		Steps[CurrentStep].Target = At;
		Marker->ShowAt(At);
	}
}

void AMission::Fail(const FString& WhyHu, const FString& WhyEn)
{
	if (bFailed)
	{
		return;
	}
	bFailed = true;
	FailedAt = GetWorld()->GetTimeSeconds();
	Marker->Hide();
	Guide->Stop();
	ACutscene::StopAll(this);
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->ClearGoal();
	}
	Announce(TEXT("K\u00dcLDET\u00c9S SIKERTELEN"), FString::Printf(TEXT("Mission failed  \u00b7  %s  \u00b7  %s"), *WhyHu, *WhyEn), 6.f);
	OnFailed();
}

void AMission::TickFailed()
{
	const double Since = GetWorld()->GetTimeSeconds() - FailedAt;
	if (Since < Mission::RetryAfter)
	{
		return;
	}
	if (Since > Mission::RetryAfter + Mission::RetryOffer)
	{
		Retry(false);
		return;
	}
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->Prompt(TEXT("E"), TEXT("\u00daj pr\u00f3ba"), TEXT("Try again"), false);
	}
	const APawn* Player = GetPlayer();
	const APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
	if (PC && PC->WasInputKeyJustPressed(EKeys::E))
	{
		Retry(true);
	}
}

void AMission::Retry(bool bPutBack)
{
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	FVector At;
	double Yaw = 0.0;
	APestiPerson* Person = Cast<APestiPerson>(GetPlayer());
	if (bPutBack && Person && GetRetrySpot(At, Yaw))
	{
		// A moment of black, and he is back at the start.
		Person->SetActorLocationAndRotation(At, FRotator(0.0, Yaw, 0.0), false, nullptr, ETeleportType::TeleportPhysics);
		if (APlayerController* PC = Cast<APlayerController>(Person->GetController()))
		{
			PC->SetControlRotation(FRotator(-10.0, Yaw, 0.0));
			if (PC->PlayerCameraManager)
			{
				PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, Mission::RetryFade, FLinearColor::Black);
			}
		}
	}
	if (Game)
	{
		Game->StartNextMission(bPutBack ? Mission::RetryStart : -1.f);
	}
	Destroy();
}

bool AMission::GetWayTo(FVector& OutTarget, FString& OutPlace) const
{
	if (bFailed || !Steps.IsValidIndex(CurrentStep) || Steps[CurrentStep].Target.IsNearlyZero())
	{
		return false;
	}
	OutTarget = Steps[CurrentStep].Target;
	OutPlace = Steps[CurrentStep].Place;
	return true;
}

void AMission::TickLetter()
{
	FString LetterHu;
	FString LetterEn;
	if (bFailed || !Steps.IsValidIndex(CurrentStep) || !GetLetter(LetterHu, LetterEn))
	{
		CloseLetter();
		return;
	}
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}
	const bool bToggle = PC->WasInputKeyJustPressed(EKeys::R);
	const bool bPutAway = bLetterOpen && PC->WasInputKeyJustPressed(EKeys::BackSpace);
	if (!bToggle && !bPutAway)
	{
		return;
	}
	if (bLetterOpen)
	{
		CloseLetter();
	}
	else if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->SetLetter(LetterHu, LetterEn);
		bLetterOpen = true;
	}
}

void AMission::CloseLetter()
{
	if (!bLetterOpen)
	{
		return;
	}
	bLetterOpen = false;
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->ClearLetter();
	}
}

void AMission::Pass()
{
	CloseLetter();
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
