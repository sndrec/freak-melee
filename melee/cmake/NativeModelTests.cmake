add_executable(native_skin_weights_test tests/native/skin_weights_test.c
    src/melee/ft/native_fighter_skin_weights.c)
target_include_directories(native_skin_weights_test PRIVATE src)
add_test(NAME native_skin_weights COMMAND native_skin_weights_test)
