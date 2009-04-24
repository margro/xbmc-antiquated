/*
 *      Copyright (C) 2005-2008 Team XBMC
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

/*
 * DESCRIPTION:
 *
 */

#include "stdafx.h"
#include "TVEPGInfoTag.h"
#include "TVChannelInfoTag.h"
#include "GUISettings.h"

/**
 * Create a blank unmodified channel tag
 */
CTVChannelInfoTag::CTVChannelInfoTag()
{
  Reset();
}

bool CTVChannelInfoTag::operator==(const CTVChannelInfoTag& right) const
{
  if (this == &right) return true;

  return (m_iIdChannel            == right.m_iIdChannel &&
          m_iChannelNum           == right.m_iChannelNum &&
          m_iClientNum            == right.m_iClientNum &&
          m_strChannel            == right.m_strChannel &&
          m_IconPath              == right.m_IconPath &&
          m_encrypted             == right.m_encrypted &&
          m_radio                 == right.m_radio &&
          m_hide                  == right.m_hide &&
          m_isRecording           == right.m_isRecording &&
          m_strFileNameAndPath    == right.m_strFileNameAndPath);
}

bool CTVChannelInfoTag::operator!=(const CTVChannelInfoTag &right) const
{
  if (m_iIdChannel            != right.m_iIdChannel) return true;
  if (m_iChannelNum           != right.m_iChannelNum) return true;
  if (m_iClientNum            != right.m_iClientNum) return true;
  if (m_strChannel            != right.m_strChannel) return true;
  if (m_IconPath              != right.m_IconPath) return true;
  if (m_encrypted             != right.m_encrypted) return true;
  if (m_radio                 != right.m_radio) return true;
  if (m_hide                  != right.m_hide) return true;
  if (m_isRecording           != right.m_isRecording) return true;
  if (m_strFileNameAndPath    != right.m_strFileNameAndPath) return true;

  return false;
}


/**
 * Initialize blank CTVChannelInfoTag
 */
void CTVChannelInfoTag::Reset()
{
  m_iIdChannel            = -1;
  m_iChannelNum           = -1;
  m_iClientNum            = -1;
  m_iGroupID              = 0;
  m_strChannel            = "";
  m_IconPath              = "";
  m_radio                 = false;
  m_encrypted             = false;
  m_hide                  = false;
  m_isRecording           = false;
  m_startTime             = NULL;
  m_endTime               = NULL;
  m_strFileNameAndPath    = "";
  m_strNextTitle          = "";

  m_EPG.Clear();

  //CVideoInfoTag::Reset();
}

bool CTVChannelInfoTag::GetEPGNowInfo(CTVEPGInfoTag &result)
{
  CDateTime now = CDateTime::GetCurrentDateTime();

  for (int i = 0; i < m_EPG.Size(); i++)
  {
    CTVEPGInfoTag* tag = m_EPG[i]->GetTVEPGInfoTag();
    if ((tag->m_startTime <= now) && (tag->m_endTime > now))
    {
      result = *tag;
      break;
    }
  }

  return false;
}


bool CTVChannelInfoTag::GetEPGNextInfo(CTVEPGInfoTag &result)
{
  CDateTime now = CDateTime::GetCurrentDateTime();

  for (int i = 0; i < m_EPG.Size(); i++)
  {
    CTVEPGInfoTag* tag = m_EPG[i]->GetTVEPGInfoTag();

    if ((tag->m_startTime <= now) && (tag->m_endTime > now))
    {
      CDateTime next = tag->m_endTime;

      for (int j = 0; j < m_EPG.Size(); j++)
      {
        CTVEPGInfoTag* tagNext = m_EPG[j]->GetTVEPGInfoTag();
        if (tagNext->m_startTime >= next)
        {
          result = *tagNext;
          break;
        }
      }
    }
  }

  return false;
}

bool CTVChannelInfoTag::GetEPGLastEntry(CTVEPGInfoTag &result)
{
  CDateTime last = CDateTime::GetCurrentDateTime();

  for (int i = m_EPG.Size()-1; i >= 0; i--)
  {
    CTVEPGInfoTag* tag = m_EPG[i]->GetTVEPGInfoTag();
    if (tag->m_endTime >= last)
    {
      result = *tag;
      last   = tag->m_endTime;
    }
  }

  return false;
}

void CTVChannelInfoTag::CleanupEPG()
{
  CDateTime m_start = CDateTime::GetCurrentDateTime()-CDateTimeSpan(0, g_guiSettings.GetInt("pvrmenu.lingertime") / 60, g_guiSettings.GetInt("pvrmenu.lingertime") % 60, 0);

  for (int i = 0; i < m_EPG.Size(); i++)
  {
    /* If entry end time is lower as epg data start time remove it from list */
    if (m_EPG[i]->GetTVEPGInfoTag()->m_endTime <= m_start)
    {
      m_EPG.Remove(i);
    }
    else
    {
      /* The items are sorted by date inside list, if the end date is above the epg data
         start date cancel cleanup */
      break;
    }
  }

  return;
}
