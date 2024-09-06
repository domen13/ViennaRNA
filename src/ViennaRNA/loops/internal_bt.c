#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include "ViennaRNA/fold_vars.h"
#include "ViennaRNA/alphabet.h"
#include "ViennaRNA/utils/basic.h"
#include "ViennaRNA/constraints/hard.h"
#include "ViennaRNA/constraints/soft.h"
#include "ViennaRNA/loops/external.h"
#include "ViennaRNA/loops/gquad.h"
#include "ViennaRNA/structured_domains.h"
#include "ViennaRNA/unstructured_domains.h"
#include "ViennaRNA/loops/internal.h"


#ifdef __GNUC__
# define INLINE inline
#else
# define INLINE
#endif

#include "internal_hc.inc"
#include "internal_sc.inc"

/*
 #################################
 # PRIVATE FUNCTION DECLARATIONS #
 #################################
 */

PRIVATE int
bt_int_loop(vrna_fold_compound_t  *fc,
            unsigned int          i,
            unsigned int          j,
            int                   en,
            vrna_bps_t            bp_stack,
            vrna_bts_t            bt_stack);


PRIVATE int
bt_stacked_pairs(vrna_fold_compound_t *fc,
         unsigned int                  i,
         unsigned int                  j,
         int                  *en,
         vrna_bps_t           bp_stack,
         vrna_bts_t           bt_stack);


/*
 #################################
 # BEGIN OF FUNCTION DEFINITIONS #
 #################################
 */
PUBLIC int
vrna_bt_int_loop(vrna_fold_compound_t *fc,
                 unsigned int         i,
                 unsigned int         j,
                 int                  en,
                 vrna_bps_t           bp_stack,
                 vrna_bts_t           bt_stack)
{
  if ((fc) &&
      (bp_stack) &&
      (bt_stack)) {
    return bt_int_loop(fc, i, j, en, bp_stack, bt_stack);
  }

  return 0;
}


PUBLIC int
vrna_bt_stacked_pairs(vrna_fold_compound_t  *fc,
              unsigned int                  i,
              unsigned int                  j,
              int                           *en,
              vrna_bps_t            bp_stack,
              vrna_bts_t            bt_stack)
{
  if (fc)
    return bt_stacked_pairs(fc, i, j, en, bp_stack, bt_stack);

  return 0;
}


PRIVATE int
bt_stacked_pairs(vrna_fold_compound_t *fc,
         unsigned int                 i,
         unsigned int                 j,
         int                          *en,
         vrna_bps_t           bp_stack,
         vrna_bts_t           bt_stack)
{
  unsigned char         sliding_window, eval_loop, hc_decompose_ij, hc_decompose_pq;
  char                  *ptype, **ptype_local;
  short                 **SS;
  unsigned int          n, n_seq, s, *sn, type, type_2;
  int                   ret, eee, ij, p, q, *idx, *my_c, **c_local, *rtype;
  vrna_param_t          *P;
  vrna_md_t             *md;
  vrna_hc_t             *hc;
  eval_hc               evaluate;
  struct hc_int_def_dat hc_dat_local;
  struct sc_int_dat     sc_wrapper;

  sliding_window  = (fc->hc->type == VRNA_HC_WINDOW) ? 1 : 0;
  n               = fc->length;
  n_seq           = (fc->type == VRNA_FC_TYPE_SINGLE) ? 1 : fc->n_seq;
  sn              = fc->strand_number;
  SS              = (fc->type == VRNA_FC_TYPE_SINGLE) ? NULL : fc->S;
  ptype           = (sliding_window) ? NULL : fc->ptype;
  ptype_local     = (sliding_window) ? fc->ptype_local : NULL;
  idx             = (sliding_window) ? NULL : fc->jindx;
  P               = fc->params;
  md              = &(P->model_details);
  hc              = fc->hc;
  my_c            = (sliding_window) ? NULL : fc->matrices->c;
  c_local         = (sliding_window) ? fc->matrices->c_local : NULL;
  ij              = (sliding_window) ? 0 : idx[j] + i;
  rtype           = &(md->rtype[0]);
  p               = i + 1;
  q               = j - 1;
  ret             = 0;
  evaluate        = prepare_hc_int_def(fc, &hc_dat_local);


  if ((sn[p] != sn[i]) || (sn[j] != sn[q]))
    return 0;

  init_sc_int(fc, &sc_wrapper);

  eee = (sliding_window) ? c_local[i][j - i] : my_c[ij];

  if (eee == *en) {
    /*  always true, if (i.j) closes canonical structure,
     * thus (i+1.j-1) must be a pair
     */
    hc_decompose_ij = (sliding_window) ? hc->matrix_local[i][j - i] : hc->mx[n * i + j];
    hc_decompose_pq = (sliding_window) ? hc->matrix_local[p][q - p] : hc->mx[n * p + q];

    eval_loop = (hc_decompose_ij & VRNA_CONSTRAINT_CONTEXT_INT_LOOP) &&
                (hc_decompose_pq & VRNA_CONSTRAINT_CONTEXT_INT_LOOP_ENC);

    if (eval_loop && evaluate(i, j, p, q, &hc_dat_local)) {
      switch (fc->type) {
        case VRNA_FC_TYPE_SINGLE:
          type = (sliding_window) ?
                 vrna_get_ptype_window(i, j, ptype_local) :
                 vrna_get_ptype(ij, ptype);
          type_2 = (sliding_window) ?
                   rtype[vrna_get_ptype_window(p, q, ptype_local)] :
                   rtype[vrna_get_ptype(idx[q] + p, ptype)];

          /* regular stack */
          *en -= P->stack[type][type_2];

          break;

        case VRNA_FC_TYPE_COMPARATIVE:
          for (s = 0; s < n_seq; s++) {
            type    = vrna_get_ptype_md(SS[s][i], SS[s][j], md);
            type_2  = vrna_get_ptype_md(SS[s][q], SS[s][p], md);
            *en     -= P->stack[type][type_2];
          }

          *en += (sliding_window) ? fc->pscore_local[i][j - i] : fc->pscore[ij];

          break;
      }

      if (sc_wrapper.pair)
        *en -= sc_wrapper.pair(i, j, p, q, &sc_wrapper);

      vrna_bps_push(bp_stack,
                    (vrna_bp_t){
                      .i  = p,
                      .j  = q
                    });

      vrna_bts_push(bt_stack,
                    (vrna_sect_t){
                      .i  = p,
                      .j  = q,
                      .ml = VRNA_MX_FLAG_C
                    });
      ret = 1;
    }
  }

  free_sc_int(&sc_wrapper);

  return ret;
}


PRIVATE int
bt_int_loop(vrna_fold_compound_t  *fc,
            unsigned int          i,
            unsigned int          j,
            int                   en,
            vrna_bps_t            bp_stack,
            vrna_bts_t            bt_stack)
{
  unsigned char         sliding_window, hc_decompose_ij, hc_decompose_pq;
  unsigned char         eval_loop;
  short                 *S2, **SS;
  unsigned int          n, n_seq, s, *sn, type, *tt;
  int                   ij, p, q, minq, *idx, no_close, energy, *my_c,
                        **c_local, ret;
  vrna_param_t          *P;
  vrna_md_t             *md;
  vrna_hc_t             *hc;
  eval_hc               evaluate;
  struct hc_int_def_dat hc_dat_local;

  ret             = 0;
  sliding_window  = (fc->hc->type == VRNA_HC_WINDOW) ? 1 : 0;
  n               = fc->length;
  n_seq           = (fc->type == VRNA_FC_TYPE_SINGLE) ? 1 : fc->n_seq;
  sn              = fc->strand_number;
  S2              = (fc->type == VRNA_FC_TYPE_SINGLE) ? fc->sequence_encoding2 : NULL;
  SS              = (fc->type == VRNA_FC_TYPE_SINGLE) ? NULL : fc->S;
  idx             = (sliding_window) ? NULL : fc->jindx;
  P               = fc->params;
  md              = &(P->model_details);
  hc              = fc->hc;
  my_c            = (sliding_window) ? NULL : fc->matrices->c;
  c_local         = (sliding_window) ? fc->matrices->c_local : NULL;
  ij              = (sliding_window) ? 0 : idx[j] + i;
  tt              = NULL;
  evaluate        = prepare_hc_int_def(fc, &hc_dat_local);

  hc_decompose_ij = (sliding_window) ? hc->matrix_local[i][j - i] : hc->mx[n * i + j];

  if (hc_decompose_ij & VRNA_CONSTRAINT_CONTEXT_INT_LOOP) {
    for (p = i + 1; p <= MIN2(j - 2, i + MAXLOOP + 1); p++) {
      minq = j - i + p - MAXLOOP - 2;
      if (minq < p + 1)
        minq = p + 1;

      if (hc->up_int[i + 1] < (p - i - 1))
        break;

      for (q = j - 1; q >= minq; q--) {
        if (hc->up_int[q + 1] < (j - q - 1))
          break;

        hc_decompose_pq = (sliding_window) ?
                          hc->matrix_local[p][q - p] :
                          hc->mx[n * p + q];

        eval_loop = hc_decompose_pq & VRNA_CONSTRAINT_CONTEXT_INT_LOOP_ENC;

        if (!(eval_loop && evaluate(i, j, p, q, &hc_dat_local)))
          continue;

        energy = (sliding_window) ?
                 c_local[p][q - p] :
                 my_c[idx[q] + p];

        energy += vrna_eval_int_loop(fc, i, j, p, q);

        if (energy == en) {
          vrna_sc_t *sc =
            (fc->type == VRNA_FC_TYPE_SINGLE) ? fc->sc : (fc->scs ? fc->scs[0] : NULL);

          if (sc) {
            if (sc->bt) {
              vrna_basepair_t *ptr, *aux_bps;
              aux_bps = sc->bt(i, j, p, q, VRNA_DECOMP_PAIR_IL, sc->data);
              for (ptr = aux_bps; ptr && ptr->i != 0; ptr++) {
                vrna_bps_push(bp_stack,
                              (vrna_bp_t){
                                .i = ptr->i,
                                .j = ptr->j
                              });
              }
              free(aux_bps);
            }
          }

          vrna_bts_push(bt_stack,
                        ((vrna_sect_t){
                          .i = p,
                          .j = q,
                          .ml = VRNA_MX_FLAG_C}));
          ret = 1; /* success */
          goto bt_int_exit;
        }
      }
    }

    /* is it a g-quadruplex? */
    if (md->gquad) {
      /*
       * The case that is handled here actually resembles something like
       * an interior loop where the enclosing base pair is of regular
       * kind and the enclosed pair is not a canonical one but a g-quadruplex
       * that should then be decomposed further...
       */
      switch (fc->type) {
        case VRNA_FC_TYPE_SINGLE:
          type = (sliding_window) ?
                 vrna_get_ptype_window(i, j, fc->ptype_local) :
                 vrna_get_ptype(ij, fc->ptype);
          no_close = (((type == 3) || (type == 4)) && md->noGUclosure);

          if (sliding_window) {
            if ((!no_close) && (sn[j] == sn[i])) {
              if (backtrack_GQuad_IntLoop_L(en, i, j, type, S2, fc->matrices->ggg_local,
                                            fc->window_size, &p, &q, P)) {
                if (vrna_bt_gquad_mfe(fc, p, q, bp_stack)) {
                  i  = j = -1; /* tell the calling block to continue backtracking with next block */
                  ret = 1;
                }
              }
            }
          } else {
            if ((!no_close) && (sn[j] == sn[i])) {
              if (vrna_bt_gquad_int(fc, i, j, en, bp_stack, bt_stack)) {
                ret = 1; /* success */
                goto bt_int_exit;
              }
            }
          }

          break;

        case VRNA_FC_TYPE_COMPARATIVE:
          tt = (unsigned int *)vrna_alloc(sizeof(unsigned int) * n_seq);

          for (s = 0; s < n_seq; s++)
            tt[s] = vrna_get_ptype_md(SS[s][i], SS[s][j], md);

          if (sliding_window) {
            if (backtrack_GQuad_IntLoop_L_comparative(en, i, j, tt, fc->S_cons, fc->S5, fc->S3,
                                                      fc->a2s,
                                                      fc->matrices->ggg_local, &p, &q, n_seq,
                                                      P)) {
              if (vrna_bt_gquad_mfe(fc, p, q, bp_stack)) {
                i  = j = -1; /* tell the calling block to continue backtracking with next block */
                ret = 1;
              }
            }
          } else {
            if (vrna_bt_gquad_int(fc, i, j, en, bp_stack, bt_stack)) {
              ret = 1; /* success */
              goto bt_int_exit;
            }
          }

          break;
      }
    }
  }

bt_int_exit:

  free(tt);

  return ret; /* unsuccessful */
}


#ifndef VRNA_DISABLE_BACKWARD_COMPATIBILITY

PUBLIC int
vrna_BT_int_loop(vrna_fold_compound_t *fc,
                 int                  *i,
                 int                  *j,
                 int                  en,
                 vrna_bp_stack_t      *bp_stack,
                 unsigned int         *stack_count)
{
  int r = 0;

  if ((fc) &&
      (i) &&
      (j) &&
      (en) &&
      (bp_stack) &&
      (stack_count)) {
    vrna_bps_t  bps = vrna_bps_init(0);
    vrna_bts_t  bts = vrna_bts_init(0);
    r = bt_int_loop(fc, *i, *j, en, bps, bts);

    while (vrna_bps_size(bps) > 0) {
      vrna_bp_t bp = vrna_bps_pop(bps);
      bp_stack[++(*stack_count)].i = bp.i;
      bp_stack[*stack_count].j = bp.j;
    }

    if (vrna_bts_size(bts) > 0) {
      /* assuming that the only element pushed on the stack is an entry in the C matrix */
      vrna_sect_t s = vrna_bts_pop(bts);
      *i = s.i;
      *j = s.j;
      /* add base pair to base pair stack for backward compatibility */
      bp_stack[++(*stack_count)].i = s.i;
      bp_stack[*stack_count].j = s.j;
    }

    vrna_bps_free(bps);
    vrna_bts_free(bts);
  }

  return r;
}


PUBLIC int
vrna_BT_stack(vrna_fold_compound_t  *fc,
              int                   *i,
              int                   *j,
              int                   *en,
              vrna_bp_stack_t       *bp_stack,
              unsigned int          *stack_count)
{
  int r = 0;

  if ((fc) &&
      (i) &&
      (j) &&
      (en) &&
      (bp_stack) &&
      (stack_count)) {
    vrna_bps_t  bps = vrna_bps_init(0);
    vrna_bts_t  bts = vrna_bts_init(0);
    r = bt_stacked_pairs(fc, *i, *j, en, bps, bts);

    while (vrna_bps_size(bps) > 0) {
      vrna_bp_t bp = vrna_bps_pop(bps);
      bp_stack[++(*stack_count)].i = bp.i;
      bp_stack[*stack_count].j = bp.j;
    }

    if (vrna_bts_size(bts) > 0) {
      /* assuming that the only element pushed on the stack is an entry in the C matrix */
      vrna_sect_t s = vrna_bts_pop(bts);
      *i = s.i;
      *j = s.j;
      /* add base pair to base pair stack for backward compatibility */
      bp_stack[++(*stack_count)].i = s.i;
      bp_stack[*stack_count].j = s.j;
    }

    vrna_bps_free(bps);
    vrna_bts_free(bts);
  }

  return r;
}

#endif
