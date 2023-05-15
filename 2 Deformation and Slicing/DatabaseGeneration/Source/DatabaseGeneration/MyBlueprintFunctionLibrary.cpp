// Code by Marvin Kinz, m.kinz@stud.uni-heidelberg.de

#include "MyBlueprintFunctionLibrary.h"
#include "NvFlexExt.h"
#include "Math/Vector.h"
#include "FlexContainerInstance.h"
//#include "PhysicsPublic.h"
#include "FileManager.h"
#include "Components/StaticMeshComponent.h"
#include "Rendering/PositionVertexBuffer.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "UObject/ConstructorHelpers.h"
//#include "DrawDebugHelpers.h"
#include "FlexComponent.h"
#include "FlexManager.h"
#include "Factories/FbxFactory.h"
#include "FlexStaticMesh.h"
#include "FlexAsset.h"
#include "FlexAssetCloth.h"
#include "FlexAssetSoft.h"
#include "LevelEditor.h"
#include "Editor.h"
#include "UnrealEd.h"
#include "FlexActor.h"
#include "Misc/Paths.h"
//for pmc to static
#include "RawMesh.h"
#include "AssetRegistryModule.h"
#include "ProceduralMeshComponent.h"
//Access Flex Skinned Mesh
#include "FlexRender.h"
//#include "FlexComponent.h"
//#include "FlexAssetSoft.h"
//#include "FlexAssetCloth.h"
//#include "FlexContainerInstance.h"
//#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
//#include "Materials/Material.h"

//----------------------Storing-------------------------------------

void UMyBlueprintFunctionLibrary::WriteVectorDataIntoFile(TArray<FVector> VectorData, FString OutputFolder, FString Filename, FString FileExtension)
{	
	FString Directory = FPaths::ProjectDir();
	FArchive *FileWriter = IFileManager::Get().CreateFileWriter(*Directory.Append(OutputFolder).Append("/").Append(Filename.Append(FileExtension)));// , EFileWrite::FILEWRITE_Append | EFileWrite::FILEWRITE_AllowRead | EFileWrite::FILEWRITE_EvenIfReadOnly);
	FString NewExcerpt;
	for (int i = 0; i < VectorData.Num(); i++) {
		FVector &Vec = VectorData[i];
		NewExcerpt += FString::Printf(TEXT("%f %f %f"), Vec.X, Vec.Y, Vec.Z);
		NewExcerpt += LINE_TERMINATOR;
	}
	FileWriter->Serialize(TCHAR_TO_ANSI(*NewExcerpt), NewExcerpt.Len());
	FileWriter->Close();
	delete FileWriter;
	return;
}

void UMyBlueprintFunctionLibrary::WriteTriangleDataIntoFile(TArray<int> TriangleData, FString OutputFolder, FString Filename, FString FileExtension)
{
	FString Directory = FPaths::ProjectDir();
	FArchive *FileWriter = IFileManager::Get().CreateFileWriter(*Directory.Append(OutputFolder).Append("/").Append(Filename.Append(FileExtension)));// , EFileWrite::FILEWRITE_Append | EFileWrite::FILEWRITE_AllowRead | EFileWrite::FILEWRITE_EvenIfReadOnly);
	FString NewExcerpt;
	for (int i = 0; i < TriangleData.Num()/3; i++) {
		NewExcerpt += FString::Printf(TEXT("%d %d %d"), TriangleData[i*3], TriangleData[i*3+1], TriangleData[i*3+2]);
		NewExcerpt += LINE_TERMINATOR;
	}
	FileWriter->Serialize(TCHAR_TO_ANSI(*NewExcerpt), NewExcerpt.Len());
	FileWriter->Close();
	delete FileWriter;
	return;
}

void UMyBlueprintFunctionLibrary::SaveObject(FString OutputFolder, FString ActorLabel, UFlexComponent *FlexComponent) {
	FString Directory = FPaths::ProjectDir();
	FArchive *FileWriter = IFileManager::Get().CreateFileWriter(*Directory.Append(OutputFolder).Append("/").Append(ActorLabel.Append(".xyz")));// , EFileWrite::FILEWRITE_Append | EFileWrite::FILEWRITE_AllowRead | EFileWrite::FILEWRITE_EvenIfReadOnly);
	FString NewExcerpt;
	for (int i = 0; i < FlexComponent->SimPositions.Num(); i++) {
		FVector4 &Vec = FlexComponent->SimPositions[i];
		FVector &VecN = FlexComponent->SimNormals[i];
		NewExcerpt += FString::Printf(TEXT("%f %f %f %f %f %f"), Vec.X, Vec.Y, Vec.Z, VecN.X, VecN.Y, VecN.Z);
		NewExcerpt += LINE_TERMINATOR;
	}
	FileWriter->Serialize(TCHAR_TO_ANSI(*NewExcerpt), NewExcerpt.Len());
	FileWriter->Close();
	delete FileWriter;
	return;
}

//Taken from FlexRender.cpp 594 "UpdateSoftTransforms" and 285 "SkinSoft"

void UMyBlueprintFunctionLibrary::Skin(UFlexComponent* FlexComponent, TArray<FVector> &Vertices, TArray<FVector> &Normals, TArray<FProcMeshTangent> &Tangents, FRotator &MeanRotation, FVector &MeanTranslation) {

	// Get Flex Soft Asset from Flex Component
	const UFlexAssetSoft* SoftAsset = Cast<UFlexAssetSoft>(FlexComponent->GetFlexAsset());
	if (!SoftAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("Passed FlexComponent is not a Soft Asset, can not be skinned."));
		return;
	}
	//Get Static Mesh from Flex Component
	const UStaticMesh* StaticMesh = FlexComponent->GetStaticMesh();

	// Get Vertex Buffers from Static Mesh of Flex Component
	const FPositionVertexBuffer& Positions = StaticMesh->RenderData->LODResources[0].VertexBuffers.PositionVertexBuffer;
	const FStaticMeshVertexBuffer& StatVertices = StaticMesh->RenderData->LODResources[0].VertexBuffers.StaticMeshVertexBuffer;

	// Get Cluster indices, weights and rest poses from the Flex Soft Asset
	const int16* ClusterIndices = &SoftAsset->IndicesVertexBuffer.Vertices[0];
	const float* ClusterWeights = &SoftAsset->WeightsVertexBuffer.Vertices[0];
	const FVector* RestPoses = &SoftAsset->ShapeCenters[0];
	int maxIndice = 0;

	// Get the Rotations and Translations of the clusters right now from the Asset Instance
	float* Rotations = FlexComponent->AssetInstance->shapeRotations;
	float* Translations = FlexComponent->AssetInstance->shapeTranslations;

	// Set output arrays to size of number of vertices
	const int NumVertices = Positions.GetNumVertices();
	Vertices.SetNum(NumVertices);
	Normals.SetNum(NumVertices);
	Tangents.SetNum(NumVertices);

	FTransform CompTrans = FlexComponent->GetComponentTransform();

	//UE_LOG(LogTemp, Warning, TEXT("%d Number of vertices."), NumVertices);
	//Go through all vertices
	for (int VertexIndex = 0; VertexIndex < NumVertices; ++VertexIndex)
	{	
		//intitialize Position, Normal and Tangent vectors with 0
		FVector SoftPos(0.0f);
		FVector SoftNormal(0.0f);
		FVector SoftTangent(0.0f);
		//go through all clusters, that have influence on current vertex
		for (int w = 0; w < 4; ++w)
		{
			//get indice and weight for current cluster
			const int Cluster = ClusterIndices[VertexIndex * 4 + w];
			const float Weight = ClusterWeights[VertexIndex * 4 + w];

			//if cluster is valid
			if (Cluster > -1)
			{
				if (Cluster > maxIndice) {
					maxIndice = Cluster;
				}

				//Get rotation of cluster from current rotations array
				float InX = Rotations[Cluster * 4];
				float InY = Rotations[Cluster * 4 + 1];
				float InZ = Rotations[Cluster * 4 + 2];
				float InW = Rotations[Cluster * 4 + 3];
				FQuat Rotation = FQuat(InX, InY, InZ, InW);
				//Get translation of cluster from current locations array
				float X = Translations[Cluster * 3];
				float Y = Translations[Cluster * 3 + 1];
				float Z = Translations[Cluster * 3 + 2];
				FVector Translation = FVector(X, Y, Z);
				//UE_LOG(LogTemp, Warning, TEXT("%f %f %f %f Rotation"), InX, InY, InZ, InW);
				//UE_LOG(LogTemp, Warning, TEXT("%f %f %f Translation"), X, Y, Z);
				//Update location of current vertex with weighted rotation and translation of current cluster
				FVector LocalPos = Positions.VertexPosition(VertexIndex) - RestPoses[Cluster];
				SoftPos += (Rotation.RotateVector(LocalPos) + Translation)*Weight;
				//Update normal and tangent of current vertex with weighted rotation of current cluster
				FVector LocalNormal = StatVertices.VertexTangentZ(VertexIndex);
				SoftNormal += (Rotation.RotateVector(LocalNormal))*Weight;
				FVector LocalTangent = StatVertices.VertexTangentX(VertexIndex);
				SoftTangent += (Rotation.RotateVector(LocalTangent))*Weight;
			}
		}

		// Set position of current vertex
		Vertices[VertexIndex] = CompTrans.InverseTransformPosition(SoftPos);
		// Set normal of current vertex
		Normals[VertexIndex] = SoftNormal;
		//Set tangent of current vertex
		FProcMeshTangent Tangent;
		Tangent.TangentX = SoftTangent;
		Tangents[VertexIndex] = Tangent;
	}
	//Get total mean rotation and translation

	float MInX = 0.0;
	float MInY = 0.0;
	float MInZ = 0.0;
	float MInW = 0.0;
	
	float MX = 0.0;
	float MY = 0.0;
	float MZ = 0.0;
	
	for (int c = 0; c < maxIndice; ++c) {
		MInX += Rotations[c * 4];
		MInY += Rotations[c * 4 + 1];
		MInZ += Rotations[c * 4 + 2];
		MInW += Rotations[c * 4 + 3];

		MX += Translations[c * 3];
		MY += Translations[c * 3 + 1];
		MZ += Translations[c * 3 + 2];
	}
	MeanRotation = FQuat(MInX/maxIndice, MInY/maxIndice, MInZ/maxIndice, MInW/maxIndice).Rotator();
	MeanTranslation = CompTrans.InverseTransformPosition(FVector(MX/maxIndice, MY/maxIndice, MZ/maxIndice));
	
}
//-----------------Simulation------------------
void UMyBlueprintFunctionLibrary::GetFlexSoftSettings(UFlexComponent* FlexComponent, float &ParticleSpacing, float &VolumeSampling, float &SurfaceSampling, float &ClusterSpacing, float &ClusterRadius, float &ClusterStiffness, UFlexContainer* &Container) {
	UFlexAssetSoft* FAS = Cast<UFlexAssetSoft>(FlexComponent->GetFlexAsset());
	if (FAS) {
		ParticleSpacing = FAS->ParticleSpacing;
		VolumeSampling = FAS->VolumeSampling;
		SurfaceSampling = FAS->SurfaceSampling;
		ClusterSpacing = FAS->ClusterSpacing;
		ClusterRadius = FAS->ClusterRadius;
		ClusterStiffness = FAS->ClusterStiffness;
		Container = FAS->ContainerTemplate;
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Passed Flex Component has no Flex Soft Asset"));
	}
}

UFlexStaticMesh* UMyBlueprintFunctionLibrary::PMCtoFlex(UProceduralMeshComponent* ProcMesh, int Number, float ParticleSpacing, float VolumeSampling, float SurfaceSampling, float ClusterSpacing, float ClusterRadius, float ClusterStiffness, UFlexContainer* Container)
{
	// Find first selected ProcMeshComp
	UProceduralMeshComponent* ProcMeshComp = ProcMesh;
	if (ProcMeshComp != nullptr)
	{
		FString ActorName = ProcMesh->GetOwner()->GetName();
		FString LevelName = ProcMesh->GetWorld()->GetMapName();
		FString AssetName = ActorName + FString::FromInt(Number);
		FString PathName = FString(TEXT("/Game/ProjectContent/Meshes/PMCtoFlex/"));
		FString PackageName = PathName + AssetName;

		// Raw mesh data we are filling in
		FRawMesh RawMesh;
		// Materials to apply to new mesh
		TArray<UMaterialInterface*> MeshMaterials;

		const int32 NumSections = ProcMeshComp->GetNumSections();
		int32 VertexBase = 0;

		for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
		{
			FProcMeshSection* ProcSection = ProcMeshComp->GetProcMeshSection(SectionIdx);

			// Copy verts
			for (FProcMeshVertex& Vert : ProcSection->ProcVertexBuffer)
			{
				RawMesh.VertexPositions.Add(Vert.Position);
			}

			// Copy 'wedge' info
			int32 NumIndices = ProcSection->ProcIndexBuffer.Num();
			for (int32 IndexIdx = 0; IndexIdx < NumIndices; IndexIdx++)
			{
				int32 Index = ProcSection->ProcIndexBuffer[IndexIdx];

				RawMesh.WedgeIndices.Add(Index + VertexBase);

				FProcMeshVertex& ProcVertex = ProcSection->ProcVertexBuffer[Index];

				FVector TangentX = ProcVertex.Tangent.TangentX;
				FVector TangentZ = ProcVertex.Normal;
				FVector TangentY = (TangentX ^ TangentZ).GetSafeNormal() * (ProcVertex.Tangent.bFlipTangentY ? -1.f : 1.f);

				RawMesh.WedgeTangentX.Add(TangentX);
				RawMesh.WedgeTangentY.Add(TangentY);
				RawMesh.WedgeTangentZ.Add(TangentZ);

				RawMesh.WedgeTexCoords[0].Add(ProcVertex.UV0);
				RawMesh.WedgeColors.Add(ProcVertex.Color);
			}

			// copy face info
			int32 NumTris = NumIndices / 3;
			for (int32 TriIdx = 0; TriIdx < NumTris; TriIdx++)
			{
				RawMesh.FaceMaterialIndices.Add(SectionIdx);
				RawMesh.FaceSmoothingMasks.Add(0); // Assume this is ignored as bRecomputeNormals is false
			}

			// Remember material
			MeshMaterials.Add(ProcMeshComp->GetMaterial(SectionIdx));

			// Update offset for creating one big index/vertex buffer
			VertexBase += ProcSection->ProcVertexBuffer.Num();

			// If we got some valid data.
			if (RawMesh.VertexPositions.Num() > 3 && RawMesh.WedgeIndices.Num() > 3)
			{
				// Then find/create it.
				UPackage* Package = CreatePackage(NULL, *PackageName);
				check(Package);

				// Create StaticMesh object as Flex
				UFlexStaticMesh* StaticMesh = NewObject<UFlexStaticMesh>(Package, FName(*AssetName), RF_Public | RF_Standalone);
				StaticMesh->InitResources();
				StaticMesh->bAllowCPUAccess = true;
				UFlexAssetSoft* FAS = NewObject<UFlexAssetSoft>(StaticMesh);
				FAS->ParticleSpacing = ParticleSpacing;
				FAS->VolumeSampling = VolumeSampling;
				FAS->SurfaceSampling = SurfaceSampling;
				FAS->ClusterSpacing = ClusterSpacing;
				FAS->ClusterRadius = ClusterRadius;
				FAS->ClusterStiffness = ClusterStiffness;
				FAS->ContainerTemplate = Container;
				//Alternative to load set container in code, but choosing in BP is easier
				//FStreamableManager AssetLoader;
				//FStringAssetReference AssetRef("/Game/ProjectContent/Flex/FlexContainerSoft.FlexContainerSoft");
				//UFlexContainer* Container = Cast<UFlexContainer>(AssetLoader.SynchronousLoad(AssetRef));
				StaticMesh->FlexAsset = FAS;

				StaticMesh->LightingGuid = FGuid::NewGuid();

				// Add source to new StaticMesh

				FStaticMeshSourceModel* SrcModel = new (StaticMesh->SourceModels) FStaticMeshSourceModel();
				SrcModel->BuildSettings.bRecomputeNormals = false;
				SrcModel->BuildSettings.bRecomputeTangents = false;
				SrcModel->BuildSettings.bRemoveDegenerates = false;
				SrcModel->BuildSettings.bUseHighPrecisionTangentBasis = false;
				SrcModel->BuildSettings.bUseFullPrecisionUVs = false;
				SrcModel->BuildSettings.bGenerateLightmapUVs = true;
				SrcModel->BuildSettings.SrcLightmapIndex = 0;
				SrcModel->BuildSettings.DstLightmapIndex = 1;
				SrcModel->RawMeshBulkData->SaveRawMesh(RawMesh);



				// Copy materials to new mesh
				for (UMaterialInterface* Material : MeshMaterials)
				{
					StaticMesh->StaticMaterials.Add(FStaticMaterial(Material));
				}

				//Set the Imported version before calling the build
				StaticMesh->ImportVersion = EImportStaticMeshVersion::LastVersion;

				// Build mesh from source
				StaticMesh->Build(false);
				StaticMesh->PostEditChange();

				// Notify asset registry of new asset
				FAssetRegistryModule::AssetCreated(StaticMesh);

				return StaticMesh;



			}
		}

	}

	return nullptr;

}

void UMyBlueprintFunctionLibrary::ReregisterFlexComponent(UFlexComponent * FlexComponent)
{
	//NvFlexExtNotifyAssetChanged(FlexComponent->ContainerInstance->Container, FlexComponent->GetFlexAsset()->GetFlexAsset());
	FlexComponent->OnUnregister();
	FlexComponent->OnRegister();
}



//-----------------Importing-------------------------
void UMyBlueprintFunctionLibrary::ImportAssets(FString InputFolder, FString RootDestination) {
	//FPaths::NormalizeDirectoryName(RootDestination);
	TArray<FString> FoundFiles;
	FString ext = ""; //could be used to filter for file extensions
	FString ProjDirectory = FPaths::ProjectDir();
	FString Directory = ProjDirectory + InputFolder;
	IFileManager::Get().FindFiles(FoundFiles,*Directory, *ext); //get all files in folder

	//define import folders
	FString StaticFolder = RootDestination + FString("/Static/");
	FString FlexFolder = RootDestination + FString("/Flex/");
	//setup import factory
	UFbxFactory* Factory = NewObject<UFbxFactory>(UFbxFactory::StaticClass(), FName("Factory")); 
	bool bImportedCancelled = false; //boolean for the import function
	//Factory->ImportUI->StaticMeshImportData->ImportUniformScale = 100; #no effect
	//Factory->ImportUI->StaticMeshImportData->bCombineMeshes = true;
	//Factory->ImportUI->StaticMeshImportData->bConvertSene = true;
	Factory->ImportUI->MeshTypeToImport = FBXIT_StaticMesh;
	Factory->ImportUI->bImportMaterials = true;
	//Factory->EnableShowOption(); //Shows import settings GUI
	//Iterate through files in folder to import one after the other
	if (FoundFiles.Num() == 0) {
		UE_LOG(LogTemp, Warning, TEXT("%s seems to be empty."), *Directory);
		return;
	}

	for (auto &File : FoundFiles) {
		FString FileName = FPaths::GetBaseFilename(File); //remove extension from file name
		FileName.RemoveFromEnd("_mesh");
		FString FileLocation = Directory + "/" + File; //reconstruct file location to import from
		//UE_LOG(LogTemp, Warning, TEXT("%s"), *FileLocation);
		//Import object as static mesh
		FString PackageName = StaticFolder + FileName;
		UPackage* Package = CreatePackage(nullptr, *PackageName);
		UObject* StaticMesh = Factory->ImportObject(Factory->ResolveSupportedClass(), Package, *FileName, RF_Public | RF_Standalone | RF_Transactional, FileLocation, nullptr, bImportedCancelled);
		//Set name for Flex mesh and save a converted flex soft static mesh
		//FString FileNameFlex = StaticMesh->GetName().Append("_Flex"); //leave this out, only clutters filename later. But if you want it change FileName to FileNameFlex three lines below
		FString PackageNameFlex = FlexFolder + FileName;
		UPackage* PackageFlex = CreatePackage(nullptr, *PackageNameFlex);
		UFlexStaticMesh* FSM = Cast<UFlexStaticMesh>(StaticDuplicateObject(StaticMesh, PackageFlex, *FileName, RF_AllFlags, UFlexStaticMesh::StaticClass()));
		UFlexAssetSoft* FAS = NewObject<UFlexAssetSoft>(FSM);
		FSM->FlexAsset = FAS;
		FSM->bAllowCPUAccess = 1;
		//UFlexAssetCloth* FAC = NewObject<UFlexAssetCloth>(FSM); 
		//FSM->FlexAsset = FAC;
	
	}
	return;
}

UObject* UMyBlueprintFunctionLibrary::SpawnInEditor(UStaticMesh* asset, FTransform T) //Courtesy of Max
	{
		UWorld* world = GEditor->LevelViewportClients[0]->GetWorld();

		// many other ways:
		//UWorld* world = GEditor->GetEditorWorldContext().World();
		// GEditor->PlayWorld;
		//GEditor->EditorWorld;



		if (world) {
			FVector Location = T.GetLocation();
			FRotator Rotation = T.GetRotation().Rotator();
			FActorSpawnParameters SpawnInfo;
			//SpawnInfo.Name = asset->GetFName(); 
			UStaticMesh* FlexStaticMesh = Cast<UStaticMesh>(asset);
			if (FlexStaticMesh) {

				if (Cast<UFlexStaticMesh>(asset)) {
					AFlexActor* FlexActor = world->SpawnActor<AFlexActor>(Location, Rotation, SpawnInfo);
					FlexActor->GetStaticMeshComponent()->SetStaticMesh(FlexStaticMesh);
					//FlexActor->SetActorLabel(asset->GetName());
					UE_LOG(LogTemp, Warning, TEXT("Spawned a FlexActor"));
					return FlexActor;
				}
				else
				{
					AStaticMeshActor* Actor = world->SpawnActor<AStaticMeshActor>(Location, Rotation, SpawnInfo);
					Actor->GetStaticMeshComponent()->SetStaticMesh(FlexStaticMesh);
					Actor->SetActorLabel(asset->GetName());
					UE_LOG(LogTemp, Warning, TEXT("Spawned a StaticMeshActor"));
					return Actor;
				}
			}

		}
		else {
			UE_LOG(LogTemp, Warning, TEXT("UEditorAutomatization::SpawnInEditor: cant find editor world / editor level"));
		}
		return nullptr;
	}

UObject* UMyBlueprintFunctionLibrary::SpawnBPInEditor(UStaticMesh* asset, FTransform T, FString Blueprint) //Based on Max Legnars Spawn in Editor
{
	UWorld* world = GEditor->LevelViewportClients[0]->GetWorld();

	// many other ways:
	//UWorld* world = GEditor->GetEditorWorldContext().World();
	// GEditor->PlayWorld;
	//GEditor->EditorWorld;


	//Check for world
	if (world) {
		//Load Blueprint as Object to spawn
		UObject* SpawnActor = Cast<UObject>(StaticLoadObject(UObject::StaticClass(), NULL, *Blueprint));//TEXT("/Game/ProjectContent/Blueprints/NewProcCompBP.NewProcCompBP")));
		//Cast Blueprint class on Object to spawn
		UBlueprint* GeneratedBP = Cast<UBlueprint>(SpawnActor);
		//Check, whether it has been loaded
		if (!SpawnActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("Can not find Blueprint to spawn"));
			return nullptr;
		}
		//Check, whether it has a class
		UClass* SpawnClass = SpawnActor->StaticClass();
		if (SpawnClass == NULL)
		{
			UE_LOG(LogTemp, Warning, TEXT("Class is NULL"));
			return nullptr;
		}
		//Check, whether passed asset is Static Mesh
		UStaticMesh* FlexStaticMesh = Cast<UStaticMesh>(asset);
		if (FlexStaticMesh) {
			//Check, whether it is Flex Static Mesh
			if (Cast<UFlexStaticMesh>(asset)) {
				//Set up spawn parameters
				FVector Location = T.GetLocation();
				FRotator Rotation = T.GetRotation().Rotator();
				FActorSpawnParameters SpawnInfo;
				//Spawn BP as actor with spawn params
				AActor* GeneratedBPS = world->SpawnActor<AActor>(GeneratedBP->GeneratedClass, Location, Rotation, SpawnInfo);
				//Get Flex Component from just spawned BP
				UFlexComponent* FlexComp = GeneratedBPS->FindComponentByClass<UFlexComponent>();
				if (FlexComp) {
					//Set the given Flex Static Mesh
					FlexComp->SetStaticMesh(FlexStaticMesh);
					//Set Label of actor to name of given Flex asset
					GeneratedBPS->SetActorLabel(asset->GetName());
					UE_LOG(LogTemp, Warning, TEXT("Spawned a Blueprint with FlexComponent"));
					return GeneratedBPS;
				}
				else {
					UE_LOG(LogTemp, Warning, TEXT("Blueprint has no FlexComponent."));
					return GeneratedBPS;
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Not a Flex Static Mesh"));
				return nullptr;
			}
		}

	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("UEditorAutomatization::SpawnInEditor: cant find editor world / editor level"));
	}
	return nullptr;
}

void UMyBlueprintFunctionLibrary::ApplyChanges(UStaticMesh* asset) { //Courtesy of Max
	UFlexStaticMesh* flex_asset = Cast<UFlexStaticMesh>(asset);
	if (flex_asset) {
		flex_asset->FlexAsset->ReImport(asset);
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("UEditorAutomatization::ApplyChanges: passed asset is not a UFlexStaticMesh"));
	}
}

//-----------NON FUNCTIONAL!!! Try out to interact directly with Flex Constraints------------

void UMyBlueprintFunctionLibrary::ChangeShapes(UFlexComponent * FlexComponent)
{   
	UFlexAsset* FA = FlexComponent->GetFlexAsset();
	
	TArray<FVector> ShapeCenters = FA->ShapeCenters;
	TArray<int32> ShapeIndices = FA->ShapeIndices;
	TArray<int32> ShapeOffsets = FA->ShapeOffsets;
	TArray<float> ShapeCoefficients = FA->ShapeCoefficients;
	
	FA->ShapeCenters.Empty();
	FA->ShapeIndices.Empty();
	FA->ShapeOffsets.Empty();
	FA->ShapeCoefficients.Empty();

	int32 NewLength = (ShapeOffsets.Num()+1)/2;

	FA->ShapeOffsets.Append(&ShapeOffsets[0], NewLength);
	FA->ShapeCenters.Append(&ShapeCenters[0], NewLength);
	FA->ShapeCoefficients.Append(&ShapeCoefficients[0], NewLength);
	FA->ShapeIndices.Append(&ShapeIndices[0], FA->ShapeOffsets.Last());
	/*
	//Play around with buffers for skinning
	const UFlexAssetSoft* SoftAsset = Cast<UFlexAssetSoft>(FA);
	//SoftAsset->IndicesVertexBuffer.VertexBufferRHI.

	FFlexMeshSceneProxy* FMSP = Cast<FFlexMeshSceneProxy>(FlexComponent->SceneProxy);
	FFlexCPUVertexFactory* VF = Cast<FFlexCPUVertexFactory>(FMSP->VertexFactory);
	VF->VertexBuffer.
	*/
	NvFlexExtNotifyAssetChanged(FlexComponent->ContainerInstance->Container, FA->GetFlexAsset());
	
	//FlexComponent->OnUnregister();
	//FlexComponent->OnRegister();
	//FlexComponent->ContainerInstance->Register(FlexComponent);
	//FlexComponent->OverrideAsset = true;
	return;
}

void UMyBlueprintFunctionLibrary::AddShapeConstraintToTearingCloth(UFlexComponent * FlexComponent) {
	NvFlexExtAsset* FA = FlexComponent->TearingAsset;
	UFlexAssetCloth* FAC = Cast<UFlexAssetCloth>(FlexComponent->GetFlexAsset());
	FA->shapeCenters = &FAC->RigidCenter[0];
	FA->shapeCoefficients = &FAC->RigidStiffness;
	TArray<int> ShapeIndices;
	for (int i = 0; i < FA->numParticles; ++i)
		ShapeIndices.Add(i);
	FA->shapeIndices = &ShapeIndices[0];
	FA->shapeOffsets = &FA->numParticles;
	FA->numShapes = 1;
	FA->numShapeIndices = FA->numParticles;
	/*
	ShapeCenters.Add(RigidCenter);
	ShapeCoefficients.Add(RigidStiffness);

	for (int i = 0; i < Particles.Num(); ++i)
		ShapeIndices.Add(i);

	ShapeOffsets.Add(Particles.Num());
	FA->numShapes = 1;
	*/
}


