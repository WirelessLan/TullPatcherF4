#include "Races.h"

#include <unordered_set>
#include <regex>
#include <any>

#include "ConfigUtils.h"
#include "Parsers.h"
#include "Utils.h"

namespace Races {
	constexpr std::string_view TypeName = "Race";

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
		kProperties,
		kMalePresets,
		kFemalePresets
	};

	std::string_view ElementTypeToString(ElementType a_value) {
		switch (a_value) {
		case ElementType::kMaleSkeletalModel: return "MaleSkeletalModel";
		case ElementType::kFemaleSkeletalModel: return "FemaleSkeletalModel";
		case ElementType::kBodyPartData: return "BodyPartData";
		case ElementType::kBipedObjectSlots: return "BipedObjectSlots";
		case ElementType::kProperties: return "Properties";
		case ElementType::kMalePresets: return "MalePresets";
		case ElementType::kFemalePresets: return "FemalePresets";
		default: return std::string_view{};
		}
	}

	enum class OperationType {
		kClear,
		kSet,
		kAdd,
		kAddIfNotExists,
		kDelete
	};

	std::string_view OperationTypeToString(OperationType a_value) {
		switch (a_value) {
		case OperationType::kClear: return "Clear";
		case OperationType::kSet: return "Set";
		case OperationType::kAdd: return "Add";
		case OperationType::kAddIfNotExists: return "AddIfNotExists";
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
			std::optional<std::any> OpData;
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

		struct PresetsData {
			bool Clear;
			std::vector<RE::TESNPC*> AddPresetVec;
			std::unordered_set<RE::TESNPC*> AddUniquePresetSet;
			std::vector<RE::TESNPC*> DeletePresetVec;
		};

		std::optional<std::string> MaleSkeletalModel;
		std::optional<std::string> FemaleSkeletalModel;
		std::optional<RE::BGSBodyPartData*> BodyPartData;
		std::optional<std::uint32_t> BipedObjectSlots;
		std::optional<PropertiesData> Properties;
		std::optional<PresetsData> MalePresets;
		std::optional<PresetsData> FemalePresets;
	};

	std::vector<Parsers::Statement<ConfigData>> g_configVec;
	std::unordered_map<RE::TESRace*, PatchData> g_patchMap;

	class RaceParser : public Parsers::Parser<ConfigData> {
	public:
		RaceParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

	protected:
		std::optional<Parsers::Statement<ConfigData>> ParseExpressionStatement() override {
			if (reader.EndOfFile() || reader.Peek().empty()) {
				return std::nullopt;
			}

			ConfigData configData{};

			if (!ParseFilter(configData)) {
				return std::nullopt;
			}

			auto token = reader.GetToken();
			if (token != ".") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '.'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			if (!ParseElement(configData)) {
				return std::nullopt;
			}

			token = reader.Peek();
			if (token == "=") {
				if (!ParseAssignment(configData)) {
					return std::nullopt;
				}

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

				if (!ParseOperation(configData)) {
					return std::nullopt;
				}

				while (true) {
					token = reader.Peek();
					if (token == ";") {
						reader.GetToken();
						break;
					}

					token = reader.GetToken();
					if (token != ".") {
						logger::warn("Line {}, Col {}: Syntax error. Expected '.' or ';'.", reader.GetLastLine(), reader.GetLastLineIndex());
						return std::nullopt;
					}

					if (!ParseOperation(configData)) {
						return std::nullopt;
					}
				}
			}

			return Parsers::Statement<ConfigData>::CreateExpressionStatement(configData);
		}

		void PrintExpressionStatement(const ConfigData& a_configData, int a_indent) override {
			std::string indent = std::string(a_indent * 4, ' ');

			switch (a_configData.Element) {
			case ElementType::kBodyPartData:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm,
					ElementTypeToString(a_configData.Element), std::any_cast<std::string>(a_configData.AssignValue.value()));
				break;

			case ElementType::kBipedObjectSlots:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm,
					ElementTypeToString(a_configData.Element), GetBipedSlots(std::any_cast<std::uint32_t>(a_configData.AssignValue.value())));
				break;

			case ElementType::kProperties:
				logger::info("{}{}({}).{}", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element));
				for (std::size_t ii = 0; ii < a_configData.Operations.size(); ii++) {
					std::string opLog;

					ConfigData::Operation::PropertyData propData = std::any_cast<ConfigData::Operation::PropertyData>(a_configData.Operations[ii].OpData.value());

					switch (a_configData.Operations[ii].OpType) {
					case OperationType::kClear:
						opLog = fmt::format(".{}()", OperationTypeToString(a_configData.Operations[ii].OpType));
						break;

					case OperationType::kSet:
						opLog = fmt::format(".{}({}, {})", OperationTypeToString(a_configData.Operations[ii].OpType),
							propData.ActorValueForm, propData.Value);
						break;

					case OperationType::kDelete:
						opLog = fmt::format(".{}({})", OperationTypeToString(a_configData.Operations[ii].OpType),
							propData.ActorValueForm);
						break;
					}

					if (ii == a_configData.Operations.size() - 1) {
						opLog += ";";
					}

					logger::info("{}    {}", indent, opLog);
				}
				break;

			case ElementType::kMalePresets:
			case ElementType::kFemalePresets:
				logger::info("{}{}({}).{}", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element));
				for (std::size_t ii = 0; ii < a_configData.Operations.size(); ii++) {
					std::string opLog;

					switch (a_configData.Operations[ii].OpType) {
					case OperationType::kClear:
						opLog = fmt::format(".{}()", OperationTypeToString(a_configData.Operations[ii].OpType));
						break;

					case OperationType::kAdd:
					case OperationType::kAddIfNotExists:
					case OperationType::kDelete:
						opLog = fmt::format(".{}({})", OperationTypeToString(a_configData.Operations[ii].OpType),
							std::any_cast<std::string>(a_configData.Operations[ii].OpData.value()));
						break;
					}

					if (ii == a_configData.Operations.size() - 1) {
						opLog += ";";
					}

					logger::info("{}    {}", indent, opLog);
				}
				break;

			case ElementType::kMaleSkeletalModel:
			case ElementType::kFemaleSkeletalModel:
				logger::info("{}{}({}).{} = \"{}\";", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm,
					ElementTypeToString(a_configData.Element), std::any_cast<std::string>(a_configData.AssignValue.value()));
				break;
			}
		}

		bool ParseFilter(ConfigData& a_config) {
			auto token = reader.GetToken();
			if (token == "FilterByFormID") {
				a_config.Filter = FilterType::kFormID;
			}
			else {
				logger::warn("Line {}, Col {}: Invalid FilterName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			token = reader.GetToken();
			if (token != "(") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '('.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			auto filterForm = ParseForm();
			if (!filterForm.has_value()) {
				return false;
			}

			a_config.FilterForm = filterForm.value();

			token = reader.GetToken();
			if (token != ")") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			return true;
		}

		bool ParseElement(ConfigData& a_config) {
			auto token = reader.GetToken();
			if (token == "MaleSkeletalModel") {
				a_config.Element = ElementType::kMaleSkeletalModel;
			}
			else if (token == "FemaleSkeletalModel") {
				a_config.Element = ElementType::kFemaleSkeletalModel;
			}
			else if (token == "BodyPartData") {
				a_config.Element = ElementType::kBodyPartData;
			}
			else if (token == "BipedObjectSlots") {
				a_config.Element = ElementType::kBipedObjectSlots;
			}
			else if (token == "Properties") {
				a_config.Element = ElementType::kProperties;
			}
			else if (token == "MalePresets") {
				a_config.Element = ElementType::kMalePresets;
			}
			else if (token == "FemalePresets") {
				a_config.Element = ElementType::kFemalePresets;
			}
			else {
				logger::warn("Line {}, Col {}: Invalid ElementName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			return true;
		}

		bool ParseAssignment(ConfigData& a_config) {
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
				auto bodyPartDataForm = ParseForm();
				if (!bodyPartDataForm.has_value()) {
					return false;
				}

				a_config.AssignValue = std::any(bodyPartDataForm.value());
			}
			else if (a_config.Element == ElementType::kBipedObjectSlots) {
				std::uint32_t bipedObjectSlotsValue = 0;
				
				auto bipedSlot = ParseBipedSlot();
				if (!bipedSlot.has_value()) {
					return false;
				}

				if (bipedSlot.value() != 0) {
					bipedObjectSlotsValue |= 1u << (bipedSlot.value() - 30);
				}

				while (true) {
					token = reader.Peek();
					if (token == ";") {
						break;
					}

					token = reader.GetToken();
					if (token != "|") {
						logger::warn("Line {}, Col {}: Syntax error. Expected '|' or ';'.", reader.GetLastLine(), reader.GetLastLineIndex());
						return false;
					}

					bipedSlot = ParseBipedSlot();
					if (!bipedSlot.has_value()) {
						return false;
					}

					if (bipedSlot.value() != 0) {
						bipedObjectSlotsValue |= 1u << (bipedSlot.value() - 30);
					}
				}

				a_config.AssignValue = std::any(bipedObjectSlotsValue);
			}
			else {
				logger::warn("Line {}, Col {}: Invalid Assignment to {}.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_config.Element));
				return false;
			}

			return true;
		}

		bool ParseOperation(ConfigData& a_config) {
			ConfigData::Operation newOp;

			auto token = reader.GetToken();
			if (token == "Clear") {
				newOp.OpType = OperationType::kClear;
			}
			else if (token == "Set") {
				newOp.OpType = OperationType::kSet;
			}
			else if (token == "Add") {
				newOp.OpType = OperationType::kAdd;
			}
			else if (token == "AddIfNotExists") {
				newOp.OpType = OperationType::kAddIfNotExists;
			}
			else if (token == "Delete") {
				newOp.OpType = OperationType::kDelete;
			}
			else {
				logger::warn("Line {}, Col {}: Invalid OperationName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			switch (a_config.Element) {
			case ElementType::kProperties:
				if (newOp.OpType != OperationType::kClear && newOp.OpType != OperationType::kSet && newOp.OpType != OperationType::kDelete) {
					logger::warn("Line {}, Col {}: Invalid Operation '{}.{}()'.",
						reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_config.Element), OperationTypeToString(newOp.OpType));
					return false;
				}
				break;

			case ElementType::kMalePresets:
			case ElementType::kFemalePresets:
				if (newOp.OpType != OperationType::kClear && newOp.OpType != OperationType::kAdd && newOp.OpType != OperationType::kAddIfNotExists && newOp.OpType != OperationType::kDelete) {
					logger::warn("Line {}, Col {}: Invalid Operation '{}.{}()'.",
						reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_config.Element), OperationTypeToString(newOp.OpType));
					return false;
				}
				break;

			default:
				logger::warn("Line {}, Col {}: Invalid Operation '{}.{}()'.",
					reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_config.Element), OperationTypeToString(newOp.OpType));
				return false;
			}

			token = reader.GetToken();
			if (token != "(") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '('.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			switch (a_config.Element) {
			case ElementType::kProperties: {
				ConfigData::Operation::PropertyData newPropData = ConfigData::Operation::PropertyData{};

				if (newOp.OpType != OperationType::kClear) {
					std::optional<std::string> opForm = ParseForm();
					if (!opForm.has_value()) {
						return false;
					}

					newPropData.ActorValueForm = opForm.value();

					if (newOp.OpType == OperationType::kSet) {
						token = reader.GetToken();
						if (token != ",") {
							logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
							return false;
						}

						std::optional<float> opValue = ParseNumber();
						if (!opValue.has_value()) {
							return false;
						}

						newPropData.Value = opValue.value();
					}
				}

				newOp.OpData = std::any(newPropData);

				break;
			}

			case ElementType::kMalePresets:
			case ElementType::kFemalePresets:
				if (newOp.OpType != OperationType::kClear) {
					std::optional<std::string> opForm = ParseForm();
					if (!opForm.has_value()) {
						return false;
					}

					newOp.OpData = std::any(opForm.value());
				}

				break;
			}

			token = reader.GetToken();
			if (token != ")") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			a_config.Operations.push_back(newOp);

			return true;
		}
	};

	void ReadConfigs() {
		g_configVec = ConfigUtils::ReadConfigs<RaceParser, Parsers::Statement<ConfigData>>(TypeName);
	}

	void Prepare(const ConfigData& a_configData) {
		if (a_configData.Filter == FilterType::kFormID) {
			RE::TESForm* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
			if (!filterForm) {
				logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
				return;
			}

			RE::TESRace* race = filterForm->As<RE::TESRace>();
			if (!race) {
				logger::warn("'{}' is not a Race.", a_configData.FilterForm);
				return;
			}

			if (a_configData.Element == ElementType::kMaleSkeletalModel) {
				g_patchMap[race].MaleSkeletalModel = std::any_cast<std::string>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kFemaleSkeletalModel) {
				g_patchMap[race].FemaleSkeletalModel = std::any_cast<std::string>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kBodyPartData) {
				std::string bodyPartDataFormStr = std::any_cast<std::string>(a_configData.AssignValue.value());

				RE::TESForm* bodyPartDataForm = Utils::GetFormFromString(bodyPartDataFormStr);
				if (!bodyPartDataForm) {
					logger::warn("Invalid Form: '{}'.", bodyPartDataFormStr);
					return;
				}

				RE::BGSBodyPartData* bodyPartData = bodyPartDataForm->As<RE::BGSBodyPartData>();
				if (!bodyPartData) {
					logger::warn("'{}' is not a BodyPartData.", bodyPartDataFormStr);
					return;
				}

				g_patchMap[race].BodyPartData = bodyPartData;
			}
			else if (a_configData.Element == ElementType::kBipedObjectSlots) {
				g_patchMap[race].BipedObjectSlots = std::any_cast<std::uint32_t>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kProperties) {
				PatchData& patchData = g_patchMap[race];

				if (!patchData.Properties.has_value()) {
					patchData.Properties = PatchData::PropertiesData{};
				}

				for (const auto& op : a_configData.Operations) {
					if (op.OpType == OperationType::kClear) {
						patchData.Properties->Clear = true;
					}
					else if (op.OpType == OperationType::kSet || op.OpType == OperationType::kDelete) {
						ConfigData::Operation::PropertyData propData = std::any_cast<ConfigData::Operation::PropertyData>(op.OpData.value());
						RE::TESForm* opForm = Utils::GetFormFromString(propData.ActorValueForm);
						if (!opForm) {
							logger::warn("Invalid Form: '{}'.", propData.ActorValueForm);
							continue;
						}

						RE::ActorValueInfo* avInfo = opForm->As<RE::ActorValueInfo>();
						if (!avInfo) {
							logger::warn("'{}' is not a ActorValue.", propData.ActorValueForm);
							continue;
						}

						if (op.OpType == OperationType::kSet) {
							patchData.Properties->SetPropertyVec.push_back({ avInfo, propData.Value });
						}
						else {
							patchData.Properties->DeletePropertyVec.push_back({ avInfo, 0 });
						}
					}
				}
			}
			else if (a_configData.Element == ElementType::kMalePresets) {
				PatchData& patchData = g_patchMap[race];

				if (!patchData.MalePresets.has_value()) {
					patchData.MalePresets = PatchData::PresetsData{};
				}

				for (const auto& op : a_configData.Operations) {
					if (op.OpType == OperationType::kClear) {
						patchData.MalePresets->Clear = true;
					}
					else if (op.OpType == OperationType::kAdd || op.OpType == OperationType::kAddIfNotExists || op.OpType == OperationType::kDelete) {
						std::string opFormStr = std::any_cast<std::string>(op.OpData.value());

						RE::TESForm* opForm = Utils::GetFormFromString(opFormStr);
						if (!opForm) {
							logger::warn("Invalid Form: '{}'.", opFormStr);
							continue;
						}

						RE::TESNPC* presetNPC = opForm->As<RE::TESNPC>();
						if (!presetNPC) {
							logger::warn("'{}' is not a NPC.", opFormStr);
							continue;
						}

						if (op.OpType == OperationType::kAdd) {
							patchData.MalePresets->AddPresetVec.push_back(presetNPC);
						}
						else if (op.OpType == OperationType::kAddIfNotExists) {
							patchData.MalePresets->AddUniquePresetSet.insert(presetNPC);
						}
						else {
							patchData.MalePresets->DeletePresetVec.push_back(presetNPC);
						}
					}
				}
			}
			else if (a_configData.Element == ElementType::kFemalePresets) {
				PatchData& patchData = g_patchMap[race];

				if (!patchData.FemalePresets.has_value()) {
					patchData.FemalePresets = PatchData::PresetsData{};
				}

				for (const auto& op : a_configData.Operations) {
					if (op.OpType == OperationType::kClear) {
						patchData.FemalePresets->Clear = true;
					}
					else if (op.OpType == OperationType::kAdd || op.OpType == OperationType::kAddIfNotExists || op.OpType == OperationType::kDelete) {
						std::string opFormStr = std::any_cast<std::string>(op.OpData.value());

						RE::TESForm* opForm = Utils::GetFormFromString(opFormStr);
						if (!opForm) {
							logger::warn("Invalid Form: '{}'.", opFormStr);
							continue;
						}

						RE::TESNPC* presetNPC = opForm->As<RE::TESNPC>();
						if (!presetNPC) {
							logger::warn("'{}' is not a NPC.", opFormStr);
							continue;
						}

						if (op.OpType == OperationType::kAdd) {
							patchData.FemalePresets->AddPresetVec.push_back(presetNPC);
						}
						else if (op.OpType == OperationType::kAddIfNotExists) {
							patchData.FemalePresets->AddUniquePresetSet.insert(presetNPC);
						}
						else {
							patchData.FemalePresets->DeletePresetVec.push_back(presetNPC);
						}
					}
				}
			}
		}
	}

	void PatchProperties(RE::TESRace* a_race, const PatchData::PropertiesData& a_propertiesData) {
		if (!a_race->properties) {
			using alloc_type = std::remove_pointer_t<decltype(a_race->properties)>;

			RE::MemoryManager& mm = RE::MemoryManager::GetSingleton();
			void* stoage = mm.Allocate(sizeof(alloc_type), 0, false);
			if (!stoage) {
				logger::critical("Failed to allocate the Race Properties array.");
				return;
			}

			a_race->properties = new (stoage) alloc_type();
		}

		// Clear
		if (a_propertiesData.Clear) {
			a_race->properties->clear();
		}

		// Delete
		for (const auto& delProp : a_propertiesData.DeletePropertyVec) {
			for (auto it = a_race->properties->begin(); it != a_race->properties->end(); it++) {
				if (delProp.ActorValue == it->first) {
					a_race->properties->erase(it);
					break;
				}
			}
		}

		// Set
		for (const auto& setProp : a_propertiesData.SetPropertyVec) {
			bool found = false;

			for (auto& raceProp : *a_race->properties) {
				if (setProp.ActorValue == raceProp.first) {
					found = true;
					raceProp.second.f = setProp.Value;
					break;
				}
			}

			if (!found) {
				a_race->properties->emplace_back(setProp.ActorValue, setProp.Value);
			}
		}
	}

	void PatchPresets(RE::TESRace* a_race, std::uint8_t a_sex, const PatchData::PresetsData& a_presetsData) {
		if (!a_race->faceRelatedData[a_sex]) {
			using alloc_type = std::remove_pointer_t<std::remove_extent_t<decltype(a_race->faceRelatedData)>>;

			RE::MemoryManager& mm = RE::MemoryManager::GetSingleton();
			void* stoage = mm.Allocate(sizeof(alloc_type), 0, false);
			if (!stoage) {
				logger::critical("Failed to allocate the Race FaceRelatedData.");
				return;
			}

			a_race->faceRelatedData[a_sex] = ::new (stoage) alloc_type();
		}

		if (!a_race->faceRelatedData[a_sex]->presetNPCs) {
			using alloc_type = std::remove_pointer_t<decltype(a_race->faceRelatedData[a_sex]->presetNPCs)>;

			RE::MemoryManager& mm = RE::MemoryManager::GetSingleton();
			void* stoage = mm.Allocate(sizeof(alloc_type), 0, false);
			if (!stoage) {
				logger::critical("Failed to allocate the Race PresetNPCs array.");
				return;
			}

			a_race->faceRelatedData[a_sex]->presetNPCs = new (stoage) alloc_type();
		}

		// Clear
		if (a_presetsData.Clear) {
			a_race->faceRelatedData[a_sex]->presetNPCs->clear();
		}

		// Delete
		for (const auto& delPreset : a_presetsData.DeletePresetVec) {
			for (auto it = a_race->faceRelatedData[a_sex]->presetNPCs->begin(); it != a_race->faceRelatedData[a_sex]->presetNPCs->end(); it++) {
				if (delPreset == *it) {
					a_race->faceRelatedData[a_sex]->presetNPCs->erase(it);
					break;
				}
			}
		}

		for (const auto& addPreset : a_presetsData.AddPresetVec) {
			a_race->faceRelatedData[a_sex]->presetNPCs->push_back(addPreset);
		}

		for (const auto& uniqPreset : a_presetsData.AddUniquePresetSet) {
			bool found = false;

			for (const auto& preset : *a_race->faceRelatedData[a_sex]->presetNPCs) {
				if (preset == uniqPreset) {
					found = true;
					break;
				}
			}

			if (!found) {
				a_race->faceRelatedData[a_sex]->presetNPCs->push_back(uniqPreset);
			}
		}
	}

	void Patch() {
		logger::info("======================== Start preparing patch for {} ========================", TypeName);

		ConfigUtils::Prepare(g_configVec, Prepare);

		logger::info("======================== Finished preparing patch for {} ========================", TypeName);
		logger::info("");

		logger::info("======================== Start patching for {} ========================", TypeName);

		for (const auto& patchData : g_patchMap) {
			if (patchData.second.MaleSkeletalModel.has_value()) {
				patchData.first->skeletonModel[0].SetModel(patchData.second.MaleSkeletalModel.value().c_str());
			}

			if (patchData.second.FemaleSkeletalModel.has_value()) {
				patchData.first->skeletonModel[1].SetModel(patchData.second.FemaleSkeletalModel.value().c_str());
			}

			if (patchData.second.BodyPartData.has_value()) {
				patchData.first->bodyPartData = patchData.second.BodyPartData.value();
			}

			if (patchData.second.BipedObjectSlots.has_value()) {
				patchData.first->bipedModelData.bipedObjectSlots = patchData.second.BipedObjectSlots.value();
			}

			if (patchData.second.Properties.has_value()) {
				PatchProperties(patchData.first, patchData.second.Properties.value());
			}

			if (patchData.second.MalePresets.has_value()) {
				PatchPresets(patchData.first, 0, patchData.second.MalePresets.value());
			}

			if (patchData.second.FemalePresets.has_value()) {
				PatchPresets(patchData.first, 1, patchData.second.FemalePresets.value());
			}
		}

		logger::info("======================== Finished patching for {} ========================", TypeName);
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}
