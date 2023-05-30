#include "NPCs.h"

#include <regex>
#include <any>

#include "Configs.h"
#include "Utils.h"

namespace NPCs {
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
		kRace,
		kSex
	};

	std::string_view ElementTypeToString(ElementType a_value) {
		switch (a_value) {
		case ElementType::kRace: return "Race";
		case ElementType::kSex: return "Sex";
		default: return std::string_view{};
		}
	}

	struct ConfigData {
		FilterType Filter;
		std::string FilterForm;
		ElementType Element;
		std::optional<std::any> AssignValue;
	};

	struct PatchData {
		std::optional<RE::TESRace*> Race;
		std::optional<std::uint8_t> Sex;
	};

	std::vector<ConfigData> g_configVec;
	std::unordered_map<RE::TESNPC*, PatchData> g_patchMap;

	class NPCParser : public Configs::Parser<ConfigData> {
	public:
		NPCParser(Configs::ConfigReader& a_configReader) : Configs::Parser<ConfigData>(a_configReader) {}

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
			if (token == "Race")
				a_config.Element = ElementType::kRace;
			else if (token == "Sex")
				a_config.Element = ElementType::kSex;
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

			if (a_config.Element == ElementType::kRace) {
				auto raceForm = parseForm();
				if (!raceForm.has_value())
					return false;

				a_config.AssignValue = std::any(raceForm.value());
			}
			else if (a_config.Element == ElementType::kSex) {
				token = reader.GetToken();
				if (token.empty() || token == ";") {
					logger::warn("Line {}, Col {}: Expected Sex '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				unsigned long parsedValue;
				auto parsingResult = std::from_chars(token.data(), token.data() + token.size(), parsedValue);
				if (parsingResult.ec != std::errc()) {
					logger::warn("Line {}, Col {}: Failed to parse sex '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				if (parsedValue != 0 && parsedValue != 1) {
					logger::warn("Line {}, Col {}: Failed to parse level '{}'. The value is out of range", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				a_config.AssignValue = std::any(static_cast<std::uint8_t>(parsedValue));
			}

			return true;
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

	void ReadConfig(std::string_view a_path) {
		Configs::ConfigReader reader(a_path);

		while (!reader.EndOfFile()) {
			NPCParser parser(reader);
			auto configData = parser.Parse();
			if (!configData.has_value()) {
				parser.RecoverFromError();
				continue;
			}

			g_configVec.push_back(configData.value());

			if (configData->Element == ElementType::kRace)
				logger::info("{}({}).{} = {};", FilterTypeToString(configData->Filter), configData->FilterForm,
					ElementTypeToString(configData->Element), std::any_cast<std::string>(configData->AssignValue.value()));
			else if (configData->Element == ElementType::kSex)
				logger::info("{}({}).{} = {};", FilterTypeToString(configData->Filter), configData->FilterForm,
					ElementTypeToString(configData->Element), std::any_cast<std::uint8_t>(configData->AssignValue.value()));
		}
	}

	void ReadConfigs() {
		const std::filesystem::path configDir{ "Data\\" + std::string(Version::PROJECT) + "\\NPC" };
		const std::regex filter(".*\\.cfg", std::regex_constants::icase);
		const std::filesystem::directory_iterator dir_iter(configDir);
		for (auto& iter : dir_iter) {
			if (!std::filesystem::is_regular_file(iter.status()))
				continue;

			if (!std::regex_match(iter.path().filename().string(), filter))
				continue;

			std::string path = iter.path().string();
			logger::info("=========== Reading NPC config file: {} ===========", path);
			ReadConfig(path);
			logger::info("");
		}
	}

	void Prepare(const std::vector<ConfigData>& a_configVec) {
		logger::info("======================== Start preparing patch for NPC ========================");

		for (const auto& configData : a_configVec) {
			if (configData.Filter == FilterType::kFormID) {
				RE::TESForm* filterForm = Utils::GetFormFromString(configData.FilterForm);
				if (!filterForm) {
					logger::warn("Invalid FilterForm: '{}'.", configData.FilterForm);
					continue;
				}

				RE::TESNPC* npc = filterForm->As<RE::TESNPC>();
				if (!npc) {
					logger::warn("'{}' is not a NPC.", configData.FilterForm);
					continue;
				}

				if (configData.Element == ElementType::kRace) {
					if (configData.AssignValue.has_value()) {
						std::string raceFormStr = std::any_cast<std::string>(configData.AssignValue.value());
						RE::TESForm* raceForm = Utils::GetFormFromString(raceFormStr);
						if (!raceForm) {
							logger::warn("Invalid Form: '{}'.", raceFormStr);
							continue;
						}

						RE::TESRace* race = raceForm->As<RE::TESRace>();
						if (!race) {
							logger::warn("'{}' is not a Race.", raceFormStr);
							continue;
						}

						g_patchMap[npc].Race = race;
					}
				}
				else if (configData.Element == ElementType::kSex) {
					if (configData.AssignValue.has_value()) {
						g_patchMap[npc].Sex = std::any_cast<std::uint8_t>(configData.AssignValue.value());
					}
				}
			}
		}

		logger::info("======================== Finished preparing patch for NPC ========================");
		logger::info("");
	}

	void Patch(RE::TESNPC* a_npc, const PatchData& a_patchData) {
		if (a_patchData.Race.has_value())
			a_npc->formRace = a_patchData.Race.value();

		if (a_patchData.Sex.has_value()) {
			bool value = a_patchData.Sex.value();

			if (value)	// Female
				a_npc->actorData.actorBaseFlags |= RE::ACTOR_BASE_DATA::Flag::kFemale;
			else		// Male
				if (a_npc->actorData.actorBaseFlags & RE::ACTOR_BASE_DATA::Flag::kFemale)
					a_npc->actorData.actorBaseFlags -= RE::ACTOR_BASE_DATA::Flag::kFemale;
		}
	}

	void Patch() {
		Prepare(g_configVec);

		logger::info("======================== Start patching for NPC ========================");

		for (const auto& patchData : g_patchMap)
			Patch(patchData.first, patchData.second);

		logger::info("======================== Finished patching for NPC ========================");
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}
