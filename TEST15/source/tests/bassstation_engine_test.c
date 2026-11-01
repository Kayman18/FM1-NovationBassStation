#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bassstation.h"

static int64_t peak_block(const int32_t *lr,int n){int i;int64_t p=0;for(i=0;i<n*2;i++){int64_t v=lr[i];if(v<0)v=-v;if(v>p)p=v;}return p;}
static void finite_block(const int32_t *lr,int n){assert(peak_block(lr,n)>0);}
static int64_t velocity_peak(int vel){bassstation_t s;int32_t lr[512];int i;int64_t p=0;bassstation_defaults(&s);s.p.amp_velocity=1.0f;s.p.amp_attack=0.0005f;s.p.amp_decay=0.01f;s.p.amp_sustain=1.0f;s.p.filter_env_mod=0.0f;s.p.filter_lfo_mod=0.0f;bassstation_note_on(&s,48,vel);for(i=0;i<8;i++){bassstation_render(&s,lr,256,4096);if(peak_block(lr,256)>p)p=peak_block(lr,256);}return p;}
static void render_frames(bassstation_t *s,int32_t *lr,uint32_t frames){while(frames){uint32_t n=frames>2048u?2048u:frames;bassstation_render(s,lr,n,4096);frames-=n;}}

int main(void){
    bassstation_t s,a,b;int32_t aaout[512],bbout[512];int32_t *lr=(int32_t*)calloc(4096,sizeof(int32_t));int i,mode,w1,w2,src1,src2;float before,ev;uint8_t est;
    assert(lr);bassstation_defaults(&s);assert(s.p.filter_24db==1);assert(s.p.osc2_range==1);assert(s.p.trigger_mode==BST_TRIG_MULTI);assert(s.p.osc1_pwm_source==0&&s.p.osc2_pwm_source==0);assert(fabsf(s.p.osc_mix-0.45f)<0.01f);

    /* Last-note priority and held-note return. */
    bassstation_note_on(&s,48,100);bassstation_render(&s,lr,256,4096);finite_block(lr,256);
    bassstation_note_on(&s,55,110);assert(s.note==55);bassstation_note_off(&s,55);assert(s.note==48);
    bassstation_sustain(&s,1);bassstation_note_off(&s,48);assert(s.gate);bassstation_sustain(&s,0);assert(!s.gate);

    /* Trigger modes. */
    bassstation_defaults(&s);s.p.amp_attack=0.05f;s.p.trigger_mode=BST_TRIG_MULTI;bassstation_note_on(&s,48,100);bassstation_render(&s,lr,256,4096);bassstation_note_on(&s,55,100);assert(s.amp_stage==1);
    /* TEST15: PORTA is a timed pitch ramp. SINGLE/MULTI glide generally; AUTO
       remains legato-only. A 1.0 s octave glide must be exactly halfway in
       semitone space at 0.5 s and land exactly after 44,100 samples. */
    bassstation_defaults(&s);s.p.trigger_mode=BST_TRIG_SINGLE;s.p.glide_s=0.5f;bassstation_note_on(&s,48,100);bassstation_render(&s,lr,256,4096);before=(float)s.current_pitch_q16;bassstation_note_on(&s,55,100);assert(fabsf((float)s.current_pitch_q16-before)<0.01f);assert(s.glide_samples_left==22050u);assert(s.current_pitch_q16!=s.target_pitch_q16);
    bassstation_note_off(&s,55);bassstation_note_off(&s,48);before=(float)s.current_pitch_q16;bassstation_note_on(&s,60,100);assert(fabsf((float)s.current_pitch_q16-before)<0.01f);assert(s.glide_samples_left==22050u);assert(s.current_pitch_q16!=s.target_pitch_q16);
    bassstation_defaults(&s);s.p.trigger_mode=BST_TRIG_AUTOGLIDE;s.p.glide_s=0.5f;bassstation_note_on(&s,48,100);bassstation_render(&s,lr,256,4096);before=(float)s.current_pitch_q16;bassstation_note_on(&s,55,100);assert(fabsf((float)s.current_pitch_q16-before)<0.01f);assert(s.glide_samples_left==22050u);assert(s.current_pitch_q16!=s.target_pitch_q16);
    bassstation_note_off(&s,55);bassstation_note_off(&s,48);bassstation_note_on(&s,60,100);assert(s.current_pitch_q16==s.target_pitch_q16&&s.glide_samples_left==0u);
    bassstation_defaults(&s);s.p.trigger_mode=BST_TRIG_SINGLE;s.p.glide_s=1.0f;bassstation_note_on(&s,48,100);bassstation_note_on(&s,60,100);assert(s.glide_samples_left==44100u);render_frames(&s,lr,22050u);assert(s.current_pitch_q16==(54<<16));assert(s.glide_samples_left==22050u);render_frames(&s,lr,22050u);assert(s.current_pitch_q16==(60<<16));assert(s.glide_samples_left==0u);
    bassstation_defaults(&s);s.p.trigger_mode=BST_TRIG_SINGLE;s.p.glide_s=1.0f;bassstation_note_on(&s,60,100);bassstation_note_on(&s,48,100);assert(s.glide_samples_left==44100u);render_frames(&s,lr,22050u);assert(s.current_pitch_q16==(54<<16));assert(s.glide_samples_left==22050u);render_frames(&s,lr,22050u);assert(s.current_pitch_q16==(48<<16));assert(s.glide_samples_left==0u);
    /* MIDI note 0 is a valid glide origin; do not confuse Q16 value zero with
       an uninitialized pitch. */
    bassstation_defaults(&s);s.p.trigger_mode=BST_TRIG_SINGLE;s.p.glide_s=0.5f;bassstation_note_on(&s,0,100);assert(s.pitch_valid);bassstation_note_on(&s,12,100);assert(s.glide_samples_left==22050u&&s.current_pitch_q16==0);
    assert(velocity_peak(127)>velocity_peak(24)*2);

    /* TEST10: the displayed envelope seconds are stage times, not RC constants.
       A one-second full-scale attack/release should finish at about one second. */
    ev=0.0f;est=1;
    for(i=0;i<39690;i++)bassstation_test_env_step(&ev,&est,1,1.0f,1.0f,0.0f,1.0f);
    assert(est==1);
    for(i=0;i<8820;i++)bassstation_test_env_step(&ev,&est,1,1.0f,1.0f,0.0f,1.0f);
    assert(est!=1);
    ev=1.0f;est=4;
    for(i=0;i<39690;i++)bassstation_test_env_step(&ev,&est,0,1.0f,1.0f,0.0f,1.0f);
    assert(est==4&&ev>0.0f);
    for(i=0;i<8820;i++)bassstation_test_env_step(&ev,&est,0,1.0f,1.0f,0.0f,1.0f);
    assert(est==0&&ev==0.0f);


    /* TEST9: plug-in SAW LFO rises across the cycle, matching the software manual. */
    bassstation_defaults(&s);s.p.lfo_shape=1;s.p.lfo_rate_hz=0.0f;s.p.lfo_delay_s=0.0f;s.lfo_phase=0.25f;assert(bassstation_test_lfo_step(&s)<-0.49f);s.lfo_phase=0.75f;assert(bassstation_test_lfo_step(&s)>0.49f);

    /* TEST9: pressure/breath PITCH controls vibrato depth, not a static pitch offset.
       With an S&H LFO held at zero, enabling pressure-pitch must not alter audio. */
    bassstation_defaults(&a);bassstation_defaults(&b);a.p.lfo_shape=b.p.lfo_shape=2;a.p.lfo_rate_hz=b.p.lfo_rate_hz=0.0f;a.p.aftertouch_pitch=7.0f;b.p.aftertouch_pitch=0.0f;bassstation_note_on(&a,60,100);bassstation_note_on(&b,60,100);bassstation_aftertouch(&a,127);bassstation_render(&a,aaout,256,4096);bassstation_render(&b,bbout,256,4096);assert(memcmp(aaout,bbout,sizeof aaout)==0);

    /* TEST9: stress all waves, slopes, independent PWM sources, wide software tuning and both filter mods. */
    bassstation_defaults(&s);bassstation_note_on(&s,60,127);
    for(mode=0;mode<2;mode++)for(w1=0;w1<2;w1++)for(w2=0;w2<2;w2++)for(src1=0;src1<3;src1++)for(src2=0;src2<3;src2++){
        s.p.filter_24db=(uint8_t)mode;s.p.osc1_wave=(uint8_t)w1;s.p.osc2_wave=(uint8_t)w2;s.p.resonance=1.0f;s.p.cutoff_hz=10000.0f;s.p.filter_env_mod=8.0f;s.p.filter_lfo_mod=8.0f;
        s.p.osc_sync=1;s.p.osc1_pwm_source=(uint8_t)src1;s.p.osc2_pwm_source=(uint8_t)src2;s.p.osc1_pw=1.0f;s.p.osc2_pw=0.0f;s.p.osc2_semitone=(src1&1)?0.0f:12.0f;s.p.osc2_detune=(src2&1)?-0.50f:0.50f;s.p.master_tune=(src1&1)?-0.5f:0.5f;
        s.p.osc1_lfo_pitch=12.0f;s.p.osc2_lfo_pitch=12.0f;s.p.osc1_env_pitch=24.0f;s.p.osc2_env_pitch=24.0f;s.p.lfo_rate_hz=1000.0f;s.p.amp_velocity=1.0f;s.p.mod_velocity=1.0f;s.p.osc_mix=(float)(src1+src2)/4.0f;
        for(i=0;i<12;i++){bassstation_render(&s,lr,256,4096);finite_block(lr,256);}
    }
    bassstation_pitch_bend(&s,16383);bassstation_modwheel(&s,127);bassstation_render(&s,lr,256,4096);finite_block(lr,256);

    /* Pressure/breath and MIDI-clocked LFO. */
    bassstation_defaults(&s);s.p.aftertouch_pitch=7.0f;s.p.aftertouch_filter=7.0f;s.p.aftertouch_bias=15.0f;s.p.breath_pitch=-7.0f;s.p.breath_filter=-7.0f;s.p.breath_bias=15.0f;
    bassstation_aftertouch(&s,127);bassstation_breath(&s,64);assert(s.aftertouch>0.99f&&s.breath>0.49f);bassstation_note_on(&s,60,100);bassstation_render(&s,lr,256,4096);finite_block(lr,256);
    s.p.lfo_clock_div=16;s.p.lfo_shape=1;s.p.lfo_delay_s=0.0f;s.lfo_phase=0.75f;assert(fabsf(bassstation_test_lfo_step(&s))<0.0001f);
    bassstation_midi_start(&s);bassstation_render(&s,lr,1024,4096);bassstation_midi_clock(&s);bassstation_render(&s,lr,1024,4096);bassstation_midi_clock(&s);before=s.lfo_phase;bassstation_render(&s,lr,1024,4096);assert(s.lfo_phase>before);assert(s.midi_clock_period>900.0f&&s.midi_clock_period<1150.0f);
    bassstation_midi_stop(&s);s.lfo_phase=0.75f;assert(fabsf(bassstation_test_lfo_step(&s))<0.0001f);
    bassstation_midi_start(&s);s.midi_clock_period=1000.0f;s.midi_clock_age=5000u;s.lfo_phase=0.75f;assert(fabsf(bassstation_test_lfo_step(&s))<0.0001f);

    bassstation_all_notes_off(&s);for(i=0;i<50;i++)bassstation_render(&s,lr,256,4096);
    free(lr);puts("bassstation TEST15 engine: PASS");return 0;
}
