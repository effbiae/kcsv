//! ld() fast csv tokenizer, copyright (c) 2021 regents of kparc, bsd-2-clause

#include"k.h"

typedef struct CSV{
	I d;       //! file descriptor
	I batch;   //! batch size in rows
	G sep;     //! separator char
	G quo;     //! quoted fields mode
	S b;U bn;  //! input buffer + size
	IT*i;U n;  //! fieldstart indices + field count
	I cct;     //! column count
	I rea;     //! realloc count
	U more;    //! more bytes to tokenize?
	F bpf;     //! bytes per field heuristic
	F rtime;   //! read time
	U rbytes;  //! read bytes
}CSV;

#ifndef __ARM_NEON
#include<x86intrin.h>
typedef __m128i v128;
#define aV(a...) ((v128){a})
#define vmull_negone(x) _mm_clmulepi64_si128(aV(x,0),aV(-1ULL),0)[0]
#ifndef __AVX2__
typedef __m128i vec;typedef struct span{vec i0,i1,i2,i3;}span;
#define vld(p)(span){vload(p),vload(p+16),vload(p+32),vload(p+48)}
#define vload _mm_loadu_si128
#define cmpeq _mm_cmpeq_epi8
#define vset1 _mm_set1_epi8
Zin U mmask(span p){R (U)(UI)_mm_movemask_epi8(p.i0)     |((U)(UI)_mm_movemask_epi8(p.i1)<<16)\
                    |((U)(UI)_mm_movemask_epi8(p.i2)<<32)|((U)(UI)_mm_movemask_epi8(p.i3)<<48);}
#else
typedef __m256i vec;typedef struct span{vec l;vec h;}span;
#define vld(p)(span){vload(p),vload(p+32)}
Zin U mmask(span z){U x=(UI)_mm256_movemask_epi8(cmpeq(b,z.l)),y=(UI)_mm256_movemask_epi8(cmpeq(b,z.h));R(y<<32)|x;}
#define vload _mm256_loadu_si256
#define cmpeq _mm256_cmpeq_epi8
#define vset1 _mm256_set1_epi8
#endif
#else
#include<arm_neon.h>
typedef uint8x16_t vec;typedef struct span{vec i0;vec i1;vec i2;vec i3;}span;
#define vld(p)(span){vload(p),vload(p+16),vload(p+32),vload(p+48)}
#define vload vld1q_u8
#define cmpeq vceqq_u8
#define vset1 vmovq_n_u8
#define vmull_negone(x) vmull_p64(-1ULL,x)
Zin U mmask(span p){
 const vec bitmask = { 0x01,0x02,0x4,0x8,0x10,0x20,0x40,0x80,0x01,0x02,0x4,0x8,0x10,0x20,0x40,0x80 };
 vec t0=vandq_u8(p.i0,bitmask),t1=vandq_u8(p.i1,bitmask),t2=vandq_u8(p.i2,bitmask),t3=vandq_u8(p.i3,bitmask),
 sum0=vpaddq_u8(t0,t1),sum1=vpaddq_u8(t2,t3);sum0=vpaddq_u8(sum0,sum1);sum0=vpaddq_u8(sum0,sum0);
 R vgetq_lane_u64(vreinterpretq_u64_u8(sum0),0);}
#endif

#define ham _(popcountll) //!< hamming weight

// compare mask vs input, 5 uops, should be cheaper with avx512
Zin U cmi(span in,G m);

// find quote mask (which is a half-open mask that covers the first quote in a quote pair and everything in the quote pair)
// we also update the prev_iter_inside_quote value to tell the next iteration whether we finished the final iteration inside
// a quote pair; if so, this inverts our behavior of whether we're inside quotes for the next iteration.
Zin U fqm(span x,U*in_q);

// flatten values in 'b' assuming that they have values of idx plus their position in the bitvector, and store these indexes at p[ba]
// incrementing ba as we go. we potentially store extra values beyond end of valid bits, so p needs to be large enough to handle this.
Zin IT zip(IT*p,IT ba,IT idx,U b);

//:~
