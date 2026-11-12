// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CarModel.h"

/**
 * How each kind of car drives and how heavy it is in a crash. The numbers are
 * shares of the plain saloon's (APestiCar's own settings), so a taxi that is
 * tuned faster stays faster in whatever body it has:
 * - the little Fiat and Opel are light and quick to turn but slow at the top;
 * - the limousine and the van are heavy: slow to pull away and to turn, they
 *   lean and wallow, but they shove other cars aside in a crash;
 * - the coupe and the SSK are fast, the SSK very fast and easy to spin;
 * - the old Ford T and town car are slow, with poor brakes.
 * A kind of car not listed here drives like the saloon.
 */
namespace CarHandling
{
	struct FHandling
	{
		/** Weight in kg, for crashes: the heavier car shoves the lighter one. */
		float Mass = 1100.f;
		/** Shares of the saloon's top speed, pull, brakes and steering. */
		float TopShare = 1.f;
		float PullShare = 1.f;
		float BrakeShare = 1.f;
		float SteerShare = 1.f;
		/** Share of the saloon's tyre grip: below 1 the car slides more in bends. */
		float GripShare = 1.f;
		/** Share of the grip left with the handbrake on: below 1 the back steps out more easily. */
		float HandbrakeShare = 1.f;
		/** How quickly the turning follows the wheel (the saloon is 7): heavy cars are lazy. */
		float TurnResponse = 7.f;
		/** Seconds the pull dips while the gear changes up (old heavy gearboxes are slow). */
		float ShiftTime = 0.25f;
	};

	inline FHandling For(CarModel::EBody Body)
	{
		FHandling H;
		switch (Body)
		{
		case CarModel::EBody::Tourer:
			H.Mass = 1050.f; H.TopShare = 1.03f; H.PullShare = 1.05f; H.BrakeShare = 0.95f;
			H.SteerShare = 1.03f; H.GripShare = 0.92f; H.HandbrakeShare = 0.9f; H.TurnResponse = 7.5f; H.ShiftTime = 0.25f;
			break;
		case CarModel::EBody::Coupe:
			H.Mass = 950.f; H.TopShare = 1.1f; H.PullShare = 1.15f; H.BrakeShare = 1.05f;
			H.SteerShare = 1.1f; H.GripShare = 1.05f; H.HandbrakeShare = 0.95f; H.TurnResponse = 8.5f; H.ShiftTime = 0.2f;
			break;
		case CarModel::EBody::Limousine:
			H.Mass = 1700.f; H.TopShare = 0.96f; H.PullShare = 0.8f; H.BrakeShare = 0.85f;
			H.SteerShare = 0.82f; H.GripShare = 0.88f; H.HandbrakeShare = 1.1f; H.TurnResponse = 4.8f; H.ShiftTime = 0.35f;
			break;
		case CarModel::EBody::Van:
			H.Mass = 1500.f; H.TopShare = 0.82f; H.PullShare = 0.75f; H.BrakeShare = 0.8f;
			H.SteerShare = 0.85f; H.GripShare = 0.8f; H.HandbrakeShare = 1.1f; H.TurnResponse = 5.f; H.ShiftTime = 0.4f;
			break;
		case CarModel::EBody::Balilla:
			H.Mass = 720.f; H.TopShare = 0.85f; H.PullShare = 1.05f; H.BrakeShare = 1.f;
			H.SteerShare = 1.22f; H.GripShare = 1.05f; H.HandbrakeShare = 0.85f; H.TurnResponse = 10.f; H.ShiftTime = 0.18f;
			break;
		case CarModel::EBody::Opel:
			H.Mass = 780.f; H.TopShare = 0.82f; H.PullShare = 0.95f; H.BrakeShare = 0.95f;
			H.SteerShare = 1.15f; H.GripShare = 1.f; H.HandbrakeShare = 0.9f; H.TurnResponse = 9.f; H.ShiftTime = 0.2f;
			break;
		case CarModel::EBody::Steyr:
			H.Mass = 1300.f; H.TopShare = 1.06f; H.PullShare = 1.05f; H.BrakeShare = 1.f;
			H.SteerShare = 0.95f; H.GripShare = 1.02f; H.HandbrakeShare = 1.f; H.TurnResponse = 6.5f; H.ShiftTime = 0.25f;
			break;
		case CarModel::EBody::Ssk:
			H.Mass = 1450.f; H.TopShare = 1.38f; H.PullShare = 1.6f; H.BrakeShare = 1.05f;
			H.SteerShare = 1.f; H.GripShare = 0.95f; H.HandbrakeShare = 0.75f; H.TurnResponse = 8.f; H.ShiftTime = 0.2f;
			break;
		case CarModel::EBody::FordT:
			// Tall, narrow and light, 20 horsepower and two gears: slow, poor brakes, tips into bends.
			H.Mass = 850.f; H.TopShare = 0.7f; H.PullShare = 0.8f; H.BrakeShare = 0.7f;
			H.SteerShare = 0.95f; H.GripShare = 0.8f; H.HandbrakeShare = 0.9f; H.TurnResponse = 6.f; H.ShiftTime = 0.15f;
			break;
		case CarModel::EBody::TownCar:
			// Big, old and heavy: slow everywhere, but it shoves others aside.
			H.Mass = 1600.f; H.TopShare = 0.72f; H.PullShare = 0.7f; H.BrakeShare = 0.7f;
			H.SteerShare = 0.8f; H.GripShare = 0.85f; H.HandbrakeShare = 1.05f; H.TurnResponse = 4.5f; H.ShiftTime = 0.45f;
			break;
		default:
			break;
		}
		return H;
	}

	/** Turning a car round needs this much push per kg (cm^2): a box 4.2 m long and 1.7 m wide. */
	constexpr float SpinPerKg = (420.f * 420.f + 170.f * 170.f) / 12.f;
	/** No crash spins a car faster than this (degrees a second). */
	constexpr float MaxSpin = 260.f;

	/** What a crash does to the two that met: the change to each one's speed (cm/s) and spin (degrees a second, + turns right). */
	struct FBump
	{
		FVector PushA = FVector::ZeroVector;
		float SpinA = 0.f;
		FVector PushB = FVector::ZeroVector;
		float SpinB = 0.f;
	};

	/**
	 * A crash between A and B, touching at Contact. Normal points from B
	 * towards A, flat. MassB 0 is something that cannot move (a wall, a tram).
	 * Bounce 0..1: how much of the speed they met at springs back. Friction:
	 * how much the rub along the touching sides slows their sliding past.
	 * A hit off the middle turns the car round as well as shoving it.
	 * False when they are not coming together (nothing to do).
	 */
	inline bool Collide(const FVector& Normal, const FVector& Contact,
		const FVector& CentreA, const FVector& VelA, float MassA,
		const FVector& CentreB, const FVector& VelB, float MassB,
		float Bounce, float Friction, FBump& Out)
	{
		const FVector Rel = FVector(VelA.X - VelB.X, VelA.Y - VelB.Y, 0.0);
		const float Closing = -static_cast<float>(FVector::DotProduct(Rel, Normal));
		if (Closing <= 0.f || MassA <= 0.f)
		{
			return false;
		}
		const FVector ArmA = FVector(Contact.X - CentreA.X, Contact.Y - CentreA.Y, 0.0);
		const FVector ArmB = FVector(Contact.X - CentreB.X, Contact.Y - CentreB.Y, 0.0);
		const float TurnA = static_cast<float>(FVector::CrossProduct(ArmA, Normal).Z);
		const float TurnB = static_cast<float>(FVector::CrossProduct(ArmB, Normal).Z);
		const float EaseA = 1.f / MassA + TurnA * TurnA / (MassA * SpinPerKg);
		const float EaseB = MassB > 0.f ? 1.f / MassB + TurnB * TurnB / (MassB * SpinPerKg) : 0.f;
		const float Shove = (1.f + Bounce) * Closing / (EaseA + EaseB);

		// The rub along the touching sides: never more than stops the sliding past.
		const FVector Slip = FVector::VectorPlaneProject(Rel, Normal);
		const float SlipSpeed = static_cast<float>(Slip.Size());
		const float Rub = SlipSpeed > 1.f
			? FMath::Min(Friction * Shove, SlipSpeed / (1.f / MassA + (MassB > 0.f ? 1.f / MassB : 0.f)))
			: 0.f;
		const FVector Impulse = Normal * Shove - (SlipSpeed > 1.f ? Slip / SlipSpeed * Rub : FVector::ZeroVector);

		Out.PushA = Impulse / MassA;
		Out.SpinA = FMath::Clamp(FMath::RadiansToDegrees(static_cast<float>(FVector::CrossProduct(ArmA, Impulse).Z) / (MassA * SpinPerKg)),
			-MaxSpin, MaxSpin);
		if (MassB > 0.f)
		{
			Out.PushB = -Impulse / MassB;
			Out.SpinB = FMath::Clamp(FMath::RadiansToDegrees(static_cast<float>(FVector::CrossProduct(ArmB, -Impulse).Z) / (MassB * SpinPerKg)),
				-MaxSpin, MaxSpin);
		}
		return true;
	}
}
