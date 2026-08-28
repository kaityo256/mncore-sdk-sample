#include "mncl/device/mncore2.h"

enum {
  VECTORS_PER_PE = 16,
};

NO_MANGLING void lifegame(DRAM const f32* input, DRAM f32* output) {
  // Each physical record contains eight neighbor vectors, the current value,
  // and the logical cell ID.  They stay together through scatter regardless
  // of the SDK's internal PE ordering.
  f32x4x2 values[VECTORS_PER_PE];
  __builtin_scatter(input, NUM_PE * sizeof(values), values, sizeof(values));

  f32x4x2 neighbors = values[0] + values[1];
  neighbors = neighbors + values[2];
  neighbors = neighbors + values[3];
  neighbors = neighbors + values[4];
  neighbors = neighbors + values[5];
  neighbors = neighbors + values[6];
  neighbors = neighbors + values[7];
  f32x4x2 current = values[8];
  f32x4x2 cell_ids = values[9];

  // Positive exactly for n == 3 or (n == 2 && current == 1).
  f32x4x2 six = {6.0f, 6.0f, 6.0f, 6.0f,
                 6.0f, 6.0f, 6.0f, 6.0f};
  f32x4x2 three_point_five = {3.5f, 3.5f, 3.5f, 3.5f,
                              3.5f, 3.5f, 3.5f, 3.5f};
  f32x4x2 eight_point_seven_five = {8.75f, 8.75f, 8.75f, 8.75f,
                                    8.75f, 8.75f, 8.75f, 8.75f};
  f32x4x2 one = {1.0f, 1.0f, 1.0f, 1.0f,
                 1.0f, 1.0f, 1.0f, 1.0f};
  f32x4x2 negative_one = {-1.0f, -1.0f, -1.0f, -1.0f,
                          -1.0f, -1.0f, -1.0f, -1.0f};
  f32x4x2 score = negative_one * (neighbors * neighbors);
  score = score + negative_one * (neighbors * current);
  score = score + six * neighbors;
  score = score + three_point_five * current;
  score = score + negative_one * eight_point_seven_five;

  f32x4x2 alive = __builtin_frelu0x8(score, one);
  f32x4x2 id_stride = {2048.0f, 2048.0f, 2048.0f, 2048.0f,
                       2048.0f, 2048.0f, 2048.0f, 2048.0f};
  f32x4x2 encoded = cell_ids + alive * id_stride;

  f32x4x2 results[VECTORS_PER_PE];
  for (int i = 0; i < VECTORS_PER_PE; ++i) {
    results[i] = __builtin_fzerox8();
  }
  results[9] = encoded;
  __builtin_gather(results, sizeof(results), output,
                   NUM_PE * sizeof(results));
}
