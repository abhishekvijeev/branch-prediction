#include <inttypes.h>

// State definitions for the choice predictor's 2 bit saturating counters
#define WG  0   // weakly global
#define SG  1   // strongly global
#define WL  2   // weakly local
#define SL  3   // strongly local

// tournament predictor data structures
uint32_t tournamentGHR;         // global history register (GHR)
uint32_t tournamentGHRMask;     // bit mask to extract 'ghistoryBits' least significant bits
                                // from GHR
uint8_t *tournamentCPT;         // choice prediction table (CPT) of 2-bit saturating counters,
                                // indexed by the history register value obtained from
                                // 'tournamentGHR', that tells us which predictor to use for
                                // this branch instruction
uint32_t tournamentCPTSize;     // size of CPT

// tournament predictor macro definitions
#define TOURNAMENT_GHR_VALUE    ((tournamentGHR) & (tournamentGHRMask));

// tournament predictor function declarations
void
tournament_init_predictor(void);

uint8_t
tournament_make_prediction(uint32_t pc);

void
tournament_train_predictor(uint32_t pc, uint8_t outcome);