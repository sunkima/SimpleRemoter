// IOCPClient.cpp: implementation of the IOCPClient class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "IOCPClient.h"
#include <IOSTREAM>
#include "zconf.h"
#include "zlib.h"
#include <assert.h>
using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

VOID IOCPClient::setManagerCallBack(class CManager* Manager)
{
	m_Manager = Manager;
}


IOCPClient::IOCPClient(bool exit_while_disconnect)
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	
	m_sClientSocket = INVALID_SOCKET;
	m_hWorkThread   = NULL;
	m_bWorkThread = S_STOP;

	memcpy(m_szPacketFlag,"Shine",FLAG_LENGTH);

	m_bIsRunning = TRUE;
	m_bConnected = FALSE;

	InitializeCriticalSection(&m_cs);
	m_exit_while_disconnect = exit_while_disconnect;
}

IOCPClient::~IOCPClient()
{
	m_bIsRunning = FALSE;

	if (m_sClientSocket!=INVALID_SOCKET)
	{
		closesocket(m_sClientSocket);
		m_sClientSocket = INVALID_SOCKET;
	}

	if (m_hWorkThread!=NULL)
	{
		CloseHandle(m_hWorkThread);
		m_hWorkThread = NULL;
	}

	WSACleanup();

	while (S_RUN == m_bWorkThread)
		Sleep(10);

	Sleep(5000);

	DeleteCriticalSection(&m_cs);

	m_bWorkThread = S_END;
}

BOOL IOCPClient::ConnectServer(char* szServerIP, unsigned short uPort)
{
	m_sClientSocket = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);    //传输层
	
	if (m_sClientSocket == SOCKET_ERROR)   
	{ 
		return FALSE;   
	}

	//构造sockaddr_in结构 也就是主控端的结构
	sockaddr_in	ServerAddr;
	ServerAddr.sin_family	= AF_INET;               //网络层  IP
	ServerAddr.sin_port	= htons(uPort);	
	ServerAddr.sin_addr.S_un.S_addr = inet_addr(szServerIP);
	
	if (connect(m_sClientSocket,(SOCKADDR *)&ServerAddr,sizeof(sockaddr_in)) == SOCKET_ERROR) 
	{
		if (m_sClientSocket!=INVALID_SOCKET)
		{
			closesocket(m_sClientSocket);
			m_sClientSocket = INVALID_SOCKET;
		}
		return FALSE;
	}

	const int chOpt = 1; // True
	// Set KeepAlive 开启保活机制, 防止服务端产生死连接
	if (setsockopt(m_sClientSocket, SOL_SOCKET, SO_KEEPALIVE,
		(char *)&chOpt, sizeof(chOpt)) == 0)
	{
		// 设置超时详细信息
		tcp_keepalive	klive;
		klive.onoff = 1; // 启用保活
		klive.keepalivetime = 1000 * 60 * 3; // 3分钟超时 Keep Alive
		klive.keepaliveinterval = 1000 * 5;  // 重试间隔为5秒 Resend if No-Reply
		WSAIoctl(m_sClientSocket, SIO_KEEPALIVE_VALS,&klive,sizeof(tcp_keepalive),
			NULL,	0,(unsigned long *)&chOpt,0,NULL);
	}
	if (m_hWorkThread == NULL){
		m_hWorkThread = (HANDLE)CreateThread(NULL, 0, 
		(LPTHREAD_START_ROUTINE)WorkThreadProc,(LPVOID)this, 0, NULL);
		m_bWorkThread = m_hWorkThread ? S_RUN : S_STOP;
	}
	std::cout<<"连接服务端成功.\n";
	m_bConnected = TRUE;
	return TRUE;
}

DWORD WINAPI IOCPClient::WorkThreadProc(LPVOID lParam)
{
	IOCPClient* This = (IOCPClient*)lParam;
	char szBuffer[MAX_RECV_BUFFER] = {0};
	fd_set fd;
	const struct timeval tm = { 2, 0 };

	while (This->IsRunning()) // 没有退出，就一直陷在这个循环中
	{
		if(!This->IsConnected())
		{
			Sleep(50);
			continue;
		}
		FD_ZERO(&fd);
		FD_SET(This->m_sClientSocket, &fd);
		int iRet = select(NULL, &fd, NULL, NULL, &tm);
		if (iRet <= 0)      
		{
			if (iRet == 0) Sleep(50);
			else
			{
				printf("[select] return %d, GetLastError= %d. \n", iRet, WSAGetLastError());
				This->Disconnect(); //接收错误处理
				if(This->m_exit_while_disconnect)
					break;
			}
		}
		else if (iRet > 0)
		{
			memset(szBuffer, 0, sizeof(szBuffer));
			int iReceivedLength = recv(This->m_sClientSocket,
				szBuffer,sizeof(szBuffer), 0); //接收主控端发来的数据
			if (iReceivedLength <= 0)
			{
				int a = GetLastError();
				This->Disconnect(); //接收错误处理
				if(This->m_exit_while_disconnect)
					break;
			}else{
				//正确接收就调用OnRead处理,转到OnRead
				This->OnServerReceiving((char*)szBuffer, iReceivedLength);
			}
		}
	}
	This->m_bWorkThread = S_STOP;
	This->m_bIsRunning = FALSE;

	return 0xDEAD;
}


VOID IOCPClient::OnServerReceiving(char* szBuffer, ULONG ulLength)
{
	try
	{
		assert (ulLength > 0);	
		//以下接到数据进行解压缩
		m_CompressedBuffer.WriteBuffer((LPBYTE)szBuffer, ulLength);
		
		//检测数据是否大于数据头大小 如果不是那就不是正确的数据
		while (m_CompressedBuffer.GetBufferLength() > HDR_LENGTH)
		{
			char szPacketFlag[FLAG_LENGTH] = {0};
			CopyMemory(szPacketFlag, m_CompressedBuffer.GetBuffer(),FLAG_LENGTH);
			//判断数据头
			if (memcmp(m_szPacketFlag, szPacketFlag, FLAG_LENGTH) != 0)
			{
				throw "Bad Buffer";
			}
			
			ULONG ulPackTotalLength = 0;
			CopyMemory(&ulPackTotalLength, m_CompressedBuffer.GetBuffer(FLAG_LENGTH), 
				sizeof(ULONG));
			
			//--- 数据的大小正确判断
			if (ulPackTotalLength && 
				(m_CompressedBuffer.GetBufferLength()) >= ulPackTotalLength)
			{
				m_CompressedBuffer.ReadBuffer((PBYTE)szPacketFlag, FLAG_LENGTH);//读取各种头部 shine

				m_CompressedBuffer.ReadBuffer((PBYTE) &ulPackTotalLength, sizeof(ULONG));            
				
				ULONG ulOriginalLength = 0; 
				m_CompressedBuffer.ReadBuffer((PBYTE) &ulOriginalLength, sizeof(ULONG)); 

				//50  
				ULONG ulCompressedLength = ulPackTotalLength - HDR_LENGTH; 
				PBYTE CompressedBuffer = new BYTE[ulCompressedLength];              
                PBYTE DeCompressedBuffer = new BYTE[ulOriginalLength]; 
				
				m_CompressedBuffer.ReadBuffer(CompressedBuffer, ulCompressedLength);
	
				int	iRet = uncompress(DeCompressedBuffer, 
					&ulOriginalLength, CompressedBuffer, ulCompressedLength);
				
				if (iRet == Z_OK)//如果解压成功
				{
					m_DeCompressedBuffer.ClearBuffer();
					m_DeCompressedBuffer.WriteBuffer(DeCompressedBuffer,
						ulOriginalLength);
					
					//解压好的数据和长度传递给对象Manager进行处理 注意这里是用了多态
					//由于m_pManager中的子类不一样造成调用的OnReceive函数不一样
					m_Manager->OnReceive((PBYTE)m_DeCompressedBuffer.GetBuffer(0),
						m_DeCompressedBuffer.GetBufferLength());
				}
				else
					throw "Bad Buffer";
				
				delete [] CompressedBuffer;
				delete [] DeCompressedBuffer;
			}
			else
				break;
		}
	}catch(...)
	{
		m_CompressedBuffer.ClearBuffer();
	}
}


int IOCPClient::OnServerSending(char* szBuffer, ULONG ulOriginalLength)  //Hello
{
	m_WriteBuffer.ClearBuffer();

	if (ulOriginalLength > 0)
	{
		//乘以1.001是以最坏的也就是数据压缩后占用的内存空间和原先一样 +12 
		//防止缓冲区溢出//  HelloWorld  10   22
		//数据压缩 压缩算法 微软提供
		//nSize   = 436
		//destLen = 448                             
		unsigned long	ulCompressedLength = (double)ulOriginalLength * 1.001  + 12;     
		LPBYTE			CompressedBuffer = new BYTE[ulCompressedLength]; 
		
		int	iRet = compress(CompressedBuffer, &ulCompressedLength, (PBYTE)szBuffer, ulOriginalLength);   

		if (iRet != Z_OK)  
		{
			delete [] CompressedBuffer;
			return FALSE;
		}
		
		ULONG ulPackTotalLength = ulCompressedLength + HDR_LENGTH;    

		m_WriteBuffer.WriteBuffer((PBYTE)m_szPacketFlag, sizeof(m_szPacketFlag));  
	   	
		m_WriteBuffer.WriteBuffer((PBYTE) &ulPackTotalLength,sizeof(ULONG));   
		//  5      4
		//[Shine][ 30 ]	

		m_WriteBuffer.WriteBuffer((PBYTE)&ulOriginalLength, sizeof(ULONG));   	
		//  5      4    4
		//[Shine][ 30 ][5]	
	
		m_WriteBuffer.WriteBuffer(CompressedBuffer,ulCompressedLength); 
	
		delete [] CompressedBuffer;
		CompressedBuffer = NULL;		
	}
	// 分块发送
	//shine[0035][0010][HelloWorld+12]
	return SendWithSplit((char*)m_WriteBuffer.GetBuffer(), m_WriteBuffer.GetBufferLength(), 
		MAX_SEND_BUFFER);
}

//  5    2   //  2  2  1
BOOL IOCPClient::SendWithSplit(char* szBuffer, ULONG ulLength, ULONG ulSplitLength)
{
	//1025
	int			 iReturn = 0;   //真正发送了多少
	const char*  Travel = (char *)szBuffer;
	int			 i = 0;
	ULONG		 ulSended = 0;
	ULONG		 ulSendRetry = 15;
	// 依次发送
     
	for (i = ulLength; i >= ulSplitLength; i -= ulSplitLength)
	{
		int j = 0;
		for (; j < ulSendRetry; j++)
		{     			
			iReturn = send(m_sClientSocket, Travel, ulSplitLength,0);
			if (iReturn > 0)
			{
				break;
			}
		}
		if (j == ulSendRetry)
		{
			return FALSE;
		}
		
		ulSended += iReturn;  
		Travel += ulSplitLength;   
		Sleep(15); //过快会引起控制端数据混乱
	}
	// 发送最后的部分
	if (i>0)  //1024
	{
		int j = 0;
		for (; j < ulSendRetry; j++)   //nSendRetry = 15
		{
			iReturn = send(m_sClientSocket, (char*)Travel,i,0);

			Sleep(15);
			if (iReturn > 0)
			{
				break;
			}
		}
		if (j == ulSendRetry)
		{
			return FALSE;
		}
		ulSended += iReturn;   //0+=1000
	}
	if (ulSended == ulLength)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


VOID IOCPClient::Disconnect() 
{
	std::cout<<"断开和服务端的连接.\n";

	CancelIo((HANDLE)m_sClientSocket);
	closesocket(m_sClientSocket);	
	m_sClientSocket = INVALID_SOCKET;

	m_bConnected = FALSE;
}


VOID IOCPClient::RunEventLoop()
{
	OutputDebugStringA("======> RunEventLoop begin\n");
	while (m_bIsRunning) Sleep(200);
	OutputDebugStringA("======> RunEventLoop end\n");
}
