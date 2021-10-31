#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "predictor.h"
#include "tage.h"

uint8_t tagged_predictor_history_lengths[NUM_TAGGED_PREDICTORS] = {64, 32, 16, 8, 4};
uint32_t tagged_predictor_table_sizes[NUM_TAGGED_PREDICTORS] = {TAGGED_PREDICTOR_TABLE_SIZE,
                                                                TAGGED_PREDICTOR_TABLE_SIZE,
                                                                TAGGED_PREDICTOR_TABLE_SIZE,
                                                                TAGGED_PREDICTOR_TABLE_SIZE,
                                                                TAGGED_PREDICTOR_TABLE_SIZE};

uint8_t global_history_register[MAX_HISTORY_BITS];

void
tage_init_predictor(void)
{
  memset(global_history_register, 0, sizeof(uint8_t) * MAX_HISTORY_BITS);

  // initialize base predictor
  for (int i = 0; i < BASE_PREDICTOR_TABLE_SIZE; i++) {
    base_predictor[i].pred = 1;
  }

  // initialize tagged predictors
  for (int i = 0; i < NUM_TAGGED_PREDICTORS; i++) {
    tagged_predictors[i].index_csr = 0;
    tagged_predictors[i].tag_csr1 = 0;
    tagged_predictors[i].tag_csr2 = 0;
    tagged_predictors[i].predictor_table_size = tagged_predictor_table_sizes[i];
    tagged_predictors[i].predictor_table = calloc(tagged_predictors[i].predictor_table_size, sizeof(tagged_predictor_entry_t));

    if (tagged_predictors[i].predictor_table == NULL) {
      perror("calloc");
      exit(-1);
    }
  }
}

uint8_t
tage_make_prediction(uint32_t pc)
{
  uint8_t prediction = NOTTAKEN;
  return prediction;
}

void
tage_train_predictor(uint32_t pc, uint8_t outcome)
{

}