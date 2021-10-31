#include <inttypes.h>

#define MAX_HISTORY_BITS 128
#define TAGGED_PREDICTOR_INDEX_BITS 9
#define TAGGED_PREDICTOR_TABLE_SIZE (1 << (TAGGED_PREDICTOR_INDEX_BITS))
#define TAGGED_PREDICTOR_TAG_BITS 8
#define BASE_PREDICTOR_INDEX_BITS 12
#define BASE_PREDICTOR_TABLE_SIZE (1 << (BASE_PREDICTOR_INDEX_BITS))
#define NUM_TAGGED_PREDICTORS 5

typedef struct base_predictor_entry {
  uint8_t pred;                       // 2-bit saturation counter for branch prediction
} base_predictor_entry_t;

typedef struct tagged_predictor_entry {
  uint8_t pred;                       // 3-bit saturation counter for branch prediction
  uint8_t tag;                        // 8-bit tag
  uint8_t u;                          // 2-bit saturation counter for usefulness
} tagged_predictor_entry_t;

typedef struct tagged_predictor {
  uint16_t index_csr;                           // 9-bit circular shift register for index
  uint16_t tag_csr1;                            // 8-bit circular shift register for tag
  uint16_t tag_csr2;                            // 7-bit circular shift register for tag
  uint32_t predictor_table_size;
  tagged_predictor_entry_t *predictor_table;
} tagged_predictor_t;

extern uint8_t tagged_predictor_history_lengths[NUM_TAGGED_PREDICTORS];
extern uint32_t tagged_predictor_table_sizes[NUM_TAGGED_PREDICTORS];

// base predictor - table of 3-bit saturation counters indexed by
// the program counter. Counter values 0 to 3 represent not taken
// with 0 being strongly not taken and values 4 to 7 represent
// taken with 7 being strongly taken
base_predictor_entry_t base_predictor[BASE_PREDICTOR_TABLE_SIZE];

// partially tagged predictor components
tagged_predictor_t tagged_predictors[NUM_TAGGED_PREDICTORS];

// TAGE function declarations
void
tage_init_predictor(void);

uint8_t
tage_make_prediction(uint32_t pc);

void
tage_train_predictor(uint32_t pc, uint8_t outcome);





