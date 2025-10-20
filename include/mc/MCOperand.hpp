#ifndef MC_OPERAND
#define MC_OPERAND

#include "utils/ADT/StringMap.hpp"
#include "utils/macro.hpp"
#include "utils/misc.hpp"
#include <cstdint>
namespace mc {

template <typename V> using StringMap = utils::ADT::StringMap<V>;

inline const StringMap<uint8_t> Registers = {
    {"zero", 0}, {"ra", 1},   {"sp", 2},    {"gp", 3},    {"tp", 4},
    {"t0", 5},   {"t1", 6},   {"t2", 7},    {"s0", 8},    {"fp", 8},
    {"s1", 9},   {"a0", 10},  {"a1", 11},   {"a2", 12},   {"a3", 13},
    {"a4", 14},  {"a5", 15},  {"a6", 16},   {"a7", 17},   {"s2", 18},
    {"s3", 19},  {"s4", 20},  {"s5", 21},   {"s6", 22},   {"s7", 23},
    {"s8", 24},  {"s9", 25},  {"s10", 26},  {"s11", 27},  {"t3", 28},
    {"t4", 29},  {"t5", 30},  {"t6", 31},   {"x0", 0},    {"x1", 1},
    {"x2", 2},   {"x3", 3},   {"x4", 4},    {"x5", 5},    {"x6", 6},
    {"x7", 7},   {"x8", 8},   {"x9", 9},    {"x10", 10},  {"x11", 11},
    {"x12", 12}, {"x13", 13}, {"x14", 14},  {"x15", 15},  {"x16", 16},
    {"x17", 17}, {"x18", 18}, {"x19", 19},  {"x20", 20},  {"x21", 21},
    {"x22", 22}, {"x23", 23}, {"x24", 24},  {"x25", 25},  {"x26", 26},
    {"x27", 27}, {"x28", 28}, {"x29", 29},  {"x30", 30},  {"x31", 31},
    {"f0", 0},   {"f1", 1},   {"f2", 2},    {"f3", 3},    {"f4", 4},
    {"f5", 5},   {"f6", 6},   {"f7", 7},    {"f8", 8},    {"f9", 9},
    {"f10", 10}, {"f11", 11}, {"f12", 12},  {"f13", 13},  {"f14", 14},
    {"f15", 15}, {"f16", 16}, {"f17", 17},  {"f18", 18},  {"f19", 19},
    {"f20", 20}, {"f21", 21}, {"f22", 22},  {"f23", 23},  {"f24", 24},
    {"f25", 25}, {"f26", 26}, {"f27", 27},  {"f28", 28},  {"f29", 29},
    {"f30", 30}, {"f31", 31}, {"ft0", 0},   {"ft1", 1},   {"ft2", 2},
    {"ft3", 3},  {"ft4", 4},  {"ft5", 5},   {"ft6", 6},   {"ft7", 7},
    {"fs0", 8},  {"fs1", 9},  {"fa0", 10},  {"fa1", 11},  {"fa2", 12},
    {"fa3", 13}, {"fa4", 14}, {"fa5", 15},  {"fa6", 16},  {"fa7", 17},
    {"fs2", 18}, {"fs3", 19}, {"fs4", 20},  {"fs5", 21},  {"fs6", 22},
    {"fs7", 23}, {"fs8", 24}, {"fs9", 25},  {"fs10", 26}, {"fs11", 27},
    {"ft8", 28}, {"ft9", 29}, {"ft10", 30}, {"ft11", 31}};

inline const StringMap<uint8_t> CRegisters = {
    {"x8", 0},  {"x9", 1},  {"x10", 2}, {"x11", 3}, {"x12", 4}, {"x13", 5},
    {"x14", 6}, {"x15", 7}, {"s0", 0},  {"s1", 1},  {"a0", 2},  {"a1", 3},
    {"a2", 4},  {"a3", 5},  {"a4", 6},  {"a5", 7},  {"f8", 0},  {"f9", 1},
    {"f10", 2}, {"f11", 3}, {"f12", 4}, {"f13", 5}, {"f14", 6}, {"f15", 7},
    {"fs0", 0}, {"fs1", 1}, {"fa0", 2}, {"fa1", 3}, {"fa2", 4}, {"fa3", 5},
    {"fa4", 6}, {"fa5", 7}};

inline const StringMap<uint8_t> RoundModes = {
    {"rnz", 000}, {"rtz", 001}, {"rdn", 010}, {"rup", 011}, {"rmm", 100}};

class MCContext;
class MCExpr;
class MCInst;

using MCReg = uint8_t;

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
    MCReg Reg;
    int64_t Imm;
    uint32_t SFPImm;
    uint64_t DFPImm;
    const MCExpr* Expr;
    const MCInst* Inst;
  };

  /// imms in RV maybe distribute separately
  /// when traversal MCOpCode::EnCoding, use this elem to record the length that
  /// is already used
  /// when op is arith: use 12 elements
  /// when op is branch: use 13 elements (the lowest never used)
  /// when op is jump: use 21 elements (the lowest never used)
  // std::array<unsigned char, 21> used{};

public:
  MCOperand() : DFPImm(0) {}

  static MCOperand makeReg(MCReg _Reg) {
    MCOperand op;

    op.Reg = std::move(_Reg);
    op.Kind = OpTy::kReg;
    return op;
  }

  static MCOperand makeImm(int64_t _Imm) {
    MCOperand op;

    op.Imm = std::move(_Imm);
    op.Kind = OpTy::kImme;
    return op;
  }

  static MCOperand makeSFPImm(uint32_t _SFPImm) {
    MCOperand op;

    op.SFPImm = std::move(_SFPImm);
    op.Kind = OpTy::kSFPImme;
    return op;
  }

  static MCOperand makeDFPImm(uint64_t _DFPImm) {
    MCOperand op;

    op.DFPImm = std::move(_DFPImm);
    op.Kind = OpTy::kDFPImme;
    return op;
  }

  static MCOperand makeExpr(const MCExpr* _Expr) {
    MCOperand op;

    op.Expr = std::move(_Expr);
    op.Kind = OpTy::kExpr;
    return op;
  }

  static MCOperand makeInst(const MCInst* _Inst) {
    MCOperand op;

    op.Inst = std::move(_Inst);
    op.Kind = OpTy::kInst;
    return op;
  }

  bool isValid() const { return Kind != kInvalid; }
  bool isReg() const { return Kind == kReg; }
  bool isImm() const { return Kind == kImme; }
  bool isSFPImm() const { return Kind == kSFPImme; }
  bool isDFPImm() const { return Kind == kDFPImme; }
  bool isGImm() const { return utils::in_set(Kind, kImme, kSFPImme, kDFPImme); }
  bool isExpr() const { return Kind == kExpr; }
  bool isInst() const { return Kind == kInst; }

  MCReg getReg() const {
    utils_assert(isReg(), "not a reg");
    return Reg;
  }

  MCReg& getReg() {
    utils_assert(isReg(), "not a reg");
    return Reg;
  }

  int64_t getImm() const {
    utils_assert(isImm(), "not an immediate");
    return Imm;
  }

  int64_t& getImm() {
    utils_assert(isImm(), "not an immediate");
    return Imm;
  }

  uint32_t getSFPImm() const {
    utils_assert(isSFPImm(), "not an SFP immediate");
    return SFPImm;
  }

  uint32_t& getSFPImm() {
    utils_assert(isSFPImm(), "not an SFP immediate");
    return SFPImm;
  }

  uint64_t getDFPImm() const {
    utils_assert(isDFPImm(), "not an FP immediate");
    return DFPImm;
  }

  uint64_t& getDFPImm() {
    utils_assert(isDFPImm(), "not an FP immediate");
    return DFPImm;
  }

  uint64_t getGImm() const {
    utils_assert(isGImm(), "not an Generic immediate");
    return Imm;
  }

  const MCExpr* getExpr() const {
    utils_assert(isExpr(), "not an expression");
    return Expr;
  }

  const MCInst* getInst() const {
    utils_assert(isInst(), "not a sub-instruction");
    return Inst;
  }

  /// encoding here expected to be processed
  void RewriteSymRelo(uint64_t encoding) {
    utils_assert(isExpr(), "expecting to have at least one expr*");

    /// change Kind
    Imm = encoding;
    this->Kind = OpTy::kImme;
  }

  /// TODO: dump / verify
};
} // namespace mc

#endif