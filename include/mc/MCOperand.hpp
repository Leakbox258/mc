#ifndef MC_OPERAND
#define MC_OPERAND

#include "utils/macro.hpp"
#include <cstdint>
namespace mc {

class MCContext;
class MCExpr;
class MCInst;

using MCReg = uint16_t;

class MCOperand {
    friend MCContext;

    enum OpTy : unsigned char {
        kInvalid,
        kReg,
        kImme,
        kSFPImme,
        kDFPImme,
        kExpr,
        kInst,
    };

    OpTy Kind = kInvalid;

    union {
        uint16_t Reg;
        int64_t Imm;
        uint32_t SFPImm;
        uint64_t DFPImm;
        const MCExpr* Expr;
        const MCInst* Inst;
    };

  public:
    MCOperand() : DFPImm(0) {}

    bool isValid() const { return Kind != kInvalid; }
    bool isReg() const { return Kind == kReg; }
    bool isImm() const { return Kind == kImme; }
    bool isSFPImm() const { return Kind == kSFPImme; }
    bool isDFPImm() const { return Kind == kDFPImme; }
    bool isExpr() const { return Kind == kExpr; }
    bool isInst() const { return Kind == kInst; }

    MCReg getReg() const {
        utils_assert(isReg(), "not a reg");
        return MCReg(Reg);
    }

    int64_t getImm() const {
        utils_assert(isImm(), "not an immediate");
        return Imm;
    }

    uint32_t getSFPImm() const {
        utils_assert(isSFPImm(), "not an SFP immediate");
        return SFPImm;
    }

    uint64_t getDFPImm() const {
        utils_assert(isDFPImm(), "not an FP immediate");
        return DFPImm;
    }

    const MCExpr* getExpr() const {
        utils_assert(isExpr(), "not an expression");
        return Expr;
    }

    const MCInst* getInst() const {
        utils_assert(isInst(), "not a sub-instruction");
        return Inst;
    }

    /// TODO: dump / verify
};
} // namespace mc

#endif