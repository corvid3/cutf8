#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint32_t cutf8_glyph;

struct cutf8_string_view
{
  char const* str;
  size_t bytesize;
};

#define cutf8_lit(str_)                                                        \
  (struct cutf8_string_view)                                                   \
  {                                                                            \
    .str = (str_), .bytesize = sizeof(str_)                                    \
  }

bool
cutf8_glyph_to_utf8(cutf8_glyph in, char (*out)[4], size_t* len);
cutf8_glyph
cutf8_utf8_to_glyph(char const* in, size_t max_size);
size_t
cutf8_utf8_glyph_size(char const* in);
size_t
cutf8_utf32_to_utf8_bytesize(cutf8_glyph c);

/* returns the byte index for a given utf8 glyph in a utf8 string */
size_t
cutf8_utf8_at_byteidx(struct cutf8_string_view str, size_t idx);

//* string facilities *//
/* simple, fast hashing algoithm for any arbitary */
size_t cutf8_utf8str_hash(struct cutf8_string_view);

/* numerically compares two strings
 * -1 = lhs < rhs, 1 = lhs > rhs, 0 = lhs == rhs */
int cutf8_utf8str_cmp(struct cutf8_string_view, struct cutf8_string_view);

cutf8_glyph
cutf8_utf8str_at(struct cutf8_string_view, size_t idx);

int cutf8_utf8str_length(struct cutf8_string_view);
