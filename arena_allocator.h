
#ifndef ARENA_ALLOCATOR_H
#define ARENA_ALLOCATOR_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef i8 b8;
typedef i32 b32;

#define KiB(n) ((u64)(n) << 10)
#define MiB(n) ((u64)(n) << 20)
#define GiB(n) ((u64)(n) << 30)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define ALIGN_UP_POW2(n, p) (((u64)(n) + ((u64)(p) - 1)) & (~((u64)(p) - 1)))

typedef struct MemArena {
    u64 reserveSize;
    u64 commitSize;

    u64 pos;
    u64 commitPos;
} MemArena;

#define ARENA_BASE_POS (sizeof(MemArena))
#define ARENA_ALIGN (sizeof(void*))

#define PUSH_STRUCT(arena, T) (T*)arenaPush((arena), sizeof(T), false)
#define PUSH_STRUCT_NZ(arena, T) (T*)arenaPush((arena), sizeof(T), true)
#define PUSH_ARRAY(arena, T, n) (T*)arenaPush((arena), sizeof(T) * (n), false)
#define PUSH_ARRAY_NZ(arena, T, n) (T*)arenaPush((arena), sizeof(T) * (n), true)

u32 platGetPagesize(void);

void* platMemReserve(u64 size);
b32 platMemCommit(void* ptr, u64 size);
b32 platMemDecomit(void* ptr, u64 size);
b32 platMemRelease(void* ptr, u64 size);

MemArena* arenaCreate(u64 reserveSize, u64 commitSize);
void arenaDestroy(MemArena* arena);
void* arenaPush(MemArena* arena, u64 size, b32 nonZero);
void arenaPop(MemArena* arena, u64 size);
void arenaPopTo(MemArena* arena, u64 pos);
void arenaClear(MemArena* arena);

#endif
