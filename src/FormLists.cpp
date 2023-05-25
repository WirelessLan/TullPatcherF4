#include "FormLists.h"

#include <regex>

#include "Configs.h"
#include "Utils.h"

namespace FormLists {
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
			std::vector<RE::TESForm*> DeleteFormVec;
		};

		std::optional<ListData> List;
	};

	std::vector<ConfigData> g_configVec;
	std::unordered_map<RE::BGSListForm*, PatchData> g_patchMap;

	class FormListParser : public Configs::Parser<ConfigData> {
	public:
		FormListParser(Configs::ConfigReader& a_configReader) : Configs::Parser<ConfigData>(a_configReader) {}

		std::optional<ConfigData> Parse() override {
			if (reader.EndOfFile() || reader.Lookup().empty())
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

			token = reader.GetToken();
			if (token != ".") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '.'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			if (!parseOperation(configData))
				return std::nullopt;

			while (true) {
				token = reader.Lookup();
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

			return configData;
		}

	private:
		bool parseFilter(ConfigData& a_configData) {
			auto token = reader.GetToken();
			if (token == "FilterByFormID")
				a_configData.Filter = FilterType::kFormID;
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

			a_configData.FilterForm = filterForm.value();

			token = reader.GetToken();
			if (token != ")") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			return true;
		}

		bool parseElement(ConfigData& a_configData) {
			auto token = reader.GetToken();
			if (token == "List")
				a_configData.Element = ElementType::kList;
			else {
				logger::warn("Line {}, Col {}: Invalid ElementName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			return true;
		}

		bool parseOperation(ConfigData& a_configData) {
			OperationType opType;

			auto token = reader.GetToken();
			if (token == "Clear")
				opType = OperationType::kClear;
			else if (token == "Add")
				opType = OperationType::kAdd;
			else if (token == "Delete")
				opType = OperationType::kDelete;
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
				opData = parseForm();
				if (!opData.has_value())
					return false;
			}

			token = reader.GetToken();
			if (token != ")") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			a_configData.Operations.push_back({ opType, opData });

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

	void readConfig(std::string_view a_path) {
		Configs::ConfigReader reader(a_path);

		while (!reader.EndOfFile()) {
			FormListParser parser(reader);
			auto configData = parser.Parse();
			if (!configData.has_value()) {
				parser.RecoverFromError();
				continue;
			}

			g_configVec.push_back(configData.value());

			logger::info("{}({})", FilterTypeToString(configData->Filter), configData->FilterForm);
			for (std::size_t ii = 0; ii < configData->Operations.size(); ii++) {
				if (ii < configData->Operations.size() - 1)
					logger::info("    .{}({})", OperationTypeToString(configData->Operations[ii].OpType),
						configData->Operations[ii].OpForm.has_value() ? configData->Operations[ii].OpForm.value() : ""sv);
				else
					logger::info("    .{}({});", OperationTypeToString(configData->Operations[ii].OpType),
						configData->Operations[ii].OpForm.has_value() ? configData->Operations[ii].OpForm.value() : ""sv);
			}
		}
	}

	void ReadConfigs() {
		const std::filesystem::path configDir{ "Data\\" + std::string(Version::PROJECT) + "\\FormList" };
		const std::regex filter(".*\\.cfg", std::regex_constants::icase);
		const std::filesystem::directory_iterator dir_iter(configDir);
		for (auto& iter : dir_iter) {
			if (!std::filesystem::is_regular_file(iter.status()))
				continue;

			if (!std::regex_match(iter.path().filename().string(), filter))
				continue;

			std::string path = iter.path().string();
			logger::info("=========== Reading FormList config file: {} ===========", path);
			readConfig(path);
			logger::info("");
		}
	}

	void Prepare(const std::vector<ConfigData>& a_configVec) {
		logger::info("======================== Start preparing patch for FormList ========================");

		for (const auto& configData : a_configVec) {
			if (configData.Filter == FilterType::kFormID) {
				RE::TESForm* filterForm = Utils::GetFormFromString(configData.FilterForm);
				if (!filterForm) {
					logger::warn("Invalid FilterForm: '{}'.", configData.FilterForm);
					continue;
				}

				RE::BGSListForm* formList = filterForm->As<RE::BGSListForm>();
				if (!formList) {
					logger::warn("'{}' is not a FormList.", configData.FilterForm);
					continue;
				}

				PatchData& patchData = g_patchMap[formList];

				if (configData.Element == ElementType::kList) {
					if (!patchData.List.has_value())
						patchData.List = PatchData::ListData{};

					for (const auto& op : configData.Operations) {
						if (op.OpType == OperationType::kClear) {
							patchData.List->Clear = true;
						}
						else if (op.OpType == OperationType::kAdd || op.OpType == OperationType::kDelete) {
							RE::TESForm* opForm = Utils::GetFormFromString(op.OpForm.value());
							if (!opForm) {
								logger::warn("Invalid Form: '{}'.", op.OpForm.value());
								continue;
							}

							if (op.OpType == OperationType::kAdd)
								patchData.List->AddFormVec.push_back(opForm);
							else
								patchData.List->DeleteFormVec.push_back(opForm);
						}
					}
				}
			}
		}

		logger::info("======================== Finished preparing patch for FormList ========================");
		logger::info("");
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
					if (*it != delForm)
						continue;

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
	}

	void Patch(RE::BGSListForm* a_formList, const PatchData& a_patchData) {
		if (a_patchData.List.has_value())
			PatchList(a_formList, a_patchData.List.value());
	}

	void Patch() {
		Prepare(g_configVec);

		logger::info("======================== Start patching for FormList ========================");

		for (const auto& patchData : g_patchMap)
			Patch(patchData.first, patchData.second);

		logger::info("======================== Finished patching for FormList ========================");
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}
