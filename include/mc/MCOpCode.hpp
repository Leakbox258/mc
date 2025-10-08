#ifndef MC_OPCODE
#define MC_OPCODE

#include "utils/ADT/SmallVector.hpp"
#include "utils/ADT/StringRef.hpp"
#include "utils/ADT/StringSwitch.hpp"
#include "utils/numeric.hpp"
#include <cstdint>
#include <optional>
#include <tuple>

using StringRef = utils::ADT::StringRef;
template <typename T> using StringSwitch = utils::ADT::StringSwitch<T>;
template <typename T, std::size_t N>
using SmallVector = utils::ADT::SmallVector<T, N>;

namespace mc {

struct EnCoding {
    enum EnCodeTy : uint16_t {
        kInvalid,
        kStatic, // opcode or padding
        kImm,
        kNzImm,
        kUImm,
        kRd,
        kRs1,
        kRs2,
        kRs3,
        kRd_short,
        kRs1_short,
        kRs2_short,
        kRs3_short,
        kRm,       // rounding mode (F/D extension)
        kMemFence, // r, w, rw
        // AQ or RL will concat with mnemonic in A extension
        // eg: LR.D.AQ t1, (s0)
    };

    EnCodeTy kind = kInvalid;
    unsigned length;
    std::optional<std::array<std::pair<uint16_t, uint16_t>, 8>> bit_range;
    std::optional<uint16_t> static_pattern;
};

struct MCOpCode {
  private:
    constexpr static std::tuple<std::array<std::pair<uint16_t, uint16_t>, 8>,
                                unsigned>
    parseBitRange(StringRef BitRange) {
        // 4:0
        // 20|10:1|11|19:12
        auto concat_range = BitRange.split<8>('|');

        std::array<std::pair<uint16_t, uint16_t>, 8> bit_range{};
        unsigned length = 0;
        unsigned idx = 0;

        for (auto rit = concat_range.rbegin(); rit != concat_range.rend();
             ++rit) {
            auto& range = *rit;

            /// NOTE: for constexpr, std::array need a more
            /// large range
            auto ranges = range.split<8>(':');

            // `low` will probably be empty
            auto low = ranges[1];
            auto high = ranges.size() == 2 ? ranges[0] : low;

            auto low_bit =
                low.empty() ? -1 : utils::stoi(low.c_str(), low.size());
            auto high_bit = utils::stoi(high.c_str(), high.size());

            bit_range[idx++] = {low_bit, high_bit};
            length += high_bit - low_bit + 1;
        }

        return {bit_range, length};
    }

  public:
    StringRef name;

    /// NOTE: for constexpr, std::array need a more
    /// large range than 6 which is ideal
    std::array<EnCoding, 8> encodings;

    /// not sure whether is constexpr
    constexpr MCOpCode(const char* InstName, const char* Pattern)
        : name(InstName) {
        /// EG: offset[11:0] rs1[4:0] 011 rd[4:0] 0000011

        /// NOTE: for constexpr, std::array need a more
        /// large range than 6 which is ideal
        auto raw_patterns = StringRef(Pattern).split<8>(' ');

        unsigned cnt = 0;
        for (auto raw_pattern_it = raw_patterns.rbegin();
             raw_pattern_it != raw_patterns.rend(); ++raw_pattern_it) {

            auto& raw_pattern = *raw_pattern_it;

            EnCoding encoding =
                StringSwitch<EnCoding>(raw_pattern)
                    .BeginWith("offset",
                               [](const StringRef& Str) -> EnCoding {
                                   /// NOTE: for constexpr, std::array need a
                                   /// more large range than 2 which is ideal
                                   auto [bit_range, length] = parseBitRange(
                                       Str.slice(7, Str.size() - 1));

                                   return EnCoding{EnCoding::kImm, length,
                                                   bit_range, std::nullopt};
                               })
                    .BeginWith("imm",
                               [](const StringRef& Str) -> EnCoding {
                                   /// NOTE: for constexpr, std::array need a
                                   /// more large range than 2 which is ideal
                                   auto [bit_range, length] = parseBitRange(
                                       Str.slice(4, Str.size() - 1));

                                   return EnCoding{EnCoding::kImm, length,
                                                   bit_range, std::nullopt};
                               })
                    .BeginWith("nzimm",
                               [](const StringRef& Str) -> EnCoding {
                                   /// NOTE: for constexpr, std::array need a
                                   /// more large range than 2 which is ideal
                                   auto [bit_range, length] = parseBitRange(
                                       Str.slice(6, Str.size() - 1));

                                   return EnCoding{EnCoding::kImm, length,
                                                   bit_range, std::nullopt};
                               })
                    .BeginWith("uimm",
                               [](const StringRef& Str) -> EnCoding {
                                   /// NOTE: for constexpr, std::array need a
                                   /// more large range than 2 which is ideal
                                   auto [bit_range, length] = parseBitRange(
                                       Str.slice(5, Str.size() - 1));

                                   return EnCoding{EnCoding::kImm, length,
                                                   bit_range, std::nullopt};
                               })
                    .BeginWith("rd_",
                               [](const StringRef& Str) -> EnCoding {
                                   // [2:0]
                                   auto [bit_range, length] = parseBitRange(
                                       Str.slice(4, Str.size() - 1));

                                   return EnCoding{EnCoding::kRd_short, length,
                                                   bit_range, std::nullopt};
                               })
                    .BeginWith("rd",
                               [](const StringRef& Str) -> EnCoding {
                                   // [4:0]
                                   auto [bit_range, length] = parseBitRange(
                                       Str.slice(3, Str.size() - 1));

                                   return EnCoding{EnCoding::kRd, length,
                                                   bit_range, std::nullopt};
                               })
                    .BeginWith(
                        "rs",
                        [](const StringRef& Str) -> EnCoding {
                            int idx = utils::stoi(Str.c_str() + 2, 1); // rsx

                            // [4:0]
                            auto [bit_range, length] = parseBitRange(
                                Str.slice(*(Str.c_str() + 3) == '_' ? 5 : 4,
                                          Str.size() - 1));

                            uint16_t kind =
                                idx + *(Str.c_str() + 3) == '_' ? 8 : 4;

                            return EnCoding{EnCoding::EnCodeTy(kind), length,
                                            bit_range, std::nullopt};
                        })
                    .BeginWith("rm",
                               [](const StringRef& Str) -> EnCoding {
                                   // [2:0]
                                   auto [bit_range, length] = parseBitRange(
                                       Str.slice(3, Str.size() - 1));

                                   return EnCoding{EnCoding::kRd, length,
                                                   bit_range, std::nullopt};
                               })
                    .BeginWith("pred",
                               [](const StringRef& Str) -> EnCoding {
                                   // [3:0]
                                   auto [bit_range, length] = parseBitRange(
                                       Str.slice(5, Str.size() - 1));

                                   return EnCoding{EnCoding::kMemFence, length,
                                                   bit_range, std::nullopt};
                               })
                    .BeginWith("succ",
                               [](const StringRef& Str) -> EnCoding {
                                   // [3:0]
                                   auto [bit_range, length] = parseBitRange(
                                       Str.slice(5, Str.size() - 1));

                                   return EnCoding{EnCoding::kMemFence, length,
                                                   bit_range, std::nullopt};
                               })
                    .Default([](const StringRef& Str) -> EnCoding {
                        return EnCoding{
                            EnCoding::kStatic,
                            static_cast<unsigned int>(Str.size()), std::nullopt,
                            utils::stoi(Str.c_str(), Str.size(), 2)};
                    });

            encodings[cnt++] = encoding;
        }
    }
};

#define ASM(name, pattern)                                                     \
    inline constexpr char name##_[] = #name;                                   \
    inline constexpr char name##_Pattern_[] = #pattern;                        \
    static constexpr MCOpCode name{name##_, name##_Pattern_};

// clang-format off

/// reference: https://ai-embedded.com/risc-v/riscv-isa-manual/

/// 32I
ASM(ADD, 0000000 rs2[4:0] rs1[4:0] 000 rd[4:0] 0110011)
ASM(SUB, 0100000 rs2[4:0] rs1[4:0] 000 rd[4:0] 0110011)
ASM(ADDI, imm[11:0] rs1[4:0] 000 rd[4:0] 0010011)
ASM(AND, 0000000 rs2[4:0] rs1[4:0] 111 rd[4:0] 0110011)
ASM(OR, 0000000 rs2[4:0] rs1[4:0] 110 rd[4:0] 0110011)
ASM(XOR, 0000000 rs2[4:0] rs1[4:0] 100 rd[4:0] 0110011)
ASM(ANDI, imm[11:0] rs1[4:0] 111 rd[4:0] 0010011)
ASM(ORI, imm[11:0] rs1[4:0] 110 rd[4:0] 0010011)
ASM(XORI, imm[11:0] rs1[4:0] 100 rd[4:0] 0010011)
ASM(SLL, 0000000 rs2[4:0] rs1[4:0] 001 rd[4:0] 0110011)
ASM(SRL, 0000000 rs2[4:0] rs1[4:0] 101 rd[4:0] 0110011)
ASM(SRA, 0100000 rs2[4:0] rs1[4:0] 101 rd[4:0] 0110011)
ASM(SLLI, 0000000 imm[4:0] rs1[4:0] 001 rd[4:0] 0010011)
ASM(SRLI, 0000000 imm[4:0] rs1[4:0] 101 rd[4:0] 0010011)
ASM(SRAI, 0100000 imm[4:0] rs1[4:0] 101 rd[4:0] 0010011)
ASM(SLT, 0000000 rs2[4:0] rs1[4:0] 010 rd[4:0] 0110011)
ASM(SLTU, 0000000 rs2[4:0] rs1[4:0] 011 rd[4:0] 0110011)
ASM(SLTI, imm[11:0] rs1[4:0] 010 rd[4:0] 0010011)
ASM(SLTIU, imm[11:0] rs1[4:0] 011 rd[4:0] 0010011)
ASM(LW, offset[11:0] rs1[4:0] 010 rd[4:0] 0000011)
ASM(LH, offset[11:0] rs1[4:0] 001 rd[4:0] 0000011)
ASM(LHU, offset[11:0] rs1[4:0] 101 rd[4:0] 0000011)
ASM(LB, offset[11:0] rs1[4:0] 000 rd[4:0] 0000011)
ASM(LBU, offset[11:0] rs1[4:0] 100 rd[4:0] 0000011)
ASM(SW, offset[11:5] rs2[4:0] rs1[4:0] 010 offset[4:0] 0100111)
ASM(SH, offset[11:5] rs2[4:0] rs1[4:0] 001 offset[4:0] 0100111)
ASM(SB, offset[11:5] rs2[4:0] rs1[4:0] 000 offset[4:0] 0100111)
ASM(BEQ, offset[12|10:5] rs2[4:0] rs1[4:0] 000 offset[4:1|11] 1100011)
ASM(BNE, offset[12|10:5] rs2[4:0] rs1[4:0] 001 offset[4:1|11] 1100011)
ASM(BLT, offset[12|10:5] rs2[4:0] rs1[4:0] 100 offset[4:1|11] 1100011)
ASM(BGE, offset[12|10:5] rs2[4:0] rs1[4:0] 101 offset[4:1|11] 1100011)
ASM(BLTU, offset[12|10:5] rs2[4:0] rs1[4:0] 110 offset[4:1|11] 1100011)
ASM(BGEU, offset[12|10:5] rs2[4:0] rs1[4:0] 111 offset[4:1|11] 1100011)
ASM(JAL, offset[20|10:1|11|19:12] rd[4:0] 1101111)
ASM(JALR, offset[11:0] rs1[4:0] 000 rd[4:0] 1100111)
ASM(LUI, imm[31:12] rd[4:0] 0110111)
ASM(AUIPC, imm[31:12] rd[4:0] 0010111)
ASM(ECALL, 000000000000 00000 000 00000 1110011)
ASM(EBREAK, 000000000001 00000 000 00000 1110011)
ASM(FENCE, 0000 pred[3:0] succ[3:0] 00000 000 00000 0001111)

/// 64I
ASM(LD, offset[11:0] rs1[4:0] 011 rd[4:0] 0000011)
ASM(SD, offset[11:5] rs2[4:0] rs1[4:0] 011 offset[4:0] 0100011)
ASM(LWU, offset[11:0] rs1[4:0] 110 rd[4:0] 0000011)
ASM(ADDIW, imm[11:0] rs1[4:0] 000 rd[4:0] 0011011)
ASM(ADDW, 0000000 rs2[4:0] rs1[4:0] 000 rd[4:0] 0111011)
ASM(SUBW, 0100000 rs2[4:0] rs1[4:0] 000 rd[4:0] 0111011)
ASM(SLLW, 0000000 rs2[4:0] rs1[4:0] 001 rd[4:0] 0111011)
ASM(SRLW, 0000000 rs2[4:0] rs1[4:0] 101 rd[4:0] 0111011)
ASM(SRAM, 0100000 rs2[4:0] rs1[4:0] 101 rd[4:0] 0111011)
ASM(SLLIW, 0000000 imm[4:0] rs1[4:0] 001 rd[4:0] 0011011)
ASM(SRLIW, 0000000 imm[4:0] rs1[4:0] 101 rd[4:0] 0011011)
ASM(SRAIW, 0100000 imm[4:0] rs1[4:0] 101 rd[4:0] 0011011)

/// 32M
ASM(MUL, 0000001 rs2[4:0] rs1[4:0] 000 rd[4:0] 0110011)
ASM(MULH, 0000001 rs2[4:0] rs1[4:0] 001 rd[4:0] 0110011)
ASM(MULHSU, 0000001 rs2[4:0] rs1[4:0] 010 rd[4:0] 0110011)
ASM(MULHU, 0000001 rs2[4:0] rs1[4:0] 011 rd[4:0] 0110011)
ASM(DIV, 0000001 rs2[4:0] rs1[4:0] 100 rd[4:0] 0110011)
ASM(REM, 0000001 rs2[4:0] rs1[4:0] 110 rd[4:0] 0110011)
ASM(REMU, 0000001 rs2[4:0] rs1[4:0] 111 rd[4:0] 0110011)

/// 64M
ASM(MULW, 0000001 rs2[4:0] rs1[4:0] 000 rd[4:0] 0111011)
ASM(DIVW, 0000001 rs2[4:0] rs1[4:0] 100 rd[4:0] 0111011)
ASM(DIVUW, 0000001 rs2[4:0] rs1[4:0] 101 rd[4:0] 0111011)
ASM(REMW, 0000001 rs2[4:0] rs1[4:0] 110 rd[4:0] 0111011)
ASM(REMUW, 0000001 rs2[4:0] rs1[4:0] 111 rd[4:0] 0111011)

/// 32A
ASM(LR_W, 00010 0 0 00000 rs1[4:0] 010 rd[4:0] 0101111)
ASM(LR_W_AQ, 00010 1 0 00000 rs1[4:0] 010 rd[4:0] 0101111)
ASM(LR_W_RL, 00010 0 1 00000 rs1[4:0] 010 rd[4:0] 0101111)
ASM(SC_W, 00011 0 0 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(SC_W_AQ, 00011 1 0 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(SC_W_RL, 00011 0 1 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOSWAP_W, 00001 0 0 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOSWAP_W_AQ, 00001 1 0 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOSWAP_W_RL, 00001 0 1 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOADD_W, 00000 0 0 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOADD_W_AQ, 00000 1 0 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOADD_W_RL, 00000 0 1 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOXOR_W, 00100 0 0 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOXOR_W_AQ, 00100 1 0 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOXOR_W_RL, 00100 0 1 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOAND_W, 01100 0 0 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOAND_W_AQ, 01100 1 0 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOAND_W_RL, 01100 0 1 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOOR_W, 01000 0 0 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOOR_W_AQ, 01000 1 0 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOOR_W_RL, 01000 0 1 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOMIN_W, 10000 0 0 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOMIN_W_AQ, 10000 1 0 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOMIN_W_RL, 10000 0 1 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOMAX_W, 10100 0 0 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOMAX_W_AQ, 10100 1 0 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOMAX_W_RL, 10100 0 1 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOMINU_W, 11000 0 0 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOMINU_W_AQ, 11000 1 0 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOMINU_W_RL, 11000 0 1 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOMAXU_W, 11100 0 0 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOMAXU_W_AQ, 11100 1 0 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)
ASM(AMOMAXU_W_RL, 11100 0 1 rs2[4:0] rs1[4:0] 010 rd[4:0] 0101111)

/// 64A
ASM(LR_D, 00010 0 0 00000 rs1[4:0] 011 rd[4:0] 0101111)
ASM(LR_D_AQ, 00010 1 0 00000 rs1[4:0] 011 rd[4:0] 0101111)
ASM(LR_D_RL, 00010 0 1 00000 rs1[4:0] 011 rd[4:0] 0101111)
ASM(SC_D, 00011 0 0 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(SC_D_AQ, 00011 1 0 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(SC_D_RL, 00011 0 1 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOSWAP_D, 00001 0 0 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOSWAP_D_AQ, 00001 1 0 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOSWAP_D_RL, 00001 0 1 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOADD_D, 00000 0 0 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOADD_D_AQ, 00000 1 0 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOADD_D_RL, 00000 0 1 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOXOR_D, 00100 0 0 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOXOR_D_AQ, 00100 1 0 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOXOR_D_RL, 00100 0 1 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOAND_D, 01100 0 0 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOAND_D_AQ, 01100 1 0 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOAND_D_RL, 01100 0 1 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOOR_D, 01000 0 0 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOOR_D_AQ, 01000 1 0 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOOR_D_RL, 01000 0 1 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOMIN_D, 10000 0 0 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOMIN_D_AQ, 10000 1 0 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOMIN_D_RL, 10000 0 1 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOMAX_D, 10100 0 0 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOMAX_D_AQ, 10100 1 0 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOMAX_D_RL, 10100 0 1 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOMINU_D, 11000 0 0 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOMINU_D_AQ, 11000 1 0 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOMINU_D_RL, 11000 0 1 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOMAXU_D, 11100 0 0 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOMAXU_D_AQ, 11100 1 0 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)
ASM(AMOMAXU_D_RL, 11100 0 1 rs2[4:0] rs1[4:0] 011 rd[4:0] 0101111)

/// 32F
ASM(FLW, offset[11:0] rs1[4:0] 010 rd[4:0] 0000111)
ASM(FSW, offset[11:5] rs2[4:0] rs1[4:0] 010 offset[4:0] 0100111)
ASM(FADD_S, 0000000 rs2[4:0] rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FSUB_S, 0000100 rs2[4:0] rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FMUL_S, 0001000 rs2[4:0] rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FDIV_S, 0001100 rs2[4:0] rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FSQRT_S, 0101100 00000 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FSGNJ_S, 0010000 rs2[4:0] rs1[4:0] 000 rd[4:0] 1010011)
ASM(FSGNJN_S, 0010000 rs2[4:0] rs1[4:0] 001 rd[4:0] 1010011)
ASM(FSGNJX_S, 0010000 rs2[4:0] rs1[4:0] 010 rd[4:0] 1010011)
ASM(FMIN_S, 0010100 rs2[4:0] rs1[4:0] 000 rd[4:0] 1010011)
ASM(FMAX_S, 0010100 rs2[4:0] rs1[4:0] 001 rd[4:0] 1010011)
ASM(FEQ_S, 1010000 rs2[4:0] rs1[4:0] 010 rd[4:0] 1010011)
ASM(FLT_S, 1010000 rs2[4:0] rs1[4:0] 001 rd[4:0] 1010011)
ASM(FLE_S, 1010000 rs2[4:0] rs1[4:0] 000 rd[4:0] 1010011)
ASM(FCLASS_S, 1110000 00000 rs1[4:0] 001 rd[4:0] 1010011)
ASM(FCVT_W_S, 1100000 00000 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FCVT_WU_S, 1100000 00001 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FCVT_S_W, 1101000 00000 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FCVT_S_WU, 1101000 00001 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FMADD_S, rs3[4:0] 00 rs2[4:0] rs1[4:0] rm[2:0] rd[4:0] 1000011)
ASM(FMSUB_S, rs3[4:0] 00 rs2[4:0] rs1[4:0] rm[2:0] rd[4:0] 1000111)
ASM(FNMSUB_S, rs3[4:0] 00 rs2[4:0] rs1[4:0] rm[2:0] rd[4:0] 1001011)
ASM(FNMADD_S, rs3[4:0] 00 rs2[4:0] rs1[4:0] rm[2:0] rd[4:0] 1001111)
ASM(FMV_X_W, 1110000 00000 rs1[4:0] 000 rd[4:0] 1010011)
ASM(FMV_W_X, 1111000 00000 rs1[4:0] 000 rd[4:0] 1010011)

/// 64F
ASM(FCVT_L_S, 1100000 00010 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FCVT_LU_S, 1100000 00011 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FCVT_S_L, 1101000 00010 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FCVT_S_LU, 1101000 00011 rs1[4:0] rm[2:0] rd[4:0] 1010011)

/// 32D
ASM(FLD, offset[11:0] rs1[4:0] 011 rd[4:0] 0000111)
ASM(FSD, offset[11:5] rs2[4:0] rs1[4:0] 011 offset[4:0] 0100111)
ASM(FADD_D, 0000001 rs2[4:0] rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FSUB_D, 0000101 rs2[4:0] rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FMUL_D, 0001001 rs2[4:0] rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FDIV_D, 0001101 rs2[4:0] rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FSQRT_D, 0101101 00000 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FSGNJ_D, 0010001 rs2[4:0] rs1[4:0] 000 rd[4:0] 1010011)
ASM(FSGNJN_D, 0010001 rs2[4:0] rs1[4:0] 001 rd[4:0] 1010011)
ASM(FSGNJX_D, 0010001 rs2[4:0] rs1[4:0] 010 rd[4:0] 1010011)
ASM(FMIN_D, 0010101 rs2[4:0] rs1[4:0] 000 rd[4:0] 1010011)
ASM(FMAX_D, 0010101 rs2[4:0] rs1[4:0] 001 rd[4:0] 1010011)
ASM(FEQ_D, 1010001 rs2[4:0] rs1[4:0] 010 rd[4:0] 1010011)
ASM(FLT_D, 1010001 rs2[4:0] rs1[4:0] 001 rd[4:0] 1010011)
ASM(FLE_D, 1010001 rs2[4:0] rs1[4:0] 000 rd[4:0] 1010011)
ASM(FCLASS_D, 1110001 00000 rs1[4:0] 001 rd[4:0] 1010011)
ASM(FMADD_D, rs3[4:0] 01 rs2[4:0] rs1[4:0] rm[2:0] rd[4:0] 1000011)
ASM(FMSUB_D, rs3[4:0] 01 rs2[4:0] rs1[4:0] rm[2:0] rd[4:0] 1000111)
ASM(FNMSUB_D, rs3[4:0] 01 rs2[4:0] rs1[4:0] rm[2:0] rd[4:0] 1001011)
ASM(FNMADD_D, rs3[4:0] 01 rs2[4:0] rs1[4:0] rm[2:0] rd[4:0] 1001111)
ASM(FCVT_W_D, 1100001 00000 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FCVT_WU_D, 1100001 00001 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FCVT_D_W, 1101001 00000 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FCVT_D_WU, 1101001 00001 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FCVT_S_D, 0100000 00001 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FCVT_D_S, 0100001 00000 rs1[4:0] rm[2:0] rd[4:0] 1010011)

/// 64D
ASM(FCVT_L_D, 1100001 00010 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FCVT_LU_D, 1100001 00011 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FCVT_D_L, 1101001 00010 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FCVT_D_LU, 1101001 00011 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FMV_X_D, 1110001 00000 rs1[4:0] 000 rd[4:0] 1010011)
ASM(FMV_D_X, 1111001 00000 rs1[4:0] 000 rd[4:0] 1010011)

/// 32C
ASM(C_ADDI, 000 imm[5] rd[4:0] imm[4:0] 01)
ASM(C_LI_W, 010 imm[5] rd[4:0] imm[4:0] 01)
ASM(C_LUI_W, 011 nzimm[17] rd[4:0] nzimm[16:12] 01)
ASM(C_MV, 1000 0 rs2[4:0] rd[4:0] 10)
ASM(C_ADD, 1001 0 rs2[4:0] rd[4:0] 10)
ASM(C_LW, 010 offset[5:3] rs1_[2:0] offset[2|6] rd_[2:0] 00)
ASM(C_SW, 110 offset[5:3] rs1_[2:0] offset[2|6] rs2_[2:0] 00)
ASM(C_J, 101 offset[11|4|9:8|10|6|7|3:1|5] 01)
ASM(C_JAL, 001 offset[11|4|9:8|10|6|7|3:1|5] 01)
ASM(C_BEQZ, 110 offset[8|4:3] rs1_[2:0] offset[7:6|2:1|5] 01)
ASM(C_BNEZ, 111 offset[8|4:3] rs1_[2:0] offset[7:6|2:1|5] 01)
ASM(C_ADDI16SP, 011 nzimm[9] 00010 nzimm[4|6|8:7|5] 01)
ASM(C_LWSP, 010 offset[5] rd[4:0] offset[4:2|7:6] 10)
ASM(C_SWSP, 110 offset[5:2|7:6] rs2[4:0] 10)
ASM(C_NOP, 000 0 00000 00000 01)
ASM(C_EBREAK, 100 1 00000 00000 10)
ASM(C_ADDI4SPN, 000 nzuimm[5:4|9:6|2|3] rd_[2:0] 00)
ASM(C_SLLI_W, 000 imm[5] rd[4:0] imm[4:0] 10) // shamt
ASM(C_SRLI_W, 100 imm[5] 00 rd_[2:0] imm[4:0] 01) // shamt
ASM(C_ANDI, 100 imm[5] 10 rd_[2:0] imm[4:0] 01)
ASM(C_SUB, 100 0 11 rd_[2:0] 00 rs2_[2:0] 01)
ASM(C_XOR, 100 0 11 rd_[2:0] 01 rs2_[2:0] 01)
ASM(C_OR, 100 0 11 rd_[2:0] 10 rs2_[2:0] 01)
ASM(C_AND, 100 0 11 rd_[2:0] 11 rs2_[2:0] 01)
ASM(C_JR, 100 0 rs1[4:0] 00000 10)
ASM(C_JALR, 100 1 rs1[4:0] 00000 10)

/// 64C
ASM(C_ADDIW, 001 imm[5] rd[4:0] imm[4:0] 01)
ASM(C_SUBW, 100 1 11 rd_[2:0] 00 rs2_[2:0] 01)
ASM(C_ADDW, 100 1 11 rd_[2:0] 01 rs2_[2:0] 01)
ASM(C_LD, 011 offset[5:3] rs1_[2:0] offset[7:6] rd_[2:0] 00)
ASM(C_SD, 111 offset[5:3] rs1_[2:0] offset[7:6] rs2_[2:0] 00)
ASM(C_LDSP, 011 offset[5] rd[4:0] offset[4:3|8:6] 10)
ASM(C_SDSP, 111 offset[5:3|8:6] rs2[4:0] 10)
ASM(C_LI_D, 010 imm[5] rd[4:0] imm[4:0] 01)
ASM(C_LUI_D, 011 nzimm[17] rd[4:0] nzimm[16:12] 01)
ASM(C_SLLI_D, 000 0 rd[4:0] uimm[5|4:0] 10)
ASM(C_SRLI_D, 100 0 00 rd_[2:0] uimm[5|4:0] 01)
ASM(C_SRAI, 100 0 01 rd_[2:0] uimm[5|4:0] 01)

/// 64C

// clang-format on

} // namespace mc

#endif