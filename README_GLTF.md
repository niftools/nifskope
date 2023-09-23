# NifSkope glTF 2.0 Exporter v1.0

# glTF Import

glTF Import into NifSkope will be implemented in a subsequent release.

# glTF Export

glTF export is currently supported on static and skinned Starfield meshes. 

To view and export Starfield meshes, you must first:

1. Enable and add the path to your Starfield installation in Settings > Resources.
2. Add the archives or extracted folders containing `geometries` to Archives or Paths in Settings > Resources, under Starfield.

Skinned meshes currently require a small amount of setup before export. For the Blender glTF importer they require certain import options.

## Skinned meshes

### Pre-Export

If the mesh does not have a skeleton with a `COM` or `COM_Twin` NiNode in its NIF, you need to:

1. Open the skeleton.nif for that skinned mesh (e.g. `meshes\actors\human\characterassets\skeleton.nif`)
2. Copy (Ctrl-C) the `COM` NiNode in skeleton.nif
3. Paste (Ctrl-V) the entire `COM` branch onto the mesh NIF's root NiNode (0)
4. Export to glTF

In Starfield, this needs to be done for meshes skinned on the human skeleton.
This does not need to be done for most creatures, as they come with a `COM_Twin` node.

**Note:** This step will be more automated in the future.

### Pre-Blender Import

In the Blender glTF import window:

1. Ensure "Guess Original Bind Pose" is **unchecked**.

## Blender scripts

Blender scripts are provided for use with glTF exports. They may be opened and run from the Scripting tab inside Blender.

1. `gltf_lod_blender_script.py` is included in the `scripts` folder for managing LOD visibility in exported glTF.
