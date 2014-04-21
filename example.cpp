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

	boost::optional<std::wstring> bHtml = mClient( argv[1] );

}
