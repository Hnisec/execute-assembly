#include "pch.h"
#include "HttpHelper.h"

#pragma comment(lib, "ws2_32.lib")

HttpHelper::HttpHelper()
{
}


HttpHelper::~HttpHelper()
{
}


int GetHeaderLength(char *content)
{
	const char *srchStr1 = "\r\n\r\n", *srchStr2 = "\n\r\n\r";
	char *findPos;
	int ofset = -1;

	findPos = strstr(content, srchStr1);
	if (findPos != NULL)
	{
		ofset = findPos - content;
		ofset += strlen(srchStr1);
	}
	else
	{
		findPos = strstr(content, srchStr2);
		if (findPos != NULL)
		{
			ofset = findPos - content;
			ofset += strlen(srchStr2);
		}
	}
	return ofset;
}

char* HttpGet(string m_ip, int m_port, string req, int* lenght)
{
	char* ret = NULL;
	try
	{
		WSADATA wData;
		WSAStartup(MAKEWORD(2, 2), &wData);

		SOCKET clientSocket = socket(AF_INET, 1, 0);
		struct sockaddr_in ServerAddr = { 0 };
		ServerAddr.sin_addr.s_addr = inet_addr(m_ip.c_str());
		ServerAddr.sin_port = htons(m_port);
		ServerAddr.sin_family = AF_INET;
		int errNo = connect(clientSocket, (sockaddr*)&ServerAddr, sizeof(ServerAddr));
		if (errNo == 0)
		{
			string strSend = "GET " + req + " HTTP/1.1\r\n\r\n";
			errNo = send(clientSocket, strSend.c_str(), strSend.length(), 0);
			if (errNo <= 0)
			{
				printf("[-] socket send error: %d\n", errNo);
			}
			else
			{
				const int bufSize = 512;
				int totalBytesRead = 0;
				char readBuffer[bufSize];
				char* tmpResult = NULL;
				while (1)
				{
					memset(readBuffer, 0, bufSize);
					int onceReadSize = recv(clientSocket, readBuffer, bufSize, 0);
					if (onceReadSize <= 0)
						break;

					tmpResult = (char*)realloc(tmpResult, onceReadSize + totalBytesRead);
					memcpy(tmpResult + totalBytesRead, readBuffer, onceReadSize);
					totalBytesRead += onceReadSize;
				}

				int headerLen = GetHeaderLength(tmpResult);
				int contenLen = totalBytesRead - headerLen;
				char* result = new char[contenLen + 1];
				memcpy(result, tmpResult + headerLen, contenLen);
				result[contenLen] = 0x0;
				*lenght = contenLen;
				ret = result;
			}

		}
		else
		{
			errNo = WSAGetLastError();
			printf("[-] socket connect error: %d\n", errNo);
		}
		WSACleanup();
	}
	catch (...)
	{
	}
	return ret;
}

void mParseUrl(char *mUrl, string &serverName, string &filename, int &port)
{
	string url = mUrl;
	if (url.substr(0, 7) == "http://") {
		url.erase(0, 7);
		filename = url.substr(url.find('/'));
		string temp = url.substr(0, url.find('/'));

		if (temp.find(":") < temp.length())
		{
			serverName = url.substr(0, temp.find(':'));
			port = stoi(url.substr(temp.find(':') + 1));
		}
		else
		{
			serverName = temp;
			port = 80;
		}
	}
	else
	{
		url.erase(0, 8);
		filename = url.substr(url.find('/'));
		string temp = url.substr(0, url.find('/'));

		if (temp.find(":") < temp.length())
		{
			serverName = url.substr(0, temp.find(':'));
			port = stoi(url.substr(temp.find(':') + 1));
		}
		else
		{
			serverName = temp;
			port = 443;
		}
	}
}

char* HttpHelper::HttpRequest(char* Url, int* lenght) 
{
	string server, filename;
	int port;
	mParseUrl(Url, server, filename, port);
	return HttpGet(server, port, filename, lenght);
}