// Fill out your copyright notice in the Description page of Project Settings.

#include "RouteGuideComponent.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "PestiHud.h"
#include "TrafficManager.h"

namespace RouteGuide
{
	// Further than this from the route counts as having left it (cm).
	constexpr double OffRoute = 1500.0;
	// A target that moves more than this gets a new route (cm).
	constexpr double GoalMoved = 200.0;
	// Wait at least this long between plans (s).
	constexpr double PlanInterval = 1.5;

	FVector Flat(const FVector& V)
	{
		return FVector(V.X, V.Y, 0.0);
	}
}

URouteGuideComponent::URouteGuideComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URouteGuideComponent::Stop()
{
	Route.Reset();
	NextPlanTime = 0.0;
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
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->ShowRoute(Route, bOwnMark);
	}
}
