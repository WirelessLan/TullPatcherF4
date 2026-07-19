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
				}
				while (reader.Peek() == ".");
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
			case ElementType::kFullName:
				logger::info("{}{}({}).{} = \"{}\";", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element), a_configData.AssignValue.value());
				break;

			case ElementType::kItems:
				logger::info("{}{}({}).{}", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element));
				for (std::size_t opIndex = 0; opIndex < a_configData.Operations.size(); opIndex++) {
					std::string opLog;

					switch (a_configData.Operations[opIndex].OpType) {
					case OperationType::kClear:
						opLog = std::format(".{}()", OperationTypeToString(a_configData.Operations[opIndex].OpType));
						break;

					case OperationType::kAdd:
						opLog = std::format(".{}({}, {})", OperationTypeToString(a_configData.Operations[opIndex].OpType), a_configData.Operations[opIndex].OpData->Form, a_configData.Operations[opIndex].OpData->Count);
						break;

					case OperationType::kDelete:
					case OperationType::kDeleteAll:
						opLog = std::format(".{}({})", OperationTypeToString(a_configData.Operations[opIndex].OpType), a_configData.Operations[opIndex].OpData->Form);
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

			const auto filterFormOpt = ParseForm();
			if (!filterFormOpt.has_value()) {
				return false;
			}

			a_config.FilterForm = filterFormOpt.value();

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
				const auto fullNameOpt = ParseString();
				if (!fullNameOpt.has_value())
				{
					return false;
				}

				a_config.AssignValue = fullNameOpt.value();
			}
			else {
				logger::warn("Line {}, Col {}: Invalid Assignment for '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_config.Element));
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
			else if (token == "DeleteAll") {
				newOp.OpType = OperationType::kDeleteAll;
			}
			else {
				logger::warn("Line {}, Col {}: Invalid OperationName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			auto isValidOperation = [](ElementType elem, OperationType op) -> bool {
				if (elem == ElementType::kItems) {
					return op == OperationType::kClear || op == OperationType::kAdd || op == OperationType::kDelete || op == OperationType::kDeleteAll;
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

			if (a_configData.Element == ElementType::kItems) {
				if (newOp.OpType != OperationType::kClear) {
					ConfigData::Operation::Data opData{};

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

						const auto valueOpt = ParseNumber<std::uint32_t>();
						if (!valueOpt.has_value()) {
							return false;
						}

						opData.Count = valueOpt.value();
					}

					newOp.OpData = opData;
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
		g_configVec = ConfigUtils::ReadConfigs<ContainerParser, Parsers::Statement<ConfigData>>(TypeName);
	}

	void Prepare(const ConfigData& a_configData) {
		if (a_configData.Filter == FilterType::kFormID) {
			auto* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
			if (!filterForm) {
				logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
				return;
			}

			auto* container = filterForm->As<RE::TESObjectCONT>();
			if (!container) {
				logger::warn("'{}' is not a Container.", a_configData.FilterForm);
				return;
			}

			auto& patchData = g_patchMap[container];

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
						auto* opForm = Utils::GetFormFromString(op.OpData->Form);
						if (!opForm) {
							logger::warn("Invalid Form: '{}'.", op.OpData->Form);
							continue;
						}

						auto* boundObj = opForm->As<RE::TESBoundObject>();
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
		std::vector<PatchData::ItemsData::Item> entries;

		if (!a_container || !a_container->containerObjects || a_container->numContainerObjects == 0) {
			return entries;
		}

		entries.reserve(a_container->numContainerObjects);

		for (std::uint32_t contIndex = 0; contIndex < a_container->numContainerObjects; ++contIndex) {
			auto& item = entries.emplace_back();
			item.Form = a_container->containerObjects[contIndex]->obj;
			item.Count = static_cast<std::uint32_t>(a_container->containerObjects[contIndex]->count);
		}

		return entries;
	}

	void FreeItems(RE::ContainerObject** a_items, std::uint32_t a_count) {
		if (!a_items) {
			return;
		}

		for (std::uint32_t itemIndex = 0; itemIndex < a_count; ++itemIndex) {
			RE::free(a_items[itemIndex]);
		}

		RE::free(a_items);
	}

	void SetItems(RE::TESObjectCONT* a_container, const std::vector<PatchData::ItemsData::Item>& a_items) {
		if (a_items.empty()) {
			FreeItems(a_container->containerObjects, a_container->numContainerObjects);
			a_container->containerObjects = nullptr;
			a_container->numContainerObjects = 0;
			return;
		}

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
				FreeItems(newItems, actualCount);
				return;
			}

			newItems[actualCount++] = ::new (newItem) RE::ContainerObject(entry.Form, static_cast<std::int32_t>(entry.Count));
		}

		auto** oldItems = a_container->containerObjects;
		const auto oldItemCount = a_container->numContainerObjects;

		a_container->containerObjects = newItems;
		a_container->numContainerObjects = actualCount;

		FreeItems(oldItems, oldItemCount);
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
