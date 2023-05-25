#pragma once

namespace Utils {
	RE::TESForm* GetFormFromIdentifier(std::string_view pluginName, std::uint32_t formId);
    RE::TESForm* GetFormFromIdentifier(std::string_view pluginName, std::string_view formIdStr);
	RE::TESForm* GetFormFromString(std::string_view formStr);
}
