# Starting a new move or costume

## Create or duplicate an animation

In **Animation**, select the source clip, then open **New / duplicate clip** in
the left sidebar. Give the new clip a name and choose its **Target action**.
**Duplicate selected clip** makes an independent copy. **New from rest pose**
creates a clip with the costume's rest transforms and the chosen frame length.
You can then pose bones, insert keys and edit curves in the existing tools.

The target action keeps its events and playback settings. Save keeps the clip;
Undo restores the previous assignment. A new clip replaces the animation of an
existing action. Choose the action whose behavior you intend to author.

Clip names accept letters, numbers and underscores. An action's display name
can contain spaces: open **Edit name / visibility** in the Action Editor's left
sidebar and change **Action name**. **Hide action in editor** removes an unused entry from that
workspace's list. **Show hidden actions** reveals it again. Hidden actions keep
their animations, events and engine slots, so gameplay can still use them.

## Import several meshes

In **Model**, import a GLB or glTF and select the meshes to bring in. **Select
all** and **Select none** apply to the source list. Choose the attachment bone,
scale and binding, then **Import selected meshes**.

Adding meshes imports the selection together. If any selected mesh cannot be
imported, the draft stays as it was. Undo removes the whole imported selection.
Replacing existing geometry requires selecting one source mesh and one
destination mesh. The destination keeps its material.
Existing expressions and mesh visibility stay attached to their original meshes.
Undoing a move or attribute edit preserves the costume's current assignments.

Source mesh and material names appear in the outliner. Select a mesh, material
or bone and use its **Name** field to give it a useful name. **Apply name** changes
the label in the editor. **Save** keeps names and hidden-action choices together
with the edited model and animations. Undo restores the previous edit. Names
and hidden-action choices are local editing preferences.

## Replace expression images

Some materials choose from several images as the fighter animates. Select the
material in **Model → Materials**, choose a PNG or JPEG in **Texture**, and
choose an image under **Animated texture images**. **Replace selected image**
changes that image in the sequence. The sequence keeps its image indices and
animation timing, and other images stay intact. Play an animation using that
expression to inspect the result. Save persists it; Undo restores the image.

**Apply texture** changes a material's regular image. Existing UV transforms,
wrapping and filtering remain intact. Meshes sharing the material change
together. Use **Replace selected image** for images chosen by a material
animation.

## Move behavior

Special moves have separate grounded and airborne motion names. See
[`state.enter`](scripting/fighter.md#stateenter) when starting a move or changing
between its ground and air variants. To adjust a newly created hitbox before
contact, register an [accessory helper](scripting/fighter_hitboxes.md#creating-a-hitbox).
