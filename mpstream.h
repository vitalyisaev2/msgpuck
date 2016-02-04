#ifndef MSGPUCK_MPSTREAM_H_INCLUDED
#define MSGPUCK_MPSTREAM_H_INCLUDED
/*
 * Copyright 2010-2015, Tarantool AUTHORS, please see AUTHORS file.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "msgpuck.h"

#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

/**
 * Ask the allocator to reserve at least size bytes. It can reserve
 * more, and update *size with the new size.
 */
typedef	void *(* mpstream_reserve_f)(void *ctx, size_t *size);

/** Actually use the bytes. */
typedef	void *(* mpstream_alloc_f)(void *ctx, size_t size);

/** Actually use the bytes. */
typedef	void (* mpstream_error_f)(void *error_ctx, const char *err,
				  size_t errlen);

struct mpstream {
	/**
	 * When pos >= end, or required size doesn't fit in
	 * pos..end range alloc() is called to advance the stream
	 * and reserve() to get a new chunk.
	 */
	char *buf, *pos, *end;
	void *ctx;
	mpstream_reserve_f reserve;
	mpstream_alloc_f alloc;
	mpstream_error_f error;
	void *error_ctx;
	bool from;
};

void
mpstream_init(struct mpstream *stream, void *ctx,
	      mpstream_reserve_f reserve, mpstream_alloc_f alloc,
	      mpstream_error_f error, void *error_ctx);

void
mpstream_reset(struct mpstream *stream);

void
mpstream_reserve_slow(struct mpstream *stream, size_t size);

static inline void
mpstream_flush(struct mpstream *stream)
{
	if (stream->from && stream->error) {
		const char *err = "Immutable 'mpstream' buffer";
		stream->error(stream->error_ctx, err, strlen(err));
	}
	stream->alloc(stream->ctx, stream->pos - stream->buf);
	stream->buf = stream->pos;
}

static inline char *
mpstream_reserve(struct mpstream *stream, size_t size)
{
	if (stream->from && stream->error) {
		const char *err = "Immutable 'mpstream' buffer";
		stream->error(stream->error_ctx, err, strlen(err));
	}
	if (stream->pos + size > stream->end)
		mpstream_reserve_slow(stream, size);
	return stream->pos;
}

static inline void
mpstream_advance(struct mpstream *stream, size_t size)
{
	if (stream->from && stream->error) {
		const char *err = "Immutable 'mpstream' buffer";
		stream->error(stream->error_ctx, err, strlen(err));
	}
	assert(stream->pos + size <= stream->end);
	stream->pos += size;
}

void
mpstream_reserve_slow(struct mpstream *stream, size_t size);

void
mpstream_reset(struct mpstream *stream);

char *
mpstream_encode_array(struct mpstream *stream, uint32_t size);

char *
mpstream_encode_map(struct mpstream *stream, uint32_t size);

char *
mpstream_encode_uint(struct mpstream *stream, uint64_t num);

char *
mpstream_encode_int(struct mpstream *stream, int64_t num);

char *
mpstream_encode_float(struct mpstream *stream, float num);

char *
mpstream_encode_double(struct mpstream *stream, double num);

char *
mpstream_encode_str(struct mpstream *stream, const char *str, uint32_t len);

char *
mpstream_encode_bin(struct mpstream *stream, const char *bin, uint32_t len);

char *
mpstream_encode_nil(struct mpstream *stream);

char *
mpstream_encode_bool(struct mpstream *stream, bool val);

char *
mpstream_encode_ext(struct mpstream *stream, const char *ext, uint32_t len,
		    uint8_t type);

struct mpstream_decode_t {
	enum mp_type type;
	union {
		/* info about float/double/boolean */
		float fval;
		double dval;
		bool bval;
		/* info about int/uint */
		uint64_t uval;
		int64_t  ival;
		/* info about str/bin */
		struct {
			uint32_t size;
			const char *val;
		} sval;
		/* info about extension */
		struct {
			uint32_t size;
			const char *val;
			uint8_t type;
		} eval;
		/* info about int/map */
		uint32_t size;
	};
};

struct mpstream_iter {
	struct mpstream *stream;
	const char *pos;
	struct mpstream_decode_t obj;
};

void
mpstream_iter_init(struct mpstream *stream, struct mpstream_iter *iter);

int
mpstream_skip(struct mpstream_iter *iter);

struct mpstream_decode_t *
mpstream_next(struct mpstream_iter *iter);


#if defined(__cplusplus)
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* MSGPUCK_MPSTREAM_H_INCLUDED */
