// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Networking.h"
#include "Async/Async.h"
#include "Sockets.h"
#include "Kismet/GameplayStatics.h"
#include "BreakableActor.h"
#include "FractureNetworkActor.generated.h"

UCLASS()
class CAPSTONE_API AFractureNetworkActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFractureNetworkActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	bool IsConnectionOpen = false;
	bool WaitingForConnection = false;

	void OpenConnection();
	void CloseConnection();
	void ManageConnection();
	void ReceiveArrayMessages();

	FSocket* ListenSocket = NULL;
	FSocket* ConnectionSocket = NULL;

	TFuture<void> ClientConnectionFinishedFuture;
	TArray<ABreakableActor*> BreakableActorArr;
};
