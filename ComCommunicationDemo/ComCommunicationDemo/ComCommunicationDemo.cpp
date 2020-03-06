// ComCommunicationDemo.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <wtypes.h>
#include "SerialCom.h"
#include <string>
#include <algorithm>
#include "CH341PT.H"
#pragma comment (lib, "CH341PT")

void deleteAllMark(std::string& s, const std::string& mark)
{
	size_t nSize = mark.size();
	while (1)
	{
		size_t pos = s.find(mark);
		if (pos == std::string::npos)
		{
			return;
		}

		s.erase(pos, nSize);
	}
}

char * transfor(char * parimary_string, int flag)
{
	if (flag != 0) // HEX 形式
	{
		char * temp = new char[100];
		unsigned int number;
		for (int i = 0; i < 100; i++)
			temp[i] = parimary_string[i];
		
		bool start_flag = false;
		bool last_room = false;
		for (int i = 0, j = 0; i < 100;)
		{
			if (temp[i] == '\0')
			{
				parimary_string[j] = '\0';
				break;
			}
			if (temp[i] == ' ')
			{
				if (!last_room&&start_flag)
					j++;
				else
					last_room = true;
				i++;
			}
			if (temp[i] != ' ')
			{
				last_room = false;
				start_flag = true;
				if (temp[i] < 60)
					number = temp[i] - '0';
				else
					number = (temp[i] - 'A' + 10);
				i++;
				number *= 16;
				if (temp[i] < 60)
					number += (temp[i] - '0');
				else
					number += (temp[i] - 'A' + 10);
				parimary_string[j] = (char)number;
				i++;
				j++;
			}
		}
		delete[] temp;
	}
	return parimary_string;
};

int main()
{
	SerialCom serialCom; // 实例化对象
	std::string OrderStr = "";
	int ports, nbyte;
	//////////////////////////////////////////////////////////////////////////
	// 先通过CH341PTDLL枚举串口
	char* portName = new char[10];
	char* fullportName = new char[20];
	HANDLE porthandle = nullptr;
	std::cout << "Find CH34X Com " << std::endl;
	for (int j = 1; j < 21; j++) {
		sprintf_s(portName, 10, "COM%d\0", j);                 //设备名
		sprintf_s(fullportName, 20, "\\\\.\\%s\0", portName);  //完整的设备名
		porthandle = CreateFileA(fullportName,
			GENERIC_READ | GENERIC_WRITE,              // 打开设备,支持重叠操作
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
		if (porthandle != INVALID_HANDLE_VALUE) {
			if (CH341PtHandleIsCH341(porthandle))
				std::cout << portName << std::endl;
			CloseHandle(porthandle);
		}
	}
	delete[] portName;
	delete[] fullportName;
	//////////////////////////////////////////////////////////////////////////

	std::cout << "please input a com port\n";	// 端口号
	std::cin >> ports;
	serialCom.init_port(ports); // 打开指定端口，默认波特率9600
	serialCom.start_monitoring(); // 开启串口监视线程
	std::cout << "please input the Data Format : 0(ASCII) or 1(Hex)!\n";
	std::cin >> nbyte; // 确定输入格式为字符串or16进制字符串
	char* temp = new char[100];
	std::cin.ignore(); // 清空回车

	while (true)
	{
		std::cout << "please input word, less then 100 words\n";
		std::getline(std::cin, OrderStr); // 可输入空格
		deleteAllMark(OrderStr, " ");
		// 小写变大写
		for (auto& i : OrderStr)
		{
			if (i <= 'z' && i >= 'a')
			{
				i -= 'a' - 'A';
			}
		}
		temp = (char*)OrderStr.c_str();
		temp = transfor(temp, nbyte);
		serialCom.write_to_port(temp);
	}
	delete[] temp;
	return 0;
}


