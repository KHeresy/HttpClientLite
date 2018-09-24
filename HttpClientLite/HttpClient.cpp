// STL Header
#include <chrono>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>

// Boost Header
#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

// Application Header
#include "HttpClient.h"
#include "root_certificates.hpp"

namespace HttpClientLite
{
	template<typename TStreamType>
	class TAbsSession : public Session
	{
	public:
		TAbsSession(boost::asio::io_context& ctxAsio) : m_Resolver(ctxAsio){}

		virtual bool Request() override
		{
			try
			{
				using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
				namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

				// Set up an HTTP GET request message
				http::request<http::string_body> mRequest{ http::verb::get, m_Url.getTarget(), m_iHttpVersion };
				mRequest.set(http::field::host, m_Url.m_sHost);
				mRequest.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

				// Send the HTTP request to the remote host
				http::write(*m_tStream, mRequest);
			}
			catch (std::exception e)
			{
				return false;
			}

			return true;
		}

		virtual THttpResponse Read() override
		{
			namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

			// Declare a container to hold the response
			http::response<http::string_body> res;

			// Receive the HTTP response
			http::read(*m_tStream, m_Buffer, res);

			return res;
		}

	protected:
		URL								m_Url;
		boost::asio::ip::tcp::resolver	m_Resolver;
		std::unique_ptr<TStreamType>	m_tStream;
		boost::beast::flat_buffer		m_Buffer;
		int								m_iHttpVersion = 11;
	};

	class CClientNoSSL : public TAbsSession<boost::asio::ip::tcp::socket>
	{
	public:
		CClientNoSSL(boost::asio::io_context& ctxAsio) :TAbsSession(ctxAsio)
		{
			m_tStream = std::make_unique<boost::asio::ip::tcp::socket>(ctxAsio);
		}

		virtual bool Connect(const URL& rURL) override
		{
			m_Url = rURL;
			try
			{
				// Look up the domain name
				auto const mResults = m_Resolver.resolve(m_Url.m_sHost, std::to_string(m_Url.m_uPort));

				// Make the connection on the IP address we get from a lookup
				boost::asio::connect(*m_tStream, mResults.begin(), mResults.end());
			}
			catch (std::exception e)
			{
				return false;
			}

			return true;
		}

		virtual void Close() override
		{
			boost::system::error_code ec;
			m_tStream->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

			// not_connected happens sometimes
			// so don't bother reporting it.
			//
			if (ec && ec != boost::system::errc::not_connected)
				throw boost::system::system_error{ ec };
		}
	};

	class CClientSSL : public TAbsSession<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>
	{
	public:
		CClientSSL(boost::asio::io_context& ctxAaio, boost::asio::ssl::context&	ctxSSL) : TAbsSession(ctxAaio)
		{
			m_tStream = std::make_unique<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>(ctxAaio, ctxSSL);
		}

		virtual bool Connect(const URL& rURL) override
		{
			m_Url = rURL;
			try
			{
				// Set SNI Hostname (many hosts need this to handshake successfully)
				if (!SSL_set_tlsext_host_name(m_tStream->native_handle(), m_Url.m_sHost.c_str()))
				{
					boost::system::error_code ec{ static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category() };
					throw boost::system::system_error{ ec };
				}

				// Look up the domain name
				auto const results = m_Resolver.resolve(m_Url.m_sHost, std::to_string(m_Url.m_uPort));

				// Make the connection on the IP address we get from a lookup
				boost::asio::connect(m_tStream->next_layer(), results.begin(), results.end());

				// Perform the SSL handshake
				m_tStream->handshake(ssl::stream_base::client);
			}
			catch (std::exception e)
			{
				return false;
			}

			return true;
		}

		virtual void Close() override
		{
			// Gracefully close the stream
			boost::system::error_code ec;
			m_tStream->shutdown(ec);
			if (ec == boost::asio::error::eof || ec == boost::asio::ssl::error::stream_truncated)
			{
				// Rationale:
				// http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
				ec.assign(0, ec.category());
			}
			if (ec)
			{
				auto x = ec.message();
				throw boost::system::system_error{ ec };
			}
		}
	};

	std::optional<std::wstring> Session::GetBody(const THttpResponse & rResponse, const std::string sDefaultCodePage)
	{
		std::string sContent = rResponse.body();
		if (sContent.size() > 0)
		{
			std::string sEncoding = sDefaultCodePage;
			size_t uPos = sContent.find("charset=");
			if (uPos != std::string::npos)
			{
				auto uEnd = sContent.find_first_of("'\" ", uPos + 9);
				if (uEnd != std::string::npos)
				{
					sEncoding = sContent.substr(uPos, 20);
					sEncoding = sContent.substr(uPos + 8, uEnd - uPos - 7);
					if (sEncoding[0] == '\"' || sEncoding[0] == '\'')
						sEncoding = sEncoding.substr(1);

					uPos = sEncoding.find_first_of("'\"");
					if (uPos != std::string::npos)
						sEncoding = sEncoding.substr(0, uPos);
				}
			}
			return boost::locale::conv::to_utf<wchar_t>(sContent, sEncoding);
		}

		return std::optional<std::wstring>();
	}

	bool Session::SaveBinaryFile(const THttpResponse & rResponse, const std::string & sFilename)
	{
		std::ofstream sFile(sFilename, std::ios::binary);
		if (sFile.is_open())
		{
			auto& rData = rResponse.body();
			sFile.write(rData.data(), rData.size());
			sFile.close();
			return true;
		}
		return false;
	}
}

using namespace HttpClientLite;

#pragma region Functions of HTNLTag
std::optional< std::pair<size_t, size_t> > HTMLTag::GetData(const std::string& sHtmlSource, size_t uBeginPos)
{
	if (m_sTagName != "")
	{
		std::string sTagBegin = "<" + m_sTagName;

		// find the position of tag
		size_t uStartPos = sHtmlSource.find(sTagBegin, uBeginPos);
		if (uStartPos != std::string::npos)
		{
			// find the end of the start tag
			size_t uPos1 = uStartPos + sTagBegin.length();
			size_t uPos2 = sHtmlSource.find(">", uPos1);
			if (uPos2 != std::string::npos)
			{
				// process attribute
				std::string sAttribStr = sHtmlSource.substr(uPos1, uPos2 - uPos1);
				boost::trim(sAttribStr);
				size_t uStartPos = 0;
				while (true)
				{
					size_t uPos = sAttribStr.find_first_of(" =", uStartPos);
					if (uPos == std::string::npos)
						break;

					std::string sName = sAttribStr.substr(uStartPos, uPos - uStartPos);
					if (sAttribStr[uPos] == '=')
					{
						if (sAttribStr[uPos + 1] == '\"')
						{
							size_t uEnd = sAttribStr.find_first_of('\"', uPos + 2);
							if (uEnd == std::string::npos)
							{
							}
							else
							{
								m_mAttributes[sName] = sAttribStr.substr(uPos + 2, uEnd - uPos - 2);
								uStartPos = uEnd + 1;
							}
						}
						else
						{
							size_t uEnd = sAttribStr.find_first_of(' ', uPos + 1);
							if (uEnd == std::string::npos)
							{
								m_mAttributes[sName] = sAttribStr.substr(uPos + 1);
								break;
							}
							else
							{
								m_mAttributes[sName] = sAttribStr.substr(uPos + 1, uEnd - uPos - 1);
								uStartPos = uEnd + 1;
							}
						}
					}
					else
					{
						m_mAttributes[sName];
						uStartPos = uPos + 1;
					}
				}

				// try to find the close tag only if this tag is not self-closed
				if (sHtmlSource[uPos2 - 1] != '/')
				{
					std::string sTagEnd = "</" + m_sTagName + ">";
					size_t uPos3 = sHtmlSource.find(sTagEnd, uPos2 + 1);
					if (uPos3 != std::string::npos)
					{
						m_sContent = sHtmlSource.substr(uPos2 + 1, uPos3 - uPos2 - 1);
						uPos2 = uPos3 + sTagEnd.size() - 1;
					}
				}

				return std::make_pair(uStartPos, uPos2);
			}
		}
	}
	return std::optional< std::pair<size_t, size_t> >();
}

std::optional< std::pair<size_t, size_t> > HTMLTag::FindQuoteContent(const std::string& rSource, const char c, size_t uBeginPos)
{
	size_t uPos = rSource.find_first_of("\'\"", uBeginPos);
	if (uPos != std::string::npos)
	{
		if (uPos > uBeginPos && rSource[uPos - 1] == '\\')
		{
		}
	}
	return std::optional< std::pair<size_t, size_t> >();
}
#pragma endregion

#pragma region Functions of HTMLParsr
void HTMLParser::FindTag(const std::wstring& rHtml, const std::wstring& rTag, const std::map< std::wstring, std::optional<std::wstring> >& rAttribute, size_t uStartPos)
{
	std::wstring	sTagBegin = L"<" + rTag,
		sTagEnd = L"</" + rTag + L">";

	// find the position of tag
	size_t uPos1 = rHtml.find(sTagBegin, uStartPos);
	if (uPos1 == std::string::npos)
	{
		return;
	}

	// find the end of the start tag
	size_t uPos2 = rHtml.find(L">", uPos1 + sTagBegin.length());
	if (uPos2 == std::string::npos)
	{
		return;
	}

	if (rAttribute.size() > 0)
	{
		// get attributes
		std::wstring sAttributes = rHtml.substr(uPos1 + sTagBegin.length(), uPos2 - (uPos1 + sTagBegin.length()));

		// TODO: check attribute
	}

	// TODO: check nested tag?

	// find close tag
	size_t uPos3 = rHtml.find(sTagEnd, uPos2 + 1);
	if (uPos3 != std::string::npos)
	{
		std::wstring sContent = rHtml.substr(uPos2 + 1, uPos3 - uPos2 - 1);
	}
}

std::pair<size_t, std::wstring> HTMLParser::FindContentBetweenTag(const std::wstring& rHtml, const std::pair<std::wstring, std::wstring>& rTag, size_t uStartPos)
{
	size_t uPos1 = rHtml.find(rTag.first, uStartPos);
	if (uPos1 != std::wstring::npos)
	{
		size_t uPos2 = rHtml.find(rTag.second, uPos1);
		if (uPos2 != std::wstring::npos)
		{
			uPos1 = uPos1 + rTag.first.length();

			std::wstring sContent = rHtml.substr(uPos1, uPos2 - uPos1);
			boost::algorithm::trim(sContent);
			return make_pair(uPos1, sContent);
		}
	}
	return std::make_pair(std::wstring::npos, L"");
}

std::optional< std::pair<std::wstring, std::wstring> > HTMLParser::AnalyzeLink(const std::wstring& rHtml, size_t uStartPos)
{
	size_t uPos1 = rHtml.find(L"<a ", uStartPos);
	if (uPos1 != std::wstring::npos)
	{
		size_t uPos2 = rHtml.find(L"href=\"", uPos1);
		if (uPos2 != std::wstring::npos)
		{
			uPos2 += 6;
			uPos1 = rHtml.find(L"\"", uPos2);
			if (uPos1 != std::wstring::npos)
			{
				std::wstring sLinkUrl = rHtml.substr(uPos2, uPos1 - uPos2);

				uPos1 = rHtml.find(L">", uPos1);
				if (uPos1 != std::wstring::npos)
				{
					uPos1 += 1;
					uPos2 = rHtml.find(L"</a>", uPos1);
					std::wstring sLinkText = rHtml.substr(uPos1, uPos2 - uPos1);
					boost::algorithm::trim(sLinkText);

					return std::optional< std::pair<std::wstring, std::wstring> >(make_pair(sLinkText, sLinkUrl));
				}
			}
		}
	}
	return std::optional< std::pair<std::wstring, std::wstring> >();
}
#pragma endregion

HttpClientLite::Client::Client() : m_ctxSSL(ssl::context::sslv23_client)
{
	load_root_certificates(m_ctxSSL);
}

std::shared_ptr<Session> HttpClientLite::Client::Connect(const URL& rURL)
{
	if (rURL)
	{
		std::shared_ptr< Session> pSession;
		if (rURL.m_sProtocol == "http")
			pSession = std::make_shared<CClientNoSSL>(m_ctxAaio);
		else if (rURL.m_sProtocol == "https")
			pSession = std::make_shared<CClientSSL>(m_ctxAaio, m_ctxSSL);

		if (pSession)
			if(pSession->Connect(rURL))
				return pSession;
	}
	return nullptr;
}

std::optional<std::wstring> HttpClientLite::Client::ReadHtml(const URL & rURL, const std::string sDefaultCodePage)
{
	auto pSession = Connect(rURL);
	if (pSession->Request())
	{
		auto res = pSession->Read();
		if (res.result_int() / 100 == 3 ) // 300 redirect
			return ReadHtml(URL(res[boost::beast::http::field::location].to_string()), sDefaultCodePage);
		else if(res.result_int() == 200)
			return pSession->GetBody(res, sDefaultCodePage);

		pSession->Close();
	}

	return std::optional<std::wstring>();
}

bool HttpClientLite::Client::GetBinaryFile(const URL & rURL, const std::wstring & sFilename)
{
	return false;
}

Session::THttpResponse HttpClientLite::Client::ReadWithAuroRedirect(const URL & rURL)
{
	auto pSession = Connect(rURL);
	if (pSession->Request())
	{
		auto res = pSession->Read();
		if (res.result_int() == 200)
			return res;

		if (res.result_int() / 100 == 3) // 300 redirect
			return ReadWithAuroRedirect(URL(res[boost::beast::http::field::location].to_string()));

		pSession->Close();
	}

	return Session::THttpResponse();
}
