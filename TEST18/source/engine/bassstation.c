#include "bassstation.h"
#include "fastmath.h"

#define SR 44100.0f
#define FILTER_SR (SR * 2.0f)
#define PI 3.14159265358979323846f

static float clampf(float x,float lo,float hi){ return x<lo?lo:(x>hi?hi:x); }
static int clampi(int x,int lo,int hi){ return x<lo?lo:(x>hi?hi:x); }
/* The plug-in presents A/D/R as stage times (up to 20 seconds), not RC time
   constants.  Keep the exponential contour, but calibrate each one-pole stage
   so a full-scale transition reaches env_step()'s completion threshold at the
   requested time. */
static float env_stage_k(float seconds,float log_remaining){
    if(seconds<=0.0f)return 1.0f;
    seconds=seconds<0.0005f?0.0005f:seconds;
    return 1.0f-fm_expf(log_remaining/(seconds*SR));
}
static float env_attack_k(float seconds){ return env_stage_k(seconds,-4.519810f); } /* ln((1.01-.999)/1.01) */
static float env_decay_k(float seconds){ return env_stage_k(seconds,-6.907755f); } /* ln(.001), full-scale */
static float env_release_k(float seconds){ return env_stage_k(seconds,-7.600902f); } /* ln(.0005), full-scale */
static float poly_blep(float t,float dt){
    if(dt<=0.0f)return 0.0f;
    if(t<dt){float x=t/dt; return x+x-x*x-1.0f;}
    if(t>1.0f-dt){float x=(t-1.0f)/dt; return x*x+x+x+1.0f;}
    return 0.0f;
}
static float pulse(float ph,float dt,float duty){
    float y; duty=clampf(duty,0.03f,0.97f); y=ph<duty?1.0f:-1.0f; y+=poly_blep(ph,dt);
    {float p=ph+1.0f-duty; if(p>=1.0f)p-=1.0f; y-=poly_blep(p,dt);} return y;
}
static float saw(float ph,float dt){ float y=2.0f*ph-1.0f; return y-poly_blep(ph,dt); }
static float noise(bassstation_t *s){ uint32_t x=s->noise_state?s->noise_state:0x9E3779B9u; x^=x<<13; x^=x>>17; x^=x<<5; s->noise_state=x; return ((float)(x&0x00ffffffu)*(1.0f/8388607.5f))-1.0f; }

static float env_step(float *v,uint8_t *stage,int gate,float a,float d,float sus,float r){
    float x=*v; uint8_t st=*stage;
    if(st==1){ x+=(1.01f-x)*a; if(x>=0.999f){x=1.0f;st=2;} }
    else if(st==2){ x+=(sus-x)*d; if(fm_fabsf(x-sus)<0.001f){x=sus;st=3;} }
    else if(st==3){ x=sus; if(!gate)st=4; }
    else if(st==4){ x+=(0.0f-x)*r; if(x<0.0005f){x=0.0f;st=0;} }
    *v=x;*stage=st;return x;
}

static int newest_note(const bassstation_t *s){
    int n,best=-1; uint32_t newest=0;
    for(n=0;n<128;n++) if(s->pressed[n]||s->sustained[n]){
        if(best<0||s->age[n]>=newest){best=n;newest=s->age[n];}
    }
    return best;
}

static void set_pitch_target_time(bassstation_t *s,int note,int glide_allowed,float glide_s){
    int32_t target=(int32_t)note<<16;
    uint32_t ns; int32_t delta;
    s->target_pitch_q16=target;

    if(!glide_allowed || !s->pitch_valid || glide_s<=0.0002f ||
       s->current_pitch_q16==target){
        s->current_pitch_q16=target;s->pitch_valid=1;
        s->glide_step_q16=0;s->glide_remainder_q16=0;s->glide_error_q16=0;
        s->glide_samples_left=0;s->glide_total_samples=0;
        return;
    }

    /* Bass Station/Rack documentation specifies PORTA as a 0..5 second ramp
       time.  Model that as a constant pitch/CV ramp rather than an RC smoother.
       The Q16.16 DDA distributes the fractional step so the target is reached
       on the exact requested sample without accumulating float drift. */
    ns=(uint32_t)(clampf(glide_s,0.0005f,5.0f)*SR+0.5f);
    if(ns<1u)ns=1u;
    delta=target-s->current_pitch_q16;s->pitch_valid=1;
    s->glide_step_q16=delta/(int32_t)ns;
    s->glide_remainder_q16=delta%(int32_t)ns;
    s->glide_error_q16=0;
    s->glide_samples_left=ns;
    s->glide_total_samples=ns;
}

static void set_pitch_target(bassstation_t *s,int note,int glide_allowed){
    set_pitch_target_time(s,note,glide_allowed,s->p.glide_s);
}

static void pitch_glide_step(bassstation_t *s){
    int32_t den;
    if(!s->glide_samples_left)return;
    den=(int32_t)s->glide_total_samples;
    s->current_pitch_q16+=s->glide_step_q16;
    s->glide_error_q16+=s->glide_remainder_q16;
    if(s->glide_error_q16>=den){s->current_pitch_q16++;s->glide_error_q16-=den;}
    else if(s->glide_error_q16<=-den){s->current_pitch_q16--;s->glide_error_q16+=den;}
    if(--s->glide_samples_left==0u){
        s->current_pitch_q16=s->target_pitch_q16;
        s->glide_step_q16=0;s->glide_remainder_q16=0;s->glide_error_q16=0;
        s->glide_total_samples=0;
    }
}

static void select_note(bassstation_t *s,int best,int note_on_event,int had_gate){
    int glide_allowed;
    if(best<0){
        s->note=-1;s->gate=0;s->amp_stage=4;s->mod_stage=4;
        return;
    }
    s->note=(int8_t)best;s->gate=1;s->velocity=clampf((float)s->note_velocity[best]/127.0f,0.0f,1.0f);

    /* PORTA applies in SINGLE and MULTI even across a key gap.  AUTO is the
       special legato-only mode: a fresh phrase starts exactly on pitch. */
    glide_allowed=!(s->p.trigger_mode==BST_TRIG_AUTOGLIDE && !had_gate);
    set_pitch_target(s,best,glide_allowed);

    if(!had_gate || (note_on_event && s->p.trigger_mode==BST_TRIG_MULTI)){
        s->amp_stage=1;s->mod_stage=1;s->lfo_fade=0.0f;
    }
}

void bassstation_defaults(bassstation_t *s){
    uint8_t *q=(uint8_t*)s; uint32_t i; for(i=0;i<(uint32_t)sizeof(*s);i++)q[i]=0;
    s->note=-1;s->noise_state=0x1234ABCDu;
    s->p.osc1_wave=0;s->p.osc2_wave=0;s->p.osc2_range=1;
    s->p.osc1_pwm_source=0;s->p.osc2_pwm_source=0;
    s->p.osc1_pw=0.5f;s->p.osc2_pw=0.5f;
    s->p.osc2_semitone=0.0f;s->p.osc2_detune=0.0f;
    s->p.osc1_lfo_pitch=0.0f;s->p.osc2_lfo_pitch=0.0f;s->p.osc1_env_pitch=0.0f;s->p.osc2_env_pitch=0.0f;
    s->p.osc_mix=0.45f;
    s->p.cutoff_hz=1200.0f;s->p.resonance=0.28f;s->p.filter_env_mod=2.5f;s->p.filter_lfo_mod=0.0f;s->p.filter_keytrack=0.35f;s->p.filter_24db=1;
    s->p.amp_attack=0.004f;s->p.amp_decay=0.20f;s->p.amp_sustain=0.82f;s->p.amp_release=0.10f;s->p.amp_velocity=0.0f;
    s->p.mod_attack=0.004f;s->p.mod_decay=0.30f;s->p.mod_sustain=0.15f;s->p.mod_release=0.12f;s->p.mod_velocity=0.0f;
    s->p.lfo_shape=0;s->p.lfo_rate_hz=5.0f;s->p.lfo_delay_s=0.0f;
    s->p.trigger_mode=BST_TRIG_MULTI;s->p.lfo_clock_div=0;s->p.glide_s=0.0f;s->p.master_tune=0.0f;s->p.bend_range=2.0f;s->p.wheel_pitch=2.0f;s->p.wheel_filter=1.0f;
    s->p.aftertouch_pitch=0.0f;s->p.aftertouch_filter=0.0f;s->p.aftertouch_bias=0.0f;
    s->p.breath_pitch=0.0f;s->p.breath_filter=0.0f;s->p.breath_bias=0.0f;s->p.volume=0.85f;
}
void bassstation_note_on(bassstation_t *s,int note,int vel){
    int had=s->gate,best;if((unsigned)note>=128u)return;
    s->pressed[note]=1;s->sustained[note]=0;s->age[note]=++s->age_counter;s->note_velocity[note]=(uint8_t)clampi(vel,1,127);
    best=newest_note(s);select_note(s,best,1,had);
}
void bassstation_note_off(bassstation_t *s,int note){
    int had=s->gate,best;if((unsigned)note>=128u)return;
    s->pressed[note]=0;if(s->sustain)s->sustained[note]=1;else s->sustained[note]=0;
    best=newest_note(s);select_note(s,best,0,had);
}
void bassstation_note_slide(bassstation_t *s,int from_note,int to_note,int vel,float seconds){
    if((unsigned)to_note>=128u)return;
    if(!s->gate){bassstation_note_on(s,to_note,vel);return;}
    if((unsigned)from_note<128u){s->pressed[from_note]=0;s->sustained[from_note]=0;}
    s->pressed[to_note]=1;s->sustained[to_note]=0;s->age[to_note]=++s->age_counter;
    s->note_velocity[to_note]=(uint8_t)clampi(vel,1,127);s->note=(int8_t)to_note;s->gate=1;
    s->velocity=clampf((float)s->note_velocity[to_note]/127.0f,0.0f,1.0f);
    set_pitch_target_time(s,to_note,1,seconds);
    /* A 303 slide is legato: pitch changes while the envelopes remain in their
       current stages.  This helper is deliberately separate from PORTA so the
       sequencer can use tempo-relative slide time without changing the patch. */
}
void bassstation_sustain(bassstation_t *s,int on){
    int n,best;s->sustain=on?1u:0u;if(!s->sustain){for(n=0;n<128;n++)if(!s->pressed[n])s->sustained[n]=0;best=newest_note(s);select_note(s,best,0,s->gate);}
}
void bassstation_all_notes_off(bassstation_t *s){uint32_t i;for(i=0;i<128u;i++){s->pressed[i]=0;s->sustained[i]=0;}s->sustain=0;s->gate=0;s->note=-1;s->amp_stage=4;s->mod_stage=4;}
void bassstation_pitch_bend(bassstation_t *s,int bend14){s->bend=clampf((float)(bend14-8192)/8192.0f,-1,1);}
void bassstation_modwheel(bassstation_t *s,int value){s->modwheel=clampf((float)value/127.0f,0,1);}
void bassstation_aftertouch(bassstation_t *s,int value){s->aftertouch=clampf((float)value/127.0f,0,1);}
void bassstation_breath(bassstation_t *s,int value){s->breath=clampf((float)value/127.0f,0,1);}
void bassstation_midi_start(bassstation_t *s){s->midi_clock_running=1;s->midi_clock_seen=0;s->midi_clock_age=0;s->lfo_phase=0.0f;}
void bassstation_midi_stop(bassstation_t *s){s->midi_clock_running=0;}
void bassstation_midi_clock(bassstation_t *s){
    uint32_t age=s->midi_clock_age;
    s->midi_clock_running=1;
    if(s->midi_clock_seen&&age>64u&&age<(uint32_t)(SR*2.0f)){
        float a=(float)age;
        if(s->midi_clock_period<=0.0f)s->midi_clock_period=a;
        else s->midi_clock_period=s->midi_clock_period*0.75f+a*0.25f;
    }
    s->midi_clock_seen=1;s->midi_clock_age=0;
}

static int clock_ticks(uint8_t idx){
    static const uint8_t ticks[17]={0,2,3,4,6,8,9,12,16,18,24,32,36,48,64,72,96};
    return ticks[idx<17u?idx:0u];
}
static float lfo_step(bassstation_t *s){
    float v,ph,inc;int ticks=clock_ticks(s->p.lfo_clock_div),clock_ok=1;
    if(ticks){
        clock_ok=s->midi_clock_running&&s->midi_clock_period>1.0f&&
                 (float)s->midi_clock_age<(s->midi_clock_period*4.0f+64.0f);
        inc=clock_ok?1.0f/(s->midi_clock_period*(float)ticks):0.0f;
    } else inc=clampf(s->p.lfo_rate_hz,0.0f,1000.0f)/SR;
    s->lfo_phase+=inc;if(s->lfo_phase>=1.0f){s->lfo_phase-=1.0f;if(s->p.lfo_shape==2)s->lfo_hold=noise(s);}ph=s->lfo_phase;
    if(s->p.lfo_shape==1)v=2.0f*ph-1.0f;else if(s->p.lfo_shape==2)v=s->lfo_hold;else v=1.0f-4.0f*fm_fabsf(ph-0.5f);
    if(s->p.lfo_delay_s<=0.0001f)s->lfo_fade=1.0f;else{s->lfo_fade+=1.0f/(s->p.lfo_delay_s*SR);if(s->lfo_fade>1.0f)s->lfo_fade=1.0f;}
    if(ticks&&!clock_ok)return 0.0f;
    return v*s->lfo_fade;
}
#ifdef BASSSTATION_TEST
float bassstation_test_lfo_step(bassstation_t *s){return lfo_step(s);}
float bassstation_test_env_step(float *v,uint8_t *stage,int gate,float attack,float decay,float sustain,float release){
    return env_step(v,stage,gate,env_attack_k(attack),env_decay_k(decay),sustain,env_release_k(release));
}
#endif

static float ladder_step(float x,float g,float k,float *z1,float *z2,float *z3,float *z4,int poles24){
    float G=g/(1.0f+g),u,y1,y2,y3,y4,fb=poles24?*z4:*z2;
    u=fm_tanhf((x-k*fb)*1.18f);
    y1=G*(u-*z1)+*z1;*z1=2.0f*y1-*z1;
    y2=G*(y1-*z2)+*z2;*z2=2.0f*y2-*z2;
    if(!poles24)return y2;
    y3=G*(y2-*z3)+*z3;*z3=2.0f*y3-*z3;
    y4=G*(y3-*z4)+*z4;*z4=2.0f*y4-*z4;return y4;
}

static float filter_2x(bassstation_t *s,float sig,float fc,float resonance,int poles24){
    float g,k,x0,y;
    /* Bass Station 12 dB mode intentionally has a TB-303-like restricted top end;
       24 dB mode extends to ~10 kHz. */
    fc=clampf(fc,5.0f,poles24?10000.0f:4500.0f);
    g=fm_tanf(PI*fc/FILTER_SR);g=clampf(g,0.00005f,1.5f);
    k=clampf(resonance,0.0f,1.0f)*(poles24?4.28f:2.08f);
    x0=0.5f*(s->filter_prev_in+sig);
    y=ladder_step(x0,g,k,&s->z1,&s->z2,&s->z3,&s->z4,poles24);
    y=ladder_step(sig,g,k,&s->z1,&s->z2,&s->z3,&s->z4,poles24);
    s->filter_prev_in=sig;
    return y;
}

static float pw_value(float control,int src,float mod){
    control=clampf(control,0.0f,1.0f);
    if(src==0){
        /* Plug-in MAN control: centre is square; either direction narrows one side. */
        return clampf(0.03f+0.94f*control,0.03f,0.97f);
    }
    /* ENV2/fixed-PWM-LFO: centre is zero depth and the two sides invert modulation. */
    return clampf(0.5f+(control-0.5f)*1.88f*mod,0.03f,0.97f);
}

void bassstation_render(bassstation_t *s,int32_t *lr,uint32_t n,uint32_t master_q12){
    uint32_t i;static const float range_st[4]={-12.0f,0.0f,12.0f,24.0f};
    float aa=env_attack_k(s->p.amp_attack),ad=env_decay_k(s->p.amp_decay),ar=env_release_k(s->p.amp_release),ma=env_attack_k(s->p.mod_attack),md=env_decay_k(s->p.mod_decay),mr=env_release_k(s->p.mod_release);
    float out=7600000.0f*((float)master_q12/4096.0f)*clampf(s->p.volume,0.0f,1.25f);
    int r2=clampi((int)s->p.osc2_range,0,3),w1=s->p.osc1_wave?1:0,w2=s->p.osc2_wave?1:0;
    for(i=0;i<n;i++){
        if(s->midi_clock_age<0x7fffffffu)s->midi_clock_age++;
        float lfo=lfo_step(s),ae=env_step(&s->amp_env,&s->amp_stage,s->gate,aa,ad,clampf(s->p.amp_sustain,0,1),ar),me=env_step(&s->mod_env,&s->mod_stage,s->gate,ma,md,clampf(s->p.mod_sustain,0,1),mr);
        float vel=s->velocity,amp_vel=1.0f-clampf(s->p.amp_velocity,0,1)*(1.0f-vel),mod_vel=1.0f-clampf(s->p.mod_velocity,0,1)*(1.0f-vel);
        float me_v=me*mod_vel,wheel=s->modwheel,bend=s->bend*clampf(s->p.bend_range,0,12);
        float ctrl_vibrato=s->aftertouch*s->p.aftertouch_pitch+s->breath*s->p.breath_pitch;
        float ctrl_filter=0.5f*(s->aftertouch*s->p.aftertouch_filter+s->breath*s->p.breath_filter);
        float ctrl_bias=(s->aftertouch*s->p.aftertouch_bias+s->breath*s->p.breath_bias)*(1.0f/15.0f);
        float pwmph,pwm,pwm1,pwm2,pw1,pw2,semi1,semi2,base_note,hz1,hz2,dt1,dt2,o1,o2,sig,fc,envdepth,y,sample;int wrap1=0;
        pitch_glide_step(s);
        base_note=(float)s->current_pitch_q16*(1.0f/65536.0f);
        s->pwm_phase+=0.35f/SR;if(s->pwm_phase>=1.0f)s->pwm_phase-=1.0f;pwmph=s->pwm_phase;pwm=1.0f-4.0f*fm_fabsf(pwmph-0.5f);
        pwm1=s->p.osc1_pwm_source==1?me_v:(s->p.osc1_pwm_source==2?pwm:0.0f);
        pwm2=s->p.osc2_pwm_source==1?me_v:(s->p.osc2_pwm_source==2?pwm:0.0f);
        pw1=pw_value(s->p.osc1_pw,s->p.osc1_pwm_source,pwm1);pw2=pw_value(s->p.osc2_pw,s->p.osc2_pwm_source,pwm2);
        semi1=s->p.master_tune+bend+lfo*(s->p.osc1_lfo_pitch+wheel*s->p.wheel_pitch+ctrl_vibrato)+me_v*s->p.osc1_env_pitch;
        semi2=range_st[r2]+s->p.master_tune+s->p.osc2_semitone+s->p.osc2_detune+bend+lfo*(s->p.osc2_lfo_pitch+wheel*s->p.wheel_pitch+ctrl_vibrato)+me_v*s->p.osc2_env_pitch;
        hz1=440.0f*fm_exp2f((base_note-69.0f+semi1)*(1.0f/12.0f));hz2=440.0f*fm_exp2f((base_note-69.0f+semi2)*(1.0f/12.0f));dt1=clampf(hz1/SR,0,0.49f);dt2=clampf(hz2/SR,0,0.49f);
        s->phase1+=dt1;if(s->phase1>=1.0f){s->phase1-=1.0f;wrap1=1;}if(s->p.osc_sync&&wrap1)s->phase2=0.0f;else{s->phase2+=dt2;if(s->phase2>=1.0f)s->phase2-=1.0f;}
        o1=w1?pulse(s->phase1,dt1,pw1):saw(s->phase1,dt1);o2=w2?pulse(s->phase2,dt2,pw2):saw(s->phase2,dt2);
        {float mix=clampf(s->p.osc_mix,0.0f,1.0f);sig=(o1*(1.0f-mix)+o2*mix)*0.78f;}
        envdepth=s->p.filter_env_mod;
        if(!s->p.filter_24db)envdepth=0.15f+(clampf(envdepth,-8.0f,8.0f)+8.0f)*(7.85f/16.0f);
        fc=s->p.cutoff_hz*fm_exp2f(me_v*envdepth+lfo*s->p.filter_lfo_mod+lfo*wheel*s->p.wheel_filter+ctrl_filter);
        if(s->p.filter_24db&&s->p.filter_keytrack>0&&s->note>=0)fc*=fm_exp2f(((float)s->note-60.0f)*(clampf(s->p.filter_keytrack,0,1)/12.0f));
        y=filter_2x(s,sig,fc,s->p.resonance,s->p.filter_24db!=0);
        sample=fm_tanhf(y*clampf(ae*amp_vel+ctrl_bias,0.0f,1.25f)*1.1f)*out;lr[2u*i]=(int32_t)sample;lr[2u*i+1u]=(int32_t)sample;
    }
}
