#include "guidialogyesno.h"
#include "guiWindowManager.h"
#include "localizeStrings.h"

CGUIDialogYesNo::CGUIDialogYesNo(void)
:CGUIDialog(0)
{
	m_bConfirmed=false;
}

CGUIDialogYesNo::~CGUIDialogYesNo(void)
{
}

bool CGUIDialogYesNo::OnMessage(CGUIMessage& message)
{
	switch ( message.GetMessage() )
  {
    case GUI_MSG_WINDOW_INIT:
    {
			m_bConfirmed=false;
		}
		break;

    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
			if (iControl==10)
			{
				m_bConfirmed=false;
				Close();
				return true;
			}
			if (iControl==11)
			{
				m_bConfirmed=true;
				Close();
				return true;
			}
		}
		break;
	}
	return CGUIDialog::OnMessage(message);
}

bool CGUIDialogYesNo::IsConfirmed() const
{
	return m_bConfirmed;
}


void  CGUIDialogYesNo::SetHeading(const wstring& strLine)
{
	int iControl=1;
  CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iControl,0,0,(void*)strLine.c_str());
  OnMessage(msg);

}

void CGUIDialogYesNo::SetLine(int iLine, const wstring& strLine)
{
	int iControl=iLine+2;

  CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iControl,0,0,(void*)strLine.c_str());
  OnMessage(msg);
}

void CGUIDialogYesNo::SetLine(int iLine, const string& strLine)
{
	WCHAR wszLine[1024];
	swprintf(wszLine,L"%S", strLine.c_str());
	SetLine(iLine,wszLine);
}
void CGUIDialogYesNo::SetHeading(int iString)
{
	const WCHAR* pszTxt=g_localizeStrings.Get(iString).c_str();
	SetHeading(pszTxt);
}


void	CGUIDialogYesNo::SetLine(int iLine, int iString)
{
	const WCHAR* pszTxt=g_localizeStrings.Get(iString).c_str();
	SetLine(iLine,pszTxt);
}