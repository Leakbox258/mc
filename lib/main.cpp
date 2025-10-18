#include "mc/MCContext.hpp"
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"
#include "utils/logger.hpp"
#include "utils/macro.hpp"
#include <cstring>
#include <fstream>

std::string readFileToString(const std::string& filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    utils::unreachable("Failed to open file");
  }

  file.seekg(0, std::ios::end);
  std::size_t size = file.tellg();
  std::string buffer(size, '\0');

  file.seekg(0);
  file.read(buffer.data(), size);
  return buffer;
}

int main(int argc, char* argv[]) {
  std::ofstream OutputFile;
  std::string SourceFileStr;

  utils_assert(argc == 5, "expecting 4 arguments");

  utils_assert(!std::memcmp(argv[1], "-c", 2), "expecting '-c' argument");
  SourceFileStr = readFileToString(argv[2]);
  utils_assert(!std::memcmp(argv[3], "-o", 2), "expecting '-o' argument");
  OutputFile = std::ofstream(argv[4], std::ios::binary);

  auto Lexer = parser::Lexer(SourceFileStr); // source file
  auto Ctx = mc::MCContext(OutputFile);
  auto Parser = parser::Parser(Ctx, Lexer);

  Parser.parse();

  Ctx.writein();

  return 0;
}