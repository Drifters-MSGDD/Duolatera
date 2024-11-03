// Fill out your copyright notice in the Description page of Project Settings.


#include "VRMultiplayerGameMode.h"

//constructor
AVRMultiplayerGameMode::AVRMultiplayerGameMode()
{
}

void AVRMultiplayerGameMode::PlayerStartBubbleSort(TArray<APlayerStart*>& arr)
{
	if (arr.Num() <= 1) return;
	for (int i = 0; i < arr.Num() - 1; i++) {
		if (arr[i]->GetUniqueID() > arr[i+1]->GetUniqueID()) {
			APlayerStart* temp = arr[i];
			arr[i] = arr[i + 1];
			arr[i + 1] = temp;
		}
	}
	return;
}

int AVRMultiplayerGameMode::GetRelativeIDFromPlayerStates(AController* controller)
{
	if (controller == nullptr)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("Warning: AVRMultiplayerGameMode::GetRelativeIDFromPlayerStates: No PlayerState Input"));
		return -1;
	}
	APlayerController* pc = Cast<APlayerController>(controller);
	if (pc == nullptr)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("Warning: AVRMultiplayerGameMode::GetRelativeIDFromPlayerStates: valid input, but not a player controller"));
		return -1;
	}

	UWorld* World = GetWorld();
	int lowestPlayerID = 2147483647;
	for (TActorIterator<APlayerController> It(World); It; ++It)
	{
		lowestPlayerID = FMath::Min(lowestPlayerID, It->PlayerState->GetPlayerId());
	}

	return pc->PlayerState->GetPlayerId() == lowestPlayerID ? 0 : 1;
}

AActor* AVRMultiplayerGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// Tsingtao: This is copied from AGameModeBase::ChoosePlayerStart_Implementation(AController* Player)
	// My changes will be commented
	// instead of choosing a random PlayerStart, I use AVRMultiplayerGameMode::ID to choose from  UnOccupiedStartPoints
	
	// Choose a player start
	APlayerStart* FoundPlayerStart = nullptr;
	UClass* PawnClass = GetDefaultPawnClassForController(Player);
	APawn* PawnToFit = PawnClass ? PawnClass->GetDefaultObject<APawn>() : nullptr;
	TArray<APlayerStart*> UnOccupiedStartPoints;
	TArray<APlayerStart*> OccupiedStartPoints;
	UWorld* World = GetWorld();
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		APlayerStart* PlayerStart = *It;

		if (PlayerStart->IsA<APlayerStartPIE>())
		{
			// Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
			FoundPlayerStart = PlayerStart;
			break;
		}
		else
		{
			FVector ActorLocation = PlayerStart->GetActorLocation();
			const FRotator ActorRotation = PlayerStart->GetActorRotation();
			// Tsingtao: This is not where I make changes, but this if statement always returns true
			// which means EncroachingBlockingGeometry always consider no overlaping happening, even if this playerStart already has a VRPawn
			// However, I stepped in the function, which shows that our DefaulPawnClass, VRPawn, it's rootComponent is nullptr, 
			// and that's where this function returns false, thus if returns true
			if (!World->EncroachingBlockingGeometry(PawnToFit, ActorLocation, ActorRotation))
			{
				UnOccupiedStartPoints.Add(PlayerStart);
			}
			else if (World->FindTeleportSpot(PawnToFit, ActorLocation, ActorRotation))
			{
				OccupiedStartPoints.Add(PlayerStart);
			}
		}
	}
	//sort the PlayerStart arr, for a matter of fact, only the UnOccipied one is enough.
	PlayerStartBubbleSort(UnOccupiedStartPoints);
	PlayerStartBubbleSort(OccupiedStartPoints);
	if (FoundPlayerStart == nullptr)
	{
		//Since above reason, this if statement will always be true
		if (UnOccupiedStartPoints.Num() > 0)
		{
			// Tsingtao: This is where I changed, instead of random Range, I choose PlayerStart based on current ID
			// So no more 2 players spawning at the same spot
			FoundPlayerStart = UnOccupiedStartPoints[FMath::Clamp(GetRelativeIDFromPlayerStates(Player), 0, UnOccupiedStartPoints.Num() - 1)];
		}
		else if (OccupiedStartPoints.Num() > 0)
		{
			FoundPlayerStart = OccupiedStartPoints[FMath::RandRange(0, OccupiedStartPoints.Num() - 1)];
		}
	}
	return FoundPlayerStart;
}



