#include "Parsers.h"

#include "Utils.h"

namespace Parsers {
	bool EvaluateCondition(const Condition& a_condition) {
		if (a_condition.Name == PluginExistsConditionName) {
			return Utils::IsPluginExists(a_condition.Params);
		}
		else if (a_condition.Name == FormExistsConditionName) {
			return Utils::GetFormFromString(a_condition.Params) != nullptr;
		}
		return false;
	}

	bool EvaluateConditions(const std::vector<ConditionToken>& a_conditions) {
		std::stack<ConditionToken> opStack;
		std::queue<ConditionToken> outQueue;

		for (const auto& conditionToken : a_conditions) {
			if (conditionToken.Type == ConditionToken::TokenType::kCondition) {
				outQueue.push(conditionToken);
			}
			else if (conditionToken.Type == ConditionToken::TokenType::kParenthesis) {
				if (conditionToken.Operator == "(") {
					opStack.push(conditionToken);
				}
				else {
					while (true) {
						if (opStack.top().Type == ConditionToken::TokenType::kParenthesis && opStack.top().Operator == "(") {
							break;
						}

						outQueue.push(opStack.top());
						opStack.pop();
					}

					opStack.pop();
				}
			}
			else if (conditionToken.Type == ConditionToken::TokenType::kOperator) {
				if (opStack.empty()) {
					opStack.push(conditionToken);
					continue;
				}

				if (opStack.top().Type == ConditionToken::TokenType::kCondition || 
					opStack.top().Type == ConditionToken::TokenType::kParenthesis) {
					opStack.push(conditionToken);
				}
				else if (opStack.top().Type == ConditionToken::TokenType::kOperator) {
					if ((conditionToken.Operator == "&&" && opStack.top().Operator == "||") ||
						(conditionToken.Operator == "!" && opStack.top().Operator == "&&") ||
						(conditionToken.Operator == "!" && opStack.top().Operator == "||")) {
						opStack.push(conditionToken);
					}
					else {
						outQueue.push(opStack.top());
						opStack.pop();
						opStack.push(conditionToken);
					}
				}
			}
		}

		while (!opStack.empty()) {
			outQueue.push(opStack.top());
			opStack.pop();
		}

		std::stack<bool> evalStack;

		while (!outQueue.empty()) {
			auto token = outQueue.front();
			outQueue.pop();

			if (token.Type == ConditionToken::TokenType::kCondition) {
				evalStack.push(EvaluateCondition(token.Condition.value()));
			}
			else if (token.Type == ConditionToken::TokenType::kOperator) {
				if (token.Operator == "&&" || token.Operator == "||") {
					auto right = evalStack.top();
					evalStack.pop();
					auto left = evalStack.top();
					evalStack.pop();

					if (token.Operator == "&&") {
						evalStack.push(left && right);
					}
					else if (token.Operator == "||") {
						evalStack.push(left || right);
					}
				}
				else if (token.Operator == "!") {
					auto right = evalStack.top();
					evalStack.pop();

					evalStack.push(!right);
				}
			}
		}

		if (evalStack.size() != 1) {
			logger::critical("Failed to evaluate conditional statement's condition.");
			return false;
		}

		return evalStack.top();
	}
}
