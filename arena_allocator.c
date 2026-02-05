
#include "arena_allocator.h"

 MemArena* arenaCreate(u64 reserveSize, u64 commitSize) {
    u32 pageSize = platGetPagesize();
    reserveSize = ALIGN_UP_POW2(reserveSize, pageSize);
    commitSize = ALIGN_UP_POW2(commitSize, pageSize);

    MemArena* arena = (MemArena*)platMemReserve(reserveSize);
    if (!platMemCommit(arena, commitSize)) {
        return NULL;
    }
    arena->reserveSize = reserveSize;
    arena->commitSize = commitSize;
    arena->pos = ARENA_BASE_POS;
    arena->commitPos = commitSize;

    return arena;
}

void arenaDestroy(MemArena* arena) {
    platMemRelease(arena, arena->reserveSize);
    arena = NULL;
}

void* arenaPush(MemArena* arena, u64 size, b32 nonZero) {
    u64 posAligned = ALIGN_UP_POW2(arena->pos, ARENA_ALIGN);
    u64 newPos = posAligned + size;

    if (newPos > arena->reserveSize) return NULL;
    if (newPos > arena->commitPos) {
        u64 newCommitPos = newPos;
        newCommitPos += arena->commitSize - 1;
        newCommitPos -= newCommitPos % arena->commitSize;
        newCommitPos = MIN(newCommitPos, arena->reserveSize);

        u8* mem = (u8*)arena + arena->commitPos;
        u64 commitSize = newCommitPos - arena->commitPos;
        if (!platMemCommit(mem, commitSize)) {
            return NULL;
        }
        arena->commitPos = newCommitPos;
    }
    arena->pos = newPos;

    u8* out = (u8*)arena + posAligned;
    if (!nonZero) memset(out, 0, size);

    return out;
}

void arenaPop(MemArena* arena, u64 size) {
    size = MIN(size, arena->pos - ARENA_BASE_POS);
    arena->pos -= size;
}

void arenaPopTo(MemArena* arena, u64 pos) {
    u64 size = pos < arena->pos ? arena->pos - pos : 0;
    arenaPop(arena, size);
}

void arenaClear(MemArena* arena) {
    arenaPopTo(arena, ARENA_BASE_POS);
}

#ifdef _WIN32

#include <windows.h>

u32 platGetPagesize(void) {
    SYSTEM_INFO sysInfo = {0};
    GetSystemInfo(&sysInfo);
    return sysInfo.dwPageSize;
}

void* platMemReserve(u64 size) {
    return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);
}

b32 platMemCommit(void* ptr, u64 size) {
    void* ret = VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
    return ret != NULL;
}

b32 platMemDecomit(void* ptr, u64 size) {
    return VirtualFree(ptr, size, MEM_DECOMMIT);
}

b32 platMemRelease(void* ptr, u64 size) {
    return VirtualFree(ptr, size, MEM_RELEASE);
}


#elif __linux__
#include <unistd.h>
#include <sys/mman.h>

u32 platGetPagesize(void) {
    return (u32)sysconf(_SC_PAGESIZE);
}

void* platMemReserve(u64 size) {
    void* ptr = mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return (ptr == MAP_FAILED) ? NULL : ptr;
}

b32 platMemCommit(void* ptr, u64 size) {
    return mprotect(ptr, size, PROT_READ | PROT_WRITE) == 0;
}

b32 platMemDecomit(void* ptr, u64 size) {
    return mprotect(ptr, size, PROT_NONE) == 0;
}

b32 platMemRelease(void* ptr, u64 size) {
    return munmap(ptr, size) == 0;
}

#endif

