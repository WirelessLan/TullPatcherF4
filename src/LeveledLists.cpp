#include "LeveledLists.h"

#include <unordered_set>
#include <regex>

#include "ConfigUtils.h"
#include "Parsers.h"
#include "Utils.h"

namespace LeveledLists {
	constexpr std::string_view TypeName = "LeveledList";

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
		kEntries,
		kChanceNone,
		kMaxCount,
		kFlags
	};

	std::string_view ElementTypeToString(ElementType a_value) {
		switch (a_value) {
		case ElementType::kEntries: return "Entries";
		case ElementType::kChanceNone: return "ChanceNone";
		case ElementType::kMaxCount: return "MaxCount";
		case ElementType::kFlags: return "Flags";
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
				std::uint16_t Level;
				std::string Form;
				std::uint16_t Count;
				std::uint8_t ChanceNone;
			};

			OperationType OpType;
			std::optional<Data> OpData;
		};

		FilterType Filter;
		std::string FilterForm;
		ElementType Element;
		std::vector<Operation> Operations;
		std::optional<std::uint8_t> AssignValue;
	};

	struct PatchData {
		struct EntriesData {
			struct Entry {
				std::uint16_t Level;
				RE::TESForm* Form;
				std::uint16_t Count;
				std::uint8_t ChanceNone;
			};

			bool Clear;
			std::vector<Entry> AddEntryVec;
			std::vector<Entry> DeleteEntryVec;
			std::unordered_set<RE::TESForm*> DeleteAllEntrySet;
		};

		std::optional<std::uint8_t> ChanceNone;
		std::optional<std::uint8_t> MaxCount;
		std::optional<std::uint8_t> Flags;
		std::optional<EntriesData> Entries;
	};

	std::vector<Parsers::Statement<ConfigData>> g_configVec;
	std::unordered_map<RE::TESLeveledList*, PatchData> g_patchMap;

	class LeveledListParser : public Parsers::Parser<ConfigData> {
	public:
		LeveledListParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

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
			case ElementType::kEntries:
				logger::info("{}{}({}).{}", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element));
				for (std::size_t opIndex = 0; opIndex < a_configData.Operations.size(); opIndex++) {
					std::string opLog;

					switch (a_configData.Operations[opIndex].OpType) {
					case OperationType::kClear:
						opLog = fmt::format(".{}()", OperationTypeToString(a_configData.Operations[opIndex].OpType));
						break;

					case OperationType::kDeleteAll:
						opLog = fmt::format(".{}({})", OperationTypeToString(a_configData.Operations[opIndex].OpType), a_configData.Operations[opIndex].OpData->Form);
						break;

					case OperationType::kAdd:
					case OperationType::kDelete:
						opLog = fmt::format(".{}({}, {}, {}, {})", OperationTypeToString(a_configData.Operations[opIndex].OpType),
							a_configData.Operations[opIndex].OpData->Level,
							a_configData.Operations[opIndex].OpData->Form,
							a_configData.Operations[opIndex].OpData->Count,
							a_configData.Operations[opIndex].OpData->ChanceNone);
						break;
					}

					if (opIndex == a_configData.Operations.size() - 1) {
						opLog += ";";
					}

					logger::info("{}    {}", indent, opLog);
				}
				break;

			case ElementType::kChanceNone:
			case ElementType::kFlags:
			case ElementType::kMaxCount:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm,
					ElementTypeToString(a_configData.Element), a_configData.AssignValue.value());
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
			if (token == "Entries") {
				a_configData.Element = ElementType::kEntries;
			}
			else if (token == "ChanceNone") {
				a_configData.Element = ElementType::kChanceNone;
			}
			else if (token == "MaxCount") {
				a_configData.Element = ElementType::kMaxCount;
			}
			else if (token == "Flags") {
				a_configData.Element = ElementType::kFlags;
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

			if (a_configData.Element != ElementType::kChanceNone && a_configData.Element != ElementType::kMaxCount && a_configData.Element != ElementType::kFlags) {
				logger::warn("Line {}, Col {}: Invalid Assignment for '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_configData.Element));
				return false;
			}

			token = reader.GetToken();
			if (token.empty()) {
				logger::warn("Line {}, Col {}: Expected value.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			unsigned long parsedValue;
			auto parseResult = std::from_chars(token.data(), token.data() + token.size(), parsedValue);
			if (parseResult.ec != std::errc()) {
				logger::warn("Line {}, Col {}: Failed to parse value '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			if (parsedValue > UINT8_MAX) {
				logger::warn("Line {}, Col {}: Failed to parse value '{}'. The value is out of range", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}
			else if (a_configData.Element == ElementType::kFlags && parsedValue > 7) {
				logger::warn("Line {}, Col {}: Failed to parse Flags '{}'. The value is out of range", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			a_configData.AssignValue = static_cast<std::uint8_t>(parsedValue);

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

			bool isValidOperation = [](ElementType elem, OperationType op) {
				if (elem == ElementType::kEntries) {
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

			if (a_configData.Element == ElementType::kEntries) {
				if (newOp.OpType != OperationType::kClear) {
					ConfigData::Operation::Data opData{};

					if (newOp.OpType == OperationType::kAdd || newOp.OpType == OperationType::kDelete) {
						token = reader.GetToken();
						if (token.empty()) {
							logger::warn("Line {}, Col {}: Expected level.", reader.GetLastLine(), reader.GetLastLineIndex());
							return false;
						}

						unsigned long parsedValue;
						auto parseResult = std::from_chars(token.data(), token.data() + token.size(), parsedValue);
						if (parseResult.ec != std::errc()) {
							logger::warn("Line {}, Col {}: Failed to parse level '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
							return false;
						}

						if (parsedValue > UINT16_MAX) {
							logger::warn("Line {}, Col {}: Failed to parse level '{}'. The value is out of range", reader.GetLastLine(), reader.GetLastLineIndex(), token);
							return false;
						}

						opData.Level = static_cast<std::uint16_t>(parsedValue);

						token = reader.GetToken();
						if (token != ",") {
							logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
							return false;
						}

						std::optional<std::string> opForm = ParseForm();
						if (!opForm.has_value()) {
							return false;
						}

						opData.Form = opForm.value();

						token = reader.GetToken();
						if (token != ",") {
							logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
							return false;
						}

						token = reader.GetToken();
						if (token.empty()) {
							logger::warn("Line {}, Col {}: Expected count.", reader.GetLastLine(), reader.GetLastLineIndex());
							return false;
						}

						parseResult = std::from_chars(token.data(), token.data() + token.size(), parsedValue);
						if (parseResult.ec != std::errc()) {
							logger::warn("Line {}, Col {}: Failed to parse count '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
							return false;
						}

						if (parsedValue > UINT16_MAX) {
							logger::warn("Line {}, Col {}: Failed to parse count '{}'. The value is out of range", reader.GetLastLine(), reader.GetLastLineIndex(), token);
							return false;
						}

						opData.Count = static_cast<std::uint16_t>(parsedValue);

						token = reader.GetToken();
						if (token != ",") {
							logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
							return false;
						}

						token = reader.GetToken();
						if (token.empty()) {
							logger::warn("Line {}, Col {}: Expected chaceNone.", reader.GetLastLine(), reader.GetLastLineIndex());
							return false;
						}

						parseResult = std::from_chars(token.data(), token.data() + token.size(), parsedValue);
						if (parseResult.ec != std::errc()) {
							logger::warn("Line {}, Col {}: Failed to parse chanceNone '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
							return false;
						}

						if (parsedValue > UINT8_MAX) {
							logger::warn("Line {}, Col {}: Failed to parse chanceNone '{}'. The value is out of range", reader.GetLastLine(), reader.GetLastLineIndex(), token);
							return false;
						}

						opData.ChanceNone = static_cast<std::uint8_t>(parsedValue);
					}
					else if (newOp.OpType == OperationType::kDeleteAll) {
						std::optional<std::string> opForm = ParseForm();
						if (!opForm.has_value()) {
							return false;
						}

						opData.Form = opForm.value();
					}

					newOp.OpData = opData;
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
		g_configVec = ConfigUtils::ReadConfigs<LeveledListParser, Parsers::Statement<ConfigData>>(TypeName);
	}

	void Prepare(const ConfigData& a_configData) {
		if (a_configData.Filter == FilterType::kFormID) {
			RE::TESForm* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
			if (!filterForm) {
				logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
				return;
			}

			RE::TESLeveledList* leveledList = filterForm->As<RE::TESLeveledList>();
			if (!leveledList) {
				logger::warn("'{}' is not a LeveledList.", a_configData.FilterForm);
				return;
			}

			PatchData& patchData = g_patchMap[leveledList];

			if (a_configData.Element == ElementType::kEntries) {
				if (!patchData.Entries.has_value()) {
					patchData.Entries = PatchData::EntriesData{};
				}

				for (const auto& op : a_configData.Operations) {
					if (op.OpType == OperationType::kClear) {
						patchData.Entries->Clear = true;
					}
					else if (op.OpType == OperationType::kAdd || op.OpType == OperationType::kDelete || op.OpType == OperationType::kDeleteAll) {
						RE::TESForm* opForm = Utils::GetFormFromString(op.OpData->Form);
						if (!opForm) {
							logger::warn("Invalid Form: '{}'.", op.OpData->Form);
							continue;
						}

						if (op.OpType == OperationType::kAdd) {
							patchData.Entries->AddEntryVec.push_back({ op.OpData->Level, opForm, op.OpData->Count, op.OpData->ChanceNone });
						}
						else if (op.OpType == OperationType::kDelete) {
							patchData.Entries->DeleteEntryVec.push_back({ op.OpData->Level, opForm, op.OpData->Count, op.OpData->ChanceNone });
						}
						else {
							patchData.Entries->DeleteAllEntrySet.insert(opForm);
						}
					}
				}
			}
			else if (a_configData.Element == ElementType::kChanceNone) {
				patchData.ChanceNone = a_configData.AssignValue.value();
			}
			else if (a_configData.Element == ElementType::kMaxCount) {
				patchData.MaxCount = a_configData.AssignValue.value();
			}
			else if (a_configData.Element == ElementType::kFlags) {
				patchData.Flags = a_configData.AssignValue.value();
			}
		}
	}

	std::vector<RE::LEVELED_OBJECT> GetLeveledListEntries(RE::TESLeveledList* a_leveledList) {
		std::vector<RE::LEVELED_OBJECT> retVec;

		if (!a_leveledList || !a_leveledList->leveledLists || a_leveledList->baseListCount == 0) {
			return retVec;
		}

		for (std::size_t entryIndex = 0; entryIndex < static_cast<std::uint8_t>(a_leveledList->baseListCount); entryIndex++) {
			retVec.push_back(a_leveledList->leveledLists[entryIndex]);
		}

		return retVec;
	}

	struct LL_ALLOC {
		std::uint32_t count;
		RE::LEVELED_OBJECT ll[1];
	};

	LL_ALLOC* AllocateLL(std::size_t a_entriesCnt) {
		return static_cast<LL_ALLOC*>(RE::MemoryManager::GetSingleton().Allocate(sizeof(RE::LEVELED_OBJECT) * a_entriesCnt + sizeof(std::size_t), 0, false));
	}

	void FreeLeveledListEntries(RE::LEVELED_OBJECT* a_lobj, uint32_t arg2 = 0x3) {
		if (!a_lobj) {
			return;
		}

		using func_t = decltype(&FreeLeveledListEntries);
		const REL::Relocation<func_t> func{ REL::ID(296092) };
		return func(a_lobj, arg2);
	}

	void SetLeveledListEntries(RE::TESLeveledList* a_leveledList, const std::vector<RE::LEVELED_OBJECT>& a_entries) {
		if (!a_leveledList) {
			return;
		}

		if (a_leveledList->leveledLists) {
			std::uint8_t listCount = static_cast<std::uint8_t>(a_leveledList->baseListCount);
			memset(a_leveledList->leveledLists, 0, listCount * sizeof(RE::LEVELED_OBJECT));
			FreeLeveledListEntries(a_leveledList->leveledLists);

			a_leveledList->leveledLists = nullptr;
			a_leveledList->baseListCount = 0;
		}

		if (a_entries.empty()) {
			return;
		}

		std::size_t entriesCnt = a_entries.size();
		if (entriesCnt > UINT8_MAX) {
			logger::critical("Entries size is bigger than 255. The entries size has been set to 255.");
			entriesCnt = UINT8_MAX;
		}

		LL_ALLOC* newEntries = AllocateLL(entriesCnt);
		if (!newEntries) {
			logger::critical("Failed to allocate the LeveledList entries.");
			return;
		}

		newEntries->count = static_cast<std::uint32_t>(entriesCnt);
		for (std::size_t entryIndex = 0; entryIndex < entriesCnt; entryIndex++) {
			newEntries->ll[entryIndex] = a_entries[entryIndex];
		}

		a_leveledList->leveledLists = newEntries->ll;
		a_leveledList->baseListCount = static_cast<std::int8_t>(entriesCnt);
	}

	void PatchEntries(RE::TESLeveledList* a_leveledList, const PatchData::EntriesData& a_entriesData) {
		bool isCleared = false, isModified = false;

		std::vector<RE::LEVELED_OBJECT> leveledListVec;

		// Clear
		if (a_entriesData.Clear) {
			isCleared = true;
		}
		else {
			leveledListVec = GetLeveledListEntries(a_leveledList);
		}

		// Delete, DeleteAll
		if (!isCleared) {
			// Delete
			for (const auto& delEntry : a_entriesData.DeleteEntryVec) {
				for (auto it = leveledListVec.begin(); it != leveledListVec.end(); it++) {
					if (delEntry.Level == it->level && delEntry.Form == it->form && delEntry.Count == it->count && delEntry.ChanceNone == it->chanceNone) {
						leveledListVec.erase(it);
						isModified = true;
						break;
					}
				}
			}

			// DeleteAll
			for (const auto& delForm : a_entriesData.DeleteAllEntrySet) {
				for (auto it = leveledListVec.begin(); it != leveledListVec.end();) {
					if (delForm == it->form) {
						leveledListVec.erase(it);
						isModified = true;
						continue;
					}

					it++;
				}
			}
		}

		// Add
		for (const auto& addEntry : a_entriesData.AddEntryVec) {
			leveledListVec.push_back({ addEntry.Form, nullptr, addEntry.Count, addEntry.Level, static_cast<std::int8_t>(addEntry.ChanceNone) });
			isModified = true;
		}

		std::sort(leveledListVec.begin(), leveledListVec.end(), [](const RE::LEVELED_OBJECT& a, const RE::LEVELED_OBJECT& b) {
			return a.level < b.level;
		});

		if (isCleared || isModified) {
			SetLeveledListEntries(a_leveledList, leveledListVec);
		}
	}

	void Patch(RE::TESLeveledList* a_leveledList, const PatchData& a_patchData) {
		if (a_patchData.Entries.has_value()) {
			PatchEntries(a_leveledList, a_patchData.Entries.value());
		}

		if (a_patchData.ChanceNone.has_value()) {
			a_leveledList->chanceNone = static_cast<std::int8_t>(a_patchData.ChanceNone.value());
		}

		if (a_patchData.MaxCount.has_value()) {
			a_leveledList->maxUseAllCount = static_cast<std::int8_t>(a_patchData.MaxCount.value());
		}

		if (a_patchData.Flags.has_value()) {
			a_leveledList->llFlags = static_cast<std::int8_t>(a_patchData.Flags.value());
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
