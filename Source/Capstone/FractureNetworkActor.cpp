// Fill out your copyright notice in the Description page of Project Settings.


#include "FractureNetworkActor.h"

// Sets default values
AFractureNetworkActor::AFractureNetworkActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AFractureNetworkActor::BeginPlay()
{
	Super::BeginPlay();
	OpenConnection();

	TSubclassOf<ABreakableActor> classToFind;
	TArray<AActor*> find;
	classToFind = ABreakableActor::StaticClass();
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), classToFind, find);
	UE_LOG(LogTemp, Warning, TEXT("Get Actor: %d"), find.Num());

	ABreakableActor* caster = NULL;

	for (int i = 0; i < find.Num(); i++) {
		caster = Cast<ABreakableActor>(find[i]);
		BreakableActorArr.Add(caster);
	}
}

void AFractureNetworkActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	CloseConnection();
}

// Called every frame
void AFractureNetworkActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ManageConnection();
}

void AFractureNetworkActor::OpenConnection()
{
	if (!IsConnectionOpen)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TCP] Opening Connection"));
		IsConnectionOpen = true;
		WaitingForConnection = true;

		FIPv4Address IPAddress;
		FIPv4Address::Parse(FString("127.0.0.1"), IPAddress);
		FIPv4Endpoint Endpoint(IPAddress, (uint16)5050);

		ListenSocket = FTcpSocketBuilder(TEXT("TcpSocket")).AsReusable();

		ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
		ListenSocket->Bind(*SocketSubsystem->CreateInternetAddr(Endpoint.Address.Value, Endpoint.Port));
		ListenSocket->Listen(1);
		UE_LOG(LogTemp, Warning, TEXT("[TCP] Listening..."));
	}
}

void AFractureNetworkActor::CloseConnection()
{
	if (IsConnectionOpen)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TCP] Closing Connection"));
		IsConnectionOpen = false;

		ListenSocket->Close();
	}
}

void AFractureNetworkActor::ManageConnection()
{
	// Accept Connection
	if (WaitingForConnection)
	{
		//UE_LOG(LogTemp, Warning, TEXT("[TCP] Waiting..."));
		TSharedRef<FInternetAddr> RemoteAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

		bool hasConnection = false;
		if (ListenSocket->HasPendingConnection(hasConnection) && hasConnection) {
			ConnectionSocket = ListenSocket->Accept(*RemoteAddress, TEXT("[TCP] Received Socket Connection"));
			WaitingForConnection = false;
			UE_LOG(LogTemp, Warning, TEXT("[TCP] Incoming Connection"));

			// Start send & recv thread
			ClientConnectionFinishedFuture = Async(EAsyncExecution::LargeThreadPool, [&]() {
				UE_LOG(LogTemp, Warning, TEXT("[TCP] Thread started"));
				SendArrayMessages();
				ReceiveArrayMessages();
				}
			);
		}
	}
}

void AFractureNetworkActor::SendArrayMessages()
{
	TArray<float> FloatArray;
	TArray<FVector> VectorArray = BreakableActorArr[0]->GetPieceLocArray();
	for (int i = 0; i < VectorArray.Num(); i++) {
		FloatArray.Add(VectorArray[i].X);
		FloatArray.Add(VectorArray[i].Y);
		FloatArray.Add(VectorArray[i].Z);
	}
	UE_LOG(LogTemp, Warning, TEXT("Number of pieces: %d"), VectorArray.Num());

	uint32 ArraySize = FloatArray.Num();
	int32 sent = 0;
	ConnectionSocket->Send(reinterpret_cast<const uint8*>(&ArraySize), sizeof(uint32), sent);

	TArray<uint8> ArrayData;
	ArrayData.SetNumUninitialized(ArraySize * sizeof(float));
	FMemory::Memcpy(ArrayData.GetData(), FloatArray.GetData(), ArrayData.Num());
	sent = 0;
	ConnectionSocket->Send(ArrayData.GetData(), ArrayData.Num(), sent);
}

void AFractureNetworkActor::ReceiveArrayMessages()
{
	// Repeat
	while (IsConnectionOpen) {
		uint32 size;
		TArray<uint8> ReceivedData;

		if (ConnectionSocket->HasPendingData(size)) {
			ReceivedData.Init(0, 4100);
			int32 Read = 0;
			ConnectionSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), Read);
			UE_LOG(LogTemp, Warning, TEXT("ReceivedData Size: %d"), ReceivedData.Num());

			if (ReceivedData.Num() > 0)
			{
				// Receive Array
				TArray<int32> ReceivedArray;
				const int32 numElements = ReceivedData.Num() / sizeof(int32);
				UE_LOG(LogTemp, Warning, TEXT("numElements: %d"), numElements);

				ReceivedArray.SetNum(numElements);
				FMemory::Memcpy(ReceivedArray.GetData(), ReceivedData.GetData(), ReceivedArray.Num());
				int arrayNum = ReceivedArray[0];
				UE_LOG(LogTemp, Warning, TEXT("Received Array length: %d"), arrayNum);

				// Set cluster index array
				TArray<int32> ClusteredIndex;
				ClusteredIndex.Append(ReceivedArray.GetData() + 1, arrayNum);
				BreakableActorArr[0]->SetClusteredIndex(ClusteredIndex);
				BreakableActorArr[0]->SetIsClustered();

				CloseConnection();
			}
		}
	}
}
