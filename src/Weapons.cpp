#include "Weapons.h"

#include <regex>

#include "Parsers.h"
#include "Utils.h"

namespace Weapons {
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
		kNPCAddAmmoList,
		kObjectEffect,
	};

	std::string_view ElementTypeToString(ElementType a_value) {
		switch (a_value) {
		case ElementType::kAmmo: return "Ammo";
		case ElementType::kNPCAddAmmoList: return "NPCAddAmmoList";
		case ElementType::kObjectEffect: return "ObjectEffect";
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
		std::optional<RE::TESAmmo*> Ammo;
		std::optional<RE::TESLevItem*> NPCAddAmmoList;
		std::optional<RE::EnchantmentItem*> ObjectEffect;
	};

	std::vector<Parsers::Statement<ConfigData>> g_configVec;
	std::unordered_map<RE::TESObjectWEAP*, PatchData> g_patchMap;

	class WeaponParser : public Parsers::Parser<ConfigData> {
	public:
		WeaponParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

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
			case ElementType::kAmmo:
			case ElementType::kNPCAddAmmoList:
			case ElementType::kObjectEffect:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm,
					ElementTypeToString(a_configData.Element), a_configData.AssignValue.value());
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
			if (token == "Ammo")
				a_config.Element = ElementType::kAmmo;
			else if (token == "NPCAddAmmoList")
				a_config.Element = ElementType::kNPCAddAmmoList;
			else if (token == "ObjectEffect")
				a_config.Element = ElementType::kObjectEffect;
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
					reader.GetToken();
					a_config.AssignValue = "null";
				}
				else {
					auto effectForm = ParseForm();
					if (!effectForm.has_value())
						return false;

					a_config.AssignValue = effectForm.value();
				}
			}
			else {
				logger::warn("Line {}, Col {}: Invalid Assignment to {}.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_config.Element));
				return false;
			}

			return true;
		}
	};

	void ReadConfig(std::string_view a_path) {
		WeaponParser parser(a_path);
		auto parsedStatements = parser.Parse();
		g_configVec.insert(g_configVec.end(), parsedStatements.begin(), parsedStatements.end());
	}

	void ReadConfigs() {
		const std::filesystem::path configDir{ "Data\\" + std::string(Version::PROJECT) + "\\Weapon" };
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
			logger::info("=========== Reading Weapon config file: {} ===========", path);
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

			RE::TESObjectWEAP* weap = filterForm->As<RE::TESObjectWEAP>();
			if (!weap) {
				logger::warn("'{}' is not a Weapon.", a_configData.FilterForm);
				return;
			}

			if (a_configData.Element == ElementType::kAmmo) {
				if (!a_configData.AssignValue.has_value())
					return;

				if (a_configData.AssignValue.value() == "null") {
					g_patchMap[weap].Ammo = nullptr;
				}
				else {
					RE::TESForm* ammoForm = Utils::GetFormFromString(a_configData.AssignValue.value());
					if (!ammoForm) {
						logger::warn("Invalid Form: '{}'.", a_configData.AssignValue.value());
						return;
					}

					RE::TESAmmo* ammo = ammoForm->As<RE::TESAmmo>();
					if (!ammo) {
						logger::warn("'{}' is not an Ammo.", a_configData.AssignValue.value());
						return;
					}

					g_patchMap[weap].Ammo = ammo;
				}
			}
			else if (a_configData.Element == ElementType::kNPCAddAmmoList) {
				if (!a_configData.AssignValue.has_value())
					return;

				if (a_configData.AssignValue.value() == "null") {
					g_patchMap[weap].NPCAddAmmoList = nullptr;
				}
				else {
					RE::TESForm* levItemForm = Utils::GetFormFromString(a_configData.AssignValue.value());
					if (!levItemForm) {
						logger::warn("Invalid Form: '{}'.", a_configData.AssignValue.value());
						return;
					}

					RE::TESLevItem* levItem = levItemForm->As<RE::TESLevItem>();
					if (!levItem) {
						logger::warn("'{}' is not a Leveled Item.", a_configData.AssignValue.value());
						return;
					}

					g_patchMap[weap].NPCAddAmmoList = levItem;
				}
			}
			else if (a_configData.Element == ElementType::kObjectEffect) {
				if (!a_configData.AssignValue.has_value())
					return;

				if (a_configData.AssignValue.value() == "null") {
					g_patchMap[weap].ObjectEffect = nullptr;
				}
				else {
					RE::TESForm* effectForm = Utils::GetFormFromString(a_configData.AssignValue.value());
					if (!effectForm) {
						logger::warn("Invalid Form: '{}'.", a_configData.AssignValue.value());
						return;
					}

					RE::EnchantmentItem* objectEffect = effectForm->As<RE::EnchantmentItem>();
					if (!objectEffect) {
						logger::warn("'{}' is not an Object Effect.", a_configData.AssignValue.value());
						return;
					}

					g_patchMap[weap].ObjectEffect = objectEffect;
				}
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
		logger::info("======================== Start preparing patch for Weapon ========================");

		Prepare(g_configVec);

		logger::info("======================== Finished preparing patch for Weapon ========================");
		logger::info("");

		logger::info("======================== Start patching for Weapon ========================");

		for (const auto& patchData : g_patchMap) {
			if (patchData.second.Ammo.has_value())
				patchData.first->weaponData.ammo = patchData.second.Ammo.value();
			if (patchData.second.NPCAddAmmoList.has_value())
				patchData.first->weaponData.npcAddAmmoList = patchData.second.NPCAddAmmoList.value();
			if (patchData.second.ObjectEffect.has_value())
				patchData.first->formEnchanting = patchData.second.ObjectEffect.value();
		}

		logger::info("======================== Finished patching for Weapon ========================");
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}
