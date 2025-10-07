#pragma once

#include "Configs.h"
#include "Utils.h"

namespace Parsers {
	constexpr std::string_view PluginExistsConditionName = "IsPluginExists";
	constexpr std::string_view FormExistsConditionName = "IsFormExists";

	struct Condition {
		enum class ConditionType {
			kFunction
		};

		ConditionType Type;
		std::string Name;
		std::string Params;
	};

	struct ConditionToken {
		enum class TokenType {
			kCondition,
			kOperator,
			kParenthesis
		};

		TokenType Type;
		std::optional<Condition> Condition = std::nullopt;
		std::optional<std::string> Operator = std::nullopt;
	};

	bool EvaluateConditions(const std::vector<ConditionToken>& a_conditions);

	enum class StatementType {
		kConditional,
		kExpression,
		kNone,
	};

	template<typename T>
	class ConditionalStatement;

	template<typename T>
	class Statement {
	public:
		static Statement<T> CreateConditionalStatement(const ConditionalStatement<T>& a_conditionalStatement) {
			Statement<T> retStatement;
			retStatement.Type = StatementType::kConditional;
			retStatement.ConditionalStatement = a_conditionalStatement;
			return retStatement;
		}

		static Statement<T> CreateExpressionStatement(const T& a_expressionStatement) {
			Statement<T> retStatement;
			retStatement.Type = StatementType::kExpression;
			retStatement.ExpressionStatement = a_expressionStatement;
			return retStatement;
		}

		StatementType Type = StatementType::kNone;
		std::optional<ConditionalStatement<T>> ConditionalStatement = std::nullopt;
		std::optional<T> ExpressionStatement = std::nullopt;
	};

	template<typename T>
	class ConditionalStatement {
	public:
		std::pair<std::vector<ConditionToken>, std::vector<Statement<T>>> IfStatements;
		std::vector<std::pair<std::vector<ConditionToken>, std::vector<Statement<T>>>> ElseIfStatements;
		std::vector<Statement<T>> ElseStatements;

		const std::vector<Statement<T>>& Evaluates() const {
			if (EvaluateConditions(IfStatements.first)) {
				return IfStatements.second;
			}
			
			for (const auto& elseIfStatement : ElseIfStatements) {
				if (EvaluateConditions(elseIfStatement.first)) {
					return elseIfStatement.second;
				}
			}

			return ElseStatements;
		}
	};

	template<typename T>
	class Parser {
	public:
		Parser(std::string_view a_configPath) : reader(a_configPath) {}

		std::vector<Statement<T>> Parse() {
			std::vector<Statement<T>> retVec;

			while (!reader.EndOfFile()) {
				auto parsedStatement = ParseStatement();
				if (!parsedStatement.has_value()) {
					break;
				}

				PrintStatement(parsedStatement.value(), 0);

				retVec.push_back(parsedStatement.value());
			}

			return retVec;
		}

	protected:
		virtual std::optional<Statement<T>> ParseExpressionStatement() = 0;

		void PrintStatement(const Statement<T>& a_statement, int a_indent) {
			if (a_statement .Type == StatementType::kExpression) {
				PrintExpressionStatement(a_statement.ExpressionStatement.value(), a_indent);
			}
			else if (a_statement.Type == StatementType::kConditional) {
				PrintConditionalStatement(a_statement.ConditionalStatement.value(), a_indent);
			}
		}

		virtual void PrintExpressionStatement(const T& a_expressionStatement, int a_indent) = 0;

		std::string ConditionsToString(const std::vector<ConditionToken>& a_conditions) {
			std::string retStr;

			for (const ConditionToken& conditionToken : a_conditions) {
				if (conditionToken.Type == ConditionToken::TokenType::kParenthesis) {
					retStr += conditionToken.Operator.value();
				}
				else if (conditionToken.Type == ConditionToken::TokenType::kOperator) {
					if (conditionToken.Operator == "!") {
						retStr += conditionToken.Operator.value();
					}
					else {
						retStr += " " + conditionToken.Operator.value() + " ";
					}
				}
				else {
					retStr += conditionToken.Condition->Name + "(" + conditionToken.Condition->Params + ")";
				}
			}

			return retStr;
		}

		void PrintConditionalStatement(const ConditionalStatement<T>& a_conditionalStatement, int a_indent) {
			std::string indent = std::string(a_indent * 4, ' ');
			std::string logmsg;

			logmsg = "if (";
			logmsg += ConditionsToString(a_conditionalStatement.IfStatements.first);
			logmsg += ")";

			logger::info("{}{}", indent, logmsg);
			logger::info("{}{{", indent);

			for (const Statement<T> statement : a_conditionalStatement.IfStatements.second) {
				PrintStatement(statement, a_indent + 1);
			}

			logger::info("{}}}", indent);

			for (const std::pair<std::vector<ConditionToken>, std::vector<Statement<T>>>& elseIfStatement : a_conditionalStatement.ElseIfStatements) {
				logmsg = "else if (";
				logmsg += ConditionsToString(elseIfStatement.first);
				logmsg += ")";

				logger::info("{}{}", indent, logmsg);
				logger::info("{}{{", indent);

				for (const Statement<T> statement : elseIfStatement.second) {
					PrintStatement(statement, a_indent + 1);
				}

				logger::info("{}}}", indent);
			}

			if (!a_conditionalStatement.ElseStatements.empty()) {
				logmsg = "else";

				logger::info("{}{}", indent, logmsg);
				logger::info("{}{{", indent);

				for (const Statement<T> statement : a_conditionalStatement.ElseStatements) {
					PrintStatement(statement, a_indent + 1);
				}

				logger::info("{}}}", indent);
			}
		}

		std::optional<Statement<T>> ParseStatement() {
			auto token = reader.Peek();
			if (token == "if") {
				return ParseConditionalStatement();
			}
			return ParseExpressionStatement();
		}

		std::optional<Statement<T>> ParseConditionalStatement() {
			if (reader.EndOfFile() || reader.Peek().empty()) {
				return std::nullopt;
			}

			ConditionalStatement<T> conditionalStatement;

			auto token = reader.GetToken();
			if (token != "if") {
				logger::warn("Line {}, Col {}: Syntax error. Expected 'if'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			token = reader.GetToken();
			if (token != "(") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '('.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			std::vector<ConditionToken> ifConditions = ParseConditions();
			if (ifConditions.empty()) {
				return std::nullopt;
			}

			token = reader.GetToken();
			if (token != ")") {
				logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			token = reader.GetToken();
			if (token != "{") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '{{'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			std::vector<Statement<T>> ifStatements;

			while (true) {
				token = reader.Peek();
				if (token == "}") {
					reader.GetToken();
					break;
				}

				std::optional<Statement<T>> parsedStatement = ParseStatement();
				if (!parsedStatement.has_value()) {
					return std::nullopt;
				}

				ifStatements.push_back(parsedStatement.value());
			}

			conditionalStatement.IfStatements = std::make_pair(ifConditions, ifStatements);

			token = reader.Peek();
			if (token != "else") {
				return Statement<T>::CreateConditionalStatement(conditionalStatement);
			}

			while (true) {
				reader.GetToken();

				bool isElseStatement = true;

				std::vector<ConditionToken> elseIfConditions;
				std::vector<Statement<T>> elseIfStatements;

				token = reader.Peek();
				if (token == "if") {
					isElseStatement = false;

					reader.GetToken();

					token = reader.GetToken();
					if (token != "(") {
						logger::warn("Line {}, Col {}: Syntax error. Expected '('.", reader.GetLastLine(), reader.GetLastLineIndex());
						return std::nullopt;
					}

					elseIfConditions = ParseConditions();
					if (elseIfConditions.empty()) {
						return std::nullopt;
					}

					token = reader.GetToken();
					if (token != ")") {
						logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
						return std::nullopt;
					}
				}

				token = reader.GetToken();
				if (token != "{") {
					logger::warn("Line {}, Col {}: Syntax error. Expected '{{'.", reader.GetLastLine(), reader.GetLastLineIndex());
					return std::nullopt;
				}

				while (true) {
					token = reader.Peek();
					if (token == "}") {
						reader.GetToken();
						break;
					}

					auto parsedStatement = ParseStatement();
					if (!parsedStatement.has_value()) {
						return std::nullopt;
					}

					elseIfStatements.push_back(parsedStatement.value());
				}

				if (!isElseStatement) {
					conditionalStatement.ElseIfStatements.push_back(std::make_pair(elseIfConditions, elseIfStatements));
				}
				else {
					conditionalStatement.ElseStatements = elseIfStatements;
					break;
				}

				token = reader.Peek();
				if (token != "else") {
					break;
				}
			}

			return Statement<T>::CreateConditionalStatement(conditionalStatement);
		}

		std::vector<ConditionToken> ParseConditions() {
			std::vector<ConditionToken> retVec;

			auto token = reader.Peek();
			if (token == ")") {
				logger::warn("Line {}, Col {}: Syntax error. Operand expected.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::vector<ConditionToken>{};
			}

			while (true) {
				token = reader.Peek();
				if (token == ")") {
					break;
				}

				token = reader.GetToken();
				if (token == "(") {
					retVec.push_back(ConditionToken{ ConditionToken::TokenType::kParenthesis, std::nullopt, std::string(token) });

					auto subConditions = ParseConditions();
					if (subConditions.empty()) {
						return std::vector<ConditionToken>{};
					}

					retVec.insert(retVec.end(), subConditions.begin(), subConditions.end());

					token = reader.GetToken();
					if (token != ")") {
						logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
						return std::vector<ConditionToken>{};
					}

					retVec.push_back(ConditionToken{ ConditionToken::TokenType::kParenthesis, std::nullopt, std::string(token) });
				}
				else {
					if (token == PluginExistsConditionName || token == FormExistsConditionName) {
						if (!retVec.empty() && retVec.back().Type != ConditionToken::TokenType::kOperator) {
							logger::warn("Line {}, Col {}: Syntax error. Operator expected.", reader.GetLastLine(), reader.GetLastLineIndex());
							return std::vector<ConditionToken>{};
						}

						std::string conditionName = std::string(token);

						token = reader.GetToken();
						if (token != "(") {
							logger::warn("Line {}, Col {}: Syntax error. Expected '('.", reader.GetLastLine(), reader.GetLastLineIndex());
							return std::vector<ConditionToken>{};
						}

						if (conditionName == PluginExistsConditionName) {
							token = reader.GetToken();
							if (token.empty()) {
								logger::warn("Line {}, Col {}: Expected pluginName.", reader.GetLastLine(), reader.GetLastLineIndex());
								return std::vector<ConditionToken>{};
							}

							if (!token.starts_with('\"')) {
								logger::warn("Line {}, Col {}: PluginName must be a string.", reader.GetLastLine(), reader.GetLastLineIndex());
								return std::vector<ConditionToken>{};
							}
							else if (!token.ends_with('\"')) {
								logger::warn("Line {}, Col {}: String must end with '\"'.", reader.GetLastLine(), reader.GetLastLineIndex());
								return std::vector<ConditionToken>{};
							}

							std::string pluginName(token.substr(1, token.length() - 2));
							retVec.push_back(ConditionToken{ ConditionToken::TokenType::kCondition, Condition{ Condition::ConditionType::kFunction, conditionName, pluginName }, std::nullopt });
						}
						else if (conditionName == FormExistsConditionName) {
							auto parsedForm = ParseForm();
							if (!parsedForm.has_value()) {
								return std::vector<ConditionToken>{};
							}

							retVec.push_back(ConditionToken{ ConditionToken::TokenType::kCondition, Condition{ Condition::ConditionType::kFunction, conditionName, parsedForm.value() }, std::nullopt });
						}

						token = reader.GetToken();
						if (token != ")") {
							logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
							return std::vector<ConditionToken>{};
						}
					}
					else if (token == "&" || token == "|") {
						std::string _operator(token);

						if (token == "&") {
							token = reader.GetToken();
							if (token != "&") {
								logger::warn("Line {}, Col {}: Syntax error. Expected '&&'.", reader.GetLastLine(), reader.GetLastLineIndex());
								return std::vector<ConditionToken>{};
							}
							_operator = "&&";
						}
						else if (token == "|") {
							token = reader.GetToken();
							if (token != "|") {
								logger::warn("Line {}, Col {}: Syntax error. Expected '||'.", reader.GetLastLine(), reader.GetLastLineIndex());
								return std::vector<ConditionToken>{};
							}
							_operator = "||";
						}

						if (retVec.empty() || retVec.back().Type == ConditionToken::TokenType::kOperator) {
							logger::warn("Line {}, Col {}: Syntax error. Operand expected.", reader.GetLastLine(), reader.GetLastLineIndex());
							return std::vector<ConditionToken>{};
						}

						retVec.push_back(ConditionToken{ ConditionToken::TokenType::kOperator, std::nullopt, _operator });
					}
					else if (token == "!") {
						std::string _operator(token);

						if (!retVec.empty() && retVec.back().Type == ConditionToken::TokenType::kOperator && retVec.back().Operator == "!") {
							logger::warn("Line {}, Col {}: Syntax error. Operand expected.", reader.GetLastLine(), reader.GetLastLineIndex());
							return std::vector<ConditionToken>{};
						}
						else if ((!retVec.empty() && retVec.back().Type == ConditionToken::TokenType::kParenthesis && retVec.back().Operator == ")")
							|| (!retVec.empty() && retVec.back().Type == ConditionToken::TokenType::kCondition)) {
							logger::warn("Line {}, Col {}: Syntax error. Operator or ')' expected.", reader.GetLastLine(), reader.GetLastLineIndex());
							return std::vector<ConditionToken>{};
						}

						retVec.push_back(ConditionToken{ ConditionToken::TokenType::kOperator, std::nullopt, _operator });
					}
					else {
						logger::warn("Line {}, Col {}: Syntax error. Unknown keyword '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
						return std::vector<ConditionToken>{};
					}
				}
			}

			if (retVec.empty() || retVec.back().Type == ConditionToken::TokenType::kOperator) {
				logger::warn("Line {}, Col {}: Syntax error. Operand expected.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::vector<ConditionToken>{};
			}

			return retVec;
		}

		bool IsHexString(std::string_view a_token) {
			if (a_token.empty()) {
				return false;
			}

			if (a_token.starts_with("0x") || a_token.starts_with("0X")) {
				a_token.remove_prefix(2);
				if (a_token.empty()) {
					return false;
				}
			}

			return std::all_of(a_token.begin(), a_token.end(), [](unsigned char c) { return std::isxdigit(c); });
		}

		std::optional<std::string> ParseForm() {
			std::string form;

			auto token = reader.GetToken();
			if (!token.starts_with('\"')) {
				logger::warn("Line {}, Col {}: PluginName must be a string.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}
			else if (!token.ends_with('\"')) {
				logger::warn("Line {}, Col {}: String must end with '\"'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}
			form += token.substr(1, token.length() - 2);

			token = reader.GetToken();
			if (token != "|") {
				logger::warn("Line {}, Col {}: Syntax error. Expected '|'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}
			form += token;

			token = reader.GetToken();
			if (!IsHexString(token)) {
				logger::warn("Line {}, Col {}: Expected FormID '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return std::nullopt;
			}
			form += token;

			return form;
		}

		std::optional<float> ParseNumber() {
			auto token = reader.GetToken();
			if (token.empty()) {
				logger::warn("Line {}, Col {}: Expected value.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			std::string numStr = std::string(token);
			if (reader.Peek() == ".") {
				numStr += reader.GetToken();

				token = reader.GetToken();
				if (token.empty()) {
					logger::warn("Line {}, Col {}: Expected decimal value.", reader.GetLastLine(), reader.GetLastLineIndex());
					return std::nullopt;
				}

				numStr += std::string(token);
			}

			if (!Utils::IsValidDecimalNumber(numStr)) {
				logger::warn("Line {}, Col {}: Failed to parse value '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), numStr);
				return std::nullopt;
			}

			float parsedValue;
			auto parseResult = std::from_chars(numStr.data(), numStr.data() + numStr.size(), parsedValue);
			if (parseResult.ec != std::errc()) {
				logger::warn("Line {}, Col {}: Failed to parse value '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), numStr);
				return std::nullopt;
			}

			return parsedValue;
		}

		std::optional<std::uint32_t> ParseBipedSlot() {
			auto token = reader.GetToken();
			if (token.empty()) {
				logger::warn("Line {}, Col {}: Expected bipedSlot.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			unsigned long parsedValue = 0;

			auto parseResult = std::from_chars(token.data(), token.data() + token.size(), parsedValue);
			if (parseResult.ec != std::errc()) {
				logger::warn("Line {}, Col {}: Failed to parse bipedSlot '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return std::nullopt;
			}

			if (parsedValue != 0 && (parsedValue < 30 || parsedValue > 61)) {
				logger::warn("Line {}, Col {}: Failed to parse bipedSlot '{}'. The value is out of range", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return std::nullopt;
			}

			return static_cast<std::uint32_t>(parsedValue);
		}

		std::string GetBipedSlots(std::uint32_t a_bipedObjSlots) {
			std::string retStr;
			const std::string separator = " | ";

			if (a_bipedObjSlots == 0) {
				return "0";
			}

			for (std::size_t slotIndex = 0; slotIndex < 32; ++slotIndex) {
				if (a_bipedObjSlots & (1u << slotIndex)) {
					retStr += std::to_string(slotIndex + 30) + separator;
				}
			}

			if (!retStr.empty()) {
				retStr.erase(retStr.size() - separator.size());
			}

			return retStr;
		}

		Configs::ConfigReader reader;
	};
}
