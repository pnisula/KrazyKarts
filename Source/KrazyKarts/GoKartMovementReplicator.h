// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.h"
#include "GoKartMovementReplicator.generated.h"

USTRUCT()
struct FGoKartState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	FVector Velocity;

	UPROPERTY()
	FGoKartMove LastMove;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UGoKartMovementReplicator : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGoKartMovementReplicator();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
	UFUNCTION()
	void ClearUnacknowledgedMoves(FGoKartMove LastMove);
			
	UFUNCTION()
	void UpdateServerState(const FGoKartMove& Move);

	UFUNCTION()
	void ClientTick(float DeltaTime);

	UFUNCTION()
	void OnRep_ServerState();

	UFUNCTION()
	void AutonomousProxy_OnRep_ServerState();

	UFUNCTION()
	void SimulatedProxy_OnRep_ServerState();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendMove(FGoKartMove Move);	

	UPROPERTY(ReplicatedUsing = OnRep_ServerState)
	FGoKartState ServerState;

	UPROPERTY()
	TArray<FGoKartMove> UnacknowledgedMoves;

	UPROPERTY()
	float ClientTimeSinceUpdate;

	UPROPERTY()
	float ClientTimeBetweenLastUpdates;
	
	UPROPERTY()
	FTransform ClientStartTransform;	

	UPROPERTY()
	UGoKartMovementComponent* MovementComponent;
};
