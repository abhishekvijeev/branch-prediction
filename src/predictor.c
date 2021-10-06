//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include "predictor.h"

#define LOGGER false

#define LOG(fmt, ...)                                 \
  do {                                                \
  if (LOGGER) {                                       \
  fprintf(stderr, "%s():%d -> " fmt "\n",             \
          __func__, __LINE__, __VA_ARGS__);           \
    }                                                 \
  } while (0)

//
// Student Information
//
const char *studentName = "Abhishek Vijeev";
const char *studentID   = "A59003133";
const char *email       = "avijeev@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

// gshare data structures
uint32_t gshareGHR;         // global history register (GHR)
uint32_t gshareGHRMask;     // bit mask to extract 'ghistoryBits' least significant bits
                            // from GHR
uint8_t *gsharePHT;         // pattern history table (PHT) of 2-bit saturating counters indexed
                            // by the GHR value
uint32_t gsharePHTSize;     // size of PHT
uint32_t gsharePCMask;      // bit mask to extract 'ghistoryBits' least significant bits from PC

#define GSHARE_GHR_VALUE            ((gshareGHR) & (gshareGHRMask));
#define GSHARE_PC_LOWER_BITS(pc)    ((pc) & (gsharePCMask));

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize Branch Predictor Data Structures
void
init_predictor()
{
  switch (bpType) {
    case STATIC:
      break;

    case GSHARE:
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
      break;

    case TOURNAMENT:
      break;

    case CUSTOM:
      break;

    default:
      break;
  }

}

// Make a prediction using the gshare predictor
uint8_t
gshare_make_prediction(uint32_t pc)
{
  uint32_t ghr = GSHARE_GHR_VALUE;
  uint32_t pc_lower_bits = GSHARE_PC_LOWER_BITS(pc);
  uint32_t pht_index = ghr ^ pc_lower_bits;
  assert((pht_index >= 0) && (pht_index < gsharePHTSize));
  uint8_t current_state, prediction;

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

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  // If there is not a compatable bpType then return NOTTAKEN
  uint8_t prediction = NOTTAKEN;

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      prediction = TAKEN;
      break;

    case GSHARE:
      prediction = gshare_make_prediction(pc);
      break;

    case TOURNAMENT:
      break;

    case CUSTOM:
      break;

    default:
      break;
  }

  assert((prediction == TAKEN) || (prediction == NOTTAKEN));
  return prediction;
}

// Train the gshare predictor
void
gshare_train_predictor(uint32_t pc, uint8_t outcome)
{
  uint32_t ghr = GSHARE_GHR_VALUE;
  uint32_t pc_lower_bits = GSHARE_PC_LOWER_BITS(pc);
  uint32_t pht_index = ghr ^ pc_lower_bits;
  assert((pht_index >= 0) && (pht_index < gsharePHTSize));
  uint8_t current_state = gsharePHT[pht_index];
  uint8_t updated_state;

  LOG("ghr = %" PRIu32, ghr);
  LOG("pc = 0x%" PRIx32, pc);
  LOG("pc_lower_bits = %" PRIu32, pc_lower_bits);
  LOG("pht_index = %" PRIu32, pht_index);
  LOG("current state = %" PRIu8, current_state);
  LOG("outcome = %" PRIu8, outcome);

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

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  switch (bpType) {
    case STATIC:
      break;

    case GSHARE:
      gshare_train_predictor(pc, outcome);
      break;

    case TOURNAMENT:
      break;

    case CUSTOM:
      break;

    default:
      break;
  }
}
