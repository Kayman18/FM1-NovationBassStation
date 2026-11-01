/* SPDX-License-Identifier: MPL-2.0 */
#include "bassstation.h"
#include "fastmath.h"

static float clampf_factory(float x,float lo,float hi){return x<lo?lo:(x>hi?hi:x);}
static float exp8(uint8_t v,float lo,float hi){return lo*fm_exp2f(((float)v/255.0f)*fm_log2f(hi/lo));}
static float signed8(uint8_t v,float depth){int q=(int)v-128;float n=q<0?(float)q/128.0f:(float)q/127.0f;return clampf_factory(n*depth,-depth,depth);}
/* Exact compact-state transforms recovered from Novation Bass Station 2.2.
   MIX clamps native 0..84 to full OSC1 then expands 85..255 over 0..255.
   PW reserves native zero as the full-positive endpoint and maps 1..255 to
   public normalized codes 10..255.  Keep this separate from Rack SysEx. */
static uint8_t plugin_mix8(uint8_t v){return v<85u?0u:(uint8_t)(((unsigned)(v-85u)*255u)/170u);}
static uint8_t plugin_pw8(uint8_t v){return v==0u?255u:(uint8_t)(((unsigned)v*245u)/255u+10u);}
static uint8_t pwm_source(uint8_t v){return v==0u?2u:(v==2u?1u:0u);} /* wire: LFO,MAN,ENV2 */
static uint8_t lfo_shape(uint8_t v){return v==0u?2u:(v==2u?1u:0u);}  /* wire: RANDOM,TRI,SAW */
/* Native software switch bits: 0=MULTI, 1=SINGLE, 2/3=AUTO.
   Do not reuse the Rack SysEx trigger encoding. */
static uint8_t trigger(uint8_t v){return v==0u?BST_TRIG_MULTI:(v==1u?BST_TRIG_SINGLE:BST_TRIG_AUTOGLIDE);}
/* Exact native v2.2 sync-rate recovery table.  The desktop loader has no
   compact-state code for public enum 16; raw 64 is invalid/off. */
static uint8_t clock_index_factory(uint8_t raw){static const uint8_t t[16]={0,2,3,4,6,8,9,12,16,18,24,32,36,48,72,96};uint8_t i;for(i=0;i<16u;i++)if(t[i]==raw)return i;return 0;}

int bassstation_plugin_factory_decode(bassstation_params_t *p,const uint8_t r[34]){
    uint8_t sw1,sw2;
    if(!p||!r)return 0;
    /* Keep software-only/global defaults (key tracking policy, etc.) supplied by caller. */
    p->glide_s=r[0]?exp8(r[0],0.005f,5.0f):0.0f;
    /* The software plug-in widens OSC2 fine tune to +/-96 cents. */
    p->osc2_detune=signed8(r[1],0.96f);
    /* Plug-in OSC2 SEMI is the public 0..+12 integer control. */
    p->osc2_semitone=(float)(((unsigned)r[2]*12u+127u)/255u);
    p->osc_mix=(float)plugin_mix8(r[3])/255.0f;
    p->osc1_env_pitch=signed8(r[4],24.0f);
    p->osc1_lfo_pitch=signed8(r[5],12.0f);
    p->osc1_pw=(float)plugin_pw8(r[6])/255.0f;
    p->lfo_rate_hz=r[7]?exp8(r[7],0.0055f,1000.0f):0.0f;
    p->lfo_delay_s=r[8]?exp8(r[8],0.005f,5.0f):0.0f;
    p->cutoff_hz=exp8(r[9],10.0f,10000.0f);
    p->resonance=(float)r[10]/255.0f;
    p->filter_lfo_mod=signed8(r[11],8.0f);
    p->amp_attack=r[12]?exp8(r[12],0.0005f,20.0f):0.0f;
    p->amp_decay=r[13]?exp8(r[13],0.003f,20.0f):0.0f;
    p->amp_sustain=(float)r[14]/255.0f;
    p->amp_release=r[15]?exp8(r[15],0.003f,20.0f):0.0f;
    p->amp_velocity=(float)r[16]/255.0f;
    p->mod_attack=r[17]?exp8(r[17],0.0005f,20.0f):0.0f;
    p->mod_decay=r[18]?exp8(r[18],0.003f,20.0f):0.0f;
    p->mod_sustain=(float)r[19]/255.0f;
    p->mod_release=r[20]?exp8(r[20],0.003f,20.0f):0.0f;
    p->mod_velocity=(float)r[21]/255.0f;
    p->osc2_env_pitch=signed8(r[22],24.0f);
    p->osc2_lfo_pitch=signed8(r[23],12.0f);
    p->osc2_pw=(float)plugin_pw8(r[24])/255.0f;
    p->filter_env_mod=signed8(r[25],8.0f);

    /* Native plug-in controller fields use different packed widths.
       r26 and r29 are pairs of unsigned 4-bit values (0..15).
       r27 and r28 are pairs of unsigned 3-bit values (0..7); bit 3 of each
       nibble is ignored by the original v2.2 loader.  Rack SysEx is a
       separate format and retains its signed -7..+7 controller depths. */
    p->wheel_pitch=(float)(r[26]&15u);
    p->wheel_filter=(float)((r[26]>>4)&15u)*(6.0f/15.0f);
    p->breath_pitch=(float)(r[27]&7u);
    p->aftertouch_pitch=(float)((r[27]>>4)&7u);
    p->breath_filter=(float)(r[28]&7u);
    p->aftertouch_filter=(float)((r[28]>>4)&7u);
    p->breath_bias=(float)(r[29]&15u);
    p->aftertouch_bias=(float)((r[29]>>4)&15u);
    {uint8_t bend=(uint8_t)(r[30]&15u);p->bend_range=(float)(bend>12u?12u:bend);}
    p->lfo_clock_div=clock_index_factory(r[31]);

    sw1=r[32]; sw2=r[33];
    /* Wire format: bit 7/4 are OSC1/OSC2 with 0=SQUARE,1=SAW. */
    p->osc1_wave=(uint8_t)(((sw1>>7)&1u)?0u:1u);
    p->osc2_wave=(uint8_t)(((sw1>>4)&1u)?0u:1u);
    p->osc2_range=(uint8_t)((sw1>>5)&3u);
    p->filter_24db=(uint8_t)(((sw1>>2)&1u)?0u:1u);
    p->lfo_shape=lfo_shape(sw1&3u);
    p->osc_sync=(uint8_t)((sw2>>6)&1u);
    p->trigger_mode=trigger((sw2>>4)&3u);
    p->osc1_pwm_source=pwm_source((sw2>>2)&3u);
    p->osc2_pwm_source=pwm_source(sw2&3u);

    /* These are plug-in globals, not fields in the 34-byte program record. */
    p->master_tune=0.0f;
    p->volume=(63.0f/127.0f)*1.25f;
    return 1;
}
