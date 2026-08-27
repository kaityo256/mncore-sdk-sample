#include "mncl/device/mncore2.h"

NO_MANGLING void add(DRAM const f32* x, DRAM const f32* y, DRAM f32* z) {
  L2BM f32* l2bm_x = __builtin_l2bm_malloc(2048);
  L2BM f32* l2bm_y = __builtin_l2bm_malloc(2048);
  L2BM f32* l2bm_z = __builtin_l2bm_malloc(2048);
  L1BM f32* l1bm_x = __builtin_l1bm_malloc(256);
  L1BM f32* l1bm_y = __builtin_l1bm_malloc(256);
  L1BM f32* l1bm_z = __builtin_l1bm_malloc(256);

  __builtin_dram_punicast_l2bm_0(x, l2bm_x, 128);
  __builtin_dram_punicast_l2bm_1(x + 4096, l2bm_x, 128);
  __builtin_dram_punicast_l2bm_0(y, l2bm_y, 128);
  __builtin_dram_punicast_l2bm_1(y + 4096, l2bm_y, 128);

  for (int i = 0; i < 8; ++i) {
    __builtin_l2bm_distribute(l2bm_x + i * 512, l1bm_x + i * 64);
    __builtin_l2bm_distribute(l2bm_y + i * 512, l1bm_y + i * 64);
  }

  f32x4x2 vx;
  f32x4x2 vy;
  __builtin_l1bm_distribute(l1bm_x, (void*)&vx);
  __builtin_l1bm_distribute(l1bm_y, (void*)&vy);
  f32x4x2 vz = vx + vy;
  __builtin_l1bm_gather((void*)&vz, l1bm_z);

  for (int i = 0; i < 8; ++i) {
    __builtin_l2bm_gather(l1bm_z + i * 64, l2bm_z + i * 512);
  }

  __builtin_dram_punicast_upload_0(l2bm_z, z, 2048);
  __builtin_dram_punicast_upload_1(l2bm_z, z + 4096, 2048);
}
