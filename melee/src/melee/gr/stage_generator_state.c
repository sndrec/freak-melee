#include "stage_generator_state.h"

// Actual singleton selected by the original shared generator context. No heap
// mirror; source item callbacks and the driver use these records directly.
StageGeneratorState stage_generator_state;
