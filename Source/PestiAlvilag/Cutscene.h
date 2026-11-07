// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Cutscene.generated.h"

class ACameraActor;
class APlayerController;

/** One camera shot of a scene: the camera glides from From to FromEnd while it looks at LookAt. */
struct FSceneShot
{
	FVector From = FVector::ZeroVector;
	FVector FromEnd = FVector::ZeroVector;
	FVector LookAt = FVector::ZeroVector;
	float Seconds = 3.f;
};

/**
 * A short scene, GTA 3 style: black bars slide in at the top and bottom,
 * the HUD goes (only what people say shows, as subtitles), the player stands
 * still and a camera of its own shows a few shots. At the end it can show a
 * mission's number and name in art deco letters. Space (or Enter) skips it.
 *
 * The scene only shows things: the mission carries on underneath it (its
 * lines are said through the HUD as usual), so skipping loses nothing.
 * Only one scene plays at a time; a new one ends the old.
 */
UCLASS()
class PESTIALVILAG_API ACutscene : public AActor
{
	GENERATED_BODY()

public:
	ACutscene();

	/** Plays these shots one after the other. A title (Name not empty) shows over the last TitleSeconds. */
	static ACutscene* Play(UWorld* World, const TArray<FSceneShot>& InShots, int32 Number = 0,
		const FString& Name = FString(), const FString& NameEn = FString(), float TitleSeconds = 0.f);

	/**
	 * Two people talking, the player and someone at With (their middle):
	 * a wide shot of both from the side, then over the player's shoulder,
	 * Seconds in all. With a title, a last wider shot of the two shows it.
	 */
	static ACutscene* PlayTalk(UWorld* World, AActor* With, const FVector& WithAt, float Seconds,
		int32 Number = 0, const FString& Name = FString(), const FString& NameEn = FString());

	/** Where a camera can stand on the way from From (a person) to Want, short of walls; OutReach gives how far it got (cm). */
	static FVector Clear(UWorld* World, const FVector& From, const FVector& Want, const TArray<const AActor*>& Ignore, double& OutReach);

	/** Ends the scene that plays now, if any (a mission that fails, a thief who runs off). */
	static void StopAll(const UObject* WorldContextObject);
	static bool IsPlaying(const UObject* WorldContextObject);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void Begin();
	/** Gives the camera and the keys back to the player; the bars slide away after. */
	void Finish();

	TArray<FSceneShot> Shots;
	int32 TitleNumber = 0;
	FString TitleName;
	FString TitleEn;
	float TitleFor = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> Camera;

	/** The player held still while the scene plays. */
	TWeakObjectPtr<APlayerController> Held;

	float Clock = 0.f;
	float Total = 0.f;
	/** How far the bars are in, 0 to 1. */
	float Bars = 0.f;
	bool bStarted = false;
	bool bFinished = false;
	bool bTitleShown = false;
};
