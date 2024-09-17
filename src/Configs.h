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
		int GetChar();
		void UndoGetChar();

		std::string _fileContents;
		std::vector<std::string> _lines;
		std::size_t _currIndex;
		std::size_t _currLine;
	};
}
