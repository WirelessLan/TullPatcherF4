#include "Armors.h"

#include <regex>
#include <any>

#include "Parsers.h"
#include "Utils.h"

namespace Armors {
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
		kArmorRating,
		kBipedObjectSlots,
		kFullName,
		kKeywords,
		kObjectEffect,
		kResistances,
	};

	std::string_view ElementTypeToString(ElementType a_value) {
		switch (a_value) {
		case ElementType::kArmorRating: return "ArmorRating";
		case ElementType::kBipedObjectSlots: return "BipedObjectSlots";
		case ElementType::kFullName: return "FullName";
		case ElementType::kKeywords: return "Keywords";
		case ElementType::kObjectEffect: return "ObjectEffect";
		case ElementType::kResistances: return "Resistances";
		default: return std::string_view{};
		}
	}

	enum class OperationType {
		kClear,
		kAdd,
		kDelete
	};

	std::string_view OperationTypeToString(OperationType a_value) {
		switch (a_value) {
		case OperationType::kClear: return "Clear";
		case OperationType::kAdd: return "Add";
		case OperationType::kDelete: return "Delete";
		default: return std::string_view{};
		}
	}

	struct ConfigData {
		struct Operation {
			struct ResistanceData {
				std::string Form;
				std::uint32_t Value;
			};

			OperationType OpType;
			std::optional<std::any> OpData;
		};

		FilterType Filter;
		std::string FilterForm;
		ElementType Element;
		std::vector<Operation> Operations;
		std::optional<std::any> AssignValue;
	};

	struct PatchData {
		struct KeywordsData {
			bool Clear;
			std::vector<RE::BGSKeyword*> AddKeywordVec;
			std::vector<RE::BGSKeyword*> DeleteKeywordVec;
		};

		struct ResistancesData {
			struct Resistance {
				RE::BGSDamageType* DamageType;
				std::uint32_t Value;
			};

			bool Clear;
			std::vector<Resistance> AddResistanceVec;
			std::vector<Resistance> DeleteResistanceVec;
		};

		std::optional<std::uint16_t> ArmorRating;
		std::optional<std::uint32_t> BipedObjectSlots;
		std::optional<std::string> FullName;
		std::optional<KeywordsData> Keywords;
		std::optional<RE::EnchantmentItem*> ObjectEffect;
		std::optional<ResistancesData> Resistances;
	};

	std::vector<Parsers::Statement<ConfigData>> g_configVec;
	std::unordered_map<RE::TESObjectARMO*, PatchData> g_patchMap;

	class ArmorParser : public Parsers::Parser<ConfigData> {
	public:
		ArmorParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

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

			token = reader.GetToken();
			if (token != ";") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ';'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			return Parsers::Statement<ConfigData>::CreateExpressionStatement(configData);
		}

		void PrintExpressionStatement(const ConfigData& a_configData, int a_indent) override {
			std::string indent = std::string(a_indent * 4, ' ');

			switch (a_configData.Element) {
			case ElementType::kArmorRating:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm,
					ElementTypeToString(a_configData.Element), std::any_cast<std::uint16_t>(a_configData.AssignValue.value()));
				break;

			case ElementType::kBipedObjectSlots:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm,
					ElementTypeToString(a_configData.Element), GetBipedSlots(std::any_cast<std::uint32_t>(a_configData.AssignValue.value())));
				break;

			case ElementType::kFullName:
				logger::info("{}{}({}).{} = \"{}\";", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm,
					ElementTypeToString(a_configData.Element), std::any_cast<std::string>(a_configData.AssignValue.value()));
				break;

			case ElementType::kKeywords:
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

			case ElementType::kObjectEffect:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm,
					ElementTypeToString(a_configData.Element), std::any_cast<std::string>(a_configData.AssignValue.value()));
				break;

			case ElementType::kResistances:
				logger::info("{}{}({}).{}", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element));
				for (std::size_t ii = 0; ii < a_configData.Operations.size(); ii++) {
					std::string opLog;

					switch (a_configData.Operations[ii].OpType) {
					case OperationType::kClear:
						opLog = fmt::format(".{}()", OperationTypeToString(a_configData.Operations[ii].OpType));
						break;

					case OperationType::kAdd:
						opLog = fmt::format(".{}({}, {})", OperationTypeToString(a_configData.Operations[ii].OpType),
							std::any_cast<ConfigData::Operation::ResistanceData>(a_configData.Operations[ii].OpData.value()).Form,
							std::any_cast<ConfigData::Operation::ResistanceData>(a_configData.Operations[ii].OpData.value()).Value);
						break;

					case OperationType::kDelete:
						opLog = fmt::format(".{}({})", OperationTypeToString(a_configData.Operations[ii].OpType),
							std::any_cast<ConfigData::Operation::ResistanceData>(a_configData.Operations[ii].OpData.value()).Form);
						break;
					}

					if (ii == a_configData.Operations.size() - 1) {
						opLog += ";";
					}

					logger::info("{}    {}", indent, opLog);
				}
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
			if (token == "ArmorRating") {
				a_config.Element = ElementType::kArmorRating;
			}
			else if (token == "BipedObjectSlots") {
				a_config.Element = ElementType::kBipedObjectSlots;
			}
			else if (token == "FullName") {
				a_config.Element = ElementType::kFullName;
			}
			else if (token == "Keywords") {
				a_config.Element = ElementType::kKeywords;
			}
			else if (token == "ObjectEffect") {
				a_config.Element = ElementType::kObjectEffect;
			}
			else if (token == "Resistances") {
				a_config.Element = ElementType::kResistances;
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

			if (a_config.Element == ElementType::kArmorRating) {
				token = reader.GetToken();
				if (token.empty()) {
					logger::warn("Line {}, Col {}: Expected Value '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				unsigned long parsedValue;
				auto parsingResult = std::from_chars(token.data(), token.data() + token.size(), parsedValue);
				if (parsingResult.ec != std::errc()) {
					logger::warn("Line {}, Col {}: Failed to parse value '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				a_config.AssignValue = std::any(static_cast<std::uint16_t>(parsedValue));
			}
			else if (a_config.Element == ElementType::kBipedObjectSlots) {
				std::uint32_t bipedObjectSlotsValue = 0;

				auto bipedSlot = ParseBipedSlot();
				if (!bipedSlot.has_value()) {
					return false;
				}

				if (bipedSlot.value() != 0) {
					bipedObjectSlotsValue |= 1 << (bipedSlot.value() - 30);
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
						bipedObjectSlotsValue |= 1 << (bipedSlot.value() - 30);
					}
				}

				a_config.AssignValue = std::any(bipedObjectSlotsValue);
			}
			else if (a_config.Element == ElementType::kFullName) {
				token = reader.GetToken();
				if (!token.starts_with('\"')) {
					logger::warn("Line {}, Col {}: FullName must be a string.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}
				else if (!token.ends_with('\"')) {
					logger::warn("Line {}, Col {}: String must end with '\"'.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}

				std::string value = std::string(token.substr(1, token.length() - 2));
				a_config.AssignValue = std::any(value);
			}
			else if (a_config.Element == ElementType::kObjectEffect) {
				token = reader.Peek();
				if (token == "null") {
					a_config.AssignValue = std::any(std::string(reader.GetToken()));
				} else {
					auto effectForm = ParseForm();
					if (!effectForm.has_value()) {
						return false;
					}

					a_config.AssignValue = std::any(std::string(effectForm.value()));
				}
			}
			else {
				logger::warn("Line {}, Col {}: Invalid Assignment to {}.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_config.Element));
				return false;
			}

			return true;
		}

		bool ParseOperation(ConfigData& a_configData) {
			ConfigData::Operation newOp;

			auto token = reader.GetToken();
			if (token == "Clear") {
				newOp.OpType = OperationType::kClear;
			}
			else if (token == "Add") {
				newOp.OpType = OperationType::kAdd;
			}
			else if (token == "Delete") {
				newOp.OpType = OperationType::kDelete;
			}
			else {
				logger::warn("Line {}, Col {}: Invalid OperationName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			switch (a_configData.Element) {
			case ElementType::kKeywords:
			case ElementType::kResistances:
				switch (newOp.OpType) {
				case OperationType::kClear:
				case OperationType::kAdd:
				case OperationType::kDelete:
					break;

				default:
					logger::warn("Line {}, Col {}: Invalid Operation '{}.{}()'.",
						reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_configData.Element), OperationTypeToString(newOp.OpType));
					return false;
				}
				break;
			}

			token = reader.GetToken();
			if (token != "(") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '('.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			switch (a_configData.Element) {
			case ElementType::kKeywords:
				if (newOp.OpType != OperationType::kClear) {
					auto form = ParseForm();
					if (!form.has_value()) {
						return false;
					}

					newOp.OpData = std::any(form.value());
				}
				break;

			case ElementType::kResistances:
				if (newOp.OpType != OperationType::kClear) {
					ConfigData::Operation::ResistanceData resistanceData{};

					auto form = ParseForm();
					if (!form.has_value()) {
						return false;
					}

					resistanceData.Form = form.value();

					if (newOp.OpType == OperationType::kAdd) {
						token = reader.GetToken();
						if (token != ",") {
							logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
							return false;
						}

						token = reader.GetToken();
						if (token.empty()) {
							logger::warn("Line {}, Col {}: Expected Value '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
							return false;
						}

						unsigned long parsedValue;
						auto parsingResult = std::from_chars(token.data(), token.data() + token.size(), parsedValue);
						if (parsingResult.ec != std::errc()) {
							logger::warn("Line {}, Col {}: Failed to parse value '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
							return false;
						}

						resistanceData.Value = static_cast<std::uint32_t>(parsedValue);
					}

					newOp.OpData = std::any(resistanceData);
				}
				break;
			}

			token = reader.GetToken();
			if (token != ")") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			a_configData.Operations.push_back(newOp);

			return true;
		}
	};

	void ReadConfig(std::string_view a_path) {
		ArmorParser parser(a_path);
		auto parsedStatements = parser.Parse();
		g_configVec.insert(g_configVec.end(), parsedStatements.begin(), parsedStatements.end());
	}

	void ReadConfigs() {
		const std::filesystem::path configDir{ "Data\\" + std::string(Version::PROJECT) + "\\Armor" };
		if (!std::filesystem::exists(configDir)) {
			return;
		}

		const std::regex filter(".*\\.cfg", std::regex_constants::icase);
		const std::filesystem::directory_iterator dir_iter(configDir);
		for (auto& iter : dir_iter) {
			if (!std::filesystem::is_regular_file(iter.status())) {
				continue;
			}

			if (!std::regex_match(iter.path().filename().string(), filter)) {
				continue;
			}

			std::string path = iter.path().string();
			logger::info("=========== Reading Armor config file: {} ===========", path);
			ReadConfig(path);
			logger::info("");
		}
	}

	void Prepare(const ConfigData& a_configData) {
		if (a_configData.Filter == FilterType::kFormID) {
			RE::TESForm* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
			if (!filterForm) {
				logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
				return;
			}

			RE::TESObjectARMO* armo = filterForm->As<RE::TESObjectARMO>();
			if (!armo) {
				logger::warn("'{}' is not a Armor.", a_configData.FilterForm);
				return;
			}

			if (a_configData.Element == ElementType::kArmorRating) {
				g_patchMap[armo].ArmorRating = std::any_cast<std::uint16_t>(a_configData.AssignValue.value());
			} 
			else if (a_configData.Element == ElementType::kBipedObjectSlots) {
				g_patchMap[armo].BipedObjectSlots = std::any_cast<std::uint32_t>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kFullName) {
				g_patchMap[armo].FullName = std::any_cast<std::string>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kKeywords) {
				if (!g_patchMap[armo].Keywords.has_value()) {
					g_patchMap[armo].Keywords = PatchData::KeywordsData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.OpType == OperationType::kClear) {
						g_patchMap[armo].Keywords->Clear = true;
					}
					else if (operation.OpType == OperationType::kAdd || operation.OpType == OperationType::kDelete) {
						std::string keywordFormStr = std::any_cast<std::string>(operation.OpData.value());

						RE::TESForm* keywordForm = Utils::GetFormFromString(keywordFormStr);
						if (!keywordForm) {
							logger::warn("Invalid Form: '{}'.", keywordFormStr);
							return;
						}

						RE::BGSKeyword* keyword = keywordForm->As<RE::BGSKeyword>();
						if (!keyword) {
							logger::warn("'{}' is not a Keyword.", keywordFormStr);
							return;
						}

						if (operation.OpType == OperationType::kAdd) {
							g_patchMap[armo].Keywords->AddKeywordVec.push_back(keyword);
						}
						else {
							g_patchMap[armo].Keywords->DeleteKeywordVec.push_back(keyword);
						}
					}
				}
			}
			else if (a_configData.Element == ElementType::kObjectEffect) {
				std::string effectFormStr = std::any_cast<std::string>(a_configData.AssignValue.value());

				if (effectFormStr == "null") {
					g_patchMap[armo].ObjectEffect = nullptr;
				}
				else {
					RE::TESForm* effectForm = Utils::GetFormFromString(effectFormStr);
					if (!effectForm) {
						logger::warn("Invalid Form: '{}'.", effectFormStr);
						return;
					}

					RE::EnchantmentItem* objectEffect = effectForm->As<RE::EnchantmentItem>();
					if (!objectEffect) {
						logger::warn("'{}' is not an Object Effect.", effectFormStr);
						return;
					}

					g_patchMap[armo].ObjectEffect = objectEffect;
				}
			}
			else if (a_configData.Element == ElementType::kResistances) {
				if (!g_patchMap[armo].Resistances.has_value()) {
					g_patchMap[armo].Resistances = PatchData::ResistancesData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.OpType == OperationType::kClear) {
						g_patchMap[armo].Resistances->Clear = true;
					}
					else if (operation.OpType == OperationType::kAdd || operation.OpType == OperationType::kDelete) {
						ConfigData::Operation::ResistanceData resistanceData = std::any_cast<ConfigData::Operation::ResistanceData>(operation.OpData.value());

						RE::TESForm* form = Utils::GetFormFromString(resistanceData.Form);
						if (!form) {
							logger::warn("Invalid Form: '{}'.", resistanceData.Form);
							continue;
						}

						RE::BGSDamageType* damageType = form->As<RE::BGSDamageType>();
						if (!damageType) {
							logger::warn("'{}' is not a Damage Type.", resistanceData.Form);
							continue;
						}

						PatchData::ResistancesData::Resistance resistance{ damageType, resistanceData.Value };

						if (operation.OpType == OperationType::kAdd) {
							g_patchMap[armo].Resistances->AddResistanceVec.push_back(resistance);
						}
						else {
							g_patchMap[armo].Resistances->DeleteResistanceVec.push_back(resistance);
						}
					}
				}
			}
		}
	}

	void Prepare(const std::vector<Parsers::Statement<ConfigData>>& a_configVec) {
		for (const auto& configData : a_configVec) {
			if (configData.Type == Parsers::StatementType::kExpression) {
				Prepare(configData.ExpressionStatement.value());
			}
			else if (configData.Type == Parsers::StatementType::kConditional) {
				Prepare(configData.ConditionalStatement->Evaluates());
			}
		}
	}

	void PatchKeywords(RE::TESObjectARMO* a_armo, const PatchData::KeywordsData& a_keywordsData) {
		bool isCleared = false;

		if (a_keywordsData.Clear) {
			while (a_armo->numKeywords > 0) {
				a_armo->RemoveKeyword(a_armo->keywords[0]);
			}
			isCleared = true;
		}

		// Delete
		if (!isCleared) {
			for (const auto& keyword : a_keywordsData.DeleteKeywordVec) {
				if (a_armo->HasKeyword(keyword)) {
					a_armo->RemoveKeyword(keyword);
				}
			}
		}

		// Add
		for (const auto& keyword : a_keywordsData.AddKeywordVec) {
			if (!a_armo->HasKeyword(keyword)) {
				a_armo->AddKeyword(keyword);
			}
		}
	}

	void PatchResistances(RE::TESObjectARMO* a_armo, const PatchData::ResistancesData& a_resistancesData) {
		bool isCleared = false;

		if (a_resistancesData.Clear) {
			a_armo->armorData.damageTypes->clear();
			isCleared = true;
		}

		// Delete
		if (!isCleared) {
			for (const auto& resistance : a_resistancesData.DeleteResistanceVec) {
				auto iter = std::find_if(a_armo->armorData.damageTypes->begin(), a_armo->armorData.damageTypes->end(), [&resistance](const RE::BSTTuple<RE::TESForm*, RE::BGSTypedFormValuePair::SharedVal>& a_elem) {
					return a_elem.first == resistance.DamageType;
				});

				if (iter != a_armo->armorData.damageTypes->end()) {
					a_armo->armorData.damageTypes->erase(iter);
				}
			}
		}

		// Add
		for (const auto& resistance : a_resistancesData.AddResistanceVec) {
			auto iter = std::find_if(a_armo->armorData.damageTypes->begin(), a_armo->armorData.damageTypes->end(), [&resistance](const RE::BSTTuple<RE::TESForm*, RE::BGSTypedFormValuePair::SharedVal>& a_elem) {
				return a_elem.first == resistance.DamageType && a_elem.second.i == resistance.Value;
			});

			if (iter == a_armo->armorData.damageTypes->end()) {
				a_armo->armorData.damageTypes->emplace_back(resistance.DamageType, resistance.Value);
			}
		}
	}

	void Patch() {
		logger::info("======================== Start preparing patch for Armor ========================");

		Prepare(g_configVec);

		logger::info("======================== Finished preparing patch for Armor ========================");
		logger::info("");

		logger::info("======================== Start patching for Armor ========================");

		for (const auto& patchData : g_patchMap) {
			if (patchData.second.ArmorRating.has_value()) {
				patchData.first->armorData.rating = patchData.second.ArmorRating.value();
			}
			if (patchData.second.BipedObjectSlots.has_value()) {
				patchData.first->bipedModelData.bipedObjectSlots = patchData.second.BipedObjectSlots.value();
			}
			if (patchData.second.FullName.has_value()) {
				patchData.first->fullName = patchData.second.FullName.value();
			}
			if (patchData.second.Keywords.has_value()) {
				PatchKeywords(patchData.first, patchData.second.Keywords.value());
			}
			if (patchData.second.ObjectEffect.has_value()) {
				patchData.first->formEnchanting = patchData.second.ObjectEffect.value();
			}
			if (patchData.second.Resistances.has_value()) {
				PatchResistances(patchData.first, patchData.second.Resistances.value());
			}
		}

		logger::info("======================== Finished patching for Armor ========================");
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}
