#include "FormLists.h"

#include <regex>
#include <unordered_set>

#include "ConfigUtils.h"
#include "Parsers.h"
#include "Utils.h"

namespace FormLists
{
	namespace
	{
		constexpr std::string_view kTypeName = "FormList";

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
			kList
		};

		std::string_view ElementTypeToString(ElementType a_value)
		{
			switch (a_value)
			{
			case ElementType::kList:
				return "List";
			default:
				return std::string_view{};
			}
		}

		enum class OperationType
		{
			kClear,
			kAdd,
			kAddIfNotExists,
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
			case OperationType::kAddIfNotExists:
				return "AddIfNotExists";
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
				OperationType OpType;
				std::optional<std::string> OpForm;
			};

			FilterType Filter;
			std::string FilterForm;
			ElementType Element;
			std::vector<Operation> Operations;
		};

		struct PatchData
		{
			struct ListData
			{
				bool Clear = false;
				std::vector<RE::TESForm*> AddFormVec;
				std::unordered_set<RE::TESForm*> AddUniqueFormSet;
				std::vector<RE::TESForm*> DeleteFormVec;
			};

			std::optional<ListData> List;
		};

		std::vector<Parsers::Statement<ConfigData>> g_configVec;
		std::unordered_map<RE::BGSListForm*, PatchData> g_patchMap;

		class FormListParser : public Parsers::Parser<ConfigData>
		{
		public:
			FormListParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

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
				case ElementType::kList:
					logger::info("{}{}({}).{}", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element));
					for (std::size_t opIndex = 0; opIndex < a_configData.Operations.size(); ++opIndex)
					{
						auto opLog = fmt::format(".{}({})", OperationTypeToString(a_configData.Operations[opIndex].OpType),
							a_configData.Operations[opIndex].OpForm.has_value() ? a_configData.Operations[opIndex].OpForm.value() : "");

						if (opIndex == a_configData.Operations.size() - 1)
						{
							opLog += ";";
						}

						logger::info("{}    {}", indent, opLog);
					}
					break;
				}
			}

			bool ParseFilter(ConfigData& a_configData)
			{
				auto token = reader.GetToken();
				if (token == "FilterByFormID")
				{
					a_configData.Filter = FilterType::kFormID;
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

				a_configData.FilterForm = filterFormOpt.value();

				token = reader.GetToken();
				if (token != ")")
				{
					logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}

				return true;
			}

			bool ParseElement(ConfigData& a_configData)
			{
				const auto token = reader.GetToken();
				if (token == "List")
				{
					a_configData.Element = ElementType::kList;
				}
				else
				{
					logger::warn("Line {}, Col {}: Invalid ElementName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
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
				else if (token == "AddIfNotExists")
				{
					newOp.OpType = OperationType::kAddIfNotExists;
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
					if (elem == ElementType::kList)
					{
						return op == OperationType::kClear || op == OperationType::kAdd || op == OperationType::kAddIfNotExists || op == OperationType::kDelete;
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

				if (a_configData.Element == ElementType::kList)
				{
					if (newOp.OpType != OperationType::kClear)
					{
						const auto formOpt = ParseForm();
						if (!formOpt.has_value())
						{
							return false;
						}
						newOp.OpForm = formOpt.value();
					}
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

				auto* formList = filterForm->As<RE::BGSListForm>();
				if (!formList)
				{
					logger::warn("'{}' is not a FormList.", a_configData.FilterForm);
					return;
				}

				auto& patchData = g_patchMap[formList];

				if (a_configData.Element == ElementType::kList)
				{
					if (!patchData.List.has_value())
					{
						patchData.List = PatchData::ListData{};
					}

					for (const auto& op : a_configData.Operations)
					{
						if (op.OpType == OperationType::kClear)
						{
							patchData.List->Clear = true;
						}
						else if (op.OpType == OperationType::kAdd || op.OpType == OperationType::kAddIfNotExists || op.OpType == OperationType::kDelete)
						{
							auto* opForm = Utils::GetFormFromString(op.OpForm.value());
							if (!opForm)
							{
								logger::warn("Invalid Form: '{}'.", op.OpForm.value());
								continue;
							}

							if (op.OpType == OperationType::kAdd)
							{
								patchData.List->AddFormVec.emplace_back(opForm);
							}
							else if (op.OpType == OperationType::kAddIfNotExists)
							{
								patchData.List->AddUniqueFormSet.insert(opForm);
							}
							else
							{
								patchData.List->DeleteFormVec.emplace_back(opForm);
							}
						}
					}
				}
			}
		}

		void PatchList(RE::BGSListForm* a_formList, const PatchData::ListData& a_listData)
		{
			bool cleared = false;

			// Clear
			if (a_listData.Clear)
			{
				a_formList->arrayOfForms.clear();
				cleared = true;
			}

			// Delete
			if (!cleared)
			{
				for (const auto& delForm : a_listData.DeleteFormVec)
				{
					for (auto it = a_formList->arrayOfForms.begin(); it != a_formList->arrayOfForms.end(); ++it)
					{
						if (*it != delForm)
						{
							continue;
						}

						a_formList->arrayOfForms.erase(it);
						break;
					}
				}
			}

			// Add
			for (const auto& addForm : a_listData.AddFormVec)
			{
				a_formList->arrayOfForms.emplace_back(addForm);
			}

			// Add if not exists
			for (const auto& addForm : a_listData.AddUniqueFormSet)
			{
				const auto it = std::find(a_formList->arrayOfForms.begin(), a_formList->arrayOfForms.end(), addForm);
				if (it == a_formList->arrayOfForms.end())
				{
					a_formList->arrayOfForms.emplace_back(addForm);
				}
			}
		}
	}  // namespace

	void ReadConfigs()
	{
		g_configVec = ConfigUtils::ReadConfigs<FormListParser, Parsers::Statement<ConfigData>>(kTypeName);
	}

	void Patch()
	{
		logger::info("======================== Start preparing patch for {} ========================", kTypeName);

		ConfigUtils::Prepare(g_configVec, Prepare);

		logger::info("======================== Finished preparing patch for {} ========================", kTypeName);
		logger::info("");

		logger::info("======================== Start patching for {} ========================", kTypeName);

		for (const auto& [formList, patchData] : g_patchMap)
		{
			if (patchData.List.has_value())
			{
				PatchList(formList, patchData.List.value());
			}
		}

		logger::info("======================== Finished patching for {} ========================", kTypeName);
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}  // namespace FormLists
