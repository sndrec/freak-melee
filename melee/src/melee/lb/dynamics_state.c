#include "dynamics_state_internal.h"

// Actual simulation payload. The page oracle still covers this owner until
// candidate-wide graph closure and aggregate restoration are verified.
NativeDynamicsState dynamics_state;
