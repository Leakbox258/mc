#ifndef MC_EXPR
#define MC_EXPR

#include "utils/ADT/StringRef.hpp"
#include <cstdint>
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
    StringRef Symbol;

  public:
    MCExpr() : Kind(kInValid), Symbol() {}

    template <ExprTy ty>
    MCExpr(StringRef _Symbol) : Kind(ty), Symbol(_Symbol) {}

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

    /// TODO: dump
};
} // namespace mc

#endif
