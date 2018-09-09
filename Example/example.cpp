#include <iostream>

#include "HttpClient.h"

int main(int argc, char** argv )
{
	if (argc < 2)
	{
		std::cerr << "Please give a web URL" << std::endl;
		return -1;
	}

	HttpClient	mClient;
	mClient.m_sigErrorLog.connect([](const std::string& sError) {
		std::cout << "[Error] " << sError << std::endl;
	});

	std::optional<std::wstring> bHtml = mClient.ReadHtml( argv[1] );
	if (bHtml)
	{
		std::wcout << bHtml.value();
	}
	else
	{
		std::cout << "Error" << std::endl;
	}
}
