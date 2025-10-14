#include "mc/MCContext.hpp"
#include "utils/logger.hpp"
#include "utils/macro.hpp"
#include <algorithm>
#include <cstdint>
#include <elf.h>
#include <type_traits>

using namespace mc;

void MCContext::Relo() {

  for (auto [inst, sym] : ReloInst) {
    /// if sym def in .text: const Expr* -> imme
    if (auto where = TextLabels.find(sym)) {
      int64_t offset = static_cast<int64_t>(*where - inst->getOffset());
      utils_assert(offset % 2 == 0,
                   "offset in .text should be align to as least 2");

      inst->reloSym(offset);

    }
    /// else: configure Elf_Rela
    else if (auto iter = std::find(ReloSymbols.begin(), ReloSymbols.end(), sym);
             iter != ReloSymbols.end()) {

      uint32_t idx = std::distance(ReloSymbols.begin(), iter);
      Elf64_Rela Rela;
      Rela.r_offset = inst->getOffset();
      Rela.r_info = ELF64_R_INFO(idx, inst->getReloType());

      auto expr = inst->getExprOp();

      utils_assert(expr, "failed to find an expr in current instruction");

      Rela.r_addend = expr->getExpr()->getAppend();

      Elf_Rela.emplace_back(std::move(Rela));

      inst->reloSym(0ll);

    } else {
      utils::unreachable("unknown sym");
    }
  }
}

void MCContext::Gen_ELF_Hdr() {
  auto& hdr = this->Elf_Ehdr;

  /// magic number
  std::memcpy(hdr.e_ident,
              "\x7f"
              "ELF",
              4);
  hdr.e_ident[EI_CLASS] = ELFCLASS64;
  hdr.e_ident[EI_DATA] = ELFDATA2LSB;
  hdr.e_ident[EI_VERSION] = EV_CURRENT;
  hdr.e_ident[EI_OSABI] = ELFOSABI_NONE; // none

  hdr.e_type = ET_REL;
  hdr.e_machine = EM_RISCV;
  hdr.e_version = EV_CURRENT;
  hdr.e_ehsize = sizeof(std::decay_t<decltype(hdr)>);
  hdr.e_shentsize = sizeof(std::decay_t<decltype(hdr)>);

  /// TODO: .section and more sections
  /// .text .rela_text .data .bss .symtab .strtab .shstrtab
  hdr.e_shnum = 7;
  hdr.e_shstrndx = 6;

  /// estimate the offset to the section header table
  size_ty offset = 0;
  auto mkAlign = [&](size_ty alignment) {
    offset += (alignment - (offset % alignment)) % alignment;
  };

  offset += sizeof(std::decay_t<decltype(hdr)>);

  mkAlign(16);
  Offsets.push_back({".text", offset});
  offset += TextOffset;

  mkAlign(16);
  Offsets.push_back({".data", offset});
  offset += DataBuffer.size();

  mkAlign(8);
  Offsets.push_back({".bss", offset});
}

void MCContext::Gen_Text() {}

void MCContext::Gen_Rela_Text() {}

void MCContext::Gen_Data() {}

void MCContext::Gen_Bss() {}

void MCContext::Gen_SymTab() {}

void MCContext::Gen_StrTab() {}

void MCContext::Gen_ShStrTab() {}

void MCContext::Gen_Section_Hdr_Tab() {}
