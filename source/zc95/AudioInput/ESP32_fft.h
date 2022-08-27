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
  in the Software withoaut restriction, including without limitation the rights
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

typedef enum
{
  FFT_REAL,
  FFT_COMPLEX
} fft_type_t;

typedef enum
{
  FFT_FORWARD,
  FFT_BACKWARD
} fft_direction_t;

#ifndef TWO_PI 
#define TWO_PI 6.28318530
#endif

#define FFT_OWN_INPUT_MEM 1
#define FFT_OWN_OUTPUT_MEM 2
#define USE_SPLIT_RADIX 1
#define LARGE_BASE_CASE 1

class ESP_fft {

public:
  ESP_fft(int size, int samplefreq,fft_type_t type, fft_direction_t direction, float input[], float output[]);
  ~ESP_fft();

  void  hammingWindow();
  void  removeDC();
  void  execute();
  void  complexToMagnitude();
  float majorPeak()       {return _max_mag ;}
  float majorPeakFreq()   {return _fundamental_freq ;}
  float binWidth()        {return _binwidth; }
  float frequency(int i)  {return i * _binwidth; }

private:
  int             _size;          // FFT size
  int             _samplefreq;
  float           _binwidth;      // freq per bin
  float        *  _input;         // pointer to input buffer
  float        *  _output;        // pointer to output buffer
  float        *  _twiddle_factors;  // pointer to buffer holding twiddle factors
  fft_type_t      _type;          // real or complex
  fft_direction_t _direction;     // forward or backward
  unsigned int    _flags;         // FFT flags
  float           _max_mag = 0;
  float           _fundamental_freq = 0;

private:
  // workers
  void fft();
  void ifft();
  void rfft();
  void irfft();
  // helpers
  void fft_primitive(float *x, float *y, int n, int stride, int tw_stride);
  void ifft_primitive(float *x, float *y,int n, int stride, int tw_stride);
  void split_radix_fft(float *x, float *y, int n, int stride, int tw_stride);
  void fft8(float *input,int stride_in, float *output, int stride_out);
  void fft4(float *input,int stride_in, float *output,int stride_out);

};