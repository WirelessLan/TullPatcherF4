#pragma once

namespace Utils {
	bool IsValidDecimalNumber(const std::string& str);
	bool IsPluginExists(std::string_view a_pluginName);
	RE::TESForm* GetFormFromIdentifier(std::string_view pluginName, std::uint32_t formId);
    RE::TESForm* GetFormFromIdentifier(std::string_view pluginName, std::string_view formIdStr);
	RE::TESForm* GetFormFromString(std::string_view formStr);
}
