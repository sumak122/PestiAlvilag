// Fill out your copyright notice in the Description page of Project Settings.

#include "PestiPerson.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "CarBody.h"
#include "FightSound.h"
#include "Bicycle.h"
#include "HorseCart.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiCar.h"
#include "ProceduralMeshComponent.h"
#include "PoliceCar.h"
#include "TrafficCar.h"
#include "TrafficManager.h"
#include "PestiHud.h"
#include "PestiMenu.h"

namespace PestiPerson
{
	constexpr float CapsuleRadius = 35.f;
	constexpr float CapsuleHalfHeight = 92.f;

	constexpr float ArmLength = 380.f;
	constexpr float MinPitch = -70.f;
	constexpr float MaxPitch = 40.f;

	// Limbs swing this far at a run (degrees), and one stride is this many cm.
	constexpr float MaxSwing = 35.f;
	constexpr float StrideLength = 120.f;
	// Holding something in front: how far the arms reach forward, standing
	// and sitting (degrees).
	constexpr float HoldPitch = 50.f;
	constexpr float SeatHoldPitch = 40.f;
	// Elbows: a little bent standing, more the faster he goes, as a runner pumps his arms (degrees).
	constexpr float RestElbow = 10.f;
	constexpr float RunElbow = 75.f;

	constexpr double MinTimeOnFoot = 0.3;
	// How close he must stand to a bicycle's saddle to get on (cm).
	constexpr double BikeReach = 200.0;
	// A locked car: F let go within this long (s) smashes the window; held, it
	// picks the lock in this long more. Walking off this far further (cm), or
	// moving faster than this (cm/s) while picking, gives up.
	constexpr double SmashTapTime = 0.3;
	constexpr double PickTime = 6.0;
	constexpr double BreakInSlack = 120.0;
	constexpr double PickStillSpeed = 60.0;

	// Being hit: slower than this does no harm, and each cm/s over it
	// takes this much health (a car at 36 km/h takes about half). One bump
	// hurts once, and knocks him this hard at most (cm/s).
	constexpr float HarmlessSpeed = 250.f;
	constexpr float HurtPerSpeed = 0.06f;
	constexpr double HitCooldown = 0.8;
	constexpr float MaxKnock = 900.f;
	constexpr float KnockUp = 350.f;
	// Landing faster than this hurts, more the harder he lands (cm/s).
	constexpr float SafeFallSpeed = 900.f;
	constexpr float HurtPerFallSpeed = 0.1f;

	// The camera's pivot above his middle, on foot (cm).
	constexpr float PivotHeight = 60.f;
	// Riding a tram: the camera just over his head and a little behind,
	// kept under the roof, looking along the car; C moves it out to see
	// the whole tram (cm, degrees).
	constexpr float SeatArmLength = 130.f;
	constexpr float SeatPivotHeight = 90.f;
	constexpr float SeatMinPitch = -12.f;
	constexpr float RideArmLength = 1100.f;
	constexpr float RidePivotHeight = 150.f;

	// Vaulting and sliding: no slower than this (cm/s), each move this long at least (s).
	constexpr float FreeRunMinSpeed = 200.f;
	constexpr float MinVaultSeconds = 0.45f;
	constexpr float MinSlideSeconds = 0.55f;
	constexpr float MaxMoveSeconds = 1.1f;
	// Running faster than the jog by this much counts as running at it (cm/s).
	constexpr float RunningMargin = 50.f;
	// A late move goes this much slower than he came.
	constexpr float LatePace = 0.8f;

	// Fighting: the player takes this share of a blow's harm out of his
	// health; thrown into something whose face is steeper than this, it is a
	// slam; facing a blow this squarely (cosine) a guard stops it.
	constexpr float PlayerHurtShare = 0.7f;
	constexpr double SlamNormalZ = 0.6;
	constexpr double GuardFacing = 0.3;
	// A counter reaches this much further than a blow (cm); holding the
	// punch key this long makes a heavy swing (s).
	constexpr double CounterSlack = 40.0;
	constexpr double HeavyHold = 0.2;
	// Thrown off, he looks this far ahead of his capsule for a wall, after
	// this long in the air (cm, s).
	constexpr double SlamLook = 25.0;
	constexpr float SlamLookAfter = 0.05f;
	// Both slide on a little after a tackle (cm/s).
	constexpr float TackleSlide = 250.f;
	// Fight sounds: heard this far (cm), and this loud.
	constexpr float FightSoundFalloff = 3000.f;
	constexpr float FightSoundVolume = 1.f;
	// Short of breath below this much stamina, panting at most this hard
	// (winded, he pants as hard as he can).
	constexpr float PantBelow = 30.f;
	constexpr float LowPant = 0.6f;
}

APestiPerson::APestiPerson()
{
	PrimaryActorTick.bCanEverTick = true;

	BaseMaterial = CarBody::LoadPaintMaterial();

	GetCapsuleComponent()->InitCapsuleSize(PestiPerson::CapsuleRadius, PestiPerson::CapsuleHalfHeight);

	PoseRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PoseRoot"));
	PoseRoot->SetupAttachment(GetCapsuleComponent());

	// The body turns to face where he walks; the camera follows the mouse.
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, 600.f, 0.f);
	Movement->MaxWalkSpeed = JogSpeed;
	Movement->JumpZVelocity = 380.f;
	Movement->AirControl = 0.2f;

	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	CameraArm->SetupAttachment(GetCapsuleComponent());
	CameraArm->TargetArmLength = PestiPerson::ArmLength;
	CameraArm->SocketOffset = FVector(0.f, 0.f, PestiPerson::PivotHeight);
	CameraArm->bUsePawnControlRotation = true;
	CameraArm->bEnableCameraLag = true;
	CameraArm->CameraLagSpeed = 12.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void APestiPerson::BeginPlay()
{
	Super::BeginPlay();

	PersonModel::Dress(this, PoseRoot, BaseMaterial, GetDress(), Pieces);
}

void APestiPerson::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// A landed blow's blink of stillness ends (it is counted in real time).
	if (HitStopUntil >= 0.0 && GetWorld()->GetRealTimeSeconds() >= HitStopUntil)
	{
		HitStopUntil = -1.0;
		CustomTimeDilation = 1.f;
	}

	if (FreeMove != FreeRun::EMove::None)
	{
		TickMove(DeltaSeconds);
	}
	else if (Act != Brawl::EAct::None)
	{
		TickBrawl(DeltaSeconds);
	}
	else
	{
		SwingLimbs(DeltaSeconds);
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	if (!bCameraAimed)
	{
		// Start looking the way he faces, a little downwards (the game mode
		// aims the view at the level's PlayerStart, which may be anywhere).
		PC->SetControlRotation(FRotator(-10.f, GetActorRotation().Yaw, 0.f));
		bCameraAimed = true;
	}
	if (bRiding)
	{
		if (PC->WasInputKeyJustPressed(EKeys::C) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Top))
		{
			SetRideView(!bRideOutside, PC);
		}
		Look(PC, DeltaSeconds);
		return;
	}
	Look(PC, DeltaSeconds);
	if (FreeMove != FreeRun::EMove::None)
	{
		return;
	}
	// Held still (a scene, a menu, fainting): no jumping, vaulting or getting into cars either.
	if (Act == Brawl::EAct::None && PC->IsMoveInputIgnored())
	{
		return;
	}
	if (Act != Brawl::EAct::None)
	{
		PlayerFight(PC);
		return;
	}
	Walk(PC);
	if (GetCharacterMovement()->IsFalling())
	{
		FallSpeed = FMath::Max(FallSpeed, static_cast<float>(-GetVelocity().Z));
	}

	if (bCarrying)
	{
		if (GEngine)
		{
			APestiHUD::Show(this, 1, 0.f, FColor::White, TEXT("On foot, carrying something heavy"));
		}
		return;
	}

	// Something ahead to vault or slide under: say so, and Space does it.
	FreeRun::FChance Chance;
	const bool bChance = !bHoldingInFront && GetCharacterMovement()->IsMovingOnGround()
		&& GetVelocity().Size2D() >= PestiPerson::FreeRunMinSpeed
		&& FreeRun::FindMove(GetWorld(), this, GetActorLocation(), GetVelocity(),
			GetCapsuleComponent()->GetScaledCapsuleRadius(), GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), Chance);
	if (bChance)
	{
		if (APestiHUD* Hud = APestiHUD::Get(this))
		{
			const bool bVault = Chance.Move == FreeRun::EMove::Vault;
			Hud->Prompt(TEXT("SPACE"), bVault ? TEXT("\u00c1tugr\u00e1s") : TEXT("Al\u00e1cs\u00fasz\u00e1s"), bVault ? TEXT("Vault") : TEXT("Slide"),
				Chance.Face >= FreeRun::GoodFrom && Chance.Face <= FreeRun::GoodTo);
		}
	}
	if (PC->WasInputKeyJustPressed(EKeys::SpaceBar) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom))
	{
		if (bChance)
		{
			StartMove(Chance);
			return;
		}
		Jump();
	}
	if (TickBreakIn(PC))
	{
		return;
	}
	if (PC->WasInputKeyJustPressed(EKeys::F) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right))
	{
		TryEnterCar(PC);
		return;
	}

	if (GEngine)
	{
		APestiHUD::Show(this, 1, 0.f, FColor::White,
			TEXT("On foot   (F: get in or steal a car, or climb onto a cart, Shift: sprint, Space: jump, or vault and slide running at things, N: traffic view, P: police test, F9: new game)"));
	}
}

void APestiPerson::Look(APlayerController* PC, float DeltaSeconds)
{
	// Not while a menu is up.
	if (PC->IsLookInputIgnored())
	{
		return;
	}
	float MouseX = 0.f;
	float MouseY = 0.f;
	PC->GetInputMouseDelta(MouseX, MouseY);
	PestiSettings::AdjustMouse(MouseX, MouseY);
	const float TurnX = MouseX * MouseLookScale + PC->GetInputAnalogKeyState(EKeys::Gamepad_RightX) * StickLookRate * DeltaSeconds;
	const float TurnY = MouseY * MouseLookScale + PC->GetInputAnalogKeyState(EKeys::Gamepad_RightY) * StickLookRate * DeltaSeconds;
	if (FMath::IsNearlyZero(TurnX) && FMath::IsNearlyZero(TurnY))
	{
		return;
	}
	const FRotator View = PC->GetControlRotation();
	const float LowestPitch = bRiding && !bRideOutside ? PestiPerson::SeatMinPitch : PestiPerson::MinPitch;
	const float Pitch = FMath::Clamp(FRotator::NormalizeAxis(View.Pitch + TurnY), LowestPitch, PestiPerson::MaxPitch);
	PC->SetControlRotation(FRotator(Pitch, View.Yaw + TurnX, 0.f));
}

void APestiPerson::Walk(const APlayerController* PC)
{
	auto KeyAxis = [PC](const FKey& Positive, const FKey& Negative)
	{
		return (PC->IsInputKeyDown(Positive) ? 1.f : 0.f) - (PC->IsInputKeyDown(Negative) ? 1.f : 0.f);
	};
	const float Forward = FMath::Clamp(KeyAxis(EKeys::W, EKeys::S) + KeyAxis(EKeys::Up, EKeys::Down)
		+ PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY), -1.f, 1.f);
	const float Right = FMath::Clamp(KeyAxis(EKeys::D, EKeys::A) + KeyAxis(EKeys::Right, EKeys::Left)
		+ PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX), -1.f, 1.f);

	// Walk relative to where the camera looks.
	const FRotator Heading(0.f, PC->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Heading).GetUnitAxis(EAxis::X), Forward);
	AddMovementInput(FRotationMatrix(Heading).GetUnitAxis(EAxis::Y), Right);

	const bool bRun = PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::Gamepad_LeftThumbstick);
	// A clean vault or slide carries him on a little faster; a stumble slows him.
	const double Now = GetWorld()->GetTimeSeconds();
	const float Run = Now < BoostUntil ? RunSpeed * FreeRun::CleanBoost : RunSpeed;
	GetCharacterMovement()->MaxWalkSpeed = bCarrying ? CarrySpeed
		: Now < StumbleUntil ? JogSpeed * FreeRun::StumbleSlow
		: bRun ? Run : JogSpeed;
}

void APestiPerson::SwingLimbs(float DeltaSeconds)
{
	// No animation rig yet: swing the arms and legs from the hips and
	// shoulders, legs against arms, faster and wider the faster he goes.
	const float Speed = GetCharacterMovement()->IsMovingOnGround() ? static_cast<float>(GetVelocity().Size2D()) : 0.f;
	StridePhase += DeltaSeconds * Speed / PestiPerson::StrideLength * PI;
	const float Reach = PestiPerson::MaxSwing * FMath::Clamp(Speed / RunSpeed, 0.f, 1.f);
	const float Swing = FMath::Sin(StridePhase) * Reach;
	// The knees bend with the stride, more when running.
	PersonModel::StrideLegs(Pieces, StridePhase, Reach, FMath::Clamp(Speed / JogSpeed, 0.f, 1.7f));
	// The arm with a load in it hangs still; both arms holding something in front reach forward.
	const float Hold = bRiding ? PestiPerson::SeatHoldPitch : PestiPerson::HoldPitch;
	const float LeftArm = bHoldingInFront ? Hold : -Swing;
	const float RightArm = bHoldingInFront ? Hold : bCarrying ? 0.f : Swing;
	const float Arms[] = { LeftArm, RightArm };
	for (int32 i = 0; i < 2; ++i)
	{
		const int32 Arm = PersonKit::ArmLeft + i;
		if (Pieces.IsValidIndex(Arm) && Pieces[Arm])
		{
			Pieces[Arm]->SetRelativeRotation(FRotator(Arms[i], 0.f, 0.f));
		}
	}
	const float Elbow = FMath::Lerp(PestiPerson::RestElbow, PestiPerson::RunElbow, FMath::Clamp(Speed / RunSpeed, 0.f, 1.f));
	PersonModel::PoseElbows(Pieces, bHoldingInFront ? 0.f : Elbow, bHoldingInFront || bCarrying ? 0.f : Elbow);

	// Just after a bad landing: a lurch over the stride.
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now < StumbleUntil)
	{
		const float T = 1.f - static_cast<float>((StumbleUntil - Now) / FreeRun::StumbleSeconds);
		ApplyPose(FreeRun::Stumble(T));
	}
	else if (PoseRoot && !PoseRoot->GetRelativeTransform().Equals(FTransform::Identity))
	{
		PoseRoot->SetRelativeTransform(FTransform::Identity);
	}
}

void APestiPerson::StartMove(const FreeRun::FChance& Chance)
{
	const float Speed = static_cast<float>(GetVelocity().Size2D());
	bMoveClean = Chance.Face >= FreeRun::GoodFrom && Chance.Face <= FreeRun::GoodTo && Speed > JogSpeed + PestiPerson::RunningMargin;
	bMoveLate = Chance.Face < FreeRun::Late;
	FreeMove = Chance.Move;
	MoveChance = Chance;
	MoveFrom = GetActorLocation();
	MoveDir = (Chance.To - MoveFrom).GetSafeNormal2D();
	MoveClock = 0.f;
	const float Pace = bMoveClean ? RunSpeed * FreeRun::CleanBoost
		: FMath::Max(Speed, JogSpeed) * (bMoveLate ? PestiPerson::LatePace : 1.f);
	const float Shortest = Chance.Move == FreeRun::EMove::Vault ? PestiPerson::MinVaultSeconds : PestiPerson::MinSlideSeconds;
	MoveLength = FMath::Clamp(static_cast<float>(FVector::Dist2D(MoveFrom, Chance.To)) / Pace, Shortest, PestiPerson::MaxMoveSeconds);
	SetActorRotation(FRotator(0.0, MoveDir.Rotation().Yaw, 0.0));
	// He goes where the move takes him, through the thing he passes over or under.
	GetCharacterMovement()->DisableMovement();
}

void APestiPerson::TickMove(float DeltaSeconds)
{
	MoveClock += DeltaSeconds;
	const float T = FMath::Clamp(MoveClock / FMath::Max(MoveLength, 0.01f), 0.f, 1.f);
	const FVector To = MoveChance.To;
	const FVector Flat = FMath::Lerp(MoveFrom, To, static_cast<double>(T));
	const bool bVault = FreeMove == FreeRun::EMove::Vault;
	const double Z = bVault ? FreeRun::ArcZ(MoveFrom.Z, To.Z, MoveChance.PeakZ, MoveChance.Over, T) : Flat.Z;
	SetActorLocation(FVector(Flat.X, Flat.Y, Z), false, nullptr, ETeleportType::TeleportPhysics);
	// Whoever watches his speed (people stepping aside) sees him going.
	GetCharacterMovement()->Velocity = MoveDir * (FVector::Dist2D(MoveFrom, To) / FMath::Max(MoveLength, 0.01f));
	ApplyPose(bVault ? FreeRun::Vault(T) : FreeRun::Slide(T));
	if (T >= 1.f)
	{
		FinishMove();
	}
}

void APestiPerson::FinishMove()
{
	const double Now = GetWorld()->GetTimeSeconds();
	const float Pace = static_cast<float>(GetCharacterMovement()->Velocity.Size2D());
	FreeMove = FreeRun::EMove::None;
	PoseRoot->SetRelativeTransform(FTransform::Identity);
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->SetMovementMode(MOVE_Walking);
	float Out = Pace;
	if (bMoveClean)
	{
		BoostUntil = Now + FreeRun::BoostSeconds;
		Out = RunSpeed * FreeRun::CleanBoost;
	}
	else if (bMoveLate)
	{
		StumbleUntil = Now + FreeRun::StumbleSeconds;
		Out = JogSpeed * FreeRun::StumbleSlow;
	}
	Movement->Velocity = MoveDir * Out;
}

void APestiPerson::ApplyPose(const FreeRun::FPose& Pose, float LegTwist)
{
	if (PoseRoot)
	{
		PoseRoot->SetRelativeLocationAndRotation(FVector(0.0, 0.0, Pose.Lift), FRotator(Pose.Pitch, Pose.Yaw, Pose.Roll));
	}
	PersonModel::PoseLegs(Pieces, Pose.LegL, Pose.LegR, Pose.KneeL, Pose.KneeR, LegTwist);
	// Rolling an arm about the forward axis takes it out to its side: the left one one way, the right the other.
	const FRotator Arms[] = { FRotator(Pose.ArmL, 0.f, Pose.ArmOutL), FRotator(Pose.ArmR, 0.f, -Pose.ArmOutR) };
	for (int32 i = 0; i < 2; ++i)
	{
		const int32 Arm = PersonKit::ArmLeft + i;
		if (Pieces.IsValidIndex(Arm) && Pieces[Arm])
		{
			Pieces[Arm]->SetRelativeRotation(Arms[i]);
		}
	}
	PersonModel::PoseElbows(Pieces, Pose.ElbowL, Pose.ElbowR);
}

void APestiPerson::TryEnterCar(APlayerController* PC)
{
	// The F that got him out of a car may still read as pressed this frame.
	if (GetWorld()->GetTimeSeconds() - OutOfCarTime < PestiPerson::MinTimeOnFoot)
	{
		return;
	}
	// A horse and cart: up onto the bench.
	for (TActorIterator<AHorseCart> It(GetWorld()); It; ++It)
	{
		if (!It->GetController() && FVector::Dist2D(It->GetBenchLocation(), GetActorLocation()) < EnterCarRange)
		{
			SetInWorld(false);
			It->TakeDriver(this);
			PC->Possess(*It);
			return;
		}
	}
	// A bicycle: onto the saddle (he has to stand right by it).
	ABicycle* NearestBike = nullptr;
	double BikeDist = FMath::Min(static_cast<double>(EnterCarRange), PestiPerson::BikeReach);
	for (TActorIterator<ABicycle> It(GetWorld()); It; ++It)
	{
		const double Dist = FVector::Dist2D(It->GetSaddleLocation(), GetActorLocation());
		if (!It->GetController() && Dist < BikeDist)
		{
			NearestBike = *It;
			BikeDist = Dist;
		}
	}
	if (NearestBike)
	{
		SetInWorld(false);
		NearestBike->TakeRider(this);
		PC->Possess(NearestBike);
		return;
	}
	APestiCar* Nearest = nullptr;
	double NearestDist = EnterCarRange;
	for (TActorIterator<APestiCar> It(GetWorld()); It; ++It)
	{
		const double Dist = FVector::Dist2D(It->GetActorLocation(), GetActorLocation());
		if (!It->GetController() && Dist < NearestDist)
		{
			Nearest = *It;
			NearestDist = Dist;
		}
	}
	// A car from the traffic that is nearer still: pull the driver out and take it.
	ATrafficCar* Victim = nullptr;
	for (TActorIterator<ATrafficCar> It(GetWorld()); It; ++It)
	{
		const double Dist = FVector::Dist2D(It->GetActorLocation(), GetActorLocation());
		if (!Cast<APoliceCar>(*It) && Dist < NearestDist)
		{
			Victim = *It;
			NearestDist = Dist;
		}
	}
	if (Victim)
	{
		for (TActorIterator<ATrafficManager> It(GetWorld()); It; ++It)
		{
			Nearest = It->StealCar(Victim);
			break;
		}
	}
	if (!Nearest)
	{
		return;
	}
	// Locked: F held picks the lock, a quick tap smashes the window (TickBreakIn).
	if (Nearest->IsLocked())
	{
		LockedCar = Nearest;
		BreakInStart = GetWorld()->GetTimeSeconds();
		return;
	}
	GetIntoCar(PC, Nearest);
}

void APestiPerson::GetIntoCar(APlayerController* PC, APestiCar* Car)
{
	SetInWorld(false);
	Car->TakeDriver(this);
	PC->Possess(Car);
}

bool APestiPerson::TickBreakIn(APlayerController* PC)
{
	APestiHUD* Hud = APestiHUD::Get(this);
	APestiCar* Car = LockedCar.Get();
	if (!Car)
	{
		// Standing by a locked car: say how to get in.
		if (Hud)
		{
			for (TActorIterator<APestiCar> It(GetWorld()); It; ++It)
			{
				if (It->IsLocked() && !It->GetController() && FVector::Dist2D(It->GetActorLocation(), GetActorLocation()) < EnterCarRange)
				{
					Hud->Prompt(TEXT("F"), TEXT("Koppints: ablakbet\u00f6r\u00e9s \u00b7 tartsd: z\u00e1rfeszeget\u00e9s"),
						TEXT("Tap: smash the window \u00b7 hold: pick the lock"), false);
					break;
				}
			}
		}
		return false;
	}
	const double Held = GetWorld()->GetTimeSeconds() - BreakInStart;
	const bool bDown = PC->IsInputKeyDown(EKeys::F) || PC->IsInputKeyDown(EKeys::Gamepad_FaceButton_Right);
	const bool bPicking = Held > PestiPerson::SmashTapTime;
	if (!Car->IsLocked() || Car->GetController()
		|| FVector::Dist2D(Car->GetActorLocation(), GetActorLocation()) > EnterCarRange + PestiPerson::BreakInSlack
		|| (bPicking && GetVelocity().Size2D() > PestiPerson::PickStillSpeed))
	{
		LockedCar = nullptr;
		return false;
	}
	if (!bDown)
	{
		LockedCar = nullptr;
		if (bPicking)
		{
			// Let go while picking: he gives up.
			return false;
		}
		Car->BreakIn(true);
		GetIntoCar(PC, Car);
		return true;
	}
	if (!bPicking)
	{
		return true;
	}
	const double Share = FMath::Clamp((Held - PestiPerson::SmashTapTime) / PestiPerson::PickTime, 0.0, 1.0);
	if (Share >= 1.0)
	{
		LockedCar = nullptr;
		Car->BreakIn(false);
		GetIntoCar(PC, Car);
		return true;
	}
	if (Hud)
	{
		const int32 Percent = FMath::RoundToInt(Share * 100.0);
		Hud->Prompt(TEXT("F"), FString::Printf(TEXT("Z\u00e1rfeszeget\u00e9s... %d%%"), Percent),
			FString::Printf(TEXT("Picking the lock... %d%%"), Percent), true);
	}
	return true;
}

bool APestiPerson::TryStepOutAt(const FVector& At, double Yaw)
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PestiPersonStepOut), false, this);
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, At + FVector(0.f, 0.f, 200.f), At - FVector(0.f, 0.f, 1000.f), ECC_Visibility, Params))
	{
		return false;
	}
	const FVector Location = Hit.ImpactPoint + FVector(0.f, 0.f, PestiPerson::CapsuleHalfHeight + 5.f);
	const FCollisionShape Capsule = FCollisionShape::MakeCapsule(PestiPerson::CapsuleRadius, PestiPerson::CapsuleHalfHeight);
	if (GetWorld()->OverlapAnyTestByChannel(Location, FQuat::Identity, ECC_Pawn, Capsule, Params))
	{
		return false;
	}

	SetActorLocationAndRotation(Location, FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	SetInWorld(true);
	return true;
}

void APestiPerson::HitByVehicle(float Speed, const FVector& Push)
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (bRiding || IsHidden() || Speed < PestiPerson::HarmlessSpeed || Now - LastHitTime < PestiPerson::HitCooldown)
	{
		return;
	}
	LastHitTime = Now;
	if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
	{
		Game->HurtPlayer((Speed - PestiPerson::HarmlessSpeed) * PestiPerson::HurtPerSpeed);
	}
	const FVector Knock = Push.GetSafeNormal2D() * FMath::Min(Speed, PestiPerson::MaxKnock) + FVector(0.0, 0.0, PestiPerson::KnockUp);
	LaunchCharacter(Knock, true, true);
}

void APestiPerson::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	if (FallSpeed > PestiPerson::SafeFallSpeed)
	{
		if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
		{
			Game->HurtPlayer((FallSpeed - PestiPerson::SafeFallSpeed) * PestiPerson::HurtPerFallSpeed);
		}
	}
	FallSpeed = 0.f;
}

void APestiPerson::SetInWorld(bool bInWorld)
{
	SetActorHiddenInGame(!bInWorld);
	SetActorEnableCollision(bInWorld);
	SetActorTickEnabled(bInWorld);
	if (bInWorld)
	{
		OutOfCarTime = GetWorld()->GetTimeSeconds();
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	else
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}
}

void APestiPerson::RideOn(AActor* Vehicle, const FVector& SeatAt, double SeatYaw)
{
	bRiding = true;
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	SetActorEnableCollision(false);
	PersonKit::FOutfit Seated = GetDress();
	Seated.bSeated = true;
	PersonModel::Dress(this, PoseRoot, BaseMaterial, Seated, Pieces);
	RideSeat = FTransform(FRotator(0.0, SeatYaw, 0.0), SeatAt);
	AttachToActor(Vehicle, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SetActorRelativeTransform(RideSeat);
	// The camera rides with him, without lagging behind into the woodwork,
	// and without being pushed in by the tram it sits in.
	CameraArm->bEnableCameraLag = false;
	CameraArm->bDoCollisionTest = false;
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		// Look along the car, the way it runs.
		PC->SetControlRotation(FRotator(-5.f, Vehicle->GetActorRotation().Yaw, 0.f));
	}
	SetRideView(false, PC);
}

void APestiPerson::SetRideView(bool bOutside, APlayerController* PC)
{
	bRideOutside = bOutside;
	CameraArm->TargetArmLength = bOutside ? PestiPerson::RideArmLength : PestiPerson::SeatArmLength;
	CameraArm->SocketOffset = FVector(0.f, 0.f, bOutside ? PestiPerson::RidePivotHeight : PestiPerson::SeatPivotHeight);
	if (PC && !bOutside)
	{
		const FRotator View = PC->GetControlRotation();
		const float Pitch = FMath::Clamp(FRotator::NormalizeAxis(View.Pitch), PestiPerson::SeatMinPitch, PestiPerson::MaxPitch);
		PC->SetControlRotation(FRotator(Pitch, View.Yaw, 0.f));
	}
}

bool APestiPerson::GetOffAt(const FVector& At, double Yaw)
{
	AActor* Vehicle = GetAttachParentActor();
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	// Where he wants, or a step to either side if someone stands there.
	const FVector Along = FRotator(0.0, Yaw, 0.0).Vector();
	for (const double Step : { 0.0, 100.0, -100.0, 200.0, -200.0 })
	{
		if (TryStepOutAt(At + Along * Step, Yaw))
		{
			bRiding = false;
			bRideOutside = false;
			CameraArm->TargetArmLength = PestiPerson::ArmLength;
			CameraArm->bEnableCameraLag = true;
			CameraArm->bDoCollisionTest = true;
			CameraArm->SocketOffset = FVector(0.f, 0.f, PestiPerson::PivotHeight);
			PersonModel::Dress(this, PoseRoot, BaseMaterial, GetDress(), Pieces);
			return true;
		}
	}
	if (Vehicle)
	{
		AttachToActor(Vehicle, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		SetActorRelativeTransform(RideSeat);
	}
	return false;
}

void APestiPerson::SetCarrying(bool bInCarrying)
{
	bCarrying = bInCarrying;
}

PersonKit::FOutfit APestiPerson::GetDress() const
{
	PersonKit::FOutfit Outfit = PersonModel::PlayerOutfit();
	// Schwimmer's bamboo baton in his right hand, once he has it.
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (Game && Game->HasBaton())
	{
		Outfit.Held = PersonKit::EHeld::Baton;
		Outfit.HeldColor = PersonKit::Bamboo();
	}
	return Outfit;
}

void APestiPerson::RefreshDress()
{
	if (!bRiding)
	{
		PersonModel::Dress(this, PoseRoot, BaseMaterial, GetDress(), Pieces);
	}
}

float APestiPerson::GetBlowStrength() const
{
	if (!IsPlayerControlled())
	{
		return 1.f;
	}
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	return Game && Game->HasBaton() ? Brawl::BatonStrength : 1.f;
}

void APestiPerson::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved,
	FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	// Thrown off into something standing (a wall, a stall, crates): it hurts, and down he goes.
	if (Act == Brawl::EAct::Shoved && !bBlowDone && Other != ThrownBy.Get() && FMath::Abs(HitNormal.Z) < PestiPerson::SlamNormalZ)
	{
		bBlowDone = true;
		GetCharacterMovement()->StopMovementImmediately();
		TakeBlow(bThrownHard ? Brawl::EBlow::WallSlam : Brawl::EBlow::Slam, ThrownBy.Get());
	}
}

void APestiPerson::LoseHealth(float Amount)
{
	if (!IsPlayerControlled())
	{
		return;
	}
	if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
	{
		// Kemény áll (Bandi's lesson): blows take less out of him.
		const float Chin = KnowsLesson(Brawl::ELesson::Chin) ? Brawl::ChinShare : 1.f;
		Game->HurtPlayer(Amount * PestiPerson::PlayerHurtShare * Chin);
	}
}

void APestiPerson::StartFight(APestiPerson* InFoe)
{
	Foe = InFoe;
	if (Act == Brawl::EAct::None)
	{
		FightMotion.Reset();
		FightMotion.SetSeed(FMath::FRandRange(0.f, 6.28f));
		BeginAct(Brawl::EAct::Stance, 0.f);
	}
	Stamina = Brawl::MaxStamina;
	// He faces the other man, whichever way he steps.
	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void APestiPerson::EndFight()
{
	if (FightNoise)
	{
		FightNoise->SetBreathless(0.f);
	}
	LetGo();
	Act = Brawl::EAct::None;
	Foe = nullptr;
	HitStopUntil = -1.0;
	CustomTimeDilation = 1.f;
	ChainStep = 0;
	KeptPunch = 0;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	if (Movement->MovementMode == MOVE_None)
	{
		Movement->SetMovementMode(MOVE_Walking);
	}
	Movement->MaxWalkSpeed = JogSpeed;
	PoseRoot->SetRelativeTransform(FTransform::Identity);
}

void APestiPerson::BeginAct(Brawl::EAct NewAct, float Seconds)
{
	// A punch may still come out of a sidestep just ended (SlipPunch).
	if (Act == Brawl::EAct::Sidestep && NewAct != Brawl::EAct::Sidestep)
	{
		DodgeEndedAt = GetWorld()->GetTimeSeconds();
	}
	Act = NewAct;
	ActClock = 0.f;
	ActLength = Seconds;
	bBlowDone = false;
	FightMotion.Begin(NewAct);

	// What he sounds like doing it (the blows landing sound in TakeBlow).
	using FightSynth::EKind;
	switch (NewAct)
	{
	case Brawl::EAct::Jab:
		PlayFightSound(EKind::Swish, 0.55f, 0.04f);
		break;
	case Brawl::EAct::Cross:
	case Brawl::EAct::Hook:
		PlayFightSound(EKind::Swish, 0.65f, 0.05f);
		break;
	case Brawl::EAct::Uppercut:
		PlayFightSound(EKind::Swish, 0.8f, 0.1f);
		PlayFightSound(EKind::Effort, 0.5f, 0.08f);
		break;
	case Brawl::EAct::Rising:
		PlayFightSound(EKind::Swish, 1.f, 0.16f);
		PlayFightSound(EKind::Effort, 1.f, 0.12f);
		break;
	case Brawl::EAct::Haymaker:
		PlayFightSound(EKind::Swish, 1.f, 0.24f);
		PlayFightSound(EKind::Effort, 1.f, 0.2f);
		break;
	case Brawl::EAct::HeavySwing:
		PlayFightSound(EKind::Swish, 1.f);
		PlayFightSound(EKind::Effort, 0.9f);
		break;
	case Brawl::EAct::Counter:
		PlayFightSound(EKind::Swish, 0.8f, 0.12f);
		PlayFightSound(EKind::Effort, 0.6f, 0.1f);
		break;
	case Brawl::EAct::GrabReach:
		PlayFightSound(EKind::Grab, 0.6f, Seconds * 0.85f);
		break;
	case Brawl::EAct::Knee:
		PlayFightSound(EKind::Effort, 0.6f);
		break;
	case Brawl::EAct::Shove:
		PlayFightSound(EKind::Effort, 1.f);
		break;
	case Brawl::EAct::BodyBlow:
		PlayFightSound(EKind::Swish, 0.7f, 0.12f);
		PlayFightSound(EKind::Effort, 0.6f, 0.1f);
		break;
	case Brawl::EAct::SlipCounter:
		PlayFightSound(EKind::Swish, 0.8f, 0.08f);
		break;
	case Brawl::EAct::Headbutt:
		PlayFightSound(EKind::Effort, 0.9f, 0.15f);
		break;
	case Brawl::EAct::WallThrow:
		PlayFightSound(EKind::Effort, 1.f, 0.3f);
		break;
	case Brawl::EAct::Tackle:
		PlayFightSound(EKind::Effort, 1.f);
		PlayFightSound(EKind::Thud, 0.8f, 0.35f);
		break;
	case Brawl::EAct::Tackled:
		PlayFightSound(EKind::Pain, 0.6f, 0.15f);
		PlayFightSound(EKind::Thud, 1.f, 0.3f);
		break;
	case Brawl::EAct::Down:
		PlayFightSound(EKind::Thud, 1.f, Seconds * Brawl::FallShare);
		break;
	case Brawl::EAct::GetUp:
		PlayFightSound(EKind::Effort, 0.35f);
		break;
	case Brawl::EAct::Winded:
		PlayFightSound(EKind::Pain, 0.35f);
		break;
	case Brawl::EAct::Beaten:
		PlayFightSound(EKind::Pain, 0.5f);
		break;
	default:
		break;
	}
}

void APestiPerson::PlayFightSound(FightSynth::EKind Kind, float Strength, float Delay)
{
	if (!FightNoise)
	{
		FightNoise = NewObject<UFightSound>(this);
		FightNoise->SetupAttachment(GetRootComponent());
		FightNoise->bAllowSpatialization = true;
		FightNoise->bOverrideAttenuation = true;
		FightNoise->AttenuationOverrides.bAttenuate = true;
		FightNoise->AttenuationOverrides.bSpatialize = true;
		FightNoise->AttenuationOverrides.FalloffDistance = PestiPerson::FightSoundFalloff;
		FightNoise->RegisterComponent();
		AddInstanceComponent(FightNoise);
		FightNoise->SetVolumeMultiplier(PestiPerson::FightSoundVolume);
		FightNoise->SetPitch(GetVoicePitch());
	}
	if (!FightNoise->IsPlaying())
	{
		FightNoise->Start();
	}
	FightNoise->Queue(Kind, Strength, Delay);
}

float APestiPerson::GetWindUpLeft() const
{
	switch (Act)
	{
	case Brawl::EAct::HeavyWind:
		return ActLength - ActClock + Brawl::HeavySwingSeconds * Brawl::HeavyLands;
	case Brawl::EAct::HeavySwing:
		return bBlowDone ? -1.f : Brawl::HeavySwingSeconds * Brawl::HeavyLands - ActClock;
	case Brawl::EAct::GrabReach:
		return ActLength - ActClock;
	default:
		return -1.f;
	}
}

bool APestiPerson::SpendStamina(float Cost)
{
	if (Stamina <= 0.f)
	{
		return false;
	}
	// Kitartás (Bandi's lesson): every move costs him less breath.
	Stamina -= KnowsLesson(Brawl::ELesson::Breath) ? Cost * Brawl::BreathShare : Cost;
	RestFrom = GetWorld()->GetTimeSeconds() + Brawl::RestBefore;
	return true;
}

bool APestiPerson::Jab()
{
	if (!Brawl::IsFree(Act) || !SpendStamina(Brawl::JabCost))
	{
		return false;
	}
	BeginAct(Brawl::EAct::Jab, Brawl::JabSeconds);
	return true;
}

bool APestiPerson::Dodge(const FVector& Way)
{
	using namespace Brawl;
	// From the fists up, or straight out of a quick punch once it has landed.
	const bool bOutOfPunch = IsLightPunch(Act) && ActLength > 0.f && ActClock >= ActLength * ChainFrom;
	if ((!IsFree(Act) && !bOutOfPunch) || !SpendStamina(SidestepCost))
	{
		return false;
	}
	// Aside, to whichever side the keys lean; with no keys or straight at him, back.
	const FVector Flat = Way.GetSafeNormal2D();
	const double Across = FVector::DotProduct(Flat, GetActorRightVector());
	if (Flat.IsNearlyZero() || FMath::Abs(Across) < 0.3)
	{
		DodgeSide = 0.f;
		DodgeWay = -GetActorForwardVector().GetSafeNormal2D() * SidestepBack;
	}
	else
	{
		DodgeSide = Across > 0.0 ? 1.f : -1.f;
		DodgeWay = GetActorRightVector().GetSafeNormal2D() * (DodgeSide * SidestepAside);
	}
	ChainStep = 0;
	BeginAct(EAct::Sidestep, SidestepSeconds);
	PlayFightSound(FightSynth::EKind::Swish, 0.35f);
	return true;
}

bool APestiPerson::IsDodging() const
{
	return Brawl::IsDodging(Act, ActLength > 0.f ? ActClock / ActLength : 0.f);
}

bool APestiPerson::HeavySwing()
{
	if (!Brawl::IsFree(Act) || !SpendStamina(Brawl::HeavyCost))
	{
		return false;
	}
	BeginAct(Brawl::EAct::HeavyWind, Brawl::HeavyWindSeconds);
	return true;
}

bool APestiPerson::ChainPunch(bool bFinisher)
{
	using namespace Brawl;
	const double Now = GetWorld()->GetTimeSeconds();
	// Late in a quick punch (after it landed) the next one may start; late in
	// the uppercut, only the finisher.
	const bool bLate = ActLength > 0.f && ActClock >= ActLength * ChainFrom
		&& (IsLightPunch(Act) || (bFinisher && Act == EAct::Uppercut));
	if (!IsFree(Act) && !bLate)
	{
		return false;
	}
	if (!bLate && Now > ChainUntil)
	{
		ChainStep = 0;
	}
	if (bFinisher && ChainStep == 0)
	{
		return HeavySwing();
	}
	if (!bFinisher && ChainStep >= ChainLength)
	{
		ChainStep = 0;
	}

	EAct Next = EAct::Jab;
	float Length = JabSeconds;
	if (bFinisher)
	{
		Next = ChainStep <= 2 ? EAct::Rising : EAct::Haymaker;
		Length = ChainStep <= 2 ? RisingSeconds : HaymakerSeconds;
	}
	else if (ChainStep == 1)
	{
		Next = EAct::Cross;
		Length = CrossSeconds;
	}
	else if (ChainStep == 2)
	{
		Next = EAct::Hook;
		Length = HookSeconds;
	}
	else if (ChainStep == 3)
	{
		Next = EAct::Uppercut;
		Length = UppercutSeconds;
	}
	if (!SpendStamina(bFinisher ? FinisherCost : ChainCost))
	{
		return false;
	}
	// A finisher ends the chain: the next click starts again from the jab.
	ChainStep = bFinisher ? 0 : ChainStep + 1;
	ChainUntil = bFinisher ? -1.0 : Now + Length + ChainGap;
	BeginAct(Next, Length);
	return true;
}

void APestiPerson::HitStop(float Seconds)
{
	HitStopUntil = GetWorld()->GetRealTimeSeconds() + Seconds;
	CustomTimeDilation = Brawl::HitStopSlow;
}

bool APestiPerson::Counter()
{
	APestiPerson* Other = Foe.Get();
	if (!Brawl::IsFree(Act) || !Other)
	{
		return false;
	}
	const float Left = Other->GetWindUpLeft();
	const double Apart = FVector::Dist2D(Other->GetActorLocation(), GetActorLocation());
	if (Left < 0.f || Left > Brawl::CounterWindow || Apart > Brawl::Reach + PestiPerson::CounterSlack || !SpendStamina(Brawl::CounterCost))
	{
		return false;
	}
	BeginAct(Brawl::EAct::Counter, Brawl::CounterSeconds);
	// His swing or grab never comes: he is caught pulling back.
	Other->LetGo();
	Other->HurtSide = -1.f;
	Other->BeginAct(Brawl::EAct::Hurt, Brawl::CounterSeconds * Brawl::CounterLands + 0.1f);
	return true;
}

bool APestiPerson::Grab(bool bSlowly)
{
	if (!Brawl::IsFree(Act) || !SpendStamina(Brawl::GrabCost))
	{
		return false;
	}
	BeginAct(Brawl::EAct::GrabReach, bSlowly ? Brawl::SlowGrabReachSeconds : Brawl::GrabReachSeconds);
	return true;
}

bool APestiPerson::Knee()
{
	if (Act != Brawl::EAct::Holding || KneesGiven >= Brawl::MaxKnees || !SpendStamina(Brawl::KneeCost))
	{
		return false;
	}
	++KneesGiven;
	BeginAct(Brawl::EAct::Knee, Brawl::KneeSeconds);
	return true;
}

bool APestiPerson::Shove(const FVector& Direction)
{
	if (Act != Brawl::EAct::Holding || !SpendStamina(Brawl::ShoveCost))
	{
		return false;
	}
	const FVector Flat = Direction.GetSafeNormal2D();
	if (!Flat.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.0, Flat.Rotation().Yaw, 0.0));
	}
	BeginAct(Brawl::EAct::Shove, Brawl::ShoveSeconds);
	return true;
}

bool APestiPerson::KnowsLesson(Brawl::ELesson Lesson) const
{
	if (!IsPlayerControlled())
	{
		return false;
	}
	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	return Game && Game->KnowsLesson(static_cast<int32>(Lesson));
}

bool APestiPerson::BellyPunch()
{
	if (!Brawl::IsFree(Act) || !SpendStamina(Brawl::BodyBlowCost))
	{
		return false;
	}
	ChainStep = 0;
	BeginAct(Brawl::EAct::BodyBlow, Brawl::BodyBlowSeconds);
	return true;
}

bool APestiPerson::SlipPunch()
{
	using namespace Brawl;
	// Late in the hop, or a moment after it, with the fists up.
	const bool bLateInHop = Act == EAct::Sidestep && ActLength > 0.f && ActClock >= ActLength * SlipFrom;
	const bool bJustAfter = IsFree(Act) && GetWorld()->GetTimeSeconds() - DodgeEndedAt <= SlipWindow;
	if ((!bLateInHop && !bJustAfter) || !SpendStamina(SlipCounterCost))
	{
		return false;
	}
	ChainStep = 0;
	BeginAct(EAct::SlipCounter, SlipCounterSeconds);
	return true;
}

bool APestiPerson::ButtHim()
{
	if ((Act != Brawl::EAct::Holding && Act != Brawl::EAct::Knee) || !Holding.IsValid() || !SpendStamina(Brawl::HeadbuttCost))
	{
		return false;
	}
	BeginAct(Brawl::EAct::Headbutt, Brawl::HeadbuttSeconds);
	return true;
}

bool APestiPerson::ThrowAtWall()
{
	if ((Act != Brawl::EAct::Holding && Act != Brawl::EAct::Knee) || !Holding.IsValid() || !SpendStamina(Brawl::WallThrowCost))
	{
		return false;
	}
	// Round to the nearest wall (he goes round with him), or straight on if there is none.
	FVector Way = FVector::ZeroVector;
	if (FindWall(Way))
	{
		SetActorRotation(FRotator(0.0, Way.Rotation().Yaw, 0.0));
	}
	BeginAct(Brawl::EAct::WallThrow, Brawl::WallThrowSeconds);
	return true;
}

bool APestiPerson::FindWall(FVector& OutWay) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(BrawlWall), false, this);
	if (const APestiPerson* Held = Holding.Get())
	{
		Params.AddIgnoredActor(Held);
	}
	const FVector From = GetActorLocation();
	double Best = Brawl::WallLook;
	bool bFound = false;
	constexpr int32 Ways = 16;
	for (int32 i = 0; i < Ways; ++i)
	{
		const FVector Way = FRotator(0.0, 360.0 * i / Ways, 0.0).Vector();
		FHitResult Wall;
		if (World->LineTraceSingleByChannel(Wall, From, From + Way * Brawl::WallLook, ECC_Visibility, Params)
			&& FMath::Abs(Wall.ImpactNormal.Z) < PestiPerson::SlamNormalZ && !Cast<APawn>(Wall.GetActor()) && Wall.Distance < Best)
		{
			Best = Wall.Distance;
			OutWay = Way;
			bFound = true;
		}
	}
	return bFound;
}

void APestiPerson::GivePoints(APestiPerson* From, int32 Points) const
{
	if (!From || From == this || !From->IsPlayerControlled() || IsPlayerControlled() || Points <= 0)
	{
		return;
	}
	if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
	{
		Game->AddFightPoints(Points);
	}
}

void APestiPerson::SetGuard(bool bUp)
{
	if (bUp && Act == Brawl::EAct::Stance)
	{
		BeginAct(Brawl::EAct::Guard, 0.f);
	}
	else if (!bUp && Act == Brawl::EAct::Guard)
	{
		BeginAct(Brawl::EAct::Stance, 0.f);
	}
}

void APestiPerson::TugFree()
{
	if (Act != Brawl::EAct::Held || ++Tugs < Brawl::BreakFreePresses)
	{
		return;
	}
	// Free: both stagger apart.
	APestiPerson* Grabber = HeldBy.Get();
	LetGo();
	BeginAct(Brawl::EAct::Stagger, Brawl::StaggerSeconds * 0.6f);
	if (Grabber)
	{
		Grabber->BeginAct(Brawl::EAct::Stagger, Brawl::StaggerSeconds * 0.6f);
	}
}

void APestiPerson::BeHeld(APestiPerson* Grabber)
{
	LetGo();
	HeldBy = Grabber;
	Tugs = 0;
	BeginAct(Brawl::EAct::Held, 0.f);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
}

void APestiPerson::BeThrown(const FVector& Direction, APestiPerson* Thrower)
{
	HeldBy = nullptr;
	ThrownBy = Thrower;
	bThrownHard = false;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->SetMovementMode(MOVE_Walking);
	BeginAct(Brawl::EAct::Shoved, Brawl::ShovedSeconds);
	LaunchCharacter(Direction.GetSafeNormal2D() * Brawl::ThrowSpeed + FVector(0.0, 0.0, Brawl::ThrowLift), true, true);
}

void APestiPerson::LetGo()
{
	// Let go of the one he holds.
	if (APestiPerson* Held = Holding.Get())
	{
		Holding = nullptr;
		if (Held->HeldBy.Get() == this)
		{
			Held->HeldBy = nullptr;
			Held->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			if (Held->Act == Brawl::EAct::Held)
			{
				Held->BeginAct(Brawl::EAct::Stance, 0.f);
			}
		}
	}
	// Or get let go of.
	if (APestiPerson* Grabber = HeldBy.Get())
	{
		HeldBy = nullptr;
		if (Grabber->Holding.Get() == this)
		{
			Grabber->Holding = nullptr;
			if (Grabber->Act == Brawl::EAct::Holding || Grabber->Act == Brawl::EAct::Knee || Grabber->Act == Brawl::EAct::Shove)
			{
				Grabber->BeginAct(Brawl::EAct::Stance, 0.f);
			}
		}
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void APestiPerson::TackleDown(APestiPerson* Runner)
{
	if (!Runner)
	{
		return;
	}
	Foe = Runner;
	Runner->Foe = this;
	Runner->LetGo();
	GetCharacterMovement()->bOrientRotationToMovement = false;
	Runner->GetCharacterMovement()->bOrientRotationToMovement = false;
	// He dives at the runner's back; the runner goes down face first.
	const FVector Ahead = (Runner->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!Ahead.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.0, Ahead.Rotation().Yaw, 0.0));
	}
	GetCharacterMovement()->Velocity = Ahead * PestiPerson::TackleSlide;
	Runner->GetCharacterMovement()->Velocity = Ahead * PestiPerson::TackleSlide;
	BeginAct(Brawl::EAct::Tackle, Brawl::TackleSeconds);
	Runner->BeginAct(Brawl::EAct::Tackled, Brawl::TackledSeconds);
	Stamina = Brawl::MaxStamina;
	Runner->Stamina = Brawl::MaxStamina;
}

void APestiPerson::GiveIn()
{
	LetGo();
	// How he ends up on the ground: on his back, face down or curled up (see Brawl::Beaten).
	const int32 Way = FMath::RandRange(0, 2);
	HurtSide = Way == 0 ? 1.f : Way == 1 ? -1.f : 0.f;
	NextGroanAt = GetWorld()->GetTimeSeconds() + FMath::FRandRange(2.f, 4.f);
	BeginAct(Brawl::EAct::Beaten, 0.f);
	GetCharacterMovement()->StopMovementImmediately();
}

void APestiPerson::TakeBlow(Brawl::EBlow Blow, APestiPerson* From)
{
	using namespace Brawl;
	if (Act == EAct::None || IsOnGround(Act))
	{
		return;
	}
	// Sidestepping: it goes past him.
	if (IsDodging())
	{
		PlayFightSound(FightSynth::EKind::Swish, 0.5f);
		return;
	}
	const FVector ToHim = From ? (From->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() : GetActorForwardVector();
	const bool bFacing = FVector::DotProduct(GetActorForwardVector(), ToHim) > PestiPerson::GuardFacing;
	float Lost = Damage(Blow) * (Act == EAct::Winded ? WindedDamage : 1.f);
	if (From)
	{
		Lost *= From->GetBlowStrength();
	}

	// A guard stops jabs, the chain's punches and knees, at a cost in breath;
	// a heavy swing or a finisher breaks through it.
	const bool bQuick = Blow == EBlow::Jab || Blow == EBlow::Cross || Blow == EBlow::Hook || Blow == EBlow::Uppercut || Blow == EBlow::Knee;
	const bool bBig = Blow == EBlow::Heavy || Blow == EBlow::Rising || Blow == EBlow::Haymaker;
	// Bandi's moves get past a guard: under it, round it, or he is held.
	const bool bLesson = Blow == EBlow::Body || Blow == EBlow::SlipCounter || Blow == EBlow::Headbutt || Blow == EBlow::WallSlam;
	if (Act == EAct::Guard && bFacing && bQuick)
	{
		HitStop(HitStopLight);
		FightMotion.Block(FightAnim::BlowWeight(Blow));
		if (From)
		{
			From->HitStop(HitStopLight);
			From->FightMotion.Recoil(0.6f * FightAnim::BlowWeight(Blow));
		}
		PlayFightSound(FightSynth::EKind::Block, Blow == EBlow::Knee ? 0.6f : 0.8f);
		Stamina -= BlockCost;
		RestFrom = GetWorld()->GetTimeSeconds() + RestBefore;
		if (Stamina <= 0.f)
		{
			BeginAct(EAct::Winded, WindedSeconds);
		}
		return;
	}
	// Not the player: a run of quick punches gets broken after two or three.
	// He takes the next one on his arms and (mostly) hits straight back.
	if (!IsPlayerControlled() && From && bQuick && Blow != EBlow::Knee)
	{
		const double Now = GetWorld()->GetTimeSeconds();
		if (Now - LastQuickHitAt > PunchRunGap)
		{
			QuickHitsInRow = 0;
		}
		LastQuickHitAt = Now;
		if (QuickHitsInRow >= BreakRunAfter && (Act == EAct::Hurt || Act == EAct::Stance) && Stamina > 0.f && FMath::FRand() < BreakChance)
		{
			QuickHitsInRow = 0;
			BreakRunAfter = FMath::RandRange(BreakRunMin, BreakRunMax);
			HitStop(HitStopLight);
			From->HitStop(HitStopLight);
			FightMotion.Block(FightAnim::BlowWeight(Blow));
			From->FightMotion.Recoil(0.6f * FightAnim::BlowWeight(Blow));
			PlayFightSound(FightSynth::EKind::Block, 0.85f);
			Stamina -= BlockCost;
			RestFrom = GetWorld()->GetTimeSeconds() + RestBefore;
			BeginAct(EAct::Stance, 0.f);
			if (FMath::FRand() >= BreakHitBack || !Jab())
			{
				BeginAct(EAct::Guard, 0.f);
			}
			return;
		}
		++QuickHitsInRow;
	}

	const bool bGuardBroken = Act == EAct::Guard && bFacing && bBig;
	if (bGuardBroken)
	{
		PlayFightSound(FightSynth::EKind::Block, 1.f);
		Lost *= GuardBreakShare;
		Stamina -= HeavyCost;
	}

	// Both stand still for a blink as it lands: longer for the big ones.
	const float Still = bBig || bLesson || Blow == EBlow::Counter || Blow == EBlow::Slam || Blow == EBlow::Uppercut ? HitStopHeavy : HitStopLight;
	HitStop(Still);
	if (From)
	{
		From->HitStop(Still);
	}

	// Held and kneed, he stays held; anything else lets go and knocks him back.
	const bool bStayHeld = Act == EAct::Held && Blow == EBlow::Knee;
	if (!bStayHeld)
	{
		LetGo();
	}
	// The blow landing, and him crying out (not at every jab).
	switch (Blow)
	{
	case EBlow::Jab:
	case EBlow::Cross:
	case EBlow::Hook:
		PlayFightSound(FightSynth::EKind::Smack, Blow == EBlow::Jab ? 0.6f : 0.75f);
		if (FMath::RandBool())
		{
			PlayFightSound(FightSynth::EKind::Pain, 0.5f, 0.03f);
		}
		break;
	case EBlow::Uppercut:
		PlayFightSound(FightSynth::EKind::Smack, 0.9f);
		PlayFightSound(FightSynth::EKind::Pain, 0.7f, 0.03f);
		break;
	case EBlow::Heavy:
	case EBlow::Rising:
	case EBlow::Haymaker:
		PlayFightSound(FightSynth::EKind::Smack, Lost < Damage(Blow) ? 0.5f : 1.f);
		PlayFightSound(FightSynth::EKind::Pain, 0.9f, 0.03f);
		break;
	case EBlow::Counter:
		PlayFightSound(FightSynth::EKind::Smack, 0.95f);
		PlayFightSound(FightSynth::EKind::Pain, 0.8f, 0.03f);
		break;
	case EBlow::Knee:
		PlayFightSound(FightSynth::EKind::Smack, 0.75f);
		PlayFightSound(FightSynth::EKind::Pain, 0.7f, 0.03f);
		break;
	case EBlow::Slam:
	case EBlow::WallSlam:
		PlayFightSound(FightSynth::EKind::Slam, 1.f);
		PlayFightSound(FightSynth::EKind::Pain, 0.9f, 0.05f);
		break;
	case EBlow::Body:
		PlayFightSound(FightSynth::EKind::Smack, 0.7f);
		PlayFightSound(FightSynth::EKind::Pain, 0.6f, 0.04f);
		break;
	case EBlow::SlipCounter:
	case EBlow::Headbutt:
		PlayFightSound(FightSynth::EKind::Smack, 0.95f);
		PlayFightSound(FightSynth::EKind::Pain, 0.8f, 0.03f);
		break;
	default:
		break;
	}
	LoseHealth(Lost);
	// The player's fight points: more for the big blows, most for beating him.
	const int32 BlowPoints = bBig || bLesson || Blow == EBlow::Counter || Blow == EBlow::Slam ? PointsBig : PointsHit;
	if (Act == EAct::Beaten)
	{
		GivePoints(From, BlowPoints + PointsWin);
		return;
	}
	GivePoints(From, BlowPoints);
	HurtSide = FMath::RandBool() ? 1.f : -1.f;
	// The blow rocks him (less through a guard), and jolts back up the arm that threw it.
	if (bGuardBroken)
	{
		FightMotion.Block(FightAnim::BlowWeight(Blow));
	}
	else
	{
		FightMotion.Hit(FightAnim::BlowWeight(Blow), HurtSide);
	}
	if (From)
	{
		From->FightMotion.Recoil(FightAnim::BlowWeight(Blow));
	}
	switch (Blow)
	{
	case EBlow::Jab:
	case EBlow::Cross:
	case EBlow::Hook:
	case EBlow::Knee:
		if (!bStayHeld)
		{
			BeginAct(EAct::Hurt, HurtSeconds);
		}
		break;
	case EBlow::Slam:
	case EBlow::WallSlam:
		GetCharacterMovement()->StopMovementImmediately();
		BeginAct(EAct::Down, DownSeconds);
		GivePoints(From, PointsDown);
		break;
	case EBlow::Body:
		// The breath knocked out of him: doubled up if it was the last of it.
		Stamina -= BodyBlowBreath;
		RestFrom = GetWorld()->GetTimeSeconds() + RestBefore;
		if (Stamina <= 0.f)
		{
			Stamina = 0.f;
			BeginAct(EAct::Winded, WindedSeconds);
		}
		else
		{
			BeginAct(EAct::Hurt, HurtSeconds);
		}
		break;
	case EBlow::Rising:
		// Lifted off his feet: down he goes (only reeling, through a guard).
		if (bGuardBroken)
		{
			BeginAct(EAct::Stagger, StaggerSeconds);
		}
		else
		{
			GetCharacterMovement()->StopMovementImmediately();
			BeginAct(EAct::Down, DownSeconds);
			GivePoints(From, PointsDown);
		}
		break;
	case EBlow::Haymaker:
		// Sent flying back; into a wall or a stall, a slam (see Shoved).
		if (From && !bGuardBroken)
		{
			BeThrown(GetActorLocation() - From->GetActorLocation(), From);
		}
		else
		{
			BeginAct(EAct::Stagger, StaggerSeconds);
		}
		break;
	default:
		BeginAct(EAct::Stagger, StaggerSeconds);
		break;
	}
}

void APestiPerson::LandBlow(Brawl::EBlow Blow)
{
	APestiPerson* Other = Foe.Get();
	if (!Other)
	{
		return;
	}
	const FVector Between = Other->GetActorLocation() - GetActorLocation();
	const double Cos = FVector::DotProduct(GetActorForwardVector().GetSafeNormal2D(), Between.GetSafeNormal2D());
	if (Between.Size2D() <= Brawl::Reach && Cos >= FMath::Cos(FMath::DegreesToRadians(Brawl::ReachAngle)))
	{
		Other->TakeBlow(Blow, this);
	}
}

void APestiPerson::FaceFoe(float DeltaSeconds)
{
	const APestiPerson* Other = Foe.Get();
	if (!Other)
	{
		return;
	}
	const FVector Between = (Other->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (Between.IsNearlyZero())
	{
		return;
	}
	const float Yaw = FMath::FixedTurn(static_cast<float>(GetActorRotation().Yaw), static_cast<float>(Between.Rotation().Yaw), Brawl::TurnRate * DeltaSeconds);
	SetActorRotation(FRotator(0.0, Yaw, 0.0));
}

void APestiPerson::TickBrawl(float DeltaSeconds)
{
	using namespace Brawl;
	ActClock += DeltaSeconds;
	const float T = ActLength > 0.f ? FMath::Clamp(ActClock / ActLength, 0.f, 1.f) : 0.f;
	const bool bOver = ActLength > 0.f && ActClock >= ActLength;
	const double Now = GetWorld()->GetTimeSeconds();
	APestiPerson* Other = Foe.Get();
	UCharacterMovementComponent* Movement = GetCharacterMovement();

	// Breath: held guard wears it down; at rest it comes back.
	if (Act == EAct::Guard)
	{
		Stamina -= GuardDrain * DeltaSeconds;
		if (Stamina <= 0.f)
		{
			BeginAct(EAct::Winded, WindedSeconds);
		}
	}
	else if (Act != EAct::Winded && Now >= RestFrom)
	{
		Stamina = FMath::Min(MaxStamina, Stamina + StaminaBack * DeltaSeconds);
	}
	if (FightNoise)
	{
		const float Short = FMath::Clamp((PestiPerson::PantBelow - Stamina) / PestiPerson::PantBelow, 0.f, 1.f);
		FightNoise->SetBreathless(Act == EAct::Winded ? 1.f : Short * PestiPerson::LowPant);
	}

	// The one he holds stays in front of him, facing him.
	APestiPerson* Victim = Holding.Get();
	if (Victim && (Act == EAct::Holding || Act == EAct::Knee || Act == EAct::Shove || Act == EAct::Headbutt || Act == EAct::WallThrow))
	{
		const FVector Ahead = GetActorForwardVector().GetSafeNormal2D();
		Victim->SetActorLocationAndRotation(GetActorLocation() + Ahead * HoldGap, FRotator(0.0, (-Ahead).Rotation().Yaw, 0.0),
			false, nullptr, ETeleportType::TeleportPhysics);
	}

	switch (Act)
	{
	case EAct::Jab:
		if (!bBlowDone && T >= JabLands)
		{
			bBlowDone = true;
			LandBlow(EBlow::Jab);
		}
		break;
	case EAct::HeavyWind:
		if (bOver)
		{
			BeginAct(EAct::HeavySwing, HeavySwingSeconds);
			return;
		}
		break;
	case EAct::HeavySwing:
		if (!bBlowDone && T >= HeavyLands)
		{
			bBlowDone = true;
			LandBlow(EBlow::Heavy);
		}
		break;
	case EAct::Counter:
		if (!bBlowDone && T >= CounterLands)
		{
			bBlowDone = true;
			LandBlow(EBlow::Counter);
		}
		break;
	case EAct::Cross:
	case EAct::Hook:
	case EAct::Uppercut:
	case EAct::Rising:
	case EAct::Haymaker:
	{
		const float LandsAt = Act == EAct::Cross ? CrossLands : Act == EAct::Hook ? HookLands
			: Act == EAct::Uppercut ? UppercutLands : Act == EAct::Rising ? RisingLands : HaymakerLands;
		if (!bBlowDone && T >= LandsAt)
		{
			bBlowDone = true;
			const EBlow Landing = Act == EAct::Cross ? EBlow::Cross : Act == EAct::Hook ? EBlow::Hook
				: Act == EAct::Uppercut ? EBlow::Uppercut : Act == EAct::Rising ? EBlow::Rising : EBlow::Haymaker;
			LandBlow(Landing);
			// The blow may have ended this act (a hit back can't come in the
			// same moment, but be safe): stop here if it did.
			if (Act == EAct::None)
			{
				return;
			}
		}
		break;
	}
	case EAct::GrabReach:
		if (bOver)
		{
			const bool bCaught = Other && !IsOnGround(Other->Act) && !Other->IsDodging() && Other->Act != EAct::Held && Other->Act != EAct::Holding
				&& Other->Act != EAct::Shoved && FVector::Dist2D(Other->GetActorLocation(), GetActorLocation()) <= GrabDistance
				&& FVector::DotProduct(GetActorForwardVector(), (Other->GetActorLocation() - GetActorLocation()).GetSafeNormal2D()) > PestiPerson::GuardFacing;
			if (bCaught)
			{
				Other->BeHeld(this);
				Holding = Other;
				HoldUntil = Now + HoldSeconds;
				KneesGiven = 0;
				BeginAct(EAct::Holding, 0.f);
			}
			else
			{
				BeginAct(EAct::Stance, 0.f);
			}
			return;
		}
		break;
	case EAct::Holding:
		// He wriggles free in the end.
		if (!Victim || Now >= HoldUntil)
		{
			LetGo();
			BeginAct(EAct::Stance, 0.f);
		}
		break;
	case EAct::Knee:
		if (!bBlowDone && T >= KneeLands && Victim)
		{
			bBlowDone = true;
			Victim->TakeBlow(EBlow::Knee, this);
		}
		if (bOver)
		{
			BeginAct(Holding.IsValid() ? EAct::Holding : EAct::Stance, 0.f);
			return;
		}
		break;
	case EAct::BodyBlow:
		if (!bBlowDone && T >= BodyBlowLands)
		{
			bBlowDone = true;
			LandBlow(EBlow::Body);
		}
		break;
	case EAct::SlipCounter:
		if (!bBlowDone && T >= SlipCounterLands)
		{
			bBlowDone = true;
			LandBlow(EBlow::SlipCounter);
		}
		break;
	case EAct::Headbutt:
		// Into his face, and he lets go of him: the man reels back free.
		if (!bBlowDone && T >= HeadbuttLands)
		{
			bBlowDone = true;
			LetGo();
			if (Victim)
			{
				Victim->TakeBlow(EBlow::Headbutt, this);
			}
		}
		break;
	case EAct::WallThrow:
		// Swung round, then hurled off hard: into the wall, a slam that hurts more.
		if (!bBlowDone && T >= WallThrowLets)
		{
			bBlowDone = true;
			Holding = nullptr;
			if (Victim)
			{
				const FVector Ahead = GetActorForwardVector().GetSafeNormal2D();
				Victim->BeThrown(Ahead, this);
				Victim->bThrownHard = true;
				Victim->LaunchCharacter(Ahead * WallThrowSpeed + FVector(0.0, 0.0, ThrowLift), true, true);
			}
		}
		break;
	case EAct::Shove:
		if (!bBlowDone && T >= ShoveLets)
		{
			bBlowDone = true;
			Holding = nullptr;
			if (Victim)
			{
				Victim->BeThrown(GetActorForwardVector(), this);
			}
		}
		break;
	case EAct::Held:
		if (!HeldBy.IsValid())
		{
			BeginAct(EAct::Stance, 0.f);
		}
		break;
	case EAct::Shoved:
		// Into something standing just ahead: a slam (as NotifyHit, should the move miss telling him).
		if (!bBlowDone && ActClock > PestiPerson::SlamLookAfter)
		{
			const FVector Going = Movement->Velocity.GetSafeNormal2D();
			FHitResult Wall;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(BrawlSlam), false, this);
			Params.AddIgnoredActor(ThrownBy.Get());
			const FVector From = GetActorLocation();
			if (!Going.IsNearlyZero()
				&& GetWorld()->LineTraceSingleByChannel(Wall, From, From + Going * (GetCapsuleComponent()->GetScaledCapsuleRadius() + PestiPerson::SlamLook), ECC_Visibility, Params)
				&& FMath::Abs(Wall.ImpactNormal.Z) < PestiPerson::SlamNormalZ && !Cast<APawn>(Wall.GetActor()))
			{
				bBlowDone = true;
				Movement->StopMovementImmediately();
				TakeBlow(bThrownHard ? EBlow::WallSlam : EBlow::Slam, ThrownBy.Get());
				return;
			}
		}
		// Thrown off without hitting anything: a stagger as he lands.
		if (bOver)
		{
			if (!bBlowDone)
			{
				PlayFightSound(FightSynth::EKind::Thud, 0.7f);
				LoseHealth(Damage(EBlow::Throw));
			}
			BeginAct(Act == EAct::Beaten ? EAct::Beaten : EAct::Stagger, Act == EAct::Beaten ? 0.f : StaggerSeconds * 0.6f);
			return;
		}
		break;
	case EAct::Down:
		if (bOver)
		{
			BeginAct(EAct::GetUp, GetUpSeconds);
			return;
		}
		break;
	case EAct::Winded:
		if (bOver)
		{
			Stamina = AfterWinded;
		}
		break;
	case EAct::Beaten:
		// Lying there, a groan now and then.
		if (Now >= NextGroanAt)
		{
			NextGroanAt = Now + FMath::FRandRange(3.f, 7.f);
			PlayFightSound(FightSynth::EKind::Pain, 0.3f);
		}
		break;
	default:
		break;
	}

	// Stepping in behind his punches (the player's), so a punch from a little
	// too far still reaches; never closer than fighters keep.
	if (IsPlayerControlled() && (IsChainPunch(Act) || Act == EAct::BodyBlow || Act == EAct::SlipCounter) && Other && ActLength > 0.f && T < StepShare)
	{
		const FVector Between = Other->GetActorLocation() - GetActorLocation();
		const double Gap = Between.Size2D() - KeepApart;
		if (Gap > 1.0)
		{
			const double Step = FMath::Min(Gap, StepIn * DeltaSeconds / (ActLength * StepShare));
			AddActorWorldOffset(Between.GetSafeNormal2D() * Step, true);
		}
	}

	// Sidestepping: the hop itself, quick off the mark and easing as he lands.
	if (Act == EAct::Sidestep && ActLength > 0.f)
	{
		const float WasDone = FightAnim::Eased(FightAnim::EEase::Snap, (ActClock - DeltaSeconds) / ActLength);
		const float NowDone = FightAnim::Eased(FightAnim::EEase::Snap, ActClock / ActLength);
		AddActorWorldOffset(DodgeWay * static_cast<double>(NowDone - WasDone), true);
	}

	// Anything with an end goes back to the fists up (or to getting his breath back).
	if (bOver && Act != EAct::Beaten && Act != EAct::Held && Act != EAct::Holding)
	{
		BeginAct(Stamina <= 0.f && Act != EAct::Winded ? EAct::Winded : EAct::Stance, Stamina <= 0.f && Act != EAct::Winded ? WindedSeconds : 0.f);
	}

	// Moving only with the fists up or covering up; turning to him unless knocked about.
	const bool bFree = IsFree(Act);
	if (Movement->MovementMode != MOVE_None)
	{
		Movement->MaxWalkSpeed = Act == EAct::Stance ? FightSpeed : Act == EAct::Guard ? GuardSpeed : 0.f;
	}
	if (bFree || IsChainPunch(Act) || Act == EAct::Sidestep || Act == EAct::HeavyWind || Act == EAct::HeavySwing || Act == EAct::Counter
		|| Act == EAct::GrabReach || Act == EAct::Holding || Act == EAct::Knee || Act == EAct::Winded
		|| Act == EAct::BodyBlow || Act == EAct::SlipCounter || Act == EAct::Headbutt)
	{
		FaceFoe(DeltaSeconds);
	}

	// The pose, through his fighting body (FightAnim): melting from the last
	// move, breathing, stepping and leaning as he moves, rocked by blows.
	const float Shown = ActLength > 0.f ? FMath::Clamp(ActClock / ActLength, 0.f, 1.f) : 0.f;
	const FVector Moving = Movement->IsMovingOnGround() ? GetVelocity() : FVector::ZeroVector;
	const float GoingAhead = static_cast<float>(FVector::DotProduct(Moving, GetActorForwardVector()));
	const float GoingAside = static_cast<float>(FVector::DotProduct(Moving, GetActorRightVector()));
	const float Tired = FMath::Clamp(1.f - Stamina / MaxStamina, 0.f, 1.f);
	const FreeRun::FPose Moved = FightMotion.Update(PoseFor(Act, Shown, ActClock, Act == EAct::Sidestep || Act == EAct::SlipCounter ? DodgeSide : HurtSide), Act, DeltaSeconds, GoingAhead, GoingAside, Tired);
	ApplyPose(Moved, FightMotion.GetLegTwist());
}

FVector APestiPerson::InputDirection(const APlayerController* PC) const
{
	auto KeyAxis = [PC](const FKey& Positive, const FKey& Negative)
	{
		return (PC->IsInputKeyDown(Positive) ? 1.f : 0.f) - (PC->IsInputKeyDown(Negative) ? 1.f : 0.f);
	};
	const float Forward = KeyAxis(EKeys::W, EKeys::S) + KeyAxis(EKeys::Up, EKeys::Down) + PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
	const float Right = KeyAxis(EKeys::D, EKeys::A) + KeyAxis(EKeys::Right, EKeys::Left) + PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
	const FRotator Heading(0.f, PC->GetControlRotation().Yaw, 0.f);
	const FVector Way = FRotationMatrix(Heading).GetUnitAxis(EAxis::X) * Forward + FRotationMatrix(Heading).GetUnitAxis(EAxis::Y) * Right;
	return Way.GetSafeNormal2D();
}

void APestiPerson::PlayerFight(APlayerController* PC)
{
	using namespace Brawl;
	const double Now = GetWorld()->GetTimeSeconds();
	APestiPerson* Other = Foe.Get();

	// Stepping about with the fists up (no running).
	if (IsFree(Act))
	{
		Walk(PC);
		GetCharacterMovement()->MaxWalkSpeed = Act == EAct::Guard ? GuardSpeed : FightSpeed;
	}

	const bool bPunchDown = PC->IsInputKeyDown(EKeys::LeftMouseButton) || PC->IsInputKeyDown(EKeys::Gamepad_FaceButton_Left);
	const bool bPunchPressed = PC->WasInputKeyJustPressed(EKeys::LeftMouseButton) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Left);
	const bool bGuardDown = PC->IsInputKeyDown(EKeys::RightMouseButton) || PC->IsInputKeyDown(EKeys::Gamepad_RightShoulder);
	const bool bGuardPressed = PC->WasInputKeyJustPressed(EKeys::RightMouseButton) || PC->WasInputKeyJustPressed(EKeys::Gamepad_RightShoulder);
	const bool bGrabPressed = PC->WasInputKeyJustPressed(EKeys::E) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right);
	const bool bDodgePressed = PC->WasInputKeyJustPressed(EKeys::SpaceBar) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom);
	const bool bThrowPressed = PC->WasInputKeyJustPressed(EKeys::F) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Top);
	const bool bHoldingHim = Act == EAct::Holding || Act == EAct::Knee;

	// Bandi's moves, once learnt. Holding him: Space a headbutt, F a throw at the wall.
	if (bHoldingHim && bDodgePressed && KnowsLesson(ELesson::Headbutt) && ButtHim())
	{
		return;
	}
	if (bHoldingHim && bThrowPressed && KnowsLesson(ELesson::WallThrow) && ThrowAtWall())
	{
		return;
	}
	// A click late in a sidestep or just after it: the punch from the side.
	if (bPunchPressed && KnowsLesson(ELesson::SlipCounter) && SlipPunch())
	{
		KeptPunch = 0;
		PunchDownAt = -1.0;
		return;
	}
	// A click while stepping back from him: the punch to the belly.
	if (bPunchPressed && Other && IsFree(Act) && KnowsLesson(ELesson::BodyBlow))
	{
		const FVector Toward = (Other->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		if (FVector::DotProduct(InputDirection(PC), Toward) < -0.5 && BellyPunch())
		{
			KeptPunch = 0;
			PunchDownAt = -1.0;
			return;
		}
	}

	// Space: a sidestep the way the keys point (back, with none).
	if (bDodgePressed && Dodge(InputDirection(PC)))
	{
		KeptPunch = 0;
		PunchDownAt = -1.0;
	}

	// Guard: the right button; pressed as his swing comes, a counter instead.
	if (bGuardPressed)
	{
		Counter();
	}
	// Covering up straight out of a quick punch once it has landed: its pulling back need not be waited out.
	if (bGuardDown && IsLightPunch(Act) && ActLength > 0.f && ActClock >= ActLength * ChainFrom)
	{
		BeginAct(EAct::Guard, 0.f);
	}
	SetGuard(bGuardDown);

	// Punch: a tap jabs, holding it pulls back for a heavy swing; holding him, a knee.
	if (Act == EAct::Holding)
	{
		if (bPunchPressed)
		{
			Knee();
		}
		PunchDownAt = -1.0;
	}
	else if (bPunchPressed)
	{
		PunchDownAt = Now;
		bPunchHeld = false;
	}
	else if (PunchDownAt >= 0.0)
	{
		// A tap is the next punch of the chain, holding it the finisher;
		// either one is kept until he can throw it.
		if (bPunchDown && !bPunchHeld && Now - PunchDownAt >= PestiPerson::HeavyHold)
		{
			bPunchHeld = true;
			KeptPunch = 2;
			KeptAt = Now;
		}
		else if (!bPunchDown)
		{
			if (!bPunchHeld)
			{
				KeptPunch = 1;
				KeptAt = Now;
			}
			PunchDownAt = -1.0;
		}
	}
	if (KeptPunch != 0)
	{
		if (Act == EAct::Holding || Act == EAct::Held || Now - KeptAt > KeepClick || ChainPunch(KeptPunch == 2))
		{
			KeptPunch = 0;
		}
	}

	// E: grab him; holding him, shove him off the way the keys point; held, tug free.
	if (bGrabPressed)
	{
		if (Act == EAct::Held)
		{
			TugFree();
		}
		else if (Act == EAct::Holding || Act == EAct::Knee)
		{
			const FVector Way = InputDirection(PC);
			Shove(Way.IsNearlyZero() ? GetActorForwardVector() : Way);
		}
		else
		{
			Grab();
		}
	}

	// On the screen: the keys, the other man's health, his own breath, and the key to press now.
	APestiHUD::Show(this, 1, 0.f, FColor::White,
		TEXT("Kattint\u00e1s: \u00fct\u00e9ssor, nyomva tartva: befejez\u0151 \u00fct\u00e9s \u00b7 jobb gomb: v\u00e9dekez\u00e9s \u00b7 E: megragad\u00e1s \u00b7 Sz\u00f3k\u00f6z: kit\u00e9r\u00e9s"
			"  (Click: punch chain, hold: finisher \u00b7 right click: guard \u00b7 E: grab \u00b7 Space: sidestep)"));
	APestiHUD* Hud = APestiHUD::Get(this);
	if (!Hud)
	{
		return;
	}
	if (Other)
	{
		Hud->ShowFight(Other->GetFightName(), Other->GetHealthShare(), Stamina / MaxStamina);
	}
	const float Left = Other ? Other->GetWindUpLeft() : -1.f;
	if (Act == EAct::Held)
	{
		Hud->Prompt(TEXT("E"), TEXT("Kiszabadul\u00e1s"), TEXT("Break free"), true);
	}
	else if (Left >= 0.f && Other && FVector::Dist2D(Other->GetActorLocation(), GetActorLocation()) <= Reach + PestiPerson::CounterSlack)
	{
		Hud->Prompt(TEXT("JOBB GOMB"), TEXT("Ellent\u00e1mad\u00e1s"), TEXT("Counter"), Left <= CounterWindow);
	}
	else if (Act == EAct::Holding || Act == EAct::Knee)
	{
		FVector WallWay = FVector::ZeroVector;
		if (KnowsLesson(ELesson::WallThrow) && FindWall(WallWay))
		{
			Hud->Prompt(TEXT("F"), TEXT("Falhoz v\u00e1g\u00e1s"), TEXT("Wall throw"), true);
		}
		else
		{
			Hud->Prompt(TEXT("E"), TEXT("L\u00f6k\u00e9s"), TEXT("Shove"), false);
		}
	}
}
