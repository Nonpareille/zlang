#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "tokens.c"

#define KB(count) count*1024
#define MB(count) KB(count)*1024
#define GB(count) MB(count)*1024

#define MEMORY_ARENA_SIZE MB(10)
#define MEMORY_ARENA_START_PAGE 64

void PrintTokens(TokenList tokens) {
  StringBuilder b = {};
  for (size_t i = 0; i < tokens.count; ++i) {
    AppendCString(&b, SPLIT_STRING_LEN("<TYPE: "));
    String typename = TokenTypeToString(tokens.types[i]);
    AppendString(&b, typename);
    AppendCString(&b, SPLIT_STRING_LEN(", VALUE: \""));
    String value = tokens.values[i];
    AppendString(&b, value);
    AppendCString(&b, SPLIT_STRING_LEN("\", POS: [line: "));
    AppendInt32(&b, token.lines[i], 10);
    AppendCString(&b, SPLIT_STRING_LEN(", col: "));
    AppendInt32(&b, token.cols[i], 10);
    AppendCString(&b, SPLIT_STRING_LEN("]>\n"));
  }
  write(0, b.begin, b.end-b.begin);
}

int main(void) {
  int result = 0;

  long const page_size = sysconf(_SC_PAGESIZE);

  char * src_path = "test.z";
  struct stat64 src_st = {};
  if (stat64(src_path, &src_st) == -1) {
    result = errno;
    goto exit_now;
  }

  int src_fd = open(src_path, O_RDONLY);
  if (src_fd == -1) {
    result = errno;
    goto exit_now;
  }
  
  void *mem_arena = 
    (void*)(page_size * MEMORY_ARENA_START_PAGE);
  mem_arena = mmap(mem_arena, MEMORY_ARENA_SIZE, 
      PROT_READ|PROT_WRITE, 
      MAP_SHARED|MAP_ANONYMOUS|MAP_FIXED, 
      -1, 0);
  if (mem_arena == (void*)-1) {
    result = errno;
    goto exit_close;
  }
  char *src = (char*)mem_arena;

  size_t read_c = 0;
  while ((read_c = 
        read(src_fd, src, MEMORY_ARENA_SIZE)) > 0) {
    TokenList tokens = Lex(src, read_c);
    PrintTokens(tokens);
    //AST syntax_tree = Parse(tokens);
    //syntax_tree = Analyze(syntax_tree);
    //Code code = Generate(syntax_tree);
  }
  if (read_c == -1) {
    result = errno;
    goto exit_unmap;
  }

  if (munmap(src, MEMORY_ARENA_SIZE) == -1) {
    result = errno;
    goto exit_close;
  }

  if (close(src_fd) == -1) {
    result = errno;
    goto exit_close;
  }

  return result;

exit_unmap:
  munmap(mem_arena, MEMORY_ARENA_SIZE);
exit_close:
  close(src_fd);
exit_now:
  // TODO(nick): error handling
  _exit(result);
}
