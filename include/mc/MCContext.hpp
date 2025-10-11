#ifndef MC_CONTEXT
#define MC_CONTEXT

#include "MCInst.hpp"
#include "utils/ADT/StringMap.hpp"
#include "utils/ADT/StringRef.hpp"
#include "utils/ADT/StringSet.hpp"
#include <cstddef>
#include <cstdint>
#include <elf.h>
#include <fstream>
#include <set>
#include <string>
#include <vector>

namespace {

template <std::size_t N = 4096> struct ByteStream {
    using size_ty = std::size_t;

    std::ofstream file;

    std::vector<uint8_t> buffer;

    explicit ByteStream() : buffer(N) {}

    size_ty size() const { return buffer.size(); }
    void writein() {
        file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());

        buffer.clear();
    }

    void balignTo(size_ty balign) {
        auto paddingSize = (balign - (buffer.size() % balign)) % balign;

        for (auto i = 0ull; i < paddingSize; ++i) {
            buffer.push_back('\x00');
        }
    }

    template <IsPOD T> ByteStream& operator<<(T&& Value) {

        this->balignTo(sizeof(T));

        for (int i = 0; i < sizeof(T); ++i) {
            buffer.push_back(((uint8_t*)(&Value))[i]);
        }

        return *this;
    }

    template <size_ty Num> ByteStream& operator<<(const char (&Value)[Num]) {
        for (int i = 0; i < Num; ++i) {
            buffer.push_back(Value[i]);
        }

        return *this;
    }
};
} // namespace

namespace mc {
using StringRef = utils::ADT::StringRef;
template <typename V> using StringMap = utils::ADT::StringMap<V>;
using StringSet = utils::ADT::StringSet<>;

class MCContext {
  public:
    using size_ty = std::size_t;

  private:
    /// inst use symbols, which need gen Elf64_Rela or cul offset(.text)
    std::set<std::tuple<MCInst*, std::string>> ReloInst;

    /// buffer
    // ByteStream<> TextBuffer;

    /// ELF Header
    Elf64_Ehdr Elf_Ehdr;

    /// .text
    size_ty TextOffset = 0;
    StringMap<size_ty> TextLabels;
    std::vector<MCInst> Insts;

    /// .rela.text
    StringSet ReloSymbols; // symbols cross sections or rely on extern libs
    std::vector<Elf64_Rela> Elf_Rela;

    /// .data
    ByteStream<> DataBuffer;

    /// .bss
    size_ty BssSize;

    /// .symtab
    std::vector<Elf64_Sym> Elf_Syms;

    /// .strtab

    /// .shstrtab

    /// Section Header Table
    Elf64_Shdr Elf_Shdr;

  public:
    MCContext() {
        // TextBuffer = ByteStream();
        DataBuffer = ByteStream();
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

    size_ty incTextOffset(bool IsCompressed = false) {
        return IsCompressed ? TextOffset += 2 : TextOffset += 4;
    }

  public:
    bool addTextSym(StringRef Str) {
        return this->TextLabels.insert(Str, TextOffset);
    }

    bool addTextSym(StringRef Str, size_ty offset) {
        return this->TextLabels.insert(Str, std::move(offset));
    }

    bool addReloSym(StringRef Str) { return this->ReloSymbols.insert(Str); }

    bool getTextOffset() const { return TextOffset; }

    size_ty addTextInst(MCInst&& inst) {

        auto newOffset = incTextOffset(inst.isCompressed());

        Insts.push_back(std::move(inst));

        return newOffset;
    }

    void addReloInst(MCInst* inst, std::string label) {
        ReloInst.emplace(inst, std::move(label));
    }

    template <typename T> size_ty pushDataBuf(T&& Value) {
        this->DataBuffer << std::forward<T>(Value);
        return this->DataBuffer.size();
    }

    template <size_ty N> size_ty pushDataBuf(char (&Value)[N]) {
        this->DataBuffer << std::forward<decltype(Value)>(Value);
        return this->DataBuffer.size();
    }

    size_ty makeDataBufAlign(size_ty balign) {
        this->DataBuffer.balignTo(balign);
        return this->DataBuffer.size();
    }

    size_ty pushBssBuf(size_ty size) { return this->BssSize += size; }

    size_ty makeBssBufAlign(size_ty balign) {
        return this->BssSize += (balign - this->BssSize % balign) % balign;
    }
};
} // namespace mc

#endif