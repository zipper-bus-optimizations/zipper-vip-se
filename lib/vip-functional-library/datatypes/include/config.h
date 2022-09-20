#include <cstdint>
#include <cmath>

#ifndef __SW_CONFIG_H__
#define __SW_CONFIG_H__

struct Operand{
	uint8_t val[16] = {0};
	uint8_t valid_w_id = 0;
};
#define SIZE_OF_CACHELINE 64

static const uint64_t NUM_OPERAND = 3;
#if defined(REUSE) || defined(NOREUSE) || defined(BASELINE)
static const uint64_t NUM_FPGA_ENTRIES = 8;
#endif

// #ifdef BASELINE
// static const uint64_t NUM_FPGA_ENTRIES = 1;
// #endif

static const uint64_t NUM_OPERAND_ENTRIES  = exp2(ceil(log2(NUM_OPERAND*NUM_FPGA_ENTRIES)));
static const uint64_t WRITE_GRANULARITY = 64;
static const uint64_t READ_GRANULARITY = exp2(ceil(log2(sizeof(Operand))));
static const uint64_t NUM_OPERAND_PER_CACHELINE =  SIZE_OF_CACHELINE/READ_GRANULARITY;
static const uint64_t BITS_FOR_OFFSET = log2(NUM_OPERAND_PER_CACHELINE);
static const uint64_t NUM_CACHELINE = NUM_OPERAND_ENTRIES / NUM_OPERAND_PER_CACHELINE <= (14 - BITS_FOR_OFFSET)? NUM_OPERAND_ENTRIES / NUM_OPERAND_PER_CACHELINE : (14 - BITS_FOR_OFFSET);
static const uint64_t TOTAL_ENTRIES = NUM_CACHELINE*NUM_OPERAND_PER_CACHELINE;
static const uint64_t BITS_FOR_CACHELINE = NUM_CACHELINE;
static const uint64_t BITS_FOR_ID = 1;
#define CALC_OFFSET( x ) (x % NUM_OPERAND_PER_CACHELINE)
#define CALC_OFFSET_IN_BYTE( x ) (CALC_OFFSET(x) * READ_GRANULARITY) 
#define CALC_CACHELINE_ONE_HOT( x ) ( 1 << (BITS_FOR_OFFSET + (x%TOTAL_ENTRIES)/NUM_OPERAND_PER_CACHELINE))
#define CALC_CACHELINE_OFFSET( x ) ((int64_t) (x%TOTAL_ENTRIES)/NUM_OPERAND_PER_CACHELINE)
#define CALC_CACHELINE_OFFSET_IN_BYTE( x ) (CALC_CACHELINE_OFFSET( x ) << 6)
#define CALC_ID( x ) ( (x/TOTAL_ENTRIES) & 1)
#define CL(x) ((x) * CACHELINE_BYTES)

#endif
