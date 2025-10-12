#ifndef MC_INST
#define MC_INST

#include "MCExpr.hpp"
#include "mc/MCOpCode.hpp"
#include "mc/MCOperand.hpp"
#include "utils/ADT/SmallVector.hpp"
#include "utils/macro.hpp"
#include "utils/source.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace mc {

using Location = utils::Location;

class MCInst {
public:
  using size_ty = std::size_t;

private:
  const MCOpCode* OpCode; // MCOpCode will all be static and constepxr

  Location Loc;
  size_ty Offset; // offset from the begin of .text
  SmallVector<MCOperand, 6> Operands;

public:
  MCInst() = delete;
  explicit MCInst(MCOpCode* _OpCode LIFETIME_BOUND, Location _Loc,
                  size_ty _Offset)
      : OpCode(_OpCode), Loc(_Loc), Offset(_Offset), Operands(6) {}

  explicit MCInst(const StringRef& _OpCode LIFETIME_BOUND, Location _Loc,
                  size_ty _Offset)
      : OpCode(parser::MnemonicFind(_OpCode.c_str())), Loc(_Loc),
        Offset(_Offset), Operands(6) {}

  [[nodiscard]] decltype(Operands)::size_ty getOpSize() const {
    return Operands.size();
  }

  const MCOperand& addOperand(MCOperand&& newOp) {
    utils_assert(Operands.size() < Operands.capacity(),
                 "too many operand for an inst");
    Operands.push_back(std::move(newOp));
    return Operands.back();
  }

  void addOperand(const MCOperand& newOp) {
    utils_assert(Operands.size() < Operands.capacity(),
                 "too many operand for an inst");

    Operands.push_back(newOp);
  }

  const MCOpCode* getOpCode() const { return OpCode; }
  utils::Location getLoc() const { return Loc; }

  template <decltype(Operands)::size_ty Idx>
  const MCOperand& getOperand() const {
    utils_assert(Idx < Operands.size(),
                 "random access to uninitialized memery");
    return Operands[Idx];
  }

  bool isCompressed() const { return OpCode->name.begin_with("c."); }

  /// allow 20 bits offset
  bool isJmp() const { return OpCode->name.begin_with("j"); }

  bool isBranch() const { return OpCode->name.begin_with("b"); }

  MCExpr::ExprTy getModifier() const {
    for (auto& operand : Operands) {
      if (operand.isExpr()) {
        return operand.getExpr()->getModifier();
      }
    }

    return MCExpr::kInValid;
  }

  size_ty getOffset() const { return Offset; }

  using const_iter = decltype(Operands)::const_iter;
  using const_rev_iter = decltype(Operands)::const_rev_iter;

  const_iter begin() const { return Operands.begin(); }
  const_iter end() const { return Operands.end(); }

  const_rev_iter rbegin() const { return Operands.rbegin(); }
  const_rev_iter rend() const { return Operands.rend(); }

  void reloSym(int64_t offset);

private:
  bool hasExpr() const {
    return std::any_of(Operands.begin(), Operands.end(),
                       [&](const MCOperand& op) { return op.isExpr(); });
  }

  const MCOperand* findExprOp() const {
    return std::find_if(Operands.begin(), Operands.end(),
                        [&](const MCOperand& op) { return op.isExpr(); });
  }

public:
  uint32_t getReloType() const;

  /// TODO: dump(), verify()
};

} // namespace mc

#endif