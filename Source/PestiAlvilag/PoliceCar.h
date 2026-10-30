// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TrafficCar.h"
#include "PoliceCar.generated.h"

class APestiAlvilagGameModeBase;
class UMaterialInstanceDynamic;
class UTextRenderComponent;

/**
 * A police patrol. It drives with the traffic until the player is wanted
 * and it is close enough to hear of it; then it chases the player along the
 * streets (ignoring one-way rules) and, once in sight, straight at them.
 * Staying next to a slow or stopped player arrests them. When the player is
 * no longer wanted it goes back to patrolling.
 */
UCLASS()
class PESTIALVILAG_API APoliceCar : public ATrafficCar
{
	GENERATED_BODY()

public:
	APoliceCar();

	virtual void Tick(float DeltaSeconds) override;

	bool IsChasing() const { return bChasing; }
	const TArray<FVector>& GetChasePath() const { return ChasePath; }

	/** Top speed in a chase (cm/s); a little under the player's 2800. */
	UPROPERTY(EditAnywhere, Category = "Police")
	float ChaseSpeed = 2500.f;

	/** Degrees per second of turning in a chase. */
	UPROPERTY(EditAnywhere, Category = "Police")
	float ChaseTurnRate = 150.f;

	/** Patrols this close when the player becomes wanted join the chase (cm). */
	UPROPERTY(EditAnywhere, Category = "Police")
	float ResponseRange = 40000.f;

	/** The same once a constable has telephoned for help (cm). */
	UPROPERTY(EditAnywhere, Category = "Police")
	float CalledInRange = 200000.f;

	/** How far a patrol can see the player (cm). */
	UPROPERTY(EditAnywhere, Category = "Police")
	float SightRange = 8000.f;

protected:
	virtual void BeginPlay() override;
	virtual CarModel::FStyle PickStyle(const FLinearColor& Paint) const override;

	/** White sign on the roof reading RENDŐRSÉG; it flashes red in a chase. */
	UPROPERTY(VisibleAnywhere, Category = "Police")
	TObjectPtr<UStaticMeshComponent> RoofSign;

	UPROPERTY(VisibleAnywhere, Category = "Police")
	TObjectPtr<UTextRenderComponent> SignTextLeft;

	UPROPERTY(VisibleAnywhere, Category = "Police")
	TObjectPtr<UTextRenderComponent> SignTextRight;

private:
	void StartChase();
	void StopChase();
	void ChaseTick(APawn* Player, APestiAlvilagGameModeBase* Game, float DeltaSeconds);
	bool CanSee(const APawn* Player) const;
	void MoveChasing(const FVector& Delta, const APawn* Player, float DeltaSeconds);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SignPaint;

	bool bChasing = false;
	bool bSeesPlayer = false;
	TArray<FVector> ChasePath;
	int32 ChasePathIndex = 0;
	float ReplanTimer = 0.f;
	float SightTimer = 0.f;
	float FlashTimer = 0.f;
};
