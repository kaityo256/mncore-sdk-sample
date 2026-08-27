#include "mncl/device/mncore2.h"

// z = x + y
//
// Each PE adds one f32x4x2 vector. Data is moved through the MN-Core 2
// hierarchy: DRAM -> L2BM -> L1BM -> PE, then back in reverse order.
NO_MANGLING void add(DRAM const f32* x,
                     DRAM const f32* y,
                     DRAM f32* z) {
  L2BM f32* l2bm_x = __builtin_l2bm_malloc(2048);
  L2BM f32* l2bm_y = __builtin_l2bm_malloc(2048);
  L2BM f32* l2bm_z = __builtin_l2bm_malloc(2048);

  L1BM f32* l1bm_x = __builtin_l1bm_malloc(256);
  L1BM f32* l1bm_y = __builtin_l1bm_malloc(256);
  L1BM f32* l1bm_z = __builtin_l1bm_malloc(256);

  // DRAM -> L2BM
  __builtin_dram_punicast_l2bm_0(x, l2bm_x, 128);
  __builtin_dram_punicast_l2bm_1(x + 4096, l2bm_x, 128);
  __builtin_dram_punicast_l2bm_0(y, l2bm_y, 128);
  __builtin_dram_punicast_l2bm_1(y + 4096, l2bm_y, 128);

  // L2BM -> L1BM
  for (int i = 0; i < 8; ++i) {
    __builtin_l2bm_distribute(l2bm_x + i * 512, l1bm_x + i * 64);
    __builtin_l2bm_distribute(l2bm_y + i * 512, l1bm_y + i * 64);
  }

  // L1BM -> PE
  f32x4x2 vx;
  f32x4x2 vy;
  __builtin_l1bm_distribute(l1bm_x, (void*)&vx);
  __builtin_l1bm_distribute(l1bm_y, (void*)&vy);

  // Exactly one vector addition.
  f32x4x2 vz = vx + vy;

  // PE -> L1BM -> L2BM
  __builtin_l1bm_gather((void*)&vz, l1bm_z);
  for (int i = 0; i < 8; ++i) {
    __builtin_l2bm_gather(l1bm_z + i * 64, l2bm_z + i * 512);
  }

  // L2BM -> DRAM
  __builtin_dram_punicast_upload_0(l2bm_z, z, 2048);
  __builtin_dram_punicast_upload_1(l2bm_z, z + 4096, 2048);
}
