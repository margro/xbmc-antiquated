#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "AddonHelpers_local.h"

class cHelper_libXBMC_addon
{
public:
  cHelper_libXBMC_addon();
  ~cHelper_libXBMC_addon();

  bool RegisterMe(void *Handle);
  void Log(const addon_log_t loglevel, const char *format, ... );
  bool GetSetting(std::string settingName, void *settingValue);
  void QueueNotification(const queue_msg_t type, const char *format, ... );
  void UnknownToUTF8(std::string &str);

private:
  AddonCB      *m_Handle;
  CB_AddOnLib  *m_cb;
};
