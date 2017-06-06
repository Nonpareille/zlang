
#include <cstring>

#define TYPEENUM(Name, ...) \
struct Name##Type {         \
  enum Name##Enum {         \
    Invalid = -1,           \
    __VA_ARGS__,            \
    End                     \
  } type;                   \
};

struct RE {
  uint32_t size;
};

TYPEENUM(RENode,
    AtomNode,
);

enum NFAState {
  BRANCH, /// <- start of branch, '(', or '|'
  PIECE, ///<- repetition, '*', '+', or '{,}', and '?'
  ATOM, /// <- character, char-string, or metachar
  CLASS, /// <- character class, '[' or '[^', till ']'
  TAIL /// <- end of branch,  ')'
};

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

static bool is_lower(char l) {
  return (l >= 'a' && l <= 'z');
}

static bool is_upper(char l) {
  return (l >= 'A' && l <= 'Z');
}

static bool is_alpha(char a) {
  return is_lower(a) || is_upper(a);
}

static bool is_digit(char d) {
  return (a >= '0' && a <= '9');
}

static bool is_word(char w) {
  return is_alpha(w) || is_digit(w) || (w == '_');
}

static bool is_hex_digit(char h) {
  return is_digit(h) 
      || (h >= 'a' && h <= 'f') || (h >= 'A' && h <= 'F');
}

static 
RENodeType::RENodeTypeEnum consume_escape(char ** escape) {
  if (**escape != '\\') {
    return -1;
  }

  char * l = *escape;
  switch (*(++l)) {

    // backreference, accepts "\\[1-9]\d?"
    case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': {
      // backreference one or two digits
      if (is_digit(*(++l))) {
        *escape += 3;
      } else {
        *escape += 2;
      }
      *in_atom = false;
      return RENodeType::BackReferenceNode;
    } break;

    // backreference,
    // accepts "\\g(\{(-?\d\d?|[\w]*)\}|-?\d\d?)?
    case 'g': {
      ++l;
      char c = *l;

      if (c == '{') {
        c = *(l+1);
        if (c == '-' && is_digit(*(l+2))) {
          if (is_digit(*(l+3))) {
            *escape = l+4;
          } else {
            *escape = l+3;
          }
          return RENodeType::BackReferenceNode;
        } else if (is_digit(c)) {
          if (is_digit(*(l+2))) {
            *escape = l+3;
          } else {
            *escape = l+2;
          }
          return RENodeType::BackReferenceNode;
        } else {
          while (*(++l)) {
            c = *l;
            if (c == '}' && *(l-1) != '{') {
              escape = ++l;
              return RENodeType::BackReferenceNode;
            } else if (!is_word(c)) {
              // fails, this is a single char escape
              break;
            }
          }
        }
      } else if (c == '-' && is_digit(*(l+1))) {
        if (is_digit(*(l+2))) {
          escape = l+3;
        } else {
          escape = l+2;
        }
        return RENodeType::BackReferenceNode;
      } else if (is_digit(c)) {
        if (is_digit(*(l+1))) {
          escape = l+2;
        } else {
          escape = l+1;
        }
        return RENodeType::BackReferenceNode;
      }

      // this is a single character escape, consume "\g"
      escape += 2;
      return RENodeType::AtomNode;
    } break;

    // named backreference,
    // accepts "\\k(\{[a-zA-Z_][\w]*\})?
    case 'k': {
      ++l;
      if (*l == '{' && (is_alpha(*(++l)) || *l == '_')) {
        while (*(++l)) {
          if (*l == '}' && *(l-1) != '{') {
            escape = ++l;
            return RENodeType::BackReferenceNode;
          } else if (!is_word(*l)) {
            //fails, this is a single char escape
            break;
          }
        }
      }

      // this is a single character escape, consume "\k"
      escape += 2;
      return RENodeType::AtomNode;
    } break;

    // posix/unicode character class 
    // accepts "\\(p|P)(\{[\w\-]*\}|[CLMNPSZ])?"
    case 'p': case 'P': {
      ++l;
      char c = *l;

      if (c == '{') {
        // all unicode/posix character classes are composed
        // of word characters and '-'
        while (*(++l)) {
          c = *l;
          if (c == '}' && *(l-1) != '{') {
            escape = ++l;
            return RENodeType::ClassNode;
          } else if (!is_word(c) && c != '-') {
            // fails, this is a single char escape
            break;
          }
        }
      } else if (c == 'C' || (c >= 'L' && c <= 'N')
              || c == 'P' || c == 'S' || c == 'Z') {
        escape = ++l;
        return RENodeType::ClassNode;
      }

      // this is a single character escape, consume '\p'
      escape += 2;
      return RENodeType::AtomNode;
    } break;

    // hex escape, accepts "\\x(\{\h\h{1,3}\}|\h\h)?"
    case 'x': {
      // lookahead
      ++l;

      if (*l == '{') {
        // 2-4 digits representing unicode code point
        size_t num_digits = 0;
        while (*(++l)) {
          ++num_digits;
          if (*l == '}' && num_digits > 2) {
            escape = ++l;
            return RENodeType::AtomNode;
          } else if (!is_hex_digit(*l)
                  || num_digits >= 4) {
            // fails, this is a single char escape
            break;
          }
        }
      } else if (is_hex_digit(*l) && is_hex_digit(*(++l))) {
        // 2 hex digits representing character
        escape = ++l;
        return RENodeType::AtomNode;
      }

      // this is a single character escape, consume '\x'
      escape += 2;
      return RENodeType::AtomNode;
    } break;

    // literal escape, accepts "\\Q(.*\\E)?"
    case 'Q': {
      char * q_begin = l+1;
      // literal escape until "\E"
      while (*(++l)) {
        if (*l == '\\' && *(l+1) == 'E') {
          escape = l+2;
          return RENodeType::AtomNode;
        }
      }

      // this is a single character escape, consume '\Q'
      escape += 2;
      return RENodeType::AtomNode;
    } break;

    // control character accepts "\\c[a-zA-Z]?"
    case 'c': {
      if (is_alpha(*(++l))) {
        escape = ++l;
        return RENodeType::AtomNode;
      }

      // this is a single character escape, consume '\c'
      escape += 2;
      return RENodeType::AtomNode;
    } break;

    case 'K' {
      // keep left out of match
      escaped += 2
      return RENodeType::KeepOutNode;
    } break;

    case 'd': case 'D': case 'h': case 'H': case 'l': 
    case 'L': case 'n': case 'N': case 's': case 'S': 
    case 'u': case 'U': case 'v': case 'V': case 'w': 
    case 'W': case 'X': {
      // character class shorthand
      escaped += 2;
      return RENodeType::ClassNode;
    } break;

    case 'A': case 'Z': case 'G': case 'b': case 'B': 
    case '>': case '<': {
      // anchor/boundary sequence
      escape += 2;
      return RENodeType::AnchorNode;
    } break;

    default: {
      // just an escaped character
      escape += 2;
      return RENodeType::AtomNode;
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
        // escape
        RENodeType::RENodeTypeEnum type = consume_escape(&c);
        result += re_node_size();
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

