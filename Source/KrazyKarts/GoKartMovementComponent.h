// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.generated.h"

USTRUCT()
struct FGoKartMove
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	float Throttle;

	UPROPERTY()
	float SteeringThrow;

	UPROPERTY()
	float DeltaTime;

	UPROPERTY()
	float Time;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UGoKartMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGoKartMovementComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION()
	void SimulateMove(const FGoKartMove& Move);
	
	UFUNCTION()
	FVector GetVelocity() { return Velocity; };
	
	UFUNCTION()
	void SetVelocity(FVector Value) { Velocity = Value; };

	UFUNCTION()
	void SetThrottle(float Value) { Throttle = Value; };

	UFUNCTION()
	void SetSteeringThrow(float Value) { SteeringThrow = Value; };

	UFUNCTION()
	FGoKartMove GetLastMove() { return LastMove; };

private:	
	UFUNCTION()
	FGoKartMove CreateMove(float DeltaTime);

	UFUNCTION()
	FVector GetAirResistance();
	
	UFUNCTION()
	FVector GetRollingResistance();

	UFUNCTION()
	void ApplyRotation(float DeltaTime, float SteeringThrow);

	UFUNCTION()
	void UpdateLocationFromVelocity(float DeltaTime);

	// The mass of the car (kg)
	UPROPERTY(EditAnywhere)
	float Mass = 1000;

	// The force applied to the car when the throttle is fully down (N)
	UPROPERTY(EditAnywhere)
	float MaxDrivingForce = 10000;

	// Minimum radius of the car turning circle at full lock (m).
	UPROPERTY(EditAnywhere)
	float MinimumTurningRadius = 10;

	//Higher means more drag.
	UPROPERTY(EditAnywhere)
	float DragCoefficient = 16;

	//Higher means more drag.
	UPROPERTY(EditAnywhere)
	float RollingResistantCoefficient = 0.015;

	UPROPERTY()
	FVector Velocity;

	UPROPERTY()
	float Throttle;

	UPROPERTY()
	float SteeringThrow;

	UPROPERTY()
	FGoKartMove LastMove;
};
