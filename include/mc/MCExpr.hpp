#ifndef MC_EXPR
#define MC_EXPR

#include "utils/ADT/StringRef.hpp"
#include "utils/ADT/StringSwitch.hpp"
#include <cstdint>
#include <utility>
namespace mc {
using StringRef = utils::ADT::StringRef;

class MCExpr {
public:
  enum ExprTy : uint8_t {
    kInValid,
    kLO,
    kHI,
    kPCREL_LO,
    kPCREL_HI,
    kGOT_PCREL_HI,
    kTPREL_ADD,
    kTPREL_HI,
    kTLS_IE_PCREL_HI, // Initial Exec
    kTLS_GD_PCREL_HI  // Global Dynamic
  };

private:
  ExprTy Kind = kInValid;
  std::string Symbol;
  uint64_t Append; // +/-
public:
  // MCExpr() : Kind(kInValid), Symbol() {}

  MCExpr(ExprTy ty, StringRef _Symbol, uint64_t _Append)
      : Kind(std::move(ty)), Symbol(_Symbol.str()), Append(_Append) {}

  bool isLO() const { return Kind == kLO; }
  bool isHI() const { return Kind == kHI; }
  bool isPCREL_LO() const { return Kind == kPCREL_LO; }
  bool isPCREL_HI() const { return Kind == kPCREL_HI; }
  bool isGOT_PCREL_HI() const { return Kind == kGOT_PCREL_HI; }
  bool isTPREL_ADD() const { return Kind == kTPREL_ADD; }
  bool isTPREL_HI() const { return Kind == kTPREL_HI; }
  bool isTLS_IE_PCREL_HI() const { return Kind == kTLS_IE_PCREL_HI; }
  bool isTLS_GD_PCREL_HI() const { return Kind == kTLS_GD_PCREL_HI; }

  StringRef getSym() const { return Symbol; }
  ExprTy getModifier() const { return Kind; }
  uint64_t getAppend() const { return Append; }

  /// TODO: dump
};

inline MCExpr::ExprTy getExprTy(const StringRef& Mod) {
  return utils::ADT::StringSwitch<MCExpr::ExprTy>(Mod)
      .Case("%lo", MCExpr::kLO)
      .Case("%hi", MCExpr::kHI)
      .Case("%pcrel_lo", MCExpr::kPCREL_LO)
      .Case("%pcrel_hi", MCExpr::kPCREL_HI)
      .Case("%got_pcrel_hi", MCExpr::kGOT_PCREL_HI)
      .Case("%tprel_add", MCExpr::kTPREL_ADD)
      .Case("%tprel_hi", MCExpr::kTPREL_HI)
      .Case("%tls_ie_pcrel_hi", MCExpr::kTLS_IE_PCREL_HI)
      .Case("%tls_gd_pcrel_hi", MCExpr::kTLS_GD_PCREL_HI)
      .Default(MCExpr::kInValid);
}

inline unsigned getModifierSize(MCExpr::ExprTy mod) {
  using ExprTy = MCExpr::ExprTy;
  switch (mod) {
  case ExprTy::kInValid:
    utils::unreachable("invalid modifier");
  case ExprTy::kLO:
  case ExprTy::kPCREL_LO:
    return 12;
  case ExprTy::kHI:
  case ExprTy::kPCREL_HI:
  case ExprTy::kGOT_PCREL_HI:
  case ExprTy::kTPREL_ADD:
  case ExprTy::kTPREL_HI:
  case ExprTy::kTLS_IE_PCREL_HI:
  case ExprTy::kTLS_GD_PCREL_HI:
    return 20;
  }
  utils::unreachable("unknown modifier");
  return 0;
}

} // namespace mc

#endif
