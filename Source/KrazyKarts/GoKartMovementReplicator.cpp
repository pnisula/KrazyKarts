// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKartMovementReplicator.h"
#include "UnrealNetwork.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

// Sets default values for this component's properties
UGoKartMovementReplicator::UGoKartMovementReplicator()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicated(true);	
}

void UGoKartMovementReplicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGoKartMovementReplicator, ServerState);
}

// Called when the game starts
void UGoKartMovementReplicator::BeginPlay()
{
	Super::BeginPlay();
	MovementComponent = GetOwner()->FindComponentByClass<UGoKartMovementComponent>();

	//VERY UGLY HACK, BECAUSE SET OFFSET MESH ROOT WAS NOT WORKING
	MeshOffsetRoot = GetOwner()->FindComponentByClass<USceneComponent>()->GetChildComponent(0);
	//UE_LOG(LogTemp, Warning, TEXT("MeshOffsetRoot Name: %s"), *MeshOffsetRoot->GetName());
}


// Called every frame
void UGoKartMovementReplicator::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (MovementComponent == nullptr) return;
	
	FGoKartMove LastMove = MovementComponent->GetLastMove();
	
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{		
		UnacknowledgedMoves.Add(LastMove);
		Server_SendMove(LastMove);
		//UE_LOG(LogTemp, Warning, TEXT("Queue length: %d"), UnacknowledgedMoves.Num());
	}	
	if (GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{		
		UpdateServerState(LastMove);				
	}
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		ClientTick(DeltaTime);
	}
}
void UGoKartMovementReplicator::UpdateServerState(const FGoKartMove& Move)
{
	ServerState.LastMove = Move;
	ServerState.Transform = GetOwner()->GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();
}
void UGoKartMovementReplicator::ClientTick(float DeltaTime)
{
	ClientTimeSinceUpdate += DeltaTime;

	if (ClientTimeBetweenLastUpdates < KINDA_SMALL_NUMBER) return;
	if (MovementComponent == nullptr) return;

	float LerpRatio = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdates;	
	FHermiteCubicSpline Spline = CreateSpline();
	
	InterpolateLocation(Spline, LerpRatio);
	InterpolateVelocity(Spline, LerpRatio);
	InterpolateRotation(LerpRatio);	
}
float UGoKartMovementReplicator::VelocityToDerivative()
{
	return ClientTimeBetweenLastUpdates * 100.0f; //Meters to centimeters
}
FHermiteCubicSpline UGoKartMovementReplicator::CreateSpline()
{	
	FHermiteCubicSpline Spline;
	Spline.TargetLocation = ServerState.Transform.GetLocation();	
	Spline.StartLocation = ClientStartTransform.GetLocation();	
	Spline.StartDerivative = ClientStartVelocity * VelocityToDerivative();
	Spline.TargetDerivative = ServerState.Velocity * VelocityToDerivative();	
	return Spline;
}
void UGoKartMovementReplicator::InterpolateLocation(const FHermiteCubicSpline& Spline, float LerpRatio)
{
	FVector NewLocation = Spline.InterpolateLocation(LerpRatio);
	if (MeshOffsetRoot != nullptr)
	{
		MeshOffsetRoot->SetWorldLocation(NewLocation);		
	}	
}
void UGoKartMovementReplicator::InterpolateVelocity(const FHermiteCubicSpline& Spline, float LerpRatio)
{
	FVector NewDerivative = Spline.InterpolateDerivative(LerpRatio);
	FVector NewVelocity = NewDerivative / VelocityToDerivative();
	MovementComponent->SetVelocity(NewVelocity);
}
void UGoKartMovementReplicator::InterpolateRotation(float LerpRatio)
{
	FQuat TargetRotation = ServerState.Transform.GetRotation();
	FQuat StartRotation = ClientStartTransform.GetRotation();
	FQuat NewRotation = FQuat::Slerp(StartRotation, TargetRotation, LerpRatio);
	
	if (MeshOffsetRoot != nullptr)
	{
		MeshOffsetRoot->SetWorldRotation(NewRotation);		
	}		
}
void UGoKartMovementReplicator::OnRep_ServerState()
{
	switch (GetOwnerRole())
	{
		case ROLE_AutonomousProxy: 
			AutonomousProxy_OnRep_ServerState();
			break;
		case ROLE_SimulatedProxy:
			SimulatedProxy_OnRep_ServerState();
			break;
		default:
			break;
	}
}
void UGoKartMovementReplicator::AutonomousProxy_OnRep_ServerState()
{
	if (MovementComponent == nullptr) return;

	//UE_LOG(LogTemp, Warning, TEXT("Replicated Location"));
	GetOwner()->SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.Velocity);

	ClearUnacknowledgedMoves(ServerState.LastMove);

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		MovementComponent->SimulateMove(Move);
	}
}


void UGoKartMovementReplicator::SimulatedProxy_OnRep_ServerState()
{
	if (MovementComponent == nullptr) return;

	ClientTimeBetweenLastUpdates = ClientTimeSinceUpdate;
	ClientTimeSinceUpdate = 0;

	if (MeshOffsetRoot != nullptr)
	{
		ClientStartTransform.SetLocation(MeshOffsetRoot->GetComponentLocation());
		ClientStartTransform.SetRotation(MeshOffsetRoot->GetComponentQuat());
	}
	
	ClientStartVelocity = MovementComponent->GetVelocity();

	GetOwner()->SetActorTransform(ServerState.Transform);
}

void UGoKartMovementReplicator::ClearUnacknowledgedMoves(FGoKartMove LastMove)
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
void UGoKartMovementReplicator::Server_SendMove_Implementation(FGoKartMove Move)
{
	if (MovementComponent == nullptr) return;

	ClientSimulatedTime += Move.DeltaTime;
	MovementComponent->SimulateMove(Move);
	UpdateServerState(Move);	
}
bool UGoKartMovementReplicator::Server_SendMove_Validate(FGoKartMove Move)
{
	float ProposedTime = ClientSimulatedTime + Move.DeltaTime;
	bool ClientNotRunningAhead = ProposedTime < GetWorld()->TimeSeconds;
	if (!ClientNotRunningAhead)
	{
		return false;
	}
	return Move.IsValid();
}