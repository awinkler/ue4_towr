// Copyright(c) 2018, Alexander W.Winkler. All rights reserved.

#include "MyProject3Character.h"
#include <fstream>
#include <iostream>
#include <sstream>


AMyProject3Character::AMyProject3Character()
{
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionCylinder"));
	CapsuleComponent->InitCapsuleSize(42.f, 96.0f);
	RootComponent = CapsuleComponent;

	// The position and rotation of this mesh with respect to the Capsule Component allows
	// to set the virtual base coordinate system. So to make the UE4 Humanoid execute the
	// motion designed for a quadruped, I rotated this Mesh 90 degrees. 
	Mesh = CreateOptionalDefaultSubobject<USkeletalMeshComponent>(TEXT("Mannequin"));
	if (Mesh)
	{
		Mesh->AlwaysLoadOnClient = true;
		Mesh->AlwaysLoadOnServer = true;
		Mesh->bOwnerNoSee = false;
		Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
		Mesh->bCastDynamicShadow = true;
		Mesh->bAffectDynamicIndirectLighting = true;
		Mesh->SetupAttachment(CapsuleComponent);
	}
	
	// default trajectory
	filename = "anymal_wall_new3.txt";
	dt = 0.0025;

	// So tick update is actually called
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
	UE_LOG(LogTemp, Display, TEXT("Loaded AMyProject3Character"));
}

void AMyProject3Character::PreInitializeComponents()
{
	// load the data
	FString contentFolder = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FPaths::GameContentDir());
	FString filePath = contentFolder + "TowrTrajectories/" + filename;
	std::ifstream file(TCHAR_TO_UTF8(*filePath));
	if (file.fail()) {
		UE_LOG(LogTemp, Error, TEXT("Couldn't find file %s. \n Please double-check path in Robot->TOWR Trajectory"), *filePath);
		PrimaryActorTick.bStartWithTickEnabled = false; // don't go into tick loop because will cause segfault
		return;
	}

	// parse CVS file
	trajectory.clear();
	IdxTick = 0;
  std::string line;

	int row = -1;
	while (std::getline(file, line, '\n')) {
		row++;

		//Skipping first line (column description)
		if (row == 0)
			continue;

		// split line into separate strings
		std::stringstream lineStream(line);
		std::string cell;
		std::vector<std::string> lineVector;
		while (std::getline(lineStream, cell, ',')) {
			lineVector.push_back(cell);
		}

		// get discretization time  
		if (row == 2)
			dt = std::stof(lineVector.at(1)) / 1.0e9f;

		// extract and save state from line
		trajectory.push_back(TowrState(lineVector));
	}

	UE_LOG(LogTemp, Warning, TEXT("TOWR states read: %d"), trajectory.size());
}


TowrState::TowrState(const std::vector<std::string>& line) {

	int scale = 100; // towr in meters, unreal in centimeters
	int idx = 2;     // 0 and 1 are times

	// Base location
	for (int i = 0; i < 3; ++i) {
		BasePos[i] = scale * std::stof(line.at(idx + i));
	}

	// Base rotation
	// Unreal has LHS  (ROS using RHS, but ignoring difference for now), 
	idx = 2 + 3; // index of first quaternion component
	BaseRot.X = std::stof(line.at(idx + 0));
	BaseRot.Y = std::stof(line.at(idx + 1));
	BaseRot.Z = std::stof(line.at(idx + 2));
	BaseRot.W = std::stof(line.at(idx + 3));

	// Desired feet locations
	idx = 2 + (3 + 4) + (3 + 3) + (3 + 3); // pos,vel,acc (linear + angular) 
	int n_ee = (line.size() - idx) / (4 * 3 + 1); // pos,vel,acc,force,contact
	FootPos.resize(n_ee);
	for (int ee = 0; ee < n_ee; ++ee) {
		for (int i = 0; i < 3; ++i) {
			FootPos.at(ee)[i] = scale * std::stof(line.at(idx + ee * 3 * 3 + i));
		}
	}
}


void AMyProject3Character::Tick(float DeltaTime) 
{

	IdxTick += DeltaTime / dt;

	// remain at last reference in trajectory
	if (IdxTick >= trajectory.size()) {
		IdxTick = trajectory.size() - 1;
	}

	// set the base directly
	TowrState curr = trajectory.at(IdxTick);
	SetActorLocation(curr.BasePos);
	SetActorRotation(curr.BaseRot);

	// the feet positions are extracted in the Animation Blueprint
}


FVector AMyProject3Character::GetEndeffector(int ee) const 
{
	if (trajectory.empty())
		return FVector(0.0f); // make sure to initialize

	return trajectory.at(IdxTick).FootPos.at(ee);
}