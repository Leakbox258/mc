#include "mc/MCContext.hpp"
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"
#include "utils/macro.hpp"
#include <cstring>
#include <fstream>

int main(int argc, char* argv[]) {
  std::ofstream OutputFile;
  std::ifstream SourceFile;

  utils_assert(argc == 5, "expecting 4 arguments");

  utils_assert(!std::memcmp(argv[1], "-c", 2), "expecting '-c' argument");
  SourceFile = std::ifstream(argv[2]);
  utils_assert(!std::memcmp(argv[3], "-o", 2), "expecting '-o' argument");
  OutputFile = std::ofstream(argv[4]);

  std::string buffer;
  SourceFile >> buffer;

  auto Lexer = parser::Lexer(buffer); // source file
  auto Ctx = mc::MCContext(OutputFile);
  auto Parser = parser::Parser(Ctx, Lexer);

  Parser.parse();

  Ctx.writein();

  return 0;
}