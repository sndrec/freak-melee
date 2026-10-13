// These unselected self-operation leaves link the complete production Item
// binding. They abort if reached; this fixture accepts only accessory tracking.
bool it_802751D8(Item_GObj* o){(void)o;abort();}
bool it_80273030(Item_GObj* o){(void)o;abort();}
bool itColl_BounceOffShield(Item_GObj* o){(void)o;abort();}
bool it_8027781C(Item_GObj* o){(void)o;abort();}
void it_80272860(Item_GObj* o,float a,float b){(void)o;(void)a;(void)b;abort();}
void it_8027321C(Item_GObj* o){(void)o;abort();}
void it_8026B3A8(Item_GObj* o){(void)o;abort();}
void lbVector_Mirror(Vec3* v,Vec3* n){(void)v;(void)n;abort();}
void Item_8026AE84(Item* i,int sound,u8 volume,u8 pan){(void)i;(void)sound;(void)volume;(void)pan;abort();}
void Item_8026AF0C(Item* i,int sound,u8 volume,u8 pan){(void)i;(void)sound;(void)volume;(void)pan;abort();}
int it_80274658(Item_GObj* o,float x){(void)o;(void)x;abort();}
void it_8026B724(Item_GObj* o){(void)o;abort();}
void it_8026B73C(Item_GObj* o){(void)o;abort();}

// Observe only the accessory scheduler tail, independently of legitimate
// nested destruction work counted by the shared substrate's scheduler_tail.
void it_80271A58(Item_GObj* o){(void)o;++accessory_tail_calls;}
void it_80272280(Item_GObj* o){(void)o;++accessory_tail_calls;}
void it_802722B0(Item_GObj* o){(void)o;++accessory_tail_calls;}
