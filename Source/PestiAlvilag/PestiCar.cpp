// Fill out your copyright notice in the Description page of Project Settings.

#include "PestiCar.h"

#include "Camera/CameraComponent.h"
#include "CarHandling.h"
#include "CarVisuals.h"
#include "Components/BoxComponent.h"
#include "CrashBits.h"
#include "CrashSound.h"
#include "CrashSynth.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineSound.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Pedestrians.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiPerson.h"
#include "PersonModel.h"
#include "PoliceCar.h"
#include "PolicePatrols.h"
#include "TrafficCar.h"
#include "Tram.h"
#include "Weather.h"
#include "PestiHud.h"
#include "PestiMenu.h"

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

	// Ramming something faster than this is a crime, if the police see it (cm/s; 900 is about 32 km/h).
	constexpr float CrimeImpactSpeed = 900.f;
	// A police car this near sees a crash (cm); constables see it as far as they see.
	constexpr double PoliceSeeCrash = 3500.0;
	// Hitting anything faster than this jolts a passenger (cm/s).
	constexpr float BumpSpeed = 300.f;

	// Crash feedback (cm/s straight into what was hit): a thump from here, a crunch with sparks from here,
	// and a crunch is at full strength this much faster than that.
	constexpr float ThumpSpeed = 120.f;
	constexpr float CrunchSpeed = 550.f;
	// Passers-by stand round a crash this long (s).
	constexpr float CrowdHold = 25.f;
	constexpr float CrunchRange = 1500.f;
	// Two crash sounds at least this far apart (s), so one long contact is one crash.
	constexpr double CrashGap = 0.25;
	// Sliding along a wall faster than this (cm/s) scrapes, at full grind at ScrapeFull; sparks every ScrapeSparkGap s.
	constexpr float ScrapeSpeed = 250.f;
	constexpr float ScrapeFull = 1800.f;
	constexpr double ScrapeHold = 0.15;
	constexpr double ScrapeSparkGap = 0.05;
	// The crash sounds are heard this far (cm) and this loud.
	constexpr float CrashSoundFalloff = 6000.f;
	constexpr float CrashSoundVolume = 1.f;
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
	// An old car (CarModel::IsOld) is started with the crank (E): each turn takes
	// this long (s) and catches this often, cold or warm (its driver got out less
	// than this long ago, s).
	constexpr double CrankTime = 0.7;
	constexpr float ColdCrankChance = 0.35f;
	constexpr float WarmCrankChance = 0.75f;
	constexpr double WarmEngineTime = 60.0;
	// It stalls slower than this (cm/s): now and then (about once in this many
	// seconds), sometimes when the pedal goes down hard, and in some crashes.
	constexpr float StallSpeed = 200.f;
	constexpr float StallEvery = 90.f;
	constexpr float StallOnPullChance = 0.15f;
	constexpr float StallCrashSpeed = 550.f;
	constexpr float StallCrashChance = 0.5f;
	// A smashed car window is heard by a police car this near (cm); constables hear it as far as they see.
	constexpr double SmashHeard = 5000.0;

	// Grip: how fast the tyres kill sliding sideways (each second the slide
	// shrinks to e^-Grip of itself). Firm on the road, less when cornering
	// hard at speed, and very little with the handbrake on: the back steps out.
	constexpr float RoadGrip = 14.f;
	constexpr float CorneringGrip = 6.f;
	constexpr float HandbrakeGrip = 1.8f;
	// Sliding sideways also scrubs off speed (cm/s each second, at most).
	constexpr float SlideDrag = 900.f;

	// Crashes (CarHandling::Collide): how much of the speed they met at springs
	// back off another car and off a wall, and the rub along what was hit.
	constexpr float CarBounce = 0.3f;
	constexpr float WallBounce = 0.2f;
	constexpr float CrashRub = 0.3f;
	// The car's own spin from a crash dies away (each second to e^-SpinFade of
	// itself, and by SpinDrag degrees a second); slower with the handbrake on.
	constexpr float SpinFade = 2.5f;
	constexpr float HandbrakeSpinFade = 1.2f;
	constexpr float SpinDrag = 60.f;
	// Nobody driving: the wheels are locked and slow it along and across (cm/s each second).
	constexpr float DriverlessRollDrag = 1500.f;
	constexpr float DriverlessSkidDrag = 1800.f;
	constexpr float DriverlessStill = 5.f;
	// The weight shifts this quickly (per second). With the weight on the
	// front (braking) the car turns in this much more sharply and the back has
	// this much less grip; pulling hard it turns this much less (runs wide).
	constexpr float WeightShiftRate = 5.f;
	constexpr float BrakeTurnIn = 0.3f;
	constexpr float BrakeLooseRear = 0.3f;
	constexpr float PullRunWide = 0.2f;
	// Soaked setts: the tyres grip and brake this share less.
	constexpr float WetGripLoss = 0.35f;
	constexpr float WetBrakeLoss = 0.3f;
	// A wrecked car pulls this share less (in between, by its wear).
	constexpr float WornPullLoss = 0.35f;
	// While the gear changes up the engine pulls only this share; only when pulling harder than ShiftPedal.
	constexpr float ShiftPull = 0.3f;
	constexpr float ShiftPedal = 0.3f;
	// Tyre squeal: sliding sideways from SkidFrom cm/s, at full at SkidFull; the
	// handbrake locks the wheels and they squeal faster than LockSkidSpeed.
	constexpr float SkidFrom = 250.f;
	constexpr float SkidFull = 1100.f;
	constexpr float LockSkidSpeed = 400.f;
	constexpr float LockSkidFull = 1500.f;
	// The chase camera's view widens from ViewSlow to ViewFast degrees at top speed.
	constexpr float ViewSlow = 90.f;
	constexpr float ViewFast = 100.f;
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
	ApplyHandling();

	SnapToGround();
}

void APestiCar::ApplyHandling()
{
	// Shares of the car's own settings, so a taxi tuned faster stays faster. Only once.
	if (bHandlingSet)
	{
		return;
	}
	bHandlingSet = true;
	const CarHandling::FHandling Kind = CarHandling::For(Style.Body);
	Mass = Kind.Mass;
	MaxSpeed *= Kind.TopShare;
	Acceleration *= Kind.PullShare;
	BrakeDeceleration *= Kind.BrakeShare;
	HandbrakeDeceleration *= Kind.BrakeShare;
	MaxTurnRate *= Kind.SteerShare;
	GripShare = Kind.GripShare;
	HandbrakeGripShare = Kind.HandbrakeShare;
	TurnWeight = Kind.TurnResponse;
	ShiftTime = Kind.ShiftTime;
}

void APestiCar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// The scrape grinds on only while the car keeps sliding along the wall.
	if (CrashNoise)
	{
		CrashNoise->SetScrape(GetWorld()->GetTimeSeconds() < ScrapeUntil ? ScrapeAmount : 0.f);
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	// Nobody at the wheel: the engine is off, and so are the lights. Crashed
	// too often, the engine dies: the car only rolls on and can be steered.
	const bool bEngineDead = Visuals->IsEngineDead();
	// An old car's engine runs only once the crank has got it going.
	const bool bOld = CarModel::IsOld(Style.Body);
	const double Now = GetWorld()->GetTimeSeconds();
	if (CrankUntil > 0.0 && Now >= CrankUntil)
	{
		CrankUntil = -1.0;
		bOldEngineOn = bCrankCatches;
		// It catches with a cough of smoke, or dies away with a last putt.
		if (bCrankCatches || FMath::FRand() < 0.4f)
		{
			EngineNoise->Backfire();
			Visuals->Backfire();
		}
	}
	const bool bCranking = CrankUntil > 0.0;
	const bool bEngineOff = bEngineDead || (bOld && !bOldEngineOn);
	Visuals->SetEngineRunning(PC != nullptr && !bEngineOff);
	if (!PC)
	{
		bOldEngineOn = false;
		CrankUntil = -1.0;
		Visuals->SetHeadlights(false);
		Visuals->SetBraking(false);
		TurnRate = 0.f;
		if (EngineNoise->IsPlaying())
		{
			EngineNoise->Stop();
		}
		if (CrashNoise)
		{
			CrashNoise->SetSkid(0.f, 0.f);
		}
		SkidAmount = 0.f;
		RollDriverless(DeltaSeconds);
		return;
	}
	const bool bEngineHeard = !bEngineOff || bCranking;
	if (!bEngineHeard && EngineNoise->IsPlaying())
	{
		EngineNoise->Stop();
	}
	else if (bEngineHeard && !EngineNoise->IsPlaying())
	{
		EngineNoise->Start();
	}

	auto KeyAxis = [PC](const FKey& Positive, const FKey& Negative)
	{
		return (PC->IsInputKeyDown(Positive) ? 1.f : 0.f) - (PC->IsInputKeyDown(Negative) ? 1.f : 0.f);
	};

	float Throttle = FMath::Clamp(
		KeyAxis(EKeys::W, EKeys::S) + KeyAxis(EKeys::Up, EKeys::Down)
			+ PC->GetInputAnalogKeyState(EKeys::Gamepad_RightTriggerAxis)
			- PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftTriggerAxis),
		-1.f, 1.f);
	const float Steer = FMath::Clamp(
		KeyAxis(EKeys::D, EKeys::A) + KeyAxis(EKeys::Right, EKeys::Left)
			+ PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX),
		-1.f, 1.f);
	// A dead (or stopped) engine pulls no more: the pedal only brakes.
	if (bEngineOff)
	{
		Throttle = ForwardSpeed > 20.f ? FMath::Min(Throttle, 0.f) : 0.f;
	}
	// Held still (Laci's garage, a scene): no pedal, and no getting out.
	const bool bHeld = PC->IsMoveInputIgnored();
	if (bHeld)
	{
		Throttle = 0.f;
	}
	const bool bHandbrake = PC->IsInputKeyDown(EKeys::SpaceBar) || PC->IsInputKeyDown(EKeys::Gamepad_FaceButton_Bottom);

	// E turns an old car's crank; a warm engine catches more easily.
	if (bOld && !bEngineDead && !bOldEngineOn && !bCranking && !bHeld
		&& (PC->WasInputKeyJustPressed(EKeys::E) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Left)))
	{
		const bool bWarm = Now - DriverOutTime < PestiCar::WarmEngineTime;
		bCrankCatches = FMath::FRand() < (bWarm ? PestiCar::WarmCrankChance : PestiCar::ColdCrankChance);
		CrankUntil = Now + PestiCar::CrankTime;
	}
	// A running old engine stalls now and then when slow, and sometimes when the pedal goes down hard.
	if (bOld && bOldEngineOn && !bHeld && FMath::Abs(ForwardSpeed) < PestiCar::StallSpeed)
	{
		const bool bStamped = Throttle > 0.9f && LastPedal < 0.1f;
		if ((bStamped && FMath::FRand() < PestiCar::StallOnPullChance) || FMath::FRand() < DeltaSeconds / PestiCar::StallEvery)
		{
			StallEngine();
			Throttle = 0.f;
		}
	}

	if (PC->WasInputKeyJustPressed(EKeys::C) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Top))
	{
		ToggleCamera();
	}
	if (!bHeld && (PC->WasInputKeyJustPressed(EKeys::F) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right)))
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

	// How wet the setts are.
	if (!WeatherFound.IsValid())
	{
		WeatherFound = AWeather::Get(GetWorld());
	}
	RoadWet = WeatherFound.IsValid() ? FMath::Clamp(WeatherFound->GetWet(), 0.f, 1.f) : 0.f;

	// The weight shifts forward under braking and back when pulling away.
	const bool bPedalBrakes = (Throttle < -0.05f && ForwardSpeed > 20.f) || (Throttle > 0.05f && ForwardSpeed < -20.f);
	const float Moving01 = FMath::Clamp(FMath::Abs(ForwardSpeed) / FullTurnSpeed, 0.f, 1.f);
	const float WantedShift = bPedalBrakes ? FMath::Abs(Throttle)
		: (bHandbrake ? 0.5f : -FMath::Max(Throttle, 0.f) * (1.f - FMath::Clamp(FMath::Abs(ForwardSpeed) / MaxSpeed, 0.f, 1.f)));
	WeightShift = FMath::FInterpTo(WeightShift, WantedShift * Moving01, DeltaSeconds, PestiCar::WeightShiftRate);

	UpdateFreeLook(PC, DeltaSeconds);
	UpdateSpeed(Throttle, bHandbrake, DeltaSeconds);
	Visuals->SetSteer(Steer);
	Visuals->SetThrottle(FMath::Abs(Throttle));
	if (bCranking)
	{
		// The crank turns the engine over: a slow putter.
		EngineNoise->SetEngine(0.f, 0.f);
	}
	else if (!bOldEngineOn && bOld)
	{
		LastPedal = FMath::Abs(Throttle);
	}
	else if (!bEngineDead)
	{
		UpdateEngineNoise(Throttle);
	}

	// Turning needs some speed, and gets gentler near top speed. The car is
	// heavy: the turning builds up and dies away rather than snapping.
	const float AbsSpeed = FMath::Abs(ForwardSpeed);
	const float SpeedFactor = FMath::Clamp(AbsSpeed / FullTurnSpeed, 0.f, 1.f);
	const float SpeedShare = FMath::Clamp(AbsSpeed / MaxSpeed, 0.f, 1.f);
	const float HighSpeedDamping = 1.f - 0.45f * SpeedShare;
	const float HandbrakeBoost = bHandbrake ? 1.6f : 1.f;
	// Weight on the nose tucks it into the bend; pulling hard, it runs wide.
	const float WeightTurn = WeightShift > 0.f ? 1.f + PestiCar::BrakeTurnIn * WeightShift : 1.f + PestiCar::PullRunWide * WeightShift;
	const float WantedTurnRate = Steer * MaxTurnRate * SpeedFactor * HighSpeedDamping * HandbrakeBoost * WeightTurn
		* FMath::Sign(ForwardSpeed);
	TurnRate = FMath::FInterpTo(TurnRate, WantedTurnRate, DeltaSeconds, TurnWeight);
	// A crash's spin: the tyres slowly stop it (less so on locked wheels).
	const float SpinFadeNow = bHandbrake ? PestiCar::HandbrakeSpinFade : PestiCar::SpinFade * (1.f - PestiCar::WetGripLoss * RoadWet);
	SpinRate = FMath::FInterpConstantTo(SpinRate * FMath::Exp(-SpinFadeNow * DeltaSeconds), 0.f, DeltaSeconds, PestiCar::SpinDrag);

	// The car turns, but the way it was going carries on: what was straight
	// ahead is now partly sideways, and the tyres have to pull it round.
	const FVector Moving = GetActorForwardVector() * ForwardSpeed + GetActorRightVector() * SideSpeed;
	AddActorWorldRotation(FRotator(0.f, (TurnRate + SpinRate) * DeltaSeconds, 0.f));
	ForwardSpeed = static_cast<float>(FVector::DotProduct(Moving, GetActorForwardVector()));
	SideSpeed = static_cast<float>(FVector::DotProduct(Moving, GetActorRightVector()));

	// Each car grips its own way; braking into a bend lightens the back; wet setts grip less.
	const float Grip = (bHandbrake
		? PestiCar::HandbrakeGrip * HandbrakeGripShare
		: FMath::Lerp(PestiCar::RoadGrip, PestiCar::CorneringGrip, SpeedShare * FMath::Abs(Steer)) * GripShare
			* (1.f - PestiCar::BrakeLooseRear * FMath::Max(WeightShift, 0.f) * FMath::Abs(Steer)))
		* (1.f - PestiCar::WetGripLoss * RoadWet);
	SideSpeed *= FMath::Exp(-Grip * DeltaSeconds);
	// Sliding scrubs off some of the speed too.
	const float Scrub = FMath::Min(FMath::Abs(SideSpeed) / 1000.f, 1.f) * PestiCar::SlideDrag * DeltaSeconds;
	ForwardSpeed = FMath::FInterpConstantTo(ForwardSpeed, 0.f, 1.f, Scrub);

	MoveWithSlide((GetActorForwardVector() * ForwardSpeed + GetActorRightVector() * SideSpeed) * DeltaSeconds);
	SnapToGround();
	UpdateSkidAndView(bHandbrake, DeltaSeconds);

	if (GEngine)
	{
		APestiHUD::Show(this, 1, 0.f, FColor::White,
			FString::Printf(TEXT("%d km/h   %s(F: get out, C: camera, N: traffic view, P: police test)"), FMath::RoundToInt(GetVelocity().Size2D() * 0.036f),
				bEngineDead ? TEXT("A motor lefulladt / The engine is dead   ")
				: bCranking ? TEXT("Hajtod az ind\u00edt\u00f3kart... / Cranking...   ")
				: bEngineOff ? TEXT("A motor \u00e1ll \u00b7 E: ind\u00edt\u00f3kar / Engine off \u00b7 E: crank it   ")
				: TEXT("")));
	}
}

FVector APestiCar::GetVelocity() const
{
	return GetActorForwardVector() * ForwardSpeed + GetActorRightVector() * SideSpeed;
}

void APestiCar::TakeLooksFrom(UCarVisuals* From)
{
	if (Visuals && From)
	{
		Visuals->TakeOverFrom(From);
	}
}

void APestiCar::BreakIn(bool bSmash)
{
	bLocked = false;
	if (!bSmash)
	{
		return;
	}
	// The driver's window (on the left) bursts.
	const CarModel::FLayout Plan = CarModel::Layout(Style.Body);
	const FVector Window = GetActorTransform().TransformPosition(FVector(Plan.SeatX + 30.0, -Plan.HalfWidth, CarModel::Waist + 20.0));
	const double Ground = GetActorLocation().Z - CollisionBox->GetScaledBoxExtent().Z - HoverHeight;
	if (ACrashBits* Bits = ACrashBits::Get(GetWorld()))
	{
		Bits->Burst(Window, -GetActorRightVector(), 0.5f, true, Ground);
	}
	PlayCrashSound(static_cast<uint8>(CrashSynth::EKind::Glass), 0.8f, 0.f);
	// Loud: police near enough hear it.
	if (APolicePatrols::Witnessed(this, Window, PestiCar::SmashHeard))
	{
		if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
		{
			Game->ReportCrime(1);
		}
	}
}

void APestiCar::TakeDriver(APawn* InDriver)
{
	Driver = InDriver;
	bLocked = false;
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
			DriverOutTime = GetWorld()->GetTimeSeconds();
			Visuals->HideDriver();
			PC->Possess(Person);
			PC->SetControlRotation(FRotator(-10.f, GetActorRotation().Yaw, 0.f));
			return;
		}
	}
}

float APestiCar::GetWear() const
{
	return Visuals ? Visuals->GetWear() : 0.f;
}

bool APestiCar::IsWrecked() const
{
	return Visuals && Visuals->IsEngineDead();
}

bool APestiCar::IsDamaged() const
{
	return Visuals && Visuals->IsDamaged();
}

void APestiCar::Repaint(const FLinearColor& Paint)
{
	Style.PaintColor = Paint;
	if (Visuals)
	{
		Visuals->Repaint(Paint);
	}
}

FLinearColor APestiCar::GetPaint() const
{
	return Visuals ? Visuals->GetPaint() : Style.PaintColor;
}

void APestiCar::PlayCrashSound(uint8 Kind, float Strength, float Delay)
{
	if (!CrashNoise)
	{
		CrashNoise = NewObject<UCrashSound>(this);
		CrashNoise->SetupAttachment(GetRootComponent());
		CrashNoise->bAllowSpatialization = true;
		CrashNoise->bOverrideAttenuation = true;
		CrashNoise->AttenuationOverrides.bAttenuate = true;
		CrashNoise->AttenuationOverrides.bSpatialize = true;
		CrashNoise->AttenuationOverrides.FalloffDistance = PestiCar::CrashSoundFalloff;
		CrashNoise->RegisterComponent();
		AddInstanceComponent(CrashNoise);
		CrashNoise->SetVolumeMultiplier(PestiCar::CrashSoundVolume);
	}
	if (!CrashNoise->IsPlaying())
	{
		CrashNoise->Start();
	}
	if (Kind < static_cast<uint8>(CrashSynth::EKind::Count))
	{
		CrashNoise->Queue(static_cast<CrashSynth::EKind>(Kind), Strength, Delay);
	}
}

void APestiCar::CrashFeedback(const FVector& At, const FVector& Away, float Into, float Along, int32 Knocked, bool bSoft)
{
	// A hard knock often stalls an old engine.
	if (bOldEngineOn && Into > PestiCar::StallCrashSpeed && FMath::FRand() < PestiCar::StallCrashChance)
	{
		StallEngine();
	}
	const double Now = GetWorld()->GetTimeSeconds();
	// Where the bits come to rest: the road under the car.
	const double Ground = GetActorLocation().Z - CollisionBox->GetScaledBoxExtent().Z - HoverHeight;
	ACrashBits* Bits = ACrashBits::Get(GetWorld());

	if (Into > PestiCar::ThumpSpeed && Now - LastCrashTime > PestiCar::CrashGap)
	{
		LastCrashTime = Now;
		if (bSoft || Into < PestiCar::CrunchSpeed)
		{
			PlayCrashSound(static_cast<uint8>(CrashSynth::EKind::Bump),
				FMath::Clamp(Into / PestiCar::CrunchSpeed, 0.3f, 1.f), 0.f);
		}
		else
		{
			const float Strength = FMath::Clamp((Into - PestiCar::CrunchSpeed) / PestiCar::CrunchRange, 0.f, 1.f);
			PlayCrashSound(static_cast<uint8>(CrashSynth::EKind::Crunch), 0.6f + 0.5f * Strength, 0.f);
			if (Bits)
			{
				Bits->Burst(At, Away, Strength, (Knocked & CarVisuals::KnockGlass) != 0, Ground);
			}
			// A real crash: passers-by stop and stare.
			APedestrians::Gather(this, At, PestiCar::CrowdHold, false);
		}
		if (Knocked & CarVisuals::KnockGlass)
		{
			PlayCrashSound(static_cast<uint8>(CrashSynth::EKind::Glass), 0.8f, 0.02f);
		}
		if (Knocked & CarVisuals::KnockPart)
		{
			PlayCrashSound(static_cast<uint8>(CrashSynth::EKind::Clang), 0.9f, 0.35f);
		}
	}

	// Sliding along a wall: the grind, and a few sparks now and then.
	if (!bSoft && Along > PestiCar::ScrapeSpeed && Into < PestiCar::CrunchSpeed)
	{
		ScrapeUntil = Now + PestiCar::ScrapeHold;
		ScrapeAmount = FMath::Clamp(Along / PestiCar::ScrapeFull, 0.2f, 1.f);
		if (!CrashNoise)
		{
			// Makes the sound so the scrape can be heard.
			PlayCrashSound(static_cast<uint8>(CrashSynth::EKind::Count), 0.f, 0.f);
		}
		if (Bits && Now - LastScrapeSparks > PestiCar::ScrapeSparkGap)
		{
			LastScrapeSparks = Now;
			const FVector Going = GetActorForwardVector() * ForwardSpeed + GetActorRightVector() * SideSpeed;
			Bits->Scrape(At, Away, FVector::VectorPlaneProject(Going, Away), Ground);
		}
	}
}

void APestiCar::TakeKnock(const FVector& At, const FVector& Push)
{
	const FVector Into = FVector(Push.X, Push.Y, 0.0).GetSafeNormal();
	const float Speed = static_cast<float>(Push.Size2D());
	if (Into.IsNearlyZero() || Speed < 1.f)
	{
		return;
	}
	const int32 Knocked = Visuals->Knock(At, Into, Speed);
	CrashFeedback(At, Into, Speed, 0.f, Knocked, false);
	ForwardSpeed += static_cast<float>(FVector::DotProduct(Push, GetActorForwardVector()));
	SideSpeed += static_cast<float>(FVector::DotProduct(Push, GetActorRightVector()));
	ForwardSpeed = FMath::Clamp(ForwardSpeed, -MaxReverseSpeed, MaxSpeed);
}

bool APestiCar::ForceDriverOut()
{
	APestiPerson* Person = Cast<APestiPerson>(Driver);
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!Person || !PC)
	{
		return false;
	}
	ForwardSpeed = 0.f;
	SideSpeed = 0.f;
	const FVector Left = -GetActorRightVector().GetSafeNormal2D();
	for (const FVector& Side : { Left, -Left })
	{
		const FVector At = GetActorLocation() + Side * PestiCar::ExitSideDistance;
		if (Person->TryStepOutAt(At, GetActorRotation().Yaw))
		{
			Driver = nullptr;
			Visuals->HideDriver();
			PC->Possess(Person);
			PC->SetControlRotation(FRotator(-10.f, GetActorRotation().Yaw, 0.f));
			return true;
		}
	}
	return false;
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

void APestiCar::StallEngine()
{
	bOldEngineOn = false;
	CrankUntil = -1.0;
	EngineNoise->Backfire();
	Visuals->Backfire();
}

void APestiCar::UpdateEngineNoise(float Throttle)
{
	// The revs climb with the speed in each gear, then drop back at the change up.
	const float Share = FMath::Clamp(FMath::Abs(ForwardSpeed) / MaxSpeed, 0.f, 1.f);
	if (Gear < PestiCar::NumGears - 1 && Share > PestiCar::GearTops[Gear])
	{
		++Gear;
		// The clutch goes down for a moment: the pull dips while the gear changes.
		if (Throttle > PestiCar::ShiftPedal)
		{
			ShiftPause = ShiftTime;
		}
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
	// Wet setts brake worse; a battered engine pulls less, and nothing while the gear changes.
	const float Brakes = BrakeDeceleration * (1.f - PestiCar::WetBrakeLoss * RoadWet);
	ShiftPause = FMath::Max(0.f, ShiftPause - DeltaSeconds);
	const float Pull = Acceleration * (1.f - PestiCar::WornPullLoss * GetWear()) * (ShiftPause > 0.f ? PestiCar::ShiftPull : 1.f);
	if (bHandbrake)
	{
		ForwardSpeed = FMath::FInterpConstantTo(ForwardSpeed, 0.f, DeltaSeconds, HandbrakeDeceleration * (1.f - PestiCar::WetBrakeLoss * RoadWet));
	}
	else if (Throttle > 0.05f)
	{
		const float Rate = ForwardSpeed < 0.f
			? Brakes
			: Pull * (1.f - 0.6f * FMath::Clamp(ForwardSpeed / MaxSpeed, 0.f, 1.f));
		ForwardSpeed += Rate * Throttle * DeltaSeconds;
	}
	else if (Throttle < -0.05f)
	{
		const float Rate = ForwardSpeed > 0.f ? Brakes : Pull * 0.6f;
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

	// A real crash into traffic brings the police, but only when a police
	// car or a constable is there to see it; ramming the police more so.
	const AActor* Rammed = Hit.GetActor();
	if (Rammed && GetController() && HitSpeed > PestiCar::CrimeImpactSpeed && (Rammed->IsA<ATrafficCar>() || Rammed->IsA<ATram>())
		&& (Rammed->IsA<APoliceCar>() || APolicePatrols::Witnessed(this, Hit.ImpactPoint, PestiCar::PoliceSeeCrash)))
	{
		if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
		{
			Game->ReportCrime(Rammed->IsA<APoliceCar>() ? 2 : 1);
		}
	}

	const FVector WallNormal = FVector(Hit.Normal.X, Hit.Normal.Y, 0.f).GetSafeNormal();

	// The crash dents the car, and the car it hit: by how fast they met, straight on.
	const FVector Going = GetActorForwardVector() * ForwardSpeed + GetActorRightVector() * SideSpeed;
	const float CrashSpeed = static_cast<float>(FMath::Abs(FVector::DotProduct(Going, WallNormal)));
	const int32 Knocked = Visuals->Knock(Hit.ImpactPoint, WallNormal, CrashSpeed);
	// A person or a horse is a soft knock: a thump, no sparks.
	const bool bSoft = Rammed && Rammed->IsA<APawn>() && !Rammed->FindComponentByClass<UCarVisuals>();
	const float AlongSpeed = static_cast<float>(FVector::VectorPlaneProject(Going, WallNormal).Size());
	CrashFeedback(Hit.ImpactPoint, WallNormal, CrashSpeed, AlongSpeed, Knocked, bSoft);
	// A police telephone box run into hard: its glass bursts and it leans.
	if (APoliceBox* Box = Cast<APoliceBox>(Hit.GetActor()))
	{
		if (CrashSpeed > APoliceBox::SmashSpeed)
		{
			Box->Smash(Hit.ImpactPoint, -WallNormal, true);
		}
	}
	if (Rammed)
	{
		if (UCarVisuals* RammedLooks = Rammed->FindComponentByClass<UCarVisuals>())
		{
			RammedLooks->Knock(Hit.ImpactPoint, -WallNormal, CrashSpeed);
		}
	}
	if (bSoft)
	{
		// A person or a horse: a head-on hit nearly stops the car; a glancing one lets it go on.
		const float Impact = static_cast<float>(FMath::Abs(FVector::DotProduct(GetActorForwardVector(), WallNormal)));
		ForwardSpeed *= 1.f - 0.8f * Impact;
		const float SideImpact = static_cast<float>(FMath::Abs(FVector::DotProduct(GetActorRightVector(), WallNormal)));
		SideSpeed *= 1.f - 0.8f * SideImpact;
	}
	else
	{
		// Another car takes its share of the speed by its weight and is shoved
		// and spun (it slides on by itself); a wall or a tram does not give, so
		// the car bounces off it a little. A hit off the middle spins the car.
		AActor* HitActor = Hit.GetActor();
		ATrafficCar* HitTraffic = Cast<ATrafficCar>(HitActor);
		APestiCar* HitCar = Cast<APestiCar>(HitActor);
		const ATram* HitTram = Cast<ATram>(HitActor);
		FVector OtherGoing = FVector::ZeroVector;
		float OtherMass = 0.f;
		if (HitTraffic)
		{
			OtherGoing = HitTraffic->GetMoveVelocity();
			OtherMass = HitTraffic->GetMass();
		}
		else if (HitCar)
		{
			OtherGoing = HitCar->GetVelocity();
			OtherMass = HitCar->GetMass();
		}
		else if (HitTram)
		{
			OtherGoing = HitTram->GetActorForwardVector().GetSafeNormal2D() * HitTram->GetDrivingSpeed();
		}
		CarHandling::FBump Bump;
		if (!WallNormal.IsNearlyZero() && CarHandling::Collide(WallNormal, Hit.ImpactPoint, GetActorLocation(), Going, Mass,
			HitActor ? HitActor->GetActorLocation() : Hit.ImpactPoint, OtherGoing, OtherMass,
			OtherMass > 0.f ? PestiCar::CarBounce : PestiCar::WallBounce, PestiCar::CrashRub, Bump))
		{
			const FVector After = Going + Bump.PushA;
			ForwardSpeed = static_cast<float>(FVector::DotProduct(After, GetActorForwardVector()));
			SideSpeed = static_cast<float>(FVector::DotProduct(After, GetActorRightVector()));
			SpinRate = FMath::Clamp(SpinRate + Bump.SpinA, -CarHandling::MaxSpin, CarHandling::MaxSpin);
			if (HitTraffic)
			{
				HitTraffic->TakeHit(Bump.PushB, Bump.SpinB);
			}
			else if (HitCar)
			{
				// (Its dent is done above.)
				HitCar->Shove(Bump.PushB, Bump.SpinB, Hit.ImpactPoint, 0.f);
			}
		}
	}
	AddActorWorldOffset(FVector::VectorPlaneProject(Delta * (1.f - Hit.Time), WallNormal), true);
}

void APestiCar::Shove(const FVector& Push, float Spin, const FVector& At, float DentSpeed)
{
	ForwardSpeed += static_cast<float>(FVector::DotProduct(Push, GetActorForwardVector()));
	SideSpeed += static_cast<float>(FVector::DotProduct(Push, GetActorRightVector()));
	ForwardSpeed = FMath::Clamp(ForwardSpeed, -MaxSpeed, MaxSpeed);
	SpinRate = FMath::Clamp(SpinRate + Spin, -CarHandling::MaxSpin, CarHandling::MaxSpin);
	const FVector Into = FVector(Push.X, Push.Y, 0.0).GetSafeNormal();
	if (DentSpeed > 0.f && !Into.IsNearlyZero())
	{
		const int32 Knocked = Visuals->Knock(At, Into, DentSpeed);
		CrashFeedback(At, Into, DentSpeed, 0.f, Knocked, false);
	}
}

void APestiCar::RollDriverless(float DeltaSeconds)
{
	if (FMath::Abs(ForwardSpeed) < PestiCar::DriverlessStill && FMath::Abs(SideSpeed) < PestiCar::DriverlessStill
		&& FMath::Abs(SpinRate) < PestiCar::DriverlessStill)
	{
		ForwardSpeed = 0.f;
		SideSpeed = 0.f;
		SpinRate = 0.f;
		return;
	}
	// Locked wheels: it slides to a stop along and across, and the spin dies away.
	ForwardSpeed = FMath::FInterpConstantTo(ForwardSpeed, 0.f, DeltaSeconds, PestiCar::DriverlessRollDrag);
	SideSpeed = FMath::FInterpConstantTo(SideSpeed, 0.f, DeltaSeconds, PestiCar::DriverlessSkidDrag);
	SpinRate = FMath::FInterpConstantTo(SpinRate * FMath::Exp(-PestiCar::SpinFade * DeltaSeconds), 0.f, DeltaSeconds, PestiCar::SpinDrag);
	const FVector Moving = GetActorForwardVector() * ForwardSpeed + GetActorRightVector() * SideSpeed;
	AddActorWorldRotation(FRotator(0.f, SpinRate * DeltaSeconds, 0.f));
	ForwardSpeed = static_cast<float>(FVector::DotProduct(Moving, GetActorForwardVector()));
	SideSpeed = static_cast<float>(FVector::DotProduct(Moving, GetActorRightVector()));
	MoveWithSlide(Moving * DeltaSeconds);
	SnapToGround();
}

void APestiCar::UpdateSkidAndView(bool bHandbrake, float DeltaSeconds)
{
	// The tyres squeal sliding sideways, or on locked wheels; on wet setts they hiss.
	const float Sideways = FMath::Clamp((FMath::Abs(SideSpeed) - PestiCar::SkidFrom) / (PestiCar::SkidFull - PestiCar::SkidFrom), 0.f, 1.f);
	const float Locked = bHandbrake
		? FMath::Clamp((FMath::Abs(ForwardSpeed) - PestiCar::LockSkidSpeed) / (PestiCar::LockSkidFull - PestiCar::LockSkidSpeed), 0.f, 1.f) * 0.7f
		: 0.f;
	SkidAmount = FMath::FInterpTo(SkidAmount, FMath::Max(Sideways, Locked), DeltaSeconds, 12.f);
	if (SkidAmount > 0.02f && !CrashNoise)
	{
		// Makes the sound so the squeal can be heard.
		PlayCrashSound(static_cast<uint8>(CrashSynth::EKind::Count), 0.f, 0.f);
	}
	if (CrashNoise)
	{
		CrashNoise->SetSkid(SkidAmount > 0.02f ? SkidAmount : 0.f, RoadWet);
	}

	// The chase camera widens a little with speed.
	if (!bOverheadCamera && ChaseCamera)
	{
		const float Share = FMath::Clamp(FMath::Abs(ForwardSpeed) / MaxSpeed, 0.f, 1.f);
		ChaseView = FMath::FInterpTo(ChaseView, FMath::Lerp(PestiCar::ViewSlow, PestiCar::ViewFast, Share), DeltaSeconds, 2.f);
		ChaseCamera->SetFieldOfView(ChaseView);
	}
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
	PestiSettings::AdjustMouse(MouseX, MouseY);
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
