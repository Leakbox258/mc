#include "mc/MCInst.hpp"
#include "mc/MCExpr.hpp"
#include "mc/MCOpCode.hpp"
#include "mc/MCOperand.hpp"
#include "utils/logger.hpp"
#include "utils/macro.hpp"
#include "utils/misc.hpp"
#include <asm-generic/errno.h>
#include <cstdint>
#include <elf.h>

using namespace mc;

void MCInst::reloSym(int64_t offset) {

  if (auto mod = getModifier()) {
    auto op = std::find_if(Operands.begin(), Operands.end(),
                           [&](MCOperand& op) { return op.isExpr(); });
    op->RewriteSymRelo(utils::signIntCompress(offset, getModifierSize(mod)));
  } else {
    // usually branch & jumps without any explicit modifier
    auto op = std::find_if(Operands.begin(), Operands.end(),
                           [&](MCOperand& op) { return op.isImm(); });
    auto& imm = op->getImm();
    imm = offset;
  }

  return;
}

uint32_t MCInst::getReloType() const {
  if (!hasExpr()) {
    /// Relo type will rely on the instruction types
    /// jmp / branch inst
    /// TODO: to fit more relo type
    return StringSwitch<uint32_t>(OpCode->name)
        .BeginWith("J", static_cast<uint32_t>(R_RISCV_JAL))
        .BeginWith("B", static_cast<uint32_t>(R_RISCV_BRANCH))
        .BeginWith("C_J", static_cast<uint32_t>(R_RISCV_RVC_JUMP))
        .BeginWith("C_B", static_cast<uint32_t>(R_RISCV_RVC_BRANCH))
        .Error();
  } else {
    using ExprTy = MCExpr::ExprTy;

    switch (getExprOp()->getExpr()->getModifier()) {
    case ExprTy::kInValid:
      utils::unreachable("invalid modifier");
    case ExprTy::kLO:
      return OpCode->imm_distribute == 1 ? R_RISCV_LO12_I : R_RISCV_LO12_S;
    case ExprTy::kPCREL_LO:
      return OpCode->imm_distribute == 1 ? R_RISCV_PCREL_LO12_I
                                         : R_RISCV_PCREL_LO12_S;
    case ExprTy::kHI:
      return R_RISCV_HI20;
    case ExprTy::kPCREL_HI:
      return R_RISCV_PCREL_HI20;
    case ExprTy::kGOT_PCREL_HI:
      return R_RISCV_GOT_HI20;
    case ExprTy::kTPREL_ADD:
      return R_RISCV_TPREL_ADD;
    case ExprTy::kTPREL_HI:
      return R_RISCV_TPREL_HI20;
    case ExprTy::kTLS_IE_PCREL_HI:
      return R_RISCV_TLS_GOT_HI20;
    case ExprTy::kTLS_GD_PCREL_HI:
      return R_RISCV_TLS_GD_HI20;
    }
  }
}

uint32_t MCInst::makeEncoding() const {
  auto& pattern = OpCode->encodings;

  struct Bits {
    uint32_t bits = 0;
    unsigned len = 0;

    void add(uint32_t elem, unsigned length) {
      bits |= (elem & ((1u << length) - 1)) << len;
      len += length;
    }
  };

  Bits inst;

  for (auto& encode : pattern) {
    auto length = encode.length;
    auto highest = encode.highest;

    switch (encode.kind) {
    case EnCoding::kInvalid:
      /// typically, reach the end of the pattern array
      continue;
    case EnCoding::kStatic:
      inst.add(*encode.static_pattern, length);
      break;
    case EnCoding::kRd:
    case EnCoding::kRd_short:
      inst.add(this->findRegOp<0>().getReg(), length);
      break;
    case EnCoding::kRs1:
    case EnCoding::kRs1_short:
      inst.add(this->findRegOp<1>().getReg(), length);
      break;
    case EnCoding::kRs2:
    case EnCoding::kRs2_short:
      inst.add(this->findRegOp<2>().getReg(), length);
      break;
    case EnCoding::kRs3:
    case EnCoding::kRs3_short:
      inst.add(this->findRegOp<3>().getReg(), length);
      break;
    case EnCoding::kRm:
    case EnCoding::kMemFence:
    case EnCoding::kImm:
    case EnCoding::kNzImm:
    case EnCoding::kUImm:
      auto immOp = this->findGImmOp();

      unsigned tmp_len = 0;
      int64_t gimm =
          utils::signIntCompress(immOp.getGImm(), highest + 1); // expecting len

      for (auto [high, low] : *encode.bit_range) {
        if (tmp_len == length) {
          break;
        }

        auto immSlice = utils::signIntSlice(gimm, high, low);
        inst.add(immSlice, high - low + 1);
        tmp_len += high - low + 1;
      }

      break;
    }
  }

  if (this->isCompressed()) {
    utils_assert(inst.len == 16, "encoding check failed");
  } else {
    utils_assert(inst.len == 32, "encoding check failed");
  }

  return inst.bits;
}