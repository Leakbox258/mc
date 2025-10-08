#ifndef MC_OPCODE
#define MC_OPCODE

#include "utils/ADT/SmallVector.hpp"
#include "utils/ADT/StringRef.hpp"
#include "utils/ADT/StringSwitch.hpp"
#include "utils/numeric.hpp"
#include <cstdint>
#include <optional>

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
        kRd,
        kRs1,
        kRs2,
        kRs3,
        kRd_short,
        kRs1_short,
        kRs2_short,
        kRs3_short,
        kRm, // rounding mode (F/D extension)
        // AQ or RL will concat with mnemonic in A extension
        // eg: LR.D.AQ t1, (s0)
    };

    EnCodeTy kind = kInvalid;
    unsigned length;
    std::optional<uint16_t> static_pattern;
};

template <const char* InstName, const char* Pattern> struct MCOpCode {
    StringRef name;

    /// NOTE: for constexpr, std::array need a more
    /// large range than 6 which is ideal
    std::array<EnCoding, 8> encodings;

    /// not sure whether is constexpr
    constexpr MCOpCode() : name(InstName) {
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
                    .BeginWith(
                        "offset",
                        [](const StringRef& Str) -> EnCoding {
                            /// NOTE: for constexpr, std::array need a more
                            /// large range than 2 which is ideal
                            auto bit_range =
                                Str.slice(7, Str.size() - 1).split<6>(':');

                            int high = utils::stoi(bit_range[0].c_str(),
                                                   bit_range[0].size());
                            int low = utils::stoi(bit_range[1].c_str(),
                                                  bit_range[1].size());

                            return EnCoding{
                                EnCoding::kImm,
                                static_cast<unsigned int>(high - low),
                                std::nullopt};
                        })
                    .BeginWith(
                        "imm",
                        [](const StringRef& Str) -> EnCoding {
                            /// NOTE: for constexpr, std::array need a more
                            /// large range than 2 which is ideal
                            auto bit_range =
                                Str.slice(4, Str.size() - 1).split<6>(':');

                            int high = utils::stoi(bit_range[0].c_str(),
                                                   bit_range[0].size());
                            int low = utils::stoi(bit_range[1].c_str(),
                                                  bit_range[1].size());

                            return EnCoding{
                                EnCoding::kImm,
                                static_cast<unsigned int>(high - low),
                                std::nullopt};
                        })
                    .BeginWith("rd'",
                               [](const StringRef& Str) -> EnCoding {
                                   // [2:0]

                                   return EnCoding{EnCoding::kRd_short, 3,
                                                   std::nullopt};
                               })
                    .BeginWith("rs'",
                               [](const StringRef& Str) -> EnCoding {
                                   int idx =
                                       utils::stoi(Str.c_str() + 3, 1); // rsx

                                   // [2:0]
                                   return EnCoding{EnCoding::EnCodeTy(8 + idx),
                                                   5, std::nullopt};
                               })
                    .BeginWith("rd",
                               [](const StringRef& Str) -> EnCoding {
                                   // [4:0]

                                   return EnCoding{EnCoding::kRd, 5,
                                                   std::nullopt};
                               })
                    .BeginWith("rs",
                               [](const StringRef& Str) -> EnCoding {
                                   int idx =
                                       utils::stoi(Str.c_str() + 2, 1); // rsx

                                   // [4:0]
                                   return EnCoding{EnCoding::EnCodeTy(4 + idx),
                                                   5, std::nullopt};
                               })
                    .BeginWith("rm",
                               [](const StringRef& Str) -> EnCoding {
                                   // [2:0]

                                   return EnCoding{EnCoding::kRd, 3,
                                                   std::nullopt};
                               })
                    .Default([](const StringRef& Str) -> EnCoding {
                        return EnCoding{
                            EnCoding::kStatic,
                            static_cast<unsigned int>(Str.size()),
                            utils::stoi(Str.c_str(), Str.size(), 2)};
                    });

            encodings[cnt++] = encoding;
        }
    }
};

#define ASM(name, pattern)                                                     \
    inline constexpr char name##_[] = #name;                                   \
    inline constexpr char name##_Pattern_[] = #pattern;                        \
    static constexpr MCOpCode<name##_, name##_Pattern_> name;

// clang-format off

/// reference: https://ai-embedded.com/risc-v/riscv-isa-manual/

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

/// 64M
ASM(MULW, 0000001 rs2[4:0] rs1[4:0] 000 rd[4:0] 0111011)
ASM(DIVW, 0000001 rs2[4:0] rs1[4:0] 100 rd[4:0] 0111011)
ASM(DIVUW, 0000001 rs2[4:0] rs1[4:0] 101 rd[4:0] 0111011)
ASM(REMW, 0000001 rs2[4:0] rs1[4:0] 110 rd[4:0] 0111011)
ASM(REMUW, 0000001 rs2[4:0] rs1[4:0] 111 rd[4:0] 0111011)

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

/// 64F
ASM(FCVT_L_S, 1100000 00010 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FCVT_LU_S, 1100000 00011 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FCVT_S_L, 1101000 00010 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FCVT_S_LU, 1101000 00011 rs1[4:0] rm[2:0] rd[4:0] 1010011)

/// 64D
ASM(FCVT_L_D, 1100001 00010 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FCVT_LU_D, 1100001 00011 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FCVT_D_L, 1101001 00010 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FCVT_D_LU, 1101001 00011 rs1[4:0] rm[2:0] rd[4:0] 1010011)
ASM(FMV_X_D, 1110001 00000 rs1[4:0] 000 rd[4:0] 1010011)
ASM(FMV_D_X, 1111001 00000 rs1[4:0] 000 rd[4:0] 1010011)

// clang-format on

} // namespace mc

#endif