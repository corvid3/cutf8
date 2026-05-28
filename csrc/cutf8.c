#include "utf8.h"

static inline bool
is_ascii(char const in)
{
  enum : unsigned int
  {
    non_ascii_bit = 0b10000000U,
  };

  bool out = ((unsigned)in & non_ascii_bit) == 0;

  return out;
}

static inline bool
is_4byte(unsigned char const in)
{
  enum : unsigned int
  {
    mask = 0b11111000,
    marker = 0b11110000,
  };

  return ((in & mask) ^ marker) == 0;
}

static inline bool
is_3byte(unsigned char const in)
{
  enum : unsigned int
  {
    mask = 0b11110000,
    marker = 0b11100000,
  };

  return ((in & mask) ^ marker) == 0;
}

static inline bool
is_2byte(unsigned char const in)
{
  enum : unsigned int
  {
    mask = 0b11100000,
    marker = 0b11000000,
  };

  return ((in & mask) ^ marker) == 0;
}

inline size_t
cutf8_utf8_glyph_size(char const* in)
{
  if (is_4byte(*in))
    return 4;
  if (is_3byte(*in))
    return 3;
  if (is_2byte(*in))
    return 2;
  if (is_ascii(*in))
    return 1;
  return -1U;
}

size_t
cutf8_utf32_to_utf8_bytesize(cutf8_glyph c)
{
  enum
  {
    max_1 = 0x7F,
    max_2 = 0x7FF,
    max_3 = 0xFFFF,
    max_4 = 0x10FFFF,
  };

  if (c <= max_1)
    return 1;
  if (c <= max_2)
    return 2;
  if (c <= max_3)
    return 3;
  if (c <= max_4)
    return 4;

  return -1;
}

size_t
cutf8_utf8_at_byteidx(struct cutf8_string_view str, size_t idx)
{
  unsigned bytesize = 0;

  while (bytesize < str.bytesize) {
    if (idx == 0)
      return bytesize;

    unsigned const addend = cutf8_utf8_glyph_size(&str.str[bytesize]);
    if (addend == -1U)
      return -1U;

    bytesize += addend;

    idx--;
  }

  return -1;
}

bool
cutf8_glyph_to_utf8(cutf8_glyph in, char (*out)[4], size_t* len)
{
  enum
  {
    max_1 = 0x7F,
    max_2 = 0x7FF,
    max_3 = 0xFFFF,
    max_4 = 0x10FFFF,
  };

  if (in > max_4)
    return false;

  if (in <= max_1) {
    (*out)[0] = (char)in;
    if (len)
      *len = 1;
    return true;
  }

  if (in <= max_2) {
    (*out)[0] = (char)(0b11000000U | ((in >> 6U) & 0b00011111U));
    (*out)[1] = (char)(0b10000000U | (in & 0b00111111U));
    if (len)
      *len = 2;
    return true;
  }

  if (in <= max_3) {
    (*out)[0] = (char)(0b1110000U | ((in >> 12U) & 0b00001111U));
    (*out)[1] = (char)(0b10000000U | ((in >> 6U) & 0b00111111U));
    (*out)[2] = (char)(0b10000000U | (in & 0b00111111U));
    if (len)
      *len = 3;
    return true;
  }

  if (in <= max_4) {
    (*out)[0] = (char)(0b1111000U | ((in >> 18U) & 0b00000111U));
    (*out)[1] = (char)(0b10000000U | ((in >> 12U) & 0b00111111U));
    (*out)[2] = (char)(0b10000000U | ((in >> 6U) & 0b00111111U));
    (*out)[3] = (char)(0b10000000U | ((in) & 0b00111111U));
    if (len)
      *len = 4;
    return true;
  }

  return false;
}

inline cutf8_glyph
cutf8_utf8_to_glyph(char const* in, size_t const max_size)
{
  enum : unsigned int
  {
    MASK_4_HEAD = 0b111,
    MASK_3_HEAD = 0b1111,
    MASK_2_HEAD = 0b11111,

    HEAD_4_OFF = 18,
    HEAD_3_OFF = 12,
    HEAD_2_OFF = 6,

    MIN_4 = 0x10000,
    MAX_4 = 0x10FFFF,

    MIN_3 = 0x800,
    MIN_2 = 0x80,

    CONTINUATION_BIT = 0b10000000,
  };

  if (max_size == 0)
    return 0;

  if (max_size == 1)
    goto try_ascii;
  if (max_size == 2)
    goto try2;
  if (max_size == 3)
    goto try3;

#define ISCONT(in)                                                             \
  if (((unsigned)((in) >> 6U) ^ 0b10U) != 0U)                                  \
    return -1U;
#define GETCONT(in, off)                                                       \
  if ((((unsigned char)(in) & 0b11000000U) ^ 0b10000000U) != 0U)               \
    return -1U;                                                                \
  out |= ((unsigned char)(in) & 0b00111111U) << (off);
#define SETHEAD(in, mask, off) (out |= ((unsigned)(in) & (mask)) << (off))

  if (is_4byte(*in)) {
    cutf8_glyph out = 0;

    SETHEAD(in[0], MASK_4_HEAD, 18U);
    GETCONT(in[1], 12U);
    GETCONT(in[2], 6U);
    GETCONT(in[3], 0U);

    if (out < MIN_4 || out > MAX_4)
      return -1;

    return out;
  }

try3:
  if (is_3byte(*in)) {
    cutf8_glyph out = 0;

    SETHEAD(in[0], MASK_3_HEAD, 12U);
    GETCONT(in[1], 6U);
    GETCONT(in[2], 0U);

    if (out < MIN_3)
      return -1;

    return out;
  }

try2:
  if (is_2byte(*in)) {
    cutf8_glyph out = 0;

    SETHEAD(in[0], MASK_2_HEAD, 6U);
    GETCONT(in[1], 0U);

    if (out < MIN_2)
      return -1;

    return out;
  }

try_ascii:
  if (!is_ascii(*in))
    return -1;

  return *in;
}

size_t
cutf8_utf8str_hash(struct cutf8_string_view const str)
{
  enum
  {
    POLYNOMIAL = 31,
  };

  unsigned h = 0;
  for (unsigned i = 0; i < str.bytesize; i++)
    h = h * POLYNOMIAL + str.str[i];
  return h;
}

int
cutf8_utf8str_cmp(struct cutf8_string_view const lhs,
                  struct cutf8_string_view const rhs)
{
  if (lhs.bytesize < rhs.bytesize)
    return -1;

  if (lhs.bytesize > rhs.bytesize)
    return 1;

  for (unsigned i = 0; i < lhs.bytesize; i++) {
    if (lhs.str[i] < rhs.str[i])
      return -1;

    if (lhs.str[i] > rhs.str[i])
      return 1;
  }

  return 0;
}

cutf8_glyph
cutf8_utf8str_at(struct cutf8_string_view const str, size_t idx)
{
  unsigned i = 0;
  unsigned size = 0;

  while (idx < str.bytesize) {
    if (size == idx)
      return cutf8_utf8_to_glyph(&str.str[str.bytesize], str.bytesize - i);

    unsigned const addend = cutf8_utf8_glyph_size(&str.str[i]);
    if (addend == -1U)
      return -1;
    idx += addend;
    size++;
  }

  return -1;
}

int
cutf8_utf8str_length(struct cutf8_string_view const str)
{
  unsigned idx = 0;
  unsigned size = 0;
  while (idx < str.bytesize) {
    unsigned const addend = cutf8_utf8_glyph_size(&str.str[idx]);
    if (addend == -1U)
      return -1;
    idx += addend;
    size++;
  }

  return (signed)size;
}
