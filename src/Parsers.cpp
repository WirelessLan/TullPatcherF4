#include "Parsers.h"

#include "Utils.h"

namespace Parsers
{
	bool EvaluateCondition(const Condition& a_condition)
	{
		if (a_condition.Name == kPluginExistsConditionName)
		{
			return Utils::IsPluginExists(a_condition.Params);
		}
		else if (a_condition.Name == kFormExistsConditionName)
		{
			return Utils::GetFormFromString(a_condition.Params) != nullptr;
		}
		return false;
	}

	bool EvaluateConditions(const std::vector<ConditionToken>& a_conditions)
	{
		std::stack<ConditionToken> opStack;
		std::queue<ConditionToken> outQueue;

		for (const auto& conditionToken : a_conditions)
		{
			switch (conditionToken.Type)
			{
			case ConditionToken::TokenType::kCondition:
				outQueue.push(conditionToken);
				break;

			case ConditionToken::TokenType::kParenthesis:
				if (conditionToken.Operator == "(")
				{
					opStack.push(conditionToken);
				}
				else
				{
					bool foundOpeningParenthesis = false;
					while (!opStack.empty())
					{
						if (opStack.top().Type == ConditionToken::TokenType::kParenthesis && opStack.top().Operator == "(")
						{
							foundOpeningParenthesis = true;
							opStack.pop();
							break;
						}

						outQueue.push(opStack.top());
						opStack.pop();
					}

					if (!foundOpeningParenthesis)
					{
						logger::critical("Failed to evaluate conditional statement: unmatched ')'.");
						return false;
					}
				}
				break;

			case ConditionToken::TokenType::kOperator:
				if (opStack.empty())
				{
					opStack.push(conditionToken);
					continue;
				}

				if (opStack.top().Type == ConditionToken::TokenType::kCondition || opStack.top().Type == ConditionToken::TokenType::kParenthesis)
				{
					opStack.push(conditionToken);
				}
				else if (opStack.top().Type == ConditionToken::TokenType::kOperator)
				{
					if ((conditionToken.Operator == "&&" && opStack.top().Operator == "||") ||
						(conditionToken.Operator == "!" && opStack.top().Operator == "&&") ||
						(conditionToken.Operator == "!" && opStack.top().Operator == "||"))
					{
						opStack.push(conditionToken);
					}
					else
					{
						outQueue.push(opStack.top());
						opStack.pop();
						opStack.push(conditionToken);
					}
				}
				break;

			default:
				break;
			}
		}

		while (!opStack.empty())
		{
			if (opStack.top().Type == ConditionToken::TokenType::kParenthesis)
			{
				logger::critical("Failed to evaluate conditional statement: unmatched '('.");
				return false;
			}

			outQueue.push(opStack.top());
			opStack.pop();
		}

		std::stack<bool> evalStack;

		while (!outQueue.empty())
		{
			const auto token = outQueue.front();
			outQueue.pop();

			if (token.Type == ConditionToken::TokenType::kCondition)
			{
				if (!token.Condition.has_value())
				{
					logger::critical("Failed to evaluate conditional statement: missing condition.");
					return false;
				}

				evalStack.push(EvaluateCondition(token.Condition.value()));
			}
			else if (token.Type == ConditionToken::TokenType::kOperator)
			{
				if (token.Operator == "&&" || token.Operator == "||")
				{
					if (evalStack.size() < 2)
					{
						logger::critical("Failed to evaluate conditional statement: missing operand.");
						return false;
					}

					const auto right = evalStack.top();
					evalStack.pop();
					const auto left = evalStack.top();
					evalStack.pop();

					if (token.Operator == "&&")
					{
						evalStack.push(left && right);
					}
					else if (token.Operator == "||")
					{
						evalStack.push(left || right);
					}
				}
				else if (token.Operator == "!")
				{
					if (evalStack.empty())
					{
						logger::critical("Failed to evaluate conditional statement: missing operand.");
						return false;
					}

					const auto right = evalStack.top();
					evalStack.pop();

					evalStack.push(!right);
				}
				else
				{
					logger::critical("Failed to evaluate conditional statement: unknown operator.");
					return false;
				}
			}
		}

		if (evalStack.size() != 1)
		{
			logger::critical("Failed to evaluate conditional statement's condition.");
			return false;
		}

		return evalStack.top();
	}
}  // namespace Parsers
