#pragma once

#include "Parsers.h"

namespace ConfigUtils
{
	template <typename ParserT, typename StatementT>
	inline std::vector<StatementT> ReadConfigs(std::string_view a_configType)
	{
		const std::filesystem::path configDir{ "Data\\" + std::string(Version::PROJECT) + "\\" + std::string(a_configType) };
		if (!std::filesystem::exists(configDir))
		{
			return {};
		}

		static const std::regex kFilter(".*\\.cfg", std::regex_constants::icase);
		std::vector<StatementT> configs;

		for (const auto& entry : std::filesystem::recursive_directory_iterator(configDir))
		{
			if (!std::filesystem::is_regular_file(entry.status()))
			{
				continue;
			}

			if (!std::regex_match(entry.path().filename().string(), kFilter))
			{
				continue;
			}

			const auto path = entry.path().string();
			logger::info("=========== Reading {} config file: {} ===========", a_configType, path);

			const auto parsedStatements = ParserT(path).Parse();
			configs.insert(configs.end(), parsedStatements.begin(), parsedStatements.end());

			logger::info("");
		}

		return configs;
	}

	template <typename StatementT, typename PrepareF>
	inline void Prepare(const std::vector<StatementT>& a_configVec, PrepareF a_prepareFunc)
	{
		for (const auto& configData : a_configVec)
		{
			if (configData.Type == Parsers::StatementType::kExpression)
			{
				a_prepareFunc(configData.ExpressionStatement.value());
			}
			else if (configData.Type == Parsers::StatementType::kConditional)
			{
				Prepare(configData.ConditionalStatement->Evaluates(), a_prepareFunc);
			}
		}
	}
}  // namespace ConfigUtils
