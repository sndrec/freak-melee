#pragma once
#ifdef MELEE_NATIVE
namespace Rml { class ElementDocument; }
void nativeWorkshopDocs_Init(Rml::ElementDocument* document);
bool nativeWorkshopDocs_Close();
#endif
