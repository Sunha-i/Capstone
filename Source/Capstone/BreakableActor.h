// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "BreakableActor.generated.h"

class UGeometryCollectionComponent;

UCLASS()
class CAPSTONE_API ABreakableActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABreakableActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void CalculateBoneCenters();
	void DebugSocketInfo();
	void CreateMeshForBoneIndex();

	UFUNCTION(BlueprintCallable, Category = "GeomCollectMesh")
	void SetClusteredIndex(const TArray<int32>& NewClusteredIndex);
	void SetIsClustered();

	FString CreateVertexKey(const FVector& V1, const FVector& V2, const FVector& V3);
	void DFS(int32 CurrentKey, const TMap<int32, TArray<int32>>& InnerMap, TSet<int32>& Visited, 
			const TMap<FString, TArray<int32>>& FaceConnectedComb, const TArray<FVector>& SelectedVertices, 
			const TArray<int32>& SelectedIndices, TArray<int32>& CurrentGroup);

	TArray<FVector> GetPieceLocArray() const;

	UPROPERTY(EditAnywhere, Category = "ProceduralMesh")
	TArray<UMaterialInterface*> ProceduralMeshMaterial;

	UPROPERTY(EditAnywhere, Category = "ProceduralMesh")
	UMaterialInterface* PieceMaterial;		// M_Color

private:
	UPROPERTY(VisibleAnywhere)
	UGeometryCollectionComponent* GeometryCollectionComponent;

	UPROPERTY(VisibleAnywhere)
	UProceduralMeshComponent* ProceduralMeshComponent;
	
	TArray<FVector> PieceLocArr;	// center of mass
	
	UPROPERTY(EditAnywhere, Category = "GeomCollectMesh")
	TArray<int32> ClusteredIndex;

	bool isClustered = false;
};
