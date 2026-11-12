// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StreetFurniture.generated.h"

class FJsonObject;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
struct FFurniturePart;

/**
 * The street furniture of the whole map, and the park on Tisza Kálmán tér,
 * read from the JSON that Tools/OsmGraybox/street_furniture.py writes:
 * period street lamps along the pavements (a slim pole with a scrolled
 * crook and a hanging bell lantern, after Fortepan 100450), round
 * advertising columns near street corners, benches in the parks, and the
 * 1932 Tisza Kálmán tér with gravel paths, flower beds, autumn trees,
 * benches, lamps and a low railing, leaf piles and the park keepers who
 * rake them, the statue east of the middle and the big playground in the
 * south-east quarter (ParkModel.h), and the taxi stands near the Népszínház and Keleti with
 * their parked taxis, and the 1930s shop fronts and house gates along
 * Népszínház utca (ShopModel.h), and Baross tér of the early 1930s in front
 * of Keleti: tree clumps, a flower bed, two-armed lamps, the tram islands
 * and trams waiting on the side track. It also starts the leaves blowing about (AFallingLeaves).
 *
 * Everything is drawn as instances of a few engine shapes, one batch per
 * shape and colour, so thousands of them cost little. A car driven fast into
 * a bench or a litter bin smashes it: its pieces fly (Wreck.h), and it
 * stands again a minute later at the earliest, once the camera is far away
 * and nobody sees. A street lamp hit hard tips over and falls across the
 * road, its lantern glass shatters and its light goes out until it stands
 * again the same way.
 */
UCLASS()
class PESTIALVILAG_API AStreetFurniture : public AActor
{
	GENERATED_BODY()

public:
	AStreetFurniture();

	/** The furniture file, relative to the project folder. */
	UPROPERTY(EditAnywhere, Category = "Furniture")
	FString DataFile = TEXT("Data/OsmGraybox/street_furniture.json");

	/** Small things are not drawn beyond this distance (cm). */
	UPROPERTY(EditAnywhere, Category = "Furniture")
	float SmallCullDistance = 9000.f;

	/** Lamps and columns are not drawn beyond this distance (cm). */
	UPROPERTY(EditAnywhere, Category = "Furniture")
	float CullDistance = 18000.f;

	/** Trees are not drawn beyond this distance (cm). */
	UPROPERTY(EditAnywhere, Category = "Furniture")
	float TreeCullDistance = 35000.f;

	virtual void Tick(float DeltaSeconds) override;

	/** The world's street furniture, or none. */
	static AStreetFurniture* Get(const UWorld* World);

	/**
	 * A dry spot to wait out the rain in, near At and no further than Reach:
	 * under a shop's awning, in a shop doorway or a house gate, kept for
	 * whoever takes it until LeaveShelter. Only on At's own side of the
	 * street: At no further than MaxOut in front of the wall. Gives where to
	 * stand (on the pavement) and which way to face (out at the street).
	 */
	bool TakeShelter(const FVector2D& At, double Reach, double MaxOut, FVector& OutSpot, float& OutYaw, int32& OutIndex);
	void LeaveShelter(int32 Index);

protected:
	virtual void BeginPlay() override;

	/** The pieces of smashed benches and bins. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Wreckage;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> ShapeMeshes;

	/** The shards of a lamp's glass. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GlassMaterial;

private:
	/** The instances waiting for the batch of this shape, colour, collision and draw distance (and which batch it is). */
	TArray<FTransform>& Batch(int32 Shape, const FLinearColor& Color, bool bCollide, float Cull, int32* OutBatch = nullptr);
	/** Puts one object made of Parts on the ground at At, turned to Yaw; a car can smash it if bBreakable. */
	void AddObject(const TArray<FFurniturePart>& Parts, const FVector& At, double Yaw, double Scale, float Cull, bool bBreakable = false);
	/** A flat piece lying on the ground from A to B. */
	void AddStrip(const FVector2D& A, const FVector2D& B, double Width, double Thickness, double Lift, const FLinearColor& Color);
	void AddRailing(const FVector2D& A, const FVector2D& B);
	/** A knee-high wire fence on little wooden posts along a lawn. */
	void AddLawnFence(const FVector2D& A, const FVector2D& B);
	/** A low clipped hedge from A to B. */
	void AddHedge(const FVector2D& A, const FVector2D& B);
	/** The ground under a map point; false where it is not open map ground. */
	bool GroundAt(const FVector2D& At, FVector& OutGround) const;

	void BuildStreets(const FJsonObject& Root);
	/**
	 * A park or square laid out in the JSON (tisza, baross): paths, flower
	 * beds, trees, benches, bins, lamps, railings, fences, hedges, a statue,
	 * a playground, leaf piles and their keepers, and waiting trams; each only
	 * where the JSON has it.
	 */
	void BuildPark(const FJsonObject& Park);
	/** The sand, things to play on, children and nannies of the playground in Tisza Kálmán tér. */
	void BuildPlayground(const FJsonObject& Playground);
	/** Taxi ranks by the kerb near the landmarks: parked taxis and a sign. */
	void BuildTaxiStands(const FJsonObject& Root);
	/** A stop sign by each tram and bus stop (tram_stops). */
	void BuildTramStops(const FJsonObject& Root);
	/** Street food sellers with their carts (food_vendors). */
	void BuildFoodVendors(const FJsonObject& Root);
	/** The Teleki tér market (an AFleaMarket). */
	void BuildMarket(const FJsonObject& Root);
	/** The shop fronts and tenement gates of Népszínház utca (shops, gates), with some shopkeepers (ShopModel.h). */
	void BuildShops(const FJsonObject& Root);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Batches;

	/** Where people can stand out of the rain (TakeShelter), and whether someone stands there. */
	struct FShelter
	{
		FVector Spot = FVector::ZeroVector;
		float Yaw = 0.f;
		bool bTaken = false;
	};
	TArray<FShelter> Shelters;
	/** Adds a shelter spot Out cm in front of a wall's foot and Across along it (the wall faces Yaw). */
	void AddShelter(const FVector& Foot, double Yaw, double Out, double Across);

	TMap<FString, int32> BatchIndex;
	TArray<TArray<FTransform>> Pending;

	/** Trees that still have leaves to lose: ground point and size. */
	TArray<FVector4> LeafyTrees;

	/** One shape of a breakable object: its batch, its instance there, and where it stands. */
	struct FBreakPart
	{
		int32 Batch = 0;
		int32 Instance = 0;
		int32 Shape = 0;
		FTransform Transform;
		/** A lamp's post and crook: they fall over whole (the lantern flies off). */
		bool bFalls = false;
		/** A lamp's glass: it shatters. */
		bool bGlass = false;
	};

	/** A bench or a litter bin a car can smash. */
	struct FBreakable
	{
		FVector At = FVector::ZeroVector;
		double Yaw = 0.0;
		/** Half its size along its own X and Y (cm). */
		FVector2D Half = FVector2D::ZeroVector;
		TArray<FBreakPart> Parts;
		/** Someone sits on it (a nanny at the playground): it stays whole. */
		bool bSturdy = false;
		bool bBroken = false;
		bool bSettled = false;
		double BrokenAt = 0.0;
		/** The pieces lying about; a lamp's falling post and crook come first. */
		TArray<UStaticMeshComponent*> Pieces;

		/** A street lamp: its lanterns' numbers in ADayNight. */
		TArray<int32> LampIds;
		/** A knocked lamp still tipping over, how far it has tipped (radians), how fast, towards where, and how far it turns about itself as it goes so its crook lands flat. */
		bool bFalling = false;
		double FallAngle = 0.0;
		double FallSpeed = 0.0;
		FVector FallDir = FVector::ForwardVector;
		double FallTwist = 0.0;
		/** How far over it ends up: flat on the road, or less where it leans on a wall. */
		double LieAngle = 0.0;
		/** Where the falling pieces sit on the standing lamp, from its foot. */
		TArray<FTransform> FallFrom;
	};

	void Smash(int32 Index, double Now);
	/** Knocks a lamp's post over and shatters its glass, as Smash's part of it. */
	void KnockLamp(FBreakable& Thing);
	/** Puts a falling lamp's post and crook where its tipping has got to. */
	void PoseFall(const FBreakable& Thing) const;
	void PutBack(FBreakable& Thing);

	TArray<FBreakable> Breakables;
	/** Breakables on a coarse grid, to find the ones near the car quickly. */
	TMultiMap<FIntPoint, int32> BreakableCells;
	/** The ones lying smashed. */
	TArray<int32> Smashed;
	/** Where the playground's grown-ups and children sit. */
	TArray<FVector> SeatedAt;
	FRandomStream WreckRandom;
	int32 NextReset = 0;

	/** The player (or the car he drives) and how fast it goes, worked out each frame. */
	TWeakObjectPtr<AActor> LastPlayer;
	FVector LastPlayerAt = FVector::ZeroVector;
	FVector PlayerVelocity = FVector::ZeroVector;
};
