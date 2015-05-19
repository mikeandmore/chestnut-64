// -*- c++ -*-
#include "uslab.h"

namespace kernel {
int BaseBitmapSlab::AllocFromBitmapArray(int n, u64 *bitmap_array) {
	//n represent the NBitsmap's N
	int idx = 64;
	int row = 0;
	for (int i = 0; i < n; i++) {
		int temp_idx = __builtin_ctzl(bitmap_array[i]);
		if (temp_idx < idx) {
			idx = temp_idx;
			row = i;
		}
	}
	bitmap_array[row] ^= (1 << idx);
	return idx + 64 * row;
}

void BaseBitmapSlab::FreeToBitmapArray(int n, u64 *bitmap_array, int obj_idx) {
	int row = obj_idx / 64;
	int idx = obj_idx % 64;
	bitmap_array[row] ^= (1 << idx);
}

}
