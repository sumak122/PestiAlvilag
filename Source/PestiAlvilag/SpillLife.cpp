// Fill out your copyright notice in the Description page of Project Settings.

#include "SpillLife.h"

#include "Animals.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Pedestrians.h"
#include "ProceduralMeshComponent.h"

namespace SpillLife
{
	// Spilt milk keeps the cats lapping this long (s).
	constexpr float MilkSeconds = 35.f;

	// Kids come to a coal heap this often (a share of the spills), never more often than once in
	// this long (s); two, sometimes three; from this far (cm), out of sight.
	constexpr float KidsChance = 0.7f;
	constexpr double KidsEvery = 45.0;
	constexpr double KidsNear = 1500.0;
	constexpr double KidsFar = 2500.0;
	// Their run, and crouched at the heap this long (s), this far from its middle (cm).
	constexpr float KidRun = 380.f;
	constexpr float GrabLow = 4.f;
	constexpr float GrabHigh = 6.5f;
	constexpr double GrabBack = 62.0;
	// How tall they are (cm).
	constexpr float KidLow = 122.f;
	constexpr float KidHigh = 142.f;

	// A barrel rolls off at this speed (cm/s) the way the cart was going, a little sideways,
	// slowing by this much each second; it stops below this, and bounces back off a wall with this share.
	constexpr float RollLow = 260.f;
	constexpr float RollHigh = 420.f;
	constexpr float RollDrag = 60.f;
	constexpr float RollStop = 12.f;
	constexpr float Bounce = 0.4f;
	// Someone runs after it this often; from this far (cm), out of sight; and rolls it off at this pace.
	constexpr float ChaseChance = 0.7f;
	constexpr double ChaseNear = 1500.0;
	constexpr double ChaseFar = 2400.0;
	constexpr float ChaseRun = 400.f;
	constexpr float PushPace = 95.f;
	// He takes hold of it this close (cm), once it is this slow (cm/s), and walks this far behind it.
	constexpr double TakeHold = 110.0;
	constexpr float HoldSlow = 40.f;
	constexpr double Behind = 72.0;

	// Gone once this far from the camera (cm) and out of sight this long (s), or after this long at most.
	constexpr double GoneFar = 2000.0;
	constexpr float GoneUnseen = 1.5f;
	constexpr float GoingAtMost = 40.f;
	constexpr float ComingAtMost = 30.f;
	// Going off: this far (cm).
	constexpr double OffDistance = 3000.0;
	// A person's middle stands this high (cm); crouched, it is lower by this share of his height.
	constexpr double MiddleHigh = 90.0;
	constexpr double CrouchDrop = 0.25;
	constexpr double Arrive = 30.0;
	constexpr float TurnRate = 540.f;
	constexpr float GroundEvery = 0.25f;
	// In the camera's view: within this cosine of where it looks.
	constexpr double InView = 0.45;
}

ASpillLife::ASpillLife()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Random.Initialize(1987);
}

void ASpillLife::Spilled(const UObject* WorldContextObject, HorseCartModel::ELoad Load, UProceduralMeshComponent* Lying,
	UProceduralMeshComponent* Puddle, const FVector& Where, const FVector2D& Going)
{
	using HorseCartModel::ELoad;
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}
	if (Load == ELoad::Milk)
	{
		AAnimals::CallToMilk(WorldContextObject, Where, SpillLife::MilkSeconds);
		return;
	}

	ASpillLife* Life = nullptr;
	for (TActorIterator<ASpillLife> It(World); It; ++It)
	{
		Life = *It;
		break;
	}
	if (!Life)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Life = World->SpawnActor<ASpillLife>(ASpillLife::StaticClass(), FTransform::Identity, Params);
		if (!Life)
		{
			return;
		}
	}
	Life->UpdateCamera();

	if (Load == ELoad::Coal)
	{
		const double Now = World->GetTimeSeconds();
		if (!Puddle || Now - Life->LastKids < SpillLife::KidsEvery || Life->Random.FRand() >= SpillLife::KidsChance)
		{
			return;
		}
		FHeap& Heap = Life->Heaps.AddDefaulted_GetRef();
		Heap.Coal = Puddle;
		Heap.At = Where;
		Life->SendKids(Life->Heaps.Num() - 1);
		if (Heap.Kids > 0)
		{
			Life->LastKids = Now;
		}
		return;
	}

	if (Load == ELoad::Beer && Lying)
	{
		// Rolling on the way the cart went, a little to one side; it lies across the way it rolls.
		FVector2D Way = Going.GetSafeNormal();
		if (Way.IsNearlyZero())
		{
			Way = FVector2D(1.0, 0.0);
		}
		Way = Way.GetRotated(Life->Random.FRandRange(-25.f, 25.f));
		FBarrel& Barrel = Life->Barrels.AddDefaulted_GetRef();
		Barrel.Mesh = Lying;
		Barrel.Centre = Where + FVector(0.0, 0.0, HorseCartModel::BarrelRadius);
		const double WayYaw = FMath::RadiansToDegrees(FMath::Atan2(Way.Y, Way.X));
		Barrel.Lie = FQuat(FRotator(0.0, WayYaw + 90.0, 0.0));
		Barrel.Axle = Barrel.Lie.RotateVector(FVector(0.0, 1.0, 0.0));
		// It rolls about its length: along Axle x up, either way.
		const FVector2D Roll = FVector2D(FVector::CrossProduct(Barrel.Axle, FVector::UpVector)).GetSafeNormal();
		const double Sign = FVector2D::DotProduct(Roll, Way) >= 0.0 ? 1.0 : -1.0;
		Barrel.Velocity = Roll * Sign * Life->Random.FRandRange(SpillLife::RollLow, SpillLife::RollHigh);
		Life->PlaceBarrel(Barrel);

		bool bSomeoneAfterOne = false;
		for (const FHelper& Helper : Life->Helpers)
		{
			bSomeoneAfterOne |= !Helper.bKid;
		}
		if (!bSomeoneAfterOne && Life->Random.FRand() < SpillLife::ChaseChance)
		{
			Life->SendRoller(Life->Barrels.Num() - 1);
		}
	}
}

void ASpillLife::UpdateCamera()
{
	if (const APlayerCameraManager* Viewer = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		Eye = Viewer->GetCameraLocation();
		EyeAhead = Viewer->GetCameraRotation().Vector();
	}
}

bool ASpillLife::IsClear(const FVector& From, const FVector& To) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SpillLifeClear), false, this);
	return !GetWorld()->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Params);
}

bool ASpillLife::FindHidden(const FVector& To, double Near, double Far, FVector& OutGround)
{
	const float FirstTurn = Random.FRandRange(0.f, 360.f);
	const FVector Lift(0.0, 0.0, 60.0);
	for (int32 Try = 0; Try < 16; ++Try)
	{
		const FVector2D Way = FVector2D(FRotator(0.0, FirstTurn + Try * 67.5f, 0.0).Vector());
		const FVector Probe = To + FVector(Way * Random.FRandRange(Near, Far), 0.0);
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(SpillLifeGround), false, this);
		if (!GetWorld()->LineTraceSingleByChannel(Hit, Probe + FVector(0.0, 0.0, 300.0), Probe - FVector(0.0, 0.0, 600.0), ECC_Visibility, Params)
			|| Hit.ImpactNormal.Z < 0.8 || FMath::Abs(Hit.ImpactPoint.Z - To.Z) > 80.0)
		{
			continue;
		}
		const FVector Ground = Hit.ImpactPoint;
		if (!IsClear(Ground + Lift, To + Lift))
		{
			continue;
		}
		// Out of the camera's view, or behind something.
		const FVector Look = Ground + FVector(0.0, 0.0, 100.0) - Eye;
		if (FVector::DotProduct(Look.GetSafeNormal(), EyeAhead) > SpillLife::InView && IsClear(Eye, Ground + FVector(0.0, 0.0, 100.0)))
		{
			continue;
		}
		OutGround = Ground;
		return true;
	}
	return false;
}

APedestrian* ASpillLife::MakePerson(const PersonKit::FOutfit& Dress, const FVector& Ground, float Yaw)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APedestrian* Made = GetWorld()->SpawnActor<APedestrian>(APedestrian::StaticClass(), Ground + FVector(0.0, 0.0, SpillLife::MiddleHigh),
		FRotator(0.0, Yaw, 0.0), Params);
	if (Made)
	{
		Made->SetOutfit(Dress);
	}
	return Made;
}

void ASpillLife::SendKids(int32 HeapIndex)
{
	FHeap& Heap = Heaps[HeapIndex];
	const int32 Count = Random.FRand() < 0.3f ? 3 : 2;
	const FLinearColor Coats[] = {
		FLinearColor(0.22f, 0.18f, 0.14f), FLinearColor(0.14f, 0.16f, 0.22f), FLinearColor(0.30f, 0.20f, 0.12f),
		FLinearColor(0.18f, 0.19f, 0.16f), FLinearColor(0.25f, 0.12f, 0.10f) };
	for (int32 k = 0; k < Count; ++k)
	{
		FVector Ground;
		if (!FindHidden(Heap.At, SpillLife::KidsNear, SpillLife::KidsFar, Ground))
		{
			continue;
		}
		PersonKit::FOutfit Dress;
		Dress.Height = Random.FRandRange(SpillLife::KidLow, SpillLife::KidHigh);
		Dress.Build = 0.9f;
		Dress.Coat = Coats[Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Coats)) - 1)];
		Dress.Trousers = FLinearColor(0.08f, 0.07f, 0.07f);
		Dress.Top = Random.FRand() < 0.5f ? PersonKit::ETop::Jacket : PersonKit::ETop::Waistcoat;
		Dress.Hat = Random.FRand() < 0.6f ? PersonKit::EHat::FlatCap : PersonKit::EHat::None;
		Dress.HatColor = FLinearColor(0.12f, 0.10f, 0.08f);
		Dress.bTie = false;
		Dress.bMoustache = false;
		const FVector2D ToHeap = FVector2D(Heap.At - Ground).GetSafeNormal();
		const float Yaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(ToHeap.Y, ToHeap.X)));
		APedestrian* Kid = MakePerson(Dress, Ground, Yaw);
		if (!Kid)
		{
			continue;
		}
		FHelper& Helper = Helpers.AddDefaulted_GetRef();
		Helper.Person = Kid;
		Helper.bKid = true;
		Helper.Stage = EStage::Coming;
		Helper.Thing = HeapIndex;
		Helper.Spot = FVector2D(Ground);
		Helper.GroundZ = Ground.Z;
		Helper.Facing = Yaw;
		Helper.Height = Dress.Height;
		Helper.Grab = Random.FRandRange(SpillLife::GrabLow, SpillLife::GrabHigh);
		// Round the heap on the side he comes from, spread a little.
		Helper.Goal = FVector2D(Heap.At) - ToHeap.GetRotated(Random.FRandRange(-40.f, 40.f)) * SpillLife::GrabBack;
		++Heap.Kids;
	}
}

void ASpillLife::SendRoller(int32 BarrelIndex)
{
	FBarrel& Barrel = Barrels[BarrelIndex];
	const FVector Ground0 = Barrel.Centre - FVector(0.0, 0.0, HorseCartModel::BarrelRadius);
	FVector Ground;
	if (!FindHidden(Ground0, SpillLife::ChaseNear, SpillLife::ChaseFar, Ground))
	{
		return;
	}
	PersonKit::FOutfit Dress;
	Dress.Height = Random.FRandRange(168.f, 184.f);
	Dress.Build = Random.FRandRange(1.f, 1.2f);
	Dress.Coat = FLinearColor(0.25f, 0.22f, 0.18f);
	Dress.Trousers = FLinearColor(0.10f, 0.09f, 0.08f);
	Dress.Top = PersonKit::ETop::Waistcoat;
	Dress.Hat = PersonKit::EHat::FlatCap;
	Dress.HatColor = FLinearColor(0.15f, 0.12f, 0.10f);
	Dress.bTie = false;
	Dress.bMoustache = Random.FRand() < 0.6f;
	const FVector2D ToBarrel = FVector2D(Ground0 - Ground).GetSafeNormal();
	const float Yaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(ToBarrel.Y, ToBarrel.X)));
	APedestrian* Man = MakePerson(Dress, Ground, Yaw);
	if (!Man)
	{
		return;
	}
	FHelper& Helper = Helpers.AddDefaulted_GetRef();
	Helper.Person = Man;
	Helper.bKid = false;
	Helper.Stage = EStage::Coming;
	Helper.Thing = BarrelIndex;
	Helper.Spot = FVector2D(Ground);
	Helper.GroundZ = Ground.Z;
	Helper.Facing = Yaw;
	Helper.Height = Dress.Height;
	Barrel.bChased = true;
}

void ASpillLife::PlaceBarrel(FBarrel& Barrel) const
{
	UProceduralMeshComponent* Mesh = Barrel.Mesh.Get();
	if (!Mesh)
	{
		return;
	}
	// The mesh's own middle is a radius over its foot: turn it about that.
	const FQuat Turned = FQuat(Barrel.Axle, Barrel.Angle) * Barrel.Lie;
	const FVector Foot = Barrel.Centre - Turned.RotateVector(FVector(0.0, 0.0, HorseCartModel::BarrelRadius));
	Mesh->SetWorldLocationAndRotation(Foot, Turned);
}

void ASpillLife::RollBarrel(FBarrel& Barrel, float DeltaSeconds)
{
	const double Speed = Barrel.Velocity.Size();
	if (Speed < SpillLife::RollStop)
	{
		Barrel.Velocity = FVector2D::ZeroVector;
		return;
	}
	if (!Barrel.bPushed)
	{
		Barrel.Velocity *= FMath::Max(0.0, Speed - SpillLife::RollDrag * DeltaSeconds) / Speed;
	}
	const FVector2D Step = Barrel.Velocity * DeltaSeconds;
	// A wall or anything solid ahead: back off it, slower.
	const FVector2D Going = Barrel.Velocity.GetSafeNormal();
	if (!IsClear(Barrel.Centre, Barrel.Centre + FVector(Going * (HorseCartModel::BarrelRadius + Step.Size() + 5.0), 0.0)))
	{
		Barrel.Velocity = -Barrel.Velocity * SpillLife::Bounce;
		return;
	}
	FVector Next = Barrel.Centre + FVector(Step, 0.0);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SpillLifeBarrel), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Next + FVector(0.0, 0.0, 60.0), Next - FVector(0.0, 0.0, 300.0), ECC_Visibility, Params))
	{
		Next.Z = Hit.ImpactPoint.Z + HorseCartModel::BarrelRadius;
	}
	// Turning as it goes: a radius of travel is a radian, which way by the side it rolls to.
	const FVector2D Roll = FVector2D(FVector::CrossProduct(Barrel.Axle, FVector::UpVector)).GetSafeNormal();
	Barrel.Angle += FVector2D::DotProduct(Step, Roll) / HorseCartModel::BarrelRadius;
	Barrel.Centre = Next;
	PlaceBarrel(Barrel);
}

FVector2D ASpillLife::WayOff(const FVector2D& Here, double Reach) const
{
	FVector2D Away = (Here - FVector2D(Eye)).GetSafeNormal();
	if (Away.IsNearlyZero())
	{
		Away = FVector2D(1.0, 0.0);
	}
	const FVector Lift(0.0, 0.0, 60.0);
	for (const double Turn : { 0.0, 35.0, -35.0, 70.0, -70.0, 110.0, -110.0, 150.0, -150.0 })
	{
		const FVector2D Try = Away.GetRotated(Turn);
		const FVector From(Here, Eye.Z);
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(SpillLifeWay), false, this);
		// From his own height: the ground under him first.
		double Z = Eye.Z;
		if (GetWorld()->LineTraceSingleByChannel(Hit, From + FVector(0.0, 0.0, 200.0), From - FVector(0.0, 0.0, 2000.0), ECC_Visibility, Params))
		{
			Z = Hit.ImpactPoint.Z;
		}
		const FVector Start(Here, Z);
		if (IsClear(Start + Lift, Start + Lift + FVector(Try * Reach, 0.0)))
		{
			return Try;
		}
	}
	return Away;
}

void ASpillLife::UpdateGround(FHelper& Helper, float DeltaSeconds)
{
	Helper.GroundClock -= DeltaSeconds;
	if (Helper.GroundClock > 0.f)
	{
		return;
	}
	Helper.GroundClock = SpillLife::GroundEvery;
	const FVector Over(Helper.Spot, Helper.GroundZ + 150.0);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SpillLifeFeet), false, this);
	if (APedestrian* Person = Helper.Person.Get())
	{
		Params.AddIgnoredActor(Person);
	}
	if (GetWorld()->LineTraceSingleByChannel(Hit, Over, Over - FVector(0.0, 0.0, 400.0), ECC_Visibility, Params) && Hit.ImpactNormal.Z > 0.7)
	{
		Helper.GroundZ = Hit.ImpactPoint.Z;
	}
}

bool ASpillLife::RunTo(FHelper& Helper, const FVector2D& Goal, float Speed, float DeltaSeconds, float ArmsOut)
{
	APedestrian* Person = Helper.Person.Get();
	const FVector2D ToGoal = Goal - Helper.Spot;
	const double Left = ToGoal.Size();
	if (Left < SpillLife::Arrive)
	{
		return true;
	}
	const FVector2D Way = ToGoal / Left;
	Helper.Spot += Way * FMath::Min(Left, static_cast<double>(Speed * DeltaSeconds));
	const float Wanted = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Way.Y, Way.X)));
	Helper.Facing = FMath::FixedTurn(Helper.Facing, Wanted, SpillLife::TurnRate * DeltaSeconds);
	UpdateGround(Helper, DeltaSeconds);
	if (Person)
	{
		Person->WalkTo(FVector(Helper.Spot, Helper.GroundZ + SpillLife::MiddleHigh), Helper.Facing, Speed, DeltaSeconds, ArmsOut);
	}
	return false;
}

bool ASpillLife::TickHelper(FHelper& Helper, float DeltaSeconds)
{
	APedestrian* Person = Helper.Person.Get();
	if (!Person)
	{
		return true;
	}
	Helper.Clock += DeltaSeconds;

	switch (Helper.Stage)
	{
	case EStage::Coming:
	{
		if (Helper.bKid)
		{
			if (RunTo(Helper, Helper.Goal, SpillLife::KidRun, DeltaSeconds, 0.f))
			{
				Helper.Stage = EStage::Crouching;
				Helper.Clock = 0.f;
				const FVector2D ToHeap = (Heaps.IsValidIndex(Helper.Thing) ? FVector2D(Heaps[Helper.Thing].At) : Helper.Goal) - Helper.Spot;
				Helper.Facing = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(ToHeap.Y, ToHeap.X)));
			}
			else if (Helper.Clock > SpillLife::ComingAtMost)
			{
				Helper.Stage = EStage::Going;
				Helper.Clock = 0.f;
				Helper.Goal = Helper.Spot + WayOff(Helper.Spot, SpillLife::OffDistance) * SpillLife::OffDistance;
			}
			break;
		}
		// After the barrel: run to it, take hold once it has nearly stopped.
		if (!Barrels.IsValidIndex(Helper.Thing) || !Barrels[Helper.Thing].Mesh.IsValid() || Helper.Clock > SpillLife::ComingAtMost)
		{
			Helper.Stage = EStage::Going;
			Helper.Clock = 0.f;
			Helper.Goal = Helper.Spot + WayOff(Helper.Spot, SpillLife::OffDistance) * SpillLife::OffDistance;
			break;
		}
		FBarrel& Barrel = Barrels[Helper.Thing];
		const FVector2D BarrelAt(Barrel.Centre);
		const FVector2D FromBarrel = (Helper.Spot - BarrelAt).GetSafeNormal();
		const FVector2D Reach = BarrelAt + FromBarrel * SpillLife::Behind;
		const bool bThere = FVector2D::Distance(Helper.Spot, BarrelAt) < SpillLife::TakeHold;
		if (bThere && Barrel.Velocity.Size() < SpillLife::HoldSlow)
		{
			// Lines it up and rolls it off, away from the camera.
			const FVector2D Off = WayOff(BarrelAt, SpillLife::OffDistance);
			const double OffYaw = FMath::RadiansToDegrees(FMath::Atan2(Off.Y, Off.X));
			Barrel.Lie = FQuat(FRotator(0.0, OffYaw + 90.0, 0.0));
			Barrel.Axle = Barrel.Lie.RotateVector(FVector(0.0, 1.0, 0.0));
			Barrel.Angle = 0.0;
			Barrel.Velocity = Off * SpillLife::PushPace;
			Barrel.bPushed = true;
			Helper.Stage = EStage::Pushing;
			Helper.Clock = 0.f;
			Helper.Unseen = 0.f;
			break;
		}
		if (!bThere)
		{
			RunTo(Helper, Reach, SpillLife::ChaseRun, DeltaSeconds, 0.f);
		}
		break;
	}
	case EStage::Crouching:
	{
		// Down at the heap, both hands in the coal.
		const float Reach = 55.f + 20.f * FMath::Sin(Helper.Clock * 5.f);
		const double Middle = Helper.GroundZ + SpillLife::MiddleHigh - SpillLife::CrouchDrop * Helper.Height;
		Person->Hold(FVector(Helper.Spot, Middle), FRotator(-8.0, Helper.Facing, 0.0), 75.f, 115.f, Reach);
		if (Helper.Clock > Helper.Grab)
		{
			Helper.Stage = EStage::Going;
			Helper.Clock = 0.f;
			Helper.Goal = Helper.Spot + WayOff(Helper.Spot, SpillLife::OffDistance) * SpillLife::OffDistance;
			// The last one up: the heap is gone.
			if (Heaps.IsValidIndex(Helper.Thing))
			{
				FHeap& Heap = Heaps[Helper.Thing];
				if (--Heap.Kids <= 0)
				{
					if (UProceduralMeshComponent* Coal = Heap.Coal.Get())
					{
						Coal->SetVisibility(false);
					}
				}
			}
		}
		break;
	}
	case EStage::Pushing:
	{
		// Behind the barrel, walking it off; it turns aside from walls on its own (bouncing back), so he follows it.
		if (!Barrels.IsValidIndex(Helper.Thing) || !Barrels[Helper.Thing].Mesh.IsValid())
		{
			Helper.Stage = EStage::Going;
			Helper.Clock = 0.f;
			Helper.Goal = Helper.Spot + WayOff(Helper.Spot, SpillLife::OffDistance) * SpillLife::OffDistance;
			break;
		}
		FBarrel& Barrel = Barrels[Helper.Thing];
		if (Barrel.Velocity.Size() < SpillLife::PushPace * 0.9f)
		{
			// Bumped into something: a new way, turned from that one.
			const FVector2D Off = WayOff(FVector2D(Barrel.Centre), SpillLife::OffDistance * 0.5);
			const double OffYaw = FMath::RadiansToDegrees(FMath::Atan2(Off.Y, Off.X));
			Barrel.Lie = FQuat(FRotator(0.0, OffYaw + 90.0, 0.0));
			Barrel.Axle = Barrel.Lie.RotateVector(FVector(0.0, 1.0, 0.0));
			Barrel.Angle = 0.0;
			Barrel.Velocity = Off * SpillLife::PushPace;
		}
		const FVector2D Going = Barrel.Velocity.GetSafeNormal();
		RunTo(Helper, FVector2D(Barrel.Centre) - Going * SpillLife::Behind, SpillLife::PushPace * 1.3f, DeltaSeconds, 0.35f);
		const bool bSeen = Person->WasRecentlyRendered(0.2f) || Barrel.Mesh->WasRecentlyRendered(0.2f);
		const bool bFar = FVector::Dist2D(FVector(Helper.Spot, 0.0), Eye) > SpillLife::GoneFar;
		Helper.Unseen = bSeen || !bFar ? 0.f : Helper.Unseen + DeltaSeconds;
		if (Helper.Unseen > SpillLife::GoneUnseen || Helper.Clock > SpillLife::GoingAtMost * 2.f)
		{
			// Home with it: the barrel goes with him.
			Barrel.Mesh->SetVisibility(false);
			Barrel.Velocity = FVector2D::ZeroVector;
			Barrel.bPushed = false;
			Person->Destroy();
			return true;
		}
		break;
	}
	case EStage::Going:
	default:
	{
		// Off and away, a kid with his arms full of coal.
		if (RunTo(Helper, Helper.Goal, Helper.bKid ? SpillLife::KidRun : 130.f, DeltaSeconds, Helper.bKid ? 0.35f : 0.f))
		{
			Helper.Goal = Helper.Spot + WayOff(Helper.Spot, SpillLife::OffDistance) * SpillLife::OffDistance;
		}
		const bool bFar = FVector::Dist2D(FVector(Helper.Spot, 0.0), Eye) > SpillLife::GoneFar;
		Helper.Unseen = Person->WasRecentlyRendered(0.2f) || !bFar ? 0.f : Helper.Unseen + DeltaSeconds;
		if (Helper.Unseen > SpillLife::GoneUnseen || Helper.Clock > SpillLife::GoingAtMost)
		{
			Person->Destroy();
			return true;
		}
		break;
	}
	}
	return false;
}

void ASpillLife::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (DeltaSeconds <= 0.f)
	{
		return;
	}
	UpdateCamera();

	for (FBarrel& Barrel : Barrels)
	{
		if (Barrel.Mesh.IsValid() && Barrel.Mesh->IsVisible())
		{
			RollBarrel(Barrel, DeltaSeconds);
		}
	}
	for (int32 i = Helpers.Num() - 1; i >= 0; --i)
	{
		if (TickHelper(Helpers[i], DeltaSeconds))
		{
			Helpers.RemoveAt(i);
		}
	}

	// Forget barrels and heaps that are gone or done with, once nobody is after them.
	if (Helpers.Num() == 0)
	{
		Barrels.RemoveAll([](const FBarrel& Barrel) { return !Barrel.Mesh.IsValid() || (!Barrel.Mesh->IsVisible()) || Barrel.Velocity.IsNearlyZero(); });
		Heaps.Reset();
	}
}
