/*
 *  ViennaRNA/utils/structures.c
 *
 *  Various functions to convert, parse, encode secondary structures
 *
 *  c  Ivo L Hofacker, Walter Fontana, Ronny Lorenz
 *              Vienna RNA package
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "ViennaRNA/fold_vars.h"
#include "ViennaRNA/utils/basic.h"
#include "ViennaRNA/params/basic.h"
#include "ViennaRNA/loops/gquad.h"
#include "ViennaRNA/utils/structures.h"
#include "ViennaRNA/utils/log.h"
#include "ViennaRNA/MEA.h"

#ifdef __GNUC__
# define INLINE inline
#else
# define INLINE
#endif


#define SHAPES_OPEN   '['
#define SHAPES_CLOSE  ']'
#define SHAPES_UP     '_'

struct shrep {
  struct shrep  *pred;
  struct shrep  *succ;
  char          character;
};


/*
 #################################
 # PRIVATE FUNCTION DECLARATIONS #
 #################################
 */
PRIVATE vrna_ep_t *
wrap_get_plist(vrna_mx_pf_t     *matrices,
               int              length,
               int              *index,
               short            *S,
               vrna_exp_param_t *pf_params,
               double           cut_off);


PRIVATE vrna_ep_t *
wrap_plist(vrna_fold_compound_t *vc,
           double               cut_off);


PRIVATE void
assign_elements_pair(short  *pt,
                     int    i,
                     int    j,
                     char   *elements);


PRIVATE INLINE void
flatten_brackets(char       *string,
                 const char pair[3],
                 const char target[3]);


PRIVATE INLINE int
extract_pairs(short       *pt,
              const char  *structure,
              const char  *pair);


PRIVATE INLINE struct shrep *
shrep_insert_after(struct shrep *target,
                   char         character);


PRIVATE INLINE struct shrep *
shrep_insert_before(struct shrep  *target,
                    char          character);


PRIVATE INLINE void
shrep_concat(struct shrep **target,
             struct shrep *suffix);


PRIVATE INLINE char *
db2shapes(const char    *structure,
          unsigned int  level);


PRIVATE INLINE char *
db2shapes_pt(const short  *pt,
             unsigned int n,
             unsigned int level);


PRIVATE INLINE struct shrep *
get_shrep(const short   *pt,
          unsigned int  start,
          unsigned int  end,
          unsigned int  level);


/*
 #################################
 # BEGIN OF FUNCTION DEFINITIONS #
 #################################
 */
PUBLIC char *
vrna_db_pack(const char *struc)
{
  /* 5:1 compression using base 3 encoding */
  int           i, j, l, pi;
  unsigned char *packed;

  l       = (int)strlen(struc);
  packed  = (unsigned char *)vrna_alloc(((l + 4) / 5 + 1) * sizeof(unsigned char));

  j = i = pi = 0;
  while (i < l) {
    register int p;
    for (p = pi = 0; pi < 5; pi++) {
      p *= 3;
      switch (struc[i]) {
        case '(':
        case '\0':
          break;
        case ')':
          p++;
          break;
        case '.':
          p += 2;
          break;
        default:
          vrna_log_warning("vrna_db_pack: "
                               "illegal character %c at position %d in structure\n%s",
                               struc[i],
                               i + 1,
                               struc);
          return NULL;
      }
      if (i < l)
        i++;
    }
    packed[j++] = (unsigned char)(p + 1); /* never use 0, so we can use
                                           * strcmp()  etc. */
  }
  packed[j] = '\0';                       /* for str*() functions */
  return (char *)packed;
}


PUBLIC char *
vrna_db_unpack(const char *packed)
{
  /* 5:1 compression using base 3 encoding */
  int                 i, j, l;
  char                *struc;
  unsigned const char *pp;
  char                code[3] = {
    '(', ')', '.'
  };

  l     = (int)strlen(packed);
  pp    = (const unsigned char *)packed;
  struc = (char *)vrna_alloc((l * 5 + 1) * sizeof(char));   /* up to 4 byte extra */

  for (i = j = 0; i < l; i++) {
    register int p, c, k;

    p = (int)pp[i] - 1;
    for (k = 4; k >= 0; k--) {
      c             = p % 3;
      p             /= 3;
      struc[j + k]  = code[c];
    }
    j += 5;
  }
  struc[j--] = '\0';
  /* strip trailing ( */
  while ((j >= 0) &&
         (struc[j] == '('))
    struc[j--] = '\0';

  return struc;
}


PUBLIC short *
vrna_ptable(const char *structure)
{
  return vrna_ptable_from_string(structure, VRNA_BRACKETS_RND);
}


PUBLIC short *
vrna_pt_pk_get(const char *structure)
{
  return vrna_ptable_from_string(structure, VRNA_BRACKETS_RND | VRNA_BRACKETS_SQR);
}


PUBLIC short *
vrna_pt_snoop_get(const char *structure)
{
  return vrna_ptable_from_string(structure, VRNA_BRACKETS_ANG);
}


PUBLIC short *
vrna_pt_ali_get(const char *structure)
{
  return vrna_ptable_from_string(structure,
                                 VRNA_BRACKETS_RND | VRNA_BRACKETS_ANG | VRNA_BRACKETS_SQR);
}


PUBLIC short *
vrna_ptable_copy(const short *pt)
{
  short *table = (short *)vrna_alloc(sizeof(short) * (pt[0] + 2));

  memcpy(table, pt, sizeof(short) * (pt[0] + 2));
  return table;
}


PUBLIC int *
vrna_loopidx_from_ptable(const short *pt)
{
  /* number each position by which loop it belongs to (positions start
   * at 1) */
  int i, hx, l, nl;
  int length;
  int *stack  = NULL;
  int *loop   = NULL;

  length  = pt[0];
  stack   = (int *)vrna_alloc(sizeof(int) * (length + 1));
  loop    = (int *)vrna_alloc(sizeof(int) * (length + 2));
  hx      = l = nl = 0;

  for (i = 1; i <= length; i++) {
    if ((pt[i] != 0) && (i < pt[i])) {
      /* ( */
      nl++;
      l           = nl;
      stack[hx++] = i;
    }

    loop[i] = l;

    if ((pt[i] != 0) && (i > pt[i])) {
      /* ) */
      --hx;
      if (hx > 0)
        l = loop[stack[hx - 1]];  /* index of enclosing loop   */
      else
        l = 0;                    /* external loop has index 0 */

      if (hx < 0) {
        vrna_log_warning("vrna_loopidx_from_ptable: "
                             "unbalanced brackets in make_pair_table");
        free(stack);
        return NULL;
      }
    }
  }
  loop[0] = nl;
  free(stack);
  return loop;
}


PUBLIC short *
vrna_pt_pk_remove(const short   *ptable,
                  unsigned int  options)
{
  short *pt = NULL;

  if (ptable) {
    char          *mea_structure;
    unsigned int  i, j, n;
    vrna_ep_t     *pairs;

    n             = (unsigned int)ptable[0];
    mea_structure = (char *)vrna_alloc(sizeof(char) * (n + 1));
    pairs         = (vrna_ep_t *)vrna_alloc(sizeof(vrna_ep_t) * (n + 1));

    /* compose list of pairs to be used in MEA() function */
    for (j = 0, i = 1; i <= n; i++)
      if (ptable[i] > i) {
        pairs[j].i    = i;
        pairs[j].j    = ptable[i];
        pairs[j].p    = 1.;
        pairs[j].type = VRNA_PLIST_TYPE_BASEPAIR;
        j++;
      }

    pairs[j].i    = 0;
    pairs[j].j    = 0;
    pairs[j].p    = 0;
    pairs[j].type = VRNA_PLIST_TYPE_BASEPAIR;

    /* use MEA() implementation to remove crossing base pairs */
    memset(mea_structure, '.', n);

    (void)MEA(pairs, mea_structure, 2.0);

    /* convert dot-bracket structure to pair table */
    pt = vrna_ptable(mea_structure);

    free(mea_structure);
    free(pairs);
  }

  return pt;
}


PUBLIC char *
vrna_db_pk_remove(const char    *structure,
                  unsigned int  options)
{
  char  *s;
  short *pt_pk, *pt;

  s = NULL;

  if (structure) {
    pt_pk = vrna_ptable_from_string(structure, options & VRNA_BRACKETS_ANY);
    pt    = vrna_pt_pk_remove(pt_pk, options);
    s     = vrna_db_from_ptable(pt);

    free(pt_pk);
    free(pt);
  }

  return s;
}


PUBLIC char *
vrna_db_from_ptable(const short *ptable)
{
  char          *dotbracket           = NULL;
  const char    *bracket_open_avail   = "([{<ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const char    *bracket_close_avail  = ")]}>abcdefghijklmnopqrstuvwxyz";
  short         *pt;
  unsigned int  n, i, stack_cnt, *stack, bracket_count, recheck;

  if (ptable) {
    n = (unsigned int)ptable[0];
    if (n > 0) {
      pt = (short *)vrna_alloc(sizeof(short) * (n + 1));
      pt = memcpy(pt, ptable, sizeof(short) * (n + 1));

      /* check for validity, i.e. both pairing positions must specify pairing partner */
      for (i = 1; i <= n; i++) {
        if (((unsigned int)pt[i] > i) &&
            ((unsigned int)pt[pt[i]] != i))
          return NULL;
      }

      /* prepare everything */
      dotbracket = (char *)vrna_alloc((n + 1) * sizeof(char));
      memset(dotbracket, '.', n);

      bracket_count = 0;
      recheck       = 1;
      stack         = (unsigned int *)vrna_alloc(sizeof(unsigned int) * (n + 1));
      stack_cnt     = 0;

      while (recheck) {
        recheck = 0;

        for (i = 1; i <= n; i++) {
          if (pt[i] > i) {
            /* check for clash */
            if ((stack_cnt > 0) &&
                (pt[i] > stack[stack_cnt - 1])) {
              recheck = 1;
              continue;
            } else {
              stack[stack_cnt++] = pt[i];
              dotbracket[i - 1] = bracket_open_avail[bracket_count];
              dotbracket[pt[i] - 1] = bracket_close_avail[bracket_count];
            }
          } else if (pt[i] != 0) {
            if (stack_cnt == 0)
              continue;

            if (i == stack[stack_cnt - 1]) {
              /* remove pair from pair table */
              pt[i] = pt[pt[i]] = 0;
              stack_cnt--;
            }
          }
        }

        stack_cnt = 0;
        bracket_count++;

        if (bracket_count >= 30) {
          vrna_log_warning("Not enough bracket types available in vrna_db_from_ptable()! Skipping remaining base pairs!");
          break;
        }
      }

      dotbracket[i - 1] = '\0';
      free(stack);
      free(pt);
    }
  }

  return dotbracket;
}


PUBLIC void
vrna_db_flatten(char          *string,
                unsigned int  options)
{
  vrna_db_flatten_to(string, "()", options);
}


PUBLIC void
vrna_db_flatten_to(char         *string,
                   const char   target[3],
                   unsigned int options)
{
  if (string) {
    if (options & VRNA_BRACKETS_RND)
      flatten_brackets(string, "()", target);

    if (options & VRNA_BRACKETS_ANG)
      flatten_brackets(string, "<>", target);

    if (options & VRNA_BRACKETS_CLY)
      flatten_brackets(string, "{}", target);

    if (options & VRNA_BRACKETS_SQR)
      flatten_brackets(string, "<>", target);

    if (options & VRNA_BRACKETS_ALPHA) {
      char pairs[3];

      for (int i = 65; i < 91; i++) {
        pairs[0]  = (char)i;
        pairs[1]  = (char)(i + 32);
        pairs[2]  = '\0';
        flatten_brackets(string, pairs, target);
      }
    }
  }
}


PUBLIC short *
vrna_ptable_from_string(const char    *string,
                        unsigned int  options)
{
  char          pairs[3];
  short         *pt;
  unsigned int  i, n;

  n = strlen(string);

  if (n > SHRT_MAX) {
    vrna_log_warning("vrna_ptable_from_string: "
                         "Structure too long to be converted to pair table (n=%d, max=%d)",
                         n,
                         SHRT_MAX);
    return NULL;
  }

  pt    = (short *)vrna_alloc(sizeof(short) * (n + 2));
  pt[0] = (short)n;


  if ((options & VRNA_BRACKETS_RND) &&
      (!extract_pairs(pt, string, "()"))) {
    free(pt);
    return NULL;
  }

  if ((options & VRNA_BRACKETS_ANG) &&
      (!extract_pairs(pt, string, "<>"))) {
    free(pt);
    return NULL;
  }

  if ((options & VRNA_BRACKETS_CLY) &&
      (!extract_pairs(pt, string, "{}"))) {
    free(pt);
    return NULL;
  }

  if ((options & VRNA_BRACKETS_SQR) &&
      (!extract_pairs(pt, string, "[]"))) {
    free(pt);
    return NULL;
  }

  if (options & VRNA_BRACKETS_ALPHA) {
    for (i = 65; i < 91; i++) {
      pairs[0]  = (char)i;
      pairs[1]  = (char)(i + 32);
      pairs[2]  = '\0';
      if (!extract_pairs(pt, string, pairs)) {
        free(pt);
        return NULL;
      }
    }
  }

  return pt;
}


PUBLIC char *
vrna_db_from_WUSS(const char *wuss)
{
  char          *db, *tmp;
  short         *pt;
  unsigned int  i, p, q, pos, L, l[3], num_gq, n;

  db = NULL;

  if (wuss) {
    n       = strlen(wuss);
    num_gq  = 0;

    /*
     *  Note, in WUSS notation, matching pairs of (), <>, {}, [] are allowed but must not
     *  cross! Thus, we can simply flatten all brackets to ().
     */
    tmp = (char *)vrna_alloc(sizeof(char) * (n + 1));
    tmp = (char *)memcpy(tmp, wuss, n + 1);

    vrna_db_flatten(tmp, VRNA_BRACKETS_DEFAULT);

    /* now convert flattened structure string to pair-table (removes pseudo-knots) */
    pt = vrna_ptable_from_string(tmp, VRNA_BRACKETS_RND);

    /* convert back to dot-bracket (replaces all special characters for unpaired positions) */
    db = vrna_db_from_ptable(pt);

    /* check for G-Quadruplexes, annotated as G quartets */
    q = 0;
    while ((pos = vrna_gq_parse(wuss + q, &L, l)) > 0) {
      num_gq++;
      q += pos;

      if ((num_gq == 1) &&
          (4 * L + l[0] + l[1] + l[2] > pos)) {
        /* G-Quadruplex wraps around n,1 junction */
        p = n + pos + 1 - 4 * L - l[0] - l[1] - l[2] + 1;

        for (i = 0; i < L; i++) {
          unsigned int p1, p2, p3, p4;
          p1  = p + i;
          p2  = p1 + L + l[0];
          p3  = p2 + L + l[1];
          p4  = p3 + L + l[2];

          if (p4 > n)
            p4 = ((p4 - 1) % n) + 1;

          if (p3 > n)
            p3 = ((p3 - 1) % n) + 1;

          if (p2 > n)
            p2 = ((p2 - 1) % n) + 1;

          if (p1 > n)
            p1 = ((p1 - 1) % n) + 1;

          db[p1 - 1] = db[p2 - 1] = db[p3 - 1] = db[p4 - 1] = VRNA_GQUAD_DB_SYMBOL;

          if (i + 1 == L) /* last position of G-Quadruplex */
            db[p4 - 1] = VRNA_GQUAD_DB_SYMBOL_END;
        }
      } else {
        p = q - 4 * L - l[0] - l[1] - l[2] + 1;

        if (q > n)
          break;

        /* re-insert G-Quadruplex */
        for (i = 0; i < L; i++) {
          db[p + i - 1]                                 = VRNA_GQUAD_DB_SYMBOL;
          db[p + L + l[0] + i - 1]                      = VRNA_GQUAD_DB_SYMBOL;
          db[p + (2 * L) + l[0] + l[1] + i - 1]         = VRNA_GQUAD_DB_SYMBOL;
          db[p + (3 * L) + l[0] + l[1] + l[2] + i - 1]  = VRNA_GQUAD_DB_SYMBOL;
        }
      }

//      q++;
    }

    free(pt);
    free(tmp);
  }

  return db;
}


/*---------------------------------------------------------------------------*/

PUBLIC int
vrna_bp_distance_pt(const short *pt1,
                    const short *pt2)
{
  /*
   * dist = {number of base pairs in one structure but not in the other}
   * same as edit distance with pair_open pair_close as move set
   */
  int   dist;
  short i, l;

  dist = 0;

  if (pt1 && pt2) {
    l = (pt1[0] < pt2[0]) ? pt1[0] : pt2[0]; /* minimum of the two lengths */

    for (i = 1; i <= l; i++)
      if (pt1[i] != pt2[i]) {
        if (pt1[i] > i)
          dist++;

        if (pt2[i] > i)
          dist++;
      }
  }

  return dist;
}


PUBLIC int
vrna_bp_distance(const char *str1,
                 const char *str2)
{
  int   dist = 0;
  short *pt1, *pt2;

  pt1 = vrna_ptable(str1);
  pt2 = vrna_ptable(str2);

  dist = vrna_bp_distance_pt(pt1, pt2);

  free(pt1);
  free(pt2);

  return dist;
}


PUBLIC double
vrna_dist_mountain(const char   *str1,
                   const char   *str2,
                   unsigned int p)
{
  short         *pt1, *pt2;
  unsigned int  i, n;
  double        distance, w, *f1, *f2;

  distance  = -1.;
  f1        = NULL;
  f2        = NULL;

  if ((str1) && (str2)) {
    n = strlen(str1);

    if (n != strlen(str2)) {
      vrna_log_warning("vrna_dist_mountain: input structures have unequal lengths!");
      return distance;
    }

    pt1 = vrna_ptable(str1);
    pt2 = vrna_ptable(str2);
    f1  = (double *)vrna_alloc(sizeof(double) * (n + 1));
    f2  = (double *)vrna_alloc(sizeof(double) * (n + 1));

    /* count (mountain)heights for positions 1 <= i <= n */
    for (w = 0., i = 1; i <= n; i++) {
      if (pt1[i] == 0)
        continue;

      if (pt1[i] > i)
        w += 1. / (double)(pt1[i] - i);
      else
        w -= 1. / (double)(i - pt1[i]);

      f1[i] = w;
    }

    for (w = 0., i = 1; i <= n; i++) {
      if (pt2[i] == 0)
        continue;

      if (pt2[i] > i)
        w += 1. / (double)(pt2[i] - i);
      else
        w -= 1. / (double)(i - pt2[i]);

      f2[i] = w;
    }


    /* finally, compute L_p-norm */
    for (distance = 0., i = 1; i <= n; i++)
      distance += pow(fabs(f1[i] - f2[i]), (double)p);

    distance = pow(distance, 1. / (double)p);

    free(pt1);
    free(pt2);
    free(f1);
    free(f2);
  }

  return distance;
}


/* get a matrix containing the number of basepairs of a reference structure for each interval [i,j] with i<j
 *  access it via iindx!!!
 */
PUBLIC unsigned int *
vrna_refBPcnt_matrix(const short  *reference_pt,
                     unsigned int turn)
{
  unsigned int  i, j, k, ij, length;
  int           *iindx;
  unsigned int  *array;
  unsigned int  size;

  length  = (unsigned int)reference_pt[0];
  size    = ((length + 1) * (length + 2)) / 2;
  iindx   = vrna_idx_row_wise(length);
  array   = (unsigned int *)vrna_alloc(sizeof(unsigned int) * size);
  /* matrix containing number of basepairs of reference structure1 in interval [i,j] */;
  for (k = 0; k <= turn; k++)
    for (i = 1; i <= length - k; i++) {
      j         = i + k;
      ij        = iindx[i] - j;
      array[ij] = 0;
    }

  for (i = length - turn - 1; i >= 1; i--)
    for (j = i + turn + 1; j <= length; j++) {
      int bps;
      ij  = iindx[i] - j;
      bps = array[ij + 1];
      if ((i <= (unsigned int)reference_pt[j]) && ((unsigned int)reference_pt[j] < j))
        bps++;

      array[ij] = bps;
    }
  free(iindx);
  return array;
}


PUBLIC unsigned int *
vrna_refBPdist_matrix(const short   *pt1,
                      const short   *pt2,
                      unsigned int  turn)
{
  unsigned int  *array;
  unsigned int  n, size, i, j, ij, d;

  n     = (unsigned int)pt1[0];
  size  = ((n + 1) * (n + 2)) / 2;
  array = (unsigned int *)vrna_alloc(sizeof(unsigned int) * size);
  int           *iindx = vrna_idx_row_wise(n);

  for (i = n - turn - 1; i >= 1; i--) {
    d = 0;
    for (j = i + turn + 1; j <= n; j++) {
      ij  = iindx[i] - j;
      d   = array[ij + 1];
      if (pt1[j] != pt2[j]) {
        if (i <= (unsigned int)pt1[j] && (unsigned int)pt1[j] < j)
          /* we got an additional base pair in reference structure 1 */
          d++;

        if (i <= (unsigned int)pt2[j] && (unsigned int)pt2[j] < j)
          /* we got another base pair in reference structure 2 */
          d++;
      }

      array[ij] = d;
    }
  }
  free(iindx);
  return array;
}


PUBLIC char
vrna_bpp_symbol(const float *x)
{
  /*  if( ((x[1]-x[2])*(x[1]-x[2]))<0.1&&x[0]<=0.677) return '|'; */
  if (x[0] > 0.667)
    return '.';

  if (x[1] > 0.667)
    return '(';

  if (x[2] > 0.667)
    return ')';

  if ((x[1] + x[2]) > x[0]) {
    if ((x[1] / (x[1] + x[2])) > 0.667)
      return '{';

    if ((x[2] / (x[1] + x[2])) > 0.667)
      return '}';
    else
      return '|';
  }

  if (x[0] > (x[1] + x[2]))
    return ',';

  return ':';
}


PUBLIC char *
vrna_db_from_probs(const FLT_OR_DBL *p,
                   unsigned int     length)
{
  int   i, j, *index;
  float P[3];    /* P[][0] unpaired, P[][1] upstream p, P[][2] downstream p */
  char  *s;

  s = NULL;

  if (p) {
    index = vrna_idx_row_wise(length);
    s     = (char *)vrna_alloc(sizeof(char) * (length + 1));

    for (j = 1; j <= length; j++) {
      P[0]  = 1.0;
      P[1]  = P[2] = 0.0;
      for (i = 1; i < j; i++) {
        P[2]  += (float)p[index[i] - j];  /* j is paired downstream */
        P[0]  -= (float)p[index[i] - j];  /* j is unpaired */
      }
      for (i = j + 1; i <= length; i++) {
        P[1]  += (float)p[index[j] - i];  /* j is paired upstream */
        P[0]  -= (float)p[index[j] - i];  /* j is unpaired */
      }
      s[j - 1] = vrna_bpp_symbol(P);
    }
    s[length] = '\0';
    free(index);
  }

  return s;
}


PUBLIC char *
vrna_pairing_tendency(vrna_fold_compound_t *fc)
{
  unsigned int  i, j, n;
  int           *idx;
  FLT_OR_DBL    *p, pp;
  float         P[3];    /* P[][0] unpaired, P[][1] upstream p, P[][2] downstream p */
  vrna_md_t     *md;
  char                      *s;
  vrna_smx_csr(FLT_OR_DBL)  *p_gq;

  s = NULL;

  if ((fc) &&
      (fc->exp_matrices) &&
      (fc->exp_matrices->probs)) {
    n   = fc->length;
    idx = fc->iindx;
    p   = fc->exp_matrices->probs;
    md  = &(fc->exp_params->model_details);
    s   = (char *)vrna_alloc(sizeof(char) * (n + 1));

    if ((md->circ) &&
        (md->gquad) &&
        (fc->exp_matrices->p_gq))
      p_gq = fc->exp_matrices->p_gq;
    else
      p_gq = NULL;

    for (j = 1; j <= n; j++) {
      P[0]  = 1.0;
      P[1]  = P[2] = 0.0;

      for (i = 1; i < j; i++) {
        P[2]  += (float)p[idx[i] - j];  /* j is paired downstream */
        P[0]  -= (float)p[idx[i] - j];  /* j is unpaired */
      }

      for (i = j + 1; i <= n; i++) {
        P[1]  += (float)p[idx[j] - i];  /* j is paired upstream */
        P[0]  -= (float)p[idx[j] - i];  /* j is unpaired */
      }

      if (p_gq) {
        /* do something about the gquads wrapping around the n,1 junction */
        for (i = 1; i < j; i++) {
#ifndef VRNA_DISABLE_C11_FEATURES
          pp    = vrna_smx_csr_get(p_gq, j, i, 0.);
#else
          pp    = vrna_smx_csr_FLT_OR_DBL_get(p_gq, j, i, 0.);
#endif
          P[1]  += (float)pp;  /* j is paired downstream */
          P[0]  -= (float)pp;  /* j is unpaired */
        }

        for (i = j + 1; i <= n; i++) {
#ifndef VRNA_DISABLE_C11_FEATURES
          pp    = vrna_smx_csr_get(p_gq, i, j, 0.);
#else
          pp    = vrna_smx_csr_FLT_OR_DBL_get(p_gq, i, j, 0.);
#endif
          P[2]  += (float)pp;  /* j is paired downstream */
          P[0]  -= (float)pp;  /* j is unpaired */
        }
      }

      s[j - 1] = vrna_bpp_symbol(P);
    }

    s[n] = '\0';
  }

  return s;
}


PUBLIC void
vrna_letter_structure(char            *structure,
                      vrna_bp_stack_t *bp,
                      unsigned int    length)
{
  int   n, k, x, y;
  char  alpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

  if (length > 0) {
    memset(structure, '.', length);
    structure[length] = '\0';

    for (n = 0, k = 1; k <= bp[0].i; k++) {
      y = bp[k].j;
      x = bp[k].i;
      if ((x - 1 > 0) && (y + 1 <= length)) {
        if (structure[x - 2] != ' ' && structure[y] == structure[x - 2]) {
          structure[x - 1]  = structure[x - 2];
          structure[y - 1]  = structure[x - 1];
          continue;
        }
      }

      if ((structure[x] != ' ') && (structure[y - 2] == structure[x])) {
        structure[x - 1]  = structure[x];
        structure[y - 1]  = structure[x - 1];
        continue;
      }

      n++;
      structure[x - 1]  = alpha[n - 1];
      structure[y - 1]  = alpha[n - 1];
    }
  }
}


PUBLIC char *
vrna_db_from_bps(vrna_bps_t   bp_stack,
                 unsigned int length)
{
  size_t        k;
  unsigned int  i, j, tmp;
  int           temp;
  char          *structure;
  vrna_bp_t     bp;

  structure = NULL;

  if (bp_stack) {
    structure = vrna_alloc(sizeof(char) * (length + 1));

    if (length > 0)
      memset(structure, '.', length);

    structure[length] = '\0';

    for (k = 0; k < vrna_bps_size(bp_stack); k++) {
      bp = vrna_bps_at(bp_stack, k);
      i = bp.i;
      j = bp.j;
      if (i > length)
        i -= length;

      if (j > length)
        j -= length;

      if (i > j) {
        temp  = i;
        i     = j;
        j     = temp;
      }

      if (i == j) {
        /* Gquad bonds are marked as bp[i].i == bp[i].j */
        if (bp.L > 0) {
          vrna_db_insert_gq(structure, i, bp.L, bp.l, length);
        } else {
          structure[i - 1] = '+';
        }
      } else {
        /* the following ones are regular base pairs */
        structure[i - 1]  = '(';
        structure[j - 1]  = ')';
      }
    }
  }

  return structure;
}


PUBLIC char *
vrna_db_from_bp_stack(vrna_bp_stack_t *bp,
                      unsigned int    length)
{
  int   k, i, j, temp;
  char  *structure;

  structure = NULL;

  if (bp) {
    structure = vrna_alloc(sizeof(char) * (length + 1));

    if (length > 0)
      memset(structure, '.', length);

    structure[length] = '\0';

    for (k = 1; k <= bp[0].i; k++) {
      i = bp[k].i;
      j = bp[k].j;
      if (i > length)
        i -= length;

      if (j > length)
        j -= length;

      if (i > j) {
        temp  = i;
        i     = j;
        j     = temp;
      }

      if (i == j) {
        /* Gquad bonds are marked as bp[i].i == bp[i].j */
        structure[i - 1] = '+';
      } else {
        /* the following ones are regular base pairs */
        structure[i - 1]  = '(';
        structure[j - 1]  = ')';
      }
    }
  }

  return structure;
}


PUBLIC vrna_ep_t *
vrna_plist(const char *struc,
           float      pr)
{
  /* convert bracket string to plist */
  short     *pt;
  int       i, k = 0, size, n;
  vrna_ep_t *gpl, *ptr, *pl;

  pl = NULL;

  if (struc) {
    size  = strlen(struc);
    n     = 2;

    pt  = vrna_ptable(struc);
    pl  = (vrna_ep_t *)vrna_alloc(n * size * sizeof(vrna_ep_t));
    for (i = 1; i < size; i++) {
      if (pt[i] > i) {
        (pl)[k].i       = i;
        (pl)[k].j       = pt[i];
        (pl)[k].p       = pr;
        (pl)[k++].type  = VRNA_PLIST_TYPE_BASEPAIR;
      }
    }

    gpl = get_plist_gquad_from_db(struc, pr);
    for (ptr = gpl; ptr->i != 0; ptr++) {
      if (k == n * size - 1) {
        n   *= 2;
        pl  = (vrna_ep_t *)vrna_realloc(pl, n * size * sizeof(vrna_ep_t));
      }

      (pl)[k].i       = ptr->i;
      (pl)[k].j       = ptr->j;
      (pl)[k].p       = ptr->p;
      (pl)[k++].type  = ptr->type;
    }
    free(gpl);

    (pl)[k].i       = 0;
    (pl)[k].j       = 0;
    (pl)[k].p       = 0.;
    (pl)[k++].type  = 0.;
    free(pt);
    pl = (vrna_ep_t *)vrna_realloc(pl, k * sizeof(vrna_ep_t));
  }

  return pl;
}


PUBLIC vrna_ep_t *
vrna_plist_from_probs(vrna_fold_compound_t  *vc,
                      double                cut_off)
{
  if (!vc)
    vrna_log_warning("vrna_pl_get_from_pr: run vrna_pf_fold first!");
  else if (!vc->exp_matrices->probs)
    vrna_log_warning("vrna_pl_get_from_pr: probs==NULL!");
  else
    return wrap_plist(vc, cut_off);

  return NULL;
}


PUBLIC char *
vrna_db_from_plist(vrna_ep_t    *pairs,
                   unsigned int n)
{
  vrna_ep_t *ptr;
  char      *structure = NULL;

  if ((n > 0) &&
      (pairs)) {
    structure = (char *)vrna_alloc(sizeof(char) * (n + 1));
    memset(structure, '.', n);
    structure[n] = '\0';

    for (ptr = pairs; (*ptr).i; ptr++) {
      if (((*ptr).i < n) && ((*ptr).j <= n)) {
        structure[(*ptr).i - 1] = '(';
        structure[(*ptr).j - 1] = ')';
      }
    }
  }

  return structure;
}


PUBLIC int
vrna_plist_append(vrna_ep_t       **target,
                  const vrna_ep_t *list)
{
  int             size1, size2;
  const vrna_ep_t *ptr;

  if ((target) && (list)) {
    size1 = size2 = 0;

    if (*target)
      for (ptr = *target; ptr->i; size1++, ptr++);

    for (ptr = list; ptr->i; size2++, ptr++);

    *target = (vrna_ep_t *)vrna_realloc(*target, sizeof(vrna_ep_t) * (size1 + size2 + 1));

    if (*target) {
      memcpy(*target + size1, list, sizeof(vrna_ep_t) * size2);
      (*target)[size1 + size2].i    = (*target)[size1 + size2].j = 0;
      (*target)[size1 + size2].type = 0;
      return 1;
    }
  }

  return 0;
}


PUBLIC vrna_hx_t *
vrna_hx_from_ptable(short *pt)
{
  int       i, k, n, l, s, *stack;
  vrna_hx_t *list;

  list = NULL;

  if (pt) {
    n     = pt[0];
    l     = 0;
    s     = 1;
    list  = (vrna_hx_t *)vrna_alloc(sizeof(vrna_hx_t) * (n / 2 + 2));
    stack = (int *)vrna_alloc(sizeof(int) * (n / 2 + 2));

    stack[s] = 1;

    do {
      for (i = stack[s--]; i <= n; i++) {
        if (pt[i] > (short)i) {
          /* found a base pair */
          k = i;
          /* go through stack */
          for (; pt[k + 1] == pt[k] - 1; k++);
          list[l].start   = i;
          list[l].end     = pt[i];
          list[l].length  = k - i + 1;
          list[l].up5     = list[l].up3 = 0;
          l++;
          stack[++s]  = pt[i] + 1;
          /*
           *  only push enclosed part to stack if there is anything
           *  actually enclosed by the helix
           */
          if (k != pt[i])
            stack[++s]  = k + 1;
          break;
        } else if (pt[i]) {
          /* end of region */
          break;
        }
      }
    } while (s > 0);

    list          = vrna_realloc(list, (l + 1) * sizeof(vrna_hx_t));
    list[l].start = list[l].end = list[l].length = list[l].up5 = list[l].up3 = 0;

    free(stack);
  }

  return list;
}

PRIVATE int
hx_cmp(const void *a,
       const void *b)
{
  if (((vrna_hx_t *)a)->start > ((vrna_hx_t *)b)->start)
    return 1;

  if (((vrna_hx_t *)a)->start < ((vrna_hx_t *)b)->start)
    return -1;

  return 0;
}


PUBLIC vrna_hx_t *
vrna_hx_merge(const vrna_hx_t *list,
              int             maxdist)
{
  int       merged, i, j, s, neighbors, n;
  vrna_hx_t *merged_list;

  merged_list = NULL;

  if (list) {
    for (n = 0; list[n].length > 0; n++); /* check size of list */

    merged_list = (vrna_hx_t *)vrna_alloc(sizeof(vrna_hx_t) * (n + 1));
    memcpy(merged_list, list, sizeof(vrna_hx_t) * (n + 1));

    qsort(merged_list, n, sizeof(vrna_hx_t), hx_cmp);

    s = n + 1;

    do {
      merged = 0;
      for (i = 1; merged_list[i].length > 0; i++) {
        /*
         * GOAL: merge two consecutive helices i and i-1, if i-1
         * subsumes i, and not more than i
         */

        /* 1st, check for neighboring helices or any crossing
         * helices forming a pseudoknot
         */
        unsigned char attempt_merge = 1;
        /* go through list of remaining helices and see whether helix[i - 1]
         * (i) encloses another helix that also encloses helix[i]
         * (ii) encloses another helix apart from [i]
         * (iii) is enclosing parts of another helix forming a pseudoknot, or
         * (iv) helix[i] forms a pseudoknot with another helix not enclosed by helix[i]
         */
        unsigned int is5, ie5, is3, ie3;
        unsigned int i1s5, i1e5, i1s3, i1e3;
        i1s5 = merged_list[i - 1].start;
        i1e3 = merged_list[i - 1].end;
        i1e5 = i1s5 + merged_list[i - 1].length + merged_list[i - 1].up5 - 1;
        i1s3 = i1e3 + 1 - merged_list[i - 1].length - merged_list[i - 1].up3;
        is5 = merged_list[i].start;
        ie3 = merged_list[i].end;
        ie5 = is5 + merged_list[i].length + merged_list[i].up5 - 1;
        is3 = ie3 + 1 - merged_list[i].length - merged_list[i].up3;

        /* do not merge if helices are not nested */
        if ((is5 < i1e5) || (ie3 > i1s3))
          continue;

        /* check for conflicting helices */
        for (j = 0; j < i - 1; j++) {
          /*
           * upstream starting helices can potentially form pseudoknots into the
           * region between the two helices we are about to merge
           */
          unsigned int s5, e5, s3, e3;
          s5 = merged_list[j].start;
          e3 = merged_list[j].end;
          e5 = s5 + merged_list[j].length + merged_list[j].up5 - 1;
          s3 = e3 + 1 - merged_list[j].length - merged_list[j].up3;

          /* do not allow for cases:
           * [.(.(..).].) or [.(.].(..).)
           */
          if ((e5 < i1s5) &&
              (i1e5 < s3) &&
              ((e3 < is5) || ((e3 < i1s3) && (ie3 < s3)))) {
            attempt_merge = 0;
            break;
          }
        }

        if (!attempt_merge)
          continue;

        for (j = i + 1; merged_list[j].length > 0; j++) {
          unsigned int s5, e5, s3, e3;
          s5 = merged_list[j].start;
          e3 = merged_list[j].end;
          e5 = s5 + merged_list[j].length + merged_list[j].up5 - 1;
          s3 = e3 + 1 - merged_list[j].length - merged_list[j].up3;

          if ((i1e3 < s5) ||
              (e3 < i1s5))
            continue;

          if (e5 < i1s5) {
            if (((i1e5 < s3) && (e3 < is5)) ||
                ((ie3 < s3) && (e3 < i1s3))) {
              attempt_merge = 0;
              break;
            }
            continue;
          }

          if ((i1e5 < s5) &&
              (e5 < is5)) {
            if ((ie5 < s3) && (e3 < is3)) {
              attempt_merge = 0;
              break;
            } else if (i1e3 < s3) {
              attempt_merge = 0;
              break;
            } else if (e3 < is5) {
              attempt_merge = 0;
              break;
            } else if ((ie3 < s3) && (e3 < i1s3)) {
              attempt_merge = 0;
              break;
            }
            continue;
          }      

          if ((ie5 < s5) && (e5 < is3) && (ie3 < s3) && (e3 < i1s3)) {
            attempt_merge = 0;
            break;
          }

          if ((ie3 < s5) && (e5 < i1s3)) {
            if ((e3 < i1s3) ||
                (i1e3 < s3)) {
              attempt_merge = 0;
              break;
            }
          }
        }

        if (!attempt_merge)
          continue;

        /* check if we may merge i with i-1 */
        if ((i1e5 < is5) && (ie3 < i1s3)) {
          merged_list[i - 1].up5 += merged_list[i].up5
                                    + (is5 - i1e5 - 1);
          merged_list[i - 1].up3 += merged_list[i].up3
                                    + (i1s3 - ie3 - 1);
          merged_list[i - 1].length += merged_list[i].length;
          /* splice out helix i */
          memmove(merged_list + i, merged_list + i + 1, sizeof(vrna_hx_t) * (n - i));
          s--;
          merged = 1;
          break;
        }
      }
    } while (merged);

    merged_list = vrna_realloc(merged_list, sizeof(vrna_hx_t) * s);
  }

  return merged_list;
}


PUBLIC char *
vrna_db_to_element_string(const char *structure)
{
  char  *elements;
  int   n, i;
  short *pt;

  elements = NULL;

  if (structure) {
    n         = (int)strlen(structure);
    pt        = vrna_ptable(structure);
    elements  = (char *)vrna_alloc(sizeof(char) * (n + 1));

    for (i = 1; i <= n; i++) {
      if (!pt[i]) {
        /* mark nucleotides in exterior loop */
        elements[i - 1] = 'e';
      } else {
        assign_elements_pair(pt, i, pt[i], elements);
        i = pt[i];
      }
    }

    elements[n] = '\0';
    free(pt);
  }

  return elements;
}


PUBLIC char *
vrna_abstract_shapes(const char   *structure,
                     unsigned int level)
{
  if (structure) {
    if (level > 5)
      level = 5;

    return db2shapes(structure, level);
  }

  return NULL;
}


PUBLIC char *
vrna_abstract_shapes_pt(const short   *pt,
                        unsigned int  level)
{
  if (pt) {
    if (level > 5)
      level = 5;

    return db2shapes_pt(pt, (unsigned int)pt[0], level);
  }

  return NULL;
}


/*
 #####################################
 # BEGIN OF STATIC HELPER FUNCTIONS  #
 #####################################
 */
PRIVATE INLINE struct shrep *
shrep_insert_after(struct shrep *target,
                   char         character)
{
  struct shrep *entry, *ptr;

  entry             = (struct shrep *)vrna_alloc(sizeof(struct shrep));
  entry->character  = character;
  entry->pred       = NULL;
  entry->succ       = NULL;

  if (target) {
    for (ptr = target; ptr->succ; ptr = ptr->succ);
    entry->pred = ptr;
    ptr->succ   = entry;
  }

  return entry;
}


PRIVATE INLINE struct shrep *
shrep_insert_before(struct shrep  *target,
                    char          character)
{
  struct shrep *entry, *ptr;

  entry             = (struct shrep *)vrna_alloc(sizeof(struct shrep));
  entry->character  = character;
  entry->pred       = NULL;
  entry->succ       = NULL;

  if (target) {
    for (ptr = target; ptr->pred; ptr = ptr->pred);
    entry->succ = ptr;
    ptr->pred   = entry;
  }

  return entry;
}


PRIVATE INLINE void
shrep_concat(struct shrep **target,
             struct shrep *suffix)
{
  struct shrep *ptr_s, *ptr_p;

  ptr_p = *target;
  ptr_s = suffix;

  if (ptr_p)
    for (; ptr_p->succ; ptr_p = ptr_p->succ);

  if (ptr_s)
    for (; ptr_s->pred; ptr_s = ptr_s->pred);

  if (!ptr_p) {
    *target = ptr_s;
  } else if (ptr_s) {
    ptr_p->succ = ptr_s;
    ptr_s->pred = ptr_p;
  }
}


PRIVATE INLINE char *
db2shapes(const char    *structure,
          unsigned int  level)
{
  unsigned int  n;
  char          *SHAPE;
  short         *pt;

  n   = strlen(structure);
  pt  = vrna_ptable(structure);

  SHAPE = db2shapes_pt(pt, n, level);

  free(pt);

  return SHAPE;
}


PRIVATE INLINE char *
db2shapes_pt(const short  *pt,
             unsigned int n,
             unsigned int level)
{
  unsigned int  start;
  struct shrep  *ptr, *ptr2;
  char          *SHAPE;

  SHAPE = NULL;
  start = 1;

  ptr = get_shrep(pt, start, n, level);

  if (ptr) {
    SHAPE = (char *)vrna_alloc(sizeof(char) * (n + 1));

    for (; ptr->pred; ptr = ptr->pred);

    for (n = 0; ptr; n++) {
      SHAPE[n]  = ptr->character;
      ptr2      = ptr;
      ptr       = ptr->succ;
      free(ptr2);
    }

    free(ptr);

    SHAPE     = vrna_realloc(SHAPE, sizeof(char) * (n + 1));
    SHAPE[n]  = '\0';
  }

  return SHAPE;
}


PRIVATE INLINE struct shrep *
get_shrep(const short   *pt,
          unsigned int  start,
          unsigned int  end,
          unsigned int  level)
{
  struct shrep  *shape_string, *substring, *sub5, *sub3;
  unsigned int  components, i, k, l, ext, u5, u3, cnt;
  unsigned char no_stack, bulge;

  shape_string = NULL;

  /* find out if there is more than one component */
  for (i = start, components = 0; i <= end; i++) {
    if (i < pt[i]) {
      components++;
      i = pt[i];
      if (components > 1)
        break;
    }
  }

  /* find out if we process the exterior loop */
  for (i = start - 1, ext = 1; i > 0; i--) {
    if (pt[i] > end) {
      ext = 0;
      break;
    }
  }

  for (; start <= end; start++) {
    if (start < pt[start]) {
      /* we have base pair (start, pt[start]) */
      substring = sub5 = sub3 = NULL;
      k         = start + 1;
      l         = pt[start] - 1;

      while (1) {
        u5  = 0;
        u3  = 0;

        for (; pt[k] == 0; k++, u5++);  /* skip unpaired 5' side */
        for (; pt[l] == 0; l--, u3++);  /* skip unpaired 3' side */

        if (k >= l) {
          /* hairpin loop */
          if (level == 0)
            for (cnt = 0; cnt < u5; cnt++)
              substring = shrep_insert_after(substring, SHAPES_UP);

          break;
        } else {
          if (pt[k] != l) {
            /* multi branch loop or external loop */
            substring = get_shrep(pt, k, l, level);
            if (level == 1) {
              if (u5)
                sub5 = shrep_insert_after(sub5, SHAPES_UP);

              if (u3)
                sub3 = shrep_insert_before(sub3, SHAPES_UP);
            } else if (level == 0) {
              for (cnt = 0; cnt < u5; cnt++)
                sub5 = shrep_insert_after(sub5, SHAPES_UP);

              for (cnt = 0; cnt < u3; cnt++)
                sub3 = shrep_insert_before(sub3, SHAPES_UP);
            }

            break;
          } else {
            /* interior loop with enclosed pair (k,l) */
            no_stack  = (u5 + u3 > 0) ? 1 : 0;
            bulge     = ((u5 > 0) && (u3 > 0)) ? 1 : 0;

            switch (level) {
              case 4:
                if (bulge) {
                  sub5  = shrep_insert_after(sub5, SHAPES_OPEN);
                  sub3  = shrep_insert_before(sub3, SHAPES_CLOSE);
                }

                break;

              case 3:
                if (no_stack) {
                  sub5  = shrep_insert_after(sub5, SHAPES_OPEN);
                  sub3  = shrep_insert_before(sub3, SHAPES_CLOSE);
                }

                break;

              case 2: /* fall through */
              case 1:
                if (no_stack) {
                  if (u5)
                    sub5 = shrep_insert_after(sub5, SHAPES_UP);

                  if (u3)
                    sub3 = shrep_insert_before(sub3, SHAPES_UP);

                  sub5  = shrep_insert_after(sub5, SHAPES_OPEN);
                  sub3  = shrep_insert_before(sub3, SHAPES_CLOSE);
                }

                break;

              case 0:
                for (unsigned int cnt = 0; cnt < u5; cnt++)
                  sub5 = shrep_insert_after(sub5, SHAPES_UP);

                for (unsigned int cnt = 0; cnt < u3; cnt++)
                  sub3 = shrep_insert_before(sub3, SHAPES_UP);

                sub5  = shrep_insert_after(sub5, SHAPES_OPEN);
                sub3  = shrep_insert_before(sub3, SHAPES_CLOSE);
                break;

              default:
                break;
            }

            k++;
            l--;
          }
        }
      }

      sub5 = shrep_insert_before(sub5, SHAPES_OPEN);
      shrep_concat(&sub5, substring);
      shrep_concat(&sub5, sub3);
      sub5 = shrep_insert_after(sub5, SHAPES_CLOSE);
      shrep_concat(&shape_string, sub5);

      start = pt[start];
    } else if ((pt[start] == 0) && (level < 3)) {
      if (level == 0) {
        shape_string = shrep_insert_after(shape_string, SHAPES_UP);
      } else if ((level == 1) ||
                 ((components < 2) && (ext == 0))) {
        shape_string = shrep_insert_after(shape_string, SHAPES_UP);
        for (; (start <= end) && (pt[start] == 0); start++);
        start--;
      }
    }
  }

  return shape_string;
}


PRIVATE INLINE void
flatten_brackets(char       *string,
                 const char pair[3],
                 const char target[3])
{
  unsigned int i;

  for (i = 0; string[i] != '\0'; i++) {
    if (string[i] == pair[0])
      string[i] = target[0];
    else if (string[i] == pair[1])
      string[i] = target[1];
  }
}


/* requires that pt[0] already contains the length of the string! */
PRIVATE INLINE int
extract_pairs(short       *pt,
              const char  *structure,
              const char  *pair)
{
  const char    *ptr;
  char          open, close;
  short         *stack;
  unsigned int  i, j, n;
  int           hx;

  n     = (unsigned int)pt[0];
  stack = (short *)vrna_alloc(sizeof(short) * (n + 1));

  open  = pair[0];
  close = pair[1];

  for (hx = 0, i = 1, ptr = structure; (i <= n) && (*ptr != '\0'); ptr++, i++) {
    if (*ptr == open) {
      stack[hx++] = i;
    } else if (*ptr == close) {
      j = stack[--hx];

      if (hx < 0) {
        vrna_log_warning("%s\nunbalanced brackets '%2s' found while extracting base pairs",
                             structure,
                             pair);
        free(stack);
        return 0;
      }

      pt[i] = j;
      pt[j] = i;
    }
  }

  free(stack);

  if (hx != 0) {
    vrna_log_warning("%s\nunbalanced brackets '%2s' found while extracting base pairs",
                         structure,
                         pair);
    return 0;
  }

  return 1; /* success */
}


PRIVATE vrna_ep_t *
wrap_get_plist(vrna_mx_pf_t     *matrices,
               int              length,
               int              *index,
               short            *S,
               vrna_exp_param_t *pf_params,
               double           cut_off)
{
  int         i, j, k, n, count, gquad;
  FLT_OR_DBL  *probs, *scale;
  vrna_ep_t   *pl;
  vrna_smx_csr(FLT_OR_DBL) *q_gq;

  probs = matrices->probs;
  q_gq  = matrices->q_gq;
  scale = matrices->scale;
  gquad = pf_params->model_details.gquad;

  count = 0;
  n     = 2;

  /* first guess of the size needed for pl */
  pl = (vrna_ep_t *)vrna_alloc(n * length * sizeof(vrna_ep_t));

  for (i = 1; i < length; i++) {
    for (j = i + 1; j <= length; j++) {
      /* skip all entries below the cutoff */
      if (probs[index[i] - j] < (FLT_OR_DBL)cut_off)
        continue;

      /* do we need to allocate more memory? */
      if (count == n * length - 1) {
        n   *= 2;
        pl  = (vrna_ep_t *)vrna_realloc(pl, n * length * sizeof(vrna_ep_t));
      }

      /* check for presence of gquadruplex */
      if (gquad && (S[i] == 3) && (S[j] == 3)) {
        /* add probability of a gquadruplex at position (i,j)
         * for dot_plot
         */
        (pl)[count].i       = i;
        (pl)[count].j       = j;
        (pl)[count].p       = (float)probs[index[i] - j];
        (pl)[count++].type  = VRNA_PLIST_TYPE_GQUAD;
        /* now add the probabilies of it's actual pairing patterns */
        vrna_ep_t *inner, *ptr;
        inner = get_plist_gquad_from_pr(S, i, j, q_gq, probs, scale, pf_params);
        for (ptr = inner; ptr->i != 0; ptr++) {
          if (count == n * length - 1) {
            n   *= 2;
            pl  = (vrna_ep_t *)vrna_realloc(pl, n * length * sizeof(vrna_ep_t));
          }

          /* check if we've already seen this pair */
          for (k = 0; k < count; k++)
            if (((pl)[k].i == ptr->i) &&
                ((pl)[k].j == ptr->j))
              break;

          (pl)[k].i     = ptr->i;
          (pl)[k].j     = ptr->j;
          (pl)[k].type  = ptr->type;
          if (k == count) {
            (pl)[k].p = ptr->p;
            count++;
          } else {
            (pl)[k].p += ptr->p;
          }
        }
      } else {
        (pl)[count].i       = i;
        (pl)[count].j       = j;
        (pl)[count].p       = (float)probs[index[i] - j];
        (pl)[count++].type  = VRNA_PLIST_TYPE_BASEPAIR;
      }
    }
  }
  /* mark the end of pl */
  (pl)[count].i     = 0;
  (pl)[count].j     = 0;
  (pl)[count].type  = 0;
  (pl)[count++].p   = 0.;
  /* shrink memory to actual size needed */
  pl = (vrna_ep_t *)vrna_realloc(pl, count * sizeof(vrna_ep_t));

  return pl;
}


PRIVATE vrna_ep_t *
wrap_plist(vrna_fold_compound_t *vc,
           double               cut_off)
{
  short             *S;
  unsigned int      with_gquad, with_circ, i, j, k, n, m, length, count;
  int               *index;
  FLT_OR_DBL        *probs;
  vrna_ep_t         *pl;
  vrna_mx_pf_t      *matrices;
  vrna_exp_param_t  *pf_params;
  vrna_smx_csr(FLT_OR_DBL)  *p_gq;

  S           = (vc->type == VRNA_FC_TYPE_SINGLE) ? vc->sequence_encoding2 : vc->S_cons;
  index       = vc->iindx;
  length      = vc->length;
  pf_params   = vc->exp_params;
  matrices    = vc->exp_matrices;
  probs       = matrices->probs;
  with_gquad  = pf_params->model_details.gquad;
  with_circ   = pf_params->model_details.circ;

  count = 0;
  n     = 2;

  /* first guess of the size needed for pl */
  pl = (vrna_ep_t *)vrna_alloc(n * length * sizeof(vrna_ep_t));

  for (i = 1; i < length; i++) {
    for (j = i + 1; j <= length; j++) {
      /* skip all entries below the cutoff */
      if (probs[index[i] - j] < (FLT_OR_DBL)cut_off)
        continue;

      /* do we need to allocate more memory? */
      if (count == n * length - 1) {
        n   *= 2;
        pl  = (vrna_ep_t *)vrna_realloc(pl, n * length * sizeof(vrna_ep_t));
      }

      /* check for presence of gquadruplex */
      if ((with_gquad) &&
          (S[i] == 3) &&
          (S[j] == 3)) {
        /* add probability of a gquadruplex at position (i,j)
         * for dot_plot
         */
        (pl)[count].i       = i;
        (pl)[count].j       = j;
        (pl)[count].p       = (float)probs[index[i] - j];
        (pl)[count++].type  = VRNA_PLIST_TYPE_GQUAD;

        /* now add the probabilies of it's actual pairing patterns */
        vrna_ep_t *inner, *ptr;
        inner = vrna_plist_gquad_from_pr(vc, i, j);
        for (ptr = inner; ptr->i != 0; ptr++) {
          if (count == n * length - 1) {
            n   *= 2;
            pl  = (vrna_ep_t *)vrna_realloc(pl, n * length * sizeof(vrna_ep_t));
          }

          /* check if we've already seen this pair */
          for (k = 0; k < count; k++)
            if (((pl)[k].i == ptr->i) &&
                ((pl)[k].j == ptr->j) &&
                ((pl)[k].type == VRNA_PLIST_TYPE_TRIPLE))
              break;

          (pl)[k].i     = ptr->i;
          (pl)[k].j     = ptr->j;
          (pl)[k].type  = ptr->type;
          if (k == count) {
            (pl)[k].p = ptr->p;
            count++;
          } else {
            (pl)[k].p += ptr->p;
          }
        }
        free(inner);
      } else {
        (pl)[count].i       = i;
        (pl)[count].j       = j;
        (pl)[count].p       = (float)probs[index[i] - j];
        (pl)[count++].type  = VRNA_PLIST_TYPE_BASEPAIR;
      }
    }
  }

  if ((with_gquad) &&
      (with_circ) &&
      (vc->exp_matrices->p_gq)) {
    p_gq = vc->exp_matrices->p_gq;
    unsigned int imin = 2;
    if (imin + VRNA_GQUAD_MAX_BOX_SIZE - 1 <= length)
       imin = length - VRNA_GQUAD_MAX_BOX_SIZE + 1;

    for (i = imin; i <= length; i++) {
      unsigned int jmin = 1;
      unsigned int jmax = VRNA_GQUAD_MAX_BOX_SIZE - 1 - (length - i);
      if (jmax >= length)
        jmax = i - 1;

      for (j = jmin; j <= jmax; j++) {
        FLT_OR_DBL p_g;
#ifndef VRNA_DISABLE_C11_FEATURES
        if ((p_g = vrna_smx_csr_get(p_gq, i, j, 0.)) >= (FLT_OR_DBL)cut_off) {
#else
        if ((p_g = vrna_smx_csr_FLT_OR_DBL_get(p_gq, i, j, 0.)) >= (FLT_OR_DBL)cut_off) {
#endif
          (pl)[count].i       = i;
          (pl)[count].j       = j;
          (pl)[count].p       = (float)p_g;
          (pl)[count++].type  = VRNA_PLIST_TYPE_GQUAD;

          /* now add the probabilies of it's actual pairing patterns */
          vrna_ep_t *inner, *ptr;
          inner = vrna_plist_gquad_from_pr(vc, i, j);
          for (ptr = inner; ptr->i != 0; ptr++) {
            if (count == n * length - 1) {
              n   *= 2;
              pl  = (vrna_ep_t *)vrna_realloc(pl, n * length * sizeof(vrna_ep_t));
            }

            /* check if we've already seen this pair */
            for (k = 0; k < count; k++)
              if (((pl)[k].i == ptr->i) &&
                  ((pl)[k].j == ptr->j) &&
                  ((pl)[k].type == VRNA_PLIST_TYPE_TRIPLE))
                break;

            (pl)[k].i     = ptr->i;
            (pl)[k].j     = ptr->j;
            (pl)[k].type  = ptr->type;
            if (k == count) {
              (pl)[k].p = ptr->p;
              count++;
            } else {
              (pl)[k].p += ptr->p;
            }
          }
          free(inner);
        }
      }
    }
  }

  /* check unstructured domains */
  if (vc->domains_up) {
    vrna_ud_t *domains_up;
    domains_up = vc->domains_up;

    if (domains_up->probs_get) {
      for (i = 1; i <= length; i++)
        for (m = 0; m < domains_up->motif_count; m++) {
          FLT_OR_DBL pp;
          j   = i + domains_up->motif_size[m] - 1;
          pp  = 0.;
          pp  += domains_up->probs_get(vc,
                                       i,
                                       j,
                                       VRNA_UNSTRUCTURED_DOMAIN_EXT_LOOP,
                                       m,
                                       domains_up->data);
          pp += domains_up->probs_get(vc,
                                      i,
                                      j,
                                      VRNA_UNSTRUCTURED_DOMAIN_HP_LOOP,
                                      m,
                                      domains_up->data);
          pp += domains_up->probs_get(vc,
                                      i,
                                      j,
                                      VRNA_UNSTRUCTURED_DOMAIN_INT_LOOP,
                                      m,
                                      domains_up->data);
          pp += domains_up->probs_get(vc,
                                      i,
                                      j,
                                      VRNA_UNSTRUCTURED_DOMAIN_MB_LOOP,
                                      m,
                                      domains_up->data);
          if (pp >= (FLT_OR_DBL)cut_off) {
            /* do we need to allocate more memory? */
            if (count == n * length - 1) {
              n   *= 2;
              pl  = (vrna_ep_t *)vrna_realloc(pl, n * length * sizeof(vrna_ep_t));
            }

            (pl)[count].i       = i;
            (pl)[count].j       = j;
            (pl)[count].p       = (float)pp;
            (pl)[count++].type  = VRNA_PLIST_TYPE_UD_MOTIF;
          }
        }
    }
  }

  /* mark the end of pl */
  (pl)[count].i     = 0;
  (pl)[count].j     = 0;
  (pl)[count].type  = 0;
  (pl)[count++].p   = 0.;
  /* shrink memory to actual size needed */
  pl = (vrna_ep_t *)vrna_realloc(pl, count * sizeof(vrna_ep_t));

  return pl;
}


PRIVATE void
assign_elements_pair(short  *pt,
                     int    i,
                     int    j,
                     char   *elements)
{
  int p, k, num_pairs;

  num_pairs = 0;
  /* first, determine the number of pairs (i,j) is enclosing */
  for (k = i + 1; k < j; k++) {
    if (k < pt[k]) {
      num_pairs++;
      k = pt[k];
    }
  }

  switch (num_pairs) {
    case 0:   /* hairpin loop */
      elements[i - 1] = elements[j - 1] = 'H';
      for (k = i + 1; k < j; k++)
        elements[k - 1] = 'h';
      break;

    case 1:   /* interior loop */
      elements[i - 1] = elements[j - 1] = 'I';
      p               = 0;
      for (k = i + 1; k < j; k++) {
        if (!pt[k]) {
          elements[k - 1] = 'i';
        } else {
          p = k;
          k = pt[k];
        }
      }

      if (p)
        assign_elements_pair(pt, p, pt[p], elements);

      break;

    default:  /* multibranch loop */
      elements[i - 1] = elements[j - 1] = 'M';
      for (k = i + 1; k < j; k++) {
        if (!pt[k]) {
          elements[k - 1] = 'm';
        } else {
          assign_elements_pair(pt, k, pt[k], elements);
          k = pt[k];
        }
      }
      break;
  }
}


#ifndef VRNA_DISABLE_BACKWARD_COMPATIBILITY

/*
 * ###########################################
 * # deprecated functions below              #
 *###########################################
 */


PUBLIC char *
pack_structure(const char *struc)
{
  return vrna_db_pack(struc);
}


PUBLIC char *
unpack_structure(const char *packed)
{
  return vrna_db_unpack(packed);
}


PUBLIC void
parenthesis_structure(char            *structure,
                      vrna_bp_stack_t *bp,
                      int             length)
{
  char *s = vrna_db_from_bp_stack(bp, length);

  strncpy(structure, s, length + 1);
  free(s);
}


PUBLIC void
letter_structure(char             *structure,
                 vrna_bp_stack_t  *bp,
                 int              length)
{
  vrna_letter_structure(structure, bp, length);
}


PUBLIC void
parenthesis_zuker(char            *structure,
                  vrna_bp_stack_t *bp,
                  int             length)
{
  char *s = vrna_db_from_bp_stack(bp, length);

  strncpy(structure, s, length + 1);
  free(s);
}


PUBLIC void
assign_plist_from_pr(vrna_ep_t  **pl,
                     FLT_OR_DBL *probs,
                     int        length,
                     double     cut_off)
{
  int               *index;
  vrna_mx_pf_t      *matrices;
  vrna_md_t         md;
  vrna_exp_param_t  *pf_params;

  index     = vrna_idx_row_wise(length);
  matrices  = (vrna_mx_pf_t *)vrna_alloc(sizeof(vrna_mx_pf_t));

  set_model_details(&md);
  md.gquad        = 0;
  pf_params       = vrna_exp_params(&md);
  matrices->probs = probs;

  *pl = wrap_get_plist(matrices,
                       length,
                       index,
                       NULL,
                       pf_params,
                       cut_off);

  free(index);
  free(pf_params);
  free(matrices);
}


PUBLIC void
assign_plist_from_db(vrna_ep_t  **pl,
                     const char *struc,
                     float      pr)
{
  *pl = vrna_plist(struc, pr);
}


PUBLIC short *
make_pair_table(const char *structure)
{
  return vrna_ptable(structure);
}


PUBLIC short *
copy_pair_table(const short *pt)
{
  return vrna_ptable_copy(pt);
}


PUBLIC short *
make_pair_table_pk(const char *structure)
{
  return vrna_pt_pk_get(structure);
}


PUBLIC short *
make_pair_table_snoop(const char *structure)
{
  return vrna_pt_snoop_get(structure);
}


PUBLIC short *
alimake_pair_table(const char *structure)
{
  return vrna_pt_ali_get(structure);
}


PUBLIC int *
make_loop_index_pt(short *pt)
{
  return vrna_loopidx_from_ptable((const short *)pt);
}


PUBLIC int
bp_distance(const char  *str1,
            const char  *str2)
{
  return vrna_bp_distance(str1, str2);
}


PUBLIC unsigned int *
make_referenceBP_array(short        *reference_pt,
                       unsigned int turn)
{
  return vrna_refBPcnt_matrix((const short *)reference_pt, turn);
}


PUBLIC unsigned int *
compute_BPdifferences(short         *pt1,
                      short         *pt2,
                      unsigned int  turn)
{
  return vrna_refBPdist_matrix((const short *)pt1, (const short *)pt2, turn);
}


PUBLIC char
bppm_symbol(const float *x)
{
  return vrna_bpp_symbol(x);
}


PUBLIC void
bppm_to_structure(char          *structure,
                  FLT_OR_DBL    *p,
                  unsigned int  length)
{
  char *s = vrna_db_from_probs((const FLT_OR_DBL *)p, length);

  memcpy(structure, s, length);
  structure[length] = '\0';
  free(s);
}


#endif
