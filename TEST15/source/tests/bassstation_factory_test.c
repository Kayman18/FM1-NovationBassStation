#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bassstation.h"
#include "../app/factory_bank.h"

static int finite_params(const bassstation_params_t *p){
    const float *f=(const float*)&p->osc1_pw;
    const float *end=(const float*)(((const uint8_t*)p)+sizeof(*p));
    while(f<end){ if(!isfinite(*f)) return 0; f++; }
    return 1;
}
int main(void){
    unsigned i; bassstation_t s; int32_t lr[256*2];
    assert(BST_FACTORY_COUNT==100u&&BST_FACTORY_RECORD_BYTES==34u);
    assert(!strcmp(bst_factory_name[0],"Moog Bass"));
    assert(!strcmp(bst_factory_name[99],"Use it up Bass"));
    /* Recovered state bank anchors: program 00 and its historical switch bytes. */
    assert(bst_factory_record[0][0]==0x03u);
    assert(bst_factory_record[0][26]==0x05u); /* mod-wheel pitch 5 */
    assert(bst_factory_record[0][30]==0x02u); /* pitch-bend range 2 */
    assert(bst_factory_record[0][32]==0xA9u);
    assert(bst_factory_record[0][33]==0x01u);
    /* Independent Novation Automap screenshot values for program 00. */
    assert((bst_factory_record[0][7]>>1)==79u);  /* LFO speed 79 */
    assert((bst_factory_record[0][8]>>1)==91u);  /* LFO delay 91 */
    assert((bst_factory_record[0][10]>>1)==40u); /* resonance 40 */
    assert((bst_factory_record[0][12]>>1)==5u);  /* amp attack 5 */
    assert((bst_factory_record[0][13]>>1)==32u); /* amp decay 32 */
    assert((bst_factory_record[0][14]>>1)==126u);/* amp sustain 126 */
    assert((bst_factory_record[0][15]>>1)==61u); /* amp release 61 */
    assert((bst_factory_record[0][17]>>1)==25u); /* mod attack 25 */
    assert((bst_factory_record[0][18]>>1)==94u); /* mod decay 94 */
    assert((bst_factory_record[0][19]>>1)==15u); /* mod sustain 15 */
    assert((bst_factory_record[0][20]>>1)==49u); /* mod release 49 */
    /* A second independent Novation Automap screenshot shows factory program
       13/100 "TB303 Eow Bass".  These raw anchors confirm that the plug-in's
       MIX/PW display is not a simple signed linear view of the stored bytes. */
    assert(!strcmp(bst_factory_name[12],"TB303 Eow Bass"));
    assert(bst_factory_record[12][3]==0x4Du);   /* native MIX; Automap displays -64 */
    assert(bst_factory_record[12][6]==0x03u);   /* native OSC1 PW; Automap displays -58 */
    assert((bst_factory_record[12][7]>>1)==78u);/* LFO speed 78 */
    assert((bst_factory_record[12][8]>>1)==64u);/* LFO delay 64 */
    assert((bst_factory_record[12][9]>>1)==44u);/* filter frequency 44 */
    assert((bst_factory_record[12][10]>>1)==127u);/* resonance 127 */
    /* TEST15: exact native-state transfer recovered from the original v2.2 DLL.
       MIX: native <85 -> public 0; 85..255 -> floor((raw-85)*255/170).
       PW: native 0 -> 255, otherwise floor(raw*245/255)+10. */
    bassstation_defaults(&s); assert(bassstation_plugin_factory_decode(&s.p,bst_factory_record[0]));
    assert(fabsf(s.p.osc_mix-(135.0f/255.0f))<1e-6f); /* 0xAF -> panel +3 */
    assert(fabsf(s.p.osc1_pw-(12.0f/255.0f))<1e-6f);  /* 0x03 -> panel -58 */
    assert(fabsf(s.p.osc2_pw-(14.0f/255.0f))<1e-6f);  /* 0x05 -> panel -57 */
    bassstation_defaults(&s); assert(bassstation_plugin_factory_decode(&s.p,bst_factory_record[12]));
    assert(s.p.osc_mix==0.0f);                         /* 0x4D -> panel -64 */
    assert(fabsf(s.p.osc1_pw-(12.0f/255.0f))<1e-6f);
    /* TEST15: OSC2 SEMI is the plug-in's 0..+12 integer control. */
    bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,bst_factory_record[20]));assert(s.p.osc2_semitone==12.0f); /* raw FC */
    bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,bst_factory_record[10]));assert(s.p.trigger_mode==BST_TRIG_AUTOGLIDE); /* TB303 Autoglide Bas */
    bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,bst_factory_record[16]));assert(s.p.trigger_mode==BST_TRIG_SINGLE);    /* Trance 1 */
    bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,bst_factory_record[44]));assert(s.p.osc2_semitone==4.0f);  /* raw 4F */
    for(i=0;i<BST_FACTORY_COUNT;i++){
        bassstation_defaults(&s);
        assert(bassstation_plugin_factory_decode(&s.p,bst_factory_record[i]));
        assert(finite_params(&s.p));
        assert(s.p.osc1_wave<=1u&&s.p.osc2_wave<=1u&&s.p.osc2_range<=3u);
        assert(s.p.osc1_pwm_source<=2u&&s.p.osc2_pwm_source<=2u);
        assert(s.p.lfo_shape<=2u&&s.p.trigger_mode<=BST_TRIG_MULTI&&s.p.lfo_clock_div<=16u);
        assert(s.p.osc2_semitone>=0.0f&&s.p.osc2_semitone<=12.0f);
        assert(s.p.osc2_detune>=-0.961f&&s.p.osc2_detune<=0.961f);
        assert(s.p.osc1_pw>=0.0f&&s.p.osc1_pw<=1.0f&&s.p.osc2_pw>=0.0f&&s.p.osc2_pw<=1.0f);
        assert(s.p.resonance>=0.0f&&s.p.resonance<=1.0f);
        assert(s.p.amp_sustain>=0.0f&&s.p.amp_sustain<=1.0f&&s.p.mod_sustain>=0.0f&&s.p.mod_sustain<=1.0f);
        bassstation_note_on(&s,48+(int)(i%24u),100);
        bassstation_render(&s,lr,256,4096);
        bassstation_note_off(&s,48+(int)(i%24u));
    }
    /* Native MIX/PW edge cases from the desktop decoder. */
    {uint8_t r[BST_FACTORY_RECORD_BYTES];memcpy(r,bst_factory_record[0],sizeof r);
     r[3]=84u;r[6]=0u;bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,r));
     assert(s.p.osc_mix==0.0f&&s.p.osc1_pw==1.0f);
     r[3]=85u;r[6]=1u;bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,r));
     assert(s.p.osc_mix==0.0f&&fabsf(s.p.osc1_pw-(10.0f/255.0f))<1e-6f);
     r[3]=86u;r[6]=255u;bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,r));
     assert(fabsf(s.p.osc_mix-(1.0f/255.0f))<1e-6f&&s.p.osc1_pw==1.0f);
     r[3]=255u;bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,r));assert(s.p.osc_mix==1.0f);}

    {uint8_t r[BST_FACTORY_RECORD_BYTES];memcpy(r,bst_factory_record[0],sizeof r);
     r[2]=0u;bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,r));assert(s.p.osc2_semitone==0.0f);
     r[2]=127u;bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,r));assert(s.p.osc2_semitone==6.0f);
     r[2]=255u;bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,r));assert(s.p.osc2_semitone==12.0f);}

    /* TEST15: exhaustive valid native switch-domain vectors recovered from
       the v2.2 loader.  These intentionally test software records, not Rack. */
    {uint8_t r[BST_FACTORY_RECORD_BYTES];memcpy(r,bst_factory_record[0],sizeof r);
     r[32]=0x00u;r[33]=0x00u;bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,r));
     assert(s.p.osc1_wave==1u&&s.p.osc2_wave==1u&&s.p.osc2_range==0u);
     assert(s.p.filter_24db==1u&&s.p.lfo_shape==2u);
     assert(s.p.osc1_pwm_source==2u&&s.p.osc2_pwm_source==2u&&s.p.osc_sync==0u);
     r[32]=(uint8_t)(0x80u|0x10u|0x60u|0x04u|0x01u); /* saw/saw, 2', 12dB, TRI */
     r[33]=(uint8_t)(0x40u|0x08u|0x02u);             /* sync, O1 ENV2, O2 ENV2 */
     bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,r));
     assert(s.p.osc1_wave==0u&&s.p.osc2_wave==0u&&s.p.osc2_range==3u);
     assert(s.p.filter_24db==0u&&s.p.lfo_shape==0u&&s.p.osc_sync==1u);
     assert(s.p.osc1_pwm_source==1u&&s.p.osc2_pwm_source==1u);
     r[32]=(uint8_t)((r[32]&~0x03u)|0x02u);r[33]=(uint8_t)((r[33]&~0x0fu)|0x05u); /* SAW LFO, both MAN */
     bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,r));
     assert(s.p.lfo_shape==1u&&s.p.osc1_pwm_source==0u&&s.p.osc2_pwm_source==0u);}

    /* TEST15: native switch-byte trigger encoding is not Rack encoding.
       0=MULTI, 1=SINGLE, 2/3=AUTO in the software record. */
    {uint8_t r[BST_FACTORY_RECORD_BYTES];memcpy(r,bst_factory_record[0],sizeof r);
     r[33]=(uint8_t)(r[33]&~0x30u);bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,r));assert(s.p.trigger_mode==BST_TRIG_MULTI);
     r[33]=(uint8_t)((r[33]&~0x30u)|0x10u);bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,r));assert(s.p.trigger_mode==BST_TRIG_SINGLE);
     r[33]=(uint8_t)((r[33]&~0x30u)|0x20u);bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,r));assert(s.p.trigger_mode==BST_TRIG_AUTOGLIDE);
     r[33]=(uint8_t)((r[33]&~0x30u)|0x30u);bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,r));assert(s.p.trigger_mode==BST_TRIG_AUTOGLIDE);}

    /* TEST15: exact native LFO-sync compact codes from the v2.2 DLL.
       Raw 64 is deliberately invalid/off; 72 and 96 map to 14 and 15. */
    {uint8_t r[BST_FACTORY_RECORD_BYTES];memcpy(r,bst_factory_record[0],sizeof r);
     r[31]=48u;bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,r));assert(s.p.lfo_clock_div==13u);
     r[31]=64u;bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,r));assert(s.p.lfo_clock_div==0u);
     r[31]=72u;bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,r));assert(s.p.lfo_clock_div==14u);
     r[31]=96u;bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,r));assert(s.p.lfo_clock_div==15u);}

    /* TEST15: native software controller depths are packed unsigned 3-bit
       fields.  The original v2.2 loader masks each nibble with 7, so bit 3
       is ignored rather than acting as a sign bit.  Keep this distinct from
       Rack SysEx, whose controller depths remain signed -7..+7. */
    {uint8_t r[BST_FACTORY_RECORD_BYTES];memcpy(r,bst_factory_record[0],sizeof r);
     r[27]=0xF9u; r[28]=0x8Fu; r[29]=0xA5u; r[30]=0xF2u;
     bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,r));
     assert(s.p.breath_pitch==1.0f&&s.p.aftertouch_pitch==7.0f);
     assert(s.p.breath_filter==7.0f&&s.p.aftertouch_filter==0.0f);
     assert(s.p.breath_bias==5.0f&&s.p.aftertouch_bias==10.0f);
     assert(s.p.bend_range==2.0f);}

    /* TEST15: native plug-in bipolar bytes are signed -128..+127 around raw 128.
       Preserve both extreme negative codes instead of collapsing raw 0 and 1. */
    {uint8_t r[BST_FACTORY_RECORD_BYTES];float a,b,c;memcpy(r,bst_factory_record[0],sizeof r);
     r[4]=0u;bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,r));a=s.p.osc1_env_pitch;
     r[4]=1u;bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,r));b=s.p.osc1_env_pitch;
     r[4]=255u;bassstation_defaults(&s);assert(bassstation_plugin_factory_decode(&s.p,r));c=s.p.osc1_env_pitch;
     assert(a==-24.0f&&b>a&&b<-23.7f&&c==24.0f);}
    bassstation_defaults(&s); assert(bassstation_plugin_factory_decode(&s.p,bst_factory_record[0]));
    assert(s.p.trigger_mode==BST_TRIG_MULTI);
    assert(s.p.osc1_wave==0u&&s.p.osc2_wave==1u&&s.p.osc2_range==1u);
    assert(s.p.filter_24db==1u&&s.p.lfo_shape==0u&&s.p.osc_sync==0u);
    assert(s.p.osc1_pwm_source==2u&&s.p.osc2_pwm_source==0u);
    assert(s.p.wheel_pitch==5.0f&&s.p.bend_range==2.0f);
    assert(s.p.resonance>0.31f&&s.p.resonance<0.32f);
    assert(s.p.cutoff_hz>10.0f&&s.p.cutoff_hz<12.0f);
    puts("bassstation TEST15 100-program factory bank: PASS");
    return 0;
}
