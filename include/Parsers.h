#pragma once

#include <type_traits>

#include "Configs.h"
#include "Utils.h"

namespace Parsers
{
	constexpr std::string_view kPluginExistsConditionName = "IsPluginExists";
	constexpr std::string_view kFormExistsConditionName = "IsFormExists";

	struct Condition
	{
		enum class ConditionType
		{
			kFunction
		};

		ConditionType Type;
		std::string Name;
		std::string Params;
	};

	struct ConditionToken
	{
		enum class TokenType
		{
			kCondition,
			kOperator,
			kParenthesis
		};

		TokenType Type;
		std::optional<Condition> Condition = std::nullopt;
		std::optional<std::string> Operator = std::nullopt;
	};

	bool EvaluateConditions(const std::vector<ConditionToken>& a_conditions);

	enum class StatementType
	{
		kConditional,
		kExpression,
		kNone,
	};

	template <typename T>
	class ConditionalStatement;

	template <typename T>
	class Statement
	{
	public:
		static Statement<T> CreateConditionalStatement(const ConditionalStatement<T>& a_conditionalStatement)
		{
			Statement<T> retStatement;
			retStatement.Type = StatementType::kConditional;
			retStatement.ConditionalStatement = a_conditionalStatement;
			return retStatement;
		}

		static Statement<T> CreateExpressionStatement(const T& a_expressionStatement)
		{
			Statement<T> retStatement;
			retStatement.Type = StatementType::kExpression;
			retStatement.ExpressionStatement = a_expressionStatement;
			return retStatement;
		}

		StatementType Type = StatementType::kNone;
		std::optional<ConditionalStatement<T>> ConditionalStatement = std::nullopt;
		std::optional<T> ExpressionStatement = std::nullopt;
	};

	template <typename T>
	class ConditionalStatement
	{
	public:
		std::pair<std::vector<ConditionToken>, std::vector<Statement<T>>> IfStatements;
		std::vector<std::pair<std::vector<ConditionToken>, std::vector<Statement<T>>>> ElseIfStatements;
		std::vector<Statement<T>> ElseStatements;

		const std::vector<Statement<T>>& Evaluates() const
		{
			if (EvaluateConditions(IfStatements.first))
			{
				return IfStatements.second;
			}

			for (const auto& elseIfStatement : ElseIfStatements)
			{
				if (EvaluateConditions(elseIfStatement.first))
				{
					return elseIfStatement.second;
				}
			}

			return ElseStatements;
		}
	};

	template <typename T>
	class Parser
	{
	public:
		Parser(std::string_view a_configPath) : reader(a_configPath) {}
		virtual ~Parser() = default;

		std::vector<Statement<T>> Parse()
		{
			std::vector<Statement<T>> statements;

			while (!reader.EndOfFile())
			{
				const auto statementOpt = ParseStatement();
				if (!statementOpt.has_value())
				{
					break;
				}

				PrintStatement(statementOpt.value(), 0);
				statements.emplace_back(statementOpt.value());
			}

			return statements;
		}

	protected:
		virtual std::optional<Statement<T>> ParseExpressionStatement() = 0;

		void PrintStatement(const Statement<T>& a_statement, int a_indent)
		{
			if (a_statement.Type == StatementType::kExpression)
			{
				PrintExpressionStatement(a_statement.ExpressionStatement.value(), a_indent);
			}
			else if (a_statement.Type == StatementType::kConditional)
			{
				PrintConditionalStatement(a_statement.ConditionalStatement.value(), a_indent);
			}
		}

		virtual void PrintExpressionStatement(const T& a_expressionStatement, int a_indent) = 0;

		std::string ConditionsToString(const std::vector<ConditionToken>& a_conditions)
		{
			std::string conditionsStr;

			for (const ConditionToken& conditionToken : a_conditions)
			{
				switch (conditionToken.Type)
				{
				case ConditionToken::TokenType::kParenthesis:
					conditionsStr += conditionToken.Operator.value();
					break;

				case ConditionToken::TokenType::kOperator:
					if (conditionToken.Operator == "!")
					{
						conditionsStr += conditionToken.Operator.value();
					}
					else
					{
						conditionsStr += " " + conditionToken.Operator.value() + " ";
					}
					break;

				default:
					conditionsStr += conditionToken.Condition->Name + "(" + conditionToken.Condition->Params + ")";
					break;
				}
			}

			return conditionsStr;
		}

		void PrintConditionalStatement(const ConditionalStatement<T>& a_conditionalStatement, int a_indent)
		{
			auto indent = std::string(a_indent * 4, ' ');
			std::string logmsg;

			logmsg = "if (";
			logmsg += ConditionsToString(a_conditionalStatement.IfStatements.first);
			logmsg += ")";

			logger::info("{}{}", indent, logmsg);
			logger::info("{}{{", indent);

			for (const auto& statement : a_conditionalStatement.IfStatements.second)
			{
				PrintStatement(statement, a_indent + 1);
			}

			logger::info("{}}}", indent);

			for (const auto& elseIfStatement : a_conditionalStatement.ElseIfStatements)
			{
				logmsg = "else if (";
				logmsg += ConditionsToString(elseIfStatement.first);
				logmsg += ")";

				logger::info("{}{}", indent, logmsg);
				logger::info("{}{{", indent);

				for (const auto& statement : elseIfStatement.second)
				{
					PrintStatement(statement, a_indent + 1);
				}

				logger::info("{}}}", indent);
			}

			if (!a_conditionalStatement.ElseStatements.empty())
			{
				logmsg = "else";

				logger::info("{}{}", indent, logmsg);
				logger::info("{}{{", indent);

				for (const auto& statement : a_conditionalStatement.ElseStatements)
				{
					PrintStatement(statement, a_indent + 1);
				}

				logger::info("{}}}", indent);
			}
		}

		std::optional<Statement<T>> ParseStatement()
		{
			const auto token = reader.Peek();
			return (token == "if") ? ParseConditionalStatement() : ParseExpressionStatement();
		}

		std::optional<Statement<T>> ParseConditionalStatement()
		{
			ConditionalStatement<T> conditionalStatement;

			auto token = reader.GetToken();
			if (token != "if")
			{
				logger::warn("Line {}, Col {}: Syntax error. Expected 'if'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			token = reader.GetToken();
			if (token != "(")
			{
				logger::warn("Line {}, Col {}: Syntax error. Expected '('.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			auto ifConditions = ParseConditions();
			if (ifConditions.empty())
			{
				return std::nullopt;
			}

			token = reader.GetToken();
			if (token != ")")
			{
				logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			token = reader.GetToken();
			if (token != "{")
			{
				logger::warn("Line {}, Col {}: Syntax error. Expected '{{'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			std::vector<Statement<T>> ifStatements;

			while (reader.Peek() != "}")
			{
				const auto statementOpt = ParseStatement();
				if (!statementOpt.has_value())
				{
					return std::nullopt;
				}

				ifStatements.emplace_back(statementOpt.value());
			}
			reader.GetToken();  // ;

			conditionalStatement.IfStatements = std::make_pair(ifConditions, ifStatements);

			while (reader.Peek() == "else")
			{
				reader.GetToken();

				bool isElseStatement = true;
				std::vector<ConditionToken> elseIfConditions;
				std::vector<Statement<T>> elseIfStatements;

				token = reader.Peek();
				if (token == "if")
				{
					reader.GetToken();

					isElseStatement = false;

					token = reader.GetToken();
					if (token != "(")
					{
						logger::warn("Line {}, Col {}: Syntax error. Expected '('.", reader.GetLastLine(), reader.GetLastLineIndex());
						return std::nullopt;
					}

					elseIfConditions = ParseConditions();
					if (elseIfConditions.empty())
					{
						return std::nullopt;
					}

					token = reader.GetToken();
					if (token != ")")
					{
						logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
						return std::nullopt;
					}
				}

				token = reader.GetToken();
				if (token != "{")
				{
					logger::warn("Line {}, Col {}: Syntax error. Expected '{{'.", reader.GetLastLine(), reader.GetLastLineIndex());
					return std::nullopt;
				}

				while (reader.Peek() != "}")
				{
					const auto statementOpt = ParseStatement();
					if (!statementOpt.has_value())
					{
						return std::nullopt;
					}

					elseIfStatements.emplace_back(statementOpt.value());
				}
				reader.GetToken();  // ;

				if (isElseStatement)
				{
					conditionalStatement.ElseStatements = elseIfStatements;
					break;
				}

				conditionalStatement.ElseIfStatements.emplace_back(std::make_pair(elseIfConditions, elseIfStatements));
			}

			return Statement<T>::CreateConditionalStatement(conditionalStatement);
		}

		std::vector<ConditionToken> ParseConditions()
		{
			std::vector<ConditionToken> conditions;

			while (reader.Peek() != ")")
			{
				auto token = reader.GetToken();
				if (token == "(")
				{
					if (!conditions.empty() && conditions.back().Type != ConditionToken::TokenType::kOperator)
					{
						logger::warn("Line {}, Col {}: Syntax error. Operator expected.", reader.GetLastLine(), reader.GetLastLineIndex());
						return {};
					}

					conditions.emplace_back(ConditionToken{ ConditionToken::TokenType::kParenthesis, std::nullopt, std::string(token) });

					auto subConditions = ParseConditions();
					if (subConditions.empty())
					{
						return {};
					}

					conditions.insert(conditions.end(), subConditions.begin(), subConditions.end());

					token = reader.GetToken();
					if (token != ")")
					{
						logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
						return {};
					}

					conditions.emplace_back(ConditionToken{ ConditionToken::TokenType::kParenthesis, std::nullopt, std::string(token) });
				}
				else
				{
					if (token == kPluginExistsConditionName || token == kFormExistsConditionName)
					{
						if (!conditions.empty() && conditions.back().Type != ConditionToken::TokenType::kOperator)
						{
							logger::warn("Line {}, Col {}: Syntax error. Operator expected.", reader.GetLastLine(), reader.GetLastLineIndex());
							return {};
						}

						std::string conditionName{ token };

						token = reader.GetToken();
						if (token != "(")
						{
							logger::warn("Line {}, Col {}: Syntax error. Expected '('.", reader.GetLastLine(), reader.GetLastLineIndex());
							return {};
						}

						if (conditionName == kPluginExistsConditionName)
						{
							const auto pluginNameOpt = ParseString();
							if (!pluginNameOpt.has_value())
							{
								return {};
							}

							conditions.emplace_back(ConditionToken{ ConditionToken::TokenType::kCondition, Condition{ Condition::ConditionType::kFunction, conditionName, pluginNameOpt.value() }, std::nullopt });
						}
						else if (conditionName == kFormExistsConditionName)
						{
							const auto formOpt = ParseForm();
							if (!formOpt.has_value())
							{
								return {};
							}

							conditions.push_back(ConditionToken{ ConditionToken::TokenType::kCondition, Condition{ Condition::ConditionType::kFunction, conditionName, formOpt.value() }, std::nullopt });
						}

						token = reader.GetToken();
						if (token != ")")
						{
							logger::warn("Line {}, Col {}: Syntax error. Expected ')'.", reader.GetLastLine(), reader.GetLastLineIndex());
							return {};
						}
					}
					else if (token == "&" || token == "|")
					{
						std::string operator_(token);

						if (token == "&")
						{
							token = reader.GetToken();
							if (token != "&")
							{
								logger::warn("Line {}, Col {}: Syntax error. Expected '&&'.", reader.GetLastLine(), reader.GetLastLineIndex());
								return {};
							}
							operator_ = "&&";
						}
						else if (token == "|")
						{
							token = reader.GetToken();
							if (token != "|")
							{
								logger::warn("Line {}, Col {}: Syntax error. Expected '||'.", reader.GetLastLine(), reader.GetLastLineIndex());
								return {};
							}
							operator_ = "||";
						}

						if (conditions.empty() || conditions.back().Type == ConditionToken::TokenType::kOperator)
						{
							logger::warn("Line {}, Col {}: Syntax error. Operand expected.", reader.GetLastLine(), reader.GetLastLineIndex());
							return {};
						}

						conditions.emplace_back(ConditionToken{ ConditionToken::TokenType::kOperator, std::nullopt, operator_ });
					}
					else if (token == "!")
					{
						std::string operator_(token);

						if (!conditions.empty() && conditions.back().Type == ConditionToken::TokenType::kOperator && conditions.back().Operator == "!")
						{
							logger::warn("Line {}, Col {}: Syntax error. Operand expected.", reader.GetLastLine(), reader.GetLastLineIndex());
							return {};
						}
						else if ((!conditions.empty() && conditions.back().Type == ConditionToken::TokenType::kParenthesis && conditions.back().Operator == ")") ||
								 (!conditions.empty() && conditions.back().Type == ConditionToken::TokenType::kCondition))
						{
							logger::warn("Line {}, Col {}: Syntax error. Operator or ')' expected.", reader.GetLastLine(), reader.GetLastLineIndex());
							return {};
						}

						conditions.emplace_back(ConditionToken{ ConditionToken::TokenType::kOperator, std::nullopt, operator_ });
					}
					else
					{
						logger::warn("Line {}, Col {}: Syntax error. Unknown keyword '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
						return {};
					}
				}
			}

			if (conditions.empty() || conditions.back().Type == ConditionToken::TokenType::kOperator)
			{
				logger::warn("Line {}, Col {}: Syntax error. Operand expected.", reader.GetLastLine(), reader.GetLastLineIndex());
				return {};
			}

			return conditions;
		}

		bool IsHexString(std::string_view a_token)
		{
			if (a_token.empty())
			{
				return false;
			}

			if (a_token.starts_with("0x") || a_token.starts_with("0X"))
			{
				a_token.remove_prefix(2);
				if (a_token.empty())
				{
					return false;
				}
			}

			return std::all_of(a_token.begin(), a_token.end(), [](unsigned char c) { return std::isxdigit(c); });
		}

		std::optional<std::string> ParseForm()
		{
			std::string form;

			const auto pluginNameOpt = ParseString();
			if (!pluginNameOpt.has_value())
			{
				return std::nullopt;
			}
			form += pluginNameOpt.value();

			auto token = reader.GetToken();
			if (token != "|")
			{
				logger::warn("Line {}, Col {}: Syntax error. Expected '|'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}
			form += token;

			token = reader.GetToken();
			if (!IsHexString(token))
			{
				logger::warn("Line {}, Col {}: Expected FormID '{}'.", reader.GetLastLine(), reader.GetLastLineIndex(), token);
				return std::nullopt;
			}
			form += token;

			return form;
		}

		std::optional<std::string> ParseString()
		{
			const auto token = reader.GetToken();

			if (!token.starts_with('\"'))
			{
				logger::warn("Line {}, Col {}: String must start with '\"'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			if (!token.ends_with('\"'))
			{
				logger::warn("Line {}, Col {}: String must end with '\"'.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			return std::string(token.substr(1, token.length() - 2));
		}

		template <typename T>
		std::optional<T> ParseNumber()
		{
			auto token = reader.GetToken();
			if (token.empty())
			{
				logger::warn("Line {}, Col {}: Expected value.", reader.GetLastLine(), reader.GetLastLineIndex());
				return std::nullopt;
			}

			std::string numStr{ token };

			if constexpr (std::is_floating_point_v<T>)
			{
				if (reader.Peek() == ".")
				{
					numStr += reader.GetToken();

					token = reader.GetToken();
					if (token.empty())
					{
						logger::warn("Line {}, Col {}: Expected decimal value.", reader.GetLastLine(), reader.GetLastLineIndex());
						return std::nullopt;
					}

					numStr += token;
				}
			}

			T parsedValue{};
			if (!Utils::ConvertNumber(numStr, parsedValue))
			{
				logger::warn("Line {}, Col {}: Failed to parse value '{}'. The value must be a number", reader.GetLastLine(), reader.GetLastLineIndex(), numStr);
				return std::nullopt;
			}

			return parsedValue;
		}

		std::optional<std::uint32_t> ParseBipedSlot()
		{
			const auto parsedValueOpt = ParseNumber<std::uint32_t>();
			if (!parsedValueOpt.has_value())
			{
				return std::nullopt;
			}

			const auto parsedValue = parsedValueOpt.value();
			if (parsedValue != 0 && (parsedValue < 30 || parsedValue > 61))
			{
				logger::warn("Line {}, Col {}: Failed to parse bipedSlot '{}'. The value is out of range", reader.GetLastLine(), reader.GetLastLineIndex(), parsedValue);
				return std::nullopt;
			}

			return parsedValue;
		}

		std::string GetBipedSlots(std::uint32_t a_bipedObjSlots)
		{
			static constexpr std::uint32_t kBipedSlotOffset = 30u;
			static constexpr std::uint32_t kMaxBipedSlotCount = 32u;
			static constexpr std::string_view kSeparator = " | ";

			if (a_bipedObjSlots == 0)
			{
				return "0";
			}

			std::string bipedSlots;

			for (std::uint32_t slotIndex = 0; slotIndex < kMaxBipedSlotCount; ++slotIndex)
			{
				if ((a_bipedObjSlots & (1u << slotIndex)) == 0)
				{
					continue;
				}

				if (!bipedSlots.empty())
				{
					bipedSlots += kSeparator;
				}

				bipedSlots += std::to_string(slotIndex + kBipedSlotOffset);
			}

			return bipedSlots;
		}

		Configs::ConfigReader reader;
	};
}  // namespace Parsers
