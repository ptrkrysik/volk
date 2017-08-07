/* -*- c++ -*- */
/*
 */

/*!
 * \page volk_32f_s32f_s32f_mod_range_32f
 *
 * \b Overview
 *
 * Performs FM-detect differentiation on the input vector and stores
 * the results in the output vector.
 *
 * <b>Dispatcher Prototype</b>
 * \code
 * void volk_32f_s32f_s32f_mod_range_32f(float* outputVector, const float* inputVector, const float bound, float* saveValue, unsigned int num_points)
 * \endcode
 *
 * \b Inputs
 * \li inputVector: The input vector containing phase data (must be on the interval (-bound, bound]).
 * \li bound: The interval that the input phase data is in, which is used to modulo the differentiation.
 * \li saveValue: A pointer to a float which contains the phase value of the sample before the first input sample.
 * \li num_points The number of data points.
 *
 * \b Outputs
 * \li outputVector: The vector where the results will be stored.
 *
 * \b Example
 * \code
 * int N = 10000;
 *
 * <FIXME>
 *
 * volk_32f_s32f_s32f_mod_range_32f();
 *
 * \endcode
 */

#ifndef INCLUDED_VOLK_32F_S32F_S32F_MOD_RANGE_32F_A_H
#define INCLUDED_VOLK_32F_S32F_S32F_MOD_RANGE_32F_A_H

#ifdef LV_HAVE_AVX
#include <xmmintrin.h>

static inline void volk_32f_s32f_s32f_mod_range_32f_u_avx(float* outputVector, const float* inputVector, const float lower_bound, const float upper_bound, unsigned int num_points){
  __m256 lower = _mm256_set1_ps(lower_bound);
  __m256 upper = _mm256_set1_ps(upper_bound);
  __m256 distance = _mm256_sub_ps(upper,lower);
  float dist = upper_bound - lower_bound;
  __m256 input, output;
  __m256 is_smaller, is_bigger;
  __m256 excess, adj;

  const float *inPtr = inputVector;
  float *outPtr = outputVector;
  size_t eight_points = num_points / 8;
  for(size_t counter = 0; counter < eight_points; counter++) {
    input = _mm256_loadu_ps(inPtr);
    // calculate mask: input < lower, input > upper
    is_smaller = _mm256_cmp_ps(input, lower, 0x11); //0x11: Less than, ordered, non-signalling
    is_smaller = _mm256_cmp_ps(input, upper, 0x1e); //0x1e: greater than, ordered, non-signalling
    // find out how far we are out-of-bound – positive values!
    excess = _mm256_and_ps(_mm256_sub_ps(lower, input), is_smaller);
    excess = _mm256_or_ps(_mm256_and_ps(_mm256_sub_ps(input, upper), is_bigger), excess);
    // how many do we have to add? (int(excess/distance+1)*distance)
    excess = _mm256_div_ps(excess, distance);
    // round down
    excess = _mm256_cvtepi32_ps(_mm256_cvttps_epi32(excess));
    // plus 1
    adj = _mm256_set1_ps(1.0f);
    excess = _mm256_add_ps(excess, adj);
    // get the sign right, adj is still {1.0f,1.0f,1.0f,1.0f}
    adj = _mm256_and_ps(adj, is_smaller);
    adj = _mm256_or_ps(_mm256_and_ps(_mm256_set1_ps(-1.0f), is_bigger), adj);
    // scale by distance, sign
    excess = _mm256_mul_ps(_mm_mul256_ps(excess, adj), distance);
    output = _mm256_add_ps(input, excess);
    _mm256_storeu_ps(outPtr, output);
    inPtr += 8;
    outPtr += 8;
  }

  for(size_t counter = eight_points * 8; counter < num_points; counter++){
    float val = inputVector[counter];
    if(val < lower_bound){
      float excess = lower_bound - val;
      signed int count = (int)(excess/dist);
      outputVector[counter] = val + (count+1)*dist;
    }
    else if(val > upper_bound){
      float excess = val - upper_bound;
      signed int count = (int)(excess/dist);
      outputVector[counter] = val - (count+1)*dist;
    }
    else
      outputVector[counter] = val;
  }
}
#endif /* LV_HAVE_AVX */


#ifdef LV_HAVE_SSE2
#include <xmmintrin.h>

static inline void volk_32f_s32f_s32f_mod_range_32f_u_sse2(float* outputVector, const float* inputVector, const float lower_bound, const float upper_bound, unsigned int num_points){
  __m128 lower = _mm_set_ps1(lower_bound);
  __m128 upper = _mm_set_ps1(upper_bound);
  __m128 distance = _mm_sub_ps(upper,lower);
  float dist = upper_bound - lower_bound;
  __m128 input, output;
  __m128 is_smaller, is_bigger;
  __m128 excess, adj;

  const float *inPtr = inputVector;
  float *outPtr = outputVector;
  size_t quarter_points = num_points / 4;
  for(size_t counter = 0; counter < quarter_points; counter++) {
    input = _mm_load_ps(inPtr);
    // calculate mask: input < lower, input > upper
    is_smaller = _mm_cmplt_ps(input, lower);
    is_bigger = _mm_cmpgt_ps(input, upper);
    // find out how far we are out-of-bound – positive values!
    excess = _mm_and_ps(_mm_sub_ps(lower, input), is_smaller);
    excess = _mm_or_ps(_mm_and_ps(_mm_sub_ps(input, upper), is_bigger), excess);
    // how many do we have to add? (int(excess/distance+1)*distance)
    excess = _mm_div_ps(excess, distance);
    // round down
    excess = _mm_cvtepi32_ps(_mm_cvttps_epi32(excess));
    // plus 1
    adj = _mm_set_ps1(1.0f);
    excess = _mm_add_ps(excess, adj);
    // get the sign right, adj is still {1.0f,1.0f,1.0f,1.0f}
    adj = _mm_and_ps(adj, is_smaller);
    adj = _mm_or_ps(_mm_and_ps(_mm_set_ps1(-1.0f), is_bigger), adj);
    // scale by distance, sign
    excess = _mm_mul_ps(_mm_mul_ps(excess, adj), distance);
    output = _mm_add_ps(input, excess);
    _mm_store_ps(outPtr, output);
    inPtr += 4;
    outPtr += 4;
  }

  for(size_t counter = quarter_points * 4; counter < num_points; counter++){
    float val = inputVector[counter];
    if(val < lower_bound){
      float excess = lower_bound - val;
      signed int count = (int)(excess/dist);
      outputVector[counter] = val + (count+1)*dist;
    }
    else if(val > upper_bound){
      float excess = val - upper_bound;
      signed int count = (int)(excess/dist);
      outputVector[counter] = val - (count+1)*dist;
    }
    else
      outputVector[counter] = val;
  }
}
static inline void volk_32f_s32f_s32f_mod_range_32f_a_sse2(float* outputVector, const float* inputVector, const float lower_bound, const float upper_bound, unsigned int num_points){
  __m128 lower = _mm_set_ps1(lower_bound);
  __m128 upper = _mm_set_ps1(upper_bound);
  __m128 distance = _mm_sub_ps(upper,lower);
  __m128 input, output;
  __m128 is_smaller, is_bigger;
  __m128 excess, adj;

  const float *inPtr = inputVector;
  float *outPtr = outputVector;
  size_t quarter_points = num_points / 4;
  for(size_t counter = 0; counter < quarter_points; counter++) {
    input = _mm_load_ps(inPtr);
    // calculate mask: input < lower, input > upper
    is_smaller = _mm_cmplt_ps(input, lower);
    is_bigger = _mm_cmpgt_ps(input, upper);
    // find out how far we are out-of-bound – positive values!
    excess = _mm_and_ps(_mm_sub_ps(lower, input), is_smaller);
    excess = _mm_or_ps(_mm_and_ps(_mm_sub_ps(input, upper), is_bigger), excess);
    // how many do we have to add? (int(excess/distance+1)*distance)
    excess = _mm_div_ps(excess, distance);
    // round down – for some reason, SSE doesn't come with a 4x float -> 4x int32 conversion.
    excess = _mm_cvtepi32_ps(_mm_cvttps_epi32(excess));
    // plus 1
    adj = _mm_set_ps1(1.0f);
    excess = _mm_add_ps(excess, adj);
    // get the sign right, adj is still {1.0f,1.0f,1.0f,1.0f}
    adj = _mm_and_ps(adj, is_smaller);
    adj = _mm_or_ps(_mm_and_ps(_mm_set_ps1(-1.0f), is_bigger), adj);
    // scale by distance, sign
    excess = _mm_mul_ps(_mm_mul_ps(excess, adj), distance);
    output = _mm_add_ps(input, excess);
    _mm_store_ps(outPtr, output);
    inPtr += 4;
    outPtr += 4;
  }

  float dist = upper_bound - lower_bound;
  for(size_t counter = quarter_points * 4; counter < num_points; counter++){
    float val = inputVector[counter];
    if(val < lower_bound){
      float excess = lower_bound - val;
      signed int count = (int)(excess/dist);
      outputVector[counter] = val + (count+1)*dist;
    }
    else if(val > upper_bound){
      float excess = val - upper_bound;
      signed int count = (int)(excess/dist);
      outputVector[counter] = val - (count+1)*dist;
    }
    else
      outputVector[counter] = val;
  }
}
#endif /* LV_HAVE_SSE2 */

#ifdef LV_HAVE_SSE
#include <xmmintrin.h>

static inline void volk_32f_s32f_s32f_mod_range_32f_u_sse(float* outputVector, const float* inputVector, const float lower_bound, const float upper_bound, unsigned int num_points){
  __m128 lower = _mm_set_ps1(lower_bound);
  __m128 upper = _mm_set_ps1(upper_bound);
  __m128 distance = _mm_sub_ps(upper,lower);
  float dist = upper_bound - lower_bound;
  __m128 input, output;
  __m128 is_smaller, is_bigger;
  __m128 excess, adj;
  __m64 lo, hi;

  const float *inPtr = inputVector;
  float *outPtr = outputVector;
  size_t quarter_points = num_points / 4;
  for(size_t counter = 0; counter < quarter_points; counter++) {
    input = _mm_load_ps(inPtr);
    // calculate mask: input < lower, input > upper
    is_smaller = _mm_cmplt_ps(input, lower);
    is_bigger = _mm_cmpgt_ps(input, upper);
    // find out how far we are out-of-bound – positive values!
    excess = _mm_and_ps(_mm_sub_ps(lower, input), is_smaller);
    excess = _mm_or_ps(_mm_and_ps(_mm_sub_ps(input, upper), is_bigger), excess);
    // how many do we have to add? (int(excess/distance+1)*distance)
    excess = _mm_div_ps(excess, distance);
    // round down – for some reason, SSE doesn't come with a 4x float -> 4x int32 conversion.
    // we're using adj as temp variable
    adj = _mm_setzero_ps();
    adj = _mm_movehl_ps(adj, excess);
    lo = _mm_cvttps_pi32(excess);
    hi = _mm_cvttps_pi32(adj);
    excess = _mm_cvtpi32x2_ps(lo,hi);
    // plus 1
    adj = _mm_set_ps1(1.0f);
    excess = _mm_add_ps(excess, adj);
    // get the sign right, adj is still {1.0f,1.0f,1.0f,1.0f}
    adj = _mm_and_ps(adj, is_smaller);
    adj = _mm_or_ps(_mm_and_ps(_mm_set_ps1(-1.0f), is_bigger), adj);
    // scale by distance, sign
    excess = _mm_mul_ps(_mm_mul_ps(excess, adj), distance);
    output = _mm_add_ps(input, excess);
    _mm_store_ps(outPtr, output);
    inPtr += 4;
    outPtr += 4;
  }

  for(size_t counter = quarter_points * 4; counter < num_points; counter++){
    float val = inputVector[counter];
    if(val < lower_bound){
      float excess = lower_bound - val;
      signed int count = (int)(excess/dist);
      outputVector[counter] = val + (count+1)*dist;
    }
    else if(val > upper_bound){
      float excess = val - upper_bound;
      signed int count = (int)(excess/dist);
      outputVector[counter] = val - (count+1)*dist;
    }
    else
      outputVector[counter] = val;
  }
}
static inline void volk_32f_s32f_s32f_mod_range_32f_a_sse(float* outputVector, const float* inputVector, const float lower_bound, const float upper_bound, unsigned int num_points){
  __m128 lower = _mm_set_ps1(lower_bound);
  __m128 upper = _mm_set_ps1(upper_bound);
  __m128 distance = _mm_sub_ps(upper,lower);
  __m128 input, output;
  __m128 is_smaller, is_bigger;
  __m128 excess, adj;
  __m64 lo, hi;

  const float *inPtr = inputVector;
  float *outPtr = outputVector;
  size_t quarter_points = num_points / 4;
  for(size_t counter = 0; counter < quarter_points; counter++) {
    input = _mm_load_ps(inPtr);
    // calculate mask: input < lower, input > upper
    is_smaller = _mm_cmplt_ps(input, lower);
    is_bigger = _mm_cmpgt_ps(input, upper);
    // find out how far we are out-of-bound – positive values!
    excess = _mm_and_ps(_mm_sub_ps(lower, input), is_smaller);
    excess = _mm_or_ps(_mm_and_ps(_mm_sub_ps(input, upper), is_bigger), excess);
    // how many do we have to add? (int(excess/distance+1)*distance)
    excess = _mm_div_ps(excess, distance);
    // round down – for some reason, SSE doesn't come with a 4x float -> 4x int32 conversion.
    // we're using adj as temp variable
    adj = _mm_setzero_ps();
    adj = _mm_movehl_ps(adj, excess);
    lo = _mm_cvttps_pi32(excess);
    hi = _mm_cvttps_pi32(adj);
    excess = _mm_cvtpi32x2_ps(lo,hi);
    // plus 1
    adj = _mm_set_ps1(1.0f);
    excess = _mm_add_ps(excess, adj);
    // get the sign right, adj is still {1.0f,1.0f,1.0f,1.0f}
    adj = _mm_and_ps(adj, is_smaller);
    adj = _mm_or_ps(_mm_and_ps(_mm_set_ps1(-1.0f), is_bigger), adj);
    // scale by distance, sign
    excess = _mm_mul_ps(_mm_mul_ps(excess, adj), distance);
    output = _mm_add_ps(input, excess);
    _mm_store_ps(outPtr, output);
    inPtr += 4;
    outPtr += 4;
  }

  float dist = upper_bound - lower_bound;
  for(size_t counter = quarter_points * 4; counter < num_points; counter++){
    float val = inputVector[counter];
    if(val < lower_bound){
      float excess = lower_bound - val;
      signed int count = (int)(excess/dist);
      outputVector[counter] = val + (count+1)*dist;
    }
    else if(val > upper_bound){
      float excess = val - upper_bound;
      signed int count = (int)(excess/dist);
      outputVector[counter] = val - (count+1)*dist;
    }
    else
      outputVector[counter] = val;
  }
}
#endif /* LV_HAVE_SSE */

#ifdef LV_HAVE_GENERIC

static inline void volk_32f_s32f_s32f_mod_range_32f_generic(float* outputVector, const float* inputVector, const float lower_bound, const float upper_bound, unsigned int num_points){
  float* outPtr = outputVector;
  float distance = upper_bound - lower_bound;

  for(const float *inPtr = inputVector; inPtr < inputVector + num_points; inPtr++){
    float val = *inPtr;
    if(val < lower_bound){
      float excess = lower_bound - val;
      signed int count = (int)(excess/distance);
      *outPtr = val + (count+1)*distance;
    }
    else if(val > upper_bound){
      float excess = val - upper_bound;
      signed int count = (int)(excess/distance);
      *outPtr = val - (count+1)*distance;
    }
    else
      *outPtr = val;
    outPtr++;
  }
}
#endif /* LV_HAVE_GENERIC */




#endif /* INCLUDED_VOLK_32F_S32F_S32F_MOD_RANGE_32F_A_H */
