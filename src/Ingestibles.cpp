#include "Ingestibles.h"

#include <regex>

#include "Configs.h"
#include "Utils.h"

namespace Ingestibles {
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

	std::vector<ConfigData> g_configVec;
	std::unordered_map<RE::AlchemyItem*, PatchData> g_patchMap;

	class IngestibleParser : public Configs::Parser<ConfigData> {
	public:
		IngestibleParser(Configs::ConfigReader& a_configReader) : Configs::Parser<ConfigData>(a_configReader) {}

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
			if (token == "Effects")
				a_configData.Element = ElementType::kEffects;
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
			else if (token == "Delete")
				opType = OperationType::kDelete;
			else {
				logger::warn("Line {}, Col {}: Invalid OperationName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			if (opType == OperationType::kClear || opType == OperationType::kAdd || opType == OperationType::kDelete) {
				if (a_configData.Element != ElementType::kEffects) {
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

			if (a_configData.Element == ElementType::kEffects) {
				ConfigData::Operation newOp{};
				newOp.OpType = opType;

				if (opType != OperationType::kClear) {
					newOp.OpEffectData = ConfigData::Operation::EffectData{};

					std::optional<std::string> opForm = parseForm();
					if (!opForm.has_value())
						return false;

					newOp.OpEffectData->EffectForm = opForm.value();

					token = reader.GetToken();
					if (token != ",") {
						logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
						return false;
					}

					token = reader.GetToken();
					if (token.empty() || token == ",") {
						logger::warn("Line {}, Col {}: Expected Magnitude '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
						return false;
					}

					std::string magStr = std::string(token);
					if (reader.LookAhead() == ".") {
						magStr += reader.GetToken();

						token = reader.GetToken();
						if (token.empty() || token == ",") {
							logger::warn("Line {}, Col {}: Expected Magnitude's decimal '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
							return false;
						}

						magStr += std::string(token);
					}

					float fParsedValue;
					try {
						fParsedValue = std::stof(magStr);
					}
					catch (...) {
						logger::warn("Line {}, Col {}: Failed to parse magnitude '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
						return false;
					}

					newOp.OpEffectData->Magnitude = fParsedValue;

					token = reader.GetToken();
					if (token != ",") {
						logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
						return false;
					}

					token = reader.GetToken();
					if (token.empty() || token == ",") {
						logger::warn("Line {}, Col {}: Expected Area '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
						return false;
					}

					unsigned long uParsedValue;
					auto parsingResult = std::from_chars(token.data(), token.data() + token.size(), uParsedValue);
					if (parsingResult.ec != std::errc()) {
						logger::warn("Line {}, Col {}: Failed to parse area '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
						return false;
					}

					newOp.OpEffectData->Area = static_cast<std::uint32_t>(uParsedValue);

					token = reader.GetToken();
					if (token != ",") {
						logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
						return false;
					}

					token = reader.GetToken();
					if (token.empty() || token == ")") {
						logger::warn("Line {}, Col {}: Expected Duration '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
						return false;
					}

					parsingResult = std::from_chars(token.data(), token.data() + token.size(), uParsedValue);
					if (parsingResult.ec != std::errc()) {
						logger::warn("Line {}, Col {}: Failed to parse duration '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
						return false;
					}

					newOp.OpEffectData->Duration = static_cast<std::uint32_t>(uParsedValue);
				}

				a_configData.Operations.push_back(newOp);
			}

			token = reader.GetToken();
			if (token != ")") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

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
			IngestibleParser parser(reader);
			auto configData = parser.Parse();
			if (!configData.has_value()) {
				parser.RecoverFromError();
				continue;
			}

			g_configVec.push_back(configData.value());

			if (configData->Element == ElementType::kEffects) {
				logger::info("{}({}).{}", FilterTypeToString(configData->Filter), configData->FilterForm, ElementTypeToString(configData->Element));
				for (std::size_t ii = 0; ii < configData->Operations.size(); ii++) {
					std::string opLog;

					if (configData->Operations[ii].OpType == OperationType::kClear)
						opLog = fmt::format(".{}()", OperationTypeToString(configData->Operations[ii].OpType));
					else
						opLog = fmt::format(".{}({}, {}, {}, {})", OperationTypeToString(configData->Operations[ii].OpType),
							configData->Operations[ii].OpEffectData->EffectForm,
							configData->Operations[ii].OpEffectData->Magnitude,
							configData->Operations[ii].OpEffectData->Area,
							configData->Operations[ii].OpEffectData->Duration);

					if (ii == configData->Operations.size() - 1)
						opLog += ";";

					logger::info("    {}", opLog);
				}
			}
		}
	}

	void ReadConfigs() {
		const std::filesystem::path configDir{ "Data\\" + std::string(Version::PROJECT) + "\\Ingestible" };
		const std::regex filter(".*\\.cfg", std::regex_constants::icase);
		const std::filesystem::directory_iterator dir_iter(configDir);
		for (auto& iter : dir_iter) {
			if (!std::filesystem::is_regular_file(iter.status()))
				continue;

			if (!std::regex_match(iter.path().filename().string(), filter))
				continue;

			std::string path = iter.path().string();
			logger::info("=========== Reading Ingestible config file: {} ===========", path);
			ReadConfig(path);
			logger::info("");
		}
	}

	void Prepare(const std::vector<ConfigData> a_configVec) {
		logger::info("======================== Start preparing patch for Ingestible ========================");

		for (const auto& configData : a_configVec) {
			if (configData.Filter == FilterType::kFormID) {
				RE::TESForm* filterForm = Utils::GetFormFromString(configData.FilterForm);
				if (!filterForm) {
					logger::warn("Invalid FilterForm: '{}'.", configData.FilterForm);
					continue;
				}

				RE::AlchemyItem* ingestibleForm = filterForm->As<RE::AlchemyItem>();
				if (!ingestibleForm) {
					logger::warn("'{}' is not a Ingestible.", configData.FilterForm);
					continue;
				}

				PatchData& patchData = g_patchMap[ingestibleForm];

				if (configData.Element == ElementType::kEffects) {
					if (!patchData.Effects.has_value())
						patchData.Effects = PatchData::EffectsData{};

					for (const auto& op : configData.Operations) {
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

							if (op.OpType == OperationType::kAdd)
								patchData.Effects->AddEffectVec.push_back({ effectSetting, op.OpEffectData->Magnitude, op.OpEffectData->Area, op.OpEffectData->Duration });
							else
								patchData.Effects->DeleteEffectVec.push_back({ effectSetting, op.OpEffectData->Magnitude, op.OpEffectData->Area, op.OpEffectData->Duration });
						}
					}
				}
			}
		}

		logger::info("======================== Finished preparing patch for Ingestible ========================");
		logger::info("");
	}

	std::vector<RE::EffectItem*> GetEffects(RE::AlchemyItem* a_alchemyItem) {
		std::vector<RE::EffectItem*> retVec;

		if (!a_alchemyItem || a_alchemyItem->listOfEffects.empty())
			return retVec;

		for (auto efItem : a_alchemyItem->listOfEffects)
			retVec.push_back(efItem);

		return retVec;
	}

	void SetEffects(RE::AlchemyItem* a_alchemyItem, const std::vector<RE::EffectItem*>& a_effects) {
		if (!a_alchemyItem)
			return;

		a_alchemyItem->listOfEffects.clear();

		if (a_effects.empty())
			return;

		for (auto effItem : a_effects)
			a_alchemyItem->listOfEffects.push_back(effItem);
	}

	RE::EffectItem* AllocEffect(const PatchData::EffectsData::Effect& a_effect) {
		RE::MemoryManager mm = RE::MemoryManager::GetSingleton();
		void* newData = mm.Allocate(sizeof(RE::EffectItem), 0, false);
		if (!newData) {
			logger::critical("Failed to allocate the new EffectItem.");
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
		if (!a_item)
			return;

		RE::MemoryManager mm = RE::MemoryManager::GetSingleton();
		mm.Deallocate(a_item, false);
	}

	void PatchEffects(RE::AlchemyItem* a_alchemyItem, const PatchData::EffectsData& a_effectsData) {
		bool isCleared = false, isDeleted = false, isAdded = false;

		std::vector<RE::EffectItem*> effectsVec = GetEffects(a_alchemyItem);
		// Clear
		if (a_effectsData.Clear) {
			isCleared = true;
			for (auto effItem : effectsVec)
				FreeEffect(effItem);
			effectsVec.clear();
		}

		// Delete
		if (!isCleared && !a_effectsData.DeleteEffectVec.empty()) {
			std::size_t preSize = effectsVec.size();

			for (const auto& delEffect : a_effectsData.DeleteEffectVec) {
				for (auto it = effectsVec.begin(); it != effectsVec.end(); it++) {
					RE::EffectItem* effItem = *it;
					if (delEffect.BaseEffect != effItem->effectSetting || delEffect.Magnitude != effItem->data.magnitude
						|| delEffect.Area != static_cast<std::uint32_t>(effItem->data.area) || delEffect.Duration != static_cast<std::uint32_t>(effItem->data.duration))
						continue;

					effectsVec.erase(it);
					FreeEffect(effItem);
					break;
				}
			}

			if (preSize != effectsVec.size())
				isDeleted = true;
		}

		// Add
		if (!a_effectsData.AddEffectVec.empty()) {
			for (const auto& addEffect : a_effectsData.AddEffectVec) {
				RE::EffectItem* newEffItem = AllocEffect(addEffect);
				if (newEffItem)
					effectsVec.push_back(newEffItem);
			}
			isAdded = true;
		}

		if (isCleared || isDeleted || isAdded)
			SetEffects(a_alchemyItem, effectsVec);
	}

	void Patch(RE::AlchemyItem* a_alchemyItem, const PatchData& a_patchData) {
		if (a_patchData.Effects.has_value())
			PatchEffects(a_alchemyItem, a_patchData.Effects.value());
	}

	void Patch() {
		Prepare(g_configVec);

		logger::info("======================== Start patching for Ingestible ========================");

		for (const auto& patchData : g_patchMap)
			Patch(patchData.first, patchData.second);

		logger::info("======================== Finished patching for Ingestible ========================");
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}
