#include <melee/ft/ftlib.h>
#include <stdlib.h>

// Controller color queries share the production target dispatcher, but this
// fixture never requests them. An accidental call is a hard failure.
bool ftLib_800870BC(Fighter_GObj* object, GXColor* color)
{
    (void)object;
    (void)color;
    abort();
}
