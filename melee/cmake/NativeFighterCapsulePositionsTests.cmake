# Direct collision-coordinate access, distinct from the bone attachment offset.
set(CAPSULE_POSITIONS_DIR "${CMAKE_CURRENT_BINARY_DIR}/fighter_capsule_positions")
set(CAPSULE_POSITIONS_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/fighter_capsule_positions")
set(CAPSULE_POSITIONS_ACTION "${CAPSULE_POSITIONS_DIR}/generated.inc")
set(CAPSULE_POSITIONS_STATE "${CAPSULE_POSITIONS_DIR}/generated_state.inc")
set(CAPSULE_POSITIONS_ACTUAL "${CAPSULE_POSITIONS_DIR}/actual.inc")
add_executable(native_script_capsule_positions_compiler_test
    tests/native/fighter_capsule_positions/compiler_test.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_capsule_positions_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_capsule_positions_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_custom_command(OUTPUT "${CAPSULE_POSITIONS_ACTION}" "${CAPSULE_POSITIONS_STATE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CAPSULE_POSITIONS_DIR}"
    COMMAND $<TARGET_FILE:native_script_capsule_positions_compiler_test>
        "${CAPSULE_POSITIONS_SOURCE}/fixture.fms" "${CAPSULE_POSITIONS_ACTION}" "${CAPSULE_POSITIONS_STATE}"
    DEPENDS native_script_capsule_positions_compiler_test tests/native/fighter_capsule_positions/fixture.fms VERBATIM)
add_custom_command(OUTPUT "${CAPSULE_POSITIONS_ACTUAL}"
    COMMAND Python3::Interpreter "${CAPSULE_POSITIONS_SOURCE}/fixture.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/melee/ft/ftcoll.c" "${CAPSULE_POSITIONS_DIR}"
    DEPENDS tests/native/fighter_capsule_positions/fixture.py src/melee/ft/ftcoll.c VERBATIM)
add_test(NAME native_script_capsule_positions_compiler
    COMMAND native_script_capsule_positions_compiler_test "${CAPSULE_POSITIONS_SOURCE}/fixture.fms"
        "${CAPSULE_POSITIONS_DIR}/ctest_action.inc" "${CAPSULE_POSITIONS_DIR}/ctest_state.inc")
foreach(form IN ITEMS action state)
    add_executable(native_script_capsule_positions_${form}_test
        tests/native/fighter_capsule_positions/module_test.c
        "${CAPSULE_POSITIONS_ACTION}" "${CAPSULE_POSITIONS_STATE}" "${CAPSULE_POSITIONS_ACTUAL}")
    target_include_directories(native_script_capsule_positions_${form}_test PRIVATE
        "${CAPSULE_POSITIONS_DIR}" src extern/dolphin/include)
    target_compile_definitions(native_script_capsule_positions_${form}_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    if(form STREQUAL "state")
        target_compile_definitions(native_script_capsule_positions_${form}_test PRIVATE CAPSULE_STATE_ROUTE=1)
    endif()
    add_test(NAME native_script_capsule_positions_${form} COMMAND native_script_capsule_positions_${form}_test)
endforeach()
