/*
  ESP32 FFT
  =========
  This provides a vanilla radix-2 FFT implementation and a test example.
  Author
  ------
  This code was written by [Robin Scheibler](http://www.robinscheibler.org) during rainy days in October 2017.

 "Classified" for ESP and ARduino by M. Steltman Mey 2021
  
  License
  -------
  Copyright (c) 2017 Robin Scheibler
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include "ESP32_fft.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>

/*
 * Constructor Prepare an FFT of correct size and types.
 *
 * If no input or output buffers are provided, they will be allocated.
 */
ESP_fft::ESP_fft(int size, int samplefreq,fft_type_t type, fft_direction_t direction, float input[], float output[]) :
  _size(size), _samplefreq(samplefreq), _type(type),_direction(direction),_input(input),_output(output)  {

  // Check if the size is a power of two
  if ((size & (size-1)) != 0)  // tests if size is a power of two
    return ;

  _binwidth = (float)_samplefreq/(_size/2.0);

  // start configuration
  _flags = 0;

  // Allocate and precompute twiddle factors
  _twiddle_factors = (float *)malloc(2 * _size * sizeof(float));

  // Allocate input buffer
  if (_input == NULL) {
    if (_type == FFT_REAL)
      _input = (float *)malloc(_size * sizeof(float));
    else if (_type == FFT_COMPLEX)
      _input = (float *)malloc(2 * _size * sizeof(float));
    _flags |= FFT_OWN_INPUT_MEM;
  }

  // Allocate output buffer
  if (_output == NULL)  {
    if (_type == FFT_REAL) 
      _output = (float *)malloc(_size * sizeof(float));
    else if (_type == FFT_COMPLEX)
      _output = (float *)malloc(2 * _size * sizeof(float));
    _flags |= FFT_OWN_OUTPUT_MEM;
  }

//   If malloc failed a constructing an instace there is something seriously wrong.
//   Hence: abort(), pointless to continue
//
  if (_input == NULL || _output == NULL || _twiddle_factors == NULL) abort();

  // init twiddle
  float two_pi_by_n = TWO_PI / _size;

  for (int k = 0, m = 0 ; k < _size ; k++, m+=2)
  {
    _twiddle_factors[m] = cosf(two_pi_by_n * k);    // real
    _twiddle_factors[m+1] = sinf(two_pi_by_n * k);  // imag
  }
}

// release memory
ESP_fft::~ESP_fft()
{
  if (_flags & FFT_OWN_INPUT_MEM)
    free(_input);

  if (_flags & FFT_OWN_OUTPUT_MEM)
    free(_output);

  free(_twiddle_factors);
}

void ESP_fft::complexToMagnitude() {

  for (int k = 1 ; k < _size/ 2 ; k++)
  {
    /*The real part of a magnitude at a frequency is followed by the corresponding imaginary part in the output*/
    float mag = sqrt(pow(_output[2*k],2) + pow(_output[2*k+1],2))/1;
    float freq = frequency(k);
//    sprintf(print_buf,"%f Hz : %f", freq, mag);
//    Serial.println(print_buf);
    if(mag > _max_mag)
    {
        _max_mag = mag;
        _fundamental_freq = freq;
    }
    _output[k] = mag;
  }
}

// Remove DC by removing any signal offeset from average
//
void ESP_fft::removeDC() {
  float avg = 0;
  for (int k = 1 ; k < _size ; k++) {
    avg += _input[k];
  }
  avg/= _size;
  for (int k = 1 ; k < _size; k++) {
    _input[k] -= avg;
  }
}



// Hamming, copied from Arduino FFT
//
//
void ESP_fft::hammingWindow() {

  float samplesMinusOne = (float(_size) - 1.0);
  for (uint16_t i = 0; i < (_size >> 1); i++) {
    float indexMinusOne = float(i);
    float ratio = (indexMinusOne / samplesMinusOne);
    float weighingFactor = 0.54 - (0.46 * cos(TWO_PI * ratio));
    _input[i] *= weighingFactor;
    _input[_size - (i + 1)] *= weighingFactor;
  }
}


// execute
//
void ESP_fft::execute()
{

  if (_type == FFT_REAL) {
      if (_direction == FFT_FORWARD)  
        rfft();
      else
      if (_direction == FFT_BACKWARD) 
        irfft();
  } else if (_type == FFT_COMPLEX) {
      if (_direction == FFT_FORWARD)  
        fft();
      else
      if (_direction == FFT_BACKWARD) 
        ifft();
    }
}

//
// Privat helpers, unmodified from c
// not necessary to pass parameters
void ESP_fft::fft()
{
  /*
   * Forward fast Fourier transform
   * DIT, radix-2, out-of-place implementation
   *
   * Parameters
   * ----------
   *  input (float *)
   *    The input array containing the complex samples with
   *    real/imaginary parts interleaved [Re(x0), Im(x0), ..., Re(x_n-1), Im(x_n-1)]
   *  output (float *)
   *    The output array containing the complex samples with
   *    real/imaginary parts interleaved [Re(x0), Im(x0), ..., Re(x_n-1), Im(x_n-1)]
   *  n (int)
   *    The FFT size, should be a power of 2
   */

#if USE_SPLIT_RADIX
  split_radix_fft(_input, _output, _size, 2, 2);
#else
  fft_primitive(_input, _output, _size, 2, 2);
#endif
}

void ESP_fft::ifft()
{
  /*
   * Inverse fast Fourier transform
   * DIT, radix-2, out-of-place implementation
   *
   * Parameters
   * ----------
   *  input (float *)
   *    The input array containing the complex samples with
   *    real/imaginary parts interleaved [Re(x0), Im(x0), ..., Re(x_n-1), Im(x_n-1)]
   *  output (float *)
   *    The output array containing the complex samples with
   *    real/imaginary parts interleaved [Re(x0), Im(x0), ..., Re(x_n-1), Im(x_n-1)]
   *  n (int)
   *    The FFT size, should be a power of 2
   */
  ifft_primitive(_input, _output, _size, 2, 2);
}

void ESP_fft::rfft()
{

  float *x = _input;
  float *y = _output;
  int n = _size;
  
// This code uses the two-for-the-price-of-one strategy
#if USE_SPLIT_RADIX
  split_radix_fft(x, y, n / 2, 2, 4);
#else
  fft_primitive  (x, y, n / 2, 2, 4);
#endif

  // Now apply post processing to recover positive
  // frequencies of the real FFT
  float t = y[0];
  y[0] = t + y[1];  // DC coefficient
  y[1] = t - y[1];  // Center coefficient

  // Apply post processing to quarter element
  // this boils down to taking complex conjugate
  y[n/2+1] = -y[n/2+1];

  // Now process all the other frequencies
  int k;
  for (k = 2 ; k < n / 2 ; k += 2)
  {
    float xer, xei, x0r, xoi, c, s, tr, ti;

    c = _twiddle_factors[k];
    s = _twiddle_factors[k+1];
    
    // even half coefficient
    xer = 0.5 * (y[k] + y[n-k]);
    xei = 0.5 * (y[k+1] - y[n-k+1]);

    // odd half coefficient
    x0r = 0.5 * (y[k+1] + y[n-k+1]);
    xoi = - 0.5 * (y[k] - y[n-k]);

    tr =  c * x0r + s * xoi;
    ti = -s * x0r + c * xoi;

    y[k]   = xer + tr;
    y[k+1] = xei + ti;

    y[n-k]   =   xer - tr;
    y[n-k+1] = -(xei - ti);
  }
}

void ESP_fft::irfft()
{
  /*
   * Destroys content of input vector
   */
  int k;
  float *x = _input;
  float *y = _output;
  int n = _size;
  
  // Here we need to apply a pre-processing first
  float t = x[0];
  x[0] = 0.5 * (t + x[1]);
  x[1] = 0.5 * (t - x[1]);

  x[n/2+1] = -x[n/2+1];

  for (k = 2 ; k < n / 2 ; k += 2)
  {
    float xer, xei, x0r, xoi, c, s, tr, ti;

    c = _twiddle_factors[k];
    s = _twiddle_factors[k+1];

    xer = 0.5 * (x[k] + x[n-k]);
    tr  = 0.5 * (x[k] - x[n-k]);

    xei = 0.5 * (x[k+1] - x[n-k+1]);
    ti  = 0.5 * (x[k+1] + x[n-k+1]);

    x0r = c * tr - s * ti;
    xoi = s * tr + c * ti;

    x[k]   = xer - xoi;
    x[k+1] = x0r + xei;

    x[n-k]   = xer + xoi;
    x[n-k+1] = x0r - xei;
  }

  ifft_primitive(x, y, n / 2, 2,4);
}

void ESP_fft::fft_primitive(float *x, float *y, int n, int stride,int tw_stride)
{
  /*
   * This code will compute the FFT of the input vector x
   *
   * The input data is assumed to be real/imag interleaved
   *
   * The size n should be a power of two
   *
   * y is an output buffer of size 2n to accomodate for complex numbers
   *
   * Forward fast Fourier transform
   * DIT, radix-2, out-of-place implementation
   *
   * For a complex FFT, call first stage as:
   * fft(x, y, n, 2, 2);
   *
   * Parameters
   * ----------
   *  x (float *)
   *    The input array containing the complex samples with
   *    real/imaginary parts interleaved [Re(x0), Im(x0), ..., Re(x_n-1), Im(x_n-1)]
   *  y (float *)
   *    The output array containing the complex samples with
   *    real/imaginary parts interleaved [Re(x0), Im(x0), ..., Re(x_n-1), Im(x_n-1)]
   *  n (int)
   *    The FFT size, should be a power of 2
   *  stride (int)
   *    The number of elements to skip between two successive samples
   *  tw_stride (int)
   *    The number of elements to skip between two successive twiddle factors
   */
  int k;
  float t;

#if LARGE_BASE_CASE
  // End condition, stop at n=8 to avoid one trivial recursion
  if (n == 8)
  {
    fft8(x, stride, y, 2);
    return;
  }
#else
  // End condition, stop at n=2 to avoid one trivial recursion
  if (n == 2)
  {
    y[0] = x[0] + x[stride];
    y[1] = x[1] + x[stride + 1];
    y[2] = x[0] - x[stride];
    y[3] = x[1] - x[stride + 1];
    return;
  }
#endif

  // Recursion -- Decimation In Time algorithm
  fft_primitive(x, y, n / 2, 2 * stride,  2 * tw_stride);             // even half
  fft_primitive(x + stride, y+n, n / 2, 2 * stride, 2 * tw_stride);  // odd half

  // Stitch back together

  // We can a few multiplications in the first step
  t = y[0];
  y[0] = t + y[n];
  y[n] = t - y[n];

  t = y[1];
  y[1] = t + y[n+1];
  y[n+1] = t - y[n+1];

  for (k = 1 ; k < n / 2 ; k++)
  {
    float x1r, x1i, x2r, x2i, c, s;
    c = _twiddle_factors[k * tw_stride];
    s = _twiddle_factors[k * tw_stride + 1];

    x1r = y[2 * k];
    x1i = y[2 * k + 1];
    x2r =  c * y[n + 2 * k] + s * y[n + 2 * k + 1];
    x2i = -s * y[n + 2 * k] + c * y[n + 2 * k + 1];

    y[2 * k] = x1r + x2r;
    y[2 * k + 1] = x1i + x2i;

    y[n + 2 * k] = x1r - x2r;
    y[n + 2 * k + 1] = x1i - x2i;
  }

}

void ESP_fft::split_radix_fft(float *x, float *y, int n, int stride,int tw_stride)
{
  /*
   * This code will compute the FFT of the input vector x
   *
   * The input data is assumed to be real/imag interleaved
   *
   * The size n should be a power of two
   *
   * y is an output buffer of size 2n to accomodate for complex numbers
   *
   * Forward fast Fourier transform
   * Split-Radix
   * DIT, radix-2, out-of-place implementation
   *
   * For a complex FFT, call first stage as:
   * fft(x, y, n, 2, 2);
   *
   * Parameters
   * ----------
   *  x (float *)
   *    The input array containing the complex samples with
   *    real/imaginary parts interleaved [Re(x0), Im(x0), ..., Re(x_n-1), Im(x_n-1)]
   *  y (float *)
   *    The output array containing the complex samples with
   *    real/imaginary parts interleaved [Re(x0), Im(x0), ..., Re(x_n-1), Im(x_n-1)]
   *  n (int)
   *    The FFT size, should be a power of 2
   *  stride (int)
   *    The number of elements to skip between two successive samples
   *  _twiddle_factors (float *)
   *    The array of twiddle factors
   *  tw_stride (int)
   *    The number of elements to skip between two successive twiddle factors
   */
  int k;

#if LARGE_BASE_CASE
  // End condition, stop at n=2 to avoid one trivial recursion
  if (n == 8)
  {
    fft8(x, stride, y, 2);
    return;
  }
  else if (n == 4)
  {
    fft4(x, stride, y, 2);
    return;
  }
#else
  // End condition, stop at n=2 to avoid one trivial recursion
  if (n == 2)
  {
    y[0] = x[0] + x[stride];
    y[1] = x[1] + x[stride + 1];
    y[2] = x[0] - x[stride];
    y[3] = x[1] - x[stride + 1];
    return;
  }
  else if (n == 1)
  {
    y[0] = x[0];
    y[1] = x[1];
    return;
  }
#endif

  // Recursion -- Decimation In Time algorithm
  split_radix_fft(x, y, n / 2, 2 * stride, 2 * tw_stride);
  split_radix_fft(x + stride, y + n, n / 4, 4 * stride, 4 * tw_stride);
  split_radix_fft(x + 3 * stride, y + n + n / 2, n / 4, 4 * stride, 4 * tw_stride);

  // Stitch together the output
  float u1r, u1i, u2r, u2i, x1r, x1i, x2r, x2i;
  float t;

  // We can save a few multiplications in the first step
  u1r = y[0];
  u1i = y[1];
  u2r = y[n / 2];
  u2i = y[n / 2 + 1];

  x1r = y[n];
  x1i = y[n + 1];
  x2r = y[n / 2 + n];
  x2i = y[n / 2 + n + 1];

  t = x1r + x2r;
  y[0] = u1r + t;
  y[n]     = u1r - t;

  t = x1i + x2i;
  y[1] = u1i + t;
  y[n + 1] = u1i - t;

  t = x2i - x1i;
  y[n / 2]     = u2r - t;
  y[n + n / 2]     = u2r + t;

  t = x1r - x2r;
  y[n / 2 + 1] = u2i - t;
  y[n + n / 2 + 1] = u2i + t;

  for (k = 1 ; k < n / 4 ; k++)
  {
    float u1r, u1i, u2r, u2i, x1r, x1i, x2r, x2i, c1, s1, c2, s2;
    c1 = _twiddle_factors[k * tw_stride];
    s1 = _twiddle_factors[k * tw_stride + 1];
    c2 = _twiddle_factors[3 * k * tw_stride];
    s2 = _twiddle_factors[3 * k * tw_stride + 1];

    u1r = y[2 * k];
    u1i = y[2 * k + 1];
    u2r = y[2 * k + n / 2];
    u2i = y[2 * k + n / 2 + 1];

    x1r =  c1 * y[n + 2 * k] + s1 * y[n + 2 * k + 1];
    x1i = -s1 * y[n + 2 * k] + c1 * y[n + 2 * k + 1];
    x2r =  c2 * y[n / 2 + n + 2 * k] + s2 * y[n / 2 + n + 2 * k + 1];
    x2i = -s2 * y[n / 2 + n + 2 * k] + c2 * y[n / 2 + n + 2 * k + 1];

    t = x1r + x2r;
    y[2 * k]     = u1r + t;
    y[2 * k + n]     = u1r - t;

    t = x1i + x2i;
    y[2 * k + 1] = u1i + t;
    y[2 * k + n + 1] = u1i - t;

    t = x2i - x1i;
    y[2 * k + n / 2]     = u2r - t;
    y[2 * k + n + n / 2]     = u2r + t;

    t = x1r - x2r;
    y[2 * k + n / 2 + 1] = u2i - t;
    y[2 * k + n + n / 2 + 1] = u2i + t;
  }

}

void ESP_fft::ifft_primitive(float *x, float *y, int n, int stride, int tw_stride)
{

#if USE_SPLIT_RADIX
  split_radix_fft(x, y, n, stride, tw_stride);
#else
  fft_primitive(x, y, n, stride, tw_stride);
#endif

  int ks;

  int ns = n * stride;

  // reverse all coefficients from 1 to n / 2 - 1
  for (ks = stride ; ks < ns / 2 ; ks += stride)
  {
    float t;

    t = y[ks];
    y[ks] = y[ns-ks];
    y[ns-ks] = t;

    t = y[ks+1];
    y[ks+1] = y[ns-ks+1];
    y[ns-ks+1] = t;
  }

  // Apply normalization
  float norm = 1. / n;
  for (ks = 0 ; ks < ns ; ks += stride)
  {
    y[ks]   *= norm;
    y[ks+1] *= norm;
  }

}
// in class, so default is inlined
//
void ESP_fft::fft8(float *input, int stride_in, float *output, int stride_out)
{
  /*
   * Unrolled implementation of FFT8 for a little more performance
   */
  float a0r, a1r, a2r, a3r, a4r, a5r, a6r, a7r;
  float a0i, a1i, a2i, a3i, a4i, a5i, a6i, a7i;
  float b0r, b1r, b2r, b3r, b4r, b5r, b6r, b7r;
  float b0i, b1i, b2i, b3i, b4i, b5i, b6i, b7i;
  float t;
  float sin_pi_4 = 0.7071067812;

  a0r = input[0];
  a0i = input[1];
  a1r = input[stride_in];
  a1i = input[stride_in+1];
  a2r = input[2*stride_in];
  a2i = input[2*stride_in+1];
  a3r = input[3*stride_in];
  a3i = input[3*stride_in+1];
  a4r = input[4*stride_in];
  a4i = input[4*stride_in+1];
  a5r = input[5*stride_in];
  a5i = input[5*stride_in+1];
  a6r = input[6*stride_in];
  a6i = input[6*stride_in+1];
  a7r = input[7*stride_in];
  a7i = input[7*stride_in+1];

  // Stage 1

  b0r = a0r + a4r;
  b0i = a0i + a4i;

  b1r = a1r + a5r;
  b1i = a1i + a5i;

  b2r = a2r + a6r;
  b2i = a2i + a6i;

  b3r = a3r + a7r;
  b3i = a3i + a7i;

  b4r = a0r - a4r;
  b4i = a0i - a4i;

  b5r = a1r - a5r;
  b5i = a1i - a5i;
  // W_8^1 = 1/sqrt(2) - j / sqrt(2)
  t = b5r + b5i;
  b5i = (b5i - b5r) * sin_pi_4;
  b5r = t * sin_pi_4;

  // W_8^2 = -j
  b6r = a2i - a6i;
  b6i = a6r - a2r;

  b7r = a3r - a7r;
  b7i = a3i - a7i;
  // W_8^3 = -1 / sqrt(2) + j / sqrt(2)
  t = sin_pi_4 * (b7i - b7r);
  b7i = - (b7r + b7i) * sin_pi_4;
  b7r = t;

  // Stage 2

  a0r = b0r + b2r;
  a0i = b0i + b2i;

  a1r = b1r + b3r;
  a1i = b1i + b3i;

  a2r = b0r - b2r;
  a2i = b0i - b2i;

  // * j
  a3r = b1i - b3i;
  a3i = b3r - b1r;

  a4r = b4r + b6r;
  a4i = b4i + b6i;

  a5r = b5r + b7r;
  a5i = b5i + b7i;

  a6r = b4r - b6r;
  a6i = b4i - b6i;

  // * j
  a7r = b5i - b7i;
  a7i = b7r - b5r;

  // Stage 3

  // X[0]
  output[0] = a0r + a1r;
  output[1] = a0i + a1i;

  // X[4]
  output[4*stride_out] = a0r - a1r;
  output[4*stride_out+1] = a0i - a1i;

  // X[2]
  output[2*stride_out] = a2r + a3r;
  output[2*stride_out+1] = a2i + a3i;

  // X[6]
  output[6*stride_out] = a2r - a3r;
  output[6*stride_out+1] = a2i - a3i;

  // X[1]
  output[stride_out] = a4r + a5r;
  output[stride_out+1] = a4i + a5i;

  // X[5]
  output[5*stride_out] = a4r - a5r;
  output[5*stride_out+1] = a4i - a5i;

  // X[3]
  output[3*stride_out] = a6r + a7r;
  output[3*stride_out+1] = a6i + a7i;

  // X[7]
  output[7*stride_out] = a6r - a7r;
  output[7*stride_out+1] = a6i - a7i;

}

void ESP_fft::fft4(float *input, int stride_in, float *output, int stride_out)
{
  /*
   * Unrolled implementation of FFT4 for a little more performance
   */
  float t1, t2;

  t1 = input[0] + input[2*stride_in];
  t2 = input[stride_in] + input[3*stride_in];
  output[0] = t1 + t2;
  output[2*stride_out] = t1 - t2;

  t1 = input[1] + input[2*stride_in+1];
  t2 = input[stride_in+1] + input[3*stride_in+1];
  output[1] = t1 + t2;
  output[2*stride_out+1] = t1 - t2;

  t1 = input[0] - input[2*stride_in];
  t2 = input[stride_in+1] - input[3*stride_in+1];
  output[stride_out] = t1 + t2;
  output[3*stride_out] = t1 - t2;

  t1 = input[1] - input[2*stride_in+1];
  t2 = input[3*stride_in] - input[stride_in];
  output[stride_out+1] = t1 + t2;
  output[3*stride_out+1] = t1 - t2;
}