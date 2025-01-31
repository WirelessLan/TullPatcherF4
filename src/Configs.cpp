#include "Configs.h"

#include <fstream>

#include "Utils.h"

namespace Configs {
	constexpr int EOF_CHAR = std::char_traits<char>::eof();

	ConfigReader::ConfigReader(std::string_view a_path) : _currentTokenIndex(0), _lastTokenIndex(0) {
		std::ifstream configFile(std::string(a_path).c_str());
		if (!configFile.is_open()) {
			logger::warn("Cannot open the config file: {}", a_path);
			return;
		}

		std::stringstream buffer;
		buffer << configFile.rdbuf();

		_fileContents = buffer.str();

		ParseTokens();
	}

	bool ConfigReader::EndOfFile() const {
		return _currentTokenIndex >= _tokens.size();
	}

	std::string_view ConfigReader::GetToken() {
		if (EndOfFile()) {
			return std::string_view{};
		}
		_lastTokenIndex = _currentTokenIndex;
		return _tokens[_currentTokenIndex++].value;
	}

	std::string_view ConfigReader::Peek() {
		if (EndOfFile()) {
			return std::string_view{};
		}
		_lastTokenIndex = _currentTokenIndex;
		return _tokens[_currentTokenIndex].value;
	}

	std::size_t ConfigReader::GetLastLine() const {
		return _tokens[_lastTokenIndex].line;
	}

	std::size_t ConfigReader::GetLastLineIndex() const {
		return _tokens[_lastTokenIndex].column;
	}

	bool ConfigReader::IsDelimiter(char ch) const {
		return ch == '.' || ch == ',' || ch == '=' || ch == '!' ||
		       ch == '&' || ch == '|' || ch == '(' || ch == ')' || ch == ';';
	}

	void ConfigReader::ParseTokens() {
		std::size_t index = 0;
		std::size_t line = 1;
		std::size_t column = 1;
		const std::size_t fileLength = _fileContents.size();

		while (index < fileLength) {
			char ch = _fileContents[index];

			// Handle comments
			if (ch == '#') {
				while (index < fileLength && _fileContents[index] != '\n') {
					index++;
					column++;
				}
			}
			// Handle newline characters
			else if (ch == '\n') {
				index++;
				line++;
				column = 1;
			}
			// Handle whitespace characters
			else if (std::isspace(static_cast<int>(ch))) {
				while (index < fileLength && std::isspace(static_cast<int>(_fileContents[index])) && _fileContents[index] != '\n') {
					index++;
					column++;
				}
			}
			// Handle string literals
			else if (ch == '\"') {
				std::size_t startIdx = index;
				std::size_t startLine = line;
				std::size_t startColumn = column;

				index++;
				column++;

                while (index < fileLength) {
					char current = _fileContents[index];

					if (current == '\"') {
						index++;
						column++;
						break;
					} else if (current == '\n') {
						break;
					} else {
						index++;
						column++;
					}
				}

				std::size_t tokenLen = index - startIdx;
				std::string_view tokenValue = std::string_view(_fileContents).substr(startIdx, tokenLen);
				_tokens.emplace_back(Token{ tokenValue, startLine, startColumn });
			}
			// Handle delimiters as individual tokens
			else if (IsDelimiter(ch)) {
				std::size_t startIdx = index;
				std::size_t startLine = line;
				std::size_t startColumn = column;
				std::size_t tokenLen = 1;
				std::string_view tokenValue = std::string_view(_fileContents).substr(startIdx, tokenLen);
				_tokens.emplace_back(Token{ tokenValue, startLine, startColumn });
				index++;
				column++;
			}
			// Handle general tokens
			else {
				std::size_t startIdx = index;
				std::size_t startLine = line;
				std::size_t startColumn = column;
				std::size_t tokenLen = 0;

				while (index < fileLength) {
					char current = _fileContents[index];
					if (std::isspace(static_cast<int>(current)) || current == '#' || current == '\n' || IsDelimiter(current)) {
						break;
					}
					index++;
					column++;
					tokenLen++;
				}

				if (tokenLen > 0) {
					std::string_view tokenValue = std::string_view(_fileContents).substr(startIdx, tokenLen);
					_tokens.emplace_back(Token{ tokenValue, startLine, startColumn });
				}
			}
		}
	}
}
