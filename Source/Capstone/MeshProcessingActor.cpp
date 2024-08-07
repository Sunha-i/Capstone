// Fill out your copyright notice in the Description page of Project Settings.


#include "MeshProcessingActor.h"
#include "Engine/ObjectLibrary.h"

// Sets default values
AMeshProcessingActor::AMeshProcessingActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AMeshProcessingActor::BeginPlay()
{
	Super::BeginPlay();
	
	FString MeshFolderPath = TEXT("/Game/Destruction/GeometryCollection/pivot");
	LoadAllStaticMeshesFromPath(MeshFolderPath);

}

void AMeshProcessingActor::LoadAllStaticMeshesFromPath(const FString& Path)
{
    UObjectLibrary* ObjectLibrary = UObjectLibrary::CreateLibrary(UStaticMesh::StaticClass(), false, GIsEditor);
    ObjectLibrary->AddToRoot();

    ObjectLibrary->LoadAssetDataFromPath(Path);

    TArray<FAssetData> AssetDatas;
    ObjectLibrary->GetAssetDataList(AssetDatas);

    for (const FAssetData& AssetData : AssetDatas)
    {
        UStaticMesh* StaticMesh = Cast<UStaticMesh>(AssetData.GetAsset());
        if (StaticMesh)
        {
            LoadedMeshes.Add(StaticMesh);

            LogStaticMeshVertices(StaticMesh);
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("Total Loaded Static Meshes: %d"), LoadedMeshes.Num());

    ObjectLibrary->RemoveFromRoot();
}

void AMeshProcessingActor::LogStaticMeshVertices(UStaticMesh* StaticMesh)
{
    if (StaticMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("Loaded Static Mesh: %s"), *StaticMesh->GetName());

        FStaticMeshLODResources& LODResource = StaticMesh->GetRenderData()->LODResources[0];

        FPositionVertexBuffer* VertexBuffer = &LODResource.VertexBuffers.PositionVertexBuffer;

        for (uint32 VertexIndex = 0; VertexIndex < VertexBuffer->GetNumVertices(); ++VertexIndex)
        {
            FVector3f VertexPosition = VertexBuffer->VertexPosition(VertexIndex);
            FVector ConvertedPosition = FVector(VertexPosition);
            UE_LOG(LogTemp, Log, TEXT("Mesh: %s, Vertex %d: %s"), *StaticMesh->GetName(), VertexIndex, *ConvertedPosition.ToString());
        }

        FRawStaticIndexBuffer* IndexBuffer = &LODResource.IndexBuffer;

        uint32 NumIndices = IndexBuffer->GetNumIndices();
        for (uint32 idx = 0; idx < NumIndices; idx += 3)
        {
            int32 Index0 = IndexBuffer->GetIndex(idx);
            int32 Index1 = IndexBuffer->GetIndex(idx + 1);
            int32 Index2 = IndexBuffer->GetIndex(idx + 2);

            FVector Vertex0 = FVector(VertexBuffer->VertexPosition(Index0));
            FVector Vertex1 = FVector(VertexBuffer->VertexPosition(Index1));
            FVector Vertex2 = FVector(VertexBuffer->VertexPosition(Index2));

            UE_LOG(LogTemp, Log, TEXT("Mesh: %s, Face %d: [%d, %d, %d] -> Vertices: [%s, %s, %s]"),
                *StaticMesh->GetName(),
                idx / 3,
                Index0, Index1, Index2,
                *Vertex0.ToString(), *Vertex1.ToString(), *Vertex2.ToString());
        }
    }
}
