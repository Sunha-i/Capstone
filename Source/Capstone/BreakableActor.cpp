// Fill out your copyright notice in the Description page of Project Settings.


#include "BreakableActor.h"
#include "GeometryCollection/GeometryCollectionComponent.h"

// Sets default values
ABreakableActor::ABreakableActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	GeometryCollectionComponent = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("GeometryCollection"));
	GeometryCollectionComponent->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void ABreakableActor::BeginPlay()
{
	Super::BeginPlay();
	
	auto pieceNameArr = GeometryCollectionComponent->GetAllSocketNames();

	uint16 numberOfPieces = pieceNameArr.Num() - 1;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Purple, FString::Printf(TEXT("Number of pieces: %d"), numberOfPieces));

	for (int i = 0; i < numberOfPieces; i++) {
		FVector CenterOfMass = GeometryCollectionComponent->GetSocketLocation(pieceNameArr[i]);
		PieceLocArr.Add(CenterOfMass);
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("%f %f %f"), CenterOfMass.X, CenterOfMass.Y, CenterOfMass.Z));
	}
}

TArray<FVector> ABreakableActor::GetPieceLocArray() const
{
	return PieceLocArr;
}
