#include "local.h"
#include "gshare.h"
#include "predictor.h"
#include "tournament.h"

#include <assert.h>
#include <stdio.h>

void
tournament_init_predictor(void)
{
  local_init_predictor();
  gshare_init_predictor(/*use_pc_hash =*/false);

  tournamentGHR = 0;
  tournamentGHRMask = (1UL << ghistoryBits) - 1;
  LOG("tournamentGHRMask = 0x%" PRIx32 "x", tournamentGHRMask);

  tournamentCPTSize = (1UL << ghistoryBits);
  LOG("tournamentCPTSize = %" PRIu32 "\n", tournamentCPTSize);

  tournamentCPT = (uint8_t *)calloc(tournamentCPTSize, sizeof(uint8_t));
  if (tournamentCPT == NULL) {
    perror("calloc");
    exit(-1);
  }
}

uint8_t
tournament_make_prediction(uint32_t pc)
{
  uint8_t prediction = NOTTAKEN;
  uint32_t cpt_index = TOURNAMENT_GHR_VALUE;
  assert((cpt_index >= 0) && (cpt_index < tournamentCPTSize));
  uint8_t choice = tournamentCPT[cpt_index];

  assert(choice == WG
    || choice == SG
    || choice == WL
    || choice == SL);

  if (choice == WG || choice == SG) {
    prediction = gshare_make_prediction(pc);
  }
  else if (choice == WL || choice == SL) {
    prediction = local_make_prediction(pc);
  }

  return prediction;
}

void
tournament_train_predictor(uint32_t pc, uint8_t outcome)
{
  uint8_t global_prediction = gshare_make_prediction(pc);
  uint8_t local_prediction = local_make_prediction(pc);
  uint32_t cpt_index = tournamentGHR & tournamentGHRMask;
  assert((cpt_index >= 0) && (cpt_index < tournamentCPTSize));
  uint8_t current_choice = tournamentCPT[cpt_index];
  uint8_t new_choice;

  assert(current_choice == WG
    || current_choice == SG
    || current_choice == WL
    || current_choice == SL);

  // Shift tournament GHR
  tournamentGHR = tournamentGHR << 1;
  tournamentGHR = tournamentGHR | outcome;

  // Update the choice predictor as follows:
  // 1) If both global and local predictors are correct or wrong, do nothing
  // 2) If only one is correct, move the choice predictor's state machine in
  // the direction of the correct predictor
  if ((global_prediction == outcome) && (local_prediction != outcome)) {
    switch (current_choice) {
      case WG:
        new_choice = SG;
        break;

      case SG:
        new_choice = SG;
        break;

      case WL:
        new_choice = WG;
        break;

      case SL:
        new_choice = WL;
        break;

      default:
        fprintf(stderr, "Current state of choice predictor is none of {SG, WG, WL, SL}\n");
        exit(-1);
    }
  }
  else if ((local_prediction == outcome) && (global_prediction != outcome)) {
    switch (current_choice) {
      case WG:
        new_choice = WL;
        break;

      case SG:
        new_choice = WG;
        break;

      case WL:
        new_choice = SL;
        break;

      case SL:
        new_choice = SL;
        break;

      default:
        fprintf(stderr, "Current state of choice predictor is none of {SG, WG, WL, SL}\n");
        exit(-1);
    }
  }
  else {
    new_choice = current_choice;
  }

  assert((new_choice == SG)
    || (new_choice == WG)
    || (new_choice == WL)
    || (new_choice == SL));

  tournamentCPT[cpt_index] = new_choice;
  LOG("new choice = %" PRIu8 "\n", new_choice);

  // Train the local and global predictors
  local_train_predictor(pc, outcome);
  gshare_train_predictor(pc, outcome);
}