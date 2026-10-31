// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PestiPerson.h"
#include "ChaseThief.generated.h"

class UStaticMesh;
class UStaticMeshComponent;

/**
 * The pickpocket of mission 3 (Zsebtolvaj): a thin young man in a flat cap
 * and a shabby jacket. He sidles up to someone, snatches the purse, and
 * runs a set way (ChaseModel::ThiefRoute) vaulting and sliding like the
 * player (FreeRun), a little faster when the chaser is close behind and
 * slower when he is far back, tiring towards the end; now and then he
 * mistimes a vault and stumbles. Brought down, he fights back with his
 * fists by the same rules as the player (Brawl), and beaten he sits on the
 * ground with his hands up.
 */
UCLASS()
class PESTIALVILAG_API AChaseThief : public APestiPerson
{
	GENERATED_BODY()

public:
	AChaseThief();

	virtual void Tick(float DeltaSeconds) override;

	/** The way he runs, in the city (cm), from where he stands. */
	void SetRoute(const TArray<FVector2D>& InRoute);
	/** Walks up behind Mark, slowly, as if browsing. */
	void SneakUpTo(AActor* Mark);
	/** True once he stands right by whoever he sneaks up on. */
	bool IsAtMark() const;
	/** Off along his route, Chaser after him; he goes like the wind at first. */
	void RunFrom(APestiPerson* Chaser);
	bool IsRunning() const { return Mode == EMode::Run; }
	/** At the end of his route: he got away. */
	bool HasGotAway() const { return Mode == EMode::Away; }
	/** How far along his route he is (0 to 1). */
	float RouteShare() const;
	/** Fists up against Player. */
	void FightBack(APestiPerson* Player);
	bool IsBeaten() const { return Act == Brawl::EAct::Beaten; }
	/** The purse in his right hand, or not. */
	void ShowPurse(bool bShow);
	/** Beaten and robbed of the purse: up and off down the rest of his route, then gone. */
	void Slink();

	virtual FString GetFightName() const override;
	virtual float GetHealthShare() const override;

protected:
	virtual void BeginPlay() override;
	virtual PersonKit::FOutfit GetDress() const override;
	virtual void LoseHealth(float Amount) override;
	/** A younger man's voice, higher than the player's. */
	virtual float GetVoicePitch() const override { return 135.f; }

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	/** The stolen purse and its clasp. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Purse;

private:
	enum class EMode : uint8 { Idle, Sneak, Run, Fight, Slink, Away };

	void TickRun(float DeltaSeconds);
	void TickFight();
	/** Heads for To at Speed, and vaults or slides over what is in the way. */
	void Steer(const FVector2D& To, float Speed);
	/** Moves on past the route points he has reached; false at the end. */
	bool NextPointAhead();
	/** Whether he bumps into the chaser's people (running he slips past everyone). */
	void SetSlipping(bool bSlip);

	EMode Mode = EMode::Idle;
	TArray<FVector2D> Route;
	/** Along the route: the point he heads for, the length of the whole and of the legs behind him (cm). */
	int32 NextPoint = 0;
	double RouteLength = 0.0;
	double LegsDone = 0.0;

	TWeakObjectPtr<AActor> Mark;
	TWeakObjectPtr<APestiPerson> Chaser;
	double RunStart = 0.0;
	float Health = 0.f;

	/** For the next thing in his way: whether he mistimes it, decided when he first sees it. */
	bool bDecided = false;
	bool bFumble = false;
	/** Not getting anywhere since then (s, -1 when moving). */
	double StuckSince = -1.0;

	// His head in a fight: when he next decides, guards until, backs off
	// until, which way he circles and when he turns about, a second jab coming.
	double NextThink = 0.0;
	double GuardUntil = 0.0;
	double BackOffUntil = 0.0;
	float CircleSide = 1.f;
	double CircleFlipAt = 0.0;
	double SecondJabAt = -1.0;
	bool bSawWindUp = false;
};
