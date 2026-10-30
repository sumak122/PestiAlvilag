// Fill out your copyright notice in the Description page of Project Settings.

#include "ChaseThief.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace ChaseThief
{
	constexpr float MaxHealth = 70.f;

	// Sneaking up: this slow, and this close counts as by her (cm/s, cm).
	constexpr float SneakSpeed = 150.f;
	constexpr double MarkReach = 110.0;

	// Running (cm/s): off like the wind for the first seconds, then as fast
	// as the player with the chaser close, slower the further back he is,
	// and slower again once tired on the last part of the way.
	constexpr float BurstSpeed = 800.f;
	constexpr double BurstSeconds = 3.0;
	constexpr float CloseSpeed = 725.f;
	constexpr float Pace = 700.f;
	constexpr float FarSpeed = 620.f;
	constexpr float VeryFarSpeed = 540.f;
	constexpr double Close = 500.0;
	constexpr double Far = 1500.0;
	constexpr double VeryFar = 2600.0;
	constexpr float TiredFrom = 0.6f;
	constexpr float TiredSlower = 45.f;
	constexpr float SlinkSpeed = 520.f;
	// A route point counts as reached this close (cm).
	constexpr double PointReach = 150.0;
	// He mistimes this share of vaults and slides, and goes for the rest
	// a little before the best moment.
	constexpr float FumbleChance = 0.2f;
	constexpr double GoFrom = 0.75;
	// Stuck behind something for this long, he hops (s), when slower than this (cm/s).
	constexpr double StuckSeconds = 0.6;
	constexpr float StuckSpeed = 80.f;

	// Fighting: he keeps about this far off (cm) and steps in to hit; he
	// thinks again after this long (s), circles this way for this long (s).
	constexpr double TooClose = 95.0;
	constexpr float ThinkMin = 0.5f;
	constexpr float ThinkMax = 1.1f;
	constexpr float CircleMin = 1.5f;
	constexpr float CircleMax = 3.f;
	constexpr float CircleShare = 0.6f;
	constexpr double SecondJabAfter = 0.42;
	constexpr float LowBreath = 25.f;

	// The purse: a small brown leather bag with a brass clasp, in his right hand (cm, from the shoulder).
	const FVector PurseAt(2.0, 0.0, -70.0);
	const FVector PurseSize(14.0, 5.0, 10.0);
	const FVector ClaspAt(2.0, 0.0, -64.5);
	const FVector ClaspSize(12.0, 5.6, 1.5);
	const FLinearColor Leather(0.24f, 0.11f, 0.05f);
	const FLinearColor Brass(0.62f, 0.48f, 0.20f);
}

AChaseThief::AChaseThief()
{
	// He runs with nobody at the controls.
	AutoPossessAI = EAutoPossessAI::Disabled;
	GetCharacterMovement()->bRunPhysicsWithNoController = true;
	RunSpeed = ChaseThief::BurstSpeed;
	Health = ChaseThief::MaxHealth;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	CubeMesh = Cube.Object;
}

void AChaseThief::BeginPlay()
{
	Super::BeginPlay();
	// No camera of his own is ever looked through.
	if (CameraArm)
	{
		CameraArm->bDoCollisionTest = false;
	}
}

PersonKit::FOutfit AChaseThief::GetDress() const
{
	PersonKit::FOutfit O;
	O.Height = 171.f;
	O.Build = 0.92f;
	O.Hat = PersonKit::EHat::FlatCap;
	O.Top = PersonKit::ETop::Jacket;
	O.bTie = false;
	O.bMoustache = true;
	O.Coat = FLinearColor(0.23f, 0.19f, 0.14f);
	O.HatColor = FLinearColor(0.14f, 0.13f, 0.12f);
	O.Trousers = FLinearColor(0.17f, 0.16f, 0.15f);
	O.Shirt = FLinearColor(0.66f, 0.62f, 0.52f);
	O.bScarf = true;
	O.Scarf = FLinearColor(0.12f, 0.20f, 0.13f);
	return O;
}

FString AChaseThief::GetFightName() const
{
	return TEXT("ZSEBTOLVAJ");
}

float AChaseThief::GetHealthShare() const
{
	return FMath::Clamp(Health / ChaseThief::MaxHealth, 0.f, 1.f);
}

void AChaseThief::LoseHealth(float Amount)
{
	Health = FMath::Max(0.f, Health - Amount);
	if (Health <= 0.f && Act != Brawl::EAct::Beaten)
	{
		GiveIn();
		Mode = EMode::Idle;
	}
}

void AChaseThief::SetRoute(const TArray<FVector2D>& InRoute)
{
	Route = InRoute;
	NextPoint = 0;
	LegsDone = 0.0;
	RouteLength = 0.0;
	for (int32 i = 1; i < Route.Num(); ++i)
	{
		RouteLength += FVector2D::Distance(Route[i - 1], Route[i]);
	}
}

void AChaseThief::SneakUpTo(AActor* InMark)
{
	Mark = InMark;
	Mode = EMode::Sneak;
}

bool AChaseThief::IsAtMark() const
{
	const AActor* Target = Mark.Get();
	return Target && FVector::Dist2D(Target->GetActorLocation(), GetActorLocation()) <= ChaseThief::MarkReach;
}

void AChaseThief::RunFrom(APestiPerson* InChaser)
{
	Chaser = InChaser;
	Mode = EMode::Run;
	RunStart = GetWorld()->GetTimeSeconds();
	SetSlipping(true);
}

float AChaseThief::RouteShare() const
{
	if (RouteLength <= 0.0 || !Route.IsValidIndex(NextPoint))
	{
		return NextPoint >= Route.Num() ? 1.f : 0.f;
	}
	const double Left = FVector2D::Distance(FVector2D(GetActorLocation()), Route[NextPoint]);
	const double Leg = NextPoint > 0 ? FVector2D::Distance(Route[NextPoint - 1], Route[NextPoint]) : 0.0;
	return static_cast<float>(FMath::Clamp((LegsDone + FMath::Max(0.0, Leg - Left)) / RouteLength, 0.0, 1.0));
}

void AChaseThief::FightBack(APestiPerson* Player)
{
	Mode = EMode::Fight;
	SetSlipping(false);
	StartFight(Player);
	NextThink = GetWorld()->GetTimeSeconds() + ChaseThief::ThinkMax;
}

void AChaseThief::Slink()
{
	// Up off the ground first, then away.
	BeginAct(Brawl::EAct::GetUp, Brawl::GetUpSeconds);
	Mode = EMode::Slink;
	SetSlipping(true);
}

void AChaseThief::SetSlipping(bool bSlip)
{
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, bSlip ? ECR_Ignore : ECR_Block);
}

void AChaseThief::ShowPurse(bool bShow)
{
	for (UStaticMeshComponent* Part : Purse)
	{
		if (Part)
		{
			Part->DestroyComponent();
		}
	}
	Purse.Reset();
	if (!bShow || !CubeMesh || !Pieces.IsValidIndex(PersonKit::ArmRight) || !Pieces[PersonKit::ArmRight])
	{
		return;
	}
	auto Add = [this](const FVector& At, const FVector& Size, const FLinearColor& Color)
	{
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
		Part->SetupAttachment(Pieces[PersonKit::ArmRight]);
		Part->SetStaticMesh(CubeMesh);
		Part->SetRelativeLocation(At);
		Part->SetRelativeScale3D(Size / 100.0);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCanEverAffectNavigation(false);
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Paint->SetVectorParameterValue(TEXT("Color"), Color);
			Part->SetMaterial(0, Paint);
		}
		Part->RegisterComponent();
		Purse.Add(Part);
	};
	Add(ChaseThief::PurseAt, ChaseThief::PurseSize, ChaseThief::Leather);
	Add(ChaseThief::ClaspAt, ChaseThief::ClaspSize, ChaseThief::Brass);
}

void AChaseThief::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	switch (Mode)
	{
	case EMode::Sneak:
		if (const AActor* Target = Mark.Get())
		{
			if (!IsAtMark())
			{
				Steer(FVector2D(Target->GetActorLocation()), ChaseThief::SneakSpeed);
			}
		}
		break;
	case EMode::Run:
	case EMode::Slink:
		TickRun(DeltaSeconds);
		break;
	case EMode::Fight:
		TickFight();
		break;
	default:
		break;
	}
}

bool AChaseThief::NextPointAhead()
{
	const FVector2D Here(GetActorLocation());
	while (Route.IsValidIndex(NextPoint))
	{
		const FVector2D Point = Route[NextPoint];
		const FVector2D From = NextPoint > 0 ? Route[NextPoint - 1] : Here;
		const FVector2D Leg = (Point - From).GetSafeNormal();
		// Reached it, or past it along its leg (a vault may carry him by).
		if (FVector2D::Distance(Here, Point) > ChaseThief::PointReach && FVector2D::DotProduct(Here - Point, Leg) <= 0.0)
		{
			return true;
		}
		if (NextPoint > 0)
		{
			LegsDone += FVector2D::Distance(From, Point);
		}
		++NextPoint;
	}
	return false;
}

void AChaseThief::TickRun(float DeltaSeconds)
{
	// Getting up to slink off: once on his feet, fists down and away.
	if (Mode == EMode::Slink && Act != Brawl::EAct::None)
	{
		if (Act != Brawl::EAct::Stance)
		{
			return;
		}
		EndFight();
	}
	// Over or under something, or knocked down: nothing to steer.
	if (IsFreeRunning() || Act != Brawl::EAct::None)
	{
		return;
	}
	if (!NextPointAhead())
	{
		Mode = EMode::Away;
		GetCharacterMovement()->StopMovementImmediately();
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	float Speed = ChaseThief::SlinkSpeed;
	if (Mode == EMode::Run)
	{
		const APestiPerson* After = Chaser.Get();
		const double Behind = After ? FVector::Dist2D(After->GetActorLocation(), GetActorLocation()) : ChaseThief::Far;
		Speed = Now - RunStart < ChaseThief::BurstSeconds ? ChaseThief::BurstSpeed
			: Behind < ChaseThief::Close ? ChaseThief::CloseSpeed
			: Behind > ChaseThief::VeryFar ? ChaseThief::VeryFarSpeed
			: Behind > ChaseThief::Far ? ChaseThief::FarSpeed
			: ChaseThief::Pace;
		if (RouteShare() > ChaseThief::TiredFrom)
		{
			Speed -= ChaseThief::TiredSlower;
		}
	}
	if (Now < StumbleUntil)
	{
		Speed *= FreeRun::StumbleSlow;
	}
	Steer(Route[NextPoint], Speed);

	// Stuck on something: a hop to get over it.
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement->IsMovingOnGround() && GetVelocity().Size2D() < ChaseThief::StuckSpeed)
	{
		if (StuckSince < 0.0)
		{
			StuckSince = Now;
		}
		else if (Now - StuckSince > ChaseThief::StuckSeconds)
		{
			Jump();
			StuckSince = -1.0;
		}
	}
	else
	{
		StuckSince = -1.0;
	}
}

void AChaseThief::Steer(const FVector2D& To, float Speed)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = Speed;
	const FVector Way = FVector(To - FVector2D(GetActorLocation()), 0.0).GetSafeNormal();
	AddMovementInput(Way, 1.f);

	// Something in the way: over it or under it, at about the right moment,
	// or now and then too late, with a stumble after.
	FreeRun::FChance Chance;
	const bool bChance = Movement->IsMovingOnGround() && GetVelocity().Size2D() >= ChaseThief::SneakSpeed * 2.f
		&& FreeRun::FindMove(GetWorld(), this, GetActorLocation(), GetVelocity(),
			GetCapsuleComponent()->GetScaledCapsuleRadius(), GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), Chance);
	if (!bChance)
	{
		bDecided = false;
		return;
	}
	if (!bDecided)
	{
		bDecided = true;
		bFumble = FMath::FRand() < ChaseThief::FumbleChance;
	}
	if (bFumble ? Chance.Face < FreeRun::Late : Chance.Face <= FreeRun::GoodTo * ChaseThief::GoFrom)
	{
		bDecided = false;
		StartMove(Chance);
	}
}

void AChaseThief::TickFight()
{
	using namespace Brawl;
	APestiPerson* Player = Foe.Get();
	if (!Player || !IsFighting())
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	const FVector Between = Player->GetActorLocation() - GetActorLocation();
	const double Apart = Between.Size2D();
	const FVector Toward = Between.GetSafeNormal2D();

	if (Act == EAct::Guard && Now >= GuardUntil)
	{
		SetGuard(false);
	}

	// Holding him: a knee or two, then throw him off.
	if (Act == EAct::Holding)
	{
		if (Now >= NextThink)
		{
			NextThink = Now + FMath::FRandRange(ChaseThief::ThinkMin, ChaseThief::ThinkMax) * 0.6f;
			if (FMath::FRand() < 0.55f && Knee())
			{
				return;
			}
			Shove(GetActorForwardVector());
		}
		return;
	}
	if (!IsFree(Act))
	{
		return;
	}

	// Stepping: in close enough to hit, back when crowded or tired, round him otherwise.
	FVector Step = FVector::ZeroVector;
	if (Now < BackOffUntil)
	{
		Step = -Toward;
	}
	else if (Apart > Reach - 15.0)
	{
		Step = Toward;
	}
	else if (Apart < ChaseThief::TooClose)
	{
		Step = -Toward;
	}
	else
	{
		Step = FVector(-Toward.Y, Toward.X, 0.0) * CircleSide * ChaseThief::CircleShare;
	}
	AddMovementInput(Step, 1.f);
	if (Now >= CircleFlipAt)
	{
		CircleSide = -CircleSide;
		CircleFlipAt = Now + FMath::FRandRange(ChaseThief::CircleMin, ChaseThief::CircleMax);
	}

	// His heavy swing or grab coming: cover up, step back, or now and then counter it.
	const float Left = Player->GetWindUpLeft();
	if (Left < 0.f)
	{
		bSawWindUp = false;
	}
	else if (!bSawWindUp && Apart <= Reach + 40.0 && Left <= CounterWindow)
	{
		bSawWindUp = true;
		const float Pick = FMath::FRand();
		if (Pick < 0.2f && Counter())
		{
			return;
		}
		if (Pick < 0.55f)
		{
			BackOffUntil = Now + 0.6;
		}
		else
		{
			GuardUntil = Now + 0.8;
			SetGuard(true);
		}
		return;
	}

	if (SecondJabAt > 0.0 && Now >= SecondJabAt)
	{
		SecondJabAt = -1.0;
		Jab();
		return;
	}
	if (Now < NextThink)
	{
		return;
	}
	NextThink = Now + FMath::FRandRange(ChaseThief::ThinkMin, ChaseThief::ThinkMax);

	// Short of breath: cover up and back off.
	if (Stamina < ChaseThief::LowBreath)
	{
		GuardUntil = Now + 1.0;
		BackOffUntil = Now + 0.8;
		SetGuard(true);
		return;
	}
	// He is punching: sometimes cover up.
	if (Player->GetAct() == EAct::Jab && FMath::FRand() < 0.35f)
	{
		GuardUntil = Now + 0.6;
		SetGuard(true);
		return;
	}
	if (Apart > Reach)
	{
		return;
	}
	const float Pick = FMath::FRand();
	if (Pick < 0.5f)
	{
		if (Jab() && FMath::FRand() < 0.4f)
		{
			SecondJabAt = Now + ChaseThief::SecondJabAfter;
		}
	}
	else if (Pick < 0.75f)
	{
		HeavySwing();
	}
	else if (Pick < 0.88f)
	{
		Grab(true);
	}
	else
	{
		GuardUntil = Now + 0.7;
		SetGuard(true);
	}
}
