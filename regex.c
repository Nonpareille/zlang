
#include <cstring>

struct RE {
  uint32_t size;
};

enum RENodeType {
  Invalid = -1,
  ClassNode,
  TailNode,
  BackReferenceNode,
  AtomNode,
  PieceNode, // <- repetition or capture
  KeepOutNode,
  AnchorNode,
  BranchNode,
  End
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
  return (d >= '0' && d <= '9');
}

static bool is_word(char w) {
  return is_alpha(w) || is_digit(w) || (w == '_');
}

static bool is_hex_digit(char h) {
  return is_digit(h)
      || (h >= 'a' && h <= 'f') || (h >= 'A' && h <= 'F');
}

static bool consume_name(char ** name, char * term) {
  if (!is_alpha(**name) && **name != '_') {
    return false;
  }

  char *l = *name;
  while (*(++l)) {
    for (char *t = term; t != '\0'; ++t) {
      if (*l == *t) {
        *name = l;
        return true;
      }
    }
    if (!is_word(*l)) {
      return false;
    }
  }
  return false;
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
        } else if (consume_name(&l, "}") {
          // consume "\\g\{(?&consume_name)\}" (=l+1)
          *escape = l+1;
          return RENodeType::BackReferenceNode;
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
    // accepts "\\k([{<'][a-zA-Z_]\w*\})?
    case 'k': {
      ++l;
      if (*l == '{' || *l == '<') {
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

static RENodeType consume_group(char ** special) {
  if (**special != '(' && **(special+1) != '?') {
    return RENodeType::Invalid;
  }

  char * l = *(special+2);

  switch (*l) {
    // comment,
    // accepts "\(\?#.*\)"
    case '#': {
    } break;

    // named group,
    // accepts "\(\?(P[=<]|[<'])[a-zA-Z_]\w+[>']?"
    case 'P': case '<': case '\'': {
    } break;

    // mode modifying group,
    // accepts "\(\?[imnsx]*(-[imnsx])?:?"
    case 'i': case 'm': case 'n': case 's': case 'x': {
    } break;

    // mode reset group,
    // accepts "\(\?^:?"
    case '^': {
    } break;

    // special group,
    // accepts "\(\?[|>=!:]"
    case '|': case '>': case '=': case '!': case ':': {
    } break;

    // lookbehind group
    // accepts "\(\?<[=!]"
    case '<': {
    } break;

    // conditional branch group
    // accepts "\(\?\(([<'][a-zA-Z_]\w+[>']\)|[+-]?\d+\)"
    case '(': {
    } break;

    // not a supported special group,
    // accepts "\("
    default: {
      // consume "\(" (+1)
      *special += 1;
      return RENodeType::BranchNode;
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
    // accepts "\((\?(?&consume_special))?"
    case '(': {
      if (*(l+1) == '?') {
        // special block
        return consume_special(pattern);
      }
      // consume "\(" (+1)
      *patern += 1;
      return RENodeType::BranchNode;
    } break;

    case '|': {
      // new branch
      // consume "\|" (+1)
      *pattern += 1;
      return RENodeType::BranchNode;
    } break;

    // capture piece, accepts "(\*|\+|\?)(\?)?"
    case '*': case '+': case '?': {
      if (*(l+1) == '?') {
        // consume "(\*|\+|\?)\?" (+2)
        *pattern += 2;
      } else {
        // consume "(\*|\+|\?)" (+1)
        *pattern += 1;
      }
      return RENodeType::PieceNode;
    } break;

    // repeated piece, accepts "{(\d+,?[\d]*})?
    case '{': {
      if (is_digit(*(++l))) {
        bool found_comma = false;
        while (*(++l)) {
          if (*l == '}' && *(l-2) != '{') {
            // consume "{\d+,?[\d]*}" (=l+1)
            *pattern = l+1;
            return RENodeType::PieceNode;
            break;
          } else if (!is_digit(*l)) {
            if (found_comma || *l != ',') {
              break;
            }
            found_comma = true;
          }
        }
      }
      // consume "{" (+1)
      *pattern += 1;
      return RENodeType::AtomNode;
    } break;

    case '\\': {
      // escape sequence
      // consumes "\\(?&consume_escape)"
      return consume_escape(pattern);
    } break;

    case '.': {
      // pseudo class
      // consume "\." (+1)
      *pattern += 1;
      return RENodeType::ClassNode;
    }

    case '^': case '$': {
      // anchor
      // consume "(\^|\$)" (+1)
      *pattern += 1;
      return RENodeType::AnchorNode;
    } break;

    case '[': {
      // character class,
      // accepts "[(?consume_class)
      return consume_class(pattern);
    } break;

    case ')': {
      // tail of a branch
      // consume "\)" (+1)
      *pattern += 1;
      return RENodeType::TailNode;
    } break;

    default: {
      // consume "." (+1)
      *pattern += 1
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

