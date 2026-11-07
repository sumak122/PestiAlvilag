// Fill out your copyright notice in the Description page of Project Settings.

#include "Bicycle.h"

#include "BicycleModel.h"
#include "Camera/CameraComponent.h"
#include "CarBody.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SpotLightComponent.h"
#include "DayNight.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PersonModel.h"
#include "PestiHud.h"
#include "PestiMenu.h"
#include "PestiPerson.h"
#include "ProceduralMeshComponent.h"

namespace BicycleRide
{
	// Pedalling up to a brisk 27 km/h (cm/s, and cm/s each second); rolling on, it slows a little.
	constexpr float TopSpeed = 750.f;
	constexpr float PedalPush = 230.f;
	constexpr float RollDrag = 35.f;
	constexpr float Brake = 450.f;
	constexpr float HardBrake = 750.f;
	// Stopped, S walks it back slowly.
	constexpr float BackSpeed = 60.f;

	// The bars turn this far at a walking pace, less at full speed (degrees), this fast (degrees a second).
	constexpr float MaxSteerSlow = 35.f;
	constexpr float MaxSteerFast = 8.f;
	constexpr float SlowFrom = 150.f;
	constexpr float SteerRate = 90.f;
	// It leans into turns as far as this (degrees), getting there this quickly.
	constexpr float MaxLean = 28.f;
	constexpr float LeanRate = 6.f;
	constexpr float Gravity = 980.f;
	// Left standing, it settles onto its kickstand this quickly (degrees a second).
	constexpr float StandRate = 160.f;
	constexpr float ParkSteerRate = 30.f;

	// The dynamo lamp: dark below FirstGlow (cm/s), full by FullGlow; as bright as a car's
	// headlamp by day and at night (times a lit grey wall), and its beam at full.
	constexpr float FirstGlow = 40.f;
	constexpr float FullGlow = 450.f;
	const FLinearColor LampLight(1.f, 0.84f, 0.55f);
	constexpr float LampDay = 5.f;
	constexpr float LampNight = 30.f;
	constexpr float BeamStrength = 160.f;
	constexpr float BeamReach = 2200.f;
	constexpr float BeamDip = 8.f;
	// The glow is set again only when it has changed by this much.
	constexpr float GlowStep = 0.04f;

	// One turn of the cranks rolls it this far (cm): the rear wheel's round, geared up.
	constexpr float GearRatio = 2.4f;

	// He gets off only nearly stopped, this far to the left of the saddle (cm).
	constexpr float MaxExitSpeed = 80.f;
	constexpr double ExitSide = 80.0;
	constexpr double MinTimeOnSaddle = 0.3;
	// Put down at once (a mission ending): beside it, else behind it (cm).
	constexpr double ExitBehind = 150.0;

	// Riding into something head on faster than this (cm/s, about 14 km/h) is a hard knock.
	constexpr float HardKnockSpeed = 400.f;

	constexpr float ArmLength = 520.f;
	constexpr float ChasePitch = -12.f;
	constexpr float CameraHeight = 140.f;
	constexpr float MouseLookScale = 2.5f;
	constexpr float StickLookRate = 150.f;
	constexpr float MinLookPitch = -50.f;
	constexpr float MaxLookPitch = 20.f;
	constexpr float LookResetDelay = 2.f;
	constexpr float LookResetRate = 120.f;
}

ABicycle::ABicycle()
{
	PrimaryActorTick.bCanEverTick = true;
	BaseMaterial = CarBody::LoadPaintMaterial();

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetBoxExtent(BicycleModel::BoxHalf());
	CollisionBox->SetCollisionProfileName(TEXT("Pawn"));
	RootComponent = CollisionBox;

	Rig = CreateDefaultSubobject<USceneComponent>(TEXT("Rig"));
	Rig->SetupAttachment(CollisionBox);
	Rig->SetRelativeLocation(FVector(-BicycleModel::BoxMiddle().X, 0.0, -(BicycleModel::BoxHalf().Z + BicycleModel::BoxBottom)));

	SteerPivot = CreateDefaultSubobject<USceneComponent>(TEXT("SteerPivot"));
	SteerPivot->SetupAttachment(Rig);
	SteerTurn = CreateDefaultSubobject<USceneComponent>(TEXT("SteerTurn"));
	SteerTurn->SetupAttachment(SteerPivot);
	SteerBody = CreateDefaultSubobject<USceneComponent>(TEXT("SteerBody"));
	SteerBody->SetupAttachment(SteerTurn);

	RiderRoot = CreateDefaultSubobject<USceneComponent>(TEXT("RiderRoot"));
	RiderRoot->SetupAttachment(Rig);
	RiderBody = CreateDefaultSubobject<USceneComponent>(TEXT("RiderBody"));
	RiderBody->SetupAttachment(RiderRoot);

	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	CameraArm->SetupAttachment(CollisionBox);
	CameraArm->TargetArmLength = BicycleRide::ArmLength;
	CameraArm->SetRelativeRotation(FRotator(BicycleRide::ChasePitch, 0.f, 0.f));
	CameraArm->TargetOffset = FVector(0.f, 0.f, BicycleRide::CameraHeight);
	CameraArm->bUsePawnControlRotation = false;
	CameraArm->bInheritRoll = false;
	CameraArm->bEnableCameraLag = true;
	CameraArm->CameraLagSpeed = 8.f;
	CameraArm->bEnableCameraRotationLag = true;
	CameraArm->CameraRotationLagSpeed = 5.f;

	ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
	ChaseCamera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
}

void ABicycle::BeginPlay()
{
	Super::BeginPlay();
	using namespace BicycleModel;
	const EKind Kind = bShopBike ? EKind::ShopBike : EKind::Roadster;
	const FVector Head = HeadBottom(Kind);
	const float Tilt = static_cast<float>(SteerTilt(Kind));

	// The steering: tilted back along its axis, turned, and tilted upright again for the pieces.
	SteerPivot->SetRelativeLocationAndRotation(Head, FRotator(Tilt, 0.f, 0.f));
	SteerBody->SetRelativeRotation(FRotator(-Tilt, 0.f, 0.f));

	FBicycle Bike;
	Build(Bike, Kind, 0);
	Parts.SetNum(NumParts);
	Parts[Frame] = MakeMesh(Rig.Get(), Bike.Parts[Frame], FVector::ZeroVector);
	Parts[Steer] = MakeMesh(SteerBody.Get(), Bike.Parts[Steer], FVector::ZeroVector);
	Parts[WheelFront] = MakeMesh(SteerBody.Get(), Bike.Parts[WheelFront], FrontAxle(Kind) - Head);
	Parts[WheelRear] = MakeMesh(Rig.Get(), Bike.Parts[WheelRear], RearAxle());
	Parts[Cranks] = MakeMesh(Rig.Get(), Bike.Parts[Cranks], BottomBracket());
	PersonKit::FPiece Glass;
	BuildLens(Glass, Kind);
	LensMesh = MakeMesh(SteerBody.Get(), Glass, FVector::ZeroVector);
	LensPaint = LensMesh->GetMaterial(0);
	PersonKit::FPiece Kickstand;
	BuildStand(Kickstand);
	StandMesh = MakeMesh(Rig.Get(), Kickstand, StandHinge());
	StandPitch = static_cast<float>(StandDown);
	Lean = static_cast<float>(ParkedLean);
	SteerAngle = static_cast<float>(ParkedSteer);

	// The parcels, in the basket on the frame, or on the fork of a roadster.
	for (int32 i = 0; i < MaxParcels; ++i)
	{
		PersonKit::FPiece Parcel;
		BuildParcel(Parcel, FVector::ZeroVector, ParcelHalf(i));
		UProceduralMeshComponent* Mesh = BasketTurns(Kind)
			? MakeMesh(SteerBody.Get(), Parcel, ParcelSpot(Kind, i) - Head)
			: MakeMesh(Rig.Get(), Parcel, ParcelSpot(Kind, i));
		Mesh->SetVisibility(false);
		ParcelMeshes.Add(Mesh);
	}

	// The rider: the player's figure, his hips on the saddle, leaning forward from them.
	const PersonKit::FOutfit Outfit = PersonModel::PlayerOutfit();
	const double HipZ = 92.0 * Outfit.Height / 178.0;
	RiderRoot->SetRelativeLocationAndRotation(RiderHip(), FRotator(static_cast<float>(RiderLean), 0.f, 0.f));
	RiderBody->SetRelativeLocation(FVector(0.0, 0.0, PersonModel::MiddleHeight - HipZ));
	PersonModel::Dress(this, RiderBody.Get(), BaseMaterial, Outfit, RiderPieces);
	for (UProceduralMeshComponent* Piece : RiderPieces)
	{
		if (Piece)
		{
			Piece->SetVisibility(false);
		}
	}

	SnapToGround();
	Animate(0.f);
}

UProceduralMeshComponent* ABicycle::MakeMesh(USceneComponent* Parent, const PersonKit::FPiece& Piece, const FVector& At)
{
	UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this);
	Mesh->SetupAttachment(Parent);
	Mesh->SetRelativeLocation(At);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->RegisterComponent();
	AddInstanceComponent(Mesh);
	for (int32 i = 0; i < Piece.Sections.Num(); ++i)
	{
		const PersonKit::FSection& Section = Piece.Sections[i];
		Mesh->CreateMeshSection_LinearColor(i, Section.Vertices, Section.Triangles, Section.Normals,
			TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Paint->SetVectorParameterValue(TEXT("Color"), Section.Color);
			Mesh->SetMaterial(i, Paint);
		}
	}
	return Mesh;
}

void ABicycle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		Ride(PC, DeltaSeconds);
		if (GetController() != PC)
		{
			return;
		}
	}
	else
	{
		// Nobody on it: it rolls to a stop, and stands on its kickstand with the bars turned a little.
		bPedalling = false;
		Speed = FMath::FInterpConstantTo(Speed, 0.f, DeltaSeconds, BicycleRide::Brake);
		const float ParkedBars = FMath::Abs(Speed) < 5.f ? static_cast<float>(BicycleModel::ParkedSteer) : 0.f;
		SteerAngle = FMath::FInterpConstantTo(SteerAngle, ParkedBars, DeltaSeconds, BicycleRide::ParkSteerRate);
	}
	Move(DeltaSeconds);
	Animate(DeltaSeconds);
}

void ABicycle::Ride(APlayerController* PC, float DeltaSeconds)
{
	using namespace BicycleRide;
	auto KeyAxis = [PC](const FKey& Positive, const FKey& Negative)
	{
		return (PC->IsInputKeyDown(Positive) ? 1.f : 0.f) - (PC->IsInputKeyDown(Negative) ? 1.f : 0.f);
	};

	if (PC->WasInputKeyJustPressed(EKeys::F) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right))
	{
		LetRiderOff(PC);
		if (GetController() != PC)
		{
			return;
		}
	}

	const bool bForward = PC->IsInputKeyDown(EKeys::W) || PC->IsInputKeyDown(EKeys::Up) || PC->IsInputKeyDown(EKeys::Gamepad_RightTrigger);
	const bool bBack = PC->IsInputKeyDown(EKeys::S) || PC->IsInputKeyDown(EKeys::Down) || PC->IsInputKeyDown(EKeys::Gamepad_LeftTrigger);
	const bool bStop = PC->IsInputKeyDown(EKeys::SpaceBar) || PC->IsInputKeyDown(EKeys::Gamepad_FaceButton_Bottom);
	bPedalling = bForward && !bBack && !bStop && Speed >= 0.f;
	if (bStop)
	{
		Speed = FMath::FInterpConstantTo(Speed, 0.f, DeltaSeconds, HardBrake);
	}
	else if (bBack)
	{
		// Braking while it rolls; stopped, walking it back.
		Speed = Speed > 5.f ? FMath::FInterpConstantTo(Speed, 0.f, DeltaSeconds, Brake) : FMath::FInterpConstantTo(Speed, -BackSpeed, DeltaSeconds, Brake);
	}
	else if (bPedalling)
	{
		// Hardest to push on when it is already going fast.
		const float Push = PedalPush * (1.f - 0.6f * FMath::Clamp(Speed / TopSpeed, 0.f, 1.f));
		Speed = FMath::FInterpConstantTo(Speed, TopSpeed, DeltaSeconds, Push);
	}
	else
	{
		Speed = FMath::FInterpConstantTo(Speed, 0.f, DeltaSeconds, Speed < 0.f ? Brake : RollDrag);
	}

	const float Steer = FMath::Clamp(KeyAxis(EKeys::D, EKeys::A) + KeyAxis(EKeys::Right, EKeys::Left)
		+ PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX), -1.f, 1.f);
	const float Fast = FMath::Clamp((FMath::Abs(Speed) - SlowFrom) / (TopSpeed - SlowFrom), 0.f, 1.f);
	const float MaxSteer = FMath::Lerp(MaxSteerSlow, MaxSteerFast, Fast);
	SteerAngle = FMath::FInterpConstantTo(SteerAngle, Steer * MaxSteer, DeltaSeconds, SteerRate);

	UpdateFreeLook(PC, DeltaSeconds);

	const int32 Kmh = FMath::RoundToInt(FMath::Abs(Speed) * 0.036f);
	APestiHUD::Show(this, 1, 0.f, FColor::White,
		FString::Printf(TEXT("Ker\u00e9kp\u00e1r / bicycle: %d km/h   (W pedal, S brake, A/D steer, Space stop, F get off)"), Kmh));
}

void ABicycle::Move(float DeltaSeconds)
{
	using namespace BicycleRide;
	const BicycleModel::EKind Kind = bShopBike ? BicycleModel::EKind::ShopBike : BicycleModel::EKind::Roadster;
	const double Wheelbase = BicycleModel::FrontAxle(Kind).X - BicycleModel::RearAxle().X;
	const double TurnRate = Speed * FMath::Tan(FMath::DegreesToRadians(SteerAngle)) / Wheelbase;

	// It leans into the turn as far as the turn throws it outward.
	// Left standing, it leans over onto its kickstand.
	const bool bParked = !Rider && FMath::Abs(Speed) < 5.f;
	const float WantLean = bParked ? static_cast<float>(BicycleModel::ParkedLean)
		: FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(Speed * TurnRate), Gravity)), -MaxLean, MaxLean);
	Lean = FMath::FInterpTo(Lean, WantLean, DeltaSeconds, LeanRate);
	const float WantStand = static_cast<float>(Rider ? BicycleModel::StandFolded : BicycleModel::StandDown);
	StandPitch = FMath::FInterpConstantTo(StandPitch, WantStand, DeltaSeconds, StandRate);

	if (FMath::IsNearlyZero(Speed))
	{
		return;
	}
	const double Turn = FMath::RadiansToDegrees(TurnRate * DeltaSeconds);
	const FVector Rear = Rig->GetComponentTransform().TransformPosition(BicycleModel::RearAxle());
	const FVector Pivot(Rear.X, Rear.Y, GetActorLocation().Z);
	const FRotator Facing = GetActorRotation() + FRotator(0.0, Turn, 0.0);
	const FVector Swung = Pivot + FRotator(0.0, Turn, 0.0).RotateVector(GetActorLocation() - Pivot);
	const FVector Delta = Swung - GetActorLocation() + Facing.Vector().GetSafeNormal2D() * Speed * DeltaSeconds;

	SetActorRotation(Facing);
	FHitResult Hit;
	AddActorWorldOffset(Delta, true, &Hit);
	if (Hit.bStartPenetrating)
	{
		AddActorWorldOffset(Hit.Normal * (Hit.PenetrationDepth + 1.f), false);
	}
	else if (Hit.bBlockingHit)
	{
		const FVector WallNormal = FVector(Hit.Normal.X, Hit.Normal.Y, 0.f).GetSafeNormal();
		const float Impact = static_cast<float>(FMath::Abs(FVector::DotProduct(GetActorForwardVector(), WallNormal)));
		if (Impact > 0.6f)
		{
			if (FMath::Abs(Speed) > HardKnockSpeed)
			{
				++HardKnocks;
			}
			Speed = 0.f;
		}
		else
		{
			Speed *= 1.f - 0.5f * Impact;
			AddActorWorldOffset(FVector::VectorPlaneProject(Delta * (1.f - Hit.Time), WallNormal), true);
		}
	}
	SnapToGround();
}

void ABicycle::Animate(float DeltaSeconds)
{
	using namespace BicycleModel;
	const EKind Kind = bShopBike ? EKind::ShopBike : EKind::Roadster;
	Rig->SetRelativeRotation(FRotator(0.f, 0.f, Lean));
	SteerTurn->SetRelativeRotation(FRotator(0.f, SteerAngle, 0.f));

	const float Step = Speed * DeltaSeconds;
	Rolled = FMath::Fmod(Rolled + Step, static_cast<float>(2.0 * PI * RearRadius * FrontRadius(Kind)));
	Parts[WheelRear]->SetRelativeRotation(FRotator(-FMath::RadiansToDegrees(Rolled / static_cast<float>(RearRadius)), 0.f, 0.f));
	Parts[WheelFront]->SetRelativeRotation(FRotator(-FMath::RadiansToDegrees(Rolled / static_cast<float>(FrontRadius(Kind))), 0.f, 0.f));
	if (bPedalling && Step > 0.f)
	{
		const float CrankTravel = static_cast<float>(2.0 * PI * RearRadius) * BicycleRide::GearRatio;
		Crank = FMath::Fmod(Crank + Step / CrankTravel * 360.f, 360.f);
	}
	Parts[Cranks]->SetRelativeRotation(FRotator(-Crank, 0.f, 0.f));
	if (StandMesh)
	{
		StandMesh->SetRelativeRotation(FRotator(StandPitch, 0.f, 0.f));
	}
	UpdateLamp();
	if (Rider)
	{
		PoseRider();
	}
}

void ABicycle::UpdateLamp()
{
	using namespace BicycleRide;
	ADayNight* Clock = ADayNight::Get(GetWorld());
	if (!Clock || !LensMesh)
	{
		return;
	}
	// The dynamo makes more light the faster the wheel turns.
	const float Glow = FMath::Clamp((FMath::Abs(Speed) - FirstGlow) / (FullGlow - FirstGlow), 0.f, 1.f);
	const float Darkness = Clock->GetDarkness();
	if (Glow <= 0.f)
	{
		if (LampGlow > 0.f)
		{
			LensMesh->SetMaterial(0, LensPaint.Get());
			LampGlow = 0.f;
		}
	}
	else if (LampGlow <= 0.f || FMath::Abs(Glow - LampGlow) > GlowStep || FMath::Abs(Darkness - LampDarkness) > GlowStep)
	{
		if (!LensGlow)
		{
			LensGlow = Clock->MakeGlowMaterial(this);
		}
		if (LensGlow)
		{
			Clock->SetGlow(LensGlow, LampLight, FMath::Lerp(LampDay, LampNight, Darkness) * Glow);
			if (LampGlow <= 0.f)
			{
				LensMesh->SetMaterial(0, LensGlow.Get());
			}
		}
		LampGlow = Glow;
		LampDarkness = Darkness;
	}

	// A beam on the road ahead, at night, for the bicycle someone is riding.
	const bool bWantBeam = Glow > 0.f && Rider && Clock->AreLampsLit();
	if (bWantBeam && !Beam)
	{
		const BicycleModel::EKind Kind = bShopBike ? BicycleModel::EKind::ShopBike : BicycleModel::EKind::Roadster;
		Beam = NewObject<USpotLightComponent>(this);
		Beam->SetupAttachment(SteerBody);
		Beam->SetRelativeLocationAndRotation(BicycleModel::LampAt(Kind) - BicycleModel::HeadBottom(Kind) + FVector(4.0, 0.0, 0.0), FRotator(-BeamDip, 0.f, 0.f));
		Beam->SetMobility(EComponentMobility::Movable);
		Beam->SetCastShadows(false);
		Beam->SetIntensityUnits(ELightUnits::Candelas);
		Beam->SetAttenuationRadius(BeamReach);
		Beam->SetInnerConeAngle(8.f);
		Beam->SetOuterConeAngle(22.f);
		Beam->SetLightColor(LampLight);
		Beam->RegisterComponent();
		AddInstanceComponent(Beam);
	}
	if (Beam)
	{
		Beam->SetVisibility(bWantBeam);
		if (bWantBeam)
		{
			Beam->SetIntensity(Clock->LampCandela(BeamStrength) * Glow);
		}
	}
}

void ABicycle::PoseRider()
{
	using namespace PersonKit;
	const BicycleModel::EKind Kind = bShopBike ? BicycleModel::EKind::ShopBike : BicycleModel::EKind::Roadster;
	const BicycleModel::FRiderPose Pose = BicycleModel::RiderPose(Kind, Crank, PersonModel::PlayerOutfit().Height);
	auto Turn = [this](int32 Piece, double Pitch)
	{
		if (RiderPieces.IsValidIndex(Piece) && RiderPieces[Piece])
		{
			RiderPieces[Piece]->SetRelativeRotation(FRotator(static_cast<float>(Pitch), 0.f, 0.f));
		}
	};
	for (int32 s = 0; s < 2; ++s)
	{
		const int32 Leg = s == 0 ? LegLeft : LegRight;
		const int32 Arm = s == 0 ? ArmLeft : ArmRight;
		Turn(Leg, Pose.Thigh[s]);
		Turn(ShinOf(Leg), Pose.Shin[s]);
		Turn(SkirtOf(Leg), Pose.Thigh[s] * BicycleModel::RiderSkirtFollow);
		Turn(Arm, Pose.Arm[s]);
		Turn(ForearmOf(Arm), Pose.Forearm[s]);
	}
}

void ABicycle::UpdateFreeLook(const APlayerController* PC, float DeltaSeconds)
{
	using namespace BicycleRide;
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
		LookPitch = FMath::Clamp(LookPitch + TurnY, MinLookPitch - ChasePitch, MaxLookPitch - ChasePitch);
	}
	else
	{
		TimeSinceLook += DeltaSeconds;
		if (TimeSinceLook > LookResetDelay && FMath::Abs(Speed) > SlowFrom)
		{
			LookYaw = FMath::FixedTurn(LookYaw, 0.f, LookResetRate * DeltaSeconds);
			LookPitch = FMath::FInterpConstantTo(LookPitch, 0.f, DeltaSeconds, LookResetRate);
		}
	}
	CameraArm->SetRelativeRotation(FRotator(ChasePitch + LookPitch, LookYaw, 0.f));
}

FVector ABicycle::GetVelocity() const
{
	return GetActorForwardVector() * Speed;
}

void ABicycle::TakeRider(APawn* InRider)
{
	Rider = InRider;
	RiderOnTime = GetWorld()->GetTimeSeconds();
	for (UProceduralMeshComponent* Piece : RiderPieces)
	{
		if (Piece)
		{
			Piece->SetVisibility(true);
		}
	}
	PoseRider();
	LookYaw = 0.f;
	LookPitch = 0.f;
}

bool ABicycle::LetRiderOff(APlayerController* PC, bool bForce)
{
	APestiPerson* Person = Cast<APestiPerson>(Rider);
	const bool bJustOn = GetWorld()->GetTimeSeconds() - RiderOnTime < BicycleRide::MinTimeOnSaddle;
	if (!Person || !PC || (!bForce && (bJustOn || FMath::Abs(Speed) > BicycleRide::MaxExitSpeed)))
	{
		return false;
	}
	const FTransform ToWorld = Rig->GetComponentTransform();
	const double SaddleX = BicycleModel::RiderHip().X;
	TArray<FVector, TInlineAllocator<3>> Spots;
	Spots.Add(FVector(SaddleX, -BicycleRide::ExitSide, 0.0));
	Spots.Add(FVector(SaddleX, BicycleRide::ExitSide, 0.0));
	if (bForce)
	{
		Spots.Add(FVector(SaddleX - BicycleRide::ExitBehind, 0.0, 0.0));
	}
	for (const FVector& Spot : Spots)
	{
		if (Person->TryStepOutAt(ToWorld.TransformPosition(Spot), GetActorRotation().Yaw))
		{
			Speed = 0.f;
			bPedalling = false;
			Rider = nullptr;
			for (UProceduralMeshComponent* Piece : RiderPieces)
			{
				if (Piece)
				{
					Piece->SetVisibility(false);
				}
			}
			PC->Possess(Person);
			PC->SetControlRotation(FRotator(-10.f, GetActorRotation().Yaw, 0.f));
			return true;
		}
	}
	return false;
}

void ABicycle::PutRiderOff()
{
	LetRiderOff(Cast<APlayerController>(GetController()), true);
}

FVector ABicycle::GetSaddleLocation() const
{
	return Rig->GetComponentTransform().TransformPosition(FVector(BicycleModel::RiderHip().X, 0.0, 0.0));
}

void ABicycle::SetParcels(int32 Count)
{
	Parcels = FMath::Clamp(Count, 0, BicycleModel::MaxParcels);
	for (int32 i = 0; i < ParcelMeshes.Num(); ++i)
	{
		ParcelMeshes[i]->SetVisibility(i < Parcels);
	}
}

void ABicycle::SnapToGround()
{
	const FVector Start = GetActorLocation();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(BicycleGround), false, this);
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, Start - FVector(0.f, 0.f, 2000.f), ECC_Visibility, Params))
	{
		FVector Location = Start;
		Location.Z = Hit.ImpactPoint.Z + BicycleModel::BoxBottom + CollisionBox->GetScaledBoxExtent().Z;
		SetActorLocation(Location, false);
	}
}
