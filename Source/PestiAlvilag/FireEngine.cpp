// Fill out your copyright notice in the Description page of Project Settings.

#include "FireEngine.h"

#include "Camera/PlayerCameraManager.h"
#include "CarBody.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Pedestrians.h"
#include "PestiPerson.h"
#include "ProceduralMeshComponent.h"
#include "SpotSound.h"
#include "TrafficManager.h"

namespace FireRun
{
	// Picking up speed and braking (cm/s each second), and its top speed (about 45 km/h).
	constexpr float Acceleration = 420.f;
	constexpr float Braking = 1200.f;
	constexpr float TopSpeed = 1250.f;
	// Through a right-angle corner, and slower for a sharper one (cm/s).
	constexpr float CornerSpeed = 520.f;
	constexpr float HairpinSpeed = 320.f;
	constexpr double LookAhead = 4000.0;
	// It keeps this far right of the lane's middle (cm): down the middle of the road.
	constexpr double Side = 170.0;
	// How far ahead it aims its nose (cm).
	constexpr double AimAhead = 400.0;
	// Stuck on something this long (s), it pushes on through at a crawl (cm/s).
	constexpr float MaxBlocked = 1.2f;
	constexpr float Crawl = 280.f;
	// The road block it carries ahead of itself, so the traffic stops for it (cm ahead of its middle).
	constexpr int32 BlockSlot = 3;
	const double BlockAhead[] = { 450.0, 1100.0, 1800.0 };
	// The bell is heard this far (cm).
	constexpr double BellReach = 9000.0;
}

AFireEngine::AFireEngine()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	const FVector Half = DailyLifeModel::FireHalf();
	CollisionBox->SetBoxExtent(FVector(Half.X, Half.Y, Half.Z * 0.6));
	CollisionBox->SetCollisionProfileName(TEXT("Pawn"));
	RootComponent = CollisionBox;

	BaseMaterial = CarBody::LoadPaintMaterial();
}

void AFireEngine::BeginPlay()
{
	Super::BeginPlay();
	using namespace DailyLifeModel;

	// The body, the wheels and the bell, each about its own pivot, under the box's middle.
	PersonKit::FPiece Shapes[NumFireParts];
	FireEngine(Shapes);
	const FVector Floor(0.0, 0.0, -FireHalf().Z * 0.6);
	for (int32 p = 0; p < NumFireParts; ++p)
	{
		UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this);
		Mesh->SetupAttachment(CollisionBox);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCanEverAffectNavigation(false);
		// It moves all the time: kept out of the ray-traced lighting and the distance fields.
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
				UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
				Paint->SetVectorParameterValue(TEXT("Color"), Section.Color);
				Mesh->SetMaterial(s, Paint);
			}
		}
		FVector Pivot = FVector::ZeroVector;
		if (p >= FireWheelFrontLeft && p <= FireWheelBackRight)
		{
			Pivot = FireWheelHub(p);
		}
		else if (p == FireBell)
		{
			Pivot = FireBellAt();
		}
		Mesh->SetRelativeLocation(Floor + Pivot);
		Mesh->RegisterComponent();
		AddInstanceComponent(Mesh);
		Parts.Add(Mesh);
	}

	// The crew: the driver and the officer in front, the firemen on the bench, all in brass helmets.
	FRandomStream Dice(FMath::Rand());
	const int32 Seats = NumFiremen + 2;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 i = 0; i < Seats; ++i)
	{
		APedestrian* Man = GetWorld()->SpawnActor<APedestrian>(APedestrian::StaticClass(), GetActorLocation(), GetActorRotation(), Params);
		if (!Man)
		{
			continue;
		}
		const PersonKit::FOutfit Look = FiremanLook(Dice);
		Man->SetOutfit(Look);
		PersonKit::FPiece Helmet;
		FireHelmet(Helmet);
		Man->AddProp(Helmet, PersonKit::Body, HeadMiddle(Look), FRotator::ZeroRotator);
		Crew.Add(Man);
	}

	Bell = NewObject<USpotSound>(this);
	Bell->SetupAttachment(CollisionBox);
	Bell->RegisterComponent();
	Bell->SetSpot(CitySynth::ESpot::FireBell, FMath::Rand());
	Bell->SetHeard(0.f, 1.f);
	Bell->Start();
}

void AFireEngine::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Traffic.IsValid())
	{
		Traffic->SetRoadBlock(FireRun::BlockSlot, TArray<FVector>());
	}
	for (APedestrian* Man : Crew)
	{
		if (IsValid(Man))
		{
			Man->Destroy();
		}
	}
	Crew.Reset();
	Super::EndPlay(EndPlayReason);
}

void AFireEngine::Setup(const TArray<FVector>& InWay, ATrafficManager* InTraffic)
{
	Way = InWay;
	Traffic = InTraffic;
	Cumulative.Reset();
	Total = 0.0;
	for (int32 i = 0; i < Way.Num(); ++i)
	{
		if (i > 0)
		{
			Total += FVector::Dist2D(Way[i - 1], Way[i]);
		}
		Cumulative.Add(Total);
	}
	Along = 0.0;
	Speed = FireRun::TopSpeed * 0.6f;
	bReady = Way.Num() >= 2;
	if (!bReady)
	{
		return;
	}
	FVector Dir = FVector::ForwardVector;
	FVector At = PointAt(0.0, Dir) + FVector(-Dir.Y, Dir.X, 0.0) * FireRun::Side;
	At.Z = GroundAt(At) + DailyLifeModel::FireHalf().Z * 0.6;
	SetActorLocationAndRotation(At, FRotator(0.0, Dir.Rotation().Yaw, 0.0));
	PlaceCrew(0.f);
}

FVector AFireEngine::PointAt(double S, FVector& OutDir) const
{
	OutDir = FVector::ForwardVector;
	if (Way.Num() == 0)
	{
		return FVector::ZeroVector;
	}
	if (Way.Num() == 1)
	{
		return Way[0];
	}
	const int32 Last = Way.Num() - 1;
	int32 Seg = 0;
	while (Seg < Last - 1 && Cumulative[Seg + 1] < S)
	{
		++Seg;
	}
	const double SegLength = FMath::Max(Cumulative[Seg + 1] - Cumulative[Seg], 1.0);
	const double F = FMath::Clamp((S - Cumulative[Seg]) / SegLength, 0.0, 1.0);
	const FVector A = Way[Seg];
	const FVector B = Way[Seg + 1];
	OutDir = (B - A).GetSafeNormal2D();
	if (OutDir.IsNearlyZero())
	{
		OutDir = FVector::ForwardVector;
	}
	return A + (B - A) * F;
}

double AFireEngine::GroundAt(const FVector& At) const
{
	FHitResult Hit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(FireEngineGround), false, this);
	if (GetWorld()->LineTraceSingleByObjectType(Hit, FVector(At.X, At.Y, At.Z + 400.0), FVector(At.X, At.Y, At.Z - 1500.0),
		FCollisionObjectQueryParams(ECC_WorldStatic), Query))
	{
		return Hit.ImpactPoint.Z;
	}
	return At.Z - DailyLifeModel::FireHalf().Z * 0.6;
}

float AFireEngine::CornerLimit() const
{
	float Limit = FireRun::TopSpeed;
	for (int32 i = 1; i + 1 < Way.Num(); ++i)
	{
		const double Ahead = Cumulative[i] - Along;
		if (Ahead < -50.0)
		{
			continue;
		}
		if (Ahead > FireRun::LookAhead)
		{
			break;
		}
		const FVector In = (Way[i] - Way[i - 1]).GetSafeNormal2D();
		const FVector Out = (Way[i + 1] - Way[i]).GetSafeNormal2D();
		const double Cos = FMath::Clamp(FVector::DotProduct(In, Out), -1.0, 1.0);
		const float Turn = static_cast<float>(FMath::RadiansToDegrees(FMath::Acos(Cos)));
		if (Turn < 12.f)
		{
			continue;
		}
		const float Bend = Turn <= 90.f
			? FMath::Lerp(FireRun::TopSpeed, FireRun::CornerSpeed, (Turn - 12.f) / 78.f)
			: FMath::Lerp(FireRun::CornerSpeed, FireRun::HairpinSpeed, FMath::Clamp((Turn - 90.f) / 90.f, 0.f, 1.f));
		const float Room = static_cast<float>(FMath::Max(Ahead, 0.0));
		Limit = FMath::Min(Limit, FMath::Sqrt(FMath::Square(Bend) + 2.f * FireRun::Braking * Room));
	}
	// Slowing to a stop at the end of its way.
	const float ToEnd = static_cast<float>(FMath::Max(Total - Along, 0.0));
	return FMath::Min(Limit, FMath::Sqrt(2.f * FireRun::Braking * ToEnd) + 50.f);
}

void AFireEngine::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Clock += DeltaSeconds;
	if (!bReady)
	{
		return;
	}

	// The bell: heard round the engine, louder near.
	if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const double Distance = FVector::Dist(Camera->GetCameraLocation(), GetActorLocation());
		const float Far = static_cast<float>(FMath::Clamp(Distance / FireRun::BellReach, 0.0, 1.0));
		Bell->SetHeard(Distance < FireRun::BellReach ? 1.f - Far : 0.f, Far);
	}

	if (!HasArrived())
	{
		const float Wanted = CornerLimit();
		const float Rate = Wanted < Speed ? FireRun::Braking : FireRun::Acceleration;
		Speed = FMath::FInterpConstantTo(Speed, Wanted, DeltaSeconds, Rate);

		const double Step = static_cast<double>(Speed) * DeltaSeconds;
		const double NextAlong = FMath::Min(Along + Step, Total);
		FVector Dir = FVector::ForwardVector;
		FVector Next = PointAt(NextAlong, Dir) + FVector(-Dir.Y, Dir.X, 0.0) * FireRun::Side;
		FVector AimDir = FVector::ForwardVector;
		const FVector Aim = PointAt(FMath::Min(NextAlong + FireRun::AimAhead, Total), AimDir) + FVector(-AimDir.Y, AimDir.X, 0.0) * FireRun::Side;
		const FVector Location = GetActorLocation();
		Next.Z = GroundAt(FVector(Next.X, Next.Y, Location.Z)) + DailyLifeModel::FireHalf().Z * 0.6;
		const FVector Facing = (Aim - Next).GetSafeNormal2D();
		const double AimYaw = Facing.IsNearlyZero() ? GetActorRotation().Yaw : Facing.Rotation().Yaw;
		const float NewYaw = FMath::FixedTurn(static_cast<float>(GetActorRotation().Yaw), static_cast<float>(AimYaw), 160.f * DeltaSeconds);
		SetActorRotation(FRotator(0.0, NewYaw, 0.0));

		FHitResult Hit;
		SetActorLocation(Next, true, &Hit);
		if (!Hit.bBlockingHit)
		{
			Along = NextAlong;
			BlockedTime = 0.f;
		}
		else
		{
			BlockedTime += DeltaSeconds;
			Speed *= 0.5f;
			APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
			const AActor* Blocker = Hit.GetActor();
			if (Blocker && Blocker == PlayerPawn)
			{
				// Running into him on foot knocks him down; his car it only shoves.
				if (APestiPerson* Person = Cast<APestiPerson>(PlayerPawn))
				{
					Person->HitByVehicle(FMath::Max(Speed, 400.f) * 2.f, GetActorForwardVector());
				}
			}
			else if (BlockedTime > FireRun::MaxBlocked)
			{
				// Wedged on a car or a cart: on through it.
				Speed = FMath::Max(Speed, FireRun::Crawl);
				SetActorLocation(Next, false);
				Along = NextAlong;
			}
		}
	}
	else
	{
		Speed = 0.f;
	}

	// The traffic ahead stops for it.
	if (Traffic.IsValid())
	{
		TArray<FVector> Block;
		const FVector Ahead = GetActorForwardVector().GetSafeNormal2D();
		for (const double Distance : FireRun::BlockAhead)
		{
			Block.Add(GetActorLocation() + Ahead * Distance);
		}
		Traffic->SetRoadBlock(FireRun::BlockSlot, HasArrived() ? TArray<FVector>() : Block);
	}

	// The wheels roll.
	using namespace DailyLifeModel;
	WheelTurn = FMath::Fmod(WheelTurn + Speed * DeltaSeconds, static_cast<float>(2.0 * PI * FireWheelRadius));
	const float Spin = -FMath::RadiansToDegrees(WheelTurn / static_cast<float>(FireWheelRadius));
	for (int32 w = FireWheelFrontLeft; w <= FireWheelBackRight; ++w)
	{
		if (Parts.IsValidIndex(w) && Parts[w])
		{
			Parts[w]->SetRelativeRotation(FRotator(Spin, 0.f, 0.f));
		}
	}
	PlaceCrew(DeltaSeconds);
}

void AFireEngine::PlaceCrew(float DeltaSeconds)
{
	using namespace DailyLifeModel;
	// The bell swings as it is rung.
	BellSwing = 22.f * FMath::Sin(Clock * 9.f) * FMath::Clamp(Speed / 400.f, 0.3f, 1.f);
	if (Parts.IsValidIndex(FireBell) && Parts[FireBell])
	{
		Parts[FireBell]->SetRelativeRotation(FRotator(0.f, 0.f, BellSwing));
	}
	const FTransform Frame = GetActorTransform();
	const FVector Floor(0.0, 0.0, -FireHalf().Z * 0.6);
	// They rock with the engine on the cobbles.
	const double Bump = 1.5 * FMath::Sin(Clock * 13.0) * FMath::Clamp(Speed / 800.f, 0.f, 1.f);
	for (int32 i = 0; i < Crew.Num(); ++i)
	{
		APedestrian* Man = Crew[i];
		if (!IsValid(Man))
		{
			continue;
		}
		FVector Seat;
		double FaceYaw = 0.0;
		FLimbs Pose;
		if (i < NumFiremen)
		{
			double Facing = 1.0;
			Seat = FiremanSeat(i, Facing);
			FaceYaw = Facing * 90.0;
			// One in three holds on to the rail over his head.
			Seated(Pose, i % 3 == 0 ? 1.f : 0.f);
		}
		else
		{
			Seat = i == NumFiremen ? FireDriverSeat() : FireOfficerSeat();
			Seated(Pose, 0.f);
			if (i == NumFiremen)
			{
				// The driver's hands on the wheel.
				Pose.Arms[0] = FRotator(60.f, 0.f, 10.f);
				Pose.Arms[1] = FRotator(60.f, 0.f, -10.f);
				Pose.Elbows[0] = 30.f;
				Pose.Elbows[1] = 30.f;
			}
		}
		const FVector At = Frame.TransformPosition(Floor + Seat + FVector(0.0, 0.0, Bump * ((i % 2) ? 1.0 : -0.7)));
		Man->Strike(At, Frame.Rotator().Yaw + FaceYaw, Pose);
	}
}
