// Fill out your copyright notice in the Description page of Project Settings.


#include "BreakableActor.h"
#include "GeometryCollection/GeometryCollectionComponent.h"

// Sets default values
ABreakableActor::ABreakableActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GeometryCollectionComponent = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("VoronoiFracturedMesh"));
	GeometryCollectionComponent->SetupAttachment(GetRootComponent());

	ProceduralMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ClusterResultMesh"));
	ProceduralMeshComponent->SetupAttachment(GetRootComponent());

	ProceduralMeshMaterial.SetNum(12);
}

// Called when the game starts or when spawned
void ABreakableActor::BeginPlay()
{
	Super::BeginPlay();
	CalculateBoneCenters();
}

// Called every frame
void ABreakableActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (isClustered)
	{
		UE_LOG(LogTemp, Warning, TEXT("Start creating pieces"));
		isClustered = false;
		CreateMeshForBoneIndex();
	}
}

void ABreakableActor::CalculateBoneCenters()
{
	if (!GeometryCollectionComponent || !GeometryCollectionComponent->GetRestCollection())
	{
		UE_LOG(LogTemp, Warning, TEXT("GeometryCollectionComponent or RestCollection is invalid"));
		return;
	}

	const UGeometryCollection* RestCollection = GeometryCollectionComponent->GetRestCollection();
	const FGeometryCollection* GeometryCollection = RestCollection->GetGeometryCollection().Get();
	if (!GeometryCollection)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to get GeometryCollection"));
		return;
	}

	// Get vertices group
	const TManagedArray<FVector3f>& VertexArray = GeometryCollection->GetAttribute<FVector3f>("Vertex", FGeometryCollection::VerticesGroup);
	const TManagedArray<int32>& BoneMapArray = GeometryCollection->GetAttribute<int32>("BoneMap", FGeometryCollection::VerticesGroup);
	if (VertexArray.Num() == 0 || BoneMapArray.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("VertexArray or BoneMapArray is empty"));
		return;
	}

	const int32 NumOfBones = GeometryCollection->NumElements(FGeometryCollection::TransformGroup);
	UE_LOG(LogTemp, Warning, TEXT("Number of unique bones: %d"), NumOfBones - 1);

	// store the sum & count of vertices per bone
	TArray<FVector3f> BoneVertexSums;
	TArray<int32> BoneVertexCounts;
	BoneVertexSums.SetNumZeroed(NumOfBones);
	BoneVertexCounts.SetNumZeroed(NumOfBones);

	for (int32 i = 0; i < VertexArray.Num(); ++i)
	{
		FVector3f Vertex = VertexArray[i];
		int32 BoneIndex = BoneMapArray[i];

		BoneVertexSums[BoneIndex] += Vertex;
		BoneVertexCounts[BoneIndex]++;

		// UE_LOG(LogTemp, Warning, TEXT("Vertex %d: Position = (%f, %f, %f), BoneIndex = %d"), i, Vertex.X, Vertex.Y, Vertex.Z, BoneIndex);
	}

	// Calculate center of mass for each bone
	for (int32 BoneIdx = 1; BoneIdx < NumOfBones; ++BoneIdx)	// 0th bone is not a piece. root of pieces
	{
		FVector3f SumOfVertices = BoneVertexSums[BoneIdx];
		int32 VertexCount = BoneVertexCounts[BoneIdx];
		if (VertexCount)
		{
			FVector3f CenterOfMass = SumOfVertices / VertexCount;
			PieceLocArr.Add(FVector(CenterOfMass.X, CenterOfMass.Y, CenterOfMass.Z));
			//UE_LOG(LogTemp, Log, TEXT("Bone %d: Center of Mass = (%f, %f, %f)"), BoneIdx, CenterOfMass.X, CenterOfMass.Y, CenterOfMass.Z);

			DrawDebugSphere(
				GetWorld(),
				FVector(CenterOfMass.X, CenterOfMass.Y, CenterOfMass.Z),
				2.0f,		 // Sphere radius
				12,          // Number of segments
				FColor::Red, // Sphere color
				false,       // Persistent (false = only for a single frame)
				1.0f         // Lifetime (1 second)
			);
		}
	}
}

void ABreakableActor::DebugSocketInfo()
{
	// set pivot to each pieces
	auto pieceNameArr = GeometryCollectionComponent->GetAllSocketNames();

	uint16 numberOfPieces = pieceNameArr.Num() - 1;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Purple, FString::Printf(TEXT("Number of pieces: %d"), numberOfPieces));

	for (int i = 0; i < numberOfPieces; i++) {
		FVector CenterOfMass = GeometryCollectionComponent->GetSocketLocation(pieceNameArr[i]);
		// PieceLocArr.Add(CenterOfMass);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("%f %f %f"), CenterOfMass.X, CenterOfMass.Y, CenterOfMass.Z));
	}
}

void ABreakableActor::CreateMeshForBoneIndex()
{
	// Validation check
	if (!GeometryCollectionComponent || !GeometryCollectionComponent->GetRestCollection())
	{
		UE_LOG(LogTemp, Warning, TEXT("GeometryCollectionComponent or RestCollection is invalid"));
		return;
	}
	UGeometryCollection* RestCollection = const_cast<UGeometryCollection*>(GeometryCollectionComponent->GetRestCollection());
	FGeometryCollection* GeometryCollection = RestCollection->GetGeometryCollection().Get();
	if (!GeometryCollection)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to get GeometryCollection"));
		return;
	}

	// Get VerticesGroup & FacesGroup data
	const TManagedArray<FVector3f>& VertexArray = GeometryCollection->GetAttribute<FVector3f>("Vertex", FGeometryCollection::VerticesGroup);
	const TManagedArray<int32>& BoneMapArray = GeometryCollection->GetAttribute<int32>("BoneMap", FGeometryCollection::VerticesGroup);
	const TManagedArray<FVector3f>& NormalArray = GeometryCollection->GetAttribute<FVector3f>("Normal", FGeometryCollection::VerticesGroup);
	/*for (int32 i = 0; i < VertexArray.Num(); ++i)
	{
		FVector3f Vertex = VertexArray[i];
		int32 BoneIndex = BoneMapArray[i];
		UE_LOG(LogTemp, Warning, TEXT("Vertex %d: Position = (%f, %f, %f), BoneIndex = %d"), i, Vertex.X, Vertex.Y, Vertex.Z, BoneIndex);
	}*/
	const TManagedArray<FIntVector>& IndicesArray = GeometryCollection->GetAttribute<FIntVector>("Indices", FGeometryCollection::FacesGroup);
	/*for (int32 i = 0; i < IndicesArray.Num(); ++i)
	{
		FIntVector Indices = IndicesArray[i];
		UE_LOG(LogTemp, Log, TEXT("Face %d: Indices = (%d, %d, %d)"), i, Indices.X, Indices.Y, Indices.Z);
	}*/

	const int32 NumOfBones = GeometryCollection->NumElements(FGeometryCollection::TransformGroup);
	UE_LOG(LogTemp, Warning, TEXT("Before) Number of unique bones: %d"), NumOfBones - 1);

	// Check validation for Clustered Index
	if (ClusteredIndex.Num() != NumOfBones - 1)
	{
		UE_LOG(LogTemp, Error, TEXT("ClusteredIndex array size does not match the number of bones"));
		return;
	}

	// Create section based on ClusteredIndex values
	TMap<int32, TArray<FVector>> SectionVerticesMap;
	TMap<int32, TArray<int32>> SectionIndicesMap;
	TMap<int32, TArray<FVector>> SectionNormalsMap;
	TMap<int32, int32> VertexMap;

	// Set offset for visualization
	FVector ActorLocation = GetActorLocation();
	FBox ActorBoundingBox = GetComponentsBoundingBox();
	FVector BoxExtent = ActorBoundingBox.GetExtent() * 2.0f;

	const FVector Offset(BoxExtent.X + 100.f, 0.f, ActorLocation.Z);

	// Group each triangle using its cluster index
	for (int32 i = 0; i < IndicesArray.Num(); ++i)
	{
		FIntVector Indices = IndicesArray[i];
		//UE_LOG(LogTemp, Log, TEXT("%d %d %d"), Indices.X, Indices.Y, Indices.Z);

		int32 BoneIndex = BoneMapArray[Indices.X];
		//UE_LOG(LogTemp, Log, TEXT("%d %d %d"), BoneIndex, BoneMapArray[Indices.Y], BoneMapArray[Indices.Z]);

		if (ClusteredIndex.IsValidIndex(BoneIndex - 1))
		{
			int32 ClusterIndex = ClusteredIndex[BoneIndex - 1];

			TArray<int32> FaceIndices = { Indices.X, Indices.Y, Indices.Z };
			FaceIndices.Sort();

			// Generate data for each section
			if (!SectionVerticesMap.Contains(ClusterIndex))
			{
				SectionVerticesMap.Add(ClusterIndex, TArray<FVector>());
				SectionIndicesMap.Add(ClusterIndex, TArray<int32>());
				SectionNormalsMap.Add(ClusterIndex, TArray<FVector>());
			}

			if (!VertexMap.Contains(Indices.X))
			{
				VertexMap.Add(Indices.X, SectionVerticesMap[ClusterIndex].Num());
				SectionVerticesMap[ClusterIndex].Add(FVector(VertexArray[Indices.X]) + Offset);
				SectionNormalsMap[ClusterIndex].Add(FVector(NormalArray[Indices.X]));
			}
			if (!VertexMap.Contains(Indices.Y))
			{
				VertexMap.Add(Indices.Y, SectionVerticesMap[ClusterIndex].Num());
				SectionVerticesMap[ClusterIndex].Add(FVector(VertexArray[Indices.Y]) + Offset);
				SectionNormalsMap[ClusterIndex].Add(FVector(NormalArray[Indices.Y]));
			}
			if (!VertexMap.Contains(Indices.Z))
			{
				VertexMap.Add(Indices.Z, SectionVerticesMap[ClusterIndex].Num());
				SectionVerticesMap[ClusterIndex].Add(FVector(VertexArray[Indices.Z]) + Offset);
				SectionNormalsMap[ClusterIndex].Add(FVector(NormalArray[Indices.Z]));
			}

			SectionIndicesMap[ClusterIndex].Add(VertexMap[Indices.X]);
			SectionIndicesMap[ClusterIndex].Add(VertexMap[Indices.Y]);
			SectionIndicesMap[ClusterIndex].Add(VertexMap[Indices.Z]);
		}
		else if (BoneIndex != 0) 
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid index for the given ClusteredIndex"));
		}
	}

	// Generate mesh for each section
	for (auto& Section : SectionVerticesMap)
	{
		int32 SectionIndex = Section.Key;
		const TArray<FVector>& SelectedVertices = Section.Value;
		const TArray<int32>& SelectedIndices = SectionIndicesMap[SectionIndex];
		const TArray<FVector>& SelectedNormals = SectionNormalsMap[SectionIndex];

		if (SelectedVertices.Num() > 0 && SelectedIndices.Num() > 0)
		{
			// Random vertex colors
			FLinearColor RandomColor = FLinearColor::MakeRandomColor();
			TArray<FLinearColor> VertexColors;
			VertexColors.Init(RandomColor, SelectedVertices.Num());
			//UE_LOG(LogTemp, Warning, TEXT("Random Color: %f %f %f"), RandomColor.R, RandomColor.G, RandomColor.B);

			ProceduralMeshComponent->CreateMeshSection_LinearColor(
				SectionIndex,				// Section index
				SelectedVertices,			// Vertex array
				SelectedIndices,			// Triangle index array
				SelectedNormals,			// Normal vector array (opt)
				TArray<FVector2D>(),		// UV0 - Texture coordinate (opt)
				VertexColors,				// Vertex color array (opt)
				TArray<FProcMeshTangent>(), // Tangent vector array (opt)
				true						// Whether to create collision
			);

			if (ProceduralMeshMaterial.Num() < 12)
			{
				UE_LOG(LogTemp, Error, TEXT("No material !!!"));
				return;
			}
			//ProceduralMeshComponent->SetMaterial(SectionIndex, ProceduralMeshMaterial[SectionIndex]);
			//PieceMaterial = GeometryCollectionComponent->GetMaterial(0);
			UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(PieceMaterial, this);
			DynamicMaterial->SetVectorParameterValue(TEXT("Param"), RandomColor);
			ProceduralMeshComponent->SetMaterial(SectionIndex, DynamicMaterial);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("No vertices or indices found for SectionIndex %d"), SectionIndex);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("After) Number of unique bones: %d"), SectionVerticesMap.Num());

}

void ABreakableActor::RemoveFaces()
{
	/*for (int32 i = 0; i < IndicesArray.Num(); ++i)
	{
		FIntVector Indices = IndicesArray[i];
		int32 BoneIndex = BoneMapArray[Indices.X];

		if (ClusteredIndex.IsValidIndex(BoneIndex - 1))
		{
			int32 ClusterIndex = ClusteredIndex[BoneIndex - 1];

			int32 vtxidx_x = Indices.X;
			int32 vtxidx_y = Indices.Y;
			int32 vtxidx_z = Indices.Z;

			while (true)
			{
				if (vtxidx_x == 0)	break;
				if (VertexArray[vtxidx_x].X != VertexArray[vtxidx_x - 1].X
					|| VertexArray[vtxidx_x].Y != VertexArray[vtxidx_x - 1].Y
					|| VertexArray[vtxidx_x].Z != VertexArray[vtxidx_x - 1].Z)
					break;

				vtxidx_x--;
			}
			while (true)
			{
				if (vtxidx_y == 0)	break;
				if (VertexArray[vtxidx_y].X != VertexArray[vtxidx_y - 1].X
					|| VertexArray[vtxidx_y].Y != VertexArray[vtxidx_y - 1].Y
					|| VertexArray[vtxidx_y].Z != VertexArray[vtxidx_y - 1].Z)
					break;

				vtxidx_y--;
			}
			while (true)
			{
				if (vtxidx_z == 0)	break;
				if (VertexArray[vtxidx_z].X != VertexArray[vtxidx_z - 1].X
					|| VertexArray[vtxidx_z].Y != VertexArray[vtxidx_z - 1].Y
					|| VertexArray[vtxidx_z].Z != VertexArray[vtxidx_z - 1].Z)
					break;

				vtxidx_z--;
			}

			TArray<int32> CorrectionIndices = { vtxidx_x, vtxidx_y, vtxidx_z };
			CorrectionIndices.Sort();

			if (!UniqueFacesMap.Contains(CorrectionIndices))
			{
				UniqueFacesMap.Add(CorrectionIndices, 1);
			}
			else
			{
				UniqueFacesMap[CorrectionIndices]++;
				UE_LOG(LogTemp, Warning, TEXT("face cnt: %d"), UniqueFacesMap[CorrectionIndices]);
			}
		}
	}*/
}

void ABreakableActor::SetClusteredIndex(const TArray<int32>& NewClusteredIndex)
{
	ClusteredIndex = NewClusteredIndex;
}

void ABreakableActor::SetIsClustered()
{
	isClustered = true;
}

TArray<FVector> ABreakableActor::GetPieceLocArray() const
{
	return PieceLocArr;
}