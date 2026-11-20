// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StoryPerson.h"
#include "MusicBarModel.h"
#include "BandPlayer.generated.h"

class UProceduralMeshComponent;

/**
 * One of a bar band's players (MusicBarModel): Dzsoki at his cimbalom, a
 * fiddler, the bass player, the clarinettist or Rózsi singing. Set the part
 * (SetPart) and the clothes before spawning him for good (SpawnActorDeferred);
 * he gets his instrument in his hands (the cimbalom stands in front of him)
 * and plays in time while SetPlaying is on, or stands at ease with it.
 */
UCLASS()
class PESTIALVILAG_API ABandPlayer : public AStoryPerson
{
	GENERATED_BODY()

public:
	ABandPlayer();

	virtual void Tick(float DeltaSeconds) override;

	void SetPart(MusicBarModel::EPlayer InPart) { Part = InPart; }
	MusicBarModel::EPlayer GetPart() const { return Part; }
	void SetPlaying(bool bInPlaying) { bPlaying = bInPlaying; }

protected:
	virtual void BeginPlay() override;

private:
	/** A shape from the model as a mesh under Parent, at Location turned by Rotation. */
	UProceduralMeshComponent* AddProp(const PersonKit::FPiece& Shape, USceneComponent* Parent, const FVector& Location, const FRotator& Rotation);
	/** Puts the arms, legs and body in the pose for this moment. */
	void Pose(const MusicBarModel::FPlay& Play);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> Props;

	/** The props held in each hand (may be null), turned as the arm moves. */
	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> LeftHand;
	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> RightHand;

	MusicBarModel::EPlayer Part = MusicBarModel::EPlayer::Fiddle;
	bool bPlaying = false;
	/** Where in the music he is (beats), started a little apart for each player. */
	float Beat = 0.f;
};
