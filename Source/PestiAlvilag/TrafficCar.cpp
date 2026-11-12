// Fill out your copyright notice in the Description page of Project Settings.

#include "TrafficCar.h"

#include "CarBody.h"
#include "CarHandling.h"
#include "CarVisuals.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PestiCar.h"
#include "PestiPerson.h"
#include "TrafficManager.h"
#include "UObject/ConstructorHelpers.h"

namespace TrafficCar
{
	// How far ahead a car looks for others (cm).
	constexpr float LookAhead = 2500.f;
	// The car moves on to the next point once it is this close (cm).
	constexpr float ReachRadius = 300.f;
	// Slowest fraction of lane speed in a sharp corner.
	constexpr float CornerSpeedFraction = 0.35f;
	// Cars ask for a junction this far before it (cm)...
	constexpr float JunctionApproach = 1500.f;
	// ...stop this far before it if it is busy...
	constexpr float JunctionStopLine = 700.f;
	// ...and free it once this far past (cm).
	constexpr float JunctionExit = 900.f;
	// Free space needed past a junction before driving into it (cm).
	constexpr float ExitRoom = 900.f;
	// Seconds stuck against another car or a wall before driving through it.
	constexpr float MaxBlockedTime = 3.f;
	// Speed while driving through whatever it is stuck on (cm/s).
	constexpr float UnstickSpeed = 300.f;
	// Signal a turn from this far before the junction (cm)...
	constexpr float SignalDistance = 2500.f;
	// ...if the road bends more than this (degrees).
	constexpr float SignalAngle = 25.f;
	const FLinearColor TrafficatorColor(1.f, 0.35f, 0.f);

	// Passing a horse cart: held up this long (s) by one within CartNear (cm), on a street whose
	// other lane is at least MinGap across (at most MaxGap is used), with this much of the lane left
	// before the junction, and nothing in the other lane for PassLook ahead, PassHalfWidth each side.
	constexpr float PassAfter = 2.f;
	constexpr float CartNear = 1500.f;
	constexpr float MinGap = 300.f;
	constexpr float MaxGap = 450.f;
	constexpr float MinRoomLeft = 2500.f;
	constexpr float PassLook = 6000.f;
	constexpr float PassHalfWidth = 200.f;
	// Something closer than this (cm) the other way, before the car is level with the cart, and it backs off.
	constexpr float AbortLook = 2500.f;
	// Level with the cart: its middle less than this far ahead (cm). Past it: this far behind the car.
	constexpr double LevelWithCart = 600.0;
	constexpr double PassedCart = 750.0;
	// Pulling out and back in (cm/s); a pass is given up after this long (s), or if the cart is this far off (cm).
	constexpr float ShiftRate = 220.f;
	constexpr float MaxPassingTime = 10.f;
	constexpr double LostCart = 3000.0;

	// Knocked by a crash: the tyres slow the car rolling along itself and,
	// much harder, skidding sideways (cm/s each second); the spin dies away
	// (each second to e^-SpinFade of itself, and by SpinDrag degrees a second).
	constexpr float KnockRollDrag = 700.f;
	constexpr float KnockSkidDrag = 1500.f;
	constexpr float KnockSpinFade = 1.5f;
	constexpr float KnockSpinDrag = 90.f;
	// Slower than this (cm/s, degrees a second) it has stopped; it sits shaken this long (s), then drives on.
	constexpr float KnockStillSpeed = 40.f;
	constexpr float KnockStillSpin = 8.f;
	constexpr float KnockShaken = 1.2f;
	// Knocked into something: how much bounces back from another car, and from a wall; the rub along it.
	constexpr float KnockCarBounce = 0.3f;
	constexpr float KnockWallBounce = 0.2f;
	constexpr float KnockRub = 0.3f;
	// Meeting at least this fast (cm/s) dents it, and hurts someone on foot.
	constexpr float KnockDentSpeed = 250.f;
	// A shove smaller than this (cm/s, degrees a second) only stops a driving car.
	constexpr float KnockLeast = 30.f;
	constexpr float KnockLeastSpin = 5.f;
}

ATrafficCar::ATrafficCar()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	BaseMaterial = CarBody::LoadPaintMaterial();

	// Same size and look as the player's car (see APestiCar).
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetBoxExtent(FVector(210.f, 85.f, 70.f));
	CollisionBox->SetCollisionProfileName(TEXT("Pawn"));
	RootComponent = CollisionBox;

	// Built in StartOnLane, once the car knows its paint.
	Visuals = CreateDefaultSubobject<UCarVisuals>(TEXT("Visuals"));

	// Trafficator arms swing out sideways from behind the front doors; the
	// car's left is -Y. Hidden until the car signals.
	auto MakeTrafficator = [&](const TCHAR* PartName, float Side)
	{
		UStaticMeshComponent* Arm = CreateDefaultSubobject<UStaticMeshComponent>(PartName);
		Arm->SetupAttachment(CollisionBox);
		Arm->SetStaticMesh(CubeMesh.Object);
		Arm->SetRelativeLocation(FVector(-45.f, Side * 86.f, 30.f));
		Arm->SetRelativeScale3D(FVector(0.05f, 0.3f, 0.06f));
		Arm->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Arm->SetVisibility(false);
		return Arm;
	};
	TrafficatorLeft = MakeTrafficator(TEXT("TrafficatorLeft"), -1.f);
	TrafficatorRight = MakeTrafficator(TEXT("TrafficatorRight"), 1.f);
}

void ATrafficCar::StartOnLane(ATrafficManager* InManager, int32 InLaneIndex, int32 InPointIndex, const FLinearColor& Paint)
{
	Manager = InManager;
	LaneIndex = InLaneIndex;
	NextLaneIndex = Manager->PickNextLane(LaneIndex);
	PointIndex = InPointIndex;
	CurrentSpeed = 0.f;
	PaintColor = Paint;

	Style = PickStyle(Paint);
	Visuals->Show(CollisionBox, Style);
	Visuals->ShowStreetDriver(FMath::RandRange(0, UCarVisuals::NumDriverLooks - 1));
	Visuals->SetStreetEngine(true);
	if (!BaseMaterial)
	{
		return;
	}
	for (UStaticMeshComponent* Arm : { TrafficatorLeft.Get(), TrafficatorRight.Get() })
	{
		UMaterialInstanceDynamic* Paintwork = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Paintwork->SetVectorParameterValue(TEXT("Color"), TrafficCar::TrafficatorColor);
		Arm->SetMaterial(0, Paintwork);
	}
}

void ATrafficCar::CarryPassenger(const PersonKit::FOutfit& Outfit, float Seconds)
{
	Visuals->ShowPassenger(Outfit);
	PassengerTime = Seconds;
}

FVector ATrafficCar::GetMoveVelocity() const
{
	return bKnocked ? KnockVelocity : GetActorForwardVector().GetSafeNormal2D() * CurrentSpeed;
}

float ATrafficCar::GetMass() const
{
	return CarHandling::For(Style.Body).Mass;
}

void ATrafficCar::TakeHit(const FVector& Push, float Spin)
{
	if (!bKnocked)
	{
		// A mere touch only stops it; it is not knocked out of its lane.
		if (Push.Size2D() < TrafficCar::KnockLeast && FMath::Abs(Spin) < TrafficCar::KnockLeastSpin)
		{
			CurrentSpeed = 0.f;
			return;
		}
		// Out of the traffic until it has stopped: what it was doing carries on as a slide.
		KnockVelocity = GetActorForwardVector().GetSafeNormal2D() * CurrentSpeed;
		KnockSpin = 0.f;
		bKnocked = true;
		CurrentSpeed = 0.f;
		if (Manager)
		{
			ReleaseAllJunctions();
		}
	}
	KnockVelocity += FVector(Push.X, Push.Y, 0.0);
	KnockSpin = FMath::Clamp(KnockSpin + Spin, -CarHandling::MaxSpin, CarHandling::MaxSpin);
	KnockStill = 0.f;
}

bool ATrafficCar::TickKnocked(float DeltaSeconds)
{
	using namespace TrafficCar;
	if (!bKnocked)
	{
		return false;
	}

	// The tyres: rolling along the car slows it gently, skidding sideways hard.
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	const FVector Right(-Forward.Y, Forward.X, 0.0);
	const float Rolling = FMath::FInterpConstantTo(static_cast<float>(FVector::DotProduct(KnockVelocity, Forward)), 0.f, DeltaSeconds, KnockRollDrag);
	const float Skidding = FMath::FInterpConstantTo(static_cast<float>(FVector::DotProduct(KnockVelocity, Right)), 0.f, DeltaSeconds, KnockSkidDrag);
	KnockVelocity = Forward * Rolling + Right * Skidding;
	KnockSpin = FMath::FInterpConstantTo(KnockSpin * FMath::Exp(-KnockSpinFade * DeltaSeconds), 0.f, DeltaSeconds, KnockSpinDrag);

	if (KnockVelocity.Size2D() < KnockStillSpeed && FMath::Abs(KnockSpin) < KnockStillSpin)
	{
		KnockVelocity = FVector::ZeroVector;
		KnockSpin = 0.f;
		KnockStill += DeltaSeconds;
		if (KnockStill < KnockShaken)
		{
			return true;
		}
		// Shaken but fine: back into the traffic from the nearest lane going its way.
		bKnocked = false;
		KnockStill = 0.f;
		CurrentSpeed = 0.f;
		BlockedTime = 0.f;
		int32 Lane = INDEX_NONE;
		int32 Point = 0;
		if (Manager && Manager->FindLanePointNear(GetActorLocation(), GetActorForwardVector().GetSafeNormal2D(), Lane, Point))
		{
			ResumeOnLane(Lane, Point);
		}
		return false;
	}

	AddActorWorldRotation(FRotator(0.f, KnockSpin * DeltaSeconds, 0.f));
	FHitResult Hit;
	AddActorWorldOffset(KnockVelocity * DeltaSeconds, true, &Hit);
	if (Hit.bStartPenetrating)
	{
		// Spun a corner into something: step back out.
		AddActorWorldOffset(FVector(Hit.Normal.X, Hit.Normal.Y, 0.0) * (Hit.PenetrationDepth + 1.0), false);
		return true;
	}
	if (!Hit.bBlockingHit)
	{
		return true;
	}

	// Slid into something: knock it too if it is a car, else bounce off it.
	const FVector Normal = FVector(Hit.Normal.X, Hit.Normal.Y, 0.0).GetSafeNormal();
	if (Normal.IsNearlyZero())
	{
		return true;
	}
	AActor* Other = Hit.GetActor();
	ATrafficCar* OtherCar = Cast<ATrafficCar>(Other);
	APestiCar* DrivenCar = Cast<APestiCar>(Other);
	FVector OtherVelocity = FVector::ZeroVector;
	float OtherMass = 0.f;
	if (OtherCar)
	{
		OtherVelocity = OtherCar->GetMoveVelocity();
		OtherMass = OtherCar->GetMass();
	}
	else if (DrivenCar)
	{
		OtherVelocity = DrivenCar->GetVelocity();
		OtherMass = DrivenCar->GetMass();
	}
	const float Meet = -static_cast<float>(FVector::DotProduct(KnockVelocity - OtherVelocity, Normal));
	// Sliding into him on foot hurts him.
	if (APestiPerson* Person = Cast<APestiPerson>(Other))
	{
		if (Meet > KnockDentSpeed)
		{
			Person->HitByVehicle(Meet, -Normal);
		}
	}
	CarHandling::FBump Bump;
	if (!CarHandling::Collide(Normal, Hit.ImpactPoint, GetActorLocation(), KnockVelocity, GetMass(),
		Other ? Other->GetActorLocation() : Hit.ImpactPoint, OtherVelocity, OtherMass,
		OtherMass > 0.f ? KnockCarBounce : KnockWallBounce, KnockRub, Bump))
	{
		return true;
	}
	KnockVelocity += Bump.PushA;
	KnockSpin = FMath::Clamp(KnockSpin + Bump.SpinA, -CarHandling::MaxSpin, CarHandling::MaxSpin);
	if (Meet > KnockDentSpeed && Visuals)
	{
		Visuals->Knock(Hit.ImpactPoint, Normal, Meet);
	}
	if (OtherCar)
	{
		OtherCar->TakeHit(Bump.PushB, Bump.SpinB);
		if (Meet > KnockDentSpeed)
		{
			if (UCarVisuals* OtherLooks = OtherCar->FindComponentByClass<UCarVisuals>())
			{
				OtherLooks->Knock(Hit.ImpactPoint, -Normal, Meet);
			}
		}
	}
	else if (DrivenCar)
	{
		// A soft nudge only shoves; a real knock dents his car and sounds.
		DrivenCar->Shove(Bump.PushB, Bump.SpinB, Hit.ImpactPoint, Meet > KnockDentSpeed);
	}
	return true;
}

void ATrafficCar::ResumeOnLane(int32 InLaneIndex, int32 InPointIndex)
{
	LaneIndex = InLaneIndex;
	PointIndex = InPointIndex;
	NextLaneIndex = Manager->PickNextLane(LaneIndex);
	BlockedTime = 0.f;
	JunctionWaitTime = 0.f;
	PassingCart = nullptr;
	BehindCartTime = 0.f;
	PassShift = 0.f;
	PassShiftWanted = 0.f;
}

void ATrafficCar::ReleaseAllJunctions()
{
	for (const int32 Node : HeldNodes)
	{
		Manager->ReleaseNode(Node, this);
	}
	HeldNodes.Reset();
	JunctionWaitTime = 0.f;
	TrafficatorLeft->SetVisibility(false);
	TrafficatorRight->SetVisibility(false);
}

void ATrafficCar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (PassengerTime > 0.f)
	{
		PassengerTime -= DeltaSeconds;
		if (PassengerTime <= 0.f)
		{
			Visuals->HidePassenger();
		}
	}

	// Knocked about by a crash: no driving until it has stopped.
	if (TickKnocked(DeltaSeconds))
	{
		return;
	}

	if (!Manager || LaneIndex == INDEX_NONE)
	{
		return;
	}

	// Pulled up for someone (a fare hailing a taxi): wait where it is.
	if (HoldTime > 0.f)
	{
		HoldTime -= DeltaSeconds;
		CurrentSpeed = 0.f;
		return;
	}

	FVector Location = GetActorLocation();

	// Advance along the lane, switching lanes at the end of each street piece.
	const FTrafficLane* Lane = &Manager->GetLane(LaneIndex);
	FVector Target = Lane->Points[PointIndex];
	// (Out passing a cart, the car goes by the points off to the side.)
	for (int32 Guard = 0; Guard < 4 && FVector::Dist2D(Location, Target) < TrafficCar::ReachRadius + PassShift; ++Guard)
	{
		if (++PointIndex >= Lane->Points.Num())
		{
			LaneIndex = NextLaneIndex;
			NextLaneIndex = Manager->PickNextLane(LaneIndex);
			Lane = &Manager->GetLane(LaneIndex);
			PointIndex = 0;
		}
		Target = Lane->Points[PointIndex];
	}

	// Steer towards the target point at a limited turn rate (shifted to the right while passing a cart).
	const FVector LaneFrom = PointIndex > 0 ? Lane->Points[PointIndex - 1] : Location;
	const FVector LaneDir = (Target - LaneFrom).GetSafeNormal2D();
	const FVector LaneRight(-LaneDir.Y, LaneDir.X, 0.0);
	const FVector ToTarget = (Target + LaneRight * PassShift - Location).GetSafeNormal2D();
	const float CurrentYaw = static_cast<float>(GetActorRotation().Yaw);
	const float WantedYaw = static_cast<float>(ToTarget.Rotation().Yaw);
	const float TurnNeeded = FMath::Abs(FRotator::NormalizeAxis(WantedYaw - CurrentYaw));
	const float NewYaw = FMath::FixedTurn(CurrentYaw, WantedYaw, TurnRate * DeltaSeconds);
	SetActorRotation(FRotator(0.f, NewYaw, 0.f));

	// Slow for the corner we are in, and brake early enough for the one at
	// the target point, so the car doesn't swing into a wall at a junction.
	const float CornerSpeed = Lane->Speed * CornerFraction(TurnNeeded);
	const FVector* AfterTarget = PointIndex + 1 < Lane->Points.Num()
		? &Lane->Points[PointIndex + 1]
		: (NextLaneIndex != INDEX_NONE ? &Manager->GetLane(NextLaneIndex).Points[0] : nullptr);
	float WantedSpeed = CornerSpeed;
	if (AfterTarget && FVector::Dist2D(*AfterTarget, Target) > 1.f)
	{
		const FVector NextHeading = (*AfterTarget - Target).GetSafeNormal2D();
		const float NextTurn = static_cast<float>(FMath::RadiansToDegrees(FMath::Acos(
			FMath::Clamp(FVector::DotProduct(ToTarget, NextHeading), -1.0, 1.0))));
		const float NextCornerSpeed = Lane->Speed * CornerFraction(NextTurn);
		const float DistToTarget = static_cast<float>(FVector::Dist2D(Location, Target));
		const float BrakeRoom = FMath::Max(0.f, DistToTarget - TrafficCar::ReachRadius);
		WantedSpeed = FMath::Min(WantedSpeed,
			FMath::Sqrt(FMath::Square(NextCornerSpeed) + BrakeDeceleration * BrakeRoom));
	}
	const float Clear = Manager->ClearDistanceAhead(this, TrafficCar::LookAhead, false, 0.f, PassingCart.Get());
	UpdatePassing(Location, LaneDir, *Lane, Clear, DeltaSeconds);
	WantedSpeed *= FMath::Clamp((Clear - StopGap) / (TrafficCar::LookAhead - StopGap), 0.f, 1.f);

	// Take turns at junctions.
	const float DistToJunction = static_cast<float>(FVector::Dist2D(Location, Lane->Points.Last()));
	WantedSpeed *= JunctionSpeedLimit(DistToJunction, Lane->ToNode, DeltaSeconds);
	ReleasePassedJunctions(Location, Lane->ToNode);
	UpdateTrafficators(DistToJunction);

	const float Rate = WantedSpeed < CurrentSpeed ? BrakeDeceleration : Acceleration;
	CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, WantedSpeed, DeltaSeconds, Rate);

	// Move with collision so cars bump into the player and each other instead
	// of passing through. If stuck on another AI car or a wall for a while,
	// drive through it so traffic cannot jam for good; never through the player.
	const FVector NewLocation = Location + GetActorForwardVector().GetSafeNormal2D() * CurrentSpeed * DeltaSeconds;
	FHitResult Hit;
	SetActorLocation(NewLocation, true, &Hit);
	if (!Hit.bBlockingHit)
	{
		BlockedTime = 0.f;
		return;
	}
	BlockedTime += DeltaSeconds;
	const AActor* Blocker = Hit.GetActor();
	const bool bBlockedByPlayer = Blocker && Blocker == UGameplayStatics::GetPlayerPawn(this, 0);
	if (!bBlockedByPlayer && BlockedTime > TrafficCar::MaxBlockedTime)
	{
		// Keep a crawl going, or the car would creep a hair a frame and stay
		// wedged in the wall for good.
		CurrentSpeed = FMath::Max(CurrentSpeed, TrafficCar::UnstickSpeed);
		SetActorLocation(Location + GetActorForwardVector().GetSafeNormal2D() * CurrentSpeed * DeltaSeconds, false);
		return;
	}
	// Running into him on foot hurts him.
	if (bBlockedByPlayer)
	{
		if (APestiPerson* Person = Cast<APestiPerson>(UGameplayStatics::GetPlayerPawn(this, 0)))
		{
			Person->HitByVehicle(CurrentSpeed, GetActorForwardVector());
		}
	}
	CurrentSpeed = 0.f;
}

void ATrafficCar::UpdatePassing(const FVector& Location, const FVector& LaneDir, const FTrafficLane& Lane, float Clear, float DeltaSeconds)
{
	using namespace TrafficCar;
	const FVector LaneRight(-LaneDir.Y, LaneDir.X, 0.0);
	const AActor* Cart = PassingCart.Get();
	if (!Cart)
	{
		// Held up behind a cart (nothing nearer than its tail, which is some 2.5 m behind its middle)?
		// After a while, pull out if the way is clear.
		PassShiftWanted = 0.f;
		PassingTime = 0.f;
		float CartDistance = CartNear;
		const AActor* Ahead = Manager->FindCartAhead(this, CartNear, CartDistance);
		const bool bHeldUp = Ahead && Clear >= CartDistance - 400.f && CurrentSpeed < Lane.Speed * 0.6f;
		BehindCartTime = bHeldUp ? BehindCartTime + DeltaSeconds : 0.f;
		if (bHeldUp && BehindCartTime > PassAfter && PassShift < 1.f)
		{
			const float Gap = Manager->OncomingLaneGap(LaneIndex);
			const float RoomLeft = static_cast<float>(FVector::Dist2D(Location, Lane.Points.Last()));
			if (Gap >= MinGap && RoomLeft > MinRoomLeft
				&& Manager->IsStripClear(Location + LaneRight * FMath::Min(Gap, MaxGap), LaneDir, PassLook, PassHalfWidth, this, Ahead))
			{
				PassingCart = Ahead;
				PassGap = FMath::Min(Gap, MaxGap);
				BehindCartTime = 0.f;
			}
		}
	}
	else
	{
		PassingTime += DeltaSeconds;
		// How far the cart's middle is still ahead along the street (below 0: the car is past it).
		const double CartAhead = FVector::DotProduct(Cart->GetActorLocation() - Location, LaneDir);
		const bool bPassed = CartAhead < -PassedCart;
		// Something coming the other way while still behind the cart: back in behind it.
		const bool bBackOff = CartAhead > LevelWithCart
			&& !Manager->IsStripClear(Location + LaneRight * PassShift, LaneDir, AbortLook, PassHalfWidth, this, Cart);
		const bool bGiveUp = PassingTime > MaxPassingTime || FVector::Dist2D(Cart->GetActorLocation(), Location) > LostCart;
		if (bPassed || bBackOff || bGiveUp)
		{
			PassingCart = nullptr;
			PassShiftWanted = 0.f;
		}
		else
		{
			PassShiftWanted = PassGap;
		}
	}
	PassShift = FMath::FInterpConstantTo(PassShift, PassShiftWanted, DeltaSeconds, ShiftRate);
}

float ATrafficCar::CornerFraction(float TurnDegrees)
{
	return FMath::Lerp(1.f, TrafficCar::CornerSpeedFraction, FMath::Clamp(TurnDegrees / 90.f, 0.f, 1.f));
}

float ATrafficCar::JunctionSpeedLimit(float DistToJunction, int32 Node, float DeltaSeconds)
{
	if (HeldNodes.Contains(Node) || DistToJunction > TrafficCar::JunctionApproach)
	{
		return 1.f;
	}

	// Don't enter a junction (or the tram tracks across it) unless there is
	// room on the far side; after a long wait go anyway, so traffic can't
	// lock up for good.
	const bool bRoomToExit = !Manager->IsJunction(Node)
		|| Manager->HasRoomOnLane(NextLaneIndex, TrafficCar::ExitRoom, this)
		|| JunctionWaitTime > MaxJunctionWait * 3.f;
	const bool bForce = JunctionWaitTime > MaxJunctionWait;
	if (bRoomToExit && Manager->TryClaimNode(Node, this, bForce))
	{
		HeldNodes.Add(Node);
		JunctionWaitTime = 0.f;
		return 1.f;
	}

	JunctionWaitTime += DeltaSeconds;
	return FMath::Clamp((DistToJunction - TrafficCar::JunctionStopLine)
		/ (TrafficCar::JunctionApproach - TrafficCar::JunctionStopLine), 0.f, 1.f);
}

void ATrafficCar::ReleasePassedJunctions(const FVector& Location, int32 NextNode)
{
	for (int32 i = HeldNodes.Num() - 1; i >= 0; --i)
	{
		const int32 Node = HeldNodes[i];
		if (Node != NextNode && FVector::Dist2D(Location, Manager->GetNodeLocation(Node)) > TrafficCar::JunctionExit)
		{
			Manager->ReleaseNode(Node, this);
			HeldNodes.RemoveAt(i);
		}
	}
}

void ATrafficCar::UpdateTrafficators(float DistToJunction)
{
	bool bLeft = false;
	bool bRight = false;
	const FTrafficLane& Lane = Manager->GetLane(LaneIndex);
	if (DistToJunction < TrafficCar::SignalDistance && Manager->IsJunction(Lane.ToNode) && NextLaneIndex != INDEX_NONE)
	{
		// Compare the heading at the end of this lane with the start of the next.
		const FTrafficLane& Next = Manager->GetLane(NextLaneIndex);
		const int32 Last = Lane.Points.Num() - 1;
		const float InYaw = static_cast<float>((Lane.Points[Last] - Lane.Points[Last - 1]).Rotation().Yaw);
		const float OutYaw = static_cast<float>((Next.Points[1] - Next.Points[0]).Rotation().Yaw);
		const float Turn = FRotator::NormalizeAxis(OutYaw - InYaw);
		// Unreal's yaw grows clockwise seen from above, so positive is a right turn.
		bLeft = Turn < -TrafficCar::SignalAngle;
		bRight = Turn > TrafficCar::SignalAngle;
	}
	TrafficatorLeft->SetVisibility(bLeft);
	TrafficatorRight->SetVisibility(bRight);
}

CarModel::FStyle ATrafficCar::PickStyle(const FLinearColor& Paint) const
{
	FRandomStream Random(FMath::Rand());
	return CarModel::RandomStyle(Random, Paint);
}

ATrafficTaxi::ATrafficTaxi()
{
}

CarModel::FStyle ATrafficTaxi::PickStyle(const FLinearColor& /*Paint*/) const
{
	return CarModel::TaxiStyle();
}

UCarVisuals* ATrafficCar::GetVisuals() const
{
	return Visuals.Get();
}
