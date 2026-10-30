// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MarketModel.h"
#include "Pedestrians.h"
#include "FleaMarket.generated.h"

class AStoryPerson;
class FJsonObject;
class UBoxComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UProceduralMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Someone at the market: a seller behind a stall or a shopper going from
 * stall to stall. AFleaMarket moves and poses him.
 */
UCLASS()
class PESTIALVILAG_API AMarketPerson : public APedestrian
{
	GENERATED_BODY()

public:
	/**
	 * Holds him in a pose at Location (his middle) facing Yaw: leaning
	 * forward by Lean (degrees; back if negative), each arm raised forward
	 * by its own pitch (140 is above the head) and each thigh and knee as
	 * PersonModel::PoseLegs takes them. With bPlace false he stays where
	 * and how he stands and only his arms and legs move (for the stall
	 * keepers the story turns to face the player).
	 */
	void Pose(const FVector& Location, double Yaw, float Lean, float ArmLeft, float ArmRight,
		float LegLeft = 0.f, float LegRight = 0.f, float KneeLeft = 0.f, float KneeRight = 0.f, bool bPlace = true);
};

/**
 * The Teleki tér market of 1932, after the Fortepan aerial photos: the
 * square laid with trodden earth instead of lawn, a grid of big open
 * market sheds on its east side with tables under them, and on the west
 * an open flea market of tables and blankets spread with second-hand
 * clothes, shoes, pots, junk, books, furniture, crockery and vegetables.
 * Sellers stand behind some stalls, fidgeting, tidying their goods and
 * calling out their wares; shoppers go from stall to stall to look and
 * haggle. They are people like those in the streets: they stumble when
 * bumped, step aside for a runner, leap clear of a car or run from it,
 * and can be knocked down or run over. A car driven fast into a stall
 * smashes it: its boards, legs and goods fly and roll (the big sheds stand
 * firm, but the tables under them break), and the seller curses the
 * driver. Once the camera is far away and nobody sees, the stalls stand
 * again and everyone is back in his place. Anyone at the market has a word
 * for the player who walks up and presses E. Two stalls belong to people the story needs
 * (Pista bácsi, Bözsi néni): FindKeeper finds them. A third keeper is a
 * little secret: Ibn Iszhák, the carpet seller from Baghdad in a white
 * hooded robe, who has a word for anyone who walks up and presses E.
 *
 * Laid out by the street furniture script (market); the street furniture
 * actor spawns it at the world origin and calls Setup.
 */
UCLASS()
class PESTIALVILAG_API AFleaMarket : public AActor
{
	GENERATED_BODY()

public:
	AFleaMarket();

	/** Builds the whole market from the furniture file's "market" object. */
	void Setup(const FJsonObject& Market);

	/** A stall keeper the story needs, by name (as in the furniture file), or null. */
	static AStoryPerson* FindKeeper(const UWorld* World, const FString& Name);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "Market")
	TObjectPtr<UProceduralMeshComponent> Ground;

	/** All the sheds' roofs, in one mesh. */
	UPROPERTY(VisibleAnywhere, Category = "Market")
	TObjectPtr<UProceduralMeshComponent> Roofs;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	/** Cube, cylinder, sphere and cone from the engine's basic shapes. */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> Shapes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Batches;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AMarketPerson>> People;

	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<AStoryPerson>> Keepers;

	/** The stalls' wreckage, flying about after a car drove through them. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Wreckage;

	/** What stops the player walking through a stall (gone while it lies smashed). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBoxComponent>> Blockers;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> BloodPools;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BloodPaint;

private:
	/** What someone at the market is doing. */
	enum class EAct : uint8
	{
		Stand,		// at his stall (a seller) or looking at the goods (a shopper)
		Walk,		// to the next stall, or back to his own
		Aside,		// out of the way of someone running
		Stumble,	// bumped into
		Leap,		// clear of a car
		Flee,		// away from a car, or from someone run over
		Angry,		// his stall smashed: shaking his fists at the driver
		Down,		// knocked down by a car
		GetUp,
		Dead,		// run over at speed
	};

	struct FPart
	{
		int32 Batch = 0;
		int32 Instance = 0;
		int32 Shape = 0;
		FTransform Transform;
		/** A blanket on the ground stays where it lies when the goods on it fly. */
		bool bStays = false;
	};

	struct FStall
	{
		FVector Middle = FVector::ZeroVector;
		double Yaw = 0.0;
		MarketModel::EBuild Build = MarketModel::EBuild::Table;
		MarketModel::EGoods Goods = MarketModel::EGoods::Junk;
		/** Half its size along its facing and across it, all of it (cm). */
		FVector2D Half = FVector2D::ZeroVector;
		TArray<FPart> Parts;
		int32 Blocker = INDEX_NONE;
		/** Its seller, in People, or none. */
		int32 Seller = INDEX_NONE;
		/** A lighter lies on it: it is sturdy, so the lighter is not lost. */
		bool bSturdy = false;
		bool bBroken = false;
		double BrokenAt = 0.0;
		bool bSettled = false;
		/** Its flying pieces (also held in Wreckage). */
		TArray<UStaticMeshComponent*> Pieces;
	};

	struct FCrowd
	{
		bool bSeller = false;
		bool bKeeper = false;
		int32 Stall = INDEX_NONE;
		FVector2D Home = FVector2D::ZeroVector;
		float HomeYaw = 0.f;
		FVector2D At = FVector2D::ZeroVector;
		float Yaw = 0.f;
		FVector2D Target = FVector2D::ZeroVector;
		float TargetYaw = 0.f;
		float Speed = 110.f;
		EAct Act = EAct::Stand;
		/** Seconds left in what he is doing, and how long since it began. */
		float Left = 0.f;
		float Time = 0.f;
		FVector2D Shove = FVector2D::ZeroVector;
		/** His fidgeting: which gesture, and when he changes to another. */
		int32 Gesture = 0;
		float GestureLeft = 0.f;
		float Phase = 0.f;
		float NoticeAt = -1.f;
		bool bFrozen = false;
		double NextCall = 0.0;
		int32 Line = 0;
		int32 Blood = INDEX_NONE;
	};

	/** Moves and poses everyone near the camera, and has them react to the player. */
	void TickCrowd(float DeltaSeconds, const FVector& Camera, double Now);
	void React(int32 Index, double Now);
	void StartAct(FCrowd& Who, EAct Act, float Seconds, const FVector2D& Shove = FVector2D::ZeroVector);
	void RunOver(int32 Index, double Speed);
	/** Has him run off, away from From, without running into a stall if he can help it. */
	void Flee(FCrowd& Who, const FVector2D& From, float Seconds);
	/** Back to his stall, or on to another stall to look at (a shopper). */
	void NextWalk(FCrowd& Who);
	bool ClearWay(const FVector2D& From, const FVector2D& To) const;
	/** Where a shopper stands to look at a stall, and which way he faces. */
	FVector2D StallFront(const FStall& Stall, FRandomStream& Random, float& OutYaw) const;
	/** The player can talk to anyone nearby with E; true if someone was in reach. */
	bool TickTalk(double Now);
	/** The carpet seller's own lines; true if the player is with him. */
	bool TickCarpetSeller();
	void TickStalls(const FVector& Camera, float DeltaSeconds, double Now);
	void Smash(int32 Index, double Now);
	void PutBack(FStall& Stall);
	void Shout(const TCHAR* Who, const TCHAR* Hu, const TCHAR* En, bool bOverOthers = false);
	void PutHome(FCrowd& Who);

	TArray<FStall> StallList;
	TArray<FCrowd> Crowd;
	FRandomStream CrowdRandom;
	double LastShout = -10.0;
	int32 NextReset = 0;

	/** The player (or the car he drives) and how fast it goes, worked out each frame. */
	TWeakObjectPtr<AActor> LastPlayer;
	FVector LastPlayerAt = FVector::ZeroVector;
	FVector PlayerVelocity = FVector::ZeroVector;
	FVector PlayerAt = FVector::ZeroVector;
	FVector2D PlayerForward = FVector2D(1.0, 0.0);
	bool bPlayerDriving = false;
	bool bPlayerAbout = false;

	/** One shape, in world cm (Size is the whole size), batched by shape and colour: its batch and instance. */
	FIntPoint Add(int32 Shape, const FLinearColor& Color, const FVector& At, const FVector& Size,
		const FRotator& Turn = FRotator::ZeroRotator, bool bCollide = false);
	void BuildGround(const FJsonObject& Market);
	/** Adds a shed's pyramid roof to the roof mesh. */
	void BuildShedRoof(const FVector& Middle, double Yaw, double Width, double Depth);
	AMarketPerson* SpawnPerson(const FVector& Feet, double Yaw, FRandomStream& Random, const FString& Keeper = FString(), bool bShopper = false);

	double GroundZ = 0.0;
	/** The carpet seller's next line. */
	int32 CarpetLine = 0;
	TArray<FVector> RoofVertices;
	TArray<int32> RoofTriangles;
	TArray<FVector> RoofNormals;
	TMap<FString, int32> BatchIndex;
};
