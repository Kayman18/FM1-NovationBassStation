/* SPDX-License-Identifier: MPL-2.0 */
#include "bassstation.h"
#include "fastmath.h"

static float clampf(float x,float lo,float hi){return x<lo?lo:(x>hi?hi:x);}
static int clampi(int x,int lo,int hi){return x<lo?lo:(x>hi?hi:x);}
static unsigned pair_get(const uint8_t *p,unsigned full_off){return ((unsigned)p[full_off-1u]<<4)|(unsigned)p[full_off];}
static void pair_put(uint8_t *p,unsigned full_off,unsigned v){if(v>255u)v=255u;p[full_off]=(uint8_t)((v>>4)&15u);p[full_off+1u]=(uint8_t)(v&15u);}
static float bipolar8(unsigned v,float depth){int q=(int)v-128;return (q<0?(float)q/128.0f:(float)q/127.0f)*depth;}
static unsigned unipolar8(float x,float lo,float hi){float q;if(x<=lo)return 0;if(x>=hi)return 255;q=(x-lo)/(hi-lo);return (unsigned)(q*255.0f+0.5f);}
static unsigned bipolar8_put(float x,float depth){float q=x/depth;int d;if(q<=-1.0f)return 0u;if(q>=1.0f)return 255u;d=(int)(q*(q<0.0f?128.0f:127.0f)+(q>=0.0f?0.5f:-0.5f));return (unsigned)clampi(128+d,0,255);}
static float expmap(unsigned v,float lo,float hi){return lo*fm_exp2f(((float)v/255.0f)*fm_log2f(hi/lo));}
static unsigned expunmap(float x,float lo,float hi){float q;if(x<=lo)return 0;if(x>=hi)return 255;q=fm_log2f(x/lo)/fm_log2f(hi/lo);return (unsigned)clampi((int)(q*255.0f+0.5f),0,255);}
static int signed_nib(uint8_t v){int n=(int)(v&15u);return n>=8?n-16:n;}
static uint8_t signed_nib_put(float x){int n=(int)(x+(x>=0?0.5f:-0.5f));n=clampi(n,-7,7);return (uint8_t)(n&15);}
static int clock_index(unsigned raw){static const uint8_t t[17]={0,2,3,4,6,8,9,12,16,18,24,32,36,48,64,72,96};int i;for(i=0;i<17;i++)if(t[i]==raw)return i;return 0;}
static unsigned clock_raw(unsigned idx){static const uint8_t t[17]={0,2,3,4,6,8,9,12,16,18,24,32,36,48,64,72,96};return t[idx<17u?idx:0u];}
static uint8_t rack_pwm_to_internal(unsigned v){return v==0?2u:(v==2?1u:0u);} /* Rack: LFO,MAN,ENV2; internal: MAN,ENV2,LFO */
static unsigned internal_pwm_to_rack(unsigned v){return v==2?0u:(v==1?2u:1u);}
static uint8_t rack_lfo_to_internal(unsigned v){return v==0?2u:(v==2?1u:0u);} /* Rack: random,tri,saw; internal: tri,saw,S&H */
static unsigned internal_lfo_to_rack(unsigned v){return v==2?0u:(v==1?2u:1u);}
static uint8_t rack_trigger_to_internal(unsigned v){if(v==3)return BST_TRIG_AUTOGLIDE;if(v==2)return BST_TRIG_SINGLE;return BST_TRIG_MULTI;}
static unsigned internal_trigger_to_rack(unsigned v){return v==BST_TRIG_AUTOGLIDE?3u:(v==BST_TRIG_SINGLE?2u:1u);}

int bassstation_rack_sysex_decode(bassstation_params_t *p,const uint8_t *d,uint32_t n){
    unsigned v,packed;
    if(!p||!d||n!=74u)return 0;
    if(d[0]!=0x00u||d[1]!=0x20u||d[2]!=0x29u||d[3]!=0x01u||d[4]!=0x03u||d[5]!=0x22u)return 0;
    v=pair_get(d,7);p->glide_s=v?expmap(v,0.005f,5.0f):0.0f;
    v=pair_get(d,9);p->osc2_detune=clampf(((float)((int)v-128)/127.0f)*0.50f,-0.50f,0.50f);
    v=pair_get(d,11);p->osc2_semitone=(float)(((v*12u)+127u)/255u);
    p->osc_mix=(float)pair_get(d,13)/255.0f;
    p->osc1_env_pitch=bipolar8(pair_get(d,15),24.0f);
    p->osc1_lfo_pitch=bipolar8(pair_get(d,17),12.0f);
    p->osc1_pw=(float)pair_get(d,19)/255.0f;
    p->lfo_rate_hz=expmap(pair_get(d,21),0.05f,50.0f);
    v=pair_get(d,23);p->lfo_delay_s=v?expmap(v,0.005f,5.0f):0.0f;
    p->cutoff_hz=expmap(pair_get(d,25),5.0f,10000.0f);
    p->resonance=(float)pair_get(d,27)/255.0f;
    p->filter_lfo_mod=bipolar8(pair_get(d,29),8.0f);
    p->amp_attack=expmap(pair_get(d,31),0.0005f,5.0f);
    p->amp_decay=expmap(pair_get(d,33),0.003f,10.0f);
    p->amp_sustain=(float)pair_get(d,35)/255.0f;
    p->amp_release=expmap(pair_get(d,37),0.003f,10.0f);
    p->amp_velocity=(float)pair_get(d,39)/255.0f;
    p->mod_attack=expmap(pair_get(d,41),0.0005f,5.0f);
    p->mod_decay=expmap(pair_get(d,43),0.003f,10.0f);
    p->mod_sustain=(float)pair_get(d,45)/255.0f;
    p->mod_release=expmap(pair_get(d,47),0.003f,10.0f);
    p->mod_velocity=(float)pair_get(d,49)/255.0f;
    p->osc2_env_pitch=bipolar8(pair_get(d,51),24.0f);
    p->osc2_lfo_pitch=bipolar8(pair_get(d,53),12.0f);
    p->osc2_pw=(float)pair_get(d,55)/255.0f;
    p->filter_env_mod=bipolar8(pair_get(d,57),8.0f);
    p->wheel_pitch=(float)(d[58]&15u);
    p->wheel_filter=((float)(d[59]&15u)/15.0f)*6.0f;
    p->breath_pitch=(float)signed_nib(d[60]);
    p->aftertouch_pitch=(float)signed_nib(d[61]);
    p->breath_filter=(float)signed_nib(d[62]);
    p->aftertouch_filter=(float)signed_nib(d[63]);
    p->breath_bias=(float)(d[64]&15u);
    p->aftertouch_bias=(float)(d[65]&15u);
    p->bend_range=(float)clampi((int)pair_get(d,67),0,12);
    p->lfo_clock_div=(uint8_t)clock_index(pair_get(d,69));
    /* Two packed switch bytes are themselves nibble-expanded on the wire. */
    packed=pair_get(d,71);
    p->lfo_shape=rack_lfo_to_internal(packed&3u);
    p->filter_24db=(uint8_t)(((packed>>2)&1u)?0u:1u);
    /* bit 3 is the Rack filter-mod selector; both stored depths are retained on FM1. */
    p->osc2_wave=(uint8_t)(((packed>>4)&1u)?0u:1u); /* wire 0=square,1=saw */
    p->osc2_range=(uint8_t)((packed>>5)&3u);
    p->osc1_wave=(uint8_t)(((packed>>7)&1u)?0u:1u);
    packed=pair_get(d,73);
    p->osc2_pwm_source=rack_pwm_to_internal(packed&3u);
    p->osc1_pwm_source=rack_pwm_to_internal((packed>>2)&3u);
    p->trigger_mode=rack_trigger_to_internal((packed>>4)&3u);
    p->osc_sync=(uint8_t)((packed>>6)&1u);
    return 1;
}

uint32_t bassstation_rack_sysex_encode(const bassstation_params_t *p,uint8_t out[76]){
    unsigned packed;if(!p||!out)return 0;
    {unsigned i;for(i=0;i<76u;i++)out[i]=0;}
    out[0]=0xF0;out[1]=0x00;out[2]=0x20;out[3]=0x29;out[4]=0x01;out[5]=0x03;out[6]=0x22;
    pair_put(out,7,p->glide_s<=0.0001f?0u:expunmap(p->glide_s,0.005f,5.0f));
    pair_put(out,9,bipolar8_put(clampf(p->osc2_detune,-0.50f,0.50f),0.50f));
    pair_put(out,11,(unsigned)((clampf(p->osc2_semitone,0.0f,12.0f)*(255.0f/12.0f))+0.5f));
    pair_put(out,13,unipolar8(p->osc_mix,0,1));
    pair_put(out,15,bipolar8_put(p->osc1_env_pitch,24.0f));
    pair_put(out,17,bipolar8_put(p->osc1_lfo_pitch,12.0f));
    pair_put(out,19,unipolar8(p->osc1_pw,0,1));
    pair_put(out,21,expunmap(p->lfo_rate_hz,0.05f,50.0f));
    pair_put(out,23,p->lfo_delay_s<=0.0001f?0u:expunmap(p->lfo_delay_s,0.005f,5.0f));
    pair_put(out,25,expunmap(p->cutoff_hz,5.0f,10000.0f));
    pair_put(out,27,unipolar8(p->resonance,0,1));
    pair_put(out,29,bipolar8_put(p->filter_lfo_mod,8.0f));
    pair_put(out,31,expunmap(p->amp_attack,0.0005f,5.0f));pair_put(out,33,expunmap(p->amp_decay,0.003f,10.0f));pair_put(out,35,unipolar8(p->amp_sustain,0,1));pair_put(out,37,expunmap(p->amp_release,0.003f,10.0f));pair_put(out,39,unipolar8(p->amp_velocity,0,1));
    pair_put(out,41,expunmap(p->mod_attack,0.0005f,5.0f));pair_put(out,43,expunmap(p->mod_decay,0.003f,10.0f));pair_put(out,45,unipolar8(p->mod_sustain,0,1));pair_put(out,47,expunmap(p->mod_release,0.003f,10.0f));pair_put(out,49,unipolar8(p->mod_velocity,0,1));
    pair_put(out,51,bipolar8_put(p->osc2_env_pitch,24.0f));pair_put(out,53,bipolar8_put(p->osc2_lfo_pitch,12.0f));pair_put(out,55,unipolar8(p->osc2_pw,0,1));pair_put(out,57,bipolar8_put(p->filter_env_mod,8.0f));
    out[59]=(uint8_t)clampi((int)(clampf(p->wheel_pitch,0,15)+0.5f),0,15);
    out[60]=(uint8_t)clampi((int)(clampf(p->wheel_filter,0,6)*(15.0f/6.0f)+0.5f),0,15);
    out[61]=signed_nib_put(p->breath_pitch);out[62]=signed_nib_put(p->aftertouch_pitch);out[63]=signed_nib_put(p->breath_filter);out[64]=signed_nib_put(p->aftertouch_filter);
    out[65]=(uint8_t)clampi((int)(p->breath_bias+0.5f),0,15);out[66]=(uint8_t)clampi((int)(p->aftertouch_bias+0.5f),0,15);
    pair_put(out,67,(unsigned)clampi((int)(p->bend_range+0.5f),0,12));pair_put(out,69,clock_raw(p->lfo_clock_div));
    packed=internal_lfo_to_rack(p->lfo_shape)
          |((unsigned)(p->filter_24db?0u:1u)<<2)
          |((unsigned)(p->osc2_wave?0u:1u)<<4)
          |((unsigned)(p->osc2_range&3u)<<5)
          |((unsigned)(p->osc1_wave?0u:1u)<<7);
    pair_put(out,71,packed);
    packed=internal_pwm_to_rack(p->osc2_pwm_source)
          |(internal_pwm_to_rack(p->osc1_pwm_source)<<2)
          |(internal_trigger_to_rack(p->trigger_mode)<<4)
          |((unsigned)(p->osc_sync?1u:0u)<<6);
    pair_put(out,73,packed);
    out[75]=0xF7;return 76u;
}
