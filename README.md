
# 前言

本文介绍了Windows平台上的串口相关软件开发的具体步骤，其大致分为4步：（1）打开串口 （2）配置串口（3）串口通讯（4）关闭串口。接下来，会用C++代码示例来详细介绍具体的开发流程。

# 1. 打开串口
调用CreateFile()函数来打开串口，函数执行成功返回串口句柄，出错返回INVALID_HANDLE_VALUE。

```cpp
HANDLE WINAPI CreateFile(
_In_ LPCTSTR lpFileName,
_In_ DWORD dwDesiredAccess,
_In_ DWORD dwShareMode,
_In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
_In_ DWORD dwCreationDisposition,
_In_ DWORD dwFlagsAndAttributes,
_In_opt_ HANDLE hTemplateFile
);
```

## 1.1 参数详解
<kbd>lpFileName</kbd>表示要打开的串口逻辑名，如“COM1”；注意当串口号大于9时，字符串前需加上"\\\\.\\"的前缀；

<kbd>dwDesiredAccess</kbd>指定类型的访问对象。如果为 GENERIC_READ 表示允许对设备进行读访问；如果为 GENERIC_WRITE 表示允许对设备进行写访问（可组合使用）；如果为零，表示只允许获取与一个设备有关的信息 ；

<kbd>dwShareMode</kbd>打开串口时，该参数只能为0，表示独占方式；

<kbd>lpSecurityAttributes</kbd>设置为NULL即可；

<kbd>dwCreationDisposition</kbd>打开串口时必须设置为OPEN_EXISTING;

<kbd>dwFlagsAndAttributes</kbd>对于串口而言，唯一有意义的设置时FILE_FLAG_OVERLAPPED,创建时指定该设置，串口可执行异步操作；否则只能执行同步操作；

<kbd>hTemplateFile</kbd>串口操作时必须设置为NULL.


## 1.2 代码示例
### 1.2.1 获取串口号
若使用的是沁恒微的CH34X USB转串口芯片产品，可通过调用CH341PT.DLL库中的接口，识别出为其分配的串口和监测串口设备的插拔行为。
下面给出一段调用示例以供参考：

```cpp
// 方法1 ： 通过串口句柄来识别串口， 使用的库函数CH341PtHandleIsCH341()
...
 for (j=1;j<21;j++) 
 {
 	// fullportname为完整的串口名字符串，格式为"\\\\.\\COMxx"
	porthandle=CreateFile((CHAR *)fullportname, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL );
	if ( porthandle != INVALID_HANDLE_VALUE )
	{
		if(CH341PtHandleIsCH341(porthandle))
			// ... 识别到位CH34X串口
		CloseHandle(porthandle);
	}
}
...
```

```cpp
// 方法2 ： 通过串口名称来识别串口， 使用的库函数CH341PtNameIsCH341()
...
 for (j=0;j<20;j++) 
 {
	// fullportnamebuf数组中元素为完整的串口名字符串，从COM1开始枚举
	if(CH341PtNameIsCH341(fullportnamebuf[j]))
	{							
		// ... 	识别到位CH34X串口
	}
}
...
```

### 1.2.2 打开串口（同步通信）
```cpp
HANDLE m_hCom;
m_hCom = CreateFile("COM1",                 //串口名，COM10及以上的串口名格式应为："\\\\.\\COM10"
		GENERIC_READ|GENERIC_WRITE, //允许读或写
		0,			    //独占方式
		NULL,
		OPEN_EXISTING,		    //打开而不是创建
		NULL,			    //同步方式
		NULL );
```

### 1.2.3 打开串口（异步通信）
```cpp
HANDLE m_hCom;
m_hCom = CreateFile("\\\\.\\COM11",         //串口名，COM10及以上的串口名格式应为："\\\\.\\COM10"
		GENERIC_READ|GENERIC_WRITE, //允许读或写
		0,			    //独占方式
		NULL,
		OPEN_EXISTING,		    //打开而不是创建
		FILE_FLAG_OVERLAPPED,	    //异步方式
		NULL );
```
# 2. 关闭串口
调用CloseHandle()函数来关闭串口，函数参数为串口句柄。

```cpp
BOOL WINAPI CloseHandle(HANDLE hObject);
```

# 3. 配置串口
串口成功打开后，就可以开始对串口属性进行配置。
## 3.1 配置输入输出缓冲区
我们可以根据需求用SetupComm()来配置输入输出缓冲区：

```cpp
BOOL WINAPI SetupComm(
    __in HANDLE hFile,		//串口句柄
    __in DWORD dwInQueue,	//输入缓冲区大小
    __in DWORD dwOutQueue	//输出缓冲区大小
    );
```
其中缓冲区大小以字节为单位，也可以不调用该函数，Windows系统也会分配默认的发送和接收缓冲区
## 3.2 配置超时时间
==超时有两种，一个叫间隔超时，另一个叫总超时。==

读端口有间隔超时和总超时设置，写端口只有总超时设置。

要查询当前的超时设置应调用GetCommTimeouts函数，该函数会填充一个COMMTIMEOUTS结构。调用SetCommTimeouts可以用某一个COMMTIMEOUTS结构的内容来设置超时。

<kbd>COMMTIMEOUTS</kbd>结构介绍：

```cpp
typedef struct _COMMTIMEOUTS {
    DWORD ReadIntervalTimeout;          /* Maximum time between read chars. */
    DWORD ReadTotalTimeoutMultiplier;   /* Multiplier of characters.        */
    DWORD ReadTotalTimeoutConstant;     /* Constant in milliseconds.        */
    DWORD WriteTotalTimeoutMultiplier;  /* Multiplier of characters.        */
    DWORD WriteTotalTimeoutConstant;    /* Constant in milliseconds.        */
} COMMTIMEOUTS,*LPCOMMTIMEOUTS;
```
<kbd>ReadIntervalTimeout</kbd>为读操作时两个字符间的间隔超时；

<kbd>ReadTotalTimeoutMultiplier</kbd>为读操作在读取每个字符时的超时；

<kbd>ReadTotalTimeoutConstant</kbd>为读操作的固定超时；

<kbd>WriteTotalTimeoutMultiplier</kbd>为写操作在写每个字符时的超时；

<kbd>WriteTotalTimeoutConstant</kbd>为写操作的固定超时；

==读操作的间隔超时 = ReadIntervalTimeout==
==读操作的总超时 = ReadTotalTimeoutConstant + ReadTotalTimeoutMultiplier*要读的字符数==
==写操作的总超时 = WriteTotalTimeoutConstant + WriteTotalTimeoutMultiplier*要写的字符数==


需要注意的是，对于异步读写时，SetCommTimeouts()仍然是起作用的，在这种情况下，超时规定的是I/O操作的完成时间，而不是ReadFile和WriteFile的返回时间。

## 3.3 串口的属性配置
在配置串口波特率，数据位，奇偶校验，停止位等属性时，首先调用GetCommState获取一个DCB结构：

```cpp
DCB dcb;
GetCommState(m_hCom, &dcb); // m_hCom是串口句柄
```
然后对dcb结构体中的成员进行赋值：

```cpp
dcb.BaudRate = 115200; 		// 波特率
dcb.ByteSize = 8; 	        // 8位数据位
dcb.StopBits = ONESTOPBIT;	// 1位停止位
				// | ONESTOPBIT | 1位停止位 |
				// | ONE5STOPBITS | 1.5位停止位 |
				// | TWOSTOPBITS | 2位停止位 |
dcb.Parity = NOPARITY; 	        // 无校验 
				// | EVENPARITY | 偶校验 |
				// | MARKPARITY | 标号校验 |
				// | NOPARITY | 无校验 |
				// | ODDPARITY | 奇校验 |
				// | SPACEPARITY | 空格校验 |
```
其中：

<kbd>BaudRate</kbd>用于指定串口设备通信的数据传输速率。

<kbd>ByteSize</kbd>指定端口当前使用的数据位数。

<kbd>Parity</kbd>用于指定端口当前使用的奇偶校验方式。

<kbd>StopBits</kbd>用于指定串口当前使用的停止位数。

如想查看完整DCB结构说明，请点击[https://docs.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-dcb](https://docs.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-dcb)


# 4. 同步方式读写串口
## 4.1 函数介绍
在串口打开并配置好之后，可以开始读写串口，读写之前，需要清空串口缓冲区和清除错误，代码参考如下：

```cpp
PurgeComm(m_hCom, PURGE_TXCLEAR|PURGE_RXCLEAR|PURGE_TXABORT|PURGE_RXABORT);	// 清空串口缓冲区，终止发送接收异步操作
DWORD dwError;
if (!ClearCommError(g_hCom, &dwError, NULL))
{
	...
}
```
完成后，可以调用ReadFile和WriteFile来对串口进行读写操作，API原型如下：

```cpp
BOOL ReadFile(
  HANDLE       hFile,			// 串口句柄
  LPVOID       lpBuffer,		// 读缓冲区
  DWORD        nNumberOfBytesToRead,	// 要求读入的字节数
  LPDWORD      lpNumberOfBytesRead,	// 实际读入的字节数
  LPOVERLAPPED lpOverlapped		// 重叠结构
); 
```
```cpp
BOOL WriteFile(
  HANDLE  hFile,			//文件句柄
  LPCVOID lpBuffer,			//数据缓存区指针
  DWORD   nNumberOfBytesToWrite,	//你要写的字节数
  LPDWORD lpNumberOfBytesWritten,	//用于保存实际写入字节数的存储区域的指针
  LPOVERLAPPED lpOverlapped		//OVERLAPPED结构体指针
) ;
```

## 4.2 代码示例
下面给出一个简单的同步通信的控制台程序示例：
```cpp
// ComDemo.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <windows.h>
#include <iostream>

// 全局变量
HANDLE h_Com; // 串口句柄

// 打开串口
void ConnectCom(LPCTSTR ComName)
{
	COMMTIMEOUTS TimeOuts; //串口设置超时结构体
	DCB		dcb;

	//打开一个串口设备
	h_Com = CreateFile(ComName, GENERIC_READ | GENERIC_WRITE, 0, NULL,OPEN_EXISTING, NULL, NULL); 

	if (h_Com == INVALID_HANDLE_VALUE)
	{
		std::cout << "串口无法打开" << std::endl;
		return;
	}

	SetupComm(h_Com, 4096, 4096); //设置输入输出缓冲
	// 超时设置
	TimeOuts.ReadIntervalTimeout = 100;
	TimeOuts.ReadTotalTimeoutMultiplier = 500;
	TimeOuts.ReadTotalTimeoutConstant = 0;

	TimeOuts.WriteTotalTimeoutMultiplier = 0;
	TimeOuts.WriteTotalTimeoutConstant = 0;
	
	SetCommTimeouts(h_Com, &TimeOuts);
	// 设置串口属性
	GetCommState(h_Com, &dcb); //串口属性配置
	dcb.BaudRate = 115200;
	dcb.ByteSize = 8;
	dcb.StopBits = ONESTOPBIT;
	dcb.Parity = NOPARITY;

	if (!SetCommState(h_Com, &dcb))
	{
		CloseHandle(h_Com);
		std::cout << "串口配置失败" << std::endl;
		return ;
	}
	PurgeComm(h_Com, PURGE_TXCLEAR | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_RXABORT);//清空串口缓冲区
}

int main()
{
	ConnectCom(L"COM7"); // 打开指定串口并进行配置
	
	// 发送
	DWORD dwError;
	DWORD dwSend = 0;
	const char* pSendBuf = "test send";
	
	if (ClearCommError(h_Com, &dwError, NULL))
	{
		PurgeComm(h_Com, PURGE_TXABORT | PURGE_TXCLEAR);
	}
	if (!WriteFile(h_Com, pSendBuf, strlen(pSendBuf), &dwSend,NULL))
	{
		std::cout << "发送失败" << std::endl;
	}

	std::cout << "已发送: ";
	std::cout << dwSend << "个字节" << std::endl;

	// 数据接收
	DWORD dwWantRead = 100;
	DWORD dwRead = 0;
	char* pReadBuf = new char[100];
	memset(pReadBuf, 0, 100);

	if (ClearCommError(h_Com, &dwError, NULL))
	{
		PurgeComm(h_Com, PURGE_RXABORT | PURGE_RXCLEAR);
	}

	if (!ReadFile(h_Com, pReadBuf, dwWantRead, &dwRead, NULL))
	{
		std::cout << "读取失败" << std::endl;
	}
	std::cout << "读取的内容: ";
	std::cout << pReadBuf << std::endl;
	delete[] pReadBuf;
}


```

# 5. 异步读写数据
异步（重叠）I/O操作指的是应用程序可以在后台读或是写数据，需要使用OVERLAPPED结构。
接下来，会重点介绍异步操作的具体步骤。

## 5.1 异步操作介绍

1.首先要使用OVERLAPPED结构，==CreateFile()函数的dwFlagsAndAttributes参数必须设为FILE_FLAG_OVERLAPPED,调用读写串口时，也必须在参数中指定OVERLAPPED结构。==

2.OVERLAPPED结构类型说明如下：
```cpp
typedef struct _OVERLAPPED {
  ULONG_PTR Internal;
  ULONG_PTR InternalHigh;
  union {
    struct {
      DWORD Offset;
      DWORD OffsetHigh;
    } DUMMYSTRUCTNAME;
    PVOID Pointer;
  } DUMMYUNIONNAME;
  HANDLE    hEvent;
} OVERLAPPED, *LPOVERLAPPED;
```
其中，hEvent表示I/O操作完成后触发的事件（信号），其余完整参数说明请参考：[https://docs.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-overlapped](https://docs.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-overlapped)

3.设置了异步I/O操作后，I/O操作和函数返回有以下两种情况：

	① 函数返回时I/O操作已完成
	
	② 函数返回时I/O操作还未完成：此时一方面，函数返回值为FALSE，并且GetLastError函数返回ERROR_IO_PENDING;另一方面，系统把OVERLAPPED中的信号事件设为无信号状态。当I/O操作完成时，系统要把它设为信号状态。

4.异步I/O操作可以由==GetOverLappedResult()函数==来获取结果，也可以使用==等待函数==来查询事件当前状态或等待Windows状态信号。

关于GetOverLappedResult()详细说明，请参看以下链接：[https://docs.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getoverlappedresult](https://docs.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getoverlappedresult)

## 5.2 代码示例
下面给出一个简单的异步通讯的控制台程序作为参考：

```cpp
// ComDemo.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <windows.h>
#include <iostream>

// 全局变量
HANDLE h_Com; // 串口句柄

// 打开串口
void ConnectCom(LPCTSTR ComName)
{
	COMMTIMEOUTS TimeOuts; //串口设置超时结构体
	DCB		dcb;

	//打开一个串口设备
	h_Com = CreateFile(ComName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL); // 重叠方式

	if (h_Com == INVALID_HANDLE_VALUE)
	{
		std::cout << "串口无法打开" << std::endl;
		return;
	}

	SetupComm(h_Com, 4096, 4096); // 设置输入输出缓冲

	// 超时设置
	TimeOuts.ReadIntervalTimeout = 100;
	TimeOuts.ReadTotalTimeoutMultiplier = 500;
	TimeOuts.ReadTotalTimeoutConstant = 0;

	TimeOuts.WriteTotalTimeoutMultiplier = 0;
	TimeOuts.WriteTotalTimeoutConstant = 0;

	SetCommTimeouts(h_Com, &TimeOuts);

	// 设置串口属性
	GetCommState(h_Com, &dcb);	//串口属性配置
	dcb.BaudRate = 115200;
	dcb.ByteSize = 8;
	dcb.StopBits = ONESTOPBIT;
	dcb.Parity = NOPARITY;

	if (!SetCommState(h_Com, &dcb))
	{
		CloseHandle(h_Com);
		std::cout << "串口配置失败" << std::endl;
		return ;
	}


	PurgeComm(h_Com, PURGE_TXCLEAR | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_RXABORT);//清空串口缓冲区
}

int main()
{
	ConnectCom(L"COM7"); // 打开指定串口并进行配置

	// 建立一个重叠结构
	OVERLAPPED wrOverlapped;
	ZeroMemory(&wrOverlapped, sizeof(wrOverlapped));
	if (wrOverlapped.hEvent != NULL)
	{
		ResetEvent(wrOverlapped.hEvent);
		wrOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	}

	// 发送
	DWORD dwError;
	DWORD dwSend = 0;
	const char* pSendBuf = "test send";
	
	if (ClearCommError(h_Com, &dwError, NULL))
	{
		PurgeComm(h_Com, PURGE_TXABORT | PURGE_TXCLEAR);
	}
	if (!WriteFile(h_Com, pSendBuf, strlen(pSendBuf), &dwSend, &wrOverlapped))
	{
		if (GetLastError() == ERROR_IO_PENDING)
		{
			while (!GetOverlappedResult(h_Com, &wrOverlapped, &dwSend, FALSE))
			{
				if (GetLastError() == ERROR_IO_INCOMPLETE)
				{
					continue;
				}
				else
				{
					std::cout << "发送失败" << std::endl;
					ClearCommError(h_Com, &dwError, NULL);
					break;
				}
			}
		}
	}

	std::cout << "已发送: ";
	std::cout << dwSend << "个字节" << std::endl;

	// 数据接收
	DWORD dwWantRead = 100;
	DWORD dwRead = 0;
	char* pReadBuf = new char[100];
	memset(pReadBuf, 0, 100);

	if (ClearCommError(h_Com, &dwError, NULL))
	{
		PurgeComm(h_Com, PURGE_RXABORT | PURGE_RXCLEAR);
	}

	if (!ReadFile(h_Com, pReadBuf, dwWantRead, &dwRead, &wrOverlapped))
	{
		if (dwError = GetLastError() == ERROR_IO_PENDING)
		{
			while (!GetOverlappedResult(h_Com, &wrOverlapped, &dwRead, FALSE))
			{
				if (GetLastError() == ERROR_IO_INCOMPLETE)
				{
					continue;
				}
				else
				{
					std::cout << "接收失败" << std::endl;
					ClearCommError(h_Com, &dwError, NULL);
					return 0;
				}
			}
		}
	}
	std::cout << "读取的内容: ";
	std::cout << pReadBuf << std::endl;
	delete[] pReadBuf;
}


```

# 6. 通信事件
Windows进程中监视发生在通信资源中的一组事件，这样应用程序可以不检查端口状态就可以知道某些条件何时发生。
Windows通信事件列表如下：
|值| 描述 |
|--|--|
| EV_BREAK | 检测到输入的终止 |
| EV_CTS | CTS(清除发送)信号改变状态 |
| EV_DSR | DSR(数据设置就绪)信号改变状态 |
| EV_ERR | 发生了线路状态错误，线路状态错误时CE_FRAME(帧错误)，CE_OVERRUN(接收缓冲区超限)和CE_RXPARITY(奇偶校验错误)
| EV_RING | 检测到振铃 |
| EV_RLSD | RLSD(接收线路信号检测)信号改变状态 |
| EV_RXCHAR | 接收到一个字符，并放入输入缓冲区 |
| EV_RXFLAG | 接收到事件字符(DCB结构的EvtChar成员)，并放入输入缓冲区 |
| EV_TXEMPTY | 输出缓冲区中最后一个字符发送出去 |

==1. 操作通信事件==
应用程序可以通过SetCommMask()函数建立事件掩模来监视指定通信资源上的事件。SetCommMask()函数的声明如下：

```cpp
BOOL SetCommMask(
  HANDLE hFile,
  DWORD  dwEvtMask
);
```
其中 dwEvtMask标识被监视的通信事件，其值可以是上表中的任意通信事件组合。

如果想获取特定通信资源的当前事件掩模，可以使用GetCommMask()函数。其函数声明如下：

```cpp
BOOL GetCommMask(
  HANDLE  hFile,
  LPDWORD lpEvtMask
);
```

==2. 监视通信事件==
在用SetCommMask()指定了有用的事件后，应用程序就调用WaitCommEvent()函数来等待其中一个事件发生。其函数声明如下：

```cpp
BOOL WaitCommEvent(
  HANDLE       hFile,
  LPDWORD      lpEvtMask,
  LPOVERLAPPED lpOverlapped
);
```
其参数的详细说明，请参见：[https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-waitcommevent](https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-waitcommevent)

当lpOverlapped参数指向了一个OVERLAPPED结构，并且打开hfile参数标识的通信设备时指定了FILE_FLAG_OVERLAPPED标志，则WaitCommEvent()函数以异步操作实现。

这种情况下，OVERLAPPED结构体必须含有一个人工复位事件的句柄，当异步操作不能立即实现时，WaitCommEvent()返回false，且GetLastError()函数返回ERROR_IO_PENDING。此时，在该函数返回前，系统将OVERLAPPED结构中的hEvent参数设置为无信号状态；当指定事件发生后，再置为有信号状态。

==操作通信事件的具体代码见下章节--API串口通讯示例。==

# 7. API串口通讯代码示例
接下来我们提供了一个完整的API串口通讯的控制台程序示例。
[https://download.csdn.net/download/WCH_TechGroup/12228991](https://download.csdn.net/download/WCH_TechGroup/12228991)

也可在github上下载我们的示例程序。
[https://github.com/PengKun-PK/SerialPortCommunication](https://github.com/PengKun-PK/SerialPortCommunication)
