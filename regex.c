
#include <cstring>

struct NFA {
  uint32_t size;
};

enum NFAState {
  BRANCH, /// <- start of branch, BOL, '(', or '|'
  PIECE, ///<- 
  ATOM, /// <- repeatable element, any character or class
  CLASS, /// <- character class, '['
  TAIL /// <- end of branch, EOL or ')'
}

uint32_t nfa_len(char* pattern) {
  int result = 0;
  NFAState state = NFAState::START;
  
  //TODO(nick): can we precalculate the number of branches?
  char* branch_start_stack[MAX_BRANCHES_RECURSE];
  int cur_branch = 0;

  char* c = pattern;
  while (c != '\0') {
    switch (state) {
      case NFAState::BRANCH: {
        result += sizeof(NFABranchNode);
        if (*c == '(') {
          // opening another branch
          ++c;
        } else {
          state = NFAState::PIECE;
        }
      } break;
      case NFAState::PIECE: {
        result 
      } break;
      case NFAState::ATOM: {
      } break;
      case NFAState::CLASS: {
      } break; 
      case NFAState::TAIL: {
        result += sizeof(NFATailNode);

        state = NFAState::BRANCH;
      } break; 
    }
  }
}

NFA build_nfa(char* pattern) {
  NFA result;

  result.size = nfa_len(pattern);
}

int main(void) {
  char * pattern = "";

  int pattern_len = strlen(pattern);
  
  NFA nfa_size = build_nfa(pattern);
}
