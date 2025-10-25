#ifndef MC_PSEUDO
#define MC_PSEUDO

#include "MCExpr.hpp"
#include "MCInst.hpp"
#include "MCOperand.hpp"
#include "utils/ADT/SmallVector.hpp"
#include "utils/ADT/StringMap.hpp"
#include <cstdint>

namespace mc {
using MCInsts = utils::ADT::SmallVector<MCInst, 2>;
template <typename T> using StringMap = utils::ADT::StringMap<T>;

/// used as call-back
MCInsts ld_rd_symbol(MCReg rd, StringRef symbol);
MCInsts lda_rd_symbol(MCReg rd, StringRef symbol);
MCInsts lla_rd_symbol(MCReg rd, StringRef symbol);
MCInsts lga_rd_symbol(MCReg rd, StringRef symbol);
MCInsts lb_rd_symbol(MCReg rd, StringRef symbol);
MCInsts lh_rd_symbol(MCReg rd, StringRef symbol);
MCInsts lw_rd_symbol(MCReg rd, StringRef symbol);
MCInsts ld_rd_symbol(MCReg rd, StringRef symbol);
MCInsts lbu_rd_symbol(MCReg rd, StringRef symbol);
MCInsts lhu_rd_symbol(MCReg rd, StringRef symbol);
MCInsts lwu_rd_symbol(MCReg rd, StringRef symbol);

MCInsts sb_rd_rt_symbol(MCReg rd, MCReg rt, StringRef symbol);
MCInsts sh_rd_rt_symbol(MCReg rd, MCReg rt, StringRef symbol);
MCInsts sw_rd_rt_symbol(MCReg rd, MCReg rt, StringRef symbol);
MCInsts sd_rd_rt_symbol(MCReg rd, MCReg rt, StringRef symbol);

MCInsts flw_rd_rt_symbol(MCReg rd, MCReg rt, StringRef symbol);
MCInsts fld_rd_rt_symbol(MCReg rd, MCReg rt, StringRef symbol);

MCInsts fsw_rd_rt_symbol(MCReg rd, MCReg rt, StringRef symbol);
MCInsts fsd_rd_rt_symbol(MCReg rd, MCReg rt, StringRef symbol);

MCInsts nop();

MCInsts li_rd_imm(MCReg rd, uint64_t immediate);

MCInsts sext_b_rd_rs(MCReg rd, MCReg rs);
MCInsts sext_h_rd_rs(MCReg rd, MCReg rs);
MCInsts sext_w_rd_rs(MCReg rd, MCReg rs);
MCInsts zext_b_rd_rs(MCReg rd, MCReg rs);
MCInsts zext_h_rd_rs(MCReg rd, MCReg rs);
MCInsts zext_w_rd_rs(MCReg rd, MCReg rs);

MCInsts call_symbol(); // call_offset
MCInsts tail_symbol(); // tail_offset

/// TODO: more

} // namespace mc

#endif