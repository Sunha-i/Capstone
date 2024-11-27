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

	/*ProceduralMeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
	ProceduralMeshComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	ProceduralMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ProceduralMeshComponent->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);

	ProceduralMeshComponent->bUseComplexAsSimpleCollision = false;
	ProceduralMeshComponent->SetSimulatePhysics(true);
	ProceduralMeshComponent->bAlwaysCreatePhysicsState = true;*/

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

			//DrawDebugSphere(
			//	GetWorld(),
			//	FVector(CenterOfMass.X, CenterOfMass.Y, CenterOfMass.Z),
			//	2.0f,		 // Sphere radius
			//	12,          // Number of segments
			//	FColor::Red, // Sphere color
			//	false,       // Persistent (false = only for a single frame)
			//	1.0f         // Lifetime (1 second)
			//);
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
		UE_LOG(LogTemp, Log, TEXT("BoneIndex: %d %d %d"), BoneMapArray[Indices.X], BoneMapArray[Indices.Y], BoneMapArray[Indices.Z]);
	}*/

	const int32 NumOfBones = GeometryCollection->NumElements(FGeometryCollection::TransformGroup);
	UE_LOG(LogTemp, Warning, TEXT("Before) Number of unique bones: %d"), NumOfBones - 1);
	UE_LOG(LogTemp, Warning, TEXT("VertexArray.Num(): %d, BoneMapArray.Num(): %d, NormalArray.Num(): %d, IndicesArray.Num(): %d"), VertexArray.Num(), BoneMapArray.Num(), NormalArray.Num(), IndicesArray.Num());

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

	TMap<FString, TArray<int32>> FaceConnectedComb;
	TMap<int32, TMap<int32, TArray<int32>>> OriginalSection;

	// Set offset for visualization
	FVector ActorLocation = GetActorLocation();
	FBox ActorBoundingBox = GetComponentsBoundingBox();
	FVector BoxExtent = ActorBoundingBox.GetExtent() * 2.0f;

	//const FVector Offset(BoxExtent.X + 100.f, 0.f, ActorLocation.Z);
	const FVector Offset = ActorLocation;	// Apply scale

	PieceMaterial = GeometryCollectionComponent->GetMaterial(0);

	if (GeometryCollectionComponent)
	{
		GeometryCollectionComponent->DestroyComponent();
		GeometryCollectionComponent = nullptr;
	}

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
				OriginalSection.Add(ClusterIndex, TMap<int32, TArray<int32>>());
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

			// Record the vertex index within the cluster's section
			if (!OriginalSection[ClusterIndex].Contains(BoneIndex - 1))
			{
				OriginalSection[ClusterIndex].Add(BoneIndex - 1, { SectionIndicesMap[ClusterIndex].Num() });
			}
			else
			{
				OriginalSection[ClusterIndex][BoneIndex - 1].Add(SectionIndicesMap[ClusterIndex].Num());
			}

			const FVector Vertex1 = FVector(VertexArray[FaceIndices[0]]) + Offset;
			const FVector Vertex2 = FVector(VertexArray[FaceIndices[1]]) + Offset;
			const FVector Vertex3 = FVector(VertexArray[FaceIndices[2]]) + Offset;

			FString VertexKey = CreateVertexKey(Vertex1, Vertex2, Vertex3);

			// Check if this combination is already in the hash map
			if (FaceConnectedComb.Contains(VertexKey))
			{
				//UE_LOG(LogTemp, Warning, TEXT("Duplicate vertex combination found at index %d"), i);
				FaceConnectedComb[VertexKey].Add(BoneIndex - 1);
			}
			else
			{
				FaceConnectedComb.Add(VertexKey, { BoneIndex - 1 });
			}
		}
		else if (BoneIndex != 0) 
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid index for the given ClusteredIndex"));
		}
	}

	int32 numberoftwo = 0;
	for (const TPair<FString, TArray<int32>>& Elem : FaceConnectedComb)
	{
		FString Key = Elem.Key;

		const TArray<int32>& Values = Elem.Value;
		if (Values.Num() > 2)
			numberoftwo++;
	}
	UE_LOG(LogTemp, Log, TEXT("it's over 2: %d"), numberoftwo);

	int32 tmpSectionIndex = 0;

	// Generate mesh for each section
	for (auto& Section : SectionVerticesMap)
	{
		int32 SectionIndex = Section.Key;
		const TArray<FVector>& SelectedVertices = Section.Value;
		const TArray<int32>& SelectedIndices = SectionIndicesMap[SectionIndex];
		const TArray<FVector>& SelectedNormals = SectionNormalsMap[SectionIndex];

		UE_LOG(LogTemp, Warning, TEXT("bonindex: %d"), OriginalSection[SectionIndex].Num());
		
		TSet<int32> Visited;

		const TMap<int32, TArray<int32>>& InnerMap = OriginalSection[SectionIndex];

		UE_LOG(LogTemp, Warning, TEXT("Number of pieces in Section %d: %d"), SectionIndex, InnerMap.Num());

		for (const TPair<int32, TArray<int32>>& InnerPair : InnerMap)
		{
			int32 InnerKey = InnerPair.Key;
			UE_LOG(LogTemp, Log, TEXT("InnerKey List : %d"), InnerKey);
		}

		for (const TPair<int32, TArray<int32>>& InnerPair : InnerMap)
		{
			int32 InnerKey = InnerPair.Key;

			if (!Visited.Contains(InnerKey))
			{
				UE_LOG(LogTemp, Warning, TEXT("Starting DFS for group at piece %d"), InnerKey);

				TArray<int32> CurrentGroup;
				DFS(InnerKey, InnerMap, Visited, FaceConnectedComb, SelectedVertices, SelectedIndices, CurrentGroup);

				TArray<int32> tmpSelectedIndices;
				UE_LOG(LogTemp, Log, TEXT("Group formed:"));
				for (int32 GroupElement : CurrentGroup)
				{
					UE_LOG(LogTemp, Log, TEXT("%d"), GroupElement);

					int32 tmp = SelectedIndices[GroupElement];

					const TArray<int32>& tmpArray = OriginalSection[SectionIndex][GroupElement];
					for (int32 tmpElement : tmpArray)
					{
						tmpSelectedIndices.Add(SectionIndicesMap[SectionIndex][tmpElement - 3]);
						tmpSelectedIndices.Add(SectionIndicesMap[SectionIndex][tmpElement - 2]);
						tmpSelectedIndices.Add(SectionIndicesMap[SectionIndex][tmpElement - 1]);
					}
				}

				UE_LOG(LogTemp, Warning, TEXT("Finished DFS for group starting at piece %d"), InnerKey);

				// Map for counting combination with repetition
				TMap<FString, int32> TriangleCombinationCount;

				// Count the number of duplicate triangle combinations
				for (int32 i = 0; i < tmpSelectedIndices.Num(); i += 3)
				{
					if (tmpSelectedIndices.IsValidIndex(i) &&
						tmpSelectedIndices.IsValidIndex(i + 1) &&
						tmpSelectedIndices.IsValidIndex(i + 2))
					{
						// Create a key based on the triangle's vertex combination
						const FVector& Vertex1 = SelectedVertices[tmpSelectedIndices[i]];
						const FVector& Vertex2 = SelectedVertices[tmpSelectedIndices[i + 1]];
						const FVector& Vertex3 = SelectedVertices[tmpSelectedIndices[i + 2]];

						FString TriangleKey = CreateVertexKey(Vertex1, Vertex2, Vertex3);

						// Increment the count
						if (TriangleCombinationCount.Contains(TriangleKey))
						{
							TriangleCombinationCount[TriangleKey]++;
						}
						else
						{
							TriangleCombinationCount.Add(TriangleKey, 1);
						}
					}
				}

				// Keep only combinations that occur once
				TArray<int32> FilteredIndices;
				for (int32 i = 0; i < tmpSelectedIndices.Num(); i += 3)
				{
					if (tmpSelectedIndices.IsValidIndex(i) &&
						tmpSelectedIndices.IsValidIndex(i + 1) &&
						tmpSelectedIndices.IsValidIndex(i + 2))
					{
						const FVector& Vertex1 = SelectedVertices[tmpSelectedIndices[i]];
						const FVector& Vertex2 = SelectedVertices[tmpSelectedIndices[i + 1]];
						const FVector& Vertex3 = SelectedVertices[tmpSelectedIndices[i + 2]];

						FString TriangleKey = CreateVertexKey(Vertex1, Vertex2, Vertex3);

						// Add only combinations with a count of 1
						if (TriangleCombinationCount.Contains(TriangleKey) && TriangleCombinationCount[TriangleKey] == 1)
						{
							FilteredIndices.Add(tmpSelectedIndices[i]);
							FilteredIndices.Add(tmpSelectedIndices[i + 1]);
							FilteredIndices.Add(tmpSelectedIndices[i + 2]);
						}
					}
				}

				// Replace tmpSelectedIndices with the filtered result
				tmpSelectedIndices = FilteredIndices;

				if (SelectedVertices.Num() > 0 && SelectedIndices.Num() > 0)
				{
					// Random vertex colors
					FLinearColor RandomColor = FLinearColor::MakeRandomColor();
					TArray<FLinearColor> VertexColors;
					VertexColors.Init(RandomColor, SelectedVertices.Num());
					UE_LOG(LogTemp, Warning, TEXT("Random Color: %f %f %f"), RandomColor.R, RandomColor.G, RandomColor.B);

					ProceduralMeshComponent->CreateMeshSection_LinearColor(
						tmpSectionIndex,				// Section index
						SelectedVertices,				// Vertex array
						tmpSelectedIndices,				// Triangle index array
						SelectedNormals,				// Normal vector array (opt)
						TArray<FVector2D>(),			// UV0 - Texture coordinate (opt)
						VertexColors,					// Vertex color array (opt)
						TArray<FProcMeshTangent>(),		// Tangent vector array (opt)
						true							// Whether to create collision
					);

					/* Dynamically ~ */
					//NewMeshComponent->AddCollisionConvexMesh(SelectedVertices);
					//NewMeshComponent->UpdateCollisionProfile();

					if (ProceduralMeshMaterial.Num() < 12)
					{
						UE_LOG(LogTemp, Error, TEXT("No material !!!"));
						return;
					}
				
					tmpSectionIndex++;
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("No vertices or indices found for SectionIndex %d"), SectionIndex);
				}
			}
		}
	}

	// Set Color
	for (int i = 0; i <= tmpSectionIndex; i++) {
		const float Saturation = 0.95f;
		const float Value = 1.0f;

		// Evenly distribute hue values across sections
		float Hue = (360.0f / tmpSectionIndex) * i;

		// HSV to RGB conversion
		FLinearColor newRandomColor = FLinearColor::MakeFromHSV8(
			static_cast<uint8>(Hue),
			static_cast<uint8>(Saturation * 255),
			static_cast<uint8>(Value * 255)
		);

		UE_LOG(LogTemp, Log, TEXT("Section %d Color Applied: R=%f, G=%f, B=%f"), i, newRandomColor.R, newRandomColor.G, newRandomColor.B);
		UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(PieceMaterial, this);
		DynamicMaterial->SetVectorParameterValue(TEXT("Param"), newRandomColor);
		ProceduralMeshComponent->SetMaterial(i, DynamicMaterial);
	}

	UE_LOG(LogTemp, Warning, TEXT("number of faces: %d"), FaceConnectedComb.Num());


	UE_LOG(LogTemp, Warning, TEXT("After) Number of unique bones: %d"), SectionVerticesMap.Num());

}

void ABreakableActor::DFS(int32 CurrentKey, const TMap<int32, TArray<int32>>& InnerMap, TSet<int32>& Visited, 
	const TMap<FString, TArray<int32>>& FaceConnectedComb, const TArray<FVector>& SelectedVertices, 
	const TArray<int32>& SelectedIndices, TArray<int32>& CurrentGroup)
{
	const TArray<int32>* InnerArrayPtr = InnerMap.Find(CurrentKey);
	if (!InnerArrayPtr)
	{
		//UE_LOG(LogTemp, Warning, TEXT("CurrentKey %d not found in InnerMap!"), CurrentKey);
		return;
	}
	const TArray<int32>& InnerArray = *InnerArrayPtr;

	Visited.Add(CurrentKey);
	CurrentGroup.Add(CurrentKey);

	UE_LOG(LogTemp, Log, TEXT("\tVertexArray.Num(): %d"), SelectedVertices.Num());
	UE_LOG(LogTemp, Log, TEXT("\tInnerMap[CurrentKey] : %d %d"), InnerMap[CurrentKey][0], InnerMap[CurrentKey][InnerMap[CurrentKey].Num()-1]);

	//UE_LOG(LogTemp, Warning, TEXT("InnerArray: %d %d"), InnerArray[0], InnerArray[InnerArray.Num()-1]);

	for (int32 ArrayValue : InnerMap[CurrentKey])
	{
		//UE_LOG(LogTemp, Log, TEXT("ArrayValue: %d"), ArrayValue);

		FVector v1 = SelectedVertices[SelectedIndices[ArrayValue - 3]];
		FVector v2 = SelectedVertices[SelectedIndices[ArrayValue - 2]];
		FVector v3 = SelectedVertices[SelectedIndices[ArrayValue - 1]];  

		FString VertexKey = CreateVertexKey(v1, v2, v3);

		if (FaceConnectedComb.Contains(VertexKey))
		{
			const TArray<int32>& ConnectedPieces = FaceConnectedComb[VertexKey];

			if (ConnectedPieces.Num() == 2)
			{
				for (int32 PieceIndex : ConnectedPieces)
				{
					if (PieceIndex != CurrentKey && !Visited.Contains(PieceIndex))
					{
						//UE_LOG(LogTemp, Warning, TEXT("target index : %d <-> connected index : %d"), CurrentKey, PieceIndex);

						DFS(PieceIndex, InnerMap, Visited, FaceConnectedComb, SelectedVertices, SelectedIndices, CurrentGroup);
					}
				}
			}
		}
	}
}

FString ABreakableActor::CreateVertexKey(const FVector& V1, const FVector& V2, const FVector& V3)
{
	TArray<FVector> Vertices = { V1, V2, V3 };

	// Sort the vertices so that their order doesn't matter
	Vertices.Sort([](const FVector& A, const FVector& B)
		{
			return A.X < B.X || (A.X == B.X && A.Y < B.Y) || (A.X == B.X && A.Y == B.Y && A.Z < B.Z);
		});

	// Sort the X, Y, Z components of each vertex to handle permutation of coordinates
	for (FVector& Vec : Vertices)
	{
		TArray<double> Components = { Vec.X, Vec.Y, Vec.Z };

		Components.Sort();
		Vec = FVector(Components[0], Components[1], Components[2]);
	}

	// Create a unique string key from the sorted vertices
	return FString::Printf(TEXT("%f,%f,%f-%f,%f,%f-%f,%f,%f"),
		Vertices[0].X, Vertices[0].Y, Vertices[0].Z,
		Vertices[1].X, Vertices[1].Y, Vertices[1].Z,
		Vertices[2].X, Vertices[2].Y, Vertices[2].Z);
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