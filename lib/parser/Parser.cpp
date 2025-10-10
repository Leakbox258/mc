#include "parser/Parser.hpp"
#include "mc/MCContext.hpp"
#include "mc/MCInst.hpp"
#include "mc/MCOperand.hpp"
#include "parser/Lexer.hpp"
#include "utils/ADT/SmallVector.hpp"
#include "utils/ADT/StringSwitch.hpp"
#include "utils/logger.hpp"
#include "utils/macro.hpp"
#include "utils/misc.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <system_error>
#include <tuple>

using namespace parser;
using namespace mc;
template <typename T> using StringSwitch = utils::ADT::StringSwitch<T>;
template <typename T, std::size_t N>
using SmallVector = utils::ADT::SmallVector<T, N>;

void Parser::parse() {

    auto token = this->lexer.nextToken();
    std::optional<MCInst> curInst = std::nullopt;
    SmallVector<std::string, 4> DirectiveStack;
    MCContext::size_ty curOffset = 0;

    auto advance = [&]() { token = this->lexer.nextToken(); };

    auto RegHelper = [&](const StringRef& reg) -> uint8_t {
        utils_assert(curInst, "expect curInst to be valid");
        auto find_reg = curInst->isCompressed() ? CRegisters.find(reg)
                                                : Registers.find(reg);
        utils_assert(find_reg, "invalid register literal");
        return *find_reg;
    };

    auto JmpBrHelper = [&](const StringRef& label) {
        utils_assert(curInst, "expect curInst to be valid");
        ctx.addReloInst(&(*curInst), label.str());

        /// 12 bits offset padding
        curInst->addOperand(MCOperand::makeImm(0));
    };

    while (token.type != TokenType::END_OF_FILE) {
        utils_assert(token.type != TokenType::UNKNOWN, "Lexer: error");

        switch (token.type) {
        case TokenType::NEWLINE:
            /// Inst commit
            curOffset = ctx.addTextInst(std::move(*curInst));
            curInst = std::nullopt;
            advance();
            break;
        case TokenType::COMMA:
            utils_assert(curInst, "encounter dangling comma");
            advance();
            break;
        case TokenType::LPAREN:
            utils_assert(curInst, "encounter dangling left paren");
            advance();
            utils_assert(token.type == TokenType::REGISTER,
                         "parse as an expr failed");
            curInst->addOperand(MCOperand::makeReg(RegHelper(token.lexeme)));
            advance();
            utils_assert(token.type == TokenType::RPAREN,
                         "expecting right paren");
            advance();
            break;
        case TokenType::RPAREN:
            utils::unreachable("encounter dangling right paren");
        case TokenType::COLON:
            advance();
            break;
        case TokenType::IDENTIFIER:
            /// symbol used without modifiers
            /// this is often branch/jump insts or
            /// TODO: pseudo instructions
            JmpBrHelper(token.lexeme);
            advance();
            break;
        case TokenType::INTEGER:
            if (curInst) {
                curInst->addOperand(
                    MCOperand::makeImm(std::stoll(token.lexeme)));
            } else {
                /// TODO: check directive
                utils_assert(utils::in_set(DirectiveStack.back(), "."),
                             "expecting data def directive");
            }
            advance();
            break;
        case TokenType::HEX_INTEGER:
            curInst->addOperand(
                MCOperand::makeImm(std::stoll(token.lexeme, nullptr, 16)));
            advance();
            break;
        case TokenType::FLOAT:
            /// TODO: pseudo instruction
            {
                utils_assert(!DirectiveStack.empty(),
                             "expecting in an directive");
                auto [fimm, isDouble] =
                    StringSwitch<std::tuple<uint64_t, bool>>(
                        DirectiveStack.back())
                        .BeginWith(
                            ".float",
                            [](auto&& Str) -> std::tuple<uint64_t, bool> {
                                float value;
                                auto [ptr, ec] = std::from_chars(
                                    Str.data(), Str.data() + Str.size(), value);

                                utils_assert(ec == std::error_code{},
                                             "parse float point failed");

                                return std::make_tuple(
                                    *reinterpret_cast<uint64_t*>(&value),
                                    false);
                            })
                        .BeginWith(
                            ".double",
                            [](auto&& Str) -> std::tuple<uint64_t, bool> {
                                double value;
                                auto [ptr, ec] = std::from_chars(
                                    Str.data(), Str.data() + Str.size(), value);

                                utils_assert(ec == std::error_code{},
                                             "parse float point failed");

                                return std::make_tuple(
                                    *reinterpret_cast<uint64_t*>(&value),
                                    false);
                            })
                        .Default();

                if (isDouble) {
                    ctx.pushDataBuf(fimm);
                } else {
                    ctx.pushDataBuf(static_cast<uint32_t>(fimm));
                }
            }
            advance();
            break;
        case TokenType::INSTRUCTION:
            *curInst = MCInst(token.lexeme, token.loc, curOffset);
            advance();
            break;
        case TokenType::REGISTER:
            utils_assert(curInst, "expect curInst to be valid");
            curInst->addOperand(MCOperand::makeReg(RegHelper(token.lexeme)));
            advance();
            break;
        case TokenType::DIRECTIVE:
            /// TODO: .section
            DirectiveStack.emplace_back(token.lexeme);
            advance();
            break;
        case TokenType::LABEL_DEFINITION:
            /// TODO: if in .text
            /// TODO: else, check if in .global/.globl
            advance();
            break;
        case TokenType::STRING_LITERAL:
            advance();
            break;
        default:
            utils::unreachable("unkwnow type of current token");
        }
    }
}