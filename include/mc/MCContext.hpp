#ifndef MC_CONTEXT
#define MC_CONTEXT

#include "utils/ADT/StringMap.hpp"
#include "utils/ADT/StringRef.hpp"
#include "utils/ADT/StringSet.hpp"
#include <cstddef>
#include <elf.h>
#include <vector>
namespace mc {
using StringRef = utils::ADT::StringRef;
template <typename V> using StringMap = utils::ADT::StringMap<V>;
using StringSet = utils::ADT::StringSet<>;

class MCContext {
  public:
    using size_ty = std::size_t;
    using ByteStream = std::vector<std::byte>;

  private:
    ByteStream GenericBuffer;

    /// ELF Header
    Elf64_Ehdr Elf_Ehdr;

    /// .text
    size_ty TextOffset = 0;
    StringMap<size_ty> TextSymbols;

    /// .rela.text
    StringSet ReloSymbols; // symbols cross sections or rely on extern libs

    /// .data

    /// .bss

    /// .symtab
    std::vector<Elf64_Sym> Elf_Syms;

    /// .strtab

    /// .shstrtab

    /// Section Header Table
    Elf64_Shdr Elf_Shdr;

  public:
    MCContext(std::size_t buffer_size = 1024) {
        GenericBuffer = ByteStream(buffer_size);
    }
    MCContext(const MCContext&) = delete;
    MCContext(MCContext&&) = delete;
    MCContext& operator=(const MCContext&) = delete;

  private:
    /// binary obj rewriters
    size_ty Gen_ELF_Hdr();
    size_ty Gen_Text();
    size_ty Gen_Rela_Text();
    size_ty Gen_Data();
    size_ty Gen_Bss();
    size_ty Gen_SymTab();
    size_ty Gen_StrTab();
    size_ty Gen_ShStrTab();
    size_ty Gen_Section_Hdr_Tab();

  public:
    bool addTextSym(StringRef Str, size_ty offset) {
        return this->TextSymbols.insert(Str, std::move(offset));
    }

    bool addReloSym(StringRef Str) { return this->ReloSymbols.insert(Str); }

    bool getTextOffset() const { return TextOffset; }

    size_ty incTextOffset(bool IsCompressed = false) {
        return IsCompressed ? TextOffset += 2 : TextOffset += 4;
    }
};
} // namespace mc

#endif