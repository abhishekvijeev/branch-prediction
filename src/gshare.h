#include <inttypes.h>
#include <stdbool.h>

// gshare data structures
uint32_t gshareGHR;         // global history register (GHR)
uint32_t gshareGHRMask;     // bit mask to extract 'ghistoryBits' least significant bits
                            // from GHR
uint8_t *gsharePHT;         // pattern history table (PHT) of 2-bit saturating counters indexed
                            // by the GHR value
uint32_t gsharePHTSize;     // size of PHT
uint32_t gsharePCMask;      // bit mask to extract 'ghistoryBits' least significant bits from PC
bool usePCHash;             // indicates whether the program counter and GHR contents must be
                            // XOR'd to compute PHT index

// gshare macro definitions
#define GSHARE_GHR_VALUE            ((gshareGHR) & (gshareGHRMask));
#define GSHARE_PC_LOWER_BITS(pc)    ((pc) & (gsharePCMask));

// gshare function declarations
void
gshare_init_predictor(bool use_pc_hash);

uint8_t
gshare_make_prediction(uint32_t pc);

void
gshare_train_predictor(uint32_t pc, uint8_t outcome);