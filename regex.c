
#include <cstring>

struct RE {
  uint32_t size;
};

enum RENodeType {
  Invalid = -1,
  BranchNode,
  PieceNode,
  AtomNode,
  ClassNode,
  TailNode,
  AnchorNode,
  BackReferenceNode,
  KeepOutNode,
  End
};

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

static bool is_upper(char u) {
  return (u >= 'A' && u <= 'Z');
}

static bool is_alpha(char a) {
  return is_lower(a) || is_upper(a);
}

static bool is_digit(char d) {
  return (d >= '0' && d <= '9');
}

static bool is_word(char w) {
  return is_alpha(w) || is_digit(w) || (w == '_');
}

static bool is_hex_digit(char h) {
  return is_digit(h) 
      || (h >= 'a' && h <= 'f') || (h >= 'A' && h <= 'F');
}

/** Consumes an escaped sequence.
 * @detail
 *   Takes a pointer to the beginning of an escaped 
 * sequence, must be a backslash '\' character. This 
 * procedure attempts to find a special escape sequence, 
 * and if not found, reverts to treating the input as a 
 * single escaped atom. The special escape sequence 
 * currently supported are as follows:
 *   - backreferences "\1-99,\g-99-99,\g{-99-99}"
 *   - named backreferences "\g{name},\k{name}"
 *   - posix/unicode character classes 
 *     "\pC,\PC,\p{class},\P{class}"
 *   - unicode hex codepoints "\x00-FF,\x{00-FFFF}"
 *   - raw strings "\Q raw string *+\<...\E"
 *   - control characters "\cA-Z"
 *   - keep outs "\K"
 *   - character class shorthands
 *     "\d,D,h,H,l,L,n,N,s,S,u,U,v,V,w,W,X"
 *   - boundary anchors "\A,Z,G,b,B,>,<"
 *
 * @param escape [inout] Points to the beginning of the 
 *   sequence, on return points one past the end.
 * 
 * @return The node type consumed.
 */
static RENodeType consume_escape(char ** escape) {
  if (**escape != '\\') {
    return RENodeType::Invalid;
  }

  char * l = *escape + 1;
  switch (*l++) {

    // backreference, accepts "\\[1-9]\d?"
    case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': {
      // backreference one or two digits
      if (is_digit(*(++l))) {
        // consume "\\\d\d" (+3)
        *escape += 3;
      } else {
        // consume "\\\d" (+2)
        *escape += 2;
      }
      return RENodeType::BackReferenceNode;
    } break;

    // backreference,
    // accepts "\\g(\{(-?\d\d?|[a-zA-Z_]\w*)\}|-?\d\d?)?
    case 'g': {
      char c = *l;
      char d = *l + 1;

      if (c == '{' && d != '\0') {
        char e = *l + 2;
        if (d == '-' && is_digit(e)) {
          char f = *l + 3;
          if (is_digit(f)) {
            // consume "\\g\{-\d\d\}" (+7)
            *escape += 7;
          } else {
            // consume "\\g\{-\d\}" (+6)
            *escape += 6;
          }
          return RENodeType::BackReferenceNode;
        } else if (is_digit(d)) {
          if (is_digit(e)) {
            // consume "\\g\{\d\d\}" (+6)
            *escape += 6;
          } else {
            // consume "\\g\{\d\}" (+5)
            *escape += 5;
          }
          return RENodeType::BackReferenceNode;
        } else if (is_alpha(d) || d == '_') {
          while (*(++l)) {
            c = d;
            d = *l;
            if (d == '}' && c != '{') {
              // consume "\\g\{[a-zA-Z_]\w*\}" (=l+1)
              *escape = l+1;
              return RENodeType::BackReferenceNode;
            } else if (!is_word(d)) {
              // fails, this is a single char escape
              break;
            }
          }
        }
      } else if (c == '-' && is_digit(d)) {
        if (is_digit(e)) {
          // consume "\\g-\d\d" (+5)
          *escape += 5;
        } else {
          // consume "\\g-\d" (+4)
          *escape += 4;
        }
        return RENodeType::BackReferenceNode;
      } else if (is_digit(c)) {
        if (is_digit(d)) {
          // consume "\\g\d\d" (+4)
          *escape += 4;
        } else {
          // consume "\\g\d" (+3)
          *escape += 3;
        }
        return RENodeType::BackReferenceNode;
      }

      // consume "\\g" (+2)
      *escape += 2;
      return RENodeType::AtomNode;
    } break;

    // named backreference,
    // accepts "\\k(\{[a-zA-Z_]\w*\})?
    case 'k': {
      ++l;
      if (*l == '{' && (is_alpha(*(++l)) || *l == '_')) {
        while (*(++l)) {
          if (*l == '}' && *(l-1) != '{') {
            // consume "\\k\{[a-zA-Z_]\w*\}" (=l+1)
            *escape = l+1;
            return RENodeType::BackReferenceNode;
          } else if (!is_word(*l)) {
            //fails, this is a single char escape
            break;
          }
        }
      }

      // consume "\\k" (+2)
      *escape += 2;
      return RENodeType::AtomNode;
    } break;

    // posix/unicode character class 
    // accepts "\\[pP](\{[\w\-]+\}|[CLMNPSZ])?"
    case 'p': case 'P': {
      ++l;
      char c = *l;

      if (c == '{') {
        // all unicode/posix character classes are composed
        // of word characters and '-'
        while (*(++l)) {
          c = *l;
          if (c == '}' && *(l-1) != '{') {
            // consume "\\[pP]\{[\w\-]+\}" (=l+1)
            *escape = l+1;
            return RENodeType::ClassNode;
          } else if (!is_word(c) && c != '-') {
            // fails, this is a single char escape
            break;
          }
        }
      } else if (c == 'C' || (c >= 'L' && c <= 'N')
              || c == 'P' || c == 'S' || c == 'Z') {
        // consume "\\[pP][CLMNPSZ]" (+3)
        *escape += 3;
        return RENodeType::ClassNode;
      }

      // consume "\\[pP]" (+2)
      *escape += 2;
      return RENodeType::AtomNode;
    } break;

    // hex escape, accepts "\\x(\{\h{2,4}\}|\h\h)?"
    case 'x': {
      // lookahead
      ++l;
      char c = *l;

      if (c == '{') {
        // 2-4 digits representing unicode code point
        size_t num_digits = 0;
        while (*(++l)) {
          ++num_digits;
          if (*l == '}' && num_digits > 2) {
            // consume "\\x\{\h{2,4}\}" (=l+1)
            *escape = l+1;
            return RENodeType::AtomNode;
          } else if (!is_hex_digit(*l)
                  || num_digits >= 4) {
            // fails, this is a single char escape
            break;
          }
        }
      } else if (is_hex_digit(c) && is_hex_digit(*(++l))) {
        // consume "\\x\h\h" (+4)
        *escape += 4;
        return RENodeType::AtomNode;
      }

      // consume "\\x" (+2)
      *escape += 2;
      return RENodeType::AtomNode;
    } break;

    // literal escape, accepts "\\Q(.*\\E)?"
    case 'Q': {
      // literal escape until "\E"
      while (*(++l)) {
        if (*l == '\\' && *(l+1) == 'E') {
          // consume "\\Q.*\\E" (=l+2)
          *escape = l+2;
          return RENodeType::AtomNode;
        }
      }

      // consume "\\Q" (+2)
      *escape += 2;
      return RENodeType::AtomNode;
    } break;

    // control character accepts "\\c[a-zA-Z]?"
    case 'c': {
      if (is_alpha(*(++l))) {
        // consume "\\c[a-zA-Z]" (+3)
        *escape += 3;
        return RENodeType::AtomNode;
      }

      // consume "\\c" (+2)
      *escape += 2;
      return RENodeType::AtomNode;
    } break;

    case 'K' {
      // keep left out of match
      // consume "\\K" (+2)
      *escape += 2;
      return RENodeType::KeepOutNode;
    } break;

    case 'd': case 'D': case 'h': case 'H': case 'l': 
    case 'L': case 'n': case 'N': case 's': case 'S': 
    case 'u': case 'U': case 'v': case 'V': case 'w': 
    case 'W': case 'X': {
      // character class shorthand
      // consume "\\[dDhHlLnNsSuUvVwWX]" (+2)
      *escape += 2;
      return RENodeType::ClassNode;
    } break;

    case 'A': case 'Z': case 'G': case 'b': case 'B': 
    case '>': case '<': {
      // anchor/boundary sequence
      // consume "\\[AZGbB><]" (+2)
      *escape += 2;
      return RENodeType::AnchorNode;
    } break;

    default: {
      // consume "\\." (+2)
      *escape += 2;
      return RENodeType::AtomNode;
    } break;
  }
}

static size_t unicode_length(const char * unicode) {
}

static size_t node_length(RENodeType type, 
                          const char * c,
                          const char * end = nullptr) {
  switch (type) {
    case RENodeType::BranchNode: {
      //
    } break;
    case RENodeType::PieceNode: {
      //
    } break;
    case RENodeType::AtomNode: {
      //
    } break;
    case RENodeType::ClassNode: {
      //
    } break;
    case RENodeType::TailNode: {
      //
    } break;
    case RENodeType::AnchorNode: {
      //
    } break;
    case RENodeType::BackReferenceNode: {
      //
    } break;
    case RENodeType::KeepOutNode: {
      //
    } break;
  }
}

static RENodeType consume_subpattern(char ** pattern) {
  size_t result = 0;
  
  char * l = *pattern;
  switch (*l) {
    // new branch or special block, 
    // accepts "(\(|\|)(\?...)?"
    case '(': case '|': {
      if (*(l+1) == '?') {
        // special block
        return consume_special(pattern);
      }
      // consume "(\(|\|)"
      *patern += 1;
      return RENodeType::BranchNode;
    } break;

    // capture piece accepts "(\*|\+|\?)(\?)?"
    case '*': case '+': case '?': {
      if (*c == '?') {
        // consume the lazy flag
        ++c;
      }
      // captured piece
      return RENodeType::PieceNode;
    } break;

    case '{': {
      char * l = c;
      bool is_atom = false;
      while (*(++l)) {
        if (*l == '}' && *(l-1) != '{') {
          // consume "\{\d+,?[\d]*\}" (=l)
          *pattern = l;
          return RENodeType::PieceNode;
          break;
        } else if (!is_digit(*l) && *l != ',') {
          break;
        }
      }
      // atom
      return RENodeType::AtomNode;
    } break;

    case '\\': {
      // escape
      return consume_escape(&pattern);
    } break;

    case '.': {
      // pseudo class
      return RENodeType::ClassNode;
    }

    case '^': case '$': {
      // anchor
      return RENodeType::AnchorNode;
    } break;

    case '[': {
      if (is_atom(c, '[', ']')) {
        // atom
        return RENodeType::AtomNode;
      }
      // class
      return RENodeType::ClassNode;
    } break;

    case ')': {
      // tail
      return RENodeType::TailNode;
    } break;

    default: {
      // atom
      return RENodeType::AtomNode;
    }
  }
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

