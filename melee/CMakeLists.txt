cmake_minimum_required(VERSION 3.25)
project(freak_melee LANGUAGES C CXX)

if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "The native client currently targets Windows x86-64.")
endif()

if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang" OR MINGW)
    message(FATAL_ERROR "Use LLVM Clang targeting Windows/MSVC, not the MinGW toolchain.")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(MELEE_ISO "${CMAKE_CURRENT_SOURCE_DIR}/../Super Smash Bros. Melee (USA) (v1.02).iso"
    CACHE FILEPATH "Original USA v1.02 ISO used for native asset preparation")
if(NOT EXISTS "${MELEE_ISO}")
    message(FATAL_ERROR "Set MELEE_ISO to the original USA v1.02 Melee ISO.")
endif()
set(MELEE_GENERATED_ASSETS "${CMAKE_BINARY_DIR}/assets")
set(MELEE_DSP_COEFFICIENTS "${CMAKE_CURRENT_SOURCE_DIR}/../.local/dsp_coef.bin"
    CACHE FILEPATH "Local 4096-byte DSP resampling coefficient table")
set(MELEE_AUDIO_COEFFICIENTS "${MELEE_GENERATED_ASSETS}/include/sysdolphin/baselib/audio_coefficients.inc")
add_custom_command(OUTPUT "${MELEE_AUDIO_COEFFICIENTS}"
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/tools/prepare_audio_coefficients.py"
        --input "${MELEE_DSP_COEFFICIENTS}" --output "${MELEE_AUDIO_COEFFICIENTS}"
    DEPENDS "${MELEE_DSP_COEFFICIENTS}" tools/prepare_audio_coefficients.py
    VERBATIM)
add_custom_target(melee_audio_coefficients DEPENDS "${MELEE_AUDIO_COEFFICIENTS}")

set(MELEE_GENERATED_FONTS
    "${MELEE_GENERATED_ASSETS}/include/sysdolphin/baselib/debug_font.inc"
    "${MELEE_GENERATED_ASSETS}/include/sysdolphin/baselib/sislib_font.inc")
add_custom_command(
    OUTPUT ${MELEE_GENERATED_FONTS}
        "${MELEE_GENERATED_ASSETS}/main.dol"
        "${MELEE_GENERATED_ASSETS}/extraction.json"
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/tools/prepare_native_assets.py"
        --iso "${MELEE_ISO}" --output "${MELEE_GENERATED_ASSETS}"
    DEPENDS "${MELEE_ISO}" tools/prepare_native_assets.py
        config/GALE01/config.yml config/GALE01/symbols.txt
    VERBATIM)
add_custom_target(melee_embedded_assets DEPENDS ${MELEE_GENERATED_FONTS})

# Compile the full gameplay/HSD tree in place for the native runtime.
file(GLOB_RECURSE MELEE_GAME_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/melee/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/melee/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sysdolphin/*.c")
set(NATIVE_INVERSE_TRIG "${CMAKE_CURRENT_SOURCE_DIR}/src/melee/lb/native_float_math.c")
list(REMOVE_ITEM MELEE_GAME_SOURCES "${NATIVE_INVERSE_TRIG}")
file(GLOB_RECURSE CORE_MATH_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/extern/core-math/*.c")
add_library(melee_math STATIC ${CORE_MATH_SOURCES} "${NATIVE_INVERSE_TRIG}")
set(NATIVE_MATH_BUILD_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/src/melee/lb/native_math_build.h")
if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(melee_math PRIVATE /clang:-ffp-contract=off /clang:-frounding-math)
    set(NATIVE_MATH_COMPILE_OPTIONS /clang:-include "/clang:${NATIVE_MATH_BUILD_HEADER}")
else()
    target_compile_options(melee_math PRIVATE -ffp-contract=off -frounding-math)
    set(NATIVE_MATH_COMPILE_OPTIONS "SHELL:-include \"${NATIVE_MATH_BUILD_HEADER}\"")
endif()
add_executable(native_math_test tests/native/native_math_test.c)
target_include_directories(native_math_test PRIVATE src)
target_compile_options(native_math_test PRIVATE ${NATIVE_MATH_COMPILE_OPTIONS})
target_link_libraries(native_math_test PRIVATE melee_math)
add_test(NAME native_math COMMAND native_math_test)
list(SORT MELEE_GAME_SOURCES)
list(LENGTH MELEE_GAME_SOURCES MELEE_GAME_SOURCE_COUNT)
message(STATUS "Native compilation covers ${MELEE_GAME_SOURCE_COUNT} game/HSD source files")

set(NATIVE_FIGHTER_SCRIPT_SOURCES
    tools/native_fighter_script.c
    tools/native_fighter_item_functions.c tools/native_fighter_item_statements.c
    tools/native_fighter_item_terrain.c
    tools/native_fighter_item_script.c tools/native_fighter_item_links.c tools/native_fighter_item_operations.c tools/native_fighter_article_operations.c
    tools/native_fighter_item_presentation.c tools/native_fighter_article_resources.c
    tools/native_fighter_script_code.c tools/native_fighter_script_state.c tools/native_fighter_script_functions.c
    tools/native_fighter_script_types.c tools/native_fighter_script_article_refs.c
    tools/native_fighter_script_vectors.c tools/native_fighter_script_kinetics.c
    tools/native_fighter_script_spatial.c
    tools/native_fighter_script_relations.c
    tools/native_fighter_script_queries.c
    tools/native_fighter_script_colors.c
    tools/native_fighter_script_pose.c
    tools/native_fighter_script_sound_policy.c
    tools/native_fighter_script_surface_policy.c
    tools/native_fighter_script_lifetime.c tools/native_fighter_script_store.c
    tools/native_fighter_script_motion_policy.c
    tools/native_fighter_script_action_metadata.c
    tools/native_fighter_script_model_effect.c
    tools/native_fighter_script_jump.c
    tools/native_fighter_script_projectile.c
    tools/native_fighter_script_signal.c
    tools/native_fighter_item_tables.c
    tools/native_fighter_script_trail.c
    tools/native_fighter_script_primitives.c tools/native_fighter_script_sound.c tools/native_fighter_script_guard.c tools/native_fighter_script_terrain.c
    tools/native_fighter_script_collision.c
    tools/native_fighter_script_smash.c
    tools/native_fighter_script_cpu.c)

foreach(script_contract article_resource article_helpers article_terrain ness_flash ness_thunder guard_front_filter)
    if(script_contract STREQUAL "article_resource")
        set(contract_source tests/native/article_resource_script_test.c)
    elseif(script_contract STREQUAL "article_helpers")
        set(contract_source tests/native/article_helpers_compiler_test.c)
    elseif(script_contract STREQUAL "article_terrain")
        set(contract_source tests/native/article_terrain_compiler_test.c)
    elseif(script_contract STREQUAL "ness_flash")
        set(contract_source tests/native/ness_flash_compiler_test.c)
    elseif(script_contract STREQUAL "guard_front_filter")
        set(contract_source tests/native/script_guard_front_filter_test.c)
    else()
        set(contract_source tests/native/ness_thunder_compiler_test.c)
    endif()
    add_executable(native_${script_contract}_compiler_test ${contract_source}
        ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
    if(script_contract STREQUAL "ness_thunder")
        target_sources(native_${script_contract}_compiler_test PRIVATE
            src/melee/ft/native_fighter_parameter_import.c
            src/melee/ft/native_fighter_article_bindings.c src/sysdolphin/baselib/dat.c)
    endif()
    target_include_directories(native_${script_contract}_compiler_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_${script_contract}_compiler_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    add_test(NAME native_${script_contract}_compiler COMMAND native_${script_contract}_compiler_test)
endforeach()
# Generic direct seven-channel calls, in both compiler routes.
add_executable(native_script_sound_channel_compiler_test
    tests/native/sound_channel_calls/compiler.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_sound_channel_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_sound_channel_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_script_sound_channel_compiler COMMAND native_script_sound_channel_compiler_test)
set(SOUND_CHANNEL_ACTION "${CMAKE_CURRENT_BINARY_DIR}/sound_channel_action.inc")
set(SOUND_CHANNEL_CALLBACK "${CMAKE_CURRENT_BINARY_DIR}/sound_channel_callback.inc")
add_custom_command(OUTPUT "${SOUND_CHANNEL_ACTION}" "${SOUND_CHANNEL_CALLBACK}"
    COMMAND $<TARGET_FILE:native_script_sound_channel_compiler_test> "${SOUND_CHANNEL_ACTION}" "${SOUND_CHANNEL_CALLBACK}"
    DEPENDS native_script_sound_channel_compiler_test VERBATIM)
foreach(SOUND_CHANNEL_ROUTE IN ITEMS action callback)
    set(SOUND_CHANNEL_CALLBACKS 0)
    set(SOUND_CHANNEL_MODULE "sound_channel_action.inc")
    if(SOUND_CHANNEL_ROUTE STREQUAL "callback")
        set(SOUND_CHANNEL_CALLBACKS 1)
        set(SOUND_CHANNEL_MODULE "sound_channel_callback.inc")
    endif()
    add_executable(native_script_sound_channel_${SOUND_CHANNEL_ROUTE}_test
        tests/native/sound_channel_calls/module_test.c "${SOUND_CHANNEL_ACTION}" "${SOUND_CHANNEL_CALLBACK}")
    target_include_directories(native_script_sound_channel_${SOUND_CHANNEL_ROUTE}_test PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}" src extern/dolphin/include)
    target_compile_definitions(native_script_sound_channel_${SOUND_CHANNEL_ROUTE}_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS SOUND_CHANNEL_CALLBACKS=${SOUND_CHANNEL_CALLBACKS}
        SOUND_CHANNEL_MODULE="${SOUND_CHANNEL_MODULE}")
    add_test(NAME native_script_sound_channel_${SOUND_CHANNEL_ROUTE} COMMAND native_script_sound_channel_${SOUND_CHANNEL_ROUTE}_test)
endforeach()


# Alternate-member activation and destination hook, in both script routes.
add_executable(native_partner_transfer_compiler_test
    tests/native/partner_transfer/compiler.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_partner_transfer_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_partner_transfer_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_partner_transfer_compiler COMMAND native_partner_transfer_compiler_test)
set(PARTNER_TRANSFER_ACTION "${CMAKE_CURRENT_BINARY_DIR}/partner_transfer_action.inc")
set(PARTNER_TRANSFER_CALLBACK "${CMAKE_CURRENT_BINARY_DIR}/partner_transfer_callback.inc")
add_custom_command(OUTPUT "${PARTNER_TRANSFER_ACTION}" "${PARTNER_TRANSFER_CALLBACK}"
    COMMAND $<TARGET_FILE:native_partner_transfer_compiler_test> "${PARTNER_TRANSFER_ACTION}" "${PARTNER_TRANSFER_CALLBACK}"
    DEPENDS native_partner_transfer_compiler_test VERBATIM)
foreach(PARTNER_TRANSFER_ROUTE IN ITEMS action callback)
    set(PARTNER_TRANSFER_CALLBACKS 0)
    if(PARTNER_TRANSFER_ROUTE STREQUAL "callback")
        set(PARTNER_TRANSFER_CALLBACKS 1)
    endif()
    add_executable(native_partner_transfer_${PARTNER_TRANSFER_ROUTE}_test
        tests/native/partner_transfer/module.c "${PARTNER_TRANSFER_ACTION}" "${PARTNER_TRANSFER_CALLBACK}")
    target_include_directories(native_partner_transfer_${PARTNER_TRANSFER_ROUTE}_test PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}" src extern/dolphin/include)
    target_compile_definitions(native_partner_transfer_${PARTNER_TRANSFER_ROUTE}_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS PARTNER_TRANSFER_CALLBACKS=${PARTNER_TRANSFER_CALLBACKS}
        PARTNER_TRANSFER_MODULE="partner_transfer_${PARTNER_TRANSFER_ROUTE}.inc")
    add_test(NAME native_partner_transfer_${PARTNER_TRANSFER_ROUTE} COMMAND native_partner_transfer_${PARTNER_TRANSFER_ROUTE}_test)
endforeach()

# Shared paired-fighter observations and typed output publication in both routes.
add_executable(native_fighter_relations_compiler_test
    tests/native/fighter_relations/compiler.c ${NATIVE_FIGHTER_SCRIPT_SOURCES}
    src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_fighter_relations_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_fighter_relations_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_fighter_relations_compiler COMMAND native_fighter_relations_compiler_test)
set(RELATIONS_ACTION "${CMAKE_CURRENT_BINARY_DIR}/relations_action.inc")
set(RELATIONS_CALLBACK "${CMAKE_CURRENT_BINARY_DIR}/relations_callback.inc")
add_custom_command(OUTPUT "${RELATIONS_ACTION}" "${RELATIONS_CALLBACK}"
    COMMAND $<TARGET_FILE:native_fighter_relations_compiler_test> "${RELATIONS_ACTION}" "${RELATIONS_CALLBACK}"
    DEPENDS native_fighter_relations_compiler_test VERBATIM)
foreach(RELATIONS_ROUTE IN ITEMS action callback)
    set(RELATIONS_CALLBACKS 0)
    if(RELATIONS_ROUTE STREQUAL "callback")
        set(RELATIONS_CALLBACKS 1)
    endif()
    add_executable(native_fighter_relations_${RELATIONS_ROUTE}_test
        tests/native/fighter_relations/module.c "${RELATIONS_ACTION}" "${RELATIONS_CALLBACK}")
    target_include_directories(native_fighter_relations_${RELATIONS_ROUTE}_test PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}" src extern/dolphin/include)
    target_compile_definitions(native_fighter_relations_${RELATIONS_ROUTE}_test PRIVATE MELEE_NATIVE=1
        _CRT_SECURE_NO_WARNINGS RELATIONS_CALLBACKS=${RELATIONS_CALLBACKS}
        RELATIONS_MODULE="relations_${RELATIONS_ROUTE}.inc")
    add_test(NAME native_fighter_relations_${RELATIONS_ROUTE} COMMAND native_fighter_relations_${RELATIONS_ROUTE}_test)
endforeach()

# Explicit-position opponent queries and atomic article model translation.
add_executable(native_script_queries_compiler_test
    tests/native/script_queries/compiler.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_queries_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_queries_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_script_queries_compiler COMMAND native_script_queries_compiler_test)
set(QUERY_ACTION "${CMAKE_CURRENT_BINARY_DIR}/script_queries_action.inc")
set(QUERY_CALLBACK "${CMAKE_CURRENT_BINARY_DIR}/script_queries_callback.inc")
add_custom_command(OUTPUT "${QUERY_ACTION}" "${QUERY_CALLBACK}"
    COMMAND $<TARGET_FILE:native_script_queries_compiler_test> "${QUERY_ACTION}" "${QUERY_CALLBACK}"
    DEPENDS native_script_queries_compiler_test VERBATIM)
foreach(QUERY_ROUTE IN ITEMS action callback)
    set(QUERY_CALLBACKS 0)
    if(QUERY_ROUTE STREQUAL "callback")
        set(QUERY_CALLBACKS 1)
    endif()
    add_executable(native_script_queries_${QUERY_ROUTE}_test
        tests/native/script_queries/module_test.c "${QUERY_ACTION}" "${QUERY_CALLBACK}")
    target_include_directories(native_script_queries_${QUERY_ROUTE}_test PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}" src extern/dolphin/include)
    target_compile_definitions(native_script_queries_${QUERY_ROUTE}_test PRIVATE MELEE_NATIVE=1
        _CRT_SECURE_NO_WARNINGS QUERY_CALLBACKS=${QUERY_CALLBACKS}
        QUERY_MODULE="script_queries_${QUERY_ROUTE}.inc")
    add_test(NAME native_script_queries_${QUERY_ROUTE} COMMAND native_script_queries_${QUERY_ROUTE}_test)
endforeach()

# Authored early CPU policy, before common mode/history initialization.
add_executable(native_script_cpu_early_compiler_test
    tests/native/cpu_early_policy/compiler.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_cpu_early_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_cpu_early_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_script_cpu_early_compiler COMMAND native_script_cpu_early_compiler_test)
set(EARLY_CPU_ACTION "${CMAKE_CURRENT_BINARY_DIR}/early_cpu_action.inc")
set(EARLY_CPU_CALLBACK "${CMAKE_CURRENT_BINARY_DIR}/early_cpu_callback.inc")
add_custom_command(OUTPUT "${EARLY_CPU_ACTION}" "${EARLY_CPU_CALLBACK}"
    COMMAND $<TARGET_FILE:native_script_cpu_early_compiler_test> "${EARLY_CPU_ACTION}" "${EARLY_CPU_CALLBACK}"
    DEPENDS native_script_cpu_early_compiler_test VERBATIM)
foreach(EARLY_CPU_ROUTE IN ITEMS action callback)
    add_executable(native_script_cpu_early_${EARLY_CPU_ROUTE}_test
        tests/native/cpu_early_policy/module.c "${EARLY_CPU_ACTION}" "${EARLY_CPU_CALLBACK}")
    target_include_directories(native_script_cpu_early_${EARLY_CPU_ROUTE}_test PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}" src extern/dolphin/include)
    target_compile_definitions(native_script_cpu_early_${EARLY_CPU_ROUTE}_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS EARLY_CPU_MODULE="early_cpu_${EARLY_CPU_ROUTE}.inc")
    add_test(NAME native_script_cpu_early_${EARLY_CPU_ROUTE}
        COMMAND native_script_cpu_early_${EARLY_CPU_ROUTE}_test)
endforeach()

# Generic follower control/history/planner interface, both compiler routes.
add_executable(native_script_cpu_follower_compiler_test
    tests/native/cpu_follower_foundation/compiler.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_cpu_follower_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_cpu_follower_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_test(NAME native_script_cpu_follower_compiler COMMAND native_script_cpu_follower_compiler_test)
set(CPU_FOLLOWER_DIR "${CMAKE_CURRENT_BINARY_DIR}/cpu_follower_fixture")
file(MAKE_DIRECTORY "${CPU_FOLLOWER_DIR}")
set(CPU_FOLLOWER_ACTION "${CPU_FOLLOWER_DIR}/follower_action.inc")
set(CPU_FOLLOWER_CALLBACK "${CPU_FOLLOWER_DIR}/follower_callback.inc")
add_custom_command(OUTPUT "${CPU_FOLLOWER_ACTION}" "${CPU_FOLLOWER_CALLBACK}"
    COMMAND $<TARGET_FILE:native_script_cpu_follower_compiler_test> "${CPU_FOLLOWER_ACTION}" "${CPU_FOLLOWER_CALLBACK}"
    DEPENDS native_script_cpu_follower_compiler_test VERBATIM)
foreach(CPU_FOLLOWER_ROUTE action callback)
    add_executable(native_script_cpu_follower_${CPU_FOLLOWER_ROUTE}_test
        tests/native/cpu_follower_foundation/module.c "${CPU_FOLLOWER_ACTION}" "${CPU_FOLLOWER_CALLBACK}")
    target_include_directories(native_script_cpu_follower_${CPU_FOLLOWER_ROUTE}_test PRIVATE "${CPU_FOLLOWER_DIR}" src extern/dolphin/include)
    target_compile_definitions(native_script_cpu_follower_${CPU_FOLLOWER_ROUTE}_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS FOLLOWER_MODULE="follower_${CPU_FOLLOWER_ROUTE}.inc")
    add_test(NAME native_script_cpu_follower_${CPU_FOLLOWER_ROUTE} COMMAND native_script_cpu_follower_${CPU_FOLLOWER_ROUTE}_test)
endforeach()

# Generic article surface responses and asynchronous joint effect calls.
add_executable(native_article_surface_compiler_test
    tests/native/article_surface_calls/compiler.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_article_surface_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_article_surface_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_article_surface_compiler COMMAND native_article_surface_compiler_test)
set(ARTICLE_SURFACE_ACTION "${CMAKE_CURRENT_BINARY_DIR}/article_surface_action.inc")
set(ARTICLE_SURFACE_CALLBACK "${CMAKE_CURRENT_BINARY_DIR}/article_surface_callback.inc")
add_custom_command(OUTPUT "${ARTICLE_SURFACE_ACTION}" "${ARTICLE_SURFACE_CALLBACK}"
    COMMAND $<TARGET_FILE:native_article_surface_compiler_test> "${ARTICLE_SURFACE_ACTION}" "${ARTICLE_SURFACE_CALLBACK}"
    DEPENDS native_article_surface_compiler_test VERBATIM)
foreach(ARTICLE_SURFACE_ROUTE IN ITEMS action callback)
    add_executable(native_article_surface_${ARTICLE_SURFACE_ROUTE}_test
        tests/native/article_surface_calls/module.c "${ARTICLE_SURFACE_ACTION}" "${ARTICLE_SURFACE_CALLBACK}")
    target_include_directories(native_article_surface_${ARTICLE_SURFACE_ROUTE}_test PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}" src extern/dolphin/include)
    target_compile_definitions(native_article_surface_${ARTICLE_SURFACE_ROUTE}_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS ARTICLE_SURFACE_MODULE="article_surface_${ARTICLE_SURFACE_ROUTE}.inc")
    add_test(NAME native_article_surface_${ARTICLE_SURFACE_ROUTE} COMMAND native_article_surface_${ARTICLE_SURFACE_ROUTE}_test)
endforeach()

# Immutable seven-channel fighter sound suppression, in both compiler routes.
add_executable(native_script_sound_policy_compiler_test
    tests/native/fighter_sound_mute_policy/compiler.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_sound_policy_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_sound_policy_compiler_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_test(NAME native_script_sound_policy_compiler COMMAND native_script_sound_policy_compiler_test)
set(SOUND_POLICY_DIR "${CMAKE_CURRENT_BINARY_DIR}/sound_policy_fixture")
file(MAKE_DIRECTORY "${SOUND_POLICY_DIR}")
set(SOUND_POLICY_INCLUDES)
foreach(sound_case decl_action decl_state inherit_action inherit_state)
    list(APPEND SOUND_POLICY_INCLUDES "${SOUND_POLICY_DIR}/${sound_case}.inc")
endforeach()
add_custom_command(OUTPUT ${SOUND_POLICY_INCLUDES}
    COMMAND $<TARGET_FILE:native_script_sound_policy_compiler_test> ${SOUND_POLICY_INCLUDES}
    DEPENDS native_script_sound_policy_compiler_test VERBATIM)
foreach(sound_case decl_action decl_state inherit_action inherit_state)
    if(sound_case MATCHES "^inherit")
        set(sound_inherit 1)
    else()
        set(sound_inherit 0)
    endif()
    add_executable(native_script_sound_policy_${sound_case}_test
        tests/native/fighter_sound_mute_policy/module_test.c ${SOUND_POLICY_INCLUDES})
    target_include_directories(native_script_sound_policy_${sound_case}_test PRIVATE
        src extern/dolphin/include "${SOUND_POLICY_DIR}")
    target_compile_definitions(native_script_sound_policy_${sound_case}_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES
        SOUND_MODULE="${sound_case}.inc" SOUND_INHERIT=${sound_inherit})
    add_test(NAME native_script_sound_policy_${sound_case} COMMAND native_script_sound_policy_${sound_case}_test)
endforeach()

# Immutable sound routing/surface policy and typed shared-load flags.
add_executable(native_script_common_init_compiler_test
    tests/native/fighter_common_init_policy/compiler.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_common_init_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_common_init_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_test(NAME native_script_common_init_compiler COMMAND native_script_common_init_compiler_test)
set(COMMON_INIT_DIR "${CMAKE_CURRENT_BINARY_DIR}/common_init_fixture")
file(MAKE_DIRECTORY "${COMMON_INIT_DIR}")
set(COMMON_INIT_ACTION "${COMMON_INIT_DIR}/action.inc")
set(COMMON_INIT_STATE "${COMMON_INIT_DIR}/state.inc")
add_custom_command(OUTPUT "${COMMON_INIT_ACTION}" "${COMMON_INIT_STATE}"
    COMMAND $<TARGET_FILE:native_script_common_init_compiler_test> "${COMMON_INIT_ACTION}" "${COMMON_INIT_STATE}"
    DEPENDS native_script_common_init_compiler_test VERBATIM)
foreach(COMMON_INIT_ROUTE action state)
    if(COMMON_INIT_ROUTE STREQUAL "state")
        set(COMMON_INIT_IS_STATE 1)
    else()
        set(COMMON_INIT_IS_STATE 0)
    endif()
    add_executable(native_script_common_init_${COMMON_INIT_ROUTE}_test
        tests/native/fighter_common_init_policy/module.c
        src/melee/ft/native_fighter_sound_policy.c "${COMMON_INIT_ACTION}" "${COMMON_INIT_STATE}")
    target_include_directories(native_script_common_init_${COMMON_INIT_ROUTE}_test PRIVATE "${COMMON_INIT_DIR}" src extern/dolphin/include)
    target_compile_definitions(native_script_common_init_${COMMON_INIT_ROUTE}_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS COMMON_INIT_STATE=${COMMON_INIT_IS_STATE} COMMON_INIT_MODULE="${COMMON_INIT_ROUTE}.inc")
    add_test(NAME native_script_common_init_${COMMON_INIT_ROUTE} COMMAND native_script_common_init_${COMMON_INIT_ROUTE}_test)
endforeach()
add_custom_command(OUTPUT "${COMMON_INIT_DIR}/original_common_init.inc" "${COMMON_INIT_DIR}/candidate_common_init.inc"
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/fighter_common_init_policy/fixture.py"
        --root "${CMAKE_CURRENT_SOURCE_DIR}" --output "${COMMON_INIT_DIR}"
    DEPENDS tests/native/fighter_common_init_policy/fixture.py
        tests/native/fighter_common_init_policy/originals.h tests/native/fighter_common_init_policy/reference_manifest.json
        src/melee/ft/ftmovement.c src/melee/ft/ft_0877.c src/melee/ft/ftlib.c VERBATIM)
add_executable(native_fighter_common_init_source_test
    tests/native/fighter_common_init_policy/source_test.c src/melee/ft/native_fighter_sound_policy.c
    "${COMMON_INIT_DIR}/original_common_init.inc" "${COMMON_INIT_DIR}/candidate_common_init.inc")
target_include_directories(native_fighter_common_init_source_test PRIVATE "${COMMON_INIT_DIR}" src extern/dolphin/include)
target_compile_definitions(native_fighter_common_init_source_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_fighter_common_init_source COMMAND native_fighter_common_init_source_test)

set(GUARD_FRONT_FILTER_DIR "${CMAKE_CURRENT_BINARY_DIR}/guard_front_filter_fixture")
set(GUARD_FRONT_FILTER_INCLUDES
    "${GUARD_FRONT_FILTER_DIR}/script_guard_front_filter_generated.inc"
    "${GUARD_FRONT_FILTER_DIR}/script_guard_front_filter_original.inc")
add_custom_command(OUTPUT ${GUARD_FRONT_FILTER_INCLUDES}
    COMMAND Python3::Interpreter
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/script_guard_front_filter_fixture.py"
        --root "${CMAKE_CURRENT_SOURCE_DIR}" --output "${GUARD_FRONT_FILTER_DIR}"
        --compiler-test $<TARGET_FILE:native_guard_front_filter_compiler_test>
    DEPENDS native_guard_front_filter_compiler_test
        tests/native/script_guard_front_filter_fixture.py src/melee/ft/ftcoll.c
    VERBATIM)
add_executable(native_guard_front_filter_module_test
    tests/native/script_guard_front_filter_module_test.c ${GUARD_FRONT_FILTER_INCLUDES})
target_include_directories(native_guard_front_filter_module_test PRIVATE
    src extern/dolphin/include "${GUARD_FRONT_FILTER_DIR}")
target_compile_definitions(native_guard_front_filter_module_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_test(NAME native_guard_front_filter_module COMMAND native_guard_front_filter_module_test)
set(RESOURCE_SCRIPT_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/article_resource_script_fixture.c")
add_custom_command(OUTPUT "${RESOURCE_SCRIPT_FIXTURE}"
    COMMAND $<TARGET_FILE:native_article_resource_compiler_test> "${RESOURCE_SCRIPT_FIXTURE}"
    DEPENDS native_article_resource_compiler_test VERBATIM)
add_library(native_article_resource_generated OBJECT "${RESOURCE_SCRIPT_FIXTURE}")
target_include_directories(native_article_resource_generated PRIVATE src extern/dolphin/include)
target_compile_definitions(native_article_resource_generated PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)

add_library(melee_game OBJECT ${MELEE_GAME_SOURCES})
target_compile_options(melee_game PRIVATE "$<$<COMPILE_LANGUAGE:C>:${NATIVE_MATH_COMPILE_OPTIONS}>")
set(NATIVE_NETPLAY_DIAGNOSTICS_URL "" CACHE STRING "Playtest-only HTTPS desync report endpoint")
set(NATIVE_NETPLAY_DIAGNOSTICS_BUILD "" CACHE STRING "Build label for playtest desync reports")
# The upstream repository contains both melee/ and aurora/. The retained donor
# checkout inside melee/ has its own history and is not this client's revision.
execute_process(COMMAND git -C "${CMAKE_CURRENT_SOURCE_DIR}/.." rev-parse HEAD
    OUTPUT_VARIABLE NATIVE_FEEDBACK_SOURCE_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process(COMMAND git -C "${CMAKE_CURRENT_SOURCE_DIR}/.." status --porcelain --untracked-files=no
    OUTPUT_VARIABLE NATIVE_FEEDBACK_SOURCE_CHANGES ERROR_QUIET)
if(NATIVE_FEEDBACK_SOURCE_REVISION MATCHES "^[0-9a-f]+$")
    if(NATIVE_FEEDBACK_SOURCE_CHANGES)
        string(APPEND NATIVE_FEEDBACK_SOURCE_REVISION "-modified")
    endif()
    set_property(SOURCE src/melee/gm/native_feedback_context.c APPEND PROPERTY COMPILE_DEFINITIONS
        "NATIVE_FEEDBACK_SOURCE_REVISION=\"${NATIVE_FEEDBACK_SOURCE_REVISION}\"")
endif()
if(NATIVE_NETPLAY_DIAGNOSTICS_BUILD)
    if(NOT NATIVE_NETPLAY_DIAGNOSTICS_BUILD MATCHES "^[A-Za-z0-9._-]+$")
        message(FATAL_ERROR "The client build label must contain letters, numbers, dots, underscores or dashes")
    endif()
    set_property(SOURCE src/melee/gm/native_feedback.cpp src/melee/gm/native_feedback_context.c
        APPEND PROPERTY COMPILE_DEFINITIONS
        "NATIVE_NETPLAY_DIAGNOSTICS_BUILD=\"${NATIVE_NETPLAY_DIAGNOSTICS_BUILD}\"")
endif()
if(NATIVE_NETPLAY_DIAGNOSTICS_URL)
    if(NOT NATIVE_NETPLAY_DIAGNOSTICS_URL MATCHES "^https://[A-Za-z0-9./_-]+/diagnostics$" OR
       NOT NATIVE_NETPLAY_DIAGNOSTICS_BUILD MATCHES "^[A-Za-z0-9._-]+$")
        message(FATAL_ERROR "Playtest diagnostics require an HTTPS /diagnostics URL and a build label")
    endif()
    set_property(SOURCE src/melee/gm/native_netplay_diagnostics.c APPEND PROPERTY COMPILE_DEFINITIONS
        "NATIVE_NETPLAY_DIAGNOSTICS_URL=L\"${NATIVE_NETPLAY_DIAGNOSTICS_URL}\""
        "NATIVE_NETPLAY_DIAGNOSTICS_BUILD=\"${NATIVE_NETPLAY_DIAGNOSTICS_BUILD}\"")
    set_property(SOURCE
        src/melee/gm/native_netplay_match.c
        src/melee/gm/native_replay_capsule_trace.c
        src/melee/gm/native_replay_state.c
        src/melee/lb/lbcollision.c
        APPEND PROPERTY COMPILE_DEFINITIONS NATIVE_NETPLAY_DIAGNOSTICS=1)
endif()
option(MELEE_PROFILE_MATCH_SAVE "Compile offline snapshot save-path profiler" OFF)
if(MELEE_PROFILE_MATCH_SAVE)
    target_compile_definitions(melee_game PRIVATE MELEE_PROFILE_MATCH_SAVE=1)
endif()
add_library(melee_runtime STATIC $<TARGET_OBJECTS:melee_game>)
add_dependencies(melee_game melee_embedded_assets melee_audio_coefficients)
target_include_directories(melee_game PRIVATE src extern/dolphin/include
    tools "${MELEE_GENERATED_ASSETS}/include")
target_compile_definitions(melee_game PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
set(MELEE_ROLLBACK_HOST_STATE_SOURCES
    src/melee/ft/native_script_runtime.c
    src/melee/ft/native_script_fighter_program.c
    src/melee/ft/native_script_item_program.c
    src/melee/ft/native_script_fighter_launch.c
    src/melee/gm/native_rollback_benchmark.c
    src/melee/db/dbnativecontrol.c
    src/melee/db/dbscreenshot.c
    src/melee/gm/native_asset_identity.c
    src/melee/gm/native_content_store.c
    src/melee/gm/native_fighter_package.c
    src/melee/gm/native_content_transfer.c
    src/melee/mn/native_css_layout.c
    src/melee/mn/native_css_original.c
    src/melee/ft/native_costume_import.c
    src/melee/ft/native_costume_runtime.c
    src/melee/ft/native_fighter_runtime.c
    src/melee/gm/native_match_launch.c
    src/melee/gm/native_match_fixture.c
    src/melee/gm/native_lobby_match.c
    src/melee/gm/native_netplay_exchange.c
    src/melee/gm/native_netplay_input.c
    src/melee/gm/native_netplay_match.c
    src/melee/gm/native_netplay_room.c
    src/melee/gm/native_netplay_room_content.c
    src/melee/gm/native_netplay_room_gift.c
    src/melee/gm/native_netplay_udp.c
    src/melee/gm/native_online_session.c
    src/melee/gm/native_profile.c
    src/melee/gm/native_presentation.c
    src/melee/gm/native_profile_input.c
    src/melee/gm/native_controller_calibration.c
    src/melee/gm/native_replay.c
    src/melee/gm/native_replay_state.c
    src/melee/gm/native_replay_resources.c
    src/melee/gm/native_replay_geometry.c
    src/melee/gm/native_settings.c
    src/melee/gm/native_rollback_census.c
    src/melee/gm/native_match_object_census.c
    src/melee/gm/native_snapshot_census.c
    src/melee/gm/native_match_snapshot_capture.c
    src/melee/gm/native_match_history.c
    src/melee/gm/native_match_snapshot_load.c
    src/melee/gm/native_snapshot_actor_graph.c
    src/melee/gm/native_snapshot_class_graph.c
    src/melee/gm/native_snapshot_endpoint.c
    src/melee/gm/native_versus_control_snapshot.c
    src/melee/gm/native_texture_identity_snapshot.c
    src/melee/pl/native_player_snapshot.c
    src/melee/cm/native_camera_snapshot.c
    src/melee/gr/native_stage_snapshot.c
    src/melee/gr/native_stage_start_snapshot.c
    src/melee/gr/native_stage_hazard_snapshot.c
    src/melee/gr/native_stage_generator_snapshot.c
    src/melee/gr/native_stage_spawn_snapshot.c
    src/melee/ef/native_effect_control_snapshot.c
    src/sysdolphin/baselib/native_particle_control_snapshot.c
    src/sysdolphin/baselib/native_light_control_snapshot.c
    src/sysdolphin/baselib/native_material_texture_snapshot.c
    src/melee/gr/native_green_grid_snapshot.c
    src/melee/gm/native_rollback_hash.c
    src/melee/gm/native_rollback_input.c
    src/melee/gm/native_rollback_pages.c
    src/melee/gm/native_rollback_probe.c
    src/melee/gm/native_rollback_regions.c
    src/melee/gm/native_rollback_session.c
    src/melee/gm/native_rollback_state.c
    src/melee/lb/native_dynamics_snapshot.c
    src/melee/lb/lbmthp.c
    src/melee/gm/native_home.c
    src/melee/mn/native_online_menu.c
    src/melee/mn/native_online_sss.c
    src/melee/ui/native_hub.cpp
    src/melee/ui/native_hub_news.cpp
    src/melee/ui/native_hub_announcements.cpp
    src/melee/ui/native_news_text.cpp
    src/melee/gm/native_community.cpp
    src/melee/gm/native_community_upload.cpp
    src/melee/gm/native_feedback.cpp
    src/melee/gm/native_feedback_input.c
    src/melee/ui/native_feedback_view.cpp
    src/melee/gm/native_feedback_context.c
    src/melee/ui/native_hub_startup.cpp
    src/melee/ui/native_hub_startup_view.cpp
    src/melee/ui/native_hub_startup_roster.cpp
    src/melee/ui/native_hub_portrait.cpp
    src/melee/ui/native_hub_startup_stamp.cpp
    src/melee/ui/native_hub_startup_triggers.cpp
    src/melee/lb/native_disc_files.c
    src/melee/ui/native_hub_defaults.cpp
    src/melee/mn/native_menu_tile.c
    src/melee/ui/native_menu_portrait_capture.cpp
    src/melee/ui/native_hub_pages.cpp
    src/melee/ui/native_hub_settings.cpp
    src/melee/ui/native_hub_controls.cpp
    src/melee/ui/native_css_chat.cpp
    src/melee/ui/native_hub_lobbies.cpp
    src/melee/ui/native_hub_account.cpp
    src/melee/ui/native_hub_notice.cpp
    src/melee/ui/native_hub_text.cpp
    src/melee/ui/native_hub_social.cpp
    src/melee/gm/native_account.cpp
    src/melee/gm/native_account_json.cpp
    src/melee/gm/native_account_social.cpp
    src/melee/gm/native_account_login.cpp
    src/melee/gm/native_account_key.cpp
    src/melee/gm/native_account_key_file.cpp
    src/melee/gm/native_account_transport.cpp
    src/melee/gm/native_account_store.cpp
    src/melee/ui/native_hub_catalog.cpp
    src/melee/ui/native_hub_delete.cpp
    src/melee/mn/native_menu_paint.c
    src/melee/mn/native_menu_font.c
    src/melee/mn/native_menu_skin.c
    src/melee/mn/native_menu_scene.c
    src/melee/mn/native_menu_scenery.c
    src/melee/mn/native_menu_prize.c
    src/melee/mn/native_menu_fighter.c
    src/melee/ui/native_hub_hand.c
    src/melee/ui/native_play_menu.cpp
    src/melee/mn/native_css_interaction.c
    src/melee/mn/native_css_cards.c
    src/melee/ft/native_costume_catalog.c
    src/melee/mn/native_css_participants.c
    src/melee/mn/native_menu_portrait.c
    src/melee/ui/native_shell.cpp
    src/melee/ui/native_shell_collection.cpp
    src/melee/ui/native_shell_css_original.cpp
    src/melee/ui/native_shell_costumes.cpp
    src/melee/ui/native_chat.c
    src/melee/ui/native_social_presence.c
    src/melee/gm/native_room_chat.c
    src/melee/gm/native_netplay_room_cache.c
    src/melee/ui/native_shell_profiles.cpp
    src/melee/ui/native_shell_rules.cpp
    src/sysdolphin/baselib/audio_rollback.c
    src/sysdolphin/baselib/audio_output.c
    src/sysdolphin/baselib/audio_capture.c
    src/sysdolphin/baselib/debug.c
    src/sysdolphin/baselib/devcom.c
    src/sysdolphin/baselib/perf.c
    src/sysdolphin/baselib/report.c
    src/sysdolphin/baselib/native_controller_snapshot.c
    src/sysdolphin/baselib/rumble_output.c)
set_property(SOURCE ${MELEE_ROLLBACK_HOST_STATE_SOURCES}
    APPEND PROPERTY COMPILE_DEFINITIONS MELEE_ROLLBACK_HOST_STATE=1)
set(MELEE_ROLLBACK_AUDIO_STATE_SOURCES
    src/melee/lb/lbaudio_ax.c
    src/sysdolphin/baselib/audio_aux.c
    src/sysdolphin/baselib/audio_frame.c
    src/sysdolphin/baselib/audio_voice.c
    src/sysdolphin/baselib/axdriver.c
    src/sysdolphin/baselib/synth.c)
set_source_files_properties(${MELEE_ROLLBACK_AUDIO_STATE_SOURCES}
    PROPERTIES COMPILE_DEFINITIONS MELEE_ROLLBACK_AUDIO_STATE=1)
if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(melee_game PRIVATE /clang:-ffp-contract=off
        /clang:-fno-strict-float-cast-overflow /clang:-ffunction-sections
        /clang:-fdata-sections /clang:-fno-zero-initialized-in-bss
        /clang:-include
        /clang:${CMAKE_CURRENT_SOURCE_DIR}/src/melee/gm/native_rollback_state_section.h)
else()
    target_compile_options(melee_game PRIVATE -ffp-contract=off
        -fno-strict-float-cast-overflow -ffunction-sections -fdata-sections
        -fno-zero-initialized-in-bss -include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/melee/gm/native_rollback_state_section.h)
endif()

# Keep a reviewable coverage manifest alongside the compilation database.
string(REPLACE ";" "\n" MELEE_GAME_SOURCE_MANIFEST "${MELEE_GAME_SOURCES}")
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/game-sources.txt"
    CONTENT "${MELEE_GAME_SOURCE_MANIFEST}\n")

option(MELEE_BUILD_TESTS "Build native client tests" ON)
enable_testing()
if(MELEE_BUILD_TESTS)
    add_executable(native_numeric_conversion_test
        tests/native/numeric_conversion_test.c)
    if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        target_compile_options(native_numeric_conversion_test PRIVATE
            /clang:-fno-strict-float-cast-overflow)
    else()
        target_compile_options(native_numeric_conversion_test PRIVATE
            -fno-strict-float-cast-overflow)
    endif()
    add_test(NAME native_numeric_conversion
        COMMAND native_numeric_conversion_test)
    add_executable(native_sis_state_test tests/native/sis_state_test.c
        src/sysdolphin/baselib/sislib_state.c
        src/sysdolphin/baselib/sislib_measure.c
        src/sysdolphin/baselib/sislib_data.c
        src/sysdolphin/baselib/dat.c src/sysdolphin/baselib/dat_sis.c)
    target_include_directories(native_sis_state_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_sis_state_test PRIVATE MELEE_NATIVE=1)
    add_test(NAME native_sis_state COMMAND native_sis_state_test)
    add_executable(native_sfxcommands_test tests/native/sfxcommands_test.c
        src/sysdolphin/baselib/sfxcommands.c)
    target_include_directories(native_sfxcommands_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_sfxcommands_test PRIVATE MELEE_NATIVE=1)
    add_test(NAME native_sfxcommands COMMAND native_sfxcommands_test)
    add_executable(native_audio_frame_test tests/native/audio_frame_test.c
        src/sysdolphin/baselib/audio_frame.c src/sysdolphin/baselib/audio_aux.c
        src/sysdolphin/baselib/audio_reverb.c src/sysdolphin/baselib/audio_delay.c)
    target_include_directories(native_audio_frame_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_audio_frame_test PRIVATE MELEE_NATIVE=1)
    add_test(NAME native_audio_frame COMMAND native_audio_frame_test)
    add_executable(native_audio_aux_test tests/native/audio_aux_test.c
        src/sysdolphin/baselib/audio_aux.c src/sysdolphin/baselib/audio_reverb.c
        src/sysdolphin/baselib/audio_delay.c)
    target_include_directories(native_audio_aux_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_audio_aux_test PRIVATE MELEE_NATIVE=1)
    if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        target_compile_options(native_audio_aux_test PRIVATE /clang:-ffp-contract=off)
    else()
        target_compile_options(native_audio_aux_test PRIVATE -ffp-contract=off)
    endif()
    add_test(NAME native_audio_aux COMMAND native_audio_aux_test)
    add_executable(native_audio_reverb_test tests/native/audio_reverb_test.c
        src/sysdolphin/baselib/audio_reverb.c)
    target_include_directories(native_audio_reverb_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_audio_reverb_test PRIVATE MELEE_NATIVE=1)
    if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        target_compile_options(native_audio_reverb_test PRIVATE /clang:-ffp-contract=off)
    else()
        target_compile_options(native_audio_reverb_test PRIVATE -ffp-contract=off)
    endif()
    add_test(NAME native_audio_reverb COMMAND native_audio_reverb_test)
    add_executable(native_audio_delay_test tests/native/audio_delay_test.c
        src/sysdolphin/baselib/audio_delay.c)
    target_include_directories(native_audio_delay_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_audio_delay_test PRIVATE MELEE_NATIVE=1)
    add_test(NAME native_audio_delay COMMAND native_audio_delay_test)
    add_executable(native_audio_mix_test tests/native/audio_mix_test.c
        src/sysdolphin/baselib/audio_mix.c src/sysdolphin/baselib/audio_resample.c
        src/sysdolphin/baselib/audio_voice.c src/sysdolphin/baselib/audio_sample.c)
    add_dependencies(native_audio_mix_test melee_audio_coefficients)
    target_include_directories(native_audio_mix_test PRIVATE src extern/dolphin/include
        "${MELEE_GENERATED_ASSETS}/include")
    target_compile_definitions(native_audio_mix_test PRIVATE MELEE_NATIVE=1)
    add_test(NAME native_audio_mix COMMAND native_audio_mix_test)
    add_executable(native_audio_resample_test tests/native/audio_resample_test.c
        src/sysdolphin/baselib/audio_resample.c src/sysdolphin/baselib/audio_voice.c
        src/sysdolphin/baselib/audio_sample.c)
    add_dependencies(native_audio_resample_test melee_audio_coefficients)
    target_include_directories(native_audio_resample_test PRIVATE src extern/dolphin/include
        "${MELEE_GENERATED_ASSETS}/include")
    target_compile_definitions(native_audio_resample_test PRIVATE MELEE_NATIVE=1)
    add_test(NAME native_audio_resample COMMAND native_audio_resample_test)
    add_executable(native_audio_voice_test tests/native/audio_voice_test.c
        src/sysdolphin/baselib/audio_voice.c src/sysdolphin/baselib/audio_sample.c)
    target_include_directories(native_audio_voice_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_audio_voice_test PRIVATE MELEE_NATIVE=1)
    add_test(NAME native_audio_voice COMMAND native_audio_voice_test)
    add_executable(native_audio_sample_test tests/native/audio_sample_test.c
        src/sysdolphin/baselib/audio_sample.c)
    target_include_directories(native_audio_sample_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_audio_sample_test PRIVATE MELEE_NATIVE=1)
    add_test(NAME native_audio_sample COMMAND native_audio_sample_test)
    add_executable(native_audio_rollback_test
        tests/native/audio_rollback_test.c
        src/sysdolphin/baselib/audio_rollback.c)
    target_include_directories(native_audio_rollback_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_audio_rollback_test PRIVATE MELEE_NATIVE=1)
    add_test(NAME native_audio_rollback COMMAND native_audio_rollback_test)
    set(AUDIO_DRIVER_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/tests/generated/audio_rollback_driver.inc")
    add_custom_command(OUTPUT "${AUDIO_DRIVER_FIXTURE}"
        COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/prepare_audio_driver_fixture.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/sysdolphin/baselib/axdriver.c" "${AUDIO_DRIVER_FIXTURE}"
        DEPENDS tests/native/prepare_audio_driver_fixture.py src/sysdolphin/baselib/axdriver.c
        VERBATIM)
    add_executable(native_audio_rollback_driver_test "${AUDIO_DRIVER_FIXTURE}"
        tests/native/audio_rollback_driver_test.c
        src/sysdolphin/baselib/audio_rollback.c)
    target_include_directories(native_audio_rollback_driver_test PRIVATE src extern/dolphin/include
        src/sysdolphin/baselib "${CMAKE_CURRENT_BINARY_DIR}/tests/generated")
    target_compile_definitions(native_audio_rollback_driver_test PRIVATE MELEE_NATIVE=1)
    add_test(NAME native_audio_rollback_driver COMMAND native_audio_rollback_driver_test)
    add_executable(native_stream_test tests/native/stream_test.c
        src/sysdolphin/baselib/sfxstream.c)
    target_include_directories(native_stream_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_stream_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    add_test(NAME native_stream COMMAND Python3::Interpreter
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/check_streams.py"
        "${MELEE_ISO}" "$<TARGET_FILE:native_stream_test>")
    add_executable(native_sfxload_test tests/native/sfxload_test.c
        src/sysdolphin/baselib/sfxbank.c src/sysdolphin/baselib/sfxstream.c
        src/sysdolphin/baselib/audio_voice.c src/sysdolphin/baselib/audio_sample.c
        src/sysdolphin/baselib/audio_mix.c src/sysdolphin/baselib/audio_resample.c)
    add_dependencies(native_sfxload_test melee_audio_coefficients)
    target_include_directories(native_sfxload_test PRIVATE src extern/dolphin/include
        "${MELEE_GENERATED_ASSETS}/include")
    target_compile_definitions(native_sfxload_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    target_compile_options(native_sfxload_test PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(native_sfxload_test PRIVATE "LINKER:/opt:ref")
    add_test(NAME native_sfxload COMMAND native_sfxload_test)
    add_executable(native_sfxbank_test tests/native/sfxbank_test.c
        src/sysdolphin/baselib/sfxbank.c)
    target_include_directories(native_sfxbank_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_sfxbank_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    add_test(NAME native_sfxbank COMMAND Python3::Interpreter
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/check_sfxbanks.py"
        "${MELEE_ISO}" "$<TARGET_FILE:native_sfxbank_test>")
    add_executable(native_devcom_test tests/native/devcom_test.c)
    target_include_directories(native_devcom_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_devcom_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    target_link_libraries(native_devcom_test PRIVATE melee_runtime)
    add_test(NAME native_devcom COMMAND native_devcom_test)
    add_executable(native_lbmemory_test tests/native/lbmemory_test.c)
    target_include_directories(native_lbmemory_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_lbmemory_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    target_link_libraries(native_lbmemory_test PRIVATE melee_runtime)
    target_link_options(native_lbmemory_test PRIVATE "LINKER:/opt:ref")
    add_test(NAME native_lbmemory COMMAND native_lbmemory_test)
    add_executable(native_input_timing_test tests/native/input_timing_test.c
        src/melee/lb/lb_0195.c src/melee/gm/native_match_arena.c)
    target_include_directories(native_input_timing_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_input_timing_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    add_test(NAME native_input_timing COMMAND native_input_timing_test)
    add_executable(native_home_input_test tests/native/home_input_test.c
        src/melee/gm/native_home_input.c)
    target_include_directories(native_home_input_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_home_input_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    add_test(NAME native_home_input COMMAND native_home_input_test)
    add_executable(native_replay_test tests/native/replay_test.c
        src/melee/gm/native_replay.c)
    target_include_directories(native_replay_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_replay_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    add_test(NAME native_replay COMMAND native_replay_test)
    add_executable(native_match_history_test tests/native/match_history_test.c
        src/melee/gm/native_match_history.c src/melee/gm/native_match_arena.c)
    target_include_directories(native_match_history_test PRIVATE src)
    add_test(NAME native_match_history COMMAND native_match_history_test)
    add_executable(native_stage_start_snapshot_test tests/native/stage_start_snapshot_test.c
        src/melee/gr/stage_start_state.c src/melee/gr/native_stage_start_snapshot.c
        src/melee/gm/native_match_pool.c src/melee/gm/native_match_snapshot_pool.c
        src/melee/gm/native_match_arena.c)
    target_include_directories(native_stage_start_snapshot_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_stage_start_snapshot_test PRIVATE MELEE_NATIVE=1)
    add_test(NAME native_stage_start_snapshot COMMAND native_stage_start_snapshot_test)
    add_executable(native_match_fixture_test tests/native/match_fixture_test.c
        src/melee/gm/native_match_fixture.c)
    target_include_directories(native_match_fixture_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_match_fixture_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    add_test(NAME native_match_fixture COMMAND native_match_fixture_test)
    set(NATIVE_HOOKSHOT_SCALE_HEADER
        "${CMAKE_CURRENT_BINARY_DIR}/hookshot_scale.generated.h")
    add_custom_command(OUTPUT "${NATIVE_HOOKSHOT_SCALE_HEADER}"
        COMMAND Python3::Interpreter
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/hookshot_scale_fixture.py"
            "${CMAKE_CURRENT_SOURCE_DIR}" "${NATIVE_HOOKSHOT_SCALE_HEADER}"
        DEPENDS tests/native/hookshot_scale_fixture.py
            src/melee/it/kinds/itlinkhookshot.c src/melee/it/it_2725.c
            src/sysdolphin/baselib/jobj.c
        VERBATIM)
    add_executable(native_hookshot_owner_scale_test
        tests/native/hookshot_scale_test.c "${NATIVE_HOOKSHOT_SCALE_HEADER}")
    target_include_directories(native_hookshot_owner_scale_test PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}" src extern/dolphin/include)
    target_compile_definitions(native_hookshot_owner_scale_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    add_test(NAME native_hookshot_owner_scale
        COMMAND native_hookshot_owner_scale_test)
    add_executable(native_semantic_state_test
        tests/native/semantic_state_test.c
        src/melee/gm/native_semantic_state.c
        src/melee/ft/native_script_source.c
        src/melee/ft/native_script_storage.c
        src/melee/ft/native_script_expression.c
        src/melee/ft/native_script_value.c)
    target_include_directories(native_semantic_state_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_semantic_state_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(native_semantic_state_test PRIVATE aurora_gx aurora_vi)
    add_test(NAME native_semantic_state COMMAND native_semantic_state_test)
    add_executable(native_rollback_pages_test tests/native/rollback_pages_test.c
        src/melee/gm/native_rollback_pages.c)
    target_include_directories(native_rollback_pages_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_rollback_pages_test PRIVATE MELEE_NATIVE=1)
    add_test(NAME native_rollback_pages COMMAND native_rollback_pages_test)
    add_executable(native_rollback_input_test
        tests/native/rollback_input_test.c
        src/melee/gm/native_rollback_input.c)
    target_include_directories(native_rollback_input_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_rollback_input_test PRIVATE MELEE_NATIVE=1)
    add_test(NAME native_rollback_input COMMAND native_rollback_input_test)
    add_executable(native_confirmed_checkpoint_test
        tests/native/confirmed_checkpoint_test.c
        src/melee/gm/native_confirmed_checkpoint.c
        src/melee/gm/native_match_arena.c
        src/melee/gm/native_rollback_input.c)
    target_include_directories(native_confirmed_checkpoint_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_confirmed_checkpoint_test PRIVATE MELEE_NATIVE=1)
    add_test(NAME native_confirmed_checkpoint COMMAND native_confirmed_checkpoint_test)
    add_executable(native_rollback_session_test
        tests/native/rollback_session_test.c
        src/melee/gm/native_rollback_input.c
        src/melee/gm/native_rollback_session.c)
    target_include_directories(native_rollback_session_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_rollback_session_test PRIVATE
        MELEE_NATIVE=1)
    add_test(NAME native_rollback_session COMMAND native_rollback_session_test)
    add_executable(native_netplay_input_test
        tests/native/netplay_input_test.c
        src/melee/gm/native_netplay_input.c
        src/melee/gm/native_rollback_input.c)
    target_include_directories(native_netplay_input_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_netplay_input_test PRIVATE
        MELEE_NATIVE=1)
    add_test(NAME native_netplay_input COMMAND native_netplay_input_test)
    add_executable(native_netplay_udp_test
        tests/native/netplay_udp_test.c
        src/melee/gm/native_netplay_exchange.c
        src/melee/gm/native_netplay_input.c
        src/melee/gm/native_netplay_udp.c
        src/melee/gm/native_rollback_input.c)
    target_include_directories(native_netplay_udp_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_netplay_udp_test PRIVATE
        MELEE_NATIVE=1)
    target_link_libraries(native_netplay_udp_test PRIVATE ws2_32)
    add_test(NAME native_netplay_udp COMMAND native_netplay_udp_test)
    add_executable(native_signaling_wire_test
        tests/native/signaling_wire_test.c
        src/melee/gm/native_signaling_wire.c)
    target_include_directories(native_signaling_wire_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_signaling_wire_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    add_test(NAME native_signaling_wire COMMAND native_signaling_wire_test)
    add_executable(native_room_chat_test
        tests/native/room_chat_test.c
        src/melee/gm/native_room_chat.c
        src/melee/gm/native_signaling_client.c
        src/melee/gm/native_signaling_wire.c)
    target_include_directories(native_room_chat_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_room_chat_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(native_room_chat_test PRIVATE winhttp)
    target_sources(native_room_chat_test PRIVATE src/melee/gm/native_account_transport.cpp)
    add_test(NAME native_room_chat COMMAND native_room_chat_test)
    add_executable(native_utf8_test tests/native/utf8_test.c)
    target_include_directories(native_utf8_test PRIVATE src)
    add_test(NAME native_utf8 COMMAND native_utf8_test)
    add_test(NAME native_menu_fonts COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/menu_font_test.py"
        --fonts "$<TARGET_FILE_DIR:melee_native>/ui/fonts")
    add_executable(native_chat_wire_test
        tests/native/chat_wire_test.c src/melee/ui/native_chat_wire.c)
    target_include_directories(native_chat_wire_test PRIVATE src)
    target_compile_definitions(native_chat_wire_test PRIVATE _CRT_SECURE_NO_WARNINGS)
    add_test(NAME native_chat_wire COMMAND native_chat_wire_test)
    add_executable(native_social_presence_test tests/native/social_presence_test.c
        src/melee/ui/native_social_presence.c)
    target_include_directories(native_social_presence_test PRIVATE src)
    target_compile_definitions(native_social_presence_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    target_compile_options(native_social_presence_test PRIVATE -UNDEBUG)
    add_test(NAME native_social_presence COMMAND native_social_presence_test)
    add_executable(native_turn_credentials_test
        tests/native/turn_credentials_test.c
        src/melee/gm/native_turn_credentials.c)
    target_include_directories(native_turn_credentials_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_turn_credentials_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(native_turn_credentials_test PRIVATE winhttp)
    add_test(NAME native_turn_credentials COMMAND native_turn_credentials_test)
    add_executable(native_lobby_match_test
        tests/native/lobby_match_test.c
        src/melee/gm/native_lobby_match.c)
    target_include_directories(native_lobby_match_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_lobby_match_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    add_test(NAME native_lobby_match COMMAND native_lobby_match_test)
    add_executable(native_match_stream_test tests/native/match_stream_test.c
        src/melee/gm/native_match_stream.c src/melee/gm/native_lobby_match.c)
    target_include_directories(native_match_stream_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_match_stream_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    add_test(NAME native_match_stream COMMAND native_match_stream_test)
    add_executable(native_match_playback_test tests/native/match_playback_test.c
        src/melee/gm/native_match_playback.c src/melee/gm/native_match_stream.c
        src/melee/gm/native_lobby_match.c)
    target_include_directories(native_match_playback_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_match_playback_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    add_test(NAME native_match_playback COMMAND native_match_playback_test)
    add_executable(native_room_watch_test tests/native/room_watch_test.c
        src/melee/gm/native_netplay_room_watch.c src/melee/gm/native_match_stream.c
        src/melee/gm/native_lobby_match.c)
    target_include_directories(native_room_watch_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_room_watch_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    add_test(NAME native_room_watch COMMAND native_room_watch_test)
    add_executable(native_room_cache_test tests/native/room_cache_test.c
        src/melee/gm/native_netplay_room_cache.c src/melee/gm/native_content_transfer.c
        src/melee/gm/native_lobby_selection.c)
    target_include_directories(native_room_cache_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_room_cache_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    add_test(NAME native_room_cache COMMAND native_room_cache_test)
    add_executable(native_lobby_selection_test
        tests/native/lobby_selection_test.c
        src/melee/gm/native_lobby_selection.c)
    target_include_directories(native_lobby_selection_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_lobby_selection_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    add_test(NAME native_lobby_selection COMMAND native_lobby_selection_test)
    add_executable(native_match_selection_test tests/native/match_selection_test.c
        src/melee/gm/native_match_selection.c src/melee/gm/native_stock_content.c)
    target_include_directories(native_match_selection_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_match_selection_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    add_test(NAME native_match_selection COMMAND native_match_selection_test)
    add_executable(native_lobby_topology_test
        tests/native/lobby_topology_test.c
        src/melee/gm/native_lobby_match.c
        src/melee/gm/native_lobby_topology.c)
    target_include_directories(native_lobby_topology_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_lobby_topology_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    add_test(NAME native_lobby_topology COMMAND native_lobby_topology_test)
    add_executable(native_profile_test
        tests/native/profile_test.c
        src/melee/gm/native_profile.c)
    target_include_directories(native_profile_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_profile_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    add_test(NAME native_profile COMMAND native_profile_test)
    add_executable(native_settings_test
        tests/native/settings_test.c
        src/melee/gm/native_settings.c)
    target_include_directories(native_settings_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_settings_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    add_test(NAME native_settings COMMAND native_settings_test)
    add_executable(native_content_store_test
        tests/native/content_store_test.c
        src/melee/gm/native_content_store.c
    src/melee/gm/native_fighter_package.c)
    target_include_directories(native_content_store_test PRIVATE src)
    target_compile_definitions(native_content_store_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(native_content_store_test PRIVATE bcrypt)
    add_test(NAME native_content_store COMMAND native_content_store_test)
    add_executable(native_room_gift_test tests/native/room_gift_test.c
        src/melee/gm/native_netplay_room_gift.c
        src/melee/gm/native_content_transfer.c
        src/melee/gm/native_content_store.c src/melee/gm/native_fighter_package.c)
    target_include_directories(native_room_gift_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_room_gift_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(native_room_gift_test PRIVATE bcrypt)
    add_test(NAME native_room_gift COMMAND native_room_gift_test)
    add_executable(native_fighter_family_package_test
        tests/native/fighter_family_package/test.c
        src/melee/gm/native_content_store.c src/melee/gm/native_fighter_package.c)
    target_include_directories(native_fighter_family_package_test PRIVATE src)
    target_compile_definitions(native_fighter_family_package_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(native_fighter_family_package_test PRIVATE bcrypt)
    add_test(NAME native_fighter_family_package COMMAND native_fighter_family_package_test)
    add_executable(native_content_transfer_test
        tests/native/content_transfer_test.c
        src/melee/gm/native_content_transfer.c)
    target_include_directories(native_content_transfer_test PRIVATE src)
    target_compile_definitions(native_content_transfer_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    add_test(NAME native_content_transfer COMMAND native_content_transfer_test)
    add_executable(native_profile_input_test
        tests/native/profile_input_test.c
        src/melee/gm/native_profile.c
        src/melee/gm/native_profile_input.c src/melee/gm/native_controller_calibration.c)
    target_include_directories(native_profile_input_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_profile_input_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    add_test(NAME native_profile_input COMMAND native_profile_input_test)
    add_executable(native_rollback_hash_test
        tests/native/rollback_hash_test.c
        src/melee/gm/native_asset_identity.c
        src/melee/gm/native_rollback_hash.c)
    target_include_directories(native_rollback_hash_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_rollback_hash_test PRIVATE MELEE_NATIVE=1)
    add_test(NAME native_rollback_hash COMMAND native_rollback_hash_test)
    add_executable(native_rumble_output_test
        tests/native/rumble_output_test.c
        src/sysdolphin/baselib/rumble_output.c)
    target_include_directories(native_rumble_output_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_rumble_output_test PRIVATE MELEE_NATIVE=1)
    add_test(NAME native_rumble_output COMMAND native_rumble_output_test)
    add_executable(native_report_test tests/native/report_test.c
        src/sysdolphin/baselib/report.c)
    target_include_directories(native_report_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_report_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    add_test(NAME native_report COMMAND native_report_test)
    add_executable(native_objalloc_test tests/native/objalloc_test.c
        src/sysdolphin/baselib/objalloc.c src/sysdolphin/baselib/id.c
        src/sysdolphin/baselib/native_hsd_reservation.c
        src/melee/gm/native_match_pool.c src/melee/gm/native_match_snapshot_pool.c)
    target_include_directories(native_objalloc_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_objalloc_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    add_test(NAME native_objalloc COMMAND native_objalloc_test)
    add_executable(native_dat_test tests/native/dat_test.c
        tests/native/dat_texture_test.c src/sysdolphin/baselib/dat.c
        tests/native/dat_import_test.c src/sysdolphin/baselib/dat_import.c
        tests/native/dat_animation_test.c src/sysdolphin/baselib/dat_animation.c
        src/sysdolphin/baselib/dat_model.c src/sysdolphin/baselib/dat_geometry.c
        src/sysdolphin/baselib/dat_shape_skin.c
        src/sysdolphin/baselib/dat_scene.c
        tests/native/dat_model_test.c
        src/sysdolphin/baselib/dat_material.c src/sysdolphin/baselib/dat_texture.c)
    target_include_directories(native_dat_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_dat_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    add_test(NAME native_dat COMMAND native_dat_test)
    add_executable(native_collision_test tests/native/collision_test.c
        src/melee/mp/mpdat.c src/sysdolphin/baselib/dat.c)
    target_include_directories(native_collision_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_collision_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    add_test(NAME native_collision COMMAND native_collision_test)
endif()

# Integrate the supplied Aurora source into this same build. The platform target
# can be validated before the game's remaining SDK calls are ready to link.
set(MELEE_AURORA_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../aurora"
    CACHE PATH "Aurora source directory")
if(NOT EXISTS "${MELEE_AURORA_SOURCE}/CMakeLists.txt")
    message(FATAL_ERROR "Set MELEE_AURORA_SOURCE to the Aurora source checkout.")
endif()
set(AURORA_ENABLE_RMLUI ON CACHE BOOL
    "Enable Aurora's native desktop UI for the Freak Melee shell" FORCE)
include(cmake/NativeSDL.cmake)
add_subdirectory("${MELEE_AURORA_SOURCE}" "${CMAKE_BINARY_DIR}/aurora" EXCLUDE_FROM_ALL)
target_compile_options(aurora_mtx PRIVATE ${NATIVE_MATH_COMPILE_OPTIONS})
target_link_libraries(aurora_mtx PUBLIC melee_math)
include("${MELEE_AURORA_SOURCE}/cmake/AuroraCopyRuntimeDLLs.cmake")
add_custom_target(melee_platform DEPENDS aurora_core aurora_os aurora_gx
    aurora_mtx aurora_vi aurora_pad aurora_dvd aurora_card aurora_thp)

# ICE owns the live UDP socket through candidate selection, TURN permissions,
# and consent checks. Pin the implementation exactly so packet timing and
# callback behavior cannot drift between client builds.
include(FetchContent)
FetchContent_Declare(yyjson
    GIT_REPOSITORY https://github.com/ibireme/yyjson.git
    GIT_TAG 7871d321ff4cd8068c1f777c97975dc2fb640ab3)
FetchContent_MakeAvailable(yyjson)
FetchContent_Declare(monocypher
    GIT_REPOSITORY https://github.com/LoupVaillant/Monocypher.git
    GIT_TAG 0d85f98c9d9b0227e42cf795cb527dff372b40a4)
FetchContent_MakeAvailable(monocypher)
add_library(melee_account_crypto STATIC
    ${monocypher_SOURCE_DIR}/src/monocypher.c
    ${monocypher_SOURCE_DIR}/src/optional/monocypher-ed25519.c)
target_include_directories(melee_account_crypto PUBLIC
    ${monocypher_SOURCE_DIR}/src ${monocypher_SOURCE_DIR}/src/optional)
if(MELEE_BUILD_TESTS)
    add_executable(native_account_test tests/native/account_test.cpp
        src/melee/gm/native_account.cpp src/melee/gm/native_account_json.cpp
        src/melee/gm/native_account_store.cpp)
    target_sources(native_account_test PRIVATE src/melee/gm/native_account_social.cpp)
    target_sources(native_account_test PRIVATE src/melee/gm/native_account_transport.cpp)
    target_include_directories(native_account_test PRIVATE src)
    target_compile_definitions(native_account_test PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(native_account_test PRIVATE -UNDEBUG)
    target_sources(native_account_test PRIVATE src/melee/gm/native_account_login.cpp)
    target_sources(native_account_test PRIVATE src/melee/gm/native_account_key.cpp)
    target_sources(native_account_test PRIVATE src/melee/gm/native_account_key_file.cpp)
    target_link_libraries(native_account_test PRIVATE yyjson melee_account_crypto winhttp crypt32 comdlg32 bcrypt)
    add_test(NAME native_account COMMAND native_account_test)
endif()
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build dependencies as static libraries" FORCE)
set(NO_SERVER ON CACHE BOOL "Disable libjuice's standalone TURN server" FORCE)
set(NO_TESTS ON CACHE BOOL "Disable libjuice's upstream tests" FORCE)
FetchContent_Declare(libjuice
    GIT_REPOSITORY https://github.com/paullouisageneau/libjuice.git
    GIT_TAG 3c40a3545b6b1b62c7adee7f8f2bd58aa290afd6)
FetchContent_MakeAvailable(libjuice)
unset(NO_TESTS CACHE)
unset(NO_SERVER CACHE)
unset(BUILD_SHARED_LIBS CACHE)

if(MELEE_BUILD_TESTS)
    add_executable(native_netplay_ice_test
        tests/native/netplay_ice_test.c
        src/melee/gm/native_netplay_ice.c)
    target_include_directories(native_netplay_ice_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_netplay_ice_test PRIVATE MELEE_NATIVE=1)
    target_link_libraries(native_netplay_ice_test PRIVATE LibJuice::LibJuice)
    add_test(NAME native_netplay_ice COMMAND native_netplay_ice_test)
    add_executable(native_netplay_connection_wire_test
        tests/native/netplay_connection_wire_test.c
        src/melee/gm/native_netplay_connection_wire.c)
    target_include_directories(native_netplay_connection_wire_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_netplay_connection_wire_test PRIVATE
        MELEE_NATIVE=1)
    add_test(NAME native_netplay_connection_wire
        COMMAND native_netplay_connection_wire_test)
endif()

if(MELEE_BUILD_TESTS)
    add_executable(native_hitbox_lifetime_test
        tests/native/hitbox_lifetime_test.c src/melee/ft/native_hitbox_lifetime.c)
    target_include_directories(native_hitbox_lifetime_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_hitbox_lifetime_test PRIVATE MELEE_NATIVE=1)
    add_test(NAME native_hitbox_lifetime COMMAND native_hitbox_lifetime_test)
    add_executable(native_heap_test tests/native/heap_test.c
        tests/native/animation_test.c src/sysdolphin/baselib/fobj.c
        tests/native/animation_scheduler_boundary.c src/sysdolphin/baselib/native_animation_pool.c
        src/melee/gm/native_match_pool.c src/melee/gm/native_match_snapshot_pool.c
        src/sysdolphin/baselib/spline.c src/sysdolphin/baselib/dat.c
        src/sysdolphin/baselib/dat_import.c src/sysdolphin/baselib/dat_material.c
        src/sysdolphin/baselib/dat_texture.c src/sysdolphin/baselib/dat_animation.c
        src/sysdolphin/baselib/dat_model.c src/sysdolphin/baselib/dat_geometry.c
        src/sysdolphin/baselib/dat_shape_skin.c
        src/sysdolphin/baselib/dat_scene.c
        tests/native/class_test.c src/sysdolphin/baselib/class.c src/sysdolphin/baselib/native_hsd_arena.c src/melee/gm/native_match_arena.c
        src/sysdolphin/baselib/hash.c
        src/sysdolphin/baselib/native_hsd_reservation.c
        src/sysdolphin/baselib/memory.c src/sysdolphin/baselib/objalloc.c
        src/sysdolphin/baselib/debug.c src/sysdolphin/baselib/report.c)
    target_include_directories(native_heap_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_heap_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        target_compile_options(native_heap_test PRIVATE /clang:-ffp-contract=off)
    else()
        target_compile_options(native_heap_test PRIVATE -ffp-contract=off)
    endif()
    target_link_libraries(native_heap_test PRIVATE aurora_os aurora_gx aurora_vi
        aurora_pad aurora_mtx aurora_dvd aurora_card aurora_thp)
    aurora_copy_runtime_dlls(native_heap_test)
    add_test(NAME native_heap COMMAND native_heap_test)
    add_executable(native_aram_transfer_test tests/native/aram_transfer_test.cpp)
    target_include_directories(native_aram_transfer_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_aram_transfer_test PRIVATE MELEE_NATIVE=1)
    target_link_libraries(native_aram_transfer_test PRIVATE aurora_os aurora_gx
        aurora_vi aurora_pad aurora_mtx aurora_dvd aurora_card aurora_thp)
    aurora_copy_runtime_dlls(native_aram_transfer_test)
    add_test(NAME native_aram_transfer COMMAND native_aram_transfer_test)
    add_executable(native_profile_pad_test tests/native/profile_pad_test.cpp
        src/melee/gm/native_profile.c src/melee/gm/native_profile_input.c
        src/melee/gm/native_controller_calibration.c)
    target_include_directories(native_profile_pad_test PRIVATE src
        "${MELEE_AURORA_SOURCE}/lib")
    target_compile_definitions(native_profile_pad_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(native_profile_pad_test PRIVATE aurora_pad aurora_os
        aurora_gx aurora_vi aurora_mtx aurora_dvd aurora_card aurora_thp absl::flat_hash_map)
    aurora_copy_runtime_dlls(native_profile_pad_test)
    add_test(NAME native_profile_pad COMMAND native_profile_pad_test
        "${CMAKE_BINARY_DIR}/profile-pad-fixture")
    FetchContent_GetProperties(SDL SOURCE_DIR _native_sdl_source)
    add_test(NAME native_gamecube_reports COMMAND Python3::Interpreter
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/gamecube_report_test.py"
        --source "${_native_sdl_source}/src/joystick/hidapi/SDL_hidapi_gamecube.c"
        --output "${CMAKE_BINARY_DIR}/gamecube-report-fixture"
        --compiler "${CMAKE_C_COMPILER}")
    add_executable(native_dvd_transfer_test tests/native/dvd_transfer_test.cpp)
    target_include_directories(native_dvd_transfer_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_dvd_transfer_test PRIVATE MELEE_NATIVE=1)
    target_link_libraries(native_dvd_transfer_test PRIVATE melee_runtime
        aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx aurora_dvd aurora_card aurora_thp)
    aurora_copy_runtime_dlls(native_dvd_transfer_test)
    add_test(NAME native_dvd_transfer COMMAND native_dvd_transfer_test "${MELEE_ISO}")
    add_executable(native_stage_data_test tests/native/stage_data_test.c)
    target_include_directories(native_stage_data_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_stage_data_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    target_link_libraries(native_stage_data_test PRIVATE melee_runtime
        aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx aurora_dvd aurora_card aurora_thp)
    target_link_options(native_stage_data_test PRIVATE "LINKER:/opt:ref")
    aurora_copy_runtime_dlls(native_stage_data_test)
    add_test(NAME native_stage_data COMMAND native_stage_data_test "${MELEE_ISO}")
    add_executable(native_item_common_data_test
        tests/native/item_common_data_test.c)
    target_include_directories(native_item_common_data_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_item_common_data_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    target_link_libraries(native_item_common_data_test PRIVATE melee_runtime
        aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx aurora_dvd
        aurora_card aurora_thp)
    target_link_options(native_item_common_data_test PRIVATE "LINKER:/opt:ref")
    aurora_copy_runtime_dlls(native_item_common_data_test)
    add_test(NAME native_item_common_data COMMAND
        native_item_common_data_test "${MELEE_ISO}")
    add_executable(native_fighter_common_assets_test
        tests/native/fighter_common_assets_test.c)
    target_include_directories(native_fighter_common_assets_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_fighter_common_assets_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    target_link_libraries(native_fighter_common_assets_test PRIVATE
        melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx
        aurora_dvd aurora_card aurora_thp)
    target_link_options(native_fighter_common_assets_test PRIVATE
        "LINKER:/opt:ref")
    aurora_copy_runtime_dlls(native_fighter_common_assets_test)
    add_test(NAME native_fighter_common_assets COMMAND
        native_fighter_common_assets_test "${MELEE_ISO}")
    add_executable(native_fighter_character_assets_test
        tests/native/fighter_character_assets_test.c)
    target_include_directories(native_fighter_character_assets_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_fighter_character_assets_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    target_link_libraries(native_fighter_character_assets_test PRIVATE
        melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx
        aurora_dvd aurora_card aurora_thp)
    target_link_options(native_fighter_character_assets_test PRIVATE
        "LINKER:/opt:ref")
    aurora_copy_runtime_dlls(native_fighter_character_assets_test)
    add_test(NAME native_fighter_character_assets COMMAND
        native_fighter_character_assets_test "${MELEE_ISO}")
    add_test(NAME native_fighter_action_script_inventory COMMAND
        Python3::Interpreter
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/audit_native_action_scripts.py"
        --iso "${MELEE_ISO}")
    add_test(NAME native_item_script_inventory COMMAND
        Python3::Interpreter
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/audit_native_item_scripts.py"
        --iso "${MELEE_ISO}")
    add_test(NAME native_color_script_inventory COMMAND
        Python3::Interpreter
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/audit_native_color_scripts.py"
        --iso "${MELEE_ISO}")
    add_test(NAME native_stage_item_script_inventory COMMAND
        Python3::Interpreter
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/audit_native_stage_item_scripts.py"
        --iso "${MELEE_ISO}")
    add_test(NAME native_cpu_data_inventory COMMAND
        Python3::Interpreter
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/audit_native_cpu_data.py"
        --iso "${MELEE_ISO}")
    add_test(NAME native_kirby_copy_data_inventory COMMAND
        Python3::Interpreter
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/audit_native_kirby_copy_data.py"
        --iso "${MELEE_ISO}")
    add_executable(native_kirby_copy_assets_test
        tests/native/kirby_copy_assets_test.c)
    target_include_directories(native_kirby_copy_assets_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_kirby_copy_assets_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    target_link_libraries(native_kirby_copy_assets_test PRIVATE
        melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx
        aurora_dvd aurora_card aurora_thp)
    target_link_options(native_kirby_copy_assets_test PRIVATE
        "LINKER:/opt:ref")
    aurora_copy_runtime_dlls(native_kirby_copy_assets_test)
    add_test(NAME native_kirby_copy_assets COMMAND
        native_kirby_copy_assets_test "${MELEE_ISO}")
    add_executable(native_fighter_costume_assets_test
        tests/native/fighter_costume_assets_test.c)
    target_include_directories(native_fighter_costume_assets_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_fighter_costume_assets_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    target_link_libraries(native_fighter_costume_assets_test PRIVATE
        melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx
        aurora_dvd aurora_card aurora_thp)
    target_link_options(native_fighter_costume_assets_test PRIVATE
        "LINKER:/opt:ref")
    aurora_copy_runtime_dlls(native_fighter_costume_assets_test)
    add_test(NAME native_fighter_costume_assets COMMAND
        native_fighter_costume_assets_test "${MELEE_ISO}")
    add_executable(native_costume_package_test
        tests/native/costume_package_test.c)
    target_include_directories(native_costume_package_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_costume_package_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    target_link_libraries(native_costume_package_test PRIVATE
        melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx
        aurora_dvd aurora_card aurora_thp)
    target_link_options(native_costume_package_test PRIVATE
        "LINKER:/opt:ref")
    aurora_copy_runtime_dlls(native_costume_package_test)
    add_test(NAME native_costume_package COMMAND
        native_costume_package_test "${MELEE_ISO}")
    add_executable(native_costume_family_test tests/native/costume_family_test.c)
    target_include_directories(native_costume_family_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_costume_family_test PRIVATE MELEE_NATIVE=1)
    target_link_libraries(native_costume_family_test PRIVATE
        melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx
        aurora_dvd aurora_card aurora_thp)
    target_link_options(native_costume_family_test PRIVATE "LINKER:/opt:ref")
    aurora_copy_runtime_dlls(native_costume_family_test)
    add_executable(native_css_layout_test
        tests/native/css_layout_test.c)
    target_include_directories(native_css_layout_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_css_layout_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    target_link_libraries(native_css_layout_test PRIVATE
        melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx
        aurora_dvd aurora_card aurora_thp)
    target_link_options(native_css_layout_test PRIVATE
        "LINKER:/opt:ref")
    aurora_copy_runtime_dlls(native_css_layout_test)
    add_test(NAME native_css_layout COMMAND native_css_layout_test)
    add_executable(native_css_participants_test
        tests/native/css_participants_test.c
        src/melee/mn/native_css_participants.c
        src/melee/gm/native_lobby_selection.c)
    target_include_directories(native_css_participants_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_css_participants_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    add_test(NAME native_css_participants COMMAND native_css_participants_test)
    add_executable(native_fighter_animation_bank_test
        tests/native/fighter_animation_bank_test.c)
    target_include_directories(native_fighter_animation_bank_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_fighter_animation_bank_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    target_link_libraries(native_fighter_animation_bank_test PRIVATE
        melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx
        aurora_dvd aurora_card aurora_thp)
    target_link_options(native_fighter_animation_bank_test PRIVATE
        "LINKER:/opt:ref")
    aurora_copy_runtime_dlls(native_fighter_animation_bank_test)
    add_test(NAME native_fighter_animation_bank COMMAND
        native_fighter_animation_bank_test "${MELEE_ISO}")
    add_executable(native_fighter_animation_editor_test
        tests/native/fighter_animation_editor/test.c
        tests/native/animation_scheduler_boundary.c src/sysdolphin/baselib/native_animation_pool.c src/melee/gm/native_match_arena.c
        src/melee/gm/native_match_pool.c src/melee/gm/native_match_snapshot_pool.c
        src/melee/ft/native_fighter_animation_editor.c
        src/melee/lb/lbanim_dat.c src/sysdolphin/baselib/dat.c
        src/sysdolphin/baselib/fobj.c src/sysdolphin/baselib/spline.c)
    target_include_directories(native_fighter_animation_editor_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_fighter_animation_editor_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    target_compile_options(native_fighter_animation_editor_test PRIVATE
        -ffp-contract=off -ffunction-sections -fdata-sections)
    target_link_options(native_fighter_animation_editor_test PRIVATE "LINKER:/opt:ref")
    add_test(NAME native_fighter_animation_editor COMMAND native_fighter_animation_editor_test)
    add_executable(native_fighter_pose_editor_test
        tests/native/fighter_pose_editor/test.c
        tests/native/animation_scheduler_boundary.c src/sysdolphin/baselib/native_animation_pool.c src/melee/gm/native_match_arena.c
        src/melee/gm/native_match_pool.c src/melee/gm/native_match_snapshot_pool.c
        src/melee/ft/native_fighter_pose_editor.c
        src/melee/ft/native_fighter_animation_editor.c
        src/melee/lb/lbanim_dat.c src/sysdolphin/baselib/dat.c
        src/sysdolphin/baselib/fobj.c src/sysdolphin/baselib/spline.c)
    target_include_directories(native_fighter_pose_editor_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_fighter_pose_editor_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    target_compile_options(native_fighter_pose_editor_test PRIVATE
        -ffp-contract=off -ffunction-sections -fdata-sections)
    target_link_options(native_fighter_pose_editor_test PRIVATE "LINKER:/opt:ref")
    add_test(NAME native_fighter_pose_editor COMMAND native_fighter_pose_editor_test)
    add_executable(native_pause_assets_test tests/native/pause_assets_test.c)
    target_include_directories(native_pause_assets_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_pause_assets_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    target_link_libraries(native_pause_assets_test PRIVATE
        melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx
        aurora_dvd aurora_card aurora_thp)
    target_link_options(native_pause_assets_test PRIVATE "LINKER:/opt:ref")
    aurora_copy_runtime_dlls(native_pause_assets_test)
    add_test(NAME native_pause_assets COMMAND
        native_pause_assets_test "${MELEE_ISO}")
    add_executable(native_result_assets_test tests/native/result_assets_test.c)
    target_include_directories(native_result_assets_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_result_assets_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    target_link_libraries(native_result_assets_test PRIVATE
        melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx
        aurora_dvd aurora_card aurora_thp)
    target_link_options(native_result_assets_test PRIVATE "LINKER:/opt:ref")
    aurora_copy_runtime_dlls(native_result_assets_test)
    add_test(NAME native_result_assets COMMAND
        native_result_assets_test "${MELEE_ISO}")
    add_executable(native_ifall_assets_test tests/native/ifall_assets_test.c)
    target_include_directories(native_ifall_assets_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_ifall_assets_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    target_link_libraries(native_ifall_assets_test PRIVATE
        melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx
        aurora_dvd aurora_card aurora_thp)
    target_link_options(native_ifall_assets_test PRIVATE "LINKER:/opt:ref")
    aurora_copy_runtime_dlls(native_ifall_assets_test)
    add_test(NAME native_ifall_assets COMMAND
        native_ifall_assets_test "${MELEE_ISO}")
    add_executable(native_ifcoget_assets_test
        tests/native/ifcoget_assets_test.c)
    target_include_directories(native_ifcoget_assets_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_ifcoget_assets_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    target_link_libraries(native_ifcoget_assets_test PRIVATE
        melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx
        aurora_dvd aurora_card aurora_thp)
    target_link_options(native_ifcoget_assets_test PRIVATE "LINKER:/opt:ref")
    aurora_copy_runtime_dlls(native_ifcoget_assets_test)
    add_test(NAME native_ifcoget_assets COMMAND
        native_ifcoget_assets_test "${MELEE_ISO}")
    add_executable(native_lbbf_assets_test tests/native/lbbf_assets_test.c)
    target_include_directories(native_lbbf_assets_test PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_lbbf_assets_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    target_link_libraries(native_lbbf_assets_test PRIVATE
        melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx
        aurora_dvd aurora_card aurora_thp)
    target_link_options(native_lbbf_assets_test PRIVATE "LINKER:/opt:ref")
    aurora_copy_runtime_dlls(native_lbbf_assets_test)
    add_test(NAME native_lbbf_assets COMMAND
        native_lbbf_assets_test "${MELEE_ISO}")
    add_test(NAME native_panic COMMAND Python3::Interpreter
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/check_panic.py"
        "$<TARGET_FILE:native_heap_test>")
    add_executable(native_model_runtime_test tests/native/model_runtime_test.c
        tests/native/figatree_test.c tests/native/gobj_test.c
        tests/native/moving_collision_test.c tests/native/fighter_material_test.c
        tests/native/fighter_parts_test.c tests/native/article_model_test.c
        tests/native/article_model_roots_import_test.c
        tests/native/article_animation_array_import_test.c
        tests/native/article_joint_visibility_test.c)
    target_include_directories(native_model_runtime_test PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_model_runtime_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    target_link_libraries(native_model_runtime_test PRIVATE melee_runtime
        aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx aurora_dvd aurora_card aurora_thp)
    target_link_options(native_model_runtime_test PRIVATE "LINKER:/errorlimit:0" "LINKER:/opt:ref")
    aurora_copy_runtime_dlls(native_model_runtime_test)
    add_test(NAME native_model_runtime COMMAND native_model_runtime_test)
    target_link_libraries(native_collision_test PRIVATE melee_runtime
        aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx aurora_dvd aurora_card aurora_thp)
    target_link_options(native_collision_test PRIVATE "LINKER:/errorlimit:0" "LINKER:/opt:ref")
    aurora_copy_runtime_dlls(native_collision_test)
    # Release optimizations must not compile away assert-based test checks (or
    # the setup calls inside them). This affects test executables only.
    get_property(native_test_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(test_target IN LISTS native_test_targets)
        if(test_target MATCHES "^native_.*_test$")
            target_compile_options(${test_target} PRIVATE -UNDEBUG)
        endif()
    endforeach()
endif()

# Rendering bring-up uses the original HSD runtime, not a replacement model renderer.
add_executable(native_model_view tools/native_model_view.c tools/native_fighter_preview.c)
target_include_directories(native_model_view PRIVATE src extern/dolphin/include)
target_compile_definitions(native_model_view PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
target_link_libraries(native_model_view PRIVATE aurora_main melee_runtime aurora_os aurora_gx
    aurora_vi aurora_pad aurora_mtx aurora_dvd aurora_card aurora_thp)
aurora_copy_runtime_dlls(native_model_view)

add_executable(native_content_tool tools/native_workshop_authoring_tool.c tools/native_content_tool.c tools/native_source_content_tool.c tools/native_source_resource_tool.c
    tools/native_fighter_parameter_tool.c tools/native_fighter_cpu_tool.c)

# Generated C is retained only as a behavioral oracle for comparison fixtures.
add_executable(native_script_reference_exporter tests/native/script_reference_exporter.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_reference_exporter PRIVATE src tools extern/dolphin/include)
target_compile_definitions(native_script_reference_exporter PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_library(native_fighter_module_fixture MODULE
    tests/native/fighter_module_fixture.c)
target_include_directories(native_fighter_module_fixture PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_module_fixture PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
set_target_properties(native_fighter_module_fixture PROPERTIES PREFIX "")
add_library(native_fighter_module_wrong_layout_fixture MODULE
    tests/native/fighter_module_wrong_layout_fixture.c)
target_include_directories(native_fighter_module_wrong_layout_fixture PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_module_wrong_layout_fixture PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
set_target_properties(native_fighter_module_wrong_layout_fixture
    PROPERTIES PREFIX "")
add_executable(native_fighter_module_lifetime_test
    tests/native/fighter_module_lifetime_test.c)
target_include_directories(native_fighter_module_lifetime_test PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_module_lifetime_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
target_link_libraries(native_fighter_module_lifetime_test PRIVATE
    melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx
    aurora_dvd aurora_card aurora_thp)
add_test(NAME native_fighter_module_lifetime COMMAND
    native_fighter_module_lifetime_test
    "$<TARGET_FILE:native_fighter_module_fixture>"
    "$<TARGET_FILE:native_fighter_module_wrong_layout_fixture>")
add_executable(native_fighter_article_ownership_test
    tests/native/fighter_article_ownership_test.c)
target_include_directories(native_fighter_article_ownership_test PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_article_ownership_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
target_link_libraries(native_fighter_article_ownership_test PRIVATE
    melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx
    aurora_dvd aurora_card aurora_thp)
add_test(NAME native_fighter_article_ownership COMMAND
    "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/fighter_article_ownership.py"
    "$<TARGET_FILE:native_content_tool>" "$<TARGET_FILE:native_fighter_article_ownership_test>"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Super Smash Bros. Melee (USA) (v1.02).iso")
set(NATIVE_FACING_SCRIPT_C
    "${CMAKE_CURRENT_BINARY_DIR}/facing_speed.generated.c")
add_custom_command(OUTPUT "${NATIVE_FACING_SCRIPT_C}"
    COMMAND $<TARGET_FILE:native_script_reference_exporter>
        "${CMAKE_SOURCE_DIR}/tests/native/facing_speed.fms"
        script_probe "${NATIVE_FACING_SCRIPT_C}"
    DEPENDS native_script_reference_exporter
        "${CMAKE_SOURCE_DIR}/tests/native/facing_speed.fms"
        "${CMAKE_SOURCE_DIR}/tools/native_fighter_script.c"
    VERBATIM)
add_library(native_fighter_script_fixture MODULE "${NATIVE_FACING_SCRIPT_C}")
target_include_directories(native_fighter_script_fixture PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_script_fixture PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
set_target_properties(native_fighter_script_fixture PROPERTIES PREFIX "")
add_executable(native_article_operations_script_test tests/native/article_operations_script_test.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_article_operations_script_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_article_operations_script_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_test(NAME native_article_operations_script COMMAND native_article_operations_script_test)

add_executable(native_fighter_target_test tests/native/fighter_target_test.c)
target_include_directories(native_fighter_target_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_fighter_target_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
target_link_libraries(native_fighter_target_test PRIVATE melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx aurora_dvd aurora_card aurora_thp)
target_link_options(native_fighter_target_test PRIVATE "LINKER:/opt:ref")
aurora_copy_runtime_dlls(native_fighter_target_test)
add_test(NAME native_fighter_target COMMAND native_fighter_target_test)

add_executable(native_fighter_script_test tests/native/fighter_script_test.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES})
target_include_directories(native_fighter_script_test PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_script_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
target_link_libraries(native_fighter_script_test PRIVATE
    melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx
    aurora_dvd aurora_card aurora_thp)
add_test(NAME native_fighter_script COMMAND native_fighter_script_test
    "$<TARGET_FILE:native_fighter_script_fixture>"
    "${CMAKE_SOURCE_DIR}/tests/native/facing_speed.fms")

add_executable(native_script_model_effect_compiler_test
    tests/native/script_model_effect_compiler_test.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_model_effect_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_model_effect_compiler_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_test(NAME native_script_model_effect_compiler COMMAND native_script_model_effect_compiler_test)
add_executable(native_script_cpu_projectile_defense_compiler_test
    tests/native/script_cpu_projectile_defense_compiler_test.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_cpu_projectile_defense_compiler_test PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_script_cpu_projectile_defense_compiler_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_test(NAME native_script_cpu_projectile_defense_compiler
    COMMAND native_script_cpu_projectile_defense_compiler_test)
set(MODEL_EFFECT_ACTION_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/script_model_effect_action.generated.c")
set(MODEL_EFFECT_STATE_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/script_model_effect_state.generated.c")
add_custom_command(OUTPUT "${MODEL_EFFECT_ACTION_SOURCE}" "${MODEL_EFFECT_STATE_SOURCE}"
    COMMAND $<TARGET_FILE:native_script_model_effect_compiler_test>
        "${MODEL_EFFECT_ACTION_SOURCE}" "${MODEL_EFFECT_STATE_SOURCE}"
    DEPENDS native_script_model_effect_compiler_test VERBATIM)
foreach(MODEL_EFFECT_FORM IN ITEMS action state)
    add_library(native_script_model_effect_${MODEL_EFFECT_FORM}_fixture MODULE
        "${CMAKE_CURRENT_BINARY_DIR}/script_model_effect_${MODEL_EFFECT_FORM}.generated.c")
    target_include_directories(native_script_model_effect_${MODEL_EFFECT_FORM}_fixture PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_script_model_effect_${MODEL_EFFECT_FORM}_fixture PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    set_target_properties(native_script_model_effect_${MODEL_EFFECT_FORM}_fixture PROPERTIES PREFIX "")
endforeach()
add_executable(native_script_model_effect_module_test
    tests/native/script_model_effect_module_test.c src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_model_effect_module_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_model_effect_module_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_dependencies(native_script_model_effect_module_test
    native_script_model_effect_action_fixture native_script_model_effect_state_fixture)
add_test(NAME native_script_model_effect_module COMMAND native_script_model_effect_module_test
    "$<TARGET_FILE:native_script_model_effect_action_fixture>"
    "$<TARGET_FILE:native_script_model_effect_state_fixture>")
foreach(independent_form action state)
    set(oracle_construction)
    if(independent_form STREQUAL "state")
        set(oracle_construction --construction ftDataTest 341 3)
    endif()
    set(independent_source "${CMAKE_SOURCE_DIR}/tests/native/independent_fixture/${independent_form}.fms")
    set(independent_generated "${CMAKE_CURRENT_BINARY_DIR}/independent_${independent_form}.generated.c")
    add_custom_command(OUTPUT "${independent_generated}"
        COMMAND $<TARGET_FILE:native_script_reference_exporter> "${independent_source}"
            independent_probe "${independent_generated}" ${oracle_construction}
        DEPENDS native_script_reference_exporter "${independent_source}" VERBATIM)
    add_library(native_fighter_independent_${independent_form}_fixture MODULE "${independent_generated}")
    target_include_directories(native_fighter_independent_${independent_form}_fixture PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_fighter_independent_${independent_form}_fixture PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    set_target_properties(native_fighter_independent_${independent_form}_fixture PROPERTIES PREFIX "")
    add_test(NAME native_fighter_independent_${independent_form} COMMAND
        native_fighter_script_test --independent
        "$<TARGET_FILE:native_fighter_independent_${independent_form}_fixture>")
endforeach()

add_executable(native_script_typed_locals_compiler_test
    tests/native/script_typed_locals_compiler_test.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_typed_locals_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_typed_locals_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_script_typed_locals_compiler COMMAND native_script_typed_locals_compiler_test)
set(TYPED_LOCALS_ACTION_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/script_typed_locals_action_fixture.h")
set(TYPED_LOCALS_CALLBACK_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/script_typed_locals_callback_fixture.h")
add_custom_command(OUTPUT "${TYPED_LOCALS_ACTION_FIXTURE}" "${TYPED_LOCALS_CALLBACK_FIXTURE}"
    COMMAND $<TARGET_FILE:native_script_typed_locals_compiler_test>
        "${TYPED_LOCALS_ACTION_FIXTURE}" "${TYPED_LOCALS_CALLBACK_FIXTURE}"
    DEPENDS native_script_typed_locals_compiler_test VERBATIM)
foreach(TYPED_LOCALS_MODE IN ITEMS action callback)
    if(TYPED_LOCALS_MODE STREQUAL "callback")
        set(TYPED_LOCALS_CALLBACKS 1)
    else()
        set(TYPED_LOCALS_CALLBACKS 0)
    endif()
    add_executable(native_script_typed_locals_${TYPED_LOCALS_MODE}_test
        tests/native/script_typed_locals_module_test.c
        "${TYPED_LOCALS_ACTION_FIXTURE}" "${TYPED_LOCALS_CALLBACK_FIXTURE}")
    target_include_directories(native_script_typed_locals_${TYPED_LOCALS_MODE}_test PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}" src extern/dolphin/include)
    target_compile_definitions(native_script_typed_locals_${TYPED_LOCALS_MODE}_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS TYPED_LOCALS_CALLBACKS=${TYPED_LOCALS_CALLBACKS})
    add_test(NAME native_script_typed_locals_${TYPED_LOCALS_MODE}
        COMMAND native_script_typed_locals_${TYPED_LOCALS_MODE}_test)
endforeach()
add_executable(native_fighter_parameters_test
    tests/native/fighter_parameters_test.c
    src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_fighter_parameters_test PRIVATE src)
add_test(NAME native_fighter_parameters COMMAND native_fighter_parameters_test)
add_executable(native_smash_contracts_compiler_test
    tests/native/smash_contracts/compiler.c ${NATIVE_FIGHTER_SCRIPT_SOURCES}
    src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_smash_contracts_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_smash_contracts_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_smash_contracts_compiler COMMAND native_smash_contracts_compiler_test)

foreach(trail_form action state)
    set(oracle_construction)
    if(trail_form STREQUAL "state")
        set(oracle_construction --construction ftDataTest 341 1)
    endif()
    set(trail_source "${CMAKE_SOURCE_DIR}/tests/native/trail_fixture/${trail_form}.fms")
    set(trail_generated "${CMAKE_CURRENT_BINARY_DIR}/trail_${trail_form}.generated.c")
    add_custom_command(OUTPUT "${trail_generated}"
        COMMAND $<TARGET_FILE:native_script_reference_exporter> "${trail_source}"
            trail_probe "${trail_generated}" ${oracle_construction}
        DEPENDS native_script_reference_exporter "${trail_source}"
            tests/native/trail_fixture/fighter_parameters.fmp
        VERBATIM)
    add_library(native_fighter_trail_${trail_form}_fixture MODULE "${trail_generated}")
    target_include_directories(native_fighter_trail_${trail_form}_fixture PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_fighter_trail_${trail_form}_fixture PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    set_target_properties(native_fighter_trail_${trail_form}_fixture PROPERTIES PREFIX "")
endforeach()
foreach(guard_form action state)
    set(oracle_construction)
    if(guard_form STREQUAL "state")
        set(oracle_construction --construction ftDataTest 341 1)
    endif()
    set(guard_source "${CMAKE_SOURCE_DIR}/tests/native/guard_fixture/${guard_form}.fms")
    set(guard_generated "${CMAKE_CURRENT_BINARY_DIR}/guard_${guard_form}.generated.c")
    add_custom_command(OUTPUT "${guard_generated}"
        COMMAND $<TARGET_FILE:native_script_reference_exporter> "${guard_source}"
            guard_probe "${guard_generated}" ${oracle_construction}
        DEPENDS native_script_reference_exporter "${guard_source}" VERBATIM)
    add_library(native_fighter_guard_${guard_form}_fixture MODULE "${guard_generated}")
    target_include_directories(native_fighter_guard_${guard_form}_fixture PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_fighter_guard_${guard_form}_fixture PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    set_target_properties(native_fighter_guard_${guard_form}_fixture PROPERTIES PREFIX "")
endforeach()
add_executable(native_fighter_guard_test tests/native/fighter_guard_test.c)
target_include_directories(native_fighter_guard_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_fighter_guard_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_fighter_guard COMMAND native_fighter_guard_test
    "$<TARGET_FILE:native_fighter_guard_action_fixture>"
    "$<TARGET_FILE:native_fighter_guard_state_fixture>")
foreach(policy_form action state single_plane)
    set(oracle_construction)
    if(policy_form STREQUAL "state")
        set(oracle_construction --construction ftDataTest 341 1)
    endif()
    set(policy_source "${CMAKE_SOURCE_DIR}/tests/native/common_policy_fixture/${policy_form}.fms")
    set(policy_generated "${CMAKE_CURRENT_BINARY_DIR}/common_policy_${policy_form}.generated.c")
    add_custom_command(OUTPUT "${policy_generated}"
        COMMAND $<TARGET_FILE:native_script_reference_exporter> "${policy_source}"
            common_policy_probe "${policy_generated}" ${oracle_construction}
        DEPENDS native_script_reference_exporter "${policy_source}" VERBATIM)
    add_library(native_fighter_common_policy_${policy_form}_fixture MODULE "${policy_generated}")
    target_include_directories(native_fighter_common_policy_${policy_form}_fixture PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_fighter_common_policy_${policy_form}_fixture PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    set_target_properties(native_fighter_common_policy_${policy_form}_fixture PROPERTIES PREFIX "")
endforeach()
add_executable(native_fighter_common_policy_test tests/native/fighter_common_policy_test.c)
target_include_directories(native_fighter_common_policy_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_fighter_common_policy_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_fighter_common_policy COMMAND native_fighter_common_policy_test
    "$<TARGET_FILE:native_fighter_common_policy_action_fixture>"
    "$<TARGET_FILE:native_fighter_common_policy_state_fixture>"
    "$<TARGET_FILE:native_fighter_common_policy_single_plane_fixture>")
add_executable(native_fighter_trail_test tests/native/fighter_trail_test.c
    src/melee/ft/native_fighter_parameters.c src/melee/ft/native_fighter_trail.c)
target_include_directories(native_fighter_trail_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_fighter_trail_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_fighter_trail COMMAND native_fighter_trail_test
    "$<TARGET_FILE:native_fighter_trail_action_fixture>"
    "$<TARGET_FILE:native_fighter_trail_state_fixture>"
    "${CMAKE_SOURCE_DIR}/tests/native/trail_fixture/fighter_parameters.fmp")

set(NATIVE_PARAMETER_SCRIPT_C
    "${CMAKE_CURRENT_BINARY_DIR}/parameter_fixture.generated.c")
add_custom_command(OUTPUT "${NATIVE_PARAMETER_SCRIPT_C}"
    COMMAND $<TARGET_FILE:native_script_reference_exporter>
        "${CMAKE_SOURCE_DIR}/tests/native/parameter_fixture/behavior.fms"
        parameter_fixture "${NATIVE_PARAMETER_SCRIPT_C}"
    DEPENDS native_script_reference_exporter
        "${CMAKE_SOURCE_DIR}/tests/native/parameter_fixture/behavior.fms"
        "${CMAKE_SOURCE_DIR}/tests/native/parameter_fixture/fighter_parameters.fmp"
    VERBATIM)
add_library(native_fighter_parameter_fixture MODULE "${NATIVE_PARAMETER_SCRIPT_C}")
target_include_directories(native_fighter_parameter_fixture PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_parameter_fixture PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
set_target_properties(native_fighter_parameter_fixture PROPERTIES PREFIX "")
add_executable(native_fighter_parameter_module_test
    tests/native/fighter_parameter_module_test.c
    src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_fighter_parameter_module_test PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_parameter_module_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_dependencies(native_fighter_parameter_module_test
    native_fighter_parameter_fixture)
add_test(NAME native_fighter_parameter_module
    COMMAND native_fighter_parameter_module_test
    "$<TARGET_FILE:native_fighter_parameter_fixture>")
set(NATIVE_STOCK_SPECIAL_FIXTURES)
foreach(kind side up down)
    set(source "${CMAKE_CURRENT_BINARY_DIR}/stock_${kind}_special.generated.c")
    set(script "${CMAKE_SOURCE_DIR}/tests/native/stock_${kind}_special.fms")
    add_custom_command(OUTPUT "${source}"
        COMMAND $<TARGET_FILE:native_script_reference_exporter>
            "${script}" stock_special_probe "${source}"
        DEPENDS native_script_reference_exporter "${script}"
            "${CMAKE_SOURCE_DIR}/tools/native_fighter_script.c"
        VERBATIM)
    add_library(native_fighter_stock_${kind}_script_fixture MODULE "${source}")
    target_include_directories(native_fighter_stock_${kind}_script_fixture
        PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_fighter_stock_${kind}_script_fixture
        PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    set_target_properties(native_fighter_stock_${kind}_script_fixture
        PROPERTIES PREFIX "")
    list(APPEND NATIVE_STOCK_SPECIAL_FIXTURES
        "$<TARGET_FILE:native_fighter_stock_${kind}_script_fixture>")
endforeach()
add_executable(native_fighter_stock_special_script_test
    tests/native/fighter_stock_special_script_test.c)
target_include_directories(native_fighter_stock_special_script_test PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_stock_special_script_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
target_link_libraries(native_fighter_stock_special_script_test PRIVATE
    melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx
    aurora_dvd aurora_card aurora_thp)
add_test(NAME native_fighter_stock_special_script COMMAND
    native_fighter_stock_special_script_test
    ${NATIVE_STOCK_SPECIAL_FIXTURES})
add_executable(native_fighter_clone_metadata_test
    tests/native/fighter_clone_metadata_test.c)
target_include_directories(native_fighter_clone_metadata_test PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_clone_metadata_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
target_link_libraries(native_fighter_clone_metadata_test PRIVATE
    melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx
    aurora_dvd aurora_card aurora_thp)
add_test(NAME native_fighter_clone_metadata COMMAND
    native_fighter_clone_metadata_test)
set(NATIVE_AUTHORED_SCRIPT_C
    "${CMAKE_CURRENT_BINARY_DIR}/authored_state.generated.c")
add_custom_command(OUTPUT "${NATIVE_AUTHORED_SCRIPT_C}"
    COMMAND $<TARGET_FILE:native_script_reference_exporter>
        "${CMAKE_SOURCE_DIR}/tests/native/authored_state.fms"
        authored_probe "${NATIVE_AUTHORED_SCRIPT_C}"
    DEPENDS native_script_reference_exporter
        "${CMAKE_SOURCE_DIR}/tests/native/authored_state.fms"
        "${CMAKE_SOURCE_DIR}/tools/native_fighter_script.c"
    VERBATIM)
add_library(native_fighter_authored_script_fixture MODULE
    "${NATIVE_AUTHORED_SCRIPT_C}")
target_include_directories(native_fighter_authored_script_fixture PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_authored_script_fixture PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
set_target_properties(native_fighter_authored_script_fixture
    PROPERTIES PREFIX "")
add_executable(native_fighter_script_authored_state_test
    tests/native/fighter_script_authored_state_test.c)
target_include_directories(native_fighter_script_authored_state_test PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_script_authored_state_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_test(NAME native_fighter_script_authored_state COMMAND
    native_fighter_script_authored_state_test
    "$<TARGET_FILE:native_fighter_authored_script_fixture>")

# Exercise Rook's actual editable up-special source, not a duplicate test move.
set(ROOK_SCRIPT_STATE "${CMAKE_SOURCE_DIR}/examples/rook_workshop/fighter_state.fms")
set(ROOK_SCRIPT_UP "${CMAKE_SOURCE_DIR}/examples/rook_workshop/moves/003_UpSpecial.fms")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${ROOK_SCRIPT_STATE}" "${ROOK_SCRIPT_UP}")
file(READ "${ROOK_SCRIPT_STATE}" ROOK_SCRIPT_STATE_TEXT)
file(READ "${ROOK_SCRIPT_UP}" ROOK_SCRIPT_UP_TEXT)
file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/rook_up.fms"
    CONTENT "${ROOK_SCRIPT_STATE_TEXT}\n${ROOK_SCRIPT_UP_TEXT}" @ONLY)
set(NATIVE_ROOK_UP_SCRIPT_C "${CMAKE_CURRENT_BINARY_DIR}/rook_up.generated.c")
add_custom_command(OUTPUT "${NATIVE_ROOK_UP_SCRIPT_C}"
    COMMAND $<TARGET_FILE:native_script_reference_exporter>
        "${CMAKE_CURRENT_BINARY_DIR}/rook_up.fms" rook_up_probe
        "${NATIVE_ROOK_UP_SCRIPT_C}"
    DEPENDS native_script_reference_exporter "${CMAKE_CURRENT_BINARY_DIR}/rook_up.fms"
    VERBATIM)
add_library(native_fighter_rook_up_fixture MODULE "${NATIVE_ROOK_UP_SCRIPT_C}")
target_include_directories(native_fighter_rook_up_fixture PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_rook_up_fixture PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
set_target_properties(native_fighter_rook_up_fixture PROPERTIES PREFIX "")
add_executable(native_fighter_rook_up_script_test
    tests/native/fighter_rook_up_script_test.c)
target_include_directories(native_fighter_rook_up_script_test PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_rook_up_script_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_test(NAME native_fighter_rook_up_script COMMAND
    native_fighter_rook_up_script_test
    "$<TARGET_FILE:native_fighter_rook_up_fixture>")

set(NATIVE_INTEGER_RANDOM_SCRIPT_C
    "${CMAKE_CURRENT_BINARY_DIR}/integer_random_state.generated.c")
add_custom_command(OUTPUT "${NATIVE_INTEGER_RANDOM_SCRIPT_C}"
    COMMAND $<TARGET_FILE:native_script_reference_exporter>
        "${CMAKE_SOURCE_DIR}/tests/native/integer_random_state.fms"
        integer_random_probe "${NATIVE_INTEGER_RANDOM_SCRIPT_C}" --construction ftDataRook 341 12
    DEPENDS native_script_reference_exporter
        "${CMAKE_SOURCE_DIR}/tests/native/integer_random_state.fms"
        "${CMAKE_SOURCE_DIR}/tools/native_fighter_script.c"
    VERBATIM)
add_library(native_fighter_integer_random_fixture MODULE
    "${NATIVE_INTEGER_RANDOM_SCRIPT_C}")
target_include_directories(native_fighter_integer_random_fixture PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_integer_random_fixture PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
set_target_properties(native_fighter_integer_random_fixture PROPERTIES PREFIX "")
add_executable(native_fighter_integer_random_script_test
    tests/native/fighter_integer_random_script_test.c)
target_include_directories(native_fighter_integer_random_script_test PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_integer_random_script_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_test(NAME native_fighter_integer_random_script COMMAND
    native_fighter_integer_random_script_test
    "$<TARGET_FILE:native_fighter_integer_random_fixture>")

set(NATIVE_ARTICLE_ONLY_SCRIPT_C
    "${CMAKE_CURRENT_BINARY_DIR}/article_only.generated.c")
add_custom_command(OUTPUT "${NATIVE_ARTICLE_ONLY_SCRIPT_C}"
    COMMAND $<TARGET_FILE:native_script_reference_exporter>
        "${CMAKE_SOURCE_DIR}/tests/native/article_only.fms"
        article_only_probe "${NATIVE_ARTICLE_ONLY_SCRIPT_C}"
    DEPENDS native_script_reference_exporter
        "${CMAKE_SOURCE_DIR}/tests/native/article_only.fms"
        "${CMAKE_SOURCE_DIR}/tools/native_fighter_script.c"
    VERBATIM)
add_library(native_fighter_article_only_fixture MODULE
    "${NATIVE_ARTICLE_ONLY_SCRIPT_C}")
target_include_directories(native_fighter_article_only_fixture PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_article_only_fixture PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
set_target_properties(native_fighter_article_only_fixture
    PROPERTIES PREFIX "")
add_executable(native_fighter_script_article_only_test
    tests/native/fighter_script_article_only_test.c)
target_include_directories(native_fighter_script_article_only_test PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_script_article_only_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_test(NAME native_fighter_script_article_only COMMAND
    native_fighter_script_article_only_test
    "$<TARGET_FILE:native_fighter_article_only_fixture>")
set(NATIVE_MULTI_SPECIAL_SCRIPT_C
    "${CMAKE_CURRENT_BINARY_DIR}/multi_special.generated.c")
add_custom_command(OUTPUT "${NATIVE_MULTI_SPECIAL_SCRIPT_C}"
    COMMAND $<TARGET_FILE:native_script_reference_exporter>
        "${CMAKE_SOURCE_DIR}/tests/native/multi_special.fms"
        multi_probe "${NATIVE_MULTI_SPECIAL_SCRIPT_C}"
    DEPENDS native_script_reference_exporter
        "${CMAKE_SOURCE_DIR}/tests/native/multi_special.fms"
        "${CMAKE_SOURCE_DIR}/tools/native_fighter_script.c"
    VERBATIM)
add_library(native_fighter_multi_special_fixture MODULE
    "${NATIVE_MULTI_SPECIAL_SCRIPT_C}")
target_include_directories(native_fighter_multi_special_fixture PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_multi_special_fixture PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
set_target_properties(native_fighter_multi_special_fixture
    PROPERTIES PREFIX "")
add_executable(native_fighter_multi_special_script_test
    tests/native/fighter_multi_special_script_test.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES})
target_include_directories(native_fighter_multi_special_script_test PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_multi_special_script_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_dependencies(native_fighter_multi_special_script_test
    native_fighter_multi_special_fixture)
add_test(NAME native_fighter_multi_special_script COMMAND
    native_fighter_multi_special_script_test
    "$<TARGET_FILE:native_fighter_multi_special_fixture>"
    "${CMAKE_SOURCE_DIR}/tests/native/multi_special.fms")
set(NATIVE_MARIO_CAPE_SCRIPT_C
    "${CMAKE_CURRENT_BINARY_DIR}/mario_cape.generated.c")
add_custom_command(OUTPUT "${NATIVE_MARIO_CAPE_SCRIPT_C}"
    COMMAND $<TARGET_FILE:native_script_reference_exporter>
        "${CMAKE_SOURCE_DIR}/tests/native/cape_lifecycle.fms"
        mario_cape "${NATIVE_MARIO_CAPE_SCRIPT_C}"
    DEPENDS native_script_reference_exporter
        "${CMAKE_SOURCE_DIR}/tests/native/cape_lifecycle.fms"
        "${CMAKE_SOURCE_DIR}/tools/native_fighter_script.c"
    VERBATIM)
add_library(native_fighter_cape_fixture MODULE
    "${NATIVE_MARIO_CAPE_SCRIPT_C}")
target_include_directories(native_fighter_cape_fixture PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_cape_fixture PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
set_target_properties(native_fighter_cape_fixture PROPERTIES PREFIX "")
# Full fighter templates use project-owned parameters and assets.
# Their source comparison drivers import, compile and exercise real packages;
# raw template DLLs (without parameter import) are not runnable modules.
add_test(NAME native_fox_falco_rig_assets COMMAND Python3::Interpreter
    "${CMAKE_SOURCE_DIR}/tests/native/fox_falco_rig_stock_compare.py"
    --iso "${MELEE_ISO}" --validate-clones
    --output "${CMAKE_CURRENT_BINARY_DIR}/fox_falco_rig_test.json")
set(NATIVE_ACTION_HANDLER_SCRIPT_C
    "${CMAKE_CURRENT_BINARY_DIR}/fighter_action_handler.generated.c")
add_custom_command(OUTPUT "${NATIVE_ACTION_HANDLER_SCRIPT_C}"
    COMMAND $<TARGET_FILE:native_script_reference_exporter>
        "${CMAKE_SOURCE_DIR}/tests/native/fighter_action_handler.fms"
        action_handler_probe "${NATIVE_ACTION_HANDLER_SCRIPT_C}"
    DEPENDS native_script_reference_exporter
        "${CMAKE_SOURCE_DIR}/tests/native/fighter_action_handler.fms"
        "${CMAKE_SOURCE_DIR}/tools/native_fighter_script.c"
    VERBATIM)
add_library(native_fighter_action_handler_fixture MODULE
    "${NATIVE_ACTION_HANDLER_SCRIPT_C}")
target_include_directories(native_fighter_action_handler_fixture PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_action_handler_fixture PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
set_target_properties(native_fighter_action_handler_fixture PROPERTIES PREFIX "")
add_executable(native_fighter_action_handler_test
    tests/native/fighter_action_handler_test.c)
target_include_directories(native_fighter_action_handler_test PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_action_handler_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_test(NAME native_fighter_action_handler COMMAND
    native_fighter_action_handler_test
    "$<TARGET_FILE:native_fighter_action_handler_fixture>")
set(NATIVE_LINK_CLINK_SCRIPT_C "${CMAKE_CURRENT_BINARY_DIR}/link_clink_wip.generated.c")
add_custom_command(OUTPUT "${NATIVE_LINK_CLINK_SCRIPT_C}"
    COMMAND Python3::Interpreter
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/fighter_script_inline_fixture.py"
        $<TARGET_FILE:native_script_reference_exporter>
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/link_clink_wip.fms"
        link_clink_wip "${NATIVE_LINK_CLINK_SCRIPT_C}"
    DEPENDS native_script_reference_exporter "${CMAKE_CURRENT_SOURCE_DIR}/examples/link_clink_wip.fms"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/fighter_script_inline_fixture.py"
    VERBATIM)
add_library(native_fighter_link_clink_example MODULE "${NATIVE_LINK_CLINK_SCRIPT_C}")
target_include_directories(native_fighter_link_clink_example PRIVATE src extern/dolphin/include)
target_compile_definitions(native_fighter_link_clink_example PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
set_target_properties(native_fighter_link_clink_example PROPERTIES PREFIX "")
add_executable(native_fighter_yoshi_stock_physics_test tests/native/yoshi_script_stock_physics_test.c)
target_include_directories(native_fighter_yoshi_stock_physics_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_fighter_yoshi_stock_physics_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
target_link_libraries(native_fighter_yoshi_stock_physics_test PRIVATE melee_runtime
    aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx aurora_dvd aurora_card aurora_thp)
target_link_options(native_fighter_yoshi_stock_physics_test PRIVATE "LINKER:/opt:ref")
aurora_copy_runtime_dlls(native_fighter_yoshi_stock_physics_test)

add_executable(native_fighter_link_spin_script_test
    tests/native/fighter_link_spin_script_test.c
    src/melee/ft/kinds/ftLink/ftlinkspecialhi.c
    src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_fighter_link_spin_script_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_fighter_link_spin_script_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_dependencies(native_fighter_link_spin_script_test native_fighter_link_clink_example)
add_test(NAME native_fighter_link_spin_script COMMAND native_fighter_link_spin_script_test "$<TARGET_FILE:native_fighter_link_clink_example>")

add_executable(native_fighter_link_bomb_script_test
    tests/native/fighter_link_bomb_script_test.c
    src/melee/it/kinds/itlinkbomb.c
    src/melee/ft/kinds/ftLink/ftlinkspeciallw.c
    src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_fighter_link_bomb_script_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_fighter_link_bomb_script_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_dependencies(native_fighter_link_bomb_script_test native_fighter_link_clink_example)
add_test(NAME native_fighter_link_bomb_script COMMAND native_fighter_link_bomb_script_test "$<TARGET_FILE:native_fighter_link_clink_example>")

add_executable(native_fighter_capture_enclosure_test tests/native/capture_enclosure_test.c src/melee/gm/native_match_pool.c src/melee/ft/native_fighter_capture_rig.c)
target_include_directories(native_fighter_capture_enclosure_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_fighter_capture_enclosure_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_test(NAME native_fighter_capture_enclosure COMMAND native_fighter_capture_enclosure_test)

set(NATIVE_ARROW_VIBRATION_C "${CMAKE_BINARY_DIR}/link_arrow_vibration.generated.c")
add_custom_command(OUTPUT "${NATIVE_ARROW_VIBRATION_C}"
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/link_arrow_vibration_fixture.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/melee/it/kinds/itlinkarrow.c" "${NATIVE_ARROW_VIBRATION_C}"
    DEPENDS tests/native/link_arrow_vibration_fixture.py src/melee/it/kinds/itlinkarrow.c
    VERBATIM)
add_executable(native_fighter_arrow_vibration_test tests/native/link_arrow_vibration_test.c "${NATIVE_ARROW_VIBRATION_C}")
target_include_directories(native_fighter_arrow_vibration_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_fighter_arrow_vibration_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_test(NAME native_fighter_arrow_vibration COMMAND native_fighter_arrow_vibration_test)

add_executable(native_fighter_mario_cape_script_test
    tests/native/fighter_mario_cape_script_test.c)
target_include_directories(native_fighter_mario_cape_script_test PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_fighter_mario_cape_script_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_dependencies(native_fighter_mario_cape_script_test native_fighter_cape_fixture)
add_test(NAME native_fighter_mario_cape_script COMMAND
    native_fighter_mario_cape_script_test
    "$<TARGET_FILE:native_fighter_cape_fixture>")
target_include_directories(native_content_tool PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_content_tool PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
target_link_libraries(native_content_tool PRIVATE
    melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx
    aurora_dvd aurora_card aurora_thp)
target_link_options(native_content_tool PRIVATE "LINKER:/opt:ref")
aurora_copy_runtime_dlls(native_content_tool)
add_test(NAME native_fighter_module_load COMMAND native_content_tool
    module-probe "$<TARGET_FILE:native_fighter_module_fixture>")
add_test(NAME native_fighter_animation_edit_roundtrip COMMAND
    Python3::Interpreter
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/fighter_animation_edit_roundtrip.py"
    "$<TARGET_FILE:native_content_tool>" "${MELEE_ISO}")
add_test(NAME native_fighter_pose_transfer_roundtrip COMMAND
    Python3::Interpreter
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/fighter_pose_transfer_roundtrip.py"
    "$<TARGET_FILE:native_content_tool>" "${MELEE_ISO}")
add_test(NAME native_fighter_normal_copy_roundtrip COMMAND
    Python3::Interpreter
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/fighter_normal_copy_roundtrip.py"
    "$<TARGET_FILE:native_content_tool>" "${MELEE_ISO}")
add_test(NAME native_fighter_sound_edit_roundtrip COMMAND
    Python3::Interpreter
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/fighter_sound_edit_roundtrip.py"
    "$<TARGET_FILE:native_content_tool>" "${MELEE_ISO}")
add_test(NAME native_fighter_action_duplicate_roundtrip COMMAND
    Python3::Interpreter
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/fighter_action_duplicate_roundtrip.py"
    "$<TARGET_FILE:native_content_tool>" "${MELEE_ISO}"
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/mario_full.fms")
add_test(NAME native_fighter_effect_edit_roundtrip COMMAND
    Python3::Interpreter
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/fighter_effect_edit_roundtrip.py"
    "$<TARGET_FILE:native_content_tool>" "${MELEE_ISO}")
add_test(NAME native_fighter_throw_damage_roundtrip COMMAND
    Python3::Interpreter
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/fighter_throw_damage_roundtrip.py"
    "$<TARGET_FILE:native_content_tool>" "${MELEE_ISO}")
add_test(NAME native_fighter_installed_clone_roundtrip COMMAND
    Python3::Interpreter
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/fighter_installed_clone_roundtrip.py"
    "$<TARGET_FILE:native_content_tool>" "${MELEE_ISO}"
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/mario_full.fms")

# The client follows the original game entry point and scene state machine.
# Build it by default so source changes also refresh the playable client.
target_link_libraries(melee_game PRIVATE yyjson melee_account_crypto aurora_main aurora_core
    LibJuice::LibJuice)
target_link_libraries(melee_runtime PUBLIC melee_math LibJuice::LibJuice yyjson melee_account_crypto ws2_32 winhttp bcrypt crypt32 comdlg32 windowscodecs ole32)

add_executable(native_feedback_input_test tests/native/native_feedback_input_test.c
    src/melee/gm/native_feedback_input.c)
target_include_directories(native_feedback_input_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_feedback_input_test PRIVATE MELEE_NATIVE=1)
add_test(NAME native_feedback_input COMMAND native_feedback_input_test)
add_executable(native_feedback_owner_test tests/native/native_feedback_owner_test.cpp
    src/melee/gm/native_feedback.cpp)
target_include_directories(native_feedback_owner_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_feedback_owner_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
target_link_libraries(native_feedback_owner_test PRIVATE aurora_core yyjson bcrypt)
add_test(NAME native_feedback_owner COMMAND native_feedback_owner_test "${CMAKE_BINARY_DIR}/tests/feedback-owner")
add_executable(native_news_text_test tests/native/native_news_text_test.cpp
    src/melee/ui/native_news_text.cpp)
target_include_directories(native_news_text_test PRIVATE src)
add_test(NAME native_news_text COMMAND native_news_text_test)
add_executable(melee_native src/melee/gm/gmmain.c)
target_include_directories(melee_native PRIVATE src extern/dolphin/include)
target_compile_definitions(melee_native PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
target_link_libraries(melee_native PRIVATE aurora_main melee_runtime aurora_os aurora_gx
    aurora_vi aurora_pad aurora_mtx aurora_dvd aurora_card aurora_thp)
target_link_options(melee_native PRIVATE "LINKER:/errorlimit:0" "LINKER:/opt:ref")
include(cmake/NativeOptimization.cmake)
aurora_copy_runtime_dlls(melee_native)
file(GENERATE OUTPUT "$<TARGET_FILE_DIR:melee_native>/native_release_runtime.txt"
    CONTENT "$<JOIN:$<TARGET_GENEX_EVAL:melee_native,$<TARGET_PROPERTY:melee_native,AURORA_RUNTIME_FILES>>,\n>\n$<JOIN:$<TARGET_GENEX_EVAL:native_content_tool,$<TARGET_PROPERTY:native_content_tool,AURORA_RUNTIME_FILES>>,\n>\n")
include(cmake/NativeUi.cmake)
include(cmake/NativeModelTests.cmake)

add_test(NAME native_samus_template_source COMMAND Python3::Interpreter
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/samus_grapple_constructor/compose.py"
    --check-template "${CMAKE_CURRENT_SOURCE_DIR}/examples/samus_full.fms")

add_executable(native_signaling_probe
    tools/native_signaling_probe.c
    src/melee/gm/native_signaling_client.c
    src/melee/gm/native_signaling_wire.c)
target_include_directories(native_signaling_probe PRIVATE src extern/dolphin/include)
target_compile_definitions(native_signaling_probe PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
target_link_libraries(native_signaling_probe PRIVATE winhttp)
target_sources(native_signaling_probe PRIVATE src/melee/gm/native_account_transport.cpp)
add_executable(native_chat_probe tools/native_chat_probe.c
    src/melee/ui/native_chat.c src/melee/ui/native_chat_wire.c)
target_include_directories(native_chat_probe PRIVATE src)
target_compile_definitions(native_chat_probe PRIVATE _CRT_SECURE_NO_WARNINGS)
target_link_libraries(native_chat_probe PRIVATE winhttp)
target_sources(native_chat_probe PRIVATE src/melee/gm/native_account_transport.cpp)

add_executable(native_connection_probe
    tools/native_connection_probe.c
    src/melee/gm/native_content_store.c
    src/melee/gm/native_fighter_package.c
    src/melee/gm/native_content_transfer.c
    src/melee/gm/native_lobby_match.c
    src/melee/gm/native_lobby_session.c
    src/melee/gm/native_lobby_topology.c
    src/melee/gm/native_netplay_exchange.c
    src/melee/gm/native_netplay_connection.c
    src/melee/gm/native_room_chat.c
    src/melee/gm/native_netplay_content_channel.c
    src/melee/gm/native_netplay_connection_wire.c
    src/melee/gm/native_netplay_ice.c
    src/melee/gm/native_netplay_input.c
    src/melee/gm/native_netplay_room.c
    src/melee/gm/native_netplay_room_content.c
    src/melee/gm/native_turn_credentials.c
    src/melee/gm/native_rollback_input.c
    src/melee/gm/native_signaling_client.c
    src/melee/gm/native_signaling_wire.c)
target_include_directories(native_connection_probe PRIVATE
    src extern/dolphin/include)
target_compile_definitions(native_connection_probe PRIVATE
    MELEE_NATIVE=1 MELEE_CONNECTION_PROBE=1 _CRT_SECURE_NO_WARNINGS)
target_link_options(native_connection_probe PRIVATE "LINKER:/opt:ref")
target_link_libraries(native_connection_probe PRIVATE
    melee_runtime aurora_main aurora_os aurora_gx aurora_vi aurora_pad
    aurora_mtx aurora_dvd aurora_card aurora_thp
    LibJuice::LibJuice winhttp bcrypt)

add_executable(native_content_channel_probe
    tools/native_content_channel_probe.c
    src/melee/gm/native_netplay_connection.c
    src/melee/gm/native_room_chat.c
    src/melee/gm/native_netplay_content_channel.c
    src/melee/gm/native_netplay_connection_wire.c
    src/melee/gm/native_netplay_ice.c
    src/melee/gm/native_signaling_client.c
    src/melee/gm/native_signaling_wire.c)
target_include_directories(native_content_channel_probe PRIVATE src extern/dolphin/include)
target_compile_definitions(native_content_channel_probe PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
target_link_libraries(native_content_channel_probe PRIVATE LibJuice::LibJuice winhttp bcrypt)
target_sources(native_content_channel_probe PRIVATE src/melee/gm/native_account_transport.cpp)

add_executable(melee_native_control tools/native_control.c
    src/melee/db/dbnativecontrol_protocol.c)
target_include_directories(melee_native_control PRIVATE src extern/dolphin/include)
target_compile_definitions(melee_native_control PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)

foreach(fixture IN ITEMS laser projectile_launch)
    if(fixture STREQUAL "laser")
        set(script_input "${CMAKE_SOURCE_DIR}/examples/fox_falco_laser.fms")
    else()
        set(script_input "${CMAKE_SOURCE_DIR}/tests/native/projectile_launch.fms")
    endif()
    set(script_output "${CMAKE_CURRENT_BINARY_DIR}/${fixture}.generated.c")
    add_custom_command(OUTPUT "${script_output}"
        COMMAND $<TARGET_FILE:native_script_reference_exporter>
            "${script_input}" "${fixture}_probe" "${script_output}"
        DEPENDS native_script_reference_exporter "${script_input}" VERBATIM)
    add_library(native_fighter_${fixture}_fixture MODULE "${script_output}")
    target_include_directories(native_fighter_${fixture}_fixture PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_fighter_${fixture}_fixture PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    set_target_properties(native_fighter_${fixture}_fixture PROPERTIES PREFIX "")
endforeach()
add_executable(native_fighter_laser_parity_test tests/native/laser_script_parity_test.c)
target_include_directories(native_fighter_laser_parity_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_fighter_laser_parity_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
target_link_libraries(native_fighter_laser_parity_test PRIVATE melee_runtime
    aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx aurora_dvd aurora_card aurora_thp)
target_link_options(native_fighter_laser_parity_test PRIVATE "LINKER:/errorlimit:0" "LINKER:/opt:ref")
aurora_copy_runtime_dlls(native_fighter_laser_parity_test)
add_test(NAME native_fighter_laser_parity COMMAND native_fighter_laser_parity_test
    "$<TARGET_FILE:native_fighter_laser_fixture>" "$<TARGET_FILE:native_fighter_projectile_launch_fixture>")

foreach(fixture IN ITEMS blaster_model article_signals)
    if(fixture STREQUAL "blaster_model")
        set(script_input "${CMAKE_SOURCE_DIR}/examples/fox_falco_blaster.fms")
    else()
        set(script_input "${CMAKE_SOURCE_DIR}/tests/native/article_signals.fms")
    endif()
    set(script_output "${CMAKE_CURRENT_BINARY_DIR}/${fixture}.generated.c")
    add_custom_command(OUTPUT "${script_output}"
        COMMAND $<TARGET_FILE:native_script_reference_exporter> "${script_input}" ${fixture}_probe "${script_output}"
        DEPENDS native_script_reference_exporter "${script_input}"
        VERBATIM)
    add_library(native_fighter_${fixture}_fixture MODULE "${script_output}")
    target_include_directories(native_fighter_${fixture}_fixture PRIVATE src extern/dolphin/include)
    target_compile_definitions(native_fighter_${fixture}_fixture PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
    set_target_properties(native_fighter_${fixture}_fixture PROPERTIES PREFIX "")
endforeach()
add_executable(native_fighter_blaster_model_parity_test tests/native/blaster_model_parity_test.c)
target_include_directories(native_fighter_blaster_model_parity_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_fighter_blaster_model_parity_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
target_link_libraries(native_fighter_blaster_model_parity_test PRIVATE melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx aurora_dvd aurora_card aurora_thp)
target_link_options(native_fighter_blaster_model_parity_test PRIVATE "LINKER:/errorlimit:0" "LINKER:/opt:ref")
aurora_copy_runtime_dlls(native_fighter_blaster_model_parity_test)
add_test(NAME native_fighter_blaster_model_parity COMMAND native_fighter_blaster_model_parity_test
    "$<TARGET_FILE:native_fighter_blaster_model_fixture>" "$<TARGET_FILE:native_fighter_article_signals_fixture>")

add_executable(native_fighter_article_links_test tests/native/article_links_test.c tools/native_fighter_item_links.c)
target_include_directories(native_fighter_article_links_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_fighter_article_links_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
target_link_libraries(native_fighter_article_links_test PRIVATE melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx aurora_dvd aurora_card aurora_thp)
target_link_options(native_fighter_article_links_test PRIVATE "LINKER:/errorlimit:0" "LINKER:/opt:ref")
aurora_copy_runtime_dlls(native_fighter_article_links_test)
add_test(NAME native_fighter_article_links COMMAND native_fighter_article_links_test)

# Native guard unions must retain the scalar slots proven by console word accesses.
add_executable(native_yoshi_guard_layout_test tests/native/yoshi_guard_layout_test.c)
target_include_directories(native_yoshi_guard_layout_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_yoshi_guard_layout_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_yoshi_guard_layout COMMAND native_yoshi_guard_layout_test)
add_test(NAME native_yoshi_guard_console_words COMMAND Python3::Interpreter
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/yoshi_guard_dol_test.py" --iso "${MELEE_ISO}")

foreach(policy_form action state)
    set(oracle_construction)
    if(policy_form STREQUAL "state")
        set(oracle_construction --construction ftDataTest 341 1)
    endif()
    set(policy_source "${CMAKE_SOURCE_DIR}/tests/native/guard_policy_fixture/${policy_form}.fms")
    set(policy_generated "${CMAKE_CURRENT_BINARY_DIR}/guard_policy_${policy_form}.generated.c")
    add_custom_command(OUTPUT "${policy_generated}"
        COMMAND $<TARGET_FILE:native_script_reference_exporter> "${policy_source}"
            guard_policy_probe "${policy_generated}" ${oracle_construction}
        DEPENDS native_script_reference_exporter "${policy_source}" VERBATIM)
    add_library(native_fighter_guard_policy_${policy_form}_fixture MODULE "${policy_generated}")
    target_include_directories(native_fighter_guard_policy_${policy_form}_fixture PRIVATE
        src extern/dolphin/include)
    target_compile_definitions(native_fighter_guard_policy_${policy_form}_fixture PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
    set_target_properties(native_fighter_guard_policy_${policy_form}_fixture PROPERTIES PREFIX "")
endforeach()
add_executable(native_fighter_guard_policy_test tests/native/fighter_guard_policy_test.c)
target_include_directories(native_fighter_guard_policy_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_fighter_guard_policy_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_fighter_guard_policy COMMAND native_fighter_guard_policy_test
    "$<TARGET_FILE:native_fighter_guard_policy_action_fixture>"
    "$<TARGET_FILE:native_fighter_guard_policy_state_fixture>")

set(ITEM_BEHAVIOR_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/item_behavior_fixture.h")
add_custom_command(OUTPUT "${ITEM_BEHAVIOR_FIXTURE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/item_behavior_fixture.py"
        "${CMAKE_CURRENT_SOURCE_DIR}" "${ITEM_BEHAVIOR_FIXTURE}"
    DEPENDS tests/native/item_behavior_fixture.py tests/native/callback_engine_fixture.py
        src/melee/it/item.c VERBATIM)
add_executable(native_item_behavior_test tests/native/item_behavior_test.c
    src/melee/it/native_item_behavior.c src/melee/ft/native_callback_lifetime.c
    tests/native/model_effect_absent_fixture.c "${ITEM_BEHAVIOR_FIXTURE}")
target_include_directories(native_item_behavior_test PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}" src extern/dolphin/include)
target_compile_definitions(native_item_behavior_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_item_behavior COMMAND native_item_behavior_test)

add_executable(native_article_creation_test tests/native/article_creation_test.c
    src/melee/it/native_article.c src/melee/ft/native_callback_lifetime.c
    tests/native/model_effect_absent_fixture.c)
target_include_directories(native_article_creation_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_article_creation_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
target_compile_options(native_article_creation_test PRIVATE -ffunction-sections -fdata-sections)
target_link_options(native_article_creation_test PRIVATE "LINKER:/opt:ref")
add_test(NAME native_article_creation COMMAND native_article_creation_test)

add_executable(native_article_refs_test tests/native/article_refs_test.c
    src/melee/it/native_article_refs.c src/melee/ft/native_callback_lifetime.c
    tests/native/model_effect_absent_fixture.c)
target_include_directories(native_article_refs_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_article_refs_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_article_refs COMMAND native_article_refs_test)

# Definition-backed resource fixtures were superseded by the source-owned
# frontend/kernel/history cohorts in tests/native/script_{resource_frontend,item_resources}.
add_executable(native_script_resource_metadata_test
    tests/native/script_resource_metadata/test.c
    src/melee/ft/native_script_source.c src/melee/ft/native_script_resource_metadata.c
    src/melee/ft/native_fighter_rig.c src/sysdolphin/baselib/dat.c)
target_include_directories(native_script_resource_metadata_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_resource_metadata_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_script_resource_metadata COMMAND native_script_resource_metadata_test)

set(ARTICLE_SIGNAL_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/article_signal_originals.h")
add_custom_command(OUTPUT "${ARTICLE_SIGNAL_FIXTURE}"
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/article_signal_source_test.py"
        --extract-only --source "${CMAKE_CURRENT_SOURCE_DIR}/src/melee/ft/native_fighter_module.c"
        --output "${ARTICLE_SIGNAL_FIXTURE}"
    DEPENDS tests/native/article_signal_source_test.py src/melee/ft/native_fighter_module.c VERBATIM)
add_executable(native_article_signal_test tests/native/article_signal_test.c
    src/melee/ft/native_callback_lifetime.c "${ARTICLE_SIGNAL_FIXTURE}")
target_include_directories(native_article_signal_test PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}" src extern/dolphin/include)
target_compile_definitions(native_article_signal_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_article_signal COMMAND native_article_signal_test)

set(COLLISION_MASK_FIXTURE_DIR "${CMAKE_CURRENT_BINARY_DIR}/collision_mask_fixture")
set(COLLISION_MASK_FIXTURES
    "${COLLISION_MASK_FIXTURE_DIR}/collision_mask_original.inc"
    "${COLLISION_MASK_FIXTURE_DIR}/collision_mask_candidate.inc")
add_custom_command(OUTPUT ${COLLISION_MASK_FIXTURES}
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/collision_mask_stock_compare.py"
        --root "${CMAKE_CURRENT_SOURCE_DIR}" --output "${COLLISION_MASK_FIXTURE_DIR}"
    DEPENDS tests/native/collision_mask_stock_compare.py
        src/melee/it/itgroundcoll.c src/melee/it/kinds/itpeachturnip.c VERBATIM)
add_executable(native_item_collision_masks_test tests/native/collision_mask_stock_test.c
    ${COLLISION_MASK_FIXTURES})
target_include_directories(native_item_collision_masks_test PRIVATE
    "${COLLISION_MASK_FIXTURE_DIR}" src extern/dolphin/include)
target_compile_definitions(native_item_collision_masks_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_item_collision_masks COMMAND native_item_collision_masks_test)

set(CPU_CALLBACK_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/cpu_callback_originals.h")
add_custom_command(OUTPUT "${CPU_CALLBACK_FIXTURE}"
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/cpu_callback_lifetime_source_test.py"
        --extract-only --source "${CMAKE_CURRENT_SOURCE_DIR}/src/melee/ft/ftcpuattack.c"
        --common "${CMAKE_CURRENT_SOURCE_DIR}/src/melee/ft/kinds/ftCommon/ftCo_0A01.c"
        --output "${CPU_CALLBACK_FIXTURE}"
    DEPENDS tests/native/cpu_callback_lifetime_source_test.py
        src/melee/ft/ftcpuattack.c src/melee/ft/kinds/ftCommon/ftCo_0A01.c VERBATIM)
add_executable(native_cpu_callback_lifetime_test tests/native/cpu_callback_lifetime_test.c
    src/melee/ft/native_callback_lifetime.c "${CPU_CALLBACK_FIXTURE}")
target_include_directories(native_cpu_callback_lifetime_test PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}" src extern/dolphin/include)
target_compile_definitions(native_cpu_callback_lifetime_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_cpu_callback_lifetime COMMAND native_cpu_callback_lifetime_test)

set(PEACH_RARE_SCALE_DIR "${CMAKE_CURRENT_BINARY_DIR}/peach_rare_scale_fixture")
set(PEACH_RARE_SCALE_INCLUDES
    "${PEACH_RARE_SCALE_DIR}/peach_rare_scale_original.inc"
    "${PEACH_RARE_SCALE_DIR}/peach_rare_scale_candidate.inc"
    "${PEACH_RARE_SCALE_DIR}/peach_rare_scale_legacy.inc")
add_custom_command(OUTPUT ${PEACH_RARE_SCALE_INCLUDES}
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/peach_rare_scale_stock_compare.py"
        --root "${CMAKE_CURRENT_SOURCE_DIR}" --output "${PEACH_RARE_SCALE_DIR}" --iso "${MELEE_ISO}"
    DEPENDS tests/native/peach_rare_scale_stock_compare.py
        src/melee/it/kinds/inlines.h src/melee/it/kinds/itpeachturnip.c
        src/melee/it/kinds/itbombhei.c src/melee/it/kinds/itdosei.c
        src/melee/it/itCommonItems.h src/melee/it/itCharItems.h
        src/melee/it/types.h src/melee/it/item.c src/melee/it/it_2725.c VERBATIM)
add_executable(native_peach_rare_scale_test tests/native/peach_rare_scale_test.c
    ${PEACH_RARE_SCALE_INCLUDES})
target_include_directories(native_peach_rare_scale_test PRIVATE
    "${PEACH_RARE_SCALE_DIR}" src extern/dolphin/include)
target_compile_definitions(native_peach_rare_scale_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_peach_rare_scale COMMAND native_peach_rare_scale_test)

add_executable(native_callback_lifetime_test tests/native/callback_lifetime_test.c
    src/melee/ft/native_callback_lifetime.c tests/native/model_effect_absent_fixture.c)
target_include_directories(native_callback_lifetime_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_callback_lifetime_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_callback_lifetime COMMAND native_callback_lifetime_test)

add_executable(native_model_effect_lifetime_test tests/native/model_effect_lifetime_test.c
    src/melee/ef/native_model_effect.c src/melee/ft/native_callback_lifetime.c)
target_include_directories(native_model_effect_lifetime_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_model_effect_lifetime_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_model_effect_lifetime COMMAND native_model_effect_lifetime_test)

set(MODEL_EFFECT_GROUP_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/model_effect_group_fixture.h")
add_custom_command(OUTPUT "${MODEL_EFFECT_GROUP_FIXTURE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/model_effect_group_fixture.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/src" "${MODEL_EFFECT_GROUP_FIXTURE}"
    DEPENDS tests/native/model_effect_group_fixture.py src/melee/ef/efsync.c
        src/melee/ef/efasync.c src/melee/ef/eflib.c VERBATIM)
add_executable(native_model_effect_group_test tests/native/model_effect_group_test.c
    src/melee/ef/native_model_effect.c src/melee/ft/native_callback_lifetime.c
    "${MODEL_EFFECT_GROUP_FIXTURE}")
target_include_directories(native_model_effect_group_test PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}" src extern/dolphin/include)
target_compile_definitions(native_model_effect_group_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_test(NAME native_model_effect_group COMMAND native_model_effect_group_test)

set(CALLBACK_ENGINE_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/callback_engine_fixture.h")
add_custom_command(OUTPUT "${CALLBACK_ENGINE_FIXTURE}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/callback_engine_fixture.py"
        "${CMAKE_CURRENT_SOURCE_DIR}" "${CALLBACK_ENGINE_FIXTURE}"
    DEPENDS tests/native/callback_engine_fixture.py src/melee/it/item.c src/melee/it/it_2725.c src/melee/it/inlines.h
        src/melee/ft/fighter.c src/melee/ft/fighter_physics.c src/melee/ft/ftcommon.c VERBATIM)
add_executable(native_callback_engine_lifetime_test tests/native/callback_engine_lifetime_test.c
    src/melee/ft/native_callback_lifetime.c tests/native/model_effect_absent_fixture.c
    src/melee/gm/native_match_pool.c
    "${CALLBACK_ENGINE_FIXTURE}")
target_include_directories(native_callback_engine_lifetime_test PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}" src extern/dolphin/include)
target_compile_definitions(native_callback_engine_lifetime_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_callback_engine_lifetime COMMAND native_callback_engine_lifetime_test)

add_executable(native_fighter_events_test tests/native/fighter_events_test.c
    src/melee/ft/native_fighter_events.c src/melee/ft/native_callback_lifetime.c
    tests/native/model_effect_absent_fixture.c)
target_include_directories(native_fighter_events_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_fighter_events_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_fighter_events COMMAND native_fighter_events_test)

add_executable(native_fighter_event_packages_test tests/native/fighter_event_packages_test.c)
target_include_directories(native_fighter_event_packages_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_fighter_event_packages_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
target_link_libraries(native_fighter_event_packages_test PRIVATE melee_runtime aurora_os aurora_gx
    aurora_vi aurora_pad aurora_mtx aurora_dvd aurora_card aurora_thp)
add_test(NAME native_fighter_event_packages COMMAND
    "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/fighter_event_packages.py"
    "$<TARGET_FILE:native_content_tool>" "$<TARGET_FILE:native_fighter_event_packages_test>"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Super Smash Bros. Melee (USA) (v1.02).iso")

add_executable(native_script_callback_exports_compiler_test
    tests/native/script_callback_exports_compiler_test.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_callback_exports_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_callback_exports_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_script_callback_exports_compiler COMMAND native_script_callback_exports_compiler_test)
add_executable(native_script_callback_slots_compiler_test
    tests/native/script_callback_slots_compiler_test.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_callback_slots_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_callback_slots_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_script_callback_slots_compiler COMMAND native_script_callback_slots_compiler_test)
set(CALLBACK_EXPORTS_ACTION_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/script_callback_exports_action_fixture.h")
set(CALLBACK_EXPORTS_CALLBACK_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/script_callback_exports_callback_fixture.h")
add_custom_command(OUTPUT "${CALLBACK_EXPORTS_ACTION_FIXTURE}" "${CALLBACK_EXPORTS_CALLBACK_FIXTURE}"
    COMMAND $<TARGET_FILE:native_script_callback_exports_compiler_test>
        "${CALLBACK_EXPORTS_ACTION_FIXTURE}" "${CALLBACK_EXPORTS_CALLBACK_FIXTURE}"
    DEPENDS native_script_callback_exports_compiler_test VERBATIM)
foreach(CALLBACK_EXPORTS_MODE IN ITEMS action callback)
    set(CALLBACK_EXPORTS_CALLBACKS 0)
    if(CALLBACK_EXPORTS_MODE STREQUAL "callback")
        set(CALLBACK_EXPORTS_CALLBACKS 1)
    endif()
    add_executable(native_script_callback_exports_${CALLBACK_EXPORTS_MODE}_test
        tests/native/script_callback_exports_module_test.c
        "${CALLBACK_EXPORTS_ACTION_FIXTURE}" "${CALLBACK_EXPORTS_CALLBACK_FIXTURE}")
    target_include_directories(native_script_callback_exports_${CALLBACK_EXPORTS_MODE}_test PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}" src extern/dolphin/include)
    target_compile_definitions(native_script_callback_exports_${CALLBACK_EXPORTS_MODE}_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS CALLBACK_EXPORTS_CALLBACKS=${CALLBACK_EXPORTS_CALLBACKS})
    add_test(NAME native_script_callback_exports_${CALLBACK_EXPORTS_MODE}
        COMMAND native_script_callback_exports_${CALLBACK_EXPORTS_MODE}_test)
endforeach()

# Generic item ownership and article import contracts are exercised without
# launching the game or creating a renderer/window.
include(cmake/NativeItemScriptTests.cmake)
include(cmake/NativeSourcePackageTests.cmake)
include(cmake/NativeArticleHelperReturnsTests.cmake)
include(cmake/NativeFighterCapsulePositionsTests.cmake)
include(cmake/NativeFighterPoseTests.cmake)

add_executable(native_script_callback_lifetime_compiler_test
    tests/native/script_callback_lifetime_compiler_test.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_callback_lifetime_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_callback_lifetime_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_script_callback_lifetime_compiler COMMAND native_script_callback_lifetime_compiler_test)
set(CALLBACK_LIFETIME_ACTION_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/script_callback_lifetime_action_fixture.h")
set(CALLBACK_LIFETIME_CALLBACK_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/script_callback_lifetime_callback_fixture.h")
add_custom_command(OUTPUT "${CALLBACK_LIFETIME_ACTION_FIXTURE}" "${CALLBACK_LIFETIME_CALLBACK_FIXTURE}"
    COMMAND $<TARGET_FILE:native_script_callback_lifetime_compiler_test>
        "${CALLBACK_LIFETIME_ACTION_FIXTURE}" "${CALLBACK_LIFETIME_CALLBACK_FIXTURE}"
    DEPENDS native_script_callback_lifetime_compiler_test VERBATIM)
foreach(CALLBACK_LIFETIME_MODE IN ITEMS action callback)
    set(CALLBACK_LIFETIME_CALLBACKS 0)
    if(CALLBACK_LIFETIME_MODE STREQUAL "callback")
        set(CALLBACK_LIFETIME_CALLBACKS 1)
    endif()
    add_executable(native_script_callback_lifetime_${CALLBACK_LIFETIME_MODE}_test
        tests/native/script_callback_lifetime_module_test.c
        "${CALLBACK_LIFETIME_ACTION_FIXTURE}" "${CALLBACK_LIFETIME_CALLBACK_FIXTURE}")
    target_include_directories(native_script_callback_lifetime_${CALLBACK_LIFETIME_MODE}_test PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}" src extern/dolphin/include)
    target_compile_definitions(native_script_callback_lifetime_${CALLBACK_LIFETIME_MODE}_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS CALLBACK_LIFETIME_CALLBACKS=${CALLBACK_LIFETIME_CALLBACKS})
    add_test(NAME native_script_callback_lifetime_${CALLBACK_LIFETIME_MODE}
        COMMAND native_script_callback_lifetime_${CALLBACK_LIFETIME_MODE}_test)
endforeach()

add_executable(native_script_article_refs_test tests/native/script_article_refs_test.c
    tools/native_fighter_script_article_refs.c tools/native_fighter_script_state.c)
target_include_directories(native_script_article_refs_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_article_refs_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_script_article_refs COMMAND native_script_article_refs_test)

add_executable(native_script_article_refs_compiler_test
    tests/native/script_article_refs_compiler_test.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_article_refs_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_article_refs_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_script_article_refs_compiler COMMAND native_script_article_refs_compiler_test)
set(ARTICLE_REFS_ACTION_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/script_article_refs_action_fixture.h")
set(ARTICLE_REFS_CALLBACK_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/script_article_refs_callback_fixture.h")
add_custom_command(OUTPUT "${ARTICLE_REFS_ACTION_FIXTURE}" "${ARTICLE_REFS_CALLBACK_FIXTURE}"
    COMMAND $<TARGET_FILE:native_script_article_refs_compiler_test>
        "${ARTICLE_REFS_ACTION_FIXTURE}" "${ARTICLE_REFS_CALLBACK_FIXTURE}"
    DEPENDS native_script_article_refs_compiler_test VERBATIM)
foreach(ARTICLE_REFS_MODE IN ITEMS action callback)
    if(ARTICLE_REFS_MODE STREQUAL "callback")
        set(ARTICLE_REFS_CALLBACKS 1)
    else()
        set(ARTICLE_REFS_CALLBACKS 0)
    endif()
    add_executable(native_script_article_refs_${ARTICLE_REFS_MODE}_test
        tests/native/script_article_refs_module_test.c
        "${ARTICLE_REFS_ACTION_FIXTURE}" "${ARTICLE_REFS_CALLBACK_FIXTURE}")
    target_include_directories(native_script_article_refs_${ARTICLE_REFS_MODE}_test PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}" src extern/dolphin/include)
    target_compile_definitions(native_script_article_refs_${ARTICLE_REFS_MODE}_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS ARTICLE_REFS_CALLBACKS=${ARTICLE_REFS_CALLBACKS})
    add_test(NAME native_script_article_refs_${ARTICLE_REFS_MODE}
        COMMAND native_script_article_refs_${ARTICLE_REFS_MODE}_test)
endforeach()

add_executable(native_fighter_damage_test tests/native/fighter_damage_test.c
    src/melee/ft/native_fighter_damage.c)
target_include_directories(native_fighter_damage_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_fighter_damage_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_fighter_damage COMMAND native_fighter_damage_test)

add_executable(native_ness_magnet_primitives_test tests/native/ness_magnet_primitives_test.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_ness_magnet_primitives_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_ness_magnet_primitives_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_test(NAME native_ness_magnet_primitives COMMAND native_ness_magnet_primitives_test)

add_executable(native_fighter_state_layout_test tests/native/fighter_state_layout_test.c
    src/melee/ft/native_fighter_state.c src/melee/gm/native_rollback_pages.c)
target_include_directories(native_fighter_state_layout_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_fighter_state_layout_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_fighter_state_layout COMMAND native_fighter_state_layout_test)

add_executable(native_script_state_array_test tests/native/script_state_array_test.c
    tools/native_fighter_script_state.c)
target_include_directories(native_script_state_array_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_state_array_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_script_state_array COMMAND native_script_state_array_test)
set(STATE_ARRAY_ACCESS_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/script_state_array_access_fixture.h")
add_custom_command(OUTPUT "${STATE_ARRAY_ACCESS_FIXTURE}"
    COMMAND $<TARGET_FILE:native_script_state_array_test> "${STATE_ARRAY_ACCESS_FIXTURE}"
    DEPENDS native_script_state_array_test VERBATIM)
add_executable(native_script_state_array_access_test tests/native/script_state_array_access_test.c
    "${STATE_ARRAY_ACCESS_FIXTURE}")
target_include_directories(native_script_state_array_access_test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
add_test(NAME native_script_state_array_access COMMAND native_script_state_array_access_test)

add_executable(native_script_state_compiler_test tests/native/script_state_compiler_test.c
    ${NATIVE_FIGHTER_SCRIPT_SOURCES} src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_state_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_state_compiler_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_test(NAME native_script_state_compiler COMMAND native_script_state_compiler_test)
set(STATE_ARRAY_ACTION_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/script_state_action_fixture.h")
set(STATE_ARRAY_CALLBACK_FIXTURE "${CMAKE_CURRENT_BINARY_DIR}/script_state_callback_fixture.h")
add_custom_command(OUTPUT "${STATE_ARRAY_ACTION_FIXTURE}" "${STATE_ARRAY_CALLBACK_FIXTURE}"
    COMMAND $<TARGET_FILE:native_script_state_compiler_test>
        "${STATE_ARRAY_ACTION_FIXTURE}" "${STATE_ARRAY_CALLBACK_FIXTURE}"
    DEPENDS native_script_state_compiler_test VERBATIM)
foreach(state_form action callback)
    set(state_callbacks 0)
    if(state_form STREQUAL "callback")
        set(state_callbacks 1)
    endif()
    add_executable(native_script_state_${state_form}_module_test tests/native/script_state_module_test.c
        "${STATE_ARRAY_ACTION_FIXTURE}" "${STATE_ARRAY_CALLBACK_FIXTURE}")
    target_include_directories(native_script_state_${state_form}_module_test PRIVATE
        src extern/dolphin/include "${CMAKE_CURRENT_BINARY_DIR}")
    target_compile_definitions(native_script_state_${state_form}_module_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS STATE_ARRAY_CALLBACKS=${state_callbacks})
    add_test(NAME native_script_state_${state_form}_module
        COMMAND native_script_state_${state_form}_module_test)
endforeach()

add_executable(native_script_presentation_compiler_test
    tests/native/presentation_controls/compiler.c ${NATIVE_FIGHTER_SCRIPT_SOURCES}
    src/melee/ft/native_fighter_parameters.c)
target_include_directories(native_script_presentation_compiler_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_script_presentation_compiler_test PRIVATE MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS)
add_test(NAME native_script_presentation_compiler COMMAND native_script_presentation_compiler_test)
foreach(presentation_form action state)
    set(oracle_construction)
    if(presentation_form STREQUAL "state")
        set(oracle_construction --construction ftDataNess 341 1)
    endif()
    set(presentation_source "${CMAKE_SOURCE_DIR}/tests/native/presentation_controls/${presentation_form}.fms")
    set(presentation_dir "${CMAKE_CURRENT_BINARY_DIR}/presentation_${presentation_form}")
    set(presentation_generated "${presentation_dir}/presentation_fixture.h")
    add_custom_command(OUTPUT "${presentation_generated}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${presentation_dir}"
        COMMAND $<TARGET_FILE:native_script_reference_exporter> "${presentation_source}"
            presentation_probe "${presentation_generated}" ${oracle_construction}
        DEPENDS native_script_reference_exporter "${presentation_source}" VERBATIM)
    add_executable(native_script_presentation_${presentation_form}_test
        tests/native/presentation_controls/runtime.c "${presentation_generated}")
    target_include_directories(native_script_presentation_${presentation_form}_test PRIVATE
        "${presentation_dir}" src extern/dolphin/include)
    if(presentation_form STREQUAL "state")
        set(presentation_state 1)
    else()
        set(presentation_state 0)
    endif()
    target_compile_definitions(native_script_presentation_${presentation_form}_test PRIVATE
        MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS PRESENTATION_STATE=${presentation_state})
    add_test(NAME native_script_presentation_${presentation_form} COMMAND native_script_presentation_${presentation_form}_test)
endforeach()

# Typed fighter visibility import and the actual load-context publication path.
set(FIGHTER_VISIBILITY_DIR "${CMAKE_CURRENT_BINARY_DIR}/fighter_visibility_assets")
set(FIGHTER_VISIBILITY_FIXTURE "${FIGHTER_VISIBILITY_DIR}/visibility_actual.inc")
add_custom_command(OUTPUT "${FIGHTER_VISIBILITY_FIXTURE}"
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/fighter_visibility_assets/fixture.py"
        --root "${CMAKE_CURRENT_SOURCE_DIR}" --output "${FIGHTER_VISIBILITY_DIR}"
    DEPENDS tests/native/fighter_visibility_assets/fixture.py
        tests/native/item_owner_fixture.py src/melee/ft/ftcharacter_assets.c
        src/melee/ft/ftcharacter_assets.h src/melee/ft/ftcharacter_asset_layout.h
        src/melee/ft/native_fighter_runtime.c src/melee/ft/native_fighter_runtime.h
        src/melee/ft/native_callback_lifetime.c src/melee/ft/native_callback_lifetime.h
        src/melee/ft/fighter.c
    VERBATIM)
add_executable(native_fighter_visibility_runtime_test
    tests/native/fighter_visibility_assets/runtime_test.c "${FIGHTER_VISIBILITY_FIXTURE}"
    src/melee/ft/native_callback_lifetime.c
    src/sysdolphin/baselib/dat.c src/sysdolphin/baselib/dat_import.c
    src/sysdolphin/baselib/dat_model.c src/sysdolphin/baselib/dat_geometry.c
    src/sysdolphin/baselib/dat_shape_skin.c
    src/sysdolphin/baselib/dat_material.c src/sysdolphin/baselib/dat_animation.c
    src/sysdolphin/baselib/dat_scene.c src/sysdolphin/baselib/dat_texture.c)
target_include_directories(native_fighter_visibility_runtime_test PRIVATE
    "${FIGHTER_VISIBILITY_DIR}" src extern/dolphin/include)
target_compile_definitions(native_fighter_visibility_runtime_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
add_test(NAME native_fighter_visibility_runtime COMMAND native_fighter_visibility_runtime_test)

add_executable(native_fighter_visibility_import_test
    tests/native/fighter_visibility_assets/import_test.c)
target_include_directories(native_fighter_visibility_import_test PRIVATE src extern/dolphin/include)
target_compile_definitions(native_fighter_visibility_import_test PRIVATE
    MELEE_NATIVE=1 _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
target_link_libraries(native_fighter_visibility_import_test PRIVATE
    melee_runtime aurora_os aurora_gx aurora_vi aurora_pad aurora_mtx
    aurora_dvd aurora_card aurora_thp)
target_link_options(native_fighter_visibility_import_test PRIVATE "LINKER:/opt:ref")
aurora_copy_runtime_dlls(native_fighter_visibility_import_test)
add_test(NAME native_fighter_visibility_import COMMAND
    native_fighter_visibility_import_test "${MELEE_ISO}")

include(cmake/NativePoseScriptTests.cmake)
