#pragma once
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
 *  File: waves.hpp
 *
 *  Morphing Wavetable Synthesizer
 *
 */

#include "userosc.h"
#include "biquad.hpp"

  const uint8_t k_waves_cnt[7] = {
    k_waves_a_cnt,
    k_waves_b_cnt,
    k_waves_c_cnt,
    k_waves_d_cnt,
    k_waves_e_cnt,
    k_waves_f_cnt,
    10
  };

  const float* const*k_waves_table[7] = {
    wavesA,
    wavesB,
    wavesC,
    wavesD,
    wavesE,
    wavesF,
    NULL
  };

struct Waves {

  enum {
    k_flags_none    = 0,
    k_flag_wave0    = 1<<1,
    k_flag_reset    = 1<<2
  };
  
  struct Params {
    float    shape;
    float    shiftshape;
    uint8_t  wave0;
    uint8_t  padding;
    
    Params(void) :
      shape(0.f),
      shiftshape(0.f),
      wave0(0)
    { }
  };
  
  struct State {
    const float   *wave0;
    const float * const *table;
          uint8_t waves_cnt:8;
          float    phi0;
          float    w00;
          float    lfo;
          float    lfoz;
          float    dpwz;
          float    imperfection;
          uint32_t flags:8;
    
    State(void) :
      wave0(wavesA[0]), // start with first wave in block A
      table(wavesA),
      waves_cnt(k_waves_cnt[0]),
      w00(440.f * k_samplerate_recipf),
      lfo(0.f),
      lfoz(0.f),
      dpwz(0.f),
      flags(k_flags_none)
    {
      reset();
      imperfection = osc_white() * 1.0417e-006f; // +/- 0.05Hz@48KHz
    }
    
    // reset the phases of all waves
    inline void reset(void)
    {
      phi0 = 0;
      lfo = lfoz; // the LFO only resets if specified in lfoz, probably handling free vs sync LFO
      dpwz = 0.f;
    }
  };

  Waves(void) {
    init();
  }

  void init(void) {
    state = State();
    params = Params();
  }
  
  // w0 is the phase increment
  inline void updatePitch(float w0) {
    // since this struct is probably initiated 4 times (1 for every voice)
    // imperfection will get 4 different values so that all voices are slightly detuned
    w0 += state.imperfection;
    state.w00 = w0; // phase increment for wave 1
  }
  
  inline void updateWaves(const uint16_t flags) {
    // check flags for any wave change requests from corresponding parameter changes in OSC_PARAM
    if (flags & k_flag_wave0) {
      if (state.table != NULL) { 
        uint8_t idx = params.wave0 % state.waves_cnt;      
        state.wave0 = state.table[idx];
      } else {
        state.wave0 = NULL;
      }
    }
  }

  State       state;
  Params      params;
};