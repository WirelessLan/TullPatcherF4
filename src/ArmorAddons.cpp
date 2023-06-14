#include "ArmorAddons.h"

#include <regex>

#include "Configs.h"
#include "Utils.h"

namespace ArmorAddons {
	enum class FilterType {
		kFormID
	};

	std::string_view FilterTypeToString(FilterType a_value) {
		switch (a_value) {
		case FilterType::kFormID: return "FilterByFormID";
		default: return std::string_view{};
		}
	}

	enum class ElementType {
		kBipedObjectSlots,
	};

	std::string_view ElementTypeToString(ElementType a_value) {
		switch (a_value) {
		case ElementType::kBipedObjectSlots: return "BipedObjectSlots";
		default: return std::string_view{};
		}
	}

	struct ConfigData {
		FilterType Filter;
		std::string FilterForm;
		ElementType Element;
		std::optional<std::uint32_t> AssignValue;
	};

	struct PatchData {
		std::optional<std::uint32_t> BipedObjectSlots;
	};

	std::vector<ConfigData> g_configVec;
	std::unordered_map<RE::TESObjectARMA*, PatchData> g_patchMap;

	class ArmorAddonParser : public Configs::Parser<ConfigData> {
	public:
		ArmorAddonParser(Configs::ConfigReader& a_configReader) : Configs::Parser<ConfigData>(a_configReader) {}

		std::optional<ConfigData> Parse() override {
			if (reader.EndOfFile() || reader.LookAhead().empty())
				return std::nullopt;

			ConfigData configData{};

			if (!parseFilter(configData))
				return std::nullopt;

			auto token = reader.GetToken();
			if (token != ".") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '.'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			if (!parseElement(configData))
				return std::nullopt;

			if (!parseAssignment(configData))
				return std::nullopt;

			token = reader.GetToken();
			if (token != ";") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ';'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			return configData;
		}

	private:
		bool parseFilter(ConfigData& a_config) {
			auto token = reader.GetToken();
			if (token == "FilterByFormID")
				a_config.Filter = FilterType::kFormID;
			else {
				logger::warn("Line {}, Col {}: Invalid FilterName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			token = reader.GetToken();
			if (token != "(") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '('.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			auto filterForm = parseForm();
			if (!filterForm.has_value())
				return false;

			a_config.FilterForm = filterForm.value();

			token = reader.GetToken();
			if (token != ")") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			return true;
		}

		bool parseElement(ConfigData& a_config) {
			auto token = reader.GetToken();
			if (token == "BipedObjectSlots")
				a_config.Element = ElementType::kBipedObjectSlots;
			else {
				logger::warn("Line {}, Col {}: Invalid ElementName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			return true;
		}

		bool parseAssignment(ConfigData& a_config) {
			auto token = reader.GetToken();
			if (token != "=") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '='.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			if (a_config.Element == ElementType::kBipedObjectSlots) {
				std::uint32_t bipedObjectSlotsValue = 0;

				auto bipedSlot = parseBipedSlot();
				if (!bipedSlot.has_value())
					return false;

				if (bipedSlot.value() != 0)
					bipedObjectSlotsValue |= 1 << (bipedSlot.value() - 30);

				while (true) {
					token = reader.LookAhead();
					if (token == ";")
						break;

					token = reader.GetToken();
					if (token != "|") {
						logger::warn("Line {}, Col {}: Syntax error. Expected '|' or ';'.", reader.GetLastLine(), reader.GetLastLineIndex());
						return false;
					}

					bipedSlot = parseBipedSlot();
					if (!bipedSlot.has_value())
						return false;

					if (bipedSlot.value() != 0)
						bipedObjectSlotsValue |= 1 << (bipedSlot.value() - 30);
				}

				a_config.AssignValue = bipedObjectSlotsValue;
			}
			else {
				logger::warn("Line {}, Col {}: Invalid Assignment to {}.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_config.Element));
				return false;
			}

			return true;
		}

		std::optional<std::uint32_t> parseBipedSlot() {
			unsigned long parsedValue;

			auto token = reader.GetToken();
			if (token.empty() || token == "|" || token == ";") {
				logger::warn("Line {}, Col {}: Expected BipedSlot '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return std::nullopt;
			}

			auto parsingResult = std::from_chars(token.data(), token.data() + token.size(), parsedValue);
			if (parsingResult.ec != std::errc()) {
				logger::warn("Line {}, Col {}: Failed to parse bipedSlot '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return std::nullopt;
			}

			if (parsedValue != 0 && (parsedValue < 30 || parsedValue > 61)) {
				logger::warn("Line {}, Col {}: Failed to parse bipedSlot '{}'. The value is out of range", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return std::nullopt;
			}

			return static_cast<std::uint32_t>(parsedValue);
		}

		std::optional<std::string> parseForm() {
			std::string form;

			auto token = reader.GetToken();
			if (!token.starts_with('\"')) {
				logger::warn("Line {}, Col {}: PluginName must be a string.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}
			else if (!token.ends_with('\"')) {
				logger::warn("Line {}, Col {}: String must end with '\"'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}
			form += token.substr(1, token.length() - 2);

			token = reader.GetToken();
			if (token != "|") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '|'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}
			form += token;

			token = reader.GetToken();
			if (token.empty() || token == ")") {
				logger::warn("Line {}, Col {}: Expected FormID '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return std::nullopt;
			}
			form += token;

			return form;
		}
	};

	std::string GetBipedSlots(std::uint32_t a_bipedObjSlots) {
		std::string retStr;
		std::string separtor = " | ";

		if (a_bipedObjSlots == 0)
			return "0";

		for (std::size_t ii = 0; ii < 32; ii++) {
			if (a_bipedObjSlots & (1 << ii))
				retStr += std::to_string(ii + 30) + separtor;
		}

		if (retStr.empty())
			return retStr;

		return retStr.substr(0, retStr.size() - separtor.size());
	}

	void ReadConfig(std::string_view a_path) {
		Configs::ConfigReader reader(a_path);

		while (!reader.EndOfFile()) {
			ArmorAddonParser parser(reader);
			auto configData = parser.Parse();
			if (!configData.has_value()) {
				parser.RecoverFromError();
				continue;
			}

			g_configVec.push_back(configData.value());

			if (configData->Element == ElementType::kBipedObjectSlots)
				logger::info("{}({}).{} = {};", FilterTypeToString(configData->Filter), configData->FilterForm,
					ElementTypeToString(configData->Element), GetBipedSlots(configData->AssignValue.value()));
		}
	}

	void ReadConfigs() {
		const std::filesystem::path configDir{ "Data\\" + std::string(Version::PROJECT) + "\\ArmorAddon" };
		if (!std::filesystem::exists(configDir))
			return;

		const std::regex filter(".*\\.cfg", std::regex_constants::icase);
		const std::filesystem::directory_iterator dir_iter(configDir);
		for (auto& iter : dir_iter) {
			if (!std::filesystem::is_regular_file(iter.status()))
				continue;

			if (!std::regex_match(iter.path().filename().string(), filter))
				continue;

			std::string path = iter.path().string();
			logger::info("=========== Reading ArmorAddon config file: {} ===========", path);
			ReadConfig(path);
			logger::info("");
		}
	}

	void Prepare(const std::vector<ConfigData>& a_configVec) {
		logger::info("======================== Start preparing patch for ArmorAddon ========================");

		for (const auto& configData : a_configVec) {
			if (configData.Filter == FilterType::kFormID) {
				RE::TESForm* filterForm = Utils::GetFormFromString(configData.FilterForm);
				if (!filterForm) {
					logger::warn("Invalid FilterForm: '{}'.", configData.FilterForm);
					continue;
				}

				RE::TESObjectARMA* arma = filterForm->As<RE::TESObjectARMA>();
				if (!arma) {
					logger::warn("'{}' is not a ArmorAddon.", configData.FilterForm);
					continue;
				}

				if (configData.Element == ElementType::kBipedObjectSlots) {
					if (configData.AssignValue.has_value())
						g_patchMap[arma].BipedObjectSlots = configData.AssignValue.value();
				}
			}
		}

		logger::info("======================== Finished preparing patch for ArmorAddon ========================");
		logger::info("");
	}

	void Patch() {
		Prepare(g_configVec);

		logger::info("======================== Start patching for ArmorAddon ========================");

		for (const auto& patchData : g_patchMap) {
			if (patchData.second.BipedObjectSlots.has_value())
				patchData.first->bipedModelData.bipedObjectSlots = patchData.second.BipedObjectSlots.value();
		}

		logger::info("======================== Finished patching for ArmorAddon ========================");
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}
