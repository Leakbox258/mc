#include "mc/MCContext.hpp"
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"
#include "utils/macro.hpp"
#include <cstring>
#include <fstream>

int main(int argc, char* argv[]) {
  std::ofstream OutputFile;
  std::ifstream SourceFile;

  utils_assert(argc == 4, "expecting 3 arguments");

  SourceFile = std::ifstream(argv[1]);
  utils_assert(!std::memcmp(argv[2], "-c", 2), "expecting '-c' argument");
  OutputFile = std::ofstream(argv[3]);

  std::string buffer;
  SourceFile >> buffer;

  auto Lexer = parser::Lexer(buffer); // source file
  auto Ctx = mc::MCContext(OutputFile);
  auto Parser = parser::Parser(Ctx, Lexer);

  Parser.parse();

  Ctx.writein();

  return 0;
}