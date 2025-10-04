#include "ObjectModifications.h"

#include <any>
#include <regex>
#include <unordered_set>

#include "ConfigUtils.h"
#include "Parsers.h"
#include "Utils.h"

namespace ObjectModifications {
	constexpr std::string_view TypeName = "ObjectModification";

	enum class FilterType {
		kFormID
	};

	std::string_view FilterTypeToString(FilterType a_value) {
		switch (a_value) {
		case FilterType::kFormID:
			return "FilterByFormID";
		default:
			return std::string_view{};
		}
	}

	enum class ElementType {
		kProperties
	};

	std::string_view ElementTypeToString(ElementType a_value) {
		switch (a_value) {
		case ElementType::kProperties:
			return "Properties";
		default:
			return std::string_view{};
		}
	}

	enum class OperationType {
		kClear,
		kAdd,
	};

	std::string_view OperationTypeToString(OperationType a_value) {
		switch (a_value) {
		case OperationType::kClear:
			return "Clear";
		case OperationType::kAdd:
			return "Add";
		default:
			return std::string_view{};
		}
	}

	struct ConfigData {
		struct Operation {
			struct Data {
				std::string ValueType;
				std::string FunctionType;
				std::string Property;
				std::any Value1;
				std::any Value2;
			};

			OperationType OpType;
			std::optional<Data> OpData;
		};

		FilterType Filter;
		std::string FilterForm;
		ElementType Element;
		std::vector<Operation> Operations;
	};

	using PropertyContainer = std::array<std::byte, sizeof(RE::BGSMod::Property::Mod)>;

	struct PatchData {
		struct PropertiesData {
			bool Clear = false;
			std::vector<PropertyContainer> AddProperties;
		};

		std::optional<PropertiesData> Properties;
	};

	std::vector<Parsers::Statement<ConfigData>> g_configVec;
	std::unordered_map<RE::BGSMod::Attachment::Mod*, PatchData> g_patchMap;

	const std::unordered_set<std::string_view> g_propertySet = {
		"Enchantments",
		"BashImpactDataSet",
		"BlockMaterial",
		"Keywords",
		"Weight",
		"Value",
		"Rating",
		"AddonIndex",
		"BodyPart",
		"DamageTypeValue",
		"ActorValues",
		"Health",
		"ColorRemappingIndex",
		"MaterialSwaps",
		"ForcedInventory",
		"XPOffset",
		"Speed",
		"Reach",
		"MinRange",
		"MaxRange",
		"AttackDelaySec",
		"Unknown 5",
		"OutOfRangeDamageMult",
		"SecondaryDamage",
		"CriticalChargeBonus",
		"HitBehaviour",
		"Rank",
		"Unknown 11",
		"AmmoCapacity",
		"Unknown 13",
		"Unknown 14",
		"Type",
		"IsPlayerOnly",
		"NPCsUseAmmo",
		"HasChargingReload",
		"IsMinorCrime",
		"IsFixedRange",
		"HasEffectOnDeath",
		"HasAlternateRumble",
		"IsNonHostile",
		"IgnoreResist",
		"IsAutomatic",
		"CantDrop",
		"IsNonPlayable",
		"AttackDamage",
		"AimModel",
		"AimModelMinConeDegrees",
		"AimModelMaxConeDegrees",
		"AimModelConeIncreasePerShot",
		"AimModelConeDecreasePerSec",
		"AimModelConeDecreaseDelayMs",
		"AimModelConeSneakMultiplier",
		"AimModelRecoilDiminishSpringForce",
		"AimModelRecoilDiminishSightsMult",
		"AimModelRecoilMaxDegPerShot",
		"AimModelRecoilMinDegPerShot",
		"AimModelRecoilHipMult",
		"AimModelRecoilShotsForRunaway",
		"AimModelRecoilArcDeg",
		"AimModelRecoilArcRotateDeg",
		"AimModelConeIronSightsMultiplier",
		"HasScope",
		"ZoomDataFOVMult",
		"FireSeconds",
		"NumProjectiles",
		"AttackSound",
		"AttackSound2D",
		"AttackLoop",
		"AttackFailSound",
		"IdleSound",
		"EquipSound",
		"UnEquipSound",
		"SoundLevel",
		"ImpactDataSet",
		"Ammo",
		"CritEffect",
		"AimModelBaseStability",
		"ZoomData",
		"ZoomDataOverlay",
		"ZoomDataImageSpace",
		"ZoomDataCameraOffsetX",
		"ZoomDataCameraOffsetY",
		"ZoomDataCameraOffsetZ",
		"EquipSlot",
		"SoundLevelMult",
		"NPCAmmoList",
		"ReloadSpeed",
		"DamageTypeValues",
		"AccuracyBonus",
		"AttackActionPointCost",
		"OverrideProjectile",
		"HasBoltAction",
		"StaggerValue",
		"SightedTransitionSeconds",
		"FullPowerSeconds",
		"HoldInputToPower",
		"HasRepeatableSingleFire",
		"MinPowerPerShot",
		"CriticalDamageMult",
		"FastEquipSound",
		"DisableShells",
		"HasChargingAttack"
	};

	const std::unordered_map<std::string_view, std::uint32_t> g_armorPropertyMap = {
		{ "Enchantments", 0 },
		{ "BashImpactDataSet", 1 },
		{ "BlockMaterial", 2 },
		{ "Keywords", 3 },
		{ "Weight", 4 },
		{ "Value", 5 },
		{ "Rating", 6 },
		{ "AddonIndex", 7 },
		{ "BodyPart", 8 },
		{ "DamageTypeValue", 9 },
		{ "ActorValues", 10 },
		{ "Health", 11 },
		{ "ColorRemappingIndex", 12 },
		{ "MaterialSwaps", 13 }
	};

	const std::unordered_map<std::string_view, std::uint32_t> g_actorPropertyMap = {
		{ "Keywords", 0 },
		{ "ForcedInventory", 1 },
		{ "XPOffset", 2 },
		{ "Enchantments", 3 },
		{ "ColorRemappingIndex", 4 },
		{ "MaterialSwaps", 5 }
	};

	const std::unordered_map<std::string_view, std::uint32_t> g_weaponPropertyMap = {
		{ "Speed", 0 },
		{ "Reach", 1 },
		{ "MinRange", 2 },
		{ "MaxRange", 3 },
		{ "AttackDelaySec", 4 },
		{ "Unknown 5", 5 },
		{ "OutOfRangeDamageMult", 6 },
		{ "SecondaryDamage", 7 },
		{ "CriticalChargeBonus", 8 },
		{ "HitBehaviour", 9 },
		{ "Rank", 10 },
		{ "Unknown 11", 11 },
		{ "AmmoCapacity", 12 },
		{ "Unknown 13", 13 },
		{ "Unknown 14", 14 },
		{ "Type", 15 },
		{ "IsPlayerOnly", 16 },
		{ "NPCsUseAmmo", 17 },
		{ "HasChargingReload", 18 },
		{ "IsMinorCrime", 19 },
		{ "IsFixedRange", 20 },
		{ "HasEffectOnDeath", 21 },
		{ "HasAlternateRumble", 22 },
		{ "IsNonHostile", 23 },
		{ "IgnoreResist", 24 },
		{ "IsAutomatic", 25 },
		{ "CantDrop", 26 },
		{ "IsNonPlayable", 27 },
		{ "AttackDamage", 28 },
		{ "Value", 29 },
		{ "Weight", 30 },
		{ "Keywords", 31 },
		{ "AimModel", 32 },
		{ "AimModelMinConeDegrees", 33 },
		{ "AimModelMaxConeDegrees", 34 },
		{ "AimModelConeIncreasePerShot", 35 },
		{ "AimModelConeDecreasePerSec", 36 },
		{ "AimModelConeDecreaseDelayMs", 37 },
		{ "AimModelConeSneakMultiplier", 38 },
		{ "AimModelRecoilDiminishSpringForce", 39 },
		{ "AimModelRecoilDiminishSightsMult", 40 },
		{ "AimModelRecoilMaxDegPerShot", 41 },
		{ "AimModelRecoilMinDegPerShot", 42 },
		{ "AimModelRecoilHipMult", 43 },
		{ "AimModelRecoilShotsForRunaway", 44 },
		{ "AimModelRecoilArcDeg", 45 },
		{ "AimModelRecoilArcRotateDeg", 46 },
		{ "AimModelConeIronSightsMultiplier", 47 },
		{ "HasScope", 48 },
		{ "ZoomDataFOVMult", 49 },
		{ "FireSeconds", 50 },
		{ "NumProjectiles", 51 },
		{ "AttackSound", 52 },
		{ "AttackSound2D", 53 },
		{ "AttackLoop", 54 },
		{ "AttackFailSound", 55 },
		{ "IdleSound", 56 },
		{ "EquipSound", 57 },
		{ "UnEquipSound", 58 },
		{ "SoundLevel", 59 },
		{ "ImpactDataSet", 60 },
		{ "Ammo", 61 },
		{ "CritEffect", 62 },
		{ "BashImpactDataSet", 63 },
		{ "BlockMaterial", 64 },
		{ "Enchantments", 65 },
		{ "AimModelBaseStability", 66 },
		{ "ZoomData", 67 },
		{ "ZoomDataOverlay", 68 },
		{ "ZoomDataImageSpace", 69 },
		{ "ZoomDataCameraOffsetX", 70 },
		{ "ZoomDataCameraOffsetY", 71 },
		{ "ZoomDataCameraOffsetZ", 72 },
		{ "EquipSlot", 73 },
		{ "SoundLevelMult", 74 },
		{ "NPCAmmoList", 75 },
		{ "ReloadSpeed", 76 },
		{ "DamageTypeValues", 77 },
		{ "AccuracyBonus", 78 },
		{ "AttackActionPointCost", 79 },
		{ "OverrideProjectile", 80 },
		{ "HasBoltAction", 81 },
		{ "StaggerValue", 82 },
		{ "SightedTransitionSeconds", 83 },
		{ "FullPowerSeconds", 84 },
		{ "HoldInputToPower", 85 },
		{ "HasRepeatableSingleFire", 86 },
		{ "MinPowerPerShot", 87 },
		{ "ColorRemappingIndex", 88 },
		{ "MaterialSwaps", 89 },
		{ "CriticalDamageMult", 90 },
		{ "FastEquipSound", 91 },
		{ "DisableShells", 92 },
		{ "HasChargingAttack", 93 },
		{ "ActorValues", 94 }
	};

	class ObjectModificationParser : public Parsers::Parser<ConfigData> {
	public:
		ObjectModificationParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

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

			return Parsers::Statement<ConfigData>::CreateExpressionStatement(configData);
		}

		void PrintExpressionStatement(const ConfigData& a_configData, int a_indent) override {
			std::string indent = std::string(a_indent * 4, ' ');

			switch (a_configData.Element) {
			case ElementType::kProperties:
				logger::info("{}{}({}).{}", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element));
				for (std::size_t opIndex = 0; opIndex < a_configData.Operations.size(); opIndex++) {
					std::string opLog;

					switch (a_configData.Operations[opIndex].OpType) {
					case OperationType::kClear:
						opLog = fmt::format(".{}()", OperationTypeToString(a_configData.Operations[opIndex].OpType));
						break;

					case OperationType::kAdd:
						if (a_configData.Operations[opIndex].OpData->ValueType == "Int") {
							opLog = fmt::format(".{}({}, {}, {}, {}, {})", OperationTypeToString(a_configData.Operations[opIndex].OpType), a_configData.Operations[opIndex].OpData->ValueType, a_configData.Operations[opIndex].OpData->FunctionType, a_configData.Operations[opIndex].OpData->Property, std::any_cast<std::uint32_t>(a_configData.Operations[opIndex].OpData->Value1), std::any_cast<std::uint32_t>(a_configData.Operations[opIndex].OpData->Value2));
						}
						else if (a_configData.Operations[opIndex].OpData->ValueType == "Float") {
							opLog = fmt::format(".{}({}, {}, {}, {}, {})", OperationTypeToString(a_configData.Operations[opIndex].OpType), a_configData.Operations[opIndex].OpData->ValueType, a_configData.Operations[opIndex].OpData->FunctionType, a_configData.Operations[opIndex].OpData->Property, std::any_cast<float>(a_configData.Operations[opIndex].OpData->Value1), std::any_cast<float>(a_configData.Operations[opIndex].OpData->Value2));
						} 
						else if (a_configData.Operations[opIndex].OpData->ValueType == "Bool") {
							opLog = fmt::format(".{}({}, {}, {}, {}, {})", OperationTypeToString(a_configData.Operations[opIndex].OpType), a_configData.Operations[opIndex].OpData->ValueType, a_configData.Operations[opIndex].OpData->FunctionType, a_configData.Operations[opIndex].OpData->Property, std::any_cast<bool>(a_configData.Operations[opIndex].OpData->Value1), std::any_cast<bool>(a_configData.Operations[opIndex].OpData->Value2));
						}
						else if (a_configData.Operations[opIndex].OpData->ValueType == "Enum") {
							opLog = fmt::format(".{}({}, {}, {}, {})", OperationTypeToString(a_configData.Operations[opIndex].OpType), a_configData.Operations[opIndex].OpData->ValueType, a_configData.Operations[opIndex].OpData->FunctionType, a_configData.Operations[opIndex].OpData->Property, std::any_cast<std::uint32_t>(a_configData.Operations[opIndex].OpData->Value1));
						}
						else if (a_configData.Operations[opIndex].OpData->ValueType == "FormIDInt") {
							opLog = fmt::format(".{}({}, {}, {}, {})", OperationTypeToString(a_configData.Operations[opIndex].OpType), a_configData.Operations[opIndex].OpData->ValueType, a_configData.Operations[opIndex].OpData->FunctionType, a_configData.Operations[opIndex].OpData->Property, std::any_cast<std::string>(a_configData.Operations[opIndex].OpData->Value1));
						}
						else if (a_configData.Operations[opIndex].OpData->ValueType == "FormIDFloat") {
							opLog = fmt::format(".{}({}, {}, {}, {}, {})", OperationTypeToString(a_configData.Operations[opIndex].OpType), a_configData.Operations[opIndex].OpData->ValueType, a_configData.Operations[opIndex].OpData->FunctionType, a_configData.Operations[opIndex].OpData->Property, std::any_cast<std::string>(a_configData.Operations[opIndex].OpData->Value1), std::any_cast<float>(a_configData.Operations[opIndex].OpData->Value2));
						}
						break;

					default:
						break;
					}

					if (opIndex == a_configData.Operations.size() - 1) {
						opLog += ";";
					}

					logger::info("{}    {}", indent, opLog);
				}
				break;
			}
		}

		bool ParseFilter(ConfigData& a_configData) {
			auto token = reader.GetToken();
			if (token == "FilterByFormID") {
				a_configData.Filter = FilterType::kFormID;
			} else {
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

			a_configData.FilterForm = filterForm.value();

			token = reader.GetToken();
			if (token != ")") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			return true;
		}

		bool ParseElement(ConfigData& a_configData) {
			auto token = reader.GetToken();
			if (token == "Properties") {
				a_configData.Element = ElementType::kProperties;
			} else {
				logger::warn("Line {}, Col {}: Invalid ElementName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			return true;
		}

		bool ParseOperation(ConfigData& a_configData) {
			ConfigData::Operation newOp{};

			auto token = reader.GetToken();
			if (token == "Clear") {
				newOp.OpType = OperationType::kClear;
			} else if (token == "Add") {
				newOp.OpType = OperationType::kAdd;
			} else {
				logger::warn("Line {}, Col {}: Invalid OperationName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			bool isValidOperation = [](ElementType elem, OperationType op) {
				switch (elem) {
				case ElementType::kProperties:
					return (op == OperationType::kClear || op == OperationType::kAdd);
				default:
					return false;
				}
			}(a_configData.Element, newOp.OpType);

			if (!isValidOperation) {
				logger::warn("Line {}, Col {}: Invalid Operation '{}.{}()'.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_configData.Element), OperationTypeToString(newOp.OpType));
				return false;
			}

			token = reader.GetToken();
			if (token != "(") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '('.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			if (a_configData.Element == ElementType::kProperties) {
				if (newOp.OpType != OperationType::kClear) {
					ConfigData::Operation::Data opData{};

					auto valueType = ParseValueType();
					if (!valueType.has_value()) {
						return false;
					}

					opData.ValueType = valueType.value();

					token = reader.GetToken();
					if (token != ",") {
						logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
						return false;
					}

					auto funcType = ParseFunctionType();
					if (!funcType.has_value()) {
						return false;
					}

					opData.FunctionType = funcType.value();

					token = reader.GetToken();
					if (token != ",") {
						logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
						return false;
					}

					auto prop = ParseProperty();
					if (!prop.has_value()) {
						return false;
					}

					opData.Property = prop.value();

					token = reader.GetToken();
					if (token != ",") {
						logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
						return false;
					}

					bool isValidFunctionType = [](std::string_view valueType, std::string_view funcType) {
						if (valueType == "Int" || valueType == "Float") {
							return funcType == "SET" || funcType == "ADD" || funcType == "MULADD";
						}
						else if (valueType == "Bool") {
							return funcType == "SET" || funcType == "AND" || funcType == "OR";
						}
						else if (valueType == "Enum") {
							return funcType == "SET";
						}
						else if (valueType == "FormIDInt" || valueType == "FormIDFloat") {
							return funcType == "SET" || funcType == "REM" || funcType == "ADD";
						}
						return false;
					}(opData.ValueType, opData.FunctionType);

					if (!isValidFunctionType) {
						logger::warn("Line {}, Col {}: Invalid function type for {} '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), opData.ValueType, opData.FunctionType);
						return false;
					}

					if (opData.ValueType == "Int" || opData.ValueType == "Float") {
						auto value1 = ParseNumber();
						if (!value1.has_value()) {
							return false;
						}

						if (opData.ValueType == "Int") {
							opData.Value1 = std::any(static_cast<std::uint32_t>(value1.value()));
						}
						else if (opData.ValueType == "Float") {
							opData.Value1 = std::any(value1.value());
						}

						token = reader.GetToken();
						if (token != ",") {
							logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
							return false;
						}

						auto value2 = ParseNumber();
						if (!value2.has_value()) {
							return false;
						}

						if (opData.ValueType == "Int") {
							opData.Value2 = std::any(static_cast<std::uint32_t>(value2.value()));
						}
						else if (opData.ValueType == "Float") {
							opData.Value2 = std::any(value2.value());
						}
					}
					else if (opData.ValueType == "Bool") {
						auto value1 = ParseBool();
						if (!value1.has_value()) {
							return false;
						}

						opData.Value1 = std::any(value1.value());

						token = reader.GetToken();
						if (token != ",") {
							logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
							return false;
						}

						auto value2 = ParseBool();
						if (!value2.has_value()) {
							return false;
						}

						opData.Value2 = std::any(value2.value());
					}
					else if (opData.ValueType == "Enum") {
						auto value1 = ParseNumber();
						if (!value1.has_value()) {
							return false;
						}

						opData.Value1 = std::any(static_cast<std::uint32_t>(value1.value()));
					}
					else if (opData.ValueType == "FormIDInt" || opData.ValueType == "FormIDFloat") {
						auto value1 = ParseForm();
						if (!value1.has_value()) {
							return false;
						}

						opData.Value1 = std::any(value1.value());

						if (opData.ValueType == "FormIDFloat") {
							token = reader.GetToken();
							if (token != ",") {
								logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
								return false;
							}

							auto value2 = ParseNumber();
							if (!value2.has_value()) {
								return false;
							}

							opData.Value2 = std::any(value2.value());
						}
					}

					newOp.OpData = opData;
				}
			}

			token = reader.GetToken();
			if (token != ")") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			a_configData.Operations.push_back(newOp);

			return true;
		}

		std::optional<std::string> ParseValueType() {
			auto token = reader.GetToken();
			if (token == "Int" ||
				token == "Float" || 
				token == "Bool" ||
				token == "FormIDInt" ||
				token == "Enum" ||
				token == "FormIDFloat") {
				return std::string(token);
			}
			else {
				logger::warn("Line {}, Col {}: Invalid value type '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return std::nullopt;
			}
		}

		std::optional<std::string> ParseFunctionType() {
			auto token = reader.GetToken();
			if (token == "SET" ||
				token == "REM" ||
				token == "AND" ||
				token == "OR" ||
				token == "ADD" ||
				token == "MULADD") {
				return std::string(token);
			}
			else {
				logger::warn("Line {}, Col {}: Invalid function type '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return std::nullopt;
			}
		}

		std::optional<std::string> ParseProperty() {
			auto token = reader.GetToken();
			if (g_propertySet.contains(token)) {
				return std::string(token);
			}
			else {
				logger::warn("Line {}, Col {}: Invalid property name '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return std::nullopt;
			}
		}

		std::optional<bool> ParseBool() {
			auto token = reader.GetToken();
			if (token == "true") {
				return true;
			}
			else if (token == "false") {
				return false;
			}
			else {
				logger::warn("Line {}, Col {}: Invalid bool value '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return std::nullopt;
			}
		}
	};

	void ReadConfigs() {
		g_configVec = ConfigUtils::ReadConfigs<ObjectModificationParser, Parsers::Statement<ConfigData>>(TypeName);
	}

	void Prepare(const ConfigData& a_configData) {
		if (a_configData.Filter == FilterType::kFormID) {
			RE::TESForm* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
			if (!filterForm) {
				logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
				return;
			}

			RE::BGSMod::Attachment::Mod* oMod = filterForm->As<RE::BGSMod::Attachment::Mod>();
			if (!oMod) {
				logger::warn("'{}' is not a Object Modification.", a_configData.FilterForm);
				return;
			}

			PatchData& patchData = g_patchMap[oMod];

			if (a_configData.Element == ElementType::kProperties) {
				if (!patchData.Properties.has_value()) {
					patchData.Properties = PatchData::PropertiesData{};
				}

				for (const auto& op : a_configData.Operations) {
					if (op.OpType == OperationType::kClear) {
						patchData.Properties->Clear = true;
					}
					else if (op.OpType == OperationType::kAdd) {
						patchData.Properties->AddProperties.push_back({});
						auto& prop = reinterpret_cast<RE::BGSMod::Property::Mod&>(patchData.Properties->AddProperties.back());

						std::uint32_t target = 0;
						if (oMod->targetFormType.get() == RE::ENUM_FORM_ID::kWEAP)
						{
							auto it = g_weaponPropertyMap.find(op.OpData->Property);
							if (it == g_weaponPropertyMap.end()) {
								logger::warn("Invalid weapon property: '{}'.", op.OpData->Property);
								patchData.Properties->AddProperties.pop_back();
								continue;
							}

							target = it->second;
						}
						else if(oMod->targetFormType.get() == RE::ENUM_FORM_ID::kARMO)
						{
							auto it = g_armorPropertyMap.find(op.OpData->Property);
							if (it == g_armorPropertyMap.end()) {
								logger::warn("Invalid armor property: '{}'.", op.OpData->Property);
								patchData.Properties->AddProperties.pop_back();
								continue;
							}

							target = it->second;
						}
						else if (oMod->targetFormType.get() == RE::ENUM_FORM_ID::kNPC_) {
							auto it = g_actorPropertyMap.find(op.OpData->Property);
							if (it == g_actorPropertyMap.end()) {
								logger::warn("Invalid actor property: '{}'.", op.OpData->Property);
								patchData.Properties->AddProperties.pop_back();
								continue;
							}

							target = it->second;
						}
						else {
							logger::warn("Unknown target form type: '{}'.", static_cast<int>(oMod->targetFormType.get()));
							patchData.Properties->AddProperties.pop_back();
							continue;
						}

						prop.target = target;

						if (op.OpData->ValueType == "Int" || op.OpData->ValueType == "Float") {
							if (op.OpData->ValueType == "Int") {
								prop.type = RE::BGSMod::Property::TYPE::kInt;
							} else {
								prop.type = RE::BGSMod::Property::TYPE::kFloat;
							}

							if (op.OpData->FunctionType == "SET") {
								prop.op = RE::BGSMod::Property::OP::kSet;
							} else if (op.OpData->FunctionType == "ADD") {
								prop.op = RE::BGSMod::Property::OP::kAdd;
							} else {  // op.OpData->FunctionType == "MULADD"
								prop.op = RE::BGSMod::Property::OP::kMul;
							}

							if (op.OpData->ValueType == "Int") {
								prop.data.mm.min.i = static_cast<std::int32_t>(std::any_cast<std::uint32_t>(op.OpData->Value1));
								prop.data.mm.max.i = static_cast<std::int32_t>(std::any_cast<std::uint32_t>(op.OpData->Value2));
							}
							else {
								prop.data.mm.min.f = std::any_cast<float>(op.OpData->Value1);
								prop.data.mm.max.f = std::any_cast<float>(op.OpData->Value2);
							}
						}
						else if (op.OpData->ValueType == "Bool") {
							prop.type = RE::BGSMod::Property::TYPE::kBool;

							if (op.OpData->FunctionType == "SET") {
								prop.op = RE::BGSMod::Property::OP::kSet;
							} else if (op.OpData->FunctionType == "AND") {
								prop.op = RE::BGSMod::Property::OP::kAnd;
							} else { // op.OpData->FunctionType == "OR"
								prop.op = RE::BGSMod::Property::OP::kOr;
							}

							prop.data.mm.min.i = static_cast<std::int32_t>(std::any_cast<bool>(op.OpData->Value1));
							prop.data.mm.max.i = static_cast<std::int32_t>(std::any_cast<bool>(op.OpData->Value2));
						}
						else if (op.OpData->ValueType == "Enum") {
							prop.type = RE::BGSMod::Property::TYPE::kEnum;

							prop.op = RE::BGSMod::Property::OP::kSet;

							prop.data.mm.min.i = static_cast<std::int32_t>(std::any_cast<std::uint32_t>(op.OpData->Value1));
						}
						else if (op.OpData->ValueType == "FormIDInt" || op.OpData->ValueType == "FormIDFloat") {
							std::string formSV = std::any_cast<std::string>(op.OpData->Value1);

							RE::TESForm* targetForm = Utils::GetFormFromString(formSV);
							if (!targetForm) {
								logger::warn("Invalid FormID: '{}'.", formSV);
								patchData.Properties->AddProperties.pop_back();
								continue;
							}

							if (op.OpData->ValueType == "FormIDInt") {
								prop.type = RE::BGSMod::Property::TYPE::kForm;
							}
							else {
								prop.type = RE::BGSMod::Property::TYPE::kPair;
							}

							if (op.OpData->FunctionType == "SET") {
								prop.op = RE::BGSMod::Property::OP::kSet;
							}
							else if (op.OpData->FunctionType == "REM") {
								prop.op = RE::BGSMod::Property::OP::kRem;
							}
							else {
								prop.op = RE::BGSMod::Property::OP::kAdd;
							}

							if (op.OpData->ValueType == "FormIDInt") {
								prop.data.form = targetForm;
							}
							else {
								prop.data.fv.formID = targetForm->formID;
								prop.data.fv.value = std::any_cast<float>(op.OpData->Value2);
							}
						}
					}
				}
			}
		}
	}

	void GetProperties(RE::BGSMod::Attachment::Mod* a_oMod, std::vector<PropertyContainer>& a_properties) {
		std::uint32_t oModCount = static_cast<std::uint32_t>(a_oMod->size / sizeof(RE::BGSMod::Property::Mod));

		if (!a_oMod->buffer || oModCount == 0) {
			return;
		}

		a_properties.resize(oModCount);

		std::memcpy(a_properties.data(), a_oMod->buffer, oModCount * sizeof(RE::BGSMod::Property::Mod));
	}

	void PatchProperties(RE::BGSMod::Attachment::Mod* a_oMod, const std::vector<PropertyContainer>& a_properties) {
		RE::MemoryManager& mm = RE::MemoryManager::GetSingleton();

		if (a_oMod->buffer) {
			mm.Deallocate(a_oMod->buffer, false);
			a_oMod->buffer = nullptr;
		}

		a_oMod->size = 0;

		if (a_properties.empty()) {
			return;
		}

		std::size_t allocateSize = sizeof(RE::BGSMod::Property::Mod) * a_properties.size();

		void* ptr = mm.Allocate(allocateSize + sizeof(std::size_t), 0, false);
		if (!ptr) {
			logger::critical("Failed to allocate the new Properties.");
			return;
		}

		std::memcpy(ptr, a_properties.data(), allocateSize);

		a_oMod->buffer = reinterpret_cast<std::byte*>(ptr);
		a_oMod->size = static_cast<std::uint32_t>(allocateSize);

		std::size_t postData = (0x01000000 | allocateSize) << 32;
		std::memcpy(a_oMod->buffer + allocateSize, &postData, sizeof(postData));
	}

	void PatchProperties(RE::BGSMod::Attachment::Mod* a_oMod, const PatchData::PropertiesData& a_propertiesData) {
		bool isCleared = false, isAdded = false;

		std::vector<PropertyContainer> properties;

		// Clear
		if (a_propertiesData.Clear) {
			isCleared = true;
		}
		else {
			GetProperties(a_oMod, properties);
		}

		// Add
		const auto& addProps = a_propertiesData.AddProperties;
		if (!addProps.empty()) {
			size_t originalSize = properties.size();
			size_t addCount = addProps.size();
			properties.resize(originalSize + addCount);

			std::memcpy(properties.data() + originalSize, addProps.data(), addCount * sizeof(RE::BGSMod::Property::Mod));
			isAdded = true;
		}

		if (isCleared || isAdded) {
			PatchProperties(a_oMod, properties);
		}
	}

	void Patch(RE::BGSMod::Attachment::Mod* a_oMod, const PatchData& a_patchData) {
		if (a_patchData.Properties.has_value()) {
			PatchProperties(a_oMod, a_patchData.Properties.value());
		}
	}

	void Patch()
	{
		logger::info("======================== Start preparing patch for {} ========================", TypeName);

		ConfigUtils::Prepare(g_configVec, Prepare);

		logger::info("======================== Finished preparing patch for {} ========================", TypeName);
		logger::info("");

		logger::info("======================== Start patching for {} ========================", TypeName);

		for (const auto& patchData : g_patchMap) {
			Patch(patchData.first, patchData.second);
		}

		logger::info("======================== Finished patching for {} ========================", TypeName);
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}
