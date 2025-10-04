#include "Weapons.h"

#include <any>
#include <regex>

#include "ConfigUtils.h"
#include "Parsers.h"
#include "Utils.h"

namespace Weapons {
	constexpr std::string_view TypeName = "Weapon";

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

	std::string_view ElementTypeToString(ElementType a_value) {
		switch (a_value) {
		case ElementType::kAmmo: return "Ammo";
		case ElementType::kAttackDelay: return "AttackDelay";
		case ElementType::kMaxRange: return "MaxRange";
		case ElementType::kMinRange: return "MinRange";
		case ElementType::kNPCAddAmmoList: return "NPCAddAmmoList";
		case ElementType::kObjectEffect: return "ObjectEffect";
		case ElementType::kReach: return "Reach";
		case ElementType::kReloadSpeed: return "ReloadSpeed";
		case ElementType::kSpeed: return "Speed";
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

	class WeaponParser : public Parsers::Parser<ConfigData> {
	public:
		WeaponParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

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
			if (token == "Ammo") {
				a_config.Element = ElementType::kAmmo;
			}
			else if (token == "AttackDelay") {
				a_config.Element = ElementType::kAttackDelay;
			}
			else if (token == "MaxRange") {
				a_config.Element = ElementType::kMaxRange;
			}
			else if (token == "MinRange") {
				a_config.Element = ElementType::kMinRange;
			}
			else if (token == "NPCAddAmmoList") {
				a_config.Element = ElementType::kNPCAddAmmoList;
			}
			else if (token == "ObjectEffect") {
				a_config.Element = ElementType::kObjectEffect;
			}
			else if (token == "Reach") {
				a_config.Element = ElementType::kReach;
			}
			else if (token == "ReloadSpeed") {
				a_config.Element = ElementType::kReloadSpeed;
			}
			else if (token == "Speed") {
				a_config.Element = ElementType::kSpeed;
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

			if (a_config.Element == ElementType::kAmmo || a_config.Element == ElementType::kNPCAddAmmoList || a_config.Element == ElementType::kObjectEffect) {
				token = reader.Peek();
				if (token == "null") {
					a_config.AssignValue = std::any(std::string(reader.GetToken()));
				}
				else {
					auto effectForm = ParseForm();
					if (!effectForm.has_value()) {
						return false;
					}
					a_config.AssignValue = std::any(effectForm.value());
				}
			}
			else if (a_config.Element == ElementType::kAttackDelay || a_config.Element == ElementType::kMaxRange || a_config.Element == ElementType::kMinRange ||
				     a_config.Element == ElementType::kReach || a_config.Element == ElementType::kReloadSpeed || a_config.Element == ElementType::kSpeed) {
				auto floatValue = ParseNumber();
				if (!floatValue.has_value()) {
					return false;
				}

				a_config.AssignValue = std::any(floatValue.value());
			}
			else {
				logger::warn("Line {}, Col {}: Invalid Assignment for '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_config.Element));
				return false;
			}

			return true;
		}
	};

	void ReadConfigs() {
		g_configVec = ConfigUtils::ReadConfigs<WeaponParser, Parsers::Statement<ConfigData>>(TypeName);
	}

	void Prepare(const ConfigData& a_configData) {
		if (a_configData.Filter == FilterType::kFormID) {
			RE::TESForm* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
			if (!filterForm) {
				logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
				return;
			}

			RE::TESObjectWEAP* weap = filterForm->As<RE::TESObjectWEAP>();
			if (!weap) {
				logger::warn("'{}' is not a Weapon.", a_configData.FilterForm);
				return;
			}

			if (a_configData.Element == ElementType::kAmmo) {
				std::string formStr = std::any_cast<std::string>(a_configData.AssignValue.value());

				if (formStr == "null") {
					g_patchMap[weap].Ammo = nullptr;
				}
				else {
					RE::TESForm* ammoForm = Utils::GetFormFromString(formStr);
					if (!ammoForm) {
						logger::warn("Invalid Form: '{}'.", formStr);
						return;
					}

					RE::TESAmmo* ammo = ammoForm->As<RE::TESAmmo>();
					if (!ammo) {
						logger::warn("'{}' is not an Ammo.", formStr);
						return;
					}

					g_patchMap[weap].Ammo = ammo;
				}
			}
			else if (a_configData.Element == ElementType::kAttackDelay) {
				g_patchMap[weap].AttackDelay = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kMaxRange) {
				g_patchMap[weap].MaxRange = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kMinRange) {
				g_patchMap[weap].MinRange = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kNPCAddAmmoList) {
				std::string formStr = std::any_cast<std::string>(a_configData.AssignValue.value());

				if (formStr == "null") {
					g_patchMap[weap].NPCAddAmmoList = nullptr;
				}
				else {
					RE::TESForm* levItemForm = Utils::GetFormFromString(formStr);
					if (!levItemForm) {
						logger::warn("Invalid Form: '{}'.", formStr);
						return;
					}

					RE::TESLevItem* levItem = levItemForm->As<RE::TESLevItem>();
					if (!levItem) {
						logger::warn("'{}' is not a Leveled Item.", formStr);
						return;
					}

					g_patchMap[weap].NPCAddAmmoList = levItem;
				}
			}
			else if (a_configData.Element == ElementType::kObjectEffect) {
				std::string formStr = std::any_cast<std::string>(a_configData.AssignValue.value());

				if (formStr == "null") {
					g_patchMap[weap].ObjectEffect = nullptr;
				}
				else {
					RE::TESForm* effectForm = Utils::GetFormFromString(formStr);
					if (!effectForm) {
						logger::warn("Invalid Form: '{}'.", formStr);
						return;
					}

					RE::EnchantmentItem* objectEffect = effectForm->As<RE::EnchantmentItem>();
					if (!objectEffect) {
						logger::warn("'{}' is not an Object Effect.", formStr);
						return;
					}

					g_patchMap[weap].ObjectEffect = objectEffect;
				}
			}
			else if (a_configData.Element == ElementType::kReach) {
				g_patchMap[weap].Reach = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kReloadSpeed) {
				g_patchMap[weap].ReloadSpeed = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kSpeed) {
				g_patchMap[weap].Speed = std::any_cast<float>(a_configData.AssignValue.value());
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
			if (patchData.second.Ammo.has_value()) {
				patchData.first->weaponData.ammo = patchData.second.Ammo.value();
			}
			if (patchData.second.AttackDelay.has_value()) {
				patchData.first->weaponData.attackDelaySec = patchData.second.AttackDelay.value();
			}
			if (patchData.second.MaxRange.has_value()) {
				patchData.first->weaponData.maxRange = patchData.second.MaxRange.value();
			}
			if (patchData.second.MinRange.has_value()) {
				patchData.first->weaponData.minRange = patchData.second.MinRange.value();
			}
			if (patchData.second.NPCAddAmmoList.has_value()) {
				patchData.first->weaponData.npcAddAmmoList = patchData.second.NPCAddAmmoList.value();
			}
			if (patchData.second.ObjectEffect.has_value()) {
				patchData.first->formEnchanting = patchData.second.ObjectEffect.value();
			}
			if (patchData.second.Reach.has_value()) {
				patchData.first->weaponData.reach = patchData.second.Reach.value();
			}
			if (patchData.second.ReloadSpeed.has_value()) {
				patchData.first->weaponData.reloadSpeed = patchData.second.ReloadSpeed.value();
			}
			if (patchData.second.Speed.has_value()) {
				patchData.first->weaponData.speed = patchData.second.Speed.value();
			}
		}

		logger::info("======================== Finished patching for {} ========================", TypeName);
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}
