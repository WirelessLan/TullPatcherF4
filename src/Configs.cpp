#include "Configs.h"

#include <fstream>

#include "Utils.h"

namespace Configs {
	ConfigReader::ConfigReader(std::string_view a_path) : currentTokenIndex_(0), lastTokenIndex_(0) {
		std::ifstream configFile;

		configFile.open(a_path);
		if (!configFile.is_open())
		{
			logger::warn("Cannot open the config file: {}", a_path);
			return;
		}

		std::stringstream buffer;
		buffer << configFile.rdbuf();

		fileContents_ = buffer.str();

		ParseTokens();
	}

	bool ConfigReader::EndOfFile() const {
		return currentTokenIndex_ >= tokens_.size();
	}

	std::string_view ConfigReader::GetToken() {
		if (EndOfFile())
		{
			return {};
		}
		lastTokenIndex_ = currentTokenIndex_;
		return tokens_[currentTokenIndex_++].value;
	}

	std::string_view ConfigReader::Peek() {
		if (EndOfFile())
		{
			return {};
		}
		lastTokenIndex_ = currentTokenIndex_;
		return tokens_[currentTokenIndex_].value;
	}

	std::size_t ConfigReader::GetLastLine() const {
		return tokens_[lastTokenIndex_].line;
	}

	std::size_t ConfigReader::GetLastLineIndex() const {
		return tokens_[lastTokenIndex_].column;
	}

	bool ConfigReader::IsDelimiter(char ch) const {
		return ch == '.' || ch == ',' || ch == '=' || ch == '!' || ch == '&' || ch == '|' || ch == ';' ||
		       ch == '(' || ch == ')' || ch == '{' || ch == '}' || ch == '[' || ch == ']';
	}

	void ConfigReader::ParseTokens() {
		std::size_t index = 0;
		std::size_t line = 1;
		std::size_t column = 1;
		const auto fileLength = fileContents_.size();

		while (index < fileLength)
		{
			const auto ch = fileContents_[index];

			// Handle comments
			if (ch == '#')
			{
				while (index < fileLength && fileContents_[index] != '\n')
				{
					index++;
					column++;
				}
			}
			// Handle newline characters
			else if (ch == '\n')
			{
				index++;
				line++;
				column = 1;
			}
			// Handle whitespace characters
			else if (std::isspace(static_cast<unsigned char>(ch)))
			{
				while (index < fileLength && std::isspace(static_cast<unsigned char>(fileContents_[index])) && fileContents_[index] != '\n')
				{
					index++;
					column++;
				}
			}
			// Handle string literals
			else if (ch == '\"')
			{
				const auto startIdx = index++;
				const auto startLine = line;
				const auto startColumn = column++;

                while (index < fileLength)
				{
					const auto current = fileContents_[index];
					if (current == '\"')
					{
						index++;
						column++;
						break;
					}
					else if (current == '\n')
					{
						break;
					}
					else
					{
						index++;
						column++;
					}
				}

				const auto tokenLen = index - startIdx;
				const auto tokenValue = std::string_view(fileContents_).substr(startIdx, tokenLen);
				tokens_.emplace_back(Token{ tokenValue, startLine, startColumn });
			}
			// Handle delimiters as individual tokens
			else if (IsDelimiter(ch))
			{
				const auto startIdx = index++;
				const auto startLine = line;
				const auto startColumn = column++;
				const auto tokenLen = 1;
				const auto tokenValue = std::string_view(fileContents_).substr(startIdx, tokenLen);
				tokens_.emplace_back(Token{ tokenValue, startLine, startColumn });
			}
			// Handle general tokens
			else
			{
				const auto startIdx = index;
				const auto startLine = line;
				const auto startColumn = column;
				std::size_t tokenLen = 0;

				while (index < fileLength)
				{
					const auto current = fileContents_[index];
					if (std::isspace(static_cast<unsigned char>(current)) || current == '#' || current == '\n' || IsDelimiter(current))
					{
						break;
					}

					index++;
					column++;
					tokenLen++;
				}

				if (tokenLen > 0)
				{
					const auto tokenValue = std::string_view(fileContents_).substr(startIdx, tokenLen);
					tokens_.emplace_back(Token{ tokenValue, startLine, startColumn });
				}
			}
		}
	}
}
