#ifndef __LIB_EVENT_MODULE_H__
#define __LIB_EVENT_MODULE_H__

#include "../include/event2/event.h"
#include "../include/event2/buffer.h"
#include "../include/event2/listener.h"
#include "../include/event2/bufferevent.h"
#include "../include/event2/thread.h"

//typedef void(*pListenerMonitorCallback)(evconnlistener *evListener, evutil_socket_t fd,struct sockaddr *sock, int nSockLen, void *arg);
//typedef void(*pSocketReadCallback)(bufferevent *evBuffer, void *arg);
//typedef void(*pSocketEventCallback)(bufferevent *evBuffer, short events, void *arg);
//



class CLibEventModule{
public:
	CLibEventModule();
	int Init();
	int Start(const char* pIpAddress, int& nPort);
	enum{
		ERROR_OK,
		ERROR_INIT,
		ERROR_CALLBACK_FUNCTION_NULL,
	};

#define MaxBufferLen 256
	
	enum{
		EVENT_NONE,
		EVENT_ACCEPT,
		EVENT_READ,
		EVENT_ERROR,
		EVENT_DISCONNECT,
	};
	struct EventBuffer_s{
	public:
		EventBuffer_s(CLibEventModule* pSelf, SOCKET s);
		void SetAddr(char* pszIp, int nPort);
		void SetType(int nType);
		void SetBuffer(char* szBuffer, int nLen);
		void GetAddr(char* pszIp, int& nPort);
		int GetType();
		char* GetBuffer(int& nLen);
		CLibEventModule* GetPtr();
		SOCKET GetFd();

	private:
		SOCKET s;
		char szIP[32];
		int nPort;
		char szBuff[MaxBufferLen];
		int nLen;
		int nEventType;		
		CLibEventModule* pSelf;
	};

	virtual int OnRecvMessage(EventBuffer_s* events) = 0;//return 0 --> continue, other -->disconnect

protected:
	static void ListenMonitorCallback(evconnlistener *evListener, evutil_socket_t fd, struct sockaddr *sock, int nSockLen, void *arg);
	static void SocketReadCallback(bufferevent *evBuffer, void *arg);
	static void SocketEventCallback(bufferevent *evBuffer, short events, void *arg);

private:
	static void WorkThread(void* ptr);
	
	void Work();
	event_base *m_eventBase;
	evconnlistener* m_eventListener;
	evconnlistener* m_eventListenerIPV6;
};

#endif