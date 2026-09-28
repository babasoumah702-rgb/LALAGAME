# Businesswoman source assets

- `businesswoman_original_tripo.glb`: original generated model before manual cleanup.
- `businesswoman_final_editable.blend`: latest editable Blender art file, including the fitted hair and glasses work.
- `businesswoman_rigged.fbx`: rigged interchange model used during the character pipeline.

The runtime source of truth is the skeletal mesh and related assets under `Content/orc_character/Women_Motified_Ultimate`. Reimporting a GLB or FBX can replace skeleton, skin-weight, material-slot, and animation references. Reimport on a branch or project copy, inspect the result, and keep the original skeleton assignment when animation compatibility is required.
