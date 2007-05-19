
#pragma once

#include "LibraryLoader.h"
#include "DllLoader.h"

#ifndef _LINUX
#ifndef _WINDEF_
typedef unsigned long HMODULE;
#endif // _WINDEF_
#endif

class DllLoaderContainer
{
public:  
  static void       Clear();
  static HMODULE    GetModuleAddress(const char* sName);
  static int        GetNrOfModules();
  static LibraryLoader* GetModule(int iPos);
  static LibraryLoader* GetModule(const char* sName);
  static LibraryLoader* GetModule(HMODULE hModule);  
  static LibraryLoader* LoadModule(const char* sName, const char* sCurrentDir=NULL, bool bLoadSymbols=false);
  static void       ReleaseModule(LibraryLoader*& pDll);

  static void RegisterDll(LibraryLoader* pDll);
  static void UnRegisterDll(LibraryLoader* pDll);
  static void UnloadPythonDlls();

private:
  static LibraryLoader* FindModule(const char* sName, const char* sCurrentDir, bool bLoadSymbols);
  static LibraryLoader* LoadDll(const char* sName, bool bLoadSymbols);
  static bool       IsSystemDll(const char* sName);

  static LibraryLoader* m_dlls[64];
  static int m_iNrOfDlls;
  static bool m_bTrack;
};
