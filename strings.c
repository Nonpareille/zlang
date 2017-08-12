#include <string.h>

#define SPLIT_STRING_LEN(str) str, sizeof(str)

struct String {
  char* c_str;
  size_t len;
};

struct StringBuilder {
  char *begin, *end, *cap;
};

void GrowStringBuilder(StringBuilder *b, size_t len) {
  if (b == 0) return;
  if (b->cap - b->end > len) return;
  size_t new_cap = (b->end - b->begin + len) * 1.5f;
  // TODO:(nick) switch to buddy block allocator
  char *new_buf = (char*)malloc(new_cap);
  b->cap = new-buf + new_cap;
  if (b->begin != 0) {
    // TODO:(nick) move memmove to libc layer.
    memmove(new_buf, b->begin, b->end - b->begin);
    free(b->begin);
  }
  b->end = new_buf + (b->end - b->begin);
  b->begin = new_buf;
}

StringBuilder AppendCString(StringBuilder* b, 
    char* str, size_t len) {
  GrowStringBuilder(b, len);
  // TODO:(nick) move memmove to libc layer.
  memmoce(b->end, str, len);
  return b;
}

//TODO:(nick) base32 and base64
char kValueToDigit16[] = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A',
  'B', 'C', 'D', 'E', 'F'
}

StringBuilder* AppendUInt32(StringBuilder* b, uint32_t u,
    uint8_t radix) {
  if (u == 0) {
    // 0 is a special case. log and loop don't work.
    GrowStringBuilder(b, 1);
    b->end++ = '0';
    return b;
  }
  // TODO:(nick) move log to libc layer.
  // TODO:(nick) store 1/log(radix) up to 16
  size_t len = (size_t)
    (log((double)u) / log((double)radix)) + 1;
  GrowStringBuilder(b, len);
  char *old = b->end;
  char *new = b->end += len;
  while (u) {
    *new-- = kValueToDigit16[u % radix];
    u /= radix;
  }
  return b;
}

