#include "Utils.h"

namespace Utils {
	std::string_view Trim(std::string_view a_str) {
		std::size_t sIdx, eIdx;

		for (sIdx = 0; sIdx < a_str.length(); ++sIdx) {
			if (!std::isspace(static_cast<unsigned char>(a_str[sIdx]))) {
				break;
			}
		}

		if (sIdx >= a_str.length()) {
			return std::string_view{};
		}

		for (eIdx = a_str.length(); eIdx > 0; --eIdx) {
			if (!std::isspace(static_cast<unsigned char>(a_str[eIdx - 1]))) {
				break;
			}
		}

		return a_str.substr(sIdx, eIdx - sIdx);
	}

	bool IsValidDecimalNumber(const std::string& str) {
		std::stringstream s(str);
		float f;
		s >> std::noskipws >> f;

		char remaining;
		return s.eof() || (s >> remaining && std::isspace(remaining));
	}

	std::optional<std::uint32_t> ParseHex(std::string_view a_hexStr) {
		try {
			return std::stoul(std::string(a_hexStr), nullptr, 16);
		}
		catch (...) {
			return std::nullopt;
		}
	}

	std::optional<std::uint32_t> ParseFormID(std::string_view a_formIdStr) {
		auto formId = ParseHex(a_formIdStr);
		if (!formId.has_value()) {
			return std::nullopt;
		}
		return formId.value() & 0xFFFFFFu;
	}

	bool IsPluginExists(std::string_view a_pluginName) {
		RE::TESDataHandler* g_dataHandler = RE::TESDataHandler::GetSingleton();
		if (!g_dataHandler) {
			return false;
		}

		auto mod = g_dataHandler->LookupModByName(a_pluginName);
		if (!mod) {
			return false;
		}

		return mod->IsActive();
	}

	RE::TESForm* GetFormFromIdentifier(std::string_view a_pluginName, std::uint32_t a_formID) {
		RE::TESDataHandler* g_dataHandler = RE::TESDataHandler::GetSingleton();
		if (!g_dataHandler) {
			return nullptr;
		}

		return g_dataHandler->LookupForm(a_formID, a_pluginName);
	}

	RE::TESForm* GetFormFromIdentifier(std::string_view pluginName, std::string_view formIdStr) {
		auto formId = ParseFormID(formIdStr);
		if (!formId.has_value()) {
			return nullptr;
		}
		return GetFormFromIdentifier(pluginName, formId.value());
	}

	RE::TESForm* GetFormFromString(std::string_view a_formStr) {
		auto delimiter = a_formStr.find('|');
		if (delimiter == std::string_view::npos) {
			return nullptr;
		}

		std::string_view pluginName = Trim(a_formStr.substr(0, delimiter));
		std::string_view formID = Trim(a_formStr.substr(delimiter + 1));

		return Utils::GetFormFromIdentifier(pluginName, formID);
	}

	bool GetPluginNameFormID(RE::TESForm* a_form, std::string_view& a_pluginName, std::uint32_t& a_formID) {
		if (!a_form || a_form->sourceFiles.array->empty()) {
			return false;
		}

		RE::TESFile* orgFile = a_form->sourceFiles.array->front();
		if (!orgFile) {
			return false;
		}

		a_pluginName = orgFile->filename;
		a_formID = orgFile->IsLight() ? 0xFFF & a_form->formID : 0xFFFFFF & a_form->formID;

		return true;
	}
}
