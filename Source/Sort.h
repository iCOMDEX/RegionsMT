#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef bool (*compareCallback)(const void *, const void *, void *);
typedef int8_t (*compareCallbackStable)(const void *, const void *, void *);

typedef struct { compareCallbackStable comp; } compareCallbackStableThunk;

uintptr_t *ordersStable(void *, size_t, size_t, compareCallbackStable, void *);
uintptr_t *ordersInvert(const uintptr_t *restrict, size_t);
bool createRanks(uintptr_t *restrict, uintptr_t, size_t, size_t);
uintptr_t *ranksStable(void *, size_t, size_t, compareCallbackStable, void *);
void quickSort(void *restrict, size_t, size_t, compareCallback, void *);
size_t binarySearch(const void *restrict, const void *restrict, size_t, size_t, compareCallbackStable, void *);
