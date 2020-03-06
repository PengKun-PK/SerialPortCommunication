#include "pch.h"
#include "SerialCom.h"



SerialCom::SerialCom(void)
{
	m_comm = NULL;
	m_overlapped.Offset = 0;
	m_overlapped.OffsetHigh = 0;
	m_overlapped.hEvent = NULL;
	m_write_event = NULL;
	m_shutdown_event = NULL;
	m_sz_write_buffer = NULL;
	m_n_write_size = 1;
	m_thread_alive = FALSE;
	m_thread = NULL;
}


SerialCom::~SerialCom(void)
{
	do
	{
		SetEvent(m_shutdown_event);
	} while (m_thread_alive);
	delete[] m_sz_write_buffer;
}


bool SerialCom::init_port(unsigned int port /* =1 */, unsigned int baud/* =9600 */, char parity/* ='N' */, unsigned int databits/* =8 */, unsigned int stopbits /* =1 */, DWORD dwCommEvents /* = EV_RXCHAR */, unsigned int write_buffer_size/* =1024 */)
{
	if (m_thread_alive)
	{
		do
		{
			SetEvent(m_shutdown_event);
		} while (m_thread_alive);
	}
	if (m_overlapped.hEvent != NULL)
		ResetEvent(m_overlapped.hEvent);
	m_overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (m_write_event != NULL)
		ResetEvent(m_write_event);
	m_write_event = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (m_shutdown_event != NULL)
		ResetEvent(m_shutdown_event);
	m_shutdown_event = CreateEvent(NULL, TRUE, FALSE, NULL);

	m_event_array[0] = m_shutdown_event;
	m_event_array[1] = m_overlapped.hEvent;
	m_event_array[2] = m_write_event;
	InitializeCriticalSection(&m_mm_sync);
	m_port = port;
	if (m_sz_write_buffer != NULL)
		delete[]m_sz_write_buffer;
	m_sz_write_buffer = new char[write_buffer_size];
	m_n_write_size = write_buffer_size;
	m_dw_commEvents = dwCommEvents;
	bool bResult = false;
	WCHAR * szPort = new WCHAR[50];
	WCHAR * szBaud = new WCHAR[50];
	EnterCriticalSection(&m_mm_sync);
	if (m_comm != NULL)
	{
		CloseHandle(m_comm);
		m_comm = NULL;
	}
	if (port < 10)
	{
		wsprintf(szPort, (L"COM%d"), port);
	} 
	else
	{
		wsprintf(szPort, (L"\\\\.\\COM%d"), port);
	}
	
	wsprintf(szBaud, (L"baud=%d parity=%c data=%d stop=%d"), baud, parity, databits, stopbits);
	m_comm = CreateFile(szPort, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
	if (m_comm == INVALID_HANDLE_VALUE)
	{
		delete[] szPort;
		delete[] szBaud;
		return false;
	}
	m_comm_time_out.ReadIntervalTimeout = 100;
	m_comm_time_out.ReadTotalTimeoutMultiplier = 500;
	m_comm_time_out.ReadTotalTimeoutConstant = 0;
	m_comm_time_out.WriteTotalTimeoutMultiplier = 1000;
	m_comm_time_out.WriteTotalTimeoutConstant = 0;
	if (SetCommTimeouts(m_comm, &m_comm_time_out))
	{
		if (SetCommMask(m_comm, dwCommEvents))
		{
			if (GetCommState(m_comm, &m_dcb))
			{
				m_dcb.EvtChar = 'q';
				m_dcb.fRtsControl = RTS_CONTROL_ENABLE;
				if (BuildCommDCB(szBaud, &m_dcb))
				{
					if (SetCommState(m_comm, &m_dcb))
						;
					else
						process_error_message((char*)"SetCommState()");
				}
				else
					process_error_message((char*)"BuildCommDCB()");
			}
			else
				process_error_message((char*)"GetCommState()");
		}
		else
			process_error_message((char*)"SetCommMask");
	}
	else
		process_error_message((char*)"SetCommTimeouts()");
	delete[] szPort;
	delete[] szBaud;

	PurgeComm(m_comm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);
	LeaveCriticalSection(&m_mm_sync);

	return true;

}


bool SerialCom::start_monitoring()
{
	m_thread = CreateThread(NULL, 0, comm_thread, (LPVOID)this, CREATE_SUSPENDED, NULL);
	if (m_thread == INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_thread);
		return false;
	}
	else
	{
		ResumeThread(m_thread);
	}

	return true;
}


bool SerialCom::restart_monitoring()
{
	stop_monitoring();
	return start_monitoring();
}


bool SerialCom::stop_monitoring()
{
	do
	{
		SetEvent(m_shutdown_event);
	} while (m_thread_alive);
	return true;
}


void SerialCom::process_error_message(char * error_text)
{
	char *Temp = new char[200];

	LPVOID lpMsgBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
	);
	
	sprintf_s(Temp, 200, "WARNING:  %s Failed with the following error: /n%s/nPort: %d/n", (char*)error_text, lpMsgBuf, m_port);
	std::cout << Temp << std::endl;

	LocalFree(lpMsgBuf);
	delete[] Temp;
}


DCB SerialCom::get_dcb()
{
	return m_dcb;
}


DWORD SerialCom::get_comm_events()
{
	return this->m_dw_commEvents;
}


DWORD SerialCom::get_write_buffer_size()
{
	return this->m_n_write_size;
}


void SerialCom::close_port()
{
	SetEvent(m_shutdown_event);
}


void SerialCom::write_to_port(const char* string)
{
	assert(m_comm != 0);

	memset(m_sz_write_buffer, 0, sizeof(m_sz_write_buffer));
	m_n_write_size = strlen(string);
	memcpy(m_sz_write_buffer, string, m_n_write_size);

	// set event for write
	SetEvent(m_write_event);
}


void SerialCom::write_to_port(std::string string)
{
	assert(m_comm != 0);
	memset(m_sz_write_buffer, 0, sizeof(m_sz_write_buffer));
	m_n_write_size = string.size();
	memcpy(m_sz_write_buffer, string.c_str(), m_n_write_size);
	// set event for write
	SetEvent(m_write_event);
}


void SerialCom::write_to_port(const char* string, int n)
{
	assert(m_comm != 0);

	memset(m_sz_write_buffer, 0, sizeof(m_sz_write_buffer));
	memcpy(m_sz_write_buffer, string, n);
	m_n_write_size = n;

	// set event for write
	SetEvent(m_write_event);
}


//void CConsoleSerialPort::write_to_port(LPCTSTR string)
//{
//	assert(m_comm != 0);
//
//	memset(m_sz_write_buffer, 0, sizeof(m_sz_write_buffer));
//	sprintf(m_sz_write_buffer,"%s",string);
//	m_n_write_size=strlenT(string);
//
//	// set event for write
//	SetEvent(m_write_event);
//}
//
//
//void CConsoleSerialPort::write_to_port(LPCTSTR string,int n)
//{
//	assert(m_comm != 0);
//
//	memset(m_sz_write_buffer, 0, sizeof(m_sz_write_buffer));
//	memcpy(m_sz_write_buffer, string,n);
//	m_n_write_size=n;
//
//	// set event for write
//	SetEvent(m_write_event);
//}


void SerialCom::send_char(SerialCom* port)
{
	bool bWrite = true;
	bool bResult = true;
	DWORD BytesSent = 0;
	ResetEvent(port->m_write_event);
	EnterCriticalSection(&port->m_mm_sync);
	if (bWrite)
	{
		port->m_overlapped.Offset = 0;
		port->m_overlapped.OffsetHigh = 0;
		PurgeComm(port->m_comm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);
		bResult = WriteFile(port->m_comm, port->m_sz_write_buffer, port->m_n_write_size, &BytesSent, &port->m_overlapped);
		if (!bResult)
		{
			DWORD dwError = GetLastError();
			switch (dwError)
			{
			case ERROR_IO_PENDING:
			{
				// continue to GetOverlappedResults()
				BytesSent = 0;
				bWrite = false;
				break;
			}
			default:
			{
				// all other error codes
				port->process_error_message((char*)"WriteFile()");
				bWrite = false;
			}
			}
		}
		else
		{
			LeaveCriticalSection(&port->m_mm_sync);
		}
	}
	if (!bWrite)
	{
		bWrite = true;
		bResult = GetOverlappedResult(port->m_comm, // Handle to COMM port 
			&port->m_overlapped,  // Overlapped structure
			&BytesSent,  // Stores number of bytes sent
			true);    // Wait flag
		LeaveCriticalSection(&port->m_mm_sync);
	}
}


void SerialCom::receive_char(SerialCom* port, COMSTAT comstat)
{
	bool bRead = true;
	bool bResult = true;
	DWORD dwError = 0;
	DWORD BytesRead = 0;
	unsigned char RXBuff;
	std::string buffs;
	buffs.clear();
	for (;;)
	{
		EnterCriticalSection(&port->m_mm_sync);
		bResult = ClearCommError(port->m_comm, &dwError, &comstat);
		LeaveCriticalSection(&port->m_mm_sync);
		if (comstat.cbInQue == 0)
			break;
		EnterCriticalSection(&port->m_mm_sync);
		if (bRead)
		{
			bResult = ReadFile(port->m_comm, &RXBuff, 1, &BytesRead, &port->m_overlapped);
			if (!bResult)
			{
				dwError = GetLastError();
				switch (dwError)
				{
				case ERROR_IO_PENDING:
				{
					bRead = FALSE;
					break;
				}
				default:
				{
					port->process_error_message((char*)"ReadFile()");
					break;
				}
				}
			}
			else
				bRead = true;
		}
		if (!bRead)
		{
			bRead = true;
			bResult = GetOverlappedResult(port->m_comm, &port->m_overlapped, &BytesRead, true);
			if (!bResult)
				port->process_error_message((char*)"GetOverlappedResults() in ReadFile()");
		}
		LeaveCriticalSection(&port->m_mm_sync);
		buffs += RXBuff;
	}
	int totals = buffs.size();
	const char* temps = buffs.c_str();
	char temp;
	unsigned char sum;
	int high, low;
	std::cout << "Receive :";
	for (int i = 0; i < totals; i++)
	{
		printf("%02X ", (unsigned char)temps[i]);
	}
	printf("\n");
}


DWORD WINAPI SerialCom::comm_thread(LPVOID pParam)
{
	SerialCom *port = (SerialCom*)pParam;
	port->m_thread_alive = true;
	DWORD BytesTransfered = 0;
	DWORD Event = 0;
	DWORD CommEvent = 0;
	DWORD dwError = 0;
	COMSTAT comstat;
	bool bResult = true;
	if (port->m_comm)
		PurgeComm(port->m_comm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);
	for (;;)
	{
		bResult = WaitCommEvent(port->m_comm, &Event, &port->m_overlapped);
		if (!bResult)
		{
			dwError = GetLastError();
			switch (dwError)
			{
			case ERROR_IO_PENDING:
				break;
			case  87:
				break;
			default:
			{
				port->process_error_message((char*)"WaitCommEvent()");
				break;
			}
			}
		}
		else
		{
			bResult = ClearCommError(port->m_comm, &dwError, &comstat);
			if (comstat.cbInQue == 0)
				continue;
		}
		Event = WaitForMultipleObjects(3, port->m_event_array, false, INFINITE);
		switch (Event)
		{
		case 0:
		{
			CloseHandle(port->m_comm);
			port->m_comm = NULL;
			port->m_thread_alive = false;
			return 1;
		}
		case 1:
		{
			GetCommMask(port->m_comm, &CommEvent);
			if (CommEvent & EV_RXCHAR)
			{
				Sleep(20);
				if (ClearCommError(port->m_comm, &dwError, &comstat) && dwError > 0)
				{
					PurgeComm(port->m_comm, PURGE_RXABORT);
				}
				if (comstat.cbInQue)
				{
					receive_char(port, comstat);
				}
				
			}
				
			break;
		}
		case 2:
		{
			send_char(port);
			break;
		}
		}
	}
	return 0;
}