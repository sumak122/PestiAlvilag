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
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiCar.h"
#include "ProceduralMeshComponent.h"
#include "PoliceCar.h"
#include "TrafficCar.h"
#include "TrafficManager.h"
#include "PestiHud.h"

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

	constexpr double MinTimeOnFoot = 0.3;

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
	// Stepping about in a fight: faster than this the legs step, strides
	// this long and this wide (cm/s, cm, degrees).
	constexpr float FightStepSpeed = 30.f;
	constexpr float FightStride = 70.f;
	constexpr float FightSwing = 16.f;
	// Thrown off, he looks this far ahead of his capsule for a wall, after
	// this long in the air (cm, s).
	constexpr double SlamLook = 25.0;
	constexpr float SlamLookAfter = 0.05f;
	// Both slide on a little after a tackle (cm/s).
	constexpr float TackleSlide = 250.f;
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
	if (PC->WasInputKeyJustPressed(EKeys::F) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right))
	{
		TryEnterCar(PC);
		return;
	}

	if (GEngine)
	{
		APestiHUD::Show(this, 1, 0.f, FColor::White,
			TEXT("On foot   (F: get in or steal a car, Shift: sprint, Space: jump, or vault and slide running at things, N: traffic view, P: police test, F9: new game)"));
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

void APestiPerson::ApplyPose(const FreeRun::FPose& Pose)
{
	if (PoseRoot)
	{
		PoseRoot->SetRelativeLocationAndRotation(FVector(0.0, 0.0, Pose.Lift), FRotator(Pose.Pitch, Pose.Yaw, Pose.Roll));
	}
	PersonModel::PoseLegs(Pieces, Pose.LegL, Pose.LegR, Pose.KneeL, Pose.KneeR);
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
}

void APestiPerson::TryEnterCar(APlayerController* PC)
{
	// The F that got him out of a car may still read as pressed this frame.
	if (GetWorld()->GetTimeSeconds() - OutOfCarTime < PestiPerson::MinTimeOnFoot)
	{
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

	SetInWorld(false);
	Nearest->TakeDriver(this);
	PC->Possess(Nearest);
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
	return PersonModel::PlayerOutfit();
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
		TakeBlow(Brawl::EBlow::Slam, ThrownBy.Get());
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
		Game->HurtPlayer(Amount * PestiPerson::PlayerHurtShare);
	}
}

void APestiPerson::StartFight(APestiPerson* InFoe)
{
	Foe = InFoe;
	if (Act == Brawl::EAct::None)
	{
		BeginAct(Brawl::EAct::Stance, 0.f);
	}
	Stamina = Brawl::MaxStamina;
	// He faces the other man, whichever way he steps.
	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void APestiPerson::EndFight()
{
	LetGo();
	Act = Brawl::EAct::None;
	Foe = nullptr;
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
	Act = NewAct;
	ActClock = 0.f;
	ActLength = Seconds;
	bBlowDone = false;
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
	Stamina -= Cost;
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

bool APestiPerson::HeavySwing()
{
	if (!Brawl::IsFree(Act) || !SpendStamina(Brawl::HeavyCost))
	{
		return false;
	}
	BeginAct(Brawl::EAct::HeavyWind, Brawl::HeavyWindSeconds);
	return true;
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
	const FVector ToHim = From ? (From->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() : GetActorForwardVector();
	const bool bFacing = FVector::DotProduct(GetActorForwardVector(), ToHim) > PestiPerson::GuardFacing;
	float Lost = Damage(Blow) * (Act == EAct::Winded ? WindedDamage : 1.f);

	// A guard stops jabs and knees, at a cost in breath; a heavy swing breaks through it.
	if (Act == EAct::Guard && bFacing && (Blow == EBlow::Jab || Blow == EBlow::Knee))
	{
		Stamina -= BlockCost;
		RestFrom = GetWorld()->GetTimeSeconds() + RestBefore;
		if (Stamina <= 0.f)
		{
			BeginAct(EAct::Winded, WindedSeconds);
		}
		return;
	}
	if (Act == EAct::Guard && bFacing && Blow == EBlow::Heavy)
	{
		Lost *= GuardBreakShare;
		Stamina -= HeavyCost;
	}

	// Held and kneed, he stays held; anything else lets go and knocks him back.
	const bool bStayHeld = Act == EAct::Held && Blow == EBlow::Knee;
	if (!bStayHeld)
	{
		LetGo();
	}
	LoseHealth(Lost);
	if (Act == EAct::Beaten)
	{
		return;
	}
	HurtSide = FMath::RandBool() ? 1.f : -1.f;
	switch (Blow)
	{
	case EBlow::Jab:
	case EBlow::Knee:
		if (!bStayHeld)
		{
			BeginAct(EAct::Hurt, HurtSeconds);
		}
		break;
	case EBlow::Slam:
		GetCharacterMovement()->StopMovementImmediately();
		BeginAct(EAct::Down, DownSeconds);
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

	// The one he holds stays in front of him, facing him.
	APestiPerson* Victim = Holding.Get();
	if (Victim && (Act == EAct::Holding || Act == EAct::Knee || Act == EAct::Shove))
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
	case EAct::GrabReach:
		if (bOver)
		{
			const bool bCaught = Other && !IsOnGround(Other->Act) && Other->Act != EAct::Held && Other->Act != EAct::Holding
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
				TakeBlow(EBlow::Slam, ThrownBy.Get());
				return;
			}
		}
		// Thrown off without hitting anything: a stagger as he lands.
		if (bOver)
		{
			if (!bBlowDone)
			{
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
	default:
		break;
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
	if (bFree || Act == EAct::Jab || Act == EAct::HeavyWind || Act == EAct::HeavySwing || Act == EAct::Counter
		|| Act == EAct::GrabReach || Act == EAct::Holding || Act == EAct::Knee || Act == EAct::Winded)
	{
		FaceFoe(DeltaSeconds);
	}

	// The pose, and stepping legs when moving with the fists up.
	const float Shown = ActLength > 0.f ? FMath::Clamp(ActClock / ActLength, 0.f, 1.f) : 0.f;
	ApplyPose(PoseFor(Act, Shown, ActClock, HurtSide));
	const float Speed = Movement->IsMovingOnGround() ? static_cast<float>(GetVelocity().Size2D()) : 0.f;
	if (bFree && Speed > PestiPerson::FightStepSpeed)
	{
		StridePhase += DeltaSeconds * Speed / PestiPerson::FightStride * PI;
		PersonModel::StrideLegs(Pieces, StridePhase, PestiPerson::FightSwing, 0.8f);
	}
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

	// Guard: the right button; pressed as his swing comes, a counter instead.
	if (bGuardPressed)
	{
		Counter();
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
		if (bPunchDown && !bPunchHeld && Now - PunchDownAt >= PestiPerson::HeavyHold)
		{
			bPunchHeld = true;
			HeavySwing();
		}
		else if (!bPunchDown)
		{
			if (!bPunchHeld)
			{
				Jab();
			}
			PunchDownAt = -1.0;
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
		TEXT("Kattint\u00e1s: \u00fct\u00e9s, nyomva tartva: nagy \u00fct\u00e9s \u00b7 jobb gomb: v\u00e9dekez\u00e9s \u00b7 E: megragad\u00e1s"
			"  (Click: punch, hold: heavy punch \u00b7 right click: guard \u00b7 E: grab)"));
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
		Hud->Prompt(TEXT("E"), TEXT("L\u00f6k\u00e9s"), TEXT("Shove"), false);
	}
}
