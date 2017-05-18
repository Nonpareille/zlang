#include <stdio.h>

#define TOKEN_START_SIZE 20
#define STRING_GROW_RATE 0.75

enum TokenType {

  START,
  END,

  NUM_TOKEN_TYPE
};

struct Token {
  char* string;
  TokenType type;
};

struct TokenListNode {
  Token token;
  TokenListNode* next;
};

struct TokenMatch {
  bool match;
  Token token;
};

const Token start_token = { "", TokenType::START };
const Token end_token = { "", TokenType::END };

int get_next_octet_utf8(FILE *src) {
  int c = fgetc(src);
  if (c < 0x80 || 0xBF < c) {
    //ERROR: invalid utf8 octet
    return 0;
  } else if (c == EOF) {
    //ERROR: unexpected EOL
    return 0;
  } 
  return c;
}

bool string_append(char** str, int* cap, int* len, int c) {
  if (*len + 2 >= *cap) {
    int new_cap = (1.0 + STRING_GROW_RATE) * (*len + 2);
    char* new_str = calloc(new_cap, sizeof(char));
    memcpy(str, new_str, *len * sizeof(char));
    free(str);
    *str = new_str;
    *cap = new_cap;
  }
  
  *str[*len++] = (char)(c);
}

TokenMatch match_token(char* tok, int len) {
  
}

int main(void)
{
  FILE* src = fopen("test.z", "r+");

  if (src != nullptr) {
    int cur_token_len = 0;
    int cur_token_cap = TOKEN_START_SIZE;
    char *cur_token = calloc(cur_token_cap, sizeof(char));
    TokenListNode *tok_list, tok_head;
    int num_tokens = 0;
    tok_list = &tok_head;
    tok_head.token = start_token;
    tok_head.next = nullptr;
    num_tokens++;
    int c = fgetc(src);
    while (c != EOF) {
      if (c < 0x7F) {
        string_append(&cur_token, &cur_token_cap, 
                      &cur_token_len, c);
      } else if (0xC0 < c && c < 0xDF) {
        int d;
        if ((d = get_next_octet_utf8(src)) {
          string_append(&cur_token, &cur_token_cap, 
                        &cur_token_len, c);
          string_append(&cur_token, &cur_token_cap, 
                        &cur_token_len, d);
        }
      } else if (0xE0 < c && c < 0xEF) {
        int d, e;
        if ((d = get_next_octet_utf8(src) &&
            (e = get_next_octet_utf8(src)) {
          string_append(&cur_token, &cur_token_cap, 
                        &cur_token_len, c);
          string_append(&cur_token, &cur_token_cap, 
                        &cur_token_len, d);
          string_append(&cur_token, &cur_token_cap, 
                        &cur_token_len, e);
        }
      } else if (0xF0 < c && c < 0xF7) {
        int d, e, f;
        if ((d = get_next_octet_utf8(src) &&
            (e = get_next_octet_utf8(src)
            (f = get_next_octet_utf8(src)) {
          string_append(&cur_token, &cur_token_cap, 
                        &cur_token_len, c);
          string_append(&cur_token, &cur_token_cap, 
                        &cur_token_len, d);
          string_append(&cur_token, &cur_token_cap, 
                        &cur_token_len, e);
          string_append(&cur_token, &cur_token_cap, 
                        &cur_token_len, f);
        }
      } else {
        //ERROR: invalid utf8 
      }
      TokenMatch match = match_token(cur_token, 
                                     cur_token_len);
      if (match.match) {
        //INFO: matched token
        tok_list->next = malloc(sizeof(TokenListNode));
        tok_list->next.token = match.token;
        tok_list = tok_list->next;
        num_tokens++;

        cur_token_len = 0;
      }

      c = fgetc(src);
    }
    if (cur_token_len != 0) {
      //ERROR: unexpected end of file
    }
    free(cur_token);

    tok_list->next = malloc(sizeof(TokenListNode));
    tok_list->next.token = end_token;
    tok_list->next->next = nullptr;
    num_tokens++;
    
  }
  return 0;
}

