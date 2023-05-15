// Code by Marvin Kinz, m.kinz@stud.uni-heidelberg.de

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FlexComponent.h"
#include "FlexManager.h"
#include "Math/Vector.h"
#include "ProceduralMeshComponent.h"
#include "MyBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class DATABASEGENERATION_API UMyBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	//----------------------Storing-------------------------------------
	/*
	* Saves position array (X,Y,Z) to file
	*/
	UFUNCTION(BlueprintCallable, Category = "FlexParticleLibrary|Storing")
		static void WriteVectorDataIntoFile(TArray<FVector> VectorData, FString OutputFolder, FString Filename, FString FileExtension = ".xyz");
	/*
	* Saves triangle array to file
	*/
	UFUNCTION(BlueprintCallable, Category = "FlexParticleLibrary|Storing")
		static void WriteTriangleDataIntoFile(TArray<int> TriangleData, FString OutputFolder, FString Filename, FString FileExtension = ".triangle");

	/*
	* Saves object (Flex Component simulation points) to file as .xyz with format X Y Z nx ny nz
	*/
	UFUNCTION(BlueprintCallable, Category = "FlexParticleLibrary|Storing")
		static void SaveObject(FString OutputFolder, FString ActorLabel, UFlexComponent *FlexComponent);
	
	//Gets deformed Vertice Positions, Normals and Tangents of SoftAsset
	UFUNCTION(BlueprintCallable, Category = "FlexParticleLibrary|Storing")
		static void Skin(UFlexComponent *FlexComponent, TArray<FVector> &Vertices, TArray<FVector> &Normals, TArray<FProcMeshTangent> &Tangents, FRotator &MeanRotation, FVector &MeanTranslation);

	//----------------Simulation------------------------
	/*
	* Gets settings for Flex Soft Asset from Flex Component
	*/
	UFUNCTION(BlueprintCallable, Category = "FlexParticleLibrary|Simulation")
		static void GetFlexSoftSettings(UFlexComponent * FlexComponent, float & ParticleSpacing, float & VolumeSampling, float & SurfaceSampling, float & ClusterSpacing, float & ClusterRadius, float & ClusterStiffness, UFlexContainer *& Container);

	/*
	* Converts PMC to Flex Static Mesh
	*/
	UFUNCTION(BlueprintCallable, Category = "FlexParticleLibrary|Simulation")
		static UFlexStaticMesh* PMCtoFlex(UProceduralMeshComponent* ProcMesh, int Number, float ParticleSpacing, float VolumeSampling, float SurfaceSampling, float ClusterSpacing, float ClusterRadius, float ClusterStiffness, UFlexContainer* Container);
	/*
	Reregister Flex Component
	*/
	UFUNCTION(BlueprintCallable, Category = "FlexParticleLibrary|Simulation")
		static void ReregisterFlexComponent(UFlexComponent* FlexComponent);

		
	//-----------------Importing-------------------------
	/*
	Import Assets automated as Flex Soft Asset
	*/
	UFUNCTION(BlueprintCallable, Category = "FlexParticleLibrary|Importing")
		static void ImportAssets(FString InputFolder, FString RootDestination);
	/*
	*From Max
	Place a StaticMesh or FlexStaticMesh in the current editor level with transform T
	*/
	UFUNCTION(BlueprintCallable, Category = "FlexParticleLibrary|Importing")
		static UObject* SpawnInEditor(UStaticMesh* asset, FTransform T);
	
	/*
	Place NewProcCompBP with specified flex soft asset in the current editor level with transform T
	*/
	UFUNCTION(BlueprintCallable, Category = "FlexParticleLibrary|Importing")
		static UObject * SpawnBPInEditor(UStaticMesh * asset, FTransform T, FString Blueprint);

	/*
	*From Max
	If you want to change propertys of a FlexStaticMesh, you should call this in order to execute the voxelisation again!
	*/
	UFUNCTION(BlueprintCallable, Category = "FlexParticleLibrary|Importing")
		static void ApplyChanges(UStaticMesh* asset);

	//-----------Try out to interact directly with Flex Constraints------------

	/*
	Change Flex Shapes (Currently just deletes some shape constraints)
	*/
	UFUNCTION(BlueprintCallable, Category = "FlexParticleLibrary|Experimental")
		static void ChangeShapes(UFlexComponent* FlexComponent);
	
	/*
	Current State: FATAL ERROR. So do NOT use this. Try to add shape constraints to tearable cloth
	*/
	UFUNCTION(BlueprintCallable, Category = "FlexParticleLibrary|Experimental")
		static void AddShapeConstraintToTearingCloth(UFlexComponent * FlexComponent);
};