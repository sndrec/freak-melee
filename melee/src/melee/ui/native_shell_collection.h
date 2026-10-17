#pragma once

#ifdef __cplusplus
namespace Rml {
class ElementDocument;
}
void nativeShellCollection_Init(Rml::ElementDocument* document);
void nativeShellCollection_Open();
void nativeShellCollection_Update();
void nativeShellCollection_Close();
bool nativeShellCollection_IsOpen();
#endif
