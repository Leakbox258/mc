#ifndef MC_INST
#define MC_INST

#include "mc/MCOpCode.hpp"
#include "mc/MCOperand.hpp"
#include "utils/ADT/SmallVector.hpp"
#include "utils/macro.hpp"
#include "utils/source.hpp"

namespace mc {

using Location = utils::Location;

class MCInst {
  private:
    const MCOpCode* OpCode; // MCOpCode will all be static and constepxr

    Location Loc;
    SmallVector<MCOperand, 6> Operands;

  public:
    MCInst() = delete;
    explicit MCInst(MCOpCode* _OpCode LIFETIME_BOUND, Location _Loc)
        : OpCode(_OpCode), Loc(_Loc), Operands(6) {}

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

    using const_iter = decltype(Operands)::const_iter;
    using const_rev_iter = decltype(Operands)::const_rev_iter;

    const_iter begin() const { return Operands.begin(); }
    const_iter end() const { return Operands.end(); }

    const_rev_iter rbegin() const { return Operands.rbegin(); }
    const_rev_iter rend() const { return Operands.rend(); }

    /// TODO: dump(), verify()
};

} // namespace mc

#endif