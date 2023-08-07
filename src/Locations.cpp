#include "Locations.h"

#include <regex>

#include "Configs.h"
#include "Utils.h"

namespace Locations {
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
		kKeywords
	};

	std::string_view ElementTypeToString(ElementType a_value) {
		switch (a_value) {
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
		std::vector<Operation> Operations;
	};

	struct PatchData {
		struct KeywordsData {
			bool Clear = false;
			std::vector<RE::BGSKeyword*> AddKeywordVec;
			std::vector<RE::BGSKeyword*> AddUniqueKeywordVec;
			std::vector<RE::BGSKeyword*> DeleteKeywordVec;
		};

		std::optional<KeywordsData> Keywords;
	};

	std::vector<ConfigData> g_configVec;
	std::unordered_map<RE::BGSLocation*, PatchData> g_patchMap;

	class LocationParser : public Configs::Parser<ConfigData> {
	public:
		LocationParser(Configs::ConfigReader& a_configReader) : Configs::Parser<ConfigData>(a_configReader) {}

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
			if (token == "Keywords")
				a_configData.Element = ElementType::kKeywords;
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
			else if (token == "AddIfNotExists")
				opType = OperationType::kAddIfNotExists;
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

	void ReadConfig(std::string_view a_path) {
		Configs::ConfigReader reader(a_path);

		while (!reader.EndOfFile()) {
			LocationParser parser(reader);
			auto configData = parser.Parse();
			if (!configData.has_value()) {
				parser.RecoverFromError();
				continue;
			}

			g_configVec.push_back(configData.value());

			logger::info("{}({}).{}", FilterTypeToString(configData->Filter), configData->FilterForm, ElementTypeToString(configData->Element));
			for (std::size_t ii = 0; ii < configData->Operations.size(); ii++) {
				std::string opLog = fmt::format(".{}({})", OperationTypeToString(configData->Operations[ii].OpType),
					configData->Operations[ii].OpForm.has_value() ? configData->Operations[ii].OpForm.value() : "");

				if (ii == configData->Operations.size() - 1)
					opLog += ";";

				logger::info("    {}", opLog);
			}
		}
	}

	void ReadConfigs() {
		const std::filesystem::path configDir{ "Data\\" + std::string(Version::PROJECT) + "\\Location" };
		if (!std::filesystem::exists(configDir))
			return;

		const std::regex filter(".*\\.cfg", std::regex_constants::icase);
		const std::filesystem::directory_iterator dir_iter(configDir);
		for (auto& iter : dir_iter) {
			if (!std::filesystem::is_regular_file(iter.status()))
				continue;

			if (!std::regex_match(iter.path().filename().string(), filter))
				continue;

			std::string path = iter.path().string();
			logger::info("=========== Reading Location config file: {} ===========", path);
			ReadConfig(path);
			logger::info("");
		}
	}

	void Prepare(const std::vector<ConfigData>& a_configVec) {
		logger::info("======================== Start preparing patch for Location ========================");

		for (const auto& configData : a_configVec) {
			if (configData.Filter == FilterType::kFormID) {
				RE::TESForm* filterForm = Utils::GetFormFromString(configData.FilterForm);
				if (!filterForm) {
					logger::warn("Invalid FilterForm: '{}'.", configData.FilterForm);
					continue;
				}

				RE::BGSLocation* location = filterForm->As<RE::BGSLocation>();
				if (!location) {
					logger::warn("'{}' is not a Location.", configData.FilterForm);
					continue;
				}

				PatchData& patchData = g_patchMap[location];

				if (configData.Element == ElementType::kKeywords) {
					if (!patchData.Keywords.has_value())
						patchData.Keywords = PatchData::KeywordsData{};

					for (const auto& op : configData.Operations) {
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

							if (op.OpType == OperationType::kAdd)
								patchData.Keywords->AddKeywordVec.push_back(keywordForm);
							else if (op.OpType == OperationType::kAddIfNotExists)
								patchData.Keywords->AddUniqueKeywordVec.push_back(keywordForm);
							else
								patchData.Keywords->DeleteKeywordVec.push_back(keywordForm);
						}
					}
				}
			}
		}

		logger::info("======================== Finished preparing patch for Location ========================");
		logger::info("");
	}

	void ClearKeywords(RE::BGSLocation* a_location) {
		if (!a_location)
			return;

		while (a_location->numKeywords > 0)
			a_location->RemoveKeyword(a_location->keywords[0]);
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
				for (std::uint32_t ii = 0; ii < a_location->numKeywords; ii++) {
					if (a_location->keywords[ii] != delKywd)
						continue;

					delVec.push_back(delKywd);
					break;
				}
			}

			for (auto kywd : delVec)
				a_location->RemoveKeyword(kywd);
		}

		// Add
		if (!a_keywordsData.AddKeywordVec.empty()) {
			for (const auto& addKywd : a_keywordsData.AddKeywordVec) {
				a_location->AddKeyword(addKywd);
			}
		}

		// Add if not exists
		if (!a_keywordsData.AddUniqueKeywordVec.empty()) {
			for (const auto& addKywd : a_keywordsData.AddUniqueKeywordVec) {
				bool exists = false;
				for (std::uint32_t ii = 0; ii < a_location->numKeywords; ii++) {
					if (a_location->keywords[ii] != addKywd)
						continue;

					exists = true;
					break;
				}

				if (exists)
					continue;

				a_location->AddKeyword(addKywd);
			}
		}
	}

	void Patch(RE::BGSLocation* a_location, const PatchData& a_patchData) {
		if (a_patchData.Keywords.has_value())
			PatchKeywords(a_location, a_patchData.Keywords.value());
	}

	void Patch() {
		Prepare(g_configVec);

		logger::info("======================== Start patching for Location ========================");

		for (const auto& patchData : g_patchMap)
			Patch(patchData.first, patchData.second);

		logger::info("======================== Finished patching for Location ========================");
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}
