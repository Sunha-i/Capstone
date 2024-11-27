// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "BreakableActor.h"
#include "ClusterResultActor.generated.h"

UCLASS()
class CAPSTONE_API AClusterResultActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AClusterResultActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere)
	TArray<ABreakableActor*> BreakableActorArr;

	UFUNCTION(BlueprintCallable)
	void ProcessAllActors();

};
