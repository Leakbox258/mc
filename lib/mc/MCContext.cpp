#include "mc/MCContext.hpp"
#include "utils/logger.hpp"
#include "utils/macro.hpp"
#include <algorithm>
#include <cstdint>
#include <elf.h>

using namespace mc;

void MCContext::Relo() {
  /// if sym def in .text: const Expr* -> imme
  /// else: configure Elf_Rela

  for (auto [inst, sym] : ReloInst) {
    if (auto where = TextLabels.find(sym)) {
      int64_t offset = static_cast<int64_t>(*where - inst->getOffset());
      utils_assert(offset % 2 == 0,
                   "offset in .text should be align to as least 2");

      inst->reloSym(offset);

    } else if (auto iter =
                   std::find(ReloSymbols.begin(), ReloSymbols.end(), sym);
               iter != ReloSymbols.end()) {

      uint32_t idx = std::distance(ReloSymbols.begin(), iter);
      Elf64_Rela Rela;
      Rela.r_offset = inst->getOffset();
      Rela.r_info = ELF64_R_INFO(idx, inst->getReloType());

      auto expr = inst->getExprOp();

      utils_assert(expr, "failed to find an expr in current instruction");

      Rela.r_addend = expr->getExpr()->getAppend();

      Elf_Rela.emplace_back(std::move(Rela));

    } else {
      utils::unreachable("unknown sym");
    }
  }
}