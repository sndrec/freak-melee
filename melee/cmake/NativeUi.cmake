set(_native_template_names
    mario_full drmario_full captain_wip ganon_wip luigi_wip purin_wip
    fox_wip falco_wip marth_wip roy_wip donkey_wip koopa_wip pikachu_wip
    pichu_wip samus_full mewtwo_wip link_full young_link_full peach_full
    gamewatch_full zelda_full sheik_full popo_full nana_full yoshi_wip ness_wip)
set(_native_template_sources)
set(_native_release_ui)
foreach(_name IN LISTS _native_template_names)
    list(APPEND _native_template_sources "${CMAKE_CURRENT_SOURCE_DIR}/examples/${_name}.fms")
    list(APPEND _native_release_ui "ui/${_name}.fms")
endforeach()
file(GLOB_RECURSE _native_ui_assets CONFIGURE_DEPENDS RELATIVE
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui" "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/*")
file(GLOB_RECURSE _native_script_docs CONFIGURE_DEPENDS RELATIVE
    "${CMAKE_CURRENT_SOURCE_DIR}/docs/scripting" "${CMAKE_CURRENT_SOURCE_DIR}/docs/scripting/*")
foreach(_path IN LISTS _native_ui_assets)
    list(APPEND _native_release_ui "ui/${_path}")
endforeach()
foreach(_path IN LISTS _native_script_docs)
    list(APPEND _native_release_ui "ui/docs/${_path}")
endforeach()
foreach(_name IN ITEMS menu-heading result-name result-winner notice css-tile menu-option menu-caption)
    list(APPEND _native_release_ui "ui/fonts/${_name}.bin")
endforeach()
foreach(_kind RANGE 0 26)
    list(APPEND _native_release_ui "ui/portraits/stock-${_kind}.png")
endforeach()
list(APPEND _native_release_ui "ui/skins/menu-panel.bin")
list(REMOVE_DUPLICATES _native_release_ui)
list(SORT _native_release_ui)
string(JOIN "\n" _native_release_ui_text ${_native_release_ui})
file(GENERATE OUTPUT "$<TARGET_FILE_DIR:melee_native>/native_release_ui.txt"
    CONTENT "${_native_release_ui_text}\n")

add_custom_target(melee_native_ui
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/tools/prepare_menu_fonts.py"
        --iso "${MELEE_ISO}" --output "$<TARGET_FILE_DIR:melee_native>/ui/fonts"
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/tools/prepare_menu_art.py"
        --iso "${MELEE_ISO}" --output "$<TARGET_FILE_DIR:melee_native>/ui/portraits"
    COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/tools/prepare_menu_skin.py"
        --iso "${MELEE_ISO}" --output "$<TARGET_FILE_DIR:melee_native>/ui/skins"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/scripting" "$<TARGET_FILE_DIR:melee_native>/ui/docs"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui" "$<TARGET_FILE_DIR:melee_native>/ui"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        ${_native_template_sources} "$<TARGET_FILE_DIR:melee_native>/ui"
    VERBATIM)
add_dependencies(melee_native melee_native_ui)

add_test(NAME native_release_payload COMMAND Python3::Interpreter
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/native/release_payload_test.py")
