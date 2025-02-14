#include "FormLists.h"

#include <unordered_set>
#include <regex>

#include "ConfigUtils.h"
#include "Parsers.h"
#include "Utils.h"

namespace FormLists {
	constexpr std::string_view TypeName = "FormList";

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
		kList
	};

	std::string_view ElementTypeToString(ElementType a_value) {
		switch (a_value) {
		case ElementType::kList: return "List";
		default: return std::string_view{};
		}
	}

	enum class OperationType {
		kClear,
		kAdd,
		kAddIfNotExists,
		kDelete
	};

	std::string_view OperationTypeToString(OperationType a_value) {
		switch (a_value) {
		case OperationType::kClear: return "Clear";
		case OperationType::kAdd: return "Add";
		case OperationType::kAddIfNotExists: return "AddIfNotExists";
		case OperationType::kDelete: return "Delete";
		default: return std::string_view{};
		}
	}

	struct ConfigData {
		struct Operation {
			OperationType OpType;
			std::optional<std::string> OpForm;
		};

		FilterType Filter;
		std::string FilterForm;
		ElementType Element;
		std::vector<Operation> Operations;
	};

	struct PatchData {
		struct ListData {
			bool Clear = false;
			std::vector<RE::TESForm*> AddFormVec;
			std::unordered_set<RE::TESForm*> AddUniqueFormSet;
			std::vector<RE::TESForm*> DeleteFormVec;
		};

		std::optional<ListData> List;
	};

	std::vector<Parsers::Statement<ConfigData>> g_configVec;
	std::unordered_map<RE::BGSListForm*, PatchData> g_patchMap;

	class FormListParser : public Parsers::Parser<ConfigData> {
	public:
		FormListParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

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
			case ElementType::kList:
				logger::info("{}{}({}).{}", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element));
				for (std::size_t ii = 0; ii < a_configData.Operations.size(); ii++) {
					std::string opLog = fmt::format(".{}({})", OperationTypeToString(a_configData.Operations[ii].OpType),
						a_configData.Operations[ii].OpForm.has_value() ? a_configData.Operations[ii].OpForm.value() : "");

					if (ii == a_configData.Operations.size() - 1) {
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
			if (token == "List") {
				a_configData.Element = ElementType::kList;
			}
			else {
				logger::warn("Line {}, Col {}: Invalid ElementName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			return true;
		}

		bool ParseOperation(ConfigData& a_configData) {
			OperationType opType;

			auto token = reader.GetToken();
			if (token == "Clear") {
				opType = OperationType::kClear;
			}
			else if (token == "Add") {
				opType = OperationType::kAdd;
			}
			else if (token == "AddIfNotExists") {
				opType = OperationType::kAddIfNotExists;
			}
			else if (token == "Delete") {
				opType = OperationType::kDelete;
			}
			else {
				logger::warn("Line {}, Col {}: Invalid OperationName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			token = reader.GetToken();
			if (token != "(") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '('.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			std::optional<std::string> opData;
			if (opType != OperationType::kClear) {
				opData = ParseForm();
				if (!opData.has_value()) {
					return false;
				}
			}

			token = reader.GetToken();
			if (token != ")") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			a_configData.Operations.push_back({ opType, opData });

			return true;
		}
	};

	void ReadConfigs() {
		g_configVec = ConfigUtils::ReadConfigs<FormListParser, Parsers::Statement<ConfigData>>(TypeName);
	}

	void Prepare(const ConfigData& a_configData) {
		if (a_configData.Filter == FilterType::kFormID) {
			RE::TESForm* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
			if (!filterForm) {
				logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
				return;
			}

			RE::BGSListForm* formList = filterForm->As<RE::BGSListForm>();
			if (!formList) {
				logger::warn("'{}' is not a FormList.", a_configData.FilterForm);
				return;
			}

			PatchData& patchData = g_patchMap[formList];

			if (a_configData.Element == ElementType::kList) {
				if (!patchData.List.has_value()) {
					patchData.List = PatchData::ListData{};
				}

				for (const auto& op : a_configData.Operations) {
					if (op.OpType == OperationType::kClear) {
						patchData.List->Clear = true;
					}
					else if (op.OpType == OperationType::kAdd || op.OpType == OperationType::kAddIfNotExists || op.OpType == OperationType::kDelete) {
						RE::TESForm* opForm = Utils::GetFormFromString(op.OpForm.value());
						if (!opForm) {
							logger::warn("Invalid Form: '{}'.", op.OpForm.value());
							continue;
						}

						if (op.OpType == OperationType::kAdd) {
							patchData.List->AddFormVec.push_back(opForm);
						}
						else if (op.OpType == OperationType::kAddIfNotExists) {
							patchData.List->AddUniqueFormSet.insert(opForm);
						}
						else {
							patchData.List->DeleteFormVec.push_back(opForm);
						}
					}
				}
			}
		}
	}

	void PatchList(RE::BGSListForm* a_formList, const PatchData::ListData& a_listData) {
		bool isCleared = false;

		// Clear
		if (a_listData.Clear) {
			a_formList->arrayOfForms.clear();
			isCleared = true;
		}

		// Delete
		if (!isCleared && !a_listData.DeleteFormVec.empty()) {
			for (const auto& delForm : a_listData.DeleteFormVec) {
				for (auto it = a_formList->arrayOfForms.begin(); it != a_formList->arrayOfForms.end(); it++) {
					if (*it != delForm) {
						continue;
					}

					a_formList->arrayOfForms.erase(it);
					break;
				}
			}
		}

		// Add
		if (!a_listData.AddFormVec.empty()) {
			for (const auto& addForm : a_listData.AddFormVec) {
				a_formList->arrayOfForms.push_back(addForm);
			}
		}

		// Add if not exists
		if (!a_listData.AddUniqueFormSet.empty()) {
			for (const auto& addForm : a_listData.AddUniqueFormSet) {
				if (std::find(a_formList->arrayOfForms.begin(), a_formList->arrayOfForms.end(), addForm) == a_formList->arrayOfForms.end()) {
					a_formList->arrayOfForms.push_back(addForm);
				}
			}
		}
	}

	void Patch(RE::BGSListForm* a_formList, const PatchData& a_patchData) {
		if (a_patchData.List.has_value()) {
			PatchList(a_formList, a_patchData.List.value());
		}
	}

	void Patch() {
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
