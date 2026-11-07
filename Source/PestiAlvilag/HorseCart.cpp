// Fill out your copyright notice in the Description page of Project Settings.

#include "HorseCart.h"

#include "Camera/CameraComponent.h"
#include "CarBody.h"
#include "CarModel.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HorseSound.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PersonModel.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "ProceduralMeshComponent.h"
#include "PestiMenu.h"

namespace HorseCart
{
	// The collision box floats this far above the road; the wheels and hooves reach down to it (cm).
	constexpr float HoverHeight = 15.f;

	// The paces: standing, walk, trot, canter (cm/s; a canter is about 20 km/h),
	// a full gallop (about 30 km/h), and backing.
	const float PaceSpeeds[] = { 0.f, 150.f, 330.f, 560.f, 840.f };
	constexpr int32 TopPace = 4;
	constexpr float BackSpeed = 70.f;
	// Picking up and slowing down (cm/s each second), and pulling up hard with Space.
	constexpr float SpeedUp = 170.f;
	constexpr float SlowDown = 300.f;
	constexpr float PullUp = 480.f;

	// The shafts turn this far at most at each pace (degrees), this fast (degrees a second).
	const float MaxSteer[] = { 32.f, 32.f, 24.f, 15.f, 10.f };
	constexpr float SteerRate = 70.f;
	// From the rear axle to where the horse pulls: how tight it turns (cm).
	constexpr double Wheelbase = 300.0;

	// Which gait the horse is in at a speed, and the ground one stride covers (cm).
	constexpr float WalkFrom = 12.f;
	constexpr float TrotFrom = 240.f;
	constexpr float CanterFrom = 450.f;
	constexpr float GallopFrom = 700.f;
	constexpr float WalkStride = 170.f;
	constexpr float TrotStride = 260.f;
	constexpr float CanterStride = 330.f;
	// At a gallop the same leaps, longer and quicker.
	constexpr float GallopStride = 430.f;

	// He gets down only nearly stopped, and this far to the side of the bench (cm).
	constexpr float MaxExitSpeed = 60.f;
	constexpr double ExitSide = 150.0;
	constexpr double MinTimeOnBench = 0.3;

	constexpr float ArmLength = 1050.f;
	constexpr float ChasePitch = -14.f;
	constexpr float CameraHeight = 220.f;
	constexpr float MouseLookScale = 2.5f;
	constexpr float StickLookRate = 150.f;
	constexpr float MinLookPitch = -50.f;
	constexpr float MaxLookPitch = 20.f;
	constexpr float LookResetDelay = 2.f;
	constexpr float LookResetRate = 120.f;

	// The driver sits with his hands forward on the reins (degrees), his feet on the footboard.
	constexpr double DriverArmPitch = 48.0;
	constexpr double DriverLegPitch = 30.0;
	/** His hands, from his hips (cm), and the reins' thickness. */
	const FVector HandsAt(40.0, 12.0, 16.0);
	constexpr double ReinRadius = 0.7;

	// A crate falls off when the cart runs into something faster than this
	// (cm/s), or when the load has swayed enough in hard turns at a canter:
	// each second turned past SwayFrom degrees adds its share of a shake.
	constexpr float SpillSpeed = 220.f;
	constexpr double SpillGap = 1.0;
	constexpr float SwayFrom = 8.f;
	constexpr float SwayPerDegree = 0.05f;
	constexpr float SwaySettle = 0.5f;
	// A fallen crate lands this far out from the side of the bed (cm).
	constexpr double FallOut = 70.0;
	// Near enough to the bed to lift a crate onto it (cm past its edges).
	constexpr double LoadReach = 140.0;

	// The sound: heard this far (cm); standing, he snorts every so often (s).
	constexpr float SoundFalloff = 3500.f;
	constexpr float MinSnortWait = 7.f;
	constexpr float MaxSnortWait = 20.f;
}

AHorseCart::AHorseCart()
{
	PrimaryActorTick.bCanEverTick = true;
	BaseMaterial = CarBody::LoadPaintMaterial();

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetBoxExtent(HorseCartModel::RigHalf());
	CollisionBox->SetCollisionProfileName(TEXT("Pawn"));
	RootComponent = CollisionBox;

	Rig = CreateDefaultSubobject<USceneComponent>(TEXT("Rig"));
	Rig->SetupAttachment(CollisionBox);
	Rig->SetRelativeLocation(FVector(-HorseCartModel::RigMiddleX, 0.0, -(HorseCartModel::RigHalf().Z + HorseCart::HoverHeight)));

	ForecartPivot = CreateDefaultSubobject<USceneComponent>(TEXT("ForecartPivot"));
	ForecartPivot->SetupAttachment(Rig);
	ForecartPivot->SetRelativeLocation(FVector(HorseCartModel::HingeX, 0.0, 0.0));

	HorseRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HorseRoot"));
	HorseRoot->SetupAttachment(ForecartPivot);
	HorseRoot->SetRelativeLocation(FVector(HorseCartModel::HorseAt, 0.0, 0.0));

	Sound = CreateDefaultSubobject<UHorseSound>(TEXT("Sound"));
	Sound->SetupAttachment(CollisionBox);
	Sound->bAllowSpatialization = true;
	Sound->bOverrideAttenuation = true;
	Sound->AttenuationOverrides.bAttenuate = true;
	Sound->AttenuationOverrides.bSpatialize = true;
	Sound->AttenuationOverrides.FalloffDistance = HorseCart::SoundFalloff;

	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	CameraArm->SetupAttachment(CollisionBox);
	CameraArm->TargetArmLength = HorseCart::ArmLength;
	CameraArm->SetRelativeRotation(FRotator(HorseCart::ChasePitch, 0.f, 0.f));
	CameraArm->TargetOffset = FVector(0.f, 0.f, HorseCart::CameraHeight);
	CameraArm->bUsePawnControlRotation = false;
	CameraArm->bInheritRoll = false;
	CameraArm->bEnableCameraLag = true;
	CameraArm->CameraLagSpeed = 6.f;
	CameraArm->bEnableCameraRotationLag = true;
	CameraArm->CameraRotationLagSpeed = 4.f;

	ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
	ChaseCamera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
}

void AHorseCart::BeginPlay()
{
	Super::BeginPlay();
	using namespace HorseCartModel;

	// The cart in its owner's colours: Pista bácsi's green and red, or a carrier's.
	FLinearColor Boards;
	FLinearColor Wheels;
	LoadPaints(CartLoad, Boards, Wheels);
	FCart Cart;
	BuildCart(Cart, Boards, Wheels);
	const FVector Hinge(HingeX, 0.0, 0.0);
	for (int32 p = 0; p < NumCartParts; ++p)
	{
		const bool bFront = p == Forecart || OnForecart(p);
		CartParts.Add(MakeMesh(bFront ? ForecartPivot.Get() : Rig.Get(), Cart.Parts[p], bFront ? Cart.Parts[p].Pivot - Hinge : Cart.Parts[p].Pivot));
	}

	// Pista bácsi's horse is a bay with a white blaze; the carriers' are what they are.
	FHorse Horse;
	BuildHorse(Horse, HorseCoat, bBlaze);
	HorseParts.SetNum(NumHorseParts);
	for (int32 p = 0; p < NumHorseParts; ++p)
	{
		if (!IsLower(p))
		{
			HorseParts[p] = MakeMesh(HorseRoot.Get(), Horse.Parts[p], Horse.Parts[p].Pivot);
		}
	}
	for (int32 p = 0; p < NumHorseParts; ++p)
	{
		if (IsLower(p))
		{
			const int32 Upper = UpperOf(p);
			HorseParts[p] = MakeMesh(HorseParts[Upper].Get(), Horse.Parts[p], Horse.Parts[p].Pivot - Horse.Parts[Upper].Pivot);
		}
	}

	if (CartLoad == ELoad::None)
	{
		for (int32 c = 0; c < MaxCrates; ++c)
		{
			PersonKit::FPiece Crate;
			BuildCrate(Crate, FVector::ZeroVector);
			UProceduralMeshComponent* Mesh = MakeMesh(Rig.Get(), Crate, CrateSpot(c));
			Mesh->SetVisibility(false);
			CrateMeshes.Add(Mesh);
		}
	}
	else
	{
		for (int32 i = 0; i < NumLoadItems(CartLoad); ++i)
		{
			PersonKit::FPiece Item;
			BuildLoadItem(Item, CartLoad, i);
			LoadMeshes.Add(MakeMesh(Rig.Get(), Item, LoadSpot(CartLoad, i)));
		}
	}

	DriverMesh = MakeMesh(Rig.Get(), PersonKit::FPiece(), FVector::ZeroVector);
	DressDriver(bOwnDriverLook ? DriverLook : PersonModel::PlayerOutfit());
	DriverMesh->SetVisibility(false);

	Reins = MakeMesh(Rig.Get(), PersonKit::FPiece(), FVector::ZeroVector);
	Reins->SetCastShadow(false);

	if (!bSoundWhenNear)
	{
		Sound->Start();
	}
	NextSnort = GetWorld()->GetTimeSeconds() + FMath::FRandRange(HorseCart::MinSnortWait, HorseCart::MaxSnortWait);
	SnapToGround();
	Animate(0.f);
	UpdateReins();
}

UProceduralMeshComponent* AHorseCart::MakeMesh(USceneComponent* Parent, const PersonKit::FPiece& Piece, const FVector& At)
{
	UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this);
	Mesh->SetupAttachment(Parent);
	Mesh->SetRelativeLocation(At);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);
	// It moves: kept out of the ray-traced lighting and the distance fields (it still casts its shadow).
	Mesh->bVisibleInRayTracing = false;
	Mesh->bAffectDistanceFieldLighting = false;
	if (DrawDistance > 0.f)
	{
		Mesh->SetCachedMaxDrawDistance(DrawDistance);
	}
	Mesh->RegisterComponent();
	AddInstanceComponent(Mesh);
	Fill(Mesh, Piece.Sections);
	return Mesh;
}

void AHorseCart::DressDriver(const PersonKit::FOutfit& Look)
{
	TArray<PersonKit::FSection> Seated;
	CarModel::BuildSeatedPerson(Look, HorseCartModel::DriverHip(), HorseCart::DriverArmPitch, HorseCart::DriverLegPitch, Seated);
	Fill(DriverMesh, Seated);
}

void AHorseCart::Fill(UProceduralMeshComponent* Mesh, const TArray<PersonKit::FSection>& Sections)
{
	Mesh->ClearAllMeshSections();
	for (int32 i = 0; i < Sections.Num(); ++i)
	{
		const PersonKit::FSection& Section = Sections[i];
		Mesh->CreateMeshSection_LinearColor(i, Section.Vertices, Section.Triangles, Section.Normals,
			TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Paint->SetVectorParameterValue(TEXT("Color"), Section.Color);
			Mesh->SetMaterial(i, Paint);
		}
	}
}

void AHorseCart::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		Drive(PC, DeltaSeconds);
		if (GetController() != PC)
		{
			// He got down.
			return;
		}
	}
	else
	{
		// Nobody on the bench: the horse comes to a stand and waits.
		Pace = 0;
		SteerAngle = FMath::FInterpConstantTo(SteerAngle, 0.f, DeltaSeconds, HorseCart::SteerRate);
		Speed = FMath::FInterpConstantTo(Speed, 0.f, DeltaSeconds, HorseCart::SlowDown);
	}
	// Hard turns at a canter sway the load until a crate goes over the side.
	if (FMath::Abs(Speed) >= HorseCart::CanterFrom && FMath::Abs(SteerAngle) > HorseCart::SwayFrom)
	{
		Sway += (FMath::Abs(SteerAngle) - HorseCart::SwayFrom) * HorseCart::SwayPerDegree * DeltaSeconds;
		if (Sway >= 1.f)
		{
			Sway = 0.f;
			DropCrate();
		}
	}
	else
	{
		Sway = FMath::Max(0.f, Sway - HorseCart::SwaySettle * DeltaSeconds);
	}
	Move(DeltaSeconds);
	Animate(DeltaSeconds);
	UpdateReins();
}

void AHorseCart::Drive(APlayerController* PC, float DeltaSeconds)
{
	using namespace HorseCart;
	auto KeyAxis = [PC](const FKey& Positive, const FKey& Negative)
	{
		return (PC->IsInputKeyDown(Positive) ? 1.f : 0.f) - (PC->IsInputKeyDown(Negative) ? 1.f : 0.f);
	};
	auto Pressed = [PC](const FKey& Key, const FKey& Other, const FKey& Pad)
	{
		return PC->WasInputKeyJustPressed(Key) || PC->WasInputKeyJustPressed(Other) || PC->WasInputKeyJustPressed(Pad);
	};

	if (Pressed(EKeys::F, EKeys::F, EKeys::Gamepad_FaceButton_Right))
	{
		LetDriverOut(PC);
		if (GetController() != PC)
		{
			return;
		}
	}

	// One pace up or down for each press; held at a stand, S backs him.
	if (Pressed(EKeys::W, EKeys::Up, EKeys::Gamepad_RightTrigger))
	{
		Pace = FMath::Min(Pace + 1, TopPace);
	}
	if (Pressed(EKeys::S, EKeys::Down, EKeys::Gamepad_LeftTrigger))
	{
		Pace = FMath::Max(Pace - 1, 0);
	}
	const bool bWhoa = PC->IsInputKeyDown(EKeys::SpaceBar) || PC->IsInputKeyDown(EKeys::Gamepad_FaceButton_Bottom);
	if (bWhoa)
	{
		Pace = 0;
	}
	const bool bBacking = Pace == 0 && !bWhoa && Speed < 5.f
		&& (PC->IsInputKeyDown(EKeys::S) || PC->IsInputKeyDown(EKeys::Down) || PC->IsInputKeyDown(EKeys::Gamepad_LeftTrigger));
	const float Wanted = bBacking ? -BackSpeed : PaceSpeeds[Pace];
	const float Rate = bWhoa ? PullUp : FMath::Abs(Wanted) > FMath::Abs(Speed) ? SpeedUp : SlowDown;
	Speed = FMath::FInterpConstantTo(Speed, Wanted, DeltaSeconds, Rate);

	const float Steer = FMath::Clamp(KeyAxis(EKeys::D, EKeys::A) + KeyAxis(EKeys::Right, EKeys::Left)
		+ PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX), -1.f, 1.f);
	const int32 Gait = FMath::Abs(Speed) >= GallopFrom ? 4 : FMath::Abs(Speed) >= CanterFrom ? 3 : FMath::Abs(Speed) >= TrotFrom ? 2 : 1;
	SteerAngle = FMath::FInterpConstantTo(SteerAngle, Steer * MaxSteer[Gait], DeltaSeconds, SteerRate);

	UpdateFreeLook(PC, DeltaSeconds);

	static const TCHAR* PaceNames[] = {
		TEXT("áll / standing"),
		TEXT("lépés / walk"),
		TEXT("ügetés / trot"),
		TEXT("vágta / canter"),
		TEXT("teljes v\u00e1gta / gallop"),
	};
	APestiHUD::Show(this, 1, 0.f, FColor::White,
		FString::Printf(TEXT("Lovaskocsi / horse cart: %s   (W faster, S slower or back up, A/D steer, Space stop, F get down)"),
			bBacking ? TEXT("hátra / backing") : PaceNames[Pace]));
}

void AHorseCart::Move(float DeltaSeconds)
{
	if (FMath::IsNearlyZero(Speed))
	{
		return;
	}
	// The horse leads and the cart follows: it turns about the rear axle,
	// tighter the more the shafts are over.
	const double Turn = FMath::RadiansToDegrees(Speed * DeltaSeconds * FMath::Tan(FMath::DegreesToRadians(SteerAngle)) / HorseCart::Wheelbase);
	const FVector Rear = GetActorTransform().TransformPosition(FVector(HorseCartModel::BackWheelX - HorseCartModel::RigMiddleX, 0.0, 0.0));
	const FRotator Facing = GetActorRotation() + FRotator(0.0, Turn, 0.0);
	const FVector Swung = Rear + FRotator(0.0, Turn, 0.0).RotateVector(GetActorLocation() - Rear);
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
		// One knock spills one crate, however long it scrapes along.
		const double Now = GetWorld()->GetTimeSeconds();
		if (FMath::Abs(Speed) > HorseCart::SpillSpeed && Now - LastSpill > HorseCart::SpillGap)
		{
			LastSpill = Now;
			DropCrate();
		}
		// Up against something: the horse stops rather than push into it, or scrapes along a glancing one.
		const FVector WallNormal = FVector(Hit.Normal.X, Hit.Normal.Y, 0.f).GetSafeNormal();
		const float Impact = static_cast<float>(FMath::Abs(FVector::DotProduct(GetActorForwardVector(), WallNormal)));
		if (Impact > 0.6f)
		{
			Pace = 0;
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

void AHorseCart::Animate(float DeltaSeconds)
{
	using namespace HorseCartModel;
	const float AbsSpeed = FMath::Abs(Speed);
	const EGait Gait = AbsSpeed < HorseCart::WalkFrom ? EGait::Stand
		: AbsSpeed < HorseCart::TrotFrom ? EGait::Walk
		: AbsSpeed < HorseCart::CanterFrom ? EGait::Trot : EGait::Canter;
	const float StrideLength = AbsSpeed >= HorseCart::GallopFrom ? HorseCart::GallopStride
		: Gait == EGait::Canter ? HorseCart::CanterStride : Gait == EGait::Trot ? HorseCart::TrotStride : HorseCart::WalkStride;
	Stride += Speed / StrideLength * DeltaSeconds;
	Stride -= FMath::FloorToFloat(Stride);
	Idle += DeltaSeconds;

	const FHorsePose P = Pose(Gait, Stride);
	HorseRoot->SetRelativeLocationAndRotation(FVector(HorseAt, 0.0, P.Lift), FRotator(P.Pitch, 0.f, 0.f));
	// Standing, he lowers his head now and then and swishes his tail; moving, his head looks into the turn.
	const bool bStanding = Gait == EGait::Stand;
	const float Graze = bStanding ? 4.f + 4.f * FMath::Sin(Idle * 0.4f) : 0.f;
	HorseParts[Neck]->SetRelativeRotation(FRotator(P.NeckPitch - Graze, SteerAngle * 0.4f, 0.f));
	const float Swish = (bStanding ? 9.f : 4.f) * FMath::Sin(Idle * (bStanding ? 1.3f : 2.2f)) * FMath::Sin(Idle * 0.31f);
	HorseParts[Tail]->SetRelativeRotation(FRotator(P.TailPitch, 0.f, Swish));
	for (int32 l = 0; l < 4; ++l)
	{
		HorseParts[UpperFrontLeft + l]->SetRelativeRotation(FRotator(P.Upper[l], 0.f, 0.f));
		HorseParts[LowerFrontLeft + l]->SetRelativeRotation(FRotator(P.Lower[l], 0.f, 0.f));
		// Each hoof clops as it lands.
		if (!bStanding && P.Planted[l] && !WasPlanted[l])
		{
			Sound->Clop(Gait == EGait::Walk ? 0.5f : Gait == EGait::Trot ? 0.7f : 0.9f);
		}
		WasPlanted[l] = P.Planted[l];
	}
	Sound->SetRoll(FMath::Clamp(AbsSpeed / HorseCart::PaceSpeeds[3], 0.f, 1.f));
	const double Now = GetWorld()->GetTimeSeconds();
	if (bStanding && Now >= NextSnort)
	{
		Sound->Snort();
		NextSnort = Now + FMath::FRandRange(HorseCart::MinSnortWait, HorseCart::MaxSnortWait);
	}

	// The shafts and front wheels swing with the horse; the wheels roll.
	ForecartPivot->SetRelativeRotation(FRotator(0.f, SteerAngle, 0.f));
	// Kept within one turn of both wheels together, so it never grows too big to be exact.
	const float Lap = static_cast<float>(2.0 * PI * BackWheelRadius * FrontWheelRadius);
	WheelTurn = FMath::Fmod(WheelTurn + Speed * DeltaSeconds, Lap);
	const float BackSpin = -FMath::RadiansToDegrees(WheelTurn / static_cast<float>(BackWheelRadius));
	const float FrontSpin = -FMath::RadiansToDegrees(WheelTurn / static_cast<float>(FrontWheelRadius));
	CartParts[WheelBackLeft]->SetRelativeRotation(FRotator(BackSpin, 0.f, 0.f));
	CartParts[WheelBackRight]->SetRelativeRotation(FRotator(BackSpin, 0.f, 0.f));
	CartParts[WheelFrontLeft]->SetRelativeRotation(FRotator(FrontSpin, 0.f, 0.f));
	CartParts[WheelFrontRight]->SetRelativeRotation(FRotator(FrontSpin, 0.f, 0.f));
}

void AHorseCart::UpdateReins()
{
	// Held while someone sits on the bench.
	if (!DriverMesh || !DriverMesh->IsVisible() || !Reins)
	{
		if (Reins && Reins->GetNumSections() > 0)
		{
			Reins->ClearAllMeshSections();
		}
		return;
	}
	// From the bit rings, which move with the head, back to his hands.
	using namespace HorseCartModel;
	const FTransform ToRig = Rig->GetComponentTransform();
	const FTransform Head = HorseParts[Neck]->GetComponentTransform();
	PersonKit::FPiece Lines;
	for (const double Side : { -1.0, 1.0 })
	{
		const FVector Bitring = ToRig.InverseTransformPosition(Head.TransformPosition(Bit(Side) - NeckBase()));
		const FVector Hand = DriverHip() + FVector(HorseCart::HandsAt.X, HorseCart::HandsAt.Y * Side, HorseCart::HandsAt.Z);
		// A little slack: they sag in the middle.
		const FVector Middle = (Bitring + Hand) * 0.5 - FVector(0.0, 0.0, 8.0);
		PersonKit::Rod(Lines, Leather(), Bitring, Middle, HorseCart::ReinRadius, 4);
		PersonKit::Rod(Lines, Leather(), Middle, Hand, HorseCart::ReinRadius, 4);
	}
	const bool bFirst = Reins->GetNumSections() == 0;
	for (int32 i = 0; i < Lines.Sections.Num(); ++i)
	{
		const PersonKit::FSection& Section = Lines.Sections[i];
		Reins->CreateMeshSection_LinearColor(i, Section.Vertices, Section.Triangles, Section.Normals,
			TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
		if (bFirst && BaseMaterial)
		{
			UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Paint->SetVectorParameterValue(TEXT("Color"), Section.Color);
			Reins->SetMaterial(i, Paint);
		}
	}
}

void AHorseCart::UpdateFreeLook(const APlayerController* PC, float DeltaSeconds)
{
	using namespace HorseCart;
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
		// Not looking around while moving: the camera settles back behind the cart.
		TimeSinceLook += DeltaSeconds;
		if (TimeSinceLook > LookResetDelay && FMath::Abs(Speed) > WalkFrom)
		{
			LookYaw = FMath::FixedTurn(LookYaw, 0.f, LookResetRate * DeltaSeconds);
			LookPitch = FMath::FInterpConstantTo(LookPitch, 0.f, DeltaSeconds, LookResetRate);
		}
	}
	CameraArm->SetRelativeRotation(FRotator(ChasePitch + LookPitch, LookYaw, 0.f));
}

FVector AHorseCart::GetVelocity() const
{
	return GetActorForwardVector() * Speed;
}

void AHorseCart::TakeDriver(APawn* InDriver)
{
	Driver = InDriver;
	DriverInTime = GetWorld()->GetTimeSeconds();
	DriverMesh->SetVisibility(true);
	LookYaw = 0.f;
	LookPitch = 0.f;
	Pace = 0;
}

void AHorseCart::LetDriverOut(APlayerController* PC)
{
	APestiPerson* Person = Cast<APestiPerson>(Driver);
	// The F that got him up may still read as pressed this frame.
	const bool bJustUp = GetWorld()->GetTimeSeconds() - DriverInTime < HorseCart::MinTimeOnBench;
	if (!Person || bJustUp || FMath::Abs(Speed) > HorseCart::MaxExitSpeed)
	{
		return;
	}
	// Down on the left, by the bench, or on the right if that is blocked.
	const FTransform ToWorld = Rig->GetComponentTransform();
	for (const double Side : { -1.0, 1.0 })
	{
		const FVector At = ToWorld.TransformPosition(FVector(HorseCartModel::BenchX, Side * HorseCart::ExitSide, 0.0));
		if (Person->TryStepOutAt(At, GetActorRotation().Yaw))
		{
			Pace = 0;
			Speed = 0.f;
			Driver = nullptr;
			DriverMesh->SetVisibility(false);
			UpdateReins();
			PC->Possess(Person);
			PC->SetControlRotation(FRotator(-10.f, GetActorRotation().Yaw, 0.f));
			return;
		}
	}
}

FVector AHorseCart::GetBenchLocation() const
{
	return Rig->GetComponentTransform().TransformPosition(FVector(HorseCartModel::BenchX, 0.0, 0.0));
}

void AHorseCart::SetCrates(int32 Count)
{
	Crates = FMath::Clamp(Count, 0, HorseCartModel::MaxCrates);
	for (int32 c = 0; c < CrateMeshes.Num(); ++c)
	{
		CrateMeshes[c]->SetVisibility(c < Crates);
	}
}

FVector AHorseCart::GetCrateSpot(int32 Index) const
{
	return Rig->GetComponentTransform().TransformPosition(HorseCartModel::CrateSpot(FMath::Clamp(Index, 0, HorseCartModel::MaxCrates - 1)));
}

void AHorseCart::SnapToGround()
{
	const FVector Start = GetActorLocation();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HorseCartGround), false, this);
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, Start - FVector(0.f, 0.f, 2000.f), ECC_Visibility, Params))
	{
		FVector Location = Start;
		Location.Z = Hit.ImpactPoint.Z + HorseCart::HoverHeight + CollisionBox->GetScaledBoxExtent().Z;
		SetActorLocation(Location, false);
	}
}

void AHorseCart::DropCrate()
{
	if (Crates <= 0)
	{
		return;
	}
	SetCrates(Crates - 1);
	// It goes over the side it stood on, onto the road.
	const FVector Spot = HorseCartModel::CrateSpot(Crates);
	const double Side = Spot.Y < 0.0 ? -1.0 : 1.0;
	const FVector Out(Spot.X, Side * (HorseCartModel::BedHalfWidth + HorseCart::FallOut), 0.0);
	FVector At = Rig->GetComponentTransform().TransformPosition(Out);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HorseCartSpill), false, this);
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, At + FVector(0.0, 0.0, 200.0), At - FVector(0.0, 0.0, 500.0), ECC_Visibility, Params))
	{
		At = Hit.ImpactPoint;
	}
	Fallen.Add(At);
}

bool AHorseCart::TakeFallenCrate(FVector& OutAt)
{
	if (Fallen.Num() == 0)
	{
		return false;
	}
	OutAt = Fallen[0];
	Fallen.RemoveAt(0);
	return true;
}

bool AHorseCart::IsByBed(const FVector& At) const
{
	const FVector Local = Rig->GetComponentTransform().InverseTransformPosition(At);
	return Local.X > HorseCartModel::BedBack - HorseCart::LoadReach && Local.X < HorseCartModel::BedFront
		&& FMath::Abs(Local.Y) < HorseCartModel::BedHalfWidth + HorseCart::LoadReach;
}

FVector AHorseCart::GetTailLocation() const
{
	return Rig->GetComponentTransform().TransformPosition(FVector(HorseCartModel::BedBack - 70.0, 0.0, 0.0));
}
