#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "bassstation.h"

static float ad(float a,float b){float d=a-b;return d<0?-d:d;}
int main(void){
    bassstation_t s; bassstation_params_t q; uint8_t f[76]; uint32_t n;
    bassstation_defaults(&s);
    s.p.glide_s=0.8f;s.p.osc2_detune=-0.37f;s.p.osc2_semitone=11;s.p.osc_mix=.73f;
    s.p.osc1_env_pitch=-12;s.p.osc1_lfo_pitch=5;s.p.osc1_pw=.72f;s.p.lfo_rate_hz=7.5f;s.p.lfo_delay_s=.4f;
    s.p.cutoff_hz=2300;s.p.resonance=.81f;s.p.filter_lfo_mod=-3.2f;s.p.filter_env_mod=4.1f;
    s.p.amp_attack=.12f;s.p.amp_decay=.44f;s.p.amp_sustain=.67f;s.p.amp_release=.78f;s.p.amp_velocity=.31f;
    s.p.mod_attack=.09f;s.p.mod_decay=.51f;s.p.mod_sustain=.21f;s.p.mod_release=1.1f;s.p.mod_velocity=.52f;
    s.p.osc2_env_pitch=18;s.p.osc2_lfo_pitch=-6;s.p.osc2_pw=.26f;s.p.wheel_pitch=7;s.p.wheel_filter=4;
    s.p.breath_pitch=-4;s.p.aftertouch_pitch=6;s.p.breath_filter=3;s.p.aftertouch_filter=-5;s.p.breath_bias=9;s.p.aftertouch_bias=13;s.p.bend_range=7;s.p.lfo_clock_div=14;
    s.p.osc1_wave=1;s.p.osc2_wave=0;s.p.osc2_range=3;s.p.filter_24db=0;s.p.lfo_shape=2;s.p.osc_sync=1;s.p.trigger_mode=BST_TRIG_AUTOGLIDE;s.p.osc1_pwm_source=2;s.p.osc2_pwm_source=1;
    n=bassstation_rack_sysex_encode(&s.p,f);assert(n==76);assert(f[0]==0xF0&&f[1]==0&&f[2]==0x20&&f[3]==0x29&&f[4]==1&&f[5]==3&&f[6]==0x22&&f[75]==0xF7);
    bassstation_defaults(&s);q=s.p;assert(bassstation_rack_sysex_decode(&q,f+1,74));
    assert(q.osc1_wave==1&&q.osc2_wave==0&&q.osc2_range==3&&q.filter_24db==0&&q.lfo_shape==2&&q.osc_sync==1&&q.trigger_mode==BST_TRIG_AUTOGLIDE&&q.osc1_pwm_source==2&&q.osc2_pwm_source==1);
    /* Historical Rack wire format: two switch bytes are nibble-expanded at 71..74. */
    {unsigned sw1=((unsigned)f[71]<<4)|f[72],sw2=((unsigned)f[73]<<4)|f[74];
     assert((sw1&3u)==0u); /* internal S&H -> Rack random */
     assert(((sw1>>2)&1u)==1u); /* 12 dB */
     assert(((sw1>>4)&1u)==1u); /* OSC2 internal saw -> wire saw */
     assert(((sw1>>5)&3u)==3u);
     assert(((sw1>>7)&1u)==0u); /* OSC1 internal pulse -> wire square */
     assert((sw2&3u)==2u); /* OSC2 ENV */
     assert(((sw2>>2)&3u)==0u); /* OSC1 LFO */
     assert(((sw2>>4)&3u)==3u); /* autoglide */
     assert(((sw2>>6)&1u)==1u); /* sync */}
    assert(ad(q.osc_mix,.73f)<.01f);assert(ad(q.osc2_detune,-.37f)<.015f);assert(ad(q.cutoff_hz,2300)<80);assert(ad(q.resonance,.81f)<.01f);assert(ad(q.filter_lfo_mod,-3.2f)<.1f);assert(ad(q.filter_env_mod,4.1f)<.1f);
    assert(ad(q.amp_sustain,.67f)<.01f&&ad(q.mod_sustain,.21f)<.01f);assert(q.wheel_pitch==7.0f);assert((int)q.breath_pitch==-4&&(int)q.aftertouch_pitch==6&&(int)q.breath_filter==3&&(int)q.aftertouch_filter==-5);assert(q.lfo_clock_div==14);
    /* TEST11: index 14 is the previously omitted 64-clock (1-bar triplet) Rack division. */
    assert((((unsigned)f[69]<<4)|(unsigned)f[70])==64u);
    /* TEST15: Rack full-byte bipolar fields are true signed -128..+127. */
    s.p.osc1_env_pitch=-24.0f;bassstation_rack_sysex_encode(&s.p,f);
    assert((((unsigned)f[15]<<4)|(unsigned)f[16])==0u);
    q=s.p;assert(bassstation_rack_sysex_decode(&q,f+1,74));assert(q.osc1_env_pitch==-24.0f);
    f[15]=0u;f[16]=1u;q=s.p;assert(bassstation_rack_sysex_decode(&q,f+1,74));
    assert(q.osc1_env_pitch>-24.0f&&q.osc1_env_pitch<-23.7f);
    s.p.osc1_env_pitch=24.0f;bassstation_rack_sysex_encode(&s.p,f);
    assert((((unsigned)f[15]<<4)|(unsigned)f[16])==255u);
    /* Rack bridge stays hardware-compatible: 0..+12 semitone and +/-50c fine tuning. */
    s.p.osc2_semitone=12;s.p.osc2_detune=.50f;bassstation_rack_sysex_encode(&s.p,f);q=s.p;assert(bassstation_rack_sysex_decode(&q,f+1,74));assert(q.osc2_semitone==12.0f);assert(q.osc2_detune<=.501f);
    f[1]=1;assert(!bassstation_rack_sysex_decode(&q,f+1,74));
    puts("bassstation Rack SysEx TEST15: PASS");return 0;
}
