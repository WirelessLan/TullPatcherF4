#include "Armors.h"

#include <regex>
#include <any>

#include "Parsers.h"
#include "Utils.h"

namespace Armors {
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
		kBipedObjectSlots,
		kFullName
	};

	std::string_view ElementTypeToString(ElementType a_value) {
		switch (a_value) {
		case ElementType::kBipedObjectSlots: return "BipedObjectSlots";
		case ElementType::kFullName: return "FullName";
		default: return std::string_view{};
		}
	}

	struct ConfigData {
		FilterType Filter;
		std::string FilterForm;
		ElementType Element;
		std::optional<std::any> AssignValue;
	};

	struct PatchData {
		std::optional<std::uint32_t> BipedObjectSlots;
		std::optional<std::string> FullName;
	};

	std::vector<Parsers::Statement<ConfigData>> g_configVec;
	std::unordered_map<RE::TESObjectARMO*, PatchData> g_patchMap;

	class ArmorParser : public Parsers::Parser<ConfigData> {
	public:
		ArmorParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

	protected:
		std::optional<Parsers::Statement<ConfigData>> ParseExpressionStatement() override {
			if (reader.EndOfFile() || reader.Peek().empty())
				return std::nullopt;

			ConfigData configData{};

			if (!ParseFilter(configData))
				return std::nullopt;

			auto token = reader.GetToken();
			if (token != ".") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '.'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			if (!ParseElement(configData))
				return std::nullopt;

			if (!ParseAssignment(configData))
				return std::nullopt;

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
			case ElementType::kBipedObjectSlots:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm,
					ElementTypeToString(a_configData.Element), GetBipedSlots(std::any_cast<std::uint32_t>(a_configData.AssignValue.value())));
				break;

			case ElementType::kFullName:
				logger::info("{}{}({}).{} = \"{}\";", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm,
					ElementTypeToString(a_configData.Element), std::any_cast<std::string>(a_configData.AssignValue.value()));
				break;
			}
		}

		bool ParseFilter(ConfigData& a_config) {
			auto token = reader.GetToken();
			if (token == "FilterByFormID")
				a_config.Filter = FilterType::kFormID;
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
			if (!filterForm.has_value())
				return false;

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
			if (token == "BipedObjectSlots")
				a_config.Element = ElementType::kBipedObjectSlots;
			else if (token == "FullName")
				a_config.Element = ElementType::kFullName;
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

			if (a_config.Element == ElementType::kBipedObjectSlots) {
				std::uint32_t bipedObjectSlotsValue = 0;

				auto bipedSlot = ParseBipedSlot();
				if (!bipedSlot.has_value())
					return false;

				if (bipedSlot.value() != 0)
					bipedObjectSlotsValue |= 1 << (bipedSlot.value() - 30);

				while (true) {
					token = reader.Peek();
					if (token == ";")
						break;

					token = reader.GetToken();
					if (token != "|") {
						logger::warn("Line {}, Col {}: Syntax error. Expected '|' or ';'.", reader.GetLastLine(), reader.GetLastLineIndex());
						return false;
					}

					bipedSlot = ParseBipedSlot();
					if (!bipedSlot.has_value())
						return false;

					if (bipedSlot.value() != 0)
						bipedObjectSlotsValue |= 1 << (bipedSlot.value() - 30);
				}

				a_config.AssignValue = std::any(bipedObjectSlotsValue);
			}
			else if (a_config.Element == ElementType::kFullName) {
				token = reader.GetToken();
				if (!token.starts_with('\"')) {
					logger::warn("Line {}, Col {}: FullName must be a string.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}
				else if (!token.ends_with('\"')) {
					logger::warn("Line {}, Col {}: String must end with '\"'.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}

				std::string value = std::string(token.substr(1, token.length() - 2));
				a_config.AssignValue = std::any(value);
			}
			else {
				logger::warn("Line {}, Col {}: Invalid Assignment to {}.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_config.Element));
				return false;
			}

			return true;
		}

		std::optional<std::uint32_t> ParseBipedSlot() {
			unsigned long parsedValue;

			auto token = reader.GetToken();
			if (token.empty() || token == "|" || token == ";") {
				logger::warn("Line {}, Col {}: Expected BipedSlot '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return std::nullopt;
			}

			auto parsingResult = std::from_chars(token.data(), token.data() + token.size(), parsedValue);
			if (parsingResult.ec != std::errc()) {
				logger::warn("Line {}, Col {}: Failed to parse bipedSlot '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return std::nullopt;
			}

			if (parsedValue != 0 && (parsedValue < 30 || parsedValue > 61)) {
				logger::warn("Line {}, Col {}: Failed to parse bipedSlot '{}'. The value is out of range", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return std::nullopt;
			}

			return static_cast<std::uint32_t>(parsedValue);
		}

		std::string GetBipedSlots(std::uint32_t a_bipedObjSlots) {
			std::string retStr;
			std::string separtor = " | ";

			if (a_bipedObjSlots == 0)
				return "0";

			for (std::size_t ii = 0; ii < 32; ii++) {
				if (a_bipedObjSlots & (1 << ii))
					retStr += std::to_string(ii + 30) + separtor;
			}

			if (retStr.empty())
				return retStr;

			return retStr.substr(0, retStr.size() - separtor.size());
		}
	};

	void ReadConfig(std::string_view a_path) {
		ArmorParser parser(a_path);
		auto parsedStatements = parser.Parse();
		g_configVec.insert(g_configVec.end(), parsedStatements.begin(), parsedStatements.end());
	}

	void ReadConfigs() {
		const std::filesystem::path configDir{ "Data\\" + std::string(Version::PROJECT) + "\\Armor" };
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
			logger::info("=========== Reading Armor config file: {} ===========", path);
			ReadConfig(path);
			logger::info("");
		}
	}

	void Prepare(const ConfigData& a_configData) {
		if (a_configData.Filter == FilterType::kFormID) {
			RE::TESForm* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
			if (!filterForm) {
				logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
				return;
			}

			RE::TESObjectARMO* armo = filterForm->As<RE::TESObjectARMO>();
			if (!armo) {
				logger::warn("'{}' is not a Armor.", a_configData.FilterForm);
				return;
			}

			if (a_configData.Element == ElementType::kBipedObjectSlots) {
				if (a_configData.AssignValue.has_value())
					g_patchMap[armo].BipedObjectSlots = std::any_cast<std::uint32_t>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kFullName) {
				if (a_configData.AssignValue.has_value())
					g_patchMap[armo].FullName = std::any_cast<std::string>(a_configData.AssignValue.value());
			}
		}
	}

	void Prepare(const std::vector<Parsers::Statement<ConfigData>>& a_configVec) {
		for (const auto& configData : a_configVec) {
			if (configData.Type == Parsers::StatementType::kExpression)
				Prepare(configData.ExpressionStatement.value());
			else if (configData.Type == Parsers::StatementType::kConditional)
				Prepare(configData.ConditionalStatement->Evaluates());
		}
	}

	void Patch() {
		logger::info("======================== Start preparing patch for Armor ========================");

		Prepare(g_configVec);

		logger::info("======================== Finished preparing patch for Armor ========================");
		logger::info("");

		logger::info("======================== Start patching for Armor ========================");

		for (const auto& patchData : g_patchMap) {
			if (patchData.second.BipedObjectSlots.has_value())
				patchData.first->bipedModelData.bipedObjectSlots = patchData.second.BipedObjectSlots.value();
			if (patchData.second.FullName.has_value())
				patchData.first->fullName = patchData.second.FullName.value();
		}

		logger::info("======================== Finished patching for Armor ========================");
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}
