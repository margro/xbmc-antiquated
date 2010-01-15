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

/**
 * \file	Socket.h
 * \brief	Socket class
 * \author	Marcel Groothuis
 *
 * Socket support is platform dependent. This file includes the right
 * platform specific header file and defines the platform independent
 * interface
 */

#ifndef __SOCKET_H
#define __SOCKET_H

#define _WINSOCKAPI_ //Needed here to prevent inclusion of <winsock.h> via the header below

//Include platform specific datatypes, header files, defines and constants:
#if defined __WINDOWS__ || defined WIN32 || defined _WINDOWS
	#include "windows/Socket.h"
#elif defined _LINUX
	#include "linux/Socket.h"
#else
	#error Socket support is not yet available on this platform!
#endif

using namespace std;

#define MAXCONNECTIONS 1  ///< Maximum number of pending connections before "Connection refused"
#define MAXRECV 1500      ///< Maximum packet size

enum SocketFamily
{
	#ifdef CONFIG_SOCKET_IPV6
		af_inet6 = AF_INET6,
		af_unspec = AF_UNSPEC,	///< Either INET or INET6
	#endif
	af_inet = AF_INET
};

enum SocketDomain
{
	#if defined _LINUX
		pf_unix = PF_UNIX,
		pf_local = PF_LOCAL,
	#endif
	#ifdef CONFIG_SOCKET_IPV6
		pf_inet6 = PF_INET6,
		pf_unspec = PF_UNSPEC,	//< Either INET or INET6
	#endif
	pf_inet = PF_INET
};

enum SocketType
{
	sock_stream = SOCK_STREAM,
	sock_dgram = SOCK_DGRAM
};

enum SocketProtocol
{
	//ipv6 = IPPROTO_IPV6,
	tcp = IPPROTO_TCP,
	udp = IPPROTO_UDP
};

//enum State
//{
//	INITIAL,
//	AVAILABLE,
//	BOUND,
//	CONNECTED,
//	CONNECTING,
//	STREAM
//};
//typedef enum State State;


class Socket
{
  public:
	
	/**
	 * An unconnected socket may be created directly on the local
	 * machine. The socket type (SOCK_STREAM, SOCK_DGRAM) and
	 * protocol may also be specified.
	 * If the socket cannot be created, an exception is thrown.
	 *
	 * @param family Socket family (IPv4 or IPv6)
	 * @param domain The domain parameter specifies a communications domain within which communication will take place;
	 * this selects the protocol family which should be used.
	 * @param type base type and protocol family of the socket.
	 * @param protocol specific protocol to apply.
	 */
	Socket(const enum SocketFamily family, const enum SocketDomain domain, const enum SocketType type, const enum SocketProtocol protocol = tcp);
	Socket(void);
	virtual ~Socket();

	//Socket settings
	
	/**
	 * Socket setFamily
	 * @param family	Can be af_inet or af_inet6. Default: af_inet
	 */
	void setFamily(const enum SocketFamily family)
	{
		_family = family;
	};

	/**
	 * Socket setDomain
	 * @param domain	Can be pf_unix, pf_local, pf_inet or pf_inet6. Default: pf_inet
	 */
	void setDomain(const enum SocketDomain domain)
	{
		_domain = domain;
	};

	/**
	 * Socket setType
	 * @param type	Can be sock_stream or sock_dgram. Default: sock_stream.
	 */
	void setType(const enum SocketType type)
	{
		_type = type;
	};

	/**
	 * Socket setProtocol
	 * @param protocol	Can be tcp or udp. Default: tcp.
	 */
	void setProtocol(const enum SocketProtocol protocol)
	{
		_protocol = protocol;
	};

	/**
	 * Socket setPort
	 * @param port	port number for socket communication
	 */
	void setPort (const unsigned short port)
	{
		_sockaddr.sin_port = htons ( port );
	};

	bool setHostname ( const std::string host );

	// Server initialization
	
	/**
	 * Socket create
	 * Create a new socket
	 * @return 	True if succesful
	 */
	bool create();
	
	/**
	 * Socket close
	 * Close the socket
	 * @return 	True if succesful
	 */	
	bool close();

	/**
	 * Socket bind
	 */
	bool bind ( const unsigned short port );
	bool listen() const;
	bool accept ( Socket& socket ) const;

	// Client initialization
	bool connect ( const std::string host, const unsigned short port );

	// Data Transmission

	/**
	 * Socket send function
	 *
	 * @param data	Reference to a std::string with the data to transmit
	 * @return	Number of bytes send or -1 in case of an error
	 */
	int send ( const std::string data );

	/**
	 * Socket send function
	 *
	 * @param data	Pointer to a character array of size 'size' with the data to transmit
	 * @param size	Length of the data to transmit
	 * @return	Number of bytes send or -1 in case of an error
	 */
	int send ( const char* data, const unsigned int size );

	/**
	 * Socket sendto function
	 *
	 * @param data	Reference to a std::string with the data to transmit
	 * @param size	Length of the data to transmit
	 * @param sendcompletebuffer	If 'true': do not return until the complete buffer is transmitted
	 * @return	Number of bytes send or -1 in case of an error
	 */
	int sendto ( const char* data, unsigned int size, bool sendcompletebuffer = false) const;
	// Data Receive

	/**
	 * Socket receive function
	 *
	 * @param data	Reference to a std::string for storage of the received data.
	 * @param minpacketsize	The minimum number of bytes that should be received before returning from this function
	 * @return	Number of bytes received or SOCKET_ERROR
	 */
	int receive ( std::string& data, unsigned int minpacketsize ) const;

	/**
	 * Socket receive function
	 *
	 * @param data	Reference to a std::string for storage of the received data.
	 * @return	Number of bytes received or SOCKET_ERROR
	 */
	int receive ( std::string& data ) const;
	
	/**
	 * Socket receive function
	 *
	 * @param data	Pointer to a character array of size buffersize. Used to store the received data.
	 * @param buffersize	Size of the 'data' buffer
	 * @param minpacketsize	Specifies the minimum number of bytes that need to be received before returning
	 * @return	Number of bytes received or SOCKET_ERROR
	 */
	int receive ( char* data, const unsigned int buffersize, const unsigned int minpacketsize ) const;

	/**
	 * Socket recvfrom function
	 *
	 * @param data	Pointer to a character array of size buffersize. Used to store the received data.
	 * @param buffersize	Size of the 'data' buffer
	 * @param minpacketsize	Do not return before at least 'minpacketsize' bytes are in the buffer.
	 * @param from		Optional: pointer to a sockaddr struct that will get the address from which the data is received
	 * @param fromlen	Optional, only required if 'from' is given: length of from struct
	 * @return	Number of bytes received or SOCKET_ERROR
	 */
	int recvfrom ( char* data, const int buffersize, const int minpacketsize, struct sockaddr* from = NULL, socklen_t* fromlen = NULL) const;

	bool set_non_blocking ( const bool );

  bool ReadResponse (int &code, vector<string> &lines);

  private:

	SOCKET _sd;                         ///< Socket Descriptor
	SOCKADDR_IN _sockaddr;              ///< Socket Address

	enum SocketFamily _family;          ///< Socket Address Family
	enum SocketProtocol _protocol;      ///< Socket Protocol
	enum SocketType _type;              ///< Socket Type
	enum SocketDomain _domain;          ///< Socket domain

  #ifdef WIN32
		WSADATA _wsaData;         ///< Windows Socket data
	#endif

	bool is_valid() const;
	void errormessage( int errornum, const char* functionname = NULL) const;

//  protected:

};

#endif

