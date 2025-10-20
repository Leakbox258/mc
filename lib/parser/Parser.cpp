#include "parser/Parser.hpp"
#include "mc/MCContext.hpp"
#include "mc/MCExpr.hpp"
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
  MCInst* curInst = nullptr;
  SmallVector<std::string, 4> DirectiveStack;
  MCContext::size_ty curOffset = 0;

  auto advance = [&]() { token = this->lexer.nextToken(); };

  auto RegHelper = [&](const StringRef& reg) -> uint8_t {
    utils_assert(curInst, "expect curInst to be valid");
    auto find_reg =
        curInst->isCompressed() ? CRegisters.find(reg) : Registers.find(reg);
    utils_assert(find_reg, "invalid register literal");
    return *find_reg;
  };

  auto JmpBrHelper = [&](const StringRef& label) {
    ctx.addReloInst(curInst, label.str());

    /// 12 bits offset padding
    curInst->addOperand(MCOperand::makeImm(0));
  };

  while (token.type != TokenType::END_OF_FILE) {
    utils_assert(token.type != TokenType::UNKNOWN, "Lexer: error");

    switch (token.type) {
    case TokenType::NEWLINE:
      /// Inst commit
      if (curInst) {
        curOffset = ctx.commitTextInst();
        curInst = nullptr;
      }
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
      utils_assert(token.type == TokenType::RPAREN, "expecting right paren");
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
      /// .global xxx
      /// TODO: pseudo instructions
      if (curInst) {
        JmpBrHelper(token.lexeme);
      } else {
        using Ndx = MCContext::NdxSection;

        auto isExist =
            !StringSwitch<bool>(DirectiveStack.back())
                 .Case(".global", ".globl",
                       [&](auto&& _) {
                         const auto& section =
                             DirectiveStack[DirectiveStack.size() - 2];

                         if (section == ".data") {
                           ctx.addDataVar(token.lexeme);
                           return ctx.addReloSym(token.lexeme, Ndx::data);
                         } else if (section == ".bss") {
                           ctx.addBssVar(token.lexeme);
                           return ctx.addReloSym(token.lexeme, Ndx::bss);
                         } else if (section == ".text") {
                           return ctx.addReloSym(token.lexeme, Ndx::text);
                         }

                         utils::unreachable(
                             "cant match the section of this label");
                       })
                 .Error();

        utils_assert(!isExist, "global symbol redefinition");

        DirectiveStack.pop_back(); // .global, .globl
      }
      advance();
      break;
    case TokenType::INTEGER: {
      auto dw = std::stoll(token.lexeme);
      if (curInst) {
        curInst->addOperand(MCOperand::makeImm(dw));
      } else {
        /// TODO: more directive

        if (DirectiveStack[DirectiveStack.size() - 2] == ".data") {
          StringSwitch<bool>(DirectiveStack.back())
              .Case(".half",
                    [&](auto&& _) {
                      ctx.pushDataBuf<uint16_t>(dw);
                      return true;
                    })
              .Case(".word",
                    [&](auto&& _) {
                      ctx.pushDataBuf<uint32_t>(dw);
                      return true;
                    })
              .Case(".dword",
                    [&](auto&& _) {
                      ctx.pushDataBuf<uint64_t>(dw);
                      return true;
                    })
              .Case(".align",
                    [&](auto&& _) {
                      utils_assert(dw < 16, "expectling align target to be "
                                            "small than 16");

                      ctx.makeDataBufAlign(utils::pow2i(dw));

                      return true;
                    })
              .Case(".balign",
                    [&](auto&& _) {
                      auto e = utils::log2(dw);
                      utils_assert(e, "expecting dw to be pow of 2");

                      ctx.makeDataBufAlign(dw);
                      return true;
                    })
              .Error();

          DirectiveStack.pop_back();

        } else if (DirectiveStack[DirectiveStack.size() - 2] == ".bss") {
          utils_assert(dw == 0, "data def in bss supposed to be all zero");

          StringSwitch<bool>(DirectiveStack.back())
              .Case(".zero",
                    [&](auto&& _) {
                      ctx.pushBssBuf(dw);
                      return true;
                    })
              .Case(".align",
                    [&](auto&& _) {
                      utils_assert(dw < 16, "expectling align target to be "
                                            "small than 16");

                      ctx.makeBssBufAlign(utils::pow2i(dw));

                      return true;
                    })
              .Case(".balign",
                    [&](auto&& _) {
                      auto e = utils::log2(dw);
                      utils_assert(e, "expecting dw to be pow of 2");

                      ctx.makeBssBufAlign(dw);
                      return true;
                    })
              .Error();

          DirectiveStack.pop_back();

        } else {
          utils::unreachable("expect literal in .data or .bss section");
        }
      }
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
        utils_assert(!DirectiveStack.empty(), "expecting in an directive");
        auto [fimm, isDouble] =
            StringSwitch<std::tuple<uint64_t, bool>>(DirectiveStack.back())
                .Case(".float",
                      [](auto&& Str) -> std::tuple<uint64_t, bool> {
                        float value;
                        auto [ptr, ec] = std::from_chars(
                            Str.data(), Str.data() + Str.size(), value);

                        utils_assert(ec == std::error_code{},
                                     "parse float point failed");

                        return std::make_tuple(
                            *reinterpret_cast<uint64_t*>(&value), false);
                      })
                .Case(".double",
                      [](auto&& Str) -> std::tuple<uint64_t, bool> {
                        double value;
                        auto [ptr, ec] = std::from_chars(
                            Str.data(), Str.data() + Str.size(), value);

                        utils_assert(ec == std::error_code{},
                                     "parse float point failed");

                        return std::make_tuple(
                            *reinterpret_cast<uint64_t*>(&value), false);
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
    case TokenType::MODIFIERS: {
      utils_assert(curInst, "expect curInst to be valid");
      auto ty = mc::getExprTy(token.lexeme);
      utils_assert(ty, "invalid modifier");
      advance();
      utils_assert(token.type == TokenType::LPAREN,
                   "expecting left paren after a modifier");
      advance();

      auto Symbol = token.lexeme; // IDENTIFIER

      utils_assert(token.type == TokenType::IDENTIFIER,
                   "expecting an identifier in a modifier");

      advance();

      uint64_t Append = 0;
      if (token.type == TokenType::EXPR_OPERATOR) {
        /// parse op and constexpr
        auto op = token.lexeme == "+" ? 1ll : -1ll;

        advance();

        utils_assert(token.type == TokenType::INTEGER,
                     "expecting integer append");

        op *= std::stoll(token.lexeme); // no hex

        Append = *reinterpret_cast<uint64_t*>(&op);

        advance();
      }

      utils_assert(token.type == TokenType::RPAREN,
                   "expecting right paren after a modifier");

      curInst->addOperand(
          MCOperand::makeExpr(ctx.getTextExpr(Symbol, ty, Append)));

      ctx.addReloInst(&(*curInst), Symbol);
    }
      advance();
      break;
    case TokenType::INSTRUCTION: {
      /// must empty

      curInst = ctx.newTextInst(token.lexeme);
      curInst->modifyOffset(curOffset);
      curInst->modifyLoc(token.loc);

      auto [instAlign, padInst] =
          (*curInst).isCompressed()
              ? std::make_tuple(2, MCInst::makeNop(token.loc, curOffset))
              : std::make_tuple(4, MCInst::makeNop(token.loc, curOffset));

      /// make align
      if (curOffset % instAlign) {
        curInst->modifyOffset(ctx.addTextInst(std::move(padInst)));

        curOffset = curInst->getOffset();
      }
    }
      advance();
      break;
    case TokenType::REGISTER:
      utils_assert(curInst, "expect curInst to be valid");
      curInst->addOperand(MCOperand::makeReg(RegHelper(token.lexeme)));
      advance();
      break;
    case TokenType::DIRECTIVE:
      /// TODO: .section
      {
        auto isSectionDirective = StringSwitch<bool>(token.lexeme)
                                      .Case(".data", ".bss", ".text", true)
                                      .Default(false);

        if (isSectionDirective && !DirectiveStack.empty()) {
          /// end of the last section & start of the new section
          DirectiveStack.pop_back();
        }

        DirectiveStack.push_back(token.lexeme);
      }
      advance();
      break;
    case TokenType::LABEL_DEFINITION:
      /// EG: main:

      utils_assert(DirectiveStack.back() == ".text",
                   "expecting ddefine label in .text section");
      utils_assert(
          ctx.addTextLabel(token.lexeme.substr(0, token.lexeme.length() - 1)),
          "text label redefinition!");

      advance();
      break;
    case TokenType::STRING_LITERAL:
      utils::todo("sting def pesudo not impl yet");
      break;
    default:
      utils::unreachable("unkwnow type of current token");
    }
  }

  if (curInst) {
    ctx.commitTextInst();
  }
}

const mc::MCOpCode* Parser::findOpCode(StringRef mnemonic) {
  auto lookup = CacheLookUpTab.find(mnemonic);

  if (lookup != nullptr) {
    utils_assert(*lookup, "CacheLookUpTab contains invalid key");
    return *lookup;
  }

  mc::MCOpCode const* op = MnemonicFind(mnemonic.c_str());
  CacheLookUpTab.insert(mnemonic, op);

  return op;
}