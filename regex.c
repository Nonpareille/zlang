
#include <cstring>

struct RE {
  uint32_t size;
};

enum NFAState {
  BRANCH, /// <- start of branch, '(', or '|'
  PIECE, ///<- repetition, '*', '+', or '{,}', and '?'
  ATOM, /// <- character, char-string, or metachar
  CLASS, /// <- character class, '[' or '[^', till ']'
  TAIL /// <- end of branch,  ')'
}

bool match(RE expression, const char * test) {
  if (expression.size == 0)
    return strlen(test) == 0;

  return false;
}

static bool is_atom(char * l, char start, char end) {
  char * l = c;
  while (*(++l) != end) {
    char a = *l;
    if (a == '\\') {
      if (*(++l) == '\0') {
        return true;
      }
    } else if (a == '\0' || a == start) {
      return true;
    }
  }
  return false;
}

static size_t escaped_length(const char * escape) {
  if (*escape != '\\') {
    return 0;
  }
  char * l = escape;
  switch (*(++l)) {
    case 'u': {
      // unicode codepoint
    } break;
    case '': {
      //
    } break;
    default: {
      //
    } break;

  }
}

static size_t unicode_length(const char * unicode) {
}

static size_t regex_length(const char * pattern) {
  size_t result = 0;
  
  char * c = pattern;
  while (*c) {
    switch (*c++) {
      case '(': case '|': {
        // branch
        result += sizeof(BranchNode);
        in_atom = false;
      } break;
      case '*': case '+': {
        if (*c == '?') {
          // consume the lazy flag
          ++c;
        }
        // captured piece
        result += sizeof(PieceNode);
        in_atom = false;
      } break;
      case '?': {
        // optional piece
        result += sizeof(PieceNode);
        in_atom = false;
      } break;
      case '{': {
        if (is_atom(c, '{', '}')) {
          // atom
          if (!in_atom) {
            result += sizeof(AtomNode);
            in_atom = true;
          } else {
            result += sizeof(char);
          }
        }
        // repeated piece
        result += sizeof(PieceNode);
        in_atom = false;
      } break;
      case '\\': {
        // escaped atom
        if (!in_atom) {
          result += sizeof(AtomNode);
          in_atom = true;
        } else {
          result += escaped_length(c) * sizeof(char);
        }
      } break;
      case '.': case '^': case '$': {
        // metachar atom
        result += sizeof(MetaAtomNode);
        in_atom = false;
      } break;
      case '[': {
        if (is_atom(c, '[', ']')) {
          // atom
          if (!in_atom) {
            result += sizeof(AtomNode);
            in_atom = true;
          } else {
            result += sizeof(char);
          }
        }
        // class
        result += sizeof(ClassNode);
        in_atom = false;
      } break;
      case ')': {
        // tail
      } break;
      default: {
        // atom
        if (!in_atom) {
          result += sizeof(AtomNode);
          in_atom = true;
        } else {
          result += unicode_length(c) * sizeof(char);
        }
      }
    }
  }

  return result;
}

RE regex(const char * pattern/*, REOptions options*/) {
  RE result = {0};

  return result;
}

/*
UNIT(RE) {
  WHEN("Regular expression is empty or default") {
    GIVEN(null, regex());
    GIVEN(empty, regex(""));
    THEN("should only match empty strings"){
      ASSERT_TRUE(match(null, ""));
      ASSERT_TRUE(match(empty, ""));
      ASSERT_FALSE(match(null, "\0"));
      ASSERT_FALSE(match(empty, "\0"));
      ASSERT_FALSE(match(null, EOF));
      ASSERT_FALSE(match(empty, EOF));
    }
    
    THEN("should always fail advance") {
    }
    
  }
}
*/

