#include "url.h"

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#pragma region internal code
static const std::map<std::string, uint16_t> mPortMapping = {
	{"http", 80},
	{"https", 443},
	{"ftp", 21}
};

static uint16_t getPort(const std::string& sProtocol)
{
	auto itRes = mPortMapping.find(sProtocol);
	if (itRes != mPortMapping.end())
		return itRes->second;

	return 0;
}

static bool isSpecialPort(const std::string& sProtocol, const uint16_t& uPort)
{
	return (getPort(sProtocol) != uPort);
}
#pragma endregion

std::string HttpClientLite::URL::toString() const
{
	if (!isValid())
		return "";

	// Protocol
	std::string sUrl = m_sProtocol + "://";

	// login
	if (m_mLoginInfo)
	{
		const auto& rLInfo = m_mLoginInfo.value();
		sUrl += rLInfo.first;

		if (rLInfo.second != "")
			sUrl += ":" + rLInfo.second;

		sUrl += "@";
	}

	// domain
	sUrl += m_sDomain;

	// port
	if (isSpecialPort(m_sProtocol, m_uPort))
		sUrl += ":" + std::to_string(m_uPort);

	// path
	sUrl += m_sPath;

	// GET data
	if (m_vGetData.size() > 0)
	{
		sUrl += ( "?" + m_vGetData[0].first + "=" + m_vGetData[0].second);

		for( int i = 1; i < m_vGetData.size(); ++ i)
			sUrl += ( "&" + m_vGetData[i].first + "=" + m_vGetData[i].second);
	}

	return sUrl;
}

bool HttpClientLite::URL::fromString(const std::string & sInput)
{
	reset();

	// Protocol
	size_t uPos1 = sInput.find("://");
	{
		if (uPos1 != std::string::npos)
		{
			m_sProtocol = sInput.substr(0, uPos1);
			m_uPort = getPort(m_sProtocol);
		}
		uPos1 += 3;
	}

	// login@domain:port
	size_t uPos2 = sInput.find("/", uPos1);
	{
		std::string sDomain;
		if (uPos2 == std::string::npos)
			sDomain = sInput.substr(uPos1);
		else
			sDomain = sInput.substr(uPos1, (uPos2 - uPos1));

		// login
		{
			size_t uPos = sDomain.find("@");
			if (uPos != std::string::npos)
			{
				std::string sLoginInfo = sDomain.substr(0, uPos);
				size_t uBreak = sLoginInfo.find(":");
				if (uBreak == std::string::npos)
					m_mLoginInfo = { sLoginInfo, "" };
				else
					m_mLoginInfo = { sLoginInfo.substr(0, uBreak), sLoginInfo.substr(uBreak + 1) };

				sDomain = sDomain.substr(uPos + 1);
			}
		}

		// port
		{
			size_t uPos = sDomain.find(":");
			if (uPos != std::string::npos)
			{
				m_uPort = std::stoi(sDomain.substr(uPos + 1));
				sDomain = sDomain.substr(0, uPos);
			}
		}

		m_sDomain = sDomain;
	}

	// path
	{
		m_sPath = sInput.substr(uPos2);

		uPos1 = m_sPath.find("?");
		if (uPos1 != std::string::npos)
		{
			// GET data
			std::string sToken = m_sPath.substr(uPos1+1);
			
			boost::char_separator<char> csSep("&");
			boost::tokenizer< boost::char_separator<char> > mTokens(sToken, csSep);
			for (auto it = mTokens.begin(); it != mTokens.end(); ++it)
			{
				std::vector<std::string> vRes;
				boost::split(vRes, *it, boost::is_any_of("="));
				if (vRes.size() == 1)
					m_vGetData.push_back({ vRes[0], ""});
				else if (vRes.size() == 2)
					m_vGetData.push_back({ vRes[0], vRes[1] });
			}

			// path
			m_sPath = m_sPath.substr(0, uPos1);
		}
	}

	return isValid();
}
