#pragma once

#include <string>
#include <optional>
#include <vector>
#include <map>

namespace HttpClientLite {
	class URL
	{
	public:
		std::string m_sProtocol;
		std::string m_sDomain;
		uint16_t	m_uPort;
		std::string m_sPath;

		std::optional<std::pair<std::string, std::string>>	m_mLoginInfo;	// https://user:password@domain/
		std::vector<std::pair<std::string, std::string>>	m_vGetData;		// https:://domain/page.html?key=val

	public:
		URL()
		{
			reset();
		}

		URL(const std::string& sInput)
		{
			fromString(sInput);
		}

		operator bool() const
		{
			return isValid();
		}

		operator std::string() const
		{
			return toString();
		}

		void reset()
		{
			m_sProtocol = "http";
			m_sDomain = "";
			m_uPort = 80;
			m_sPath = "/";
			m_mLoginInfo.reset();
			m_vGetData.clear();
		}

		bool isValid() const
		{
			return m_sDomain != "";
		}

		std::string toString() const;
		bool fromString(const std::string& sInput);
	};
}
