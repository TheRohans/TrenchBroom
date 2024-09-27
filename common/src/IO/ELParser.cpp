/*
 Copyright (C) 2010-2017 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ELParser.h"

#include "EL/Expression.h"
#include "EL/Value.h"
#include "FileLocation.h"

#include "kdl/string_format.h"

#include <fmt/format.h>

#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

namespace TrenchBroom::IO
{

const std::string& ELTokenizer::NumberDelim() const
{
  static const auto Delim = Whitespace() + "(){}[],:+-*/%";
  return Delim;
}

const std::string& ELTokenizer::IntegerDelim() const
{
  static const auto Delim = NumberDelim() + ".";
  return Delim;
}

ELTokenizer::ELTokenizer(std::string_view str, const size_t line, const size_t column)
  : Tokenizer{std::move(str), "\"", '\\', line, column}
{
}

void ELTokenizer::appendUntil(const std::string& pattern, std::stringstream& str)
{
  const auto* begin = curPos();
  const auto* end = discardUntilPattern(pattern);
  str << std::string{begin, end};
  if (!eof())
  {
    discard("${");
  }
}

ELTokenizer::Token ELTokenizer::emitToken()
{
  while (!eof())
  {
    auto line = this->line();
    auto column = this->column();
    const auto* c = curPos();
    switch (*c)
    {
    case '[':
      advance();
      return Token{ELToken::OBracket, c, c + 1, offset(c), line, column};
    case ']':
      advance();
      return Token{ELToken::CBracket, c, c + 1, offset(c), line, column};
    case '{':
      advance();
      if (curChar() == '{')
      {
        advance();
        return Token{ELToken::DoubleOBrace, c, c + 2, offset(c), line, column};
      }
      return Token{ELToken::OBrace, c, c + 1, offset(c), line, column};
    case '}':
      advance();
      if (curChar() == '}')
      {
        advance();
        return Token{ELToken::DoubleCBrace, c, c + 2, offset(c), line, column};
      }
      return Token{ELToken::CBrace, c, c + 1, offset(c), line, column};
    case '(':
      advance();
      return Token{ELToken::OParen, c, c + 1, offset(c), line, column};
    case ')':
      advance();
      return Token{ELToken::CParen, c, c + 1, offset(c), line, column};
    case '+':
      advance();
      return Token{ELToken::Addition, c, c + 1, offset(c), line, column};
    case '-':
      advance();
      if (curChar() == '>')
      {
        advance();
        return Token{ELToken::Case, c, c + 2, offset(c), line, column};
      }
      return Token{ELToken::Subtraction, c, c + 1, offset(c), line, column};
    case '*':
      advance();
      return Token{ELToken::Multiplication, c, c + 1, offset(c), line, column};
    case '/':
      advance();
      if (curChar() == '/')
      {
        discardUntil("\n\r");
        break;
      }
      return Token{ELToken::Division, c, c + 1, offset(c), line, column};
    case '%':
      advance();
      return Token{ELToken::Modulus, c, c + 1, offset(c), line, column};
    case '~':
      advance();
      return Token{ELToken::BitwiseNegation, c, c + 1, offset(c), line, column};
    case '&':
      advance();
      if (curChar() == '&')
      {
        advance();
        return Token{ELToken::LogicalAnd, c, c + 2, offset(c), line, column};
      }
      return Token{ELToken::BitwiseAnd, c, c + 1, offset(c), line, column};
    case '|':
      advance();
      if (curChar() == '|')
      {
        advance();
        return Token{ELToken::LogicalOr, c, c + 2, offset(c), line, column};
      }
      return Token{ELToken::BitwiseOr, c, c + 1, offset(c), line, column};
    case '^':
      advance();
      return Token{ELToken::BitwiseXOr, c, c + 1, offset(c), line, column};
    case '!':
      advance();
      if (curChar() == '=')
      {
        advance();
        return Token{ELToken::NotEqual, c, c + 2, offset(c), line, column};
      }
      return Token{ELToken::LogicalNegation, c, c + 1, offset(c), line, column};
    case '<':
      advance();
      if (curChar() == '=')
      {
        advance();
        return Token{ELToken::LessOrEqual, c, c + 2, offset(c), line, column};
      }
      else if (curChar() == '<')
      {
        advance();
        return Token{ELToken::BitwiseShiftLeft, c, c + 2, offset(c), line, column};
      }
      return Token{ELToken::Less, c, c + 1, offset(c), line, column};
    case '>':
      advance();
      if (curChar() == '=')
      {
        advance();
        return Token{ELToken::GreaterOrEqual, c, c + 2, offset(c), line, column};
      }
      else if (curChar() == '>')
      {
        advance();
        return Token{ELToken::BitwiseShiftRight, c, c + 2, offset(c), line, column};
      }
      return Token{ELToken::Greater, c, c + 1, offset(c), line, column};
    case ':':
      advance();
      return Token{ELToken::Colon, c, c + 1, offset(c), line, column};
    case ',':
      advance();
      return Token{ELToken::Comma, c, c + 1, offset(c), line, column};
    case '\'':
    case '"': {
      const char delim = curChar();
      advance();
      c = curPos();
      const char* e = readQuotedString(delim);
      return Token{ELToken::String, c, e, offset(c), line, column};
    }
    case ' ':
    case '\t':
    case '\n':
    case '\r':
      discardWhile(Whitespace());
      break;
    default:
      switch (curChar())
      {
      case '.':
        if (lookAhead() == '.')
        {
          advance(2);
          return Token{ELToken::Range, c, c + 2, offset(c), line, column};
        }
        break;
      case '=':
        if (curChar() == '=')
        {
          advance(2);
          return Token{ELToken::Equal, c, c + 2, offset(c), line, column};
        }
        break;
      default:
        break;
      }

      if (const auto* e = readDecimal(NumberDelim()))
      {
        if (!eof() && curChar() == '.' && lookAhead() != '.')
        {
          throw ParserException{
            FileLocation{line, column}, fmt::format("Unexpected character: '{}'", *c)};
        }
        return Token{ELToken::Number, c, e, offset(c), line, column};
      }

      if (const auto* e = readInteger(IntegerDelim()))
      {
        return Token{ELToken::Number, c, e, offset(c), line, column};
      }

      if (const auto* e = discard("true"))
      {
        return Token{ELToken::Boolean, c, e, offset(c), line, column};
      }
      if (const auto* e = discard("false"))
      {
        return Token{ELToken::Boolean, c, e, offset(c), line, column};
      }

      if (const auto* e = discard("null"))
      {
        return Token{ELToken::Null, c, e, offset(c), line, column};
      }

      if (isLetter(*c) || *c == '_')
      {
        const char* e = nullptr;
        do
        {
          advance();
          e = curPos();
        } while (!eof() && (isLetter(*e) || isDigit(*e) || *e == '_'));

        return Token{ELToken::Name, c, e, offset(c), line, column};
      }

      throw ParserException{
        FileLocation{line, column}, fmt::format("Unexpected character: '{}'", *c)};
    }
  }
  return Token{ELToken::Eof, nullptr, nullptr, length(), line(), column()};
}

ELParser::ELParser(
  const ELParser::Mode mode, std::string_view str, const size_t line, const size_t column)
  : m_mode{mode}
  , m_tokenizer{str, line, column}
{
}

TokenizerState ELParser::tokenizerState() const
{
  return m_tokenizer.snapshot();
}

EL::ExpressionNode ELParser::parseStrict(const std::string& str)
{
  return ELParser{Mode::Strict, str}.parse();
}

EL::ExpressionNode ELParser::parseLenient(const std::string& str)
{
  return ELParser(Mode::Lenient, str).parse();
}

EL::ExpressionNode ELParser::parse()
{
  auto result = parseExpression();
  if (m_mode == Mode::Strict)
  {
    expect(ELToken::Eof, m_tokenizer.peekToken()); // avoid trailing garbage
  }
  return result;
}

EL::ExpressionNode ELParser::parseExpression()
{
  if (m_tokenizer.peekToken().hasType(ELToken::OParen))
  {
    return parseGroupedTerm();
  }
  return parseTerm();
}

EL::ExpressionNode ELParser::parseGroupedTerm()
{
  auto token = m_tokenizer.nextToken();
  expect(ELToken::OParen, token);
  auto expression = parseTerm();
  expect(ELToken::CParen, m_tokenizer.nextToken());

  auto lhs = EL::ExpressionNode{
    EL::UnaryExpression{EL::UnaryOperation::Group, std::move(expression)},
    token.location()};
  if (m_tokenizer.peekToken().hasType(ELToken::CompoundTerm))
  {
    return parseCompoundTerm(lhs);
  }
  return lhs;
}

EL::ExpressionNode ELParser::parseTerm()
{
  expect(ELToken::SimpleTerm | ELToken::DoubleOBrace, m_tokenizer.peekToken());

  auto lhs = parseSimpleTermOrSwitch();
  if (m_tokenizer.peekToken().hasType(ELToken::CompoundTerm))
  {
    return parseCompoundTerm(lhs);
  }
  return lhs;
}

EL::ExpressionNode ELParser::parseSimpleTermOrSwitch()
{
  auto token = m_tokenizer.peekToken();
  expect(ELToken::SimpleTerm | ELToken::DoubleOBrace, token);

  if (token.hasType(ELToken::SimpleTerm))
  {
    return parseSimpleTermOrSubscript();
  }
  return parseSwitch();
}

EL::ExpressionNode ELParser::parseSimpleTermOrSubscript()
{
  auto term = parseSimpleTerm();

  while (m_tokenizer.peekToken().hasType(ELToken::OBracket))
  {
    term = parseSubscript(std::move(term));
  }

  return term;
}

EL::ExpressionNode ELParser::parseSimpleTerm()
{
  auto token = m_tokenizer.peekToken();
  expect(ELToken::SimpleTerm, token);

  if (token.hasType(ELToken::UnaryOperator))
  {
    return parseUnaryOperator();
  }
  if (token.hasType(ELToken::OParen))
  {
    return parseGroupedTerm();
  }
  if (token.hasType(ELToken::Name))
  {
    return parseVariable();
  }
  return parseLiteral();
}

EL::ExpressionNode ELParser::parseSubscript(EL::ExpressionNode lhs)
{
  auto token = m_tokenizer.nextToken();
  const auto location = token.location();

  expect(ELToken::OBracket, token);
  auto elements = std::vector<EL::ExpressionNode>{};
  if (!m_tokenizer.peekToken().hasType(ELToken::CBracket))
  {
    do
    {
      elements.push_back(parseExpressionOrAnyRange());
    } while (expect(ELToken::Comma | ELToken::CBracket, m_tokenizer.nextToken())
               .hasType(ELToken::Comma));
  }
  else
  {
    m_tokenizer.nextToken();
  }

  auto rhs = elements.size() == 1u
               ? std::move(elements.front())
               : EL::ExpressionNode{EL::ArrayExpression{std::move(elements)}, location};
  return EL::ExpressionNode{
    EL::SubscriptExpression{std::move(lhs), std::move(rhs)}, location};
}

EL::ExpressionNode ELParser::parseVariable()
{
  auto token = m_tokenizer.nextToken();
  expect(ELToken::Name, token);
  return EL::ExpressionNode{EL::VariableExpression{token.data()}, token.location()};
}

EL::ExpressionNode ELParser::parseLiteral()
{
  auto token = m_tokenizer.peekToken();
  expect(ELToken::Literal | ELToken::OBracket | ELToken::OBrace, token);

  if (token.hasType(ELToken::String))
  {
    m_tokenizer.nextToken();
    // Escaping happens in EL::Value::appendToStream
    auto value = kdl::str_unescape(token.data(), "\\\"");
    return EL::ExpressionNode{
      EL::LiteralExpression{EL::Value{std::move(value)}}, token.location()};
  }
  if (token.hasType(ELToken::Number))
  {
    m_tokenizer.nextToken();
    return EL::ExpressionNode{
      EL::LiteralExpression{EL::Value{token.toFloat<EL::NumberType>()}},
      token.location()};
  }
  if (token.hasType(ELToken::Boolean))
  {
    m_tokenizer.nextToken();
    return EL::ExpressionNode{
      EL::LiteralExpression{EL::Value{token.data() == "true"}}, token.location()};
  }
  if (token.hasType(ELToken::Null))
  {
    m_tokenizer.nextToken();
    return EL::ExpressionNode{EL::LiteralExpression{EL::Value::Null}, token.location()};
  }

  if (token.hasType(ELToken::OBracket))
  {
    return parseArray();
  }
  return parseMap();
}

EL::ExpressionNode ELParser::parseArray()
{
  auto token = m_tokenizer.nextToken();
  const auto location = token.location();

  expect(ELToken::OBracket, token);
  auto elements = std::vector<EL::ExpressionNode>{};
  if (!m_tokenizer.peekToken().hasType(ELToken::CBracket))
  {
    do
    {
      elements.push_back(parseExpressionOrBoundedRange());
    } while (expect(ELToken::Comma | ELToken::CBracket, m_tokenizer.nextToken())
               .hasType(ELToken::Comma));
  }
  else
  {
    m_tokenizer.nextToken();
  }

  return EL::ExpressionNode{EL::ArrayExpression{std::move(elements)}, location};
}

EL::ExpressionNode ELParser::parseExpressionOrBoundedRange()
{
  auto expression = parseExpression();
  if (m_tokenizer.peekToken().hasType(ELToken::Range))
  {
    auto token = m_tokenizer.nextToken();
    expression = EL::ExpressionNode{
      EL::BinaryExpression{
        EL::BinaryOperation::BoundedRange, std::move(expression), parseExpression()},
      token.location()};
  }

  return expression;
}

EL::ExpressionNode ELParser::parseExpressionOrAnyRange()
{
  auto expression = std::optional<EL::ExpressionNode>{};
  if (m_tokenizer.peekToken().hasType(ELToken::Range))
  {
    auto token = m_tokenizer.nextToken();
    expression = EL::ExpressionNode{
      EL::UnaryExpression{EL::UnaryOperation::RightBoundedRange, parseExpression()},
      token.location()};
  }
  else
  {
    expression = parseExpression();
    if (m_tokenizer.peekToken().hasType(ELToken::Range))
    {
      auto token = m_tokenizer.nextToken();
      if (m_tokenizer.peekToken().hasType(ELToken::SimpleTerm))
      {
        expression = EL::ExpressionNode{
          EL::BinaryExpression{
            EL::BinaryOperation::BoundedRange, std::move(*expression), parseExpression()},
          token.location()};
      }
      else
      {
        expression = EL::ExpressionNode{
          EL::UnaryExpression{
            EL::UnaryOperation::LeftBoundedRange, std::move(*expression)},
          token.location()};
      }
    }
  }

  return *expression;
}

EL::ExpressionNode ELParser::parseMap()
{
  auto elements = std::map<std::string, EL::ExpressionNode>{};

  auto token = m_tokenizer.nextToken();
  const auto location = token.location();

  expect(ELToken::OBrace, token);
  if (!m_tokenizer.peekToken().hasType(ELToken::CBrace))
  {
    do
    {
      token = m_tokenizer.nextToken();
      expect(ELToken::String | ELToken::Name, token);
      auto key = token.data();

      expect(ELToken::Colon, m_tokenizer.nextToken());
      elements.emplace(std::move(key), parseExpression());
    } while (expect(ELToken::Comma | ELToken::CBrace, m_tokenizer.nextToken())
               .hasType(ELToken::Comma));
  }
  else
  {
    m_tokenizer.nextToken();
  }

  return EL::ExpressionNode{EL::MapExpression{std::move(elements)}, location};
}

EL::ExpressionNode ELParser::parseUnaryOperator()
{
  static const auto TokenMap = std::unordered_map<ELToken::Type, EL::UnaryOperation>{
    {ELToken::Addition, EL::UnaryOperation::Plus},
    {ELToken::Subtraction, EL::UnaryOperation::Minus},
    {ELToken::LogicalNegation, EL::UnaryOperation::LogicalNegation},
    {ELToken::BitwiseNegation, EL::UnaryOperation::BitwiseNegation},
  };

  auto token = m_tokenizer.nextToken();
  expect(ELToken::UnaryOperator, token);

  if (const auto it = TokenMap.find(token.type()); it != TokenMap.end())
  {
    const auto op = it->second;
    return EL::ExpressionNode{
      EL::UnaryExpression{op, parseSimpleTermOrSwitch()}, token.location()};
  }
  throw ParserException{
    token.location(),
    fmt::format("Unhandled unary operator: {}", tokenName(token.type()))};
}

EL::ExpressionNode ELParser::parseSwitch()
{
  auto token = m_tokenizer.nextToken();
  expect(ELToken::DoubleOBrace, token);

  const auto location = token.location();
  auto subExpressions = std::vector<EL::ExpressionNode>{};

  token = m_tokenizer.peekToken();
  expect(ELToken::SimpleTerm | ELToken::DoubleCBrace, token);

  if (token.hasType(ELToken::SimpleTerm))
  {
    do
    {
      subExpressions.push_back(parseExpression());
    } while (expect(ELToken::Comma | ELToken::DoubleCBrace, m_tokenizer.nextToken())
               .hasType(ELToken::Comma));
  }
  else if (token.hasType(ELToken::DoubleCBrace))
  {
    m_tokenizer.nextToken();
  }

  return EL::ExpressionNode{EL::SwitchExpression{std::move(subExpressions)}, location};
}

EL::ExpressionNode ELParser::parseCompoundTerm(EL::ExpressionNode lhs)
{
  static const auto TokenMap = std::unordered_map<ELToken::Type, EL::BinaryOperation>{
    {ELToken::Addition, EL::BinaryOperation::Addition},
    {ELToken::Subtraction, EL::BinaryOperation::Subtraction},
    {ELToken::Multiplication, EL::BinaryOperation::Multiplication},
    {ELToken::Division, EL::BinaryOperation::Division},
    {ELToken::Modulus, EL::BinaryOperation::Modulus},
    {ELToken::LogicalAnd, EL::BinaryOperation::LogicalAnd},
    {ELToken::LogicalOr, EL::BinaryOperation::LogicalOr},
    {ELToken::BitwiseAnd, EL::BinaryOperation::BitwiseAnd},
    {ELToken::BitwiseXOr, EL::BinaryOperation::BitwiseXOr},
    {ELToken::BitwiseOr, EL::BinaryOperation::BitwiseOr},
    {ELToken::BitwiseShiftLeft, EL::BinaryOperation::BitwiseShiftLeft},
    {ELToken::BitwiseShiftRight, EL::BinaryOperation::BitwiseShiftRight},
    {ELToken::Less, EL::BinaryOperation::Less},
    {ELToken::LessOrEqual, EL::BinaryOperation::LessOrEqual},
    {ELToken::Greater, EL::BinaryOperation::Greater},
    {ELToken::GreaterOrEqual, EL::BinaryOperation::GreaterOrEqual},
    {ELToken::Equal, EL::BinaryOperation::Equal},
    {ELToken::NotEqual, EL::BinaryOperation::NotEqual},
    {ELToken::Range, EL::BinaryOperation::BoundedRange},
    {ELToken::Case, EL::BinaryOperation::Case},
  };

  while (m_tokenizer.peekToken().hasType(ELToken::CompoundTerm))
  {
    auto token = m_tokenizer.nextToken();
    expect(ELToken::CompoundTerm, token);


    if (const auto it = TokenMap.find(token.type()); it != TokenMap.end())
    {
      const auto op = it->second;
      lhs = EL::ExpressionNode{
        EL::BinaryExpression{op, std::move(lhs), parseSimpleTermOrSwitch()},
        token.location()};
    }
    else
    {
      throw ParserException{
        token.location(),
        fmt::format("Unhandled binary operator: {}", tokenName(token.type()))};
    }
  }

  return lhs;
}

ELParser::TokenNameMap ELParser::tokenNames() const
{
  return {
    {ELToken::Name, "variable"},
    {ELToken::String, "string"},
    {ELToken::Number, "number"},
    {ELToken::Boolean, "boolean"},
    {ELToken::OBracket, "'['"},
    {ELToken::CBracket, "']'"},
    {ELToken::OBrace, "'{'"},
    {ELToken::CBrace, "'}'"},
    {ELToken::OParen, "'('"},
    {ELToken::CParen, "')'"},
    {ELToken::Addition, "'+'"},
    {ELToken::Subtraction, "'-'"},
    {ELToken::Multiplication, "'*'"},
    {ELToken::Division, "'/'"},
    {ELToken::Modulus, "'%'"},
    {ELToken::Colon, "':'"},
    {ELToken::Comma, "','"},
    {ELToken::Range, "'..'"},
    {ELToken::LogicalNegation, "'!'"},
    {ELToken::LogicalAnd, "'&&'"},
    {ELToken::LogicalOr, "'||'"},
    {ELToken::Less, "'<'"},
    {ELToken::LessOrEqual, "'<='"},
    {ELToken::Equal, "'=='"},
    {ELToken::NotEqual, "'!='"},
    {ELToken::GreaterOrEqual, "'>='"},
    {ELToken::Greater, "'>'"},
    {ELToken::Case, "'->'"},
    {ELToken::BitwiseNegation, "'~'"},
    {ELToken::BitwiseAnd, "'&'"},
    {ELToken::BitwiseOr, "'|'"},
    {ELToken::BitwiseShiftLeft, "'<<'"},
    {ELToken::BitwiseShiftRight, "'>>'"},
    {ELToken::DoubleOBrace, "'{{'"},
    {ELToken::DoubleCBrace, "'}}'"},
    {ELToken::Null, "'null'"},
    {ELToken::Eof, "end of file"},
  };
}

} // namespace TrenchBroom::IO
