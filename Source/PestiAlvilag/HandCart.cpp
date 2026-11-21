// Fill out your copyright notice in the Description page of Project Settings.

#include "HandCart.h"

#include "Camera/PlayerCameraManager.h"
#include "CarBody.h"
#include "CarModel.h"
#include "CarVisuals.h"
#include "CarjackedDriver.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "DailyLifeModel.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Pedestrians.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PolicePatrols.h"
#include "ProceduralMeshComponent.h"
#include "StreetName.h"
#include "TrafficManager.h"
#include "Witnesses.h"

namespace HandCartRun
{
	// Pushed by the player: a brisk walk at most (cm/s, cm/s each second), and it turns slowly.
	constexpr float TopSpeed = 300.f;
	constexpr float Pull = 260.f;
	constexpr float BackSpeed = 120.f;
	constexpr float TurnRate = 70.f;
	constexpr float FullTurnAt = 60.f;
	// The porter's own pace (cm/s, about 4 km/h), how fast he gets going and stops.
	constexpr float CruiseLow = 100.f;
	constexpr float CruiseHigh = 130.f;
	constexpr float SpeedUp = 80.f;
	constexpr float SlowDown = 300.f;
	constexpr float TurnSpeed = 70.f;
	// He keeps this far towards the kerb from the cars' lane, and this far from a wall (cm).
	constexpr double KerbOffset = 210.0;
	constexpr double WallRoom = 70.0;
	constexpr double ReachRadius = 120.0;
	// Looks this far ahead in a strip this wide each side, and stops this far behind (cm); held up this long, he squeezes by.
	constexpr float LookAhead = 900.f;
	constexpr float Corridor = 90.f;
	constexpr float StopGap = 320.f;
	constexpr float MaxStuck = 6.f;
	constexpr float Squeeze = 3.f;
	// Something this much faster than the cart knocks its load off (cm/s, about 14 km/h); slower, he only shouts.
	constexpr float KnockSpeed = 380.f;
	constexpr float NudgeSpeed = 60.f;
	constexpr double TouchAlong = 40.0;
	constexpr double TouchAcross = 30.0;
	constexpr double TouchUp = 120.0;
	constexpr float BumpRest = 2.f;
	// Knocked: it slews round this far (degrees) and he stands shaking his fist this long (s); the load lands this far out (cm).
	constexpr float Slew = 25.f;
	constexpr float AngrySeconds = 7.f;
	constexpr double LoadOut = 150.0;
	// His words are shown this near (cm), for this long (s), as this note.
	constexpr double HearWords = 2500.0;
	constexpr float WordsSeconds = 3.f;
	constexpr int32 WordsKey = 7;
	// Taken: the police see it from this far (cm).
	constexpr double TheftSeen = 2500.0;
	// Gone once this far from the camera (cm) and out of its view.
	constexpr double GoneBeyond = 22000.0;
	constexpr double OutOfView = 0.2;
	// A person's middle above his soles (cm).
	constexpr double MiddleUp = 90.0;
}

AHandCart::AHandCart()
{
	MaxSpeed = HandCartRun::TopSpeed;
	MaxReverseSpeed = HandCartRun::BackSpeed;
	Acceleration = HandCartRun::Pull;
	MaxTurnRate = HandCartRun::TurnRate;
	FullTurnSpeed = HandCartRun::FullTurnAt;
	bPushedByHand = true;
	// It is the lightest thing on wheels there is.
	CarModel::FStyle Look;
	Look.Body = CarModel::EBody::Balilla;
	Look.PaintColor = HandCartModel::CartWood();
	SetStyle(Look);
	BaseMaterial = CarBody::LoadPaintMaterial();
}

void AHandCart::BeginPlay()
{
	Super::BeginPlay();
	using HandCartModel::NumParts;

	// No car body: the box takes the cart's length and width, about its middle.
	Visuals->DrawNothing();
	const FVector Half = HandCartModel::CartHalf();
	const double BoxZ = CollisionBox->GetUnscaledBoxExtent().Z;
	CollisionBox->SetBoxExtent(FVector(Half.X, Half.Y, BoxZ));
	FloorZ = -(BoxZ + HoverHeight);
	DrivenProfile = CollisionBox->GetCollisionProfileName();
	const FVector Origin(-HandCartModel::CartMiddle().X, 0.0, FloorZ);

	PersonKit::FPiece Shapes[NumParts];
	HandCartModel::Build(Shapes, Load);
	for (int32 p = 0; p < NumParts; ++p)
	{
		UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this);
		Mesh->SetupAttachment(CollisionBox);
		const bool bWheel = p == HandCartModel::WheelLeft || p == HandCartModel::WheelRight;
		Mesh->SetRelativeLocation(Origin + (bWheel ? HandCartModel::WheelHub(p) : FVector::ZeroVector));
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCanEverAffectNavigation(false);
		// It moves: kept out of the ray-traced lighting and the distance fields.
		Mesh->bVisibleInRayTracing = false;
		Mesh->bAffectDistanceFieldLighting = false;
		const PersonKit::FPiece& Shape = Shapes[p];
		for (int32 s = 0; s < Shape.Sections.Num(); ++s)
		{
			const PersonKit::FSection& Section = Shape.Sections[s];
			Mesh->CreateMeshSection_LinearColor(s, Section.Vertices, Section.Triangles, Section.Normals,
				TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
			if (BaseMaterial)
			{
				UMaterialInstanceDynamic* Coat = UMaterialInstanceDynamic::Create(BaseMaterial, this);
				Coat->SetVectorParameterValue(TEXT("Color"), Section.Color);
				Mesh->SetMaterial(s, Coat);
			}
		}
		Mesh->RegisterComponent();
		AddInstanceComponent(Mesh);
		Parts.Add(Mesh);
	}
}

void AHandCart::StartInTraffic(ATrafficManager* InManager, int32 InLaneIndex, int32 InPointIndex)
{
	Manager = InManager;
	LaneIndex = InLaneIndex;
	NextLaneIndex = Manager->PickNextLane(LaneIndex);
	PointIndex = InPointIndex;
	Target = KerbSpot(PointIndex);
	Cruise = FMath::FRandRange(HandCartRun::CruiseLow, HandCartRun::CruiseHigh);
	WalkSpeed = Cruise * 0.5f;
	PorterPick = FMath::RandRange(0, 11);
	// Out on the street it does not block: cars pass close by, and bumps are worked out by hand.
	CollisionBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionBox->SetGenerateOverlapEvents(false);
	CameraArm->bDoCollisionTest = false;
	// Start at the kerb, facing along the lane.
	const FVector Spot = KerbSpot(FMath::Max(0, PointIndex - 1));
	const FVector Ahead = (Target - Spot).GetSafeNormal2D();
	SetActorLocation(FVector(Spot.X, Spot.Y, GetActorLocation().Z), false);
	if (!Ahead.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.0, Ahead.Rotation().Yaw, 0.0));
	}
	SnapDown();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Porter = GetWorld()->SpawnActor<APedestrian>(APedestrian::StaticClass(), HandlesSpot(), GetActorRotation(), Params);
	if (Porter)
	{
		Porter->SetOutfit(HandCartModel::PorterLook(PorterPick));
		Porter->SetActorEnableCollision(false);
	}
}

void AHandCart::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (APedestrian* Man : { Porter.Get(), Pusher.Get() })
	{
		if (IsValid(Man))
		{
			Man->Destroy();
		}
	}
	Porter = nullptr;
	Pusher = nullptr;
	Super::EndPlay(EndPlayReason);
}

FVector AHandCart::KerbSpot(int32 Point) const
{
	const FTrafficLane& Lane = Manager->GetLane(LaneIndex);
	const int32 Last = Lane.Points.Num() - 1;
	const int32 At = FMath::Clamp(Point, 0, Last);
	const FVector From = Lane.Points[At > 0 ? At - 1 : 0];
	const FVector To = Lane.Points[At > 0 ? At : FMath::Min(1, Last)];
	const FVector Along = (To - From).GetSafeNormal2D();
	// Traffic keeps left (as in 1932), so the kerb is to the left: X forward, Y right.
	const FVector Left(Along.Y, -Along.X, 0.0);
	FVector Spot = Lane.Points[At];
	Spot.Z = GetActorLocation().Z;
	double Offset = HandCartRun::KerbOffset;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HandCartKerb), false, this);
	if (GetWorld()->LineTraceSingleByObjectType(Hit, Spot, Spot + Left * (Offset + HandCartRun::WallRoom), FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		Offset = FMath::Max(0.0, static_cast<double>(Hit.Distance) - HandCartRun::WallRoom);
	}
	return Spot + Left * Offset;
}

void AHandCart::SnapDown()
{
	const FVector Here = GetActorLocation();
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HandCartGround), false, this);
	if (Porter)
	{
		Params.AddIgnoredActor(Porter);
	}
	if (GetWorld()->LineTraceSingleByChannel(Hit, Here + FVector(0.0, 0.0, 200.0), Here - FVector(0.0, 0.0, 600.0), ECC_Visibility, Params))
	{
		SetActorLocation(FVector(Here.X, Here.Y, Hit.ImpactPoint.Z - FloorZ), false);
	}
}

FVector AHandCart::HandlesSpot() const
{
	const FVector Feet(-HandCartModel::CartMiddle().X + HandCartModel::PorterAt().X, 0.0, FloorZ);
	return GetActorTransform().TransformPosition(Feet + FVector(0.0, 0.0, HandCartRun::MiddleUp));
}

void AHandCart::Tick(float DeltaSeconds)
{
	const FVector Before = GetActorLocation();
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (Manager && PC)
	{
		LeaveTraffic();
	}
	if (Manager)
	{
		// Pushed by its porter along the kerb: none of a car's own driving.
		APawn::Tick(DeltaSeconds);
		if (TickAway(DeltaSeconds))
		{
			return;
		}
		BumpCooldown = FMath::Max(0.f, BumpCooldown - DeltaSeconds);
		if (AngryTime > 0.f)
		{
			AngryTime -= DeltaSeconds;
			WalkSpeed = 0.f;
		}
		else
		{
			FollowLane(DeltaSeconds);
		}
		CheckBumps();
	}
	else
	{
		Super::Tick(DeltaSeconds);
	}
	const FVector Ahead = GetActorForwardVector().GetSafeNormal2D();
	const float Moved = static_cast<float>(FVector::DotProduct(GetActorLocation() - Before, Ahead));
	Animate(DeltaSeconds, Moved);
}

void AHandCart::FollowLane(float DeltaSeconds)
{
	const FTrafficLane* Lane = &Manager->GetLane(LaneIndex);
	const FVector Here = GetActorLocation();
	for (int32 Guard = 0; Guard < 4 && FVector::Dist2D(Here, Target) < HandCartRun::ReachRadius; ++Guard)
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
		Target = KerbSpot(PointIndex);
	}

	const FVector ToTarget = (Target - Here).GetSafeNormal2D();
	const float FacingYaw = static_cast<float>(GetActorRotation().Yaw);
	const float WantedYaw = static_cast<float>(ToTarget.Rotation().Yaw);
	const float TurnNeeded = FRotator::NormalizeAxis(WantedYaw - FacingYaw);
	const float NewYaw = FMath::FixedTurn(FacingYaw, WantedYaw, HandCartRun::TurnSpeed * DeltaSeconds);

	// His own pace, slower round a corner, stopping for what is in front.
	float Wanted = Cruise * FMath::Lerp(1.f, 0.5f, FMath::Clamp(FMath::Abs(TurnNeeded) / 90.f, 0.f, 1.f));
	SqueezeTime = FMath::Max(0.f, SqueezeTime - DeltaSeconds);
	if (SqueezeTime <= 0.f)
	{
		const float Clear = Manager->ClearDistanceAhead(this, HandCartRun::LookAhead, false, HandCartRun::Corridor);
		Wanted *= FMath::Clamp((Clear - HandCartRun::StopGap) / (HandCartRun::LookAhead - HandCartRun::StopGap), 0.f, 1.f);
		StuckTime = Wanted < 10.f ? StuckTime + DeltaSeconds : 0.f;
		if (StuckTime > HandCartRun::MaxStuck)
		{
			StuckTime = 0.f;
			SqueezeTime = HandCartRun::Squeeze;
		}
	}
	WalkSpeed = FMath::FInterpConstantTo(WalkSpeed, Wanted, DeltaSeconds, Wanted < WalkSpeed ? HandCartRun::SlowDown : HandCartRun::SpeedUp);

	SetActorRotation(FRotator(0.f, NewYaw, 0.f));
	SetActorLocation(Here + GetActorForwardVector().GetSafeNormal2D() * WalkSpeed * DeltaSeconds, false);
	SnapDown();
}

void AHandCart::CheckBumps()
{
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (BumpCooldown > 0.f || !Player || Player == this || Player->GetAttachParentActor())
	{
		return;
	}
	FVector Extent(40.0, 40.0, 90.0);
	if (const UBoxComponent* Box = Cast<UBoxComponent>(Player->GetRootComponent()))
	{
		Extent = Box->GetScaledBoxExtent();
	}
	else if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Player->GetRootComponent()))
	{
		Extent = FVector(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight());
	}
	// The cart's own box against the player's, roughly: his middle near enough to it.
	const FVector Mine = CollisionBox->GetScaledBoxExtent();
	const FVector Local = GetActorTransform().InverseTransformPositionNoScale(Player->GetActorLocation());
	const double Reach = FMath::Max(Extent.X, Extent.Y);
	if (FMath::Abs(Local.X) > Mine.X + Reach + HandCartRun::TouchAlong || FMath::Abs(Local.Y) > Mine.Y + Reach + HandCartRun::TouchAcross
		|| FMath::Abs(Local.Z) > Extent.Z + HandCartRun::TouchUp)
	{
		return;
	}
	const float Impact = static_cast<float>((Player->GetVelocity() - GetVelocity()).Size2D());
	if (Impact > HandCartRun::KnockSpeed && !bLoadOff)
	{
		KnockLoadOff(Player->GetActorLocation());
	}
	else if (Impact > HandCartRun::NudgeSpeed)
	{
		CallOut(TEXT("\u201eH\u00e9, vigy\u00e1zzon m\u00e1r!\u201d   (Hey, watch it!)"));
	}
	BumpCooldown = HandCartRun::BumpRest;
}

void AHandCart::KnockLoadOff(const FVector& From)
{
	bLoadOff = true;
	AngryTime = HandCartRun::AngrySeconds;
	WalkSpeed = 0.f;
	// The cart slews round away from the blow.
	const FVector Away = (GetActorLocation() - From).GetSafeNormal2D();
	const double Side = FVector::DotProduct(Away, GetActorRightVector()) >= 0.0 ? 1.0 : -1.0;
	SetActorRotation(GetActorRotation() + FRotator(0.0, Side * HandCartRun::Slew, 0.0));
	SetActorLocation(GetActorLocation() + Away * 60.0, false);
	SnapDown();
	// The load goes over the side onto the road, tipped on its side, and stays there.
	if (Parts.IsValidIndex(HandCartModel::Cargo) && Parts[HandCartModel::Cargo])
	{
		UProceduralMeshComponent* Heap = Parts[HandCartModel::Cargo];
		const FVector Land = GetActorLocation() + Away * HandCartRun::LoadOut + FVector(0.0, 0.0, FloorZ - HandCartModel::BedZ + 10.0);
		Heap->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		Heap->SetWorldLocationAndRotation(Land, FRotator(0.0, GetActorRotation().Yaw + FMath::FRandRange(-30.f, 30.f), Side * 12.0));
	}
	CallOut(TEXT("\u201eAz \u00e1ld\u00f3j\u00e1t! Ki fizeti ezt meg nekem?!\u201d   (Blast you! Who's going to pay me for this?!)"));
	APedestrians::Gather(this, GetActorLocation(), 15.f, false);
}

void AHandCart::LeaveTraffic()
{
	Manager = nullptr;
	AngryTime = 0.f;
	CollisionBox->SetCollisionProfileName(DrivenProfile);
	CameraArm->bDoCollisionTest = true;
	const FVector Location = GetActorLocation();
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector Danger = Player ? Player->GetActorLocation() : Location;

	// The porter lets go and comes after him shouting.
	const FVector PorterSpot = Porter ? Porter->GetActorLocation() : HandlesSpot();
	if (IsValid(Porter))
	{
		Porter->Destroy();
	}
	Porter = nullptr;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Kerb = -GetActorRightVector().GetSafeNormal2D();
	if (ACarjackedDriver* Robbed = GetWorld()->SpawnActor<ACarjackedDriver>(ACarjackedDriver::StaticClass(), PorterSpot + Kerb * 120.0, GetActorRotation(), Params))
	{
		const PersonKit::FOutfit Clothes = HandCartModel::PorterLook(PorterPick);
		Robbed->SetOutfit(Clothes);
		Robbed->AfterThief(Danger, true, Clothes);
	}
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->Say(TEXT("Hord\u00e1r"), TEXT("H\u00e9! A kocsim! \u00c1llj meg, tolvaj!"), TEXT("Hey! My cart! Stop, thief!"), 3.f);
	}
	APestiAlvilagGameModeBase::Deed(this, StreetName::EDeed::CartStolen, Location);
	if (APolicePatrols::Witnessed(this, Location, HandCartRun::TheftSeen))
	{
		if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
		{
			Game->ReportCrime(1);
		}
	}
	else
	{
		AWitnesses::Report(this, Location, 1);
	}
}

bool AHandCart::TickAway(float DeltaSeconds)
{
	AwayClock -= DeltaSeconds;
	if (AwayClock > 0.f)
	{
		return false;
	}
	AwayClock = 1.f;
	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!Camera)
	{
		return false;
	}
	const FVector ToCart = GetActorLocation() - Camera->GetCameraLocation();
	if (ToCart.Size2D() < HandCartRun::GoneBeyond
		|| FVector::DotProduct(ToCart.GetSafeNormal(), Camera->GetCameraRotation().Vector()) > HandCartRun::OutOfView)
	{
		return false;
	}
	Destroy();
	return true;
}

void AHandCart::Animate(float DeltaSeconds, float Moved)
{
	using namespace HandCartModel;
	// The wheels roll as far as it went.
	WheelTurn = FMath::Fmod(WheelTurn + Moved, static_cast<float>(2.0 * PI * WheelRadius));
	const float Spin = -FMath::RadiansToDegrees(WheelTurn / static_cast<float>(WheelRadius));
	for (const int32 w : { static_cast<int32>(WheelLeft), static_cast<int32>(WheelRight) })
	{
		if (Parts.IsValidIndex(w) && Parts[w])
		{
			Parts[w]->SetRelativeRotation(FRotator(Spin, 0.f, 0.f));
		}
	}

	// Pushing: the arms forward and down to the grips, the legs walking.
	DailyLifeModel::FLimbs Push;
	Push.Arms[0] = FRotator(20.f, 0.f, 6.f);
	Push.Arms[1] = FRotator(20.f, 0.f, -6.f);
	Push.Elbows[0] = 8.f;
	Push.Elbows[1] = 8.f;
	const float Pace = DeltaSeconds > 0.f ? FMath::Abs(Moved) / DeltaSeconds : 0.f;
	const double Heading = GetActorRotation().Yaw;
	if (Manager)
	{
		if (!Porter)
		{
			return;
		}
		if (AngryTime > 0.f)
		{
			const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
			const FVector At = Porter->GetActorLocation();
			const double Facing = Player ? (Player->GetActorLocation() - At).GetSafeNormal2D().Rotation().Yaw : Heading;
			Porter->ShakeFist(At, Facing, AngryTime);
			return;
		}
		Porter->WalkTo(HandlesSpot(), Heading, Pace, DeltaSeconds);
		Porter->ArmsAs(Push);
		return;
	}

	// The player at the handles while he pushes it, gone when he lets go.
	const bool bDriven = Cast<APlayerController>(GetController()) != nullptr;
	if (!bDriven)
	{
		if (IsValid(Pusher) && !Pusher->IsHidden())
		{
			Pusher->SetActorHiddenInGame(true);
		}
		return;
	}
	if (!IsValid(Pusher))
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Pusher = GetWorld()->SpawnActor<APedestrian>(APedestrian::StaticClass(), HandlesSpot(), GetActorRotation(), Params);
		if (!Pusher)
		{
			return;
		}
		Pusher->SetOutfit(PersonModel::PlayerOutfit());
		Pusher->SetActorEnableCollision(false);
	}
	if (Pusher->IsHidden())
	{
		Pusher->SetActorHiddenInGame(false);
	}
	Pusher->WalkTo(HandlesSpot(), Heading, Pace, DeltaSeconds);
	Pusher->ArmsAs(Push);
}

void AHandCart::CallOut(const FString& Text) const
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Player && FVector::Dist(Player->GetActorLocation(), GetActorLocation()) < HandCartRun::HearWords)
	{
		APestiHUD::Show(this, HandCartRun::WordsKey, HandCartRun::WordsSeconds, FColor::Orange, Text);
	}
}
