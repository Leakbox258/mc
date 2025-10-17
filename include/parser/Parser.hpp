#ifndef PARSER_PARSER
#define PARSER_PARSER

#include "Lexer.hpp"
#include "mc/MCContext.hpp"
#include "mc/MCOpCode.hpp"
#include "utils/ADT/StringMap.hpp"
#include "utils/macro.hpp"
namespace parser {

template <typename V> using StringMap = utils::ADT::StringMap<V>;

class Parser {
private:
  mc::MCContext& ctx;
  Lexer& lexer;

  /// speed up mnemnoic class find
  StringMap<const mc::MCOpCode*> CacheLookUpTab;

public:
  Parser(mc::MCContext& _ctx LIFETIME_BOUND, Lexer& _lexer LIFETIME_BOUND)
      : ctx(_ctx), lexer(_lexer) {}

  void parse();

  ~Parser() = default;

private:
  /// this method assume mnemoic is valid
  const mc::MCOpCode* findOpCode(StringRef mnemonic);
};

} // namespace parser

#endif