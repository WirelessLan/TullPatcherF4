#include "ArmorAddons.h"

#include <regex>

#include "ConfigUtils.h"
#include "Parsers.h"
#include "Utils.h"

namespace ArmorAddons
{
	namespace
	{
		constexpr std::string_view kTypeName = "ArmorAddon";

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
			kBipedObjectSlots,
		};

		std::string_view ElementTypeToString(ElementType a_value)
		{
			switch (a_value)
			{
			case ElementType::kBipedObjectSlots:
				return "BipedObjectSlots";
			default:
				return std::string_view{};
			}
		}

		struct ConfigData
		{
			FilterType Filter;
			std::string FilterForm;
			ElementType Element;
			std::optional<std::uint32_t> AssignValue;
		};

		struct PatchData
		{
			std::optional<std::uint32_t> BipedObjectSlots;
		};

		std::vector<Parsers::Statement<ConfigData>> g_configVec;
		std::unordered_map<RE::TESObjectARMA*, PatchData> g_patchMap;

		class ArmorAddonParser : public Parsers::Parser<ConfigData>
		{
		public:
			ArmorAddonParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

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
				case ElementType::kBipedObjectSlots:
					logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm,
						ElementTypeToString(a_configData.Element), GetBipedSlots(a_configData.AssignValue.value()));
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
				if (token == "BipedObjectSlots")
				{
					a_config.Element = ElementType::kBipedObjectSlots;
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

				if (a_config.Element == ElementType::kBipedObjectSlots)
				{
					std::uint32_t bipedSlots = 0;

					auto bipedSlotOpt = ParseBipedSlot();
					if (!bipedSlotOpt.has_value())
					{
						return false;
					}

					if (bipedSlotOpt.value() != 0)
					{
						bipedSlots |= 1u << (bipedSlotOpt.value() - 30);
					}

					while (reader.Peek() == "|")
					{
						reader.GetToken();

						bipedSlotOpt = ParseBipedSlot();
						if (!bipedSlotOpt.has_value())
						{
							return false;
						}

						if (bipedSlotOpt.value() != 0)
						{
							bipedSlots |= 1u << (bipedSlotOpt.value() - 30);
						}
					}

					a_config.AssignValue = bipedSlots;
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

				auto* arma = filterForm->As<RE::TESObjectARMA>();
				if (!arma)
				{
					logger::warn("'{}' is not a ArmorAddon.", a_configData.FilterForm);
					return;
				}

				auto& patchData = g_patchMap[arma];

				if (a_configData.Element == ElementType::kBipedObjectSlots)
				{
					patchData.BipedObjectSlots = a_configData.AssignValue.value();
				}
			}
		}
	}  // namespace

	void ReadConfigs()
	{
		g_configVec = ConfigUtils::ReadConfigs<ArmorAddonParser, Parsers::Statement<ConfigData>>(kTypeName);
	}

	void Patch()
	{
		logger::info("======================== Start preparing patch for {} ========================", kTypeName);

		ConfigUtils::Prepare(g_configVec, Prepare);

		logger::info("======================== Finished preparing patch for {} ========================", kTypeName);
		logger::info("");

		logger::info("======================== Start patching for {} ========================", kTypeName);

		for (const auto& [armorAddon, patchData] : g_patchMap)
		{
			if (patchData.BipedObjectSlots.has_value())
			{
				armorAddon->bipedModelData.bipedObjectSlots = patchData.BipedObjectSlots.value();
			}
		}

		logger::info("======================== Finished patching for {} ========================", kTypeName);
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}  // namespace ArmorAddons
