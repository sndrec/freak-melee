# Optimize helper calls across game/HSD translation units in normal native
# builds. Keep debug builds straightforward and retain the existing strict
# floating-point options; IPO does not enable fast math or FMA contraction.
include(CheckIPOSupported)
check_ipo_supported(RESULT MELEE_IPO_SUPPORTED OUTPUT MELEE_IPO_DIAGNOSTIC
    LANGUAGES C CXX)
if(NOT MELEE_IPO_SUPPORTED)
    message(FATAL_ERROR "Native link-time optimization is unavailable: ${MELEE_IPO_DIAGNOSTIC}")
endif()
set_property(TARGET melee_game melee_runtime melee_native
    PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
set_property(TARGET melee_game melee_runtime melee_native
    PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
