// Fill out your copyright notice in the Description page of Project Settings.

#include "Ruffian.h"

#include "BoxingYardModel.h"
#include "GarayModel.h"

#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

namespace Ruffian
{
	// Waiting his turn: he heads for his spot on the ring round the player
	// and stands there once this close (cm).
	constexpr double SpotReach = 60.0;

	// Fighting, as the pickpocket: he keeps about this far off (cm) and
	// steps in to hit; he thinks again after this long (s), circles this
	// way for this long (s).
	constexpr double TooClose = 95.0;
	constexpr float ThinkMin = 0.55f;
	constexpr float ThinkMax = 1.2f;
	constexpr float CircleMin = 1.5f;
	constexpr float CircleMax = 3.f;
	constexpr float CircleShare = 0.6f;
	constexpr double SecondJabAfter = 0.42;
	constexpr float LowBreath = 25.f;

	// Running off: this fast (cm/s), for this long before he is gone (s).
	constexpr float FleeSpeed = 640.f;
	constexpr double FleeSeconds = 7.0;
	// Stuck once going (after this long, s): he turns this far (degrees) and tries again.
	constexpr double FleeStart = 0.5;
	constexpr double FleeTurn = 70.0;
}

ARuffian::ARuffian()
{
	// He fights with nobody at the controls.
	AutoPossessAI = EAutoPossessAI::Disabled;
	GetCharacterMovement()->bRunPhysicsWithNoController = true;
}

void ARuffian::BeginPlay()
{
	Health = GetMaxHealth();
	Super::BeginPlay();
	// No camera of his own is ever looked through.
	if (CameraArm)
	{
		CameraArm->bDoCollisionTest = false;
	}
}

FString ARuffian::GetSpeakerName() const
{
	switch (Kind)
	{
	case ERuffian::Svihak:
		return TEXT("Svih\u00e1k");
	case ERuffian::Csontos:
		return TEXT("Csontos");
	case ERuffian::Pottom:
		return TEXT("P\u00f6tt\u00f6m");
	case ERuffian::Hermann:
		return TEXT("Friedrich Hermann");
	case ERuffian::Geza:
		return TEXT("Friedrich G\u00e9za");
	case ERuffian::ClerkOne:
		return TEXT("Lajos, a seg\u00e9d");
	case ERuffian::ClerkTwo:
		return TEXT("Bandi, a seg\u00e9d");
	case ERuffian::BoxBandi:
		return TEXT("Bandi");
	case ERuffian::BoxJeno:
		return TEXT("F\u00fcrge Jen\u0151");
	case ERuffian::BoxMazsas:
		return TEXT("M\u00e1zs\u00e1s");
	case ERuffian::BoxGyuri:
		return TEXT("Sz\u0151ke Gyuri");
	case ERuffian::BoxVasoklu:
		return TEXT("Vas\u00f6kl\u0171");
	case ERuffian::Passerby:
		return TEXT("J\u00e1r\u00f3kel\u0151");
	case ERuffian::Constable:
		return TEXT("Rend\u0151r");
	default:
		return TEXT("Bika");
	}
}

FString ARuffian::GetFightName() const
{
	return GetSpeakerName().ToUpper();
}

float ARuffian::GetMaxHealth() const
{
	// The boss takes the most beating, the little one the least.
	switch (Kind)
	{
	case ERuffian::Bika:
		return 70.f;
	case ERuffian::Svihak:
	case ERuffian::Csontos:
		// Made a little easier after Daniel's test: two of them at once was a lot.
		return 45.f;
	case ERuffian::Pottom:
		return 45.f;
	case ERuffian::Hermann:
		// The old man goes down at the first blow.
		return 1.f;
	// Made easier after Daniel's test of mission 5.
	case ERuffian::Geza:
		return 50.f;
	case ERuffian::ClerkOne:
	case ERuffian::ClerkTwo:
		return 35.f;
	// The ring: each harder than the one before.
	case ERuffian::BoxBandi:
		return 45.f;
	case ERuffian::BoxJeno:
		return 50.f;
	case ERuffian::BoxMazsas:
		return 80.f;
	case ERuffian::BoxGyuri:
		return 70.f;
	case ERuffian::BoxVasoklu:
		return 100.f;
	// Someone off the street: no fighter.
	case ERuffian::Passerby:
		return 30.f;
	// A constable: used to a scuffle, but no boxer.
	case ERuffian::Constable:
		return 55.f;
	default:
		return 60.f;
	}
}

float ARuffian::Quickness() const
{
	// Little Jenő and the champion think and strike faster.
	switch (Kind)
	{
	case ERuffian::BoxJeno:
		return 1.5f;
	case ERuffian::BoxVasoklu:
		return 1.3f;
	case ERuffian::BoxMazsas:
	case ERuffian::Passerby:
		return 0.85f;
	default:
		return 1.f;
	}
}

float ARuffian::GetHealthShare() const
{
	return FMath::Clamp(Health / GetMaxHealth(), 0.f, 1.f);
}

float ARuffian::GetVoicePitch() const
{
	switch (Kind)
	{
	case ERuffian::Bika:
		return 88.f;
	case ERuffian::Svihak:
		return 118.f;
	case ERuffian::Csontos:
		return 108.f;
	case ERuffian::Hermann:
		return 84.f;
	case ERuffian::Geza:
		return 104.f;
	case ERuffian::ClerkOne:
		return 112.f;
	case ERuffian::ClerkTwo:
		return 126.f;
	case ERuffian::BoxBandi:
		return 115.f;
	case ERuffian::BoxJeno:
		return 132.f;
	case ERuffian::BoxMazsas:
		return 80.f;
	case ERuffian::BoxGyuri:
		return 110.f;
	case ERuffian::BoxVasoklu:
		return 92.f;
	case ERuffian::Passerby:
		return StreetDress.bWoman ? 205.f : 112.f;
	case ERuffian::Constable:
		return 98.f;
	default:
		return 142.f;
	}
}

float ARuffian::GetBlowStrength() const
{
	// Bandi pulls his punches sparring; the porter and the champion hit hard.
	switch (Kind)
	{
	case ERuffian::BoxBandi:
		return 0.7f;
	case ERuffian::BoxMazsas:
		return 1.35f;
	case ERuffian::BoxVasoklu:
		return 1.25f;
	case ERuffian::Passerby:
		return 0.8f;
	// The truncheon.
	case ERuffian::Constable:
		return 1.3f;
	default:
		return 1.f;
	}
}

PersonKit::FOutfit ARuffian::GetDress() const
{
	// The people of Hermann's shop dress as GarayModel has them.
	switch (Kind)
	{
	case ERuffian::Hermann:
		return GarayModel::Hermann();
	case ERuffian::Geza:
		return GarayModel::Geza();
	case ERuffian::ClerkOne:
		return GarayModel::Clerk(0);
	case ERuffian::ClerkTwo:
		return GarayModel::Clerk(1);
	case ERuffian::BoxBandi:
		return BoxingYardModel::Bandi();
	case ERuffian::BoxJeno:
		return BoxingYardModel::Rival(1);
	case ERuffian::BoxMazsas:
		return BoxingYardModel::Rival(2);
	case ERuffian::BoxGyuri:
		return BoxingYardModel::Rival(3);
	case ERuffian::BoxVasoklu:
		return BoxingYardModel::Rival(4);
	case ERuffian::Passerby:
	case ERuffian::Constable:
		return StreetDress;
	default:
		break;
	}
	PersonKit::FOutfit O;
	O.bTie = false;
	switch (Kind)
	{
	case ERuffian::Bika:
		// The boss: big, a bowler, a black jacket and a red neckerchief, boots.
		O.Height = 186.f;
		O.Build = 1.25f;
		O.Hat = PersonKit::EHat::Bowler;
		O.Top = PersonKit::ETop::Jacket;
		O.bScarf = true;
		O.bMoustache = true;
		O.bBoots = true;
		O.Coat = FLinearColor(0.10f, 0.09f, 0.08f);
		O.HatColor = FLinearColor(0.05f, 0.05f, 0.05f);
		O.Scarf = FLinearColor(0.50f, 0.08f, 0.06f);
		O.Trousers = FLinearColor(0.12f, 0.11f, 0.10f);
		O.Shirt = FLinearColor(0.70f, 0.66f, 0.58f);
		O.Hair = FLinearColor(0.06f, 0.05f, 0.04f);
		O.Skin = FLinearColor(0.70f, 0.47f, 0.36f);
		break;
	case ERuffian::Svihak:
		// The dandy: a light grey suit, a fedora, a yellow tie, tan shoes.
		O.Height = 179.f;
		O.Build = 0.95f;
		O.Hat = PersonKit::EHat::Fedora;
		O.Top = PersonKit::ETop::Jacket;
		O.bTie = true;
		O.bMoustache = true;
		O.Tie = FLinearColor(0.75f, 0.55f, 0.10f);
		O.Coat = FLinearColor(0.42f, 0.40f, 0.36f);
		O.HatColor = FLinearColor(0.30f, 0.27f, 0.22f);
		O.HatBand = FLinearColor(0.08f, 0.06f, 0.05f);
		O.Trousers = FLinearColor(0.36f, 0.34f, 0.30f);
		O.Shoes = FLinearColor(0.32f, 0.18f, 0.08f);
		break;
	case ERuffian::Csontos:
		// The beanpole: tall and thin, a flat cap, waistcoat over his shirt, a blue scarf.
		O.Height = 190.f;
		O.Build = 0.88f;
		O.Hat = PersonKit::EHat::FlatCap;
		O.Top = PersonKit::ETop::Waistcoat;
		O.bScarf = true;
		O.bBoots = true;
		O.Waistcoat = FLinearColor(0.20f, 0.15f, 0.10f);
		O.Shirt = FLinearColor(0.58f, 0.60f, 0.62f);
		O.Scarf = FLinearColor(0.14f, 0.18f, 0.30f);
		O.HatColor = FLinearColor(0.25f, 0.23f, 0.20f);
		O.Trousers = FLinearColor(0.14f, 0.14f, 0.16f);
		O.Skin = FLinearColor(0.76f, 0.56f, 0.44f);
		break;
	default:
		// Little Pöttöm: short and young, a flat cap, a brown jacket, a mustard scarf.
		O.Height = 164.f;
		O.Build = 0.9f;
		O.Hat = PersonKit::EHat::FlatCap;
		O.Top = PersonKit::ETop::Jacket;
		O.bScarf = true;
		O.Coat = FLinearColor(0.28f, 0.22f, 0.14f);
		O.HatColor = FLinearColor(0.12f, 0.12f, 0.12f);
		O.Scarf = FLinearColor(0.62f, 0.50f, 0.20f);
		O.Trousers = FLinearColor(0.20f, 0.18f, 0.15f);
		O.Shirt = FLinearColor(0.78f, 0.74f, 0.64f);
		O.Hair = FLinearColor(0.30f, 0.20f, 0.10f);
		break;
	}
	return O;
}

void ARuffian::LoseHealth(float Amount)
{
	Health = FMath::Max(0.f, Health - Amount);
	if (Health <= 0.f && Act != Brawl::EAct::Beaten)
	{
		GiveIn();
		Mode = EMode::Idle;
	}
}

void ARuffian::Circle(APestiPerson* Player, const FVector& Spot)
{
	RingSpot = Spot;
	if (Mode == EMode::Circle || IsBeaten() || Mode == EMode::Flee || Mode == EMode::Gone)
	{
		return;
	}
	Mode = EMode::Circle;
	if (!IsFighting())
	{
		StartFight(Player);
	}
}

void ARuffian::Attack(APestiPerson* Player, float Eager)
{
	Eagerness = FMath::Clamp(Eager, 0.3f, 1.f);
	if (Mode == EMode::Attack || IsBeaten() || Mode == EMode::Flee || Mode == EMode::Gone)
	{
		return;
	}
	Mode = EMode::Attack;
	if (!IsFighting())
	{
		StartFight(Player);
	}
	NextThink = GetWorld()->GetTimeSeconds() + Ruffian::ThinkMax / Eagerness;
}

void ARuffian::Flee(APestiPerson* Player)
{
	if (Mode == EMode::Flee || Mode == EMode::Gone)
	{
		return;
	}
	EndFight();
	Mode = EMode::Flee;
	const FVector Away = Player ? GetActorLocation() - Player->GetActorLocation() : GetActorForwardVector();
	FleeWay = Away.GetSafeNormal2D();
	if (FleeWay.IsNearlyZero())
	{
		FleeWay = GetActorForwardVector();
	}
	FleeUntil = GetWorld()->GetTimeSeconds() + Ruffian::FleeSeconds;
	GetCharacterMovement()->MaxWalkSpeed = Ruffian::FleeSpeed;
	// Running, he slips past everyone.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
}

void ARuffian::StandAgain(const FVector& At, double Yaw)
{
	if (Act != Brawl::EAct::None)
	{
		EndFight();
	}
	Mode = EMode::Idle;
	Health = GetMaxHealth();
	Stamina = Brawl::MaxStamina;
	SecondJabAt = -1.0;
	bSawWindUp = false;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	SetActorLocationAndRotation(At, FRotator(0.0, Yaw, 0.0), false, nullptr, ETeleportType::TeleportPhysics);
	GetCharacterMovement()->StopMovementImmediately();
}

void ARuffian::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	switch (Mode)
	{
	case EMode::Circle:
		TickCircle();
		break;
	case EMode::Attack:
		TickAttack();
		break;
	case EMode::Flee:
		TickFlee();
		break;
	default:
		break;
	}
}

void ARuffian::TickCircle()
{
	// Fists up, facing him (the brawl turns him), stepping to his spot on the ring.
	if (!IsFighting() || !Brawl::IsFree(Act))
	{
		return;
	}
	if (Act == Brawl::EAct::Guard)
	{
		SetGuard(false);
	}
	const FVector ToSpot = FVector(RingSpot.X - GetActorLocation().X, RingSpot.Y - GetActorLocation().Y, 0.0);
	if (ToSpot.Size2D() > Ruffian::SpotReach)
	{
		AddMovementInput(ToSpot.GetSafeNormal2D(), 1.f);
	}
}

void ARuffian::TickFlee()
{
	if (GetWorld()->GetTimeSeconds() >= FleeUntil)
	{
		Mode = EMode::Gone;
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
		GetCharacterMovement()->StopMovementImmediately();
		return;
	}
	AddMovementInput(FleeWay, 1.f);
	// Up against something once he has got going: off round it.
	const bool bGoing = GetWorld()->GetTimeSeconds() > FleeUntil - Ruffian::FleeSeconds + Ruffian::FleeStart;
	if (bGoing && GetVelocity().Size2D() < Ruffian::FleeSpeed * 0.25f)
	{
		FleeWay = FRotator(0.0, Ruffian::FleeTurn, 0.0).RotateVector(FleeWay);
	}
}

void ARuffian::TickAttack()
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
			NextThink = Now + FMath::FRandRange(Ruffian::ThinkMin, Ruffian::ThinkMax) * 0.6f;
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
	else if (Apart < Ruffian::TooClose)
	{
		Step = -Toward;
	}
	else
	{
		Step = FVector(-Toward.Y, Toward.X, 0.0) * CircleSide * Ruffian::CircleShare;
	}
	AddMovementInput(Step, 1.f);
	if (Now >= CircleFlipAt)
	{
		CircleSide = -CircleSide;
		CircleFlipAt = Now + FMath::FRandRange(Ruffian::CircleMin, Ruffian::CircleMax);
	}

	// His heavy swing or grab coming at me: cover up, step back, or now and then counter it.
	const float Left = Player->GetFoe() == this ? Player->GetWindUpLeft() : -1.f;
	if (Left < 0.f)
	{
		bSawWindUp = false;
	}
	else if (!bSawWindUp && Apart <= Reach + 40.0 && Left <= CounterWindow)
	{
		bSawWindUp = true;
		const float Pick = FMath::FRand();
		if (Pick < 0.15f && Counter())
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

	// He is open after a big swing or a finisher: hit him back.
	if (!Player->IsOpenAfterFinisher())
	{
		bSawOpening = false;
	}
	else if (!bSawOpening && Apart <= Reach + 20.0)
	{
		bSawOpening = true;
		if (FMath::FRand() < PunishChance && Jab())
		{
			return;
		}
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
	NextThink = Now + FMath::FRandRange(Ruffian::ThinkMin, Ruffian::ThinkMax) / (Eagerness * Quickness());

	// Short of breath: cover up and back off.
	if (Stamina < Ruffian::LowBreath)
	{
		GuardUntil = Now + 1.0;
		BackOffUntil = Now + 0.8;
		SetGuard(true);
		return;
	}
	// He is punching me: sometimes cover up.
	if (Player->GetFoe() == this && Brawl::IsLightPunch(Player->GetAct()) && FMath::FRand() < (Kind == ERuffian::BoxGyuri ? 0.7f : 0.35f))
	{
		GuardUntil = Now + 0.6;
		SetGuard(true);
		return;
	}
	if (Apart > Reach)
	{
		return;
	}
	// The second man in holds back a little: no grabbing, more feinting.
	const bool bHangBack = Eagerness < 1.f;
	const float Pick = FMath::FRand();
	if (Pick < 0.5f)
	{
		if (Jab() && !bHangBack && FMath::FRand() < 0.4f)
		{
			SecondJabAt = Now + Ruffian::SecondJabAfter;
		}
	}
	else if (Pick < 0.75f)
	{
		HeavySwing();
	}
	else if (Pick < 0.88f && !bHangBack)
	{
		Grab(true);
	}
	else
	{
		GuardUntil = Now + 0.7;
		SetGuard(true);
	}
}
