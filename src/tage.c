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
tage_update_counter(uint8_t *counter, uint8_t n_bits, uint8_t direction) {
  uint8_t counter_max = (1 << n_bits) - 1;
  if (direction == TAKEN) {
    if ((*counter) < counter_max) {
      (*counter)++;
    }
  }
  else if (direction == NOTTAKEN) {
    if ((*counter) > 0) {
      (*counter)--;
    }
  }
}

void
tage_update_compressed_history(uint16_t *compressed_hist, uint8_t compressed_hist_bits, uint8_t predictor_num)
{
  uint16_t predictor_hist_bits = tagged_predictor_history_lengths[predictor_num];
  uint16_t split_point = predictor_hist_bits % compressed_hist_bits;
  LOG("\tsplit_point: %u", split_point);
  LOG("\tpredictor_hist_bits: %u", predictor_hist_bits);
  LOG("\tglobal_hist_bit: %d", global_history_register[predictor_hist_bits]);

  (*compressed_hist) = ((*compressed_hist) << 1) | global_history_register[0];
  LOG("\tcompressed_hist: %u", (*compressed_hist));
  (*compressed_hist) ^= (global_history_register[predictor_hist_bits] << split_point);
  LOG("\tcompressed_hist: %u", (*compressed_hist));
  (*compressed_hist) ^= ((*compressed_hist) >> compressed_hist_bits);
  LOG("\tcompressed_hist: %u", (*compressed_hist));
  (*compressed_hist) &= ((1 << compressed_hist_bits) - 1);
  LOG("\tcompressed_hist: %u\n", (*compressed_hist));
}

void
tage_train_predictor(uint32_t pc, uint8_t outcome)
{
  // TODO: Reset column of u every 256k iterations

  bool allocate_entry = (final_prediction != outcome) && (provider_component > 0);

  if (allocate_entry) {
    // Count the number of predictors that have their useless bit reset
    bool found_useless = false;
    uint8_t useless_count = 0;
    for (int i = provider_component - 1; i >= 0; i--) {
      uint16_t index = tagged_predictor_cached_indices[i];
      if (tagged_predictors[i].predictor_table[index].u == 0) {
        found_useless = true;
        useless_count++;
      }
    }

    if (found_useless) {
      // Find all predictors that have their useless bit reset
      uint8_t *useless_predictors = (uint8_t *)calloc(useless_count, sizeof(uint8_t));
      for (int i = provider_component - 1, j = 0; i >= 0; i--) {
        uint16_t index = tagged_predictor_cached_indices[i];
        if (tagged_predictors[i].predictor_table[index].u == 0) {
          useless_predictors[j] = i;
          j++;
        }
      }

      // Randomly pick one of the above found predictors and allocate a new entry
      int random = rand() % useless_count;
      uint8_t predictor_to_allocate = useless_predictors[random];
      uint16_t index = tagged_predictor_cached_indices[predictor_to_allocate];
      if (outcome == TAKEN) {
        tagged_predictors[predictor_to_allocate].predictor_table[index].pred = 4;
      }
      else {
        tagged_predictors[predictor_to_allocate].predictor_table[index].pred = 3;
      }
      tagged_predictors[predictor_to_allocate].predictor_table[index].tag = tagged_predictor_cached_tags[predictor_to_allocate];
      tagged_predictors[predictor_to_allocate].predictor_table[index].u = 0;
      free(useless_predictors);
    }
    else {
      // Decrement all useful counters
      for (int i = provider_component - 1; i >= 0; i--) {
        uint16_t index = tagged_predictor_cached_indices[i];
        tage_update_counter(&tagged_predictors[i].predictor_table[index].u, 2, NOTTAKEN);
      }
    }
  }

  // Update the table that gave the final prediction 
  if (provider_found) {
    uint16_t index = tagged_predictor_cached_indices[provider_component];
    tage_update_counter(&(tagged_predictors[provider_component].predictor_table[index].pred), 3, outcome);
  }
  else {
    uint16_t index = tage_compute_base_predictor_index(pc);
    tage_update_counter(&(base_predictor[index].pred), 2, outcome);
  }

  // This situation only arises if we obtained a hit in one of the tagged predictors
  // Hence, 'provider_component' is guaranteed to have a valid value
  if (final_prediction != alternate_prediction) {
    uint16_t index = tagged_predictor_cached_indices[provider_component];
    tage_update_counter(&(tagged_predictors[provider_component].predictor_table[index].u), 2, (final_prediction == outcome));
  }

  // Update global history
  for(int i = MAX_HISTORY_BITS - 1 ; i > 0 ; i--){
      global_history_register[i] = global_history_register[i - 1];
  }
  global_history_register[0] = outcome;

  // Update compressed histories
  for (int i = 0; i < NUM_TAGGED_PREDICTORS; i++) {
    uint16_t index = tagged_predictor_cached_indices[i];
    tage_update_compressed_history(&(tagged_predictors[i].index_csr), TAGGED_PREDICTOR_INDEX_BITS, i);
    LOG("\tpredictor: %d, index_csr: %u", i, tagged_predictors[i].index_csr);
    tage_update_compressed_history(&(tagged_predictors[i].tag_csr1), TAGGED_PREDICTOR_TAG_BITS, i);
    LOG("\tpredictor: %d, tag_csr1: %u", i, tagged_predictors[i].tag_csr1);
    tage_update_compressed_history(&(tagged_predictors[i].tag_csr2), (TAGGED_PREDICTOR_TAG_BITS - 1), i);
    LOG("\tpredictor: %d, tag_csr2: %u", i, tagged_predictors[i].tag_csr2);
  }

  pc_count++;
}