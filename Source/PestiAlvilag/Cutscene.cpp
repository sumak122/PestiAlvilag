// Fill out your copyright notice in the Description page of Project Settings.

#include "Cutscene.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "PestiHud.h"

namespace Film
{
	// The bars slide in and out this fast (s).
	constexpr float BarsIn = 0.4f;
	constexpr float BarsOut = 0.6f;
	// Going back to the player's own camera (s).
	constexpr float BlendBack = 0.6f;
	// Skipping only counts after this long, so a key held from before doesn't skip it (s).
	constexpr float SkipAfter = 0.3f;
	// The film camera's field of view (degrees).
	constexpr float FieldOfView = 42.f;
	// How long the title shows at the end (s).
	constexpr float TitleSeconds = 3.5f;
	constexpr float ShortestTalk = 3.f;

	// Talk shots, from the people's middles (cm): the camera looks this far
	// above them, the wide shot stands this far out to the side (at least)
	// and this much higher, over the shoulder this far behind and beside
	// his head, the title shot further out and higher.
	constexpr double LookUp = 50.0;
	constexpr double SideOut = 330.0;
	constexpr double SideApart = 1.6;
	constexpr double WideUp = 40.0;
	constexpr double DriftIn = 0.12;
	constexpr double HeadUp = 70.0;
	constexpr double ShoulderBack = 120.0;
	constexpr double ShoulderBackEnd = 100.0;
	constexpr double ShoulderSide = 45.0;
	constexpr double ShoulderUp = 10.0;
	constexpr double ShoulderMinReach = 60.0;
	constexpr double TitleOut = 1.7;
	constexpr double TitleOutEnd = 1.9;
	constexpr double TitleUp = 220.0;
	constexpr double TitleUpEnd = 260.0;
	// A camera stays this far off walls (cm), as a ball this big.
	constexpr double OffWall = 30.0;
	constexpr float CameraBall = 20.f;
}

ACutscene::ACutscene()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

ACutscene* ACutscene::Play(UWorld* World, const TArray<FSceneShot>& InShots, int32 Number, const FString& Name, const FString& NameEn, float TitleSeconds)
{
	if (!World || InShots.Num() == 0)
	{
		return nullptr;
	}
	StopAll(World);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACutscene* Cut = World->SpawnActor<ACutscene>(ACutscene::StaticClass(), FTransform::Identity, Params);
	if (!Cut)
	{
		return nullptr;
	}
	Cut->Shots = InShots;
	Cut->TitleNumber = Number;
	Cut->TitleName = Name;
	Cut->TitleEn = NameEn;
	Cut->TitleFor = Name.IsEmpty() ? 0.f : TitleSeconds;
	Cut->Total = 0.f;
	for (const FSceneShot& Shot : InShots)
	{
		Cut->Total += FMath::Max(Shot.Seconds, 0.1f);
	}
	return Cut;
}

FVector ACutscene::Clear(UWorld* World, const FVector& From, const FVector& Want, const TArray<const AActor*>& Ignore, double& OutReach)
{
	const FVector Way = Want - From;
	const double Length = Way.Size();
	OutReach = Length;
	if (!World || Length < 1.0)
	{
		return Want;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SceneCamera), false);
	for (const AActor* Actor : Ignore)
	{
		if (Actor)
		{
			Params.AddIgnoredActor(Actor);
		}
	}
	FHitResult Hit;
	if (World->SweepSingleByChannel(Hit, From, Want, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(Film::CameraBall), Params))
	{
		OutReach = FMath::Max(static_cast<double>(Hit.Distance) - Film::OffWall, 0.0);
		return From + Way / Length * OutReach;
	}
	return Want;
}

ACutscene* ACutscene::PlayTalk(UWorld* World, AActor* With, const FVector& WithAt, float Seconds, int32 Number, const FString& Name, const FString& NameEn)
{
	using namespace Film;
	if (!World)
	{
		return nullptr;
	}
	const APlayerController* PC = World->GetFirstPlayerController();
	APawn* Player = PC ? PC->GetPawn() : nullptr;
	const FVector Other = With ? With->GetActorLocation() : WithAt;
	const FVector Me = Player ? Player->GetActorLocation() : Other - FVector(150.0, 0.0, 0.0);
	FVector Ahead = (Other - Me).GetSafeNormal2D();
	if (Ahead.IsNearlyZero())
	{
		Ahead = Player ? Player->GetActorForwardVector().GetSafeNormal2D() : FVector::ForwardVector;
	}
	if (Player)
	{
		// He turns to whoever he talks to.
		Player->SetActorRotation(FRotator(0.0, Ahead.Rotation().Yaw, 0.0));
	}
	const FVector Side(-Ahead.Y, Ahead.X, 0.0);
	const FVector Look = (Me + Other) * 0.5 + FVector(0.0, 0.0, LookUp);
	const double Out = FMath::Max(SideOut, FVector::Dist2D(Me, Other) * SideApart);
	const TArray<const AActor*> Ignore = { Player, With };

	// The wide shot from whichever side has more room.
	double ReachLeft = 0.0;
	double ReachRight = 0.0;
	const FVector Up(0.0, 0.0, WideUp);
	const FVector Left = Clear(World, Look, Look + Side * Out + Up, Ignore, ReachLeft);
	const FVector Right = Clear(World, Look, Look - Side * Out + Up, Ignore, ReachRight);
	const double Sign = ReachLeft >= ReachRight ? 1.0 : -1.0;
	const FVector WideAt = Sign > 0.0 ? Left : Right;

	const float Talk = FMath::Max(Seconds, ShortestTalk);
	TArray<FSceneShot> List;
	FSceneShot Wide;
	Wide.From = WideAt;
	Wide.FromEnd = FMath::Lerp(WideAt, Look, DriftIn);
	Wide.LookAt = Look;
	Wide.Seconds = Talk * 0.45f;
	List.Add(Wide);

	// Over his shoulder at the one who talks, from the roomier side.
	const FVector Head = Me + FVector(0.0, 0.0, HeadUp);
	FSceneShot Over;
	Over.LookAt = Other + FVector(0.0, 0.0, LookUp);
	Over.Seconds = Talk * 0.55f;
	bool bOver = false;
	for (const double Turn : { Sign, -Sign })
	{
		double Reach = 0.0;
		double ReachEnd = 0.0;
		const FVector From = Clear(World, Head, Head - Ahead * ShoulderBack + Side * Turn * ShoulderSide + FVector(0.0, 0.0, ShoulderUp), Ignore, Reach);
		const FVector FromEnd = Clear(World, Head, Head - Ahead * ShoulderBackEnd + Side * Turn * ShoulderSide + FVector(0.0, 0.0, ShoulderUp), Ignore, ReachEnd);
		if (Reach >= ShoulderMinReach && ReachEnd >= ShoulderMinReach)
		{
			Over.From = From;
			Over.FromEnd = FromEnd;
			bOver = true;
			break;
		}
	}
	if (!bOver)
	{
		// No room behind him: stay on the wide shot a little closer.
		Over.From = Wide.FromEnd;
		Over.FromEnd = FMath::Lerp(WideAt, Look, DriftIn * 2.0);
		Over.LookAt = Look;
	}
	List.Add(Over);

	// The title over a last, wider shot of the two of them.
	if (!Name.IsEmpty())
	{
		double Reach = 0.0;
		FSceneShot Last;
		Last.From = Clear(World, Look, Look + Side * Sign * Out * TitleOut + FVector(0.0, 0.0, TitleUp), Ignore, Reach);
		Last.FromEnd = Clear(World, Look, Look + Side * Sign * Out * TitleOutEnd + FVector(0.0, 0.0, TitleUpEnd), Ignore, Reach);
		Last.LookAt = Look + FVector(0.0, 0.0, LookUp);
		Last.Seconds = TitleSeconds;
		List.Add(Last);
	}
	return Play(World, List, Number, Name, NameEn, TitleSeconds);
}

void ACutscene::StopAll(const UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}
	for (TActorIterator<ACutscene> It(World); It; ++It)
	{
		if (!It->bFinished)
		{
			It->Finish();
		}
	}
}

bool ACutscene::IsPlaying(const UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return false;
	}
	for (TActorIterator<ACutscene> It(World); It; ++It)
	{
		if (!It->bFinished)
		{
			return true;
		}
	}
	return false;
}

void ACutscene::Begin()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->GetPawn())
	{
		return;
	}
	bStarted = true;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FSceneShot& First = Shots[0];
	Camera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), First.From, (First.LookAt - First.From).Rotation(), Params);
	if (Camera)
	{
		Camera->GetCameraComponent()->SetFieldOfView(Film::FieldOfView);
		Camera->GetCameraComponent()->bConstrainAspectRatio = false;
		PC->SetViewTargetWithBlend(Camera, 0.f);
	}
	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);
	Held = PC;
}

void ACutscene::Finish()
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;
	if (APlayerController* PC = Held.Get())
	{
		if (Camera && PC->GetViewTarget() == Camera.Get() && PC->GetPawn())
		{
			PC->SetViewTargetWithBlend(PC->GetPawn(), Film::BlendBack);
		}
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
	}
	Held.Reset();
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->SetSceneTitle(0, FString(), FString());
	}
}

void ACutscene::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APestiHUD* Hud = APestiHUD::Get(this);

	if (bFinished)
	{
		Bars = FMath::Max(Bars - DeltaSeconds / Film::BarsOut, 0.f);
		if (Hud)
		{
			Hud->SetSceneBars(Bars);
		}
		if (Bars <= 0.f)
		{
			Destroy();
		}
		return;
	}
	if (!bStarted)
	{
		Begin();
		if (!bStarted)
		{
			return;
		}
	}

	Clock += DeltaSeconds;
	Bars = FMath::Min(Bars + DeltaSeconds / Film::BarsIn, 1.f);
	if (Hud)
	{
		Hud->SetSceneBars(Bars);
	}

	// The shot that runs now, and how far into it.
	float Start = 0.f;
	for (const FSceneShot& Shot : Shots)
	{
		const float Length = FMath::Max(Shot.Seconds, 0.1f);
		if (Clock < Start + Length || &Shot == &Shots.Last())
		{
			const float T = FMath::Clamp((Clock - Start) / Length, 0.f, 1.f);
			const FVector At = FMath::Lerp(Shot.From, Shot.FromEnd, static_cast<double>(FMath::SmoothStep(0.f, 1.f, T)));
			if (Camera)
			{
				Camera->SetActorLocationAndRotation(At, (Shot.LookAt - At).Rotation());
			}
			break;
		}
		Start += Length;
	}

	if (!bTitleShown && TitleFor > 0.f && Clock >= Total - TitleFor)
	{
		bTitleShown = true;
		if (Hud)
		{
			Hud->SetSceneTitle(TitleNumber, TitleName, TitleEn);
		}
	}

	const APlayerController* PC = Held.Get();
	const bool bSkip = PC && Clock > Film::SkipAfter
		&& (PC->WasInputKeyJustPressed(EKeys::SpaceBar) || PC->WasInputKeyJustPressed(EKeys::Enter)
			|| PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom));
	if (bSkip || Clock >= Total)
	{
		Finish();
	}
}

void ACutscene::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Finish();
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->SetSceneBars(0.f);
	}
	if (Camera)
	{
		Camera->Destroy();
		Camera = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}
