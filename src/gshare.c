#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "gshare.h"
#include "predictor.h"

// Initialize gshare data structures
void
gshare_init_predictor(bool use_pc_hash)
{
  usePCHash = use_pc_hash;
  gshareGHR = 0;
  gshareGHRMask = (1UL << ghistoryBits) - 1;
  gsharePCMask = (1UL << ghistoryBits) - 1;
  LOG("gshareGHRMask = 0x%" PRIx32 "x", gshareGHRMask);
  LOG("gsharePCMask = 0x%" PRIx32 "x", gsharePCMask);

  gsharePHTSize = (1UL << ghistoryBits);
  LOG("gsharePHTSize = %" PRIu32 "\n", gsharePHTSize);

  gsharePHT = (uint8_t *)calloc(gsharePHTSize, sizeof(uint8_t));
  if (gsharePHT == NULL) {
    perror("calloc");
    exit(-1);
  }
  memset(gsharePHT, WN, gsharePHTSize);
}

// Make a prediction using the gshare predictor
uint8_t
gshare_make_prediction(uint32_t pc)
{
  uint32_t ghr = GSHARE_GHR_VALUE;
  uint32_t pc_lower_bits = GSHARE_PC_LOWER_BITS(pc);
  uint32_t pht_index = ghr;
  uint8_t current_state, prediction;

  if (usePCHash) {
    pht_index = ghr ^ pc_lower_bits;
  }
  assert((pht_index >= 0) && (pht_index < gsharePHTSize));

  LOG("ghr = %" PRIu32, ghr);
  LOG("pc = 0x%" PRIx32, pc);
  LOG("pc_lower_bits = %" PRIu32, pc_lower_bits);
  LOG("pht_index = %" PRIu32, pht_index);
  LOG("prediction = %" PRIu8 "\n", gsharePHT[pht_index]);
  current_state = gsharePHT[pht_index];

  if ((current_state == SN) || (current_state == WN)) {
    prediction = NOTTAKEN;
  }
  else if ((current_state == ST) || (current_state == WT)) {
    prediction = TAKEN;
  }
  else {
    fprintf(stderr, "Current state of counter is none of {SN, WN, WT, ST}\n");
    exit(-1);
  }

  return prediction;
}

// Train the gshare predictor
void
gshare_train_predictor(uint32_t pc, uint8_t outcome)
{
  uint32_t ghr = GSHARE_GHR_VALUE;
  uint32_t pc_lower_bits = GSHARE_PC_LOWER_BITS(pc);
  uint32_t pht_index = ghr;
  uint8_t current_state, updated_state;

  if (usePCHash) {
    pht_index = ghr ^ pc_lower_bits;
  }
  assert((pht_index >= 0) && (pht_index < gsharePHTSize));

  current_state = gsharePHT[pht_index];

  LOG("ghr = %" PRIu32, ghr);
  LOG("pc = 0x%" PRIx32, pc);
  LOG("pc_lower_bits = %" PRIu32, pc_lower_bits);
  LOG("pht_index = %" PRIu32, pht_index);
  LOG("current state = %" PRIu8, current_state);
  LOG("outcome = %" PRIu8, outcome);

  // Update global history register
  gshareGHR = gshareGHR << 1;
  gshareGHR = gshareGHR | outcome;

  // Update pattern history table
  if (outcome == NOTTAKEN) {
    switch (current_state) {
      case SN:
        gsharePHT[pht_index] = SN;
        break;

      case WN:
        gsharePHT[pht_index] = SN;
        break;

      case WT:
        gsharePHT[pht_index] = WN;
        break;

      case ST:
        gsharePHT[pht_index] = WT;
        break;

      default:
        fprintf(stderr, "Current state of counter is none of {SN, WN, WT, ST}\n");
        exit(-1);
    }
  }
  else if (outcome == TAKEN) {
    switch (current_state) {
      case SN:
        gsharePHT[pht_index] = WN;
        break;

      case WN:
        gsharePHT[pht_index] = WT;
        break;

      case WT:
        gsharePHT[pht_index] = ST;
        break;

      case ST:
        gsharePHT[pht_index] = ST;
        break;

      default:
        fprintf(stderr, "Current state of counter is none of {SN, WN, WT, ST}\n");
        exit(-1);
    }
  }

  updated_state = gsharePHT[pht_index];
  assert((updated_state == SN)
    || (updated_state == WN)
    || (updated_state == WT)
    || (updated_state == ST));
  LOG("updated state = %" PRIu8 "\n", gsharePHT[pht_index]);
}