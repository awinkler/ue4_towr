// Copyright(c) 2018, Alexander W.Winkler. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include <vector>
#include <string>

#include "MyProject3Character.generated.h"

// This state is read in at every tick and converted to joint-angles  
// using inverse kinematics in the Animation Blueprint.
//
// More information on how the trajectory was generated can be found here:
// https://github.com/ethz-adrl/towr

class TowrState  {
public:

	FVector BasePos; // XYZ position of base in world frame
	FQuat BaseRot;   // roll-pitch-yaw in world frame
	std::vector<FVector> FootPos; // XYZ position of each foot in world frame

	TowrState(const std::vector<std::string>& line);
};


UCLASS(config=Game)
class AMyProject3Character : public AActor
{
	GENERATED_BODY()

public:
	AMyProject3Character();

	/** The main skeletal mesh associated with this Character (optional sub-object). */
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh;

	/** The CapsuleComponent being used for movement collision (by CharacterMovement). Always treated as being vertically aligned in simple collision check functions. */
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* CapsuleComponent;

	// Location of CSV file that holds the towr trajectory
	// Make sure file is in subfolder "TowrTrajectories".
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "TOWR Trajectory")
	FString filename;

	// Time [s] between two points in trajectory
	// Filled by code from CVS file
	UPROPERTY(VisibleAnywhere, Category = "TOWR Trajectory")
	float dt;

private:
	// Reads in the towr csv file and saves the trajectory
	virtual void PreInitializeComponents() override;

	// in every tick of control loop read in new desired state
	std::vector<TowrState> trajectory;
	int IdxTick; 

	// Called inside Blueprint as reference for the Inverse Kinematics 
	UFUNCTION(BlueprintCallable)
	FVector GetEndeffector(int ee) const;

	// updates the base position and rotation (joint angles set in Blueprint)
	virtual void Tick(float DeltaTime) override;
};

