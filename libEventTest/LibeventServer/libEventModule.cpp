#include "stdafx.h"
#include "libEventModule.h"

#pragma comment(lib, "../lib/libevent_core.lib")
#pragma comment(lib, "ws2_32")

CLibEventModule::CLibEventModule()
{
	m_eventBase = nullptr;
	m_eventListener = nullptr;
	m_eventListenerIPV6 = nullptr;
}

int CLibEventModule::Init()
{
	WSADATA wsaData;

	if (WSAStartup(0x0202, &wsaData) != 0){
		return ERROR_INIT;
	}

	evthread_use_windows_threads();
	struct event_config* cfg = event_config_new();
	event_config_set_flag(cfg, EVENT_BASE_FLAG_STARTUP_IOCP);
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	event_config_set_num_cpus_hint(cfg, si.dwNumberOfProcessors);
	
	if (m_eventListener){
		evconnlistener_free(m_eventListener);
		m_eventListener = nullptr;
	}

	if (m_eventListenerIPV6){
		evconnlistener_free(m_eventListenerIPV6);
		m_eventListenerIPV6 = nullptr;
	}

	if (m_eventBase){
		event_base_free(m_eventBase);
		m_eventBase = nullptr;
	}

	m_eventBase = event_base_new_with_config(cfg);

	event_config_free(cfg);
	return ERROR_OK;
}

int CLibEventModule::Start(const char* pIpAddress, int& nPort, bool bThreaded)
{
	struct sockaddr_in sockaddrIn;
	memset(&sockaddrIn, 0, sizeof(struct sockaddr_in));
	sockaddrIn.sin_family = AF_INET;
	sockaddrIn.sin_port = htons(nPort);

	struct sockaddr_in6 sockaddrIn6;
	memset(&sockaddrIn6, 0, sizeof(struct sockaddr_in6));
	sockaddrIn6.sin6_family = AF_INET6;
	sockaddrIn6.sin6_port = htons(nPort);

	m_eventListener = evconnlistener_new_bind(\
		m_eventBase, \
		ListenMonitorCallback, \
		this, \
		LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, \
		10, \
		(struct sockaddr*)&sockaddrIn, \
		sizeof(sockaddrIn));

	m_eventListenerIPV6 = evconnlistener_new_bind(\
		m_eventBase, \
		ListenMonitorCallback, \
		this, \
		LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, \
		10, \
		(struct sockaddr*)&sockaddrIn6, \
		sizeof(sockaddrIn6));
	if (bThreaded)
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)WorkThread, this, 0, 0);
	else
		Work();
	return ERROR_OK;
}

void CLibEventModule::ListenMonitorCallback(evconnlistener *evListener, evutil_socket_t fd, struct sockaddr *sock, int nSockLen, void *arg)
{
	SOCKET s = fd;
	char szIp[128] = { 0 };
	char szPort[128] = { 0 };
	int nPort = 0;
	sockaddr_in* pSockAddrIn = (sockaddr_in*)sock;
	evutil_inet_ntop(pSockAddrIn->sin_family, &pSockAddrIn->sin_addr, szIp, sizeof(szIp));
	nPort = ntohs(pSockAddrIn->sin_port);
	CLibEventModule* pSelf = (CLibEventModule*)arg;
	EventBuffer_s* pEvents = new EventBuffer_s(pSelf, fd);
	event_base *base = pSelf->m_eventBase;
	bufferevent *bev = bufferevent_socket_new(\
		base,\
		fd,\
		BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE\
		);

	bufferevent_setcb(bev, \
		SocketReadCallback, \
		NULL, \
		SocketEventCallback, \
		pEvents);
	bufferevent_enable(bev, EV_READ | EV_PERSIST | EV_TIMEOUT | EV_CLOSED);
	pEvents->SetAddr(szIp, nPort);
	pEvents->SetType(EVENT_ACCEPT);
	pSelf->OnRecvMessage(pEvents);
}

void CLibEventModule::SocketReadCallback(bufferevent *evBuffer, void *arg)
{
	char sz[MaxBufferLen] = { 0 };
	size_t nLen;

	while (nLen = bufferevent_read(evBuffer, sz, MaxBufferLen), nLen > 0){
		EventBuffer_s* pEvents = (EventBuffer_s*)arg;
		pEvents->SetBuffer(sz, nLen);
		pEvents->SetType(EVENT_READ); 
		pEvents->GetPtr()->OnRecvMessage(pEvents);
	}
}

void CLibEventModule::SocketEventCallback(bufferevent *evBuffer, short events, void *arg)
{	
	EventBuffer_s* pEvents = (EventBuffer_s*)arg;
	if (events & BEV_EVENT_EOF){
		pEvents->SetBuffer("disconnect", 10);
		pEvents->SetType(EVENT_ERROR);
	}
	else if (events & BEV_EVENT_ERROR){
		pEvents->SetBuffer("error", 5);
		pEvents->SetType(EVENT_DISCONNECT);
	}
	pEvents->GetPtr()->OnRecvMessage(pEvents);
	bufferevent_free(evBuffer);
	delete pEvents;
}

void CLibEventModule::WorkThread(void* ptr)
{
	CLibEventModule* pSelf = (CLibEventModule*)ptr;
	pSelf->Work();
}

void CLibEventModule::Work()
{
	event_base_dispatch(m_eventBase);
}

CLibEventModule::EventBuffer_s::EventBuffer_s(CLibEventModule* pSelf, SOCKET s) : pSelf(pSelf), s(s)
{
	s = 0;
	memset(szIP, 0, 32);
	nPort = 0;
	memset(szBuff, 0, MaxBufferLen);
	nLen = 0;
	nEventType = EVENT_NONE;
}

void CLibEventModule::EventBuffer_s::SetAddr(char* pszIp, int nPort)
{
	strcpy(szIP, pszIp);
	this->nPort = nPort;
}

void CLibEventModule::EventBuffer_s::SetType(int nType)
{
	this->nEventType = nType;
}

void CLibEventModule::EventBuffer_s::SetBuffer(char* szBuffer, int nLen)
{
	if (nLen < MaxBufferLen - 1){
		this->szBuff[nLen] = 0;
	}
	memcpy(this->szBuff, szBuffer, nLen);
	this->nLen = nLen;
}

void CLibEventModule::EventBuffer_s::GetAddr(char* pszIp, int& nPort)
{
	strcpy(pszIp, this->szIP);
	nPort = this->nPort;
}

int CLibEventModule::EventBuffer_s::GetType()
{
	return this->nEventType;
}

char* CLibEventModule::EventBuffer_s::GetBuffer(int& nLen)
{
	nLen = this->nLen;
	return this->szBuff;
}

CLibEventModule* CLibEventModule::EventBuffer_s::GetPtr()
{
	return pSelf;
}

SOCKET CLibEventModule::EventBuffer_s::GetFd()
{
	return s;
}
