
enum TokenType {
  TOKENTYPE_whitespace,

  // operators
  TOKENTYPE_binary_op,
  TOKENTYPE_unary_op,

  // constants
  TOKENTYPE_number_const,

  TOKENTYPE_COUNT
};

String kTokenTypeToString[] = {
  {SPLIT_STRING_LEN("whitespace")},

  {SPLIT_STRING_LEN("binary_op")},
  {SPLIT_STRING_LEN("unary_op")},

  {SPLIT_STRING_LEN("number_const")}
}

struct TokenList {
  size_t count;
  TokenType *types;
  String *values;
  unsigned *lines, *cols;
};

String TokenTypeToString(TokenType type) {
  if (type >= TOKENTYPE_COUNT) {
    String undefined = {SPLIT_STRING_LEN("undefined")};
    return undefined;
  }

  return kTokenTypeToString[type];
}
