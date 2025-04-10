/*
 *                  Distances of Secondary Structures
 *         Walter Fontana, Ivo L Hofacker, Peter F Stadler
 *                        Vienna RNA Package
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include "ViennaRNA/dist_vars.h"
#include "ViennaRNA/RNAstruct.h"
#include "ViennaRNA/treedist.h"
#include "ViennaRNA/stringdist.h"
#include "ViennaRNA/utils/basic.h"
#include "ViennaRNA/utils/strings.h"
#include "ViennaRNA/structures/pairtable.h"
#include "ViennaRNA/structures/dotbracket.h"
#include "ViennaRNA/structures/tree.h"
#include "ViennaRNA/utils/log.h"
#include "ViennaRNA/io/utils.h"
#include "ViennaRNA/datastructures/basic.h"
#include "RNAdistance_cmdl.h"

#include "gengetopt_helpers.h"

#define MAXNUM      1000    /* max number of structs for distance matrix */

#define PUBLIC
#define PRIVATE     static

PRIVATE void
command_line(int  argc,
             char *argv[]);


PRIVATE int
parse_input(char *line);


PRIVATE int
check_tree(char *line,
           char alpha[]);


PRIVATE int
check_brackets(char *line);


PRIVATE void
print_aligned_lines(FILE *somewhere);


PRIVATE char  ruler[] = "....,....1....,....2....,....3....,....4"
                        "....,....5....,....6....,....7....,....8";
PRIVATE int   types = 1;
PRIVATE int   task;
PRIVATE int   taxa_list;
PRIVATE char  outfile[FILENAME_MAX_LENGTH], *list_title;

PRIVATE char  ttype[10] = "f";
PRIVATE int   n         = 0;

int
main(int  argc,
     char *argv[])
{
  char      *line = NULL, *xstruc, *cc;
  Tree      *T[10][MAXNUM];
  int       tree_types = 0, ttree;
  swString  *S[10][MAXNUM];
  char      *P[MAXNUM];  /* structures for base pair distances */
  int       string_types = 0, tstr;
  int       i, j, tt, istty, type;
  int       it, is;
  FILE      *somewhere = NULL;

  command_line(argc, argv);

  if ((outfile[0] == '\0') && (task == 2) && (edit_backtrack))
    strcpy(outfile, "backtrack.file");

  if (outfile[0] != '\0')
    somewhere = fopen(outfile, "w");

  if (somewhere == NULL)
    somewhere = stdout;

  istty = isatty(fileno(stdin)) && isatty(fileno(stdout));

  do {
    if ((istty) && (n == 0)) {
      printf("\nInput structure;  @ to quit\n");
      printf("%s\n", ruler);
    }

    do {
      if (line != NULL)
        free(line);

      line = vrna_read_line(stdin);
    } while ((type = parse_input(line)) == 0);

    if (((type == 999) || (type == 888)) && (task == 2)) {
      /* do matrices */
      if (taxa_list)
        printf("* END of taxa list\n");

      ttree = 0;
      tstr  = 0;
      for (tt = 0; tt < types; tt++) {
        printf("> %c   %d\n", ttype[tt], n);
        if (islower(ttype[tt])) {
          for (i = 1; i < n; i++) {
            for (j = 0; j < i; j++) {
              printf("%g ", tree_edit_distance(T[ttree][i], T[ttree][j]));
              if (edit_backtrack) {
                fprintf(somewhere, "%d %d", i + 1, j + 1);
                if (ttype[tt] == 'f')
                  unexpand_aligned_F(aligned_line);

                print_aligned_lines(somewhere);
              }
            }
            printf("\n");
          }
          printf("\n");
          for (i = 0; i < n; i++)
            free_tree(T[ttree][i]);
          ttree++;
        }

        if (ttype[tt] == 'P') {
          for (i = 1; i < n; i++) {
            for (j = 0; j < i; j++)
              printf("%g ", (float)vrna_bp_distance(P[i], P[j]));
            printf("\n");
          }
          printf("\n");
          for (i = 0; i < n; i++)
            free(P[i]);
        } else if (isupper(ttype[tt])) {
          for (i = 1; i < n; i++) {
            for (j = 0; j < i; j++) {
              printf("%g ", string_edit_distance(S[tstr][i], S[tstr][j]));
              if (edit_backtrack) {
                fprintf(somewhere, "%d %d", i + 1, j + 1);
                if (ttype[tt] == 'F')
                  unexpand_aligned_F(aligned_line);

                print_aligned_lines(somewhere);
              }
            }
            printf("\n");
          }
          printf("\n");
          for (i = 0; i < n; i++)
            free(S[tstr][i]);
          tstr++;
        }
      }
      fflush(stdout);
      if (type == 888) {
        /* do another distance matrix */
        n = 0;
        printf("%s\n", list_title);
        free(list_title);
        continue;
      }
    }                       /* END do matrices */

    if (type == 999) {
      /* finito */
      if (outfile[0] != '\0')
        fclose(somewhere);

      return 0;
    }

    if (type < 0) {
      xstruc = add_root(line);
      free(line);
      line  = xstruc;
      type  = -type;
    }

    if (type == 2) {
      xstruc = unexpand_Full(line);
      free(line);
      line  = xstruc;
      type  = 1;
    }

    if (n >= MAXNUM) {
      vrna_log_error("Number of input structures exceeds maximum of %d", MAXNUM);
      exit(EXIT_FAILURE);
    }

    tree_types    = 0;
    string_types  = 0;
    for (tt = 0; tt < types; tt++) {
      switch (ttype[tt]) {
        case 'f':
        case 'F':
          if (type != 1) {
            vrna_log_error("Can't convert back to full structure");
            exit(EXIT_FAILURE);
          }

          xstruc = expand_Full(line);
          if (islower(ttype[tt])) /* tree_edit */
            T[tree_types++][n] = make_tree(xstruc);

          if (isupper(ttype[tt])) /* string edit */
            S[string_types++][n] = Make_swString(xstruc);

          free(xstruc);
          break;
        case 'P':
          if (type != 1) {
            vrna_log_error("Can't convert back to full structure");
            exit(EXIT_FAILURE);
          }

          P[n] = strdup(line);
          break;
        case 'h':
        case 'H':
          switch (type) {
            case 1:
              xstruc = b2HIT(line);
              if (islower(ttype[tt]))
                T[tree_types++][n] = make_tree(xstruc);

              if (isupper(ttype[tt]))
                S[string_types++][n] = Make_swString(xstruc);

              free(xstruc);
              break;
            default:
              vrna_log_error("Can't convert to HIT structure");
              exit(EXIT_FAILURE);
          }
          break;
        case 'c':
        case 'C':
          switch (type) {
            case 1:
              cc      = b2C(line);
              xstruc  = expand_Shapiro(cc);
              free(cc);
              break;
            case 4:
              cc      = expand_Shapiro(line);
              xstruc  = unweight(cc);
              free(cc);
              break;
            case 3:
              xstruc = unweight(line);
              break;
            default:
              vrna_log_error("Unknown structure representation");
              exit(EXIT_FAILURE);
          }
          if (islower(ttype[tt]))
            T[tree_types++][n] = make_tree(xstruc);

          if (isupper(ttype[tt]))
            S[string_types++][n] = Make_swString(xstruc);

          free(xstruc);
          break;
        case 'w':
        case 'W':
          if (type == 1) {
            xstruc = b2Shapiro(line);
            if (islower(ttype[tt]))
              T[tree_types++][n] = make_tree(xstruc);

            if (isupper(ttype[tt]))
              S[string_types++][n] = Make_swString(xstruc);

            free(xstruc);
          } else {
            if (islower(ttype[tt]))
              T[tree_types++][n] = make_tree(line);

            if (isupper(ttype[tt]))
              S[string_types++][n] = Make_swString(line);
          }

          break;
        default:
          vrna_log_error("Unknown distance type");
          exit(EXIT_FAILURE);
      }
    }
    n++;
    switch (task) {
      float dist;
      case 1:
        if (n == 2) {
          for (it = 0, is = 0, i = 0; i < types; i++) {
            if (islower(ttype[i])) {
              dist = tree_edit_distance(T[it][0], T[it][1]);
              free_tree(T[it][0]);
              free_tree(T[it][1]);
              it++;
            } else if (ttype[i] == 'P') {
              dist = (float)vrna_bp_distance(P[0], P[1]);
              free(P[0]);
              free(P[1]);
            } else {
              /* isupper(ttype[i]) */
              dist = string_edit_distance(S[is][0], S[is][1]);
              free(S[is][0]);
              free(S[is][1]);
              is++;
            }

            printf("%c: %g  ", ttype[i], dist);
            if ((edit_backtrack) && (ttype[i] != 'P')) {
              if (ttype[i] == 'f')
                unexpand_aligned_F(aligned_line);

              print_aligned_lines(somewhere);
            }
          }
          printf("\n");
          n = 0;
        }

        break;
      case 3:
        if (n > 1) {
          for (it = 0, is = 0, i = 0; i < types; i++) {
            if (islower(ttype[i])) {
              dist = tree_edit_distance(T[it][1], T[it][0]);
              free_tree(T[it][1]);
              it++;
            } else if (ttype[i] == 'P') {
              dist = (float)vrna_bp_distance(P[0], P[1]);
              free(P[1]);
            } else {
              /* if(isupper(ttype[i])) */
              dist = string_edit_distance(S[is][0], S[is][1]);
              free(S[is][1]);
              is++;
            }

            printf("%c: %g  ", ttype[i], dist);
            if ((edit_backtrack) && (ttype[i] != 'P')) {
              if (ttype[i] == 'f')
                unexpand_aligned_F(aligned_line);

              print_aligned_lines(somewhere);
            }
          }
          printf("\n");
          n = 1;
        }

        break;
      case 4:
        if (n > 1) {
          for (it = 0, is = 0, i = 0; i < types; i++) {
            if (islower(ttype[i])) {
              dist = tree_edit_distance(T[it][1], T[it][0]);
              free_tree(T[it][0]);
              T[it][0] = T[it][1];
              it++;
            } else if (ttype[i] == 'P') {
              dist = (float)vrna_bp_distance(P[0], P[1]);
              free(P[0]);
              P[0] = P[1];
            } else {
              /* if(isupper(ttype[i])) */
              dist = string_edit_distance(S[is][0], S[is][1]);
              free(S[is][0]);
              S[is][0] = S[is][1];
              is++;
            }

            printf("%c: %g  ", ttype[i], dist);
            if ((edit_backtrack) && (ttype[i] != 'P')) {
              if (ttype[i] == 'f')
                unexpand_aligned_F(aligned_line);

              print_aligned_lines(somewhere);
            }
          }
          printf("\n");
          n = 1;
        }

        break;
    }
    fflush(stdout);
  } while (type != 999);

  if (vrna_log_fp() != stderr)
    fclose(vrna_log_fp());

  return EXIT_SUCCESS;
}


/*--------------------------------------------------------------------------*/

PRIVATE int
parse_input(char *line)
{
  int   type, rooted = 0, i, xx;
  char  *cp;

  if (line == NULL)
    return 999;

  if (line[0] == '*') {
    if (taxa_list == 0) {
      if (task == 2)
        taxa_list = 1;

      printf("%s\n", line);
      return 0;
    } else {
      list_title = strdup(line);
      return 888;
    }
  }

  if (line[0] == '>') {
    if (taxa_list)
      printf("%d :%s\n", n + 1, line + 1);
    else
      printf("%s\n", line);

    return 0;
  }

  cp = strchr(line, ' ');
  if (cp)
    *cp = '\0';          /* get rid of junk at the end of line */

  switch (line[0]) {
    case '.':
      type = 1;
      break;

    case '(':           /* it's a tree */
      i       = 1;
      xx      = 0;
      type    = 4;      /* coarse */
      rooted  = 0;
      while (line[i]) {
        if (line[i] == '.') {
          type = 1;   /* Full */
          break;
        }

        if ((line[i] == 'U') || (line[i] == 'P')) {
          type  = 2;  /* FFull */
          xx    = 1;
          break;
        }

        if (line[i] == 'S') {
          type  = 3;
          xx    = 1;
          break;      /* Shapiro tree */
        }

        if ((line[i] != '(') && (line[i] != ')'))
          xx = 1;

        i++;
      }
      if (!xx)
        type = 1;

      rooted = (line[strlen(line) - 2] == 'R');
      break;
    case '@':
      return 999;

    default:
      return 0;
  }

  switch (type) {
    case 1:
      if (check_brackets(line))
        return 1;

      break;
    case 2:
      if (check_tree(line, "UP")) {
        if (rooted)
          return 2;
        else
          return -2;
      }

      break;
    case 3:
      if (check_tree(line, "HBIMSE")) {
        if (rooted)
          return -3;
        else
          return -3;
      }

      break;
    case 4:
      if (check_tree(line, "HBIM")) {
        if (rooted)
          return 4;
        else
          return -4;
      }

      break;
  }
  return 0;
}


/*--------------------------------------------------------------------------*/


PRIVATE int
check_tree(char *line,
           char alpha[])
{
  int   n, i, o;
  char  *pos;

  n = (int)strlen(line);

  if (line[0] != '(')
    return 0;

  i = o = 1;
  while (line[i]) {
    while (line[i] == '(') {
      o++;
      i++;
    }
    pos = strchr(alpha, (int)line[i]);
    if (pos) {
      while (isdigit((int)line[++i]));
      if (line[i] != ')')
        return 0;
    }

    if (line[i] == 'R') {
      i++;
      if ((i != n - 1) || (line[i] != ')'))
        return 0;
    }

    if (line[i] == ')') {
      o--;
      if (o < 0)
        return 0;

      if ((i < n) && (line[i + 1] == ')'))
        return 0;

      i++;
    } else {
      return 0;
    }
  }
  if (o > 0)
    return 0;

  return 1;
}


/*--------------------------------------------------------------------------*/


PRIVATE int
check_brackets(char *line)
{
  int i, o;

  i = o = 0;
  while (line[i]) {
    switch (line[i]) {
      case '(':
        o++;
        i++;
        break;
      case '.':
        i++;
        break;
      case ')':
        i++;
        o--;
        if (o < 0)
          return 0;

        break;
      default:
        return 0;
    }
  }
  if (o > 0)
    return 0;

  return 1;
}


/*--------------------------------------------------------------------------*/


PRIVATE void
command_line(int  argc,
             char *argv[])
{
  int           verbose;
  struct        RNAdistance_args_info args_info;

  /* default values */
  edit_backtrack  = 0;
  types           = 1;
  ttype[0]        = 'f';
  task            = 1;

  /*
   #############################################
   # check the command line parameters
   #############################################
   */
  if (RNAdistance_cmdline_parser(argc, argv, &args_info) != 0)
    exit(1);

  /* prepare logging system and verbose mode */
  ggo_log_settings(args_info, verbose);

  /* use specified distance representations */
  if (args_info.distance_given) {
    strncpy(ttype, args_info.distance_arg, 9);
    types = (int)strlen(ttype);
  }

  if (args_info.compare_given) {
    switch (args_info.compare_arg[0]) {
      case 'p':
        task = 1;
        break;
      case 'm':
        task = 2;
        break;
      case 'f':
        task = 3;
        break;
      case 'c':
        task = 4;
        break;
      default:
        RNAdistance_cmdline_parser_print_help();
        exit(EXIT_FAILURE);
    }
  }

  if (args_info.shapiro_given)
    cost_matrix = 1;

  if (args_info.backtrack_given) {
    if (strcmp(args_info.backtrack_arg, "none"))
      strncpy(outfile, args_info.backtrack_arg, FILENAME_MAX_LENGTH - 1);

    edit_backtrack = 1;
  }

  /* free allocated memory of command line data structure */
  RNAdistance_cmdline_parser_free(&args_info);
}


/*--------------------------------------------------------------------------*/

PRIVATE void
print_aligned_lines(FILE *somewhere)
{
  if (edit_backtrack) {
    fprintf(somewhere, "\n%s\n%s\n", aligned_line[0], aligned_line[1]);
    fflush(somewhere);
  }
}
