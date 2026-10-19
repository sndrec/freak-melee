#include "native_shell_workshop_model_internal.h"
#include "native_shell_workshop.h"
#include <melee/ft/native_fighter_authoring.h>
#ifdef MELEE_NATIVE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/Elements/ElementFormControlSelect.h>
namespace WorkshopModelInternal {
namespace {
bool match_source(char* error,size_t capacity)
{
    for(unsigned i=0;i<active->model.mesh_count;++i)
        if(active->model.meshes[i].offset==active->reimport_anchor)
            return nativeFighterGltf_PlanReimport(active->project,i,active->import_path,
                active->reimport_whole,&active->reimport_plan,error,capacity);
    std::snprintf(error,capacity,"The selected mesh changed. Close this dialog and select it again.");
    return false;
}
std::string material_choice()
{
    return "<label>Materials</label><select id='workshop-model-import-materials' class='room-input'>"
        "<option value='keep'"+std::string(!active->import_replace_material?" selected":"")+
        ">Keep edited materials</option><option value='replace'"+
        std::string(active->import_replace_material?" selected":"")+
        ">Replace with source materials</option></select>"
        "<p>Replacing a material affects the selected destination. Other meshes using its old material keep it.</p>";
}
bool load_source(const char* path)
{
    char error[256]={};NativeGltfScan scan={};
    if(!nativeFighterGltf_Scan(path,&scan,error,sizeof(error))) {
        active->import_scan={};active->reimport_plan={};
        text("workshop-model-import-note",error);return false;
    }
    if(path!=active->import_path)std::snprintf(active->import_path,sizeof(active->import_path),"%s",path);
    active->import_scan=scan;
    std::fill(std::begin(active->import_selected),std::end(active->import_selected),true);
    active->reimport_plan={};
    const bool matched=!active->reimport || match_source(error,sizeof(error));
    render_import();
    if(!matched)text("workshop-model-import-note",error);
    return matched;
}
}
void render_import()
{
    text("workshop-model-import-title",active->reimport?"Reimport model":"Import model");
    for (unsigned i = 0; i < 3; ++i)
        element(("workshop-model-step-" + std::to_string(i)).c_str())
            ->SetClass("selected", active->import_step == i);
    std::string body;
    if (active->import_step == 0) {
        body = "<div class='eyebrow'>SOURCE FILE</div><p>" +
               escape(active->import_path[0] ? active->import_path : "Choose a .glb or .gltf file") +
               "</p><input id='workshop-model-import-path' class='room-input' "
               "type='text' value='" + escape(active->import_path) + "' "
               "placeholder='Path to GLB or glTF'/>"
               "<button id='workshop-model-import-browse' class='room-button'>Browse...</button>"
               "<button id='workshop-model-import-load' class='room-button'>Load file</button>";
        if(active->reimport) {
            body += "<div class='eyebrow'>REIMPORT</div><button id='workshop-model-reimport-one' class='room-button"+
                std::string(!active->reimport_whole?" selected":"")+"'>Selected mesh</button>"
                "<button id='workshop-model-reimport-whole' class='room-button"+
                std::string(active->reimport_whole?" selected":"")+"'>All meshes from this file</button>"
                "<p>Meshes are matched by their source names and material-slot names.</p>";
            for(unsigned i=0;i<active->reimport_plan.count;++i) {
                const auto& match=active->reimport_plan.matches[i];
                body += "<div class='workshop-model-import-row'><span>"+
                    escape(active->import_scan.primitives[match.primitive].name)+" → "+
                    escape(active->model.meshes[match.mesh].name)+"</span></div>";
            }
        } else if (active->import_scan.count) {
            body +=
                "<div class='eyebrow'>MESHES</div><button "
                "id='workshop-import-select-all'>Select all</button><button "
                "id='workshop-import-select-none'>Select none</button>";
            for (unsigned i = 0; i < active->import_scan.count; ++i) {
                const NativeGltfPrimitiveInfo& info =
                    active->import_scan.primitives[i];
                body += "<button id='workshop-model-import-mesh-" +
                        std::to_string(i) +
                        "' class='workshop-model-import-row" +
                        (active->import_selected[i] ? " selected" : "") +
                        "'><span>" + escape(info.name) + "</span><small>" +
                        std::to_string(info.vertices) + " vertices · " +
                        std::to_string(info.triangles) + " triangles" +
                        (info.has_skin ? " · skinned" : "") +
                        (info.has_texture ? " · textured" : "") +
                        "</small></button>";
            }
        }
    } else if (active->import_step == 1 && active->reimport) {
        body="<div class='eyebrow'>MATCHED MESHES</div><p>Each mesh keeps its import scale. "
             "Adjust a scale here to change it.</p>";
        for(unsigned i=0;i<active->reimport_plan.count;++i) {
            const auto& match=active->reimport_plan.matches[i];
            body += "<label>"+escape(active->import_scan.primitives[match.primitive].name)+" → "+
                escape(active->model.meshes[match.mesh].name)+"</label><input id='workshop-model-reimport-scale-"+
                std::to_string(i)+"' class='room-input' type='text' value='"+decimal(match.scale)+"'/>";
        }
        body+=material_choice();
    } else if (active->import_step == 1) {
        body = "<div class='eyebrow'>PLACEMENT</div>"
               "<p>glTF and Melee both use Y as up. Source skin uses its bind "
               "data for placement. Other binding modes use the mesh node's placement.</p>"
               "<label>Scale from glTF units to Melee units</label>"
               "<input id='workshop-model-import-scale' class='room-input' "
               "type='text' value='" + decimal(active->import_scale) + "' />"
               "<label>Destination</label><select id='workshop-model-import-target' "
               "class='room-input'><option value='add'" +
               std::string(active->import_replacement == UINT_MAX ? " selected" : "") +
               ">Add a new mesh</option>";
        for (unsigned i = 0; i < active->model.mesh_count; ++i)
            body += "<option value='" + std::to_string(i) + "'" +
                    (active->import_replacement == i ? " selected" : "") +
                    ">Replace " + escape(active->model.meshes[i].name) + "</option>";
        body += "</select>"+material_choice()+"<p>Adding a new mesh imports its source material.</p>";
    } else {
        body = "<div class='eyebrow'>BINDING</div>";
        body += active->reimport ?
               "<p>Keep current painted weights carries over the weights you edited here. "
               "Source weights uses the skin from your new file. Transfer fits your current weights to changed geometry.</p>" :
               "<p>The cloned fighter's compatible Melee skeleton remains "
               "in place. Source weights match named bones and compatible "
               "bind poses. Generated weights use nearby deform bones. "
               "The joint choice below applies to rigid binding.</p>";
        body += "<label>Binding</label><select id='workshop-model-import-binding' "
               "class='room-input'><option value='auto'" +
               std::string(active->import_binding == NATIVE_GLTF_BIND_AUTOMATIC
                   ? " selected" : "") +
               ">Generate weights from compatible rig</option>";
        body += "<option value='source'" +
                std::string(active->import_binding == NATIVE_GLTF_BIND_SOURCE
                    ? " selected" : "") +
                ">Keep source weights (compatible rig)</option>";
        const bool replacement=active->reimport || active->import_replacement!=UINT_MAX;
        if(replacement) {
            bool correspondence=active->reimport;
            if(!correspondence && active->import_replacement<active->model.mesh_count)
                correspondence=nativeFighterMeshSource_Get(active->project,
                    (uint32_t)active->model.meshes[active->import_replacement].offset)!=nullptr;
            if(correspondence)
                body+="<option value='keep'"+std::string(active->import_binding==NATIVE_GLTF_BIND_KEEP?" selected":"")+
                    ">Keep current painted weights</option>";
            body += "<option value='transfer'" +
                std::string(active->import_binding == NATIVE_GLTF_BIND_TRANSFER
                    ? " selected" : "") +
                ">Transfer painted weights from replaced mesh</option>";
        }
        body +=
               "<option value='rigid'" +
               std::string(active->import_binding == NATIVE_GLTF_BIND_RIGID
                   ? " selected" : "") +
               ">Rigid attachment</option></select>";
        if(!active->reimport) {
            body+="<label>Joint for rigid attachment</label><select id='workshop-model-import-joint' class='room-input'>";
            for (unsigned i = 0; i < active->model.joint_count; ++i)
                body += "<option value='" + std::to_string(i) + "'" +
                        (active->import_joint == i ? " selected" : "") + ">" +
                        escape(active->model.joints[i].name) + "</option>";
            body+="</select>";
        }
        body += "<p class='workshop-model-import-warning'>";
        if(active->reimport) {
            body+=std::to_string(active->reimport_plan.count)+" matched meshes. Current weights require unchanged "
                "source vertex and triangle order. Choose Transfer or a new binding if topology changed. "
                "Each mesh keeps its attachment bone.</p>";
        } else {
        unsigned selected_count = 0;
        bool skin = false, texture = false;
        for (unsigned i = 0; i < active->import_scan.count; ++i) {
            if (active->import_selected[i]) {
                ++selected_count;
                skin |= active->import_scan.primitives[i].has_skin;
                texture |= active->import_scan.primitives[i].has_texture;
            }
        }
        body += std::to_string(selected_count) + " selected meshes. ";
        if (skin) {
            body +=
                "Keep source weights preserves the authored skin; Generate creates a new binding from nearby fighter bones. ";
        }
        if (texture) {
            body += active->import_replacement == UINT_MAX
                        ? "Base-color images and tints are imported. "
                        : "The destination material is retained. ";
        }
        body += "The fighter's existing geometry and rig stay intact "
                "unless you chose Replace.</p>";
        }
    }
    element("workshop-model-import-body")->SetInnerRML(body);
    element("workshop-model-import-back")->SetProperty("display",
        active->import_step ? "block" : "none");
    text("workshop-model-import-next",
         active->import_step == 2 ? (active->reimport?"Reimport matched meshes":"Import selected meshes") : "Continue");
    text("workshop-model-import-note",active->reimport?"Save keeps the reimport. Undo restores the previous meshes.":
         "Native glTF 2.0 import · compatible rig");
}

bool import_click(const char* id)
{
    if(!visible || !active->project)return false;
    unsigned chosen;
    if (!std::strcmp(id, "workshop-import-select-all") ||
        !std::strcmp(id, "workshop-import-select-none"))
    {
        std::fill(std::begin(active->import_selected),
                  std::end(active->import_selected),
                  !std::strcmp(id, "workshop-import-select-all"));
        render_import();
        return true;
    }

    if (!std::strcmp(id,"workshop-model-import") || !std::strcmp(id,"workshop-model-reimport")) {
        active->reimport=!std::strcmp(id,"workshop-model-reimport");
        active->reimport_whole=false;active->reimport_plan={};
        active->import_replace_material=false;
        active->import_binding=active->reimport?NATIVE_GLTF_BIND_KEEP:NATIVE_GLTF_BIND_AUTOMATIC;
        if(active->reimport) {
            if(active->selected_mesh<0 || active->selected_mesh>=active->model.mesh_count)return true;
            active->reimport_anchor=(uint32_t)active->model.meshes[active->selected_mesh].offset;
            const auto* source=nativeFighterMeshSource_Get(active->project,active->reimport_anchor);
            if(!source){status("This mesh has no source link. Import a uniquely named mesh first.");return true;}
            std::snprintf(active->import_path,sizeof(active->import_path),"%s",source->path);
            active->import_replacement=(unsigned)active->selected_mesh;
        } else {
            active->import_replacement=UINT_MAX;
            for(unsigned i=0;i<active->model.joint_count;++i) {
                const char* role=nativeHome_WorkshopBoneRole(i);
                if(role && !std::strcmp(role,"HipN")){active->import_joint=i;break;}
            }
        }
        active->import_visible=true;active->import_step=0;
        element("workshop-model-import-overlay")->SetProperty("display","block");
        render_import();
        if(active->reimport)load_source(active->import_path);
        return true;
    }
    if(!std::strcmp(id,"workshop-model-reimport-one") || !std::strcmp(id,"workshop-model-reimport-whole")) {
        active->reimport_whole=!std::strcmp(id,"workshop-model-reimport-whole");
        const std::string path=input("workshop-model-import-path")->GetValue();
        if(path.empty() || path.size()>=sizeof(active->import_path)) {
            text("workshop-model-import-note","Enter a valid model path.");return true;
        }
        load_source(path.c_str());return true;
    }
    if (std::strcmp(id, "workshop-model-import-close") == 0) {
        active->import_visible = false;
        element("workshop-model-import-overlay")->SetProperty("display", "none");
        return true;
    }
    if (std::strcmp(id, "workshop-model-import-browse") == 0) {
        OPENFILENAMEA picker = {};
        picker.lStructSize = sizeof(picker);
        picker.hwndOwner = GetActiveWindow();
        picker.lpstrFilter = "glTF models (*.glb;*.gltf)\0*.glb;*.gltf\0"
                             "All files (*.*)\0*.*\0\0";
        picker.lpstrFile = active->import_path;
        picker.nMaxFile = sizeof(active->import_path);
        picker.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
                       OFN_NOCHANGEDIR;
        if (GetOpenFileNameA(&picker))load_source(active->import_path);
        return true;
    }
    if (std::strcmp(id, "workshop-model-import-load") == 0) {
        const std::string path = input("workshop-model-import-path")->GetValue();
        if (path.empty() || path.size() >= sizeof(active->import_path)) {
            text("workshop-model-import-note", "Enter a valid model path.");
            return true;
        }
        load_source(path.c_str());
        return true;
    }
    if (index(id, "workshop-model-import-mesh-", &chosen)) {
        if (chosen < active->import_scan.count) {
            active->import_selected[chosen] = !active->import_selected[chosen];
            render_import();
        }
        return true;
    }
    if (std::strcmp(id, "workshop-model-import-back") == 0) {
        if (active->import_step) --active->import_step;
        render_import();
        return true;
    }
    if (std::strcmp(id, "workshop-model-import-next") == 0) {
        if(active->import_step==0) {
            const std::string path=input("workshop-model-import-path")->GetValue();
            if(path.empty() || path.size()>=sizeof(active->import_path)) {
                text("workshop-model-import-note","Enter a valid model path.");return true;
            }
            if((path!=active->import_path || !active->import_scan.count ||
                (active->reimport && !active->reimport_plan.count)) && !load_source(path.c_str()))return true;
        }
        if(!active->import_scan.count || (active->reimport && !active->reimport_plan.count)) {
            text("workshop-model-import-note","Load a source with matching meshes first.");return true;
        }
        if(active->import_step==1 && active->reimport) {
            for(unsigned i=0;i<active->reimport_plan.count;++i) {
                float scale;
                if(!parse_float(("workshop-model-reimport-scale-"+std::to_string(i)).c_str(),&scale) || scale<=0 || scale>1000) {
                    text("workshop-model-import-note","Scale must be greater than zero and at most 1000.");return true;
                }
                active->reimport_plan.matches[i].scale=scale;
            }
        }
        if(active->import_step==1)
            active->import_replace_material=static_cast<Rml::ElementFormControlSelect*>(
                element("workshop-model-import-materials"))->GetValue()=="replace";
        if (active->import_step == 1 && !active->reimport) {
            float parsed;
            if (!parse_float("workshop-model-import-scale", &parsed) ||
                parsed <= 0 || parsed > 1000) {
                text("workshop-model-import-note", "Scale must be greater than zero and at most 1000.");
                return true;
            }
            active->import_scale = parsed;
            const std::string target = static_cast<
                Rml::ElementFormControlSelect*>(
                    element("workshop-model-import-target"))->GetValue();
            active->import_replacement = target == "add" ? UINT_MAX :
                                 (unsigned) std::strtoul(target.c_str(),
                                                         nullptr, 10);
            if (active->import_replacement < active->model.mesh_count)
                active->import_joint = active->model.meshes[active->import_replacement].joint;
        }
        if (active->import_step < 2) {
            if (active->import_step == 1 && !active->reimport) {
                bool selected = false, all_skinned = true;
                for (unsigned i = 0; i < active->import_scan.count; ++i)
                    if (active->import_selected[i]) {
                        selected = true;
                        all_skinned &= active->import_scan.primitives[i].has_skin;
                    }
                const bool linked=active->import_replacement<active->model.mesh_count &&
                    nativeFighterMeshSource_Get(active->project,(uint32_t)active->model.meshes[active->import_replacement].offset);
                active->import_binding = linked?NATIVE_GLTF_BIND_KEEP:selected && all_skinned
                    ? NATIVE_GLTF_BIND_SOURCE : NATIVE_GLTF_BIND_AUTOMATIC;
            }
            ++active->import_step;
            render_import();
            return true;
        }
        if(!active->reimport)active->import_joint = (unsigned) std::strtoul(static_cast<
            Rml::ElementFormControlSelect*>(
                element("workshop-model-import-joint"))->GetValue().c_str(),
                nullptr, 10);
        const std::string binding_value = static_cast<
            Rml::ElementFormControlSelect*>(
                element("workshop-model-import-binding"))->GetValue();
        active->import_binding = binding_value == "keep" ? NATIVE_GLTF_BIND_KEEP : binding_value == "transfer"
            ? NATIVE_GLTF_BIND_TRANSFER : binding_value == "auto"
            ? NATIVE_GLTF_BIND_AUTOMATIC : binding_value == "source"
            ? NATIVE_GLTF_BIND_SOURCE : NATIVE_GLTF_BIND_RIGID;
        char error[192] = {};
        std::vector<uint32_t> previous_mesh_offsets;
        previous_mesh_offsets.reserve(active->model.mesh_count);
        for (unsigned i = 0; i < active->model.mesh_count; ++i)
            previous_mesh_offsets.push_back(active->model.meshes[i].offset);
        const uint32_t destination_offset = active->import_replacement < active->model.mesh_count
            ? active->model.meshes[active->import_replacement].offset : 0;
        const std::vector<uint8_t> before(active->project->costume_dat,
            active->project->costume_dat + active->project->costume_size);
        const std::vector<uint8_t> fighter(active->project->fighter_dat,active->project->fighter_dat+active->project->fighter_size);
        auto before_labels = label_snapshot();
        NativeGltfImportRequest requests[NATIVE_GLTF_MAX_PRIMITIVES];
        unsigned count = 0;
        if(active->reimport) {
            for(unsigned i=0;i<active->reimport_plan.count;++i) {
                const auto& match=active->reimport_plan.matches[i];
                requests[count++]={match.primitive,match.joint,match.mesh,active->import_binding,
                    match.scale,active->import_replace_material};
            }
        } else for (unsigned i = 0; i < active->import_scan.count; ++i) {
            if (active->import_selected[i]) {
                requests[count++] = { i, active->import_joint,active->import_replacement,
                    active->import_binding, active->import_scale, active->import_replace_material };
            }
        }
        if (!count || (!active->reimport && count > 1 && active->import_replacement != UINT_MAX)) {
            text("workshop-model-import-note",
                 "Select one mesh to replace an existing mesh, or choose Add "
                 "for a multiple-mesh import.");
            return true;
        }
        if (!nativeFighterGltf_ApplyBatch(
                active->project, active->import_path, requests, count,
                error, sizeof(error)))
        {
            text("workshop-model-import-note", error[0] ? error :
                 "Could not convert this mesh to Melee model data.");
            return true;
        }
        const bool asset_changed=record_asset(before,fighter);
        record_label_change(before_labels, asset_changed);
        reopen_model();
        for (unsigned i = 0; i < active->model.mesh_count; ++i)
            if (destination_offset
                    ? active->model.meshes[i].offset == destination_offset
                    : std::find(previous_mesh_offsets.begin(),
                                previous_mesh_offsets.end(),
                                active->model.meshes[i].offset) ==
                          previous_mesh_offsets.end()) {
                active->selected_mesh = (int) i;
                active->selected_material = active->model.meshes[i].material;
                render();
                break;
            }
        active->import_visible = false;
        element("workshop-model-import-overlay")->SetProperty("display", "none");
        const bool preview = nativeShellWorkshop_RefreshPreview();
        status(active->reimport ? "Meshes reimported. Save to keep the changes; Undo restores the previous meshes." :
            preview ? "Mesh imported; refreshing the live model preview." : "Mesh imported. Test Character to reload the model.");
        return true;
    }
    return false;
}
}
#endif
