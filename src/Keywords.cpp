#include "Keywords.h"

#include <regex>

#include "ConfigUtils.h"
#include "Parsers.h"
#include "Utils.h"

namespace Keywords
{
	namespace
	{
		constexpr std::string_view kTypeName = "Keyword";

		enum class FilterType
		{
			kFormID
		};

		std::string_view FilterTypeToString(FilterType a_value)
		{
			switch (a_value)
			{
			case FilterType::kFormID:
				return "FilterByFormID";
			default:
				return std::string_view{};
			}
		}

		enum class ElementType
		{
			kFullName
		};

		std::string_view ElementTypeToString(ElementType a_value)
		{
			switch (a_value)
			{
			case ElementType::kFullName:
				return "FullName";
			default:
				return std::string_view{};
			}
		}

		struct ConfigData
		{
			FilterType Filter;
			std::string FilterForm;
			ElementType Element;
			std::optional<std::string> AssignValue;
		};

		struct PatchData
		{
			std::optional<std::string> FullName;
		};

		std::vector<Parsers::Statement<ConfigData>> g_configVec;
		std::unordered_map<RE::BGSKeyword*, PatchData> g_patchMap;

		class KeywordParser : public Parsers::Parser<ConfigData>
		{
		public:
			KeywordParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

		protected:
			std::optional<Parsers::Statement<ConfigData>> ParseExpressionStatement() override
			{
				ConfigData configData{};

				if (!ParseFilter(configData))
				{
					return std::nullopt;
				}

				auto token = reader.GetToken();
				if (token != ".")
				{
					logger::warn("Line {}, Col {}: Syntax error. Expected '.'.", reader.GetLastLine(), reader.GetLastLineIndex());
					return std::nullopt;
				}

				if (!ParseElement(configData))
				{
					return std::nullopt;
				}

				if (!ParseAssignment(configData))
				{
					return std::nullopt;
				}

				token = reader.GetToken();
				if (token != ";")
				{
					logger::warn("Line {}, Col {}: Syntax error. Expected ';'.", reader.GetLastLine(), reader.GetLastLineIndex());
					return std::nullopt;
				}

				return Parsers::Statement<ConfigData>::CreateExpressionStatement(configData);
			}

			void PrintExpressionStatement(const ConfigData& a_configData, int a_indent) override
			{
				auto indent = std::string(a_indent * 4, ' ');

				switch (a_configData.Element)
				{
				case ElementType::kFullName:
					logger::info("{}{}({}).{} = \"{}\";", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm,
						ElementTypeToString(a_configData.Element), a_configData.AssignValue.value());
					break;
				}
			}

			bool ParseFilter(ConfigData& a_config)
			{
				auto token = reader.GetToken();
				if (token == "FilterByFormID")
				{
					a_config.Filter = FilterType::kFormID;
				}
				else
				{
					logger::warn("Line {}, Col {}: Invalid FilterName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				token = reader.GetToken();
				if (token != "(")
				{
					logger::warn("Line {}, Col {}: Syntax error. Expected '('.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}

				const auto filterFormOpt = ParseForm();
				if (!filterFormOpt.has_value())
				{
					return false;
				}

				a_config.FilterForm = filterFormOpt.value();

				token = reader.GetToken();
				if (token != ")")
				{
					logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}

				return true;
			}

			bool ParseElement(ConfigData& a_config)
			{
				const auto token = reader.GetToken();
				if (token == "FullName")
				{
					a_config.Element = ElementType::kFullName;
				}
				else
				{
					logger::warn("Line {}, Col {}: Invalid ElementName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				return true;
			}

			bool ParseAssignment(ConfigData& a_config)
			{
				auto token = reader.GetToken();
				if (token != "=")
				{
					logger::warn("Line {}, Col {}: Syntax error. Expected '='.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}

				if (a_config.Element == ElementType::kFullName)
				{
					const auto fullNameOpt = ParseString();
					if (!fullNameOpt.has_value())
					{
						return false;
					}

					a_config.AssignValue = fullNameOpt.value();
				}
				else
				{
					logger::warn("Line {}, Col {}: Invalid Assignment for '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_config.Element));
					return false;
				}

				return true;
			}
		};

		void Prepare(const ConfigData& a_configData)
		{
			if (a_configData.Filter == FilterType::kFormID)
			{
				auto* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
				if (!filterForm)
				{
					logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
					return;
				}

				auto* keyword = filterForm->As<RE::BGSKeyword>();
				if (!keyword)
				{
					logger::warn("'{}' is not a Keyword.", a_configData.FilterForm);
					return;
				}

				auto& patchData = g_patchMap[keyword];

				if (a_configData.Element == ElementType::kFullName)
				{
					patchData.FullName = a_configData.AssignValue.value();
				}
			}
		}

		void SetKeywordFullName(RE::BGSKeyword* a_keyword, std::string_view a_fullName)
		{
			if (!a_keyword)
			{
				return;
			}

			RE::BGSLocalizedString newFullName{};
			newFullName = a_fullName;

			using func_t = void (*)(RE::TESForm*, RE::BGSLocalizedString&);
			REL::Relocation<func_t> func{ REL::ID(1548495) };
			func(a_keyword, newFullName);
		}
	}  // namespace

	void ReadConfigs()
	{
		g_configVec = ConfigUtils::ReadConfigs<KeywordParser, Parsers::Statement<ConfigData>>(kTypeName);
	}

	void Patch()
	{
		logger::info("======================== Start preparing patch for {} ========================", kTypeName);

		ConfigUtils::Prepare(g_configVec, Prepare);

		logger::info("======================== Finished preparing patch for {} ========================", kTypeName);
		logger::info("");

		logger::info("======================== Start patching for {} ========================", kTypeName);

		for (const auto& [keyword, patchData] : g_patchMap)
		{
			if (patchData.FullName.has_value())
			{
				SetKeywordFullName(keyword, patchData.FullName.value());
			}
		}

		logger::info("======================== Finished patching for {} ========================", kTypeName);
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}  // namespace Keywords
