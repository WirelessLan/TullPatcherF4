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
		kBipedObjectSlots,
		kProperties
	};

	std::string_view ElementTypeToString(ElementType a_value) {
		switch (a_value) {
		case ElementType::kMaleSkeletalModel: return "MaleSkeletalModel";
		case ElementType::kFemaleSkeletalModel: return "FemaleSkeletalModel";
		case ElementType::kBodyPartData: return "BodyPartData";
		case ElementType::kBipedObjectSlots: return "BipedObjectSlots";
		case ElementType::kProperties: return "Properties";
		default: return std::string_view{};
		}
	}

	enum class OperationType {
		kClear,
		kSet,
		kDelete
	};

	std::string_view OperationTypeToString(OperationType a_value) {
		switch (a_value) {
		case OperationType::kClear: return "Clear";
		case OperationType::kSet: return "Set";
		case OperationType::kDelete: return "Delete";
		default: return std::string_view{};
		}
	}

	struct ConfigData {
		struct Operation {
			struct PropertyData {
				std::string ActorValueForm;
				float Value;
			};

			OperationType OpType;
			std::optional<PropertyData> OpProperty;
		};

		FilterType Filter;
		std::string FilterForm;
		ElementType Element;
		std::optional<std::any> AssignValue;
		std::vector<Operation> Operations;
	};

	struct PatchData {
		struct PropertiesData {
			struct Property {
				RE::ActorValueInfo* ActorValue;
				float Value;
			};

			bool Clear;
			std::vector<Property> SetPropertyVec;
			std::vector<Property> DeletePropertyVec;
		};

		std::optional<std::string> MaleSkeletalModel;
		std::optional<std::string> FemaleSkeletalModel;
		std::optional<RE::BGSBodyPartData*> BodyPartData;
		std::optional<std::uint32_t> BipedObjectSlots;
		std::optional<PropertiesData> Properties;
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

			token = reader.LookAhead();
			if (token == "=") {
				if (!parseAssignment(configData))
					return std::nullopt;

				token = reader.GetToken();
				if (token != ";") {
					logger::warn("Line {}, Col {}: Syntax error. Expected ';'.", reader.GetLastLine(), reader.GetLastLineIndex());
					return std::nullopt;
				}
			}
			else {
				token = reader.GetToken();
				if (token != ".") {
					logger::warn("Line {}, Col {}: Syntax error. Expected '.'.", reader.GetLastLine(), reader.GetLastLineIndex());
					return std::nullopt;
				}

				if (!parseOperation(configData))
					return std::nullopt;

				while (true) {
					token = reader.LookAhead();
					if (token == ";") {
						reader.GetToken();
						break;
					}

					token = reader.GetToken();
					if (token != ".") {
						logger::warn("Line {}, Col {}: Syntax error. Expected '.' or ';'.", reader.GetLastLine(), reader.GetLastLineIndex());
						return std::nullopt;
					}

					if (!parseOperation(configData))
						return std::nullopt;
				}
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
			else if (token == "Properties")
				a_config.Element = ElementType::kProperties;
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

				a_config.AssignValue = std::any(bipedObjectSlotsValue);
			}
			else {
				logger::warn("Line {}, Col {}: Invalid Assignment to {}.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_config.Element));
				return false;
			}

			return true;
		}

		bool parseOperation(ConfigData& a_config) {
			ConfigData::Operation newOp;

			auto token = reader.GetToken();
			if (token == "Clear")
				newOp.OpType = OperationType::kClear;
			else if (token == "Set")
				newOp.OpType = OperationType::kSet;
			else if (token == "Delete")
				newOp.OpType = OperationType::kDelete;
			else {
				logger::warn("Line {}, Col {}: Invalid OperationName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			if (newOp.OpType == OperationType::kClear || newOp.OpType == OperationType::kSet || newOp.OpType == OperationType::kDelete) {
				if (a_config.Element != ElementType::kProperties) {
					logger::warn("Line {}, Col {}: Invalid Operation '{}.{}()'.",
						reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_config.Element), OperationTypeToString(newOp.OpType));
					return false;
				}
			}

			token = reader.GetToken();
			if (token != "(") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '('.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			if (a_config.Element == ElementType::kProperties) {
				newOp.OpProperty = ConfigData::Operation::PropertyData{};

				if (newOp.OpType != OperationType::kClear) {
					std::optional<std::string> opForm = parseForm();
					if (!opForm.has_value())
						return false;

					newOp.OpProperty->ActorValueForm = opForm.value();

					if (newOp.OpType == OperationType::kSet) {
						token = reader.GetToken();
						if (token != ",") {
							logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
							return false;
						}

						std::optional<float> opValue = parseNumber();
						if (!opValue.has_value())
							return false;

						newOp.OpProperty->Value = opValue.value();
					}
				}
			}

			token = reader.GetToken();
			if (token != ")") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			a_config.Operations.push_back(newOp);

			return true;

			return false;
		}

		std::optional<float> parseNumber() {
			auto token = reader.GetToken();
			if (token.empty() || token == ")") {
				logger::warn("Line {}, Col {}: Expected number '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return std::nullopt;
			}

			std::string magStr = std::string(token);
			if (reader.LookAhead() == ".") {
				magStr += reader.GetToken();

				token = reader.GetToken();
				if (token.empty() || token == ")") {
					logger::warn("Line {}, Col {}: Expected number's decimal '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return std::nullopt;
				}

				magStr += std::string(token);
			}

			float fParsedValue;
			try {
				fParsedValue = std::stof(magStr);
			}
			catch (...) {
				logger::warn("Line {}, Col {}: Failed to parse number '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return std::nullopt;
			}

			return fParsedValue;
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

			if (parsedValue != 0 && (parsedValue < 30 || parsedValue > 61)) {
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
			else if (configData->Element == ElementType::kProperties) {
				logger::info("{}({}).{}", FilterTypeToString(configData->Filter), configData->FilterForm, ElementTypeToString(configData->Element));
				for (std::size_t ii = 0; ii < configData->Operations.size(); ii++) {
					std::string opLog;
					
					if (configData->Operations[ii].OpType == OperationType::kClear)
						opLog = fmt::format(".{}()", OperationTypeToString(configData->Operations[ii].OpType));
					else if (configData->Operations[ii].OpType == OperationType::kSet)
						opLog = fmt::format(".{}({}, {})", OperationTypeToString(configData->Operations[ii].OpType),
							configData->Operations[ii].OpProperty->ActorValueForm, configData->Operations[ii].OpProperty->Value);
					else if (configData->Operations[ii].OpType == OperationType::kDelete)
						opLog = fmt::format(".{}({})", OperationTypeToString(configData->Operations[ii].OpType),
							configData->Operations[ii].OpProperty->ActorValueForm);

					if (ii == configData->Operations.size() - 1)
						opLog += ";";

					logger::info("    {}", opLog);
				}
			}
			else
				logger::info("{}({}).{} = \"{}\";", FilterTypeToString(configData->Filter), configData->FilterForm, 
					ElementTypeToString(configData->Element), std::any_cast<std::string>(configData->AssignValue.value()));
		}
	}

	void ReadConfigs() {
		const std::filesystem::path configDir{ "Data\\" + std::string(Version::PROJECT) + "\\Race" };
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
				else if (configData.Element == ElementType::kProperties) {
					PatchData& patchData = g_patchMap[race];

					if (!patchData.Properties.has_value())
						patchData.Properties = PatchData::PropertiesData{};

					for (const auto& op : configData.Operations) {
						if (op.OpType == OperationType::kClear) {
							patchData.Properties->Clear = true;
						}
						else if (op.OpType == OperationType::kSet || op.OpType == OperationType::kDelete) {
							RE::TESForm* opForm = Utils::GetFormFromString(op.OpProperty->ActorValueForm);
							if (!opForm) {
								logger::warn("Invalid Form: '{}'.", op.OpProperty->ActorValueForm);
								continue;
							}

							RE::ActorValueInfo* avInfo = opForm->As<RE::ActorValueInfo>();
							if (!avInfo) {
								logger::warn("'{}' is not a ActorValue", op.OpProperty->ActorValueForm);
								continue;
							}

							if (op.OpType == OperationType::kSet)
								patchData.Properties->SetPropertyVec.push_back({ avInfo, op.OpProperty->Value });
							else
								patchData.Properties->DeletePropertyVec.push_back({ avInfo, 0 });
						}
					}
				}
			}
		}

		logger::info("======================== Finished preparing patch for Race ========================");
		logger::info("");
	}

	void PatchProperties(RE::TESRace* a_race, const PatchData::PropertiesData& a_propertiesData) {
		if (!a_race->properties)
			return;

		bool isCleared = false;

		// Clear
		if (a_propertiesData.Clear)
			a_race->properties->clear();

		// Delete
		if (!isCleared) {
			for (const auto& delProp : a_propertiesData.DeletePropertyVec) {
				for (auto it = a_race->properties->begin(); it != a_race->properties->end(); it++) {
					if (delProp.ActorValue != it->first)
						continue;

					a_race->properties->erase(it);
					break;
				}
			}
		}

		// Set
		for (const auto& setProp : a_propertiesData.SetPropertyVec) {
			bool found = false;

			for (auto& raceProp : *a_race->properties) {
				if (setProp.ActorValue != raceProp.first)
					continue;

				found = true;
				raceProp.second.f = setProp.Value;
				break;
			}

			if (found)
				continue;

			RE::BSTTuple<RE::TESForm*, RE::BGSTypedFormValuePair::SharedVal> nTup;
			nTup.first = setProp.ActorValue;
			nTup.second.f = setProp.Value;

			a_race->properties->push_back(nTup);
		}
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

			if (patchData.second.Properties.has_value())
				PatchProperties(patchData.first, patchData.second.Properties.value());
		}

		logger::info("======================== Finished patching for Race ========================");
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}
