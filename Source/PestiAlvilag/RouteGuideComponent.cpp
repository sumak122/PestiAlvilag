// Fill out your copyright notice in the Description page of Project Settings.

#include "RouteGuideComponent.h"

#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "TrafficManager.h"
#include "UObject/ConstructorHelpers.h"

namespace RouteGuide
{
	// Further than this from the route counts as having left it (cm).
	constexpr double OffRoute = 1500.0;
	// A target that moves more than this gets a new route (cm).
	constexpr double GoalMoved = 200.0;
	// Wait at least this long between plans (s).
	constexpr double PlanInterval = 1.5;

	// A bend sharper than this at a route point is a turn (degrees).
	constexpr double TurnAngle = 35.0;
	// Bends are measured over at least this much road either side (cm), so
	// a curved street drawn in short pieces does not count as turns.
	constexpr double MeasureLength = 1500.0;
	// Arrows for at most this many turns, this far ahead along the route.
	constexpr int32 MaxArrows = 2;
	constexpr double LookAhead = 40000.0;

	constexpr double ArrowHeight = 450.0;
	constexpr float ArrowSize = 400.f;
	// Warm yellow, mostly see-through.
	const FColor ArrowColor(255, 205, 90, 110);

	FVector Flat(const FVector& V)
	{
		return FVector(V.X, V.Y, 0.0);
	}
}

URouteGuideComponent::URouteGuideComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TextMaterial(TEXT("/Engine/EngineMaterials/DefaultTextMaterialTranslucent.DefaultTextMaterialTranslucent"));
	SeeThroughMaterial = TextMaterial.Object;
}

void URouteGuideComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	for (int32 i = 0; i < RouteGuide::MaxArrows; ++i)
	{
		UTextRenderComponent* Arrow = NewObject<UTextRenderComponent>(Owner, NAME_None, RF_Transient);
		Arrow->SetUsingAbsoluteLocation(true);
		Arrow->SetUsingAbsoluteRotation(true);
		Arrow->SetUsingAbsoluteScale(true);
		Arrow->SetupAttachment(this);
		Arrow->SetHorizontalAlignment(EHTA_Center);
		Arrow->SetVerticalAlignment(EVRTA_TextCenter);
		Arrow->SetWorldSize(RouteGuide::ArrowSize);
		Arrow->SetTextRenderColor(RouteGuide::ArrowColor);
		if (SeeThroughMaterial)
		{
			Arrow->SetTextMaterial(SeeThroughMaterial);
		}
		Arrow->SetCastShadow(false);
		Arrow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Arrow->SetVisibility(false);
		Arrow->RegisterComponent();
		Arrows.Add(Arrow);
	}
}

void URouteGuideComponent::Stop()
{
	Route.Reset();
	NextPlanTime = 0.0;
	HideArrows();
}

void URouteGuideComponent::HideArrows()
{
	for (UTextRenderComponent* Arrow : Arrows)
	{
		if (Arrow)
		{
			Arrow->SetVisibility(false);
		}
	}
}

int32 URouteGuideComponent::NearestSegment(const FVector& At, double& OutDistance) const
{
	int32 Best = INDEX_NONE;
	OutDistance = TNumericLimits<double>::Max();
	for (int32 i = 0; i + 1 < Route.Num(); ++i)
	{
		const FVector Closest = FMath::ClosestPointOnSegment(RouteGuide::Flat(At), RouteGuide::Flat(Route[i]), RouteGuide::Flat(Route[i + 1]));
		const double Distance = FVector::Dist2D(Closest, At);
		if (Distance < OutDistance)
		{
			OutDistance = Distance;
			Best = i;
		}
	}
	return Best;
}

void URouteGuideComponent::Plan(const FVector& From, const FVector& To)
{
	Goal = To;
	NextPlanTime = GetWorld()->GetTimeSeconds() + RouteGuide::PlanInterval;
	Route.Reset();
	TActorIterator<ATrafficManager> It(GetWorld());
	if (!It || !It->FindPath(From, To, Route) || Route.Num() == 0)
	{
		Route.Reset();
		return;
	}
	Route.Add(To);

	// The plan starts at the nearest street corner, which may be behind the
	// player: start it where the player is instead.
	double Distance = 0.0;
	const int32 Segment = NearestSegment(From, Distance);
	if (Segment > 0)
	{
		Route.RemoveAt(0, Segment);
	}
}

void URouteGuideComponent::Guide(const FVector& From, const FVector& To)
{
	const bool bCanPlan = GetWorld()->GetTimeSeconds() >= NextPlanTime;
	double Distance = 0.0;
	const int32 Segment = Route.Num() >= 2 ? NearestSegment(From, Distance) : INDEX_NONE;
	const bool bLost = Segment == INDEX_NONE || Distance > RouteGuide::OffRoute || FVector::Dist2D(Goal, To) > RouteGuide::GoalMoved;
	if (bLost)
	{
		if (bCanPlan)
		{
			Plan(From, To);
		}
	}
	else if (Segment > 0)
	{
		// Drop what is already behind.
		Route.RemoveAt(0, Segment);
	}
	PlaceArrows();
}

void URouteGuideComponent::PlaceArrows()
{
	HideArrows();

	// Walk the route ahead and find the points where it bends sharply,
	// judging each bend over a stretch of road either side.
	auto PointBack = [this](int32 i, double Length)
	{
		FVector At = Route[i];
		for (int32 k = i - 1; k >= 0; --k)
		{
			At = Route[k];
			Length -= FVector::Dist2D(Route[k], Route[k + 1]);
			if (Length <= 0.0)
			{
				break;
			}
		}
		return At;
	};
	auto PointAhead = [this](int32 i, double Length)
	{
		FVector At = Route[i];
		for (int32 k = i + 1; k < Route.Num(); ++k)
		{
			At = Route[k];
			Length -= FVector::Dist2D(Route[k - 1], Route[k]);
			if (Length <= 0.0)
			{
				break;
			}
		}
		return At;
	};

	int32 Shown = 0;
	double Along = 0.0;
	int32 LastTurn = INDEX_NONE;
	for (int32 i = 1; i + 1 < Route.Num() && Shown < Arrows.Num() && Along < RouteGuide::LookAhead; ++i)
	{
		Along += FVector::Dist2D(Route[i - 1], Route[i]);
		// One arrow per crossing, even where the bend spans a few points.
		if (LastTurn != INDEX_NONE && FVector::Dist2D(Route[LastTurn], Route[i]) < RouteGuide::MeasureLength)
		{
			continue;
		}
		const FVector In = (Route[i] - PointBack(i, RouteGuide::MeasureLength)).GetSafeNormal2D();
		const FVector Out = (PointAhead(i, RouteGuide::MeasureLength) - Route[i]).GetSafeNormal2D();
		if (In.IsNearlyZero() || Out.IsNearlyZero())
		{
			continue;
		}
		const double InYaw = In.Rotation().Yaw;
		const double Turn = FMath::FindDeltaAngleDegrees(InYaw, Out.Rotation().Yaw);
		if (FMath::Abs(Turn) < RouteGuide::TurnAngle)
		{
			continue;
		}

		// The arrow faces the way the player comes from; yaw grows clockwise
		// seen from above, so a positive turn is to the right.
		UTextRenderComponent* Arrow = Arrows[Shown++];
		Arrow->SetWorldLocationAndRotation(Route[i] + FVector(0.0, 0.0, RouteGuide::ArrowHeight), FRotator(0.0, InYaw + 180.0, 0.0));
		Arrow->SetText(FText::FromString(Turn > 0.0 ? TEXT(">>") : TEXT("<<")));
		Arrow->SetVisibility(true);
		LastTurn = i;
	}
}
