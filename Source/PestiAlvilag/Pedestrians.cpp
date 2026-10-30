// Fill out your copyright notice in the Description page of Project Settings.

#include "Pedestrians.h"

#include "Camera/PlayerCameraManager.h"
#include "CarBody.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "OsmGraybox.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiCar.h"
#include "PestiHud.h"
#include "PolicePatrols.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogPedestrians, Log, All);

namespace Pedestrians
{
	constexpr float MinSpeed = 105.f;
	constexpr float MaxSpeed = 160.f;
	// Each keeps to the right of the walking line by this much.
	constexpr float MinLane = 40.f;
	constexpr float MaxLane = 100.f;
	constexpr float Middle = 90.f;

	constexpr double EndCell = 1000.0;
	constexpr double JoinDistance = 700.0;
	constexpr double RefindLines = 2000.0;
	constexpr float GroundEvery = 0.3f;
	constexpr int32 SpawnPerFrame = 4;
	constexpr int32 PickTries = 24;
	constexpr double FillNear = 500.0;
	// Later arrivals only appear where the camera is not looking (more than
	// about 80 degrees off its view) or behind a building, so nobody pops up
	// in view; and the ones left behind only go once they are out of sight.
	constexpr double OutOfViewDot = 0.17;
	constexpr float SeenWithin = 1.f;
	constexpr double GiveUpShare = 2.0;
	constexpr double EyeHeight = 150.0;
	// Where he is drawn catches up with his place on the line at up to this
	// many times his walking pace, and turns at this rate (degrees a second).
	constexpr double CatchUp = 1.8;
	constexpr double SnapDistance = 1500.0;
	constexpr float TurnRate = 360.f;

	constexpr float LegSwing = 25.f;
	constexpr float ArmSwing = 18.f;
	constexpr float StrideLength = 110.f;
	// The pace at which the knees bend as much as in a brisk walk (cm/s).
	constexpr float BriskWalk = 140.f;

	// Someone on foot who walks into him (closer than this, faster than
	// that) makes him stumble back; someone running at him makes him step
	// out of the way.
	constexpr double BumpDistance = 55.0;
	constexpr double BumpSpeed = 80.0;
	constexpr double RunningSpeed = 500.0;
	constexpr double RunPastDistance = 220.0;
	// A car coming at him: he sees it this far ahead (its speed times this,
	// plus a bit) and this far to either side of its path, and leaps clear;
	// unless he freezes in fright.
	constexpr double CarNoticeSpeed = 300.0;
	constexpr double CarWarnSeconds = 1.0;
	constexpr double CarWarnExtra = 250.0;
	constexpr double CarPathHalfWidth = 260.0;
	constexpr float FreezeChance = 0.25f;
	constexpr float MinNotice = 0.15f;
	constexpr float MaxNotice = 0.6f;
	constexpr double LeapSpeed = 420.0;
	// A car up on the pavement (this close to his walking line, this near
	// him) sends him running off along it, this many times faster.
	constexpr double OnPavement = 230.0;
	constexpr double PavementNear = 900.0;
	constexpr float FleeShare = 2.6f;
	constexpr float FleeSeconds = 3.f;
	// The car's front and sides, from its middle (cm), and the speed below
	// which it only nudges him. Faster than KillSpeed it kills him.
	constexpr double CarHalfLength = 215.0;
	constexpr double CarHalfWidth = 95.0;
	constexpr double HitSpeed = 150.0;
	constexpr double KillSpeed = 1100.0;
	constexpr float CarSlowOnHit = 0.85f;
	constexpr double PoliceSee = 5000.0;
	// Witnesses this near run off too.
	constexpr double WitnessNear = 1500.0;
	// Lying down: his middle this far above the ground.
	constexpr double LyingHeight = 18.0;
	constexpr float FallSeconds = 0.45f;
	constexpr float LieSeconds = 2.5f;
	constexpr float GetUpSeconds = 1.2f;
	constexpr double BloodRadius = 70.0;
	constexpr float BloodSpreads = 6.f;
	// The dead are cleared away once out of sight and at least this far off.
	constexpr double ClearBeyond = 2500.0;
	constexpr double ShoutEvery = 4.0;
	const FLinearColor BloodColor(0.16f, 0.004f, 0.004f);

	// What people say (Hungarian, then English).
	struct FLine2 { const TCHAR* Hu; const TCHAR* En; };
	const FLine2 BumpLines[] = {
		{ TEXT("Vigy\u00e1zzon m\u00e1r, k\u00e9rem! H\u00e1t nem l\u00e1t?"), TEXT("Watch where you are going! Are you blind?") },
		{ TEXT("Nofene! L\u00f6kd\u00f6s\u0151dik?"), TEXT("Well now! Pushing, are we?") },
		{ TEXT("Micsoda modortalans\u00e1g!"), TEXT("What bad manners!") },
	};
	const FLine2 LeapLines[] = {
		{ TEXT("Seg\u00edts\u00e9g! Meg\u0151r\u00fclt ez?!"), TEXT("Help! Has he gone mad?!") },
		{ TEXT("J\u00e9zusm\u00e1ria! Majdnem elg\u00e1zolt!"), TEXT("Good Lord! He nearly ran me over!") },
		{ TEXT("H\u00e9! Ez j\u00e1rda, nem orsz\u00e1g\u00fat!"), TEXT("Hey! This is a pavement, not a highway!") },
	};
	const FLine2 GetUpLines[] = {
		{ TEXT("Jaj, a l\u00e1bam! Maga vad\u00e1llat!"), TEXT("Ow, my leg! You brute!") },
		{ TEXT("Rend\u0151r! Elg\u00e1zoltak!"), TEXT("Police! I've been run over!") },
	};
	const FLine2 WitnessLine = { TEXT("\u00daristen! Ment\u0151t! H\u00edvjanak ment\u0151t!"), TEXT("Good God! Call an ambulance!") };

	FIntPoint CellOf(const FVector2D& At)
	{
		return FIntPoint(FMath::FloorToInt(At.X / EndCell), FMath::FloorToInt(At.Y / EndCell));
	}
}

APedestrian::APedestrian()
{
	// APedestrians moves him; he is no obstacle for cars or the player.
	PrimaryActorTick.bCanEverTick = false;
	Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APedestrian::WalkTo(const FVector& Location, double Yaw, float Speed, float DeltaSeconds, float ArmsUp)
{
	SetActorLocationAndRotation(Location, FRotator(0.0, Yaw, 0.0));

	Stride += DeltaSeconds * Speed / Pedestrians::StrideLength * PI;
	const float Swing = FMath::Sin(Stride);
	PersonModel::StrideLegs(Pieces, Stride, Pedestrians::LegSwing, FMath::Clamp(Speed / Pedestrians::BriskWalk, 0.f, 1.6f));
	float Arms[] = { -Swing * Pedestrians::ArmSwing, Swing * Pedestrians::ArmSwing };
	if (ArmsUp > 0.f)
	{
		// Arms thrown up, flailing as he runs.
		const float Flail = 15.f * FMath::Sin(Stride * 2.f);
		Arms[0] = FMath::Lerp(Arms[0], 140.f + Flail, ArmsUp);
		Arms[1] = FMath::Lerp(Arms[1], 140.f - Flail, ArmsUp);
	}
	for (int32 i = 0; i < 2; ++i)
	{
		if (USceneComponent* Part = Limb(PersonKit::ArmLeft + i))
		{
			Part->SetRelativeRotation(FRotator(Arms[i], 0.f, 0.f));
		}
	}
}

void APedestrian::Hold(const FVector& Location, const FRotator& Rotation, float Legs, float Knees, float Arms)
{
	SetActorLocationAndRotation(Location, Rotation);
	// Not quite the same on both sides, so he does not look like a doll.
	PersonModel::PoseLegs(Pieces, Legs, Legs * 0.6f, Knees, Knees * 0.8f);
	for (int32 i = 0; i < 2; ++i)
	{
		if (USceneComponent* Part = Limb(PersonKit::ArmLeft + i))
		{
			Part->SetRelativeRotation(FRotator(Arms * (i == 0 ? 1.f : 0.85f), 0.f, 0.f));
		}
	}
}

APedestrians::APedestrians()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	BaseMaterial = CarBody::LoadPaintMaterial();
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	CylinderMesh = Cylinder.Object;
}

void APedestrians::BeginPlay()
{
	Super::BeginPlay();

	Random.Initialize(1932);
	bReady = LoadLines();
	if (BaseMaterial)
	{
		BloodPaint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		BloodPaint->SetVectorParameterValue(TEXT("Color"), Pedestrians::BloodColor);
	}
}

bool APedestrians::LoadLines()
{
	const FString Path = FPaths::Combine(FPaths::ProjectDir(), DataFile);
	FString Text;
	TSharedPtr<FJsonObject> Root;
	if (!FFileHelper::LoadFileToString(Text, *Path)
		|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
	{
		UE_LOG(LogPedestrians, Error, TEXT("Could not read %s"), *Path);
		return false;
	}
	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!Root->TryGetArrayField(TEXT("sidewalks"), Rows))
	{
		UE_LOG(LogPedestrians, Warning, TEXT("No sidewalks in %s"), *Path);
		return false;
	}

	for (const TSharedPtr<FJsonValue>& Row : *Rows)
	{
		const TArray<TSharedPtr<FJsonValue>>& Numbers = Row->AsArray();
		FLine Line;
		for (int32 i = 0; i + 1 < Numbers.Num(); i += 2)
		{
			const FVector2D Point(Numbers[i]->AsNumber(), Numbers[i + 1]->AsNumber());
			Line.Length += Line.Points.Num() > 0 ? FVector2D::Distance(Line.Points.Last(), Point) : 0.0;
			Line.Points.Add(Point);
			Line.Along.Add(Line.Length);
		}
		if (Line.Points.Num() < 2 || Line.Length < 1.0)
		{
			continue;
		}
		const int32 Index = Lines.Add(MoveTemp(Line));
		Ends.Add(Pedestrians::CellOf(Lines[Index].Points[0]), FIntPoint(Index, 0));
		Ends.Add(Pedestrians::CellOf(Lines[Index].Points.Last()), FIntPoint(Index, 1));
	}
	UE_LOG(LogPedestrians, Log, TEXT("Pedestrians: %d walking lines"), Lines.Num());
	return Lines.Num() > 0;
}

bool APedestrians::CameraView(FVector& OutLocation, FVector2D& OutLooking) const
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager)
	{
		return false;
	}
	OutLocation = PC->PlayerCameraManager->GetCameraLocation();
	OutLooking = FVector2D(PC->PlayerCameraManager->GetCameraRotation().Vector()).GetSafeNormal();
	return true;
}

FVector2D APedestrians::PointOn(const FLine& Line, double Along, FVector2D& OutTangent) const
{
	int32 i = 0;
	while (i + 2 < Line.Points.Num() && Line.Along[i + 1] < Along)
	{
		++i;
	}
	const FVector2D A = Line.Points[i];
	const FVector2D B = Line.Points[i + 1];
	const double Span = FMath::Max(Line.Along[i + 1] - Line.Along[i], 1.0);
	OutTangent = (B - A).GetSafeNormal();
	return A + (B - A) * FMath::Clamp((Along - Line.Along[i]) / Span, 0.0, 1.0);
}

bool APedestrians::PickSpot(const FVector& Camera, const FVector2D& Looking, bool bInView, double MinDistance,
	int32& OutLine, double& OutAlong, double& OutGroundZ)
{
	if (FVector::Dist2D(Camera, NearLinesFrom) > Pedestrians::RefindLines)
	{
		NearLinesFrom = Camera;
		NearLines.Reset();
		for (int32 i = 0; i < Lines.Num(); ++i)
		{
			for (const FVector2D& Point : Lines[i].Points)
			{
				if (FVector2D::Distance(Point, FVector2D(Camera)) < Range + Lines[i].Length)
				{
					NearLines.Add(i);
					break;
				}
			}
		}
	}
	if (NearLines.IsEmpty())
	{
		return false;
	}
	for (int32 Try = 0; Try < Pedestrians::PickTries; ++Try)
	{
		const int32 Index = NearLines[Random.RandRange(0, NearLines.Num() - 1)];
		const double Along = Random.FRandRange(0.f, 1.f) * Lines[Index].Length;
		FVector2D Tangent;
		const FVector2D At = PointOn(Lines[Index], Along, Tangent);
		const FVector2D Away = At - FVector2D(Camera);
		const double Distance = Away.Size();
		if (Distance < MinDistance || Distance >= Range)
		{
			continue;
		}
		const bool bBehind = FVector2D::DotProduct(Away / FMath::Max(Distance, 1.0), Looking) < Pedestrians::OutOfViewDot;
		// Only where the ground is known, so he is on it from the first frame.
		if (GroundUnder(At, Camera.Z, OutGroundZ) && (bInView || bBehind || BehindBuilding(Camera, At, OutGroundZ)))
		{
			OutLine = Index;
			OutAlong = Along;
			return true;
		}
	}
	return false;
}

void APedestrians::TurnAtEnd(FWalker& Walker)
{
	const FLine& Line = Lines[Walker.Line];
	const FVector2D End = Walker.Dir > 0 ? Line.Points.Last() : Line.Points[0];
	const FIntPoint Cell = Pedestrians::CellOf(End);

	TArray<FIntPoint> Joins;
	for (int32 dx = -1; dx <= 1; ++dx)
	{
		for (int32 dy = -1; dy <= 1; ++dy)
		{
			TArray<FIntPoint> Found;
			Ends.MultiFind(Cell + FIntPoint(dx, dy), Found);
			for (const FIntPoint& Other : Found)
			{
				const FLine& Next = Lines[Other.X];
				const FVector2D Start = Other.Y == 0 ? Next.Points[0] : Next.Points.Last();
				if (Other.X != Walker.Line && FVector2D::Distance(Start, End) < Pedestrians::JoinDistance)
				{
					Joins.Add(Other);
				}
			}
		}
	}

	if (Joins.IsEmpty())
	{
		// A dead end: turn round.
		Walker.Dir = -Walker.Dir;
		return;
	}
	const FIntPoint Join = Joins[Random.RandRange(0, Joins.Num() - 1)];
	Walker.Line = Join.X;
	Walker.Dir = Join.Y == 0 ? 1 : -1;
	Walker.Along = Join.Y == 0 ? 0.0 : Lines[Join.X].Length;
}

bool APedestrians::BehindBuilding(const FVector& Camera, const FVector2D& At, double GroundZ) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PedestrianSight), false, this);
	const FVector Eye(At, GroundZ + Pedestrians::EyeHeight);
	return GetWorld()->LineTraceSingleByChannel(Hit, Camera, Eye, ECC_Visibility, Params)
		&& Cast<AOsmGraybox>(Hit.GetActor()) != nullptr
		&& FVector::Dist(Hit.ImpactPoint, Eye) > 100.0;
}

bool APedestrians::GroundUnder(const FVector2D& At, double Near, double& OutZ) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PedestrianGround), false, this);
	const double Top = Near + 300.0;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, FVector(At, Top), FVector(At, Near - 2000.0), ECC_Visibility, Params))
	{
		return false;
	}
	// Only the open ground: not a lamp, a bench or a car in the way.
	if (!Cast<AOsmGraybox>(Hit.GetActor()))
	{
		return false;
	}
	OutZ = Hit.ImpactPoint.Z;
	return true;
}

void APedestrians::Shout(const TCHAR* Hu, const TCHAR* En)
{
	const double Now = GetWorld()->GetTimeSeconds();
	APestiHUD* Hud = APestiHUD::Get(this);
	if (!Hud || Hud->IsSaying() || Now - LastShout < Pedestrians::ShoutEvery)
	{
		return;
	}
	LastShout = Now;
	Hud->Say(TEXT("J\u00e1r\u00f3kel\u0151"), Hu, En, 3.f);
}

void APedestrians::StartReact(FWalker& Walker, EReact React, float Seconds, const FVector2D& Shove)
{
	Walker.React = React;
	Walker.ReactLeft = Seconds;
	Walker.ReactTime = 0.f;
	Walker.Shove = Shove;
	Walker.NoticeAt = -1.f;
	Walker.bFrozen = false;
}

void APedestrians::ClearReact(FWalker& Walker)
{
	StartReact(Walker, EReact::None, 0.f, FVector2D::ZeroVector);
	if (Walker.Blood)
	{
		Walker.Blood->SetVisibility(false);
	}
}

void APedestrians::ReactToPlayer(FWalker& Walker, double Now)
{
	if (Walker.React == EReact::Down || Walker.React == EReact::GetUp || Walker.React == EReact::Dead)
	{
		return;
	}
	const FVector2D Offset = Walker.Shown - FVector2D(PlayerAt);
	if (Offset.SizeSquared() > FMath::Square(3000.0) || FMath::Abs(Walker.GroundZ - PlayerAt.Z) > 400.0)
	{
		Walker.NoticeAt = -1.f;
		return;
	}
	const double Speed = PlayerVelocity.Size2D();
	const FVector2D Going = FVector2D(PlayerVelocity).GetSafeNormal();

	if (bPlayerDriving)
	{
		const FVector2D Right(-PlayerForward.Y, PlayerForward.X);
		const double Ahead = FVector2D::DotProduct(Offset, PlayerForward);
		const double Aside = FVector2D::DotProduct(Offset, Right);
		if (Speed > Pedestrians::HitSpeed && FMath::Abs(Ahead) < Pedestrians::CarHalfLength && FMath::Abs(Aside) < Pedestrians::CarHalfWidth)
		{
			RunOver(Walker, Speed);
			return;
		}
		if (Walker.React == EReact::Leap || Walker.React == EReact::Flee)
		{
			return;
		}

		// The car coming at him (forwards, or backing up).
		const double Front = FVector2D::DotProduct(FVector2D(PlayerVelocity), PlayerForward) >= 0.0 ? Ahead : -Ahead;
		if (Speed > Pedestrians::CarNoticeSpeed && Front > 0.0
			&& Front < Speed * Pedestrians::CarWarnSeconds + Pedestrians::CarWarnExtra + Pedestrians::CarHalfLength
			&& FMath::Abs(Aside) < Pedestrians::CarPathHalfWidth)
		{
			if (Walker.NoticeAt < 0.f)
			{
				// He takes a moment to see it, and may stand frozen in fright.
				Walker.NoticeAt = static_cast<float>(Now) + Random.FRandRange(Pedestrians::MinNotice, Pedestrians::MaxNotice);
				Walker.bFrozen = Random.FRand() < Pedestrians::FreezeChance;
			}
			if (Now >= Walker.NoticeAt && !Walker.bFrozen)
			{
				const double Side = FMath::Abs(Aside) > 10.0 ? FMath::Sign(Aside) : (Random.FRand() < 0.5f ? -1.0 : 1.0);
				StartReact(Walker, EReact::Leap, 0.7f, Right * (Side * Pedestrians::LeapSpeed));
				const Pedestrians::FLine2& Line = Pedestrians::LeapLines[Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Pedestrians::LeapLines)) - 1)];
				Shout(Line.Hu, Line.En);
			}
			return;
		}
		Walker.NoticeAt = -1.f;
		Walker.bFrozen = false;

		// The car up on the pavement near him: he runs off along it.
		if (Walker.React == EReact::None && Speed > Pedestrians::CarNoticeSpeed * 0.5 && Offset.Size() < Pedestrians::PavementNear)
		{
			FVector2D Tangent;
			const FVector2D OnLine = PointOn(Lines[Walker.Line], Walker.Along, Tangent);
			const FVector2D FromLine = FVector2D(PlayerAt) - OnLine;
			if (FMath::Abs(FromLine.X * Tangent.Y - FromLine.Y * Tangent.X) < Pedestrians::OnPavement)
			{
				Walker.Dir = FVector2D::DotProduct(Tangent, Offset) >= 0.0 ? 1 : -1;
				StartReact(Walker, EReact::Flee, Pedestrians::FleeSeconds, FVector2D::ZeroVector);
				const Pedestrians::FLine2& Line = Pedestrians::LeapLines[Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Pedestrians::LeapLines)) - 1)];
				Shout(Line.Hu, Line.En);
			}
		}
		return;
	}

	// On foot: walked into, he stumbles away and tells you off.
	const double Distance = Offset.Size();
	if (Speed > Pedestrians::BumpSpeed && Distance < Pedestrians::BumpDistance
		&& (Walker.React == EReact::None || Walker.React == EReact::Aside))
	{
		const FVector2D Away = (Going * 0.7 + Offset.GetSafeNormal() * 0.3).GetSafeNormal();
		StartReact(Walker, EReact::Stumble, 0.9f, Away * 220.0);
		const Pedestrians::FLine2& Line = Pedestrians::BumpLines[Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Pedestrians::BumpLines)) - 1)];
		Shout(Line.Hu, Line.En);
		return;
	}
	// Running at him: he steps out of the way.
	if (Walker.React == EReact::None && Speed > Pedestrians::RunningSpeed && Distance < Pedestrians::RunPastDistance
		&& FVector2D::DotProduct(Offset, Going) > 0.0)
	{
		const FVector2D Right(-Going.Y, Going.X);
		const double Side = FVector2D::DotProduct(Offset, Right) >= 0.0 ? 1.0 : -1.0;
		StartReact(Walker, EReact::Aside, 0.8f, Right * (Side * 170.0));
	}
}

void APedestrians::RunOver(FWalker& Walker, double Speed)
{
	const bool bKilled = Speed > Pedestrians::KillSpeed;
	const FVector2D Going = FVector2D(PlayerVelocity).GetSafeNormal();
	const FVector2D Right(-Going.Y, Going.X);
	const double Side = FVector2D::DotProduct(Walker.Shown - FVector2D(PlayerAt), Right) >= 0.0 ? 1.0 : -1.0;
	// Thrown on ahead of the car and a little out to the side he was on.
	const FVector2D Thrown = (Going + Right * (Side * 0.5)).GetSafeNormal() * FMath::Min(Speed * 0.5, 700.0);
	StartReact(Walker, bKilled ? EReact::Dead : EReact::Down, Pedestrians::FallSeconds + Pedestrians::LieSeconds, Thrown);
	// He falls on his back, away from the car: so he faces it.
	Walker.ShownYaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(-Going.Y, -Going.X)));

	if (APestiCar* Car = Cast<APestiCar>(LastPlayer.Get()))
	{
		Car->ForwardSpeed *= Pedestrians::CarSlowOnHit;
	}

	// A police car or a constable close by sees it.
	if (APolicePatrols::Witnessed(this, PlayerAt, Pedestrians::PoliceSee))
	{
		if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
		{
			Game->ReportCrime(bKilled ? 2 : 1);
		}
	}

	if (bKilled)
	{
		// Those who saw it cry out and run.
		Shout(Pedestrians::WitnessLine.Hu, Pedestrians::WitnessLine.En);
		for (FWalker& Other : Walkers)
		{
			if (&Other != &Walker && IsValid(Other.Person) && (Other.React == EReact::None || Other.React == EReact::Aside)
				&& FVector2D::Distance(Other.Shown, Walker.Shown) < Pedestrians::WitnessNear)
			{
				FVector2D Tangent;
				PointOn(Lines[Other.Line], Other.Along, Tangent);
				Other.Dir = FVector2D::DotProduct(Tangent, Other.Shown - Walker.Shown) >= 0.0 ? 1 : -1;
				StartReact(Other, EReact::Flee, Pedestrians::FleeSeconds, FVector2D::ZeroVector);
			}
		}
	}
}

bool APedestrians::PoseReacting(FWalker& Walker, float DeltaSeconds)
{
	if (Walker.React == EReact::None || Walker.React == EReact::Flee)
	{
		return false;
	}
	APedestrian* Person = Walker.Person;
	const double Size = Person->GetActorScale3D().Z;
	const double Standing = Walker.GroundZ + Pedestrians::Middle * Size;
	const double Lying = Walker.GroundZ + Pedestrians::LyingHeight * Size;
	Walker.ReactTime += DeltaSeconds;
	Walker.ReactLeft -= DeltaSeconds;
	Walker.Shown += Walker.Shove * DeltaSeconds;
	const FVector Location(Walker.Shown, Standing);

	switch (Walker.React)
	{
	case EReact::Aside:
	case EReact::Leap:
	{
		const bool bLeap = Walker.React == EReact::Leap;
		if (bLeap)
		{
			// Turns to jump the way he goes.
			const float Toward = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Walker.Shove.Y, Walker.Shove.X)));
			Walker.ShownYaw = FMath::FixedTurn(Walker.ShownYaw, Toward, 900.f * DeltaSeconds);
		}
		Person->WalkTo(Location, Walker.ShownYaw, bLeap ? 400.f : 180.f, DeltaSeconds, bLeap ? 1.f : 0.f);
		if (Walker.ReactLeft <= 0.f)
		{
			StartReact(Walker, EReact::None, 0.f, FVector2D::ZeroVector);
		}
		return true;
	}
	case EReact::Stumble:
	{
		Walker.Shove *= FMath::Max(0.0, 1.0 - 3.0 * DeltaSeconds);
		// Pushed from behind he pitches forward, from in front he reels back.
		const FVector2D Facing = FVector2D(FRotator(0.0, Walker.ShownYaw, 0.0).Vector());
		const float Back = FVector2D::DotProduct(Facing, Walker.Shove) > 0.0 ? -1.f : 1.f;
		const float Wave = FMath::Sin(PI * FMath::Clamp(Walker.ReactTime / 0.9f, 0.f, 1.f));
		Person->Hold(Location, FRotator(Back * 18.f * Wave, Walker.ShownYaw, 0.f), -Back * 18.f * Wave, -25.f * Wave, 70.f * Wave);
		if (Walker.ReactLeft <= 0.f)
		{
			StartReact(Walker, EReact::None, 0.f, FVector2D::ZeroVector);
		}
		return true;
	}
	case EReact::Down:
	case EReact::Dead:
	{
		const bool bDead = Walker.React == EReact::Dead;
		if (Walker.ReactTime < Pedestrians::FallSeconds)
		{
			const float Fall = Walker.ReactTime / Pedestrians::FallSeconds;
			Walker.Shove *= FMath::Max(0.0, 1.0 - 2.0 * DeltaSeconds);
			Person->Hold(FVector(Walker.Shown, FMath::Lerp(Standing, Lying, static_cast<double>(Fall))),
				FRotator(88.f * Fall * Fall, Walker.ShownYaw, 0.f), 40.f * Fall, -30.f * Fall, 150.f * Fall);
			return true;
		}
		Walker.Shove = FVector2D::ZeroVector;
		const FVector Down(Walker.Shown, Lying);
		if (bDead)
		{
			// Still, twisted a little, the blood spreading under him.
			Person->Hold(Down, FRotator(88.f, Walker.ShownYaw, 8.f), 25.f, -55.f, 120.f);
			if (!Walker.Blood && CylinderMesh)
			{
				Walker.Blood = NewObject<UStaticMeshComponent>(this);
				Walker.Blood->SetupAttachment(RootComponent);
				Walker.Blood->SetStaticMesh(CylinderMesh);
				Walker.Blood->SetMaterial(0, BloodPaint);
				Walker.Blood->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Walker.Blood->SetCastShadow(false);
				Walker.Blood->SetCanEverAffectNavigation(false);
				Walker.Blood->SetCachedMaxDrawDistance(Range);
				Walker.Blood->RegisterComponent();
				AddInstanceComponent(Walker.Blood);
			}
			if (Walker.Blood)
			{
				const float Spread = FMath::Sqrt(FMath::Clamp((Walker.ReactTime - Pedestrians::FallSeconds) / Pedestrians::BloodSpreads, 0.05f, 1.f));
				const double Radius = Pedestrians::BloodRadius * Spread;
				// Under his chest and head, which lie behind his middle.
				const FVector2D Behind = -FVector2D(FRotator(0.0, Walker.ShownYaw, 0.0).Vector()) * (35.0 * Size);
				Walker.Blood->SetVisibility(true);
				Walker.Blood->SetWorldLocationAndRotation(FVector(Walker.Shown + Behind, Walker.GroundZ + 0.6), FRotator(0.0, Walker.ShownYaw, 0.0));
				Walker.Blood->SetWorldScale3D(FVector(Radius / 50.0 * 1.25, Radius / 50.0, 0.004));
			}
			return true;
		}
		Person->Hold(Down, FRotator(88.f, Walker.ShownYaw, 0.f), 20.f, -30.f, 150.f);
		if (Walker.ReactLeft <= 0.f)
		{
			StartReact(Walker, EReact::GetUp, Pedestrians::GetUpSeconds, FVector2D::ZeroVector);
			const Pedestrians::FLine2& Line = Pedestrians::GetUpLines[Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Pedestrians::GetUpLines)) - 1)];
			Shout(Line.Hu, Line.En);
		}
		return true;
	}
	case EReact::GetUp:
	{
		// Sits up, crouches, and stands.
		const float Up = FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Walker.ReactTime / Pedestrians::GetUpSeconds, 0.f, 1.f));
		const float Bent = 1.f - Up;
		Person->Hold(FVector(Walker.Shown, FMath::Lerp(Lying, Standing, static_cast<double>(Up))),
			FRotator(88.f * Bent * Bent, Walker.ShownYaw, 0.f), 75.f * Bent, -110.f * Bent, 45.f * Bent);
		if (Walker.ReactLeft <= 0.f)
		{
			StartReact(Walker, EReact::None, 0.f, FVector2D::ZeroVector);
		}
		return true;
	}
	default:
		return false;
	}
}

void APedestrians::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FVector Camera;
	FVector2D Looking;
	if (!bReady || !CameraView(Camera, Looking))
	{
		return;
	}

	// Bring the crowd up to strength: close by at the start, out of the way later.
	const bool bFilling = GetWorld()->GetTimeSeconds() < 2.0;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 n = 0; n < Pedestrians::SpawnPerFrame && Walkers.Num() < Crowd; ++n)
	{
		FWalker Walker;
		if (!PickSpot(Camera, Looking, bFilling, bFilling ? Pedestrians::FillNear : AppearBeyond, Walker.Line, Walker.Along, Walker.GroundZ))
		{
			break;
		}
		Walker.bGroundKnown = true;
		Walker.Dir = Random.FRand() < 0.5f ? 1 : -1;
		Walker.Speed = Random.FRandRange(Pedestrians::MinSpeed, Pedestrians::MaxSpeed);
		Walker.Lane = Random.FRandRange(Pedestrians::MinLane, Pedestrians::MaxLane);
		FVector2D Tangent;
		const FVector2D At = PointOn(Lines[Walker.Line], Walker.Along, Tangent);
		Walker.Person = GetWorld()->SpawnActor<APedestrian>(APedestrian::StaticClass(), FVector(At, Camera.Z), FRotator::ZeroRotator, Params);
		if (!Walker.Person)
		{
			break;
		}
		Walker.Person->SetOutfit(PersonModel::RandomOutfit(Random));
		Walkers.Add(Walker);
	}

	// Where the player is, on foot or at the wheel, and how fast he goes
	// (the car moves itself, without physics, so from how far it went).
	const double Now = GetWorld()->GetTimeSeconds();
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const bool bPlayerAbout = Player && !Player->IsHidden() && DeltaSeconds > 0.f;
	if (bPlayerAbout)
	{
		const FVector Here = Player->GetActorLocation();
		if (LastPlayer.Get() != Player)
		{
			LastPlayer = Player;
			LastPlayerAt = Here;
		}
		PlayerVelocity = FMath::Lerp(PlayerVelocity, (Here - LastPlayerAt) / DeltaSeconds, 0.5);
		LastPlayerAt = Here;
		PlayerAt = Here;
		PlayerForward = FVector2D(Player->GetActorForwardVector()).GetSafeNormal();
		bPlayerDriving = Player->IsA<APestiCar>();
	}

	for (FWalker& Walker : Walkers)
	{
		if (!IsValid(Walker.Person))
		{
			continue;
		}

		// Fallen far behind: once nobody can see him, start again somewhere
		// out of the way nearer the camera. The run-over are cleared away
		// the same way, once out of sight.
		const double Away = FVector::Dist2D(Walker.Person->GetActorLocation(), Camera);
		const bool bSeen = Walker.Person->WasRecentlyRendered(Pedestrians::SeenWithin)
			|| (Walker.Blood && Walker.Blood->IsVisible() && Walker.Blood->WasRecentlyRendered(Pedestrians::SeenWithin));
		const bool bClearDead = Walker.React == EReact::Dead && Away > Pedestrians::ClearBeyond && !bSeen;
		if ((bClearDead || (Away > Range * 1.2 && (!bSeen || Away > Range * Pedestrians::GiveUpShare)))
			&& PickSpot(Camera, Looking, false, AppearBeyond, Walker.Line, Walker.Along, Walker.GroundZ))
		{
			Walker.bPlaced = false;
			ClearReact(Walker);
		}

		if (bPlayerAbout && Walker.bPlaced)
		{
			ReactToPlayer(Walker, Now);
		}
		const bool bOnWalk = Walker.React == EReact::None || Walker.React == EReact::Flee;
		const float Pace = Walker.Speed * (Walker.React == EReact::Flee ? Pedestrians::FleeShare : 1.f);
		if (Walker.React == EReact::Flee)
		{
			Walker.ReactLeft -= DeltaSeconds;
			if (Walker.ReactLeft <= 0.f)
			{
				StartReact(Walker, EReact::None, 0.f, FVector2D::ZeroVector);
			}
		}

		if (bOnWalk)
		{
			Walker.Along += Walker.Dir * Pace * DeltaSeconds;
		}
		const FLine& Line = Lines[Walker.Line];
		if (Walker.Along < 0.0 || Walker.Along > Line.Length)
		{
			Walker.Along = FMath::Clamp(Walker.Along, 0.0, Line.Length);
			TurnAtEnd(Walker);
		}

		FVector2D Tangent;
		const FVector2D OnLine = PointOn(Lines[Walker.Line], Walker.Along, Tangent);
		const FVector2D Heading = Tangent * Walker.Dir;
		// Keep to the right, so people going opposite ways pass each other.
		const FVector2D At = OnLine + FVector2D(-Heading.Y, Heading.X) * Walker.Lane;

		Walker.GroundClock -= DeltaSeconds;
		if (!Walker.bGroundKnown || Walker.GroundClock <= 0.f)
		{
			Walker.GroundClock = Pedestrians::GroundEvery;
			double Z;
			if (GroundUnder(At, Walker.bGroundKnown ? Walker.GroundZ : Camera.Z, Z))
			{
				Walker.GroundZ = Z;
				Walker.bGroundKnown = true;
			}
		}

		// Walk over to his place rather than jump there (a new line at a
		// corner, the other side of the line after turning back), and turn
		// to face the way he goes.
		const float Yaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Heading.Y, Heading.X)));
		const FVector2D Gap = At - Walker.Shown;
		const double Step = Pace * Pedestrians::CatchUp * DeltaSeconds;
		if (!Walker.bPlaced || (bOnWalk && Gap.Size() > Pedestrians::SnapDistance))
		{
			Walker.Shown = At;
			Walker.ShownYaw = Yaw;
			Walker.bPlaced = true;
		}
		else if (bOnWalk)
		{
			Walker.Shown += Gap.Size() > Step ? Gap.GetSafeNormal() * Step : Gap;
			Walker.ShownYaw = FMath::FixedTurn(Walker.ShownYaw, Yaw, Pedestrians::TurnRate * DeltaSeconds * (Walker.React == EReact::Flee ? 3.f : 1.f));
		}

		if (!PoseReacting(Walker, DeltaSeconds))
		{
			const double Size = Walker.Person->GetActorScale3D().Z;
			Walker.Person->WalkTo(FVector(Walker.Shown, Walker.GroundZ + Pedestrians::Middle * Size), Walker.ShownYaw, Pace, DeltaSeconds,
				Walker.React == EReact::Flee ? 1.f : 0.f);
		}
	}
}
