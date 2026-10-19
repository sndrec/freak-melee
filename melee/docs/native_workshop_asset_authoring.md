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
destination mesh. Choose whether to keep its edited material or replace it with
the source material.
Existing expressions and mesh visibility stay attached to their original meshes.
Undoing a move or attribute edit preserves the costume's current assignments.

Source mesh and material names appear in the outliner. Select a mesh, material
or bone and use its **Name** field to give it a useful name. **Apply name** changes
the label in the editor. **Save** keeps names and hidden-action choices together
with the edited model and animations. Undo restores the previous edit. Names
and hidden-action choices are local editing preferences.

## Paint bone weights

GLB import keeps position and normal shape keys and their initial expression
weights. Give shape keys unique names so reimport can keep their animation
slots when the exporter changes their order.

In **Model → Skinning**, choose a mesh and a bone. The surface shows that bone's
weight from blue at zero through yellow and orange to red at one. Paint adds
weight to the selected bone and distributes the remainder among the other bones.

Painting also works on meshes with morph targets, such as facial expressions.
Each vertex uses the same bone weights in every target. Its target positions
and lighting vectors stay intact. Save keeps the painted model; Undo restores
the complete previous costume.

Morph meshes currently allow 2,000 positions and 2,000 lighting vectors. A
normal, tangent and bitangent count as three vectors. Vertices with different weights can
need separate normal entries; an edit that exceeds capacity leaves the draft
unchanged.

## Reimport an edited model

Select an imported mesh in **Model → Meshes**, then choose **Reimport source…**
in its inspector. Choose the new export and either **Selected mesh** or **All
meshes from this file**. Keep source mesh names and material-slot names unique
so the Workshop can match them to your existing meshes.

The Placement step shows each matched destination and its scale. **Keep edited
materials** retains the materials you changed here. **Replace with source
materials** imports the new export's materials for those meshes. Other meshes
sharing an old material keep it.

Binding starts with **Keep current painted weights**. This carries your weights
over when the source vertex and triangle order is unchanged. For changed
geometry topology, choose **Transfer painted weights from replaced mesh**, or
choose source weights or generated weights to make a new binding.

For named morph targets, **Keep current painted weights** also keeps the target
slots. New targets are added after the existing ones. If an existing target is
missing from the export, the import explains which name is missing and keeps
the draft unchanged. Unnamed target sets need the same count and order.

**Reimport matched meshes** updates the working draft. **Save** keeps the result;
Undo restores the previous geometry, materials and source links. The saved
fighter can be edited and played without the source GLB or glTF file.

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
