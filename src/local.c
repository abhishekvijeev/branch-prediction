#include <assert.h>
#include <stdio.h>

#include "local.h"
#include "predictor.h"

void
local_init_predictor(void)
{
  localPCMask = (1UL << pcIndexBits) - 1;
  LOG("localPCMask = 0x%" PRIx32, localPCMask);

  localBHRMask = (1UL << lhistoryBits) - 1;
  LOG("localBHRMask = 0x%" PRIx32, localBHRMask);

  localBHRTSize = (1UL << pcIndexBits);
  LOG("localBHRTSize = %" PRIu32, localBHRTSize);

  localBHRT = (uint32_t *)calloc(localBHRTSize, sizeof(uint32_t));
  if (localBHRT == NULL) {
    perror("localBHRT calloc");
    exit(-1);
  }

  localPHTSize = (1UL << lhistoryBits);
  LOG("localPHTSize = %" PRIu32 "\n", localPHTSize);

  localPHT = (uint8_t *)calloc(localPHTSize, sizeof(uint8_t));
  if (localPHT == NULL) {
    perror("localPHT calloc");
    exit(-1);
  }
}

uint8_t
local_make_prediction(uint32_t pc)
{
  uint32_t bhrt_index, branch_history_register, pt_index;
  uint8_t current_predictor_state, prediction;

  bhrt_index = LOCAL_PC_LOWER_BITS(pc);
  branch_history_register = localBHRT[bhrt_index];
  pt_index = LOCAL_BHR_VALUE(branch_history_register);
  current_predictor_state = localPHT[pt_index];

  LOG("pc = 0x%" PRIx32, pc);
  LOG("bhrt_index = %" PRIu32, bhrt_index);
  LOG("branch_history_register = %" PRIu32, branch_history_register);
  LOG("pt_index = %" PRIu32, pt_index);
  LOG("current_predictor_state = %" PRIu8, current_predictor_state);

  if ((current_predictor_state == SN) || (current_predictor_state == WN)) {
    prediction = NOTTAKEN;
  }
  else if ((current_predictor_state == ST) || (current_predictor_state == WT)) {
    prediction = TAKEN;
  }
  else {
    fprintf(stderr, "Current state of counter is none of {SN, WN, WT, ST}\n");
    exit(-1);
  }

  assert((prediction == TAKEN) || (prediction == NOTTAKEN));
  LOG("prediction = %" PRIu8 "\n", prediction);
  return prediction;
}

void
local_train_predictor(uint32_t pc, uint8_t outcome)
{
  uint32_t bhrt_index, branch_history_register, pt_index;
  uint8_t current_predictor_state, new_predictor_state;

  bhrt_index = LOCAL_PC_LOWER_BITS(pc);
  branch_history_register = localBHRT[bhrt_index];
  pt_index = LOCAL_BHR_VALUE(branch_history_register);
  current_predictor_state = localPHT[pt_index];

  LOG("pc = 0x%" PRIx32, pc);
  LOG("bhrt_index = %" PRIu32, bhrt_index);
  LOG("branch_history_register = %" PRIu32, branch_history_register);
  LOG("pt_index = %" PRIu32, pt_index);
  LOG("current_predictor_state = %" PRIu8, current_predictor_state);

  // Update BHR corresponding to pc
  localBHRT[bhrt_index] = localBHRT[bhrt_index] << 1;
  localBHRT[bhrt_index] = localBHRT[bhrt_index] | outcome;

  // Update saturating counter corresponding to history pattern
  if (outcome == NOTTAKEN) {
    switch (current_predictor_state) {
      case SN:
        localPHT[pt_index] = SN;
        break;

      case WN:
        localPHT[pt_index] = SN;
        break;

      case WT:
        localPHT[pt_index] = WN;
        break;

      case ST:
        localPHT[pt_index] = WT;
        break;

      default:
        fprintf(stderr, "Current state of counter is none of {SN, WN, WT, ST}\n");
        exit(-1);
    }
  }
  else if (outcome == TAKEN) {
    switch (current_predictor_state) {
      case SN:
        localPHT[pt_index] = WN;
        break;

      case WN:
        localPHT[pt_index] = WT;
        break;

      case WT:
        localPHT[pt_index] = ST;
        break;

      case ST:
        localPHT[pt_index] = ST;
        break;

      default:
        fprintf(stderr, "Current state of counter is none of {SN, WN, WT, ST}\n");
        exit(-1);
    }
  }

  new_predictor_state = localPHT[pt_index];
  assert((new_predictor_state == SN)
    || (new_predictor_state == WN)
    || (new_predictor_state == WT)
    || (new_predictor_state == ST));
  LOG("new_predictor_state = %" PRIu8 "\n", new_predictor_state);
}