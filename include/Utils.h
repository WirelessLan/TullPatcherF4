#pragma once

namespace Utils {
	template <typename T>
	inline bool ConvertNumber(std::string_view a_input, T& a_value) {
		T parsedValue{};
		const auto result = std::from_chars(a_input.data(), a_input.data() + a_input.size(), parsedValue);
		if (result.ec != std::errc{} || result.ptr != a_input.data() + a_input.size()) {
			return false;
		}

		a_value = parsedValue;
		return true;
	}

	std::optional<std::uint32_t> ParseHex(std::string_view a_hexStr);
	bool IsPluginExists(std::string_view a_pluginName);
	RE::TESForm* GetFormFromIdentifier(std::string_view pluginName, std::uint32_t formId);
    RE::TESForm* GetFormFromIdentifier(std::string_view pluginName, std::string_view formIdStr);
	RE::TESForm* GetFormFromString(std::string_view formStr);
}
