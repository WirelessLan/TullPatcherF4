#pragma once

namespace Utils {
	bool IsValidDecimalNumber(const std::string& str);
	std::uint32_t ParseHex(std::string_view a_hexStr);
	bool IsPluginExists(std::string_view a_pluginName);
	RE::TESForm* GetFormFromIdentifier(std::string_view pluginName, std::uint32_t formId);
    RE::TESForm* GetFormFromIdentifier(std::string_view pluginName, std::string_view formIdStr);
	RE::TESForm* GetFormFromString(std::string_view formStr);
}
