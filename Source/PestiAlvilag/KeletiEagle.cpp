// Fill out your copyright notice in the Description page of Project Settings.

#include "KeletiEagle.h"

#include "CarBody.h"
#include "Components/SceneComponent.h"
#include "EagleModel.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

namespace KeletiEagle
{
	// The circle, in the station's frame from the middle of its front (cm):
	// over the front half of the hall, well above the roof ridge (36 m).
	const FVector CircleCentre(3000.0, 0.0, 5800.0);
	constexpr double CircleRadius = 3200.0;
	constexpr double CircleBob = 300.0;
	constexpr double CircleSpeed = 1000.0;    // cm/s
	constexpr double CircleBank = 18.0;

	// The perch: the attic cornice above the arch (Keleti AtticTop + 70),
	// near its front edge, between two of the statues on top.
	const FVector Perch(-30.0, -560.0, 3320.0 + EagleModel::PerchHeight);
	constexpr double PerchPitch = 30.0;

	constexpr double MinCircling = 60.0;      // seconds
	constexpr double MaxCircling = 150.0;
	constexpr double MinPerched = 25.0;
	constexpr double MaxPerched = 45.0;
	constexpr double GlideSpeed = 900.0;      // cm/s along a landing or leaving curve
	constexpr double FoldTime = 0.8;          // seconds to fold or spread the wings

	// Wingbeats: a few strong beats now and then while circling, quick
	// braking beats when landing, hard ones taking off.
	constexpr double BeatRate = 2.2;          // beats a second
	constexpr double BeatSize = 30.0;         // degrees either way
	constexpr double Dihedral = 6.0;          // the wings held a little up in a glide

	FVector Bezier(const FVector* P, double T)
	{
		const double U = 1.0 - T;
		return P[0] * (U * U * U) + P[1] * (3.0 * U * U * T) + P[2] * (3.0 * U * T * T) + P[3] * (T * T * T);
	}

	FVector BezierDirection(const FVector* P, double T)
	{
		const double U = 1.0 - T;
		return ((P[1] - P[0]) * (3.0 * U * U) + (P[2] - P[1]) * (6.0 * U * T) + (P[3] - P[2]) * (3.0 * T * T)).GetSafeNormal();
	}

	double CurveLength(const FVector* P)
	{
		double Length = 0.0;
		FVector Last = P[0];
		for (int32 i = 1; i <= 16; ++i)
		{
			const FVector Next = Bezier(P, i / 16.0);
			Length += FVector::Dist(Last, Next);
			Last = Next;
		}
		return Length;
	}

	FQuat Turn(const FVector& Axis, double Degrees)
	{
		return FQuat(Axis, FMath::DegreesToRadians(Degrees));
	}
}

AKeletiEagle::AKeletiEagle()
{
	PrimaryActorTick.bCanEverTick = true;
	BaseMaterial = CarBody::LoadPaintMaterial();

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	auto MakePart = [this](const TCHAR* Name, USceneComponent* Parent)
	{
		UProceduralMeshComponent* Part = CreateDefaultSubobject<UProceduralMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCanEverAffectNavigation(false);
		return Part;
	};
	Body = MakePart(TEXT("Body"), Root);
	WingLeft = MakePart(TEXT("WingLeft"), Body);
	WingRight = MakePart(TEXT("WingRight"), Body);
	Feet = MakePart(TEXT("Feet"), Root);
}

void AKeletiEagle::BeginPlay()
{
	Super::BeginPlay();

	EagleModel::FEagle Eagle;
	EagleModel::Build(Eagle);
	UProceduralMeshComponent* Parts[EagleModel::NumParts] = { Body, WingLeft, WingRight, Feet };
	for (int32 p = 0; p < EagleModel::NumParts; ++p)
	{
		const PersonKit::FPiece& Piece = Eagle.Parts[p];
		for (int32 s = 0; s < Piece.Sections.Num(); ++s)
		{
			const PersonKit::FSection& Section = Piece.Sections[s];
			Parts[p]->CreateMeshSection_LinearColor(s, Section.Vertices, Section.Triangles, Section.Normals,
				TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
			if (BaseMaterial)
			{
				UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
				Paint->SetVectorParameterValue(TEXT("Color"), Section.Color);
				Parts[p]->SetMaterial(s, Paint);
			}
		}
	}

	Home = GetActorLocation();
	SetActorRotation(FRotator::ZeroRotator);
	CircleAngle = FMath::FRandRange(0.0, 2.0 * PI);
	StateTime = FMath::FRandRange(KeletiEagle::MinCircling, KeletiEagle::MaxCircling);
	BurstWait = FMath::FRandRange(2.0, 8.0);
	SetActorLocation(CirclePoint(CircleAngle));
	LastYaw = FMath::RadiansToDegrees(CircleAngle) + 90.0;
	Pose(0.0, KeletiEagle::Dihedral, 0.0);
}

FVector AKeletiEagle::CirclePoint(double Angle) const
{
	using namespace KeletiEagle;
	return Home + CircleCentre + FVector(FMath::Cos(Angle) * CircleRadius, FMath::Sin(Angle) * CircleRadius, FMath::Sin(Angle * 2.0) * CircleBob);
}

void AKeletiEagle::Pose(double InFold, double Beat, double BodyPitch)
{
	using KeletiEagle::Turn;
	for (const double Side : { -1.0, 1.0 })
	{
		UProceduralMeshComponent* Wing = Side < 0.0 ? WingLeft : WingRight;
		// Folding: the wing turns leading edge up, then swings back along the
		// body, and its feathers close up shorter and narrower.
		const FQuat Folded = Turn(FVector::UpVector, Side * 90.0 * InFold) * Turn(FVector::RightVector, -90.0 * InFold);
		const FQuat Beating = Turn(FVector::ForwardVector, Side * Beat);
		Wing->SetRelativeLocationAndRotation(EagleModel::Shoulder(Side) + FVector(-2.0, 4.0 * Side, 6.0) * InFold, Folded * Beating);
		Wing->SetRelativeScale3D(FVector(1.0 - 0.4 * InFold, 1.0 - 0.45 * InFold, 1.0));
	}
	Body->SetRelativeRotation(Turn(FVector::RightVector, -BodyPitch));
	Feet->SetVisibility(InFold > 0.5);
}

void AKeletiEagle::Face(const FVector& Direction, double Bank)
{
	using KeletiEagle::Turn;
	const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X));
	const double Climb = FMath::Clamp(FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(Direction.Z, -1.0, 1.0))), -25.0, 25.0);
	SetActorRotation(Turn(FVector::UpVector, Yaw) * Turn(FVector::RightVector, -Climb) * Turn(FVector::ForwardVector, -Bank));
	LastYaw = Yaw;
}

void AKeletiEagle::StartGlide(EFlight Next, const FVector& Control1, const FVector& Control2, const FVector& End)
{
	Curve[0] = GetActorLocation();
	Curve[1] = Control1;
	Curve[2] = Control2;
	Curve[3] = End;
	CurveT = 0.0;
	CurveSeconds = FMath::Max(4.0, KeletiEagle::CurveLength(Curve) / KeletiEagle::GlideSpeed);
	AfterGlide = Next;
}

void AKeletiEagle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	using namespace KeletiEagle;
	const double Dt = FMath::Min<double>(DeltaSeconds, 0.1);
	StateTime -= Dt;
	BeatPhase += Dt * BeatRate * 2.0 * PI;

	switch (Flight)
	{
	case EFlight::Circling:
	{
		CircleAngle += CircleSpeed / CircleRadius * Dt;
		const FVector At = CirclePoint(CircleAngle);
		const FVector Ahead = CirclePoint(CircleAngle + 0.05);
		SetActorLocation(At);
		Face(Ahead - At, CircleBank);

		// Mostly gliding, with a burst of a few beats now and then.
		BurstWait -= Dt;
		if (BurstWait <= 0.0 && BurstLeft <= 0.0)
		{
			BurstLeft = FMath::FRandRange(1.5, 2.5);
			BurstWait = FMath::FRandRange(6.0, 12.0);
			BeatPhase = 0.0;
		}
		double Beat = Dihedral;
		if (BurstLeft > 0.0)
		{
			BurstLeft -= Dt;
			Beat += BeatSize * FMath::Sin(BeatPhase);
		}
		Pose(0.0, Beat, 0.0);

		if (StateTime <= 0.0)
		{
			// Down over the square, round in front of the station, and in to the ledge.
			const FVector Forward = (Ahead - At).GetSafeNormal();
			const FVector Landing = Home + Perch;
			Flight = EFlight::Landing;
			StartGlide(EFlight::Perched, At + Forward * 2500.0, Landing + FVector(-1800.0, 0.0, 500.0), Landing);
		}
		break;
	}
	case EFlight::Landing:
	case EFlight::Leaving:
	{
		CurveT = FMath::Min(1.0, CurveT + Dt / CurveSeconds);
		const FVector At = Bezier(Curve, CurveT);
		const FVector Direction = BezierDirection(Curve, FMath::Min(CurveT, 0.98));
		const double Turning = FMath::FindDeltaAngleDegrees(LastYaw, FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X)));
		SetActorLocation(At);
		Face(Direction, FMath::Clamp(Turning / FMath::Max(Dt, 0.001) * 0.3, -35.0, 35.0));

		double Beat = Dihedral;
		double Pitch = 0.0;
		if (Flight == EFlight::Landing && CurveT > 0.8)
		{
			// Braking: nose up, quick deep beats.
			const double Brake = (CurveT - 0.8) / 0.2;
			Pitch = 35.0 * Brake;
			Beat += 40.0 * FMath::Sin(BeatPhase * 1.8) * Brake;
		}
		else if (Flight == EFlight::Leaving)
		{
			// Hard beats away from the ledge, then a glide back up.
			Fold = FMath::Max(0.0, Fold - Dt / FoldTime);
			Pitch = PerchPitch * Fold;
			const double Effort = FMath::Clamp(1.0 - CurveT * 2.5, 0.0, 1.0);
			Beat += 38.0 * FMath::Sin(BeatPhase * 1.4) * Effort;
		}
		Pose(Fold, Beat, Pitch);

		if (CurveT >= 1.0)
		{
			Flight = AfterGlide;
			if (Flight == EFlight::Perched)
			{
				StateTime = FMath::FRandRange(MinPerched, MaxPerched);
				PerchYaw = 180.0;
				LookWait = 2.0;
			}
			else
			{
				StateTime = FMath::FRandRange(MinCircling, MaxCircling);
				BurstWait = 1.0;
			}
		}
		break;
	}
	case EFlight::Perched:
	{
		// Fold up, turn to look out over Baross tér, and now and then look
		// about a little.
		Fold = FMath::Min(1.0, Fold + Dt / FoldTime);
		LookWait -= Dt;
		if (LookWait <= 0.0)
		{
			PerchYaw = 180.0 + FMath::FRandRange(-35.0, 35.0);
			LookWait = FMath::FRandRange(3.0, 7.0);
		}
		const double Yaw = LastYaw + FMath::Clamp(FMath::FindDeltaAngleDegrees(LastYaw, PerchYaw), -120.0 * Dt, 120.0 * Dt);
		SetActorLocation(Home + Perch);
		Face(FVector(FMath::Cos(FMath::DegreesToRadians(Yaw)), FMath::Sin(FMath::DegreesToRadians(Yaw)), 0.0), 0.0);
		Pose(Fold, 0.0, PerchPitch * Fold);

		if (StateTime <= 0.0)
		{
			// Drop off the ledge out over the square, then swing round and
			// climb back to the circle.
			const FVector From = Home + Perch;
			const FVector Out = GetActorForwardVector().GetSafeNormal2D();
			const FVector Drop = From + Out * 900.0 - FVector(0.0, 0.0, 400.0);
			const FVector Centre = Home + CircleCentre;
			CircleAngle = FMath::Atan2(Drop.Y - Centre.Y, Drop.X - Centre.X);
			const FVector Join = CirclePoint(CircleAngle);
			const FVector Tangent(-FMath::Sin(CircleAngle), FMath::Cos(CircleAngle), 0.0);
			Flight = EFlight::Leaving;
			BeatPhase = 0.0;
			StartGlide(EFlight::Circling, Drop, Join - Tangent * 2500.0, Join);
		}
		break;
	}
	}
}
