// LibeventServer.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include "libEventModule.h"

class CLibEventModuleImp : public CLibEventModule{
public:
	int OnRecvMessage(EventBuffer_s* events){
		char szIP[129] = { 0 };
		int nPort = 0;
		int nLen = 0;
		events->GetAddr(szIP, nPort);
		char* pBuffer = events->GetBuffer(nLen);
		printf("[%s:%d][%d]:%s\n", szIP, nPort, events->GetType(), pBuffer);		
		return 0;
	};
};

int main(int argc, char** argv)
{
	CLibEventModuleImp module;
	module.Init();
	int nPort = 4455;
	module.Start("0.0.0.0", nPort);
	while (true)
	{
		Sleep(100);
	}
	return 0;
}

