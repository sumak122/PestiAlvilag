// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CitySynth.h"
#include "DailyLifeModel.h"
#include "DailyLife.generated.h"

class AFireEngine;
class APedestrian;
class ATrafficCart;
class ATrafficManager;
class ATram;
class UMaterialInterface;
class UPointLightComponent;
class UProceduralMeshComponent;
class USpotSound;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * The daily life of the streets round the player, kept rare:
 *
 * - At dusk a lamplighter walks a round of the lamps near the player and
 *   lights them one by one with his long pole (ADayNight::SetLampUnlit).
 * - In the open courtyards by day a woman beats a carpet on the rack, one
 *   hangs her washing on the line, or kids play football (a ball the player
 *   can kick away too).
 * - Street people: an organ grinder with his fortune-telling parrot in a
 *   courtyard or by its gate, a knife grinder at his stone in front of a
 *   shop, a chestnut seller at his stove by a kiosk in the evening.
 * - Rare events: a wedding party walking behind a fiddler to the nearest
 *   church, a funeral (a hearse in the traffic, ATrafficCart::MakeHearse,
 *   with mourners walking behind it) on its way to the Kerepesi cemetery, or
 *   the fire brigade's engine racing past with its bell (AFireEngine). The
 *   traffic waits for people walking in the road (ATrafficManager::SetRoadBlock).
 * - People get off a tram at a stop near the player and walk off, and
 *   passers-by get on (ATram's benches).
 * - Shops near the player have their iron shutters down at night; in the
 *   morning a shopkeeper pushes his up with a hook pole and goes in.
 *
 * Everyone appears out of the player's sight and goes once out of it.
 */
UCLASS()
class PESTIALVILAG_API ADailyLife : public AActor
{
	GENERATED_BODY()

public:
	ADailyLife();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	/** Everyone this actor put in the street (destroyed with it). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<APedestrian>> People;

	UPROPERTY(Transient)
	TObjectPtr<AFireEngine> Brigade;

private:
	// ---- Shared ------------------------------------------------------------

	/** The camera's place and the way it looks; false before there is one. */
	bool CameraView(FVector& OutEye, FVector& OutLook) const;
	/** True when the player could see something standing at At (in front of the camera and not behind a building). */
	bool IsSeen(const FVector& At) const;
	/** The ground under At (a downward trace), false when there is none. */
	bool GroundAt(const FVector2D& At, double& OutZ) const;
	/** Someone of ours standing on the ground at Ground, facing Yaw. */
	APedestrian* MakePerson(const FVector& Ground, double Yaw, const PersonKit::FOutfit& Outfit);
	void RemovePerson(APedestrian* Person);
	/** A shape of ours in the world at Place (its own mesh, painted per section). */
	UProceduralMeshComponent* MakeMesh(const PersonKit::FPiece& Shape, const FTransform& Place);
	void RemoveMesh(UProceduralMeshComponent* Mesh);
	/** A sound of ours (CitySynth), at At. */
	USpotSound* MakeSound(CitySynth::ESpot Kind, const FVector& At);
	/** How loud it reaches the camera, Reach far at most. */
	void Hear(USpotSound* Sound, double Reach) const;
	/** A line for the player to read, if nobody else is talking. */
	void Say(const FString& Who, const FString& Hu, const FString& En) const;
	/** The hour (ADayNight), how dark it is, and how hard it rains (AWeather). */
	float HourNow() const;
	float DarkNow() const;
	float RainNow() const;
	/** A number of the game day, so each day picks its own places. */
	int32 DayNumber() const;
	/** The same 0..1 for the same day, kind and place. */
	float DayChance(int32 Kind, int32 Index) const;
	/** The traffic, found once. */
	ATrafficManager* Traffic() const;

	FRandomStream Dice;
	/** This game's own pick of places, so each play differs. */
	uint32 DaySeed = 0;
	/** Days counted from the start (midnights passed), and the hour last frame. */
	int32 Days = 0;
	float HourBefore = -1.f;
	/** Where the player is (his pawn or car), and roughly how high the ground under him is. */
	FVector PlayerAt = FVector::ZeroVector;
	double GroundGuess = 0.0;
	double Now = 0.0;
	FVector Eye = FVector::ZeroVector;
	FVector Look = FVector::ForwardVector;
	bool bHasCamera = false;
	mutable TWeakObjectPtr<ATrafficManager> TrafficFound;

	// ---- Places read from the map's files ----------------------------------

	struct FShopFront
	{
		FVector2D Foot = FVector2D::ZeroVector;
		double Yaw = 0.0;
		double Width = 0.0;
		uint32 Seed = 0;
		bool bClosed = false;
		double Ground = 0.0;
		bool bGroundKnown = false;
	};
	struct FYardThing
	{
		FString Kind;
		FVector2D At = FVector2D::ZeroVector;
		double Yaw = 0.0;
		double Length = 0.0;
	};
	struct FCourtyard
	{
		FVector2D Gate = FVector2D::ZeroVector;
		double GateYaw = 0.0;
		double Depth = 0.0;
		TArray<FVector2D> Outline;
		FVector2D Middle = FVector2D::ZeroVector;
		TArray<FYardThing> Things;
	};
	TArray<FShopFront> Shops;
	TArray<FCourtyard> Yards;
	TArray<FVector> Kiosks;
	/** Each church's door (the middle of its front, a little out) and Kerepesi's main gate. */
	TArray<FVector2D> ChurchDoors;
	FVector2D CemeteryGate = FVector2D::ZeroVector;
	bool bHasCemetery = false;
	/** Where people wait at each tram stop. */
	TArray<FVector2D> TramWaits;
	void LoadPlaces();
	static bool IsInside(const TArray<FVector2D>& Outline, const FVector2D& At);

	// ---- The lamplighter ---------------------------------------------------

	enum class ERound : uint8 { Waiting, Working, Done };
	ERound Round = ERound::Waiting;
	struct FRoundLamp
	{
		int32 Lamp = INDEX_NONE;
		FVector2D Stand = FVector2D::ZeroVector;
		double FaceYaw = 0.0;
		bool bLit = false;
		TWeakObjectPtr<UStaticMeshComponent> Cover;
	};
	TArray<FRoundLamp> RoundLamps;
	TWeakObjectPtr<APedestrian> Lamplighter;
	TWeakObjectPtr<UProceduralMeshComponent> LampPole;
	int32 RoundNext = 0;
	/** At a lamp: seconds there so far (negative while still walking to it). */
	float AtLampTime = -1.f;
	FVector2D LighterAt = FVector2D::ZeroVector;
	double LighterGround = 0.0;
	double LighterYaw = 0.0;
	/** Which way he last walked (he goes on that way after his round), when he looks at the ground next, how long he has been leaving. */
	double WalkYaw = 0.0;
	float LighterGroundClock = 0.f;
	float LeaveClock = 0.f;
	bool bGreeted = false;
	FVector2D RoundMiddle = FVector2D::ZeroVector;
	double NextRoundTry = 0.0;
	double RoundStarted = 0.0;
	void TickLamplighter(float DeltaSeconds);
	bool StartRound();
	/** Lights every lamp still waiting and sends him off. */
	void EndRound(bool bAtOnce);

	// ---- Courtyard life ----------------------------------------------------

	enum class EYardPlay : uint8 { None, Carpet, Washing, Football };
	struct FYardScene
	{
		int32 Yard = INDEX_NONE;
		EYardPlay Play = EYardPlay::None;
		TArray<TWeakObjectPtr<APedestrian>> Folk;
		TArray<TWeakObjectPtr<UProceduralMeshComponent>> Things;
		/** Folk's spots and facings, and where each is now and faces now. */
		TArray<FVector2D> Spots;
		TArray<double> Facings;
		TArray<FVector2D> FolkAt;
		TArray<double> FolkYaw;
		TArray<PersonKit::FOutfit> Looks;
		double Ground = 0.0;
		float Clock = 0.f;
		// Carpet: the beater in her hand, the dust.
		TWeakObjectPtr<UProceduralMeshComponent> Beater;
		TWeakObjectPtr<UProceduralMeshComponent> Dust;
		float DustAge = 10.f;
		float LastBeat = 0.f;
		// Washing: along the line, sheets pegged so far.
		FVector2D LineFrom = FVector2D::ZeroVector;
		FVector2D LineTo = FVector2D::ZeroVector;
		double LineYaw = 0.0;
		int32 Pegged = 0;
		float PegClock = 0.f;
		// Football: the ball, where it goes, who has it.
		TWeakObjectPtr<UProceduralMeshComponent> Ball;
		FVector2D BallAt = FVector2D::ZeroVector;
		FVector2D BallVelocity = FVector2D::ZeroVector;
		float BallHop = 0.f;
		float BallRise = 0.f;
		float BallSpin = 0.f;
		bool bKicked = false;
		int32 Holder = INDEX_NONE;
		int32 Receiver = INDEX_NONE;
		float KickClock = -1.f;
		float HoldClock = 0.f;
		double NextShout = 0.0;
	};
	TArray<FYardScene> YardScenes;
	double NextYardLook = 0.0;
	void TickCourtyards(float DeltaSeconds);
	bool StartYardScene(int32 YardIndex);
	void EndYardScene(FYardScene& Scene);
	void TickCarpet(FYardScene& Scene, float DeltaSeconds);
	void TickWashing(FYardScene& Scene, float DeltaSeconds);
	void TickFootball(FYardScene& Scene, float DeltaSeconds);

	// ---- Street people -----------------------------------------------------

	enum class ESeller : uint8 { Organ, Grinder, Chestnut, Count };
	struct FSeller
	{
		ESeller Kind = ESeller::Organ;
		bool bActive = false;
		int32 Place = INDEX_NONE;
		FVector At = FVector::ZeroVector;
		double Yaw = 0.0;
		TWeakObjectPtr<APedestrian> Person;
		TWeakObjectPtr<UProceduralMeshComponent> Stand;
		TWeakObjectPtr<UProceduralMeshComponent> Turning;
		TWeakObjectPtr<UProceduralMeshComponent> Held;
		TWeakObjectPtr<UProceduralMeshComponent> Sparks;
		TWeakObjectPtr<USpotSound> Sound;
		TWeakObjectPtr<UPointLightComponent> Glow;
		float Clock = 0.f;
		double NextTry = 0.0;
		double NextShout = 0.0;
	};
	FSeller Sellers[3];
	void TickSellers(float DeltaSeconds);
	bool StartSeller(FSeller& Seller);
	void EndSeller(FSeller& Seller);
	void PoseSeller(FSeller& Seller, float DeltaSeconds);
	/** Whether this kind works at this hour. */
	bool SellerHours(ESeller Kind, float Hour) const;

	// ---- Rare events -------------------------------------------------------

	enum class EEvent : uint8 { None, Wedding, Funeral, Fire };
	EEvent Event = EEvent::None;
	double NextEvent = 0.0;
	double EventStarted = 0.0;
	/** Over (arrived, too far, too long): it goes once nobody sees it. */
	bool bEventEnding = false;
	double EndingSince = 0.0;
	double AngrySince = 0.0;
	/** How fast the column walks this frame (cm/s). */
	float ColumnSpeed = 0.f;
	/** True while anyone or anything of the event can be seen. */
	bool IsEventSeen() const;
	void BeginEnding();
	/** The walkers of a wedding or a funeral, in order from the front, each a place in the column. */
	struct FMarcher
	{
		TWeakObjectPtr<APedestrian> Person;
		double Behind = 0.0;
		double Side = 0.0;
		double Ground = 0.0;
		float GroundClock = 0.f;
		bool bGone = false;
		/** What he does: 0 walk, 1 fiddle, 2 carry before him (bouquet, wreath). */
		int32 Doing = 0;
		TWeakObjectPtr<UProceduralMeshComponent> Held;
	};
	TArray<FMarcher> Marchers;
	TArray<FVector> EventWay;
	TArray<double> EventAlong;
	double EventS = 0.0;
	TWeakObjectPtr<ATrafficCart> Hearse;
	/** The hearse's trail, newest last, for the mourners to walk along. */
	TArray<FVector> Trail;
	TWeakObjectPtr<USpotSound> EventSound;
	double NextEventShout = 0.0;
	bool bMournersAngry = false;
	void TickEvents(float DeltaSeconds);
	bool StartEvent(EEvent Kind);
	void EndEvent();
	/**
	 * A spot on the lanes RangeFrom..RangeTo from the player, out of sight:
	 * the lane, its point and where it is. With Toward, only spots whose lane
	 * heads that way and from where the way there passes by the player.
	 */
	bool PickRoadStart(double RangeFrom, double RangeTo, const FVector2D* Toward, int32& OutLane, int32& OutPoint, FVector& OutAt);
	bool StartWedding();
	bool StartFuneral();
	bool StartFire();
	void TickMarchers(float DeltaSeconds);
	/** The point S along a path, and the path's heading there. */
	static FVector PathPoint(const TArray<FVector>& Path, const TArray<double>& Along, double S, FVector& OutDir);
	static void Measure(const TArray<FVector>& Path, TArray<double>& OutAlong);

	// ---- Trams and shops ---------------------------------------------------

	struct FRider
	{
		TWeakObjectPtr<APedestrian> Person;
		TWeakObjectPtr<ATram> Tram;
		/** Getting on (walks to the door and sits down), or off (walks away from it). */
		bool bBoarding = false;
		PersonKit::FOutfit Outfit;
		TArray<FVector2D> Path;
		int32 Next = 0;
		double Ground = 0.0;
		float GroundClock = 0.f;
		float Speed = 140.f;
		/** The tram stop's waiting spot, and when he came. */
		FVector2D Wait = FVector2D::ZeroVector;
		double Started = 0.0;
	};
	TArray<FRider> Riders;
	/** The stop each tram was last seen at (so each stop is done once). */
	TMap<TWeakObjectPtr<ATram>, int32> TramStopsDone;
	double NextTramLook = 0.0;
	void TickTrams(float DeltaSeconds);
	void TickRiders(float DeltaSeconds);

	struct FShutter
	{
		int32 Shop = INDEX_NONE;
		TWeakObjectPtr<UProceduralMeshComponent> Panel;
		TWeakObjectPtr<UProceduralMeshComponent> Roller;
		float Lift = 0.f;
		bool bOpening = false;
		TWeakObjectPtr<APedestrian> Keeper;
		TWeakObjectPtr<UProceduralMeshComponent> Hook;
		float Clock = 0.f;
		/** The keeper: where he is, where he came from, and what he does (0 coming, 1 pushing it up, 2 going in). */
		FVector2D KeeperAt = FVector2D::ZeroVector;
		int32 KeeperStep = 0;
	};
	TArray<FShutter> Shutters;
	double NextShopLook = 0.0;
	void TickShutters(float DeltaSeconds);
	/** The shop's front on the ground: its foot, with the height of the pavement. */
	bool ShopFoot(FShopFront& Shop, FVector& OutFoot);
	void RemoveShutter(FShutter& Shutter);
};
