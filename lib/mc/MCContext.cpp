#include "mc/MCContext.hpp"
#include "utils/logger.hpp"
#include "utils/macro.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
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
      Rela.r_info = ELF64_R_INFO(
          idx, inst->getReloType()); // idx pointer to idx in .symtab

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

  {
    mkAlign(2);
    Offsets.insert(".text", offset);
    offset += TextOffset;
  }

  {
    mkAlign(1);
    Offsets.insert(".data", offset);
    offset += DataBuffer.size();
  }

  {
    mkAlign(1);
    Offsets.insert(".bss", offset);
    /// readelf: Section '.bss' has no data to dump.
  }

  {
    mkAlign(1);
    Offsets.insert(".strtab", offset);

    /// gather .strtab context
    /// include label, symbol(relos, variables)
    StrTabBuffer << '\x00';

    for (const auto& relo_sym : ReloSymbols) {
      StrTabBuffer << relo_sym.c_str() << '\x00';
    }

    for (const auto& label : TextLabels.keys()) {
      StrTabBuffer << label.c_str() << '\x00';
    }

    offset += StrTabBuffer.size();
  }

  {
    mkAlign(8);
    Offsets.insert(".symtab", offset);
    offset += TextLabels.keys().size() * sizeof(Elf64_Sym);
  }

  {
    mkAlign(8);
    Offsets.insert(".rela.text", offset);
    offset += Elf_Rela.size() * sizeof(Elf64_Rela);
  }

  {
    mkAlign(1);
    Offsets.insert(".shstrtab", offset);

    /// gather .shstrtab
    /// include names of each section
    SHStrTabBuffer << '\x00';
    SHStrTabBuffer << ".text" << '\x00';
    SHStrTabBuffer << ".data" << '\x00';
    SHStrTabBuffer << ".bss" << '\x00';
    SHStrTabBuffer << ".strtab" << '\x00';
    SHStrTabBuffer << ".symtab" << '\x00';
    SHStrTabBuffer << ".rela.text" << '\x00';
    SHStrTabBuffer << ".shstrtab" << '\x00';

    offset += SHStrTabBuffer.size();
  }

  /// TODO: .section xxx

  {
    /// empty section hdr
    {
      Elf64_Shdr shdr = {};
      shdr.sh_type = SHT_NULL;
      Elf_Shdrs.emplace_back(std::move(shdr));
    }

    auto SectionHeader = [&](StringRef name, int type, int flag, uint64_t size,
                             int alignment) {
      Elf64_Shdr shdr = {};

      auto offset = Offsets.find(".text");
      utils_assert(offset, "cant find offset of this section");

      shdr.sh_name = SHStrTabBuffer.findOffset(name);
      shdr.sh_type = type;
      shdr.sh_flags = flag;
      shdr.sh_addr = 0;
      shdr.sh_offset = *offset;
      shdr.sh_size = size;
      shdr.sh_addralign = alignment;

      Elf_Shdrs.emplace_back(std::move(shdr));
    };

    /// .text
    SectionHeader(".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, TextOffset,
                  2);

    /// .data
    SectionHeader(".data", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE,
                  DataBuffer.size(), 1);

    /// .bss
    SectionHeader(".bss", SHT_NOBITS, SHF_ALLOC | SHF_WRITE, 0, 1);

    /// .strtab
    SectionHeader(".strtab", SHT_STRTAB, 0, StrTabBuffer.size(), 1);

    /// .symtab
    SectionHeader(".symtab", SHT_SYMTAB, 0,
                  TextLabels.keys().size() * sizeof(Elf64_Sym), 8);

    /// .rela.text
    SectionHeader(".rela.text", SHT_RELA, SHF_LINK_ORDER,
                  Elf_Rela.size() * sizeof(Elf64_Rela), 8);

    /// .shstrtab
    SectionHeader(".shstrtab", SHT_STRTAB, 0, SHStrTabBuffer.size(), 1);
  }
}

void MCContext::Gen_Text() {}

void MCContext::Gen_Rela_Text() {}

void MCContext::Gen_Data() {}

void MCContext::Gen_Bss() {}

void MCContext::Gen_StrTab() {}

void MCContext::Gen_SymTab() {}

void MCContext::Gen_ShStrTab() {}

void MCContext::Gen_Section_Hdr_Tab() {}
