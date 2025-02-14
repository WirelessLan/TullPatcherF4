#include "NPCs.h"

#include <unordered_set>
#include <regex>
#include <any>

#include "ConfigUtils.h"
#include "Parsers.h"
#include "Utils.h"

namespace NPCs {
	constexpr std::string_view TypeName = "NPC";

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
		kClass,
		kCombatStyle,
		kDefaultOutfit,
		kFullName,
		kHairColor,
		kHeadParts,
		kHeadTexture,
		kHeightMax,
		kHeightMin,
		kIsChargenFacePreset,
		kMorphs,
		kRace,
		kSex,
		kSkin,
		kTints,
		kWeightFat,
		kWeightMuscular,
		kWeightThin
	};

	std::string_view ElementTypeToString(ElementType a_value) {
		switch (a_value) {
		case ElementType::kClass: return "Class";
		case ElementType::kCombatStyle: return "CombatStyle";
		case ElementType::kDefaultOutfit: return "DefaultOutfit";
		case ElementType::kFullName: return "FullName";
		case ElementType::kHairColor: return "HairColor";
		case ElementType::kHeadParts: return "HeadParts";
		case ElementType::kHeadTexture: return "HeadTexture";
		case ElementType::kHeightMax: return "HeightMax";
		case ElementType::kHeightMin: return "HeightMin";
		case ElementType::kIsChargenFacePreset: return "IsChargenFacePreset";
		case ElementType::kMorphs: return "Morphs";
		case ElementType::kRace: return "Race";
		case ElementType::kSex: return "Sex";
		case ElementType::kSkin: return "Skin";
		case ElementType::kTints: return "Tints";
		case ElementType::kWeightFat: return "WeightFat";
		case ElementType::kWeightMuscular: return "WeightMuscular";
		case ElementType::kWeightThin: return "WeightThin";
		default: return std::string_view{};
		}
	}

	enum class OperationType {
		kClear,
		kAdd,
		kSet,
		kDelete
	};

	std::string_view OperationTypeToString(OperationType a_value) {
		switch (a_value) {
		case OperationType::kClear: return "Clear";
		case OperationType::kAdd: return "Add";
		case OperationType::kSet: return "Set";
		case OperationType::kDelete: return "Delete";
		default: return std::string_view{};
		}
	}

	struct ConfigData {
		struct Operation {
			struct MorphData {
				std::uint32_t Key;
				float Value;
			};

			struct TintData {
				std::uint16_t Index;
				std::uint32_t Color;
				float Alpha;
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
		struct HeadPartsData {
			bool Clear;
			std::vector<RE::BGSHeadPart*> AddPartVec;
			std::vector<RE::BGSHeadPart*> DeletePartVec;
		};

		struct MorphsData {
			bool Clear;
			std::unordered_map<std::uint32_t, float> SetMorphMap;
			std::vector<std::uint32_t> DeleteMorphVec;
		};

		struct TintsData {
			bool Clear;
			std::unordered_map<std::uint16_t, std::pair<std::uint32_t, float>> SetTintMap;
			std::vector<std::uint16_t> DeleteTintVec;
		};
		
		std::optional<RE::TESClass*> Class;
		std::optional<RE::TESCombatStyle*> CombatStyle;
		std::optional<RE::BGSOutfit*> DefaultOutfit;
		std::optional<std::string> FullName;
		std::optional<RE::BGSColorForm*> HairColor;
		std::optional<HeadPartsData> HeadParts;
		std::optional<RE::BGSTextureSet*> HeadTexture;
		std::optional<float> HeightMin;
		std::optional<float> HeightMax;
		std::optional<bool> IsChargenFacePreset;
		std::optional<MorphsData> Morphs;
		std::optional<RE::TESRace*> Race;
		std::optional<std::uint8_t> Sex;
		std::optional<RE::TESObjectARMO*> Skin;
		std::optional<TintsData> Tints;
		std::optional<float> WeightFat;
		std::optional<float> WeightMuscular;
		std::optional<float> WeightThin;
	};

	bool g_prepared = false;
	std::vector<Parsers::Statement<ConfigData>> g_configVec;
	std::unordered_map<RE::TESNPC*, PatchData> g_patchMap;

	class NPCParser : public Parsers::Parser<ConfigData> {
	public:
		NPCParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

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
			case ElementType::kHeadParts:
				logger::info("{}{}({}).{}", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element));
				for (std::size_t ii = 0; ii < a_configData.Operations.size(); ii++) {
					std::string opLog;

					switch (a_configData.Operations[ii].OpType) {
					case OperationType::kClear:
						opLog = fmt::format(".{}()", OperationTypeToString(a_configData.Operations[ii].OpType));
						break;

					case OperationType::kAdd:
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

			case ElementType::kMorphs:
				logger::info("{}{}({}).{}", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element));
				for (std::size_t ii = 0; ii < a_configData.Operations.size(); ii++) {
					std::string opLog;

					switch (a_configData.Operations[ii].OpType) {
					case OperationType::kClear:
						opLog = fmt::format(".{}()", OperationTypeToString(a_configData.Operations[ii].OpType));
						break;

					case OperationType::kSet:
						opLog = fmt::format(".{}({:08X}, {})", OperationTypeToString(a_configData.Operations[ii].OpType),
							std::any_cast<ConfigData::Operation::MorphData>(a_configData.Operations[ii].OpData.value()).Key,
							std::any_cast<ConfigData::Operation::MorphData>(a_configData.Operations[ii].OpData.value()).Value);
						break;

					case OperationType::kDelete:
						opLog = fmt::format(".{}({:08X})", OperationTypeToString(a_configData.Operations[ii].OpType),
							std::any_cast<ConfigData::Operation::MorphData>(a_configData.Operations[ii].OpData.value()).Key);
						break;
					}

					if (ii == a_configData.Operations.size() - 1) {
						opLog += ";";
					}

					logger::info("{}    {}", indent, opLog);
				}
				break;

			case ElementType::kTints:
				logger::info("{}{}({}).{}", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element));
				for (std::size_t ii = 0; ii < a_configData.Operations.size(); ii++) {
					std::string opLog;

					switch (a_configData.Operations[ii].OpType) {
					case OperationType::kClear:
						opLog = fmt::format(".{}()", OperationTypeToString(a_configData.Operations[ii].OpType));
						break;

					case OperationType::kSet:
						opLog = fmt::format(".{}({}, {}, {}, {}, {})", OperationTypeToString(a_configData.Operations[ii].OpType),
							std::any_cast<ConfigData::Operation::TintData>(a_configData.Operations[ii].OpData.value()).Index,
							std::any_cast<ConfigData::Operation::TintData>(a_configData.Operations[ii].OpData.value()).Color & 0xFF,
							(std::any_cast<ConfigData::Operation::TintData>(a_configData.Operations[ii].OpData.value()).Color >> 8) & 0xFF,
							(std::any_cast<ConfigData::Operation::TintData>(a_configData.Operations[ii].OpData.value()).Color >> 16) & 0xFF,
							std::any_cast<ConfigData::Operation::TintData>(a_configData.Operations[ii].OpData.value()).Alpha);
						break;

					case OperationType::kDelete:
						opLog = fmt::format(".{}({})", OperationTypeToString(a_configData.Operations[ii].OpType),
							std::any_cast<ConfigData::Operation::TintData>(a_configData.Operations[ii].OpData.value()).Index);
						break;
					}

					if (ii == a_configData.Operations.size() - 1) {
						opLog += ";";
					}

					logger::info("{}    {}", indent, opLog);
				}
				break;

			case ElementType::kHeightMax:
			case ElementType::kHeightMin:
			case ElementType::kWeightFat:
			case ElementType::kWeightMuscular:
			case ElementType::kWeightThin:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm,
					ElementTypeToString(a_configData.Element), std::any_cast<float>(a_configData.AssignValue.value()));
				break;

			case ElementType::kClass:
			case ElementType::kCombatStyle:
			case ElementType::kDefaultOutfit:
			case ElementType::kFullName:
			case ElementType::kHairColor:
			case ElementType::kHeadTexture:
			case ElementType::kRace:
			case ElementType::kSkin:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm,
					ElementTypeToString(a_configData.Element), std::any_cast<std::string>(a_configData.AssignValue.value()));
				break;

			case ElementType::kIsChargenFacePreset:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm,
					ElementTypeToString(a_configData.Element), std::any_cast<bool>(a_configData.AssignValue.value()));
				break;

			case ElementType::kSex:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm,
					ElementTypeToString(a_configData.Element), std::any_cast<std::uint8_t>(a_configData.AssignValue.value()));
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
			if (token == "Class") {
				a_config.Element = ElementType::kClass;
			}
			else if (token == "CombatStyle") {
				a_config.Element = ElementType::kCombatStyle;
			}
			else if (token == "DefaultOutfit") {
				a_config.Element = ElementType::kDefaultOutfit;
			}
			else if (token == "FullName") {
				a_config.Element = ElementType::kFullName;
			}
			else if (token == "HairColor") {
				a_config.Element = ElementType::kHairColor;
			}
			else if (token == "HeadParts") {
				a_config.Element = ElementType::kHeadParts;
			}
			else if (token == "HeadTexture") {
				a_config.Element = ElementType::kHeadTexture;
			}
			else if (token == "HeightMax") {
				a_config.Element = ElementType::kHeightMax;
			}
			else if (token == "HeightMin") {
				a_config.Element = ElementType::kHeightMin;
			} 
			else if (token == "IsChargenFacePreset") {
				a_config.Element = ElementType::kIsChargenFacePreset;
			}
			else if (token == "Morphs") {
				a_config.Element = ElementType::kMorphs;
			}
			else if (token == "Race") {
				a_config.Element = ElementType::kRace;
			}
			else if (token == "Sex") {
				a_config.Element = ElementType::kSex;
			}
			else if (token == "Skin") {
				a_config.Element = ElementType::kSkin;
			}
			else if (token == "Tints") {
				a_config.Element = ElementType::kTints;
			}
			else if (token == "WeightFat") {
				a_config.Element = ElementType::kWeightFat;
			}
			else if (token == "WeightMuscular") {
				a_config.Element = ElementType::kWeightMuscular;
			}
			else if (token == "WeightThin") {
				a_config.Element = ElementType::kWeightThin;
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

			if (a_config.Element == ElementType::kClass || a_config.Element == ElementType::kCombatStyle || a_config.Element == ElementType::kDefaultOutfit ||
				a_config.Element == ElementType::kHairColor || a_config.Element == ElementType::kHeadTexture || a_config.Element == ElementType::kRace|| a_config.Element == ElementType::kSkin) {
				token = reader.Peek();
				if (token == "null") {
					a_config.AssignValue = std::any(std::string(reader.GetToken()));
				}
				else {
					auto form = ParseForm();
					if (!form.has_value()) {
						return false;
					}

					a_config.AssignValue = std::any(form.value());
				}
			}
			else if (a_config.Element == ElementType::kFullName) {
				token = reader.GetToken();
				if (!token.starts_with('\"')) {
					logger::warn("Line {}, Col {}: {} must be a string.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_config.Element));
					return false;
				}
				else if (!token.ends_with('\"')) {
					logger::warn("Line {}, Col {}: String must end with '\"'.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}

				a_config.AssignValue = std::any(std::string(token.substr(1, token.length() - 2)));
			}
			else if (a_config.Element == ElementType::kHeightMax || a_config.Element == ElementType::kHeightMin ||
				     a_config.Element == ElementType::kWeightFat || a_config.Element == ElementType::kWeightMuscular || a_config.Element == ElementType::kWeightThin) {
				std::optional<float> opValue = ParseNumber();
				if (!opValue.has_value()) {
					return false;
				}

				a_config.AssignValue = std::any(opValue.value());
			}
			else if (a_config.Element == ElementType::kIsChargenFacePreset) {
				token = reader.GetToken();
				if (token == "true") {
					a_config.AssignValue = std::any(true);
				}
				else if (token == "false") {
					a_config.AssignValue = std::any(false);
				}
				else {
					logger::warn("Line {}, Col {}: Invalid value '{}'. For IsChargenFacePreset, the value must be 'true' or 'false'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}
			}
			else if (a_config.Element == ElementType::kSex) {
				token = reader.GetToken();
				if (token.empty()) {
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
					logger::warn("Line {}, Col {}: Invalid value '{}'. For Sex, the value must be 0 or 1.", reader.GetLastLine(), reader.GetLastLineIndex(), parsedValue);
					return false;
				}

				a_config.AssignValue = std::any(static_cast<std::uint8_t>(parsedValue));
			}

			return true;
		}

		bool ParseOperation(ConfigData& a_config) {
			ConfigData::Operation newOp;

			auto token = reader.GetToken();
			if (token == "Clear") {
				newOp.OpType = OperationType::kClear;
			}
			else if (token == "Add") {
				newOp.OpType = OperationType::kAdd;
			}
			else if (token == "Set") {
				newOp.OpType = OperationType::kSet;
			}
			else if (token == "Delete") {
				newOp.OpType = OperationType::kDelete;
			}
			else {
				logger::warn("Line {}, Col {}: Invalid OperationName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			switch (a_config.Element) {
			case ElementType::kHeadParts:
				if (newOp.OpType != OperationType::kClear && newOp.OpType != OperationType::kAdd && newOp.OpType != OperationType::kDelete) {
					logger::warn("Line {}, Col {}: Invalid Operation '{}.{}()'.",
						reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_config.Element), OperationTypeToString(newOp.OpType));
					return false;
				}
				break;

			case ElementType::kMorphs:
			case ElementType::kTints:
				if (newOp.OpType != OperationType::kClear && newOp.OpType != OperationType::kSet && newOp.OpType != OperationType::kDelete) {
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
			case ElementType::kHeadParts:
				if (newOp.OpType != OperationType::kClear) {
					std::optional<std::string> opForm = ParseForm();
					if (!opForm.has_value()) {
						return false;
					}

					newOp.OpData = std::any(opForm.value());
				}

				break;

			case ElementType::kMorphs:
				if (newOp.OpType != OperationType::kClear) {
					ConfigData::Operation::MorphData morphData{};

					token = reader.GetToken();
					if (!IsHexString(token)) {
						logger::warn("Line {}, Col {}: Expected MorphKey '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
						return false;
					}

					morphData.Key = Utils::ParseHex(token);

					if (newOp.OpType == OperationType::kSet) {
						token = reader.GetToken();
						if (token != ",") {
							logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
							return false;
						}

						std::optional<float> morphValue = ParseNumber();
						if (!morphValue.has_value()) {
							return false;
						}

						if (morphValue.value() < 0.0f || morphValue.value() > 1.0f) {
							logger::warn("Line {}, Col {}: Invalid MorphValue '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), morphValue.value());
							return false;
						}

						morphData.Value = morphValue.value();
					}

					newOp.OpData = std::any(morphData);
				}

				break;

			case ElementType::kTints:
				if (newOp.OpType != OperationType::kClear) {
					ConfigData::Operation::TintData tintData{};

					token = reader.GetToken();
					if (token.empty() || token == ",") {
						logger::warn("Line {}, Col {}: Expected TintIndex '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
						return false;
					}

					unsigned long parsedValue;
					auto parsingResult = std::from_chars(token.data(), token.data() + token.size(), parsedValue);
					if (parsingResult.ec != std::errc()) {
						logger::warn("Line {}, Col {}: Failed to parse TintIndex '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
						return false;
					}

					if (parsedValue > 0xFFFF) {
						logger::warn("Line {}, Col {}: Invalid TintIndex '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
						return false;
					}

					tintData.Index = static_cast<std::uint16_t>(parsedValue);

					if (newOp.OpType == OperationType::kSet) {
						token = reader.GetToken();
						if (token != ",") {
							logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
							return false;
						}

						token = reader.GetToken();
						if (token.empty()) {
							logger::warn("Line {}, Col {}: Expected TintColorRed '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
							return false;
						}

						parsingResult = std::from_chars(token.data(), token.data() + token.size(), parsedValue);
						if (parsingResult.ec != std::errc()) {
							logger::warn("Line {}, Col {}: Failed to parse TintColorRed '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
							return false;
						}

						if (parsedValue > 0xFF) {
							logger::warn("Line {}, Col {}: Invalid TintColorRed '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
							return false;
						}

						std::uint32_t tintColor = parsedValue;

						token = reader.GetToken();
						if (token != ",") {
							logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
							return false;
						}

						token = reader.GetToken();
						if (token.empty()) {
							logger::warn("Line {}, Col {}: Expected TintColorGreen '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
							return false;
						}

						parsingResult = std::from_chars(token.data(), token.data() + token.size(), parsedValue);
						if (parsingResult.ec != std::errc()) {
							logger::warn("Line {}, Col {}: Failed to parse TintColorGreen '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
							return false;
						}

						if (parsedValue > 0xFF) {
							logger::warn("Line {}, Col {}: Invalid TintColorGreen '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
							return false;
						}

						tintColor |= parsedValue << 8;

						token = reader.GetToken();
						if (token != ",") {
							logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
							return false;
						}

						token = reader.GetToken();
						if (token.empty()) {
							logger::warn("Line {}, Col {}: Expected TintColorBlue '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
							return false;
						}

						parsingResult = std::from_chars(token.data(), token.data() + token.size(), parsedValue);
						if (parsingResult.ec != std::errc()) {
							logger::warn("Line {}, Col {}: Failed to parse TintColorBlue '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
							return false;
						}

						if (parsedValue > 0xFF) {
							logger::warn("Line {}, Col {}: Invalid TintColorBlue '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
							return false;
						}

						tintColor |= parsedValue << 16;

						tintData.Color = tintColor;

						token = reader.GetToken();
						if (token != ",") {
							logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
							return false;
						}

						auto tintAlpha = ParseNumber();
						if (!tintAlpha.has_value()) {
							return false;
						}

						if (tintAlpha.value() < 0.0f || tintAlpha.value() > 1.0f) {
							logger::warn("Line {}, Col {}: Invalid TintAlpha '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
							return false;
						}

						tintData.Alpha = tintAlpha.value();
					}

					newOp.OpData = std::any(tintData);
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
		g_configVec = ConfigUtils::ReadConfigs<NPCParser, Parsers::Statement<ConfigData>>(TypeName);
	}

	void Prepare(const ConfigData& a_configData) {
		if (a_configData.Filter == FilterType::kFormID) {
			RE::TESForm* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
			if (!filterForm) {
				logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
				return;
			}

			RE::TESNPC* npc = filterForm->As<RE::TESNPC>();
			if (!npc) {
				logger::warn("'{}' is not a NPC.", a_configData.FilterForm);
				return;
			}

			if (a_configData.Element == ElementType::kClass) {
				std::string classFormStr = std::any_cast<std::string>(a_configData.AssignValue.value());
				RE::TESClass* _class = nullptr;

				if (classFormStr != "null") {
					RE::TESForm* classForm = Utils::GetFormFromString(classFormStr);
					if (!classForm) {
						logger::warn("Invalid Form: '{}'.", classFormStr);
						return;
					}

					_class = classForm->As<RE::TESClass>();
					if (!_class) {
						logger::warn("'{}' is not a Class.", classFormStr);
						return;
					}
				}

				g_patchMap[npc].Class = _class;
			}
			else if (a_configData.Element == ElementType::kCombatStyle) {
				std::string comStyleFormStr = std::any_cast<std::string>(a_configData.AssignValue.value());
				RE::TESCombatStyle* comStyle = nullptr;

				if (comStyleFormStr != "null") {
					RE::TESForm* comStyleForm = Utils::GetFormFromString(comStyleFormStr);
					if (!comStyleForm) {
						logger::warn("Invalid Form: '{}'.", comStyleFormStr);
						return;
					}
					comStyle = comStyleForm->As<RE::TESCombatStyle>();
					if (!comStyle) {
						logger::warn("'{}' is not a CombatStyle.", comStyleFormStr);
						return;
					}
				}

				g_patchMap[npc].CombatStyle = comStyle;
			}
			else if (a_configData.Element == ElementType::kDefaultOutfit) {
				std::string outfitFormStr = std::any_cast<std::string>(a_configData.AssignValue.value());
				RE::BGSOutfit* outfit = nullptr;

				if (outfitFormStr != "null") {
					RE::TESForm* outfitForm = Utils::GetFormFromString(outfitFormStr);
					if (!outfitForm) {
						logger::warn("Invalid Form: '{}'.", outfitFormStr);
						return;
					}

					outfit = outfitForm->As<RE::BGSOutfit>();
					if (!outfit) {
						logger::warn("'{}' is not a Outfit.", outfitFormStr);
						return;
					}
				}

				g_patchMap[npc].DefaultOutfit = outfit;
			}
			else if (a_configData.Element == ElementType::kFullName) {
				g_patchMap[npc].FullName = std::any_cast<std::string>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kHairColor) {
				std::string colorFormStr = std::any_cast<std::string>(a_configData.AssignValue.value());
				RE::BGSColorForm* color = nullptr;

				if (colorFormStr != "null") {
					RE::TESForm* colorForm = Utils::GetFormFromString(colorFormStr);
					if (!colorForm) {
						logger::warn("Invalid Form: '{}'.", colorFormStr);
						return;
					}

					color = colorForm->As<RE::BGSColorForm>();
					if (!color) {
						logger::warn("'{}' is not a Color.", colorFormStr);
						return;
					}
				}

				g_patchMap[npc].HairColor = color;
			}
			else if (a_configData.Element == ElementType::kHeadParts) {
				PatchData& patchData = g_patchMap[npc];

				if (!patchData.HeadParts.has_value()) {
					patchData.HeadParts = PatchData::HeadPartsData{};
				}

				for (const auto& op : a_configData.Operations) {
					if (op.OpType == OperationType::kClear) {
						patchData.HeadParts->Clear = true;
					}
					else if (op.OpType == OperationType::kAdd || op.OpType == OperationType::kDelete) {
						std::string opFormStr = std::any_cast<std::string>(op.OpData.value());

						RE::TESForm* opForm = Utils::GetFormFromString(opFormStr);
						if (!opForm) {
							logger::warn("Invalid Form: '{}'.", opFormStr);
							continue;
						}

						RE::BGSHeadPart* headPart = opForm->As<RE::BGSHeadPart>();
						if (!headPart) {
							logger::warn("'{}' is not a HeadPart.", opFormStr);
							continue;
						}

						if (op.OpType == OperationType::kAdd) {
							patchData.HeadParts->AddPartVec.push_back(headPart);
						}
						else {
							patchData.HeadParts->DeletePartVec.push_back(headPart);
						}
					}
				}
			}
			else if (a_configData.Element == ElementType::kHeadTexture) {
				std::string texFormStr = std::any_cast<std::string>(a_configData.AssignValue.value());
				RE::BGSTextureSet* textureSet = nullptr;

				if (texFormStr != "null") {
					RE::TESForm* texForm = Utils::GetFormFromString(texFormStr);
					if (!texForm) {
						logger::warn("Invalid Form: '{}'.", texFormStr);
						return;
					}

					textureSet = texForm->As<RE::BGSTextureSet>();
					if (!textureSet) {
						logger::warn("'{}' is not a TextureSet.", texFormStr);
						return;
					}
				}

				g_patchMap[npc].HeadTexture = textureSet;
			}
			else if (a_configData.Element == ElementType::kHeightMax) {
				g_patchMap[npc].HeightMax = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kHeightMin) {
				g_patchMap[npc].HeightMin = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kIsChargenFacePreset) {
				g_patchMap[npc].IsChargenFacePreset = std::any_cast<bool>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kMorphs) {
				PatchData& patchData = g_patchMap[npc];

				if (!patchData.Morphs.has_value()) {
					patchData.Morphs = PatchData::MorphsData{};
				}

				for (const auto& op : a_configData.Operations) {
					if (op.OpType == OperationType::kClear) {
						patchData.Morphs->Clear = true;
					}
					else if (op.OpType == OperationType::kSet || op.OpType == OperationType::kDelete) {
						auto morphData = std::any_cast<ConfigData::Operation::MorphData>(op.OpData.value());

						if (op.OpType == OperationType::kSet) {
							patchData.Morphs->SetMorphMap.insert(std::make_pair(morphData.Key, morphData.Value));
						}
						else {
							patchData.Morphs->DeleteMorphVec.push_back(morphData.Key);
						}
					}
				}
			}
			else if (a_configData.Element == ElementType::kRace) {
				std::string raceFormStr = std::any_cast<std::string>(a_configData.AssignValue.value());
				RE::TESRace* race = nullptr;

				if (raceFormStr != "null") {
					RE::TESForm* raceForm = Utils::GetFormFromString(raceFormStr);
					if (!raceForm) {
						logger::warn("Invalid Form: '{}'.", raceFormStr);
						return;
					}

					race = raceForm->As<RE::TESRace>();
					if (!race) {
						logger::warn("'{}' is not a Race.", raceFormStr);
						return;
					}
				}

				g_patchMap[npc].Race = race;
			}
			else if (a_configData.Element == ElementType::kSex) {
				g_patchMap[npc].Sex = std::any_cast<std::uint8_t>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kSkin) {
				std::string armoFormStr = std::any_cast<std::string>(a_configData.AssignValue.value());
				RE::TESObjectARMO* armo = nullptr;

				if (armoFormStr != "null") {
					RE::TESForm* armoForm = Utils::GetFormFromString(armoFormStr);
					if (!armoForm) {
						logger::warn("Invalid Form: '{}'.", armoFormStr);
						return;
					}

					armo = armoForm->As<RE::TESObjectARMO>();
					if (!armo) {
						logger::warn("'{}' is not a Armor.", armoFormStr);
						return;
					}
				}

				g_patchMap[npc].Skin = armo;
			}
			else if (a_configData.Element == ElementType::kTints) {
				PatchData& patchData = g_patchMap[npc];

				if (!patchData.Tints.has_value()) {
					patchData.Tints = PatchData::TintsData{};
				}

				for (const auto& op : a_configData.Operations) {
					if (op.OpType == OperationType::kClear) {
						patchData.Tints->Clear = true;
					}
					else if (op.OpType == OperationType::kSet || op.OpType == OperationType::kDelete) {
						auto tintData = std::any_cast<ConfigData::Operation::TintData>(op.OpData.value());

						if (op.OpType == OperationType::kSet) {
							patchData.Tints->SetTintMap.insert(std::make_pair(tintData.Index, std::make_pair(tintData.Color, tintData.Alpha)));
						}
						else {
							patchData.Tints->DeleteTintVec.push_back(tintData.Index);
						}
					}
				}
			}
			else if (a_configData.Element == ElementType::kWeightFat) {
				g_patchMap[npc].WeightFat = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kWeightMuscular) {
				g_patchMap[npc].WeightMuscular = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kWeightThin) {
				g_patchMap[npc].WeightThin = std::any_cast<float>(a_configData.AssignValue.value());
			}
		}
	}

	void PrepareOnce() {
		if (g_prepared) {
			return;
		}

		logger::info("======================== Start preparing patch for {} ========================", TypeName);

		ConfigUtils::Prepare(g_configVec, Prepare);
		g_prepared = true;

		logger::info("======================== Finished preparing patch for {} ========================", TypeName);
		logger::info("");
	}

	void AddHeadPart(RE::TESNPC* a_npc, RE::BGSHeadPart* a_hdpt) {
		using func_t = void(*)(RE::TESNPC*, RE::BGSHeadPart*, std::uint32_t, std::uint32_t);
		const REL::Relocation<func_t> func{ REL::ID(735660) };
		func(a_npc, a_hdpt, 1, false);
	}

	void RemoveHeadPart(RE::TESNPC* a_npc, RE::BGSHeadPart* a_hdpt) {
		using func_t = void(*)(RE::TESNPC*, RE::BGSHeadPart*, bool);
		const REL::Relocation<func_t> func{ REL::ID(880456) };
		func(a_npc, a_hdpt, true);
	}

	void ClearHeadParts(RE::TESNPC* a_npc) {
		if (a_npc->headParts) {
			RE::MemoryManager& mm = RE::MemoryManager::GetSingleton();
			mm.Deallocate(a_npc->headParts, false);
		}
		a_npc->headParts = nullptr;
		a_npc->numHeadParts = 0;
	}

	void PatchHeadParts(RE::TESNPC* a_npc, const PatchData::HeadPartsData& a_headPartsData) {
		bool isCleared = false;

		// Clear
		if (a_headPartsData.Clear) {
			ClearHeadParts(a_npc);
			isCleared = true;
		}

		// Delete
		if (!isCleared) {
			for (const auto& delPart : a_headPartsData.DeletePartVec) {
				RemoveHeadPart(a_npc, delPart);
			}
		}

		// Add
		for (const auto& addPart : a_headPartsData.AddPartVec) {
			AddHeadPart(a_npc, addPart);
		}
	}

	void SetMorphSliderValue(RE::TESNPC* a_npc, std::uint32_t a_morphKey, float a_morphValue) {
		using func_t = decltype(&SetMorphSliderValue);
		const REL::Relocation<func_t> func{ REL::ID(1432151) };
		func(a_npc, a_morphKey, a_morphValue);
	}

	void PatchMorphs(RE::TESNPC* a_npc, const PatchData::MorphsData& a_morphsData) {
		if (a_morphsData.Clear) {
			if (a_npc->morphSliderValues) {
				a_npc->morphSliderValues->clear();
			}
		}

		// Delete
		if (a_npc->morphSliderValues) {
			for (auto deleteKey : a_morphsData.DeleteMorphVec) {
				SetMorphSliderValue(a_npc, deleteKey, 0);
			}
		}

		// Set
		for (auto setPair : a_morphsData.SetMorphMap) {
			SetMorphSliderValue(a_npc, setPair.first, setPair.second);
		}
	}

	void SetTintingData(RE::TESNPC* a_npc, std::uint16_t a_index, float a_value, std::uint32_t a_color) {
		using func_t = decltype(&SetTintingData);
		const REL::Relocation<func_t> func{ REL::ID(452734) };
		func(a_npc, a_index, a_value, a_color);
	}

	void PatchTints(RE::TESNPC* a_npc, const PatchData::TintsData& a_tintsData) {
		auto tintingData = reinterpret_cast<RE::BSTArray<RE::BGSCharacterTint::Entries*>*>(a_npc->tintingData);

		if (a_tintsData.Clear) {
			if (tintingData) {
				tintingData->clear();
			}
		}

		// Delete
		if (tintingData) {
			for (auto deleteIndex : a_tintsData.DeleteTintVec) {
				SetTintingData(a_npc, deleteIndex, 0, 0xFFFFFFFF);
			}
		}

		// Set
		for (auto setPair : a_tintsData.SetTintMap) {
			SetTintingData(a_npc, setPair.first, setPair.second.second, setPair.second.first);
		}
	}

	template<std::uint64_t id, std::ptrdiff_t diff>
	class TESNPC_ClearStaticDataHook {
	public:
		static void Install() {
			REL::Relocation<std::uintptr_t> target(REL::ID(id), diff);
			func = *(func_t*)(target.get());
			REL::safe_write(target.get(), (std::uintptr_t)ProcessHook);
		}
	private:
		using func_t = void(*)(RE::TESNPC&);

		static void Patch_PreFunc(RE::TESNPC* a_npc, const PatchData& a_patchData) {
			if (a_patchData.FullName.has_value()) {
				a_npc->fullName = a_patchData.FullName.value();
			}

			if (a_patchData.HeightMax.has_value()) {
				a_npc->heightMax = a_patchData.HeightMax.value();
			}

			if (a_patchData.HeightMin.has_value()) {
				a_npc->height = a_patchData.HeightMin.value();
			}

			if (a_patchData.IsChargenFacePreset.has_value()) {
				bool value = a_patchData.IsChargenFacePreset.value();

				if (value) {
					a_npc->actorData.actorBaseFlags |= RE::ACTOR_BASE_DATA::Flag::kIsChargenFacePreset;
				}
				else {
					if (a_npc->actorData.actorBaseFlags & RE::ACTOR_BASE_DATA::Flag::kIsChargenFacePreset) {
						a_npc->actorData.actorBaseFlags -= RE::ACTOR_BASE_DATA::Flag::kIsChargenFacePreset;
					}
				}
			}

			if (a_patchData.Race.has_value()) {
				a_npc->formRace = a_patchData.Race.value();
			}

			if (a_patchData.Sex.has_value()) {
				bool value = a_patchData.Sex.value();

				if (value) {
					// Female
					a_npc->actorData.actorBaseFlags |= RE::ACTOR_BASE_DATA::Flag::kFemale;
				} else {
					// Male
					if (a_npc->actorData.actorBaseFlags & RE::ACTOR_BASE_DATA::Flag::kFemale) {
						a_npc->actorData.actorBaseFlags -= RE::ACTOR_BASE_DATA::Flag::kFemale;
					}
				}
			}

			if (a_patchData.Skin.has_value()) {
				a_npc->formSkin = a_patchData.Skin.value();
			}

			if (a_patchData.WeightFat.has_value()) {
				a_npc->morphWeight.z = a_patchData.WeightFat.value();
			}

			if (a_patchData.WeightMuscular.has_value()) {
				a_npc->morphWeight.y = a_patchData.WeightMuscular.value();
			}

			if (a_patchData.WeightThin.has_value()) {
				a_npc->morphWeight.x = a_patchData.WeightThin.value();
			}
		}

		static void Patch_PostFunc(RE::TESNPC* a_npc, const PatchData& a_patchData) {
			if (a_patchData.Class.has_value()) {
				a_npc->cl = a_patchData.Class.value();
			}

			if (a_patchData.CombatStyle.has_value()) {
				a_npc->combatStyle = a_patchData.CombatStyle.value();
			}

			if (a_patchData.DefaultOutfit.has_value()) {
				a_npc->defOutfit = a_patchData.DefaultOutfit.value();
			}

			if (a_patchData.HairColor.has_value()) {
				if (a_npc->headRelatedData) {
					a_npc->headRelatedData->hairColor = a_patchData.HairColor.value();
				}
			}

			if (a_patchData.HeadTexture.has_value()) {
				if (a_npc->headRelatedData) {
					a_npc->headRelatedData->faceDetails = a_patchData.HeadTexture.value();
				}
			}

			if (a_patchData.HeadParts.has_value()) {
				PatchHeadParts(a_npc, a_patchData.HeadParts.value());
			}

			if (a_patchData.Morphs.has_value()) {
				PatchMorphs(a_npc, a_patchData.Morphs.value());
			}

			if (a_patchData.Tints.has_value()) {
				PatchTints(a_npc, a_patchData.Tints.value());
			}
		}

		static void ProcessHook(RE::TESNPC& a_npc) {
			PrepareOnce();

			auto it = g_patchMap.find(&a_npc);
			if (it == g_patchMap.end()) {
				func(a_npc);
				return;
			}

			Patch_PreFunc(it->first, it->second);
			func(a_npc);
			Patch_PostFunc(it->first, it->second);
		}

		inline static func_t func;
	};

	void Install() {
		TESNPC_ClearStaticDataHook<1261646, 0xB0>::Install();
	}
}
