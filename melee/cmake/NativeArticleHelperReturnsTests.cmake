# Compile both callback-generation forms and execute their generated functions.
# These checks create no game window, renderer, adapter or audio device.
set(ARTICLE_RETURNS_DIR "${CMAKE_CURRENT_BINARY_DIR}/article_helper_returns")
set(ARTICLE_RETURNS_ACTION "${ARTICLE_RETURNS_DIR}/action/article_returns.inc")
set(ARTICLE_RETURNS_STATE "${ARTICLE_RETURNS_DIR}/state/article_returns.inc")
set(ARTICLE_RETURNS_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/article_helper_returns")

add_executable(native_script_article_returns_compiler_test
    tests/native/article_helper_returns/compiler.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_article_returns_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_article_returns_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_custom_command(OUTPUT "${ARTICLE_RETURNS_ACTION}" "${ARTICLE_RETURNS_STATE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${ARTICLE_RETURNS_DIR}/action" "${ARTICLE_RETURNS_DIR}/state"
    COMMAND $<TARGET_FILE:native_script_article_returns_compiler_test>
        "${ARTICLE_RETURNS_SOURCE}/probe.fms" "${ARTICLE_RETURNS_ACTION}" "${ARTICLE_RETURNS_STATE}"
    DEPENDS native_script_article_returns_compiler_test tests/native/article_helper_returns/probe.fms VERBATIM)
add_test(NAME native_script_article_returns_compiler
    COMMAND native_script_article_returns_compiler_test "${ARTICLE_RETURNS_SOURCE}/probe.fms"
        "${ARTICLE_RETURNS_DIR}/ctest_action.inc" "${ARTICLE_RETURNS_DIR}/ctest_state.inc")
foreach(form IN ITEMS action state)
    add_executable(native_script_article_returns_${form}_test
        tests/native/article_helper_returns/module.c "${ARTICLE_RETURNS_ACTION}" "${ARTICLE_RETURNS_STATE}")
    target_include_directories(native_script_article_returns_${form}_test PRIVATE
        "${ARTICLE_RETURNS_DIR}/${form}" src extern/dolphin/include)
    target_compile_definitions(native_script_article_returns_${form}_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    add_test(NAME native_script_article_returns_${form} COMMAND native_script_article_returns_${form}_test)
endforeach()
