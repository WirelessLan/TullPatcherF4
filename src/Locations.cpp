#include "Locations.h"

#include <unordered_set>
#include <regex>

#include "ConfigUtils.h"
#include "Parsers.h"
#include "Utils.h"

namespace Locations {
	constexpr std::string_view TypeName = "Location";

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
		kKeywords
	};

	std::string_view ElementTypeToString(ElementType a_value) {
		switch (a_value) {
		case ElementType::kFullName: return "FullName";
		case ElementType::kKeywords: return "Keywords";
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
		std::optional<std::string> AssignValue;
		std::vector<Operation> Operations;
	};

	struct PatchData {
		struct KeywordsData {
			bool Clear = false;
			std::vector<RE::BGSKeyword*> AddKeywordVec;
			std::unordered_set<RE::BGSKeyword*> AddUniqueKeywordSet;
			std::vector<RE::BGSKeyword*> DeleteKeywordVec;
		};

		std::optional<std::string> FullName;
		std::optional<KeywordsData> Keywords;
	};

	std::vector<Parsers::Statement<ConfigData>> g_configVec;
	std::unordered_map<RE::BGSLocation*, PatchData> g_patchMap;

	class LocationParser : public Parsers::Parser<ConfigData> {
	public:
		LocationParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

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
				logger::info("{}{}({}).{} = \"{}\";", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm,
					ElementTypeToString(a_configData.Element), a_configData.AssignValue.value());
				break;

			case ElementType::kKeywords:
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
			if (token == "FullName") {
				a_configData.Element = ElementType::kFullName;
			}
			else if (token == "Keywords") {
				a_configData.Element = ElementType::kKeywords;
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
			else if (token == "AddIfNotExists") {
				newOp.OpType = OperationType::kAddIfNotExists;
			}
			else if (token == "Delete") {
				newOp.OpType = OperationType::kDelete;
			}
			else {
				logger::warn("Line {}, Col {}: Invalid OperationName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			bool isValidOperation = [](ElementType elem, OperationType op) {
				if (elem == ElementType::kKeywords) {
					return op == OperationType::kClear || op == OperationType::kAdd || op == OperationType::kAddIfNotExists || op == OperationType::kDelete;
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

			if (a_configData.Element == ElementType::kKeywords) {
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
		g_configVec = ConfigUtils::ReadConfigs<LocationParser, Parsers::Statement<ConfigData>>(TypeName);
	}

	void Prepare(const ConfigData& a_configData) {
		if (a_configData.Filter == FilterType::kFormID) {
			RE::TESForm* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
			if (!filterForm) {
				logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
				return;
			}

			RE::BGSLocation* location = filterForm->As<RE::BGSLocation>();
			if (!location) {
				logger::warn("'{}' is not a Location.", a_configData.FilterForm);
				return;
			}

			PatchData& patchData = g_patchMap[location];

			if (a_configData.Element == ElementType::kFullName) {
				patchData.FullName = a_configData.AssignValue.value();
			}
			else if (a_configData.Element == ElementType::kKeywords) {
				if (!patchData.Keywords.has_value()) {
					patchData.Keywords = PatchData::KeywordsData{};
				}

				for (const auto& op : a_configData.Operations) {
					if (op.OpType == OperationType::kClear) {
						patchData.Keywords->Clear = true;
					}
					else if (op.OpType == OperationType::kAdd || op.OpType == OperationType::kAddIfNotExists || op.OpType == OperationType::kDelete) {
						RE::TESForm* opForm = Utils::GetFormFromString(op.OpForm.value());
						if (!opForm) {
							logger::warn("Invalid Form: '{}'.", op.OpForm.value());
							continue;
						}

						RE::BGSKeyword* keywordForm = opForm->As<RE::BGSKeyword>();
						if (!keywordForm) {
							logger::warn("'{}' is not a Keyword.", op.OpForm.value());
							continue;
						}

						if (op.OpType == OperationType::kAdd) {
							patchData.Keywords->AddKeywordVec.push_back(keywordForm);
						}
						else if (op.OpType == OperationType::kAddIfNotExists) {
							patchData.Keywords->AddUniqueKeywordSet.insert(keywordForm);
						}
						else {
							patchData.Keywords->DeleteKeywordVec.push_back(keywordForm);
						}
					}
				}
			}
		}
	}

	void ClearKeywords(RE::BGSLocation* a_location) {
		if (!a_location) {
			return;
		}

		while (a_location->numKeywords > 0) {
			a_location->RemoveKeyword(a_location->keywords[0]);
		}
	}

	bool KeywordExists(RE::BGSLocation* a_location, RE::BGSKeyword* a_keyword) {
		for (std::uint32_t keywordIndex = 0; keywordIndex < a_location->numKeywords; keywordIndex++) {
			if (a_location->keywords[keywordIndex] == a_keyword) {
				return true;
			}
		}
		return false;
	}

	void PatchKeywords(RE::BGSLocation* a_location, const PatchData::KeywordsData& a_keywordsData) {
		bool isCleared = false;

		// Clear
		if (a_keywordsData.Clear) {
			ClearKeywords(a_location);
			isCleared = true;
		}

		// Delete
		if (!isCleared && !a_keywordsData.DeleteKeywordVec.empty()) {
			std::vector<RE::BGSKeyword*> delVec;
			for (const auto& delKywd : a_keywordsData.DeleteKeywordVec) {
				if (KeywordExists(a_location, delKywd)) {
					delVec.push_back(delKywd);
				}
			}

			for (auto kywd : delVec) {
				a_location->RemoveKeyword(kywd);
			}
		}

		// Add
		if (!a_keywordsData.AddKeywordVec.empty()) {
			for (const auto& addKywd : a_keywordsData.AddKeywordVec) {
				a_location->AddKeyword(addKywd);
			}
		}

		// Add if not exists
		if (!a_keywordsData.AddUniqueKeywordSet.empty()) {
			for (const auto& addKywd : a_keywordsData.AddUniqueKeywordSet) {
				if (!KeywordExists(a_location, addKywd)) {
					a_location->AddKeyword(addKywd);
				}
			}
		}
	}

	void Patch(RE::BGSLocation* a_location, const PatchData& a_patchData) {
		if (a_patchData.FullName.has_value()) {
			a_location->fullName = a_patchData.FullName.value();
		}

		if (a_patchData.Keywords.has_value()) {
			PatchKeywords(a_location, a_patchData.Keywords.value());
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
