#pragma once

namespace Configs {
	class ConfigReader {
	public:
		ConfigReader(std::string_view a_path);
		bool EndOfFile() const;
		std::string_view GetToken();
		std::string_view Peek();
		std::size_t GetLastLine() const;
		std::size_t GetLastLineIndex() const;

	protected:
		bool IsDelimiter(char ch) const;
		void ParseTokens();

		struct Token {
			std::string_view value;
			std::size_t line;
			std::size_t column;
		};

		std::string _fileContents;
		std::vector<Token> _tokens;
		std::size_t _currentTokenIndex;
		std::size_t _lastTokenIndex;
	};
}
