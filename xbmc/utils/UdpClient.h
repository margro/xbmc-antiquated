#pragma once
#include "stdstring.h"
#include "Thread.h"
#include <vector>

using namespace std;

class CUdpClient : CThread
{
public:
  CUdpClient();
  virtual ~CUdpClient(void);

protected:
  
  bool Create();

  void OnStartup();
  void Process();

  bool Broadcast(int aPort, CStdString& aMessage);
  bool Send(CStdString aIpAddress, int aPort, CStdString& aMessage); 
  bool Send(SOCKADDR_IN aAddress, CStdString& aMessage); 
  bool Send(SOCKADDR_IN aAddress, LPBYTE pMessage, DWORD dwSize); 

  virtual void OnMessage(SOCKADDR_IN& aRemoteAddress, CStdString& aMessage, LPBYTE pMessage, DWORD dwMessageLength){};

protected:

	struct UdpCommand
	{
		SOCKADDR_IN address;
		CStdString  message;
		LPBYTE		binary;
		DWORD		binarySize;
	};

	void DispatchNextCommand();

	SOCKET client_socket;

	vector<UdpCommand> commands;
	typedef vector<UdpCommand> ::iterator COMMANDITERATOR;

	CRITICAL_SECTION critical_section;
};
