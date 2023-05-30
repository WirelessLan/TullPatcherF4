#include "CObjs.h"

#include <unordered_set>
#include <regex>

#include "Configs.h"
#include "Utils.h"

namespace CObjs {
	enum class FilterType {
		kFormID,
		kCategoryKeyword
	};

	std::string_view FilterTypeToString(FilterType a_value) {
		switch (a_value) {
		case FilterType::kFormID: return "FilterByFormID";
		case FilterType::kCategoryKeyword: return "FilterByCategoryKeyword";
		default: return std::string_view{};
		}
	}

	enum class ElementType {
		kCategories
	};

	std::string_view ElementTypeToString(ElementType a_value) {
		switch (a_value) {
		case ElementType::kCategories: return "Categories";
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
		struct Data {
			struct CategoriesData {
				bool Clear = false;
				std::unordered_set<RE::BGSKeyword*> AddKeywordSet;
				std::unordered_set<RE::BGSKeyword*> DeleteKeywordSet;
			};

			std::optional<CategoriesData> Categories;
		};

		std::optional<Data> FilterByFormID;
		std::optional<Data> FilterByCategoryKeyword;
	};

	std::vector<ConfigData> g_configVec;
	std::unordered_map<RE::BGSConstructibleObject*, PatchData> g_patchMap;
	std::unordered_map<RE::BGSKeyword*, std::uint16_t> g_keywordIndexMap;
	std::unordered_map<std::uint16_t, std::vector<ConfigData::Operation>> g_cobjFilterMap;

	class CObjParser : public Configs::Parser<ConfigData> {
	public:
		CObjParser(Configs::ConfigReader& a_configReader) : Configs::Parser<ConfigData>(a_configReader) {}

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

			return configData;
		}

	private:
		bool parseFilter(ConfigData& a_configData) {
			auto token = reader.GetToken();
			if (token == "FilterByFormID")
				a_configData.Filter = FilterType::kFormID;
			else if (token == "FilterByCategoryKeyword")
				a_configData.Filter = FilterType::kCategoryKeyword;
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
			if (token == "Categories")
				a_configData.Element = ElementType::kCategories;
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

			if (opType == OperationType::kClear || opType == OperationType::kAdd || opType == OperationType::kDelete) {
				if (a_configData.Element != ElementType::kCategories) {
					logger::warn("Line {}, Col {}: Invalid Operation '{}.{}()'.", 
						reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_configData.Element), OperationTypeToString(opType));
					return false;
				}
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
			CObjParser parser(reader);
			auto configData = parser.Parse();
			if (!configData.has_value()) {
				parser.RecoverFromError();
				continue;
			}

			g_configVec.push_back(configData.value());

			logger::info("{}({}).{}", FilterTypeToString(configData->Filter), configData->FilterForm, ElementTypeToString(configData->Element));
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
		const std::filesystem::path configDir{ "Data\\" + std::string(Version::PROJECT) + "\\ConstructibleObject" };
		const std::regex filter(".*\\.cfg", std::regex_constants::icase);
		const std::filesystem::directory_iterator dir_iter(configDir);
		for (auto& iter : dir_iter) {
			if (!std::filesystem::is_regular_file(iter.status()))
				continue;

			if (!std::regex_match(iter.path().filename().string(), filter))
				continue;

			std::string path = iter.path().string();
			logger::info("=========== Reading ConstructibleObject config file: {} ===========", path);
			readConfig(path);
			logger::info("");
		}
	}

	void SetKeywordIndexMap() {
		RE::KeywordType type = RE::KeywordType::kRecipeFilter;
		const auto keywords = RE::BGSKeyword::GetTypedKeywords();
		if (keywords) {
			const auto& arr = (*keywords)[RE::stl::to_underlying(type)];
			for (uint16_t ii = 0; ii < arr.size(); ii++)
				g_keywordIndexMap[arr[ii]] = ii;
		}
	}

	void PrepareFilterByFormID(const ConfigData& a_configData) {
		RE::TESForm* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
		if (!filterForm) {
			logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
			return;
		}

		RE::BGSConstructibleObject* cobjForm = filterForm->As<RE::BGSConstructibleObject>();
		if (!cobjForm) {
			logger::warn("'{}' is not a ConstructibleObject", a_configData.FilterForm);
			return;
		}

		PatchData& patchData = g_patchMap[cobjForm];

		if (!patchData.FilterByFormID.has_value())
			patchData.FilterByFormID = PatchData::Data{};

		if (a_configData.Element == ElementType::kCategories) {
			if (!patchData.FilterByFormID->Categories.has_value())
				patchData.FilterByFormID->Categories = PatchData::Data::CategoriesData{};

			for (const auto& op : a_configData.Operations) {
				if (op.OpType == OperationType::kClear) {
					patchData.FilterByFormID->Categories->Clear = true;
				}
				else if (op.OpType == OperationType::kAdd || op.OpType == OperationType::kDelete) {
					RE::TESForm* keywordForm = Utils::GetFormFromString(op.OpForm.value());
					if (!keywordForm) {
						logger::warn("Invalid KeywordForm: '{}'.", op.OpForm.value());
						continue;
					}

					RE::BGSKeyword* keyword = keywordForm->As<RE::BGSKeyword>();
					if (!keyword) {
						logger::warn("'{}' is not a Keyword.", op.OpForm.value());
						continue;
					}

					auto keywordIndexMap_iter = g_keywordIndexMap.find(keyword);
					if (keywordIndexMap_iter == g_keywordIndexMap.end()) {
						logger::warn("'{}' is not a recipe filter keyword.", op.OpForm.value());
						continue;
					}

					if (op.OpType == OperationType::kAdd)
						patchData.FilterByFormID->Categories->AddKeywordSet.insert(keyword);
					else
						patchData.FilterByFormID->Categories->DeleteKeywordSet.insert(keyword);
				}
			}
		}
	}

	void PrepareFilterByCategoryKeyword(const ConfigData& a_configData) {
		if (a_configData.Element == ElementType::kCategories) {
			RE::TESForm* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
			if (!filterForm) {
				logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
				return;
			}

			RE::BGSKeyword* keywordFilterForm = filterForm->As<RE::BGSKeyword>();
			if (!keywordFilterForm) {
				logger::warn("'{}' is not a Keyword.", a_configData.FilterForm);
				return;
			}

			auto keywordIndexMap_iter = g_keywordIndexMap.find(keywordFilterForm);
			if (keywordIndexMap_iter == g_keywordIndexMap.end()) {
				logger::warn("'{}' is not a recipe filter keyword.", a_configData.FilterForm);
				return;
			}

			g_cobjFilterMap[keywordIndexMap_iter->second] = a_configData.Operations;
		}
	}

	void Prepare(const std::vector<ConfigData>& a_configVec) {
		logger::info("======================== Start preparing patch for ConstructibleObject ========================");

		SetKeywordIndexMap();

		for (const auto& configData : a_configVec) {
			if (configData.Filter == FilterType::kFormID)
				PrepareFilterByFormID(configData);
			else if (configData.Filter == FilterType::kCategoryKeyword)
				PrepareFilterByCategoryKeyword(configData);
		}

		RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();
		if (dataHandler && !g_cobjFilterMap.empty()) {
			for (RE::TESForm* form : dataHandler->formArrays[RE::stl::to_underlying(RE::ENUM_FORM_ID::kCOBJ)]) {
				RE::BGSConstructibleObject* cobjForm = form->As<RE::BGSConstructibleObject>();
				if (!cobjForm || cobjForm->filterKeywords.size == 0 || !cobjForm->filterKeywords.array)
					continue;

				for (std::uint32_t ii = 0; ii < cobjForm->filterKeywords.size; ii++) {
					if (cobjForm->filterKeywords.array[ii].keywordIndex == 0xFFFF)
						continue;

					auto cobjFilterMap_iter = g_cobjFilterMap.find(cobjForm->filterKeywords.array[ii].keywordIndex);
					if (cobjFilterMap_iter == g_cobjFilterMap.end())
						continue;

					PatchData& patchData = g_patchMap[cobjForm];

					if (!patchData.FilterByCategoryKeyword.has_value())
						patchData.FilterByCategoryKeyword = PatchData::Data{};

					if (!patchData.FilterByCategoryKeyword->Categories.has_value())
						patchData.FilterByCategoryKeyword->Categories = PatchData::Data::CategoriesData{};

					for (const auto& op : cobjFilterMap_iter->second) {
						if (op.OpType == OperationType::kClear) {
							patchData.FilterByCategoryKeyword->Categories->Clear = true;
						}
						else if (op.OpType == OperationType::kAdd || op.OpType == OperationType::kDelete) {
							RE::TESForm* keywordForm = Utils::GetFormFromString(op.OpForm.value());
							if (!keywordForm) {
								logger::warn("Invalid KeywordForm: '{}'.", op.OpForm.value());
								continue;
							}

							RE::BGSKeyword* keyword = keywordForm->As<RE::BGSKeyword>();
							if (!keyword) {
								logger::warn("'{}' is not a Keyword.", op.OpForm.value());
								continue;
							}

							auto keywordIndexMap_iter = g_keywordIndexMap.find(keyword);
							if (keywordIndexMap_iter == g_keywordIndexMap.end()) {
								logger::warn("'{}' is not a recipe filter keyword.", op.OpForm.value());
								continue;
							}

							if (op.OpType == OperationType::kAdd)
								patchData.FilterByCategoryKeyword->Categories->AddKeywordSet.insert(keyword);
							else
								patchData.FilterByCategoryKeyword->Categories->DeleteKeywordSet.insert(keyword);
						}
					}
				}
			}
		}

		logger::info("======================== Finished preparing patch for ConstructibleObject ========================");
		logger::info("");
	}


	std::unordered_set<std::uint16_t> GetCategoryKeywords(RE::BGSConstructibleObject* a_cobjForm) {
		std::unordered_set<std::uint16_t> retVec;

		if (!a_cobjForm || !a_cobjForm->filterKeywords.array || a_cobjForm->filterKeywords.size == 0)
			return retVec;

		for (std::uint32_t ii = 0; ii < a_cobjForm->filterKeywords.size; ii++) {
			if (a_cobjForm->filterKeywords.array[ii].keywordIndex == 0xFFFF)
				continue;

			retVec.insert(a_cobjForm->filterKeywords.array[ii].keywordIndex);
		}

		return retVec;
	}

	void SetCategoryKeywords(RE::BGSConstructibleObject* a_cobjForm, std::unordered_set<std::uint16_t> a_keywordIndexSet) {
		if (!a_cobjForm)
			return;

		if (!a_keywordIndexSet.empty() && (!a_cobjForm->filterKeywords.array || a_cobjForm->filterKeywords.size < a_keywordIndexSet.size())) {
			RE::MemoryManager& mm = RE::MemoryManager::GetSingleton();

			std::size_t newSize = sizeof(std::uint16_t) * a_keywordIndexSet.size();
			RE::BGSTypedKeywordValue<RE::BGSKeyword::KeywordType::kRecipeFilter>* newArray
				= (RE::BGSTypedKeywordValue<RE::BGSKeyword::KeywordType::kRecipeFilter>*)mm.Allocate(newSize, 0, false);
			if (!newArray) {
				logger::critical("Failed to allocate the new Category Keywords array.");
				return;
			}

			if (a_cobjForm->filterKeywords.array)
				mm.Deallocate(a_cobjForm->filterKeywords.array, false);

			a_cobjForm->filterKeywords.array = newArray;
			a_cobjForm->filterKeywords.size = 0;
		}

		if (a_keywordIndexSet.empty()) {
			a_cobjForm->filterKeywords.size = 0;
			return;
		}

		std::uint32_t ii = 0;
		for (const auto keywordIndex : a_keywordIndexSet) {
			RE::BGSTypedKeywordValue<RE::BGSKeyword::KeywordType::kRecipeFilter> newValue{ keywordIndex };
			a_cobjForm->filterKeywords.array[ii] = newValue;
			ii++;
		}

		a_cobjForm->filterKeywords.size = ii;
	}

	void PatchCategories(RE::BGSConstructibleObject* a_cobjForm, const PatchData::Data::CategoriesData& a_categoriesData) {
		bool isCleared = false, isDeleted = false, isAdded = false;

		std::unordered_set<std::uint16_t> categoryKeywordIndexSet;

		// Clear
		if (a_categoriesData.Clear)
			isCleared = true;
		else if (!a_categoriesData.AddKeywordSet.empty() || !a_categoriesData.DeleteKeywordSet.empty())
			categoryKeywordIndexSet = GetCategoryKeywords(a_cobjForm);

		// Delete
		if (!isCleared && !a_categoriesData.DeleteKeywordSet.empty()) {
			std::size_t preSize = categoryKeywordIndexSet.size();

			for (const auto& delForm : a_categoriesData.DeleteKeywordSet) {
				auto keywordIndexMap_iter = g_keywordIndexMap.find(delForm);
				if (keywordIndexMap_iter == g_keywordIndexMap.end())
					continue;

				categoryKeywordIndexSet.erase(keywordIndexMap_iter->second);
			}

			if (preSize != categoryKeywordIndexSet.size())
				isDeleted = true;
		}

		if (!a_categoriesData.AddKeywordSet.empty()) {
			std::size_t preSize = categoryKeywordIndexSet.size();

			// Add
			for (const auto& addForm : a_categoriesData.AddKeywordSet) {
				auto keywordIndexMap_iter = g_keywordIndexMap.find(addForm);
				if (keywordIndexMap_iter == g_keywordIndexMap.end())
					continue;

				categoryKeywordIndexSet.insert(keywordIndexMap_iter->second);
			}

			if (preSize != categoryKeywordIndexSet.size())
				isAdded = true;
		}

		if (isCleared || isDeleted || isAdded)
			SetCategoryKeywords(a_cobjForm, categoryKeywordIndexSet);
	}

	void Patch(RE::BGSConstructibleObject* a_cobjForm, const PatchData& a_patchData) {
		const PatchData::Data* filterData = nullptr;
		if (a_patchData.FilterByFormID.has_value())
			filterData = &a_patchData.FilterByFormID.value();
		else if (a_patchData.FilterByCategoryKeyword.has_value())
			filterData = &a_patchData.FilterByCategoryKeyword.value();
		else
			return;

		if (filterData->Categories.has_value())
			PatchCategories(a_cobjForm, filterData->Categories.value());
	}

	void Patch() {
		Prepare(g_configVec);

		logger::info("======================== Start patching for ConstructibleObject ========================");

		for (const auto& patchData : g_patchMap)
			Patch(patchData.first, patchData.second);

		logger::info("======================== Finished patching for ConstructibleObject ========================");
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
		g_keywordIndexMap.clear();
		g_cobjFilterMap.clear();
	}
}
