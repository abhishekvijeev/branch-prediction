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
#include <stdio.h>

#include "gshare.h"
#include "predictor.h"

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
      gshare_init_predictor();
      break;

    case TOURNAMENT:
      break;

    case CUSTOM:
      break;

    default:
      break;
  }

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
