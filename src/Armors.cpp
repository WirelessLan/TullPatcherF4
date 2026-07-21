#include "Armors.h"

#include <any>
#include <regex>

#include "ConfigUtils.h"
#include "Parsers.h"
#include "Utils.h"

namespace Armors
{
	namespace
	{
		constexpr std::string_view kTypeName = "Armor";

		enum class FilterType
		{
			kFormID
		};

		std::string_view FilterTypeToString(FilterType a_value)
		{
			switch (a_value)
			{
			case FilterType::kFormID:
				return "FilterByFormID";
			default:
				return std::string_view{};
			}
		}

		enum class ElementType
		{
			kArmorRating,
			kBipedObjectSlots,
			kFullName,
			kKeywords,
			kObjectEffect,
			kResistances,
		};

		std::string_view ElementTypeToString(ElementType a_value)
		{
			switch (a_value)
			{
			case ElementType::kArmorRating:
				return "ArmorRating";
			case ElementType::kBipedObjectSlots:
				return "BipedObjectSlots";
			case ElementType::kFullName:
				return "FullName";
			case ElementType::kKeywords:
				return "Keywords";
			case ElementType::kObjectEffect:
				return "ObjectEffect";
			case ElementType::kResistances:
				return "Resistances";
			default:
				return std::string_view{};
			}
		}

		enum class OperationType
		{
			kClear,
			kAdd,
			kDelete
		};

		std::string_view OperationTypeToString(OperationType a_value)
		{
			switch (a_value)
			{
			case OperationType::kClear:
				return "Clear";
			case OperationType::kAdd:
				return "Add";
			case OperationType::kDelete:
				return "Delete";
			default:
				return std::string_view{};
			}
		}

		struct ConfigData
		{
			struct Operation
			{
				struct ResistanceData
				{
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

		struct PatchData
		{
			struct KeywordsData
			{
				bool Clear;
				std::vector<RE::BGSKeyword*> AddKeywordVec;
				std::vector<RE::BGSKeyword*> DeleteKeywordVec;
			};

			struct ResistancesData
			{
				struct Resistance
				{
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

		class ArmorParser : public Parsers::Parser<ConfigData>
		{
		public:
			ArmorParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

		protected:
			std::optional<Parsers::Statement<ConfigData>> ParseExpressionStatement() override
			{
				ConfigData configData{};

				if (!ParseFilter(configData))
				{
					return std::nullopt;
				}

				auto token = reader.GetToken();
				if (token != ".")
				{
					logger::warn("Line {}, Col {}: Syntax error. Expected '.'.", reader.GetLastLine(), reader.GetLastLineIndex());
					return std::nullopt;
				}

				if (!ParseElement(configData))
				{
					return std::nullopt;
				}

				token = reader.Peek();
				if (token == "=")
				{
					if (!ParseAssignment(configData))
					{
						return std::nullopt;
					}
				}
				else
				{
					do
					{
						token = reader.GetToken();
						if (token != ".")
						{
							logger::warn("Line {}, Col {}: Syntax error. Expected '.'.", reader.GetLastLine(), reader.GetLastLineIndex());
							return std::nullopt;
						}

						if (!ParseOperation(configData))
						{
							return std::nullopt;
						}
					} while (reader.Peek() == ".");
				}

				token = reader.GetToken();
				if (token != ";")
				{
					logger::warn("Line {}, Col {}: Syntax error. Expected ';'.", reader.GetLastLine(), reader.GetLastLineIndex());
					return std::nullopt;
				}

				return Parsers::Statement<ConfigData>::CreateExpressionStatement(configData);
			}

			void PrintExpressionStatement(const ConfigData& a_configData, int a_indent) override
			{
				auto indent = std::string(a_indent * 4, ' ');

				switch (a_configData.Element)
				{
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
					for (std::size_t opIndex = 0; opIndex < a_configData.Operations.size(); ++opIndex)
					{
						std::string opLog;

						switch (a_configData.Operations[opIndex].OpType)
						{
						case OperationType::kClear:
							opLog = fmt::format(".{}()", OperationTypeToString(a_configData.Operations[opIndex].OpType));
							break;

						case OperationType::kAdd:
						case OperationType::kDelete:
							opLog = fmt::format(".{}({})", OperationTypeToString(a_configData.Operations[opIndex].OpType),
								std::any_cast<std::string>(a_configData.Operations[opIndex].OpData.value()));
							break;
						}

						if (opIndex == a_configData.Operations.size() - 1)
						{
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
					for (std::size_t opIndex = 0; opIndex < a_configData.Operations.size(); ++opIndex)
					{
						std::string opLog;

						switch (a_configData.Operations[opIndex].OpType)
						{
						case OperationType::kClear:
							opLog = fmt::format(".{}()", OperationTypeToString(a_configData.Operations[opIndex].OpType));
							break;

						case OperationType::kAdd:
							opLog = fmt::format(".{}({}, {})", OperationTypeToString(a_configData.Operations[opIndex].OpType),
								std::any_cast<ConfigData::Operation::ResistanceData>(a_configData.Operations[opIndex].OpData.value()).Form,
								std::any_cast<ConfigData::Operation::ResistanceData>(a_configData.Operations[opIndex].OpData.value()).Value);
							break;

						case OperationType::kDelete:
							opLog = fmt::format(".{}({})", OperationTypeToString(a_configData.Operations[opIndex].OpType),
								std::any_cast<ConfigData::Operation::ResistanceData>(a_configData.Operations[opIndex].OpData.value()).Form);
							break;
						}

						if (opIndex == a_configData.Operations.size() - 1)
						{
							opLog += ";";
						}

						logger::info("{}    {}", indent, opLog);
					}
					break;
				}
			}

			bool ParseFilter(ConfigData& a_config)
			{
				auto token = reader.GetToken();
				if (token == "FilterByFormID")
				{
					a_config.Filter = FilterType::kFormID;
				}
				else
				{
					logger::warn("Line {}, Col {}: Invalid FilterName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				token = reader.GetToken();
				if (token != "(")
				{
					logger::warn("Line {}, Col {}: Syntax error. Expected '('.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}

				const auto filterFormOpt = ParseForm();
				if (!filterFormOpt.has_value())
				{
					return false;
				}

				a_config.FilterForm = filterFormOpt.value();

				token = reader.GetToken();
				if (token != ")")
				{
					logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}

				return true;
			}

			bool ParseElement(ConfigData& a_config)
			{
				const auto token = reader.GetToken();
				if (token == "ArmorRating")
				{
					a_config.Element = ElementType::kArmorRating;
				}
				else if (token == "BipedObjectSlots")
				{
					a_config.Element = ElementType::kBipedObjectSlots;
				}
				else if (token == "FullName")
				{
					a_config.Element = ElementType::kFullName;
				}
				else if (token == "Keywords")
				{
					a_config.Element = ElementType::kKeywords;
				}
				else if (token == "ObjectEffect")
				{
					a_config.Element = ElementType::kObjectEffect;
				}
				else if (token == "Resistances")
				{
					a_config.Element = ElementType::kResistances;
				}
				else
				{
					logger::warn("Line {}, Col {}: Invalid ElementName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				return true;
			}

			bool ParseAssignment(ConfigData& a_config)
			{
				auto token = reader.GetToken();
				if (token != "=")
				{
					logger::warn("Line {}, Col {}: Syntax error. Expected '='.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}

				if (a_config.Element == ElementType::kArmorRating)
				{
					const auto valueOpt = ParseNumber<std::uint16_t>();
					if (!valueOpt.has_value())
					{
						return false;
					}

					a_config.AssignValue = std::any(valueOpt.value());
				}
				else if (a_config.Element == ElementType::kBipedObjectSlots)
				{
					std::uint32_t bipedSlots = 0;

					auto bipedSlotOpt = ParseBipedSlot();
					if (!bipedSlotOpt.has_value())
					{
						return false;
					}

					if (bipedSlotOpt.value() != 0)
					{
						bipedSlots |= 1u << (bipedSlotOpt.value() - 30);
					}

					while (reader.Peek() == "|")
					{
						reader.GetToken();

						bipedSlotOpt = ParseBipedSlot();
						if (!bipedSlotOpt.has_value())
						{
							return false;
						}

						if (bipedSlotOpt.value() != 0)
						{
							bipedSlots |= 1u << (bipedSlotOpt.value() - 30);
						}
					}

					a_config.AssignValue = std::any(bipedSlots);
				}
				else if (a_config.Element == ElementType::kFullName)
				{
					const auto fullNameOpt = ParseString();
					if (!fullNameOpt.has_value())
					{
						return false;
					}

					a_config.AssignValue = std::any(fullNameOpt.value());
				}
				else if (a_config.Element == ElementType::kObjectEffect)
				{
					token = reader.Peek();
					if (token == "null")
					{
						a_config.AssignValue = std::any(std::string(reader.GetToken()));
						return true;
					}

					const auto effectFormOpt = ParseForm();
					if (!effectFormOpt.has_value())
					{
						return false;
					}

					a_config.AssignValue = std::any(effectFormOpt.value());
				}
				else
				{
					logger::warn("Line {}, Col {}: Invalid Assignment for '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_config.Element));
					return false;
				}

				return true;
			}

			bool ParseOperation(ConfigData& a_configData)
			{
				ConfigData::Operation newOp{};

				auto token = reader.GetToken();
				if (token == "Clear")
				{
					newOp.OpType = OperationType::kClear;
				}
				else if (token == "Add")
				{
					newOp.OpType = OperationType::kAdd;
				}
				else if (token == "Delete")
				{
					newOp.OpType = OperationType::kDelete;
				}
				else
				{
					logger::warn("Line {}, Col {}: Invalid OperationName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				auto isValidOperation = [](ElementType elem, OperationType op) -> bool {
					if (elem == ElementType::kKeywords)
					{
						return op == OperationType::kClear || op == OperationType::kAdd || op == OperationType::kDelete;
					}
					else if (elem == ElementType::kResistances)
					{
						return op == OperationType::kClear || op == OperationType::kAdd || op == OperationType::kDelete;
					}
					return false;
				}(a_configData.Element, newOp.OpType);

				if (!isValidOperation)
				{
					logger::warn("Line {}, Col {}: Invalid Operation '{}.{}()'.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_configData.Element), OperationTypeToString(newOp.OpType));
					return false;
				}

				token = reader.GetToken();
				if (token != "(")
				{
					logger::warn("Line {}, Col {}: Syntax error. Expected '('.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}

				switch (a_configData.Element)
				{
				case ElementType::kKeywords:
					if (newOp.OpType != OperationType::kClear)
					{
						const auto formOpt = ParseForm();
						if (!formOpt.has_value())
						{
							return false;
						}

						newOp.OpData = std::any(formOpt.value());
					}
					break;

				case ElementType::kResistances:
					if (newOp.OpType != OperationType::kClear)
					{
						ConfigData::Operation::ResistanceData resistanceData{};

						const auto formOpt = ParseForm();
						if (!formOpt.has_value())
						{
							return false;
						}
						resistanceData.Form = formOpt.value();

						if (newOp.OpType == OperationType::kAdd)
						{
							token = reader.GetToken();
							if (token != ",")
							{
								logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
								return false;
							}

							const auto valueOpt = ParseNumber<std::uint32_t>();
							if (!valueOpt.has_value())
							{
								return false;
							}
							resistanceData.Value = valueOpt.value();
						}

						newOp.OpData = std::any(resistanceData);
					}
					break;
				}

				token = reader.GetToken();
				if (token != ")")
				{
					logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}

				a_configData.Operations.emplace_back(newOp);

				return true;
			}
		};

		void Prepare(const ConfigData& a_configData)
		{
			if (a_configData.Filter == FilterType::kFormID)
			{
				auto* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
				if (!filterForm)
				{
					logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
					return;
				}

				auto* armo = filterForm->As<RE::TESObjectARMO>();
				if (!armo)
				{
					logger::warn("'{}' is not a Armor.", a_configData.FilterForm);
					return;
				}

				auto& patchData = g_patchMap[armo];

				if (a_configData.Element == ElementType::kArmorRating)
				{
					patchData.ArmorRating = std::any_cast<std::uint16_t>(a_configData.AssignValue.value());
				}
				else if (a_configData.Element == ElementType::kBipedObjectSlots)
				{
					patchData.BipedObjectSlots = std::any_cast<std::uint32_t>(a_configData.AssignValue.value());
				}
				else if (a_configData.Element == ElementType::kFullName)
				{
					patchData.FullName = std::any_cast<std::string>(a_configData.AssignValue.value());
				}
				else if (a_configData.Element == ElementType::kKeywords)
				{
					if (!patchData.Keywords.has_value())
					{
						patchData.Keywords = PatchData::KeywordsData{};
					}

					for (const auto& operation : a_configData.Operations)
					{
						if (operation.OpType == OperationType::kClear)
						{
							patchData.Keywords->Clear = true;
						}
						else if (operation.OpType == OperationType::kAdd || operation.OpType == OperationType::kDelete)
						{
							const auto keywordFormStr = std::any_cast<std::string>(operation.OpData.value());

							auto* keywordForm = Utils::GetFormFromString(keywordFormStr);
							if (!keywordForm)
							{
								logger::warn("Invalid Form: '{}'.", keywordFormStr);
								return;
							}

							auto* keyword = keywordForm->As<RE::BGSKeyword>();
							if (!keyword)
							{
								logger::warn("'{}' is not a Keyword.", keywordFormStr);
								return;
							}

							if (operation.OpType == OperationType::kAdd)
							{
								patchData.Keywords->AddKeywordVec.emplace_back(keyword);
							}
							else
							{
								patchData.Keywords->DeleteKeywordVec.emplace_back(keyword);
							}
						}
					}
				}
				else if (a_configData.Element == ElementType::kObjectEffect)
				{
					const auto effectFormStr = std::any_cast<std::string>(a_configData.AssignValue.value());

					if (effectFormStr == "null")
					{
						patchData.ObjectEffect = nullptr;
					}
					else
					{
						auto* effectForm = Utils::GetFormFromString(effectFormStr);
						if (!effectForm)
						{
							logger::warn("Invalid Form: '{}'.", effectFormStr);
							return;
						}

						auto* objectEffect = effectForm->As<RE::EnchantmentItem>();
						if (!objectEffect)
						{
							logger::warn("'{}' is not an Object Effect.", effectFormStr);
							return;
						}

						patchData.ObjectEffect = objectEffect;
					}
				}
				else if (a_configData.Element == ElementType::kResistances)
				{
					if (!patchData.Resistances.has_value())
					{
						patchData.Resistances = PatchData::ResistancesData{};
					}

					for (const auto& operation : a_configData.Operations)
					{
						if (operation.OpType == OperationType::kClear)
						{
							patchData.Resistances->Clear = true;
						}
						else if (operation.OpType == OperationType::kAdd || operation.OpType == OperationType::kDelete)
						{
							const auto resistanceData = std::any_cast<ConfigData::Operation::ResistanceData>(operation.OpData.value());

							auto* form = Utils::GetFormFromString(resistanceData.Form);
							if (!form)
							{
								logger::warn("Invalid Form: '{}'.", resistanceData.Form);
								continue;
							}

							auto* damageType = form->As<RE::BGSDamageType>();
							if (!damageType)
							{
								logger::warn("'{}' is not a Damage Type.", resistanceData.Form);
								continue;
							}

							PatchData::ResistancesData::Resistance resistance{};
							resistance.DamageType = damageType;
							resistance.Value = resistanceData.Value;

							if (operation.OpType == OperationType::kAdd)
							{
								patchData.Resistances->AddResistanceVec.emplace_back(resistance);
							}
							else
							{
								patchData.Resistances->DeleteResistanceVec.emplace_back(resistance);
							}
						}
					}
				}
			}
		}

		void PatchKeywords(RE::TESObjectARMO* a_armo, const PatchData::KeywordsData& a_keywordsData)
		{
			bool cleared = false;

			if (a_keywordsData.Clear)
			{
				while (a_armo->numKeywords > 0)
				{
					a_armo->RemoveKeyword(a_armo->keywords[0]);
				}
				cleared = true;
			}

			// Delete
			if (!cleared)
			{
				for (const auto& keyword : a_keywordsData.DeleteKeywordVec)
				{
					if (a_armo->HasKeyword(keyword))
					{
						a_armo->RemoveKeyword(keyword);
					}
				}
			}

			// Add
			for (const auto& keyword : a_keywordsData.AddKeywordVec)
			{
				if (!a_armo->HasKeyword(keyword))
				{
					a_armo->AddKeyword(keyword);
				}
			}
		}

		void PatchResistances(RE::TESObjectARMO* a_armo, const PatchData::ResistancesData& a_resistancesData)
		{
			if (!a_armo->armorData.damageTypes)
			{
				using alloc_type = std::remove_pointer_t<decltype(a_armo->armorData.damageTypes)>;

				auto* storage = RE::malloc(sizeof(alloc_type));
				if (!storage)
				{
					logger::critical("Failed to allocate the Armor Resistances array.");
					return;
				}

				a_armo->armorData.damageTypes = ::new (storage) alloc_type();
			}

			// Clear
			if (a_resistancesData.Clear)
			{
				a_armo->armorData.damageTypes->clear();
			}

			// Delete
			for (const auto& resistance : a_resistancesData.DeleteResistanceVec)
			{
				const auto it = std::find_if(a_armo->armorData.damageTypes->begin(), a_armo->armorData.damageTypes->end(), [&resistance](const RE::BSTTuple<RE::TESForm*, RE::BGSTypedFormValuePair::SharedVal>& a_elem) {
					return a_elem.first == resistance.DamageType;
				});

				if (it != a_armo->armorData.damageTypes->end())
				{
					a_armo->armorData.damageTypes->erase(it);
				}
			}

			// Add
			for (const auto& resistance : a_resistancesData.AddResistanceVec)
			{
				const auto it = std::find_if(a_armo->armorData.damageTypes->begin(), a_armo->armorData.damageTypes->end(), [&resistance](const RE::BSTTuple<RE::TESForm*, RE::BGSTypedFormValuePair::SharedVal>& a_elem) {
					return a_elem.first == resistance.DamageType && a_elem.second.i == resistance.Value;
				});

				if (it == a_armo->armorData.damageTypes->end())
				{
					a_armo->armorData.damageTypes->emplace_back(resistance.DamageType, resistance.Value);
				}
			}
		}
	}  // namespace

	void ReadConfigs()
	{
		g_configVec = ConfigUtils::ReadConfigs<ArmorParser, Parsers::Statement<ConfigData>>(kTypeName);
	}

	void Patch()
	{
		logger::info("======================== Start preparing patch for {} ========================", kTypeName);

		ConfigUtils::Prepare(g_configVec, Prepare);

		logger::info("======================== Finished preparing patch for {} ========================", kTypeName);
		logger::info("");

		logger::info("======================== Start patching for {} ========================", kTypeName);

		for (const auto& [armor, patchData] : g_patchMap)
		{
			if (patchData.ArmorRating.has_value())
			{
				armor->armorData.rating = patchData.ArmorRating.value();
			}
			if (patchData.BipedObjectSlots.has_value())
			{
				armor->bipedModelData.bipedObjectSlots = patchData.BipedObjectSlots.value();
			}
			if (patchData.FullName.has_value())
			{
				armor->fullName = patchData.FullName.value();
			}
			if (patchData.Keywords.has_value())
			{
				PatchKeywords(armor, patchData.Keywords.value());
			}
			if (patchData.ObjectEffect.has_value())
			{
				armor->formEnchanting = patchData.ObjectEffect.value();
			}
			if (patchData.Resistances.has_value())
			{
				PatchResistances(armor, patchData.Resistances.value());
			}
		}

		logger::info("======================== Finished patching for {} ========================", kTypeName);
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}  // namespace Armors
