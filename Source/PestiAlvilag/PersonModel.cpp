// Fill out your copyright notice in the Description page of Project Settings.

#include "PersonModel.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

namespace PersonModel
{
	using namespace PersonKit;

	// Autumn 1932: browns, greys, navy, black, loden and a few light ones.
	const FLinearColor Coats[] = {
		FLinearColor(0.10f, 0.07f, 0.05f),
		FLinearColor(0.16f, 0.12f, 0.08f),
		FLinearColor(0.12f, 0.12f, 0.12f),
		FLinearColor(0.05f, 0.06f, 0.10f),
		FLinearColor(0.03f, 0.03f, 0.03f),
		FLinearColor(0.10f, 0.12f, 0.07f),
		FLinearColor(0.35f, 0.30f, 0.22f),
		FLinearColor(0.22f, 0.10f, 0.08f),
		FLinearColor(0.20f, 0.20f, 0.22f),
	};
	const FLinearColor Dresses[] = {
		FLinearColor(0.30f, 0.08f, 0.10f),
		FLinearColor(0.08f, 0.12f, 0.25f),
		FLinearColor(0.12f, 0.22f, 0.14f),
		FLinearColor(0.35f, 0.28f, 0.18f),
		FLinearColor(0.25f, 0.22f, 0.30f),
		FLinearColor(0.06f, 0.06f, 0.06f),
	};
	const FLinearColor Hats[] = {
		FLinearColor(0.04f, 0.04f, 0.04f),
		FLinearColor(0.20f, 0.17f, 0.13f),
		FLinearColor(0.12f, 0.10f, 0.08f),
		FLinearColor(0.18f, 0.18f, 0.18f),
	};
	const FLinearColor Cloths[] = {
		FLinearColor(0.25f, 0.22f, 0.17f),
		FLinearColor(0.18f, 0.18f, 0.18f),
		FLinearColor(0.20f, 0.15f, 0.10f),
	};
	const FLinearColor Trousers[] = {
		FLinearColor(0.06f, 0.06f, 0.07f),
		FLinearColor(0.10f, 0.09f, 0.08f),
		FLinearColor(0.14f, 0.14f, 0.15f),
		FLinearColor(0.18f, 0.15f, 0.11f),
	};
	const FLinearColor Shirts[] = {
		FLinearColor(0.85f, 0.84f, 0.80f),
		FLinearColor(0.75f, 0.78f, 0.82f),
		FLinearColor(0.80f, 0.76f, 0.66f),
	};
	const FLinearColor Ties[] = {
		FLinearColor(0.30f, 0.06f, 0.05f),
		FLinearColor(0.05f, 0.08f, 0.22f),
		FLinearColor(0.08f, 0.18f, 0.10f),
		FLinearColor(0.03f, 0.03f, 0.03f),
		FLinearColor(0.35f, 0.25f, 0.10f),
	};
	const FLinearColor Scarves[] = {
		FLinearColor(0.45f, 0.10f, 0.08f),
		FLinearColor(0.75f, 0.70f, 0.55f),
		FLinearColor(0.20f, 0.25f, 0.35f),
		FLinearColor(0.30f, 0.30f, 0.30f),
	};
	const FLinearColor Hairs[] = {
		FLinearColor(0.03f, 0.025f, 0.02f),
		FLinearColor(0.10f, 0.07f, 0.05f),
		FLinearColor(0.25f, 0.16f, 0.08f),
		FLinearColor(0.45f, 0.35f, 0.20f),
		FLinearColor(0.40f, 0.40f, 0.38f),
	};
	const FLinearColor Skins[] = {
		FLinearColor(0.72f, 0.50f, 0.38f),
		FLinearColor(0.78f, 0.58f, 0.46f),
		FLinearColor(0.65f, 0.45f, 0.33f),
	};
	const FLinearColor Leathers[] = {
		FLinearColor(0.25f, 0.13f, 0.06f),
		FLinearColor(0.08f, 0.05f, 0.03f),
		FLinearColor(0.45f, 0.32f, 0.15f),
	};

	template <SIZE_T N>
	const FLinearColor& Pick(FRandomStream& Random, const FLinearColor (&Colors)[N])
	{
		return Colors[Random.RandRange(0, static_cast<int32>(N) - 1)];
	}

	void Dress(AActor* Owner, USceneComponent* Parent, UMaterialInterface* BaseMaterial, const FOutfit& Outfit,
		TArray<TObjectPtr<UProceduralMeshComponent>>& InOutPieces)
	{
		for (UProceduralMeshComponent* Old : InOutPieces)
		{
			if (Old)
			{
				Old->DestroyComponent();
			}
		}
		InOutPieces.Reset();
		if (!Owner || !Parent)
		{
			return;
		}

		FPerson Person;
		Build(Outfit, Person);
		for (int32 p = 0; p < NumPieces; ++p)
		{
			const FPiece& Piece = Person.Pieces[p];
			UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(Owner);
			// A shin hangs from its thigh's knee, a forearm from its elbow; everything else from the middle.
			const EPiece Hangs = ParentOf(p);
			if (Hangs != NumPieces && InOutPieces.IsValidIndex(Hangs) && InOutPieces[Hangs])
			{
				Mesh->SetupAttachment(InOutPieces[Hangs]);
				Mesh->SetRelativeLocation(Piece.Pivot - Person.Pieces[Hangs].Pivot);
			}
			else
			{
				Mesh->SetupAttachment(Parent);
				Mesh->SetRelativeLocation(Piece.Pivot - FVector(0.0, 0.0, MiddleHeight));
			}
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Mesh->SetCanEverAffectNavigation(false);
			for (int32 s = 0; s < Piece.Sections.Num(); ++s)
			{
				const FSection& Section = Piece.Sections[s];
				Mesh->CreateMeshSection_LinearColor(s, Section.Vertices, Section.Triangles, Section.Normals,
					TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
				if (BaseMaterial)
				{
					UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, Owner);
					Paint->SetVectorParameterValue(TEXT("Color"), Section.Color);
					Mesh->SetMaterial(s, Paint);
				}
			}
			Mesh->RegisterComponent();
			Owner->AddInstanceComponent(Mesh);
			InOutPieces.Add(Mesh);
		}
	}

	void PoseLegs(const TArray<TObjectPtr<UProceduralMeshComponent>>& Pieces, float Left, float Right, float LeftKnee, float RightKnee)
	{
		const float Pitches[] = { Left, Right, LeftKnee, RightKnee, Left * SkirtFollow, Right * SkirtFollow };
		const EPiece Parts[] = { LegLeft, LegRight, ShinLeft, ShinRight, SkirtLeft, SkirtRight };
		for (int32 i = 0; i < static_cast<int32>(UE_ARRAY_COUNT(Parts)); ++i)
		{
			if (Pieces.IsValidIndex(Parts[i]) && Pieces[Parts[i]])
			{
				Pieces[Parts[i]]->SetRelativeRotation(FRotator(Pitches[i], 0.f, 0.f));
			}
		}
	}

	void StrideLegs(const TArray<TObjectPtr<UProceduralMeshComponent>>& Pieces, float Phase, float Swing, float KneeAmount)
	{
		const float Thigh = FMath::Sin(Phase) * Swing;
		// The leg swinging forward bends most; the knee comes straight as the foot lands.
		PoseLegs(Pieces, Thigh, -Thigh, KneeBend(Phase, KneeAmount), KneeBend(Phase + PI, KneeAmount));
	}

	void PoseElbows(const TArray<TObjectPtr<UProceduralMeshComponent>>& Pieces, float Left, float Right)
	{
		const float Bends[] = { Left, Right };
		const EPiece Parts[] = { ForearmLeft, ForearmRight };
		for (int32 i = 0; i < 2; ++i)
		{
			if (Pieces.IsValidIndex(Parts[i]) && Pieces[Parts[i]])
			{
				Pieces[Parts[i]]->SetRelativeRotation(FRotator(Bends[i], 0.f, 0.f));
			}
		}
	}

	FOutfit RandomOutfit(FRandomStream& Random)
	{
		FOutfit O;
		O.bWoman = Random.FRand() < 0.35f;
		O.Skin = Pick(Random, Skins);
		O.Hair = Pick(Random, Hairs);
		O.Shirt = Pick(Random, Shirts);
		O.Trousers = Pick(Random, Trousers);
		O.Shoes = Random.FRand() < 0.7f ? FLinearColor(0.03f, 0.02f, 0.02f) : FLinearColor(0.15f, 0.08f, 0.04f);
		O.HatColor = Pick(Random, Hats);
		O.HatBand = Random.FRand() < 0.7f ? FLinearColor(0.02f, 0.02f, 0.02f) : Pick(Random, Ties);
		O.Tie = Pick(Random, Ties);
		O.Scarf = Pick(Random, Scarves);
		O.bScarf = Random.FRand() < 0.2f;
		O.HeldColor = Pick(Random, Leathers);
		const float Held = Random.FRand();

		if (O.bWoman)
		{
			O.Height = Random.FRandRange(155.f, 170.f);
			O.Build = Random.FRandRange(0.95f, 1.12f);
			O.Hat = Random.FRand() < 0.75f ? EHat::Cloche : EHat::None;
			O.HatColor = Random.FRand() < 0.5f ? Pick(Random, Hats) : Pick(Random, Dresses);
			O.bTie = false;
			if (Random.FRand() < 0.5f)
			{
				O.Top = ETop::Overcoat;
				O.Coat = Pick(Random, Coats);
				O.Trousers = Pick(Random, Dresses);
			}
			else
			{
				O.Top = ETop::Dress;
				O.Coat = Pick(Random, Dresses);
			}
			O.Held = Held < 0.25f ? EHeld::Basket : Held < 0.35f ? EHeld::Umbrella : EHeld::None;
			return O;
		}

		O.Height = Random.FRandRange(165.f, 186.f);
		O.Build = Random.FRandRange(0.95f, 1.22f);
		O.bMoustache = Random.FRand() < 0.4f;
		const float Kind = Random.FRand();
		if (Kind < 0.2f)
		{
			// A working man: flat cap, waistcoat or work jacket, often boots.
			O.Hat = EHat::FlatCap;
			O.HatColor = Pick(Random, Cloths);
			O.Top = Random.FRand() < 0.5f ? ETop::Waistcoat : ETop::Jacket;
			O.Waistcoat = Pick(Random, Cloths);
			O.Coat = Pick(Random, Cloths);
			O.Trousers = Pick(Random, Cloths);
			O.bTie = Random.FRand() < 0.2f;
			O.bBoots = Random.FRand() < 0.6f;
			O.Held = Held < 0.15f ? EHeld::Newspaper : EHeld::None;
			return O;
		}
		O.Hat = Kind < 0.55f ? EHat::Fedora : Kind < 0.8f ? EHat::Bowler : Kind < 0.84f ? EHat::Boater : Kind < 0.9f ? EHat::FlatCap : EHat::None;
		O.Top = Random.FRand() < 0.6f ? ETop::Overcoat : ETop::Jacket;
		O.Coat = Pick(Random, Coats);
		O.bTie = Random.FRand() < 0.85f;
		O.Held = Held < 0.12f ? EHeld::Briefcase : Held < 0.2f ? EHeld::Umbrella : Held < 0.28f ? EHeld::Newspaper : EHeld::None;
		return O;
	}

	FOutfit PlayerOutfit()
	{
		FOutfit O;
		O.Height = 182.f;
		O.Hat = EHat::Fedora;
		O.Top = ETop::Overcoat;
		O.Coat = FLinearColor(0.16f, 0.14f, 0.11f);
		O.HatColor = FLinearColor(0.20f, 0.17f, 0.13f);
		O.bScarf = true;
		O.Scarf = FLinearColor(0.45f, 0.10f, 0.08f);
		return O;
	}
}
