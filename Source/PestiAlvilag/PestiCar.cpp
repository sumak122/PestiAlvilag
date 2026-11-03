// Fill out your copyright notice in the Description page of Project Settings.

#include "PestiCar.h"

#include "Camera/CameraComponent.h"
#include "CarVisuals.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineSound.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiPerson.h"
#include "PersonModel.h"
#include "PoliceCar.h"
#include "TrafficCar.h"
#include "Tram.h"
#include "PestiHud.h"

namespace PestiCar
{
	// Box half size in cm: a 4.2 m long, 1.7 m wide period saloon.
	const FVector BoxExtent(210.f, 85.f, 70.f);

	constexpr float ChaseArmLength = 700.f;
	constexpr float ChasePitch = -12.f;
	constexpr float OverheadArmLength = 2600.f;
	constexpr float OverheadPitch = -70.f;
	// How far free look may tilt the chase camera up and down (degrees).
	constexpr float MinLookPitch = -50.f;
	constexpr float MaxLookPitch = 20.f;
	// Free look swings back behind the car at this many degrees per second,
	// once the car is moving faster than this (cm/s).
	constexpr float LookResetRate = 120.f;
	constexpr float LookResetSpeed = 200.f;

	// Ramming something faster than this is a crime (cm/s; 500 is 18 km/h).
	constexpr float CrimeImpactSpeed = 500.f;
	// Hitting anything faster than this jolts a passenger (cm/s).
	constexpr float BumpSpeed = 300.f;
	// The driver can only get out below this speed (cm/s).
	constexpr float MaxExitSpeed = 300.f;
	// How far beside the car the driver steps out (cm from the centre).
	constexpr float ExitSideDistance = 170.f;
	constexpr double MinTimeInCar = 0.3;

	// The engine has four gears: each runs up to this share of top speed...
	const float GearTops[] = { 0.2f, 0.42f, 0.68f, 1.f };
	constexpr int32 NumGears = static_cast<int32>(UE_ARRAY_COUNT(GearTops));
	// ...where the revs reach this (1 is flat out, so the engine never screams).
	constexpr float GearChangeRevs = 0.8f;
	// It changes down once the speed falls this far below the lower gear's top.
	constexpr float ChangeDownShare = 0.85f;
	// Lifting off sharply above this share of top speed sometimes bangs the exhaust.
	constexpr float BackfireSpeed = 0.4f;
	constexpr float BackfireChance = 0.3f;

	// Grip: how fast the tyres kill sliding sideways (each second the slide
	// shrinks to e^-Grip of itself). Firm on the road, less when cornering
	// hard at speed, and very little with the handbrake on: the back steps out.
	constexpr float RoadGrip = 14.f;
	constexpr float CorneringGrip = 6.f;
	constexpr float HandbrakeGrip = 1.8f;
	// Sliding sideways also scrubs off speed (cm/s each second, at most).
	constexpr float SlideDrag = 900.f;
	// The car's weight: how fast the turning follows the wheel (higher is quicker).
	constexpr float TurnResponse = 7.f;
}

APestiCar::APestiCar()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetBoxExtent(PestiCar::BoxExtent);
	CollisionBox->SetCollisionProfileName(TEXT("Pawn"));
	RootComponent = CollisionBox;

	// The box bottom floats HoverHeight above the road; the body and wheels
	// reach down to the road (see CarModel). Built in BeginPlay, in its style.
	Visuals = CreateDefaultSubobject<UCarVisuals>(TEXT("Visuals"));

	EngineNoise = CreateDefaultSubobject<UEngineSound>(TEXT("EngineNoise"));
	EngineNoise->SetupAttachment(CollisionBox);

	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	CameraArm->SetupAttachment(CollisionBox);
	CameraArm->TargetArmLength = PestiCar::ChaseArmLength;
	CameraArm->SetRelativeRotation(FRotator(PestiCar::ChasePitch, 0.f, 0.f));
	CameraArm->TargetOffset = FVector(0.f, 0.f, 150.f);
	CameraArm->bUsePawnControlRotation = false;
	CameraArm->bInheritRoll = false;
	CameraArm->bEnableCameraLag = true;
	CameraArm->CameraLagSpeed = 8.f;
	CameraArm->bEnableCameraRotationLag = true;
	CameraArm->CameraRotationLagSpeed = 6.f;

	ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
	ChaseCamera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
}

void APestiCar::BeginPlay()
{
	Super::BeginPlay();

	if (!bStyleSet)
	{
		Style = CarModel::FStyle();
		Style.PaintColor = BodyColor;
	}
	Visuals->Show(CollisionBox, Style);

	SnapToGround();
}

void APestiCar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = Cast<APlayerController>(GetController());
	// Nobody at the wheel: the engine is off, and so are the lights.
	Visuals->SetEngineRunning(PC != nullptr);
	if (!PC)
	{
		Visuals->SetHeadlights(false);
		Visuals->SetBraking(false);
		SideSpeed = 0.f;
		TurnRate = 0.f;
		if (EngineNoise->IsPlaying())
		{
			EngineNoise->Stop();
		}
		return;
	}
	if (!EngineNoise->IsPlaying())
	{
		EngineNoise->Start();
	}

	auto KeyAxis = [PC](const FKey& Positive, const FKey& Negative)
	{
		return (PC->IsInputKeyDown(Positive) ? 1.f : 0.f) - (PC->IsInputKeyDown(Negative) ? 1.f : 0.f);
	};

	const float Throttle = FMath::Clamp(
		KeyAxis(EKeys::W, EKeys::S) + KeyAxis(EKeys::Up, EKeys::Down)
			+ PC->GetInputAnalogKeyState(EKeys::Gamepad_RightTriggerAxis)
			- PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftTriggerAxis),
		-1.f, 1.f);
	const float Steer = FMath::Clamp(
		KeyAxis(EKeys::D, EKeys::A) + KeyAxis(EKeys::Right, EKeys::Left)
			+ PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX),
		-1.f, 1.f);
	const bool bHandbrake = PC->IsInputKeyDown(EKeys::SpaceBar) || PC->IsInputKeyDown(EKeys::Gamepad_FaceButton_Bottom);

	if (PC->WasInputKeyJustPressed(EKeys::C) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Top))
	{
		ToggleCamera();
	}
	if (PC->WasInputKeyJustPressed(EKeys::F) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right))
	{
		LetDriverOut(PC);
		return;
	}

	// H switches the headlights on and off.
	if (PC->WasInputKeyJustPressed(EKeys::H) || PC->WasInputKeyJustPressed(EKeys::Gamepad_DPad_Up))
	{
		bHeadlightsOn = !bHeadlightsOn;
	}
	Visuals->SetHeadlights(bHeadlightsOn);
	// The brake lights: the handbrake, or the pedal against the way the car is going.
	const bool bBraking = (bHandbrake || (Throttle < -0.05f && ForwardSpeed > 20.f) || (Throttle > 0.05f && ForwardSpeed < -20.f))
		&& FMath::Abs(ForwardSpeed) > 20.f;
	Visuals->SetBraking(bBraking);

	UpdateFreeLook(PC, DeltaSeconds);
	UpdateSpeed(Throttle, bHandbrake, DeltaSeconds);
	Visuals->SetSteer(Steer);
	Visuals->SetThrottle(FMath::Abs(Throttle));
	UpdateEngineNoise(Throttle);

	// Turning needs some speed, and gets gentler near top speed. The car is
	// heavy: the turning builds up and dies away rather than snapping.
	const float AbsSpeed = FMath::Abs(ForwardSpeed);
	const float SpeedFactor = FMath::Clamp(AbsSpeed / FullTurnSpeed, 0.f, 1.f);
	const float SpeedShare = FMath::Clamp(AbsSpeed / MaxSpeed, 0.f, 1.f);
	const float HighSpeedDamping = 1.f - 0.45f * SpeedShare;
	const float HandbrakeBoost = bHandbrake ? 1.6f : 1.f;
	const float WantedTurnRate = Steer * MaxTurnRate * SpeedFactor * HighSpeedDamping * HandbrakeBoost
		* FMath::Sign(ForwardSpeed);
	TurnRate = FMath::FInterpTo(TurnRate, WantedTurnRate, DeltaSeconds, PestiCar::TurnResponse);

	// The car turns, but the way it was going carries on: what was straight
	// ahead is now partly sideways, and the tyres have to pull it round.
	const FVector Moving = GetActorForwardVector() * ForwardSpeed + GetActorRightVector() * SideSpeed;
	AddActorWorldRotation(FRotator(0.f, TurnRate * DeltaSeconds, 0.f));
	ForwardSpeed = static_cast<float>(FVector::DotProduct(Moving, GetActorForwardVector()));
	SideSpeed = static_cast<float>(FVector::DotProduct(Moving, GetActorRightVector()));

	const float Grip = bHandbrake
		? PestiCar::HandbrakeGrip
		: FMath::Lerp(PestiCar::RoadGrip, PestiCar::CorneringGrip, SpeedShare * FMath::Abs(Steer));
	SideSpeed *= FMath::Exp(-Grip * DeltaSeconds);
	// Sliding scrubs off some of the speed too.
	const float Scrub = FMath::Min(FMath::Abs(SideSpeed) / 1000.f, 1.f) * PestiCar::SlideDrag * DeltaSeconds;
	ForwardSpeed = FMath::FInterpConstantTo(ForwardSpeed, 0.f, 1.f, Scrub);

	MoveWithSlide((GetActorForwardVector() * ForwardSpeed + GetActorRightVector() * SideSpeed) * DeltaSeconds);
	SnapToGround();

	if (GEngine)
	{
		APestiHUD::Show(this, 1, 0.f, FColor::White,
			FString::Printf(TEXT("%d km/h   (F: get out, C: camera, N: traffic view, P: police test)"), FMath::RoundToInt(GetVelocity().Size2D() * 0.036f)));
	}
}

FVector APestiCar::GetVelocity() const
{
	return GetActorForwardVector() * ForwardSpeed + GetActorRightVector() * SideSpeed;
}

void APestiCar::TakeDriver(APawn* InDriver)
{
	Driver = InDriver;
	DriverInTime = GetWorld()->GetTimeSeconds();
	Visuals->ShowDriver(PersonModel::PlayerOutfit());
	LookYaw = 0.f;
	LookPitch = 0.f;
}

void APestiCar::LetDriverOut(APlayerController* PC)
{
	APestiPerson* Person = Cast<APestiPerson>(Driver);
	// The F that got the driver in may still read as pressed this frame.
	const bool bJustGotIn = GetWorld()->GetTimeSeconds() - DriverInTime < PestiCar::MinTimeInCar;
	if (!Person || bJustGotIn || FMath::Abs(ForwardSpeed) > PestiCar::MaxExitSpeed)
	{
		return;
	}

	// Step out on the left (the kerb side when keeping left), or on the right if that is blocked.
	const FVector Left = -GetActorRightVector().GetSafeNormal2D();
	for (const FVector& Side : { Left, -Left })
	{
		const FVector At = GetActorLocation() + Side * PestiCar::ExitSideDistance;
		if (Person->TryStepOutAt(At, GetActorRotation().Yaw))
		{
			ForwardSpeed = 0.f;
			Driver = nullptr;
			Visuals->HideDriver();
			PC->Possess(Person);
			PC->SetControlRotation(FRotator(-10.f, GetActorRotation().Yaw, 0.f));
			return;
		}
	}
}

void APestiCar::ShowBackSeatPassenger(const PersonKit::FOutfit& Outfit)
{
	Visuals->ShowPassenger(Outfit);
}

void APestiCar::HideBackSeatPassenger()
{
	Visuals->HidePassenger();
}

FVector APestiCar::GetRearDoor(const FVector& Towards) const
{
	const double Side = FVector::DotProduct(Towards - GetActorLocation(), GetActorRightVector()) >= 0.0 ? 1.0 : -1.0;
	return GetActorTransform().TransformPosition(CarModel::RearDoor(Style.Body, Side));
}

void APestiCar::UpdateEngineNoise(float Throttle)
{
	// The revs climb with the speed in each gear, then drop back at the change up.
	const float Share = FMath::Clamp(FMath::Abs(ForwardSpeed) / MaxSpeed, 0.f, 1.f);
	if (Gear < PestiCar::NumGears - 1 && Share > PestiCar::GearTops[Gear])
	{
		++Gear;
	}
	else if (Gear > 0 && Share < PestiCar::GearTops[Gear - 1] * PestiCar::ChangeDownShare)
	{
		--Gear;
	}
	const float InGear = Share / PestiCar::GearTops[Gear] * PestiCar::GearChangeRevs;
	const float Pedal = FMath::Abs(Throttle);
	// Standing still, the pedal just revs the engine a little.
	const float Revs = FMath::Max(InGear + 0.05f * Pedal, 0.3f * Pedal * (1.f - Share));
	EngineNoise->SetEngine(Revs, Pedal);

	if (LastPedal > 0.5f && Pedal < 0.05f && Share > PestiCar::BackfireSpeed && FMath::FRand() < PestiCar::BackfireChance)
	{
		EngineNoise->Backfire();
		Visuals->Backfire();
	}
	LastPedal = Pedal;
}

void APestiCar::UpdateSpeed(float Throttle, bool bHandbrake, float DeltaSeconds)
{
	if (bHandbrake)
	{
		ForwardSpeed = FMath::FInterpConstantTo(ForwardSpeed, 0.f, DeltaSeconds, HandbrakeDeceleration);
	}
	else if (Throttle > 0.05f)
	{
		const float Rate = ForwardSpeed < 0.f
			? BrakeDeceleration
			: Acceleration * (1.f - 0.6f * FMath::Clamp(ForwardSpeed / MaxSpeed, 0.f, 1.f));
		ForwardSpeed += Rate * Throttle * DeltaSeconds;
	}
	else if (Throttle < -0.05f)
	{
		const float Rate = ForwardSpeed > 0.f ? BrakeDeceleration : Acceleration * 0.6f;
		ForwardSpeed += Rate * Throttle * DeltaSeconds;
	}
	else
	{
		ForwardSpeed = FMath::FInterpConstantTo(ForwardSpeed, 0.f, DeltaSeconds, RollingDrag);
	}
	ForwardSpeed = FMath::Clamp(ForwardSpeed, -MaxReverseSpeed, MaxSpeed);
}

void APestiCar::MoveWithSlide(const FVector& Delta)
{
	FHitResult Hit;
	AddActorWorldOffset(Delta, true, &Hit);

	if (Hit.bStartPenetrating)
	{
		// Turning on the spot can push a corner into a wall; step back out.
		AddActorWorldOffset(Hit.Normal * (Hit.PenetrationDepth + 1.f), false);
		return;
	}
	if (!Hit.bBlockingHit)
	{
		return;
	}

	// Scraping along a wall is one bump, not one per frame.
	const double Now = GetWorld()->GetTimeSeconds();
	const float HitSpeed = FMath::Sqrt(ForwardSpeed * ForwardSpeed + SideSpeed * SideSpeed);
	if (HitSpeed > PestiCar::BumpSpeed && Now - LastBumpTime > 1.0)
	{
		++BumpCount;
		LastBumpTime = Now;
	}

	// Ramming traffic brings the police; ramming the police more so.
	const AActor* Rammed = Hit.GetActor();
	if (Rammed && HitSpeed > PestiCar::CrimeImpactSpeed && (Rammed->IsA<ATrafficCar>() || Rammed->IsA<ATram>()))
	{
		if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
		{
			Game->ReportCrime(Rammed->IsA<APoliceCar>() ? 2 : 1);
		}
	}

	// A head-on hit nearly stops the car; a glancing one lets it scrape along.
	const FVector WallNormal = FVector(Hit.Normal.X, Hit.Normal.Y, 0.f).GetSafeNormal();
	const float Impact = static_cast<float>(FMath::Abs(FVector::DotProduct(GetActorForwardVector(), WallNormal)));
	ForwardSpeed *= 1.f - 0.8f * Impact;
	// Sliding into it sideways stops the slide the same way.
	const float SideImpact = static_cast<float>(FMath::Abs(FVector::DotProduct(GetActorRightVector(), WallNormal)));
	SideSpeed *= 1.f - 0.8f * SideImpact;
	AddActorWorldOffset(FVector::VectorPlaneProject(Delta * (1.f - Hit.Time), WallNormal), true);
}

void APestiCar::SnapToGround()
{
	const FVector Start = GetActorLocation();
	const FVector End = Start - FVector(0.f, 0.f, 2000.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PestiCarGround), false, this);
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		FVector Location = Start;
		Location.Z = Hit.ImpactPoint.Z + HoverHeight + CollisionBox->GetScaledBoxExtent().Z;
		SetActorLocation(Location, false);
	}
}

void APestiCar::ToggleCamera()
{
	bOverheadCamera = !bOverheadCamera;
	LookYaw = 0.f;
	LookPitch = 0.f;
	CameraArm->TargetArmLength = bOverheadCamera ? PestiCar::OverheadArmLength : PestiCar::ChaseArmLength;
	CameraArm->SetRelativeRotation(FRotator(bOverheadCamera ? PestiCar::OverheadPitch : PestiCar::ChasePitch, 0.f, 0.f));
	CameraArm->bDoCollisionTest = !bOverheadCamera;
}

void APestiCar::UpdateFreeLook(const APlayerController* PC, float DeltaSeconds)
{
	float MouseX = 0.f;
	float MouseY = 0.f;
	PC->GetInputMouseDelta(MouseX, MouseY);
	const float TurnX = MouseX * MouseLookScale + PC->GetInputAnalogKeyState(EKeys::Gamepad_RightX) * StickLookRate * DeltaSeconds;
	const float TurnY = MouseY * MouseLookScale + PC->GetInputAnalogKeyState(EKeys::Gamepad_RightY) * StickLookRate * DeltaSeconds;

	if (!FMath::IsNearlyZero(TurnX) || !FMath::IsNearlyZero(TurnY))
	{
		TimeSinceLook = 0.f;
		LookYaw = FRotator::NormalizeAxis(LookYaw + TurnX);
		// The overhead camera only turns around; it keeps looking down.
		if (!bOverheadCamera)
		{
			LookPitch = FMath::Clamp(LookPitch + TurnY,
				PestiCar::MinLookPitch - PestiCar::ChasePitch, PestiCar::MaxLookPitch - PestiCar::ChasePitch);
		}
	}
	else
	{
		TimeSinceLook += DeltaSeconds;
		// Like GTA: when driving and not looking around, the camera settles back behind the car.
		if (TimeSinceLook > LookResetDelay && FMath::Abs(ForwardSpeed) > PestiCar::LookResetSpeed && !bOverheadCamera)
		{
			LookYaw = FMath::FixedTurn(LookYaw, 0.f, PestiCar::LookResetRate * DeltaSeconds);
			LookPitch = FMath::FInterpConstantTo(LookPitch, 0.f, DeltaSeconds, PestiCar::LookResetRate);
		}
	}

	const float BasePitch = bOverheadCamera ? PestiCar::OverheadPitch : PestiCar::ChasePitch;
	CameraArm->SetRelativeRotation(FRotator(BasePitch + LookPitch, LookYaw, 0.f));
}
