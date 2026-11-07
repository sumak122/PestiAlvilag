// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BoxingYard.generated.h"

class APestiPerson;
class APlayerController;
class ARuffian;
class AStoryPerson;
class SWidget;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStartRingComponent;
class UStaticMesh;
class UTextRenderComponent;

/**
 * Bandi's boxing yard, a grund on Kis Fuvaros utca (BoxingYardModel). It
 * opens after mission 3: Bandi, a young boxer, stands by his ring and the
 * street comes to watch.
 *
 * Walking into the gold ring in the middle of the ring starts the next
 * fight, one man at a time (ARuffian, the Box... kinds): Bandi himself,
 * then Fürge Jenő, Mázsás, Szőke Gyuri and the champion Vasöklű. A win
 * pays a few fillér and crosses him off the slate; losing only stops the
 * fight (Bandi calls it off before the player faints), and walking out of
 * the ring gives it up. After all five, Vasöklű fights again for a smaller
 * purse.
 *
 * E by Bandi opens his lessons: the four moves and two ways of standing up
 * to a fight (Brawl::ELesson), bought with fight points. W/S or the arrows
 * choose, E or Enter learns, Q leaves.
 *
 * The actor's origin is the middle of the fence at pavement level, its +X
 * points into the lot; the game mode spawns it at BoxingYardModel::CityAt
 * turned by BoxingYardModel::CityYaw.
 */
UCLASS()
class PESTIALVILAG_API ABoxingYard : public AActor
{
	GENERATED_BODY()

public:
	ABoxingYard();

	/** Turns a point of the yard's own frame into the city, at Ground (cm). */
	static FVector ToCity(const FVector& Local, double Ground);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	enum class EBout : uint8 { None, Fighting, Over };

	/** Bandi and the watchers, once the yard is open. */
	void SpawnPeople();
	void TickIdle(APestiPerson* Player, APlayerController* PC);
	void TickBout(APestiPerson* Player);
	void StartBout(APestiPerson* Player, APlayerController* PC);
	/** The fight is over: won, or called off (lost, or walked out of). */
	void EndBout(APestiPerson* Player, bool bWon, bool bWalkedOut);
	void UpdateSlate();
	void BandiSays(const FString& Hu, const FString& En) const;

	// The lessons.
	void OpenLessons();
	void CloseLessons();
	void Learn();
	TSharedRef<SWidget> BuildLessonsWidget();

	void AddShape(int32 Shape, const FLinearColor& Color, const FVector& At, const FVector& Size, const FRotator& Turn, bool bCollide);

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	/** The engine's cube, cylinder, sphere and cone (MarketModel::EShape order). */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> Shapes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Batches;

	TMap<FString, int32> BatchIndex;

	/** Where the next fight starts: in the middle of the ring. */
	UPROPERTY(VisibleAnywhere, Category = "Boxing")
	TObjectPtr<UStartRingComponent> StartRing;

	/** The five names chalked on the slate. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextRenderComponent>> SlateNames;

	/** Bandi, standing by his corner when he is not fighting. */
	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Bandi;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AStoryPerson>> Watchers;

	/** The man in the ring with the player. */
	UPROPERTY(Transient)
	TObjectPtr<ARuffian> Rival;

	bool bPeopleUp = false;
	/** The player has been out of the gold ring since it last started a fight. */
	bool bRingArmed = false;
	EBout Bout = EBout::None;
	/** Which of the five he is fighting (0 Bandi .. 4 Vasöklű). */
	int32 BoutIndex = 0;
	/** When the man on the ground is cleared away after a fight. */
	double ClearAt = -1.0;
	/** Bandi's first words when the player first comes near. */
	bool bGreeted = false;

	TSharedPtr<SWidget> LessonsWidget;
	int32 Selected = 0;
};
