#include "Weapons.h"

#include <any>
#include <regex>

#include "ConfigUtils.h"
#include "Parsers.h"
#include "Utils.h"

namespace Weapons
{
	namespace
	{
		constexpr std::string_view kTypeName = "Weapon";

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
			kAmmo,
			kAttackDelay,
			kMaxRange,
			kMinRange,
			kNPCAddAmmoList,
			kObjectEffect,
			kReach,
			kReloadSpeed,
			kSpeed,
		};

		std::string_view ElementTypeToString(ElementType a_value)
		{
			switch (a_value)
			{
			case ElementType::kAmmo:
				return "Ammo";
			case ElementType::kAttackDelay:
				return "AttackDelay";
			case ElementType::kMaxRange:
				return "MaxRange";
			case ElementType::kMinRange:
				return "MinRange";
			case ElementType::kNPCAddAmmoList:
				return "NPCAddAmmoList";
			case ElementType::kObjectEffect:
				return "ObjectEffect";
			case ElementType::kReach:
				return "Reach";
			case ElementType::kReloadSpeed:
				return "ReloadSpeed";
			case ElementType::kSpeed:
				return "Speed";
			default:
				return std::string_view{};
			}
		}

		struct ConfigData
		{
			FilterType Filter;
			std::string FilterForm;
			ElementType Element;
			std::optional<std::any> AssignValue;
		};

		struct PatchData
		{
			std::optional<RE::TESAmmo*> Ammo;
			std::optional<float> AttackDelay;
			std::optional<float> MaxRange;
			std::optional<float> MinRange;
			std::optional<RE::TESLevItem*> NPCAddAmmoList;
			std::optional<RE::EnchantmentItem*> ObjectEffect;
			std::optional<float> Reach;
			std::optional<float> ReloadSpeed;
			std::optional<float> Speed;
		};

		std::vector<Parsers::Statement<ConfigData>> g_configVec;
		std::unordered_map<RE::TESObjectWEAP*, PatchData> g_patchMap;

		class WeaponParser : public Parsers::Parser<ConfigData>
		{
		public:
			WeaponParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

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
				case ElementType::kAmmo:
				case ElementType::kNPCAddAmmoList:
				case ElementType::kObjectEffect:
					logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm,
						ElementTypeToString(a_configData.Element), std::any_cast<std::string>(a_configData.AssignValue.value()));
					break;

				case ElementType::kAttackDelay:
				case ElementType::kMaxRange:
				case ElementType::kMinRange:
				case ElementType::kReach:
				case ElementType::kReloadSpeed:
				case ElementType::kSpeed:
					logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm,
						ElementTypeToString(a_configData.Element), std::any_cast<float>(a_configData.AssignValue.value()));
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
				if (token == "Ammo")
				{
					a_config.Element = ElementType::kAmmo;
				}
				else if (token == "AttackDelay")
				{
					a_config.Element = ElementType::kAttackDelay;
				}
				else if (token == "MaxRange")
				{
					a_config.Element = ElementType::kMaxRange;
				}
				else if (token == "MinRange")
				{
					a_config.Element = ElementType::kMinRange;
				}
				else if (token == "NPCAddAmmoList")
				{
					a_config.Element = ElementType::kNPCAddAmmoList;
				}
				else if (token == "ObjectEffect")
				{
					a_config.Element = ElementType::kObjectEffect;
				}
				else if (token == "Reach")
				{
					a_config.Element = ElementType::kReach;
				}
				else if (token == "ReloadSpeed")
				{
					a_config.Element = ElementType::kReloadSpeed;
				}
				else if (token == "Speed")
				{
					a_config.Element = ElementType::kSpeed;
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

				if (a_config.Element == ElementType::kAmmo || a_config.Element == ElementType::kNPCAddAmmoList || a_config.Element == ElementType::kObjectEffect)
				{
					token = reader.Peek();
					if (token == "null")
					{
						a_config.AssignValue = std::any(std::string(reader.GetToken()));
						return true;
					}

					const auto formOpt = ParseForm();
					if (!formOpt.has_value())
					{
						return false;
					}
					a_config.AssignValue = std::any(formOpt.value());
				}
				else if (a_config.Element == ElementType::kAttackDelay || a_config.Element == ElementType::kMaxRange || a_config.Element == ElementType::kMinRange ||
						 a_config.Element == ElementType::kReach || a_config.Element == ElementType::kReloadSpeed || a_config.Element == ElementType::kSpeed)
				{
					const auto parsedNumberOpt = ParseNumber<float>();
					if (!parsedNumberOpt.has_value())
					{
						return false;
					}

					a_config.AssignValue = std::any(parsedNumberOpt.value());
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

				auto* weap = filterForm->As<RE::TESObjectWEAP>();
				if (!weap)
				{
					logger::warn("'{}' is not a Weapon.", a_configData.FilterForm);
					return;
				}

				auto& patchData = g_patchMap[weap];

				if (a_configData.Element == ElementType::kAmmo)
				{
					const auto formStr = std::any_cast<std::string>(a_configData.AssignValue.value());

					if (formStr == "null")
					{
						patchData.Ammo = nullptr;
					}
					else
					{
						auto* ammoForm = Utils::GetFormFromString(formStr);
						if (!ammoForm)
						{
							logger::warn("Invalid Form: '{}'.", formStr);
							return;
						}

						auto* ammo = ammoForm->As<RE::TESAmmo>();
						if (!ammo)
						{
							logger::warn("'{}' is not an Ammo.", formStr);
							return;
						}

						patchData.Ammo = ammo;
					}
				}
				else if (a_configData.Element == ElementType::kAttackDelay)
				{
					patchData.AttackDelay = std::any_cast<float>(a_configData.AssignValue.value());
				}
				else if (a_configData.Element == ElementType::kMaxRange)
				{
					patchData.MaxRange = std::any_cast<float>(a_configData.AssignValue.value());
				}
				else if (a_configData.Element == ElementType::kMinRange)
				{
					patchData.MinRange = std::any_cast<float>(a_configData.AssignValue.value());
				}
				else if (a_configData.Element == ElementType::kNPCAddAmmoList)
				{
					const auto formStr = std::any_cast<std::string>(a_configData.AssignValue.value());

					if (formStr == "null")
					{
						patchData.NPCAddAmmoList = nullptr;
					}
					else
					{
						auto* levItemForm = Utils::GetFormFromString(formStr);
						if (!levItemForm)
						{
							logger::warn("Invalid Form: '{}'.", formStr);
							return;
						}

						auto* levItem = levItemForm->As<RE::TESLevItem>();
						if (!levItem)
						{
							logger::warn("'{}' is not a Leveled Item.", formStr);
							return;
						}

						patchData.NPCAddAmmoList = levItem;
					}
				}
				else if (a_configData.Element == ElementType::kObjectEffect)
				{
					const auto formStr = std::any_cast<std::string>(a_configData.AssignValue.value());

					if (formStr == "null")
					{
						patchData.ObjectEffect = nullptr;
					}
					else
					{
						auto* effectForm = Utils::GetFormFromString(formStr);
						if (!effectForm)
						{
							logger::warn("Invalid Form: '{}'.", formStr);
							return;
						}

						auto* objectEffect = effectForm->As<RE::EnchantmentItem>();
						if (!objectEffect)
						{
							logger::warn("'{}' is not an Object Effect.", formStr);
							return;
						}

						patchData.ObjectEffect = objectEffect;
					}
				}
				else if (a_configData.Element == ElementType::kReach)
				{
					patchData.Reach = std::any_cast<float>(a_configData.AssignValue.value());
				}
				else if (a_configData.Element == ElementType::kReloadSpeed)
				{
					patchData.ReloadSpeed = std::any_cast<float>(a_configData.AssignValue.value());
				}
				else if (a_configData.Element == ElementType::kSpeed)
				{
					patchData.Speed = std::any_cast<float>(a_configData.AssignValue.value());
				}
			}
		}
	}  // namespace

	void ReadConfigs()
	{
		g_configVec = ConfigUtils::ReadConfigs<WeaponParser, Parsers::Statement<ConfigData>>(kTypeName);
	}

	void Patch()
	{
		logger::info("======================== Start preparing patch for {} ========================", kTypeName);

		ConfigUtils::Prepare(g_configVec, Prepare);

		logger::info("======================== Finished preparing patch for {} ========================", kTypeName);
		logger::info("");

		logger::info("======================== Start patching for {} ========================", kTypeName);

		for (const auto& [weapon, patchData] : g_patchMap)
		{
			if (patchData.Ammo.has_value())
			{
				weapon->weaponData.ammo = patchData.Ammo.value();
			}
			if (patchData.AttackDelay.has_value())
			{
				weapon->weaponData.attackDelaySec = patchData.AttackDelay.value();
			}
			if (patchData.MaxRange.has_value())
			{
				weapon->weaponData.maxRange = patchData.MaxRange.value();
			}
			if (patchData.MinRange.has_value())
			{
				weapon->weaponData.minRange = patchData.MinRange.value();
			}
			if (patchData.NPCAddAmmoList.has_value())
			{
				weapon->weaponData.npcAddAmmoList = patchData.NPCAddAmmoList.value();
			}
			if (patchData.ObjectEffect.has_value())
			{
				weapon->formEnchanting = patchData.ObjectEffect.value();
			}
			if (patchData.Reach.has_value())
			{
				weapon->weaponData.reach = patchData.Reach.value();
			}
			if (patchData.ReloadSpeed.has_value())
			{
				weapon->weaponData.reloadSpeed = patchData.ReloadSpeed.value();
			}
			if (patchData.Speed.has_value())
			{
				weapon->weaponData.speed = patchData.Speed.value();
			}
		}

		logger::info("======================== Finished patching for {} ========================", kTypeName);
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}  // namespace Weapons
