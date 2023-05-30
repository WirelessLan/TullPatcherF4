#pragma once

namespace Configs {
	class ConfigReader {
	public:
		ConfigReader(std::string_view a_path);
		bool EndOfFile();
		std::string_view GetToken();
		std::string_view LookAhead();
		std::size_t GetLastLine();
		std::size_t GetLastLineIndex();

	protected:
		std::uint8_t GetChar();
		void UndoGetChar();

		std::string _fileContents;
		std::vector<std::string> _lines;
		std::size_t _currIndex = 0;
		std::size_t _currLine = 0;
	};

	template<class T>
	class Parser {
	public:
		Parser(Configs::ConfigReader& a_configReader) : reader(a_configReader) {}

		virtual std::optional<T> Parse() = 0;

		void RecoverFromError() {
			while (!reader.EndOfFile()) {
				auto token = reader.GetToken();
				if (token == ";")
					break;
			}
		}

	protected:
		Configs::ConfigReader& reader;
	};
}
