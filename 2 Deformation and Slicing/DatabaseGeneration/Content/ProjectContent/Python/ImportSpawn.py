#Code by Marvin Kinz, m.kinz@stud.uni-heidelberg.de

import unreal

tic = unreal.MathLibrary.now()

#directory with meshes to import, should fit to directory of Random Object Generation, starts from DatabaseGeneration project folder as root
input_directory = "../1 Random Objects/Mesh"
#path where to place imported assets in game
output_directory = "/Game/ProjectContent/Meshes/Import"

#set spawn parameters
#scaling for spacing between objects
spacing = 20 #<------------------------------------- You might want to change this, if objects intersect during simulation------------------------------
#x and y positions to spawn
x=0
y=0
#maximum bound in of all objects in any direction (or minimum distance between two rows)
maxbound = 0


#import assets
unreal.MyBlueprintFunctionLibrary.import_assets(input_directory, output_directory)
AssetRegistry = unreal.AssetRegistryHelpers.get_asset_registry()
#get imported flex assets
assets=AssetRegistry.get_assets_by_path(unreal.StringLibrary.concat_str_str(output_directory,"/Flex"))

#for each flex asset
for i,ass in enumerate(assets):
    #load asset
    asset= unreal.load_asset(unreal.World(),ass.object_path)
    bounds_max=asset.get_bounding_box().max
    bounds_min=asset.get_bounding_box().min
    bmax=max([bounds_max.x, bounds_max.y, bounds_max.z, -1*bounds_min.x, -1*bounds_min.y, -1*bounds_min.z])
    if bmax > maxbound:
        maxbound=bmax
    #change properties <-----------------------------------------------------------------Edit or add lines below to change object properties----------------------------
    #asset.get_editor_property('FlexAsset').set_editor_property('ParticleSpacing',10.0)
    #asset.get_editor_property('FlexAsset').set_editor_property('VolumeSampling',4.0)
    #asset.get_editor_property('FlexAsset').set_editor_property('SurfaceSampling',1.0)
    #asset.get_editor_property('FlexAsset').set_editor_property('ClusterSpacing',20.0)
    #asset.get_editor_property('FlexAsset').set_editor_property('ClusterRadius',30.0)
    asset.get_editor_property('FlexAsset').set_editor_property('ClusterStiffness',0.1)
    asset.get_editor_property('FlexAsset').set_editor_property('ContainerTemplate',unreal.load_asset(unreal.World(),"/Game/ProjectContent/Flex/FlexContainerSoft.FlexContainerSoft"))
    #apply changes
    unreal.MyBlueprintFunctionLibrary.apply_changes(asset)
    



#length of row for square
row=int(len(assets)**(0.5))
#spawn each asset in seperate loop to get enough time to apply changes before spawning
for i,ass in enumerate(assets):
    #load asset
    asset= unreal.load_asset(unreal.World(),ass.object_path)
    #get bounds of asset
    bounds_min=asset.get_bounding_box().min
    #bounds_max=asset.get_bounding_box().max
    #set transform with x, y parameters and z bounds to spawn above ground
    T = unreal.Transform()
    T.translation = unreal.Vector(x,y,-bounds_min.z+10)
    #spawn Blueprint with Flex Asset
    unreal.MyBlueprintFunctionLibrary.spawn_bp_in_editor(asset,T,"/Game/ProjectContent/Blueprints/SliceNStore.SliceNStore")
    #update spawn parameters for next asset to create grid
    #start new row
    if (i+1)%row==0:
    	x=0
    	y+=spacing*maxbound
    #or continue current one
    else:
    	x+=spacing*maxbound
	

toc = unreal.MathLibrary.now()

tictoc=unreal.MathLibrary.subtract_date_time_date_time(toc, tic)

print(unreal.MathLibrary.get_total_seconds(tictoc))