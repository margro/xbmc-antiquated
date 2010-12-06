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

#include "VideoInfoTag.h"
#include "DateTime.h"
#include "FileItem.h"
#include "../addons/include/xbmc_pvr_types.h"

class CPVRChannelGroup;

class CPVRChannelGroups : public std::vector<CPVRChannelGroup>
{
private:
  bool  m_bRadio;

public:
  CPVRChannelGroups(void);

  bool Load(bool radio = false);
  void Unload();

  int GetGroupList(CFileItemList* results);
  int GetFirstChannelForGroupID(int GroupId);
  int GetPrevGroupID(int current_group_id);
  int GetNextGroupID(int current_group_id);

  void AddGroup(const CStdString &name);
  bool RenameGroup(unsigned int GroupId, const CStdString &newname);
  bool DeleteGroup(unsigned int GroupId);
  bool ChannelToGroup(const CPVRChannel &channel, unsigned int GroupId);
  CStdString GetGroupName(int GroupId);
  int GetGroupId(CStdString GroupName);
  void Clear();
};

extern CPVRChannelGroups PVRChannelGroupsTV;
extern CPVRChannelGroups PVRChannelGroupsRadio;
