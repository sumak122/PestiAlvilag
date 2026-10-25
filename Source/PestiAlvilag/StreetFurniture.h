// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StreetFurniture.generated.h"

class FJsonObject;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;
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
 * Népszínház utca (ShopModel.h). It also starts the leaves blowing about (AFallingLeaves).
 *
 * Everything is drawn as instances of a few engine shapes, one batch per
 * shape and colour, so thousands of them cost little.
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

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> ShapeMeshes;

private:
	/** The instances waiting for the batch of this shape, colour, collision and draw distance. */
	TArray<FTransform>& Batch(int32 Shape, const FLinearColor& Color, bool bCollide, float Cull);
	/** Puts one object made of Parts on the ground at At, turned to Yaw. */
	void AddObject(const TArray<FFurniturePart>& Parts, const FVector& At, double Yaw, double Scale, float Cull);
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
	void BuildTiszaKalmanTer(const FJsonObject& Park);
	/** The sand, things to play on, children and nannies of the playground in Tisza Kálmán tér. */
	void BuildPlayground(const FJsonObject& Playground);
	/** Taxi ranks by the kerb near the landmarks: parked taxis and a sign. */
	void BuildTaxiStands(const FJsonObject& Root);
	/** A stop sign by each tram stop (tram_stops). */
	void BuildTramStops(const FJsonObject& Root);
	/** Street food sellers with their carts (food_vendors). */
	void BuildFoodVendors(const FJsonObject& Root);
	/** The Teleki tér market (an AFleaMarket). */
	void BuildMarket(const FJsonObject& Root);
	/** The shop fronts and tenement gates of Népszínház utca (shops, gates), with some shopkeepers (ShopModel.h). */
	void BuildShops(const FJsonObject& Root);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Batches;

	TMap<FString, int32> BatchIndex;
	TArray<TArray<FTransform>> Pending;

	/** Trees that still have leaves to lose: ground point and size. */
	TArray<FVector4> LeafyTrees;
};
