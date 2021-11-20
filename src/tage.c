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
uint16_t tagged_predictor_cached_indices[NUM_TAGGED_PREDICTORS];
uint8_t tagged_predictor_cached_tags[NUM_TAGGED_PREDICTORS];

uint8_t global_history_register[MAX_HISTORY_BITS];

uint8_t provider_component;
uint8_t provider_prediction;
uint8_t alternate_component;
uint8_t alternate_prediction;
uint8_t final_prediction;
bool provider_found;
bool alternate_found;

uint64_t pc_count = 0;

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

uint16_t
tage_compute_tagged_predictor_index(uint8_t predictor_num, uint32_t pc)
{
  uint16_t index_csr = tagged_predictors[predictor_num].index_csr;
  uint16_t index_mask = ((1 << TAGGED_PREDICTOR_INDEX_BITS) - 1);

  uint16_t index = (pc ^ (pc >> (TAGGED_PREDICTOR_INDEX_BITS - (NUM_TAGGED_PREDICTORS - predictor_num - 1)))^ index_csr) & index_mask;

  return index;
}

uint8_t
tage_compute_tagged_predictor_tag(uint8_t predictor_num, uint32_t pc)
{
  uint16_t tag_csr1 = tagged_predictors[predictor_num].tag_csr1;
  uint16_t tag_csr2 = tagged_predictors[predictor_num].tag_csr2;
  uint16_t tag_mask = ((1 << TAGGED_PREDICTOR_TAG_BITS) - 1);

  uint8_t tag = (pc ^ tag_csr1 ^ (tag_csr2 << 1)) & tag_mask;

  return tag;
}

uint16_t
tage_compute_base_predictor_index(uint32_t pc)
{
  uint16_t index_mask = ((1 << BASE_PREDICTOR_INDEX_BITS) - 1);
  uint16_t index = (pc & index_mask);

  return index;
}

uint8_t
tage_make_prediction(uint32_t pc)
{
  provider_found = false;
  alternate_found = false;
  provider_component = NUM_TAGGED_PREDICTORS;
  provider_prediction = NOTTAKEN;
  alternate_component = NUM_TAGGED_PREDICTORS;
  alternate_prediction = NOTTAKEN;
  final_prediction = NOTTAKEN;

  LOG("pc: %u, pc_count: %lu", pc, pc_count);

  // For each tagged predictor, cache the index and tag
  for (int i = 0; i < NUM_TAGGED_PREDICTORS; i++) {
    tagged_predictor_cached_indices[i] = tage_compute_tagged_predictor_index(i, pc);
    tagged_predictor_cached_tags[i] = tage_compute_tagged_predictor_tag(i, pc);
    LOG("\tpredictor: %d, index: %u, tag: %u",
      i, tagged_predictor_cached_indices[i], tagged_predictor_cached_tags[i]);
  }

  // Check if there's a tag match - this will be the provider component
  for (int i = 0; i < NUM_TAGGED_PREDICTORS; i++) {
    uint16_t table_index = tagged_predictor_cached_indices[i];
    if (tagged_predictors[i].predictor_table[table_index].tag == tagged_predictor_cached_tags[i]) {
      provider_found = true;
      provider_component = i;
      provider_prediction = (tagged_predictors[i].predictor_table[table_index].pred <= 3) ? NOTTAKEN : TAKEN;
      break;
    }
  }

  if (provider_found) {

    // Find the alternate component
    for (int i = provider_component + 1; i < NUM_TAGGED_PREDICTORS; i++) {
      uint16_t table_index = tagged_predictor_cached_indices[i];
      if (tagged_predictors[i].predictor_table[table_index].tag == tagged_predictor_cached_tags[i]) {
        alternate_found = true;
        alternate_component = i;
        break;
      }
    }

    if (alternate_found) {
      uint16_t table_index = tagged_predictor_cached_indices[alternate_component];
      alternate_prediction = (tagged_predictors[alternate_component].predictor_table[table_index].pred <= 3) ? NOTTAKEN : TAKEN;
    }
    else {
      uint16_t base_predictor_index = tage_compute_base_predictor_index(pc);
      alternate_prediction = (base_predictor[base_predictor_index].pred <= 1) ? NOTTAKEN : TAKEN;
    }

    final_prediction = provider_prediction;
  }
  else {
    // None of the tagged predictors hit
    uint16_t base_predictor_index = tage_compute_base_predictor_index(pc);
    alternate_prediction = (base_predictor[base_predictor_index].pred <= 1) ? NOTTAKEN : TAKEN;
    final_prediction = alternate_prediction;
  }

  return final_prediction;
}

void
tage_train_predictor(uint32_t pc, uint8_t outcome)
{
  pc_count++;
}