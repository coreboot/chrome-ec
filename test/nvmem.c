/* Copyright 2016 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Test Cr-50 Non-Voltatile memory module
 */

#include "nvmem_test.h"

#include "common.h"
#include "console.h"
#include "crc.h"
#include "flash.h"
#include "flash_log.h"
#include "new_nvmem.h"
#include "nvmem.h"
#include "printf.h"
#include "shared_mem.h"
#include "task.h"
#include "test_util.h"
#include "timer.h"
#include "util.h"

#define WRITE_SEGMENT_LEN 200
#define WRITE_READ_SEGMENTS 4

enum test_failure_mode failure_mode;

static const uint8_t legacy_nvmem_image[] = {
#include "legacy_nvmem_dump.h"
};

BUILD_ASSERT(sizeof(legacy_nvmem_image) == NVMEM_PARTITION_SIZE);

static uint8_t write_buffer[NVMEM_PARTITION_SIZE];

struct nvmem_test_result {
	int var_count;
	int reserved_obj_count;
	int evictable_obj_count;
	int deleted_obj_count;
	int delimiter_count;
	int unexpected_count;
	size_t valid_data_size;
	size_t erased_data_size;
	size_t tuple_data_size;
};

static struct nvmem_test_result test_result;

int app_cipher(const void *salt_p, void *out_p, const void *in_p, size_t size)
{

	const uint8_t *in = in_p;
	uint8_t *out = out_p;
	const uint8_t *salt = salt_p;
	size_t i;

	for (i = 0; i < size; i++)
		out[i] = in[i] ^ salt[i % CIPHER_SALT_SIZE];

	return 1;
}

void app_compute_hash(const void *p_buf, size_t num_bytes, void *p_hash,
		      size_t hash_bytes)
{
	uint32_t crc;
	uint32_t *p_data;
	int n;
	size_t tail_size;

	crc32_init();
	/* Assuming here that buffer is 4 byte aligned. */
	p_data = (uint32_t *)p_buf;
	for (n = 0; n < num_bytes / 4; n++)
		crc32_hash32(*p_data++);

	tail_size = num_bytes % 4;
	if (tail_size) {
		uint32_t tail;

		tail = 0;
		memcpy(&tail, p_data, tail_size);
		crc32_hash32(tail);
	}

	/*
	 * Crc32 of 0xffffffff is 0xffffffff. Let's spike the results to avoid
	 * this unfortunate Crc32 property.
	 */
	crc = crc32_result() ^ 0x55555555;

	for (n = 0; n < hash_bytes; n += sizeof(crc)) {
		size_t copy_bytes = MIN(sizeof(crc), hash_bytes - n);

		memcpy((uint8_t *)p_hash + n, &crc, copy_bytes);
	}
}

int crypto_enabled(void)
{
	return 1;
}

/* Offsets where flash erase & write shall fail. `-1` means none. */
static int write_offset_fail = -1;
static int erase_offset_fail = -1;

/* Used to allow/prevent Flash erase/write operations */
enum ec_error_list flash_pre_op_write(int offset, int size)
{
	if (write_offset_fail >= offset &&
	    write_offset_fail < (offset + size)) {
		ccprintf("--> Fail write for offset = %d, size = %d\n", offset,
			 size);
		return EC_ERROR_UNKNOWN;
	}
	return EC_SUCCESS;
}

enum ec_error_list flash_pre_op_erase(int offset, int size)
{
	if (erase_offset_fail >= offset &&
	    erase_offset_fail < (offset + size)) {
		ccprintf("--> Fail erase for offset = %d, size = %d\n", offset,
			 size);
		return EC_ERROR_UNKNOWN;
	}
	return EC_SUCCESS;
}

static void dump_nvmem_state(const char *title,
			     const struct nvmem_test_result *tr)
{
	ccprintf("\n%s:\n", title);
	ccprintf("var_count: %d\n", tr->var_count);
	ccprintf("reserved_obj_count: %d\n", tr->reserved_obj_count);
	ccprintf("evictable_obj_count: %d\n", tr->evictable_obj_count);
	ccprintf("deleted_obj_count: %d\n", tr->deleted_obj_count);
	ccprintf("deimiter_count: %d\n", tr->delimiter_count);
	ccprintf("unexpected_count: %d\n", tr->unexpected_count);
	ccprintf("valid_data_size: %zd\n", tr->valid_data_size);
	ccprintf("tuple_data_size: %zd\n", tr->tuple_data_size);
	ccprintf("erased_data_size: %zd\n\n", tr->erased_data_size);
}

static void wipe_out_nvmem_cache(void)
{
	memset(nvmem_cache_base(NVMEM_TPM), 0, nvmem_user_sizes[NVMEM_TPM]);
}

static int prepare_nvmem_contents(void)
{
	struct nvmem_tag *tag;

	memcpy(write_buffer, legacy_nvmem_image, sizeof(write_buffer));
	tag = (struct nvmem_tag *)write_buffer;

	app_compute_hash(tag->padding, NVMEM_PARTITION_SIZE - NVMEM_SHA_SIZE,
			 tag->sha, sizeof(tag->sha));
	app_cipher(tag->sha, tag + 1, tag + 1,
		   NVMEM_PARTITION_SIZE - sizeof(struct nvmem_tag));

	return flash_physical_write(CONFIG_FLASH_NVMEM_BASE_A -
					    CONFIG_PROGRAM_MEMORY_BASE,
				    sizeof(write_buffer), write_buffer);
}

static int iterate_over_flash(void)
{
	enum ec_error_list rv;
	struct nn_container *ch;
	struct access_tracker at = {};
	uint8_t buf[CONFIG_FLASH_BANK_SIZE];

	memset(&test_result, 0, sizeof(test_result));
	ch = (struct nn_container *)buf;

	while ((rv = get_next_object(&at, ch, 1)) == EC_SUCCESS)
		switch (ch->container_type) {
		case NN_OBJ_OLD_COPY:
			if (ch->container_type_copy == NN_OBJ_TRANSACTION_DEL) {
				test_result.delimiter_count++;
			} else {
				test_result.deleted_obj_count++;
				test_result.erased_data_size += ch->size;
			}
			break;

		case NN_OBJ_TUPLE:
			test_result.var_count++;
			test_result.valid_data_size += ch->size;
			test_result.tuple_data_size += ch->size -
				sizeof(struct tuple);
			break;

		case NN_OBJ_TPM_RESERVED:
			test_result.reserved_obj_count++;
			test_result.valid_data_size += ch->size;
			break;

		case NN_OBJ_TPM_EVICTABLE:
			test_result.evictable_obj_count++;
			test_result.valid_data_size += ch->size;
			break;

		case NN_OBJ_TRANSACTION_DEL:
			test_result.delimiter_count++;
			break;
		default:
			test_result.unexpected_count++;
			break;
		}

	if (rv != EC_ERROR_MEMORY_ALLOCATION) {
		ccprintf("\n%s:%d - unexpected return value %d\n", __func__,
			 __LINE__, rv);
		return rv;
	}

	/* Verify that there is a delimiter at the top of the flash. */
	if (at.mt.data_offset > sizeof(*at.mt.ph)) {
		if ((at.mt.ph == at.dt.ph) &&
		    (((at.mt.data_offset - sizeof(struct nn_container))) ==
		     at.dt.data_offset)) {
			return EC_SUCCESS;
		}
	} else {
		if ((at.dt.ph == list_element_to_ph(at.list_index)) &&
		    (at.dt.data_offset ==
		     (CONFIG_FLASH_BANK_SIZE - sizeof(struct nn_container)))) {
			ccprintf("%s:%d edge delimiter case OK\n", __func__,
				 __LINE__);
			return EC_SUCCESS;
		}
	}
	ccprintf("%s:%d bad delimiter location: ph %pP, "
		 "dt.ph %pP, offset %d, delim offset %d\n",
		 __func__, __LINE__, at.mt.ph, at.dt.ph, at.mt.data_offset,
		 at.dt.data_offset);

	return EC_ERROR_INVAL;
}

static void *page_to_flash_addr(int page_num)
{
	uint32_t base_offset = CONFIG_FLASH_NEW_NVMEM_BASE_A;

	if (page_num > NEW_NVMEM_TOTAL_PAGES)
		return NULL;

	if (page_num >= (NEW_NVMEM_TOTAL_PAGES / 2)) {
		page_num -= (NEW_NVMEM_TOTAL_PAGES / 2);
		base_offset = CONFIG_FLASH_NEW_NVMEM_BASE_B;
	}

	return (void *)((uintptr_t)base_offset +
			page_num * CONFIG_FLASH_BANK_SIZE);
}

static int fill_nvmem_pages(uint8_t flash_value)
{
	int old_write_offset = write_offset_fail;

	memset(write_buffer, flash_value, sizeof(write_buffer));

	write_offset_fail = -1; /* Enable all writes. */
	/* Overwrite nvmem flash space with junk value. */
	flash_physical_write(
		CONFIG_FLASH_NEW_NVMEM_BASE_A - CONFIG_PROGRAM_MEMORY_BASE,
		NEW_FLASH_HALF_NVMEM_SIZE, (const char *)write_buffer);
	flash_physical_write(
		CONFIG_FLASH_NEW_NVMEM_BASE_B - CONFIG_PROGRAM_MEMORY_BASE,
		NEW_FLASH_HALF_NVMEM_SIZE, (const char *)write_buffer);
	write_offset_fail = old_write_offset;
	return EC_SUCCESS;
}

static int test_init_vars_from_scratch(void)
{
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);
	TEST_ASSERT(iterate_over_flash() == EC_SUCCESS);
	TEST_ASSERT(test_result.var_count == 0);
	TEST_ASSERT(test_result.reserved_obj_count == 38);
	TEST_ASSERT(test_result.evictable_obj_count == 0);
	TEST_ASSERT(test_result.deleted_obj_count == 0);
	TEST_ASSERT(test_result.unexpected_count == 0);
	TEST_ASSERT(test_result.valid_data_size == 1088);
	TEST_ASSERT(total_var_space == 0);
	return EC_SUCCESS;
}

static int post_init_from_scratch(uint8_t flash_value)
{
	int i;
	void *flash_p;

	fill_nvmem_pages(flash_value);

	TEST_ASSERT(test_init_vars_from_scratch() == EC_SUCCESS);

	for (i = 0; i < (NEW_NVMEM_TOTAL_PAGES - 1); i++) {
		flash_p = page_to_flash_addr(i);

		TEST_ASSERT(!!flash_p);
		TEST_ASSERT(is_uninitialized(flash_p, CONFIG_FLASH_BANK_SIZE));
	}

	flash_p = page_to_flash_addr(i);
	TEST_ASSERT(!is_uninitialized(flash_p, CONFIG_FLASH_BANK_SIZE));

	return EC_SUCCESS;
}

/*
 * The purpose of this test is to check NvMem initialization when NvMem is
 * completely erased (i.e. following SpiFlash write of program). In this case,
 * nvmem_init() is expected to create initial flash storage containing
 * reserved objects only.
 */
static int test_fully_erased_nvmem(void)
{

	return post_init_from_scratch(0xff);
}

/*
 * The purpose of this test is to check nvmem_init() in the case when no valid
 * pages exist but flash space is garbled as opposed to be fully erased. In
 * this case, the initialization is expected to create one new valid page and
 * erase the rest of the pages.
 */
static int test_corrupt_nvmem(void)
{
	return post_init_from_scratch(0x55);
}

static int prepare_new_flash(void)
{
	size_t i;
	NV_RESERVED_ITEM ri;

	TEST_ASSERT(test_fully_erased_nvmem() == EC_SUCCESS);

	/* Now copy sensible information into the nvmem cache. */
	memcpy(nvmem_cache_base(NVMEM_TPM),
	       legacy_nvmem_image + sizeof(struct nvmem_tag),
	       nvmem_user_sizes[NVMEM_TPM]);

	/* Clean garbage at the tail of TPM2B objects. */
	for (i = NV_OWNER_POLICY; i <= NV_EH_PROOF; i++) {
		uint16_t tpm2b_len;
		uint8_t *addr_in_cache;

		NvGetReserved(i, &ri);
		/* Clean trailing garbage in legacy content. */
		if (ri.size >= sizeof(tpm2b_len)) {
			addr_in_cache = nvmem_cache_base(NVMEM_TPM) + ri.offset;
			/* Get actual length of object. */
			memcpy(&tpm2b_len, addr_in_cache, sizeof(tpm2b_len));

			if (tpm2b_len < ri.size - sizeof(tpm2b_len)) {
				tpm2b_len += sizeof(tpm2b_len);
				memset(addr_in_cache + tpm2b_len, 0,
				       ri.size - tpm2b_len);
			}
		}
	}

	dump_nvmem_state("after first save", &test_result);
	TEST_ASSERT(new_nvmem_save() == EC_SUCCESS);
	TEST_ASSERT(iterate_over_flash() == EC_SUCCESS);

	TEST_ASSERT(test_result.deleted_obj_count == 24);
	TEST_ASSERT(test_result.var_count == 0);
	TEST_ASSERT(test_result.reserved_obj_count == 40);
	TEST_ASSERT(test_result.evictable_obj_count == 9);
	TEST_ASSERT(test_result.unexpected_count == 0);
	TEST_ASSERT(test_result.valid_data_size == 5128);
	TEST_ASSERT(test_result.erased_data_size == 698);

	return EC_SUCCESS;
}

/* Set TPM2B objects to their full length. */
static int set_length_tpm2b_reserved_spaces(void)
{
	NV_RESERVED_ITEM ri;
	size_t i;

	/* Initialize TPM2B objects with non-zero length. */
	for (i = NV_OWNER_POLICY; i <= NV_EH_PROOF; i++) {
		uint16_t tpm2b_len;

		NvGetReserved(i, &ri);
		/* Set TPM2B len to non zero */
		if (ri.size >= sizeof(tpm2b_len)) {
			tpm2b_len = ri.size - sizeof(tpm2b_len);
			memcpy(nvmem_cache_base(NVMEM_TPM) + ri.offset,
			       &tpm2b_len, sizeof(tpm2b_len));
		}
	}
	return EC_SUCCESS;
}

static int test_nvmem_save(void)
{
	const char *key = "var1";
	const char *value = "value of var 1";
	size_t total_var_size;
	struct nvmem_test_result old_result;

	TEST_ASSERT(prepare_new_flash() == EC_SUCCESS);

	/*
	 * Verify that saving without changing the cache does not affect flash
	 * contents.
	 */
	old_result = test_result;
	TEST_ASSERT(new_nvmem_save() == EC_SUCCESS);

	/*
	 * Save of unmodified cache does not modify the flash contents and
	 * does not set the delimiter.
	 */

	TEST_ASSERT(iterate_over_flash() == EC_SUCCESS);
	TEST_ASSERT(!memcmp(&test_result, &old_result, sizeof(test_result)));

	wipe_out_nvmem_cache();
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);
	TEST_ASSERT(new_nvmem_save() == EC_SUCCESS);
	TEST_ASSERT(iterate_over_flash() == EC_SUCCESS);
	TEST_ASSERT(!memcmp(&test_result, &old_result, sizeof(test_result)));

	/*
	 * Total size test variable storage takes in flash (container header
	 * size not included).
	 */
	total_var_size = strlen(key) + strlen(value) + sizeof(struct tuple);

	/* Verify that we can add a variable to nvmem. */
	TEST_ASSERT(setvar(key, strlen(key), value, strlen(value)) ==
		    EC_SUCCESS);
	TEST_ASSERT(iterate_over_flash() == EC_SUCCESS);

	/* Remove changes caused by the new var addition. */
	test_result.delimiter_count -= 1;
	test_result.valid_data_size -= total_var_size;
	test_result.tuple_data_size -= total_var_size -
		sizeof(struct tuple) * test_result.var_count;
	test_result.var_count -= 1;

	TEST_ASSERT(memcmp(&test_result, &old_result, sizeof(test_result)) ==
		    0);

	/* Verify that we can delete a variable from nvmem. */
	TEST_ASSERT(setvar(key, strlen(key), NULL, 0) == EC_SUCCESS);
	TEST_ASSERT(iterate_over_flash() == EC_SUCCESS);
	test_result.deleted_obj_count -= 1;
	test_result.erased_data_size -= total_var_size;
	test_result.delimiter_count -= 1;
	TEST_ASSERT(memcmp(&test_result, &old_result, sizeof(test_result)) ==
		    0);

	return EC_SUCCESS;
}

static size_t get_free_nvmem_room(void)
{
	size_t free_room;
	size_t free_pages;
	/* Compaction kicks in when 3 pages or less are left. */
	const size_t max_pages = NEW_NVMEM_TOTAL_PAGES - 3;

	ccprintf("list index %d, data offset 0x%x\n", controller_at.list_index,
		 controller_at.mt.data_offset);

	if (controller_at.list_index >= max_pages)
		return 0;

	free_pages = max_pages - controller_at.list_index;
	free_room = (free_pages - 1) * (CONFIG_FLASH_BANK_SIZE -
					sizeof(struct nn_page_header)) +
		    CONFIG_FLASH_BANK_SIZE - controller_at.mt.data_offset;
	ccprintf("free pages %zd, data offset 0x%x\n", free_pages,
		 controller_at.mt.data_offset);
	return free_room;
}

static int test_nvmem_compaction(void)
{
	char value[100]; /* Definitely more than enough. */
	const char *key = "var 1";
	int i;
	size_t key_len;
	size_t val_len;
	size_t free_room;
	size_t real_var_size;
	size_t var_space;
	int max_vars;
	int erased_data_size;
	const size_t alignment_mask = CONFIG_FLASH_WRITE_SIZE - 1;

	key_len = strlen(key);
	val_len = snprintf(value, sizeof(value), "variable value is %04d", 0);

	TEST_ASSERT(prepare_new_flash() == EC_SUCCESS);

	/*
	 * Remember how much room was erased before flooding nvmem with erased
	 * values.
	 */
	erased_data_size = test_result.erased_data_size;

	/* Let's see how much free room there is. */
	free_room = get_free_nvmem_room();
	TEST_ASSERT(free_room);

	/* How much room (key, value) pair takes in a container. */
	real_var_size = val_len + key_len + sizeof(struct tuple);
	/*
	 * See how many vars including containers should be able to fit there.
	 *
	 * First calculate rounded up space a var will take. Apart from the
	 * var itself there will be a container header and a delimiter.
	 */
	var_space = (real_var_size + 2 * sizeof(struct nn_container) +
		     alignment_mask) & ~alignment_mask;

	max_vars = free_room / var_space;

	/*
	 * And now flood the NVMEM with erased values (each new setvar()
	 * invocation erases the previous instance.
	 */
	for (i = 0; i <= max_vars; i++) {
		snprintf(value, sizeof(value), "variable value is %04d", i);
		TEST_ASSERT(setvar(key, key_len, value, val_len) == EC_SUCCESS);
	}

	TEST_ASSERT(iterate_over_flash() == EC_SUCCESS);
	/* Make sure there was no compaction yet. */
	TEST_ASSERT(test_result.erased_data_size > erased_data_size);

	/* This is how much the erased space grew as a result of flooding. */
	erased_data_size = test_result.erased_data_size - erased_data_size;
	TEST_ASSERT(erased_data_size == max_vars * real_var_size);

	/* This will take it over the compaction limit. */
	val_len = snprintf(value, sizeof(value), "variable value is %03d", i);
	TEST_ASSERT(setvar(key, key_len, value, val_len) == EC_SUCCESS);
	TEST_ASSERT(iterate_over_flash() == EC_SUCCESS);
	TEST_ASSERT(test_result.erased_data_size < var_space);

	return EC_SUCCESS;
}

static int test_configured_nvmem(void)
{
	/*
	 * The purpose of this test is to check how nvmem_init() initializes
	 * from previously saved flash contents.
	 */
	TEST_ASSERT(prepare_nvmem_contents() == EC_SUCCESS);

	/*
	 * This is initialization from legacy flash contents which replaces
	 * legacy flash image with the new format flash image
	 */
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);

	/* And this is initialization from the new flash layout. */
	return nvmem_init();
}

static uint8_t find_lb(const void *data)
{
	return (const uint8_t *)memchr(data, '#', 256) - (const uint8_t *)data;
}

/*
 * Helper function, depending on the argument value either writes variables
 * into nvmem and verifies their presence, or deletes them and verifies that
 * they indeed disappear.
 */
static int var_read_write_delete_helper(int do_write)
{
	size_t i;
	uint16_t saved_total_var_space;
	uint32_t coverage_map;

	const struct {
		uint8_t *key;
		uint8_t *value;
	} kv_pairs[] = {
		/* Use # as the delimiter to allow \0 in keys/values. */
		{"\0key\00#", "value of key2#"},  {"key1#", "value of key1#"},
		{"key2#", "value of key2#"},	  {"key3#", "value of\0 key3#"},
		{"ke\04#", "value\0 of\0 key4#"},
	};

	coverage_map = 0;
	saved_total_var_space = total_var_space;

	/*
	 * Read all vars, one at a time, verifying that they shows up in
	 * getvar results when appropriate but not before.
	 */
	for (i = 0; i <= ARRAY_SIZE(kv_pairs); i++) {
		size_t j;
		uint8_t key_len;
		uint8_t val_len;
		const void *value;

		for (j = 0; j < ARRAY_SIZE(kv_pairs); j++) {
			const struct tuple *t;

			coverage_map |= 1;

			key_len = find_lb(kv_pairs[j].key);
			t = getvar(kv_pairs[j].key, key_len);

			if ((j >= i) ^ !do_write) {
				TEST_ASSERT(t == NULL);
				continue;
			}

			coverage_map |= 2;

			TEST_ASSERT(saved_total_var_space == total_var_space);

			/* Confirm that what we found is the right variable. */
			val_len = find_lb(kv_pairs[j].value);

			TEST_ASSERT(t->key_len == key_len);
			TEST_ASSERT(t->val_len == val_len);
			TEST_ASSERT(
				!memcmp(kv_pairs[j].key, t->data_, key_len));
			TEST_ASSERT(!memcmp(kv_pairs[j].value,
					    t->data_ + key_len, val_len));
			freevar(t);
		}

		if (i == ARRAY_SIZE(kv_pairs)) {
			coverage_map |= 4;
			/* All four variables have been processed. */
			break;
		}

		val_len = find_lb(kv_pairs[i].value);
		key_len = find_lb(kv_pairs[i].key);
		value = kv_pairs[i].value;
		if (!do_write) {

			coverage_map |= 8;

			saved_total_var_space -= val_len + key_len;
			/*
			 * Make sure all val_len == 0 and val == NULL
			 * combinations are exercised.
			 */
			switch (i) {
			case 0:
				val_len = 0;
				coverage_map |= 0x10;
				break;

			case 1:
				coverage_map |= 0x20;
				value = NULL;
				break;
			default:
				coverage_map |= 0x40;
				val_len = 0;
				value = NULL;
				break;
			}
		} else {
			coverage_map |= 0x80;
			saved_total_var_space += val_len + key_len;
		}
		key_len = find_lb(kv_pairs[i].key);
		TEST_ASSERT(setvar(kv_pairs[i].key, key_len, value, val_len) ==
			    EC_SUCCESS);

		TEST_ASSERT(saved_total_var_space == total_var_space);
	}

	if (do_write)
		TEST_ASSERT(coverage_map == 0x87);
	else
		TEST_ASSERT(coverage_map == 0x7f);

	return EC_SUCCESS;
}

static int test_var_read_write_delete(void)
{
	TEST_ASSERT(post_init_from_scratch(0xff) == EC_SUCCESS);

	ccprintf("\n%s: starting write cycle\n", __func__);
	TEST_ASSERT(var_read_write_delete_helper(1) == EC_SUCCESS);

	ccprintf("%s: starting delete cycle\n", __func__);
	TEST_ASSERT(var_read_write_delete_helper(0) == EC_SUCCESS);

	return EC_SUCCESS;
}

/*
 * Dump hex contents of objects passes as address/size tuples to help
 * debugging. `header` is and optional object description passed in as ASCII
 * string.
 */
static void hex_dump(const char *header, const char *data, size_t data_size)
{
	size_t i;

	if (header)
		ccprintf("\n%s:", header);

	for (i = 0; i < data_size; i++)
		ccprintf(" %02x", (uint8_t)data[i]);
	ccprintf("\n");
}

/* Validate that a key/value pair is present in the NVMEM. */
static int validate_value(const char *key, size_t key_size, const char *value,
			  size_t value_size)
{
	const struct tuple *vart;

	vart = getvar(key, key_size);

	if (vart == NULL) {
		hex_dump("failed to find value for key", key, key_size);
		return EC_ERROR_INVAL;
	}

	if (vart->val_len != value_size) {
		ccprintf("unexpected value length %d\n", vart->val_len);
		freevar(vart);
		return EC_ERROR_INVAL;
	}

	if (memcmp(vart->data_ + key_size, value, value_size) != 0) {
		hex_dump("Bad value read back.\nExpected:", value, value_size);
		hex_dump("    Read:", vart->data_, value_size);
		freevar(vart);
		return EC_ERROR_INVAL;
	}

	freevar(vart);
	return EC_SUCCESS;
}

static int test_nvmem_tuple_capacity(void)
{
	char key[5];
	char value[18];
	int rv;
	size_t prev_size;
	size_t free_room;
	size_t max_tuples;

	/* Does not matter, but for consistency let's init key and value. */
	memset(key, 0, sizeof(key));
	memset(value, 0, sizeof(value));

	TEST_ASSERT(post_init_from_scratch(0xff) == EC_SUCCESS);

	/* Fill up var space until it is full, changing all keys and values. */
	while (1) {
		rv = setvar(key, sizeof(key), value, sizeof(value) - 1);
		if (rv != EC_SUCCESS)
			break;

		key[0]++;
		value[0]++;
	}
	TEST_ASSERT(rv == EC_ERROR_OVERFLOW);

	/* Verify that the values were written correctly. */
	max_tuples = key[0];
	key[0] = 0;
	value[0] = 0;
	while (max_tuples--) {
		TEST_ASSERT(validate_value(key, sizeof(key), value,
					   sizeof(value) - 1) == EC_SUCCESS);
		key[0]++;
		value[0]++;
	}
	iterate_over_flash();
	prev_size = test_result.tuple_data_size;

	/*
	 * Verify that total variable size is as expected. We know that the
	 * allotted space will not exactly fit a number of tuples, so the
	 * check is that the total tuple data size is smaller than the space.
	 *
	 * If some parameters change in the future such that this assumption
	 * becomes wrong, the test in the next line would fail.
	 */
	TEST_ASSERT(prev_size < MAX_VAR_TOTAL_SPACE);
	TEST_ASSERT((MAX_VAR_TOTAL_SPACE - prev_size) <
		    (sizeof(key) + sizeof(value) - 1));

	/*
	 * Verify that it is still possible to modify a variable when storage
	 * is almost full and the new value is larger than the old value.
	 */
	key[0]--;
	value[0]++;
	TEST_ASSERT(setvar(key, sizeof(key), value, sizeof(value)) ==
		    EC_SUCCESS);
	iterate_over_flash();
	TEST_ASSERT(test_result.tuple_data_size == (prev_size + 1));
	TEST_ASSERT(validate_value(key, sizeof(key), value, sizeof(value)) ==
		    EC_SUCCESS);

	/* Add a variable to completely fill the space. */
	free_room = MAX_VAR_TOTAL_SPACE - test_result.tuple_data_size;

	if (free_room < (sizeof(key) + 1)) {
		/*
		 * Increase the size of the first tuple's variable to
		 * capacity.
		 */
		char new_value[sizeof(value) + sizeof(key)];

		memset(new_value, 0, sizeof(new_value));
		key[0] = 0;
		TEST_ASSERT(setvar(key, sizeof(key), new_value,
				   sizeof(value) + free_room) == EC_SUCCESS);
		TEST_ASSERT(rv == EC_SUCCESS);
	} else if (free_room != 0) {
		/* Add a new tuple. */
		key[0] += 2;
		TEST_ASSERT(setvar(key, sizeof(key), value,
				   free_room - sizeof(key)) == EC_SUCCESS);
	}

	iterate_over_flash();
	TEST_ASSERT(test_result.tuple_data_size == MAX_VAR_TOTAL_SPACE);

	/*
	 * Try adding the smallest possible tuple, see that it fails the
	 * expected result.
	 */
	TEST_ASSERT(setvar(key, 1, value, 1) == EC_ERROR_OVERFLOW);

	/*
	 * Verify variables can be changed even when the allotted room is
	 * full.
	 */
	key[0] = 1; /* Access the second tuple in the set. */
	value[1] += 1; /* Second byte of value has not been modified so far. */
	TEST_ASSERT(setvar(key, sizeof(key), value, sizeof(value) - 1) ==
		    EC_SUCCESS);

	TEST_ASSERT(validate_value(key, sizeof(key), value,
				   sizeof(value) - 1) == EC_SUCCESS);
	return EC_SUCCESS;
}

/* Verify that nvmem_erase_user_data only erases the given user's data. */
static int test_nvmem_erase_tpm_data(void)
{
	TEST_ASSERT(prepare_nvmem_contents() == EC_SUCCESS);
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);
	browse_flash_contents(1);
	TEST_ASSERT(nvmem_erase_tpm_data() == EC_SUCCESS);
	browse_flash_contents(1);
	TEST_ASSERT(iterate_over_flash() == EC_SUCCESS);
	TEST_ASSERT(test_result.deleted_obj_count == 0);
	TEST_ASSERT(test_result.var_count == 3);
	TEST_ASSERT(test_result.reserved_obj_count == 38);
	TEST_ASSERT(test_result.evictable_obj_count == 0);
	TEST_ASSERT(test_result.unexpected_count == 0);
	TEST_ASSERT(test_result.valid_data_size == 1174);
	TEST_ASSERT(test_result.erased_data_size == 0);

	return EC_SUCCESS;
}

static size_t fill_obj_offsets(uint16_t *offsets, size_t max_objects)
{
	size_t i;
	size_t obj_count;

	obj_count = init_object_offsets(offsets, max_objects);

	ccprintf("%zd objects\n", obj_count);
	for (i = 0; i < obj_count; i++) {
		uint32_t *op;

		op = evictable_offs_to_addr(offsets[i]);
		ccprintf("offs %04x:%08x:%08x:%08x addr %pP size %zd\n",
			 offsets[i], op[-1], op[0], op[1], op,
			 (uintptr_t)nvmem_cache_base(NVMEM_TPM) + op[-1] -
				 (uintptr_t)op);
	}

	return obj_count;
}

static size_t fill_cache_offsets(const void *cache, uint16_t *offsets,
				 size_t max_objects)
{
	uint8_t buf[nvmem_user_sizes[NVMEM_TPM]];
	void *real_cache;
	size_t num_offsets;

	real_cache = nvmem_cache_base(NVMEM_TPM);
	memcpy(buf, real_cache, sizeof(buf));

	memcpy(real_cache, cache, sizeof(buf));
	memset(offsets, 0, sizeof(*offsets) * max_objects);
	num_offsets = fill_obj_offsets(offsets, max_objects);

	/* Restore the real cache. */
	memcpy(real_cache, buf, sizeof(buf));

	return num_offsets;
}
#define MAX_OFFSETS 20

static uint32_t get_evict_size(const uint8_t *cache, uint16_t offset)
{
	uint32_t next_addr;
	uint32_t cache_offset;

	cache_offset = s_evictNvStart + offset;
	memcpy(&next_addr, cache + cache_offset - sizeof(next_addr),
	       sizeof(next_addr));

	return next_addr - cache_offset;
}

/* Returns zero if the two objects are identical. */
static int compare_objects(const uint8_t *cache1, uint16_t offset1,
			   const uint8_t *cache2, uint16_t offset2)
{
	uint32_t size1;
	uint32_t size2;

	size1 = get_evict_size(cache1, offset1);
	size2 = get_evict_size(cache2, offset2);

	if (size1 == size2)
		return memcmp(cache1 + s_evictNvStart + offset1,
			      cache2 + s_evictNvStart + offset2, size1);

	return 1;
}
/*
 * Compare two instances of NVMEM caches. Reserved spaces should be exactly
 * the same for the match, but evictable objects could be rearranged due to
 * compaction, updating, etc.
 *
 * For the two cache instances to be considered the same the sets and contents
 * of the evictable object spaces must also match object to object.
 */
static int caches_match(const uint8_t *cache1, const uint8_t *cache2)
{
	int failed_count;
	size_t cache1_offs_count;
	size_t cache2_offs_count;
	size_t i;
	uint16_t cache1_offsets[MAX_OFFSETS];
	uint16_t cache2_offsets[MAX_OFFSETS];

	for (failed_count = i = 0; i < NV_PSEUDO_RESERVE_LAST; i++) {
		NV_RESERVED_ITEM ri;
		struct {
			uint32_t offset;
			uint32_t size;
		} ranges[3];
		size_t j;

		NvGetReserved(i, &ri);

		ranges[0].offset = ri.offset;

		if (i != NV_STATE_CLEAR) {
			ranges[0].size = ri.size;
			ranges[1].size = 0;
		} else {
			ranges[0].size = offsetof(STATE_CLEAR_DATA, pcrSave);
			ranges[1].offset = ranges[0].offset + ranges[0].size;
			ranges[1].size = sizeof(PCR_SAVE);
			ranges[2].offset = ranges[1].offset + ranges[1].size;
			ranges[2].size = sizeof(PCR_AUTHVALUE);
		}

		for (j = 0; j < ARRAY_SIZE(ranges); j++) {

			uint32_t offset;
			uint32_t size;
			uint32_t k;

			size = ranges[j].size;
			if (!size)
				break;

			offset = ranges[j].offset;

			if (!memcmp(cache1 + offset, cache2 + offset, size))
				continue;

			ccprintf("%s:%d failed comparing %zd:%zd:\n", __func__,
				 __LINE__, i, j);
			for (k = offset; k < (offset + size); k++)
				if (cache1[k] != cache2[k])
					ccprintf(" %3d:%02x", k - offset,
						 cache1[k]);
			ccprintf("\n");
			for (k = offset; k < (offset + size); k++)
				if (cache1[k] != cache2[k])
					ccprintf(" %3d:%02x", k - offset,
						 cache2[k]);
			ccprintf("\n");

			failed_count++;
		}
	}

	TEST_ASSERT(!failed_count);

	cache1_offs_count = fill_cache_offsets(cache1, cache1_offsets,
					       ARRAY_SIZE(cache1_offsets));
	cache2_offs_count = fill_cache_offsets(cache2, cache2_offsets,
					       ARRAY_SIZE(cache2_offsets));

	TEST_ASSERT(cache1_offs_count == cache2_offs_count);

	for (i = 0; (i < ARRAY_SIZE(cache1_offsets)) && cache2_offs_count;
	     i++) {
		size_t j;

		for (j = 0; j < cache2_offs_count; j++) {
			if (compare_objects(cache1, cache1_offsets[i], cache2,
					    cache2_offsets[j]))
				continue;
			/* Remove object from the cache2 offsets. */
			cache2_offsets[j] = cache2_offsets[--cache2_offs_count];
			break;
		}
	}

	TEST_ASSERT(cache2_offs_count == 0);
	return EC_SUCCESS;
}

static int prepare_post_migration_nvmem(void)
{
	TEST_ASSERT(prepare_nvmem_contents() == EC_SUCCESS);
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);
	TEST_ASSERT(new_nvmem_save() == EC_SUCCESS);
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);

	return EC_SUCCESS;
}
/*
 * This test creates various failure conditions related to interrupted nvmem
 * save operations and verifies that transaction integrity is maintained -
 * i.e. either all variables get updated,
 */
static int test_nvmem_incomplete_transaction(void)
{
	/*
	 * Will be more than enough, we can't store more than 15 objects or so
	 * anyways.
	 */
	uint16_t offsets[MAX_OFFSETS];
	size_t num_objects;
	uint8_t buf[nvmem_user_sizes[NVMEM_TPM]];
	uint8_t *p;
	size_t object_size;
	union entry_u e;

	TEST_ASSERT(prepare_post_migration_nvmem() == EC_SUCCESS);
	TEST_ASSERT(set_length_tpm2b_reserved_spaces() == EC_SUCCESS);
	num_objects = fill_obj_offsets(offsets, ARRAY_SIZE(offsets));
	TEST_ASSERT(num_objects == 9);

	/* Save cache state before deleting objects. */
	memcpy(buf, nvmem_cache_base(NVMEM_TPM), sizeof(buf));

	drop_evictable_obj(evictable_offs_to_addr(offsets[4]));
	drop_evictable_obj(evictable_offs_to_addr(offsets[3]));

	failure_mode = TEST_FAIL_WHEN_SAVING;
	TEST_ASSERT(new_nvmem_save() == EC_SUCCESS);
	wipe_out_nvmem_cache();
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);
	TEST_ASSERT(set_length_tpm2b_reserved_spaces() == EC_SUCCESS);
	TEST_ASSERT(caches_match(buf, nvmem_cache_base(NVMEM_TPM)) ==
		    EC_SUCCESS);
	drop_evictable_obj(evictable_offs_to_addr(offsets[4]));
	drop_evictable_obj(evictable_offs_to_addr(offsets[3]));

	/* Check if failure when invalidating is recovered after restart. */
	failure_mode = TEST_FAIL_WHEN_INVALIDATING;
	TEST_ASSERT(new_nvmem_save() == EC_SUCCESS);
	ccprintf("%s:%d\n", __func__, __LINE__);
	wipe_out_nvmem_cache();
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);
	ccprintf("%s:%d\n", __func__, __LINE__);
	num_objects = fill_obj_offsets(offsets, ARRAY_SIZE(offsets));
	TEST_ASSERT(num_objects == 7);

	/*
	 * Now, let's modify an object and introduce corruption when saving
	 * it.
	 */
	p = evictable_offs_to_addr(offsets[4]);
	p[10] ^= 0x55;
	failure_mode = TEST_FAILED_HASH;
	new_nvmem_save();
	failure_mode = TEST_NO_FAILURE;

	/* And verify that nvmem can still successfully initialize. */
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);

	/*
	 * Now let's interrupt saving an object spanning two pages.
	 *
	 * First, fill up the current page to get close to the limit such that
	 * the next save will have to span two flash pages.
	 */
	object_size = offsets[4] - offsets[3];
	p = (uint8_t *)evictable_offs_to_addr(offsets[3]) + object_size - 10;
	while ((controller_at.mt.data_offset + object_size +
		sizeof(struct nn_container)) <= CONFIG_FLASH_BANK_SIZE) {
		(*p)++;
		new_nvmem_save();
	}

	/* This will trigger spilling over the page boundary. */
	(*p)++;
	failure_mode = TEST_SPANNING_PAGES;
	new_nvmem_save();
	failure_mode = TEST_NO_FAILURE;

	/* Drain the event log. */
	e.r.timestamp = 0;
	while (flash_log_dequeue_event(e.r.timestamp, e.entry, sizeof(e)) > 0)
		;

	TEST_ASSERT(nvmem_init() == EC_SUCCESS);

	/* Let's verify that a container mismatch event has been added. */
	TEST_ASSERT(flash_log_dequeue_event(e.r.timestamp, e.entry, sizeof(e))
		    > 0);
	TEST_ASSERT(e.r.type == FE_LOG_NVMEM);
	TEST_ASSERT(e.r.payload[0] == NVMEMF_CONTAINER_HASH_MISMATCH);
	return EC_SUCCESS;
}

/*
 * Verify that interrupted compaction results in a consistent state of the
 * NVMEM cache.
 */
static int test_nvmem_interrupted_compaction(void)
{
	uint8_t buf[nvmem_user_sizes[NVMEM_TPM]];
	uint8_t target_list_index;
	uint8_t filler = 1;

	TEST_ASSERT(prepare_post_migration_nvmem() == EC_SUCCESS);

	/* Let's fill up a couple of pages with erased objects. */
	target_list_index = controller_at.list_index + 2;

	do {
		/*
		 * A few randomly picked reserved objects to modify to create
		 * need for compaction.
		 */
		const uint8_t objs_to_modify[] = {1, 3, 19, 42};
		size_t i;

		for (i = 0; i < ARRAY_SIZE(objs_to_modify); i++) {
			NV_RESERVED_ITEM ri;

			NvGetReserved(i, &ri);

			/* Direct access to the object. */
			memset(nvmem_cache_base(NVMEM_TPM) + ri.offset,
			       filler++, ri.size);
		}
		TEST_ASSERT(new_nvmem_save() == EC_SUCCESS);
	} while (controller_at.list_index != target_list_index);

	/* Save the state of NVMEM cache. */
	memcpy(buf, nvmem_cache_base(NVMEM_TPM), sizeof(buf));
	failure_mode = TEST_FAIL_WHEN_COMPACTING;
	compact_nvmem();
	wipe_out_nvmem_cache();
	ccprintf("%s:%d\n", __func__, __LINE__);
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);
	TEST_ASSERT(caches_match(buf, nvmem_cache_base(NVMEM_TPM)) ==
		    EC_SUCCESS);
	return EC_SUCCESS;
}

int nvmem_first_task(void *unused)
{
	return EC_SUCCESS;
}

int nvmem_second_task(void *unused)
{
	return EC_SUCCESS;
}

static void run_test_setup(void)
{
	/* Allow Flash erase/writes */
	erase_offset_fail = -1;
	write_offset_fail = -1;
	test_reset();
}

void nvmem_wipe_cache(void)
{
}

int DCRYPTO_ladder_is_enabled(void)
{
	return 1;
}

static int test_migration(void)
{
	/*
	 * This purpose of this test is to verify migration of the 'legacy'
	 * TPM NVMEM format to the new scheme where each element is stored in
	 * flash in its own container.
	 */
	TEST_ASSERT(prepare_nvmem_contents() == EC_SUCCESS);
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);
	TEST_ASSERT(iterate_over_flash() == EC_SUCCESS);
	TEST_ASSERT(test_result.var_count == 3);
	TEST_ASSERT(test_result.reserved_obj_count == 40);
	TEST_ASSERT(test_result.evictable_obj_count == 9);
	TEST_ASSERT(test_result.delimiter_count == 1);
	TEST_ASSERT(test_result.deleted_obj_count == 0);
	TEST_ASSERT(test_result.unexpected_count == 0);
	TEST_ASSERT(test_result.valid_data_size == 5214);
	TEST_ASSERT(total_var_space == 77);
	/* Container pointer not yet set. */
	TEST_ASSERT(!controller_at.ct.data_offset && !controller_at.ct.ph);
	return EC_SUCCESS;
}

/*
 * The purpose of this test is to verify variable storage limits, both per
 * object and total.
 */
static int test_var_boundaries(void)
{
	const size_t max_size = 255; /* Key and value must fit in a byte. */
	const uint8_t *key;
	const uint8_t *val;
	size_t key_len;
	size_t val_len;
	uint16_t saved_total_var_space;
	uint32_t coverage_map;
	uint8_t var_key[10];

	TEST_ASSERT(prepare_new_flash() == EC_SUCCESS);
	saved_total_var_space = total_var_space;
	coverage_map = 0;

	/*
	 * Let's use the legacy NVMEM image as a source of fairly random but
	 * reproducible data.
	 */
	key = legacy_nvmem_image;
	val = legacy_nvmem_image;

	/*
	 * Test limit of max variable body space, use keys and values of
	 * different sizes, below and above the limit.
	 */
	for (key_len = 1; key_len < max_size; key_len += 20) {

		coverage_map |= 1;

		val_len = MIN(max_size, MAX_VAR_BODY_SPACE - key_len);
		TEST_ASSERT(setvar(key, key_len, val, val_len) == EC_SUCCESS);
		TEST_ASSERT(total_var_space ==
			    saved_total_var_space + key_len + val_len);

		/* Now drop the variable from the storage. */
		TEST_ASSERT(setvar(key, key_len, NULL, 0) == EC_SUCCESS);
		TEST_ASSERT(total_var_space == saved_total_var_space);

		/* And if key length allows it, try to write too much. */
		if (val_len == max_size)
			continue;

		coverage_map |= 2;
		/*
		 * Yes, let's try writing one byte too many and see that the
		 * attempt is rejected.
		 */
		val_len++;
		TEST_ASSERT(setvar(key, key_len, val, val_len) ==
			    EC_ERROR_INVAL);
		TEST_ASSERT(total_var_space == saved_total_var_space);
	}

	/*
	 * Test limit of max total variable space, use keys and values of
	 * different sizes, below and above the limit.
	 */
	key_len = sizeof(var_key);
	val_len = 20; /* Anything below 256 would work. */
	memset(var_key, 'x', key_len);

	while (1) {
		int rv;

		/*
		 * Change the key so that a new variable is added to the
		 * storage.
		 */
		rv = setvar(var_key, key_len, val, val_len);

		if (rv == EC_ERROR_OVERFLOW)
			break;

		coverage_map |= 4;
		TEST_ASSERT(rv == EC_SUCCESS);
		var_key[0]++;
		saved_total_var_space += key_len + val_len;
	}

	TEST_ASSERT(saved_total_var_space == total_var_space);
	TEST_ASSERT(saved_total_var_space <= MAX_VAR_TOTAL_SPACE);
	TEST_ASSERT((saved_total_var_space + key_len + val_len) >
		    MAX_VAR_TOTAL_SPACE);

	TEST_ASSERT(coverage_map == 7);
	return EC_SUCCESS;
}

static int verify_ram_index_space(size_t verify_size)
{
	NV_RESERVED_ITEM ri;
	size_t i;
	uint32_t casted_size;
	uint8_t byte;
	uint8_t fill_byte = 0x55;

	if (verify_size > RAM_INDEX_SPACE)
		return EC_ERROR_INVAL;

	NvGetReserved(NV_RAM_INDEX_SPACE, &ri);

	/*
	 * Save the size of the index space, needed on machines where size_t
	 * is a 64 bit value.
	 */
	casted_size = verify_size;

	/*
	 * Now write index space in the cache, we write the complete space,
	 * but on read back only verify_size bytes are expected to be set.
	 */
	nvmem_write(ri.offset, sizeof(casted_size), &casted_size, NVMEM_TPM);

	for (i = 0; i < RAM_INDEX_SPACE; i++)
		nvmem_write(ri.offset + sizeof(casted_size) + i,
			    sizeof(fill_byte), &fill_byte, NVMEM_TPM);

	TEST_ASSERT(new_nvmem_save() == EC_SUCCESS);
	wipe_out_nvmem_cache();
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);

	/* Make sure read back size matches. */
	nvmem_read(ri.offset, sizeof(casted_size), &casted_size, NVMEM_TPM);
	TEST_ASSERT(casted_size == verify_size);

	/*
	 * Now check spaces which were supposed to be written (up to
	 * verify_size) and left intact.
	 */
	for (i = 0; i < RAM_INDEX_SPACE; i++) {
		nvmem_read(ri.offset + sizeof(casted_size) + i, sizeof(byte),
			   &byte, NVMEM_TPM);
		if (i < verify_size)
			TEST_ASSERT(byte == fill_byte);
		else
			TEST_ASSERT(byte == 0);
	}

	return EC_SUCCESS;
}

static int test_tpm_nvmem_modify_reserved_objects(void)
{
	NV_RESERVED_ITEM ri;
	/* Some random reserved objects' indices. */
	const uint8_t res_obj_ids[] = {1, 4, 9, 20};
	size_t i;
	static uint8_t cache_copy[12 * 1024];
	struct nvmem_test_result old_result;
	uint64_t new_values[ARRAY_SIZE(res_obj_ids)];
	size_t erased_size;

	TEST_ASSERT(sizeof(cache_copy) >= nvmem_user_sizes[NVMEM_TPM]);
	TEST_ASSERT(prepare_new_flash() == EC_SUCCESS);
	/* Initialize TPM2B objects with non-zero length. */
	TEST_ASSERT(set_length_tpm2b_reserved_spaces() == EC_SUCCESS);
	TEST_ASSERT(new_nvmem_save() == EC_SUCCESS);
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);
	iterate_over_flash();
	old_result = test_result;

	/* Preserve NVMEM cache for future comparison. */
	memcpy(cache_copy, nvmem_cache_base(NVMEM_TPM),
	       nvmem_user_sizes[NVMEM_TPM]);

	erased_size = 0;
	/* Modify several reserved objects in the cache. */
	for (i = 0; i < ARRAY_SIZE(res_obj_ids); i++) {
		size_t copy_size;
		uint8_t *addr_in_cache;
		uint32_t object_id;
		size_t k;
		size_t start_byte;

		object_id = res_obj_ids[i];
		NvGetReserved(object_id, &ri);
		copy_size = MIN(sizeof(new_values[0]), ri.size);
		addr_in_cache = nvmem_cache_base(NVMEM_TPM) + ri.offset;

		/* Prepare a new value for the variable. */
		memcpy(new_values + i, addr_in_cache, copy_size);

		/* Avoid changing TPM2B size and tail for large spaces. */
		start_byte = 0;
		if (object_id >= NV_OWNER_POLICY && object_id <= NV_EH_PROOF) {
			uint16_t tpm2b_len;

			start_byte = 2;
			memcpy(&tpm2b_len, (new_values + i), sizeof(tpm2b_len));
			if (start_byte + tpm2b_len < copy_size)
				copy_size = start_byte + tpm2b_len;
		}

		for (k = start_byte; k < copy_size; k++)
			((uint8_t *)(new_values + i))[k] ^= 0x55;

		/* Update value in the cache. */
		memcpy(addr_in_cache, new_values + i, copy_size);

		/* And in the cache copy. */
		memcpy(cache_copy + ri.offset, new_values + i, copy_size);

		/*
		 * This much will be added to the erased space, object size
		 * plus index size.
		 */
		erased_size += ri.size + 1;
	}

	/* Save it into flash. */
	TEST_ASSERT(new_nvmem_save() == EC_SUCCESS);

	/* Wipe out the cache to be sure. */
	wipe_out_nvmem_cache();

	/* Read NVMEM contents from flash. */
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);

	/* Verify that the cache matches expectations. */
	TEST_ASSERT(!memcmp(cache_copy, nvmem_cache_base(NVMEM_TPM),
			    nvmem_user_sizes[NVMEM_TPM]));

	iterate_over_flash();

	/* Update previous results with our expectations. */
	old_result.deleted_obj_count += ARRAY_SIZE(res_obj_ids);
	old_result.erased_data_size += erased_size;
	old_result.delimiter_count++;

	TEST_ASSERT(!memcmp(&test_result, &old_result, sizeof(test_result)));

	/* Verify several index space cases. */
	for (i = 0; i <= RAM_INDEX_SPACE; i += (RAM_INDEX_SPACE / 2))
		TEST_ASSERT(verify_ram_index_space(i) == EC_SUCCESS);

	return EC_SUCCESS;
}

static int fill_pages(void)
{
	uint8_t key[] = "Key";
	uint8_t value[132] = {};
	size_t key_len = sizeof(key);
	size_t i;

	for (i = 0; i < sizeof(value); i++)
		TEST_ASSERT(setvar(key, key_len, value, i) == EC_SUCCESS);
	return EC_SUCCESS;
}

static int test_nvmem_flash_failure(void)
{
	fill_nvmem_pages(0x55);

	/* Now set failing offset for erase, page 123 and to write to first page
	 */
	erase_offset_fail = 123 * 2048;
	write_offset_fail = 255 * 2048; /* This is first page in the list */
	TEST_ASSERT(test_init_vars_from_scratch() == EC_SUCCESS);
	TEST_ASSERT(page_count == 8);
	TEST_ASSERT(page_list[0] == 254);

	/* After init again page 255 is not yet detected as bad */
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);
	TEST_ASSERT(page_count == 9);

	TEST_ASSERT(iterate_over_flash() == EC_SUCCESS);
	TEST_ASSERT(new_nvmem_save() == EC_SUCCESS);
	TEST_ASSERT(fill_pages() == EC_SUCCESS);
	TEST_ASSERT(new_nvmem_save() == EC_SUCCESS);
	browse_flash_contents(1);
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);
	/* Cause compaction */
	TEST_ASSERT(fill_pages() == EC_SUCCESS);
	TEST_ASSERT(new_nvmem_save() == EC_SUCCESS);

	fill_nvmem_pages(0x55);
	/* Now set failing offset for erase, page 124 */
	erase_offset_fail = 124 * 2048;
	write_offset_fail = 254 * 2048; /* This is first page in the list */
	TEST_ASSERT(test_init_vars_from_scratch() == EC_SUCCESS);
	TEST_ASSERT(page_count == 9);
	TEST_ASSERT(page_list[0] == 255);

	TEST_ASSERT(new_nvmem_save() == EC_SUCCESS);
	TEST_ASSERT(fill_pages() == EC_SUCCESS);
	/* In the write process we hit page 254 */
	TEST_ASSERT(page_count == 8);
	TEST_ASSERT(new_nvmem_save() == EC_SUCCESS);
	browse_flash_contents(1);
	/* Cause compaction */
	TEST_ASSERT(fill_pages() == EC_SUCCESS);
	TEST_ASSERT(new_nvmem_save() == EC_SUCCESS);

	TEST_ASSERT(compact_nvmem() == EC_SUCCESS);
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);
	/* Again, page with write issue is not yet detected */
	TEST_ASSERT(page_count == 9);
	TEST_ASSERT(new_nvmem_save() == EC_SUCCESS);

	erase_offset_fail = -1;
	write_offset_fail = -1;
	return EC_SUCCESS;
}

static int compare_object(uint16_t obj_offset, size_t obj_size, const void *obj)
{
	uint32_t next_addr;

	memcpy(&next_addr,
	       evictable_offs_to_addr(obj_offset - sizeof(next_addr)),
	       sizeof(next_addr));

	ccprintf("next_addr %x, sum %zx size %zd\n", next_addr,
		 (s_evictNvStart + obj_offset + obj_size), obj_size);
	TEST_ASSERT(next_addr == (s_evictNvStart + obj_offset + obj_size));

	if (!memcmp(evictable_offs_to_addr(obj_offset), obj, obj_size))
		return EC_SUCCESS;

	return EC_ERROR_INVAL;
}

static int test_tpm_nvmem_modify_evictable_objects(void)
{
	size_t num_objects;
	uint16_t offsets[MAX_OFFSETS];
	uint32_t handles[ARRAY_SIZE(offsets)];
	uint32_t new_evictable_object[30];
	size_t i;
	const uint32_t new_obj_handle = 0x100;
	static uint8_t modified_obj[CONFIG_FLASH_BANK_SIZE];
	size_t modified_obj_size;
	uint32_t modified_obj_handle;
	uint32_t deleted_obj_handle;
	uint8_t *obj_cache_addr;
	size_t num_handles;
	int new_obj_index;
	int modified_obj_index;

	TEST_ASSERT(prepare_new_flash() == EC_SUCCESS);
	TEST_ASSERT(new_nvmem_save() == EC_SUCCESS);
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);
	iterate_over_flash();

	/* Verify that all evictable objects are there. */
	num_objects = fill_obj_offsets(offsets, ARRAY_SIZE(offsets));
	TEST_ASSERT(num_objects == 9);
	num_handles = num_objects;

	/* Save handles of all objects there are. */
	for (i = 0; i < num_objects; i++) {
		memcpy(handles + i, evictable_offs_to_addr(offsets[i]),
		       sizeof(handles[i]));
		ccprintf("obj %zd handle %08x\n", i, handles[i]);
	}
	/*
	 * Let's modify the object which currently is stored second in the
	 * stack.
	 */
	modified_obj_size = offsets[3] - offsets[2] - sizeof(uint32_t);

	/* Modify the object and copy modified value into local buffer. */
	obj_cache_addr = evictable_offs_to_addr(offsets[2]);
	memcpy(&modified_obj_handle, obj_cache_addr,
	       sizeof(modified_obj_handle));

	for (i = 0; i < modified_obj_size; i++) {
		uint8_t c;

		c = obj_cache_addr[i];

		if (i >= sizeof(uint32_t)) { /* Preserve the 4 byte handle. */
			c ^= 0x55;
			obj_cache_addr[i] = c;
		}
		modified_obj[i] = c;
	}

	/* Save its handle and then drop the object at offset 5. */
	memcpy(&deleted_obj_handle, evictable_offs_to_addr(offsets[5]),
	       sizeof(deleted_obj_handle));
	drop_evictable_obj(evictable_offs_to_addr(offsets[5]));

	/* Prepare the new evictable object, first four bytes are the handle. */
	for (i = 0; i < ARRAY_SIZE(new_evictable_object); i++)
		new_evictable_object[i] = new_obj_handle + i;

	/* Add it to the cache. */
	add_evictable_obj(new_evictable_object, sizeof(new_evictable_object));

	/* Save the new cache state in the flash. */
	TEST_ASSERT(new_nvmem_save() == EC_SUCCESS);

	/* Wipe out NVMEM cache just in case. */
	wipe_out_nvmem_cache();

	/* Read back from flash into cache. */
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);

	/* One object removed, one added, the number should have not changed. */
	TEST_ASSERT(num_objects ==
		    fill_obj_offsets(offsets, ARRAY_SIZE(offsets)));

	new_obj_index = 0;
	modified_obj_index = 0;
	for (i = 0; i < num_objects; i++) {
		uint32_t handle;
		size_t j;

		memcpy(&handle, evictable_offs_to_addr(offsets[i]),
		       sizeof(handles[i]));
		ASSERT(handle != deleted_obj_handle);

		if (handle == new_obj_handle)
			new_obj_index = i;
		else if (handle == modified_obj_handle)
			modified_obj_index = i;
		/*
		 * Remove the found handle from the set of handles which were
		 * there originally.
		 */
		for (j = 0; j < num_handles; j++)
			if (handles[j] == handle) {
				num_handles--;
				handles[j] = handles[num_handles];
				break;
			}
	}

	/*
	 * Removed object's handle is still in the array, and it should be the
	 * only remaining element.
	 */
	TEST_ASSERT(num_handles == 1);
	TEST_ASSERT(handles[0] == deleted_obj_handle);
	TEST_ASSERT(new_obj_index >= 0); /* New handle was seen in the cache. */
	TEST_ASSERT(modified_obj_index >=
		    0); /* Modified object was seen in the cache. */

	TEST_ASSERT(compare_object(offsets[new_obj_index],
				   sizeof(new_evictable_object),
				   new_evictable_object) == EC_SUCCESS);
	TEST_ASSERT(compare_object(offsets[modified_obj_index],
				   modified_obj_size,
				   modified_obj) == EC_SUCCESS);
	return EC_SUCCESS;
}

static int test_nvmem_tuple_updates(void)
{
	size_t i;
	const char *modified_var1 = "var one after";
	const struct tuple *t;

	const struct {
		uint8_t *key;
		uint8_t *value;
	} kv_pairs[] = {
		/* Use # as the delimiter to allow \0 in keys/values. */
		{"key0", "var zero before"},
		{"key1", "var one before"}
	};

	TEST_ASSERT(post_init_from_scratch(0xff) == EC_SUCCESS);

	/* Save vars in the nvmem. */
	for (i = 0; i < ARRAY_SIZE(kv_pairs); i++)
		TEST_ASSERT(setvar(kv_pairs[i].key, strlen(kv_pairs[i].key),
				   kv_pairs[i].value,
				   strlen(kv_pairs[i].value)) == EC_SUCCESS);

	TEST_ASSERT(nvmem_init() == EC_SUCCESS);
	/* Verify the vars are still there. */
	for (i = 0; i < ARRAY_SIZE(kv_pairs); i++) {
		TEST_ASSERT(validate_value(
				    kv_pairs[i].key, strlen(kv_pairs[i].key),
				    kv_pairs[i].value,
				    strlen(kv_pairs[i].value)) == EC_SUCCESS);
	}

	/*
	 * Now, let's try updating variable 'key1' introducing various failure
	 * modes.
	 */
	failure_mode = TEST_FAIL_SAVING_VAR;
	TEST_ASSERT(setvar(kv_pairs[1].key, strlen(kv_pairs[1].key),
			   modified_var1, strlen(modified_var1)) == EC_SUCCESS);
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);
	/* No change should be seen. */
	for (i = 0; i < ARRAY_SIZE(kv_pairs); i++) {
		TEST_ASSERT(validate_value(
				    kv_pairs[i].key, strlen(kv_pairs[i].key),
				    kv_pairs[i].value,
				    strlen(kv_pairs[i].value)) == EC_SUCCESS);
	}
	failure_mode = TEST_FAIL_FINALIZING_VAR;
	TEST_ASSERT(setvar(kv_pairs[1].key, strlen(kv_pairs[1].key),
			   modified_var1, strlen(modified_var1)) == EC_SUCCESS);
	failure_mode = TEST_NO_FAILURE;
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);

	/* First variable should be still unchanged. */
	TEST_ASSERT(validate_value(kv_pairs[0].key, strlen(kv_pairs[0].key),
				    kv_pairs[0].value,
				    strlen(kv_pairs[0].value)) == EC_SUCCESS);

	/* Second variable should be updated. */
	TEST_ASSERT(validate_value(kv_pairs[1].key, strlen(kv_pairs[1].key),
				   modified_var1, strlen(modified_var1)) ==
		    EC_SUCCESS);

	/* A corrupted attempt to update second variable. */
	failure_mode = TEST_FAIL_FINALIZING_VAR;
	TEST_ASSERT(setvar(kv_pairs[1].key, strlen(kv_pairs[1].key),
			   kv_pairs[1].value, strlen(kv_pairs[1].value))
		    == EC_SUCCESS);
	failure_mode = TEST_NO_FAILURE;
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);

	/* Is there an instance of the second variable still in the flash. */
	t = getvar(kv_pairs[1].key, strlen(kv_pairs[1].key));
	TEST_ASSERT(t);
	freevar(t);

	/* Delete the remaining instance of the variable. */
	TEST_ASSERT(setvar(kv_pairs[1].key, strlen(kv_pairs[1].key),
			   NULL, 0) == EC_SUCCESS);

	/* Verify that it is indeed deleted before and after re-init. */
	TEST_ASSERT(!getvar(kv_pairs[1].key, strlen(kv_pairs[1].key)));
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);
	TEST_ASSERT(!getvar(kv_pairs[1].key, strlen(kv_pairs[1].key)));

	return EC_SUCCESS;
}


static int test_nvmem_erase_tpm_data_selective(void)
{
	size_t i, j;
	uint8_t key[] = "K";
	uint8_t value[255] = {};
	size_t key_len = sizeof(key);

	TEST_ASSERT(post_init_from_scratch(0xff) == EC_SUCCESS);
	TEST_ASSERT(nvmem_erase_tpm_data_selective(NULL) == EC_SUCCESS);
	/* Iterate over different sizes of variable to explore cases when
	 * we are close to the page boundary, at it and crossing it.
	 * During test some values of `i` were particular interesting, but
	 * it is better to try all, rather than few specific options.
	 */
	for (i = 0; i < sizeof(value); i += 4) {
		/* Fill pages so that it doesn't yet cause compaction, but
		 * is very close to it. 50 iterations cause compaction.
		 */
		for (j = 0; j < 49; j++) {
			TEST_ASSERT(setvar(key, key_len, value,
					   sizeof(value) - (j & 1)) ==
				    EC_SUCCESS);
		}
		/* Now vary size of variable to explore different cases. */
		TEST_ASSERT(setvar(key, key_len, value, i) == EC_SUCCESS);
		TEST_ASSERT(nvmem_erase_tpm_data_selective(NULL) == EC_SUCCESS);
	}
	/* Fill in variable space */
	for (i = 0; i < 3; i++) {
		TEST_ASSERT(setvar(value, sizeof(value) - i*10, key, key_len) ==
			    EC_SUCCESS);
	}
	TEST_ASSERT(total_var_space == 995);
	/* Test with very small var space available */
	TEST_ASSERT(nvmem_erase_tpm_data_selective(NULL) == EC_SUCCESS);
	TEST_ASSERT(total_var_space == 995);
	return EC_SUCCESS;
}

static int test_tpm2b_garbage_clean(void)
{
	size_t i;
	NV_RESERVED_ITEM ri;
	uint16_t tpm2b_len;
	uint8_t *addr_in_cache;

	TEST_ASSERT(test_fully_erased_nvmem() == EC_SUCCESS);

	/* Now copy sensible information into the nvmem cache. */
	memcpy(nvmem_cache_base(NVMEM_TPM),
	       legacy_nvmem_image + sizeof(struct nvmem_tag),
	       nvmem_user_sizes[NVMEM_TPM]);

	/* Modify one space to have incorrect TPM2B */
	NvGetReserved(NV_OWNER_POLICY, &ri);
	tpm2b_len = ri.size; /* +2 bytes overflow */
	addr_in_cache = nvmem_cache_base(NVMEM_TPM) + ri.offset;
	memcpy(addr_in_cache, &tpm2b_len, sizeof(tpm2b_len));
	browse_flash_contents(1);
	dump_nvmem_state("after first save", &test_result);

	/* Saving creates many copies as `flash` content differs. */
	TEST_ASSERT(new_nvmem_save() == EC_SUCCESS);
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);

	/* Check garbage at the tail of TPM2B objects. */
	for (i = NV_OWNER_POLICY; i <= NV_EH_PROOF; i++) {
		NvGetReserved(i, &ri);
		TEST_ASSERT(ri.size >= sizeof(tpm2b_len));
		addr_in_cache = nvmem_cache_base(NVMEM_TPM) + ri.offset;
		/* Get actual length of object. */
		memcpy(&tpm2b_len, addr_in_cache, sizeof(tpm2b_len));
		TEST_ASSERT(tpm2b_len <= ri.size - sizeof(tpm2b_len));
		tpm2b_len += sizeof(tpm2b_len);
		TEST_ASSERT_MEMSET(addr_in_cache + tpm2b_len, 0,
				   ri.size - tpm2b_len);
	}

	TEST_ASSERT(new_nvmem_save() == EC_SUCCESS);
	TEST_ASSERT(nvmem_init() == EC_SUCCESS);
	browse_flash_contents(1);
	/* Now check that some objects were replaced compared to original
	 * data in prepare_new_image() before cleaning after the
	 * new_nvmem_save().
	 */
	TEST_ASSERT(iterate_over_flash() == EC_SUCCESS);

	/* R:04 updated twice, R:07 - once */
	TEST_ASSERT(test_result.deleted_obj_count == 24 + 2 + 1);
	TEST_ASSERT(test_result.var_count == 0);
	TEST_ASSERT(test_result.reserved_obj_count == 41);
	TEST_ASSERT(test_result.evictable_obj_count == 9);
	TEST_ASSERT(test_result.unexpected_count == 0);
	TEST_ASSERT(test_result.valid_data_size == 5128 + 67);
	TEST_ASSERT(test_result.erased_data_size == 698 + 3 * 67);

	return EC_SUCCESS;
}

void run_test(void)
{
	run_test_setup();

	RUN_TEST(test_migration);
	RUN_TEST(test_corrupt_nvmem);
	RUN_TEST(test_fully_erased_nvmem);
	RUN_TEST(test_configured_nvmem);
	RUN_TEST(test_nvmem_save);
	RUN_TEST(test_var_read_write_delete);
	RUN_TEST(test_nvmem_compaction);
	RUN_TEST(test_var_boundaries);
	RUN_TEST(test_nvmem_erase_tpm_data);
	RUN_TEST(test_tpm_nvmem_modify_reserved_objects);
	RUN_TEST(test_tpm_nvmem_modify_evictable_objects);
	RUN_TEST(test_nvmem_incomplete_transaction);
	RUN_TEST(test_nvmem_tuple_updates);
	failure_mode = TEST_NO_FAILURE; /* In case the above test failed. */
	RUN_TEST(test_nvmem_tuple_capacity);
	RUN_TEST(test_nvmem_interrupted_compaction);
	failure_mode = TEST_NO_FAILURE; /* In case the above test failed. */
	RUN_TEST(test_nvmem_erase_tpm_data_selective);

	/*
	 * more tests to come
	 * RUN_TEST(test_lock);
	 * RUN_TEST(test_malloc_blocking);
	 */
	RUN_TEST(test_nvmem_flash_failure);
	RUN_TEST(test_tpm2b_garbage_clean);
	test_print_result();
}
