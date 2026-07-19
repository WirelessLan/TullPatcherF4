#include "CObjs.h"

#include <unordered_set>
#include <regex>
#include <any>

#include "ConfigUtils.h"
#include "Parsers.h"
#include "Utils.h"

namespace CObjs {
	constexpr std::string_view TypeName = "ConstructibleObject";

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
		kCategories,
		kComponents,
		kCreatedObject,
		kCreatedObjectCount,
		kWorkbenchKeyword
	};

	std::string_view ElementTypeToString(ElementType a_value) {
		switch (a_value) {
		case ElementType::kCategories: return "Categories";
		case ElementType::kComponents: return "Components";
		case ElementType::kCreatedObject: return "CreatedObject";
		case ElementType::kCreatedObjectCount: return "CreatedObjectCount";
		case ElementType::kWorkbenchKeyword: return "WorkbenchKeyword";
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
			struct ComponentData {
				std::string Form;
				std::uint32_t Count;
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
		struct CategoriesData {
			bool Clear = false;
			std::unordered_set<RE::BGSKeyword*> AddKeywordSet;
			std::unordered_set<RE::BGSKeyword*> DeleteKeywordSet;
		};

		struct ComponentsData {
			struct Component {
				RE::TESForm* Form;
				std::uint32_t Count;
			};

			bool Clear = false;
			std::vector<Component> AddComponentVec;
			std::vector<RE::TESForm*> DeleteComponentVec;
		};

		std::optional<CategoriesData> Categories;
		std::optional<ComponentsData> Components;
		std::optional<RE::TESForm*> CreatedObject;
		std::optional<std::uint16_t> CreatedObjectCount;
		std::optional<RE::BGSKeyword*> WorkbenchKeyword;
	};

	std::vector<Parsers::Statement<ConfigData>> g_configVec;
	std::unordered_map<RE::BGSKeyword*, std::uint16_t> g_keywordIndexMap;
	std::unordered_map<RE::BGSConstructibleObject*, PatchData> g_filterByFormIDPatchMap;
	std::unordered_map<std::uint16_t, PatchData> g_filterByCategoryKeywordPatchMap;

	class CObjParser : public Parsers::Parser<ConfigData> {
	public:
		CObjParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

	protected:
		std::optional<Parsers::Statement<ConfigData>> ParseExpressionStatement() override {
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
				do {
					token = reader.GetToken();
					if (token != ".") {
						logger::warn("Line {}, Col {}: Syntax error. Expected '.'.", reader.GetLastLine(), reader.GetLastLineIndex());
						return std::nullopt;
					}

					if (!ParseOperation(configData)) {
						return std::nullopt;
					}
				} while (reader.Peek() == ".");
			}

			token = reader.GetToken();
			if (token != ";") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ';'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			return Parsers::Statement<ConfigData>::CreateExpressionStatement(configData);
		}

		void PrintExpressionStatement(const ConfigData& a_configData, int a_indent) override {
			auto indent = std::string(a_indent * 4, ' ');

			switch (a_configData.Element) {
			case ElementType::kCategories:
				logger::info("{}{}({}).{}", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element));
				for (std::size_t opIndex = 0; opIndex < a_configData.Operations.size(); opIndex++) {
					std::string opLog;

					switch (a_configData.Operations[opIndex].OpType) {
					case OperationType::kClear:
						opLog = fmt::format(".{}()", OperationTypeToString(a_configData.Operations[opIndex].OpType));
						break;

					case OperationType::kAdd:
					case OperationType::kDelete:
						opLog = fmt::format(".{}({})", OperationTypeToString(a_configData.Operations[opIndex].OpType), std::any_cast<std::string>(a_configData.Operations[opIndex].OpData.value()));
						break;
					}

					if (opIndex == a_configData.Operations.size() - 1) {
						opLog += ";";
					}

					logger::info("{}    {}", indent, opLog);
				}
				break;

			case ElementType::kComponents:
				logger::info("{}{}({}).{}", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element));
				for (std::size_t opIndex = 0; opIndex < a_configData.Operations.size(); opIndex++) {
					std::string opLog;

					switch (a_configData.Operations[opIndex].OpType) {
					case OperationType::kClear:
						opLog = fmt::format(".{}()", OperationTypeToString(a_configData.Operations[opIndex].OpType));
						break;

					case OperationType::kAdd:
						opLog = fmt::format(".{}({}, {})", OperationTypeToString(a_configData.Operations[opIndex].OpType),
							std::any_cast<ConfigData::Operation::ComponentData>(a_configData.Operations[opIndex].OpData.value()).Form,
							std::any_cast<ConfigData::Operation::ComponentData>(a_configData.Operations[opIndex].OpData.value()).Count);
						break;

					case OperationType::kDelete:
						opLog = fmt::format(".{}({})", OperationTypeToString(a_configData.Operations[opIndex].OpType),
							std::any_cast<ConfigData::Operation::ComponentData>(a_configData.Operations[opIndex].OpData.value()).Form);
						break;
					}

					if (opIndex == a_configData.Operations.size() - 1) {
						opLog += ";";
					}

					logger::info("{}    {}", indent, opLog);
				}
				break;

			case ElementType::kCreatedObject:
			case ElementType::kWorkbenchKeyword:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element),
					std::any_cast<std::string>(a_configData.AssignValue.value()));
				break;

			case ElementType::kCreatedObjectCount:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element), 
					std::any_cast<std::uint16_t>(a_configData.AssignValue.value()));
				break;
			}
		}

		bool ParseFilter(ConfigData& a_configData) {
			auto token = reader.GetToken();
			if (token == "FilterByFormID") {
				a_configData.Filter = FilterType::kFormID;
			}
			else if (token == "FilterByCategoryKeyword") {
				a_configData.Filter = FilterType::kCategoryKeyword;
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

			const auto filterFormOpt = ParseForm();
			if (!filterFormOpt.has_value()) {
				return false;
			}

			a_configData.FilterForm = filterFormOpt.value();

			token = reader.GetToken();
			if (token != ")") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			return true;
		}

		bool ParseElement(ConfigData& a_configData) {
			auto token = reader.GetToken();
			if (token == "Categories") {
				a_configData.Element = ElementType::kCategories;
			}
			else if (token == "Components") {
				a_configData.Element = ElementType::kComponents;
			}
			else if (token == "CreatedObject") {
				a_configData.Element = ElementType::kCreatedObject;
			}
			else if (token == "CreatedObjectCount") {
				a_configData.Element = ElementType::kCreatedObjectCount;
			}
			else if (token == "WorkbenchKeyword") {
				a_configData.Element = ElementType::kWorkbenchKeyword;
			}
			else {
				logger::warn("Line {}, Col {}: Invalid ElementName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			return true;
		}

		bool ParseAssignment(ConfigData& a_configData) {
			auto token = reader.GetToken();
			if (token != "=") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '='.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			if (a_configData.Element == ElementType::kCreatedObject || a_configData.Element == ElementType::kWorkbenchKeyword) {
				token = reader.Peek();
				if (token == "null") {
					a_configData.AssignValue = std::any(std::string(reader.GetToken()));
					return true;
				}

				const auto formOpt = ParseForm();
				if (!formOpt.has_value()) {
					return false;
				}

				a_configData.AssignValue = std::any(formOpt.value());
			}
			else if (a_configData.Element == ElementType::kCreatedObjectCount) {
				const auto valueOpt = ParseNumber<std::uint16_t>();
				if (!valueOpt.has_value()) {
					return false;
				}

				a_configData.AssignValue = std::any(valueOpt.value());
			}
			else {
				logger::warn("Line {}, Col {}: Invalid Assignment for '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_configData.Element));
				return false;
			}

			return true;
		}

		bool ParseOperation(ConfigData& a_configData) {
			ConfigData::Operation newOp{};

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

			auto isValidOperation = [](ElementType elem, OperationType op) -> bool {
				if (elem == ElementType::kCategories) {
					return op == OperationType::kClear || op == OperationType::kAdd || op == OperationType::kDelete;
				}
				else if (elem == ElementType::kComponents) {
					return op == OperationType::kClear || op == OperationType::kAdd || op == OperationType::kDelete;
				}
				return false;
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

			switch (a_configData.Element) {
			case ElementType::kCategories:
				if (newOp.OpType != OperationType::kClear) {
					const auto formOpt = ParseForm();
					if (!formOpt.has_value()) {
						return false;
					}

					newOp.OpData = std::any(formOpt.value());
				}

				break;

			case ElementType::kComponents: {
				if (newOp.OpType != OperationType::kClear) {
					ConfigData::Operation::ComponentData opData{};

					const auto formOpt = ParseForm();
					if (!formOpt.has_value()) {
						return false;
					}
					opData.Form = formOpt.value();

					if (newOp.OpType == OperationType::kAdd) {
						token = reader.GetToken();
						if (token != ",") {
							logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
							return false;
						}

						const auto countOpt = ParseNumber<std::uint32_t>();
						if (!countOpt.has_value()) {
							return false;
						}
						opData.Count = countOpt.value();
					}

					newOp.OpData = std::any(opData);
				}

				break;
			}
			}

			token = reader.GetToken();
			if (token != ")") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			a_configData.Operations.emplace_back(newOp);

			return true;
		}
	};

	void ReadConfigs() {
		g_configVec = ConfigUtils::ReadConfigs<CObjParser, Parsers::Statement<ConfigData>>(TypeName);
	}

	void SetKeywordIndexMap() {
		const auto keywords = RE::BGSKeyword::GetTypedKeywords();
		if (keywords) {
			const auto& arr = (*keywords)[RE::stl::to_underlying(RE::KeywordType::kRecipeFilter)];
			for (std::uint16_t keywordIndex = 0; keywordIndex < arr.size(); keywordIndex++) {
				g_keywordIndexMap[arr[keywordIndex]] = keywordIndex;
			}
		}
	}

	void PreparePatchData(const ConfigData& a_configData, PatchData& a_patchData) {
		if (a_configData.Element == ElementType::kCategories) {
			if (!a_patchData.Categories.has_value()) {
				a_patchData.Categories = PatchData::CategoriesData{};
			}

			for (const auto& op : a_configData.Operations) {
				if (op.OpType == OperationType::kClear) {
					a_patchData.Categories->Clear = true;
				}
				else if (op.OpType == OperationType::kAdd || op.OpType == OperationType::kDelete) {
					const auto opForm = std::any_cast<std::string>(op.OpData.value());

					auto* keywordForm = Utils::GetFormFromString(opForm);
					if (!keywordForm) {
						logger::warn("Invalid KeywordForm: '{}'.", opForm);
						continue;
					}

					auto* keyword = keywordForm->As<RE::BGSKeyword>();
					if (!keyword) {
						logger::warn("'{}' is not a Keyword.", opForm);
						continue;
					}

					const auto it = g_keywordIndexMap.find(keyword);
					if (it == g_keywordIndexMap.end()) {
						logger::warn("'{}' is not a recipe filter keyword.", opForm);
						continue;
					}

					if (op.OpType == OperationType::kAdd) {
						a_patchData.Categories->AddKeywordSet.insert(keyword);
					}
					else {
						a_patchData.Categories->DeleteKeywordSet.insert(keyword);
					}
				}
			}
		}
		else if (a_configData.Element == ElementType::kComponents) {
			if (!a_patchData.Components.has_value()) {
				a_patchData.Components = PatchData::ComponentsData{};
			}

			for (const auto& op : a_configData.Operations) {
				if (op.OpType == OperationType::kClear) {
					a_patchData.Components->Clear = true;
				}
				else if (op.OpType == OperationType::kAdd || op.OpType == OperationType::kDelete) {
					const auto componentData = std::any_cast<ConfigData::Operation::ComponentData>(op.OpData.value());

					auto* form = Utils::GetFormFromString(componentData.Form);
					if (!form) {
						logger::warn("Invalid Form: '{}'.", componentData.Form);
						continue;
					}

					if (op.OpType == OperationType::kAdd) {
						a_patchData.Components->AddComponentVec.push_back({ form, componentData.Count });
					}
					else {
						a_patchData.Components->DeleteComponentVec.push_back(form);
					}
				}
			}
		}
		else if (a_configData.Element == ElementType::kCreatedObject) {
			const auto formStr = std::any_cast<std::string>(a_configData.AssignValue.value());

			if (formStr == "null") {
				a_patchData.CreatedObject = nullptr;
			}
			else {
				auto* form = Utils::GetFormFromString(formStr);
				if (!form) {
					logger::warn("Invalid Form: '{}'.", formStr);
					return;
				}

				a_patchData.CreatedObject = form;
			}
		}
		else if (a_configData.Element == ElementType::kCreatedObjectCount) {
			a_patchData.CreatedObjectCount = std::any_cast<std::uint16_t>(a_configData.AssignValue.value());
		}
		else if (a_configData.Element == ElementType::kWorkbenchKeyword) {
			const auto keywordFormStr = std::any_cast<std::string>(a_configData.AssignValue.value());

			if (keywordFormStr == "null") {
				a_patchData.WorkbenchKeyword = nullptr;
			}
			else {
				auto* keywordForm = Utils::GetFormFromString(keywordFormStr);
				if (!keywordForm) {
					logger::warn("Invalid Form: '{}'.", keywordFormStr);
					return;
				}

				auto* keyword = keywordForm->As<RE::BGSKeyword>();
				if (!keyword) {
					logger::warn("'{}' is not a Keyword.", keywordFormStr);
					return;
				}

				a_patchData.WorkbenchKeyword = keyword;
			}
		}
	}

	void PrepareFilterByFormID(const ConfigData& a_configData) {
		auto* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
		if (!filterForm) {
			logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
			return;
		}

		auto* cobjForm = filterForm->As<RE::BGSConstructibleObject>();
		if (!cobjForm) {
			logger::warn("'{}' is not a ConstructibleObject", a_configData.FilterForm);
			return;
		}

		auto& patchData = g_filterByFormIDPatchMap[cobjForm];
		PreparePatchData(a_configData, patchData);
	}

	void PrepareFilterByCategoryKeyword(const ConfigData& a_configData) {
		auto* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
		if (!filterForm) {
			logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
			return;
		}

		auto* keywordFilterForm = filterForm->As<RE::BGSKeyword>();
		if (!keywordFilterForm) {
			logger::warn("'{}' is not a Keyword.", a_configData.FilterForm);
			return;
		}

		const auto it = g_keywordIndexMap.find(keywordFilterForm);
		if (it == g_keywordIndexMap.end()) {
			logger::warn("'{}' is not a recipe filter keyword.", a_configData.FilterForm);
			return;
		}

		auto& patchData = g_filterByCategoryKeywordPatchMap[it->second];
		PreparePatchData(a_configData, patchData);
	}

	void Prepare(const ConfigData& a_configData) {
		if (a_configData.Filter == FilterType::kFormID) {
			PrepareFilterByFormID(a_configData);
		}
		else if (a_configData.Filter == FilterType::kCategoryKeyword) {
			PrepareFilterByCategoryKeyword(a_configData);
		}
	}

	std::unordered_set<std::uint16_t> GetCategoryKeywords(RE::BGSConstructibleObject* a_cobjForm) {
		std::unordered_set<std::uint16_t> keywords;

		if (!a_cobjForm || !a_cobjForm->filterKeywords.array || a_cobjForm->filterKeywords.size == 0) {
			return keywords;
		}

		for (std::uint32_t keywordIndex = 0; keywordIndex < a_cobjForm->filterKeywords.size; keywordIndex++) {
			if (a_cobjForm->filterKeywords.array[keywordIndex].keywordIndex == UINT16_MAX) {
				continue;
			}

			keywords.insert(a_cobjForm->filterKeywords.array[keywordIndex].keywordIndex);
		}

		return keywords;
	}

	void SetCategoryKeywords(RE::BGSConstructibleObject* a_cobjForm, std::unordered_set<std::uint16_t> a_keywordIndexSet) {
		if (!a_keywordIndexSet.empty() && (!a_cobjForm->filterKeywords.array || a_cobjForm->filterKeywords.size < a_keywordIndexSet.size())) {
			using alloc_type = std::remove_pointer_t<decltype(a_cobjForm->filterKeywords.array)>;

			auto* storage = RE::malloc(sizeof(alloc_type) * a_keywordIndexSet.size());
			if (!storage) {
				logger::critical("Failed to allocate the ConstructibleObject CategoryKeywords array.");
				return;
			}

			if (a_cobjForm->filterKeywords.array) {
				RE::free(a_cobjForm->filterKeywords.array);
			}

			a_cobjForm->filterKeywords.array = new (storage) alloc_type[a_keywordIndexSet.size()];
			a_cobjForm->filterKeywords.size = 0;
		}

		if (a_keywordIndexSet.empty()) {
			a_cobjForm->filterKeywords.size = 0;
			return;
		}

		std::uint32_t idx = 0;
		for (const auto keywordIndex : a_keywordIndexSet) {
			RE::BGSTypedKeywordValue<RE::KeywordType::kRecipeFilter> newValue{ keywordIndex };
			a_cobjForm->filterKeywords.array[idx] = newValue;
			idx++;
		}
		a_cobjForm->filterKeywords.size = idx;
	}

	void PatchCategories(RE::BGSConstructibleObject* a_cobjForm, const PatchData::CategoriesData& a_categoriesData) {
		bool isCleared = false, isDeleted = false, isAdded = false;
		std::unordered_set<std::uint16_t> categoryKeywordIndexSet;

		// Clear
		if (a_categoriesData.Clear) {
			isCleared = true;
		}
		else if (!a_categoriesData.AddKeywordSet.empty() || !a_categoriesData.DeleteKeywordSet.empty()) {
			categoryKeywordIndexSet = GetCategoryKeywords(a_cobjForm);
		}

		// Delete
		if (!isCleared && !a_categoriesData.DeleteKeywordSet.empty()) {
			std::size_t preSize = categoryKeywordIndexSet.size();

			for (const auto& delForm : a_categoriesData.DeleteKeywordSet) {
				auto keywordIndexMap_iter = g_keywordIndexMap.find(delForm);
				if (keywordIndexMap_iter == g_keywordIndexMap.end()) {
					continue;
				}

				categoryKeywordIndexSet.erase(keywordIndexMap_iter->second);
			}

			if (preSize != categoryKeywordIndexSet.size()) {
				isDeleted = true;
			}
		}

		if (!a_categoriesData.AddKeywordSet.empty()) {
			std::size_t preSize = categoryKeywordIndexSet.size();

			// Add
			for (const auto& addForm : a_categoriesData.AddKeywordSet) {
				auto keywordIndexMap_iter = g_keywordIndexMap.find(addForm);
				if (keywordIndexMap_iter == g_keywordIndexMap.end()) {
					continue;
				}

				categoryKeywordIndexSet.insert(keywordIndexMap_iter->second);
			}

			if (preSize != categoryKeywordIndexSet.size()) {
				isAdded = true;
			}
		}

		if (isCleared || isDeleted || isAdded) {
			SetCategoryKeywords(a_cobjForm, categoryKeywordIndexSet);
		}
	}

	void PatchComponents(RE::BGSConstructibleObject* a_cobjForm, const PatchData::ComponentsData& a_componentsData) {
		if (!a_cobjForm->requiredItems) {
			using alloc_type = std::remove_pointer_t<decltype(a_cobjForm->requiredItems)>;

			auto* storage = RE::malloc(sizeof(alloc_type));
			if (!storage) {
				logger::critical("Failed to allocate the ConstructibleObject RequiredItems array.");
				return;
			}

			a_cobjForm->requiredItems = new (storage) alloc_type();
		}

		// Clear
		if (a_componentsData.Clear) {
			a_cobjForm->requiredItems->clear();
		}

		// Delete
		for (const auto& delForm : a_componentsData.DeleteComponentVec) {
			for (auto it = a_cobjForm->requiredItems->begin(); it != a_cobjForm->requiredItems->end(); it++) {
				if (it->first == delForm) {
					a_cobjForm->requiredItems->erase(it);
					break;
				}
			}
		}

		// Add
		for (const auto& addComponent : a_componentsData.AddComponentVec) {
			bool found = false;

			for (auto it = a_cobjForm->requiredItems->begin(); it != a_cobjForm->requiredItems->end(); it++) {
				if (it->first == addComponent.Form) {
					found = true;
					it->second.i = addComponent.Count;
					break;
				}
			}

			if (!found) {
				a_cobjForm->requiredItems->push_back(RE::BSTTuple<RE::TESForm*, RE::BGSTypedFormValuePair::SharedVal>(addComponent.Form, addComponent.Count));
			}
		}
	}

	void Patch(RE::BGSConstructibleObject* a_cobjForm, const PatchData& a_patchData) {
		if (a_patchData.Categories.has_value()) {
			PatchCategories(a_cobjForm, a_patchData.Categories.value());
		}
		if (a_patchData.Components.has_value()) {
			PatchComponents(a_cobjForm, a_patchData.Components.value());
		}
		if (a_patchData.CreatedObject.has_value()) {
			a_cobjForm->createdItem = a_patchData.CreatedObject.value();
		}
		if (a_patchData.CreatedObjectCount.has_value()) {
			a_cobjForm->data.numConstructed = a_patchData.CreatedObjectCount.value();
		}
		if (a_patchData.WorkbenchKeyword.has_value()) {
			a_cobjForm->benchKeyword = a_patchData.WorkbenchKeyword.value();
		}
	}

	void PatchByFilters() {
		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) {
			return;
		}

		if (g_filterByFormIDPatchMap.empty() && g_filterByCategoryKeywordPatchMap.empty()) {
			return;
		}

		for (RE::TESForm* form : dataHandler->formArrays[RE::stl::to_underlying(RE::ENUM_FORM_ID::kCOBJ)]) {
			auto* cobjForm = form->As<RE::BGSConstructibleObject>();
			if (!cobjForm) {
				continue;
			}

			auto filterByFormID_iter = g_filterByFormIDPatchMap.find(cobjForm);
			if (filterByFormID_iter != g_filterByFormIDPatchMap.end()) {
				Patch(cobjForm, filterByFormID_iter->second);
			}

			if (!cobjForm->filterKeywords.array || cobjForm->filterKeywords.size == 0) {
				continue;
			}

			for (std::uint32_t keywordIndex = 0; keywordIndex < cobjForm->filterKeywords.size; keywordIndex++) {
				if (cobjForm->filterKeywords.array[keywordIndex].keywordIndex == UINT16_MAX) {
					continue;
				}

				auto filterByCategoryKeyword_iter = g_filterByCategoryKeywordPatchMap.find(cobjForm->filterKeywords.array[keywordIndex].keywordIndex);
				if (filterByCategoryKeyword_iter != g_filterByCategoryKeywordPatchMap.end()) {
					Patch(cobjForm, filterByCategoryKeyword_iter->second);
				}
			}
		}
	}

	void Patch() {
		logger::info("======================== Start preparing patch for {} ========================", TypeName);

		SetKeywordIndexMap();
		ConfigUtils::Prepare(g_configVec, Prepare);

		logger::info("======================== Finished preparing patch for {} ========================", TypeName);
		logger::info("");

		logger::info("======================== Start patching for {} ========================", TypeName);

		PatchByFilters();

		logger::info("======================== Finished patching for {} ========================", TypeName);
		logger::info("");

		g_configVec.clear();
		g_keywordIndexMap.clear();
		g_filterByFormIDPatchMap.clear();
		g_filterByCategoryKeywordPatchMap.clear();
	}
}
