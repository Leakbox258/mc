#include "mc/MCInst.hpp"
#include "mc/MCExpr.hpp"
#include "mc/MCOpCode.hpp"
#include "mc/MCOperand.hpp"
#include "utils/logger.hpp"
#include "utils/macro.hpp"
#include "utils/misc.hpp"
#include <algorithm>
#include <asm-generic/errno.h>
#include <cstdint>
#include <elf.h>

using namespace mc;

void MCInst::reloSym(int64_t offset) {
  uint64_t encoding;

  if (isBranch()) {
    /// 12 bits offset
    encoding = utils::signIntCompress<12 + 1>(offset);
  } else if (isJmp()) {
    /// 20 bits offset
    encoding = utils::signIntCompress<20 + 1>(offset);
  } else {
    /// check modifiers
    auto mod = getModifier();
    utils_assert(mod, "ExprTy is invalid");
    encoding = utils::signIntCompress(offset, getModifierSize(mod));
  }

  if (isBranch() || isJmp()) {
    auto op = std::find_if(Operands.begin(), Operands.end(),
                           [&](MCOperand& op) { return op.isImm(); });
    auto& imm = op->getImm();
    imm = encoding;

  } else {
    auto op = std::find_if(Operands.begin(), Operands.end(),
                           [&](MCOperand& op) { return op.isExpr(); });
    op->RewriteSymRelo(encoding);
  }

  return;
}

uint32_t MCInst::getReloType() const {
  if (!hasExpr()) {
    /// Relo type will rely on the instruction types
    /// jmp / branch inst
    /// TODO: to fit more relo type
    return StringSwitch<uint32_t>(OpCode->name)
        .BeginWith("j", static_cast<uint32_t>(R_RISCV_JAL))
        .BeginWith("b", static_cast<uint32_t>(R_RISCV_BRANCH))
        .BeginWith("c.j", static_cast<uint32_t>(R_RISCV_RVC_JUMP))
        .BeginWith("c.b", static_cast<uint32_t>(R_RISCV_RVC_BRANCH))
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
    unsigned length;

    void add(uint32_t elem, unsigned len) {
      bits |= (elem & ((1u << len) - 1)) << length;
      length += len;
    }
  };

  Bits inst;

  for (auto& encode : pattern) {
    auto length = encode.length;

    switch (encode.kind) {
    case EnCoding::kInvalid:
      utils::unreachable("invalid encoding");
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
      auto immOp = this->findImmOp();

      for (auto [high, low] : *encode.bit_range) {
        inst.add(immOp.getImmSlice(high, low), high - low + 1);
      }

      break;
    }
  }

  if (this->isCompressed()) {
    utils_assert(inst.length == 16, "encoding check failed");
  } else {
    utils_assert(inst.length == 32, "encoding check failed");
  }

  return inst.bits;
}