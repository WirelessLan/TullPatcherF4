#include "LeveledLists.h"

#include <regex>

#include "Configs.h"
#include "Utils.h"

namespace LeveledLists {
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
		};

		std::optional<std::uint8_t> ChanceNone;
		std::optional<std::uint8_t> MaxCount;
		std::optional<std::uint8_t> Flags;
		std::optional<EntriesData> Entries;
	};

	std::vector<ConfigData> g_configVec;
	std::unordered_map<RE::TESLeveledList*, PatchData> g_patchMap;

	class LeveldListParser : public Configs::Parser<ConfigData> {
	public:
		LeveldListParser(Configs::ConfigReader& a_configReader) : Configs::Parser<ConfigData>(a_configReader) {}

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

			token = reader.LookAhead();
			if (token == "=") {
				if (!parseAssignment(configData))
					return std::nullopt;

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
			if (token == "Entries")
				a_configData.Element = ElementType::kEntries;
			else if (token == "ChanceNone")
				a_configData.Element = ElementType::kChanceNone;
			else if (token == "MaxCount")
				a_configData.Element = ElementType::kMaxCount;
			else if (token == "Flags")
				a_configData.Element = ElementType::kFlags;
			else {
				logger::warn("Line {}, Col {}: Invalid ElementName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			return true;
		}

		bool parseAssignment(ConfigData& a_configData) {
			auto token = reader.GetToken();
			if (token != "=") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '='.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			if (a_configData.Element != ElementType::kChanceNone && a_configData.Element != ElementType::kMaxCount && a_configData.Element != ElementType::kFlags) {
				logger::warn("Line {}, Col {}: Invalid Assignment '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_configData.Element));
				return false;
			}

			token = reader.GetToken();
			if (token.empty() || token == ";") {
				logger::warn("Line {}, Col {}: Expected Value '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			unsigned long parsedValue;
			auto parsingResult = std::from_chars(token.data(), token.data() + token.size(), parsedValue);
			if (parsingResult.ec != std::errc()) {
				logger::warn("Line {}, Col {}: Failed to parse value '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			if (parsedValue > 0xFF) {
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
				if (a_configData.Element != ElementType::kEntries) {
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

				token = reader.GetToken();
				if (token.empty() || token == ",") {
					logger::warn("Line {}, Col {}: Expected Level '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				unsigned long parsedValue;
				auto parsingResult = std::from_chars(token.data(), token.data() + token.size(), parsedValue);
				if (parsingResult.ec != std::errc()) {
					logger::warn("Line {}, Col {}: Failed to parse level '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				if (parsedValue > 0xFFFF) {
					logger::warn("Line {}, Col {}: Failed to parse level '{}'. The value is out of range", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				opData->Level = static_cast<std::uint16_t>(parsedValue);

				token = reader.GetToken();
				if (token != ",") {
					logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}

				std::optional<std::string> opForm = parseForm();
				if (!opForm.has_value())
					return false;

				opData->Form = opForm.value();

				token = reader.GetToken();
				if (token != ",") {
					logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}

				token = reader.GetToken();
				if (token.empty()) {
					logger::warn("Line {}, Col {}: Expected Count '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				parsingResult = std::from_chars(token.data(), token.data() + token.size(), parsedValue);
				if (parsingResult.ec != std::errc()) {
					logger::warn("Line {}, Col {}: Failed to parse count '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				if (parsedValue > 0xFFFF) {
					logger::warn("Line {}, Col {}: Failed to parse count '{}'. The value is out of range", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				opData->Count = static_cast<std::uint16_t>(parsedValue);

				token = reader.GetToken();
				if (token != ",") {
					logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}

				token = reader.GetToken();
				if (token.empty()) {
					logger::warn("Line {}, Col {}: Expected ChaceNone '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				parsingResult = std::from_chars(token.data(), token.data() + token.size(), parsedValue);
				if (parsingResult.ec != std::errc()) {
					logger::warn("Line {}, Col {}: Failed to parse chanceNone '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				if (parsedValue > 0xFF) {
					logger::warn("Line {}, Col {}: Failed to parse chanceNone '{}'. The value is out of range", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				opData->ChanceNone = static_cast<std::uint8_t>(parsedValue);
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
			LeveldListParser parser(reader);
			auto configData = parser.Parse();
			if (!configData.has_value()) {
				parser.RecoverFromError();
				continue;
			}

			g_configVec.push_back(configData.value());

			if (configData->Element == ElementType::kEntries) {
				logger::info("{}({}).{}", FilterTypeToString(configData->Filter), configData->FilterForm, ElementTypeToString(configData->Element));
				for (std::size_t ii = 0; ii < configData->Operations.size(); ii++) {
					if (ii < configData->Operations.size() - 1) {
						if (configData->Operations[ii].OpType == OperationType::kClear)
							logger::info("    .{}()", OperationTypeToString(configData->Operations[ii].OpType));
						else if (configData->Operations[ii].OpType == OperationType::kDelete)
							logger::info("    .{}({}, {})", OperationTypeToString(configData->Operations[ii].OpType),
								configData->Operations[ii].OpData->Level,
								configData->Operations[ii].OpData->Form);
						else
							logger::info("    .{}({}, {}, {}, {})", OperationTypeToString(configData->Operations[ii].OpType),
								configData->Operations[ii].OpData->Level,
								configData->Operations[ii].OpData->Form,
								configData->Operations[ii].OpData->Count,
								configData->Operations[ii].OpData->ChanceNone);
					}
					else {
						if (configData->Operations[ii].OpType == OperationType::kClear)
							logger::info("    .{}();", OperationTypeToString(configData->Operations[ii].OpType));
						else if (configData->Operations[ii].OpType == OperationType::kDelete)
							logger::info("    .{}({}, {});", OperationTypeToString(configData->Operations[ii].OpType),
								configData->Operations[ii].OpData->Level,
								configData->Operations[ii].OpData->Form);
						else
							logger::info("    .{}({}, {}, {}, {});", OperationTypeToString(configData->Operations[ii].OpType),
								configData->Operations[ii].OpData->Level,
								configData->Operations[ii].OpData->Form,
								configData->Operations[ii].OpData->Count,
								configData->Operations[ii].OpData->ChanceNone);
					}
				}
			}
			else {
				logger::info("{}({}).{} = {};", FilterTypeToString(configData->Filter), configData->FilterForm,
					ElementTypeToString(configData->Element), configData->AssignValue.value());
			}
		}
	}

	void ReadConfigs() {
		const std::filesystem::path configDir{ "Data\\" + std::string(Version::PROJECT) + "\\LeveledList" };
		const std::regex filter(".*\\.cfg", std::regex_constants::icase);
		const std::filesystem::directory_iterator dir_iter(configDir);
		for (auto& iter : dir_iter) {
			if (!std::filesystem::is_regular_file(iter.status()))
				continue;

			if (!std::regex_match(iter.path().filename().string(), filter))
				continue;

			std::string path = iter.path().string();
			logger::info("=========== Reading LeveledList config file: {} ===========", path);
			readConfig(path);
			logger::info("");
		}
	}

	void Prepare(const std::vector<ConfigData> a_configVec) {
		logger::info("======================== Start preparing patch for LeveledList ========================");

		for (const auto& configData : a_configVec) {
			if (configData.Filter == FilterType::kFormID) {
				RE::TESForm* filterForm = Utils::GetFormFromString(configData.FilterForm);
				if (!filterForm) {
					logger::warn("Invalid FilterForm: '{}'.", configData.FilterForm);
					continue;
				}

				RE::TESLeveledList* leveledList = filterForm->As<RE::TESLeveledList>();
				if (!leveledList) {
					logger::warn("'{}' is not a LeveledList.", configData.FilterForm);
					continue;
				}

				PatchData& patchData = g_patchMap[leveledList];

				if (configData.Element == ElementType::kEntries) {
					if (!patchData.Entries.has_value())
						patchData.Entries = PatchData::EntriesData{};

					for (const auto& op : configData.Operations) {
						if (op.OpType == OperationType::kClear) {
							patchData.Entries->Clear = true;
						}
						else if (op.OpType == OperationType::kAdd || op.OpType == OperationType::kDelete) {
							RE::TESForm* opForm = Utils::GetFormFromString(op.OpData->Form);
							if (!opForm) {
								logger::warn("Invalid Form: '{}'.", op.OpData->Form);
								continue;
							}

							if (op.OpType == OperationType::kAdd)
								patchData.Entries->AddEntryVec.push_back({ op.OpData->Level, opForm, op.OpData->Count, op.OpData->ChanceNone });
							else
								patchData.Entries->DeleteEntryVec.push_back({ op.OpData->Level, opForm, op.OpData->Count, op.OpData->ChanceNone });
						}
					}
				}
				else if (configData.Element == ElementType::kChanceNone) {
					if (configData.AssignValue.has_value())
						patchData.ChanceNone = configData.AssignValue.value();
				}
				else if (configData.Element == ElementType::kMaxCount) {
					if (configData.AssignValue.has_value())
						patchData.MaxCount = configData.AssignValue.value();
				}
				else if (configData.Element == ElementType::kFlags) {
					if (configData.AssignValue.has_value())
						patchData.Flags = configData.AssignValue.value();
				}
			}
		}

		logger::info("======================== Finished preparing patch for LeveledList ========================");
		logger::info("");
	}

	std::vector<RE::LEVELED_OBJECT> GetLeveledListEntries(RE::TESLeveledList* a_leveledList) {
		std::vector<RE::LEVELED_OBJECT> retVec;

		if (!a_leveledList || !a_leveledList->leveledLists || a_leveledList->baseListCount == 0)
			return retVec;

		for (std::size_t ii = 0; ii < static_cast<std::size_t>(a_leveledList->baseListCount); ii++)
			retVec.push_back(a_leveledList->leveledLists[ii]);

		return retVec;
	}

	struct LL_ALLOC {
		std::uint32_t count;
		RE::LEVELED_OBJECT ll[1];
	};

	LL_ALLOC* AllocateLL(std::size_t a_entriesCnt) {
		RE::MemoryManager mm = RE::MemoryManager::GetSingleton();
		return (LL_ALLOC*)mm.Allocate(sizeof(RE::LEVELED_OBJECT) * a_entriesCnt + sizeof(size_t), 0, 0);
	}

	void FreeLeveledListEntries(RE::LEVELED_OBJECT* a_lobj, uint32_t arg2 = 0x3) {
		if (!a_lobj)
			return;

		using func_t = decltype(&FreeLeveledListEntries);
		const REL::Relocation<func_t> func{ REL::ID(296092) };
		return func(a_lobj, arg2);
	}

	void SetLeveledListEntries(RE::TESLeveledList* a_leveledList, const std::vector<RE::LEVELED_OBJECT>& a_entries) {
		if (!a_leveledList)
			return;

		if (a_leveledList->leveledLists) {
			memset(a_leveledList->leveledLists, 0, a_leveledList->baseListCount * sizeof(RE::LEVELED_OBJECT));
			FreeLeveledListEntries(a_leveledList->leveledLists);

			a_leveledList->leveledLists = nullptr;
			a_leveledList->baseListCount = 0;
		}

		if (a_entries.empty())
			return;

		std::size_t entriesCnt = a_entries.size();
		if (entriesCnt > 0xFF) {
			logger::critical("Entries size is bigger than 255. The entries size has been set to 255.");
			entriesCnt = 0xFF;
		}

		LL_ALLOC* newEntries = AllocateLL(entriesCnt);
		if (!newEntries) {
			logger::critical("Failed to allocate the new LeveledList entries.");
			return;
		}

		newEntries->count = static_cast<std::uint32_t>(entriesCnt);
		for (std::size_t ii = 0; ii < entriesCnt; ii++)
			newEntries->ll[ii] = a_entries[ii];

		a_leveledList->leveledLists = newEntries->ll;
		a_leveledList->baseListCount = static_cast<std::uint8_t>(entriesCnt);
	}

	void PatchEntries(RE::TESLeveledList* a_leveledList, const PatchData::EntriesData& a_entriesData) {
		bool isCleared = false, isDeleted = false, isAdded = false;

		std::vector<RE::LEVELED_OBJECT> leveledListVec;

		// Clear
		if (a_entriesData.Clear)
			isCleared = true;
		else if (!a_entriesData.AddEntryVec.empty() || !a_entriesData.DeleteEntryVec.empty())
			leveledListVec = GetLeveledListEntries(a_leveledList);

		// Delete
		if (!isCleared && !a_entriesData.DeleteEntryVec.empty()) {
			std::size_t preSize = leveledListVec.size();

			for (const auto& delEntry : a_entriesData.DeleteEntryVec) {
				for (auto it = leveledListVec.begin(); it != leveledListVec.end(); it++) {
					if (delEntry.Level != it->level || delEntry.Form != it->form || delEntry.Count != it->count || delEntry.ChanceNone != it->chanceNone)
						continue;

					leveledListVec.erase(it);
					break;
				}
			}

			if (preSize != leveledListVec.size())
				isDeleted = true;
		}

		// Add
		if (!a_entriesData.AddEntryVec.empty()) {
			for (const auto& addEntry : a_entriesData.AddEntryVec) {
				RE::LEVELED_OBJECT nLvlObj = { addEntry.Form, nullptr, addEntry.Count, addEntry.Level, static_cast<std::int8_t>(addEntry.ChanceNone) };
				leveledListVec.push_back(nLvlObj);
			}

			isAdded = true;
		}

		std::sort(leveledListVec.begin(), leveledListVec.end(), [](const RE::LEVELED_OBJECT& a, const RE::LEVELED_OBJECT& b) {
			if (a.level != b.level)
				return a.level < b.level;
			else
				return a.form->formID < b.form->formID;
		});

		if (isCleared || isDeleted || isAdded)
			SetLeveledListEntries(a_leveledList, leveledListVec);
	}

	void Patch(RE::TESLeveledList* a_leveledList, const PatchData& a_patchData) {
		if (a_patchData.Entries.has_value())
			PatchEntries(a_leveledList, a_patchData.Entries.value());

		if (a_patchData.ChanceNone.has_value())
			a_leveledList->chanceNone = static_cast<std::int8_t>(a_patchData.ChanceNone.value());

		if (a_patchData.MaxCount.has_value())
			a_leveledList->maxUseAllCount = static_cast<std::int8_t>(a_patchData.MaxCount.value());

		if (a_patchData.ChanceNone.has_value())
			a_leveledList->llFlags = static_cast<std::int8_t>(a_patchData.Flags.value());
	}

	void Patch() {
		Prepare(g_configVec);

		logger::info("======================== Start patching for LeveledList ========================");

		for (const auto& patchData : g_patchMap)
			Patch(patchData.first, patchData.second);

		logger::info("======================== Finished patching for LeveledList ========================");
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}
