// Fill out your copyright notice in the Description page of Project Settings.

#include "TrafficCart.h"

#include "Camera/PlayerCameraManager.h"
#include "CarjackedDriver.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "HorseSound.h"
#include "Kismet/GameplayStatics.h"
#include "Pedestrians.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "PolicePatrols.h"
#include "ProceduralMeshComponent.h"
#include "SpillLife.h"
#include "TrafficManager.h"
#include "Witnesses.h"

namespace TrafficCartRide
{
	// The next point is taken once the cart's middle is this close (cm).
	constexpr double ReachRadius = 320.0;
	// Each load's easy pace (cm/s): the coal cart walks, the beer cart jogs, the milk cart trots.
	constexpr float CoalLow = 165.f;
	constexpr float CoalHigh = 215.f;
	constexpr float BeerLow = 240.f;
	constexpr float BeerHigh = 290.f;
	constexpr float MilkLow = 300.f;
	constexpr float MilkHigh = 345.f;
	constexpr float SpeedUp = 140.f;
	constexpr float SlowDown = 380.f;
	// The carter turns the horse this far at most (degrees), this fast, this much for each degree off course.
	constexpr float MaxSteer = 32.f;
	constexpr float SteerRate = 60.f;
	constexpr float SteerGain = 1.2f;
	// The slowest share of the pace in a sharp bend, and before a junction.
	constexpr float CornerShare = 0.5f;
	constexpr float JunctionShare = 0.55f;
	constexpr float JunctionSlowing = 900.f;
	// Looks this far ahead in a strip this wide each side, and stops this far short (from the cart's
	// middle: the horse's nose is nearly three metres in front of it) (cm).
	constexpr float LookAhead = 1600.f;
	constexpr float Corridor = 130.f;
	constexpr float StopGap = 560.f;
	// Held up this long (s), it pushes through for this long, as a stuck car does.
	constexpr float MaxStuck = 9.f;
	constexpr float Squeeze = 2.5f;

	// Hit faster than this (cm/s, about 9 km/h), the horse bolts.
	constexpr float BoltFrom = 260.f;
	constexpr float NudgeFrom = 60.f;
	constexpr double TouchMargin = 40.0;
	constexpr float BumpRest = 1.5f;
	// Bolting: a gallop for this long (s), slowing less in bends, stopping only at the last moment.
	constexpr float BoltSpeed = 840.f;
	constexpr float BoltSeconds = 9.f;
	constexpr float BoltCornerShare = 0.65f;
	constexpr float BoltStopGap = 330.f;
	constexpr float BoltSlowDown = 700.f;
	// What goes over the side: some at the knock, then one now and then, so many at most.
	constexpr int32 SpillsAtKnock = 2;
	constexpr float SpillEvery = 1.6f;
	constexpr int32 MaxSpillsPerBolt = 5;
	// It lands this far out from the side of the bed, and a little more (cm).
	constexpr double SpillOut = 80.0;
	constexpr double SpillScatter = 70.0;
	// A bolting horse knocks down whoever it runs into, faster than this (cm/s).
	constexpr float RunDownFrom = 150.f;
	// Spilt things stay at least this long (s), and go when the camera is this far off (cm);
	// an emptied cart is loaded again when the camera is farther than this.
	constexpr double SpillsLast = 40.0;
	constexpr double SpillsGoneFar = 5000.0;
	constexpr double ReloadFar = 9000.0;

	// The carter's words are shown when the player is this near (cm), for this long (s), as this note.
	constexpr double HearWords = 2500.0;
	constexpr float WordsSeconds = 3.f;
	constexpr int32 WordsKey = 7;
	// Pulled down: the carter lands this far to the side (cm); a policeman this near sees it.
	constexpr double RunnerOut = 160.0;
	constexpr double TheftSeen = 3000.0;
	// The horse is heard within SoundOn and goes quiet past SoundOff; the reins are drawn within
	// ReinsNear; its pieces are not drawn beyond DrawDistance and beyond FarAway are moved only now and then (cm, s).
	constexpr double SoundOn = 4500.0;
	constexpr double SoundOff = 5500.0;
	constexpr double ReinsNear = 3500.0;
	constexpr float DrawDistance = 9000.f;
	constexpr double FarAway = 10000.0;
	constexpr float FarStep = 0.25f;
}

ATrafficCart::ATrafficCart()
{
	// Nobody looks through its camera unless the player takes it.
	CameraArm->bDoCollisionTest = false;
	CameraArm->PrimaryComponentTick.bStartWithTickEnabled = false;
	bSoundWhenNear = true;
	DrawDistance = TrafficCartRide::DrawDistance;
}

void ATrafficCart::PickLoad(FRandomStream& Random)
{
	using HorseCartModel::ECoat;
	using HorseCartModel::ELoad;
	const int32 Which = Random.RandRange(0, 2);
	CartLoad = Which == 0 ? ELoad::Milk : Which == 1 ? ELoad::Coal : ELoad::Beer;
	HorseCoat = static_cast<ECoat>(Random.RandRange(0, static_cast<int32>(ECoat::Count) - 1));
	bBlaze = Random.FRand() < 0.5f;
	DriverLook = HorseCartModel::CarterLook(CartLoad, Random.RandRange(0, 11));
	bOwnDriverLook = true;
	Cruise = CartLoad == ELoad::Coal ? Random.FRandRange(TrafficCartRide::CoalLow, TrafficCartRide::CoalHigh)
		: CartLoad == ELoad::Beer ? Random.FRandRange(TrafficCartRide::BeerLow, TrafficCartRide::BeerHigh)
		: Random.FRandRange(TrafficCartRide::MilkLow, TrafficCartRide::MilkHigh);
}

void ATrafficCart::BeginPlay()
{
	Super::BeginPlay();
	// The carter on the bench, reins in hand.
	DriverMesh->SetVisibility(true);
	UpdateReins();
}

void ATrafficCart::StartOnLane(ATrafficManager* InManager, int32 InLaneIndex, int32 InPointIndex)
{
	Manager = InManager;
	LaneIndex = InLaneIndex;
	NextLaneIndex = Manager->PickNextLane(LaneIndex);
	PointIndex = InPointIndex;
	Target = Manager->GetLane(LaneIndex).Points[FMath::Clamp(PointIndex, 0, Manager->GetLane(LaneIndex).Points.Num() - 1)];
	Speed = Cruise * 0.6f;
}

void ATrafficCart::Tick(float DeltaSeconds)
{
	TidySpills();
	if (!Manager)
	{
		// Taken by the player: an ordinary cart from now on.
		Super::Tick(DeltaSeconds);
		return;
	}
	APawn::Tick(DeltaSeconds);

	BumpCooldown = FMath::Max(0.f, BumpCooldown - DeltaSeconds);
	FollowLane(DeltaSeconds);

	// Pushing through after a long wait, it slips past what holds it up.
	const bool bSqueezing = SqueezeTime > 0.f;
	if (bSqueezing != (CollisionBox->GetCollisionEnabled() == ECollisionEnabled::NoCollision))
	{
		CollisionBox->SetCollisionEnabled(bSqueezing ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	}
	Move(DeltaSeconds);

	CheckBumps();
	if (BoltTime > 0.f)
	{
		WhileBolting(DeltaSeconds);
	}
	UpdateNearness(DeltaSeconds);
}

void ATrafficCart::FollowLane(float DeltaSeconds)
{
	const FTrafficLane* Lane = &Manager->GetLane(LaneIndex);
	const FVector Here = GetActorLocation();
	for (int32 Guard = 0; Guard < 4 && FVector::Dist2D(Here, Target) < TrafficCartRide::ReachRadius; ++Guard)
	{
		if (++PointIndex >= Lane->Points.Num())
		{
			if (NextLaneIndex == INDEX_NONE)
			{
				PointIndex = Lane->Points.Num() - 1;
				break;
			}
			LaneIndex = NextLaneIndex;
			NextLaneIndex = Manager->PickNextLane(LaneIndex);
			Lane = &Manager->GetLane(LaneIndex);
			PointIndex = 0;
		}
		Target = Lane->Points[PointIndex];
	}

	// The carter turns the horse's head towards the point; the cart follows.
	const FVector ToTarget = (Target - Here).GetSafeNormal2D();
	const float TurnNeeded = static_cast<float>(FRotator::NormalizeAxis(ToTarget.Rotation().Yaw - GetActorRotation().Yaw));
	const float WantSteer = FMath::Clamp(TurnNeeded * TrafficCartRide::SteerGain, -TrafficCartRide::MaxSteer, TrafficCartRide::MaxSteer);
	SteerAngle = FMath::FInterpConstantTo(SteerAngle, WantSteer, DeltaSeconds, TrafficCartRide::SteerRate);

	// Its own pace, slower in bends and coming up to a junction; a bolting horse hardly slows at all.
	const bool bBolting = BoltTime > 0.f;
	const float Bend = FMath::Clamp(FMath::Abs(TurnNeeded) / 90.f, 0.f, 1.f);
	float Wanted = bBolting
		? TrafficCartRide::BoltSpeed * FMath::Lerp(1.f, TrafficCartRide::BoltCornerShare, Bend)
		: Cruise * FMath::Lerp(1.f, TrafficCartRide::CornerShare, Bend);
	const float ToJunction = static_cast<float>(FVector::Dist2D(Here, Lane->Points.Last()));
	if (!bBolting && Manager->IsJunction(Lane->ToNode) && ToJunction < TrafficCartRide::JunctionSlowing)
	{
		Wanted *= FMath::Lerp(TrafficCartRide::JunctionShare, 1.f, ToJunction / TrafficCartRide::JunctionSlowing);
	}

	// Stopping for what is in front, unless it has waited too long already.
	SqueezeTime = FMath::Max(0.f, SqueezeTime - DeltaSeconds);
	if (SqueezeTime <= 0.f)
	{
		const float Gap = bBolting ? TrafficCartRide::BoltStopGap : TrafficCartRide::StopGap;
		const float Clear = Manager->ClearDistanceAhead(this, TrafficCartRide::LookAhead, false, TrafficCartRide::Corridor);
		Wanted *= FMath::Clamp((Clear - Gap) / (TrafficCartRide::LookAhead - Gap), 0.f, 1.f);
		StuckTime = Wanted < 15.f ? StuckTime + DeltaSeconds : 0.f;
		if (StuckTime > TrafficCartRide::MaxStuck)
		{
			StuckTime = 0.f;
			SqueezeTime = TrafficCartRide::Squeeze;
		}
	}
	const float Rate = Wanted >= Speed ? TrafficCartRide::SpeedUp : bBolting ? TrafficCartRide::BoltSlowDown : TrafficCartRide::SlowDown;
	Speed = FMath::FInterpConstantTo(Speed, Wanted, DeltaSeconds, bBolting && Wanted >= Speed ? Rate * 2.f : Rate);
	Pace = Speed < 12.f ? 0 : Speed < 240.f ? 1 : Speed < 450.f ? 2 : Speed < 700.f ? 3 : 4;
}

bool ATrafficCart::IsTouching(const APawn* Other, double Margin) const
{
	// The two boxes seen from above: apart if any of their four sides' directions separates them.
	FVector OtherHalf(40.0, 40.0, 90.0);
	if (const UBoxComponent* Box = Cast<UBoxComponent>(Other->GetRootComponent()))
	{
		OtherHalf = Box->GetScaledBoxExtent();
	}
	else if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Other->GetRootComponent()))
	{
		OtherHalf = FVector(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight());
	}
	const FVector OwnHalf = CollisionBox->GetScaledBoxExtent();
	const FVector OwnX = GetActorForwardVector().GetSafeNormal2D();
	const FVector OwnY(-OwnX.Y, OwnX.X, 0.0);
	const FVector OtherX = Other->GetActorForwardVector().GetSafeNormal2D();
	const FVector OtherY(-OtherX.Y, OtherX.X, 0.0);
	const FVector Between = (Other->GetActorLocation() - GetActorLocation()) * FVector(1.0, 1.0, 0.0);
	for (const FVector& Axis : { OwnX, OwnY, OtherX, OtherY })
	{
		const double OwnReach = OwnHalf.X * FMath::Abs(FVector::DotProduct(OwnX, Axis)) + OwnHalf.Y * FMath::Abs(FVector::DotProduct(OwnY, Axis));
		const double OtherReach = OtherHalf.X * FMath::Abs(FVector::DotProduct(OtherX, Axis)) + OtherHalf.Y * FMath::Abs(FVector::DotProduct(OtherY, Axis));
		if (FMath::Abs(FVector::DotProduct(Between, Axis)) > OwnReach + OtherReach + Margin)
		{
			return false;
		}
	}
	return FMath::Abs(Other->GetActorLocation().Z - GetActorLocation().Z) < OwnHalf.Z + OtherHalf.Z + Margin;
}

void ATrafficCart::CheckBumps()
{
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player || Player == this || Player->GetAttachParentActor())
	{
		LastPlayerVelocity = FVector::ZeroVector;
		return;
	}
	const FVector PlayerVelocity = Player->GetVelocity();
	const bool bOnFoot = Player->IsA<APestiPerson>();
	if (BumpCooldown <= 0.f && IsTouching(Player, TrafficCartRide::TouchMargin))
	{
		// How hard: the faster of this frame and the last, as the crash may have stopped him already.
		const float Impact = static_cast<float>(FMath::Max((PlayerVelocity - GetVelocity()).Size2D(), (LastPlayerVelocity - GetVelocity()).Size2D()));
		if (bOnFoot)
		{
			// Walking into it does nothing; the horse running into him knocks him down.
			if (FMath::Abs(Speed) > TrafficCartRide::RunDownFrom)
			{
				Cast<APestiPerson>(Player)->HitByVehicle(FMath::Abs(Speed), GetActorForwardVector());
				BumpCooldown = TrafficCartRide::BumpRest;
			}
		}
		else if (Impact > TrafficCartRide::BoltFrom)
		{
			Bolt();
			for (int32 i = 0; i < TrafficCartRide::SpillsAtKnock; ++i)
			{
				SpillOne();
			}
			BumpCooldown = TrafficCartRide::BumpRest;
		}
		else if (Impact > TrafficCartRide::NudgeFrom)
		{
			Sound->Snort();
			if (BoltTime <= 0.f)
			{
				CallOut(TEXT("\u201eH\u00e9, vigy\u00e1zzon m\u00e1r! Megijeszti a lovat!\u201d   (Hey, watch it! You'll scare the horse!)"));
			}
			BumpCooldown = TrafficCartRide::BumpRest;
		}
	}
	LastPlayerVelocity = PlayerVelocity;
}

void ATrafficCart::Bolt()
{
	if (BoltTime <= 0.f)
	{
		SpilledThisBolt = 0;
		if (!Sound->IsPlaying())
		{
			Sound->Start();
		}
		Sound->Whinny();
		CallOut(FMath::RandBool()
			? TEXT("\u201eH\u00f3! H\u00f3, te! \u00c1llj meg!\u201d   (Whoa! Whoa there! Stop!)")
			: TEXT("\u201eJ\u00e9zusm\u00e1ria, elragadt a l\u00f3!\u201d   (Good Lord, the horse has bolted!)"));
	}
	BoltTime = TrafficCartRide::BoltSeconds;
	SpillClock = TrafficCartRide::SpillEvery;
	SqueezeTime = 0.f;
	StuckTime = 0.f;
}

void ATrafficCart::WhileBolting(float DeltaSeconds)
{
	BoltTime -= DeltaSeconds;
	SpillClock -= DeltaSeconds;
	if (SpillClock <= 0.f)
	{
		SpillClock = TrafficCartRide::SpillEvery * FMath::FRandRange(0.7f, 1.5f);
		SpillOne();
	}
	if (BoltTime <= 0.f)
	{
		// Run out of breath, he lets himself be pulled up.
		BoltTime = 0.f;
		Sound->Snort();
		CallOut(TEXT("\u201eNa, na... csak lassan, te bolond.\u201d   (There, there... easy now, you fool.)"));
	}
}

void ATrafficCart::SpillOne()
{
	using namespace HorseCartModel;
	if (SpilledThisBolt >= TrafficCartRide::MaxSpillsPerBolt)
	{
		return;
	}
	// The top of the load goes first.
	int32 Index = INDEX_NONE;
	for (int32 i = LoadMeshes.Num() - 1; i >= 0; --i)
	{
		if (LoadMeshes[i] && LoadMeshes[i]->IsVisible())
		{
			Index = i;
			break;
		}
	}
	if (Index == INDEX_NONE)
	{
		return;
	}
	++SpilledThisBolt;
	LoadMeshes[Index]->SetVisibility(false);
	// Passers-by stop to look at the mess.
	APedestrians::Gather(this, GetActorLocation(), 25.f, false);

	// Over the side it stood on (the middle row either way), onto the road behind.
	const FVector Spot = LoadSpot(CartLoad, Index);
	const double Side = Spot.Y < -1.0 ? -1.0 : Spot.Y > 1.0 ? 1.0 : (FMath::RandBool() ? -1.0 : 1.0);
	const FVector Out(Spot.X - FMath::FRandRange(0.0, 60.0), Side * (BedHalfWidth + TrafficCartRide::SpillOut + FMath::FRandRange(0.0, TrafficCartRide::SpillScatter)), BedTop);
	FVector At = Rig->GetComponentTransform().TransformPosition(Out);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TrafficCartSpill), false, this);
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, At + FVector(0.0, 0.0, 100.0), At - FVector(0.0, 0.0, 600.0), ECC_Visibility, Params))
	{
		At = Hit.ImpactPoint;
	}
	else
	{
		At.Z = GetActorLocation().Z - CollisionBox->GetScaledBoxExtent().Z;
	}
	const double ItemYaw = FMath::FRandRange(0.0, 360.0);

	// The thing itself, lying where it fell, and what ran out of it.
	PersonKit::FPiece Item;
	BuildLoadItem(Item, CartLoad, Index);
	UProceduralMeshComponent* Lying = MakeMesh(Rig.Get(), Item, FVector::ZeroVector);
	Lying->SetAbsolute(true, true, true);
	Lying->SetWorldLocationAndRotation(At + FVector(0.0, 0.0, LyingLift(CartLoad)), FRotator(0.0, ItemYaw, LyingRoll(CartLoad)));
	PersonKit::FPiece Mess;
	BuildSpill(Mess, CartLoad, Index);
	UProceduralMeshComponent* Puddle = MakeMesh(Rig.Get(), Mess, FVector::ZeroVector);
	Puddle->SetAbsolute(true, true, true);
	Puddle->SetCastShadow(false);
	Puddle->SetWorldLocationAndRotation(At + FVector(0.0, 0.0, 0.5), FRotator(0.0, ItemYaw, 0.0));
	const double Now = GetWorld()->GetTimeSeconds();
	for (UProceduralMeshComponent* Piece : { Lying, Puddle })
	{
		Spilled.Add(Piece);
		SpilledAt.Add(Now);
	}
	// Cats come for the milk, kids for the coal, and a barrel rolls off.
	ASpillLife::Spilled(this, CartLoad, Lying, Puddle, At, FVector2D(GetActorForwardVector()));
}

void ATrafficCart::TidySpills()
{
	const APlayerCameraManager* Viewer = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!Viewer)
	{
		return;
	}
	const FVector Eye = Viewer->GetCameraLocation();
	const double Now = GetWorld()->GetTimeSeconds();
	for (int32 i = Spilled.Num() - 1; i >= 0; --i)
	{
		UProceduralMeshComponent* Piece = Spilled[i];
		if (!Piece)
		{
			Spilled.RemoveAt(i);
			SpilledAt.RemoveAt(i);
			continue;
		}
		if (Now - SpilledAt[i] > TrafficCartRide::SpillsLast && FVector::Dist(Piece->GetComponentLocation(), Eye) > TrafficCartRide::SpillsGoneFar)
		{
			Piece->DestroyComponent();
			Spilled.RemoveAt(i);
			SpilledAt.RemoveAt(i);
		}
	}
	// Back at the yard, as it were: a full load again, out of the player's sight.
	if (Manager && Spilled.Num() == 0 && BoltTime <= 0.f && FVector::Dist(GetActorLocation(), Eye) > TrafficCartRide::ReloadFar)
	{
		for (UProceduralMeshComponent* Piece : LoadMeshes)
		{
			if (Piece && !Piece->IsVisible())
			{
				Piece->SetVisibility(true);
			}
		}
	}
}

void ATrafficCart::UpdateNearness(float DeltaSeconds)
{
	const APlayerCameraManager* Viewer = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const double Far = Viewer ? FVector::Dist(Viewer->GetCameraLocation(), GetActorLocation()) : 0.0;

	// Heard only near the camera, so a dozen carts about the city cost nothing.
	if (Far < TrafficCartRide::SoundOn && !Sound->IsPlaying())
	{
		Sound->Start();
	}
	else if (Far > TrafficCartRide::SoundOff && Sound->IsPlaying())
	{
		Sound->Stop();
	}

	// Far from the camera the horse's legs and the wheels are only moved now and then.
	FarClock += DeltaSeconds;
	bNear = Far <= TrafficCartRide::FarAway;
	if (bNear || FarClock >= TrafficCartRide::FarStep)
	{
		Animate(FarClock);
		FarClock = 0.f;
	}
	if (Far < TrafficCartRide::ReinsNear)
	{
		UpdateReins();
	}
}

void ATrafficCart::CallOut(const FString& Text) const
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Player && FVector::Dist(Player->GetActorLocation(), GetActorLocation()) < TrafficCartRide::HearWords)
	{
		APestiHUD::Show(this, TrafficCartRide::WordsKey, TrafficCartRide::WordsSeconds, FColor::Orange, Text);
	}
}

void ATrafficCart::TakeDriver(APawn* InDriver)
{
	if (Manager)
	{
		// The carter is pulled down off the bench and runs, shouting.
		const FVector Bench = GetBenchLocation();
		const FVector Danger = InDriver ? InDriver->GetActorLocation() : Bench;
		const FVector Right = GetActorRightVector();
		const double Away = FVector::DotProduct(Danger - Bench, Right) > 0.0 ? -1.0 : 1.0;
		FActorSpawnParameters RunnerParams;
		RunnerParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FVector RunnerAt = Bench + Right * (Away * TrafficCartRide::RunnerOut) + FVector(0.0, 0.0, 90.0);
		if (ACarjackedDriver* Runner = GetWorld()->SpawnActor<ACarjackedDriver>(ACarjackedDriver::StaticClass(), RunnerAt, GetActorRotation(), RunnerParams))
		{
			Runner->SetOutfit(DriverLook);
			Runner->RunFrom(Danger);
		}
		CallOut(TEXT("\u201eH\u00e9! A lovam! A kocsim! Tolvaj!\u201d   (Hey! My horse! My cart! Thief!)"));
		if (APolicePatrols::Witnessed(this, Bench, TrafficCartRide::TheftSeen))
		{
			if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
			{
				Game->ReportCrime(1);
			}
		}
		else
		{
			// Nobody from the police saw it: now and then a passer-by runs to tell them.
			AWitnesses::Report(this, Bench, 1);
		}

		// Out of the traffic: an ordinary cart for the player, still loaded.
		Manager = nullptr;
		LaneIndex = INDEX_NONE;
		BoltTime = 0.f;
		SqueezeTime = 0.f;
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CameraArm->bDoCollisionTest = true;
		CameraArm->SetComponentTickEnabled(true);
		bOwnDriverLook = false;
		DressDriver(PersonModel::PlayerOutfit());
		if (!Sound->IsPlaying())
		{
			Sound->Start();
		}
	}
	Super::TakeDriver(InDriver);
}
