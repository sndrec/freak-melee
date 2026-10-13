# Traversal scratch belongs to the caller, not the retained match state.
add_executable(native_fighter_animation_traversal_test
    tests/native/fighter_animation_traversal_test.c src/melee/ft/ftanim_tree.c)
target_include_directories(native_fighter_animation_traversal_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_fighter_animation_traversal_test PRIVATE MELEE_NATIVE=1)
add_test(NAME native_fighter_animation_traversal COMMAND native_fighter_animation_traversal_test)

# Generate both module forms and execute the authored pose calls without a game.
set(FIGHTER_POSE_DIR "${CMAKE_CURRENT_BINARY_DIR}/fighter_pose_graph_compiler")
set(FIGHTER_POSE_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/fighter_pose_graph_compiler")
set(FIGHTER_POSE_ACTION "${FIGHTER_POSE_DIR}/action/pose_probe.inc")
set(FIGHTER_POSE_STATE "${FIGHTER_POSE_DIR}/state/pose_probe.inc")
add_executable(native_script_fighter_pose_compiler_test
    tests/native/fighter_pose_graph_compiler/compiler.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_fighter_pose_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_fighter_pose_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_custom_command(OUTPUT "${FIGHTER_POSE_ACTION}" "${FIGHTER_POSE_STATE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${FIGHTER_POSE_DIR}/action" "${FIGHTER_POSE_DIR}/state"
    COMMAND $<TARGET_FILE:native_script_fighter_pose_compiler_test>
        "${FIGHTER_POSE_SOURCE}/probe.fms" "${FIGHTER_POSE_ACTION}" "${FIGHTER_POSE_STATE}"
    DEPENDS native_script_fighter_pose_compiler_test tests/native/fighter_pose_graph_compiler/probe.fms VERBATIM)
add_test(NAME native_script_fighter_pose_compiler
    COMMAND native_script_fighter_pose_compiler_test "${FIGHTER_POSE_SOURCE}/probe.fms"
        "${FIGHTER_POSE_DIR}/ctest_action.inc" "${FIGHTER_POSE_DIR}/ctest_state.inc")
foreach(form IN ITEMS action state)
    add_executable(native_script_fighter_pose_${form}_test
        tests/native/fighter_pose_graph_compiler/module.c "${FIGHTER_POSE_ACTION}" "${FIGHTER_POSE_STATE}")
    target_include_directories(native_script_fighter_pose_${form}_test PRIVATE
        "${FIGHTER_POSE_DIR}/${form}" src extern/dolphin/include)
    target_compile_definitions(native_script_fighter_pose_${form}_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    add_test(NAME native_script_fighter_pose_${form} COMMAND native_script_fighter_pose_${form}_test)
endforeach()

# Exercise the real ownership resolver, pure validator and shared ftAnim bodies.
set(FIGHTER_POSE_RUNTIME "${CMAKE_CURRENT_BINARY_DIR}/fighter_pose_graph_runtime")
set(FIGHTER_POSE_RUNTIME_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/fighter_pose_graph_runtime")
set(FIGHTER_POSE_RUNTIME_BODIES "${FIGHTER_POSE_RUNTIME}/runtime.inc" "${FIGHTER_POSE_RUNTIME}/shared.inc")
add_custom_command(OUTPUT ${FIGHTER_POSE_RUNTIME_BODIES} "${FIGHTER_POSE_RUNTIME}/sources.json"
    COMMAND Python3::Interpreter "${FIGHTER_POSE_RUNTIME_SOURCE}/fixture.py"
        --root "${CMAKE_CURRENT_SOURCE_DIR}" --output "${FIGHTER_POSE_RUNTIME}"
    DEPENDS tests/native/fighter_pose_graph_runtime/fixture.py
        src/melee/ft/native_fighter_runtime.c src/melee/ft/ftanim_tracks.c
        src/melee/ft/ftanim_transition.c src/melee/ft/ftanim_skeleton.c src/melee/ft/ftanim_tree.c
        src/melee/ft/ftanim.c VERBATIM)
add_executable(native_engine_fighter_pose_test
    tests/native/fighter_pose_graph_runtime/test.c ${FIGHTER_POSE_RUNTIME_BODIES}
    src/melee/ft/native_fighter_pose.c src/melee/ft/native_fighter_pose_path.c src/melee/ft/native_callback_lifetime.c src/melee/ft/ftparts_table.c)
target_include_directories(native_engine_fighter_pose_test PRIVATE
    "${FIGHTER_POSE_RUNTIME}" src extern/dolphin/include)
target_compile_definitions(native_engine_fighter_pose_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_test(NAME native_engine_fighter_pose COMMAND native_engine_fighter_pose_test)

# Import complete retail graphs, all costumes and Chain's own animation clips.
set(FIGHTER_POSE_RETAIL "${CMAKE_CURRENT_BINARY_DIR}/fighter_pose_graph_retail")
set(FIGHTER_POSE_RETAIL_BODIES "${FIGHTER_POSE_RETAIL}/runtime.inc"
    "${FIGHTER_POSE_RETAIL}/shared.inc" "${FIGHTER_POSE_RETAIL}/rig.inc"
    "${FIGHTER_POSE_RETAIL}/retail_setup.inc")
add_custom_command(OUTPUT ${FIGHTER_POSE_RETAIL_BODIES}
    COMMAND Python3::Interpreter "${FIGHTER_POSE_RUNTIME_SOURCE}/fixture.py"
        --root "${CMAKE_CURRENT_SOURCE_DIR}" --output "${FIGHTER_POSE_RETAIL}"
    COMMAND Python3::Interpreter "${FIGHTER_POSE_RUNTIME_SOURCE}/retail_fixture.py"
        --root "${CMAKE_CURRENT_SOURCE_DIR}" --iso "${MELEE_ISO}" --output "${FIGHTER_POSE_RETAIL}"
    DEPENDS tests/native/fighter_pose_graph_runtime/fixture.py
        tests/native/fighter_pose_graph_runtime/retail_fixture.py "${MELEE_ISO}"
        src/melee/ft/native_fighter_runtime.c src/melee/ft/ftanim_tracks.c
        src/melee/ft/ftanim_transition.c src/melee/ft/ftanim_skeleton.c
        src/melee/ft/ftanim_tree.c src/melee/ft/ftanim.c
        src/melee/ft/ftparts.c src/melee/ft/ftparts_bind.c VERBATIM)
add_executable(native_engine_fighter_pose_retail_test
    tests/native/fighter_pose_graph_runtime/retail_test.c ${FIGHTER_POSE_RETAIL_BODIES}
    src/melee/ft/native_fighter_pose.c src/melee/ft/native_fighter_pose_path.c src/melee/ft/native_callback_lifetime.c src/melee/ft/ftparts_table.c)
target_include_directories(native_engine_fighter_pose_retail_test PRIVATE
    "${FIGHTER_POSE_RETAIL}" src extern/dolphin/include)
target_compile_definitions(native_engine_fighter_pose_retail_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
target_link_libraries(native_engine_fighter_pose_retail_test PRIVATE
    melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx
    aurora_dvd aurora_card aurora_thp)
target_link_options(native_engine_fighter_pose_retail_test PRIVATE "LINKER:/opt:ref")
aurora_copy_runtime_dlls(native_engine_fighter_pose_retail_test)
add_test(NAME native_engine_fighter_pose_retail COMMAND
    native_engine_fighter_pose_retail_test "${FIGHTER_POSE_RETAIL}")
