#include "mc/MCContext.hpp"
#include "utils/macro.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <elf.h>
#include <type_traits>
#include <vector>

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
    /// else if: symbols from .data or .bss configure Elf_Rela
    else if (auto iter =
                 std::find_if(ReloSymbols.begin(), ReloSymbols.end(),
                              [&](auto&& elem) { return elem.first == sym; });
             iter != ReloSymbols.end()) {

      uint32_t idx = std::distance(ReloSymbols.begin(), iter);
      Elf64_Rela Rela = {};
      Rela.r_offset = inst->getOffset();
      Rela.r_info = ELF64_R_INFO(
          idx, inst->getReloType()); // idx pointer to idx in .symtab

      auto expr = inst->getExprOp();

      utils_assert(expr, "failed to find an expr in current instruction");

      Rela.r_addend = expr->getExpr()->getAppend();

      Elf_Relas.emplace_back(std::move(Rela));

      inst->reloSym(0ll);

    }
    /// extern symbols
    else {
      ReloSymbols.insert({
          sym,
          NdxSection::und,
      });
      uint32_t idx = ReloSymbols.size() - 1;
      Elf64_Rela Rela = {};
      Rela.r_offset = inst->getOffset();
      Rela.r_info = ELF64_R_INFO(idx, inst->getReloType());
    }
  }
}

void MCContext::Ehdr_Shdr() {
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
  hdr.e_flags = EF_RISCV_RVC | EF_RISCV_FLOAT_ABI_DOUBLE;
  hdr.e_ehsize = sizeof(std::decay_t<decltype(hdr)>);
  hdr.e_shentsize = sizeof(Elf64_Shdr);

  /// TODO: .section and more sections
  /// <void> .text .data .bss .strtab .symtab .rela_text .shstrtab
  hdr.e_shnum = 8;
  hdr.e_shstrndx = 7;

  /// estimate the offset to the section header table
  size_ty offset = 0;
  auto mkAlign = [&](size_ty alignment) {
    offset += (alignment - (offset % alignment)) % alignment;
  };

  {
    Offsets.insert("elf header", 0);
    offset += sizeof(std::decay_t<decltype(hdr)>);
  }

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

    for (const auto& [relo_sym, ndx] : ReloSymbols) {
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

    /// begin with none
    Elf64_Sym symbol = {};
    Elf_Syms.emplace_back(std::move(symbol));

    /// relo symbols
    for (const auto& [sym, ndx] : ReloSymbols) {
      Elf64_Sym symbol = {};

      symbol.st_name = StrTabBuffer.findOffset(sym); /// ？
      symbol.st_info = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);
      symbol.st_other = ELF64_ST_VISIBILITY(STV_DEFAULT); // visibility
      symbol.st_shndx = ndx;

      Elf_Syms.emplace_back(std::move(symbol));
    }

    /// sections
    const auto& sections = Offsets.keys();
    for (size_ty i = 1; i < sections.size() - 1; ++i) {
      Elf64_Sym symbol = {};

      symbol.st_info = ELF64_ST_INFO(STB_LOCAL, STT_SECTION);
      symbol.st_other = ELF64_ST_VISIBILITY(STV_DEFAULT);
      symbol.st_shndx = i;

      Elf_Syms.emplace_back(std::move(symbol));
    }

    offset += (Elf_Syms.size()) * sizeof(Elf64_Sym);
  }

  {
    mkAlign(8);
    Offsets.insert(".rela.text", offset);
    offset += Elf_Relas.size() * sizeof(Elf64_Rela);
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

  {
    mkAlign(8);
    Offsets.insert("section header table", offset);
  }

  {
    hdr.e_shoff = offset;

    /// empty section hdr
    {
      Elf64_Shdr shdr = {};
      shdr.sh_type = SHT_NULL;
      Elf_Shdrs.emplace_back(std::move(shdr));
    }

    auto SectionHeader = [&](StringRef name, uint32_t type, uint64_t flag,
                             uint64_t size, uint64_t alignment,
                             uint32_t link = 0, uint32_t info = 0,
                             uint64_t entsize = 0) {
      Elf64_Shdr shdr = {};

      auto offset = Offsets.find(name);
      utils_assert(offset, "cant find offset of this section");

      shdr.sh_name = SHStrTabBuffer.findOffset(name);
      shdr.sh_type = type;
      shdr.sh_flags = flag;
      shdr.sh_addr = 0;
      shdr.sh_offset = *offset;
      shdr.sh_size = size;
      shdr.sh_addralign = alignment;

      shdr.sh_entsize = entsize;
      shdr.sh_link = link;
      shdr.sh_info = info;

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

    /// .symtab: link to .strtab
    SectionHeader(".symtab", SHT_SYMTAB, 0,
                  TextLabels.keys().size() * sizeof(Elf64_Sym), 8, 4, 0,
                  sizeof(Elf64_Sym));

    /// .rela.text: link to .symtab
    SectionHeader(".rela.text", SHT_RELA, SHF_INFO_LINK,
                  Elf_Relas.size() * sizeof(Elf64_Rela), 8, 5, 1,
                  sizeof(Elf64_Sym));

    /// .shstrtab
    SectionHeader(".shstrtab", SHT_STRTAB, 0, SHStrTabBuffer.size(), 1);
  }
}

void MCContext::writein() {
  this->Ehdr_Shdr();

  size_ty curOffset = 0;

  auto streamWriteIn = [&](const char* data, size_ty n) {
    this->file.write(data, n);
    curOffset += n;
  };

  auto padSection = [&](size_ty offset) {
    /// pad current size to offset
    std::vector<char> buffer(offset - curOffset, '\x00');
    streamWriteIn(buffer.data(), buffer.size());
  };

  /// elf header
  {
    streamWriteIn((char*)&this->Elf_Ehdr, sizeof(Elf64_Ehdr));
  }

  /// .text
  {
    padSection(*this->Offsets.find(".text"));

    for (const auto& inst : this->Insts) {
      auto encode = inst.makeEncoding();
      streamWriteIn((char*)&encode, inst.isCompressed() ? 2 : 4);
    }
  }

  /// .data
  {
    padSection(*this->Offsets.find(".data"));
    streamWriteIn((const char*)DataBuffer.data(), DataBuffer.size());
  }

  /// .bss
  {
    padSection(*this->Offsets.find(".bss"));
    std::vector<char> buffer(BssSize, '\x00');
    streamWriteIn(buffer.data(), buffer.size());
  }

  /// .strtab
  {
    padSection(*this->Offsets.find(".strtab"));
    streamWriteIn((const char*)StrTabBuffer.data(), StrTabBuffer.size());
  }

  /// .symtab
  {
    padSection(*this->Offsets.find(".symtab"));

    for (auto symbol : this->Elf_Syms) {
      streamWriteIn((char*)&symbol, sizeof(Elf64_Sym));
    }
  }

  /// .rela.text
  {
    padSection(*this->Offsets.find(".rela.text"));

    for (auto relo : this->Elf_Relas) {
      streamWriteIn((char*)&relo, sizeof(Elf64_Rela));
    }
  }

  /// .shstrtab
  {
    padSection(*this->Offsets.find(".shstrtab"));
    streamWriteIn((const char*)SHStrTabBuffer.data(), SHStrTabBuffer.size());
  }

  /// dump section headers
  {
    padSection(*this->Offsets.find("section header table"));

    for (auto shdr : Elf_Shdrs) {
      streamWriteIn((char*)&shdr, sizeof(Elf64_Shdr));
    }
  }
}