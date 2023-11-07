
#include <cstdio>
#include <cstdlib>

////////////////////////////////////////////////////////////////////////////////
static inline
int getc_and_check(FILE *f, const char* filename)
{
  int c;
  c = getc(f);
  if (feof(f)) {
    fprintf(stderr, "Error while reading a comment from %s\n", filename);
    exit(-1);
  }
  return c;
}

////////////////////////////////////////////////////////////////////////////////
static inline
void eat_comments(FILE *f, const char* filename, int& c) noexcept
{
  int length = 0;
  length = getc_and_check(f, filename) << 8;
  length |= getc_and_check(f, filename);
  length -= 2;

  for (int i = 0; i < length; ++i)
    c = getc_and_check(f, filename);
}


////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
  if (argc < 3)
  {
    fprintf(stderr, 
      "compare_files expects two arguments <filename1, filename2>\n");
    exit(-1);
  }

  FILE *f1 = fopen(argv[1], "rb");
  if (f1 == NULL) 
  {
    fprintf(stderr, "Unable to open file %s.\n", argv[1]);
    return -1;
  }

  FILE *f2 = fopen(argv[2], "rb");
  if (f2 == NULL) 
  {
    fprintf(stderr, "Unable to open file %s.\n", argv[2]);
    return -1;
  }

  bool tile_started = false;
  int old_c1 = ' ';
  while (1) // both files must end at the same time
  {
    int c1 = getc(f1);
    int c2 = getc(f2);

    bool eof1 = (feof(f1) != 0), eof2 = (feof(f2) != 0);

    if (eof1 && eof2) // both reached end of file
    {
      fprintf(stdout, "Matching files.\n");
      return 0;
    }
    else if (!eof1 && !eof2) 
    {
      if (c1 != c2)
        return -1;
      if (!tile_started && old_c1 == 0xFF && c1 == 0x64) {
        eat_comments(f1, argv[1], c1);
        eat_comments(f2, argv[2], c2);
      }
      if (!tile_started && old_c1 == 0xFF && c1 == 0x90) {
        // stop checking comments when a tile starts; we ignoring
        // the case where tile can also have comments
        tile_started = true;
      }
      old_c1 = c1;
    }
    else // only one of them reached end of file
    {
      fprintf(stderr, "One file finished before the other one.\n");
      return -1;
    }
  }

  fclose(f1);
  fclose(f2);

  return 0;
}