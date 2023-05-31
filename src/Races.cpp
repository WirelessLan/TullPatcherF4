#include "Races.h"

#include <regex>
#include <any>

#include "Configs.h"
#include "Utils.h"

namespace Races {
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
		kMaleSkeletalModel,
		kFemaleSkeletalModel,
		kBodyPartData,
		kBipedObjectSlots
	};

	std::string_view ElementTypeToString(ElementType a_value) {
		switch (a_value) {
		case ElementType::kMaleSkeletalModel: return "MaleSkeletalModel";
		case ElementType::kFemaleSkeletalModel: return "FemaleSkeletalModel";
		case ElementType::kBodyPartData: return "BodyPartData";
		case ElementType::kBipedObjectSlots: return "BipedObjectSlots";
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
		std::optional<std::string> MaleSkeletalModel;
		std::optional<std::string> FemaleSkeletalModel;
		std::optional<RE::BGSBodyPartData*> BodyPartData;
		std::optional<std::uint32_t> BipedObjectSlots;
	};

	std::vector<ConfigData> g_configVec;
	std::unordered_map<RE::TESRace*, PatchData> g_patchMap;

	class RaceParser : public Configs::Parser<ConfigData> {
	public:
		RaceParser(Configs::ConfigReader& a_configReader) : Configs::Parser<ConfigData>(a_configReader) {}

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
			if (token == "MaleSkeletalModel")
				a_config.Element = ElementType::kMaleSkeletalModel;
			else if (token == "FemaleSkeletalModel")
				a_config.Element = ElementType::kFemaleSkeletalModel;
			else if (token == "BodyPartData")
				a_config.Element = ElementType::kBodyPartData;
			else if (token == "BipedObjectSlots")
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

			if (a_config.Element == ElementType::kMaleSkeletalModel || a_config.Element == ElementType::kFemaleSkeletalModel) {
				token = reader.GetToken();
				if (!token.starts_with('\"')) {
					logger::warn("Line {}, Col {}: {} must be a string.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_config.Element));
					return false;
				}
				else if (!token.ends_with('\"')) {
					logger::warn("Line {}, Col {}: String must end with '\"'.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}

				std::string value = std::string(token.substr(1, token.length() - 2));
				a_config.AssignValue = std::any(value);
			}
			else if (a_config.Element == ElementType::kBodyPartData) {
				auto bodyPartDataForm = parseForm();
				if (!bodyPartDataForm.has_value())
					return false;

				a_config.AssignValue = std::any(bodyPartDataForm.value());
			}
			else if (a_config.Element == ElementType::kBipedObjectSlots) {
				std::uint32_t bipedObjectSlotsValue = 0;
				
				auto bipedSlot = parseBipedSlot();
				if (!bipedSlot.has_value())
					return false;

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

					bipedObjectSlotsValue |= 1 << (bipedSlot.value() - 30);
				}

				a_config.AssignValue = std::any(bipedObjectSlotsValue);
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
				logger::warn("Line {}, Col {}: Failed to parse bipedslot '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return std::nullopt;
			}

			if (parsedValue < 30 || parsedValue > 61) {
				logger::warn("Line {}, Col {}: Failed to parse bipedslot '{}'. The value is out of range", reader.GetLastLine(), reader.GetLastLineIndex(), token);
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
			RaceParser parser(reader);
			auto configData = parser.Parse();
			if (!configData.has_value()) {
				parser.RecoverFromError();
				continue;
			}

			g_configVec.push_back(configData.value());

			if (configData->Element == ElementType::kBodyPartData)
				logger::info("{}({}).{} = {};", FilterTypeToString(configData->Filter), configData->FilterForm, 
					ElementTypeToString(configData->Element), std::any_cast<std::string>(configData->AssignValue.value()));
			else if (configData->Element == ElementType::kBipedObjectSlots)
				logger::info("{}({}).{} = {};", FilterTypeToString(configData->Filter), configData->FilterForm,
					ElementTypeToString(configData->Element), GetBipedSlots(std::any_cast<std::uint32_t>(configData->AssignValue.value())));
			else
				logger::info("{}({}).{} = \"{}\";", FilterTypeToString(configData->Filter), configData->FilterForm, 
					ElementTypeToString(configData->Element), std::any_cast<std::string>(configData->AssignValue.value()));
		}
	}

	void ReadConfigs() {
		const std::filesystem::path configDir{ "Data\\" + std::string(Version::PROJECT) + "\\Race" };
		const std::regex filter(".*\\.cfg", std::regex_constants::icase);
		const std::filesystem::directory_iterator dir_iter(configDir);
		for (auto& iter : dir_iter) {
			if (!std::filesystem::is_regular_file(iter.status()))
				continue;

			if (!std::regex_match(iter.path().filename().string(), filter))
				continue;

			std::string path = iter.path().string();
			logger::info("=========== Reading Race config file: {} ===========", path);
			ReadConfig(path);
			logger::info("");
		}
	}

	void Prepare(const std::vector<ConfigData>& a_configVec) {
		logger::info("======================== Start preparing patch for Race ========================");

		for (const auto& configData : a_configVec) {
			if (configData.Filter == FilterType::kFormID) {
				RE::TESForm* filterForm = Utils::GetFormFromString(configData.FilterForm);
				if (!filterForm) {
					logger::warn("Invalid FilterForm: '{}'.", configData.FilterForm);
					continue;
				}

				RE::TESRace* race = filterForm->As<RE::TESRace>();
				if (!race) {
					logger::warn("'{}' is not a Race.", configData.FilterForm);
					continue;
				}

				if (configData.Element == ElementType::kMaleSkeletalModel) {
					if (configData.AssignValue.has_value())
						g_patchMap[race].MaleSkeletalModel = std::any_cast<std::string>(configData.AssignValue.value());
				}
				else if (configData.Element == ElementType::kFemaleSkeletalModel) {
					if (configData.AssignValue.has_value())
						g_patchMap[race].FemaleSkeletalModel = std::any_cast<std::string>(configData.AssignValue.value());
				}
				else if (configData.Element == ElementType::kBodyPartData) {
					if (!configData.AssignValue.has_value())
						continue;

					std::string bodyPartDataFormStr = std::any_cast<std::string>(configData.AssignValue.value());
					RE::TESForm* bodyPartDataForm = Utils::GetFormFromString(bodyPartDataFormStr);
					if (!bodyPartDataForm) {
						logger::warn("Invalid Form: '{}'.", bodyPartDataFormStr);
						continue;
					}

					RE::BGSBodyPartData* bodyPartData = bodyPartDataForm->As<RE::BGSBodyPartData>();
					if (!bodyPartData) {
						logger::warn("'{}' is not a BodyPartData.", bodyPartDataFormStr);
						continue;
					}

					g_patchMap[race].BodyPartData = bodyPartData;
				}
				else if (configData.Element == ElementType::kBipedObjectSlots) {
					if (configData.AssignValue.has_value())
						g_patchMap[race].BipedObjectSlots = std::any_cast<std::uint32_t>(configData.AssignValue.value());
				}
			}
		}

		logger::info("======================== Finished preparing patch for Race ========================");
		logger::info("");
	}

	void Patch() {
		Prepare(g_configVec);

		logger::info("======================== Start patching for Race ========================");

		for (const auto& patchData : g_patchMap) {
			if (patchData.second.MaleSkeletalModel.has_value())
				patchData.first->skeletonModel[0].SetModel(patchData.second.MaleSkeletalModel.value().c_str());

			if (patchData.second.FemaleSkeletalModel.has_value())
				patchData.first->skeletonModel[1].SetModel(patchData.second.FemaleSkeletalModel.value().c_str());

			if (patchData.second.BodyPartData.has_value())
				patchData.first->bodyPartData = patchData.second.BodyPartData.value();

			if (patchData.second.BipedObjectSlots.has_value())
				patchData.first->bipedModelData.bipedObjectSlots = patchData.second.BipedObjectSlots.value();
		}

		logger::info("======================== Finished patching for Race ========================");
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}
