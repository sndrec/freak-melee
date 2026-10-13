add_executable(native_script_item_pause_compiler_test
    tests/native/script_item_pause_compiler_test.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_item_pause_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_item_pause_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_script_item_pause_queries
    COMMAND Python3::Interpreter
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/native/script_item_pause_source_test.py
        --compiler ${CMAKE_C_COMPILER} --sdk ${CMAKE_CURRENT_SOURCE_DIR}
        --generator $<TARGET_FILE:native_script_item_pause_compiler_test>
        --output ${CMAKE_CURRENT_BINARY_DIR}/script_item_pause_queries)

set(ITEM_OWNER_FIXTURE_DIR "${CMAKE_CURRENT_BINARY_DIR}/item_owner_fixture")
set(ITEM_OWNER_FIXTURE "${ITEM_OWNER_FIXTURE_DIR}/item_owner_actual.inc")
add_custom_command(OUTPUT "${ITEM_OWNER_FIXTURE}"
    COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/tests/native/item_owner_fixture.py
        --root ${CMAKE_CURRENT_SOURCE_DIR} --output "${ITEM_OWNER_FIXTURE_DIR}"
    DEPENDS tests/native/item_owner_fixture.py src/melee/ft/ftlib.c
        src/melee/it/native_article.c src/melee/it/it_2725.c src/melee/it/item.c
        src/melee/it/native_item_owner.c src/melee/it/native_item_owner.h
        src/melee/it/types.h src/melee/ft/native_callback_lifetime.c VERBATIM)
add_executable(native_item_owner_test tests/native/item_owner_test.c
    "${ITEM_OWNER_FIXTURE}" src/melee/it/native_item_owner.c
    src/melee/ft/native_callback_lifetime.c src/melee/gm/native_match_pool.c)
target_include_directories(native_item_owner_test PRIVATE
    "${ITEM_OWNER_FIXTURE_DIR}" src extern/dolphin/include)
target_compile_definitions(native_item_owner_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_test(NAME native_item_owner COMMAND native_item_owner_test)

set(COMMON_ENTRY_FIXTURE_DIR "${CMAKE_CURRENT_BINARY_DIR}/common_entry_fixture")
set(COMMON_ENTRY_FIXTURE "${COMMON_ENTRY_FIXTURE_DIR}/common_entry_actual.inc")
add_custom_command(OUTPUT "${COMMON_ENTRY_FIXTURE}"
    COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/tests/native/common_entry_fixture.py
        --root ${CMAKE_CURRENT_SOURCE_DIR} --output "${COMMON_ENTRY_FIXTURE_DIR}"
    DEPENDS tests/native/common_entry_fixture.py tests/native/item_owner_fixture.py
        src/melee/ft/ft_08A1.c src/melee/ft/kinds/ftCommon/ftCo_SquatWait.c
        src/melee/ft/kinds/ftCommon/ftCo_AttackAir.c src/melee/ft/native_fighter_visuals.c
        src/melee/ft/kinds/ftCommon/ftCo_AttackS4.c
        src/melee/ft/kinds/ftCommon/ftCo_AttackHi4.c
        src/melee/ft/kinds/ftCommon/ftCo_AttackLw4.c
        src/melee/ft/fighter.c VERBATIM)
add_executable(native_common_entry_test tests/native/common_entry_test.c "${COMMON_ENTRY_FIXTURE}"
    src/melee/ft/native_callback_lifetime.c tests/native/model_effect_absent_fixture.c)
target_include_directories(native_common_entry_test PRIVATE
    "${COMMON_ENTRY_FIXTURE_DIR}" src extern/dolphin/include)
target_compile_definitions(native_common_entry_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_common_entry COMMAND native_common_entry_test)

add_executable(native_held_article_backend_test tests/native/held_article_backend_test.c
    src/melee/gm/native_match_pool.c
    src/melee/it/native_held_article.c src/melee/ft/native_callback_lifetime.c
    tests/native/model_effect_absent_fixture.c)
target_include_directories(native_held_article_backend_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_held_article_backend_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_held_article_backend COMMAND native_held_article_backend_test)

add_executable(native_held_article_protocol_compiler_test
    tests/native/held_article_protocol/compiler_test.c ${NATIVE_FIGHTER_SCRIPT_SOURCES}
    src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_held_article_protocol_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_held_article_protocol_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
set(HELD_PROTOCOL_FIXTURE_DIR "${CMAKE_CURRENT_BINARY_DIR}/held_article_protocol")
set(HELD_PROTOCOL_FIXTURE "${HELD_PROTOCOL_FIXTURE_DIR}/held_article_protocol_fixture.h")
add_custom_command(OUTPUT "${HELD_PROTOCOL_FIXTURE}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${HELD_PROTOCOL_FIXTURE_DIR}"
    COMMAND $<TARGET_FILE:native_held_article_protocol_compiler_test>
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/held_article_protocol/fixture.fms"
        "${HELD_PROTOCOL_FIXTURE}"
    DEPENDS native_held_article_protocol_compiler_test tests/native/held_article_protocol/fixture.fms
    VERBATIM)
add_executable(native_held_article_protocol_module_test
    tests/native/held_article_protocol/module_test.c "${HELD_PROTOCOL_FIXTURE}")
target_include_directories(native_held_article_protocol_module_test PRIVATE
    "${HELD_PROTOCOL_FIXTURE_DIR}" src extern/dolphin/include)
target_compile_definitions(native_held_article_protocol_module_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_held_article_protocol_compiler COMMAND native_held_article_protocol_compiler_test
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/held_article_protocol/fixture.fms")
add_test(NAME native_held_article_protocol_module COMMAND native_held_article_protocol_module_test)

add_executable(native_aerial_policy_test tests/native/aerial_policy_test.c
    src/melee/ft/kinds/ftCommon/ftCo_AerialAction.c
    src/melee/ft/native_fighter_contact.c src/melee/ft/native_callback_lifetime.c
    src/melee/gm/native_match_pool.c)
target_include_directories(native_aerial_policy_test PRIVATE tests/native src extern/dolphin/include)
target_compile_definitions(native_aerial_policy_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_aerial_policy COMMAND native_aerial_policy_test)

set(MODEL_GRAPH_FIXTURE_DIR "${CMAKE_CURRENT_BINARY_DIR}/model_graph_fixture")
set(MODEL_GRAPH_FIXTURE "${MODEL_GRAPH_FIXTURE_DIR}/model_graph_actual.inc")
add_custom_command(OUTPUT "${MODEL_GRAPH_FIXTURE}"
    COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/tests/native/model_graph_fixture.py
        --root ${CMAKE_CURRENT_SOURCE_DIR} --output "${MODEL_GRAPH_FIXTURE_DIR}"
    DEPENDS tests/native/model_graph_fixture.py tests/native/item_owner_fixture.py
        src/melee/ft/ftparts.c src/melee/ft/ftanim_tree.c src/melee/ft/native_fighter_module.c VERBATIM)
add_executable(native_model_graph_test tests/native/model_graph_test.c "${MODEL_GRAPH_FIXTURE}"
    src/melee/ft/native_fighter_model_graph.c)
target_include_directories(native_model_graph_test PRIVATE
    "${MODEL_GRAPH_FIXTURE_DIR}" src extern/dolphin/include)
target_compile_definitions(native_model_graph_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_model_graph COMMAND native_model_graph_test)

add_executable(native_script_helper_capacity_compiler_test
    tests/native/script_helper_capacity_compiler_test.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_helper_capacity_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_helper_capacity_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_script_helper_capacity_compiler COMMAND native_script_helper_capacity_compiler_test)

add_executable(native_parameter_capacity_compiler_test
    tests/native/parameter_capacity_compiler_test.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c
    src/melee/ft/native_fighter_parameter_import.c
    src/melee/ft/native_fighter_article_bindings.c src/sysdolphin/baselib/dat.c)
target_include_directories(native_parameter_capacity_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_parameter_capacity_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_parameter_capacity_compiler COMMAND native_parameter_capacity_compiler_test)
set(PARAMETER_ACTION_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/parameter_capacity_action_fixture.h")
set(PARAMETER_CALLBACK_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/parameter_capacity_callback_fixture.h")
add_custom_command(OUTPUT "${PARAMETER_ACTION_FIXTURE}" "${PARAMETER_CALLBACK_FIXTURE}"
    COMMAND $<TARGET_FILE:native_parameter_capacity_compiler_test>
        "${PARAMETER_ACTION_FIXTURE}" "${PARAMETER_CALLBACK_FIXTURE}"
    DEPENDS native_parameter_capacity_compiler_test VERBATIM)

add_executable(native_interpreter_evaluation_runtime_test
    tests/native/interpreter_evaluation/vm_runtime.c
    tests/native/interpreter_evaluation/runtime_test.c)
target_compile_definitions(native_interpreter_evaluation_runtime_test PRIVATE _CRT_SECURE_NO_WARNINGS)
target_compile_options(native_interpreter_evaluation_runtime_test PRIVATE -ffp-contract=off)
add_test(NAME native_interpreter_evaluation_runtime COMMAND native_interpreter_evaluation_runtime_test)

add_executable(native_script_expression_ir_test
    tests/native/script_expression_ir/test.c src/melee/ft/native_script_expression.c)
target_include_directories(native_script_expression_ir_test PRIVATE src)
target_compile_definitions(native_script_expression_ir_test PRIVATE _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_script_expression_ir COMMAND native_script_expression_ir_test)

add_executable(native_script_source_ir_test
    tests/native/script_source_ir/test.c src/melee/ft/native_script_source.c)
target_include_directories(native_script_source_ir_test PRIVATE src)
target_compile_definitions(native_script_source_ir_test PRIVATE _CRT_SECURE_NO_WARNINGS)
file(GLOB SCRIPT_SOURCE_IR_CORPUS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.fms")
add_test(NAME native_script_source_ir COMMAND native_script_source_ir_test ${SCRIPT_SOURCE_IR_CORPUS})

add_executable(native_script_statement_ir_test
    tests/native/script_source_ir/statement_test.c src/melee/ft/native_script_source.c
    src/melee/ft/native_script_expression.c src/melee/ft/native_script_statement.c)
target_include_directories(native_script_statement_ir_test PRIVATE src)
target_compile_definitions(native_script_statement_ir_test PRIVATE _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_script_statement_ir COMMAND native_script_statement_ir_test)

set(NATIVE_SCRIPT_FRONTEND_UNITS
    src/melee/ft/native_script_source.c src/melee/ft/native_script_expression.c
    src/melee/ft/native_script_statement.c src/melee/ft/native_script_program.c
    src/melee/ft/native_script_value.c)
set(NATIVE_SCRIPT_EXECUTION_UNITS
    src/melee/ft/native_script_bytecode.c src/melee/ft/native_script_vm.c)
set(SCRIPT_KINETICS_FIXTURE_DIR "${CMAKE_CURRENT_BINARY_DIR}/script_kinetics_fixture")
set(SCRIPT_KINETICS_FIXTURE "${SCRIPT_KINETICS_FIXTURE_DIR}/script_kinetics_actual.h")
add_custom_command(OUTPUT "${SCRIPT_KINETICS_FIXTURE}"
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/script_kinetics/extract_primitives.py"
        --source-root "${CMAKE_CURRENT_SOURCE_DIR}" --output "${SCRIPT_KINETICS_FIXTURE}"
    DEPENDS tests/native/script_kinetics/extract_primitives.py src/melee/ft/ftmovement.c
        src/melee/ft/ftcommon.c src/melee/ft/ft_084E.c src/melee/ft/ft_081B.c src/melee/ft/ftanim.c src/melee/ft/native_script_moves.c VERBATIM)
set(NATIVE_SCRIPT_ENGINE_TEST_UNITS
    src/melee/ft/native_script_bindings.c tests/native/script_bindings_unavailable_item.c src/melee/ft/native_script_binding_properties.c
    src/melee/ft/native_script_binding_kinetics.c src/melee/ft/native_script_math.c
    src/melee/ft/native_script_constants.c src/melee/ft/native_script_binding_transition.c
    tests/native/script_bindings_unavailable_presentation.c
    src/melee/ft/native_callback_lifetime.c tests/native/script_kinetics/engine_fixture.c
    src/melee/ft/native_script_binding_setup.c tests/native/script_setup/engine_fixture.c
    "${SCRIPT_KINETICS_FIXTURE}")
set(SCRIPT_STATE_FIXTURE "${SCRIPT_KINETICS_FIXTURE_DIR}/script_state_actual.h")
add_custom_command(OUTPUT "${SCRIPT_STATE_FIXTURE}"
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/script_state/extract_state.py"
        --source-root "${CMAKE_CURRENT_SOURCE_DIR}" --output "${SCRIPT_STATE_FIXTURE}"
    DEPENDS tests/native/script_state/extract_state.py tests/native/script_kinetics/extract_primitives.py
        src/melee/ft/ft_081B.c src/melee/ft/ftcollision_ground.c
        src/melee/ft/kinds/ftCommon/ftCo_Fall.c src/melee/ft/kinds/ftCommon/ftCo_FallAerial.c
        src/melee/ft/kinds/ftCommon/ftCo_FallSpecial.c src/melee/ft/kinds/ftCommon/ftCo_Landing.c
        src/melee/ft/kinds/ftCommon/ftCo_LandingAir.c VERBATIM)
list(APPEND NATIVE_SCRIPT_ENGINE_TEST_UNITS src/melee/ft/native_script_binding_state.c
    tests/native/script_state/engine_fixture.c "${SCRIPT_STATE_FIXTURE}")
add_executable(native_script_program_index_test
    tests/native/script_source_ir/program_test.c ${NATIVE_SCRIPT_FRONTEND_UNITS})
target_include_directories(native_script_program_index_test PRIVATE src)
target_compile_definitions(native_script_program_index_test PRIVATE _CRT_SECURE_NO_WARNINGS)
target_compile_options(native_script_program_index_test PRIVATE -ffp-contract=off)
file(GLOB_RECURSE SCRIPT_PROGRAM_INDEX_CORPUS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.fms")
add_test(NAME native_script_program_index COMMAND native_script_program_index_test ${SCRIPT_PROGRAM_INDEX_CORPUS})

add_executable(native_script_bytecode_test tests/native/script_bytecode/test.c
    ${NATIVE_SCRIPT_FRONTEND_UNITS} ${NATIVE_SCRIPT_EXECUTION_UNITS})
target_include_directories(native_script_bytecode_test PRIVATE src)
target_compile_definitions(native_script_bytecode_test PRIVATE _CRT_SECURE_NO_WARNINGS)
target_compile_options(native_script_bytecode_test PRIVATE -ffp-contract=off)
add_test(NAME native_script_bytecode COMMAND native_script_bytecode_test
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/script_bytecode/probe.fms")

set(SCRIPT_COMBAT_FIXTURE "${SCRIPT_KINETICS_FIXTURE_DIR}/script_combat_actual.h")
add_custom_command(OUTPUT "${SCRIPT_COMBAT_FIXTURE}"
    COMMAND Python3::Interpreter -c "import sys; sys.path.insert(0,sys.argv[1]); import extract_combat; from pathlib import Path; extract_combat.extract(Path(sys.argv[2]),Path(sys.argv[3]))"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/script_combat" "${CMAKE_CURRENT_SOURCE_DIR}" "${SCRIPT_COMBAT_FIXTURE}"
    DEPENDS tests/native/script_combat/extract_combat.py src/melee/ft/fighter.c src/melee/ef/eflib.c src/melee/ft/ftcoll.c
        src/melee/ft/native_fighter_damage.c src/melee/ft/ftcommon.c src/melee/ft/native_fighter_module.c VERBATIM)
list(APPEND NATIVE_SCRIPT_ENGINE_TEST_UNITS src/melee/ft/native_script_binding_combat.c
    src/sysdolphin/baselib/random.c tests/native/script_combat/engine_fixture.c "${SCRIPT_COMBAT_FIXTURE}"
    tests/native/script_kinetics/unavailable_runtime.c src/melee/gm/native_match_arena.c)

add_executable(native_script_native_bindings_test tests/native/script_native_bindings/test.c
    ${NATIVE_SCRIPT_FRONTEND_UNITS} ${NATIVE_SCRIPT_EXECUTION_UNITS}
    ${NATIVE_SCRIPT_ENGINE_TEST_UNITS})
target_include_directories(native_script_native_bindings_test PRIVATE src extern/dolphin/include "${SCRIPT_KINETICS_FIXTURE_DIR}")
target_compile_definitions(native_script_native_bindings_test PRIVATE
    MELEE_NATIVE=1 _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
target_compile_options(native_script_native_bindings_test PRIVATE -ffp-contract=off)
add_test(NAME native_script_native_bindings COMMAND native_script_native_bindings_test)

add_executable(native_script_storage_test tests/native/script_storage/test.c
    ${NATIVE_SCRIPT_FRONTEND_UNITS} ${NATIVE_SCRIPT_EXECUTION_UNITS}
    src/melee/ft/native_script_storage.c src/melee/ft/native_script_events.c)
target_include_directories(native_script_storage_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_storage_test PRIVATE _CRT_SECURE_NO_WARNINGS)
target_compile_options(native_script_storage_test PRIVATE -ffp-contract=off)
add_test(NAME native_script_storage COMMAND native_script_storage_test
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/script_storage/probe.fms")

add_executable(native_script_events_test tests/native/script_events/test.c
    ${NATIVE_SCRIPT_FRONTEND_UNITS} ${NATIVE_SCRIPT_EXECUTION_UNITS}
    src/melee/ft/native_script_events.c)
target_include_directories(native_script_events_test PRIVATE src)
target_compile_definitions(native_script_events_test PRIVATE _CRT_SECURE_NO_WARNINGS)
target_compile_options(native_script_events_test PRIVATE -ffp-contract=off)
add_test(NAME native_script_events COMMAND native_script_events_test unit
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/script_events/probe.fms")

add_executable(native_script_execution_test tests/native/script_execution/test.c
    ${NATIVE_SCRIPT_FRONTEND_UNITS} ${NATIVE_SCRIPT_EXECUTION_UNITS} ${NATIVE_SCRIPT_ENGINE_TEST_UNITS}
    src/melee/ft/native_script_storage.c src/melee/ft/native_script_execution.c
    src/melee/ft/native_script_event_payload.c src/melee/ft/native_script_events.c)
target_include_directories(native_script_execution_test PRIVATE src extern/dolphin/include "${SCRIPT_KINETICS_FIXTURE_DIR}")
target_compile_definitions(native_script_execution_test PRIVATE MELEE_NATIVE=1 _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
target_compile_options(native_script_execution_test PRIVATE -ffp-contract=off)
add_test(NAME native_script_execution COMMAND native_script_execution_test)

add_executable(native_script_event_payload_test tests/native/script_event_payload/test.c
    ${NATIVE_SCRIPT_FRONTEND_UNITS} ${NATIVE_SCRIPT_EXECUTION_UNITS}
    src/melee/ft/native_script_events.c src/melee/ft/native_script_event_payload.c)
target_include_directories(native_script_event_payload_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_event_payload_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
target_compile_options(native_script_event_payload_test PRIVATE -ffp-contract=off)
add_test(NAME native_script_event_payload COMMAND native_script_event_payload_test
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/script_event_payload/probe.fms")

add_executable(native_script_kinetics_test tests/native/script_kinetics/test.c
    ${NATIVE_SCRIPT_FRONTEND_UNITS} ${NATIVE_SCRIPT_EXECUTION_UNITS} ${NATIVE_SCRIPT_ENGINE_TEST_UNITS})
target_include_directories(native_script_kinetics_test PRIVATE src extern/dolphin/include "${SCRIPT_KINETICS_FIXTURE_DIR}")
target_compile_definitions(native_script_kinetics_test PRIVATE MELEE_NATIVE=1 _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
target_compile_options(native_script_kinetics_test PRIVATE -ffp-contract=off)
add_test(NAME native_script_kinetics COMMAND native_script_kinetics_test)

add_executable(native_script_state_test tests/native/script_state/test.c
    ${NATIVE_SCRIPT_FRONTEND_UNITS} ${NATIVE_SCRIPT_EXECUTION_UNITS} ${NATIVE_SCRIPT_ENGINE_TEST_UNITS})
target_include_directories(native_script_state_test PRIVATE src extern/dolphin/include "${SCRIPT_KINETICS_FIXTURE_DIR}")
target_compile_definitions(native_script_state_test PRIVATE MELEE_NATIVE=1 _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
target_compile_options(native_script_state_test PRIVATE -ffp-contract=off)
add_test(NAME native_script_state COMMAND native_script_state_test)

add_executable(native_script_compile_test tests/native/script_compile/test.c tests/native/script_compile/native_boundaries.c tests/native/script_bindings_unavailable_presentation.c
    ${NATIVE_SCRIPT_FRONTEND_UNITS} ${NATIVE_SCRIPT_EXECUTION_UNITS}
    src/melee/ft/native_script_events.c src/melee/ft/native_script_event_payload.c
    src/melee/ft/native_script_execution.c src/melee/ft/native_script_compile.c
    src/melee/ft/native_script_constants.c src/melee/ft/native_script_moves.c src/melee/ft/native_script_move_lowering.c
    src/melee/ft/native_script_storage.c src/melee/ft/native_script_bindings.c tests/native/script_bindings_unavailable_item.c src/melee/ft/native_script_math.c)
target_include_directories(native_script_compile_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_compile_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
target_compile_options(native_script_compile_test PRIVATE -ffp-contract=off)
add_test(NAME native_script_compile COMMAND native_script_compile_test
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/script_compile/probe.fms")

add_executable(native_match_object_census_test tests/native/native_match_object_census/test.c
    src/melee/gm/native_match_object_census.c)
target_include_directories(native_match_object_census_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_match_object_census_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_match_object_census COMMAND native_match_object_census_test)
set_tests_properties(native_match_object_census PROPERTIES ENVIRONMENT
    "FREAK_MELEE_OBJECT_CENSUS=${CMAKE_CURRENT_BINARY_DIR}/native_match_object_census_ctest.log")

add_executable(native_match_pool_test tests/native/native_match_pool/test.c
    src/melee/gm/native_match_pool.c src/melee/gm/native_match_snapshot_pool.c)
target_include_directories(native_match_pool_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_match_pool_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_match_pool COMMAND native_match_pool_test)
add_executable(native_match_arena_test tests/native/match_arena_test.c
    src/melee/gm/native_match_arena.c)
target_include_directories(native_match_arena_test PRIVATE src)
add_test(NAME native_match_arena COMMAND native_match_arena_test)

add_executable(native_gobj_pool_consumer_test tests/native/native_gobj_pool_consumer/test.c
    src/melee/gm/native_match_pool.c src/melee/gm/native_match_snapshot_pool.c
    src/melee/gm/native_rollback_state.c src/sysdolphin/baselib/gobj.c
    src/sysdolphin/baselib/gobjinit.c src/sysdolphin/baselib/gobjplink.c
    src/sysdolphin/baselib/gobjproc.c src/sysdolphin/baselib/gobjgxlink.c
    src/sysdolphin/baselib/gobjuserdata.c src/sysdolphin/baselib/gobjobject.c
    src/melee/gm/native_match_arena.c src/melee/gm/native_match_scene.c
    src/sysdolphin/baselib/native_gobj_pool_storage.c src/melee/ft/native_fighter_pool.c
    src/melee/gr/native_ground_pool.c src/melee/gr/native_green_grid.c
    src/melee/gr/native_green_grid_snapshot.c src/melee/it/native_item_pool.c
    src/melee/it/native_item_link_pool.c
    tests/native/item_actor_absent.c tests/native/script_scene_boundary.c)
target_include_directories(native_gobj_pool_consumer_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_gobj_pool_consumer_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
target_compile_options(native_gobj_pool_consumer_test PRIVATE -fno-zero-initialized-in-bss
    -include "${CMAKE_CURRENT_SOURCE_DIR}/src/melee/gm/native_rollback_state_section.h")
add_test(NAME native_gobj_pool_consumer COMMAND native_gobj_pool_consumer_test)

add_executable(native_article_components_compiler_test
    tests/native/article_animation_components_compiler_test.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_article_components_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_article_components_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_article_components_compiler COMMAND native_article_components_compiler_test)
set(ARTICLE_COMPONENTS_ACTION_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/article_components_action_fixture.h")
set(ARTICLE_COMPONENTS_CALLBACK_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/article_components_callback_fixture.h")
add_custom_command(OUTPUT "${ARTICLE_COMPONENTS_ACTION_FIXTURE}" "${ARTICLE_COMPONENTS_CALLBACK_FIXTURE}"
    COMMAND $<TARGET_FILE:native_article_components_compiler_test>
        "${ARTICLE_COMPONENTS_ACTION_FIXTURE}" "${ARTICLE_COMPONENTS_CALLBACK_FIXTURE}"
    DEPENDS native_article_components_compiler_test VERBATIM)

add_executable(native_article_resource_effects_compiler_test
    tests/native/article_resource_effects/compiler.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_article_resource_effects_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_article_resource_effects_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
set(RESOURCE_EFFECTS_ACTION "${CMAKE_CURRENT_BINARY_DIR}/resource_effects_action.h")
set(RESOURCE_EFFECTS_CALLBACK "${CMAKE_CURRENT_BINARY_DIR}/resource_effects_callback.h")
add_custom_command(OUTPUT "${RESOURCE_EFFECTS_ACTION}" "${RESOURCE_EFFECTS_CALLBACK}"
    COMMAND $<TARGET_FILE:native_article_resource_effects_compiler_test> "${RESOURCE_EFFECTS_ACTION}" "${RESOURCE_EFFECTS_CALLBACK}"
    DEPENDS native_article_resource_effects_compiler_test VERBATIM)
add_test(NAME native_article_resource_effects_compiler COMMAND native_article_resource_effects_compiler_test
    "${CMAKE_CURRENT_BINARY_DIR}/resource_effects_ctest_action.h" "${CMAKE_CURRENT_BINARY_DIR}/resource_effects_ctest_callback.h")
foreach(route action callback)
    string(TOUPPER "${route}" route_upper)
    add_executable(native_article_resource_effects_${route}_test
        tests/native/article_resource_effects/module.c "${RESOURCE_EFFECTS_ACTION}" "${RESOURCE_EFFECTS_CALLBACK}")
    target_include_directories(native_article_resource_effects_${route}_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_article_resource_effects_${route}_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS RESOURCE_EFFECT_MODULE="${RESOURCE_EFFECTS_${route_upper}}")
    add_test(NAME native_article_resource_effects_${route} COMMAND native_article_resource_effects_${route}_test)
endforeach()

add_executable(native_script_engine_items_compiler_test
    tests/native/script_engine_items_compiler_test.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_engine_items_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_engine_items_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_script_engine_items_compiler COMMAND native_script_engine_items_compiler_test)
set(ENGINE_ITEMS_ACTION_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/script_engine_items_action_fixture.h")
set(ENGINE_ITEMS_CALLBACK_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/script_engine_items_callback_fixture.h")
add_custom_command(OUTPUT "${ENGINE_ITEMS_ACTION_FIXTURE}" "${ENGINE_ITEMS_CALLBACK_FIXTURE}"
    COMMAND $<TARGET_FILE:native_script_engine_items_compiler_test>
        "${ENGINE_ITEMS_ACTION_FIXTURE}" "${ENGINE_ITEMS_CALLBACK_FIXTURE}"
    DEPENDS native_script_engine_items_compiler_test VERBATIM)

foreach(form action callback)
    set(callbacks 0)
    if(form STREQUAL "callback")
        set(callbacks 1)
    endif()
    add_executable(native_parameter_capacity_${form}_module_test
        tests/native/parameter_capacity_module_test.c
        "${PARAMETER_ACTION_FIXTURE}" "${PARAMETER_CALLBACK_FIXTURE}")
    target_include_directories(native_parameter_capacity_${form}_module_test PRIVATE
        src extern/dolphin/include "${CMAKE_CURRENT_BINARY_DIR}")
    target_compile_definitions(native_parameter_capacity_${form}_module_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS PARAMETER_CAPACITY_CALLBACKS=${callbacks})
    add_test(NAME native_parameter_capacity_${form}_module COMMAND native_parameter_capacity_${form}_module_test)

    add_executable(native_article_components_${form}_module_test
        tests/native/article_animation_components_module_test.c
        "${ARTICLE_COMPONENTS_ACTION_FIXTURE}" "${ARTICLE_COMPONENTS_CALLBACK_FIXTURE}")
    target_include_directories(native_article_components_${form}_module_test PRIVATE
        src extern/dolphin/include "${CMAKE_CURRENT_BINARY_DIR}")
    target_compile_definitions(native_article_components_${form}_module_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS ARTICLE_COMPONENTS_CALLBACKS=${callbacks})
    add_test(NAME native_article_components_${form}_module COMMAND native_article_components_${form}_module_test)

    add_executable(native_script_engine_items_${form}_module_test
        tests/native/script_engine_items_module_test.c
        "${ENGINE_ITEMS_ACTION_FIXTURE}" "${ENGINE_ITEMS_CALLBACK_FIXTURE}")
    target_include_directories(native_script_engine_items_${form}_module_test PRIVATE
        src extern/dolphin/include "${CMAKE_CURRENT_BINARY_DIR}")
    target_compile_definitions(native_script_engine_items_${form}_module_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS ENGINE_ITEMS_CALLBACKS=${callbacks})
    add_test(NAME native_script_engine_items_${form}_module COMMAND native_script_engine_items_${form}_module_test)
endforeach()

set(SPATIAL_SCRIPT_DIR "${CMAKE_CURRENT_BINARY_DIR}/spatial_script")
set(SPATIAL_SCRIPT_HEADER "${SPATIAL_SCRIPT_DIR}/spatial.h")
add_executable(native_script_spatial_compiler_test tests/native/spatial_script/compiler.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_spatial_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_spatial_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_custom_command(OUTPUT "${SPATIAL_SCRIPT_HEADER}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SPATIAL_SCRIPT_DIR}"
    COMMAND $<TARGET_FILE:native_script_spatial_compiler_test>
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/spatial_script/fixture.fms" "${SPATIAL_SCRIPT_HEADER}"
    DEPENDS native_script_spatial_compiler_test tests/native/spatial_script/fixture.fms VERBATIM)
add_test(NAME native_script_spatial_compiler COMMAND native_script_spatial_compiler_test
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/spatial_script/fixture.fms" "${SPATIAL_SCRIPT_DIR}/ctest.h")
add_executable(native_script_spatial_module_test tests/native/spatial_script/module_test.c "${SPATIAL_SCRIPT_HEADER}")
target_include_directories(native_script_spatial_module_test PRIVATE "${SPATIAL_SCRIPT_DIR}" src extern/dolphin/include)
target_compile_definitions(native_script_spatial_module_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_script_spatial_module COMMAND native_script_spatial_module_test)
set(AIR_TETHER_FIXTURE_DIR "${CMAKE_CURRENT_BINARY_DIR}/air_tether_policy_fixture")
set(AIR_TETHER_FIXTURE "${AIR_TETHER_FIXTURE_DIR}/air_tether_actual.inc")
add_custom_command(OUTPUT "${AIR_TETHER_FIXTURE}"
    COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/tests/native/air_tether_policy_fixture.py
        --root ${CMAKE_CURRENT_SOURCE_DIR} --output "${AIR_TETHER_FIXTURE_DIR}"
    DEPENDS tests/native/air_tether_policy_fixture.py tests/native/item_owner_fixture.py
        src/melee/ft/kinds/ftCommon/ftCo_AirCatch.c src/melee/ft/native_fighter_geometry.c VERBATIM)
add_executable(native_air_tether_policy_test tests/native/air_tether_policy_test.c
    "${AIR_TETHER_FIXTURE}" src/melee/ft/native_callback_lifetime.c
    tests/native/model_effect_absent_fixture.c)
target_include_directories(native_air_tether_policy_test PRIVATE
    "${AIR_TETHER_FIXTURE_DIR}" tests/native src extern/dolphin/include)
target_compile_definitions(native_air_tether_policy_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_air_tether_policy COMMAND native_air_tether_policy_test)
add_executable(native_script_article_collision_compiler_test
    tests/native/script_article_collision_compiler_test.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_article_collision_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_article_collision_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_script_article_collision_compiler COMMAND native_script_article_collision_compiler_test)
add_executable(native_script_tether_primitives_compiler_test
    tests/native/script_tether_primitives_compiler_test.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_tether_primitives_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_tether_primitives_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_script_tether_primitives_compiler COMMAND native_script_tether_primitives_compiler_test)
set(TETHER_ACTION_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/script_tether_primitives_action_fixture.h")
set(TETHER_CALLBACK_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/script_tether_primitives_callback_fixture.h")
add_custom_command(OUTPUT "${TETHER_ACTION_FIXTURE}" "${TETHER_CALLBACK_FIXTURE}"
    COMMAND $<TARGET_FILE:native_script_tether_primitives_compiler_test>
        "${TETHER_ACTION_FIXTURE}" "${TETHER_CALLBACK_FIXTURE}"
    DEPENDS native_script_tether_primitives_compiler_test VERBATIM)
foreach(form action callback)
    set(callbacks 0)
    if(form STREQUAL "callback")
        set(callbacks 1)
    endif()
    add_executable(native_script_tether_primitives_${form}_module_test
        tests/native/script_tether_primitives_module_test.c
        "${TETHER_ACTION_FIXTURE}" "${TETHER_CALLBACK_FIXTURE}")
    target_include_directories(native_script_tether_primitives_${form}_module_test PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}" src extern/dolphin/include)
    target_compile_definitions(native_script_tether_primitives_${form}_module_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS TETHER_CALLBACKS=${callbacks})
    add_test(NAME native_script_tether_primitives_${form}_module
        COMMAND native_script_tether_primitives_${form}_module_test)
endforeach()

add_executable(native_script_held_motion_compiler_test
    tests/native/script_held_motion_compiler_test.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_held_motion_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_held_motion_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_script_held_motion_compiler COMMAND native_script_held_motion_compiler_test)
set(HELD_MOTION_ACTION_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/script_held_motion_action_fixture.h")
set(HELD_MOTION_CALLBACK_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/script_held_motion_callback_fixture.h")
add_custom_command(OUTPUT "${HELD_MOTION_ACTION_FIXTURE}" "${HELD_MOTION_CALLBACK_FIXTURE}"
    COMMAND $<TARGET_FILE:native_script_held_motion_compiler_test>
        "${HELD_MOTION_ACTION_FIXTURE}" "${HELD_MOTION_CALLBACK_FIXTURE}"
    DEPENDS native_script_held_motion_compiler_test VERBATIM)
foreach(form action callback)
    set(callbacks 0)
    if(form STREQUAL "callback")
        set(callbacks 1)
    endif()
    add_executable(native_script_held_motion_${form}_module_test
        tests/native/script_held_motion_module_test.c
        "${HELD_MOTION_ACTION_FIXTURE}" "${HELD_MOTION_CALLBACK_FIXTURE}")
    target_include_directories(native_script_held_motion_${form}_module_test PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}" src extern/dolphin/include)
    target_compile_definitions(native_script_held_motion_${form}_module_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS HELD_MOTION_CALLBACKS=${callbacks})
    add_test(NAME native_script_held_motion_${form}_module
        COMMAND native_script_held_motion_${form}_module_test)
endforeach()

set(HELD_MOTION_ENTRY_DIR "${CMAKE_CURRENT_BINARY_DIR}/held_motion_entry")
set(HELD_MOTION_ENTRY_FIXTURE "${HELD_MOTION_ENTRY_DIR}/candidate.inc")
file(GLOB HELD_MOTION_ORIGINALS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/held_motion_entry/original53/*")
add_custom_command(OUTPUT "${HELD_MOTION_ENTRY_FIXTURE}"
    COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/tests/native/held_motion_entry/fixture.py
        --root ${CMAKE_CURRENT_SOURCE_DIR} --output "${HELD_MOTION_ENTRY_DIR}"
    DEPENDS tests/native/held_motion_entry/fixture.py tests/native/held_motion_entry/original53.inc
        tests/native/held_motion_entry/original53_manifest.json ${HELD_MOTION_ORIGINALS}
        src/melee/ft/kinds/ftCommon/ftCo_ItemParasolOpen.c
        src/melee/ft/kinds/ftCommon/ftCo_ItemParasolFallSpecial.c
        src/melee/ft/native_fighter_held_motion.c src/melee/ft/native_fighter_held_motion.h VERBATIM)
add_executable(native_held_motion_entry_test tests/native/held_motion_entry/test.c
    "${HELD_MOTION_ENTRY_FIXTURE}" src/melee/ft/native_fighter_held_motion.c
    src/melee/ft/native_callback_lifetime.c tests/native/held_motion_entry/effect_absent.c)
target_include_directories(native_held_motion_entry_test PRIVATE
    tests/native/held_motion_entry "${HELD_MOTION_ENTRY_DIR}" src extern/dolphin/include)
target_compile_definitions(native_held_motion_entry_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_held_motion_entry COMMAND native_held_motion_entry_test)

set(HELD_LIFECYCLE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/held_lifecycle_protocol")
set(HELD_LIFECYCLE_DIR "${CMAKE_CURRENT_BINARY_DIR}/held_lifecycle_protocol")
set(HELD_LIFECYCLE_ACTION "${HELD_LIFECYCLE_DIR}/held_lifecycle_fixture.h")
set(HELD_LIFECYCLE_BASE "${HELD_LIFECYCLE_DIR}/held_lifecycle_base_fixture.h")
add_executable(native_script_held_lifecycle_compiler_test
    tests/native/held_lifecycle_protocol/compiler_test.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_held_lifecycle_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_held_lifecycle_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_custom_command(OUTPUT "${HELD_LIFECYCLE_ACTION}" "${HELD_LIFECYCLE_BASE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${HELD_LIFECYCLE_DIR}"
    COMMAND $<TARGET_FILE:native_script_held_lifecycle_compiler_test>
        "${HELD_LIFECYCLE_SOURCE_DIR}/fixture.fms" "${HELD_LIFECYCLE_ACTION}" "${HELD_LIFECYCLE_BASE}"
    DEPENDS native_script_held_lifecycle_compiler_test tests/native/held_lifecycle_protocol/fixture.fms VERBATIM)
add_test(NAME native_script_held_lifecycle_compiler
    COMMAND native_script_held_lifecycle_compiler_test
        "${HELD_LIFECYCLE_SOURCE_DIR}/fixture.fms"
        "${HELD_LIFECYCLE_DIR}/ctest_action.h" "${HELD_LIFECYCLE_DIR}/ctest_base.h")
foreach(form action base)
    add_executable(native_script_held_lifecycle_${form}_module_test
        tests/native/held_lifecycle_protocol/module_test.c "${HELD_LIFECYCLE_ACTION}" "${HELD_LIFECYCLE_BASE}")
    target_include_directories(native_script_held_lifecycle_${form}_module_test PRIVATE
        "${HELD_LIFECYCLE_DIR}" src extern/dolphin/include)
    target_compile_definitions(native_script_held_lifecycle_${form}_module_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    if(form STREQUAL "base")
        target_compile_definitions(native_script_held_lifecycle_${form}_module_test PRIVATE LIFECYCLE_BASE=1)
    endif()
    add_test(NAME native_script_held_lifecycle_${form}_module
        COMMAND native_script_held_lifecycle_${form}_module_test)
endforeach()
set(HELD_LIFECYCLE_DLL_SOURCE "${HELD_LIFECYCLE_DIR}/held_lifecycle_fixture.c")
add_custom_command(OUTPUT "${HELD_LIFECYCLE_DLL_SOURCE}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HELD_LIFECYCLE_ACTION}" "${HELD_LIFECYCLE_DLL_SOURCE}"
    DEPENDS "${HELD_LIFECYCLE_ACTION}" VERBATIM)
add_library(native_held_lifecycle_fixture SHARED "${HELD_LIFECYCLE_DLL_SOURCE}")
target_include_directories(native_held_lifecycle_fixture PRIVATE src extern/dolphin/include)
target_compile_definitions(native_held_lifecycle_fixture PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_executable(native_script_held_lifecycle_dll_test tests/native/held_lifecycle_protocol/module_test.c)
target_include_directories(native_script_held_lifecycle_dll_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_held_lifecycle_dll_test PRIVATE MELEE_NATIVE=1 LIFECYCLE_DLL=1 _CRT_SECURE_NO_WARNINGS)
add_dependencies(native_script_held_lifecycle_dll_test native_held_lifecycle_fixture)
add_test(NAME native_script_held_lifecycle_dll
    COMMAND native_script_held_lifecycle_dll_test $<TARGET_FILE:native_held_lifecycle_fixture>)

set(HELD_LIFECYCLE_ENTRIES "${HELD_LIFECYCLE_DIR}/candidate_entries.inc")
add_custom_command(OUTPUT "${HELD_LIFECYCLE_ENTRIES}" "${HELD_LIFECYCLE_DIR}/original_entries.inc"
    COMMAND Python3::Interpreter "${HELD_LIFECYCLE_SOURCE_DIR}/source_fixture.py"
        --root ${CMAKE_CURRENT_SOURCE_DIR} --output "${HELD_LIFECYCLE_DIR}"
    DEPENDS tests/native/held_lifecycle_protocol/source_fixture.py
        tests/native/held_lifecycle_protocol/original54.inc tests/native/held_lifecycle_protocol/original54_manifest.json
        src/melee/ft/ft_08A1.c src/melee/ft/ft_0D27.c src/melee/ft/kinds/ftCommon/ftCo_Landing.c src/melee/ft/fighter.c VERBATIM)
add_executable(native_held_lifecycle_source_test tests/native/held_lifecycle_protocol/source_test.c
    "${HELD_LIFECYCLE_ENTRIES}" src/melee/ft/native_callback_lifetime.c
    tests/native/held_lifecycle_protocol/effect_absent.c)
target_include_directories(native_held_lifecycle_source_test PRIVATE
    "${HELD_LIFECYCLE_SOURCE_DIR}" "${HELD_LIFECYCLE_DIR}" src extern/dolphin/include)
target_compile_definitions(native_held_lifecycle_source_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_held_lifecycle_source COMMAND native_held_lifecycle_source_test)

add_executable(native_script_gamewatch_contracts_compiler_test
    tests/native/gamewatch_specials_contracts/compiler.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_gamewatch_contracts_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_gamewatch_contracts_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_script_gamewatch_contracts_compiler COMMAND native_script_gamewatch_contracts_compiler_test)

set(ENGINE_FOOD_DIR "${CMAKE_CURRENT_BINARY_DIR}/engine_food_spawn")
set(ENGINE_FOOD_FIXTURE "${ENGINE_FOOD_DIR}/food_spawn.inc")
set(ENGINE_AIRBORNE_FIXTURE "${ENGINE_FOOD_DIR}/airborne_body.inc")
set(ENGINE_AIRBORNE_CASES "${ENGINE_FOOD_DIR}/airborne_cases.inc")
add_custom_command(OUTPUT "${ENGINE_FOOD_FIXTURE}" "${ENGINE_AIRBORNE_FIXTURE}" "${ENGINE_AIRBORNE_CASES}"
    COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/tests/native/engine_food_spawn/fixture.py
        --root ${CMAKE_CURRENT_SOURCE_DIR} --output "${ENGINE_FOOD_DIR}"
    DEPENDS tests/native/engine_food_spawn/fixture.py src/melee/it/kinds/itfoods.c
        src/melee/it/it_2725.c src/melee/ft/native_fighter_target.c tests/native/fighter_target_test.c VERBATIM)
add_executable(native_engine_food_spawn_test tests/native/engine_food_spawn/test.c "${ENGINE_FOOD_FIXTURE}")
target_include_directories(native_engine_food_spawn_test PRIVATE "${ENGINE_FOOD_DIR}" src extern/dolphin/include)
target_compile_definitions(native_engine_food_spawn_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_engine_food_spawn COMMAND native_engine_food_spawn_test)
add_executable(native_engine_airborne_target_test tests/native/engine_food_spawn/airborne_test.c
    "${ENGINE_AIRBORNE_FIXTURE}" "${ENGINE_AIRBORNE_CASES}")
target_include_directories(native_engine_airborne_target_test PRIVATE "${ENGINE_FOOD_DIR}" src extern/dolphin/include)
target_compile_definitions(native_engine_airborne_target_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_engine_airborne_target COMMAND native_engine_airborne_target_test)

set(NORMAL_ATTACK_DIR "${CMAKE_CURRENT_BINARY_DIR}/normal_attack_contracts")
set(NORMAL_ATTACK_ACTION "${NORMAL_ATTACK_DIR}/normal_action.h")
set(NORMAL_ATTACK_BASE "${NORMAL_ATTACK_DIR}/normal_base.h")
add_executable(native_script_normal_attack_compiler_test
    tests/native/normal_attack_contracts/compiler.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_normal_attack_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_normal_attack_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_custom_command(OUTPUT "${NORMAL_ATTACK_ACTION}" "${NORMAL_ATTACK_BASE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${NORMAL_ATTACK_DIR}"
    COMMAND $<TARGET_FILE:native_script_normal_attack_compiler_test>
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/normal_attack_contracts/fixture.fms"
        "${NORMAL_ATTACK_ACTION}" "${NORMAL_ATTACK_BASE}"
    DEPENDS native_script_normal_attack_compiler_test tests/native/normal_attack_contracts/fixture.fms VERBATIM)
add_test(NAME native_script_normal_attack_compiler COMMAND native_script_normal_attack_compiler_test
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/normal_attack_contracts/fixture.fms"
    "${NORMAL_ATTACK_DIR}/ctest_action.h" "${NORMAL_ATTACK_DIR}/ctest_base.h")
foreach(form action base)
    add_executable(native_script_normal_attack_${form}_test
        tests/native/normal_attack_contracts/module_test.c
        src/melee/ft/native_callback_lifetime.c "${NORMAL_ATTACK_ACTION}" "${NORMAL_ATTACK_BASE}")
    target_include_directories(native_script_normal_attack_${form}_test PRIVATE "${NORMAL_ATTACK_DIR}" src extern/dolphin/include)
    target_compile_definitions(native_script_normal_attack_${form}_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    if(form STREQUAL "base")
        target_compile_definitions(native_script_normal_attack_${form}_test PRIVATE NORMAL_BASE=1)
    endif()
    add_test(NAME native_script_normal_attack_${form} COMMAND native_script_normal_attack_${form}_test)
endforeach()

set(SCRIPT_FIGHTER_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/script_fighter_fixture/actual_callsites.h")
add_custom_command(OUTPUT "${SCRIPT_FIGHTER_FIXTURE}"
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/script_fighter/extract_callsites.py"
        --root "${CMAKE_CURRENT_SOURCE_DIR}" --output "${SCRIPT_FIGHTER_FIXTURE}"
    DEPENDS tests/native/script_fighter/extract_callsites.py src/melee/ft/fighter.c src/melee/ft/ftcommon.c
        src/melee/ft/kinds/ftCommon/ftCo_Attack100.c src/melee/ft/kinds/ftCommon/ftCo_SpecialAir.c
        src/melee/ft/kinds/ftCommon/ftCo_SpecialS.c VERBATIM)
add_executable(native_script_fighter_test tests/native/script_fighter/test.c tests/native/script_bindings_unavailable_presentation.c "${SCRIPT_FIGHTER_FIXTURE}"
    ${NATIVE_SCRIPT_FRONTEND_UNITS} ${NATIVE_SCRIPT_EXECUTION_UNITS}
    src/melee/ft/native_script_events.c src/melee/ft/native_script_event_payload.c
    src/melee/ft/native_script_execution.c src/melee/ft/native_script_compile.c
    src/melee/ft/native_script_constants.c src/melee/ft/native_script_moves.c src/melee/ft/native_script_move_lowering.c
    src/melee/ft/native_script_storage.c src/melee/ft/native_script_bindings.c tests/native/script_bindings_unavailable_item.c src/melee/ft/native_script_math.c
    src/melee/ft/native_script_fighter_actor.c src/melee/ft/native_script_fighter_program.c
    src/melee/ft/native_script_runtime.c
    src/melee/ft/native_callback_lifetime.c src/melee/gm/native_match_pool.c src/melee/gm/native_match_arena.c)
target_include_directories(native_script_fighter_test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/script_fighter_fixture" src extern/dolphin/include)
target_compile_definitions(native_script_fighter_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
target_compile_options(native_script_fighter_test PRIVATE -ffp-contract=off)
add_test(NAME native_script_fighter COMMAND native_script_fighter_test)

set(SCRIPT_ITEM_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/script_item_fixture/script_item_actual.h")
add_custom_command(OUTPUT "${SCRIPT_ITEM_FIXTURE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/script_item_fixture"
    COMMAND Python3::Interpreter -c "import sys; sys.path.insert(0,sys.argv[1]); import extract_item; from pathlib import Path; extract_item.extract(Path(sys.argv[2]),Path(sys.argv[3]))"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/script_item" "${CMAKE_CURRENT_SOURCE_DIR}" "${SCRIPT_ITEM_FIXTURE}"
    DEPENDS tests/native/script_item/extract_item.py tests/native/script_kinetics/extract_primitives.py
        src/melee/it/it_2725.c src/melee/it/it_26B1.c src/melee/it/item.c
        src/melee/it/itgroundcoll.c src/melee/it/itmaplib.c src/melee/mp/mpcoll.c src/melee/lb/lbvector.c VERBATIM)
add_executable(native_script_item_test tests/native/script_item/test.c
    tests/native/script_item/engine_fixture.c tests/native/script_item/unavailable_families.c "${SCRIPT_ITEM_FIXTURE}"
    ${NATIVE_SCRIPT_FRONTEND_UNITS} ${NATIVE_SCRIPT_EXECUTION_UNITS}
    src/melee/ft/native_script_binding_item.c src/melee/ft/native_script_bindings.c
    src/melee/ft/native_script_binding_properties.c src/melee/ft/native_script_constants.c
    src/melee/ft/native_script_math.c src/melee/ft/native_callback_lifetime.c)
target_include_directories(native_script_item_test PRIVATE src extern/dolphin/include "${CMAKE_CURRENT_BINARY_DIR}/script_item_fixture")
target_compile_definitions(native_script_item_test PRIVATE MELEE_NATIVE=1 _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
target_compile_options(native_script_item_test PRIVATE -ffp-contract=off)
add_test(NAME native_script_item COMMAND native_script_item_test)

set(ANIMATION_OWNER_FIXTURE_DIR "${CMAKE_CURRENT_BINARY_DIR}/native_animation_owner_fixture")
set(ANIMATION_OWNER_INCLUDES "${ANIMATION_OWNER_FIXTURE_DIR}/heap.inc"
    "${ANIMATION_OWNER_FIXTURE_DIR}/snapshot_layout.inc" "${ANIMATION_OWNER_FIXTURE_DIR}/hermite.inc")
add_custom_command(OUTPUT ${ANIMATION_OWNER_INCLUDES}
    COMMAND Python3::Interpreter -c "import sys; sys.path.insert(0,sys.argv[1]); import run; from pathlib import Path; run.extract(Path(sys.argv[2]),Path(sys.argv[3]))"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/native_animation_dependency" "${CMAKE_CURRENT_SOURCE_DIR}" "${ANIMATION_OWNER_FIXTURE_DIR}"
    DEPENDS tests/native/native_animation_dependency/run.py src/sysdolphin/baselib/object.c
        src/sysdolphin/baselib/initialize.c src/melee/lb/lbdvd.c
        src/sysdolphin/baselib/native_animation_pool.c src/sysdolphin/baselib/spline.c VERBATIM)
add_executable(native_animation_owner_test tests/native/native_animation_dependency/test.c ${ANIMATION_OWNER_INCLUDES}
    src/sysdolphin/baselib/aobj.c src/sysdolphin/baselib/fobj.c src/sysdolphin/baselib/random.c
    src/sysdolphin/baselib/native_animation_pool.c src/melee/lb/lbanim.c
    src/melee/gm/native_match_pool.c src/melee/gm/native_match_snapshot_pool.c src/melee/gm/native_rollback_state.c
    src/melee/gm/native_match_arena.c src/sysdolphin/baselib/native_hsd_arena.c)
target_include_directories(native_animation_owner_test PRIVATE src extern/dolphin/include "${ANIMATION_OWNER_FIXTURE_DIR}")
target_compile_definitions(native_animation_owner_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
target_compile_options(native_animation_owner_test PRIVATE -ffp-contract=off -ffunction-sections -fdata-sections
    -fno-zero-initialized-in-bss -include "${CMAKE_CURRENT_SOURCE_DIR}/src/melee/gm/native_rollback_state_section.h")
target_link_options(native_animation_owner_test PRIVATE "LINKER:/opt:ref")
add_test(NAME native_animation_owner COMMAND native_animation_owner_test)

add_executable(native_script_setup_test tests/native/script_setup/test.c
    ${NATIVE_SCRIPT_FRONTEND_UNITS} ${NATIVE_SCRIPT_EXECUTION_UNITS} ${NATIVE_SCRIPT_ENGINE_TEST_UNITS})
target_include_directories(native_script_setup_test PRIVATE src extern/dolphin/include "${SCRIPT_KINETICS_FIXTURE_DIR}")
target_compile_definitions(native_script_setup_test PRIVATE MELEE_NATIVE=1 _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
target_compile_options(native_script_setup_test PRIVATE -ffp-contract=off)
add_test(NAME native_script_setup COMMAND native_script_setup_test)

add_executable(native_script_metadata_test tests/native/script_metadata/test.c
    ${NATIVE_SCRIPT_FRONTEND_UNITS} src/melee/ft/native_script_metadata.c src/sysdolphin/baselib/dat.c)
target_include_directories(native_script_metadata_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_metadata_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_script_metadata COMMAND native_script_metadata_test)

set(SCRIPT_COMBAT_ENGINE_UNITS ${NATIVE_SCRIPT_ENGINE_TEST_UNITS})
# This oracle owns reject-only transition and actor boundaries in test.c.
list(REMOVE_ITEM SCRIPT_COMBAT_ENGINE_UNITS src/melee/ft/native_script_binding_transition.c)
add_executable(native_script_combat_test tests/native/script_combat/test.c
    ${NATIVE_SCRIPT_FRONTEND_UNITS} ${NATIVE_SCRIPT_EXECUTION_UNITS} ${SCRIPT_COMBAT_ENGINE_UNITS})
target_include_directories(native_script_combat_test PRIVATE src extern/dolphin/include "${SCRIPT_KINETICS_FIXTURE_DIR}")
target_compile_definitions(native_script_combat_test PRIVATE MELEE_NATIVE=1 _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
target_compile_options(native_script_combat_test PRIVATE -ffp-contract=off)
add_test(NAME native_script_combat COMMAND native_script_combat_test)
add_executable(native_script_constants_test tests/native/script_constants/test.c
    ${NATIVE_SCRIPT_FRONTEND_UNITS} ${NATIVE_SCRIPT_EXECUTION_UNITS} src/melee/ft/native_script_constants.c)
target_include_directories(native_script_constants_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_constants_test PRIVATE MELEE_NATIVE=1 _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
target_compile_options(native_script_constants_test PRIVATE -ffp-contract=off)
add_test(NAME native_script_constants COMMAND native_script_constants_test)

set(SCRIPT_PRESENTATION_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/script_presentation_fixture/script_presentation_actual.h")
add_custom_command(OUTPUT "${SCRIPT_PRESENTATION_FIXTURE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/script_presentation_fixture"
    COMMAND Python3::Interpreter -c "import sys; sys.path.insert(0,sys.argv[1]); import extract_presentation; from pathlib import Path; extract_presentation.extract(Path(sys.argv[2]),Path(sys.argv[3]))"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/script_presentation" "${CMAKE_CURRENT_SOURCE_DIR}" "${SCRIPT_PRESENTATION_FIXTURE}"
    DEPENDS tests/native/script_presentation/extract_presentation.py src/melee/ft/ft_0877.c src/melee/ft/ft_0881.c
        src/melee/ft/native_fighter_runtime.c src/sysdolphin/baselib/jobj.c src/sysdolphin/baselib/class.c src/sysdolphin/baselib/native_hsd_arena.c src/melee/gm/native_match_arena.c
        extern/dolphin/src/dolphin/mtx/mtxvec.c VERBATIM)
add_executable(native_script_presentation_test tests/native/script_presentation/test.c
    tests/native/script_presentation/engine_fixture.c tests/native/script_presentation/unavailable_families.c
    tests/native/script_presentation/unavailable_accessory.c tests/native/script_kinetics/unavailable_runtime.c
    "${SCRIPT_PRESENTATION_FIXTURE}" ${NATIVE_SCRIPT_FRONTEND_UNITS} ${NATIVE_SCRIPT_EXECUTION_UNITS}
    src/melee/ft/native_script_bindings.c tests/native/script_bindings_unavailable_item.c src/melee/ft/native_script_binding_properties.c
    src/melee/ft/native_script_binding_presentation.c src/melee/ft/native_script_math.c src/melee/ft/native_script_constants.c
    src/melee/ft/native_callback_lifetime.c src/melee/ft/ftparts_table.c src/melee/ft/ftparts_rotation.c
    src/melee/ft/native_fighter_sound_policy.c src/sysdolphin/baselib/random.c src/melee/lb/lbjoint_position.c
    src/melee/gm/native_match_pool.c)
target_include_directories(native_script_presentation_test PRIVATE src extern/dolphin/include "${CMAKE_CURRENT_BINARY_DIR}/script_presentation_fixture")
target_compile_definitions(native_script_presentation_test PRIVATE MELEE_NATIVE=1 _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
target_compile_options(native_script_presentation_test PRIVATE -ffp-contract=off)
add_test(NAME native_script_presentation COMMAND native_script_presentation_test)

set(SCRIPT_TRANSITION_PLANES "${SCRIPT_KINETICS_FIXTURE_DIR}/script_transition_planes_actual.h")
add_custom_command(OUTPUT "${SCRIPT_TRANSITION_PLANES}"
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/script_transition/extract_planes.py"
        --source-root "${CMAKE_CURRENT_SOURCE_DIR}" --output "${SCRIPT_TRANSITION_PLANES}"
    DEPENDS tests/native/script_transition/extract_planes.py src/melee/ft/ftcommon.c VERBATIM)
add_executable(native_script_transition_test tests/native/script_transition/test.c
    tests/native/script_transition/engine_fixture.c tests/native/script_kinetics/engine_fixture.c
    "${SCRIPT_KINETICS_FIXTURE}" "${SCRIPT_STATE_FIXTURE}" "${SCRIPT_TRANSITION_PLANES}"
    ${NATIVE_SCRIPT_FRONTEND_UNITS} ${NATIVE_SCRIPT_EXECUTION_UNITS}
    src/melee/ft/native_script_compile.c src/melee/ft/native_script_moves.c src/melee/ft/native_script_move_lowering.c
    src/melee/ft/native_script_constants.c src/melee/ft/native_script_execution.c src/melee/ft/native_script_event_payload.c
    src/melee/ft/native_script_events.c src/melee/ft/native_script_storage.c src/melee/ft/native_script_bindings.c tests/native/script_bindings_unavailable_item.c
    src/melee/ft/native_script_binding_properties.c src/melee/ft/native_script_binding_kinetics.c src/melee/ft/native_script_binding_state.c
    src/melee/ft/native_script_binding_transition.c src/melee/ft/native_script_math.c src/melee/ft/native_callback_lifetime.c)
target_include_directories(native_script_transition_test PRIVATE src extern/dolphin/include "${SCRIPT_KINETICS_FIXTURE_DIR}")
target_compile_definitions(native_script_transition_test PRIVATE MELEE_NATIVE=1 _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
target_compile_options(native_script_transition_test PRIVATE -ffp-contract=off)
add_test(NAME native_script_transition COMMAND native_script_transition_test "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/script_transition/probe.fms")

# Retained receivers are part of the current direct frontend/execution contract.
set(NATIVE_SCRIPT_RECEIVER_TEST_UNITS
    src/melee/ft/native_script_receiver.c src/melee/ft/native_script_binding_receiver.c
    src/melee/ft/native_script_runtime.c)
foreach(target native_script_execution_test native_script_compile_test native_script_transition_test)
    target_sources(${target} PRIVATE ${NATIVE_SCRIPT_RECEIVER_TEST_UNITS}
        src/melee/gm/native_match_pool.c tests/native/script_receiver_unavailable_identity.c
        tests/native/script_receiver_unavailable_pool.c)
endforeach()
target_sources(native_script_fighter_test PRIVATE src/melee/ft/native_script_receiver.c
    src/melee/ft/native_script_binding_receiver.c tests/native/script_receiver_unavailable_identity.c)

foreach(target native_script_bytecode_test native_script_storage_test native_script_events_test)
    target_include_directories(${target} PRIVATE extern/dolphin/include)
    target_compile_definitions(${target} PRIVATE MELEE_NATIVE=1)
endforeach()

set(NATIVE_SCRIPT_COMPLETE_BANK_UNITS ${NATIVE_SCRIPT_FRONTEND_UNITS} ${NATIVE_SCRIPT_EXECUTION_UNITS}
    src/melee/ft/native_script_compile.c src/melee/ft/native_script_moves.c
    src/melee/ft/native_script_move_lowering.c src/melee/ft/native_script_constants.c
    src/melee/ft/native_script_execution.c src/melee/ft/native_script_event_payload.c
    src/melee/ft/native_script_events.c src/melee/ft/native_script_storage.c
    src/melee/ft/native_script_bindings.c src/melee/ft/native_script_math.c
    ${NATIVE_SCRIPT_RECEIVER_TEST_UNITS} src/melee/ft/native_callback_lifetime.c
    src/melee/gm/native_match_pool.c)
set(SCRIPT_RECEIVER_FIXTURES "${SCRIPT_KINETICS_FIXTURE_DIR}/script_receiver_identity_actual.h"
    "${SCRIPT_KINETICS_FIXTURE_DIR}/script_receiver_command_actual.h")
add_custom_command(OUTPUT ${SCRIPT_RECEIVER_FIXTURES}
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/script_receiver/extract_receivers.py"
        --root "${CMAKE_CURRENT_SOURCE_DIR}" --output "${SCRIPT_KINETICS_FIXTURE_DIR}"
    DEPENDS tests/native/script_receiver/extract_receivers.py tests/native/script_kinetics/extract_primitives.py
        src/melee/it/native_item_owner.c src/melee/it/native_article.c src/melee/ft/native_script_binding_setup.c VERBATIM)
add_executable(native_script_receiver_test tests/native/script_receiver/test.c tests/native/script_receiver/engine_fixture.c
    tests/native/script_kinetics/engine_fixture.c ${NATIVE_SCRIPT_COMPLETE_BANK_UNITS}
    src/melee/ft/native_script_binding_properties.c src/melee/ft/native_script_binding_kinetics.c
    src/melee/ft/native_script_binding_state.c src/melee/ft/native_script_binding_transition.c
    "${SCRIPT_KINETICS_FIXTURE}" "${SCRIPT_STATE_FIXTURE}" "${SCRIPT_TRANSITION_PLANES}" ${SCRIPT_RECEIVER_FIXTURES})
target_include_directories(native_script_receiver_test PRIVATE src extern/dolphin/include "${SCRIPT_KINETICS_FIXTURE_DIR}")
target_compile_definitions(native_script_receiver_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
target_compile_options(native_script_receiver_test PRIVATE -ffp-contract=off)
add_test(NAME native_script_receiver COMMAND native_script_receiver_test "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/script_receiver/probe.fms")

set(NATIVE_SCRIPT_ITEM_ACTOR_TEST_UNITS ${NATIVE_SCRIPT_COMPLETE_BANK_UNITS}
    src/melee/ft/native_script_actor_access.c src/melee/it/native_script_item_actor.c
    src/melee/it/native_script_item_snapshot.c src/melee/gm/native_match_snapshot_pool.c
    src/melee/gm/native_rollback_state.c src/sysdolphin/baselib/gobj.c src/sysdolphin/baselib/gobjinit.c
    src/sysdolphin/baselib/gobjplink.c src/sysdolphin/baselib/gobjproc.c src/sysdolphin/baselib/gobjgxlink.c
    src/sysdolphin/baselib/gobjuserdata.c src/sysdolphin/baselib/gobjobject.c src/sysdolphin/baselib/native_gobj_pool_storage.c
    src/melee/gm/native_match_arena.c src/melee/gm/native_match_scene.c
    src/melee/gr/native_ground_pool.c src/melee/gr/native_green_grid.c src/melee/gr/native_green_grid_snapshot.c
    src/melee/it/native_item_pool.c src/melee/it/native_item_link_pool.c)
add_executable(native_script_item_actor_test tests/native/script_item_actor/test.c tests/native/script_item_actor/boundary.c
    tests/native/script_receiver_unavailable_identity.c
    ${NATIVE_SCRIPT_ITEM_ACTOR_TEST_UNITS})
set(SCRIPT_ITEM_SCHEDULER_DIR "${CMAKE_CURRENT_BINARY_DIR}/script_item_scheduler_fixture")
set(SCRIPT_ITEM_SCHEDULER_FIXTURE "${SCRIPT_ITEM_SCHEDULER_DIR}/actual_scheduler.h")
add_custom_command(OUTPUT "${SCRIPT_ITEM_SCHEDULER_FIXTURE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SCRIPT_ITEM_SCHEDULER_DIR}"
    COMMAND Python3::Interpreter -c "import sys; sys.path.insert(0,sys.argv[1]); import extract_scheduler; from pathlib import Path; extract_scheduler.extract(Path(sys.argv[2]),Path(sys.argv[3]))"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/script_item_scheduler" "${CMAKE_CURRENT_SOURCE_DIR}" "${SCRIPT_ITEM_SCHEDULER_FIXTURE}"
    DEPENDS tests/native/script_item_scheduler/extract_scheduler.py tests/native/script_kinetics/extract_primitives.py
        src/melee/it/item.c src/melee/it/it_26B1.c src/melee/it/itgroundcoll.c VERBATIM)
add_executable(native_script_item_scheduler_test tests/native/script_item_scheduler/test.c tests/native/script_item_scheduler/boundary.c
    "${SCRIPT_ITEM_SCHEDULER_FIXTURE}" ${NATIVE_SCRIPT_ITEM_ACTOR_TEST_UNITS})
foreach(target native_script_item_actor_test native_script_item_scheduler_test)
    target_include_directories(${target} PRIVATE src extern/dolphin/include "${SCRIPT_ITEM_SCHEDULER_DIR}")
    target_compile_definitions(${target} PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    target_compile_options(${target} PRIVATE -ffp-contract=off -ffunction-sections -fdata-sections -fno-zero-initialized-in-bss
        -include "${CMAKE_CURRENT_SOURCE_DIR}/src/melee/gm/native_rollback_state_section.h")
    target_link_options(${target} PRIVATE "LINKER:/opt:ref")
endforeach()
add_test(NAME native_script_item_actor COMMAND native_script_item_actor_test)
add_test(NAME native_script_item_scheduler COMMAND native_script_item_scheduler_test)

set(SCRIPT_ITEM_PROGRAM_DIR "${CMAKE_CURRENT_BINARY_DIR}/script_item_program_fixture")
set(SCRIPT_ITEM_PROGRAM_FIXTURE "${SCRIPT_ITEM_PROGRAM_DIR}/build_functions.inc")
add_custom_command(OUTPUT "${SCRIPT_ITEM_PROGRAM_FIXTURE}"
    COMMAND Python3::Interpreter -c "import sys; sys.path.insert(0,sys.argv[1]); import run; run.extract(sys.argv[2],sys.argv[3])"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/script_item_program" "${CMAKE_CURRENT_SOURCE_DIR}" "${SCRIPT_ITEM_PROGRAM_DIR}"
    DEPENDS tests/native/script_item_program/run.py src/melee/ft/native_script_execution.c VERBATIM)
add_executable(native_script_item_program_test tests/native/script_item_program/test.c tests/native/script_item_program/boundary.c
    "${SCRIPT_ITEM_PROGRAM_FIXTURE}" ${NATIVE_SCRIPT_FRONTEND_UNITS} src/melee/ft/native_script_bytecode.c
    src/melee/ft/native_script_math.c src/melee/ft/native_script_constants.c src/melee/ft/native_script_bindings.c
    src/melee/ft/native_script_compile.c src/melee/ft/native_script_moves.c src/melee/ft/native_script_move_lowering.c
    src/melee/ft/native_script_events.c src/melee/ft/native_script_event_payload.c src/melee/ft/native_script_storage.c
    ${NATIVE_SCRIPT_RECEIVER_TEST_UNITS} src/melee/gm/native_match_pool.c src/melee/ft/native_callback_lifetime.c
    src/melee/ft/native_script_metadata.c src/melee/ft/native_script_item_program.c
    src/melee/ft/native_script_fighter_program.c src/sysdolphin/baselib/dat.c)
target_include_directories(native_script_item_program_test PRIVATE src extern/dolphin/include "${SCRIPT_ITEM_PROGRAM_DIR}")
target_compile_definitions(native_script_item_program_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
target_compile_options(native_script_item_program_test PRIVATE -ffp-contract=off -ffunction-sections -fdata-sections)
target_link_options(native_script_item_program_test PRIVATE "LINKER:/opt:ref")
add_test(NAME native_script_item_program COMMAND native_script_item_program_test)

target_sources(native_script_compile_test PRIVATE src/melee/ft/native_callback_lifetime.c
    tests/native/script_receiver_unavailable_registry.c)
target_sources(native_script_fighter_test PRIVATE src/melee/ft/native_script_actor_access.c
    src/melee/ft/native_script_item_program.c src/melee/ft/native_script_metadata.c
    src/sysdolphin/baselib/dat.c tests/native/script_receiver_unavailable_item_actor.c)

target_sources(native_script_item_program_test PRIVATE tests/native/script_receiver_unavailable_registry.c
    tests/native/script_receiver_unavailable_identity.c tests/native/script_receiver_unavailable_pool.c)

# Pure load-time owner selection is shared; older isolated runtime fixtures
# deliberately have no source entity factories/assets.
foreach(target native_script_native_bindings_test native_script_execution_test native_script_kinetics_test
    native_script_state_test native_script_compile_test native_script_fighter_test native_script_item_test
    native_script_setup_test native_script_combat_test native_script_presentation_test native_script_transition_test
    native_script_receiver_test native_script_item_actor_test native_script_item_scheduler_test native_script_item_program_test)
    target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_spawn.c)
endforeach()
foreach(target native_script_compile_test native_script_fighter_test native_script_transition_test
    native_script_receiver_test native_script_item_actor_test native_script_item_scheduler_test native_script_item_program_test)
    target_sources(${target} PRIVATE src/melee/ft/native_script_binding_spawn_resolve.c)
endforeach()

foreach(target native_script_native_bindings_test native_script_execution_test native_script_kinetics_test
    native_script_state_test native_script_compile_test native_script_fighter_test native_script_item_test
    native_script_setup_test native_script_combat_test native_script_presentation_test native_script_transition_test
    native_script_receiver_test native_script_item_actor_test native_script_item_scheduler_test native_script_item_program_test)
    target_sources(${target} PRIVATE src/melee/ft/native_script_binding_effect_resolve.c tests/native/script_bindings_unavailable_effect.c)
endforeach()

foreach(target native_script_fighter_test native_script_item_actor_test native_script_item_scheduler_test native_gobj_pool_consumer_test)
    target_sources(${target} PRIVATE tests/native/script_effect_actor_absent.c)
endforeach()

foreach(target native_script_native_bindings_test native_script_execution_test native_script_kinetics_test
    native_script_state_test native_script_compile_test native_script_fighter_test native_script_item_test
    native_script_setup_test native_script_combat_test native_script_presentation_test native_script_transition_test
    native_script_receiver_test native_script_item_actor_test native_script_item_scheduler_test native_script_item_program_test)
    target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_actions.c)
    target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_collision.c)
endforeach()

foreach(target native_script_native_bindings_test native_script_execution_test native_script_kinetics_test
    native_script_state_test native_script_setup_test native_script_transition_test native_script_receiver_test)
    target_sources(${target} PRIVATE tests/native/script_fighter_hooks_absent.c)
endforeach()

target_sources(native_script_fighter_test PRIVATE src/melee/ft/native_script_effect_program.c)
target_sources(native_script_item_program_test PRIVATE src/melee/ft/native_script_effect_program.c)


set(SOURCE_PACKET_CODEC_DIR "${CMAKE_CURRENT_BINARY_DIR}/source_packet_codec_fixture")
set(SOURCE_PACKET_CODEC_FIXTURE "${SOURCE_PACKET_CODEC_DIR}/codec_dependencies.inc")
add_custom_command(OUTPUT "${SOURCE_PACKET_CODEC_FIXTURE}"
    COMMAND Python3::Interpreter -c "import sys; sys.path.insert(0,sys.argv[1]); import extract; extract.extract(sys.argv[2],sys.argv[3])"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/fighter_family_package" "${CMAKE_CURRENT_SOURCE_DIR}" "${SOURCE_PACKET_CODEC_DIR}"
    DEPENDS tests/native/fighter_family_package/extract.py src/melee/ft/native_source_asset_validation.c
        src/melee/ft/ftcharacter_asset_layout.c
        src/melee/ft/native_script_moves.c src/melee/ft/ftcharacter_assets.c src/melee/ft/native_fighter_pose_path.c VERBATIM)
foreach(target native_content_store_test native_fighter_family_package_test)
    if(TARGET ${target})
        target_sources(${target} PRIVATE "${SOURCE_PACKET_CODEC_FIXTURE}" src/melee/ft/native_source_asset_record.c
            src/melee/ft/native_source_resources.c
            src/melee/ft/native_fighter_sound_policy.c)
        target_include_directories(${target} PRIVATE extern/dolphin/include "${SOURCE_PACKET_CODEC_DIR}")
    endif()
endforeach()

# Shared engine oracles provide controlled package/data/bank/view ownership.
foreach(target native_script_native_bindings_test native_script_execution_test native_script_kinetics_test
    native_script_state_test native_script_setup_test native_script_combat_test
    native_script_transition_test native_script_receiver_test)
    target_compile_definitions(${target} PRIVATE SCRIPT_FIXTURE_SOURCE_ACTOR=1)
endforeach()

# These targets link the state fixture's controlled attachment query. The common
# absent-hook unit still rejects authored execution but must not define it twice.
foreach(target native_script_native_bindings_test native_script_execution_test native_script_kinetics_test
    native_script_state_test native_script_setup_test native_script_combat_test)
    target_compile_definitions(${target} PRIVATE SCRIPT_FIXTURE_SOURCE_ATTACHED=1)
endforeach()

foreach(target native_script_receiver_test native_script_transition_test)
    target_sources(${target} PRIVATE tests/native/script_kinetics/unavailable_runtime.c
        tests/native/script_bindings_unavailable_presentation.c)
endforeach()

# The isolated primitive header extracts the exact motion bounds predicate.
# Complete compiler fixtures already link its real source unit.
foreach(target native_script_native_bindings_test native_script_execution_test native_script_kinetics_test
    native_script_state_test native_script_setup_test native_script_combat_test native_script_transition_test native_script_receiver_test)
    get_target_property(fixture_sources ${target} SOURCES)
    if("src/melee/ft/native_script_moves.c" IN_LIST fixture_sources)
        target_compile_definitions(${target} PRIVATE SCRIPT_FIXTURE_FULL_MOVE_UNIT=1)
    endif()
endforeach()

target_sources(native_script_item_test PRIVATE tests/native/script_source_fighter_absent.c)

# Catalog-only and unrelated engine-family fixtures deliberately lack ledge
# simulation. Their unavailable leaf is separate from the real primitive probe.
get_property(source_catalog_test_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(target IN LISTS source_catalog_test_targets)
    if(target MATCHES "^native_.*_test$")
        get_target_property(fixture_sources ${target} SOURCES)
        if("src/melee/ft/native_script_bindings.c" IN_LIST fixture_sources AND
           NOT "src/melee/ft/native_script_binding_ledge.c" IN_LIST fixture_sources)
            target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_ledge.c)
        endif()
        if("src/melee/ft/native_script_bindings.c" IN_LIST fixture_sources AND
           NOT "src/melee/ft/native_script_binding_jump.c" IN_LIST fixture_sources)
            target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_jump.c)
        endif()
        if("src/melee/ft/native_script_bindings.c" IN_LIST fixture_sources AND
           NOT "src/melee/ft/native_script_binding_forms.c" IN_LIST fixture_sources)
            target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_forms.c)
        endif()
        if("src/melee/ft/native_script_bindings.c" IN_LIST fixture_sources AND
           NOT "src/melee/ft/native_script_binding_feedback.c" IN_LIST fixture_sources AND
           NOT "tests/native/script_action_contracts/test.c" IN_LIST fixture_sources)
            target_sources(${target} PRIVATE tests/native/script_bindings_unavailable_feedback.c)
        endif()
    endif()
endforeach()

# Actor-aware state and accessory guards require a pool reference. Primitive-only
# consumers use an explicitly empty scene pool; actual actor fixtures own theirs.
foreach(target native_script_native_bindings_test native_script_execution_test native_script_kinetics_test
    native_script_state_test native_script_setup_test native_script_combat_test native_script_item_test)
    get_target_property(primitive_sources ${target} SOURCES)
    if(NOT "src/melee/gm/native_match_pool.c" IN_LIST primitive_sources)
        target_sources(${target} PRIVATE src/melee/gm/native_match_pool.c)
    endif()
    if(NOT target STREQUAL "native_script_combat_test" AND
       NOT "src/sysdolphin/baselib/native_gobj_pool_storage.c" IN_LIST primitive_sources AND
       NOT "tests/native/script_receiver_unavailable_pool.c" IN_LIST primitive_sources)
        target_sources(${target} PRIVATE tests/native/script_receiver_unavailable_pool.c)
    endif()
endforeach()

# These owner fixtures do not render; share the current presentation boundary.
target_sources(native_script_item_actor_test PRIVATE tests/native/script_bindings_unavailable_presentation.c)
target_sources(native_script_item_scheduler_test PRIVATE tests/native/script_bindings_unavailable_presentation.c)
