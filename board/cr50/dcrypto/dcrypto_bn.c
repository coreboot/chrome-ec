/* Copyright 2016 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "dcrypto.h"
#include "internal.h"
#include "registers.h"

/* Firmware blob for crypto accelerator */
#include "dcrypto_bn.inc"

struct DMEM_ctx_ptrs {
	uint32_t pMod;
	uint32_t pDinv;
	uint32_t pRR;
	uint32_t pA;
	uint32_t pB;
	uint32_t pC;
	uint32_t n;
	uint32_t n1;
};

/*
 * This struct is "calling convention" for passing parameters into the
 * code block above for RSA operations.  Parameters start at &DMEM[0].
 */
struct DMEM_ctx {
	struct DMEM_ctx_ptrs in_ptrs;
	struct DMEM_ctx_ptrs sqr_ptrs;
	struct DMEM_ctx_ptrs mul_ptrs;
	struct DMEM_ctx_ptrs out_ptrs;
	uint32_t in[RSA_WORDS_4K];
	uint32_t dInv[8];
	uint32_t pubexp;
	uint32_t _pad1[3];
	uint32_t rnd[2];
	uint32_t _pad2[2];
	uint32_t mod[RSA_WORDS_4K];
	uint32_t RR[RSA_WORDS_4K];
	uint32_t exp[RSA_WORDS_4K + 8]; /* extra word for randomization */
	uint32_t out[RSA_WORDS_4K];
	uint32_t bin[RSA_WORDS_4K];
	uint32_t bout[RSA_WORDS_4K];
};

BUILD_ASSERT(sizeof(struct DMEM_ctx) <= 4096);
/* Check for 256-bit alignment. */
BUILD_ASSERT((offsetof(struct DMEM_ctx, in) & 31) == 0);
BUILD_ASSERT((offsetof(struct DMEM_ctx, mod) & 31) == 0);
BUILD_ASSERT((offsetof(struct DMEM_ctx, dInv) & 31) == 0);
BUILD_ASSERT((offsetof(struct DMEM_ctx, RR) & 31) == 0);

#define DMEM_CELL_SIZE 32
#define DMEM_INDEX(p, f)                                                       \
	(((const uint8_t *)&(p)->f - (const uint8_t *)(p)) / DMEM_CELL_SIZE)

/* Get non-0 64 bit random */
static bool rand64(uint32_t dst[2])
{
	do {
		uint64_t rnd;

		rnd = fips_trng_rand32();
		if (!rand_valid(rnd))
			return false;
		dst[0] = (uint32_t)rnd;

		rnd = fips_trng_rand32();
		if (!rand_valid(rnd))
			return false;
		dst[1] = (uint32_t)rnd;
	} while ((dst[0] | dst[1]) == 0);

	return true;
}

/* Grab dcrypto lock and set things up for modulus and input */
static int setup_and_lock(const struct LITE_BIGNUM *N,
			  const struct LITE_BIGNUM *input)
{
	struct DMEM_ctx *ctx =
		(struct DMEM_ctx *)GREG32_ADDR(CRYPTO, DMEM_DUMMY);

	/* Initialize hardware; load code page. */
	dcrypto_init_and_lock();
	dcrypto_imem_load(0, IMEM_dcrypto_bn, ARRAY_SIZE(IMEM_dcrypto_bn));

	/* Setup DMEM pointers (as indices into DMEM which are 256-bit cells).
	 */
	ctx->in_ptrs.pMod = DMEM_INDEX(ctx, mod);
	ctx->in_ptrs.pDinv = DMEM_INDEX(ctx, dInv);
	ctx->in_ptrs.pRR = DMEM_INDEX(ctx, RR);
	ctx->in_ptrs.pA = DMEM_INDEX(ctx, in);
	ctx->in_ptrs.pB = DMEM_INDEX(ctx, exp);
	ctx->in_ptrs.pC = DMEM_INDEX(ctx, out);
	ctx->in_ptrs.n = bn_bits(N) / (DMEM_CELL_SIZE * 8);
	ctx->in_ptrs.n1 = ctx->in_ptrs.n - 1;

	ctx->sqr_ptrs = ctx->in_ptrs;
	ctx->mul_ptrs = ctx->in_ptrs;
	ctx->out_ptrs = ctx->in_ptrs;

	dcrypto_dmem_load(DMEM_INDEX(ctx, in), input->d, bn_words(input));
	if (dcrypto_dmem_load(DMEM_INDEX(ctx, mod), N->d, bn_words(N)) == 0) {
		/*
		 * No change detected; assume modulus precomputation is cached.
		 */
		return 0;
	}

	/* Calculate RR and d0inv. */
	return dcrypto_call(CF_modload_adr);
}

#define MONTMUL(ctx, a, b, c)                                                  \
	montmul(ctx, DMEM_INDEX(ctx, a), DMEM_INDEX(ctx, b), DMEM_INDEX(ctx, c))

static int montmul(struct DMEM_ctx *ctx, uint32_t pA, uint32_t pB,
		   uint32_t pOut)
{

	ctx->in_ptrs.pA = pA;
	ctx->in_ptrs.pB = pB;
	ctx->in_ptrs.pC = pOut;

	return dcrypto_call(CF_mulx_adr);
}

#define MONTOUT(ctx, a, b) montout(ctx, DMEM_INDEX(ctx, a), DMEM_INDEX(ctx, b))

static int montout(struct DMEM_ctx *ctx, uint32_t pA, uint32_t pOut)
{

	ctx->in_ptrs.pA = pA;
	ctx->in_ptrs.pB = 0;
	ctx->in_ptrs.pC = pOut;

	return dcrypto_call(CF_mul1_adr);
}

#define MODEXP(ctx, in, exp, out)                                              \
	modexp(ctx, CF_modexp_adr, DMEM_INDEX(ctx, RR), DMEM_INDEX(ctx, in),   \
	       DMEM_INDEX(ctx, exp), DMEM_INDEX(ctx, out))

#define MODEXP1024(ctx, in, exp, out)                                          \
	modexp(ctx, CF_modexp_1024_adr, DMEM_INDEX(ctx, RR),                   \
	       DMEM_INDEX(ctx, in), DMEM_INDEX(ctx, exp),                      \
	       DMEM_INDEX(ctx, out))

#define MODEXP_BLINDED(ctx, in, exp, out)                                      \
	modexp(ctx, CF_modexp_blinded_adr, DMEM_INDEX(ctx, RR),                \
	       DMEM_INDEX(ctx, in), DMEM_INDEX(ctx, exp),                      \
	       DMEM_INDEX(ctx, out))

static int modexp(struct DMEM_ctx *ctx, uint32_t adr, uint32_t rr, uint32_t pIn,
		  uint32_t pExp, uint32_t pOut)
{
	/* in = in * RR */
	ctx->in_ptrs.pA = pIn;
	ctx->in_ptrs.pB = rr;
	ctx->in_ptrs.pC = pIn;

	/* out = out * out */
	ctx->sqr_ptrs.pA = pOut;
	ctx->sqr_ptrs.pB = pOut;
	ctx->sqr_ptrs.pC = pOut;

	/* out = out * in */
	ctx->mul_ptrs.pA = pIn;
	ctx->mul_ptrs.pB = pOut;
	ctx->mul_ptrs.pC = pOut;

	/* out = out / R */
	ctx->out_ptrs.pA = pOut;
	ctx->out_ptrs.pB = pExp;
	ctx->out_ptrs.pC = pOut;

	return dcrypto_call(adr);
}

/* output = input ** exp % N. */
enum dcrypto_result dcrypto_modexp_blinded(struct LITE_BIGNUM *output,
			   const struct LITE_BIGNUM *input,
			   const struct LITE_BIGNUM *exp,
			   const struct LITE_BIGNUM *N, uint32_t pubexp)
{
	int result;
	size_t i;
	struct DMEM_ctx *ctx =
		(struct DMEM_ctx *)GREG32_ADDR(CRYPTO, DMEM_DUMMY);

	uint32_t r_buf[RSA_MAX_WORDS];
	uint32_t rinv_buf[RSA_MAX_WORDS];

	struct LITE_BIGNUM r;
	struct LITE_BIGNUM rinv;

	bn_init(&r, r_buf, bn_size(N));
	bn_init(&rinv, rinv_buf, bn_size(N));

	/*
	 * pick 64 bit r != 0
	 * We cannot tolerate risk of 0 since 0 breaks computation.
	 */
	if (!rand64(r_buf))
		return DCRYPTO_FAIL;

	/*
	 * compute 1/r mod N
	 * Note this cannot fail since N is product of two large primes
	 * and r != 0, so we can ignore return value.
	 */
	bn_modinv_vartime(&rinv, &r, N);

	/*
	 * compute r^pubexp mod N
	 */
	dcrypto_modexp_word(&r, &r, pubexp, N);

	/* Pick !0 64-bit random for exponent blinding */
	if (!rand64(ctx->rnd))
		return DCRYPTO_FAIL;

	result = setup_and_lock(N, input);

	ctx->pubexp = pubexp;

	ctx->_pad1[0] = ctx->_pad1[1] = ctx->_pad1[2] = 0;
	ctx->_pad2[0] = ctx->_pad2[1] = 0;

	dcrypto_dmem_load(DMEM_INDEX(ctx, bin), r.d, bn_words(&r));
	dcrypto_dmem_load(DMEM_INDEX(ctx, bout), rinv.d, bn_words(&rinv));
	dcrypto_dmem_load(DMEM_INDEX(ctx, exp), exp->d, bn_words(exp));

	/* 0 pad the exponent to full size + 8 */
	for (i = bn_words(exp); i < bn_words(N) + 8; ++i)
		ctx->exp[i] = 0;

	/* Blind input */
	result |= MONTMUL(ctx, in, RR, in);
	result |= MONTMUL(ctx, in, bin, in);

	result |= MODEXP_BLINDED(ctx, in, exp, out);

	/* remove blinding factor */
	result |= MONTMUL(ctx, out, RR, out);
	result |= MONTMUL(ctx, out, bout, out);
	/* fully reduce out */
	result |= MONTMUL(ctx, out, RR, out);
	result |= MONTOUT(ctx, out, out);

	memcpy(output->d, ctx->out, bn_size(output));

	dcrypto_unlock();
	return dcrypto_ok_if_zero(result);
}

/* output = input ** exp % N. */
enum dcrypto_result dcrypto_modexp(struct LITE_BIGNUM *output,
				   const struct LITE_BIGNUM *input,
				   const struct LITE_BIGNUM *exp,
				   const struct LITE_BIGNUM *N)
{
	int result;
	size_t i;
	struct DMEM_ctx *ctx =
		(struct DMEM_ctx *)GREG32_ADDR(CRYPTO, DMEM_DUMMY);

	result = setup_and_lock(N, input);

	dcrypto_dmem_load(DMEM_INDEX(ctx, exp), exp->d, bn_words(exp));

	/* 0 pad the exponent to full size */
	for (i = bn_words(exp); i < bn_words(N); ++i)
		ctx->exp[i] = 0;

#ifdef CONFIG_DCRYPTO_RSA_SPEEDUP
	if (bn_bits(N) == 1024) { /* special code for 1024 bits */
		result |= MODEXP1024(ctx, in, exp, out);
	} else {
		result |= MODEXP(ctx, in, exp, out);
	}
#else
	result |= MODEXP(ctx, in, exp, out);
#endif

	memcpy(output->d, ctx->out, bn_size(output));

	dcrypto_unlock();
	return dcrypto_ok_if_zero(result);
}

/* output = input ** exp % N. */
enum dcrypto_result dcrypto_modexp_word(struct LITE_BIGNUM *output,
			const struct LITE_BIGNUM *input, uint32_t exp,
			const struct LITE_BIGNUM *N)
{
	int result;
	uint32_t e = exp;
	uint32_t b = 0x80000000;
	struct DMEM_ctx *ctx =
		(struct DMEM_ctx *)GREG32_ADDR(CRYPTO, DMEM_DUMMY);

	result = setup_and_lock(N, input);

	/* Find top bit */
	while (b != 0 && !(b & e))
		b >>= 1;

	/* out = in * RR */
	result |= MONTMUL(ctx, in, RR, out);
	/* in = in * RR */
	result |= MONTMUL(ctx, in, RR, in);

	while (b > 1) {
		b >>= 1;

		/* out = out * out */
		result |= MONTMUL(ctx, out, out, out);

		if ((b & e) != 0) {
			/* out = out * in */
			result |= MONTMUL(ctx, in, out, out);
		}
	}

	/* out = out / R */
	result |= MONTOUT(ctx, out, out);

	memcpy(output->d, ctx->out, bn_size(output));

	dcrypto_unlock();
	return dcrypto_ok_if_zero(result);
}

#ifndef CRYPTO_TEST_CMD_GENP
#define CRYPTO_TEST_CMD_GENP 0
#endif

#if defined(CRYPTO_TEST_SETUP) && CRYPTO_TEST_CMD_GENP
#include "console.h"
#include "shared_mem.h"
#include "timer.h"

static uint8_t genp_seed[32];
static uint32_t prime_buf[32];
static timestamp_t genp_start;
static timestamp_t genp_end;

static int genp_core(void)
{
	struct LITE_BIGNUM prime;
	int result;

	// Spin seed out into prng candidate prime.
	DCRYPTO_hkdf((uint8_t *)prime_buf, sizeof(prime_buf), genp_seed,
		     sizeof(genp_seed), 0, 0, 0, 0);
	DCRYPTO_bn_wrap(&prime, &prime_buf, sizeof(prime_buf));

	genp_start = get_time();
	result = (DCRYPTO_bn_generate_prime(&prime) == DCRYPTO_OK) ?
			 EC_SUCCESS :
			       EC_ERROR_UNKNOWN;
	genp_end = get_time();

	return result;
}

static int call_on_bigger_stack(int (*func)(void))
{
	int result, i;
	char *new_stack;
	const int new_stack_size = 4 * 1024;

	result = shared_mem_acquire(new_stack_size, &new_stack);
	if (result == EC_SUCCESS) {
		// Paint stack arena
		memset(new_stack, 0x01, new_stack_size);

		// Call whilst switching stacks
		__asm__ volatile("mov r4, sp\n" // save sp
				 "mov sp, %[new_stack]\n"
				 "blx %[func]\n"
				 "mov sp, r4\n" // restore sp
				 "mov %[result], r0\n"
				 : [result] "=r"(result)
				 : [new_stack] "r"(new_stack + new_stack_size),
				   [func] "r"(func)
				 : "r0", "r1", "r2", "r3", "r4",
				   "lr" // clobbers
		);

		// Take guess at amount of stack that got used
		for (i = 0; i < new_stack_size && new_stack[i] == 0x01; ++i)
			;
		ccprintf("stack: %u/%u\n", new_stack_size - i, new_stack_size);

		shared_mem_release(new_stack);
	}

	return result;
}

static int command_genp(int argc, char **argv)
{
	int result;

	memset(genp_seed, 0, sizeof(genp_seed));
	if (argc > 1)
		memcpy(genp_seed, argv[1], strlen(argv[1]));

	result = call_on_bigger_stack(genp_core);

	if (result == EC_SUCCESS) {
		ccprintf("prime: %ph (lsb first)\n",
			 HEX_BUF(prime_buf, sizeof(prime_buf)));
		ccprintf("μs   : %llu\n",
			 (long long)(genp_end.val - genp_start.val));
	}

	return result;
}
DECLARE_CONSOLE_COMMAND(genp, command_genp, "[seed]", "Generate prng prime");
#endif
