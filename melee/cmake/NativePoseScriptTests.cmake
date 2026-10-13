# Direct frontend dependencies belong to every console fixture using the actual
# compiler. Engine-free fixtures explicitly reject the unavailable pose leaf.
get_property(POSE_SCRIPT_TARGETS DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(target IN LISTS POSE_SCRIPT_TARGETS)
    get_target_property(pose_sources ${target} SOURCES)
    if(("src/melee/ft/native_script_bindings.c" IN_LIST pose_sources OR
        "src/melee/ft/native_script_compile.c" IN_LIST pose_sources OR
        "src/melee/ft/native_script_item_program.c" IN_LIST pose_sources) AND
       NOT "src/melee/ft/native_script_binding_spawn_catalog.c" IN_LIST pose_sources)
        target_sources(${target} PRIVATE src/melee/ft/native_script_binding_spawn_catalog.c)
    endif()
    if("src/melee/ft/native_script_item_program.c" IN_LIST pose_sources AND
       NOT "src/melee/ft/native_script_resource_metadata.c" IN_LIST pose_sources)
        target_sources(${target} PRIVATE src/melee/ft/native_script_resource_metadata.c)
    endif()
    if(("src/melee/ft/native_script_compile.c" IN_LIST pose_sources OR
        "src/melee/ft/native_script_item_program.c" IN_LIST pose_sources) AND
       NOT "src/melee/ft/native_script_binding_item_resources_resolve.c" IN_LIST pose_sources)
        target_sources(${target} PRIVATE src/melee/ft/native_script_binding_item_resources_resolve.c)
    endif()
    if(("src/melee/ft/native_script_bindings.c" IN_LIST pose_sources OR
        "src/melee/ft/native_script_binding_properties.c" IN_LIST pose_sources OR
        "src/melee/ft/native_script_compile.c" IN_LIST pose_sources OR
        "src/melee/ft/native_script_item_program.c" IN_LIST pose_sources) AND
       NOT "src/melee/ft/native_script_binding_item_resources.c" IN_LIST pose_sources)
        target_sources(${target} PRIVATE src/melee/ft/native_script_binding_item_resources_catalog.c
            tests/native/script_bindings_unavailable_item_resources.c)
    endif()
    if("src/melee/ft/native_script_bindings.c" IN_LIST pose_sources AND
       NOT "src/melee/ft/native_script_binding_colors.c" IN_LIST pose_sources AND
       NOT "tests/native/script_bindings_unavailable_colors.c" IN_LIST pose_sources)
        target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_colors.c)
    endif()
    if("src/melee/ft/native_script_bindings.c" IN_LIST pose_sources AND
       NOT "src/melee/ft/native_script_binding_fighter_items.c" IN_LIST pose_sources AND
       NOT "tests/native/script_bindings_unavailable_fighter_items.c" IN_LIST pose_sources)
        target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_fighter_items.c)
    endif()
    foreach(family held_items absorber ground_controls fighter_hitboxes item_observations item_collision item_terrain capture hitlag stage family audio model_graph attachments)
        if(("src/melee/ft/native_script_bindings.c" IN_LIST pose_sources OR
            "src/melee/ft/native_script_binding_properties.c" IN_LIST pose_sources) AND
           NOT "src/melee/ft/native_script_binding_${family}.c" IN_LIST pose_sources AND
           NOT "tests/native/script_bindings_unavailable_${family}.c" IN_LIST pose_sources)
            target_sources(${target} PRIVATE "tests/native/script_bindings_unavailable_${family}.c")
        endif()
    endforeach()
    if("src/melee/ft/native_script_item_program.c" IN_LIST pose_sources)
        if(NOT "src/melee/ft/native_script_binding_item_animation_prepare.c" IN_LIST pose_sources)
            target_sources(${target} PRIVATE src/melee/ft/native_script_binding_item_animation_prepare.c)
        endif()
        foreach(unit native_source_declarations native_source_asset_validation ftcharacter_asset_layout native_fighter_sound_policy native_fighter_pose_path native_source_motion_policy)
            if(NOT "src/melee/ft/${unit}.c" IN_LIST pose_sources)
                target_sources(${target} PRIVATE "src/melee/ft/${unit}.c")
            endif()
        endforeach()
        foreach(unit native_article_animation_demand native_article_animation_origins native_article_animation_texture)
            if(NOT "src/melee/it/${unit}.c" IN_LIST pose_sources)
                target_sources(${target} PRIVATE "src/melee/it/${unit}.c")
            endif()
        endforeach()
        get_target_property(pose_sources ${target} SOURCES)
    endif()
    if("src/melee/ft/native_script_bindings.c" IN_LIST pose_sources AND
       NOT "src/melee/ft/native_script_binding_normal_attacks.c" IN_LIST pose_sources AND
       NOT "tests/native/script_bindings_unavailable_normal_attacks.c" IN_LIST pose_sources)
        target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_normal_attacks.c)
    endif()
    if(("src/melee/ft/native_script_fighter_actor.c" IN_LIST pose_sources OR
        "src/melee/ft/native_script_fighter_program.c" IN_LIST pose_sources) AND
       NOT "src/melee/ft/native_script_fighter_hooks.c" IN_LIST pose_sources)
        target_sources(${target} PRIVATE src/melee/ft/native_script_fighter_hooks.c)
    endif()
    if("src/melee/ft/native_script_receiver.c" IN_LIST pose_sources AND
       NOT "src/melee/it/native_script_item_actor.c" IN_LIST pose_sources AND
       NOT target STREQUAL "native_script_receiver_test")
        target_sources(${target} PRIVATE tests/native/script_receiver_unavailable_item_relations.c)
    endif()
    if(("src/melee/ft/native_script_compile.c" IN_LIST pose_sources OR
        "src/melee/ft/native_script_binding_item_model_resolve.c" IN_LIST pose_sources OR
        "src/melee/ft/native_source_entity_models.c" IN_LIST pose_sources OR
        "src/melee/ft/native_script_item_program.c" IN_LIST pose_sources) AND
       NOT "src/melee/ft/native_script_model_metadata.c" IN_LIST pose_sources)
        target_sources(${target} PRIVATE src/melee/ft/native_script_model_metadata.c)
    endif()
    if("src/melee/ft/native_script_compile.c" IN_LIST pose_sources AND
       NOT "src/melee/ft/native_script_binding_item_model_resolve.c" IN_LIST pose_sources)
        target_sources(${target} PRIVATE src/melee/ft/native_script_binding_item_model_resolve.c)
    endif()
    if(("src/melee/ft/native_script_compile.c" IN_LIST pose_sources OR
        "src/melee/ft/native_script_binding_item_model_resolve.c" IN_LIST pose_sources) AND
       NOT "src/melee/it/native_article_animation_demand.c" IN_LIST pose_sources)
        target_sources(${target} PRIVATE src/melee/it/native_article_animation_demand.c)
    endif()
    get_target_property(pose_sources ${target} SOURCES)
    if(("src/melee/it/native_article_animation_demand.c" IN_LIST pose_sources OR
        "src/melee/it/native_article_animation_origins.c" IN_LIST pose_sources OR
        "src/melee/it/native_article_animation_live.c" IN_LIST pose_sources) AND
       NOT "src/melee/it/native_article_animation_texture.c" IN_LIST pose_sources)
        target_sources(${target} PRIVATE src/melee/it/native_article_animation_texture.c)
    endif()
    if("src/melee/ft/native_script_compile.c" IN_LIST pose_sources AND
       NOT "src/melee/ft/native_script_binding_article_reference_resolve.c" IN_LIST pose_sources)
        target_sources(${target} PRIVATE src/melee/ft/native_script_binding_article_reference_resolve.c)
    endif()
    if("src/melee/ft/native_script_binding_receiver.c" IN_LIST pose_sources AND
       NOT "src/melee/ft/native_script_binding_article_reference.c" IN_LIST pose_sources AND
       NOT "tests/native/script_bindings_unavailable_article_reference.c" IN_LIST pose_sources)
        target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_article_reference.c)
    endif()
    if("src/melee/ft/native_script_compile.c" IN_LIST pose_sources AND
       NOT "src/melee/ft/native_script_binding_signal_resolve.c" IN_LIST pose_sources)
        target_sources(${target} PRIVATE src/melee/ft/native_script_binding_signal_resolve.c)
    endif()
    if("src/melee/ft/native_script_compile.c" IN_LIST pose_sources AND
       NOT "src/melee/ft/native_script_hitbox_metadata.c" IN_LIST pose_sources)
        target_sources(${target} PRIVATE src/melee/ft/native_script_hitbox_metadata.c)
    endif()
    if("src/melee/ft/native_script_compile.c" IN_LIST pose_sources AND
       NOT "src/melee/ft/native_script_pose_metadata.c" IN_LIST pose_sources)
        target_sources(${target} PRIVATE src/melee/ft/native_script_pose_metadata.c)
    endif()
    if(("src/melee/ft/native_script_compile.c" IN_LIST pose_sources OR
        "src/melee/ft/native_fighter_pose.c" IN_LIST pose_sources) AND
       NOT "src/melee/ft/native_fighter_pose_path.c" IN_LIST pose_sources)
        target_sources(${target} PRIVATE src/melee/ft/native_fighter_pose_path.c)
    endif()
    if("src/melee/ft/native_script_bindings.c" IN_LIST pose_sources AND
       NOT "src/melee/ft/native_script_binding_pose.c" IN_LIST pose_sources AND
       NOT "tests/native/script_bindings_unavailable_pose.c" IN_LIST pose_sources)
        target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_pose.c)
    endif()
    if("src/melee/ft/native_script_bindings.c" IN_LIST pose_sources)
        foreach(family air_inputs air_physics fighter_observations contact held_state hitbox_create)
            if(NOT "src/melee/ft/native_script_binding_${family}.c" IN_LIST pose_sources AND
               NOT "tests/native/script_bindings_unavailable_${family}.c" IN_LIST pose_sources)
                target_sources(${target} PRIVATE "tests/native/script_bindings_unavailable_${family}.c")
            endif()
        endforeach()
        if(NOT "src/melee/ft/native_script_binding_guard.c" IN_LIST pose_sources AND
           NOT "tests/native/script_bindings_unavailable_guard.c" IN_LIST pose_sources)
            target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_guard.c)
        endif()
        if(NOT "src/melee/ft/native_script_binding_article_motion.c" IN_LIST pose_sources AND
           NOT "tests/native/script_bindings_unavailable_article_motion.c" IN_LIST pose_sources)
            target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_article_motion.c)
        endif()
        if(NOT "src/melee/ft/native_script_binding_world_feedback.c" IN_LIST pose_sources AND
           NOT "tests/native/script_bindings_unavailable_world_feedback.c" IN_LIST pose_sources)
            target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_world_feedback.c)
        endif()
        if(NOT "src/melee/ft/native_script_binding_item_lifecycle.c" IN_LIST pose_sources AND
           NOT "tests/native/script_bindings_unavailable_item_lifecycle.c" IN_LIST pose_sources)
            target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_item_lifecycle.c)
        endif()
        if(NOT "src/melee/ft/native_script_binding_item_model.c" IN_LIST pose_sources AND
           NOT "tests/native/script_bindings_unavailable_item_model.c" IN_LIST pose_sources)
            target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_item_model.c)
        endif()
        if(NOT "src/melee/ft/native_script_binding_item_animation.c" IN_LIST pose_sources AND
           NOT "tests/native/script_bindings_unavailable_item_animation.c" IN_LIST pose_sources)
            target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_item_animation.c)
        endif()
        if(NOT "src/melee/ft/native_script_binding_article_combat.c" IN_LIST pose_sources AND
           NOT "tests/native/script_bindings_unavailable_article_combat.c" IN_LIST pose_sources)
            target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_article_combat.c)
        endif()
        if(NOT "src/melee/ft/native_script_binding_targets.c" IN_LIST pose_sources)
            target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_targets.c)
        endif()
        if(NOT "src/melee/ft/native_script_binding_cpu_control.c" IN_LIST pose_sources)
            target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_cpu_control.c)
        endif()
        if(NOT "src/melee/ft/native_script_binding_article_lifecycle.c" IN_LIST pose_sources)
            target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_article_lifecycle.c)
        endif()
        if(NOT "src/melee/ft/native_script_binding_vectors.c" IN_LIST pose_sources AND
           NOT "tests/native/script_bindings_unavailable_vectors.c" IN_LIST pose_sources)
            target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_vectors.c)
        endif()
        if(NOT "src/melee/ft/native_script_binding_signal.c" IN_LIST pose_sources AND
           NOT "tests/native/script_bindings_unavailable_signal.c" IN_LIST pose_sources)
            target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_signal.c)
        endif()
        if(NOT "src/melee/ft/native_script_binding_article_feedback.c" IN_LIST pose_sources)
            target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_article_feedback.c)
        endif()
        if(NOT "src/melee/ft/native_script_binding_common_movement.c" IN_LIST pose_sources)
            target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_common_movement.c)
        endif()
        if(NOT "src/melee/ft/native_script_binding_visuals.c" IN_LIST pose_sources)
            target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_visuals.c)
        endif()
    endif()
    if("src/melee/ft/native_script_binding_properties.c" IN_LIST pose_sources)
        if(NOT "src/melee/it/native_script_item_actor.c" IN_LIST pose_sources AND
           NOT "tests/native/script_bindings_unavailable_item_birth.c" IN_LIST pose_sources)
            target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_item_birth.c)
        endif()
        if(target STREQUAL "native_script_native_bindings_test")
            target_sources(${target} PRIVATE src/melee/ft/native_script_binding_common_properties.c)
        elseif(NOT "src/melee/ft/native_script_binding_common_properties.c" IN_LIST pose_sources)
            target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_common_properties.c)
        endif()
    endif()
endforeach()

set(COMMON_SCALE_REFERENCE "${SCRIPT_KINETICS_FIXTURE_DIR}/common_scale_reference.inc")
add_custom_command(OUTPUT "${COMMON_SCALE_REFERENCE}"
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/script_native_bindings/extract_scale.py"
        --root "${CMAKE_CURRENT_SOURCE_DIR}" --output "${COMMON_SCALE_REFERENCE}"
    DEPENDS tests/native/script_native_bindings/extract_scale.py src/melee/ft/ftcommon_data.c src/melee/ft/inlines.h VERBATIM)
target_sources(native_script_native_bindings_test PRIVATE "${COMMON_SCALE_REFERENCE}")

set(SCRIPT_POSE_RUNTIME_DIR "${CMAKE_CURRENT_BINARY_DIR}/script_pose_runtime")
set(SCRIPT_POSE_RUNTIME_CONTRACT "${SCRIPT_POSE_RUNTIME_DIR}/source_contract.inc")
add_custom_command(OUTPUT "${SCRIPT_POSE_RUNTIME_CONTRACT}"
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/script_pose_runtime/run.py"
        --extract-only --output "${SCRIPT_POSE_RUNTIME_DIR}"
    DEPENDS tests/native/script_pose_runtime/run.py src/melee/ft/native_script_bindings.c
        src/melee/ft/native_script_binding_properties.c VERBATIM)
add_executable(native_script_pose_runtime_test tests/native/script_pose_runtime/test.c
    "${SCRIPT_POSE_RUNTIME_CONTRACT}" src/melee/ft/native_script_binding_pose.c
    src/melee/ft/native_callback_lifetime.c src/melee/gm/native_match_pool.c)
target_include_directories(native_script_pose_runtime_test PRIVATE src extern/dolphin/include "${SCRIPT_POSE_RUNTIME_DIR}")
target_compile_definitions(native_script_pose_runtime_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
target_compile_options(native_script_pose_runtime_test PRIVATE -ffp-contract=off)
add_test(NAME native_script_pose_runtime COMMAND native_script_pose_runtime_test)
