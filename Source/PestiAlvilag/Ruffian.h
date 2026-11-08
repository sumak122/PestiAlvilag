// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PestiPerson.h"
#include "Ruffian.generated.h"

/**
 * One of the Teleki tér ruffians ("jasszok") of mission 4: Bika the boss,
 * Svihák the dandy, Csontos the beanpole and little Pöttöm. They pick on
 * Pista bácsi for his weekly money, then on the player.
 *
 * In the group fight the mission tells each what to do: stand about,
 * circle round the player at a distance with the fists up, waiting their
 * turn (Circle), come in and fight him by the same rules as the player and
 * the pickpocket (Attack, Brawl), or turn tail and run (Flee). Beaten, he
 * sits on the ground with his hands up.
 *
 * Bandi's boxing yard puts the men of its ring in the same skin, one at a
 * time: some quicker, some harder hitting, one who covers up a lot.
 *
 * Mission 5 ("A felmondás") borrows the same fighting for the people in
 * Friedrich Hermann's shop on Garay utca: old Hermann, who goes down at
 * the first blow, his son Géza and his two clerks.
 */
UENUM()
enum class ERuffian : uint8
{
	Bika,
	Svihak,
	Csontos,
	Pottom,
	// Mission 5: Friedrich Hermann's shop.
	Hermann,
	Geza,
	ClerkOne,
	ClerkTwo,
	// Bandi's boxing yard (ABoxingYard): the five men to beat in the ring, in order.
	BoxBandi,
	BoxJeno,
	BoxMazsas,
	BoxGyuri,
	BoxVasoklu,
	// Anyone off the street the player picks a fight with (AStreetFights): dressed as he walked.
	Passerby
};

UCLASS()
class PESTIALVILAG_API ARuffian : public APestiPerson
{
	GENERATED_BODY()

public:
	ARuffian();

	virtual void Tick(float DeltaSeconds) override;

	/** Which of them he is; set before he is spawned for good (SpawnActorDeferred). */
	ERuffian Kind = ERuffian::Bika;

	/** A passer-by's own clothes (Kind Passerby); set before he is spawned for good. */
	PersonKit::FOutfit StreetDress;

	/** His name over the fight bar and on what he says. */
	FString GetSpeakerName() const;

	/** Fists up against Player, waiting his turn around Spot. */
	void Circle(APestiPerson* Player, const FVector& Spot);
	/** In at Player; Eager below 1 thinks slower and doesn't grab. */
	void Attack(APestiPerson* Player, float Eager);
	/** Off and away from Player, as fast as he can, then gone. */
	void Flee(APestiPerson* Player);
	/** Back on his feet, fists down, healed, standing at At facing Yaw. */
	void StandAgain(const FVector& At, double Yaw);

	bool IsBeaten() const { return Act == Brawl::EAct::Beaten; }
	bool IsAttacking() const { return Mode == EMode::Attack; }
	bool IsGone() const { return Mode == EMode::Gone; }
	bool IsFleeing() const { return Mode == EMode::Flee; }

	virtual FString GetFightName() const override;
	virtual float GetHealthShare() const override;
	virtual float GetBlowStrength() const override;

protected:
	virtual void BeginPlay() override;
	virtual PersonKit::FOutfit GetDress() const override;
	virtual void LoseHealth(float Amount) override;
	virtual float GetVoicePitch() const override;

private:
	enum class EMode : uint8 { Idle, Circle, Attack, Flee, Gone };

	void TickCircle();
	void TickAttack();
	void TickFlee();
	float GetMaxHealth() const;
	/** How much faster than the usual he thinks in a fight (1 the usual). */
	float Quickness() const;

	EMode Mode = EMode::Idle;
	float Health = 0.f;
	float Eagerness = 1.f;
	FVector RingSpot = FVector::ZeroVector;
	FVector FleeWay = FVector::ZeroVector;
	double FleeUntil = 0.0;

	// His head in a fight (as the pickpocket's): when he next decides,
	// guards until, backs off until, which way he circles, a second jab coming.
	double NextThink = 0.0;
	double GuardUntil = 0.0;
	double BackOffUntil = 0.0;
	float CircleSide = 1.f;
	double CircleFlipAt = 0.0;
	double SecondJabAt = -1.0;
	bool bSawWindUp = false;
	/** The player left himself open after a finisher, seen once. */
	bool bSawOpening = false;
};
