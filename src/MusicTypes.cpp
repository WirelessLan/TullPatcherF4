#include "MusicTypes.h"

#include <regex>
#include <any>

#include "ConfigUtils.h"
#include "Parsers.h"
#include "Utils.h"

namespace MusicTypes {
	constexpr std::string_view TypeName = "MusicType";

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
		kDucking,
		kFadeDuration,
		kFlags,
		kMusicTracks,
		kPriority
	};

	std::string_view ElementTypeToString(ElementType a_value) {
		switch (a_value) {
		case ElementType::kDucking: return "Ducking";
		case ElementType::kFadeDuration: return "FadeDuration";
		case ElementType::kFlags: return "Flags";
		case ElementType::kMusicTracks: return "MusicTracks";
		case ElementType::kPriority: return "Priority";
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
			OperationType OpType;
			std::optional<std::string> OpForm;
		};

		FilterType Filter;
		std::string FilterForm;
		ElementType Element;
		std::vector<Operation> Operations;
		std::optional<std::any> AssignValue;
	};

	struct PatchData {
		struct MusicTracksData {
			bool Clear = false;
			std::vector<RE::BGSMusicTrackFormWrapper*> AddTrackVec;
			std::vector<RE::BGSMusicTrackFormWrapper*> DeleteTrackVec;
		};

		std::optional<std::uint16_t> Ducking;
		std::optional<float> FadeDuration;
		std::optional<std::uint32_t> Flags;
		std::optional<MusicTracksData> MusicTracks;
		std::optional<std::uint8_t> Priority;
	};

	std::vector<Parsers::Statement<ConfigData>> g_configVec;
	std::unordered_map<RE::BGSMusicType*, PatchData> g_patchMap;

	class MusicTypeParser : public Parsers::Parser<ConfigData> {
	public:
		MusicTypeParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

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

			token = reader.Peek();
			if (token == "=") {
				if (!ParseAssignment(configData)) {
					return std::nullopt;
				}

				token = reader.GetToken();
				if (token != ";") {
					logger::warn("Line {}, Col {}: Syntax error. Expected ';'.", reader.GetLastLine(), reader.GetLastLineIndex());
					return std::nullopt;
				}
			}
			else {
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
			}

			return Parsers::Statement<ConfigData>::CreateExpressionStatement(configData);
		}

		void PrintExpressionStatement(const ConfigData& a_configData, int a_indent) override {
			std::string indent = std::string(a_indent * 4, ' ');

			switch (a_configData.Element) {
			case ElementType::kDucking:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element),
					static_cast<float>(std::any_cast<std::uint16_t>(a_configData.AssignValue.value())) / 100.0f);
				break;

			case ElementType::kFadeDuration:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element),
					std::any_cast<float>(a_configData.AssignValue.value()));
				break;

			case ElementType::kFlags:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element),
					GetFlags(std::any_cast<std::uint32_t>(a_configData.AssignValue.value())));
				break;

			case ElementType::kMusicTracks:
				logger::info("{}{}({}).{}", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element));
				for (std::size_t ii = 0; ii < a_configData.Operations.size(); ii++) {
					std::string opLog;

					switch (a_configData.Operations[ii].OpType) {
					case OperationType::kClear:
						opLog = fmt::format(".{}()", OperationTypeToString(a_configData.Operations[ii].OpType));
						break;

					case OperationType::kAdd:
					case OperationType::kDelete:
						opLog = fmt::format(".{}({})", OperationTypeToString(a_configData.Operations[ii].OpType), a_configData.Operations[ii].OpForm.value());
						break;
					}

					if (ii == a_configData.Operations.size() - 1) {
						opLog += ";";
					}

					logger::info("{}    {}", indent, opLog);
				}
				break;

			case ElementType::kPriority:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element),
					std::any_cast<std::uint8_t>(a_configData.AssignValue.value()));
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
			if (token == "Ducking") {
				a_configData.Element = ElementType::kDucking;
			}
			else if (token == "FadeDuration") {
				a_configData.Element = ElementType::kFadeDuration;
			}
			else if (token == "Flags") {
				a_configData.Element = ElementType::kFlags;
			}
			else if (token == "MusicTracks") {
				a_configData.Element = ElementType::kMusicTracks;
			}
			else if (token == "Priority") {
				a_configData.Element = ElementType::kPriority;
			}
			else {
				logger::warn("Line {}, Col {}: Invalid ElementName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			return true;
		}

		bool ParseAssignment(ConfigData& a_configData) {
			auto token = reader.GetToken();
			if (token != "=") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '='.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			if (a_configData.Element == ElementType::kDucking) {
				token = reader.GetToken();
				if (token.empty() || token == ";") {
					logger::warn("Line {}, Col {}: Expected value '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				std::string numStr = std::string(token);
				if (reader.Peek() == ".") {
					numStr += reader.GetToken();

					token = reader.GetToken();
					if (token.empty() || token == ",") {
						logger::warn("Line {}, Col {}: Expected decimal value '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
						return false;
					}

					numStr += std::string(token);
				}

				if (!Utils::IsValidDecimalNumber(numStr)) {
					logger::warn("Line {}, Col {}: Failed to parse value '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), numStr);
					return false;
				}

				float parsedValue;
				auto parsingResult = std::from_chars(numStr.data(), numStr.data() + numStr.size(), parsedValue);
				if (parsingResult.ec != std::errc()) {
					logger::warn("Line {}, Col {}: Failed to parse value '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), numStr);
					return false;
				}

				if (parsedValue < 0.0f || parsedValue > 655.35f) {
					logger::warn("Line {}, Col {}: Failed to parse value '{}'. The value is out of range", reader.GetLastLine(), reader.GetLastLineIndex(), parsedValue);
					return false;
				}

				a_configData.AssignValue = std::any(static_cast<std::uint16_t>(std::round(parsedValue * 100.0f)));
			}
			else if (a_configData.Element == ElementType::kFadeDuration) {
				token = reader.GetToken();
				if (token.empty() || token == ";") {
					logger::warn("Line {}, Col {}: Expected value '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				std::string numStr = std::string(token);
				if (reader.Peek() == ".") {
					numStr += reader.GetToken();

					token = reader.GetToken();
					if (token.empty() || token == ",") {
						logger::warn("Line {}, Col {}: Expected decimal value '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
						return false;
					}

					numStr += std::string(token);
				}

				if (!Utils::IsValidDecimalNumber(numStr)) {
					logger::warn("Line {}, Col {}: Failed to parse value '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), numStr);
					return false;
				}

				float parsedValue;
				auto parsingResult = std::from_chars(numStr.data(), numStr.data() + numStr.size(), parsedValue);
				if (parsingResult.ec != std::errc()) {
					logger::warn("Line {}, Col {}: Failed to parse value '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), numStr);
					return false;
				}

				a_configData.AssignValue = std::any(parsedValue);
			}
			else if (a_configData.Element == ElementType::kFlags) {
				std::uint32_t flagValue = 0;

				auto flag = ParseFlag();
				if (!flag.has_value()) {
					return false;
				}

				flagValue |= flag.value();

				while (true) {
					token = reader.Peek();
					if (token == ";") {
						break;
					}

					token = reader.GetToken();
					if (token != "|") {
						logger::warn("Line {}, Col {}: Syntax error. Expected '|' or ';'.", reader.GetLastLine(), reader.GetLastLineIndex());
						return false;
					}

					flag = ParseFlag();
					if (!flag.has_value()) {
						return false;
					}

					flagValue |= flag.value();
				}

				a_configData.AssignValue = std::any(flagValue);
			}
			else if (a_configData.Element == ElementType::kPriority) {
				token = reader.GetToken();
				if (token.empty() || token == ";") {
					logger::warn("Line {}, Col {}: Expected value '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				unsigned long parsedValue;
				auto parsingResult = std::from_chars(token.data(), token.data() + token.size(), parsedValue);
				if (parsingResult.ec != std::errc()) {
					logger::warn("Line {}, Col {}: Failed to parse value '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				if (parsedValue > 0xFF) {
					logger::warn("Line {}, Col {}: Failed to parse value '{}'. The value is out of range", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				a_configData.AssignValue = std::any(static_cast<std::uint8_t>(parsedValue));
			}
			else {
				logger::warn("Line {}, Col {}: Invalid Assignment '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_configData.Element));
				return false;
			}

			return true;
		}

		bool ParseOperation(ConfigData& a_configData) {
			ConfigData::Operation newOp;

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

			switch (a_configData.Element) {
			case ElementType::kMusicTracks:
				switch (newOp.OpType) {
				case OperationType::kClear:
				case OperationType::kAdd:
				case OperationType::kDelete:
					break;

				default:
					logger::warn("Line {}, Col {}: Invalid Operation '{}.{}()'.",
						reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_configData.Element), OperationTypeToString(newOp.OpType));
					return false;
				}
				break;
			}

			token = reader.GetToken();
			if (token != "(") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '('.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			switch (a_configData.Element) {
			case ElementType::kMusicTracks:
				if (newOp.OpType != OperationType::kClear) {
					std::optional<std::string> form = ParseForm();
					if (!form.has_value()) {
						return false;
					}

					newOp.OpForm = form.value();
				}

				break;
			}

			token = reader.GetToken();
			if (token != ")") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			a_configData.Operations.push_back(newOp);

			return true;
		}

		std::optional<std::uint32_t> ParseFlag() {
			auto token = reader.GetToken();
			if (token.empty() || token == "|" || token == ";") {
				logger::warn("Line {}, Col {}: Expected flag name '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return std::nullopt;
			}

			if (token == "None") {
				return 0x0000;
			}
			else if (token == "PlaysOneSelection") {
				return 0x0001;
			}
			else if (token == "AbruptTransition") {
				return 0x0002;
			}
			else if (token == "CycleTracks") {
				return 0x0004;
			}
			else if (token == "MaintainTrackOrder") {
				return 0x0008;
			}
			else if (token == "DucksCurrentTrack") {
				return 0x0020;
			}
			else if (token == "DoesNotQueue") {
				return 0x0040;
			}
			else {
				logger::warn("Line {}, Col {}: Invalid flag name '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return std::nullopt;
			}
		}

		std::string GetFlags(std::uint32_t a_flags) {
			std::string retStr;
			std::string separtor = " | ";

			if (a_flags & 0x0001) {
				retStr += "PlaysOneSelection" + separtor;
			}
			if (a_flags & 0x0002) {
				retStr += "AbruptTransition" + separtor;
			}
			if (a_flags & 0x0004) {
				retStr += "CycleTracks" + separtor;
			}
			if (a_flags & 0x0008) {
				retStr += "MaintainTrackOrder" + separtor;
			}
			if (a_flags & 0x0020) {
				retStr += "DucksCurrentTrack" + separtor;
			}
			if (a_flags & 0x0040) {
				retStr += "DoesNotQueue" + separtor;
			}

			if (retStr.empty()) {
				return "None";
			}

			return retStr.substr(0, retStr.size() - separtor.size());
		}
	};

	void ReadConfigs() {
		g_configVec = ConfigUtils::ReadConfigs<MusicTypeParser, Parsers::Statement<ConfigData>>(TypeName);
	}

	void Prepare(const ConfigData& a_configData) {
		if (a_configData.Filter == FilterType::kFormID) {
			RE::TESForm* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
			if (!filterForm) {
				logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
				return;
			}

			RE::BGSMusicType* musicType = filterForm->As<RE::BGSMusicType>();
			if (!musicType) {
				logger::warn("'{}' is not a MusicType.", a_configData.FilterForm);
				return;
			}

			PatchData& patchData = g_patchMap[musicType];

			if (a_configData.Element == ElementType::kDucking) {
				patchData.Ducking = std::any_cast<std::uint16_t>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kFadeDuration) {
				patchData.FadeDuration = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kFlags) {
				patchData.Flags = std::any_cast<std::uint32_t>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kMusicTracks) {
				if (!patchData.MusicTracks.has_value()) {
					patchData.MusicTracks = PatchData::MusicTracksData{};
				}

				for (const auto& op : a_configData.Operations) {
					if (op.OpType == OperationType::kClear) {
						patchData.MusicTracks->Clear = true;
					}
					else if (op.OpType == OperationType::kAdd || op.OpType == OperationType::kDelete) {
						RE::TESForm* opForm = Utils::GetFormFromString(op.OpForm.value());
						if (!opForm) {
							logger::warn("Invalid Form: '{}'.", op.OpForm.value());
							continue;
						}

						RE::BGSMusicTrackFormWrapper* musicTrack = opForm->As<RE::BGSMusicTrackFormWrapper>();
						if (!musicTrack) {
							logger::warn("'{}' is not a MusicTrack.", op.OpForm.value());
							continue;
						}

						if (op.OpType == OperationType::kAdd) {
							patchData.MusicTracks->AddTrackVec.push_back(musicTrack);
						}
						else {
							patchData.MusicTracks->DeleteTrackVec.push_back(musicTrack);
						}
					}
				}
			}
			else if (a_configData.Element == ElementType::kPriority) {
				patchData.Priority = std::any_cast<std::uint8_t>(a_configData.AssignValue.value());
			}
		}
	}

	void PatchMusicTracks(RE::BGSMusicType* a_musicType, const PatchData::MusicTracksData& a_musicTracksData) {
		bool isCleared = false;

		// Clear
		if (a_musicTracksData.Clear) {
			a_musicType->tracks.clear();
			isCleared = true;
		}

		class RE::BGSMusicTrack;

		// Delete
		if (!isCleared) {
			for (const auto& delForm : a_musicTracksData.DeleteTrackVec) {
				for (auto it = a_musicType->tracks.begin(); it != a_musicType->tracks.end(); it++) {
					RE::BGSMusicTrackFormWrapper* musicTrack = RE::fallout_cast<RE::BGSMusicTrackFormWrapper*, RE::BSIMusicTrack>(*it);
					if (musicTrack && musicTrack == delForm) {
						a_musicType->tracks.erase(it);
						break;
					}
				}
			}
		}

		// Add
		for (const auto& addForm : a_musicTracksData.AddTrackVec) {
			RE::BSIMusicTrack* musicTrack = addForm->As<RE::BSIMusicTrack>();
			if (musicTrack) {
				a_musicType->tracks.push_back(musicTrack);
			}
		}
	}

	void Patch(RE::BGSMusicType* a_musicType, const PatchData& a_patchData) {
		if (a_patchData.Ducking.has_value()) {
			a_musicType->ducksOtherMusicBy = a_patchData.Ducking.value();
		}
		if (a_patchData.FadeDuration.has_value()) {
			a_musicType->fadeTime = a_patchData.FadeDuration.value();
		}
		if (a_patchData.Flags.has_value()) {
			a_musicType->flags = a_patchData.Flags.value();
		}
		if (a_patchData.MusicTracks.has_value()) {
			PatchMusicTracks(a_musicType, a_patchData.MusicTracks.value());
		}
		if (a_patchData.Priority.has_value()) {
			a_musicType->priority = static_cast<std::int8_t>(a_patchData.Priority.value());
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
