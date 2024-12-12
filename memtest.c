#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#ifndef MAGIC_NOT_ZERO
#define MAGIC_NOT_ZERO 0
#endif

#if !defined(WARMUP_INITMAGIC) && !defined(WARMUP_TESTFUNC)
#define WARMUP_INITMAGIC 1
#define WARMUP_TESTFUNC 1
#endif

#ifndef WARMUP_INITMAGIC
#define WARMUP_INITMAGIC 0
#endif

#ifndef WARMUP_TESTFUNC
#define WARMUP_TESTFUNC 0
#endif

#if !WARMUP_INITMAGIC && !WARMUP_TESTFUNC
#error "must enable at least one of WARMUP_INITMAGIC and WARMUP_TESTFUNC"
#endif

#if MAGIC_NOT_ZERO
#define MAGIC 8675728858075378228ull
#else
#define MAGIC 0ull
#endif

#ifndef TEST_GB
#define TEST_GB 8 // 8GB
#endif


struct test_function
{
    const char *name;
    void (*func)(void *, size_t);
};

struct test_result
{
    double best_rate;
    double avg_rate;
    double worst_rate;
};

void test_copy(void *memory, size_t size)
{
    size_t count = size / sizeof(uint64_t);
    size_t half = count / 2;
    uint64_t *load_ptr = (uint64_t *)memory;
    uint64_t *store_ptr = load_ptr + half;

    for (size_t i = 0; i < half; i++)
    {
        store_ptr[i] = load_ptr[i];
    }
}

void test_load(void *memory, size_t size)
{
    size_t count = size / sizeof(uint64_t);
    uint64_t *load_ptr = (uint64_t *)memory;
    for (size_t i = 0; i < count; i++)
    {
        if (load_ptr[i] != MAGIC)
        {
            fprintf(stderr, "unexpected value\n");
        }
    }
}

void test_store(void *memory, size_t size)
{
    size_t count = size / sizeof(uint64_t);
    uint64_t *store_ptr = (uint64_t *)memory;

    for (size_t i = 0; i < count; i++)
    {
        store_ptr[i] = MAGIC;
    }
}

void test_memset(void *memory, size_t size)
{
    memset(memory, 0, size);
}

void test_memcpy(void *memory, size_t size)
{
    size_t count = size / sizeof(uint64_t);
    size_t half = count / 2;
    uint64_t *load_ptr = (uint64_t *)memory;
    uint64_t *store_ptr = load_ptr + half;

    memcpy(store_ptr, load_ptr, half * sizeof(uint64_t));
}

double to_megabytes_per_second(size_t size, uint64_t duration)
{
    return (double)size / duration * 1000000000 / 1024 / 1024;
}

void memtest_init(void *memory, size_t size)
{
    size_t count = size / sizeof(uint64_t);
    uint64_t *ptr = (uint64_t *)memory;
    for (size_t i = 0; i < count; i++)
    {
        ptr[i] = MAGIC;
    }
}

struct test_result do_memory_test(size_t size, size_t times, void (*memtest_func)(void *, size_t))
{
    void *memory = malloc(size);
    struct timespec *time_points_start = (struct timespec *)malloc(sizeof(struct timespec) * (times + 1));
    struct timespec *time_points_end = time_points_start + 1;
    uint64_t *durations = (uint64_t *)malloc(sizeof(uint64_t) * times);
    uint64_t duration_sum, duration_avg, duration_min, duration_max;

#if WARMUP_INITMAGIC
    memtest_init(memory, size);
#endif
#if WARMUP_TESTFUNC
    memtest_func(memory, size);
#endif

    clock_gettime(CLOCK_MONOTONIC, &time_points_start[0]);
    for (size_t i = 0; i < times; i++)
    {
        memtest_func(memory, size);
        clock_gettime(CLOCK_MONOTONIC, &time_points_end[i]);
    }

    for (size_t i = 0; i < times; i++)
    {
        uint64_t d = (time_points_end[i].tv_sec - time_points_start[i].tv_sec) * 1000000000 + (time_points_end[i].tv_nsec - time_points_start[i].tv_nsec);
        durations[i] = d;
        if (i == 0)
        {
            duration_min = d;
            duration_max = d;
            duration_sum = d;
            continue;
        }
        if (d < duration_min)
        {
            duration_min = d;
        }
        if (d > duration_max)
        {
            duration_max = d;
        }
        duration_sum += d;
    }
    duration_avg = duration_sum / times;
    free(memory);
    free(time_points_start);
    free(durations);

    return (struct test_result){
        .best_rate = to_megabytes_per_second(size, duration_min),
        .avg_rate = to_megabytes_per_second(size, duration_avg),
        .worst_rate = to_megabytes_per_second(size, duration_max),
    };
}

struct test_function test_functions[] = {
    {"COPY", test_copy},
    {"STORE", test_store},
#if WARMUP_INITMAGIC
    {"LOAD", test_load},
#endif
    {"MEMSET", test_memset},
    {"MEMCPY", test_memcpy},
};

int main(void)
{
    size_t times = 20;
    struct test_result result;

    printf("Build with: TEST_GB=%d MAGIC_NOT_ZERO=%d WARMUP_INITMAGIC=%d WARMUP_TESTFUNC=%d\n",
            TEST_GB, MAGIC_NOT_ZERO, WARMUP_INITMAGIC, WARMUP_TESTFUNC);
    printf("TEST                Best Rate           Avg Rate            Worst Rate\n");
    printf("----------------------------------------------------------------------\n");
    for (size_t i = 0; i < sizeof(test_functions) / sizeof(test_functions[0]); i++)
    {
        result = do_memory_test(1024ull * 1024ull * 1024ull * TEST_GB, times, test_functions[i].func);
        printf("%-20s%-20.2f%-20.2f%-20.2f\n", test_functions[i].name, result.best_rate, result.avg_rate, result.worst_rate);
    }

    return 0;
}

