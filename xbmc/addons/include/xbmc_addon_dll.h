#ifndef __XBMC_ADDON_H__
#define __XBMC_ADDON_H__

#ifdef _WIN32
#include <windows.h>
#else
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __declspec
#define __declspec(X)
#endif
#endif

extern "C"
{ 

  typedef void* ADDON_HANDLE;

  enum ADDON_STATUS
  {
    STATUS_OK,
    STATUS_LOST_CONNECTION,
    STATUS_NEED_RESTART,
    STATUS_NEED_SETTINGS,
    STATUS_UNKNOWN
  };

  typedef enum addon_log {
    LOG_DEBUG,
    LOG_INFO,
    LOG_NOTICE,
    LOG_ERROR
  } addon_log_t;

    typedef struct addon_string_list {
    const char**    Strings;
    int             Items;
  } addon_string_list_t;

  typedef struct
  {
    int           type;
    char*         id;
    char*         label;
    int           current;
    char**        entry;
    unsigned int  entry_elements;
  } StructSetting;

  ADDON_STATUS __declspec(dllexport) Create(void *callbacks, void* props);
  void __declspec(dllexport) Destroy();
  ADDON_STATUS __declspec(dllexport) GetStatus();
  bool __declspec(dllexport) HasSettings();
  unsigned int __declspec(dllexport) GetSettings(StructSetting ***sSet);
  ADDON_STATUS __declspec(dllexport) SetSetting(const char *settingName, const void *settingValue);
  void __declspec(dllexport) Remove();
  void __declspec(dllexport) FreeSettings();
};

#endif