// Compat shim for Native File Dialog Extended (the recomp fetches real NFD via CMake).
// The installer wizard uses it to pick game/update/DLC source files. The lib declares
// the API; the default impl returns NFD_CANCEL (no picker) — a host links real NFD, or
// drives the install from known paths. Matches NFD-ext's N (wide on Win32) variants.
#pragma once

#include <cstddef>

#ifdef _WIN32
typedef wchar_t nfdnchar_t;
#else
typedef char nfdnchar_t;
#endif

typedef enum { NFD_ERROR = 0, NFD_OKAY = 1, NFD_CANCEL = 2 } nfdresult_t;
typedef void          nfdpathset_t;
typedef unsigned int  nfdpathsetsize_t;
typedef unsigned int  nfdfiltersize_t;
typedef struct { const nfdnchar_t* name; const nfdnchar_t* spec; } nfdnfilteritem_t;

inline nfdresult_t NFD_Init(void) { return NFD_OKAY; }
inline void        NFD_Quit(void) {}
inline nfdresult_t NFD_OpenDialogMultipleN(const nfdpathset_t** outPaths, const nfdnfilteritem_t* filterList, nfdfiltersize_t filterCount, const nfdnchar_t* defaultPath) { (void)outPaths; (void)filterList; (void)filterCount; (void)defaultPath; return NFD_CANCEL; }
inline nfdresult_t NFD_PickFolderMultipleN(const nfdpathset_t** outPaths, const nfdnchar_t* defaultPath) { (void)outPaths; (void)defaultPath; return NFD_CANCEL; }
inline nfdresult_t NFD_PathSet_GetCount(const nfdpathset_t* pathSet, nfdpathsetsize_t* count) { (void)pathSet; if (count) *count = 0; return NFD_OKAY; }
inline nfdresult_t NFD_PathSet_GetPathN(const nfdpathset_t* pathSet, nfdpathsetsize_t index, nfdnchar_t** outPath) { (void)pathSet; (void)index; if (outPath) *outPath = nullptr; return NFD_OKAY; }
inline void        NFD_PathSet_FreePathN(const nfdnchar_t* filePath) { (void)filePath; }
inline void        NFD_PathSet_Free(const nfdpathset_t* pathSet) { (void)pathSet; }
inline const char* NFD_GetError(void) { return ""; }
