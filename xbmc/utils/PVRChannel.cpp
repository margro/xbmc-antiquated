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

#include "FileItem.h"
#include "LocalizeStrings.h"
#include "utils/log.h"
#include "Util.h"
#include "FileSystem/File.h"
#include "MusicInfoTag.h"

#include "PVRChannel.h"
#include "PVREpgs.h"
#include "PVREpg.h"
#include "PVREpgInfoTag.h"
#include "TVDatabase.h"
#include "PVRManager.h"

using namespace XFILE;
using namespace MUSIC_INFO;

const CPVREpgInfoTag *m_EmptyEpgInfoTag = new CPVREpgInfoTag();

bool CPVRChannel::operator==(const CPVRChannel& right) const
{
  if (this == &right) return true;

  return (m_iDatabaseId             == right.m_iDatabaseId &&
          m_iChannelNumber          == right.m_iChannelNumber &&
          m_iChannelGroupId         == right.m_iChannelGroupId &&
          m_bIsRadio                == right.m_bIsRadio &&
          m_bIsHidden               == right.m_bIsHidden &&
          m_bClientIsRecording      == right.m_bClientIsRecording &&
          m_strIconPath             == right.m_strIconPath &&
          m_strChannelName          == right.m_strChannelName &&
          m_bIsVirtual              == right.m_bIsVirtual &&

          m_iUniqueId               == right.m_iUniqueId &&
          m_iClientId               == right.m_iClientId &&
          m_iClientChannelNumber    == right.m_iClientChannelNumber &&
          m_strClientChannelName    == right.m_strClientChannelName &&
          m_strStreamURL            == right.m_strStreamURL &&
          m_strFileNameAndPath      == right.m_strFileNameAndPath &&
          m_iClientEncryptionSystem == right.m_iClientEncryptionSystem);
}

bool CPVRChannel::operator!=(const CPVRChannel &right) const
{
  return !(*this == right);
}

CPVRChannel::CPVRChannel()
{
  m_iDatabaseId             = -1;
  m_iChannelNumber          = -1;
  m_iChannelGroupId         = -1;
  m_bIsRadio                = false;
  m_bIsHidden               = false;
  m_bClientIsRecording      = false;
  m_strIconPath             = "";
  m_strChannelName          = "";
  m_bIsVirtual              = false;

  m_EPG                     = NULL;
  m_EPGNow                  = NULL;
  m_bEPGEnabled             = true;
  m_strEPGScraper           = "client";

  m_iUniqueId               = -1;
  m_iClientId               = -1;
  m_iClientChannelNumber    = -1;
  m_strClientChannelName    = "";
  m_strInputFormat          = "";
  m_strStreamURL            = "";
  m_strFileNameAndPath      = "";
  m_iClientEncryptionSystem = -1;
}

/********** XBMC related channel methods **********/

bool CPVRChannel::UpdateFromClient(const CPVRChannel &channel)
{
  bool bChanged = false;

  if (m_iClientId != channel.ClientID())
  {
    bChanged = true;
    SetClientID(channel.ClientID());
  }

  if (m_iClientChannelNumber != channel.ClientChannelNumber())
  {
    bChanged = true;
    SetClientChannelNumber(channel.ClientChannelNumber());
  }

  return bChanged;
}

bool CPVRChannel::Persist(void)
{
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  if (database)
  {
    database->Open();
    database->UpdateDBChannel(*this);
    database->Close();

    return true;
  }

  return false;
}

void CPVRChannel::SetChannelID(long iDatabaseId, bool bSaveInDb /* = false */)
{
  if (m_iDatabaseId != iDatabaseId)
  {
    /* update the id */
    m_iDatabaseId = iDatabaseId;
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();
  }
}

void CPVRChannel::SetChannelNumber(int iChannelNumber, bool bSaveInDb /* = false */)
{
  if (m_iChannelNumber != iChannelNumber)
  {
    /* update the channel number */
    m_iChannelNumber = iChannelNumber;
    UpdatePath();
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();
  }
}

void CPVRChannel::SetGroupID(int iChannelGroupId, bool bSaveInDb /* = false */)
{
  if (m_iChannelGroupId != iChannelGroupId)
  {
    /* update the group id */
    m_iChannelGroupId = iChannelGroupId;
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();
  }
}

void CPVRChannel::SetRadio(bool bIsRadio, bool bSaveInDb /* = false */)
{
  if (m_bIsRadio != bIsRadio)
  {
    /* update the radio flag */
    m_bIsRadio = bIsRadio;
    UpdatePath();
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();
  }
}

void CPVRChannel::SetHidden(bool bIsHidden, bool bSaveInDb /* = false */)
{
  if (m_bIsHidden != bIsHidden)
  {
    /* update the hidden flag */
    m_bIsHidden = bIsHidden;
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();
  }
}

void CPVRChannel::SetRecording(bool bClientIsRecording)
{
  m_bClientIsRecording = bClientIsRecording;
  SetChanged();
}

bool CPVRChannel::SetIconPath(const CStdString &strIconPath, bool bSaveInDb /* = false */)
{
  /* check if the path is valid */
  if (!CFile::Exists(strIconPath))
    return false;

  if (m_strIconPath != strIconPath)
  {
    /* update the path */
    m_strIconPath = CStdString(strIconPath);
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();
  }

  return true;
}

void CPVRChannel::SetChannelName(const CStdString &strChannelName, bool bSaveInDb /* = false */)
{
  CStdString strName(strChannelName);

  if (strName.IsEmpty())
  {
    strName.Format(g_localizeStrings.Get(19085), ClientChannelNumber());
  }

  if (m_strChannelName != strName)
  {
    /* update the channel name */
    m_strChannelName = strName;
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();
  }
}

void CPVRChannel::SetVirtual(bool bIsVirtual, bool bSaveInDb /* = false */)
{
  if (m_bIsVirtual != bIsVirtual)
  {
    /* update the virtual flag */
    m_bIsVirtual = bIsVirtual;
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();
  }
}

bool CPVRChannel::IsEmpty() const
{
  return (m_strFileNameAndPath.IsEmpty() ||
          m_strStreamURL.IsEmpty());
}

/********** Client related channel methods **********/

void CPVRChannel::SetUniqueID(int iUniqueId, bool bSaveInDb /* = false */)
{
  if (m_iUniqueId != iUniqueId)
  {
    /* update the unique ID */
    m_iUniqueId = iUniqueId;
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();
  }
}

void CPVRChannel::SetClientID(int iClientId, bool bSaveInDb /* = false */)
{
  if (m_iClientId != iClientId)
  {
    /* update the client ID */
    m_iClientId = iClientId;
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();
  }
}

void CPVRChannel::SetClientChannelNumber(int iClientChannelNumber, bool bSaveInDb /* = false */)
{
  if (m_iClientChannelNumber != iClientChannelNumber)
  {
    /* update the client channel number */
    m_iClientChannelNumber = iClientChannelNumber;
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();
  }
}

void CPVRChannel::SetClientChannelName(const CStdString &strClientChannelName, bool bSaveInDb /* = false */)
{
  if (m_strClientChannelName != strClientChannelName)
  {
    /* update the client channel name */
    m_strClientChannelName = CStdString(strClientChannelName);
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();
  }
}

void CPVRChannel::SetInputFormat(const CStdString &strInputFormat, bool bSaveInDb /* = false */)
{
  if (m_strInputFormat != strInputFormat)
  {
    /* update the input format */
    m_strInputFormat = CStdString(strInputFormat);
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();
  }
}

void CPVRChannel::SetStreamURL(const CStdString &strStreamURL, bool bSaveInDb /* = false */)
{
  if (m_strStreamURL != strStreamURL)
  {
    /* update the stream url */
    m_strStreamURL = CStdString(strStreamURL);
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();
  }
}

void CPVRChannel::UpdatePath(void)
{
  m_strFileNameAndPath.Format("pvr://channels/%s/all/%i.pvr", (m_bIsRadio ? "radio" : "tv"), m_iChannelNumber);
  SetChanged();
}

void CPVRChannel::SetEncryptionSystem(int iClientEncryptionSystem, bool bSaveInDb /* = false */)
{
  if (m_iClientEncryptionSystem != iClientEncryptionSystem)
  {
    /* update the client encryption system */
    m_iClientEncryptionSystem = iClientEncryptionSystem;
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();
  }
}

CStdString CPVRChannel::EncryptionName() const
{
  // http://www.dvb.org/index.php?id=174
  // http://en.wikipedia.org/wiki/Conditional_access_system
  CStdString strName;

  if (     m_iClientEncryptionSystem == 0x0000)
    strName = g_localizeStrings.Get(19013); /* Free To Air */
  else if (m_iClientEncryptionSystem <  0x0000)
    strName = g_localizeStrings.Get(13205); /* Unknown */
  else if (m_iClientEncryptionSystem >= 0x0001 &&
           m_iClientEncryptionSystem <= 0x009F)
    strName.Format("%s (%X)", g_localizeStrings.Get(19014).c_str(), m_iClientEncryptionSystem); /* Fixed */
  else if (m_iClientEncryptionSystem >= 0x00A0 &&
           m_iClientEncryptionSystem <= 0x00A1)
    strName.Format("%s (%X)", g_localizeStrings.Get(338).c_str(), m_iClientEncryptionSystem); /* Analog */
  else if (m_iClientEncryptionSystem >= 0x00A2 &&
           m_iClientEncryptionSystem <= 0x00FF)
    strName.Format("%s (%X)", g_localizeStrings.Get(19014).c_str(), m_iClientEncryptionSystem); /* Fixed */
  else if (m_iClientEncryptionSystem >= 0x0100 &&
           m_iClientEncryptionSystem <= 0x01FF)
    strName.Format("%s (%X)", "SECA Mediaguard", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x0464)
    strName.Format("%s (%X)", "EuroDec", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem >= 0x0500 &&
           m_iClientEncryptionSystem <= 0x05FF)
    strName.Format("%s (%X)", "Viaccess", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem >= 0x0600 &&
           m_iClientEncryptionSystem <= 0x06FF)
    strName.Format("%s (%X)", "Irdeto", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem >= 0x0900 &&
           m_iClientEncryptionSystem <= 0x09FF)
    strName.Format("%s (%X)", "NDS Videoguard", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem >= 0x0B00 &&
           m_iClientEncryptionSystem <= 0x0BFF)
    strName.Format("%s (%X)", "Conax", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem >= 0x0D00 &&
           m_iClientEncryptionSystem <= 0x0DFF)
    strName.Format("%s (%X)", "CryptoWorks", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem >= 0x0E00 &&
           m_iClientEncryptionSystem <= 0x0EFF)
    strName.Format("%s (%X)", "PowerVu", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x1000)
    strName.Format("%s (%X)", "RAS", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem >= 0x1200 &&
           m_iClientEncryptionSystem <= 0x12FF)
    strName.Format("%s (%X)", "NagraVision", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem >= 0x1700 &&
           m_iClientEncryptionSystem <= 0x17FF)
    strName.Format("%s (%X)", "BetaCrypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem >= 0x1800 &&
           m_iClientEncryptionSystem <= 0x18FF)
    strName.Format("%s (%X)", "NagraVision", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x22F0)
    strName.Format("%s (%X)", "Codicrypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x2600)
    strName.Format("%s (%X)", "BISS", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4347)
    strName.Format("%s (%X)", "CryptOn", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4800)
    strName.Format("%s (%X)", "Accessgate", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4900)
    strName.Format("%s (%X)", "China Crypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4A10)
    strName.Format("%s (%X)", "EasyCas", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4A20)
    strName.Format("%s (%X)", "AlphaCrypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4A70)
    strName.Format("%s (%X)", "DreamCrypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4A60)
    strName.Format("%s (%X)", "SkyCrypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4A61)
    strName.Format("%s (%X)", "Neotioncrypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4A62)
    strName.Format("%s (%X)", "SkyCrypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4A63)
    strName.Format("%s (%X)", "Neotion SHL", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem >= 0x4A64 &&
           m_iClientEncryptionSystem <= 0x4A6F)
    strName.Format("%s (%X)", "SkyCrypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4A80)
    strName.Format("%s (%X)", "ThalesCrypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4AA1)
    strName.Format("%s (%X)", "KeyFly", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4ABF)
    strName.Format("%s (%X)", "DG-Crypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem >= 0x4AD0 &&
           m_iClientEncryptionSystem <= 0x4AD1)
    strName.Format("%s (%X)", "X-Crypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4AD4)
    strName.Format("%s (%X)", "OmniCrypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4AE0)
    strName.Format("%s (%X)", "RossCrypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x5500)
    strName.Format("%s (%X)", "Z-Crypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x5501)
    strName.Format("%s (%X)", "Griffin", m_iClientEncryptionSystem);
  else
    strName.Format("%s (%X)", g_localizeStrings.Get(19499).c_str(), m_iClientEncryptionSystem); /* Unknown */

  return strName;
}

/********** EPG methods **********/

CPVREpg *CPVRChannel::GetEPG(void)
{
  if (m_EPG == NULL)
  {
    /* will be cleaned up by CPVREpgs on exit */
    m_EPG = new CPVREpg(this);
    PVREpgs.push_back(m_EPG);
  }

  return m_EPG;
}

bool CPVRChannel::ClearEPG()
{
  if (m_EPG != NULL)
  {
    GetEPG()->Clear();
    m_EPGNow = NULL;
  }

  return true;
}

const CPVREpgInfoTag* CPVRChannel::GetEPGNow(void) const
{
  if (m_bIsHidden || !m_bEPGEnabled || m_EPGNow == NULL)
    return m_EmptyEpgInfoTag;

  return m_EPGNow;
}

const CPVREpgInfoTag* CPVRChannel::GetEPGNext(void) const
{
  if (m_bIsHidden || !m_bEPGEnabled || m_EPGNow == NULL)
    return m_EmptyEpgInfoTag;

  const CPVREpgInfoTag *nextTag = m_EPGNow->GetNextEvent();
  return nextTag == NULL ?
      m_EmptyEpgInfoTag :
      nextTag;
}

void CPVRChannel::SetEPGEnabled(bool bEPGEnabled /* = true */, bool bSaveInDb /* = false */)
{
  if (m_bEPGEnabled != bEPGEnabled)
  {
    /* update the EPG flag */
    m_bEPGEnabled = bEPGEnabled;
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();
  }
}

void CPVRChannel::SetEPGScraper(const CStdString &strScraper, bool bSaveInDb /* = false */)
{
  if (m_strEPGScraper != strScraper)
  {
    /* update the scraper name */
    m_strEPGScraper = CStdString(strScraper);
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();
  }
}

void CPVRChannel::UpdateEPGPointers(void)
{
  if (m_bIsHidden || !m_bEPGEnabled)
    return;

  CPVREpg *epg = GetEPG();

  if (!epg->IsUpdateRunning() &&
      (m_EPGNow == NULL ||
       m_EPGNow->End() <= CDateTime::GetCurrentDateTime()))
  {
    SetChanged();
    m_EPGNow  = epg->InfoTagNow();
    if (m_EPGNow)
    {
      CLog::Log(LOGDEBUG, "%s - EPG now pointer for channel '%s' updated to '%s'",
          __FUNCTION__, m_strChannelName.c_str(), m_EPGNow->Title().c_str());
    }
    else
    {
      CLog::Log(LOGDEBUG, "%s - no EPG now pointer for channel '%s'",
          __FUNCTION__, m_strChannelName.c_str());
    }
  }

  NotifyObservers("epg");
}
