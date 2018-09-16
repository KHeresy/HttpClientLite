#pragma once

// STL Header
#include <map>
#include <string>
#include <optional>

// Boost Header
#include <boost/asio.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/signals2.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

#include "url.h"

namespace HttpClientLite
{
	/**
	 * Class to save all information about a HTML set
	 */
	class HTMLTag
	{
	public:
		std::string											m_sTagName;
		std::optional<std::string>							m_sContent;
		std::map< std::string, std::optional<std::string> >	m_mAttributes;

	public:
		HTMLTag(const std::string& sTagName)
		{
			m_sTagName = sTagName;
		}

		HTMLTag(const std::string& sTagName, const std::string& sHtmlSource, size_t uBeginPos = 0)
		{
			m_sTagName = sTagName;
			GetData(sHtmlSource, uBeginPos);
		}

		std::optional< std::pair<size_t, size_t> > GetData(const std::string& sHtmlSource, size_t uBeginPos = 0);

		std::optional< std::pair<size_t, size_t> > FindQuoteContent(const std::string& rSourze, const char c, size_t uBeginPos = 0);

	public:
		static std::optional< std::pair< HTMLTag, std::pair<size_t, size_t> > > Construct(const std::string& sTagName, std::string& sHtmlSource, size_t uBeginPos = 0)
		{
			HTMLTag mTag(sTagName);
			std::optional< std::pair<size_t, size_t> > pInfo = mTag.GetData(sHtmlSource, uBeginPos);
			if (pInfo)
				return std::make_pair(mTag, *pInfo);

			return std::optional< std::pair< HTMLTag, std::pair<size_t, size_t> > >();
		}
	};

	class HTMLParser
	{
	public:
		typedef std::map< std::wstring, std::optional<std::wstring> >	TAttributes;

	public:
		static void FindTag(const std::wstring& rHtml, const std::wstring& rTag, const std::map< std::wstring, std::optional<std::wstring> >& rAttribute, size_t uStartPos = 0);

		static std::pair<size_t, std::wstring> FindContentBetweenTag(const std::wstring& rHtml, const std::pair<std::wstring, std::wstring>& rTag, size_t uStartPos = 0);

		static std::optional< std::pair<std::wstring, std::wstring> > AnalyzeLink(const std::wstring& rHtml, size_t uStartPos = 0);
	};

	class Session
	{
	public:
		using THttpResponse = boost::beast::http::response<boost::beast::http::string_body>;

	public:
		virtual bool Connect(const URL& rURL) = 0;
		virtual bool Request() = 0; //TODO: need to modify request
		virtual THttpResponse Read() = 0;
		virtual void Close() = 0;

	public:
		static std::optional<std::wstring> GetBody(const THttpResponse& rResponse);
	};

	class Client
	{
	public:
		boost::signals2::signal<void(const std::string&)>	m_sigErrorLog;
		boost::signals2::signal<void(const std::string&)>	m_sigInfoLog;

	public:
		Client();

		std::shared_ptr<Session> Connect(const URL& rURL);

	protected:
		std::optional<std::wstring> ReadHtml_Http(const URL& rURL);
		std::optional<std::wstring> ReadHtml_Https(const URL& rURL);

	protected:
		boost::asio::io_context		m_ctxAaio;
		boost::asio::ssl::context	m_ctxSSL;
		int							m_iHttpVersion = 11;
	};
}
