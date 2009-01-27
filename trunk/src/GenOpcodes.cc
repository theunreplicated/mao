//
// Copyright 2008 Google Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, 5th Floor, Boston, MA 02110-1301, USA.

// Generate enums and a hashtable for x86 instructions.
// Usage: GenOpcodes instruction-table
//
// Instruction table is something like:
//    binutils-2.19/opcodes/i386-opc.tbl
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

// Note: The following few helper routines, as
//       well as the main loop in main(), are directly
//       copied from the i386-gen.c sources in
//       binutils-2.19/opcodes/...
//

static char *
remove_leading_whitespaces (char *str)
{
  while (isspace (*str))
    str++;
  return str;
}

/* Remove trailing white spaces.  */

static void
remove_trailing_whitespaces (char *str)
{
  size_t last = strlen (str);

  if (last == 0)
    return;

  do
    {
      last--;
      if (isspace (str [last]))
	str[last] = '\0';
      else
	break;
    }
  while (last != 0);
}


static char *
next_field (char *str, char sep, char **next)
{
  char *p;

  p = remove_leading_whitespaces (str);
  for (str = p; *str != sep && *str != '\0'; str++);

  *str = '\0';
  remove_trailing_whitespaces (p);

  *next = str + 1;

  return p;
}

void usage(const char *argv[]) {
  fprintf(stderr, "USAGE:\n"
          "  %s table-file\n\n", argv[0] );
  fprintf(stderr,
          "Produces file: gen-opcodes.h\n" );
  exit(1);
}

int main(int argc, const char*argv[]) {
  char buf[2048];
  int  lineno = 0;
  char lastname[2048];
  char sanitized_name[2048];

  if (argc < 2)
    usage(argv);

  FILE *in = fopen(argv[1], "r");
  if (!in) {
    fprintf(stderr, "Cannot open table file: %s\n", argv[1]);
    usage(argv);
  }

  FILE *out = fopen("gen-opcodes.h", "w");
  if (!out) {
    fprintf(stderr, "Cannot open output file: gen-opcodes.h\n");
    usage(argv);
  }

  FILE *table = fopen("gen-opcodes-table.h", "w");
  if (!table) {
    fprintf(stderr, "Cannot open table file: gen-opcodes-table.h\n");
    usage(argv);
  }

  fprintf(out,
          "// DO NOT EDIT - this file is automatically "
          "generated by GenOpcodes\n//\n\n"
          "typedef enum MaoOpcode {\n"
          "  OP_invalid,\n"
          );

  fprintf(table,
          "// DO NOT EDIT - this file is automatically "
          "generated by GenOpcodes\n//\n\n"
          "#include \"gen-opcodes.h\"\n\n"
          "struct MaoOpcodeTable_ {\n"
          "   MaoOpcode    opcode;\n"
          "   const char  *name;\n"
          "} MaoOpcodeTable[] = {\n"
          "  { OP_invalid, \"invalid\" },\n"
          );

  while (!feof (in))
  {
    char *p, *str, *last, *name;
    if (fgets (buf, sizeof (buf), in) == NULL)
      break;

    lineno++;

    p = remove_leading_whitespaces (buf);

    /* Skip comments.  */
    str = strstr (p, "//");
    if (str != NULL)
      str[0] = '\0';

    /* Remove trailing white spaces.  */
    remove_trailing_whitespaces (p);

    switch (p[0])
    {
      case '#':
        fprintf (out, "%s\n", p);
      case '\0':
        continue;
        break;
      default:
        break;
    }

    last = p + strlen (p);

    /* Find name.  */
    name = next_field (p, ',', &str);

    /* sanitize name */
    int len = strlen(name);
    for (int i = 0; i < len+1; i++)
      if (name[i] == '.' || name[i] == '-')
        sanitized_name[i] = '_';
      else
        sanitized_name[i] = name[i];


    /* compare and emit */
    if (strcmp(name, lastname)) {
      fprintf(out, "  OP_%s,\n", sanitized_name );
      fprintf(table, "  { OP_%s, \t\"%s\" },\n", sanitized_name, name);
    }
    strcpy(lastname, name);
  }

  fprintf(out, "};  // MaoOpcode\n");

  fprintf(table, "  { OP_invalid, 0 }\n");
  fprintf(table, "};\n");
  fclose(in);
  fclose(out);

  return 0;
}
