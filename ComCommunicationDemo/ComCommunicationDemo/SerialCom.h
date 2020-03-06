#pragma once
#include <wtypes.h>
#include "iostream"
#include <assert.h>
#include <windows.h>

class SerialCom
{
public:
	SerialCom(void);
	~SerialCom();
	int m_nwrite_size;
	void close_port();
	bool init_port(unsigned int port = 1, unsigned int baud = 9600, char parity = 'N', unsigned int databits = 8, unsigned int stopbits = 1, DWORD dwCommEvents = EV_RXCHAR, unsigned int write_buffer_size = 1024);
	HANDLE m_comm;
	bool start_monitoring();
	bool restart_monitoring();
	bool stop_monitoring();

	DWORD get_write_buffer_size();
	DWORD get_comm_events();
	DCB get_dcb();
	void write_to_port(const char* string);
	void write_to_port(const char* string, int n);
	void write_to_port(std::string string);
	//void write_to_port(LPCTSTR string);
	//void write_to_port(LPCTSTR string,int n);

protected:
	void process_error_message(char * error_text);
	static DWORD WINAPI comm_thread(LPVOID pParam);
	static void receive_char(SerialCom* port, COMSTAT comstat);
	static void send_char(SerialCom* port);

	HANDLE m_thread;
	HANDLE m_write_event;
	HANDLE m_shutdown_event;
	HANDLE m_event_array[3];
	CRITICAL_SECTION m_mm_sync;
	bool    m_thread_alive;
	OVERLAPPED m_overlapped;
	COMMTIMEOUTS m_comm_time_out;
	DCB m_dcb;
	unsigned int m_port;
	char * m_sz_write_buffer;
	DWORD m_dw_commEvents;
	DWORD m_n_write_size;

};
