#include "WorldSpaces.h"

#include <regex>

#include "ConfigUtils.h"
#include "Parsers.h"
#include "Utils.h"

namespace WorldSpaces {
	constexpr std::string_view TypeName = "Worldspace";

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
		kFullName
	};

	std::string_view ElementTypeToString(ElementType a_value) {
		switch (a_value) {
		case ElementType::kFullName: return "FullName";
		default: return std::string_view{};
		}
	}

	struct ConfigData {
		FilterType Filter;
		std::string FilterForm;
		ElementType Element;
		std::optional<std::string> AssignValue;
	};

	struct PatchData {
		std::optional<std::string> FullName;
	};

	std::vector<Parsers::Statement<ConfigData>> g_configVec;
	std::unordered_map<RE::TESWorldSpace*, PatchData> g_patchMap;

	class WorldspaceParser : public Parsers::Parser<ConfigData> {
	public:
		WorldspaceParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

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

			if (!ParseAssignment(configData)) {
				return std::nullopt;
			}

			token = reader.GetToken();
			if (token != ";") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ';'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
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
	};

	void ReadConfigs() {
		g_configVec = ConfigUtils::ReadConfigs<WorldspaceParser, Parsers::Statement<ConfigData>>(TypeName);
	}

	void Prepare(const ConfigData& a_configData) {
		if (a_configData.Filter == FilterType::kFormID) {
			RE::TESForm* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
			if (!filterForm) {
				logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
				return;
			}

			RE::TESWorldSpace* worldspace = filterForm->As<RE::TESWorldSpace>();
			if (!worldspace) {
				logger::warn("'{}' is not a Worldspace.", a_configData.FilterForm);
				return;
			}

			if (a_configData.Element == ElementType::kFullName) {
				g_patchMap[worldspace].FullName = a_configData.AssignValue.value();
			}
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
		}

		logger::info("======================== Finished patching for {} ========================", TypeName);
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}
