#ifndef VIENNA_RNA_PACKAGE_PROBING_SHAPE_H
#define VIENNA_RNA_PACKAGE_PROBING_SHAPE_H

#include <ViennaRNA/fold_compound.h>
#include <ViennaRNA/probing/basic.h>

/**
 *  @file     ViennaRNA/probing/SHAPE.h
 *  @ingroup SHAPE_reactivities
 *  @brief  This module provides function to incorporate RNA structure
 *          probing data, e.g. SHAPE reactivities, into the folding recursions
 *          by means of soft constraints
 */


/**
 *  @ingroup SHAPE_reactivities
 */
void
vrna_constraints_add_SHAPE(vrna_fold_compound_t *fc,
                           const char           *shape_file,
                           const char           *shape_method,
                           const char           *shape_conversion,
                           int                  verbose,
                           unsigned int         constraint_type);


/**
 *  @ingroup SHAPE_reactivities
 */
void
vrna_constraints_add_SHAPE_ali(vrna_fold_compound_t *fc,
                               const char           *shape_method,
                               const char           **shape_files,
                               const int            *shape_file_association,
                               int                  verbose,
                               unsigned int         constraint_type);


/**
 *  @brief  Add SHAPE reactivity data as soft constraints (Deigan et al. method)
 *
 *  This approach of SHAPE directed RNA folding uses the simple linear ansatz
 *
 *  @f[ \Delta G_{\text{SHAPE}}(i) = m \ln(\text{SHAPE reactivity}(i)+1)+ b @f]
 *
 *  to convert SHAPE reactivity values to pseudo energies whenever a
 *  nucleotide @f$ i @f$ contributes to a stacked pair. A positive slope @f$ m @f$
 *  penalizes high reactivities in paired regions, while a negative intercept @f$ b @f$
 *  results in a confirmatory ``bonus'' free energy for correctly predicted base pairs.
 *  Since the energy evaluation of a base pair stack involves two pairs, the pseudo
 *  energies are added for all four contributing nucleotides. Consequently, the
 *  energy term is applied twice for pairs inside a helix and only once for pairs
 *  adjacent to other structures. For all other loop types the energy model remains
 *  unchanged even when the experimental data highly disagrees with a certain motif.
 *
 *  @note For further details, we refer to @rstinline :cite:t:`deigan:2009` @endrst.
 *
 *  @see  vrna_sc_remove(), vrna_sc_add_SHAPE_zarringhalam(), vrna_sc_minimize_pertubation()
 *
 *  @ingroup SHAPE_reactivities
 *  @param  fc            The #vrna_fold_compound_t the soft constraints are associated with
 *  @param  reactivities  A vector of normalized SHAPE reactivities
 *  @param  m             The slope of the conversion function
 *  @param  b             The intercept of the conversion function
 *  @param  options       The options flag indicating how/where to store the soft constraints
 *  @return               1 on successful extraction of the method, 0 on errors
 */
int
vrna_sc_add_SHAPE_deigan(vrna_fold_compound_t *fc,
                         const double         *reactivities,
                         double               m,
                         double               b,
                         unsigned int         options);


/**
 *  @brief  Add SHAPE reactivity data from files as soft constraints for consensus structure prediction (Deigan et al. method)
 *
 *  @ingroup SHAPE_reactivities
 *  @param  fc            The #vrna_fold_compound_t the soft constraints are associated with
 *  @param  shape_files   A set of filenames that contain normalized SHAPE reactivity data
 *  @param  shape_file_association  An array of integers that associate the files with sequences in the alignment
 *  @param  m             The slope of the conversion function
 *  @param  b             The intercept of the conversion function
 *  @param  options       The options flag indicating how/where to store the soft constraints
 *  @return               1 on successful extraction of the method, 0 on errors
 */
int
vrna_sc_add_SHAPE_deigan_ali(vrna_fold_compound_t *fc,
                             const char           **shape_files,
                             const int            *shape_file_association,
                             double               m,
                             double               b,
                             unsigned int         options);


/**
 *  @brief  Add SHAPE reactivity data as soft constraints (Zarringhalam et al. method)
 *
 *  This method first converts the observed SHAPE reactivity of nucleotide @f$ i @f$ into a
 *  probability @f$ q_i @f$ that position @f$ i @f$ is unpaired by means of a non-linear map.
 *  Then pseudo-energies of the form
 *
 *  @f[ \Delta G_{\text{SHAPE}}(x,i) = \beta\ |x_i - q_i| @f]
 *
 *  are computed, where @f$ x_i=0 @f$ if position @f$ i @f$ is unpaired and @f$ x_i=1 @f$
 *  if @f$ i @f$ is paired in a given secondary structure. The parameter @f$ \beta @f$ serves as
 *  scaling factor. The magnitude of discrepancy between prediction and experimental observation
 *  is represented by @f$ |x_i - q_i| @f$.
 *
 *  @note For further details, we refer to @rstinline :cite:t:`zarringhalam:2012` @endrst
 *
 *  @see  vrna_sc_remove(), vrna_sc_add_SHAPE_deigan(), vrna_sc_minimize_pertubation()
 *
 *  @ingroup SHAPE_reactivities
 *  @param  fc                The #vrna_fold_compound_t the soft constraints are associated with
 *  @param  reactivities      A vector of normalized SHAPE reactivities
 *  @param  b                 The scaling factor @f$ \beta @f$ of the conversion function
 *  @param  default_value     The default value for a nucleotide where reactivity data is missing for
 *  @param  shape_conversion  A flag that specifies how to convert reactivities to probabilities
 *  @param  options           The options flag indicating how/where to store the soft constraints
 *  @return                   1 on successful extraction of the method, 0 on errors
 */
int
vrna_sc_add_SHAPE_zarringhalam(vrna_fold_compound_t *fc,
                               const double         *reactivities,
                               double               b,
                               double               default_value,
                               const char           *shape_conversion,
                               unsigned int         options);


/**
 *  @brief  Parse a character string and extract the encoded SHAPE reactivity conversion
 *          method and possibly the parameters for conversion into pseudo free energies
 *
 *  @ingroup soft_cosntraints
 *
 *  @param  method_string   The string that contains the encoded SHAPE reactivity conversion method
 *  @param  method          A pointer to the memory location where the method character will be stored
 *  @param  param_1         A pointer to the memory location where the first parameter of the corresponding method will be stored
 *  @param  param_2         A pointer to the memory location where the second parameter of the corresponding method will be stored
 *  @return                 1 on successful extraction of the method, 0 on errors
 */
int
vrna_sc_SHAPE_parse_method(const char *method_string,
                           char       *method,
                           float      *param_1,
                           float      *param_2);



/**
 *  @brief  Add SHAPE reactivity data as soft constraints (Eddy/RNAprob-2 method)
 *
 *  This approach of SHAPE directed RNA folding uses the probability framework proposed by Eddy
 *
 *  @f[ \Delta G_{\text{SHAPE}}(i) = - RT\ln(\mathbb{P}(\text{SHAPE reactivity}(i)\mid x_i\pi_i))+ @f]
 *
 *  to convert SHAPE reactivity values to pseudo energies for given nucleotide @f$ x_i @f$ and
 *  pairedness @f$ \pi_i @f$ at position @f$ i @f$. The reactivity distribution is computed
 *  using Gaussian kernel density estimation (KDE) with bandwidth @f$ h @f$ computed using
 *  Scott factor
 * 
 * @f[ h = n^{-\frac{1}{5}} @f]
 *
 * where @f$ n @f$ is the number of data points.
 *
 *
 *  @ingroup SHAPE_reactivities
 *  @param  fc            The #vrna_fold_compound_t the soft constraints are associated with
 *  @param  reactivities  A vector of normalized SHAPE reactivities
 *  @param  unpaired_nb   Length of the array of unpaired SHAPE reactivities
 *  @param  unpaired_data Pointer to an array of unpaired SHAPE reactivities
 *  @param  paired_nb     Length of the array of paired SHAPE reactivities
 *  @param  paired_data   Pointer to an array of paired SHAPE reactivities
 *  @return               1 on successful extraction of the method, 0 on errors
 */
int
vrna_sc_add_SHAPE_eddy_2(vrna_fold_compound_t *fc,
                         const double         *reactivities,
                         int                  unpaired_nb,
                         const double         *unpaired_data,
                         int                  paired_nb,
                         const double         *paired_data);

#endif
