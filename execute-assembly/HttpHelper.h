#pragma once
class HttpHelper
{
public:
	HttpHelper();
	~HttpHelper();
	char* HttpRequest(char* Url, int* lenght);
};

