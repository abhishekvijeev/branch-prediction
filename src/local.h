#include <inttypes.h>

// local predictor data structures
uint32_t *localBHRT;      // branch history register table (BHRT), which is indexed by
                          // branch instruction address (i.e. the program counter)
uint32_t localBHRTSize;   // size of BHRT
uint8_t *localPHT;        // pattern history table (PHT) of 2-bit saturating counters
                          // indexed by the history register value obtained from 'localBHRT'
uint32_t localPHTSize;    // size of PHT
uint32_t localPCMask;     // bit mask to extract 'pcIndexBits' least significant bits from PC
uint32_t localBHRMask;    // bit mast to extract 'lhistoryBits' least significant bits from PC

// local predictor macro definitions
#define LOCAL_PC_LOWER_BITS(pc)   ((pc) & (localPCMask));
#define LOCAL_BHR_VALUE(bhr)      ((bhr) & (localBHRMask));

// local predictor function declarations
void
local_init_predictor(void);

uint8_t
local_make_prediction(uint32_t pc);

void
local_train_predictor(uint32_t pc, uint8_t outcome);