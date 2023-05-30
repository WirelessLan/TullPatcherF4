#include "Configs.h"

#include <fstream>

#include "Utils.h"

namespace Configs {
	ConfigReader::ConfigReader(std::string_view a_path) {
		std::ifstream configFile(std::string(a_path).c_str());
		if (!configFile.is_open()) {
			logger::warn("Cannot open the config file: {}", a_path);
			return;
		}

		std::stringstream buffer;
		buffer << configFile.rdbuf();

		_fileContents = buffer.str();

		buffer.clear();
		buffer.seekg(0);

		std::string line;
		while (std::getline(buffer, line))
			_lines.push_back(line);
	}

	bool ConfigReader::EndOfFile() {
		if (_currIndex >= _fileContents.size())
			return true;
		return false;
	}

	std::string_view ConfigReader::GetToken() {
		std::uint8_t ch;
		std::size_t startIdx = _currIndex;
		std::size_t tokenLen = 0;

		while (true) {
			ch = GetChar();
			if (ch == 0xFF)
				break;

			if (ch == '#') {
				if (tokenLen != 0) {
					UndoGetChar();
					break;
				}

				while (true) {
					ch = GetChar();
					if (ch == '\n') {
						UndoGetChar();
						break;
					}
					else if (ch == 0xFF)
						break;
				}

				startIdx = _currIndex;
				continue;
			}
			else if (ch == '\n') {
				if (tokenLen != 0) {
					UndoGetChar();
					break;
				}

				_currLine++;
				startIdx = _currIndex;
				continue;
			}
			else if (std::isspace(static_cast<int>(ch))) {
				if (tokenLen != 0) {
					UndoGetChar();
					break;
				}

				startIdx = _currIndex;
				continue;
			}
			else if (ch == '\"') {
				if (tokenLen != 0) {
					UndoGetChar();
					break;
				}

				tokenLen++;

				while (true) {
					ch = GetChar();
					if (ch == '\"') {
						tokenLen++;
						break;
					}
					else if (ch == '\n') {
						UndoGetChar();
						break;
					}
					else if (ch == 0xFF)
						break;

					tokenLen++;
				}

				break;
			}
			else if (ch == '.' || ch == ',' || ch == '=' || ch == '|' || ch == '(' || ch == ')' || ch == ';') {
				if (tokenLen != 0) {
					UndoGetChar();
					break;
				}

				tokenLen++;
				break;
			}

			tokenLen++;
		}
		
		return std::string_view(_fileContents).substr(startIdx, tokenLen);
	}

	std::string_view ConfigReader::LookAhead() {
		std::size_t lastIndex = _currIndex;
		std::size_t lastLine = _currLine;
		std::string_view token = GetToken();
		_currLine = lastLine;
		_currIndex = lastIndex;
		return token;
	}

	std::size_t ConfigReader::GetLastLine()	{
		return _currLine + 1;
	}

	std::size_t ConfigReader::GetLastLineIndex() {
		std::size_t retIndex = 0;
		for (std::size_t ii = 0; ii < _currLine; ii++)
			retIndex += _lines[ii].size() + 1;
		return _currIndex - retIndex + 1;
	}

	std::uint8_t ConfigReader::GetChar() {
		if (!EndOfFile())
			return _fileContents[_currIndex++];
		return 0xFF;
	}

	void ConfigReader::UndoGetChar() {
		if (_currIndex > 0)
			_currIndex--;
	}
}
