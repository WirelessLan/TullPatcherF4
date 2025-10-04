#include "ImageSpaceAdapters.h"

#include <any>
#include <regex>

#include "ConfigUtils.h"

namespace RE
{
	class NiAnimationKey
	{
	public:
		enum class KeyContent
		{
			kFloat,
			kPos,
			kRot,
			kColor,
			kText,
			kBool,

			kTotal
		};

		enum class KeyType
		{
			kNoInterp,
			kLink,
			kBez,
			kTCB,
			kEuler,
			kStrip,

			kTotal
		};

	protected:
		float _time;  // 00
	};
	static_assert(sizeof(NiAnimationKey) == 0x4);

	class NiInterpolator : public NiObject
	{
	public:
		inline static constexpr auto RTTI = RE::RTTI::NiInterpolator;
		inline static auto Ni_RTTI = RE::Ni_RTTI::NiInterpolator;

		float lastTime;       // 10
		std::uint32_t pad14;  // 14
	};
	static_assert(sizeof(NiInterpolator) == 0x18);

	class NiKeyBasedInterpolator : public NiInterpolator
	{
	public:
		inline static constexpr auto RTTI = RE::RTTI::NiKeyBasedInterpolator;
		inline static auto Ni_RTTI = RE::Ni_RTTI::NiKeyBasedInterpolator;

		using KeyType = NiAnimationKey::KeyType;
		using KeyContent = NiAnimationKey::KeyContent;
	};
	static_assert(sizeof(NiKeyBasedInterpolator) == 0x18);

	class NiFloatKey : public NiAnimationKey
	{
	protected:
		float _value;  // 04
	};
	static_assert(sizeof(NiFloatKey) == 0x8);

	class NiFloatData : public NiObject
	{
	public:
		inline static constexpr auto RTTI = RE::RTTI::NiFloatData;
		inline static auto Ni_RTTI = RE::Ni_RTTI::NiFloatData;

		using KeyType = NiFloatKey::KeyType;

		// members
		std::uint32_t numKeys;  // 10
		std::uint32_t pad14;    // 14
		NiFloatKey* keys;       // 18
		KeyType type;           // 20
		std::uint8_t keySize;   // 24
		std::uint8_t pad25;     // 25
		std::uint16_t pad26;    // 26
	};
	static_assert(sizeof(NiFloatData) == 0x28);

	class NiFloatInterpolator : public NiKeyBasedInterpolator
	{
	public:
		inline static constexpr auto RTTI = RE::RTTI::NiFloatInterpolator;
		inline static auto Ni_RTTI = RE::Ni_RTTI::NiFloatInterpolator;

		// members
		float floatValue;                  // 18
		std::uint32_t pad1C;               // 1C
		NiPointer<NiFloatData> floatData;  // 20
		std::uint32_t lastIndex;           // 28
		std::uint32_t pad2C;               // 2C
	};
	static_assert(sizeof(NiFloatInterpolator) == 0x30);
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
		kBlurRadius
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
		default:
			return std::string_view{};
		}
	}

	enum class OperationType
	{
		kClear
	};

	std::string_view OperationTypeToString(OperationType a_value)
	{
		switch (a_value) {
		case OperationType::kClear:
			return "Clear";
		default:
			return std::string_view{};
		}
	}

	struct ConfigData
	{
		struct Operation
		{
			OperationType Type;
		};

		FilterType Filter;
		std::string FilterForm;
		ElementType Element;
		std::optional<std::any> AssignValue;
		std::vector<Operation> Operations;
	};

	struct PatchData
	{
		struct FloatData
		{
			float Time;
			float Value;
		};

		struct BlurRadiusData
		{
			bool Clear;
		};

		std::optional<bool> Animatable;
		std::optional<float> Duration;
		std::optional<BlurRadiusData> BlurRadius;
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
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element), std::any_cast<bool>(a_configData.AssignValue.value()));
				break;

			case ElementType::kDuration:
				logger::info("{}{}({}).{} = {};", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element), std::any_cast<float>(a_configData.AssignValue.value()));
				break;

			case ElementType::kBlurRadius:
				logger::info("{}{}({}).{}", indent, FilterTypeToString(a_configData.Filter), a_configData.FilterForm, ElementTypeToString(a_configData.Element));
				for (std::size_t opIndex = 0; opIndex < a_configData.Operations.size(); opIndex++) {
					std::string opLog;

					switch (a_configData.Operations[opIndex].Type) {
					case OperationType::kClear:
						opLog = fmt::format(".{}()", OperationTypeToString(a_configData.Operations[opIndex].Type));
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

			if (a_config.Element == ElementType::kAnimatable) {
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
			else if (a_config.Element == ElementType::kDuration) {
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
			else {
				logger::warn("Line {}, Col {}: Invalid OperationName '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return false;
			}

			bool isValidOperation = [](ElementType elem, OperationType op) {
				if (elem == ElementType::kBlurRadius) {
					return op == OperationType::kClear;
				}
				return false;
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

			if (a_config.Element == ElementType::kBlurRadius) {
				if (newOp.Type != OperationType::kClear) {
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
					g_patchMap[imageSpaceAdapter].BlurRadius = PatchData::BlurRadiusData{};
				}

				for (const auto& operation : a_configData.Operations) {
					if (operation.Type == OperationType::kClear) {
						g_patchMap[imageSpaceAdapter].BlurRadius->Clear = true;
					}
				}
			}
			else {
				logger::warn("Invalid ElementName: '{}'.", ElementTypeToString(a_configData.Element));
				return;
			}
		}
	}

	void PatchBlurRadius(RE::TESImageSpaceModifier* a_imageSpaceAdapter, const PatchData::BlurRadiusData& a_blurRadiusData)
	{
		if (a_blurRadiusData.Clear) {
			a_imageSpaceAdapter->blurInterpolator.reset();
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
				PatchBlurRadius(imageSpaceAdapter, patchData.BlurRadius.value());
			}
		}

		logger::info("======================== Finished patching for {} ========================", TypeName);
		logger::info("");

		g_configVec.clear();
		g_patchMap.clear();
	}
}
