#include <melee/ft/ftanim.h>
#include <melee/it/it_266F.h>
#include <stdlib.h>

// Unselected setup presentation/held-item leaves must never run here.
bool itIsHeavy(Item_GObj* object){(void)object;abort();}
int itGetHoldKind(Item_GObj* object){(void)object;abort();}
void ftAnim_80070FB4(Fighter_GObj* object,s32 selector,s32 value)
{(void)object;(void)selector;(void)value;abort();}
void ftAnim_80070C48(Fighter_GObj* object,s32 selector)
{(void)object;(void)selector;abort();}
void ftAnim_80070CC4(Fighter_GObj* object,int selector)
{(void)object;(void)selector;abort();}
void ftAnim_800704F0(Fighter_GObj* object,int selector,float value)
{(void)object;(void)selector;(void)value;abort();}
