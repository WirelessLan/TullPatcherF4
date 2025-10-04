#include "CombatStyles.h"

#include <any>
#include <regex>

#include "ConfigUtils.h"

namespace CombatStyles
{
	constexpr std::string_view TypeName = "CombatStyle";

	enum class FilterType
	{
		kFormID
	};

	std::string_view FilterTypeToString(FilterType a_value)
	{
		switch (a_value) {
		case FilterType::kFormID:
			return "FilterByFormID";
		default:
			return std::string_view{};
		}
	}

	enum class ElementType
	{
		kOffensiveMult,
		kDefensiveMult,
		kGroupOffensiveMult,
		kMeleeEquipmentScoreMult,
		kMagicEquipmentScoreMult,
		kRangedEquipmentScoreMult,
		kShoutEquipmentScoreMult,
		kUnarmedEquipmentScoreMult,
		kStaffEquipmentScoreMult,
		kAvoidThreatChance,
		kDodgeThreatChance,
		kEvadeThreatChance,
		kAttackStaggeredMult,
		kPowerAttackStaggeredMult,
		kPowerAttackBlockingMult,
		kBashMult,
		kBashRecoilMult,
		kBashAttackMult,
		kBashPowerAttackMult,
		kSpecialAttackMult,
		kBlockWhenStaggeredMult,
		kAttackWhenStaggeredMult,
		kRangedAccuracyMult,
		kCircleMult,
		kFallbackMult,
		kFlankDistance,
		kStalkTime,
		kChargeDistance,
		kThrowProbability,
		kSprintFastProbability,
		kSideswipeProbability,
		kDisengageProbability,
		kThrowMaxTargets,
		kFlankVariance,
		kStrafeMult,
		kAdjustRangeMult,
		kCrouchMult,
		kWaitMult,
		kRangeMult,
		kCoverSearchDistanceMult,
		kHoverChance,
		kDiveBombChance,
		kGroundAttackChance,
		kHoverTime,
		kGroundAttackTime,
		kPerchAttackChance,
		kPerchAttackTime,
		kFlyingAttackChance,
		kFlags
	};

	std::string_view ElementTypeToString(ElementType a_value)
	{
		switch (a_value) {
		case ElementType::kOffensiveMult:
			return "OffensiveMult";
		case ElementType::kDefensiveMult:
			return "DefensiveMult";
		case ElementType::kGroupOffensiveMult:
			return "GroupOffensiveMult";
		case ElementType::kMeleeEquipmentScoreMult:
			return "MeleeEquipmentScoreMult";
		case ElementType::kMagicEquipmentScoreMult:
			return "MagicEquipmentScoreMult";
		case ElementType::kRangedEquipmentScoreMult:
			return "RangedEquipmentScoreMult";
		case ElementType::kShoutEquipmentScoreMult:
			return "ShoutEquipmentScoreMult";
		case ElementType::kUnarmedEquipmentScoreMult:
			return "UnarmedEquipmentScoreMult";
		case ElementType::kStaffEquipmentScoreMult:
			return "StaffEquipmentScoreMult";
		case ElementType::kAvoidThreatChance:
			return "AvoidThreatChance";
		case ElementType::kDodgeThreatChance:
			return "DodgeThreatChance";
		case ElementType::kEvadeThreatChance:
			return "EvadeThreatChance";
		case ElementType::kAttackStaggeredMult:
			return "AttackStaggeredMult";
		case ElementType::kPowerAttackStaggeredMult:
			return "PowerAttackStaggeredMult";
		case ElementType::kPowerAttackBlockingMult:
			return "PowerAttackBlockingMult";
		case ElementType::kBashMult:
			return "BashMult";
		case ElementType::kBashRecoilMult:
			return "BashRecoilMult";
		case ElementType::kBashAttackMult:
			return "BashAttackMult";
		case ElementType::kBashPowerAttackMult:
			return "BashPowerAttackMult";
		case ElementType::kSpecialAttackMult:
			return "SpecialAttackMult";
		case ElementType::kBlockWhenStaggeredMult:
			return "BlockWhenStaggeredMult";
		case ElementType::kAttackWhenStaggeredMult:
			return "AttackWhenStaggeredMult";
		case ElementType::kRangedAccuracyMult:
			return "RangedAccuracyMult";
		case ElementType::kCircleMult:
			return "CircleMult";
		case ElementType::kFallbackMult:
			return "FallbackMult";
		case ElementType::kFlankDistance:
			return "FlankDistance";
		case ElementType::kStalkTime:
			return "StalkTime";
		case ElementType::kChargeDistance:
			return "ChargeDistance";
		case ElementType::kThrowProbability:
			return "ThrowProbability";
		case ElementType::kSprintFastProbability:
			return "SprintFastProbability";
		case ElementType::kSideswipeProbability:
			return "SideswipeProbability";
		case ElementType::kDisengageProbability:
			return "DisengageProbability";
		case ElementType::kThrowMaxTargets:
			return "ThrowMaxTargets";
		case ElementType::kFlankVariance:
			return "FlankVariance";
		case ElementType::kStrafeMult:
			return "StrafeMult";
		case ElementType::kAdjustRangeMult:
			return "AdjustRangeMult";
		case ElementType::kCrouchMult:
			return "CrouchMult";
		case ElementType::kWaitMult:
			return "WaitMult";
		case ElementType::kRangeMult:
			return "RangeMult";
		case ElementType::kCoverSearchDistanceMult:
			return "CoverSearchDistanceMult";
		case ElementType::kHoverChance:
			return "HoverChance";
		case ElementType::kDiveBombChance:
			return "DiveBombChance";
		case ElementType::kGroundAttackChance:
			return "GroundAttackChance";
		case ElementType::kHoverTime:
			return "HoverTime";
		case ElementType::kGroundAttackTime:
			return "GroundAttackTime";
		case ElementType::kPerchAttackChance:
			return "PerchAttackChance";
		case ElementType::kPerchAttackTime:
			return "PerchAttackTime";
		case ElementType::kFlyingAttackChance:
			return "FlyingAttackChance";
		case ElementType::kFlags:
			return "Flags";
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
		std::optional<float> OffensiveMult;
		std::optional<float> DefensiveMult;
		std::optional<float> GroupOffensiveMult;
		std::optional<float> MeleeEquipmentScoreMult;
		std::optional<float> MagicEquipmentScoreMult;
		std::optional<float> RangedEquipmentScoreMult;
		std::optional<float> ShoutEquipmentScoreMult;
		std::optional<float> UnarmedEquipmentScoreMult;
		std::optional<float> StaffEquipmentScoreMult;
		std::optional<float> AvoidThreatChance;
		std::optional<float> DodgeThreatChance;
		std::optional<float> EvadeThreatChance;
		std::optional<float> AttackStaggeredMult;
		std::optional<float> PowerAttackStaggeredMult;
		std::optional<float> PowerAttackBlockingMult;
		std::optional<float> BashMult;
		std::optional<float> BashRecoilMult;
		std::optional<float> BashAttackMult;
		std::optional<float> BashPowerAttackMult;
		std::optional<float> SpecialAttackMult;
		std::optional<float> BlockWhenStaggeredMult;
		std::optional<float> AttackWhenStaggeredMult;
		std::optional<float> RangedAccuracyMult;
		std::optional<float> CircleMult;
		std::optional<float> FallbackMult;
		std::optional<float> FlankDistance;
		std::optional<float> StalkTime;
		std::optional<float> ChargeDistance;
		std::optional<float> ThrowProbability;
		std::optional<float> SprintFastProbability;
		std::optional<float> SideswipeProbability;
		std::optional<float> DisengageProbability;
		std::optional<std::uint32_t> ThrowMaxTargets;
		std::optional<float> FlankVariance;
		std::optional<float> StrafeMult;
		std::optional<float> AdjustRangeMult;
		std::optional<float> CrouchMult;
		std::optional<float> WaitMult;
		std::optional<float> RangeMult;
		std::optional<float> CoverSearchDistanceMult;
		std::optional<float> HoverChance;
		std::optional<float> DiveBombChance;
		std::optional<float> GroundAttackChance;
		std::optional<float> HoverTime;
		std::optional<float> GroundAttackTime;
		std::optional<float> PerchAttackChance;
		std::optional<float> PerchAttackTime;
		std::optional<float> FlyingAttackChance;
		std::optional<std::uint32_t> Flags;
	};

	std::vector<Parsers::Statement<ConfigData>> g_configVec;
	std::unordered_map<RE::TESCombatStyle*, PatchData> g_patchMap;

	class CombatStyleParser : public Parsers::Parser<ConfigData>
	{
	public:
		CombatStyleParser(std::string_view a_configPath) : Parsers::Parser<ConfigData>(a_configPath) {}

	protected:
		std::optional<Parsers::Statement<ConfigData>> ParseExpressionStatement() override
		{
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

			return Parsers::Statement<ConfigData>{ .Type = Parsers::StatementType::kExpression, .ExpressionStatement = configData  };
		}

		void PrintExpressionStatement(const ConfigData& a_configData, int a_indent) override
		{
			std::string indent = std::string(a_indent * 4, ' ');
			switch (a_configData.Element) {
			case ElementType::kOffensiveMult:
			case ElementType::kDefensiveMult:
			case ElementType::kGroupOffensiveMult:
			case ElementType::kMeleeEquipmentScoreMult:
			case ElementType::kMagicEquipmentScoreMult:
			case ElementType::kRangedEquipmentScoreMult:
			case ElementType::kShoutEquipmentScoreMult:
			case ElementType::kUnarmedEquipmentScoreMult:
			case ElementType::kStaffEquipmentScoreMult:
			case ElementType::kAvoidThreatChance:
			case ElementType::kDodgeThreatChance:
			case ElementType::kEvadeThreatChance:
			case ElementType::kAttackStaggeredMult:
			case ElementType::kPowerAttackStaggeredMult:
			case ElementType::kPowerAttackBlockingMult:
			case ElementType::kBashMult:
			case ElementType::kBashRecoilMult:
			case ElementType::kBashAttackMult:
			case ElementType::kBashPowerAttackMult:
			case ElementType::kSpecialAttackMult:
			case ElementType::kBlockWhenStaggeredMult:
			case ElementType::kAttackWhenStaggeredMult:
			case ElementType::kRangedAccuracyMult:
			case ElementType::kCircleMult:
			case ElementType::kFallbackMult:
			case ElementType::kFlankDistance:
			case ElementType::kStalkTime:
			case ElementType::kChargeDistance:
			case ElementType::kThrowProbability:
			case ElementType::kSprintFastProbability:
			case ElementType::kSideswipeProbability:
			case ElementType::kDisengageProbability:
			case ElementType::kFlankVariance:
			case ElementType::kStrafeMult:
			case ElementType::kAdjustRangeMult:
			case ElementType::kCrouchMult:
			case ElementType::kWaitMult:
			case ElementType::kRangeMult:
			case ElementType::kCoverSearchDistanceMult:
			case ElementType::kHoverChance:
			case ElementType::kDiveBombChance:
			case ElementType::kGroundAttackChance:
			case ElementType::kHoverTime:
			case ElementType::kGroundAttackTime:
			case ElementType::kPerchAttackChance:
			case ElementType::kPerchAttackTime:
			case ElementType::kFlyingAttackChance:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element), std::any_cast<float>(a_configData.AssignValue.value()));
				break;

			case ElementType::kThrowMaxTargets:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element), std::any_cast<std::uint32_t>(a_configData.AssignValue.value()));
				break;

			case ElementType::kFlags:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element), GetFlags(std::any_cast<std::uint32_t>(a_configData.AssignValue.value())));
				break;
			}
		}

		bool ParseFilter(ConfigData& a_configData)
		{
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
			if (token == "OffensiveMult") {
				a_configData.Element = ElementType::kOffensiveMult;
			}
			else if (token == "DefensiveMult") {
				a_configData.Element = ElementType::kDefensiveMult;
			}
			else if (token == "GroupOffensiveMult") {
				a_configData.Element = ElementType::kGroupOffensiveMult;
			}
			else if (token == "MeleeEquipmentScoreMult") {
				a_configData.Element = ElementType::kMeleeEquipmentScoreMult;
			}
			else if (token == "MagicEquipmentScoreMult") {
				a_configData.Element = ElementType::kMagicEquipmentScoreMult;
			}
			else if (token == "RangedEquipmentScoreMult") {
				a_configData.Element = ElementType::kRangedEquipmentScoreMult;
			}
			else if (token == "ShoutEquipmentScoreMult") {
				a_configData.Element = ElementType::kShoutEquipmentScoreMult;
			}
			else if (token == "UnarmedEquipmentScoreMult") {
				a_configData.Element = ElementType::kUnarmedEquipmentScoreMult;
			}
			else if (token == "StaffEquipmentScoreMult") {
				a_configData.Element = ElementType::kStaffEquipmentScoreMult;
			}
			else if (token == "AvoidThreatChance") {
				a_configData.Element = ElementType::kAvoidThreatChance;
			}
			else if (token == "DodgeThreatChance") {
				a_configData.Element = ElementType::kDodgeThreatChance;
			}
			else if (token == "EvadeThreatChance") {
				a_configData.Element = ElementType::kEvadeThreatChance;
			}
			else if (token == "AttackStaggeredMult") {
				a_configData.Element = ElementType::kAttackStaggeredMult;
			}
			else if (token == "PowerAttackStaggeredMult") {
				a_configData.Element = ElementType::kPowerAttackStaggeredMult;
			}
			else if (token == "PowerAttackBlockingMult") {
				a_configData.Element = ElementType::kPowerAttackBlockingMult;
			}
			else if (token == "BashMult") {
				a_configData.Element = ElementType::kBashMult;
			}
			else if (token == "BashRecoilMult") {
				a_configData.Element = ElementType::kBashRecoilMult;
			}
			else if (token == "BashAttackMult") {
				a_configData.Element = ElementType::kBashAttackMult;
			}
			else if (token == "BashPowerAttackMult") {
				a_configData.Element = ElementType::kBashPowerAttackMult;
			}
			else if (token == "SpecialAttackMult") {
				a_configData.Element = ElementType::kSpecialAttackMult;
			}
			else if (token == "BlockWhenStaggeredMult") {
				a_configData.Element = ElementType::kBlockWhenStaggeredMult;
			}
			else if (token == "AttackWhenStaggeredMult") {
				a_configData.Element = ElementType::kAttackWhenStaggeredMult;
			}
			else if (token == "RangedAccuracyMult") {
				a_configData.Element = ElementType::kRangedAccuracyMult;
			}
			else if (token == "CircleMult") {
				a_configData.Element = ElementType::kCircleMult;
			}
			else if (token == "FallbackMult") {
				a_configData.Element = ElementType::kFallbackMult;
			}
			else if (token == "FlankDistance") {
				a_configData.Element = ElementType::kFlankDistance;
			}
			else if (token == "StalkTime") {
				a_configData.Element = ElementType::kStalkTime;
			}
			else if (token == "ChargeDistance") {
				a_configData.Element = ElementType::kChargeDistance;
			}
			else if (token == "ThrowProbability") {
				a_configData.Element = ElementType::kThrowProbability;
			}
			else if (token == "SprintFastProbability") {
				a_configData.Element = ElementType::kSprintFastProbability;
			}
			else if (token == "SideswipeProbability") {
				a_configData.Element = ElementType::kSideswipeProbability;
			}
			else if (token == "DisengageProbability") {
				a_configData.Element = ElementType::kDisengageProbability;
			}
			else if (token == "ThrowMaxTargets") {
				a_configData.Element = ElementType::kThrowMaxTargets;
			}
			else if (token == "FlankVariance") {
				a_configData.Element = ElementType::kFlankVariance;
			}
			else if (token == "StrafeMult") {
				a_configData.Element = ElementType::kStrafeMult;
			}
			else if (token == "AdjustRangeMult") {
				a_configData.Element = ElementType::kAdjustRangeMult;
			}
			else if (token == "CrouchMult") {
				a_configData.Element = ElementType::kCrouchMult;
			}
			else if (token == "WaitMult") {
				a_configData.Element = ElementType::kWaitMult;
			}
			else if (token == "RangeMult") {
				a_configData.Element = ElementType::kRangeMult;
			}
			else if (token == "CoverSearchDistanceMult") {
				a_configData.Element = ElementType::kCoverSearchDistanceMult;
			}
			else if (token == "HoverChance") {
				a_configData.Element = ElementType::kHoverChance;
			}
			else if (token == "DiveBombChance") {
				a_configData.Element = ElementType::kDiveBombChance;
			}
			else if (token == "GroundAttackChance") {
				a_configData.Element = ElementType::kGroundAttackChance;
			}
			else if (token == "HoverTime") {
				a_configData.Element = ElementType::kHoverTime;
			}
			else if (token == "GroundAttackTime") {
				a_configData.Element = ElementType::kGroundAttackTime;
			}
			else if (token == "PerchAttackChance") {
				a_configData.Element = ElementType::kPerchAttackChance;
			}
			else if (token == "PerchAttackTime") {
				a_configData.Element = ElementType::kPerchAttackTime;
			}
			else if (token == "FlyingAttackChance") {
				a_configData.Element = ElementType::kFlyingAttackChance;
			}
			else if (token == "Flags") {
				a_configData.Element = ElementType::kFlags;
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

			if (a_configData.Element == ElementType::kOffensiveMult || 
				a_configData.Element == ElementType::kDefensiveMult ||
				a_configData.Element == ElementType::kGroupOffensiveMult ||
				a_configData.Element == ElementType::kMeleeEquipmentScoreMult ||
				a_configData.Element == ElementType::kMagicEquipmentScoreMult ||
				a_configData.Element == ElementType::kRangedEquipmentScoreMult ||
				a_configData.Element == ElementType::kShoutEquipmentScoreMult ||
				a_configData.Element == ElementType::kUnarmedEquipmentScoreMult ||
				a_configData.Element == ElementType::kStaffEquipmentScoreMult ||
				a_configData.Element == ElementType::kAvoidThreatChance ||
				a_configData.Element == ElementType::kDodgeThreatChance ||
				a_configData.Element == ElementType::kEvadeThreatChance ||
				a_configData.Element == ElementType::kAttackStaggeredMult ||
				a_configData.Element == ElementType::kPowerAttackStaggeredMult ||
				a_configData.Element == ElementType::kPowerAttackBlockingMult ||
				a_configData.Element == ElementType::kBashMult ||
				a_configData.Element == ElementType::kBashRecoilMult ||
				a_configData.Element == ElementType::kBashAttackMult ||
				a_configData.Element == ElementType::kBashPowerAttackMult ||
				a_configData.Element == ElementType::kSpecialAttackMult ||
				a_configData.Element == ElementType::kBlockWhenStaggeredMult ||
				a_configData.Element == ElementType::kAttackWhenStaggeredMult ||
				a_configData.Element == ElementType::kRangedAccuracyMult ||
				a_configData.Element == ElementType::kCircleMult ||
				a_configData.Element == ElementType::kFallbackMult ||
				a_configData.Element == ElementType::kFlankDistance ||
				a_configData.Element == ElementType::kStalkTime ||
				a_configData.Element == ElementType::kChargeDistance ||
				a_configData.Element == ElementType::kThrowProbability ||
				a_configData.Element == ElementType::kSprintFastProbability ||
				a_configData.Element == ElementType::kSideswipeProbability ||
				a_configData.Element == ElementType::kDisengageProbability ||
				a_configData.Element == ElementType::kFlankVariance ||
				a_configData.Element == ElementType::kStrafeMult ||
				a_configData.Element == ElementType::kAdjustRangeMult ||
				a_configData.Element == ElementType::kCrouchMult ||
				a_configData.Element == ElementType::kWaitMult ||
				a_configData.Element == ElementType::kRangeMult ||
				a_configData.Element == ElementType::kCoverSearchDistanceMult ||
				a_configData.Element == ElementType::kHoverChance ||
				a_configData.Element == ElementType::kDiveBombChance ||
				a_configData.Element == ElementType::kGroundAttackChance ||
				a_configData.Element == ElementType::kHoverTime ||
				a_configData.Element == ElementType::kGroundAttackTime ||
				a_configData.Element == ElementType::kPerchAttackChance ||
				a_configData.Element == ElementType::kPerchAttackTime ||
				a_configData.Element == ElementType::kFlyingAttackChance) {
				std::optional<float> parsedValue = ParseNumber();
				if (!parsedValue.has_value()) {
					return false;
				}

				a_configData.AssignValue = std::any(parsedValue.value());
			}
			else if (a_configData.Element == ElementType::kThrowMaxTargets) {
				token = reader.GetToken();
				if (token.empty()) {
					logger::warn("Line {}, Col {}: Expected value.", reader.GetLastLine(), reader.GetLastLineIndex());
					return false;
				}

				unsigned long parsedValue;
				auto parseResult = std::from_chars(token.data(), token.data() + token.size(), parsedValue);
				if (parseResult.ec != std::errc() || parseResult.ptr != token.data() + token.size()) {
					logger::warn("Line {}, Col {}: Failed to parse value '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				if (parsedValue > UINT32_MAX) {
					logger::warn("Line {}, Col {}: Value '{}' out of range.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}

				a_configData.AssignValue = std::any(static_cast<std::uint32_t>(parsedValue));
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
			else {
				logger::warn("Line {}, Col {}: Invalid Assignment for '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_configData.Element));
				return false;
			}

			return true;
		}

		std::optional<std::uint32_t> ParseFlag()
		{
			auto token = reader.GetToken();
			if (token.empty()) {
				logger::warn("Line {}, Col {}: Expected flag name.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			if (token == "None") {
				return 0x00;
			}
			else if (token == "Dueling") {
				return 0x01;
			}
			else if (token == "Flanking") {
				return 0x02;
			}
			else if (token == "AllowDualWielding") {
				return 0x04;
			}
			else if (token == "Charging") {
				return 0x08;
			}
			else if (token == "RetargetAnyNearbyMeleeTarget") {
				return 0x10;
			}
			else {
				logger::warn("Line {}, Col {}: Invalid flag name '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return std::nullopt;
			}
		}

		std::string GetFlags(std::uint32_t a_flags)
		{
			std::string retStr;
			std::string separtor = " | ";

			if (a_flags & 0x01) {
				retStr += "Dueling" + separtor;
			}
			if (a_flags & 0x02) {
				retStr += "Flanking" + separtor;
			}
			if (a_flags & 0x04) {
				retStr += "AllowDualWielding" + separtor;
			}
			if (a_flags & 0x08) {
				retStr += "Charging" + separtor;
			}
			if (a_flags & 0x10) {
				retStr += "RetargetAnyNearbyMeleeTarget" + separtor;
			}

			if (retStr.empty()) {
				return "None";
			}

			return retStr.substr(0, retStr.size() - separtor.size());
		}
	};

	void ReadConfigs()
	{
		g_configVec = ConfigUtils::ReadConfigs<CombatStyleParser, Parsers::Statement<ConfigData>>(TypeName);
	}

	void Prepare(const ConfigData& a_configData)
	{
		if (a_configData.Filter == FilterType::kFormID) {
			RE::TESForm* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
			if (!filterForm) {
				logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
				return;
			}

			RE::TESCombatStyle* combatStyle = filterForm->As<RE::TESCombatStyle>();
			if (!combatStyle) {
				logger::warn("'{}' is not a CombatStyle.", a_configData.FilterForm);
				return;
			}

			if (a_configData.Element == ElementType::kOffensiveMult) {
				g_patchMap[combatStyle].OffensiveMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kDefensiveMult) {
				g_patchMap[combatStyle].DefensiveMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kGroupOffensiveMult) {
				g_patchMap[combatStyle].GroupOffensiveMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kMeleeEquipmentScoreMult) {
				g_patchMap[combatStyle].MeleeEquipmentScoreMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kMagicEquipmentScoreMult) {
				g_patchMap[combatStyle].MagicEquipmentScoreMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kRangedEquipmentScoreMult) {
				g_patchMap[combatStyle].RangedEquipmentScoreMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kShoutEquipmentScoreMult) {
				g_patchMap[combatStyle].ShoutEquipmentScoreMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kUnarmedEquipmentScoreMult) {
				g_patchMap[combatStyle].UnarmedEquipmentScoreMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kStaffEquipmentScoreMult) {
				g_patchMap[combatStyle].StaffEquipmentScoreMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kAvoidThreatChance) {
				g_patchMap[combatStyle].AvoidThreatChance = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kDodgeThreatChance) {
				g_patchMap[combatStyle].DodgeThreatChance = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kEvadeThreatChance) {
				g_patchMap[combatStyle].EvadeThreatChance = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kAttackStaggeredMult) {
				g_patchMap[combatStyle].AttackStaggeredMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kPowerAttackStaggeredMult) {
				g_patchMap[combatStyle].PowerAttackStaggeredMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kPowerAttackBlockingMult) {
				g_patchMap[combatStyle].PowerAttackBlockingMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kBashMult) {
				g_patchMap[combatStyle].BashMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kBashRecoilMult) {
				g_patchMap[combatStyle].BashRecoilMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kBashAttackMult) {
				g_patchMap[combatStyle].BashAttackMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kBashPowerAttackMult) {
				g_patchMap[combatStyle].BashPowerAttackMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kSpecialAttackMult) {
				g_patchMap[combatStyle].SpecialAttackMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kBlockWhenStaggeredMult) {
				g_patchMap[combatStyle].BlockWhenStaggeredMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kAttackWhenStaggeredMult) {
				g_patchMap[combatStyle].AttackWhenStaggeredMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kRangedAccuracyMult) {
				g_patchMap[combatStyle].RangedAccuracyMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kCircleMult) {
				g_patchMap[combatStyle].CircleMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kFallbackMult) {
				g_patchMap[combatStyle].FallbackMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kFlankDistance) {
				g_patchMap[combatStyle].FlankDistance = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kStalkTime) {
				g_patchMap[combatStyle].StalkTime = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kChargeDistance) {
				g_patchMap[combatStyle].ChargeDistance = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kThrowProbability) {
				g_patchMap[combatStyle].ThrowProbability = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kSprintFastProbability) {
				g_patchMap[combatStyle].SprintFastProbability = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kSideswipeProbability) {
				g_patchMap[combatStyle].SideswipeProbability = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kDisengageProbability) {
				g_patchMap[combatStyle].DisengageProbability = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kThrowMaxTargets) {
				g_patchMap[combatStyle].ThrowMaxTargets = std::any_cast<std::uint32_t>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kFlankVariance) {
				g_patchMap[combatStyle].FlankVariance = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kStrafeMult) {
				g_patchMap[combatStyle].StrafeMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kAdjustRangeMult) {
				g_patchMap[combatStyle].AdjustRangeMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kCrouchMult) {
				g_patchMap[combatStyle].CrouchMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kWaitMult) {
				g_patchMap[combatStyle].WaitMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kRangeMult) {
				g_patchMap[combatStyle].RangeMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kCoverSearchDistanceMult) {
				g_patchMap[combatStyle].CoverSearchDistanceMult = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kHoverChance) {
				g_patchMap[combatStyle].HoverChance = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kDiveBombChance) {
				g_patchMap[combatStyle].DiveBombChance = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kGroundAttackChance) {
				g_patchMap[combatStyle].GroundAttackChance = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kHoverTime) {
				g_patchMap[combatStyle].HoverTime = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kGroundAttackTime) {
				g_patchMap[combatStyle].GroundAttackTime = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kPerchAttackChance) {
				g_patchMap[combatStyle].PerchAttackChance = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kPerchAttackTime) {
				g_patchMap[combatStyle].PerchAttackTime = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kFlyingAttackChance) {
				g_patchMap[combatStyle].FlyingAttackChance = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kFlags) {
				g_patchMap[combatStyle].Flags = std::any_cast<std::uint32_t>(a_configData.AssignValue.value());
			}
			else {
				logger::warn("Invalid ElementName: '{}'.", ElementTypeToString(a_configData.Element));
				return;
			}
		}
	}

	void Patch()
	{
		logger::info("======================== Start preparing patch for {} ========================", TypeName);

		ConfigUtils::Prepare(g_configVec, Prepare);

		logger::info("======================== Finished preparing patch for {} ========================", TypeName);
		logger::info("");

		logger::info("======================== Start patching for {} ========================", TypeName);

		for (const auto& [combatStyle, patchData] : g_patchMap) {
			if (patchData.OffensiveMult.has_value()) {
				combatStyle->generalData.offensiveMult = patchData.OffensiveMult.value();
			}
			if (patchData.DefensiveMult.has_value()) {
				combatStyle->generalData.defensiveMult = patchData.DefensiveMult.value();
			}
			if (patchData.GroupOffensiveMult.has_value()) {
				combatStyle->generalData.groupOffensiveMult = patchData.GroupOffensiveMult.value();
			}
			if (patchData.MeleeEquipmentScoreMult.has_value()) {
				combatStyle->generalData.meleeScoreMult = patchData.MeleeEquipmentScoreMult.value();
			}
			if (patchData.MagicEquipmentScoreMult.has_value()) {
				combatStyle->generalData.magicScoreMult = patchData.MagicEquipmentScoreMult.value();
			}
			if (patchData.RangedEquipmentScoreMult.has_value()) {
				combatStyle->generalData.rangedScoreMult = patchData.RangedEquipmentScoreMult.value();
			}
			if (patchData.ShoutEquipmentScoreMult.has_value()) {
				combatStyle->generalData.shoutScoreMult = patchData.ShoutEquipmentScoreMult.value();
			}
			if (patchData.UnarmedEquipmentScoreMult.has_value()) {
				combatStyle->generalData.unarmedScoreMult = patchData.UnarmedEquipmentScoreMult.value();
			}
			if (patchData.StaffEquipmentScoreMult.has_value()) {
				combatStyle->generalData.staffScoreMult = patchData.StaffEquipmentScoreMult.value();
			}
			if (patchData.AvoidThreatChance.has_value()) {
				combatStyle->generalData.avoidThreatChance = patchData.AvoidThreatChance.value();
			}
			if (patchData.DodgeThreatChance.has_value()) {
				combatStyle->generalData.dodgeThreatChance = patchData.DodgeThreatChance.value();
			}
			if (patchData.EvadeThreatChance.has_value()) {
				combatStyle->generalData.evadeThreatChance = patchData.EvadeThreatChance.value();
			}
			if (patchData.AttackStaggeredMult.has_value()) {
				combatStyle->meleeData.attackIncapacitatedMult = patchData.AttackStaggeredMult.value();
			}
			if (patchData.PowerAttackStaggeredMult.has_value()) {
				combatStyle->meleeData.powerAttackIncapacitatedMult = patchData.PowerAttackStaggeredMult.value();
			}
			if (patchData.PowerAttackBlockingMult.has_value()) {
				combatStyle->meleeData.powerAttackBlockingMult = patchData.PowerAttackBlockingMult.value();
			}
			if (patchData.BashMult.has_value()) {
				combatStyle->meleeData.bashMult = patchData.BashMult.value();
			}
			if (patchData.BashRecoilMult.has_value()) {
				combatStyle->meleeData.bashRecoiledMult = patchData.BashRecoilMult.value();
			}
			if (patchData.BashAttackMult.has_value()) {
				combatStyle->meleeData.bashAttackMult = patchData.BashAttackMult.value();
			}
			if (patchData.BashPowerAttackMult.has_value()) {
				combatStyle->meleeData.bashPowerAttackMult = patchData.BashPowerAttackMult.value();
			}
			if (patchData.SpecialAttackMult.has_value()) {
				combatStyle->meleeData.specialAttackMult = patchData.SpecialAttackMult.value();
			}
			if (patchData.BlockWhenStaggeredMult.has_value()) {
				combatStyle->meleeData.blockWhenIncapacitatedMult = patchData.BlockWhenStaggeredMult.value();
			}
			if (patchData.AttackWhenStaggeredMult.has_value()) {
				combatStyle->meleeData.attackWhenIncapacitatedMult = patchData.AttackWhenStaggeredMult.value();
			}
			if (patchData.RangedAccuracyMult.has_value()) {
				combatStyle->rangedData.accuracyMult = patchData.RangedAccuracyMult.value();
			}
			if (patchData.CircleMult.has_value()) {
				combatStyle->closeRangeData.circleMult = patchData.CircleMult.value();
			}
			if (patchData.FallbackMult.has_value()) {
				combatStyle->closeRangeData.fallbackMult = patchData.FallbackMult.value();
			}
			if (patchData.FlankDistance.has_value()) {
				combatStyle->closeRangeData.flankDistanceMult = patchData.FlankDistance.value();
			}
			if (patchData.StalkTime.has_value()) {
				combatStyle->closeRangeData.stalkTimeMult = patchData.StalkTime.value();
			}
			if (patchData.ChargeDistance.has_value()) {
				combatStyle->closeRangeData.chargeDistanceMult = patchData.ChargeDistance.value();
			}
			if (patchData.ThrowProbability.has_value()) {
				combatStyle->closeRangeData.flipThrowProbability = patchData.ThrowProbability.value();
			}
			if (patchData.SprintFastProbability.has_value()) {
				combatStyle->closeRangeData.sprintChargeProbability = patchData.SprintFastProbability.value();
			}
			if (patchData.SideswipeProbability.has_value()) {
				combatStyle->closeRangeData.sideswipeProbability = patchData.SideswipeProbability.value();
			}
			if (patchData.DisengageProbability.has_value()) {
				combatStyle->closeRangeData.disengageProbability = patchData.DisengageProbability.value();
			}
			if (patchData.ThrowMaxTargets.has_value()) {
				combatStyle->closeRangeData.throwMaxTargets = patchData.ThrowMaxTargets.value();
			}
			if (patchData.FlankVariance.has_value()) {
				combatStyle->closeRangeData.flankVarianceMult = patchData.FlankVariance.value();
			}
			if (patchData.StrafeMult.has_value()) {
				combatStyle->longRangeData.strafeMult = patchData.StrafeMult.value();
			}
			if (patchData.AdjustRangeMult.has_value()) {
				combatStyle->longRangeData.adjustRangeMult = patchData.AdjustRangeMult.value();
			}
			if (patchData.CrouchMult.has_value()) {
				combatStyle->longRangeData.crouchMult = patchData.CrouchMult.value();
			}
			if (patchData.WaitMult.has_value()) {
				combatStyle->longRangeData.waitMult = patchData.WaitMult.value();
			}
			if (patchData.RangeMult.has_value()) {
				combatStyle->longRangeData.rangeMult = patchData.RangeMult.value();
			}
			if (patchData.CoverSearchDistanceMult.has_value()) {
				combatStyle->coverData.coverSearchDistanceMult = patchData.CoverSearchDistanceMult.value();
			}
			if (patchData.HoverChance.has_value()) {
				combatStyle->flightData.hoverChance = patchData.HoverChance.value();
			}
			if (patchData.DiveBombChance.has_value()) {
				combatStyle->flightData.diveBombChance = patchData.DiveBombChance.value();
			}
			if (patchData.GroundAttackChance.has_value()) {
				combatStyle->flightData.groundAttackChance = patchData.GroundAttackChance.value();
			}
			if (patchData.HoverTime.has_value()) {
				combatStyle->flightData.hoverTimeMult = patchData.HoverTime.value();
			}
			if (patchData.GroundAttackTime.has_value()) {
				combatStyle->flightData.groundAttackTimeMult = patchData.GroundAttackTime.value();
			}
			if (patchData.PerchAttackChance.has_value()) {
				combatStyle->flightData.perchAttackChance = patchData.PerchAttackChance.value();
			}
			if (patchData.PerchAttackTime.has_value()) {
				combatStyle->flightData.perchAttackTimeMult = patchData.PerchAttackTime.value();
			}
			if (patchData.FlyingAttackChance.has_value()) {
				combatStyle->flightData.flyingAttackChance = patchData.FlyingAttackChance.value();
			}
			if (patchData.Flags.has_value()) {
				combatStyle->flags = patchData.Flags.value();
			}
		}

		logger::info("======================== Finished patching for {} ========================", TypeName);
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}
