#include "Ingestibles.h"

#include <regex>

#include "ConfigUtils.h"
#include "Parsers.h"
#include "Utils.h"

namespace Ingestibles {
	constexpr std::string_view TypeName = "Ingestible";

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
		kEffects
	};

	std::string_view ElementTypeToString(ElementType a_value) {
		switch (a_value) {
		case ElementType::kEffects: return "Effects";
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
			struct EffectData {
				std::string EffectForm;
				float Magnitude;
				std::uint32_t Area;
				std::uint32_t Duration;
			};

			OperationType OpType;
			std::optional<EffectData> OpEffectData;
		};

		FilterType Filter;
		std::string FilterForm;
		ElementType Element;
		std::vector<Operation> Operations;
	};

	struct PatchData {
		struct EffectsData {
			struct Effect {
				RE::EffectSetting* BaseEffect;
				float Magnitude;
				std::uint32_t Area;
				std::uint32_t Duration;
			};

			bool Clear;
			std::vector<Effect> AddEffectVec;
			std::vector<Effect> DeleteEffectVec;
		};

		std::optional<EffectsData> Effects;
	};

	std::vector<Parsers::Statement<ConfigData>> g_configVec;
	std::unordered_map<RE::AlchemyItem*, PatchData> g_patchMap;

	class IngestibleParser : public Parsers::Parser<ConfigData> {
	public:
		IngestibleParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

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

			return Parsers::Statement<ConfigData>::CreateExpressionStatement(configData);
		}

		void PrintExpressionStatement(const ConfigData& a_configData, int a_indent) override {
			std::string indent = std::string(a_indent * 4, ' ');

			switch (a_configData.Element) {
			case ElementType::kEffects:
				logger::info("{}{}({}).{}", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element));
				for (std::size_t opIndex = 0; opIndex < a_configData.Operations.size(); opIndex++) {
					std::string opLog;

					switch (a_configData.Operations[opIndex].OpType) {
					case OperationType::kClear:
						opLog = fmt::format(".{}()", OperationTypeToString(a_configData.Operations[opIndex].OpType));
						break;

					case OperationType::kAdd:
					case OperationType::kDelete:
						opLog = fmt::format(".{}({}, {}, {}, {})", OperationTypeToString(a_configData.Operations[opIndex].OpType),
							a_configData.Operations[opIndex].OpEffectData->EffectForm,
							a_configData.Operations[opIndex].OpEffectData->Magnitude,
							a_configData.Operations[opIndex].OpEffectData->Area,
							a_configData.Operations[opIndex].OpEffectData->Duration);
						break;
					}

					if (opIndex == a_configData.Operations.size() - 1) {
						opLog += ";";
					}

					logger::info("{}    {}", indent, opLog);
				}
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
			if (token == "Effects") {
				a_configData.Element = ElementType::kEffects;
			}
			else {
				logger::warn("Line {}, Col {}: Invalid ElementName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

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
			else {
				logger::warn("Line {}, Col {}: Invalid OperationName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			bool isValidOperation = [](ElementType elem, OperationType op) {
				if (elem == ElementType::kEffects) {
					return op == OperationType::kClear || op == OperationType::kAdd || op == OperationType::kDelete;
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

			if (a_configData.Element == ElementType::kEffects) {
				if (newOp.OpType != OperationType::kClear) {
					ConfigData::Operation::EffectData effectData{};

					std::optional<std::string> opForm = ParseForm();
					if (!opForm.has_value()) {
						return false;
					}

					effectData.EffectForm = opForm.value();

					token = reader.GetToken();
					if (token != ",") {
						logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
						return false;
					}

					auto parsedNumber = ParseNumber();
					if (!parsedNumber.has_value()) {
						return false;
					}

					effectData.Magnitude = parsedNumber.value();

					token = reader.GetToken();
					if (token != ",") {
						logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
						return false;
					}

					token = reader.GetToken();
					if (token.empty()) {
						logger::warn("Line {}, Col {}: Expected area.", reader.GetLastLine(), reader.GetLastLineIndex());
						return false;
					}

					unsigned long parsedValue;
					auto parseResult = std::from_chars(token.data(), token.data() + token.size(), parsedValue);
					if (parseResult.ec != std::errc()) {
						logger::warn("Line {}, Col {}: Failed to parse area '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
						return false;
					}

					effectData.Area = static_cast<std::uint32_t>(parsedValue);

					token = reader.GetToken();
					if (token != ",") {
						logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
						return false;
					}

					token = reader.GetToken();
					if (token.empty()) {
						logger::warn("Line {}, Col {}: Expected duration.", reader.GetLastLine(), reader.GetLastLineIndex());
						return false;
					}

					parseResult = std::from_chars(token.data(), token.data() + token.size(), parsedValue);
					if (parseResult.ec != std::errc()) {
						logger::warn("Line {}, Col {}: Failed to parse duration '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
						return false;
					}

					effectData.Duration = static_cast<std::uint32_t>(parsedValue);

					newOp.OpEffectData = effectData;
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
		g_configVec = ConfigUtils::ReadConfigs<IngestibleParser, Parsers::Statement<ConfigData>>(TypeName);
	}

	void Prepare(const ConfigData& a_configData) {
		if (a_configData.Filter == FilterType::kFormID) {
			RE::TESForm* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
			if (!filterForm) {
				logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
				return;
			}

			RE::AlchemyItem* ingestibleForm = filterForm->As<RE::AlchemyItem>();
			if (!ingestibleForm) {
				logger::warn("'{}' is not a Ingestible.", a_configData.FilterForm);
				return;
			}

			PatchData& patchData = g_patchMap[ingestibleForm];

			if (a_configData.Element == ElementType::kEffects) {
				if (!patchData.Effects.has_value()) {
					patchData.Effects = PatchData::EffectsData{};
				}

				for (const auto& op : a_configData.Operations) {
					if (op.OpType == OperationType::kClear) {
						patchData.Effects->Clear = true;
					}
					else if (op.OpType == OperationType::kAdd || op.OpType == OperationType::kDelete) {
						RE::TESForm* opForm = Utils::GetFormFromString(op.OpEffectData->EffectForm);
						if (!opForm) {
							logger::warn("Invalid Form: '{}'.", op.OpEffectData->EffectForm);
							continue;
						}

						RE::EffectSetting* effectSetting = opForm->As<RE::EffectSetting>();
						if (!effectSetting) {
							logger::warn("'{}' is not a Magic Effect.", op.OpEffectData->EffectForm);
							continue;
						}

						if (op.OpType == OperationType::kAdd) {
							patchData.Effects->AddEffectVec.push_back({ effectSetting, op.OpEffectData->Magnitude, op.OpEffectData->Area, op.OpEffectData->Duration });
						}
						else {
							patchData.Effects->DeleteEffectVec.push_back({ effectSetting, op.OpEffectData->Magnitude, op.OpEffectData->Area, op.OpEffectData->Duration });
						}
					}
				}
			}
		}
	}

	std::vector<RE::EffectItem*> GetEffects(RE::AlchemyItem* a_alchemyItem) {
		std::vector<RE::EffectItem*> retVec;

		if (!a_alchemyItem || a_alchemyItem->listOfEffects.empty()) {
			return retVec;
		}

		for (auto efItem : a_alchemyItem->listOfEffects) {
			retVec.push_back(efItem);
		}

		return retVec;
	}

	void SetEffects(RE::AlchemyItem* a_alchemyItem, const std::vector<RE::EffectItem*>& a_effects) {
		if (!a_alchemyItem) {
			return;
		}

		a_alchemyItem->listOfEffects.clear();

		if (a_effects.empty()) {
			return;
		}

		for (auto effItem : a_effects) {
			a_alchemyItem->listOfEffects.push_back(effItem);
		}
	}

	RE::EffectItem* AllocEffect(const PatchData::EffectsData::Effect& a_effect) {
		RE::MemoryManager& mm = RE::MemoryManager::GetSingleton();
		void* newData = mm.Allocate(sizeof(RE::EffectItem), 0, false);
		if (!newData) {
			logger::critical("Failed to allocate the Ingestible EffectItem.");
			return nullptr;
		}

		RE::EffectItem* retVal = static_cast<RE::EffectItem*>(newData);
		retVal->effectSetting = a_effect.BaseEffect;
		retVal->data.magnitude = a_effect.Magnitude;
		retVal->data.area = a_effect.Area;
		retVal->data.duration = a_effect.Duration;
		retVal->rawCost = 0.0f;
		retVal->conditions.head = nullptr;

		return retVal;
	}

	void FreeEffect(RE::EffectItem* a_item) {
		if (!a_item) {
			return;
		}

		RE::MemoryManager::GetSingleton().Deallocate(a_item, false);
	}

	void PatchEffects(RE::AlchemyItem* a_alchemyItem, const PatchData::EffectsData& a_effectsData) {
		bool isCleared = false, isDeleted = false, isAdded = false;

		std::vector<RE::EffectItem*> effectsVec = GetEffects(a_alchemyItem);

		// Clear
		if (a_effectsData.Clear) {
			for (auto effItem : effectsVec) {
				FreeEffect(effItem);
			}
			effectsVec.clear();
			isCleared = true;
		}

		// Delete
		if (!isCleared) {
			for (const auto& delEffect : a_effectsData.DeleteEffectVec) {
				for (auto it = effectsVec.begin(); it != effectsVec.end(); it++) {
					RE::EffectItem* effItem = *it;
					if (delEffect.BaseEffect == effItem->effectSetting && delEffect.Magnitude == effItem->data.magnitude && delEffect.Area == static_cast<std::uint32_t>(effItem->data.area) && delEffect.Duration == static_cast<std::uint32_t>(effItem->data.duration)) {
						effectsVec.erase(it);
						FreeEffect(effItem);
						isDeleted = true;
						break;
					}
				}
			}
		}

		// Add
		for (const auto& addEffect : a_effectsData.AddEffectVec) {
			RE::EffectItem* newEffItem = AllocEffect(addEffect);
			if (newEffItem) {
				effectsVec.push_back(newEffItem);
				isAdded = true;
			}
		}

		if (isCleared || isDeleted || isAdded) {
			SetEffects(a_alchemyItem, effectsVec);
		}
	}

	void Patch(RE::AlchemyItem* a_alchemyItem, const PatchData& a_patchData) {
		if (a_patchData.Effects.has_value()) {
			PatchEffects(a_alchemyItem, a_patchData.Effects.value());
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
