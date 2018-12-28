// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKart.h"
#include "Components/InputComponent.h"
#include "DrawDebugHelpers.h"
#include "UnrealNetwork.h"

// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	MovementComponent = CreateDefaultSubobject<UGoKartMovementComponent>(TEXT("MovementComponent"));
}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		NetUpdateFrequency = 1;
	}
}

void AGoKart::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGoKart, ServerState);		
}
// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MovementComponent == nullptr) return;
	if (Role == ROLE_AutonomousProxy)
	{		
		FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
		MovementComponent->SimulateMove(Move);

		UnacknowledgedMoves.Add(Move);
		Server_SendMove(Move);
		UE_LOG(LogTemp, Warning, TEXT("Queue length: %d"), UnacknowledgedMoves.Num());
		
	}
	if (Role == ROLE_Authority && GetRemoteRole() != ROLE_AutonomousProxy)
	{
		//We are the server and in control of the pawn.
		FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
		Server_SendMove(Move);
	}
	if (Role == ROLE_SimulatedProxy)
	{
		MovementComponent->SimulateMove(ServerState.LastMove);
	}
	//Debug log the role.
	DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumTextForRole(Role), this, FColor::White, DeltaTime);
}
void AGoKart::OnRep_ServerState()
{
	if (MovementComponent == nullptr) return;

	//UE_LOG(LogTemp, Warning, TEXT("Replicated Location"));
	SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.Velocity);

	ClearUnacknowledgedMoves(ServerState.LastMove);

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		MovementComponent->SimulateMove(Move);
	}
}
FString AGoKart::GetEnumTextForRole(ENetRole RoleToText)
{	
	switch (Role)
	{
	case ROLE_None:
		return "None";
	case ROLE_SimulatedProxy:
		return "Simulated Proxy";
	case ROLE_AutonomousProxy:
		return "Autonomous Proxy";
	case ROLE_Authority:
		return "Authority";	
	default:
		return "Error";
	}	
}

void AGoKart::ClearUnacknowledgedMoves(FGoKartMove LastMove)
{		
	TArray<FGoKartMove> NewMoves;
	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{		
		if (Move.Time > LastMove.Time)
		{
			NewMoves.Add(Move);
		}
	}
	UnacknowledgedMoves = NewMoves;
}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::MoveRight);
}
void AGoKart::MoveForward(float Value)
{
	if (MovementComponent == nullptr) return;

	MovementComponent->SetThrottle(Value);
}
void AGoKart::MoveRight(float Value)
{
	if (MovementComponent == nullptr) return;

	MovementComponent->SetSteeringThrow(Value);
}

void AGoKart::Server_SendMove_Implementation(FGoKartMove Move)
{
	if (MovementComponent == nullptr) return;

	MovementComponent->SimulateMove(Move);

	ServerState.LastMove = Move;
	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();
}
bool AGoKart::Server_SendMove_Validate(FGoKartMove Move)
{
	return true;
}