#include "ImageSpaceAdapters.h"

#include <any>
#include <regex>

#include "ConfigUtils.h"

namespace RE
{
	class __declspec(novtable) NiInterpolator : public NiObject
	{
	public:
		static constexpr auto RTTI{ RTTI::NiInterpolator };
		static constexpr auto VTABLE{ VTABLE::NiInterpolator };
		static constexpr auto Ni_RTTI{ Ni_RTTI::NiInterpolator };

		NiInterpolator() : lastTime(-FLT_MAX), pad14(0)
		{
			stl::emplace_vtable(this);
			refCount = 0;
		}

		virtual ~NiInterpolator() = default;

		float lastTime;       // 10
		std::uint32_t pad14;  // 14
	};
	static_assert(sizeof(NiInterpolator) == 0x18);

	class __declspec(novtable) NiKeyBasedInterpolator : public NiInterpolator
	{
	public:
		static constexpr auto RTTI{ RTTI::NiKeyBasedInterpolator };
		static constexpr auto VTABLE{ VTABLE::NiKeyBasedInterpolator };
		static constexpr auto Ni_RTTI{ Ni_RTTI::NiKeyBasedInterpolator };
	};
	static_assert(sizeof(NiKeyBasedInterpolator) == 0x18);

	class __declspec(novtable) NiFloatData : public NiObject
	{
	public:
		static constexpr auto RTTI{ RTTI::NiFloatData };
		static constexpr auto VTABLE{ VTABLE::NiFloatData };
		static constexpr auto Ni_RTTI{ Ni_RTTI::NiFloatData };

		NiFloatData() : numKeys(0), pad14(0), data(nullptr), type(0), keySize(0), pad25(0), pad26(0)
		{ 
			stl::emplace_vtable(this); 
			refCount = 0;
		}

		virtual ~NiFloatData() = default;

		struct Data
		{
			float time;   // 0
			float value;  // 4
		};

		// members
		std::uint32_t numKeys;  // 10
		std::uint32_t pad14;    // 14
		Data* data;             // 18
		std::uint32_t type;     // 20
		std::uint8_t keySize;   // 24
		std::uint8_t pad25;     // 25
		std::uint16_t pad26;    // 26
	};
	static_assert(sizeof(NiFloatData) == 0x28);

	class __declspec(novtable) NiColorData : public NiObject
	{
	public:
		static constexpr auto RTTI{ RTTI::NiColorData };
		static constexpr auto VTABLE{ VTABLE::NiColorData };
		static constexpr auto Ni_RTTI{ Ni_RTTI::NiColorData };

		NiColorData() : numKeys(0), pad14(0), data(nullptr), type(0), keySize(0), pad25(0), pad26(0)
		{
			stl::emplace_vtable(this);
			refCount = 0;
		}

		virtual ~NiColorData() = default;

		struct Data
		{
			float time;			// 0
			NiColorA color;		// 4
		};

		// members
		std::uint32_t numKeys;  // 10
		std::uint32_t pad14;    // 14
		Data* data;				// 18
		std::uint32_t type;     // 20
		std::uint8_t keySize;   // 24
		std::uint8_t pad25;     // 25
		std::uint16_t pad26;    // 26
	};
	static_assert(sizeof(NiColorData) == 0x28);

	class __declspec(novtable) NiFloatInterpolator : public NiKeyBasedInterpolator
	{
	public:
		static constexpr auto RTTI{ RTTI::NiFloatInterpolator };
		static constexpr auto VTABLE{ VTABLE::NiFloatInterpolator };
		static constexpr auto Ni_RTTI{ Ni_RTTI::NiFloatInterpolator };

		NiFloatInterpolator() : floatValue(-FLT_MAX), pad1C(0), floatData(nullptr), lastIndex(0), pad2C(0)
		{
			stl::emplace_vtable(this);
		}

		virtual ~NiFloatInterpolator() = default;

		// members
		float floatValue;                  // 18
		std::uint32_t pad1C;               // 1C
		NiPointer<NiFloatData> floatData;  // 20
		std::uint32_t lastIndex;           // 28
		std::uint32_t pad2C;               // 2C
	};
	static_assert(sizeof(NiFloatInterpolator) == 0x30);

	class __declspec(novtable) NiColorInterpolator : public NiKeyBasedInterpolator
	{
	public:
		static constexpr auto RTTI{ RTTI::NiColorInterpolator };
		static constexpr auto VTABLE{ VTABLE::NiColorInterpolator };
		static constexpr auto Ni_RTTI{ Ni_RTTI::NiColorInterpolator };

		NiColorInterpolator() : colorValue{ -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX }, colorData(nullptr), lastIndex(0), pad34(0)
		{
			stl::emplace_vtable(this);
		}

		virtual ~NiColorInterpolator() = default;

		// members
		NiColorA colorValue;               // 18
		NiPointer<NiColorData> colorData;  // 28
		std::uint32_t lastIndex;           // 30
		std::uint32_t pad34;               // 34
	};
	static_assert(sizeof(NiColorInterpolator) == 0x38);
}

namespace ImageSpaceAdapters
{
	constexpr std::string_view TypeName = "ImageSpaceAdapter";

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
		kAnimatable,
		kDuration,
		kBlurRadius,
		kDoubleVisionStrength,
		kTintColor,
		kFadeColor,
		kRadialBlurUseTarget,
		kRadialBlurCenterX,
		kRadialBlurCenterY,
		kRadialBlurStrength,
		kRadialBlurRampUp,
		kRadialBlurRampDown,
		kRadialBlurStart,
		kRadialBlurDownStart,
		kDepthOfFieldUseTarget,
		kDepthOfFieldStrength,
		kDepthOfFieldDistance,
		kDepthOfFieldRange,
		kDepthOfFieldVignetteRadius,
		kDepthOfFieldVignetteStrength,
		kMotionBlurStrength,
		kHDREyeAdaptSpeedMult,
		kHDREyeAdaptSpeedAdd,
		kHDRBloomBlurRadiusMult,
		kHDRBloomBlurRadiusAdd,
		kHDRBloomThresholdMult,
		kHDRBloomThresholdAdd,
		kHDRBloomScaleMult,
		kHDRBloomScaleAdd,
		kHDRTargetLumMinMult,
		kHDRTargetLumMinAdd,
		kHDRTargetLumMaxMult,
		kHDRTargetLumMaxAdd,
		kHDRSunlightScaleMult,
		kHDRSunlightScaleAdd,
		kHDRSkyScaleMult,
		kHDRSkyScaleAdd,
		kCinematicSaturationMult,
		kCinematicSaturationAdd,
		kCinematicBrightnessMult,
		kCinematicBrightnessAdd,
		kCinematicContrastMult,
		kCinematicContrastAdd
	};

	std::string_view ElementTypeToString(ElementType a_value)
	{
		switch (a_value) {
		case ElementType::kAnimatable:
			return "Animatable";
		case ElementType::kDuration:
			return "Duration";
		case ElementType::kBlurRadius:
			return "BlurRadius";
		case ElementType::kDoubleVisionStrength:
			return "DoubleVisionStrength";
		case ElementType::kTintColor:
			return "TintColor";
		case ElementType::kFadeColor:
			return "FadeColor";
		case ElementType::kRadialBlurUseTarget:
			return "RadialBlurUseTarget";
		case ElementType::kRadialBlurCenterX:
			return "RadialBlurCenterX";
		case ElementType::kRadialBlurCenterY:
			return "RadialBlurCenterY";
		case ElementType::kRadialBlurStrength:
			return "RadialBlurStrength";
		case ElementType::kRadialBlurRampUp:
			return "RadialBlurRampUp";
		case ElementType::kRadialBlurRampDown:
			return "RadialBlurRampDown";
		case ElementType::kRadialBlurStart:
			return "RadialBlurStart";
		case ElementType::kRadialBlurDownStart:
			return "RadialBlurDownStart";
		case ElementType::kDepthOfFieldUseTarget:
			return "DepthOfFieldUseTarget";
		case ElementType::kDepthOfFieldStrength:
			return "DepthOfFieldStrength";
		case ElementType::kDepthOfFieldDistance:
			return "DepthOfFieldDistance";
		case ElementType::kDepthOfFieldRange:
			return "DepthOfFieldRange";
		case ElementType::kDepthOfFieldVignetteRadius:
			return "DepthOfFieldVignetteRadius";
		case ElementType::kDepthOfFieldVignetteStrength:
			return "DepthOfFieldVignetteStrength";
		case ElementType::kMotionBlurStrength:
			return "MotionBlurStrength";
		case ElementType::kHDREyeAdaptSpeedMult:
			return "HDREyeAdaptSpeedMult";
		case ElementType::kHDREyeAdaptSpeedAdd:
			return "HDREyeAdaptSpeedAdd";
		case ElementType::kHDRBloomBlurRadiusMult:
			return "HDRBloomBlurRadiusMult";
		case ElementType::kHDRBloomBlurRadiusAdd:
			return "HDRBloomBlurRadiusAdd";
		case ElementType::kHDRBloomThresholdMult:
			return "HDRBloomThresholdMult";
		case ElementType::kHDRBloomThresholdAdd:
			return "HDRBloomThresholdAdd";
		case ElementType::kHDRBloomScaleMult:
			return "HDRBloomScaleMult";
		case ElementType::kHDRBloomScaleAdd:
			return "HDRBloomScaleAdd";
		case ElementType::kHDRTargetLumMinMult:
			return "HDRTargetLumMinMult";
		case ElementType::kHDRTargetLumMinAdd:
			return "HDRTargetLumMinAdd";
		case ElementType::kHDRTargetLumMaxMult:
			return "HDRTargetLumMaxMult";
		case ElementType::kHDRTargetLumMaxAdd:
			return "HDRTargetLumMaxAdd";
		case ElementType::kHDRSunlightScaleMult:
			return "HDRSunlightScaleMult";
		case ElementType::kHDRSunlightScaleAdd:
			return "HDRSunlightScaleAdd";
		case ElementType::kHDRSkyScaleMult:
			return "HDRSkyScaleMult";
		case ElementType::kHDRSkyScaleAdd:
			return "HDRSkyScaleAdd";
		case ElementType::kCinematicSaturationMult:
			return "CinematicSaturationMult";
		case ElementType::kCinematicSaturationAdd:
			return "CinematicSaturationAdd";
		case ElementType::kCinematicBrightnessMult:
			return "CinematicBrightnessMult";
		case ElementType::kCinematicBrightnessAdd:
			return "CinematicBrightnessAdd";
		case ElementType::kCinematicContrastMult:
			return "CinematicContrastMult";
		case ElementType::kCinematicContrastAdd:
			return "CinematicContrastAdd";
		default:
			return std::string_view{};
		}
	}

	enum class OperationType
	{
		kClear,
		kAdd
	};

	std::string_view OperationTypeToString(OperationType a_value)
	{
		switch (a_value) {
		case OperationType::kClear:
			return "Clear";
		case OperationType::kAdd:
			return "Add";
		default:
			return std::string_view{};
		}
	}

	struct ConfigData
	{
		struct Operation
		{
			struct FloatData
			{
				float Time;
				float Value;
			};

			struct ColorData
			{
				float Time;
				float Red;
				float Green;
				float Blue;
				float Alpha;
			};

			OperationType Type;
			std::optional<std::any> Data;
		};

		FilterType Filter;
		std::string FilterForm;
		ElementType Element;
		std::optional<std::any> AssignValue;
		std::vector<Operation> Operations;
	};

	struct PatchData
	{
		struct FloatInterpolatorData
		{
			struct FloatData
			{
				float Time;
				float Value;
			};

			bool Clear;
			std::vector<FloatData> AddVec;
		};

		struct ColorInterpolatorData
		{
			struct ColorData
			{
				float Time;
				RE::NiColorA Color;
			};

			bool Clear;
			std::vector<ColorData> AddVec;
		};

		std::optional<bool> Animatable;
		std::optional<float> Duration;
		std::optional<FloatInterpolatorData> BlurRadius;
		std::optional<FloatInterpolatorData> DoubleVisionStrength;
		std::optional<ColorInterpolatorData> TintColor;
		std::optional<ColorInterpolatorData> FadeColor;
		std::optional<bool> RadialBlurUseTarget;
		std::optional<float> RadialBlurCenterX;
		std::optional<float> RadialBlurCenterY;
		std::optional<FloatInterpolatorData> RadialBlurStrength;
		std::optional<FloatInterpolatorData> RadialBlurRampUp;
		std::optional<FloatInterpolatorData> RadialBlurRampDown;
		std::optional<FloatInterpolatorData> RadialBlurStart;
		std::optional<FloatInterpolatorData> RadialBlurDownStart;
		std::optional<bool> DepthOfFieldUseTarget;
		std::optional<FloatInterpolatorData> DepthOfFieldStrength;
		std::optional<FloatInterpolatorData> DepthOfFieldDistance;
		std::optional<FloatInterpolatorData> DepthOfFieldRange;
		std::optional<FloatInterpolatorData> DepthOfFieldVignetteRadius;
		std::optional<FloatInterpolatorData> DepthOfFieldVignetteStrength;
		std::optional<FloatInterpolatorData> MotionBlurStrength;
		std::optional<FloatInterpolatorData> HDREyeAdaptSpeedMult;
		std::optional<FloatInterpolatorData> HDREyeAdaptSpeedAdd;
		std::optional<FloatInterpolatorData> HDRBloomBlurRadiusMult;
		std::optional<FloatInterpolatorData> HDRBloomBlurRadiusAdd;
		std::optional<FloatInterpolatorData> HDRBloomThresholdMult;
		std::optional<FloatInterpolatorData> HDRBloomThresholdAdd;
		std::optional<FloatInterpolatorData> HDRBloomScaleMult;
		std::optional<FloatInterpolatorData> HDRBloomScaleAdd;
		std::optional<FloatInterpolatorData> HDRTargetLumMinMult;
		std::optional<FloatInterpolatorData> HDRTargetLumMinAdd;
		std::optional<FloatInterpolatorData> HDRTargetLumMaxMult;
		std::optional<FloatInterpolatorData> HDRTargetLumMaxAdd;
		std::optional<FloatInterpolatorData> HDRSunlightScaleMult;
		std::optional<FloatInterpolatorData> HDRSunlightScaleAdd;
		std::optional<FloatInterpolatorData> HDRSkyScaleMult;
		std::optional<FloatInterpolatorData> HDRSkyScaleAdd;
		std::optional<FloatInterpolatorData> CinematicSaturationMult;
		std::optional<FloatInterpolatorData> CinematicSaturationAdd;
		std::optional<FloatInterpolatorData> CinematicBrightnessMult;
		std::optional<FloatInterpolatorData> CinematicBrightnessAdd;
		std::optional<FloatInterpolatorData> CinematicContrastMult;
		std::optional<FloatInterpolatorData> CinematicContrastAdd;
	};

	std::vector<Parsers::Statement<ConfigData>> g_configVec;
	std::unordered_map<RE::TESImageSpaceModifier*, PatchData> g_patchMap;

	class ImageSpaceAdapterParser : public Parsers::Parser<ConfigData>
	{
	public:
		ImageSpaceAdapterParser(std::string_view a_configPath) :
			Parser<ConfigData>(a_configPath) {}

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

			token = reader.Peek();
			if (token == "=") {
				if (!ParseAssignment(configData)) {
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

			token = reader.GetToken();
			if (token != ";") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ';'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			return Parsers::Statement<ConfigData>::CreateExpressionStatement(configData);
		}

		void PrintExpressionStatement(const ConfigData& a_configData, int a_indent) override
		{
			std::string indent = std::string(a_indent * 4, ' ');

			switch (a_configData.Element) {
			case ElementType::kAnimatable:
			case ElementType::kRadialBlurUseTarget:
			case ElementType::kDepthOfFieldUseTarget:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element), std::any_cast<bool>(a_configData.AssignValue.value()));
				break;

			case ElementType::kDuration:
			case ElementType::kRadialBlurCenterX:
			case ElementType::kRadialBlurCenterY:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element), std::any_cast<float>(a_configData.AssignValue.value()));
				break;

			case ElementType::kBlurRadius:
			case ElementType::kDoubleVisionStrength:
			case ElementType::kRadialBlurStrength:
			case ElementType::kRadialBlurRampUp:
			case ElementType::kRadialBlurRampDown:
			case ElementType::kRadialBlurStart:
			case ElementType::kRadialBlurDownStart:
			case ElementType::kDepthOfFieldStrength:
			case ElementType::kDepthOfFieldDistance:
			case ElementType::kDepthOfFieldRange:
			case ElementType::kDepthOfFieldVignetteRadius:
			case ElementType::kDepthOfFieldVignetteStrength:
			case ElementType::kMotionBlurStrength:
			case ElementType::kHDREyeAdaptSpeedMult:
			case ElementType::kHDREyeAdaptSpeedAdd:
			case ElementType::kHDRBloomBlurRadiusMult:
			case ElementType::kHDRBloomBlurRadiusAdd:
			case ElementType::kHDRBloomThresholdMult:
			case ElementType::kHDRBloomThresholdAdd:
			case ElementType::kHDRBloomScaleMult:
			case ElementType::kHDRBloomScaleAdd:
			case ElementType::kHDRTargetLumMinMult:
			case ElementType::kHDRTargetLumMinAdd:
			case ElementType::kHDRTargetLumMaxMult:
			case ElementType::kHDRTargetLumMaxAdd:
			case ElementType::kHDRSunlightScaleMult:
			case ElementType::kHDRSunlightScaleAdd:
			case ElementType::kHDRSkyScaleMult:
			case ElementType::kHDRSkyScaleAdd:
			case ElementType::kCinematicSaturationMult:
			case ElementType::kCinematicSaturationAdd:
			case ElementType::kCinematicBrightnessMult:
			case ElementType::kCinematicBrightnessAdd:
			case ElementType::kCinematicContrastMult:
			case ElementType::kCinematicContrastAdd:
				logger::info("{}{}({}).{}", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element));
				for (std::size_t opIndex = 0; opIndex < a_configData.Operations.size(); opIndex++) {
					std::string opLog;

					switch (a_configData.Operations[opIndex].Type) {
					case OperationType::kClear: {						
						opLog = fmt::format(".{}()", OperationTypeToString(a_configData.Operations[opIndex].Type));
						break;
					}

					case OperationType::kAdd: {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(a_configData.Operations[opIndex].Data.value());
						opLog = fmt::format(".{}({}, {})", OperationTypeToString(a_configData.Operations[opIndex].Type), data.Time, data.Value);
						break;
					}
					}

					if (opIndex == a_configData.Operations.size() - 1) {
						opLog += ";";
					}

					logger::info("{}    {}", indent, opLog);
				}
				break;

			case ElementType::kTintColor:
			case ElementType::kFadeColor:
				logger::info("{}{}({}).{}", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element));
				for (std::size_t opIndex = 0; opIndex < a_configData.Operations.size(); opIndex++) {
					std::string opLog;

					switch (a_configData.Operations[opIndex].Type) {
					case OperationType::kClear: {
						opLog = fmt::format(".{}()", OperationTypeToString(a_configData.Operations[opIndex].Type));
						break;
					}

					case OperationType::kAdd: {
						auto data = std::any_cast<ConfigData::Operation::ColorData>(a_configData.Operations[opIndex].Data.value());
						opLog = fmt::format(".{}({}, {}, {}, {}, {})", OperationTypeToString(a_configData.Operations[opIndex].Type), data.Time, data.Red, data.Green, data.Blue, data.Alpha);
						break;
					}
					}

					if (opIndex == a_configData.Operations.size() - 1) {
						opLog += ";";
					}

					logger::info("{}    {}", indent, opLog);
				}
				break;
			}
		}

		bool ParseFilter(ConfigData& a_config)
		{
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

		bool ParseElement(ConfigData& a_config)
		{
			auto token = reader.GetToken();
			if (token == "Animatable") {
				a_config.Element = ElementType::kAnimatable;
			}
			else if (token == "Duration") {
				a_config.Element = ElementType::kDuration;
			}
			else if (token == "BlurRadius") {
				a_config.Element = ElementType::kBlurRadius;
			}
			else if (token == "DoubleVisionStrength") {
				a_config.Element = ElementType::kDoubleVisionStrength;
			}
			else if (token == "TintColor") {
				a_config.Element = ElementType::kTintColor;
			}
			else if (token == "FadeColor") {
				a_config.Element = ElementType::kFadeColor;
			}
			else if (token == "RadialBlurUseTarget") {
				a_config.Element = ElementType::kRadialBlurUseTarget;
			}
			else if (token == "RadialBlurCenterX") {
				a_config.Element = ElementType::kRadialBlurCenterX;
			}
			else if (token == "RadialBlurCenterY") {
				a_config.Element = ElementType::kRadialBlurCenterY;
			}
			else if (token == "RadialBlurStrength") {
				a_config.Element = ElementType::kRadialBlurStrength;
			}
			else if (token == "RadialBlurRampUp") {
				a_config.Element = ElementType::kRadialBlurRampUp;
			}
			else if (token == "RadialBlurRampDown") {
				a_config.Element = ElementType::kRadialBlurRampDown;
			}
			else if (token == "RadialBlurStart") {
				a_config.Element = ElementType::kRadialBlurStart;
			}
			else if (token == "RadialBlurDownStart") {
				a_config.Element = ElementType::kRadialBlurDownStart;
			}
			else if (token == "DepthOfFieldUseTarget") {
				a_config.Element = ElementType::kDepthOfFieldUseTarget;
			}
			else if (token == "DepthOfFieldStrength") {
				a_config.Element = ElementType::kDepthOfFieldStrength;
			}
			else if (token == "DepthOfFieldDistance") {
				a_config.Element = ElementType::kDepthOfFieldDistance;
			}
			else if (token == "DepthOfFieldRange") {
				a_config.Element = ElementType::kDepthOfFieldRange;
			}
			else if (token == "DepthOfFieldVignetteRadius") {
				a_config.Element = ElementType::kDepthOfFieldVignetteRadius;
			}
			else if (token == "DepthOfFieldVignetteStrength") {
				a_config.Element = ElementType::kDepthOfFieldVignetteStrength;
			}
			else if (token == "MotionBlurStrength") {
				a_config.Element = ElementType::kMotionBlurStrength;
			}
			else if (token == "HDREyeAdaptSpeedMult") {
				a_config.Element = ElementType::kHDREyeAdaptSpeedMult;
			}
			else if (token == "HDREyeAdaptSpeedAdd") {
				a_config.Element = ElementType::kHDREyeAdaptSpeedAdd;
			}
			else if (token == "HDRBloomBlurRadiusMult") {
				a_config.Element = ElementType::kHDRBloomBlurRadiusMult;
			}
			else if (token == "HDRBloomBlurRadiusAdd") {
				a_config.Element = ElementType::kHDRBloomBlurRadiusAdd;
			}
			else if (token == "HDRBloomThresholdMult") {
				a_config.Element = ElementType::kHDRBloomThresholdMult;
			}
			else if (token == "HDRBloomThresholdAdd") {
				a_config.Element = ElementType::kHDRBloomThresholdAdd;
			}
			else if (token == "HDRBloomScaleMult") {
				a_config.Element = ElementType::kHDRBloomScaleMult;
			}
			else if (token == "HDRBloomScaleAdd") {
				a_config.Element = ElementType::kHDRBloomScaleAdd;
			}
			else if (token == "HDRTargetLumMinMult") {
				a_config.Element = ElementType::kHDRTargetLumMinMult;
			}
			else if (token == "HDRTargetLumMinAdd") {
				a_config.Element = ElementType::kHDRTargetLumMinAdd;
			}
			else if (token == "HDRTargetLumMaxMult") {
				a_config.Element = ElementType::kHDRTargetLumMaxMult;
			}
			else if (token == "HDRTargetLumMaxAdd") {
				a_config.Element = ElementType::kHDRTargetLumMaxAdd;
			}
			else if (token == "HDRSunlightScaleMult") {
				a_config.Element = ElementType::kHDRSunlightScaleMult;
			}
			else if (token == "HDRSunlightScaleAdd") {
				a_config.Element = ElementType::kHDRSunlightScaleAdd;
			}
			else if (token == "HDRSkyScaleMult") {
				a_config.Element = ElementType::kHDRSkyScaleMult;
			}
			else if (token == "HDRSkyScaleAdd") {
				a_config.Element = ElementType::kHDRSkyScaleAdd;
			}
			else if (token == "CinematicSaturationMult") {
				a_config.Element = ElementType::kCinematicSaturationMult;
			}
			else if (token == "CinematicSaturationAdd") {
				a_config.Element = ElementType::kCinematicSaturationAdd;
			}
			else if (token == "CinematicBrightnessMult") {
				a_config.Element = ElementType::kCinematicBrightnessMult;
			}
			else if (token == "CinematicBrightnessAdd") {
				a_config.Element = ElementType::kCinematicBrightnessAdd;
			}
			else if (token == "CinematicContrastMult") {
				a_config.Element = ElementType::kCinematicContrastMult;
			}
			else if (token == "CinematicContrastAdd") {
				a_config.Element = ElementType::kCinematicContrastAdd;
			}
			else {
				logger::warn("Line {}, Col {}: Invalid ElementName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			return true;
		}

		bool ParseAssignment(ConfigData& a_config)
		{
			auto token = reader.GetToken();
			if (token != "=") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '='.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			if (a_config.Element == ElementType::kAnimatable || a_config.Element == ElementType::kRadialBlurUseTarget || a_config.Element == ElementType::kDepthOfFieldUseTarget) {
				token = reader.GetToken();
				if (token == "true") {
					a_config.AssignValue = std::any(true);
				}
				else if (token == "false") {
					a_config.AssignValue = std::any(false);
				}
				else {
					logger::warn("Line {}, Col {}: Invalid boolean value '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
					return false;
				}
			}
			else if (a_config.Element == ElementType::kDuration || a_config.Element == ElementType::kRadialBlurCenterX || a_config.Element == ElementType::kRadialBlurCenterY) {
				std::optional<float> parsedValue = ParseNumber();
				if (!parsedValue.has_value()) {
					return false;
				}

				a_config.AssignValue = std::any(parsedValue.value());
			}
			else {
				logger::warn("Line {}, Col {}: Invalid Assignment for '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_config.Element));
				return false;
			}

			return true;
		}

		bool ParseOperation(ConfigData& a_config)
		{
			ConfigData::Operation newOp{};

			auto token = reader.GetToken();
			if (token == "Clear") {
				newOp.Type = OperationType::kClear;
			}
			else if (token == "Add") {
				newOp.Type = OperationType::kAdd;
			}
			else {
				logger::warn("Line {}, Col {}: Invalid OperationName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			bool isValidOperation = [](ElementType elem, OperationType op) {
				switch (elem) {
				case ElementType::kBlurRadius:
				case ElementType::kDoubleVisionStrength:
				case ElementType::kTintColor:
				case ElementType::kFadeColor:
				case ElementType::kRadialBlurStrength:
				case ElementType::kRadialBlurRampUp:
				case ElementType::kRadialBlurRampDown:
				case ElementType::kRadialBlurStart:
				case ElementType::kRadialBlurDownStart:
				case ElementType::kDepthOfFieldStrength:
				case ElementType::kDepthOfFieldDistance:
				case ElementType::kDepthOfFieldRange:
				case ElementType::kDepthOfFieldVignetteRadius:
				case ElementType::kDepthOfFieldVignetteStrength:
				case ElementType::kMotionBlurStrength:
				case ElementType::kHDREyeAdaptSpeedMult:
				case ElementType::kHDREyeAdaptSpeedAdd:
				case ElementType::kHDRBloomBlurRadiusMult:
				case ElementType::kHDRBloomBlurRadiusAdd:
				case ElementType::kHDRBloomThresholdMult:
				case ElementType::kHDRBloomThresholdAdd:
				case ElementType::kHDRBloomScaleMult:
				case ElementType::kHDRBloomScaleAdd:
				case ElementType::kHDRTargetLumMinMult:
				case ElementType::kHDRTargetLumMinAdd:
				case ElementType::kHDRTargetLumMaxMult:
				case ElementType::kHDRTargetLumMaxAdd:
				case ElementType::kHDRSunlightScaleMult:
				case ElementType::kHDRSunlightScaleAdd:
				case ElementType::kHDRSkyScaleMult:
				case ElementType::kHDRSkyScaleAdd:
				case ElementType::kCinematicSaturationMult:
				case ElementType::kCinematicSaturationAdd:
				case ElementType::kCinematicBrightnessMult:
				case ElementType::kCinematicBrightnessAdd:
				case ElementType::kCinematicContrastMult:
				case ElementType::kCinematicContrastAdd:
					return op == OperationType::kClear || op == OperationType::kAdd;
				default:
					return false;
				}
			}(a_config.Element, newOp.Type);

			if (!isValidOperation) {
				logger::warn("Line {}, Col {}: Invalid Operation '{}.{}()'.", reader.GetLastLine(), reader.GetLastLineIndex(), ElementTypeToString(a_config.Element), OperationTypeToString(newOp.Type));
				return false;
			}

			token = reader.GetToken();
			if (token != "(") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '('.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			if (a_config.Element == ElementType::kBlurRadius ||
				a_config.Element == ElementType::kDoubleVisionStrength ||
				a_config.Element == ElementType::kRadialBlurStrength ||
				a_config.Element == ElementType::kRadialBlurRampUp ||
				a_config.Element == ElementType::kRadialBlurRampDown ||
				a_config.Element == ElementType::kRadialBlurStart ||
				a_config.Element == ElementType::kRadialBlurDownStart ||
				a_config.Element == ElementType::kDepthOfFieldStrength ||
				a_config.Element == ElementType::kDepthOfFieldDistance ||
				a_config.Element == ElementType::kDepthOfFieldRange ||
				a_config.Element == ElementType::kDepthOfFieldVignetteRadius ||
				a_config.Element == ElementType::kDepthOfFieldVignetteStrength ||
				a_config.Element == ElementType::kMotionBlurStrength ||
				a_config.Element == ElementType::kHDREyeAdaptSpeedMult ||
				a_config.Element == ElementType::kHDREyeAdaptSpeedAdd ||
				a_config.Element == ElementType::kHDRBloomBlurRadiusMult ||
				a_config.Element == ElementType::kHDRBloomBlurRadiusAdd ||
				a_config.Element == ElementType::kHDRBloomThresholdMult ||
				a_config.Element == ElementType::kHDRBloomThresholdAdd ||
				a_config.Element == ElementType::kHDRBloomScaleMult ||
				a_config.Element == ElementType::kHDRBloomScaleAdd ||
				a_config.Element == ElementType::kHDRTargetLumMinMult ||
				a_config.Element == ElementType::kHDRTargetLumMinAdd ||
				a_config.Element == ElementType::kHDRTargetLumMaxMult ||
				a_config.Element == ElementType::kHDRTargetLumMaxAdd ||
				a_config.Element == ElementType::kHDRSunlightScaleMult ||
				a_config.Element == ElementType::kHDRSunlightScaleAdd ||
				a_config.Element == ElementType::kHDRSkyScaleMult ||
				a_config.Element == ElementType::kHDRSkyScaleAdd ||
				a_config.Element == ElementType::kCinematicSaturationMult ||
				a_config.Element == ElementType::kCinematicSaturationAdd ||
				a_config.Element == ElementType::kCinematicBrightnessMult ||
				a_config.Element == ElementType::kCinematicBrightnessAdd ||
				a_config.Element == ElementType::kCinematicContrastMult ||
				a_config.Element == ElementType::kCinematicContrastAdd) {
				if (newOp.Type != OperationType::kClear) {
					ConfigData::Operation::FloatData floatData{};

					auto parsedValue = ParseNumber();
					if (!parsedValue.has_value()) {
						return false;
					}

					floatData.Time = parsedValue.value();

					token = reader.GetToken();
					if (token != ",") {
						logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
						return false;
					}

					parsedValue = ParseNumber();
					if (!parsedValue.has_value()) {
						return false;
					}

					floatData.Value = parsedValue.value();

					newOp.Data = std::any(floatData);
				}
			}
			else if (a_config.Element == ElementType::kTintColor || a_config.Element == ElementType::kFadeColor) {
				if (newOp.Type != OperationType::kClear) {
					ConfigData::Operation::ColorData colorData{};

					auto parsedValue = ParseNumber();
					if (!parsedValue.has_value()) {
						return false;
					}

					colorData.Time = parsedValue.value();

					token = reader.GetToken();
					if (token != ",") {
						logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
						return false;
					}

					parsedValue = ParseNumber();
					if (!parsedValue.has_value()) {
						return false;
					}

					colorData.Red = parsedValue.value();

					token = reader.GetToken();
					if (token != ",") {
						logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
						return false;
					}

					parsedValue = ParseNumber();
					if (!parsedValue.has_value()) {
						return false;
					}

					colorData.Green = parsedValue.value();

					token = reader.GetToken();
					if (token != ",") {
						logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
						return false;
					}

					parsedValue = ParseNumber();
					if (!parsedValue.has_value()) {
						return false;
					}

					colorData.Blue = parsedValue.value();

					token = reader.GetToken();
					if (token != ",") {
						logger::warn("Line {}, Col {}: Syntax error. Expected ','.", reader.GetLastLine(), reader.GetLastLineIndex());
						return false;
					}

					parsedValue = ParseNumber();
					if (!parsedValue.has_value()) {
						return false;
					}

					colorData.Alpha = parsedValue.value();

					newOp.Data = std::any(colorData);
				}
			}

			token = reader.GetToken();
			if (token != ")") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return false;
			}

			a_config.Operations.push_back(newOp);

			return true;
		}
	};

	void ReadConfigs()
	{
		g_configVec = ConfigUtils::ReadConfigs<ImageSpaceAdapterParser, Parsers::Statement<ConfigData>>(TypeName);
	}

	void Prepare(const ConfigData& a_configData)
	{
		if (a_configData.Filter == FilterType::kFormID) {
			RE::TESForm* filterForm = Utils::GetFormFromString(a_configData.FilterForm);
			if (!filterForm) {
				logger::warn("Invalid FilterForm: '{}'.", a_configData.FilterForm);
				return;
			}

			RE::TESImageSpaceModifier* imageSpaceAdapter = filterForm->As<RE::TESImageSpaceModifier>();
			if (!imageSpaceAdapter) {
				logger::warn("'{}' is not an ImageSpaceAdapter.", a_configData.FilterForm);
				return;
			}

			if (a_configData.Element == ElementType::kAnimatable) {
				g_patchMap[imageSpaceAdapter].Animatable = std::any_cast<bool>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kDuration) {
				g_patchMap[imageSpaceAdapter].Duration = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kBlurRadius) {
				if (!g_patchMap[imageSpaceAdapter].BlurRadius.has_value()) {
					g_patchMap[imageSpaceAdapter].BlurRadius = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].BlurRadius->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].BlurRadius->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kDoubleVisionStrength) {
				if (!g_patchMap[imageSpaceAdapter].DoubleVisionStrength.has_value()) {
					g_patchMap[imageSpaceAdapter].DoubleVisionStrength = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].DoubleVisionStrength->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].DoubleVisionStrength->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kTintColor) {
				if (!g_patchMap[imageSpaceAdapter].TintColor.has_value()) {
					g_patchMap[imageSpaceAdapter].TintColor = PatchData::ColorInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].TintColor->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::ColorData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].TintColor->AddVec.push_back({ data.Time, { data.Red, data.Green, data.Blue, data.Alpha } });
					}
				}
			}
			else if (a_configData.Element == ElementType::kFadeColor) {
				if (!g_patchMap[imageSpaceAdapter].FadeColor.has_value()) {
					g_patchMap[imageSpaceAdapter].FadeColor = PatchData::ColorInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].FadeColor->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::ColorData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].FadeColor->AddVec.push_back({ data.Time, { data.Red, data.Green, data.Blue, data.Alpha } });
					}
				}
			}
			else if (a_configData.Element == ElementType::kRadialBlurUseTarget) {
				g_patchMap[imageSpaceAdapter].RadialBlurUseTarget = std::any_cast<bool>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kRadialBlurCenterX) {
				g_patchMap[imageSpaceAdapter].RadialBlurCenterX = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kRadialBlurCenterY) {
				g_patchMap[imageSpaceAdapter].RadialBlurCenterY = std::any_cast<float>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kRadialBlurStrength) {
				if (!g_patchMap[imageSpaceAdapter].RadialBlurStrength.has_value()) {
					g_patchMap[imageSpaceAdapter].RadialBlurStrength = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].RadialBlurStrength->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].RadialBlurStrength->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kRadialBlurRampUp) {
				if (!g_patchMap[imageSpaceAdapter].RadialBlurRampUp.has_value()) {
					g_patchMap[imageSpaceAdapter].RadialBlurRampUp = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].RadialBlurRampUp->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].RadialBlurRampUp->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kRadialBlurRampDown) {
				if (!g_patchMap[imageSpaceAdapter].RadialBlurRampDown.has_value()) {
					g_patchMap[imageSpaceAdapter].RadialBlurRampDown = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].RadialBlurRampDown->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].RadialBlurRampDown->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kRadialBlurStart) {
				if (!g_patchMap[imageSpaceAdapter].RadialBlurStart.has_value()) {
					g_patchMap[imageSpaceAdapter].RadialBlurStart = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].RadialBlurStart->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].RadialBlurStart->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kRadialBlurDownStart) {
				if (!g_patchMap[imageSpaceAdapter].RadialBlurDownStart.has_value()) {
					g_patchMap[imageSpaceAdapter].RadialBlurDownStart = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].RadialBlurDownStart->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].RadialBlurDownStart->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kDepthOfFieldUseTarget) {
				g_patchMap[imageSpaceAdapter].DepthOfFieldUseTarget = std::any_cast<bool>(a_configData.AssignValue.value());
			}
			else if (a_configData.Element == ElementType::kDepthOfFieldStrength) {
				if (!g_patchMap[imageSpaceAdapter].DepthOfFieldStrength.has_value()) {
					g_patchMap[imageSpaceAdapter].DepthOfFieldStrength = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].DepthOfFieldStrength->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].DepthOfFieldStrength->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kDepthOfFieldDistance) {
				if (!g_patchMap[imageSpaceAdapter].DepthOfFieldDistance.has_value()) {
					g_patchMap[imageSpaceAdapter].DepthOfFieldDistance = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].DepthOfFieldDistance->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].DepthOfFieldDistance->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kDepthOfFieldRange) {
				if (!g_patchMap[imageSpaceAdapter].DepthOfFieldRange.has_value()) {
					g_patchMap[imageSpaceAdapter].DepthOfFieldRange = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].DepthOfFieldRange->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].DepthOfFieldRange->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kDepthOfFieldVignetteRadius) {
				if (!g_patchMap[imageSpaceAdapter].DepthOfFieldVignetteRadius.has_value()) {
					g_patchMap[imageSpaceAdapter].DepthOfFieldVignetteRadius = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].DepthOfFieldVignetteRadius->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].DepthOfFieldVignetteRadius->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kDepthOfFieldVignetteStrength) {
				if (!g_patchMap[imageSpaceAdapter].DepthOfFieldVignetteStrength.has_value()) {
					g_patchMap[imageSpaceAdapter].DepthOfFieldVignetteStrength = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].DepthOfFieldVignetteStrength->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].DepthOfFieldVignetteStrength->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kMotionBlurStrength) {
				if (!g_patchMap[imageSpaceAdapter].MotionBlurStrength.has_value()) {
					g_patchMap[imageSpaceAdapter].MotionBlurStrength = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].MotionBlurStrength->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].MotionBlurStrength->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kHDREyeAdaptSpeedMult) {
				if (!g_patchMap[imageSpaceAdapter].HDREyeAdaptSpeedMult.has_value()) {
					g_patchMap[imageSpaceAdapter].HDREyeAdaptSpeedMult = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].HDREyeAdaptSpeedMult->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].HDREyeAdaptSpeedMult->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kHDREyeAdaptSpeedAdd) {
				if (!g_patchMap[imageSpaceAdapter].HDREyeAdaptSpeedAdd.has_value()) {
					g_patchMap[imageSpaceAdapter].HDREyeAdaptSpeedAdd = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].HDREyeAdaptSpeedAdd->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].HDREyeAdaptSpeedAdd->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kHDRBloomBlurRadiusMult) {
				if (!g_patchMap[imageSpaceAdapter].HDRBloomBlurRadiusMult.has_value()) {
					g_patchMap[imageSpaceAdapter].HDRBloomBlurRadiusMult = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].HDRBloomBlurRadiusMult->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].HDRBloomBlurRadiusMult->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kHDRBloomBlurRadiusAdd) {
				if (!g_patchMap[imageSpaceAdapter].HDRBloomBlurRadiusAdd.has_value()) {
					g_patchMap[imageSpaceAdapter].HDRBloomBlurRadiusAdd = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].HDRBloomBlurRadiusAdd->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].HDRBloomBlurRadiusAdd->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kHDRBloomThresholdMult) {
				if (!g_patchMap[imageSpaceAdapter].HDRBloomThresholdMult.has_value()) {
					g_patchMap[imageSpaceAdapter].HDRBloomThresholdMult = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].HDRBloomThresholdMult->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].HDRBloomThresholdMult->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kHDRBloomThresholdAdd) {
				if (!g_patchMap[imageSpaceAdapter].HDRBloomThresholdAdd.has_value()) {
					g_patchMap[imageSpaceAdapter].HDRBloomThresholdAdd = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].HDRBloomThresholdAdd->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].HDRBloomThresholdAdd->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kHDRBloomScaleMult) {
				if (!g_patchMap[imageSpaceAdapter].HDRBloomScaleMult.has_value()) {
					g_patchMap[imageSpaceAdapter].HDRBloomScaleMult = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].HDRBloomScaleMult->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].HDRBloomScaleMult->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kHDRBloomScaleAdd) {
				if (!g_patchMap[imageSpaceAdapter].HDRBloomScaleAdd.has_value()) {
					g_patchMap[imageSpaceAdapter].HDRBloomScaleAdd = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].HDRBloomScaleAdd->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].HDRBloomScaleAdd->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kHDRTargetLumMinMult) {
				if (!g_patchMap[imageSpaceAdapter].HDRTargetLumMinMult.has_value()) {
					g_patchMap[imageSpaceAdapter].HDRTargetLumMinMult = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].HDRTargetLumMinMult->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].HDRTargetLumMinMult->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kHDRTargetLumMinAdd) {
				if (!g_patchMap[imageSpaceAdapter].HDRTargetLumMinAdd.has_value()) {
					g_patchMap[imageSpaceAdapter].HDRTargetLumMinAdd = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].HDRTargetLumMinAdd->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].HDRTargetLumMinAdd->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kHDRTargetLumMaxMult) {
				if (!g_patchMap[imageSpaceAdapter].HDRTargetLumMaxMult.has_value()) {
					g_patchMap[imageSpaceAdapter].HDRTargetLumMaxMult = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].HDRTargetLumMaxMult->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].HDRTargetLumMaxMult->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kHDRTargetLumMaxAdd) {
				if (!g_patchMap[imageSpaceAdapter].HDRTargetLumMaxAdd.has_value()) {
					g_patchMap[imageSpaceAdapter].HDRTargetLumMaxAdd = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].HDRTargetLumMaxAdd->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].HDRTargetLumMaxAdd->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kHDRSunlightScaleMult) {
				if (!g_patchMap[imageSpaceAdapter].HDRSunlightScaleMult.has_value()) {
					g_patchMap[imageSpaceAdapter].HDRSunlightScaleMult = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].HDRSunlightScaleMult->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].HDRSunlightScaleMult->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kHDRSunlightScaleAdd) {
				if (!g_patchMap[imageSpaceAdapter].HDRSunlightScaleAdd.has_value()) {
					g_patchMap[imageSpaceAdapter].HDRSunlightScaleAdd = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].HDRSunlightScaleAdd->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].HDRSunlightScaleAdd->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kHDRSkyScaleMult) {
				if (!g_patchMap[imageSpaceAdapter].HDRSkyScaleMult.has_value()) {
					g_patchMap[imageSpaceAdapter].HDRSkyScaleMult = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].HDRSkyScaleMult->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].HDRSkyScaleMult->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kHDRSkyScaleAdd) {
				if (!g_patchMap[imageSpaceAdapter].HDRSkyScaleAdd.has_value()) {
					g_patchMap[imageSpaceAdapter].HDRSkyScaleAdd = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].HDRSkyScaleAdd->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].HDRSkyScaleAdd->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kCinematicSaturationMult) {
				if (!g_patchMap[imageSpaceAdapter].CinematicSaturationMult.has_value()) {
					g_patchMap[imageSpaceAdapter].CinematicSaturationMult = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].CinematicSaturationMult->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].CinematicSaturationMult->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kCinematicSaturationAdd) {
				if (!g_patchMap[imageSpaceAdapter].CinematicSaturationAdd.has_value()) {
					g_patchMap[imageSpaceAdapter].CinematicSaturationAdd = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].CinematicSaturationAdd->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].CinematicSaturationAdd->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kCinematicBrightnessMult) {
				if (!g_patchMap[imageSpaceAdapter].CinematicBrightnessMult.has_value()) {
					g_patchMap[imageSpaceAdapter].CinematicBrightnessMult = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].CinematicBrightnessMult->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].CinematicBrightnessMult->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kCinematicBrightnessAdd) {
				if (!g_patchMap[imageSpaceAdapter].CinematicBrightnessAdd.has_value()) {
					g_patchMap[imageSpaceAdapter].CinematicBrightnessAdd = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].CinematicBrightnessAdd->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].CinematicBrightnessAdd->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kCinematicContrastMult) {
				if (!g_patchMap[imageSpaceAdapter].CinematicContrastMult.has_value()) {
					g_patchMap[imageSpaceAdapter].CinematicContrastMult = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].CinematicContrastMult->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].CinematicContrastMult->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else if (a_configData.Element == ElementType::kCinematicContrastAdd) {
				if (!g_patchMap[imageSpaceAdapter].CinematicContrastAdd.has_value()) {
					g_patchMap[imageSpaceAdapter].CinematicContrastAdd = PatchData::FloatInterpolatorData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].CinematicContrastAdd->Clear = true;
					}
					else if (operation.Type == OperationType::kAdd) {
						auto data = std::any_cast<ConfigData::Operation::FloatData>(operation.Data.value());
						g_patchMap[imageSpaceAdapter].CinematicContrastAdd->AddVec.push_back({ data.Time, data.Value });
					}
				}
			}
			else {
				logger::warn("Invalid ElementName: '{}'.", ElementTypeToString(a_configData.Element));
				return;
			}
		}
	}

	std::vector<PatchData::FloatInterpolatorData::FloatData> GetFloatData(RE::NiFloatInterpolator* a_interpolator)
	{
		std::vector<PatchData::FloatInterpolatorData::FloatData> floatDataVec;
		if (!a_interpolator || !a_interpolator->floatData) {
			return floatDataVec;
		}

		auto numKeys = a_interpolator->floatData->numKeys;
		auto data = a_interpolator->floatData->data;

		if (numKeys == 0 || !data) {
			return floatDataVec;
		}

		for (std::uint32_t floatIndex = 0; floatIndex < numKeys; floatIndex++) {
			floatDataVec.push_back({ data[floatIndex].time, data[floatIndex].value });
		}

		return floatDataVec;
	}

	std::vector<PatchData::ColorInterpolatorData::ColorData> GetColorData(RE::NiColorInterpolator* a_interpolator)
	{
		std::vector<PatchData::ColorInterpolatorData::ColorData> colorDataVec;
		if (!a_interpolator || !a_interpolator->colorData) {
			return colorDataVec;
		}

		auto numKeys = a_interpolator->colorData->numKeys;
		auto data = a_interpolator->colorData->data;
		if (numKeys == 0 || !data) {
			return colorDataVec;
		}

		for (std::uint32_t colorIndex = 0; colorIndex < numKeys; colorIndex++) {
			colorDataVec.push_back({ data[colorIndex].time, data[colorIndex].color });
		}

		return colorDataVec;
	}

	void SetFloatData(std::uint32_t& a_keySize, RE::NiPointer<RE::NiFloatInterpolator>& a_interpolator, const std::vector<PatchData::FloatInterpolatorData::FloatData>& a_floatDataVec)
	{
		if (a_floatDataVec.empty()) {
			a_keySize = 0;

			if (a_interpolator && a_interpolator->floatData) {
				if (a_interpolator->floatData->data) {
					RE::free(a_interpolator->floatData->data);
					a_interpolator->floatData->data = nullptr;
				}

				a_interpolator->floatData->numKeys = 0;
				a_interpolator->floatData->type = 0;
				a_interpolator->floatData->keySize = 0;
			}

			a_interpolator.reset();
			return;
		}

		if (!a_interpolator.get()) {
			auto storage = RE::malloc(sizeof(RE::NiFloatInterpolator));
			if (!storage) {
				logger::error("Failed to allocate memory for NiFloatInterpolator.");
				return;
			}
			a_interpolator = RE::NiPointer<RE::NiFloatInterpolator>(new (storage) RE::NiFloatInterpolator());
		}

		if (!a_interpolator->floatData) {
			auto storage = RE::malloc(sizeof(RE::NiFloatData));
			if (!storage) {
				logger::error("Failed to allocate memory for NiFloatData.");
				return;
			}
			a_interpolator->floatData = RE::NiPointer<RE::NiFloatData>(new (storage) RE::NiFloatData());
		}

		if (a_interpolator->floatData->data) {
			RE::free(a_interpolator->floatData->data);
			a_interpolator->floatData->data = nullptr;
		}

		auto dataStorage = RE::malloc(sizeof(RE::NiFloatData::Data) * a_floatDataVec.size());
		if (!dataStorage) {
			logger::error("Failed to allocate memory for NiFloatData::Data.");
			return;
		}

		for (std::size_t floatIndex = 0; floatIndex < a_floatDataVec.size(); floatIndex++) {
			auto& [time, value] = a_floatDataVec[floatIndex];
			auto& data = reinterpret_cast<RE::NiFloatData::Data*>(dataStorage)[floatIndex];
			data = {};
			data.time = time;
			data.value = value;
		}

		a_keySize = static_cast<std::uint32_t>(a_floatDataVec.size());

		a_interpolator->floatData->numKeys = static_cast<std::uint32_t>(a_floatDataVec.size());
		a_interpolator->floatData->data = reinterpret_cast<RE::NiFloatData::Data*>(dataStorage);
		a_interpolator->floatData->type = 1;
		a_interpolator->floatData->keySize = sizeof(RE::NiFloatData::Data);
	}

	void SetColorData(std::uint32_t& a_keySize, RE::NiPointer<RE::NiColorInterpolator>& a_interpolator, const std::vector<PatchData::ColorInterpolatorData::ColorData>& a_colorDataVec)
	{
		if (a_colorDataVec.empty()) {
			a_keySize = 0;

			if (a_interpolator && a_interpolator->colorData) {
				if (a_interpolator->colorData->data) {
					RE::free(a_interpolator->colorData->data);
					a_interpolator->colorData->data = nullptr;
				}

				a_interpolator->colorData->numKeys = 0;
				a_interpolator->colorData->type = 0;
				a_interpolator->colorData->keySize = 0;
			}

			a_interpolator.reset();
			return;
		}

		if (!a_interpolator.get()) {
			auto storage = RE::malloc(sizeof(RE::NiColorInterpolator));
			if (!storage) {
				logger::error("Failed to allocate memory for NiColorInterpolator.");
				return;
			}
			a_interpolator = RE::NiPointer<RE::NiColorInterpolator>(new (storage) RE::NiColorInterpolator());
		}

		if (!a_interpolator->colorData) {
			auto storage = RE::malloc(sizeof(RE::NiColorData));
			if (!storage) {
				logger::error("Failed to allocate memory for NiColorData.");
				return;
			}
			a_interpolator->colorData = RE::NiPointer<RE::NiColorData>(new (storage) RE::NiColorData());
		}

		if (a_interpolator->colorData->data) {
			RE::free(a_interpolator->colorData->data);
			a_interpolator->colorData->data = nullptr;
		}

		auto dataStorage = RE::malloc(sizeof(RE::NiColorData::Data) * a_colorDataVec.size());
		if (!dataStorage) {
			logger::error("Failed to allocate memory for NiColorData::Data.");
			return;
		}

		for (std::size_t colorIndex = 0; colorIndex < a_colorDataVec.size(); colorIndex++) {
			auto& [time, color] = a_colorDataVec[colorIndex];
			auto& data = reinterpret_cast<RE::NiColorData::Data*>(dataStorage)[colorIndex];
			data = {};
			data.time = time;
			data.color = color;
		}

		a_keySize = static_cast<std::uint32_t>(a_colorDataVec.size());

		a_interpolator->colorData->numKeys = static_cast<std::uint32_t>(a_colorDataVec.size());
		a_interpolator->colorData->data = reinterpret_cast<RE::NiColorData::Data*>(dataStorage);
		a_interpolator->colorData->type = 1;
		a_interpolator->colorData->keySize = sizeof(RE::NiColorData::Data);
	}

	void PatchFloatInterpolator(std::uint32_t& a_keySize, RE::NiPointer<RE::NiFloatInterpolator>& a_interpolator, const PatchData::FloatInterpolatorData& a_floatInterpolatorData)
	{
		bool isCleared = false, isAdded = false;
		std::vector<PatchData::FloatInterpolatorData::FloatData> floatDataVec;

		if (a_floatInterpolatorData.Clear) {
			isCleared = true;
		}
		else {
			floatDataVec = GetFloatData(a_interpolator.get());
		}

		for (const auto& data : a_floatInterpolatorData.AddVec) {
			floatDataVec.push_back(data);
			isAdded = true;
		}

		if (isCleared || isAdded) {
			SetFloatData(a_keySize, a_interpolator, floatDataVec);
		}
	}

	void PatchColorInterpolator(std::uint32_t& a_keySize, RE::NiPointer<RE::NiColorInterpolator>& a_interpolator, const PatchData::ColorInterpolatorData& a_tintColorData)
	{
		bool isCleared = false, isAdded = false;
		std::vector<PatchData::ColorInterpolatorData::ColorData> colorDataVec;

		if (a_tintColorData.Clear) {
			isCleared = true;
		}
		else {
			colorDataVec = GetColorData(a_interpolator.get());
		}

		for (const auto& data : a_tintColorData.AddVec) {
			colorDataVec.push_back(data);
			isAdded = true;
		}

		if (isCleared || isAdded) {
			SetColorData(a_keySize, a_interpolator, colorDataVec);
		}
	}

	void Patch()
	{
		logger::info("======================== Start preparing patch for {} ========================", TypeName);

		ConfigUtils::Prepare(g_configVec, Prepare);

		logger::info("======================== Finished preparing patch for {} ========================", TypeName);
		logger::info("");

		logger::info("======================== Start patching for {} ========================", TypeName);

		for (const auto& [imageSpaceAdapter, patchData] : g_patchMap) {
			if (patchData.Animatable.has_value()) {
				imageSpaceAdapter->data.animatable = patchData.Animatable.value();
			}

			if (patchData.Duration.has_value()) {
				imageSpaceAdapter->data.duration = patchData.Duration.value();
			}

			if (patchData.BlurRadius.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.blurKeySize, imageSpaceAdapter->blurInterpolator, patchData.BlurRadius.value());
			}

			if (patchData.DoubleVisionStrength.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.doubleKeySize, imageSpaceAdapter->doubleInterpolator, patchData.DoubleVisionStrength.value());
			}

			if (patchData.TintColor.has_value()) {
				PatchColorInterpolator(imageSpaceAdapter->data.tintColorKeySize, imageSpaceAdapter->tintColorInterpolator, patchData.TintColor.value());
			}

			if (patchData.FadeColor.has_value()) {
				PatchColorInterpolator(imageSpaceAdapter->data.fadeColorKeySize, imageSpaceAdapter->fadeColorInterpolator, patchData.FadeColor.value());
			}

			if (patchData.RadialBlurUseTarget.has_value()) {
				imageSpaceAdapter->data.useTargetForRadialBlur = patchData.RadialBlurUseTarget.value();
			}

			if (patchData.RadialBlurCenterX.has_value()) {
				imageSpaceAdapter->data.radialBlurCenter.x = patchData.RadialBlurCenterX.value();
			}

			if (patchData.RadialBlurCenterY.has_value()) {
				imageSpaceAdapter->data.radialBlurCenter.y = patchData.RadialBlurCenterY.value();
			}

			if (patchData.RadialBlurStrength.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.radialBlurStrengthKeySize, imageSpaceAdapter->radialBlurStrengthInterpolator, patchData.RadialBlurStrength.value());
			}

			if (patchData.RadialBlurRampUp.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.radialBlurRampupKeySize, imageSpaceAdapter->radialBlurRampupInterpolator, patchData.RadialBlurRampUp.value());	
			}

			if (patchData.RadialBlurRampDown.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.radialBlurRampDownKeySize, imageSpaceAdapter->radialBlurRampDownInterpolator, patchData.RadialBlurRampDown.value());
			}

			if (patchData.RadialBlurStart.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.radialBlurStartKeySize, imageSpaceAdapter->radialBlurStartInterpolator, patchData.RadialBlurStart.value());
			}

			if (patchData.RadialBlurDownStart.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.radialBlurDownStartKeySize, imageSpaceAdapter->radialBlurDownStartInterpolator, patchData.RadialBlurDownStart.value());
			}

			if (patchData.DepthOfFieldUseTarget.has_value()) {
				imageSpaceAdapter->data.useTargetForDepthOfField = patchData.DepthOfFieldUseTarget.value();
			}

			if (patchData.DepthOfFieldStrength.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.depthOfFieldStrengthKeySize, imageSpaceAdapter->depthOfFieldStrengthInterpolator, patchData.DepthOfFieldStrength.value());
			}

			if (patchData.DepthOfFieldDistance.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.depthOfFieldDistanceKeySize, imageSpaceAdapter->depthOfFieldDistanceInterpolator, patchData.DepthOfFieldDistance.value());
			}

			if (patchData.DepthOfFieldRange.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.depthOfFieldRangeKeySize, imageSpaceAdapter->depthOfFieldRangeInterpolator, patchData.DepthOfFieldRange.value());
			}

			if (patchData.DepthOfFieldVignetteRadius.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.depthOfFieldVignetteRadiusKeySize, imageSpaceAdapter->depthOfFieldVignetteRadiusInterpolator, patchData.DepthOfFieldVignetteRadius.value());
			}

			if (patchData.DepthOfFieldVignetteStrength.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.depthOfFieldVignetteStrengthKeySize, imageSpaceAdapter->depthOfFieldVignetteStrengthInterpolator, patchData.DepthOfFieldVignetteStrength.value());
			}

			if (patchData.MotionBlurStrength.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.motionBlurStrengthKeySize, imageSpaceAdapter->motionBlurStrengthInterpolator, patchData.MotionBlurStrength.value());
			}

			if (patchData.HDREyeAdaptSpeedMult.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.keySize[0][0], imageSpaceAdapter->interpolator[0][0], patchData.HDREyeAdaptSpeedMult.value());
			}

			if (patchData.HDREyeAdaptSpeedAdd.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.keySize[0][1], imageSpaceAdapter->interpolator[0][1], patchData.HDREyeAdaptSpeedAdd.value());
			}

			if (patchData.HDRBloomBlurRadiusMult.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.keySize[1][0], imageSpaceAdapter->interpolator[1][0], patchData.HDRBloomBlurRadiusMult.value());
			}

			if (patchData.HDRBloomBlurRadiusAdd.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.keySize[1][1], imageSpaceAdapter->interpolator[1][1], patchData.HDRBloomBlurRadiusAdd.value());
			}

			if (patchData.HDRBloomThresholdMult.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.keySize[2][0], imageSpaceAdapter->interpolator[2][0], patchData.HDRBloomThresholdMult.value());
			}

			if (patchData.HDRBloomThresholdAdd.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.keySize[2][1], imageSpaceAdapter->interpolator[2][1], patchData.HDRBloomThresholdAdd.value());
			}

			if (patchData.HDRBloomScaleMult.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.keySize[3][0], imageSpaceAdapter->interpolator[3][0], patchData.HDRBloomScaleMult.value());
			}

			if (patchData.HDRBloomScaleAdd.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.keySize[3][1], imageSpaceAdapter->interpolator[3][1], patchData.HDRBloomScaleAdd.value());
			}

			if (patchData.HDRTargetLumMinMult.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.keySize[4][0], imageSpaceAdapter->interpolator[4][0], patchData.HDRTargetLumMinMult.value());
			}

			if (patchData.HDRTargetLumMinAdd.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.keySize[4][1], imageSpaceAdapter->interpolator[4][1], patchData.HDRTargetLumMinAdd.value());
			}

			if (patchData.HDRTargetLumMaxMult.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.keySize[5][0], imageSpaceAdapter->interpolator[5][0], patchData.HDRTargetLumMaxMult.value());
			}

			if (patchData.HDRTargetLumMaxAdd.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.keySize[5][1], imageSpaceAdapter->interpolator[5][1], patchData.HDRTargetLumMaxAdd.value());
			}

			if (patchData.HDRSunlightScaleMult.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.keySize[6][0], imageSpaceAdapter->interpolator[6][0], patchData.HDRSunlightScaleMult.value());
			}

			if (patchData.HDRSunlightScaleAdd.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.keySize[6][1], imageSpaceAdapter->interpolator[6][1], patchData.HDRSunlightScaleAdd.value());
			}

			if (patchData.HDRSkyScaleMult.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.keySize[7][0], imageSpaceAdapter->interpolator[7][0], patchData.HDRSkyScaleMult.value());
			}

			if (patchData.HDRSkyScaleAdd.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.keySize[7][1], imageSpaceAdapter->interpolator[7][1], patchData.HDRSkyScaleAdd.value());
			}

			if (patchData.CinematicSaturationMult.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.keySize[17][0], imageSpaceAdapter->interpolator[17][0], patchData.CinematicSaturationMult.value());
			}

			if (patchData.CinematicSaturationAdd.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.keySize[17][1], imageSpaceAdapter->interpolator[17][1], patchData.CinematicSaturationAdd.value());
			}

			if (patchData.CinematicBrightnessMult.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.keySize[18][0], imageSpaceAdapter->interpolator[18][0], patchData.CinematicBrightnessMult.value());
			}

			if (patchData.CinematicBrightnessAdd.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.keySize[18][1], imageSpaceAdapter->interpolator[18][1], patchData.CinematicBrightnessAdd.value());
			}

			if (patchData.CinematicContrastMult.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.keySize[19][0], imageSpaceAdapter->interpolator[19][0], patchData.CinematicContrastMult.value());
			}

			if (patchData.CinematicContrastAdd.has_value()) {
				PatchFloatInterpolator(imageSpaceAdapter->data.keySize[19][1], imageSpaceAdapter->interpolator[19][1], patchData.CinematicContrastAdd.value());
			}
		}

		logger::info("======================== Finished patching for {} ========================", TypeName);
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}
