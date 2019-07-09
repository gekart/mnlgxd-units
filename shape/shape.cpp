/*
    BSD 3-Clause License

    Copyright (c) 2018, KORG INC.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//*/

/*
 * File: waves.cpp
 *
 * Morphing wavetable oscillator
 *
 */

#include "userosc.h"
#include "shape.hpp"
typedef __uint32_t uint32_t;

static inline __attribute__((optimize("Ofast"), always_inline))
float sinwf(float x) {
  const int32_t k = (int32_t)x;
  const float half = (x < 0) ? -0.5f : 0.5f;
  return fastersinf(((half + k) - x) * M_TWOPI);
}

static inline __attribute__((optimize("Ofast"), always_inline))
float sawupf(float x) {
  const int32_t k = (int32_t)x;
  return 2.f *(x - k - 0.5f);
}

static inline __attribute__((optimize("Ofast"), always_inline))
float sawdownf(float x) {
  const int32_t k = (int32_t)x;
  return -2.f *(x - k - 0.5f);
}

static inline __attribute__((optimize("Ofast"), always_inline))
float squareupf(float x) {
  const int32_t k = (int32_t)x;
  return x < 0.5f ? -1.f : 1.f;
}

static inline __attribute__((optimize("Ofast"), always_inline))
float squaredownf(float x) {
  const int32_t k = (int32_t)x;
  return x < 0.5f ? 1.f : -1.f;
}

static Waves s_waves;

// KORG init event to initialize an oscillator during startup or load phase
void OSC_INIT(uint32_t platform, uint32_t api)
{
  // create the struct
  s_waves = Waves();
}

// KORG cycle event, probably called 48k times /s devided by #frames
// batch oriented processing, buffer yn should be filled by #frames
void OSC_CYCLE(const user_osc_param_t * const params,
               int32_t *yn,
               const uint32_t frames)
{
  
  // dereference some vars for easy access and performance
  Waves::State &s = s_waves.state;
  const Waves::Params &p = s_waves.params; // params won't be changed at render time
  // watch out, p is our parameters, while OSC_CYCLE gets an argument params of type user_osc_param_t
  // Handle chages in state
  {
    // temp copy of flags
    const uint32_t flags = s.flags;
    // clear flags for next cycle
    s.flags = Waves::k_flags_none;
    
    // handle any changes in pitch parameters (does this include pitch LFO and EG? probably yes)
    // params->pitch is note in  in upper byte in [0-151] range, 
    // mod is in lower byte in [0-255] range.
    s_waves.updatePitch(osc_w0f_for_note((params->pitch)>>8, params->pitch & 0xFF));
    
    // if any wave parameters have changed, update waves accordingly
    // wave change requests are specifed in the flags
    s_waves.updateWaves(flags);
    
    // reset wave phases to 0
    if (flags & Waves::k_flag_reset)
      s.reset();
    
    // q31 is probably the ratio in 31 bits (only positive ratios)
    // what has this got to do with shape_lfo
    // probably that is the LFO when it targets the SHAPE. But what about PITCH LFO?
    s.lfo = q31_to_f32(params->shape_lfo);
  }
  
  // Temporaries.
  float phi0 = s.phi0;

  float lfoz = s.lfoz;
  const float lfo_inc = (s.lfo - lfoz) / frames;
  
  // buffer start is at yn
  // buffer position is at y
  q31_t * __restrict y = (q31_t *)yn;
  // buffer end is at y_e
  const q31_t * y_e = y + frames;
  
  // loop until you have reached end of buffer y_e
  for (; y != y_e; ) {

    // interpolate a single sample from wave table
    float sig;
    
    if(s.wave0 == NULL) {
      // special treatment for pseudo wave tables
      switch(p.wave0) {
        case 0:
          sig = osc_sinf(phi0);
          break;
        case 1:
          sig = sinwf(phi0);
          break;
        case 2:
          sig = osc_sawf(phi0);
          break;
        case 3:
          sig = sawdownf(phi0);
          break;
        case 4:          
          sig = sawupf(phi0);
          break;
        case 5:          
          sig = osc_sqrf(phi0);
          break;
        case 6:          
          sig = squaredownf(phi0);
          break;
        case 7:          
          sig = squareupf(phi0);
          break;
        case 8: {
          const float z = s.dpwz;     
          s.dpwz = osc_parf(phi0);
          sig = s.dpwz - z;
        }
          break;
        case 9:          
          sig = osc_white();
          break;
      }
    } else {
      sig = osc_wave_scanf(s.wave0, phi0);
    }

    sig *= 2.f;
    // write signal to buffer and increase position y
    *(y++) = f32_to_q31(sig);
    
    // add phase increments to phase states
    // subtract any integer value, essentially: do modulo 1
    phi0 += s.w00;
    phi0 -= (uint32_t)phi0;
    // dont do modulo 1 for the lfo, why?
    lfoz += lfo_inc;
  }
  
  // store wave phases in OSC state
  s.phi0 = phi0;
  s.lfoz = lfoz;
}

// KORG note on event, so reset the wave phases to 0
void OSC_NOTEON(const user_osc_param_t * const params)
{
  // change the flag to reset the waves at next cycle, but keep other state changes (flags)
  s_waves.state.flags |= Waves::k_flag_reset;
}

// KORG note off event
void OSC_NOTEOFF(const user_osc_param_t * const params)
{

}

// KORG parameter change event
void OSC_PARAM(uint16_t index, uint16_t value)
{ 
  // for some reason, you can only write to s and p, not read
  // when accessing these vars you get the following error:
  // undefined reference to `__cxa_guard_acquire' and `__cxa_guard_release'
  Waves::Params  &p = s_waves.params;
  Waves::State &s = s_waves.state;
  
  // which parameter has changed
  switch (index) {
    // some cases change the corresponding Params parameter after normalizing the parameters
    // others additionally set the corresponding flag in the state to indicate a change for OSC_CYCLE

  case k_osc_param_shape:
    // 10bit parameter
    {
      p.shape = param_val_to_f32(value);
      p.wave0 = si_roundf(p.shape * (s.waves_cnt - 1));
      s.flags |= Waves::k_flag_wave0;
    }
    break;
    
  case k_osc_param_shiftshape:
    // 10bit parameter
    {
      p.shiftshape = param_val_to_f32(value);

      // 6 wave tables plus 1 pseudo table
      uint8_t idx = si_roundf(p.shiftshape * 6); 
      s.table = k_waves_table[idx];
      s.waves_cnt = k_waves_cnt[idx];
      p.wave0 = 0; // when changing the table, reset to the first waveform inside table
      s.flags |= Waves::k_flag_wave0;
    }
    break;
    
  default:
    break;
  }
}