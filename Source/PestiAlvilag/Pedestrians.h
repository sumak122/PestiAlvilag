// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StoryPerson.h"
#include "Pedestrians.generated.h"

class AOsmGraybox;
class APawn;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UProceduralMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Someone out walking: a story person moved by APedestrians rather than by
 * his own tick, swinging his arms and legs as he goes.
 */
UCLASS()
class PESTIALVILAG_API APedestrian : public AStoryPerson
{
	GENERATED_BODY()

public:
	APedestrian();

	/**
	 * Puts him at Location (his middle) facing Yaw, walking at Speed. With
	 * ArmsUp above 0 he throws his arms up in fright as he goes.
	 */
	void WalkTo(const FVector& Location, double Yaw, float Speed, float DeltaSeconds, float ArmsUp = 0.f);

	/**
	 * Holds him still in a pose: tipped back by Rotation's pitch (90 is lying
	 * on his back), thighs forward by Legs, knees bent by Knees (degrees) and
	 * arms raised forward by Arms.
	 */
	void Hold(const FVector& Location, const FRotator& Rotation, float Legs, float Knees, float Arms);

	/**
	 * Stands still at Location facing Yaw and, when bPointing, holds out the
	 * arm on that side towards PointYaw (a map direction); otherwise his arms
	 * hang down.
	 */
	void PointTo(const FVector& Location, double Yaw, double PointYaw, bool bPointing);

	/**
	 * In the rain: whether he wants his umbrella up, and its cloth colour.
	 * WalkTo then raises his arm and opens it over his head (or closes it and
	 * puts it away); held in the right hand, or the left when the right one
	 * carries something. Someone with a furled umbrella opens that one.
	 */
	void WantUmbrella(bool bOpen, const FLinearColor& Cloth);

	/** Whether he has his umbrella up (or is opening it). */
	bool HasUmbrellaUp() const { return UmbrellaOpen > 0.f; }

	/**
	 * Stands still at Location facing Yaw with his arms folded against the
	 * cold, waiting out the rain; every other pose unfolds them again.
	 */
	void Huddle(const FVector& Location, double Yaw);

private:
	/** Arms back down from Huddle, if they were folded. */
	void Unfold();
	bool bArmsFolded = false;

	/** Which arm holds the umbrella: 0 left, 1 right. */
	int32 UmbrellaArm() const;
	/** Opens or shuts it a little more, and makes it the first time. */
	void UpdateUmbrella(float DeltaSeconds);
	/** Bends the elbow by Raised (0..1) and puts the umbrella in that hand. */
	void PlaceUmbrella(int32 HoldingArm, float Raised);
	/** Umbrella gone at once (knocked down, pointing, running off), the elbows straight. */
	void DropUmbrella();

	float Stride = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> Umbrella;
	/** 0 shut and away, up to 0.5 raising the arm, 1 open over his head. */
	float UmbrellaOpen = 0.f;
	bool bWantsUmbrella = false;
	FLinearColor UmbrellaCloth = FLinearColor::Black;
	/** He came out with a furled umbrella, taken out of his hand while it is open. */
	bool bFurledInUse = false;
};

/**
 * People walking the pavements around the camera. Reads the walking lines
 * ("sidewalks") from the street furniture file, keeps a crowd of walkers
 * within reach of the camera, each going along a line on the right-hand
 * side, turning onto a line that joins at its end (or turning back), and
 * moves the ones that fall far behind, once nobody can see them, to a line
 * nearer the camera. After the first moment, people only appear out of the
 * camera's view or behind a building, and they never jump: at a corner or
 * a turn they walk over to the next line.
 */
UCLASS()
class PESTIALVILAG_API APedestrians : public AActor
{
	GENERATED_BODY()

public:
	APedestrians();

	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Someone who keeps to one spot (the folk in front of a church): stands
	 * at At on the floor at Ground facing FaceYaw, or sits there with his middle
	 * SeatedUp above it when the outfit is seated (0 for one standing). He
	 * reacts to the player like any passer-by and can be asked the way;
	 * afterwards he walks back to his spot. When the player comes near on
	 * foot he turns to him, and Who calls out Hu/En now and then (no call
	 * when Hu is empty).
	 */
	void AddStander(const FVector2D& At, double Ground, double FaceYaw, const PersonKit::FOutfit& Outfit, double SeatedUp,
		const FString& Who, const FString& Hu, const FString& En);

	/**
	 * Something worth a look happened at At (a street fight, a crash, a spilled
	 * load, someone knocked down): passers-by near it stop and stand round it
	 * for Seconds (a fight keeps its crowd while it goes on, so call it again
	 * each frame), and the nearest constables walk over to see. With bFight
	 * they cheer the fighters on.
	 */
	static void Gather(const UObject* WorldContextObject, const FVector& At, float Seconds, bool bFight);

	/** The furniture file with the walking lines, relative to the project folder. */
	UPROPERTY(EditAnywhere, Category = "Pedestrians")
	FString DataFile = TEXT("Data/OsmGraybox/street_furniture.json");

	/** How many people are about at one time. */
	UPROPERTY(EditAnywhere, Category = "Pedestrians")
	int32 Crowd = 90;

	/** Nobody new appears closer than this to someone already about (cm), so they spread out. */
	UPROPERTY(EditAnywhere, Category = "Pedestrians")
	float Spacing = 600.f;

	/** People further than this from the camera are moved to somewhere nearer (cm). */
	UPROPERTY(EditAnywhere, Category = "Pedestrians")
	float Range = 9000.f;

	/** New people appear at least this far from the camera, out of the way (cm). */
	UPROPERTY(EditAnywhere, Category = "Pedestrians")
	float AppearBeyond = 3500.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BloodPaint;

private:
	/**
	 * What he is doing besides walking: stepping out of the way of someone
	 * running, stumbling back after a bump, leaping clear of a car, running
	 * off from a car on the pavement, knocked down (and getting up again), or
	 * run over at speed and lying still, or stopped to show the player the way.
	 */
	enum class EReact : uint8 { None, Aside, Stumble, Leap, Flee, Down, GetUp, Dead, Point };

	struct FLine
	{
		TArray<FVector2D> Points;
		/** Distance along the line at each point. */
		TArray<double> Along;
		double Length = 0.0;
		/** A crossing over the road, from kerb to kerb. */
		bool bCrossing = false;
	};

	/** Something on the road coming along: a car, a tram, the player. */
	struct FMover
	{
		FVector2D At = FVector2D::ZeroVector;
		FVector2D Velocity = FVector2D::ZeroVector;
	};

	struct FWalker
	{
		APedestrian* Person = nullptr;
		int32 Line = 0;
		double Along = 0.0;
		int32 Dir = 1;
		float Speed = 130.f;
		float Lane = 60.f;
		double GroundZ = 0.0;
		float GroundClock = 0.f;
		bool bGroundKnown = false;
		/** Where he is drawn: follows his place on the line, without jumps. */
		FVector2D Shown = FVector2D::ZeroVector;
		float ShownYaw = 0.f;
		bool bPlaced = false;
		EReact React = EReact::None;
		/** Seconds left in what he is doing now, and how long since it began. */
		float ReactLeft = 0.f;
		float ReactTime = 0.f;
		/** How fast he is moved off his walk while reacting (cm/s). */
		FVector2D Shove = FVector2D::ZeroVector;
		/** Not before this time does he notice a car coming (he may freeze instead). */
		float NoticeAt = -1.f;
		bool bFrozen = false;
		/** At the kerb waiting for the road to clear before crossing; on the way over, hurrying. */
		bool bWaiting = false;
		bool bRushing = false;
		float Waited = 0.f;
		float RoadClock = 0.f;
		/** The blood under him once he is run over. */
		UStaticMeshComponent* Blood = nullptr;
		/** Asked the way: the map direction he points, and whether he knows it at all. */
		float PointYaw = 0.f;
		bool bPointing = false;
		/** Whether he brought an umbrella, its colour, and how much rain it takes before he opens it (0..1). */
		bool bHasUmbrella = false;
		FLinearColor UmbrellaColor = FLinearColor::Black;
		float UmbrellaPatience = 0.f;
		/** In the rain: in a doorway out of the wet (its AStreetFurniture shelter, where he stands, which way he faces). */
		int32 Shelter = INDEX_NONE;
		FVector2D ShelterAt = FVector2D::ZeroVector;
		float ShelterYaw = 0.f;
		/** Seconds until he looks for a doorway again. */
		float ShelterClock = 0.f;
		/** Gone indoors out of a downpour (hidden until it eases), and how little rain it takes (0..1). */
		bool bIndoors = false;
		float RainShy = 1.f;
		/** Watching something (a gathering's Id, or none): where he stands from its middle, how, and when he looks for one next. */
		int32 Gawk = INDEX_NONE;
		FVector2D GawkOffset = FVector2D::ZeroVector;
		int32 GawkPose = 0;
		float GawkClock = 0.f;
		/** Stops to watch if this is below GawkShare (0..1). */
		float Nosy = 1.f;
		/** A pickpocket's takings from the player (fillér), got back by beating him. */
		int32 Stolen = 0;
		/** Keeps to one spot (AddStander): there, facing that way, and, when seated, his middle this high over the floor. */
		bool bStander = false;
		bool bSeated = false;
		FVector2D Home = FVector2D::ZeroVector;
		float HomeYaw = 0.f;
		double SeatedUp = 0.0;
		/** What he calls out to the player, and not before when again. */
		FString CallWho;
		FString CallHu;
		FString CallEn;
		double NextCall = 0.0;
	};

	bool LoadLines();
	bool CameraView(FVector& OutLocation, FVector2D& OutLooking) const;
	/**
	 * Somewhere on a line near the camera, at least MinDistance away, with
	 * the ground under it; unless bInView, only out of the camera's view.
	 */
	bool PickSpot(const FVector& Camera, const FVector2D& Looking, bool bInView, double MinDistance,
		int32& OutLine, double& OutAlong, double& OutGroundZ);
	/** At the end of a line: on to a line that starts or ends near here, or back. */
	void TurnAtEnd(FWalker& Walker);
	/** The cars, trams and the player moving near the camera this frame (for the crossings). */
	void GatherMovers(const FVector& Camera);
	/** True while something on the road would reach this crossing before he got over. */
	bool IsRoadBusy(const FLine& Line) const;
	FVector2D PointOn(const FLine& Line, double Along, FVector2D& OutTangent) const;
	/** True when a building stands between the camera and someone standing here. */
	bool BehindBuilding(const FVector& Camera, const FVector2D& At, double GroundZ) const;
	bool GroundUnder(const FVector2D& At, double Near, double& OutZ) const;
	/** Sees how the player, on foot or at the wheel, comes past him, and has him react. */
	void ReactToPlayer(FWalker& Walker, double Now);
	void StartReact(FWalker& Walker, EReact React, float Seconds, const FVector2D& Shove);
	/** The car hit him: knocked down, or killed at speed. */
	void RunOver(FWalker& Walker, double Speed);
	/**
	 * The rain thins the crowd: some go indoors out of sight and come back
	 * when it eases, those without an umbrella wait in a doorway nearby.
	 * True when it has moved and posed him itself this frame.
	 */
	bool TickRain(FWalker& Walker, float RainNow, const FVector& Camera, const FVector2D& Looking, float DeltaSeconds);
	/** Out of his doorway (back to walking). */
	void LeaveShelter(FWalker& Walker);
	/** How many more may come back out of doors this frame. */
	int32 ComeBackLeft = 0;

	/** A crowd round something: where, until when, how many watch, and its pickpocket. */
	struct FGathering
	{
		int32 Id = 0;
		FVector2D At = FVector2D::ZeroVector;
		double Until = 0.0;
		bool bFight = false;
		int32 Watchers = 0;
		/** The next ring spot to give out. */
		int32 NextSpot = 0;
		double NextShout = 0.0;
		/** Since when the player has stood in it on foot, and whether its pickpocket has had his go. */
		double PlayerSince = -1.0;
		bool bPicked = false;
	};
	TArray<FGathering> Gatherings;
	int32 NextGatheringId = 1;

	void AddGathering(const FVector& At, float Seconds, bool bFight);
	FGathering* FindGathering(int32 Id);
	/** The crowds: ends the ones that are over, calls out, lets the pickpocket work. */
	void TickGatherings(double Now, bool bPlayerAbout);
	/** A walker joins a crowd near him, walks to his place in the ring and watches; true when it has moved and posed him. */
	bool TickGawk(FWalker& Walker, double Now, float DeltaSeconds);
	void LeaveGawk(FWalker& Walker);
	/** The watcher nearest the player lifts his wallet and runs. */
	void PickPocket(FGathering& Ring);

	/** Has him pose as what he is doing now demands; false when just walking. */
	bool PoseReacting(FWalker& Walker, float DeltaSeconds);
	/** Back to walking (and the blood gone), once he is cleared away. */
	void ClearReact(FWalker& Walker);
	/** A stander's frame: back to his spot after a reaction, turning to the player near him and calling out. */
	void TickStander(FWalker& Walker, const FVector& Camera, bool bPlayerAbout, double Now, float DeltaSeconds);
	/** His middle's height over the floor: standing, or seated. */
	double MiddleUp(const FWalker& Walker) const;
	/** Someone calls out (only now and then, and not over other speech). */
	void Shout(const TCHAR* Hu, const TCHAR* En);
	/** Q on foot: the nearest passer-by stops, points the way to the mission's goal and says how far. */
	void AskTheWay(APawn* Player, double Now);
	/** A click on foot next to someone: the player picks a fight with him (AStreetFights). */
	void PickAFight(APawn* Player);
	/** The map direction to point from From to get to To along the streets, and how far that is (cm). */
	float WayFrom(const FVector2D& From, const FVector2D& To, double& OutDistance) const;

	TArray<FLine> Lines;
	/** Line ends on a coarse grid: cell -> (line, end: 0 start, 1 end). */
	TMultiMap<FIntPoint, FIntPoint> Ends;
	TArray<int32> NearLines;
	/** The longest of the near lines (people are spread along the lines by their length). */
	double NearLongest = 1.0;
	FVector NearLinesFrom = FVector(1e9);
	TArray<FWalker> Walkers;
	TArray<FMover> Movers;
	/** How many of the walkers are standers (they do not count in the crowd). */
	int32 NumStanders = 0;
	FRandomStream Random;
	bool bReady = false;

	/** The player (or the car he drives) and how fast it goes, worked out each frame. */
	TWeakObjectPtr<AActor> LastPlayer;
	FVector LastPlayerAt = FVector::ZeroVector;
	FVector PlayerVelocity = FVector::ZeroVector;
	FVector PlayerAt = FVector::ZeroVector;
	FVector2D PlayerForward = FVector2D(1.0, 0.0);
	bool bPlayerDriving = false;
	double LastShout = -10.0;
};
