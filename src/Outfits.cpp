#include "Outfits.h"

#include <regex>

#include "ConfigUtils.h"
#include "Parsers.h"
#include "Utils.h"

namespace Outfits {
	constexpr std::string_view TypeName = "Outfit";

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
		kItems
	};

	std::string_view ElementTypeToString(ElementType a_value) {
		switch (a_value) {
		case ElementType::kItems: return "Items";
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
		struct ItemsData {
			bool Clear = false;
			std::vector<RE::TESForm*> AddFormVec;
			std::vector<RE::TESForm*> DeleteFormVec;
		};

		std::optional<ItemsData> Items;
	};

	std::vector<Parsers::Statement<ConfigData>> g_configVec;
	std::unordered_map<RE::BGSOutfit*, PatchData> g_patchMap;

	class OutfitParser : public Parsers::Parser<ConfigData> {
	public:
		OutfitParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

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
			case ElementType::kItems:
				logger::info("{}{}({}).{}", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element));
				for (std::size_t opIndex = 0; opIndex < a_configData.Operations.size(); opIndex++) {
					std::string opLog = fmt::format(".{}({})", OperationTypeToString(a_configData.Operations[opIndex].OpType),
						a_configData.Operations[opIndex].OpForm.has_value() ? a_configData.Operations[opIndex].OpForm.value() : "");

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
			if (token == "Items") {
				a_configData.Element = ElementType::kItems;
			}
			else {
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

			bool isValidOperation = [](ElementType elem, OperationType op) {
				switch (elem) {
				case ElementType::kItems:
					return (op == OperationType::kClear || op == OperationType::kAdd || op == OperationType::kDelete);
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

			if (a_configData.Element == ElementType::kItems) {
				if (newOp.OpType != OperationType::kClear) {
					auto form = ParseForm();
					if (!form.has_value()) {
						return false;
					}
					newOp.OpForm = form.value();
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
	};

	void ReadConfigs() {
		g_configVec = ConfigUtils::ReadConfigs<OutfitParser, Parsers::Statement<ConfigData>>(TypeName);
	}

	void Prepare(const ConfigData& a_configData) {
		if (a_configData.Filter == FilterType::kFormID) {
			RE::TESForm* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
			if (!filterForm) {
				logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
				return;
			}

			RE::BGSOutfit* outfit = filterForm->As<RE::BGSOutfit>();
			if (!outfit) {
				logger::warn("'{}' is not a Outfit.", a_configData.FilterForm);
				return;
			}

			PatchData& patchData = g_patchMap[outfit];

			if (a_configData.Element == ElementType::kItems) {
				if (!patchData.Items.has_value()) {
					patchData.Items = PatchData::ItemsData{};
				}

				for (const auto& op : a_configData.Operations) {
					if (op.OpType == OperationType::kClear) {
						patchData.Items->Clear = true;
					}
					else if (op.OpType == OperationType::kAdd || op.OpType == OperationType::kDelete) {
						RE::TESForm* opForm = Utils::GetFormFromString(op.OpForm.value());
						if (!opForm) {
							logger::warn("Invalid Form: '{}'.", op.OpForm.value());
							continue;
						}

						if (opForm->formType != RE::ENUM_FORM_ID::kLVLI && opForm->formType != RE::ENUM_FORM_ID::kARMO) {
							logger::warn("'{}' is not a Armor or a Leveled Item.", op.OpForm.value());
							continue;
						}

						if (op.OpType == OperationType::kAdd) {
							patchData.Items->AddFormVec.push_back(opForm);
						}
						else {
							patchData.Items->DeleteFormVec.push_back(opForm);
						}
					}
				}
			}
		}
	}

	void PatchItems(RE::BGSOutfit* a_outfit, const PatchData::ItemsData& a_itemsData) {
		bool isCleared = false;

		// Clear
		if (a_itemsData.Clear) {
			a_outfit->outfitItems.clear();
			isCleared = true;
		}

		// Delete
		if (!isCleared && !a_itemsData.DeleteFormVec.empty()) {
			for (const auto& delForm : a_itemsData.DeleteFormVec) {
				for (auto it = a_outfit->outfitItems.begin(); it != a_outfit->outfitItems.end(); it++) {
					if (*it != delForm) {
						continue;
					}

					a_outfit->outfitItems.erase(it);
					break;
				}
			}
		}

		// Add
		if (!a_itemsData.AddFormVec.empty()) {
			for (const auto& addForm : a_itemsData.AddFormVec) {
				a_outfit->outfitItems.push_back(addForm);
			}
		}
	}

	void Patch(RE::BGSOutfit* a_outfit, const PatchData& a_patchData) {
		if (a_patchData.Items.has_value()) {
			PatchItems(a_outfit, a_patchData.Items.value());
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
