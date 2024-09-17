#include "Configs.h"

#include <fstream>

#include "Utils.h"

namespace Configs {
	constexpr int EOF_CHAR = std::char_traits<char>::eof();

	ConfigReader::ConfigReader(std::string_view a_path) : _currIndex(0), _currLine(0) {
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

	bool ConfigReader::EndOfFile() const {
		return _currIndex >= _fileContents.size();
	}

	std::string_view ConfigReader::GetToken() {
		int ch;
		std::size_t startIdx = _currIndex;
		std::size_t tokenLen = 0;

		while (true) {
			ch = GetChar();
			if (ch == EOF_CHAR)
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
					else if (ch == EOF_CHAR)
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
			else if (std::isspace(ch)) {
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
					else if (ch == EOF_CHAR)
						break;

					tokenLen++;
				}

				break;
			}
			else if (ch == '.' || ch == ',' || ch == '=' || ch == '!' || ch == '&' || ch == '|' || ch == '(' || ch == ')' || ch == ';') {
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

	std::string_view ConfigReader::Peek() {
		std::size_t lastIndex = _currIndex;
		std::size_t lastLine = _currLine;
		std::string_view token = GetToken();
		_currLine = lastLine;
		_currIndex = lastIndex;
		return token;
	}

	std::size_t ConfigReader::GetLastLine() const	{
		return _currLine + 1;
	}

	std::size_t ConfigReader::GetLastLineIndex() const {
		std::size_t retIndex = 0;
		for (std::size_t ii = 0; ii < _currLine; ii++)
			retIndex += _lines[ii].size() + 1;
		return _currIndex - retIndex + 1;
	}

	int ConfigReader::GetChar() {
		if (!EndOfFile())
			return _fileContents[_currIndex++];
		return EOF_CHAR;
	}

	void ConfigReader::UndoGetChar() {
		if (_currIndex > 0)
			_currIndex--;
	}
}
