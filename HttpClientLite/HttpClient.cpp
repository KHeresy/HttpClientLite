// STL Header
#include <chrono>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>

// Boost Header
#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>

// Application Header
#include "HttpClient.h"

#pragma region Functions of HTNLTag
std::optional< std::pair<size_t,size_t> > HTMLTag::GetData( const std::string& sHtmlSource, size_t uBeginPos )
{
	if( m_sTagName != "" )
	{
		std::string sTagBegin = "<" + m_sTagName;

		// find the position of tag
		size_t uStartPos = sHtmlSource.find( sTagBegin, uBeginPos );
		if( uStartPos != std::string::npos )
		{
			// find the end of the start tag
			size_t uPos1 = uStartPos + sTagBegin.length();
			size_t uPos2 = sHtmlSource.find( ">", uPos1 );
			if( uPos2 != std::string::npos )
			{
				// process attribute
				std::string sAttribStr = sHtmlSource.substr( uPos1, uPos2 - uPos1 );
				boost::trim( sAttribStr );
				size_t uStartPos = 0;
				while( true )
				{
					size_t uPos = sAttribStr.find_first_of( " =", uStartPos );
					if( uPos == std::string::npos )
						break;

					std::string sName = sAttribStr.substr( uStartPos, uPos - uStartPos );
					if( sAttribStr[uPos] == '=' )
					{
						if( sAttribStr[uPos+1] == '\"' )
						{
							size_t uEnd = sAttribStr.find_first_of( '\"', uPos + 2 );
							if( uEnd == std::string::npos )
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
							if( uEnd == std::string::npos )
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
					std::string sTagEnd		= "</" + m_sTagName + ">";
					size_t uPos3 = sHtmlSource.find( sTagEnd, uPos2 + 1 );
					if( uPos3 != std::string::npos )
					{
						m_sContent = sHtmlSource.substr( uPos2 + 1, uPos3 - uPos2 - 1 );
						uPos2 = uPos3 + sTagEnd.size() - 1;
					}
				}

				return std::make_pair( uStartPos, uPos2 );
			}
		}
	}
	return std::optional< std::pair<size_t,size_t> >();
}

std::optional< std::pair<size_t,size_t> > HTMLTag::FindQuoteContent( const std::string& rSource, const char c, size_t uBeginPos )
{
	size_t uPos = rSource.find_first_of( "\'\"", uBeginPos );
	if( uPos != std::string::npos )
	{
		if( uPos > uBeginPos && rSource[ uPos - 1 ] == '\\' )
		{
		}
	}
	return std::optional< std::pair<size_t,size_t> >();
}
#pragma endregion

#pragma region Functions of HTMLParsr
void HTMLParser::FindTag( const std::wstring& rHtml, const std::wstring& rTag, const std::map< std::wstring, std::optional<std::wstring> >& rAttribute, size_t uStartPos )
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

std::pair<size_t,std::wstring> HTMLParser::FindContentBetweenTag( const std::wstring& rHtml, const std::pair<std::wstring, std::wstring>& rTag, size_t uStartPos )
{
	size_t uPos1 = rHtml.find( rTag.first, uStartPos );
	if( uPos1 != std::wstring::npos )
	{
		size_t uPos2 = rHtml.find( rTag.second, uPos1 );
		if (uPos2 != std::wstring::npos)
		{
			uPos1 = uPos1 + rTag.first.length();

			std::wstring sContent = rHtml.substr(uPos1, uPos2 - uPos1);
			boost::algorithm::trim(sContent);
			return make_pair(uPos1, sContent);
		}
	}
	return std::make_pair(std::wstring::npos, L"" );
}

std::optional< std::pair<std::wstring, std::wstring> > HTMLParser::AnalyzeLink( const std::wstring& rHtml, size_t uStartPos )
{
	size_t uPos1 = rHtml.find( L"<a ", uStartPos );
	if( uPos1 != std::wstring::npos )
	{
		size_t uPos2 = rHtml.find( L"href=\"", uPos1 );
		if( uPos2 != std::wstring::npos )
		{
			uPos2 += 6;
			uPos1 = rHtml.find( L"\"", uPos2 );
			if( uPos1 != std::wstring::npos )
			{
				std::wstring sLinkUrl = rHtml.substr( uPos2, uPos1 - uPos2 );

				uPos1 = rHtml.find( L">", uPos1 );
				if( uPos1 != std::wstring::npos )
				{
					uPos1 += 1;
					uPos2 = rHtml.find( L"</a>", uPos1 );
					std::wstring sLinkText = rHtml.substr( uPos1, uPos2 - uPos1 );
					boost::algorithm::trim( sLinkText );

					return std::optional< std::pair<std::wstring, std::wstring> >( make_pair( sLinkText, sLinkUrl ) );
				}
			}
		}
	}
	return std::optional< std::pair<std::wstring, std::wstring> >();
}
#pragma endregion

#pragma region Functions of HttpClient
HttpClient::HttpClient() : m_Resolver(m_IO_service){}

std::optional<std::wstring> HttpClient::ReadHtml( const std::string& rServer, const std::string& rPath )
{
	// code reference to http://www.boost.org/doc/libs/1_53_0/doc/html/boost_asio/example/iostreams/http_client.cpp
	boost::asio::ip::tcp::iostream sStream;
	sStream.expires_from_now( std::chrono::seconds( 300 ) );
	if( SendRequest( rServer, rPath, sStream ) )
	{
		size_t uSize = GetHttpHeader(sStream);
		if (uSize > 0)
		{
			// Write the remaining data to output.
			std::stringstream oStream;
			oStream << sStream.rdbuf();
			std::string sHtml = oStream.str();

			std::string sEncoding = "ANSI";
			size_t uPos = sHtml.find("charset=");
			if (uPos != std::string::npos)
			{
				auto uEnd = sHtml.find_first_of("'\" ", uPos + 1);
				if (uEnd != std::string::npos)
				{
					sEncoding = sHtml.substr(uPos + 8, uEnd - uPos - 8);
				}
			}
			return boost::locale::conv::to_utf<wchar_t>(sHtml, sEncoding);
		}
	}
	return std::optional<std::wstring>();
}

bool HttpClient::GetBinaryFile( const std::string& rServer, const std::string& rPath, const std::wstring& rFilename )
{
	// code reference to http://www.boost.org/doc/libs/1_53_0/doc/html/boost_asio/example/iostreams/http_client.cpp
	boost::asio::ip::tcp::iostream sStream;
	sStream.expires_from_now(std::chrono::seconds( 300 ) );
	if( SendRequest( rServer, rPath, sStream ) )
	{
		size_t uSize = GetHttpHeader(sStream);
		if (uSize > 0)
		{
			std::ofstream outfile(rFilename, std::ios::binary);
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

std::optional< std::pair<std::string, std::string> > HttpClient::ParseURL( const std::string& sURL )
{
	if( sURL.length() > 10 )
	{
		if( sURL.substr( 0, 7 ) == "http://" )
		{
			auto uPos = sURL.find_first_of( "/", 8 );
			return std::optional< std::pair<std::string, std::string> >( make_pair( sURL.substr( 7, uPos - 7 ), sURL.substr( uPos ) ) );
		}
	}
	return std::optional< std::pair<std::string, std::string> >();
}

std::optional<std::string> HttpClient::GetFilename( const std::string& sURL )
{
	size_t uPos = sURL.find_last_of( '/' );
	if( uPos != std::string::npos && uPos != sURL.size() - 1 )
		return sURL.substr( uPos + 1 );

	return std::optional<std::string>();
}

bool HttpClient::SendRequest( const std::string& rServer, const std::string& rPath, boost::asio::ip::tcp::iostream& rStream )
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
	std::string sHttpVersion;
	rStream >> sHttpVersion;
	unsigned int uCode;
	rStream >> uCode;
	std::string sMssage;
	std::getline( rStream, sMssage );
	m_sigInfoLog( "Recive data: " + sHttpVersion + " / " + sMssage );
	if( !rStream || sHttpVersion.substr(0, 5) != "HTTP/" )
	{
		m_sigErrorLog( "Invalid response" );
		return 0;
	}

	if (uCode == 200 || uCode == 301 || uCode == 302)
	{
		// Process the response headers, which are terminated by a blank line.
		std::string sHeader;
		std::map<std::string, std::string> mHeader;
		size_t uSize = 1;
		while (std::getline(rStream, sHeader) && sHeader != "\r")
		{
			m_sigInfoLog(sHeader);
			size_t uPos = sHeader.find_first_of(':');
			if (uPos != std::string::npos)
			{
				std::string sKey = sHeader.substr(0, uPos);
				std::string sValue = sHeader.substr(uPos + 1);
				boost::trim(sKey);
				boost::trim(sValue);
				mHeader.insert(std::make_pair(sKey, sValue));

				if (sKey == "Content-Length")
					uSize = std::atoi(sValue.c_str());
			}
		}

		if (uCode == 301 || uCode == 302)
		{
			std::string sNewURL = mHeader["Location"];
			if (sNewURL == "")
				sNewURL = mHeader["location"];

			if (sNewURL != "")
			{
				auto mURL = ParseURL(sNewURL);
				if (mURL)
				{
					rStream.close();
					if (SendRequest(mURL->first, mURL->second, rStream))
						return GetHttpHeader(rStream);
				}
			}

			return 0;
		}
		return uSize;
	}
	else
	{
		m_sigErrorLog( "Response returned with status code " + uCode );
		return 0;
	}

}
#pragma endregion
