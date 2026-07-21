#include "Utils.h"

namespace Utils
{
	namespace
	{
		std::string_view Trim(std::string_view a_str)
		{
			std::size_t sIdx, eIdx;

			for (sIdx = 0; sIdx < a_str.length(); ++sIdx)
			{
				if (!std::isspace(static_cast<unsigned char>(a_str[sIdx])))
				{
					break;
				}
			}

			if (sIdx >= a_str.length())
			{
				return std::string_view{};
			}

			for (eIdx = a_str.length(); eIdx > 0; --eIdx)
			{
				if (!std::isspace(static_cast<unsigned char>(a_str[eIdx - 1])))
				{
					break;
				}
			}

			return a_str.substr(sIdx, eIdx - sIdx);
		}

		std::optional<std::uint32_t> ParseFormID(std::string_view a_formIdStr)
		{
			const auto formIdOpt = ParseHex(a_formIdStr);
			if (!formIdOpt.has_value())
			{
				return std::nullopt;
			}
			return formIdOpt.value() & 0xFFFFFFu;
		}
	}  // namespace

	std::optional<std::uint32_t> ParseHex(std::string_view a_hexStr)
	{
		try
		{
			return std::stoul(std::string(a_hexStr), nullptr, 16);
		} catch (...)
		{
			return std::nullopt;
		}
	}

	bool IsPluginExists(std::string_view a_pluginName)
	{
		auto* g_dataHandler = RE::TESDataHandler::GetSingleton();
		if (!g_dataHandler)
		{
			return false;
		}

		const auto* mod = g_dataHandler->LookupModByName(a_pluginName);
		if (!mod)
		{
			return false;
		}

		return mod->IsActive();
	}

	RE::TESForm* GetFormFromIdentifier(std::string_view a_pluginName, std::uint32_t a_formID)
	{
		auto* g_dataHandler = RE::TESDataHandler::GetSingleton();
		if (!g_dataHandler)
		{
			return nullptr;
		}

		return g_dataHandler->LookupForm(a_formID, a_pluginName);
	}

	RE::TESForm* GetFormFromIdentifier(std::string_view pluginName, std::string_view formIdStr)
	{
		const auto formIdOpt = ParseFormID(formIdStr);
		if (!formIdOpt.has_value())
		{
			return nullptr;
		}
		return GetFormFromIdentifier(pluginName, formIdOpt.value());
	}

	RE::TESForm* GetFormFromString(std::string_view a_formStr)
	{
		const auto delimiter = a_formStr.find('|');
		if (delimiter == std::string_view::npos)
		{
			return nullptr;
		}

		const auto pluginName = Trim(a_formStr.substr(0, delimiter));
		const auto formID = Trim(a_formStr.substr(delimiter + 1));

		return Utils::GetFormFromIdentifier(pluginName, formID);
	}
}  // namespace Utils
