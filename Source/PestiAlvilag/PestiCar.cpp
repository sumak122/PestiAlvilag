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
#include "PoliceCar.h"
#include "TrafficCar.h"
#include "Tram.h"

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

	// The engine has three gears: each runs up to this share of top speed.
	const float GearTops[] = { 0.28f, 0.6f, 1.f };
	// Lifting off sharply above this share of top speed sometimes bangs the exhaust.
	constexpr float BackfireSpeed = 0.4f;
	constexpr float BackfireChance = 0.3f;
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
	// Nobody at the wheel: the engine is off.
	Visuals->SetEngineRunning(PC != nullptr);
	if (!PC)
	{
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

	UpdateFreeLook(PC, DeltaSeconds);
	UpdateSpeed(Throttle, bHandbrake, DeltaSeconds);
	Visuals->SetSteer(Steer);
	Visuals->SetThrottle(FMath::Abs(Throttle));
	UpdateEngineNoise(Throttle);

	// Turning needs some speed, and gets gentler near top speed.
	const float AbsSpeed = FMath::Abs(ForwardSpeed);
	const float SpeedFactor = FMath::Clamp(AbsSpeed / FullTurnSpeed, 0.f, 1.f);
	const float HighSpeedDamping = 1.f - 0.45f * FMath::Clamp(AbsSpeed / MaxSpeed, 0.f, 1.f);
	const float HandbrakeBoost = bHandbrake ? 1.6f : 1.f;
	const float YawDelta = Steer * MaxTurnRate * SpeedFactor * HighSpeedDamping * HandbrakeBoost
		* FMath::Sign(ForwardSpeed) * DeltaSeconds;
	AddActorWorldRotation(FRotator(0.f, YawDelta, 0.f));

	MoveWithSlide(GetActorForwardVector() * ForwardSpeed * DeltaSeconds);
	SnapToGround();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::White,
			FString::Printf(TEXT("%d km/h   (F: get out, C: camera, N: traffic view, P: police test)"), FMath::RoundToInt(AbsSpeed * 0.036f)));
	}
}

FVector APestiCar::GetVelocity() const
{
	return GetActorForwardVector() * ForwardSpeed;
}

void APestiCar::TakeDriver(APawn* InDriver)
{
	Driver = InDriver;
	DriverInTime = GetWorld()->GetTimeSeconds();
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
			PC->Possess(Person);
			PC->SetControlRotation(FRotator(-10.f, GetActorRotation().Yaw, 0.f));
			return;
		}
	}
}

void APestiCar::UpdateEngineNoise(float Throttle)
{
	// The revs climb through each gear, then drop back at the change.
	const float Share = FMath::Clamp(FMath::Abs(ForwardSpeed) / MaxSpeed, 0.f, 1.f);
	float Bottom = 0.f;
	float InGear = 0.f;
	for (const float Top : PestiCar::GearTops)
	{
		if (Share <= Top || Top >= 1.f)
		{
			InGear = (Share - Bottom) / (Top - Bottom);
			break;
		}
		Bottom = Top;
	}
	const float Pedal = FMath::Abs(Throttle);
	// Standing still, the pedal just revs the engine a little.
	const float Revs = FMath::Max(InGear * 0.85f + 0.1f * Pedal, 0.3f * Pedal * (1.f - Share));
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
	if (FMath::Abs(ForwardSpeed) > PestiCar::BumpSpeed && Now - LastBumpTime > 1.0)
	{
		++BumpCount;
		LastBumpTime = Now;
	}

	// Ramming traffic brings the police; ramming the police more so.
	const AActor* Rammed = Hit.GetActor();
	if (Rammed && FMath::Abs(ForwardSpeed) > PestiCar::CrimeImpactSpeed && (Rammed->IsA<ATrafficCar>() || Rammed->IsA<ATram>()))
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
