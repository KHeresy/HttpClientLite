// STL Header
#include <map>
#include <vector>
#include <fstream>
#include <sstream>

// Boost Header
#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>

// Application Header
#include "HttpClient.h"

using namespace std;
using namespace boost::asio::ip;

// Functions of HTNLTag
boost::optional< pair<size_t,size_t> > HTMLTag::GetData( const std::string& sHtmlSource, size_t uBeginPos )
{
	if( m_sTagName != "" )
	{
		string sTagBegin = "<" + m_sTagName;

		// find the position of tag
		size_t uStartPos = sHtmlSource.find( sTagBegin, uBeginPos );
		if( uStartPos != string::npos )
		{
			// find the end of the start tag
			size_t uPos1 = uStartPos + sTagBegin.length();
			size_t uPos2 = sHtmlSource.find( ">", uPos1 );
			if( uPos2 != string::npos )
			{
				// process attribute
				string sAttribStr = sHtmlSource.substr( uPos1, uPos2 - uPos1 );
				boost::trim( sAttribStr );
				size_t uStartPos = 0;
				while( true )
				{
					size_t uPos = sAttribStr.find_first_of( " =", uStartPos );
					if( uPos == string::npos )
						break;

					string sName = sAttribStr.substr( uStartPos, uPos - uStartPos );
					if( sAttribStr[uPos] == '=' )
					{
						if( sAttribStr[uPos+1] == '\"' )
						{
							size_t uEnd = sAttribStr.find_first_of( '\"', uPos + 2 );
							if( uEnd == string::npos )
							{
							}
							else
							{
								m_mAttributes[ sName ] = sAttribStr.substr( uPos + 2, uEnd - uPos - 2 );
								uStartPos = uEnd + 1;
							}
						}
						else
						{
							size_t uEnd = sAttribStr.find_first_of( ' ', uPos + 1 );
							if( uEnd == string::npos )
							{
								m_mAttributes[ sName ] = sAttribStr.substr( uPos + 1 );
								break;
							}
							else
							{
								m_mAttributes[ sName ] = sAttribStr.substr( uPos + 1, uEnd - uPos - 1 );
								uStartPos = uEnd + 1;
							}
						}
					}
					else
					{
						m_mAttributes[ sName ];
						uStartPos = uPos + 1;
					}
				}

				// try to find the close tag only if this tag is not self-closed
				if( sHtmlSource[uPos2-1] != '/' )
				{
					string sTagEnd		= "</" + m_sTagName + ">";
					size_t uPos3 = sHtmlSource.find( sTagEnd, uPos2 + 1 );
					if( uPos3 != std::string::npos )
					{
						m_sContent = sHtmlSource.substr( uPos2 + 1, uPos3 - uPos2 - 1 );
						uPos2 = uPos3 + sTagEnd.size() - 1;
					}
				}

				return make_pair( uStartPos, uPos2 );
			}
		}
	}
	return boost::optional< std::pair<size_t,size_t> >();
}

boost::optional< std::pair<size_t,size_t> > HTMLTag::FindQuoteContent( const std::string& rSource, const char c, size_t uBeginPos )
{
	size_t uPos = rSource.find_first_of( "\'\"", uBeginPos );
	if( uPos != string::npos )
	{
		if( uPos > uBeginPos && rSource[ uPos - 1 ] == '\\' )
		{
		}
	}
	return boost::optional< pair<size_t,size_t> >();
}

// Functions of HTMLParsr
void HTMLParser::FindTag( const std::wstring& rHtml, const std::wstring& rTag, const std::map< std::wstring, boost::optional<std::wstring> >& rAttribute, size_t uStartPos )
{
	std::wstring	sTagBegin	= L"<" + rTag,
					sTagEnd		= L"</" + rTag + L">";
	
	// find the position of tag
	size_t uPos1 = rHtml.find( sTagBegin, uStartPos );
	if( uPos1 == std::string::npos )
	{
		return;
	}
	
	// find the end of the start tag
	size_t uPos2 = rHtml.find( L">", uPos1 + sTagBegin.length() );
	if( uPos2 == std::string::npos )
	{
		return;
	}

	if( rAttribute.size() > 0 )
	{
		// get attributes
		std::wstring sAttributes = rHtml.substr( uPos1 + sTagBegin.length(), uPos2 - ( uPos1 + sTagBegin.length() ) );

		// TODO: check attribute
	}

	// TODO: check nested tag?

	// find close tag
	size_t uPos3 = rHtml.find( sTagEnd, uPos2 + 1 );
	if( uPos3 != std::string::npos )
	{
		std::wstring sContent = rHtml.substr( uPos2 + 1, uPos3 - uPos2 - 1 );
	}
}

std::pair<size_t,wstring> HTMLParser::FindContentBetweenTag( const wstring& rHtml, const pair<wstring,wstring>& rTag, size_t uStartPos )
{
	size_t uPos1 = rHtml.find( rTag.first, uStartPos );
	if( uPos1 != wstring::npos )
	{
		size_t uPos2 = rHtml.find( rTag.second, uPos1 );
		if (uPos2 != wstring::npos)
		{
			uPos1 = uPos1 + rTag.first.length();

			std::wstring sContent = rHtml.substr(uPos1, uPos2 - uPos1);
			boost::algorithm::trim(sContent);
			return make_pair(uPos1, sContent);
		}
	}
	return make_pair( wstring::npos, L"" );
}

boost::optional< pair<wstring,wstring> > HTMLParser::AnalyzeLink( const wstring& rHtml, size_t uStartPos )
{
	size_t uPos1 = rHtml.find( L"<a ", uStartPos );
	if( uPos1 != wstring::npos )
	{
		size_t uPos2 = rHtml.find( L"href=\"", uPos1 );
		if( uPos2 != wstring::npos )
		{
			uPos2 += 6;
			uPos1 = rHtml.find( L"\"", uPos2 );
			if( uPos1 != wstring::npos )
			{
				wstring sLinkUrl = rHtml.substr( uPos2, uPos1 - uPos2 );

				uPos1 = rHtml.find( L">", uPos1 );
				if( uPos1 != wstring::npos )
				{
					uPos1 += 1;
					uPos2 = rHtml.find( L"</a>", uPos1 );
					wstring sLinkText = rHtml.substr( uPos1, uPos2 - uPos1 );
					boost::algorithm::trim( sLinkText );

					return boost::optional< pair<wstring,wstring> >( make_pair( sLinkText, sLinkUrl ) );
				}
			}
		}
	}
	return boost::optional< pair<wstring,wstring> >();
}

// Functions of HttpClient
HttpClient::HttpClient() : m_Resolver( m_IO_service )
{
}

boost::optional<wstring> HttpClient::ReadHtml( const string& rServer, const string& rPath )
{
	// code reference to http://www.boost.org/doc/libs/1_53_0/doc/html/boost_asio/example/iostreams/http_client.cpp
	tcp::iostream sStream;
	sStream.expires_from_now( boost::posix_time::seconds( 300 ) );
	if( SendRequest( rServer, rPath, sStream ) )
	{
		size_t uSize = GetHttpHeader(sStream);
		if (uSize > 0)
		{
			// Write the remaining data to output.
			stringstream oStream;
			oStream << sStream.rdbuf();
			string sHtml = oStream.str();

			string sEncoding = "ANSI";
			size_t uPos = sHtml.find("charset=");
			if (uPos != string::npos)
			{
				auto uEnd = sHtml.find_first_of("'\" ", uPos + 1);
				if (uEnd != string::npos)
				{
					sEncoding = sHtml.substr(uPos + 8, uEnd - uPos - 8);
				}
			}
			return boost::locale::conv::to_utf<wchar_t>(sHtml, sEncoding);
		}
	}
	return boost::optional<wstring>();
}

bool HttpClient::GetBinaryFile( const string& rServer, const string& rPath, const wstring& rFilename )
{
	// code reference to http://www.boost.org/doc/libs/1_53_0/doc/html/boost_asio/example/iostreams/http_client.cpp
	tcp::iostream sStream;
	sStream.expires_from_now( boost::posix_time::seconds( 300 ) );
	if( SendRequest( rServer, rPath, sStream ) )
	{
		size_t uSize = GetHttpHeader(sStream);
		if (uSize > 0)
		{
			std::ofstream outfile(rFilename, ios::binary);
			if (outfile.is_open())
			{
				auto buf = sStream.rdbuf();
				outfile << buf;
				outfile.close();
				return true;
			}
		}
	}
	return false;
}

boost::optional< pair<string,string> > HttpClient::ParseURL( const string& sURL )
{
	if( sURL.length() > 10 )
	{
		if( sURL.substr( 0, 7 ) == "http://" )
		{
			auto uPos = sURL.find_first_of( "/", 8 );
			return boost::optional< pair<string,string> >( make_pair( sURL.substr( 7, uPos - 7 ), sURL.substr( uPos ) ) );
		}
	}
	return boost::optional< pair<string,string> >();
}

boost::optional<std::string> HttpClient::GetFilename( const std::string& sURL )
{
	size_t uPos = sURL.find_last_of( '/' );
	if( uPos != string::npos && uPos != sURL.size() - 1 )
		return sURL.substr( uPos + 1 );

	return boost::optional<string>();
}

bool HttpClient::SendRequest( const string& rServer, const string& rPath, boost::asio::ip::tcp::iostream& rStream )
{
	// Establish a connection to the server.
	m_sigInfoLog( "Connect to " + rServer );
	rStream.connect( rServer, "http" );
	if( !rStream )
	{
		m_sigErrorLog( "Unable to connect: " + rStream.error().message() );
		return false;
	}

	// Send the request.
	m_sigInfoLog( "Request paget " + rPath );
	rStream << "GET " << rPath << " HTTP/1.0\r\n";
	rStream << "Host: " << rServer << "\r\n";
	rStream << "Referer: " <<  rServer << "\r\n";
	rStream << "Accept: */*\r\n" << "Connection: close\r\n\r\n";

	return true;
}

size_t HttpClient::GetHttpHeader( boost::asio::ip::tcp::iostream& rStream )
{
	// Check that response is OK.
	string sHttpVersion;
	rStream >> sHttpVersion;
	unsigned int uCode;
	rStream >> uCode;
	string sMssage;
	getline( rStream, sMssage );
	m_sigInfoLog( "Recive data: " + sHttpVersion + " / " + sMssage );
	if( !rStream || sHttpVersion.substr(0, 5) != "HTTP/" )
	{
		m_sigErrorLog( "Invalid response" );
		return 0;
	}
	if( uCode != 200 )
	{
		m_sigErrorLog( "Response returned with status code " + uCode );
		return 0;
	}

	// Process the response headers, which are terminated by a blank line.
	string sHeader;
	map<string, string> mHeader;
	size_t uSize = 1;
	while (getline(rStream, sHeader) && sHeader != "\r")
	{
		m_sigInfoLog(sHeader);
		vector<string> vList;
		boost::split(vList, sHeader, boost::is_any_of(":"), boost::token_compress_on);
		if (vList.size() == 2)
		{
			boost::trim(vList[0]);
			boost::trim(vList[1]);
			mHeader.insert(pair<string, string>(vList[0], vList[1]));

			if (vList[0] == "Content-Length")
				uSize = std::atoi(vList[1].c_str());
		}
	}

	return uSize;
}
