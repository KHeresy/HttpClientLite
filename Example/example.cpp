#include <iostream>

#include "HttpClient.h"

int main(int argc, char** argv )
{
	if (argc < 2)
	{
		std::cerr << "Please give a web URL" << std::endl;
		return -1;
	}

	HttpClientLite::URL mUrl(argv[1]);
	if (mUrl)
	{
		std::cout << "Try to open <" << mUrl.toString() << std::endl;

		HttpClientLite::Client	mClient;
		mClient.m_sigErrorLog.connect([](const std::string& sError) {
			std::cout << "[Error] " << sError << std::endl;
		});

		auto pSession = mClient.Connect(mUrl);
		pSession->Request();
		auto res = pSession->Read();
		std::wcout << pSession->GetBody(res).value() << std::endl;
		pSession->Close();

		//std::optional<std::wstring> bHtml = mClient.ReadHtml(argv[1]);
		//if (bHtml)
		//{
		//	std::wcout << bHtml.value();
		//}
		//else
		//{
		//	std::cout << "Error" << std::endl;
		//}
	}
}
