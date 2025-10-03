#include "Containers.h"

#include <regex>

#include "ConfigUtils.h"
#include "Parsers.h"
#include "Utils.h"

namespace Containers {
	constexpr std::string_view TypeName = "Container";

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
		kFullName,
		kItems
	};

	std::string_view ElementTypeToString(ElementType a_value) {
		switch (a_value) {
		case ElementType::kFullName: return "FullName";
		case ElementType::kItems: return "Items";
		default: return std::string_view{};
		}
	}

	enum class OperationType {
		kClear,
		kAdd,
		kDelete,
		kDeleteAll
	};

	std::string_view OperationTypeToString(OperationType a_value) {
		switch (a_value) {
		case OperationType::kClear: return "Clear";
		case OperationType::kAdd: return "Add";
		case OperationType::kDelete: return "Delete";
		case OperationType::kDeleteAll: return "DeleteAll";
		default: return std::string_view{};
		}
	}

	struct ConfigData {
		struct Operation {
			struct Data {
				std::string Form;
				std::uint32_t Count;
			};

			OperationType OpType;
			std::optional<Data> OpData;
		};

		FilterType Filter;
		std::string FilterForm;
		ElementType Element;
		std::optional<std::string> AssignValue;
		std::vector<Operation> Operations;
	};

	struct PatchData {
		struct ItemsData {
			struct Item {
				RE::TESBoundObject* Form;
				std::uint32_t Count;
			};

			bool Clear;
			std::vector<Item> AddObjectVec;
			std::vector<RE::TESBoundObject*> DeleteObjectVec;
			std::vector<RE::TESBoundObject*> DeleteAllObjectVec;
		};

		std::optional<std::string> FullName;
		std::optional<ItemsData> Items;
	};

	std::vector<Parsers::Statement<ConfigData>> g_configVec;
	std::unordered_map<RE::TESObjectCONT*, PatchData> g_patchMap;

	class ContainerParser : public Parsers::Parser<ConfigData> {
	public:
		ContainerParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

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
			case ElementType::kFullName:
				logger::info("{}{}({}).{} = \"{}\";", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element), a_configData.AssignValue.value());
				break;

			case ElementType::kItems:
				logger::info("{}{}({}).{}", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element));
				for (std::size_t ii = 0; ii < a_configData.Operations.size(); ii++) {
					std::string opLog;

					switch (a_configData.Operations[ii].OpType) {
					case OperationType::kClear:
						opLog = std::format(".{}()", OperationTypeToString(a_configData.Operations[ii].OpType));
						break;

					case OperationType::kAdd:
						opLog = std::format(".{}({}, {})", OperationTypeToString(a_configData.Operations[ii].OpType), a_configData.Operations[ii].OpData->Form, a_configData.Operations[ii].OpData->Count);
						break;

					case OperationType::kDelete:
					case OperationType::kDeleteAll:
						opLog = std::format(".{}({})", OperationTypeToString(a_configData.Operations[ii].OpType), a_configData.Operations[ii].OpData->Form);
						break;
					}

					if (ii == a_configData.Operations.size() - 1) {
						opLog += ";";
					}

					logger::info("{}    {}", indent, opLog);
				}
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
			if (token == "FullName") {
				a_config.Element = ElementType::kFullName;
			}
			else if (token == "Items") {
				a_config.Element = ElementType::kItems;
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

			if (a_config.Element == ElementType::kFullName) {
				token = reader.GetToken();
				if (!token.starts_with('\"')) {
					logger::warn("Line {}, Col {}: {} must be a string.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_config.Element));
					return false;
				}
				else if (!token.ends_with('\"')) {
					logger::warn("Line {}, Col {}: String must end with '\"'.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}

				a_config.AssignValue = std::string(token.substr(1, token.length() - 2));
			}
			else {
				logger::warn("Line {}, Col {}: Invalid Assignment to {}.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_config.Element));
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
			else if (token == "Delete") {
				opType = OperationType::kDelete;
			}
			else if (token == "DeleteAll") {
				opType = OperationType::kDeleteAll;
			}
			else {
				logger::warn("Line {}, Col {}: Invalid OperationName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			if (opType == OperationType::kClear || opType == OperationType::kAdd || opType == OperationType::kDelete || opType == OperationType::kDeleteAll) {
				if (a_configData.Element != ElementType::kItems) {
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

			std::optional<ConfigData::Operation::Data> opData;
			if (opType != OperationType::kClear) {
				opData = ConfigData::Operation::Data{};

				if (opType == OperationType::kAdd) {
					std::optional<std::string> form = ParseForm();
					if (!form.has_value()) {
						return false;
					}
					opData->Form = form.value();

					token = reader.GetToken();
					if (token != ",") {
						logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
						return false;
					}

					token = reader.GetToken();
					if (token.empty()) {
						logger::warn("Line {}, Col {}: Syntax error. Expected Count.", reader.GetLastLine(), reader.GetLastLineIndex());
						return false;
					}

					unsigned long parsedValue;
					auto parseResult = std::from_chars(token.data(), token.data() + token.size(), parsedValue);
					if (parseResult.ec != std::errc()) {
						logger::warn("Line {}, Col {}: Failed to parse Count '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
						return false;
					}

					if (parsedValue == 0 || parsedValue > UINT32_MAX) {
						logger::warn("Line {}, Col {}: Count '{}' is out of range.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
						return false;
					}
					opData->Count = static_cast<std::uint32_t>(parsedValue);
				}
				else if (opType == OperationType::kDelete || opType == OperationType::kDeleteAll) {
					std::optional<std::string> form = ParseForm();
					if (!form.has_value()) {
						return false;
					}
					opData->Form = form.value();
				}
			}

			token = reader.GetToken();
			if (token != ")") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			a_configData.Operations.push_back(ConfigData::Operation{ opType, opData });

			return true;
		}
	};

	void ReadConfigs() {
		g_configVec = ConfigUtils::ReadConfigs<ContainerParser, Parsers::Statement<ConfigData>>(TypeName);
	}

	void Prepare(const ConfigData& a_configData) {
		if (a_configData.Filter == FilterType::kFormID) {
			RE::TESForm* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
			if (!filterForm) {
				logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
				return;
			}

			RE::TESObjectCONT* container = filterForm->As<RE::TESObjectCONT>();
			if (!container) {
				logger::warn("'{}' is not a Container.", a_configData.FilterForm);
				return;
			}

			PatchData& patchData = g_patchMap[container];

			if (a_configData.Element == ElementType::kFullName) {
				patchData.FullName = a_configData.AssignValue.value();
			}
			else if (a_configData.Element == ElementType::kItems) {
				if (!patchData.Items.has_value()) {
					patchData.Items = PatchData::ItemsData{};
				}

				for (const auto& op : a_configData.Operations) {
					if (op.OpType == OperationType::kClear) {
						patchData.Items->Clear = true;
					}
					else if (op.OpType == OperationType::kAdd || op.OpType == OperationType::kDelete || op.OpType == OperationType::kDeleteAll) {
						RE::TESForm* opForm = Utils::GetFormFromString(op.OpData->Form);
						if (!opForm) {
							logger::warn("Invalid Form: '{}'.", op.OpData->Form);
							continue;
						}

						RE::TESBoundObject* boundObj = opForm->As<RE::TESBoundObject>();
						if (!boundObj) {
							logger::warn("'{}' is not a valid bound object.", op.OpData->Form);
							continue;
						}

						if (op.OpType == OperationType::kAdd) {
							patchData.Items->AddObjectVec.push_back(PatchData::ItemsData::Item{ boundObj, op.OpData->Count });
						}
						else if (op.OpType == OperationType::kDelete) {
							patchData.Items->DeleteObjectVec.push_back(boundObj);
						}
						else if (op.OpType == OperationType::kDeleteAll) {
							patchData.Items->DeleteAllObjectVec.push_back(boundObj);
						}
					}
				}
			}
		}
	}

	std::vector<PatchData::ItemsData::Item> GetContainerEntries(RE::TESObjectCONT* a_container) {
		std::vector<PatchData::ItemsData::Item> retVec;

		if (!a_container || !a_container->containerObjects || a_container->numContainerObjects == 0) {
			return retVec;
		}

		retVec.reserve(a_container->numContainerObjects);

		for (std::uint32_t contIndex = 0; contIndex < a_container->numContainerObjects; ++contIndex) {
			retVec.emplace_back(PatchData::ItemsData::Item{ a_container->containerObjects[contIndex]->obj, static_cast<std::uint32_t>(a_container->containerObjects[contIndex]->count) });
		}

		return retVec;
	}

	void SetItems(RE::TESObjectCONT* a_container, const std::vector<PatchData::ItemsData::Item>& a_items) {
		// Clear existing entries
		if (a_container->containerObjects) {
			for (std::uint32_t contIndex = 0; contIndex < a_container->numContainerObjects; ++contIndex) {
				RE::free(a_container->containerObjects[contIndex]);
			}
			RE::free(a_container->containerObjects);
			a_container->containerObjects = nullptr;
			a_container->numContainerObjects = 0;
		}

		if (a_items.empty()) {
			return;
		}

		// Set new entries
		RE::ContainerObject** newItems = static_cast<RE::ContainerObject**>(RE::malloc(sizeof(RE::ContainerObject*) * a_items.size()));
		if (!newItems) {
			logger::error("Failed to allocate the ContainerObject array.");
			return;
		}

		std::uint32_t actualCount = 0;
		for (const auto& entry : a_items) {
			RE::ContainerObject* newItem = static_cast<RE::ContainerObject*>(RE::malloc(sizeof(RE::ContainerObject)));
			if (!newItem) {
				logger::error("Failed to allocate a ContainerObject.");
				continue;
			}

			newItems[actualCount++] = ::new (newItem) RE::ContainerObject(entry.Form, static_cast<std::int32_t>(entry.Count));
		}

		a_container->containerObjects = newItems;
		a_container->numContainerObjects = actualCount;
	}

	void PatchItems(RE::TESObjectCONT* a_container, const PatchData::ItemsData& a_itemsData) {
		bool isCleared = false, isModified = false;

		std::vector<PatchData::ItemsData::Item> items;

		if (a_itemsData.Clear) {
			isCleared = true;
		}
		else {
			items = GetContainerEntries(a_container);
		}

		if (!isCleared) {
			// Delete
			for (const auto& delObject : a_itemsData.DeleteObjectVec) {
				for (auto it = items.begin(); it != items.end(); it++) {
					if (it->Form == delObject) {
						items.erase(it);
						isModified = true;
						break;
					}
				}
			}

			// DeleteAll
			for (const auto& delAllObject : a_itemsData.DeleteAllObjectVec) {
				for (auto it = items.begin(); it != items.end();) {
					if (it->Form == delAllObject) {
						it = items.erase(it);
						isModified = true;
					}
					else {
						++it;
					}
				}
			}
		}

		// Add
		for (const auto& addObject : a_itemsData.AddObjectVec) {
			items.emplace_back(addObject);
			isModified = true;
		}

		// Sort Items
		std::sort(items.begin(), items.end(), [](const PatchData::ItemsData::Item& a, const PatchData::ItemsData::Item& b) {
			return a.Form->formID < b.Form->formID;
		});

		if (isCleared || isModified) {
			SetItems(a_container, items);
		}
	}

	void Patch() {
		logger::info("======================== Start preparing patch for {} ========================", TypeName);

		ConfigUtils::Prepare(g_configVec, Prepare);

		logger::info("======================== Finished preparing patch for {} ========================", TypeName);
		logger::info("");

		logger::info("======================== Start patching for {} ========================", TypeName);

		for (const auto& patchData : g_patchMap) {
			if (patchData.second.FullName.has_value()) {
				patchData.first->fullName = patchData.second.FullName.value();
			}

			if (patchData.second.Items.has_value()) {
				PatchItems(patchData.first, patchData.second.Items.value());
			}
		}

		logger::info("======================== Finished patching for {} ========================", TypeName);
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}
