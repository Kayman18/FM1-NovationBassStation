/* SPDX-License-Identifier: MPL-2.0 */
#include "fastmath.h"
#include "bassstation.h"
#include "acid_seq.h"
#include "factory_bank.h"
#include "community_bank.h"
#include "splash_bassstation.h"
#ifndef PLAT_HOST
#include "perf.h"
#endif
#ifdef PLAT_HOST
#include <string.h>
#endif

#define SET_MAGIC 0x42535438u /* BST8 / TEST17-21 compatible settings */
#define PREV_SET_MAGIC 0x42535437u /* BST7 / TEST15-16 settings */
#define OLD_SET_MAGIC 0x42535435u /* BST5/TEST6 settings migration */
#define BANK_MAGIC 0x42534237u /* BSB7 */
#define USER_SLOTS 128u
#define FACTORY_SLOTS 100u
#define BUILTIN_SLOTS 101u /* INIT + 100 recovered factory programs */
#define BANK_CHUNK_SLOTS 20u
#define BANK_NAME_LEN 20u
#define USER_SEED_VERSION 0x21u /* TEST21: seed empty USER slots once */

typedef struct {
    uint32_t magic;
    int8_t octave;
    uint8_t preset;
    uint8_t page;
    uint8_t custom;
    bassstation_params_t p;
} legacy_settings_t;

typedef struct {
    uint32_t magic;
    int8_t octave;
    uint8_t preset;
    uint8_t page;
    uint8_t custom;
    uint8_t bank;
    uint8_t reserved[3];
    bassstation_params_t p;
} prev_settings_t;

typedef struct {
    uint32_t magic;
    int8_t octave;
    uint8_t preset;
    uint8_t page;
    uint8_t custom;
    uint8_t bank;                 /* 0 = built-in/verified, 1 = USER / COMMUNITY */
    uint8_t reserved[3];
    bassstation_params_t p;
    acid_seq_t seq;               /* TEST17: global deterministic 303-style sequence */
} settings_t;
typedef char settings_must_fit[(sizeof(settings_t)<=3840u)?1:-1];

typedef struct {
    uint32_t magic;
    uint32_t valid;               /* 20 bits, one per slot */
    uint8_t base;                 /* first slot number in this chunk */
    uint8_t reserved[3];
    bassstation_params_t patch[BANK_CHUNK_SLOTS];
    char name[BANK_CHUNK_SLOTS][BANK_NAME_LEN];
} user_bank_chunk_t;
typedef char user_bank_chunk_must_fit[(sizeof(user_bank_chunk_t)<=3840u)?1:-1];

static settings_t set;
static bassstation_t synth;
static user_bank_chunk_t bank_cache;
static uint8_t bank_cache_idx=0xffu,import_on,import_slot;
static uint8_t dirty,seq_dirty;

enum { PG_OSC1, PG_OSC2, PG_OSCMOD, PG_MIX, PG_FILTER, PG_AMPENV, PG_MODENV, PG_TRIGGER, PG_LFO, PG_GLOBAL, PG_AFTERTOUCH, PG_BREATH, PG_PERF, PG_SEQ, PG_PRESETS, PG_COUNT };
enum { SQ_GEN=0, SQ_MUTATE, SQ_PIANO, SQ_GATE, SQ_OCT, SQ_ACC, SQ_SLIDE, SQ_MODE_COUNT };
static const char *const PAGE_NAME[PG_COUNT] = {"OSCILLATOR 1","OSCILLATOR 2","OSC MODULATION","MIXER","FILTER","AMP ENVELOPE","MOD ENVELOPE","TRIGGER / VELOCITY","LFO","GLOBAL","AFTERTOUCH","BREATH","PERFORMANCE","303 SEQUENCER","PRESET BROWSER"};
static const char *const WAVE[2] = {"SAW","PULSE"};
static const char *const RANGE[4] = {"16'","8'","4'","2'"};
static const char *const PWM_SRC[3] = {"MANUAL","ENV2","PWM LFO"};
static const char *const LFO_SHAPE[3] = {"TRI","SAW UP","S&H"};
static const char *const TRIGGER[3] = {"AUTOGLIDE","SINGLE","MULTI"};
static const char *const CLOCK_DIV[17] = {"FREE","1/32T","1/32","1/16T","1/16","1/8T","1/16.","1/8","1/4T","1/8.","1/4","1/2T","1/4.","1/2","1BAR T","1/2.","1/1"};

static struct {
    uint32_t keys, btn;
    uint8_t vel[NKEYS];
    uint8_t redraw;
    uint8_t browse_bank, browse_preset;
    uint8_t seq_panel, seq_mode, seq_step;
    uint8_t home;                 /* TEST18: SH-101-style performance home overlay */
    char msg[2][24];
    uint32_t msg_until;
} ui;

typedef struct {
    uint8_t running;
    uint8_t pos;
    uint8_t midi_phase;
    uint8_t gate_pending;
    int16_t current_note;
    uint32_t step_samples;
    uint32_t samples_to_step;
    uint32_t samples_to_gate;
    volatile uint8_t redraw_pending;
} seq_runtime_t;
static seq_runtime_t seqrt;
static acid_seq_undo_t seq_undo;
static uint8_t bw_connected;
static void bw_send_synth(void);
static void bw_send_seq(void);
static int stepi(int v,int d,int lo,int hi);

static void text_cat(char *d,const char *s){while(*d)d++;while(s&&*s)*d++=*s++;*d=0;}
static void int_text(int32_t v,char *b){char t[16];int n=0;uint32_t u=(uint32_t)(v<0?-v:v);do{t[n++]=(char)('0'+u%10u);}while((u/=10u)&&n<15);if(v<0)*b++='-';while(n)*b++=t[--n];*b=0;}
static void pct_text(float v,char *b){int_text((int32_t)(v*100.0f+(v>=0?0.5f:-0.5f)),b);text_cat(b,"%");}
static void f1_text(float v,char *b){int32_t q=(int32_t)(v*10.0f+(v>=0?0.5f:-0.5f));char t[16];int_text(q/10,b);text_cat(b,".");{int32_t r=q%10;if(r<0)r=-r;int_text(r,t);}text_cat(b,t);}
static void cents_text(float st,char *b){int_text((int32_t)(st*100.0f+(st>=0?0.5f:-0.5f)),b);text_cat(b,"c");}

void app_say(const char *a,const char *b){uint32_t i;for(i=0;i<23u&&a&&a[i];i++)ui.msg[0][i]=a[i];ui.msg[0][i]=0;for(i=0;i<23u&&b&&b[i];i++)ui.msg[1][i]=b[i];ui.msg[1][i]=0;ui.msg_until=plat_ms()+1100u;ui.redraw=1;}
static void apply_params(void){synth.p=set.p;}
static void defaults(void){uint8_t *q=(uint8_t*)&set;uint32_t i;for(i=0;i<(uint32_t)sizeof set;i++)q[i]=0;set.magic=SET_MAGIC;set.octave=0;set.page=PG_OSC1;set.bank=0;bassstation_defaults(&synth);set.p=synth.p;acid_seq_defaults(&set.seq);}

static void mem_zero(void *vp,uint32_t n){uint8_t *p=(uint8_t*)vp;while(n--)*p++=0;}
static void name_copy(char *d,const char *s,uint32_t n){uint32_t i=0;if(!n)return;while(i+1u<n&&s&&s[i]){char c=s[i];d[i]=(c>=32&&c<=126)?c:' ';i++;}d[i]=0;while(++i<n)d[i]=0;}
static int bank_obj(uint8_t chunk){return (int)OBJ_BANK0+(int)chunk;}
static void default_slot_name(char *b,uint8_t slot,const char *prefix){char n[12];b[0]=0;text_cat(b,prefix);int_text((int32_t)slot+1,n);text_cat(b,n);}
static int bank_chunk_load(uint8_t chunk){int n;if(chunk>=7u)return 0;if(bank_cache_idx==chunk)return 1;n=plat_store_load(bank_obj(chunk),&bank_cache,sizeof bank_cache);if(n!=(int)sizeof bank_cache||bank_cache.magic!=BANK_MAGIC||bank_cache.base!=(uint8_t)(chunk*BANK_CHUNK_SLOTS)){mem_zero(&bank_cache,sizeof bank_cache);bank_cache.magic=BANK_MAGIC;bank_cache.base=(uint8_t)(chunk*BANK_CHUNK_SLOTS);}bank_cache_idx=chunk;return 1;}
static int bank_chunk_save(void){return bank_cache_idx<7u&&plat_store_save(bank_obj(bank_cache_idx),&bank_cache,sizeof bank_cache)==0;}
static int user_valid(uint8_t slot){uint8_t chunk=(uint8_t)(slot/BANK_CHUNK_SLOTS),i=(uint8_t)(slot%BANK_CHUNK_SLOTS);if(slot>=USER_SLOTS||!bank_chunk_load(chunk))return 0;return (bank_cache.valid&(1u<<i))!=0u;}
static void user_name(uint8_t slot,char *b){uint8_t chunk=(uint8_t)(slot/BANK_CHUNK_SLOTS),i=(uint8_t)(slot%BANK_CHUNK_SLOTS);b[0]=0;if(slot<USER_SLOTS&&bank_chunk_load(chunk)&&(bank_cache.valid&(1u<<i)))name_copy(b,bank_cache.name[i],BANK_NAME_LEN);}
static int user_store(uint8_t slot,const bassstation_params_t *p,const char *name,int imported){uint8_t chunk=(uint8_t)(slot/BANK_CHUNK_SLOTS),i=(uint8_t)(slot%BANK_CHUNK_SLOTS);char d[BANK_NAME_LEN];if(slot>=USER_SLOTS||!p||!bank_chunk_load(chunk))return 0;bank_cache.patch[i]=*p;bank_cache.valid|=1u<<i;if(name&&*name)name_copy(bank_cache.name[i],name,BANK_NAME_LEN);else if(!bank_cache.name[i][0]){default_slot_name(d,slot,imported?"IMPORT ":"USER ");name_copy(bank_cache.name[i],d,BANK_NAME_LEN);}return bank_chunk_save();}
static int user_set_name(uint8_t slot,const uint8_t *name,uint32_t n){uint8_t chunk=(uint8_t)(slot/BANK_CHUNK_SLOTS),i=(uint8_t)(slot%BANK_CHUNK_SLOTS);uint32_t k;if(slot>=USER_SLOTS||!bank_chunk_load(chunk)||(bank_cache.valid&(1u<<i))==0u)return 0;mem_zero(bank_cache.name[i],BANK_NAME_LEN);for(k=0;k<n&&k+1u<BANK_NAME_LEN;k++){uint8_t c=name[k]&0x7fu;bank_cache.name[i][k]=(char)((c>=32u&&c<=126u)?c:' ');}return bank_chunk_save();}
static int user_clear(uint8_t slot){uint8_t chunk=(uint8_t)(slot/BANK_CHUNK_SLOTS),i=(uint8_t)(slot%BANK_CHUNK_SLOTS);if(slot>=USER_SLOTS||!bank_chunk_load(chunk))return 0;bank_cache.valid&=~(1u<<i);mem_zero(bank_cache.name[i],BANK_NAME_LEN);return bank_chunk_save();}

/* TEST21 USER-bank bootstrap.  Only empty slots are populated and a marker in
   the existing settings reserved byte prevents deliberate clears from being
   refilled on later boots.  Slots 1..38 are the two recovered Rekkerd community
   banks; slots 39..128 are copies of recovered factory programs 00..89. */
static int user_seed_record(uint8_t slot,const uint8_t record[34],const char *name){
    uint8_t chunk=(uint8_t)(slot/BANK_CHUNK_SLOTS),i=(uint8_t)(slot%BANK_CHUNK_SLOTS);
    bassstation_params_t q=synth.p;
    if(slot>=USER_SLOTS||!bank_chunk_load(chunk)||(bank_cache.valid&(1u<<i)))return 0;
    if(!bassstation_plugin_factory_decode(&q,record))return 0;
    bank_cache.patch[i]=q;bank_cache.valid|=1u<<i;name_copy(bank_cache.name[i],name,BANK_NAME_LEN);
    return 1;
}
static int user_seed_empty_slots(void){
    uint8_t chunk,slot;int changed,total=0;
    if(set.reserved[0]==USER_SEED_VERSION)return 0;
    for(chunk=0;chunk<7u;chunk++){
        if(!bank_chunk_load(chunk)) continue;
        changed=0;
        for(slot=bank_cache.base;slot<USER_SLOTS&&slot<(uint8_t)(bank_cache.base+BANK_CHUNK_SLOTS);slot++){
            const uint8_t *record;const char *name;uint8_t i=(uint8_t)(slot-bank_cache.base);
            if(bank_cache.valid&(1u<<i))continue;
            if(slot<BST_COMMUNITY_COUNT){record=bst_community_record[slot];name=bst_community_name[slot];}
            else{uint8_t f=(uint8_t)(slot-BST_COMMUNITY_COUNT);record=bst_factory_record[f];name=bst_factory_name[f];}
            if(user_seed_record(slot,record,name)){changed=1;total++;}
        }
        if(changed&&!bank_chunk_save())return -1;
    }
    set.reserved[0]=USER_SEED_VERSION;
    if(plat_store_save(OBJ_SETTINGS,&set,sizeof set)!=0)return -1;
    return total;
}

/* Bass Station 2.x plug-in panel conversions used by MIDI CC automation.
   Factory programs use the recovered 34-byte native plug-in records below. */
static float plug_signed(int v,float depth){float q;if(v<-64)v=-64;if(v>63)v=63;q=v<0?(float)v/64.0f:(float)v/63.0f;return q*depth;}
static float plug_pw(int v){int q=v+64;if(q<0)q=0;if(q>127)q=127;return (float)q/127.0f;}
static float plug_exp(int v,float lo,float hi){float q=(float)v/127.0f;if(v<=0)return lo;return lo*fm_exp2f(q*fm_log2f(hi/lo));}
static float plug_attack(int v){return v<=0?0.0f:plug_exp(v,0.0005f,20.0f);}
static float plug_dr(int v){return v<=0?0.0f:plug_exp(v,0.003f,20.0f);}
static float plug_cutoff(int v){return plug_exp(v,10.0f,10000.0f);}
/* Official Novation docs give ~7 Hz at speed 75 and a 1 kHz maximum. */
static float plug_lfo_rate(int v){return v<=0?0.0f:plug_exp(v,0.0055f,1000.0f);}
static float plug_lfo_delay(int v){return v<=0?0.0f:plug_exp(v,0.005f,5.0f);}
static float plug_glide(int v){return v<=0?0.0f:plug_exp(v,0.005f,5.0f);}

static void builtin_load(int idx){
    bassstation_all_notes_off(&synth);
    bassstation_defaults(&synth);
    if(idx>0&&idx<(int)BUILTIN_SLOTS){
        /* Built-in 1..100 = original plug-in factory programs 00..99. */
        bassstation_plugin_factory_decode(&synth.p,bst_factory_record[idx-1]);
    }
    set.p=synth.p;set.bank=0;set.preset=(uint8_t)idx;set.custom=0;dirty=0;ui.redraw=1;bw_send_synth();
}

static int user_load(uint8_t slot){uint8_t chunk=(uint8_t)(slot/BANK_CHUNK_SLOTS),i=(uint8_t)(slot%BANK_CHUNK_SLOTS);int ok=user_valid(slot);bassstation_all_notes_off(&synth);bassstation_defaults(&synth);if(ok&&bank_chunk_load(chunk))synth.p=bank_cache.patch[i];set.p=synth.p;set.bank=1;set.preset=slot;set.custom=0;dirty=0;ui.redraw=1;bw_send_synth();return ok;}
static void preset_select(int idx){if(set.bank){if(idx<0)idx=(int)USER_SLOTS-1;if(idx>=(int)USER_SLOTS)idx=0;user_load((uint8_t)idx);}else{if(idx<0)idx=(int)BUILTIN_SLOTS-1;if(idx>=(int)BUILTIN_SLOTS)idx=0;builtin_load(idx);}}
static void bank_select(uint8_t bank){int p=(int)set.preset;if(bank){set.bank=1;if(p>=(int)USER_SLOTS)p=0;user_load((uint8_t)p);app_say("PRESET BANK","USER / COMMUNITY");}else{set.bank=0;if(p>=(int)BUILTIN_SLOTS)p=0;builtin_load(p);app_say("PRESET BANK","FACTORY / INIT");}}

static void browser_sync(void){ui.browse_bank=set.bank;ui.browse_preset=set.preset;}
static void browser_open(void){browser_sync();ui.home=0;set.page=PG_PRESETS;ui.redraw=1;}
static void browser_step(int d){int hi=ui.browse_bank?(int)USER_SLOTS-1:(int)BUILTIN_SLOTS-1;int p=(int)ui.browse_preset+(d>0?1:-1);if(p<0)p=hi;if(p>hi)p=0;ui.browse_preset=(uint8_t)p;ui.redraw=1;}
static void browser_bank_toggle(void){ui.browse_bank^=1u;ui.browse_preset=0;ui.redraw=1;}
static void browser_load(void){if(ui.browse_bank)user_load(ui.browse_preset);else builtin_load((int)ui.browse_preset);browser_sync();ui.redraw=1;}

void app_init(void){bw_connected=0;
    int n;legacy_settings_t old;prev_settings_t prev;
    bassstation_defaults(&synth);n=plat_store_load(OBJ_SETTINGS,&set,sizeof set);
    if(n!=(int)sizeof set||set.magic!=SET_MAGIC){
        n=plat_store_load(OBJ_SETTINGS,&prev,sizeof prev);
        if(n==(int)sizeof prev&&prev.magic==PREV_SET_MAGIC&&prev.octave>=-2&&prev.octave<=2){
            defaults();set.octave=prev.octave;set.preset=prev.preset;set.page=prev.page==13u?PG_PRESETS:prev.page;set.custom=prev.custom;set.bank=prev.bank;set.p=prev.p;
        }else{
            n=plat_store_load(OBJ_SETTINGS,&old,sizeof old);
            if(n==(int)sizeof old&&old.magic==OLD_SET_MAGIC&&old.octave>=-2&&old.octave<=2){defaults();set.octave=old.octave;set.preset=old.preset<5u?old.preset:0u;set.page=old.page<PG_SEQ?old.page:PG_OSC1;set.custom=old.custom;set.p=old.p;}
            else defaults();
        }
    }
    if(set.seq.version!=ACID_SEQ_VERSION)acid_seq_defaults(&set.seq);else acid_seq_normalize(&set.seq);
    if(set.octave<-2||set.octave>2||set.page>=PG_COUNT||set.bank>1u||(set.bank?set.preset>=USER_SLOTS:set.preset>=BUILTIN_SLOTS))defaults();
    bank_cache_idx=0xffu;
    (void)user_seed_empty_slots();
    apply_params();dirty=0;seq_dirty=0;import_on=0;import_slot=0;browser_sync();mem_zero(&seqrt,sizeof seqrt);seqrt.current_note=-1;mem_zero(&seq_undo,sizeof seq_undo);ui.seq_panel=0;ui.seq_mode=SQ_GEN;ui.seq_step=0;ui.home=0;
}
static void note_on(int note,int vel){bassstation_note_on(&synth,note,vel);}
static void note_off(int note){bassstation_note_off(&synth,note);}

#define SEQ_SR 44100u
static uint32_t seq_internal_step_samples(void){uint32_t bpm=set.seq.bpm?set.seq.bpm:120u;uint32_t n=(SEQ_SR*15u+bpm/2u)/bpm;return n<32u?32u:n;}
static uint32_t seq_clock_step_samples(void){if(set.seq.clock_mode==ACID_CLOCK_MIDI&&synth.midi_clock_period>1.0f){float n=synth.midi_clock_period*6.0f;if(n>32.0f&&n<((float)SEQ_SR*5.0f))return (uint32_t)(n+0.5f);}return seq_internal_step_samples();}
static uint8_t seq_next(uint8_t p){uint8_t n=set.seq.steps?set.seq.steps:1u;return (uint8_t)((p+1u)>=n?0u:p+1u);}
static uint8_t seq_prev(uint8_t p){uint8_t n=set.seq.steps?set.seq.steps:1u;return p?((uint8_t)(p-1u)):(uint8_t)(n-1u);}
static void seq_release_note(void){if(seqrt.current_note>=0){note_off(seqrt.current_note);seqrt.current_note=-1;}seqrt.gate_pending=0;seqrt.samples_to_gate=0;}
static void seq_begin_step(uint8_t pos,uint32_t step_samples){
    int k=acid_seq_kind(&set.seq,pos),note,vel;uint8_t prev=seq_prev(pos),next=seq_next(pos);int slide_in=0,hold=0;uint32_t gate_samples;float slide_s;
    seqrt.pos=pos;seqrt.step_samples=step_samples;seqrt.redraw_pending=1;seqrt.gate_pending=0;seqrt.samples_to_gate=0;
    if(k==ACID_REST){seq_release_note();return;}
    if(k==ACID_TIE){if(seqrt.current_note<0)seq_release_note();return;}
    note=acid_seq_midi_note(&set.seq,pos);if(note<0){seq_release_note();return;}vel=acid_seq_accent(&set.seq,pos)?127:96;
    slide_in=seqrt.current_note>=0&&acid_seq_kind(&set.seq,prev)==ACID_NOTE&&acid_seq_slide(&set.seq,prev);
    if(slide_in){slide_s=((float)step_samples/(float)SEQ_SR)*((float)set.seq.slide_time/100.0f);bassstation_note_slide(&synth,seqrt.current_note,note,vel,slide_s);}
    else{seq_release_note();note_on(note,vel);}
    seqrt.current_note=(int16_t)note;
    hold=acid_seq_kind(&set.seq,next)==ACID_TIE||(acid_seq_slide(&set.seq,pos)&&acid_seq_kind(&set.seq,next)==ACID_NOTE);
    if(!hold){gate_samples=(uint32_t)((step_samples*(uint32_t)set.seq.gate)/100u);if(gate_samples>=step_samples&&step_samples>1u)gate_samples=step_samples-1u;if(gate_samples<1u)gate_samples=1u;seqrt.gate_pending=1;seqrt.samples_to_gate=gate_samples;}
}
static void seq_transport_reset(int stop_note){if(stop_note)seq_release_note();seqrt.pos=0;seqrt.midi_phase=0;seqrt.samples_to_step=0;seqrt.step_samples=0;seqrt.redraw_pending=1;}
static void seq_set_running(int on){if(on){seqrt.running=1;seq_transport_reset(1);}else{seqrt.running=0;seq_transport_reset(1);}}
static void seq_toggle_run(void){seq_set_running(!seqrt.running);}
static void seq_midi_start_event(void){if(set.seq.clock_mode==ACID_CLOCK_MIDI){seqrt.running=1;seq_transport_reset(1);}}
static void seq_midi_stop_event(void){if(set.seq.clock_mode==ACID_CLOCK_MIDI)seq_set_running(0);}
static void seq_midi_clock_event(void){if(set.seq.clock_mode!=ACID_CLOCK_MIDI||!seqrt.running)return;if(seqrt.midi_phase==0u){if(seqrt.step_samples)seqrt.pos=seq_next(seqrt.pos);seq_begin_step(seqrt.pos,seq_clock_step_samples());}seqrt.midi_phase=(uint8_t)((seqrt.midi_phase+1u)%6u);}
static void seq_mark_changed(void){acid_seq_normalize(&set.seq);seq_dirty=1;ui.redraw=1;bw_send_seq();}
static void seq_generate_action(void){acid_seq_generate(&set.seq,plat_ms()^((uint32_t)set.preset<<16)^((uint32_t)set.bank<<30),&seq_undo);seq_mark_changed();}
static void seq_mut_time(void){acid_seq_mutate_time(&set.seq,&seq_undo);seq_mark_changed();}
static void seq_mut_note(void){acid_seq_mutate_note(&set.seq,&seq_undo);seq_mark_changed();}
static void seq_mut_art(void){acid_seq_mutate_art(&set.seq,&seq_undo);seq_mark_changed();}
static void seq_undo_action(void){if(seq_undo.valid){acid_seq_undo(&set.seq,&seq_undo);seq_mark_changed();}}
static void seq_render(int32_t *lr,uint32_t n,uint32_t master_q12){
    uint32_t done=0;
    while(done<n){uint32_t chunk=n-done;
        if(seqrt.running&&set.seq.clock_mode==ACID_CLOCK_INTERNAL){if(!seqrt.step_samples){seqrt.pos=0;seq_begin_step(0,seq_internal_step_samples());seqrt.samples_to_step=seqrt.step_samples;}else if(!seqrt.samples_to_step){seqrt.pos=seq_next(seqrt.pos);seq_begin_step(seqrt.pos,seq_internal_step_samples());seqrt.samples_to_step=seqrt.step_samples;}if(seqrt.samples_to_step<chunk)chunk=seqrt.samples_to_step;}
        if(seqrt.gate_pending&&seqrt.samples_to_gate<chunk)chunk=seqrt.samples_to_gate;
        if(chunk==0u){if(seqrt.gate_pending&&!seqrt.samples_to_gate)seq_release_note();if(seqrt.running&&set.seq.clock_mode==ACID_CLOCK_INTERNAL&&!seqrt.samples_to_step)continue;chunk=1u;}
        bassstation_render(&synth,lr+2u*done,chunk,master_q12);done+=chunk;
        if(seqrt.running&&set.seq.clock_mode==ACID_CLOCK_INTERNAL&&seqrt.samples_to_step>=chunk)seqrt.samples_to_step-=chunk;
        if(seqrt.gate_pending&&seqrt.samples_to_gate>=chunk){seqrt.samples_to_gate-=chunk;if(!seqrt.samples_to_gate)seq_release_note();}
    }
}

static float cc_attack(uint32_t v){return plug_attack((int)v);}
static float cc_dr(uint32_t v){return plug_dr((int)v);}
static float cc_cutoff(uint32_t v){return plug_cutoff((int)v);}
static float cc_signed(uint32_t v,float depth){return plug_signed((int)v-64,depth);}
static void cc_edit(uint32_t cc,uint32_t v){
    bassstation_params_t *p=&set.p;int changed=1;
    switch(cc){
        case 5:p->glide_s=plug_glide((int)v);break;
        case 7:p->volume=((float)v/127.0f)*1.25f;break;
        case 16:p->lfo_rate_hz=plug_lfo_rate((int)v);break;
        case 17:p->lfo_delay_s=plug_lfo_delay((int)v);break;
        case 18:p->lfo_clock_div=(uint8_t)((v*16u+63u)/127u);break;
        case 20:p->osc1_env_pitch=cc_signed(v,24.0f);break;
        case 21:p->osc1_lfo_pitch=cc_signed(v,12.0f);break;
        case 22:p->osc1_pw=plug_pw((int)v-64);break;
        case 23:p->osc2_semitone=(float)((v*12u+63u)/127u);break;
        case 24:p->osc2_detune=cc_signed(v,0.96f);break;
        case 25:p->osc2_env_pitch=cc_signed(v,24.0f);break;
        case 26:p->osc2_lfo_pitch=cc_signed(v,12.0f);break;
        case 27:p->osc2_pw=plug_pw((int)v-64);break;
        case 28:p->osc_mix=(float)v/127.0f;break;
        case 35:p->filter_lfo_mod=cc_signed(v,8.0f);break;
        case 105:p->cutoff_hz=cc_cutoff(v);break;
        case 106:p->resonance=(float)v/127.0f;break;
        case 107:p->filter_env_mod=cc_signed(v,8.0f);break;
        case 108:p->amp_attack=cc_attack(v);break;
        case 109:p->amp_decay=cc_dr(v);break;
        case 110:p->amp_sustain=(float)v/127.0f;break;
        case 111:p->amp_release=cc_dr(v);break;
        case 112:p->amp_velocity=(float)v/127.0f;break;
        case 114:p->mod_attack=cc_attack(v);break;
        case 115:p->mod_decay=cc_dr(v);break;
        case 116:p->mod_sustain=(float)v/127.0f;break;
        case 117:p->mod_release=cc_dr(v);break;
        case 118:p->mod_velocity=(float)v/127.0f;break;
        default:changed=0;break;
    }
    if(changed){synth.p=*p;set.custom=1;dirty=1;ui.redraw=1;bw_send_synth();}
}
static void midi_in(void){
    uint32_t pkt;int port;while(plat_midi_in(&pkt,&port)){
        uint32_t st=(pkt>>8)&0xffu,d1=(pkt>>16)&0x7fu,d2=(pkt>>24)&0x7fu;(void)port;
        if(st==0xf8u){bassstation_midi_clock(&synth);seq_midi_clock_event();}
        else if(st==0xfau||st==0xfbu){bassstation_midi_start(&synth);seq_midi_start_event();}
        else if(st==0xfcu){bassstation_midi_stop(&synth);seq_midi_stop_event();}
        else if((st&0xf0u)==0x90u&&d2)note_on((int)d1,(int)d2);
        else if((st&0xf0u)==0x80u||((st&0xf0u)==0x90u&&!d2))note_off((int)d1);
        else if((st&0xf0u)==0xe0u)bassstation_pitch_bend(&synth,(int)(d1|(d2<<7)));
        else if((st&0xf0u)==0xd0u)bassstation_aftertouch(&synth,(int)d1);
        else if((st&0xf0u)==0xc0u){if(set.bank){if(d1<USER_SLOTS)preset_select((int)d1);}else if(d1<FACTORY_SLOTS)builtin_load((int)d1+1);}
        else if((st&0xf0u)==0xb0u&&d1==0u)bank_select(d2?1u:0u);
        else if((st&0xf0u)==0xb0u&&d1==1u)bassstation_modwheel(&synth,(int)d2);
        else if((st&0xf0u)==0xb0u&&d1==2u)bassstation_breath(&synth,(int)d2);
        else if((st&0xf0u)==0xb0u&&d1==64u)bassstation_sustain(&synth,d2>=64u);
        else if((st&0xf0u)==0xb0u&&(d1==120u||d1==123u))bassstation_all_notes_off(&synth);
        else if((st&0xf0u)==0xb0u)cc_edit(d1,d2);
    }
}
void app_render(int32_t *lr,uint32_t n,uint32_t master_q12){midi_in();seq_render(lr,n,master_q12);}
void app_panic(void){seqrt.running=0;seq_transport_reset(0);seqrt.current_note=-1;bassstation_all_notes_off(&synth);synth.amp_env=synth.mod_env=0.0f;synth.amp_stage=synth.mod_stage=0;}


/* TEST20 local USB-C browser editor.  Like the V-Station editor, the webpage is
   static and talks to the FM1 through class-compliant USB-MIDI Web MIDI SysEx;
   there is no HTTP server in the firmware.  Namespace: F0 7D 'B' 'W' 01 ... F7. */
#define BW_PARAM_COUNT 48u
enum { BW_PROTO=1, BW_HELLO=1, BW_GET_SYNTH=2, BW_SET_SYNTH=3, BW_LOAD_PRESET=4,
       BW_PANIC=5, BW_GET_SEQ=6, BW_SET_SEQ_CTRL=7, BW_SET_SEQ_STEP=8,
       BW_SEQ_ACTION=9, BW_SAVE_USER=10, BW_SAVE_SETTINGS=11, BW_R_HELLO=0x41, BW_R_SYNTH=0x42,
       BW_R_ACK=0x45, BW_R_SEQ=0x46 };
static uint8_t bw_tx[320];
static unsigned bw_u14(const uint8_t *p){return (unsigned)p[0]|((unsigned)p[1]<<7);}
static unsigned bw_put_u14(uint8_t *p,unsigned v){if(v>16383u)v=16383u;p[0]=(uint8_t)(v&127u);p[1]=(uint8_t)((v>>7)&127u);return 2u;}
static unsigned bw_begin(unsigned cmd){bw_tx[0]=0xF0;bw_tx[1]=0x7D;bw_tx[2]='B';bw_tx[3]='W';bw_tx[4]=BW_PROTO;bw_tx[5]=(uint8_t)cmd;return 6u;}
static void bw_finish(unsigned n){bw_tx[n++]=0xF7;plat_sysex_send(bw_tx,n);}
static void bw_ack(unsigned cmd,unsigned status){unsigned n=bw_begin(BW_R_ACK);bw_tx[n++]=(uint8_t)cmd;bw_tx[n++]=(uint8_t)status;bw_finish(n);}
static unsigned bw_lin(float v,float lo,float hi){float q;if(v<lo)v=lo;if(v>hi)v=hi;q=(v-lo)/(hi-lo);return (unsigned)(q*1000.0f+0.5f);}
static float bw_unlin(unsigned v,float lo,float hi){if(v>1000u)v=1000u;return lo+(hi-lo)*(float)v/1000.0f;}
static unsigned bw_log(float v,float lo,float hi){float q;if(v<lo)v=lo;if(v>hi)v=hi;q=fm_log2f(v/lo)/fm_log2f(hi/lo);return (unsigned)(q*1000.0f+0.5f);}
static float bw_unlog(unsigned v,float lo,float hi){if(v>1000u)v=1000u;return lo*fm_exp2f(((float)v/1000.0f)*fm_log2f(hi/lo));}
static unsigned bw_log0(float v,float lo,float hi){float q;if(v<=0.0f)return 0u;if(v<lo)v=lo;if(v>hi)v=hi;q=fm_log2f(v/lo)/fm_log2f(hi/lo);return 1u+(unsigned)(q*999.0f+0.5f);}
static float bw_unlog0(unsigned v,float lo,float hi){if(!v)return 0.0f;if(v>1000u)v=1000u;return lo*fm_exp2f(((float)(v-1u)/999.0f)*fm_log2f(hi/lo));}
static unsigned bw_param_get(unsigned id){bassstation_params_t *p=&set.p;switch(id){
 case 0:return p->osc1_wave;case 1:return p->osc2_wave;case 2:return p->osc2_range;case 3:return p->osc_sync;case 4:return p->osc1_pwm_source;case 5:return p->osc2_pwm_source;case 6:return p->filter_24db;case 7:return p->lfo_shape;case 8:return p->trigger_mode;case 9:return p->lfo_clock_div;
 case 10:return bw_lin(p->osc1_pw,0,1);case 11:return bw_lin(p->osc2_pw,0,1);case 12:return bw_lin(p->osc2_semitone,0,12);case 13:return bw_lin(p->osc2_detune,-0.96f,0.96f);case 14:return bw_lin(p->osc1_lfo_pitch,-12,12);case 15:return bw_lin(p->osc2_lfo_pitch,-12,12);case 16:return bw_lin(p->osc1_env_pitch,-24,24);case 17:return bw_lin(p->osc2_env_pitch,-24,24);case 18:return bw_lin(p->osc_mix,0,1);
 case 19:return bw_log(p->cutoff_hz,5,10000);case 20:return bw_lin(p->resonance,0,1);case 21:return bw_lin(p->filter_env_mod,-8,8);case 22:return bw_lin(p->filter_lfo_mod,-8,8);case 23:return bw_lin(p->filter_keytrack,0,1);
 case 24:return bw_log0(p->amp_attack,0.0005f,20);case 25:return bw_log0(p->amp_decay,0.003f,20);case 26:return bw_lin(p->amp_sustain,0,1);case 27:return bw_log0(p->amp_release,0.003f,20);case 28:return bw_lin(p->amp_velocity,0,1);
 case 29:return bw_log0(p->mod_attack,0.0005f,20);case 30:return bw_log0(p->mod_decay,0.003f,20);case 31:return bw_lin(p->mod_sustain,0,1);case 32:return bw_log0(p->mod_release,0.003f,20);case 33:return bw_lin(p->mod_velocity,0,1);
 case 34:return bw_log0(p->lfo_rate_hz,0.0055f,1000);case 35:return bw_log0(p->lfo_delay_s,0.005f,5);case 36:return bw_log0(p->glide_s,0.005f,5);case 37:return bw_lin(p->master_tune,-0.5f,0.5f);case 38:return bw_lin(p->bend_range,0,12);case 39:return bw_lin(p->wheel_pitch,0,15);case 40:return bw_lin(p->wheel_filter,0,6);
 case 41:return bw_lin(p->aftertouch_pitch,-7,7);case 42:return bw_lin(p->aftertouch_filter,-7,7);case 43:return bw_lin(p->aftertouch_bias,0,15);case 44:return bw_lin(p->breath_pitch,-7,7);case 45:return bw_lin(p->breath_filter,-7,7);case 46:return bw_lin(p->breath_bias,0,15);case 47:return bw_lin(p->volume,0,1.25f);default:return 0u;}}
static int bw_param_set(unsigned id,unsigned v){bassstation_params_t *p=&set.p;switch(id){
 case 0:p->osc1_wave=(uint8_t)(v>1?1:v);break;case 1:p->osc2_wave=(uint8_t)(v>1?1:v);break;case 2:p->osc2_range=(uint8_t)(v>3?3:v);break;case 3:p->osc_sync=(uint8_t)(v?1:0);break;case 4:p->osc1_pwm_source=(uint8_t)(v>2?2:v);break;case 5:p->osc2_pwm_source=(uint8_t)(v>2?2:v);break;case 6:p->filter_24db=(uint8_t)(v?1:0);break;case 7:p->lfo_shape=(uint8_t)(v>2?2:v);break;case 8:p->trigger_mode=(uint8_t)(v>2?2:v);break;case 9:p->lfo_clock_div=(uint8_t)(v>16?16:v);break;
 case 10:p->osc1_pw=bw_unlin(v,0,1);break;case 11:p->osc2_pw=bw_unlin(v,0,1);break;case 12:p->osc2_semitone=bw_unlin(v,0,12);break;case 13:p->osc2_detune=bw_unlin(v,-0.96f,0.96f);break;case 14:p->osc1_lfo_pitch=bw_unlin(v,-12,12);break;case 15:p->osc2_lfo_pitch=bw_unlin(v,-12,12);break;case 16:p->osc1_env_pitch=bw_unlin(v,-24,24);break;case 17:p->osc2_env_pitch=bw_unlin(v,-24,24);break;case 18:p->osc_mix=bw_unlin(v,0,1);break;
 case 19:p->cutoff_hz=bw_unlog(v,5,10000);break;case 20:p->resonance=bw_unlin(v,0,1);break;case 21:p->filter_env_mod=bw_unlin(v,-8,8);break;case 22:p->filter_lfo_mod=bw_unlin(v,-8,8);break;case 23:p->filter_keytrack=bw_unlin(v,0,1);break;
 case 24:p->amp_attack=bw_unlog0(v,0.0005f,20);break;case 25:p->amp_decay=bw_unlog0(v,0.003f,20);break;case 26:p->amp_sustain=bw_unlin(v,0,1);break;case 27:p->amp_release=bw_unlog0(v,0.003f,20);break;case 28:p->amp_velocity=bw_unlin(v,0,1);break;
 case 29:p->mod_attack=bw_unlog0(v,0.0005f,20);break;case 30:p->mod_decay=bw_unlog0(v,0.003f,20);break;case 31:p->mod_sustain=bw_unlin(v,0,1);break;case 32:p->mod_release=bw_unlog0(v,0.003f,20);break;case 33:p->mod_velocity=bw_unlin(v,0,1);break;
 case 34:p->lfo_rate_hz=bw_unlog0(v,0.0055f,1000);break;case 35:p->lfo_delay_s=bw_unlog0(v,0.005f,5);break;case 36:p->glide_s=bw_unlog0(v,0.005f,5);break;case 37:p->master_tune=bw_unlin(v,-0.5f,0.5f);break;case 38:p->bend_range=bw_unlin(v,0,12);break;case 39:p->wheel_pitch=bw_unlin(v,0,15);break;case 40:p->wheel_filter=bw_unlin(v,0,6);break;
 case 41:p->aftertouch_pitch=bw_unlin(v,-7,7);break;case 42:p->aftertouch_filter=bw_unlin(v,-7,7);break;case 43:p->aftertouch_bias=bw_unlin(v,0,15);break;case 44:p->breath_pitch=bw_unlin(v,-7,7);break;case 45:p->breath_filter=bw_unlin(v,-7,7);break;case 46:p->breath_bias=bw_unlin(v,0,15);break;case 47:p->volume=bw_unlin(v,0,1.25f);break;default:return 0;}
 synth.p=*p;set.custom=1;dirty=1;ui.redraw=1;return 1;}
static void bw_send_hello(void){unsigned n=bw_begin(BW_R_HELLO);bw_tx[n++]=20;n+=bw_put_u14(bw_tx+n,BW_PARAM_COUNT);n+=bw_put_u14(bw_tx+n,BUILTIN_SLOTS);n+=bw_put_u14(bw_tx+n,USER_SLOTS);bw_tx[n++]=0x1f;bw_finish(n);}
static void bw_send_synth(void){unsigned id,n;if(!bw_connected)return;n=bw_begin(BW_R_SYNTH);bw_tx[n++]=set.bank;n+=bw_put_u14(bw_tx+n,set.preset);bw_tx[n++]=set.custom?1u:0u;bw_tx[n++]=(uint8_t)(set.octave+2);for(id=0;id<BW_PARAM_COUNT;id++)n+=bw_put_u14(bw_tx+n,bw_param_get(id));bw_finish(n);}
static void bw_send_seq(void){unsigned n,i;if(!bw_connected)return;n=bw_begin(BW_R_SEQ);n+=bw_put_u14(bw_tx+n,set.seq.bpm);bw_tx[n++]=set.seq.steps;bw_tx[n++]=set.seq.note_density;bw_tx[n++]=set.seq.range;bw_tx[n++]=set.seq.root;bw_tx[n++]=set.seq.scale;bw_tx[n++]=set.seq.gate;bw_tx[n++]=set.seq.accent_density;bw_tx[n++]=set.seq.slide_density;bw_tx[n++]=set.seq.base_octave;bw_tx[n++]=set.seq.slide_time;bw_tx[n++]=set.seq.clock_mode;bw_tx[n++]=set.seq.block;bw_tx[n++]=set.seq.mut_time_ops;bw_tx[n++]=set.seq.mut_note_ops;bw_tx[n++]=set.seq.mut_art_ops;bw_tx[n++]=seqrt.running;bw_tx[n++]=seqrt.pos;for(i=0;i<ACID_SEQ_NSTEP;i++){uint8_t c=set.seq.step[i].code;bw_tx[n++]=(uint8_t)((c&0x7fu));bw_tx[n++]=(uint8_t)(((uint8_t)(set.seq.step[i].octave+2)&7u)|((c&0x80u)?8u:0u));}bw_finish(n);}
static int bw_seq_ctrl_set(unsigned id,unsigned v){uint8_t old=set.seq.clock_mode;switch(id){case 0:set.seq.bpm=(uint16_t)stepi((int)v,0,30,300);break;case 1:set.seq.steps=(uint8_t)stepi((int)v,0,1,64);break;case 2:set.seq.note_density=(uint8_t)stepi((int)v,0,5,100);break;case 3:set.seq.range=(uint8_t)stepi((int)v,0,0,100);break;case 4:set.seq.root=(uint8_t)(v%12u);break;case 5:set.seq.scale=(uint8_t)(v%8u);break;case 6:set.seq.gate=(uint8_t)stepi((int)v,0,5,100);break;case 7:set.seq.accent_density=(uint8_t)stepi((int)v,0,0,100);break;case 8:set.seq.slide_density=(uint8_t)stepi((int)v,0,0,100);break;case 9:set.seq.base_octave=(uint8_t)stepi((int)v,0,1,5);break;case 10:set.seq.slide_time=(uint8_t)stepi((int)v,0,5,100);break;case 11:set.seq.clock_mode=(uint8_t)(v?1:0);break;case 12:set.seq.block=(uint8_t)(v?1:0);break;case 13:set.seq.mut_time_ops=(uint8_t)stepi((int)v,0,1,8);break;case 14:set.seq.mut_note_ops=(uint8_t)stepi((int)v,0,1,8);break;case 15:set.seq.mut_art_ops=(uint8_t)stepi((int)v,0,1,8);break;default:return 0;}if(old!=set.seq.clock_mode)seq_set_running(0);seq_mark_changed();return 1;}
static int bw_handle(const uint8_t *p,uint32_t n){unsigned cmd;if(n<5u||p[0]!=0x7du||p[1]!='B'||p[2]!='W'||p[3]!=BW_PROTO)return 0;cmd=p[4];bw_connected=1;
 if(cmd==BW_HELLO){bw_send_hello();bw_send_synth();bw_send_seq();return 1;}
 if(cmd==BW_GET_SYNTH){bw_send_synth();return 1;}
 if(cmd==BW_SET_SYNTH){if(n<9u){bw_ack(cmd,2);return 1;}if(!bw_param_set(bw_u14(p+5),bw_u14(p+7))){bw_ack(cmd,3);return 1;}bw_send_synth();return 1;}
 if(cmd==BW_LOAD_PRESET){unsigned bank,slot;if(n<8u){bw_ack(cmd,2);return 1;}bank=p[5]?1u:0u;slot=bw_u14(p+6);if((!bank&&slot>=BUILTIN_SLOTS)||(bank&&slot>=USER_SLOTS)){bw_ack(cmd,3);return 1;}if(bank){set.bank=1;user_load((uint8_t)slot);}else builtin_load((int)slot);return 1;}
 if(cmd==BW_PANIC){app_panic();bw_ack(cmd,0);return 1;}
 if(cmd==BW_GET_SEQ){bw_send_seq();return 1;}
 if(cmd==BW_SET_SEQ_CTRL){if(n<8u){bw_ack(cmd,2);return 1;}if(!bw_seq_ctrl_set(p[5],bw_u14(p+6))){bw_ack(cmd,3);return 1;}return 1;}
 if(cmd==BW_SET_SEQ_STEP){uint8_t st,attr,meta,kind,degree,accent,slide;int oct;if(n<8u){bw_ack(cmd,2);return 1;}st=p[5];if(st>=ACID_SEQ_NSTEP){bw_ack(cmd,3);return 1;}attr=p[6];meta=p[7];kind=attr&3u;degree=(attr>>2)&15u;accent=(attr>>6)&1u;slide=(meta>>3)&1u;oct=(int)(meta&7u)-2;if(kind>ACID_TIE)kind=ACID_REST;set.seq.step[st].code=(uint8_t)((kind&3u)|((degree&15u)<<2)|(accent?0x40u:0u)|(slide?0x80u:0u));set.seq.step[st].octave=(int8_t)oct;seq_mark_changed();return 1;}
 if(cmd==BW_SEQ_ACTION){if(n<6u){bw_ack(cmd,2);return 1;}switch(p[5]){case 0:seq_generate_action();break;case 1:seq_mut_time();break;case 2:seq_mut_note();break;case 3:seq_mut_art();break;case 4:seq_undo_action();break;case 5:acid_seq_clear(&set.seq,&seq_undo);seq_mark_changed();break;case 6:seq_toggle_run();ui.redraw=1;bw_send_seq();break;case 7:seq_set_running(1);ui.redraw=1;bw_send_seq();break;case 8:seq_set_running(0);ui.redraw=1;bw_send_seq();break;default:bw_ack(cmd,3);return 1;}return 1;}
 if(cmd==BW_SAVE_USER){unsigned slot;if(n<7u){bw_ack(cmd,2);return 1;}slot=bw_u14(p+5);if(slot>=USER_SLOTS){bw_ack(cmd,3);return 1;}if(!user_store((uint8_t)slot,&set.p,0,0)){bw_ack(cmd,4);return 1;}set.bank=1;set.preset=(uint8_t)slot;set.custom=0;dirty=0;bw_send_synth();bw_ack(cmd,0);return 1;}
 if(cmd==BW_SAVE_SETTINGS){int ok=1;set.p=synth.p;if(set.bank)ok=user_store(set.preset,&set.p,0,0);if(ok&&plat_store_save(OBJ_SETTINGS,&set,sizeof set)==0){dirty=0;seq_dirty=0;set.custom=0;bw_send_synth();bw_send_seq();bw_ack(cmd,0);}else bw_ack(cmd,4);return 1;}
 bw_ack(cmd,5);return 1;}

int app_sysex(const uint8_t *p,uint32_t n){
    uint8_t frame[76];
    if(!p)return 0;
    if(bw_handle(p,n))return 1;
    /* Novation Bass Station Rack single-program frame, bytes between F0/F7. */
    if(n==74u&&p[0]==0x00u&&p[1]==0x20u&&p[2]==0x29u&&p[3]==0x01u&&p[4]==0x03u&&p[5]==0x22u){
        bassstation_params_t q=set.p;
        if(!bassstation_rack_sysex_decode(&q,p,n))return 0;
        bassstation_all_notes_off(&synth);set.p=q;synth.p=q;ui.redraw=1;
        if(import_on){uint8_t slot=import_slot;if(!user_store(slot,&q,0,1)){app_say("BANK IMPORT","SAVE FAILED");return 1;}set.bank=1;set.preset=slot;set.custom=0;dirty=0;if(import_slot<USER_SLOTS-1u)import_slot++;else import_on=0;app_say("BANK IMPORT",import_on?"PATCH SAVED":"128 PATCHES DONE");}
        else{set.custom=1;dirty=1;app_say("RACK PATCH","LOADED");}
        bw_send_synth();return 1;
    }
    /* TEST4 compatibility: F0 7D 'B' 'S' 04 01 F7 -> current patch as Rack frame. */
    if(n==5u&&p[0]==0x7Du&&p[1]=='B'&&p[2]=='S'&&p[3]==4u&&p[4]==1u){if(bassstation_rack_sysex_encode(&set.p,frame)==76u)plat_sysex_send(frame,76u);return 1;}
    /* TEST9 user-bank control (retained from TEST7), slot numbers are 0..127:
       20 slot  start sequential Rack capture; 21 stop; 22 export; 23 load;
       24 save current; 25 name slot (ASCII follows); 26 clear slot. */
    if(n>=5u&&p[0]==0x7Du&&p[1]=='B'&&p[2]=='S'&&p[3]==7u){uint8_t op=p[4],slot=n>=6u?p[5]:0u;if(op==0x20u&&n==6u&&slot<USER_SLOTS){import_on=1;import_slot=slot;app_say("BANK IMPORT","CAPTURE ON");return 1;}if(op==0x21u&&n==5u){import_on=0;app_say("BANK IMPORT","CAPTURE OFF");return 1;}if(slot>=USER_SLOTS)return 1;if(op==0x22u&&n==6u){uint8_t chunk=(uint8_t)(slot/BANK_CHUNK_SLOTS),i=(uint8_t)(slot%BANK_CHUNK_SLOTS);if(user_valid(slot)&&bank_chunk_load(chunk)&&bassstation_rack_sysex_encode(&bank_cache.patch[i],frame)==76u)plat_sysex_send(frame,76u);return 1;}if(op==0x23u&&n==6u){user_load(slot);app_say("USER PATCH",user_valid(slot)?"LOADED":"EMPTY");return 1;}if(op==0x24u&&n==6u){if(user_store(slot,&set.p,0,0)){set.bank=1;set.preset=slot;set.custom=0;dirty=0;app_say("USER PATCH","SAVED");bw_send_synth();}else app_say("USER PATCH","SAVE FAILED");return 1;}if(op==0x25u&&n>=7u){if(user_set_name(slot,p+6u,n-6u))app_say("PATCH NAME","SAVED");return 1;}if(op==0x26u&&n==6u){if(user_clear(slot)){if(set.bank&&set.preset==slot)user_load(slot);app_say("USER PATCH","CLEARED");}return 1;}}
    /* TEST9 native plug-in-record import. The 34-byte state is nibble-expanded so every SysEx data byte is 7-bit clean:
       F0 7D 'B' 'S' 08 30 slot [68 nibbles] F7. This preserves software envelope/LFO ranges. */
    if(n==74u&&p[0]==0x7Du&&p[1]=='B'&&p[2]=='S'&&p[3]==8u&&p[4]==0x30u&&p[5]<USER_SLOTS){
        uint8_t rec[34],i,slot=p[5];bassstation_t t;
        for(i=0;i<34u;i++){if(p[6u+i*2u]>15u||p[7u+i*2u]>15u)return 1;rec[i]=(uint8_t)((p[6u+i*2u]<<4)|p[7u+i*2u]);}
        bassstation_defaults(&t);if(!bassstation_plugin_factory_decode(&t.p,rec))return 1;
        if(user_store(slot,&t.p,0,1)){bassstation_all_notes_off(&synth);set.bank=1;set.preset=slot;set.p=t.p;synth.p=t.p;set.custom=0;dirty=0;app_say("PLUGIN PATCH","IMPORTED");}
        else{app_say("PLUGIN PATCH","SAVE FAILED");}
        bw_send_synth();return 1;
    }
    return 0;
}

static float stepf(float v,float d,float lo,float hi){v+=d;return v<lo?lo:(v>hi?hi:v);}
static float stepmul(float v,int d,float lo,float hi,float div){if(v<lo)v=lo;v*=fm_exp2f((float)d/div);return v<lo?lo:(v>hi?hi:v);}
static int stepi(int v,int d,int lo,int hi){v+=d;return v<lo?lo:(v>hi?hi:v);}
static void octave_step(int d){
    /* TEST20: while the 303 transport is running, the hardware OCT-/OCT+
       buttons transpose the sequencer itself.  The sequencer uses its own
       base octave, so changing only set.octave (the touch keyboard octave)
       was inaudible until playback stopped.  Do not restart transport or
       regenerate the pattern; the new octave is picked up on the next note. */
    if(seqrt.running){
        int o=stepi((int)set.seq.base_octave,d,1,5);
        if(o!=(int)set.seq.base_octave){set.seq.base_octave=(uint8_t)o;seq_mark_changed();}
        return;
    }
    {int o=set.octave+d;if(o<-2)o=-2;if(o>2)o=2;if(o!=set.octave){set.octave=(int8_t)o;bassstation_all_notes_off(&synth);dirty=1;ui.redraw=1;bw_send_synth();}}
}
static uint8_t seq_step_move(uint8_t p,int d){int n=set.seq.steps?set.seq.steps:1u;int q=(int)p+d;while(q<0)q+=n;while(q>=n)q-=n;return (uint8_t)q;}
static uint8_t seq_page_move(uint8_t p,int d){int n=set.seq.steps?set.seq.steps:1u,pages=(n+15)/16,page=(int)p/16,col=(int)p&15,q,base,count;if(pages<1)pages=1;page=(page+d)%pages;if(page<0)page+=pages;base=page*16;count=n-base;if(count>16)count=16;if(count<1)count=1;if(col>=count)col=count-1;q=base+col;return (uint8_t)q;}
static int seq_pitch_semitone(uint8_t st){int i,n=set.seq.steps?set.seq.steps:1u,k;if(st>=n)st=0;k=acid_seq_kind(&set.seq,st);if(k==ACID_NOTE)return acid_seq_degree_semitone(&set.seq,acid_seq_degree(&set.seq,st));if(k!=ACID_TIE)return -1;for(i=1;i<=n;i++){int q=(int)st-i;while(q<0)q+=n;if(acid_seq_kind(&set.seq,(uint8_t)q)==ACID_NOTE)return acid_seq_degree_semitone(&set.seq,acid_seq_degree(&set.seq,(uint8_t)q));if(acid_seq_kind(&set.seq,(uint8_t)q)==ACID_REST)break;}return -1;}
static int seq_art_state(uint8_t p){return (acid_seq_accent(&set.seq,p)?1:0)|(acid_seq_slide(&set.seq,p)?2:0);}

static void knob(int k,int d){
    bassstation_params_t *p=&set.p;int page=set.page;
    if(ui.home){
        /* Four large performance controls on HOME; no hidden page edits. */
        if(k==0)p->cutoff_hz=stepmul(p->cutoff_hz,d,5,10000,16.0f);
        else if(k==1)p->resonance=stepf(p->resonance,d*0.02f,0,1);
        else if(k==2)p->osc_mix=stepf(p->osc_mix,d*0.02f,0,1);
        else if(k==3)p->volume=stepf(p->volume,d*0.02f,0,1.25f);
        synth.p=*p;set.custom=1;dirty=1;ui.redraw=1;bw_send_synth();return;
    }
    if(page==PG_SEQ){
        uint8_t st=ui.seq_step;int a;
        if(ui.seq_mode==SQ_GEN){
            if(ui.seq_panel==0u){
                if(k==0)set.seq.bpm=(uint16_t)stepi((int)set.seq.bpm,d,30,300);
                else if(k==1)set.seq.steps=(uint8_t)stepi((int)set.seq.steps,d,1,64);
                else if(k==2)set.seq.note_density=(uint8_t)stepi((int)set.seq.note_density,d*2,5,100);
                else if(k==3)set.seq.range=(uint8_t)stepi((int)set.seq.range,d*2,0,100);
            }else if(ui.seq_panel==1u){
                if(k==0){int v=((int)set.seq.root+d)%12;if(v<0)v+=12;set.seq.root=(uint8_t)v;}
                else if(k==1){int v=((int)set.seq.scale+d)%8;if(v<0)v+=8;set.seq.scale=(uint8_t)v;}
                else if(k==2)set.seq.gate=(uint8_t)stepi((int)set.seq.gate,d*2,5,100);
                else if(k==3)set.seq.accent_density=(uint8_t)stepi((int)set.seq.accent_density,d*2,0,100);
            }else{
                if(k==0)set.seq.slide_density=(uint8_t)stepi((int)set.seq.slide_density,d*2,0,100);
                else if(k==1)set.seq.base_octave=(uint8_t)stepi((int)set.seq.base_octave,d,1,5);
                else if(k==2)set.seq.slide_time=(uint8_t)stepi((int)set.seq.slide_time,d*2,5,100);
                else if(k==3){uint8_t old=set.seq.clock_mode;set.seq.clock_mode=(uint8_t)stepi((int)set.seq.clock_mode,d,0,1);if(old!=set.seq.clock_mode)seq_set_running(0);}
            }
        }else if(ui.seq_mode==SQ_MUTATE){
            /* Generative settings only: turning these knobs does not alter the
               current pattern.  ENV/LFO/GLO apply one mutation and REC applies
               all three when BLOCK is enabled. */
            if(k==0)set.seq.mut_time_ops=(uint8_t)stepi((int)set.seq.mut_time_ops,d,1,8);
            else if(k==1)set.seq.mut_note_ops=(uint8_t)stepi((int)set.seq.mut_note_ops,d,1,8);
            else if(k==2)set.seq.mut_art_ops=(uint8_t)stepi((int)set.seq.mut_art_ops,d,1,8);
            else if(k==3)set.seq.block=(uint8_t)stepi((int)set.seq.block,d,0,1);
        }else if(ui.seq_mode==SQ_PIANO){
            if(k==0)acid_seq_set_degree(&set.seq,st,acid_seq_degree(&set.seq,st)+d);
            else if(k==1)acid_seq_set_octave(&set.seq,st,(int)set.seq.step[st].octave+d);
            else if(k==2)acid_seq_set_kind(&set.seq,st,stepi(acid_seq_kind(&set.seq,st),d,0,2));
            else if(k==3){a=seq_art_state(st);a=(a+d)%4;if(a<0)a+=4;acid_seq_set_art(&set.seq,st,a&1,a&2);}
        }else if(ui.seq_mode==SQ_GATE){
            if(k==0)acid_seq_set_kind(&set.seq,st,stepi(acid_seq_kind(&set.seq,st),d,0,2));
            else if(k==1)set.seq.gate=(uint8_t)stepi((int)set.seq.gate,d*2,5,100);
            else if(k==2)set.seq.note_density=(uint8_t)stepi((int)set.seq.note_density,d*2,5,100);
            else if(k==3)set.seq.steps=(uint8_t)stepi((int)set.seq.steps,d,1,64);
        }else if(ui.seq_mode==SQ_OCT){
            if(k==0)acid_seq_set_octave(&set.seq,st,(int)set.seq.step[st].octave+d);
            else if(k==1)set.seq.base_octave=(uint8_t)stepi((int)set.seq.base_octave,d,1,5);
            else if(k==2)set.seq.range=(uint8_t)stepi((int)set.seq.range,d*2,0,100);
            else if(k==3){int v=((int)set.seq.root+d)%12;if(v<0)v+=12;set.seq.root=(uint8_t)v;}
        }else if(ui.seq_mode==SQ_ACC){
            if(k==0){int on=acid_seq_accent(&set.seq,st);if(d)on=!on;acid_seq_set_art(&set.seq,st,on,acid_seq_slide(&set.seq,st));}
            else if(k==1)set.seq.accent_density=(uint8_t)stepi((int)set.seq.accent_density,d*2,0,100);
            else if(k==2)set.seq.mut_art_ops=(uint8_t)stepi((int)set.seq.mut_art_ops,d,1,8);
            else if(k==3)set.seq.block=(uint8_t)stepi((int)set.seq.block,d,0,1);
        }else if(ui.seq_mode==SQ_SLIDE){
            if(k==0){int on=acid_seq_slide(&set.seq,st);if(d)on=!on;acid_seq_set_art(&set.seq,st,acid_seq_accent(&set.seq,st),on);}
            else if(k==1)set.seq.slide_density=(uint8_t)stepi((int)set.seq.slide_density,d*2,0,100);
            else if(k==2)set.seq.slide_time=(uint8_t)stepi((int)set.seq.slide_time,d*2,5,100);
            else if(k==3)set.seq.gate=(uint8_t)stepi((int)set.seq.gate,d*2,5,100);
        }
        if(ui.seq_step>=set.seq.steps)ui.seq_step=(uint8_t)(set.seq.steps-1u);
        seq_mark_changed();return;
    }
    if(page==PG_OSC1){if(k==0)p->osc1_wave=(uint8_t)stepi(p->osc1_wave,d,0,1);if(k==1)p->osc1_pw=stepf(p->osc1_pw,d*0.02f,0,1);if(k==2)p->osc1_pwm_source=(uint8_t)stepi(p->osc1_pwm_source,d,0,2);if(k==3)p->osc1_lfo_pitch=stepf(p->osc1_lfo_pitch,d*0.1f,-12,12);}
    else if(page==PG_OSC2){if(k==0)p->osc2_range=(uint8_t)stepi(p->osc2_range,d,0,3);if(k==1)p->osc2_wave=(uint8_t)stepi(p->osc2_wave,d,0,1);if(k==2)p->osc2_pw=stepf(p->osc2_pw,d*0.02f,0,1);if(k==3)p->osc2_semitone=stepf(p->osc2_semitone,d,0,12);}
    else if(page==PG_OSCMOD){if(k==0)p->osc1_env_pitch=stepf(p->osc1_env_pitch,d*0.2f,-24,24);if(k==1)p->osc2_env_pitch=stepf(p->osc2_env_pitch,d*0.2f,-24,24);if(k==2)p->osc2_lfo_pitch=stepf(p->osc2_lfo_pitch,d*0.1f,-12,12);if(k==3)p->osc2_detune=stepf(p->osc2_detune,d*0.01f,-0.96f,0.96f);}
    else if(page==PG_MIX){if(k==0)p->osc_mix=stepf(p->osc_mix,d*0.02f,0,1);if(k==1)p->osc_sync=(uint8_t)stepi(p->osc_sync,d,0,1);if(k==2)p->volume=stepf(p->volume,d*0.02f,0,1.25f);}
    else if(page==PG_FILTER){if(k==0)p->cutoff_hz=stepmul(p->cutoff_hz,d,5,10000,16.0f);if(k==1)p->resonance=stepf(p->resonance,d*0.02f,0,1);if(k==2)p->filter_env_mod=stepf(p->filter_env_mod,d*0.10f,-8,8);if(k==3)p->filter_lfo_mod=stepf(p->filter_lfo_mod,d*0.10f,-8,8);}
    else if(page==PG_AMPENV){if(k==0)p->amp_attack=stepmul(p->amp_attack,d,0.0005f,20,8.0f);if(k==1)p->amp_decay=stepmul(p->amp_decay,d,0.003f,20,8.0f);if(k==2)p->amp_sustain=stepf(p->amp_sustain,d*0.02f,0,1);if(k==3)p->amp_release=stepmul(p->amp_release,d,0.003f,20,8.0f);}
    else if(page==PG_MODENV){if(k==0)p->mod_attack=stepmul(p->mod_attack,d,0.0005f,20,8.0f);if(k==1)p->mod_decay=stepmul(p->mod_decay,d,0.003f,20,8.0f);if(k==2)p->mod_sustain=stepf(p->mod_sustain,d*0.02f,0,1);if(k==3)p->mod_release=stepmul(p->mod_release,d,0.003f,20,8.0f);}
    else if(page==PG_TRIGGER){if(k==0)p->trigger_mode=(uint8_t)stepi(p->trigger_mode,d,0,2);if(k==1){if(p->glide_s<0.001f&&d>0)p->glide_s=0.005f;else p->glide_s=stepmul(p->glide_s,d,0.005f,5,8.0f);if(d<0&&p->glide_s<=0.0051f)p->glide_s=0.0f;}if(k==2)p->amp_velocity=stepf(p->amp_velocity,d*0.02f,0,1);if(k==3)p->mod_velocity=stepf(p->mod_velocity,d*0.02f,0,1);}
    else if(page==PG_LFO){if(k==0){if(p->lfo_rate_hz<0.006f&&d>0)p->lfo_rate_hz=0.01f;else p->lfo_rate_hz=stepmul(p->lfo_rate_hz,d,0.0055f,1000.0f,18.0f);if(d<0&&p->lfo_rate_hz<=0.0056f)p->lfo_rate_hz=0.0f;}if(k==1)p->lfo_shape=(uint8_t)stepi(p->lfo_shape,d,0,2);if(k==2){if(p->lfo_delay_s<0.001f&&d>0)p->lfo_delay_s=0.005f;else p->lfo_delay_s=stepmul(p->lfo_delay_s,d,0.005f,5,8.0f);if(d<0&&p->lfo_delay_s<=0.0051f)p->lfo_delay_s=0.0f;}if(k==3)p->lfo_clock_div=(uint8_t)stepi(p->lfo_clock_div,d,0,16);}
    else if(page==PG_GLOBAL){if(k==0)p->master_tune=stepf(p->master_tune,d*0.01f,-0.5f,0.5f);if(k==1)p->bend_range=stepf(p->bend_range,d,0,12);if(k==2)p->wheel_pitch=stepf(p->wheel_pitch,d*0.1f,0,15);if(k==3)p->wheel_filter=stepf(p->wheel_filter,d*0.10f,0,6);}
    else if(page==PG_AFTERTOUCH){if(k==0)p->aftertouch_pitch=stepf(p->aftertouch_pitch,d,0,7);if(k==1)p->aftertouch_filter=stepf(p->aftertouch_filter,d,0,7);if(k==2)p->aftertouch_bias=stepf(p->aftertouch_bias,d,0,15);}
    else if(page==PG_BREATH){if(k==0)p->breath_pitch=stepf(p->breath_pitch,d,0,7);if(k==1)p->breath_filter=stepf(p->breath_filter,d,0,7);if(k==2)p->breath_bias=stepf(p->breath_bias,d,0,15);}
    else if(page==PG_PERF){
#ifndef PLAT_HOST
        if(d)perf_reset();
#endif
        ui.redraw=1;return;
    }
    synth.p=*p;set.custom=1;dirty=1;ui.redraw=1;bw_send_synth();
}

static void cycle_pwm_source(int osc2){uint8_t *v=osc2?&set.p.osc2_pwm_source:&set.p.osc1_pwm_source;*v=(uint8_t)((*v+1u)%3u);synth.p=set.p;set.custom=1;dirty=1;app_say(osc2?"OSC2 PWM SOURCE":"OSC1 PWM SOURCE",PWM_SRC[*v]);bw_send_synth();}
static void seq_button_action(uint32_t k){
    if(k==B_SEQ){ui.seq_mode=(uint8_t)((ui.seq_mode+1u)%SQ_MODE_COUNT);if(ui.seq_step>=set.seq.steps)ui.seq_step=0;ui.redraw=1;}
    else if(k==B_EDIT){ui.seq_mode=(ui.seq_mode>=SQ_PIANO)?SQ_GEN:SQ_PIANO;if(ui.seq_step>=set.seq.steps)ui.seq_step=0;ui.redraw=1;}
    else if(k==B_PLAY){seq_toggle_run();ui.redraw=1;bw_send_seq();}
    else if(k==B_REC)seq_generate_action();
    else if(k==B_ARP){set.seq.block^=1u;seq_mark_changed();}
    else if(k==B_ENV)seq_mut_time();
    else if(k==B_LFO)seq_mut_note();
    else if(k==B_GLO)seq_mut_art();
    else if(k==B_FX)seq_undo_action();
}
static void input(void){
    uint32_t keys,btn=plat_buttons(),k;int e;touch_poll();keys=touch_keys();
    for(k=0;k<NKEYS;k++){uint32_t bit=1u<<k;int note=53+(int)k+12*set.octave;if((keys&bit)&&!(ui.keys&bit)){int v=(int)touch_vel(k);ui.vel[k]=(uint8_t)v;note_on(note,v);plat_midi_send(0x09u|0x90u<<8|(uint32_t)note<<16|(uint32_t)v<<24,TO_USB);}else if(!(keys&bit)&&(ui.keys&bit)){note_off(note);plat_midi_send(0x08u|0x80u<<8|(uint32_t)note<<16,TO_USB);}}
    for(k=0;k<NB;k++){uint32_t bit=1u<<k;if(!(btn&bit)||(ui.btn&bit))continue;
        if(k!=B_HOME&&k!=B_OCTDN&&k!=B_OCTUP&&k!=B_SAVE)ui.home=0;
        if(k==B_OCTDN)octave_step(-1);
        else if(k==B_OCTUP)octave_step(1);
        else if(k==B_HOME){set.page=PG_OSC1;ui.home=1;ui.redraw=1;}
        else if(k==B_SEL){if(set.page==PG_PRESETS)browser_load();else browser_open();}
        else if(set.page==PG_SEQ&&(k==B_SEQ||k==B_EDIT||k==B_PLAY||k==B_REC||k==B_ARP||k==B_ENV||k==B_LFO||k==B_GLO||k==B_FX))seq_button_action(k);
        else if(k==B_SEQ){set.page=PG_SEQ;ui.seq_mode=SQ_GEN;ui.redraw=1;}
        else if(k==B_EDIT){set.page=(set.page==PG_OSC2)?PG_OSCMOD:PG_OSC2;ui.redraw=1;}
        else if(k==B_FX){set.page=PG_FILTER;ui.redraw=1;}
        else if(k==B_ENV){set.page=(set.page==PG_AMPENV)?PG_MODENV:(set.page==PG_MODENV?PG_TRIGGER:PG_AMPENV);ui.redraw=1;}
        else if(k==B_LFO){set.page=PG_LFO;ui.redraw=1;}
        else if(k==B_GLO){set.page=(set.page==PG_GLOBAL)?PG_AFTERTOUCH:(set.page==PG_AFTERTOUCH?PG_BREATH:PG_GLOBAL);ui.redraw=1;}
        else if(k==B_ARP){set.p.filter_24db^=1u;synth.p=set.p;set.custom=1;dirty=1;app_say("FILTER SLOPE",set.p.filter_24db?"24 dB":"12 dB");bw_send_synth();}
        else if(k==B_PLAY)cycle_pwm_source(set.page==PG_OSC2);
        else if(k==B_REC){if(set.page==PG_PRESETS)browser_bank_toggle();else bank_select(set.bank?0u:1u);}
        else if(k==B_SAVE){int ok=1;set.p=synth.p;if(set.bank)ok=user_store(set.preset,&set.p,0,0);if(ok&&plat_store_save(OBJ_SETTINGS,&set,sizeof set)==0){dirty=0;seq_dirty=0;set.custom=0;app_say(set.bank?"USER PATCH":"SETTINGS","SAVED");bw_send_synth();bw_send_seq();}else app_say("SAVE","FAILED");}
    }
    if((e=plat_enc(EN_SELECT))!=0){
        ui.home=0;
        if(set.page==PG_PRESETS)browser_step(e);
        else if(set.page==PG_SEQ){if(ui.seq_mode>=SQ_PIANO)ui.seq_step=seq_step_move(ui.seq_step,e>0?1:-1);else if(ui.seq_mode==SQ_GEN){int q=(int)ui.seq_panel+(e>0?1:-1);if(q<0)q=2;if(q>2)q=0;ui.seq_panel=(uint8_t)q;}ui.redraw=1;}
        else{set.page=(uint8_t)((set.page+PG_SEQ+(e>0?1:-1))%PG_SEQ);ui.redraw=1;}
    }
    if((e=plat_enc(EN_PRESET))!=0){
        if(set.page==PG_PRESETS)browser_step(e);
        else if(set.page==PG_SEQ){if(ui.seq_mode>=SQ_PIANO)ui.seq_step=seq_page_move(ui.seq_step,e>0?1:-1);else if(ui.seq_mode==SQ_GEN){int q=(int)ui.seq_panel+(e>0?1:-1);if(q<0)q=2;if(q>2)q=0;ui.seq_panel=(uint8_t)q;}ui.redraw=1;}
        else{int p=(int)set.preset+(e>0?1:-1),hi=set.bank?(int)USER_SLOTS-1:(int)BUILTIN_SLOTS-1;if(p<0)p=hi;if(p>hi)p=0;preset_select(p);}
    }
    for(k=0;k<4;k++)if((e=plat_enc((int)EN_K1+(int)k))!=0)knob((int)k,e);
    ui.keys=keys;ui.btn=btn;plat_glow(1);plat_leds(btn|((dirty||seq_dirty)?1u<<B_SAVE:0u),keys);
}
void app_input_only(void){input();}

static void val_text(int page,int k,char *b){bassstation_params_t *p=&set.p;b[0]=0;
    if(page==PG_OSC1){if(k==0)text_cat(b,WAVE[p->osc1_wave]);if(k==1)pct_text(p->osc1_pw,b);if(k==2)text_cat(b,PWM_SRC[p->osc1_pwm_source]);if(k==3)f1_text(p->osc1_lfo_pitch,b);}
    else if(page==PG_OSC2){if(k==0)text_cat(b,RANGE[p->osc2_range]);if(k==1)text_cat(b,WAVE[p->osc2_wave]);if(k==2)pct_text(p->osc2_pw,b);if(k==3)f1_text(p->osc2_semitone,b);}
    else if(page==PG_OSCMOD){if(k==0)f1_text(p->osc1_env_pitch,b);if(k==1)f1_text(p->osc2_env_pitch,b);if(k==2)f1_text(p->osc2_lfo_pitch,b);if(k==3)cents_text(p->osc2_detune,b);}
    else if(page==PG_MIX){if(k==0)pct_text(p->osc_mix,b);if(k==1)text_cat(b,p->osc_sync?"ON":"OFF");if(k==2)pct_text(p->volume,b);if(k==3)text_cat(b,"-");}
    else if(page==PG_FILTER){if(k==0){int_text((int)p->cutoff_hz,b);text_cat(b,"Hz");}if(k==1)pct_text(p->resonance,b);if(k==2)f1_text(p->filter_env_mod,b);if(k==3)f1_text(p->filter_lfo_mod,b);}
    else if(page==PG_AMPENV){if(k==0)f1_text(p->amp_attack,b);if(k==1)f1_text(p->amp_decay,b);if(k==2)pct_text(p->amp_sustain,b);if(k==3)f1_text(p->amp_release,b);}
    else if(page==PG_MODENV){if(k==0)f1_text(p->mod_attack,b);if(k==1)f1_text(p->mod_decay,b);if(k==2)pct_text(p->mod_sustain,b);if(k==3)f1_text(p->mod_release,b);}
    else if(page==PG_TRIGGER){if(k==0)text_cat(b,TRIGGER[p->trigger_mode]);if(k==1)f1_text(p->glide_s,b);if(k==2)pct_text(p->amp_velocity,b);if(k==3)pct_text(p->mod_velocity,b);}
    else if(page==PG_LFO){if(k==0)f1_text(p->lfo_rate_hz,b);if(k==1)text_cat(b,LFO_SHAPE[p->lfo_shape]);if(k==2)f1_text(p->lfo_delay_s,b);if(k==3)text_cat(b,CLOCK_DIV[p->lfo_clock_div<17?p->lfo_clock_div:0]);}
    else if(page==PG_GLOBAL){if(k==0)cents_text(p->master_tune,b);if(k==1)f1_text(p->bend_range,b);if(k==2)f1_text(p->wheel_pitch,b);if(k==3)f1_text(p->wheel_filter,b);}
    else if(page==PG_AFTERTOUCH){if(k==0)f1_text(p->aftertouch_pitch,b);if(k==1)f1_text(p->aftertouch_filter,b);if(k==2)f1_text(p->aftertouch_bias,b);if(k==3)pct_text(synth.aftertouch,b);}
    else if(page==PG_BREATH){if(k==0)f1_text(p->breath_pitch,b);if(k==1)f1_text(p->breath_filter,b);if(k==2)f1_text(p->breath_bias,b);if(k==3)pct_text(synth.breath,b);}
    else if(page==PG_PERF){
#ifdef PLAT_HOST
        (void)k;text_cat(b,"HOST");
#else
        perf_stats_t ps;perf_read(&ps);if(k==0){int_text((int32_t)ps.audio_pct,b);text_cat(b,"%");}if(k==1){int_text((int32_t)ps.peak_pct,b);text_cat(b,"%");}if(k==2){int_text((int32_t)ps.max_us,b);text_cat(b,"us");}if(k==3)int_text((int32_t)ps.deadline_misses,b);
#endif
    }
}
static void browser_name(uint8_t bank,uint8_t slot,char *b){b[0]=0;if(bank){user_name(slot,b);if(!b[0])text_cat(b,"EMPTY");}else if(slot==0u)text_cat(b,"INIT");else text_cat(b,bst_factory_name[slot-1u]);}
static void browser_slot_text(uint8_t bank,uint8_t slot,char *b){char n[12];b[0]=0;if(bank){text_cat(b,"USER ");int_text((int32_t)slot+1,n);text_cat(b,n);text_cat(b," / 128");}else if(slot==0u)text_cat(b,"INIT");else{text_cat(b,"FACTORY ");int_text((int32_t)slot,n);text_cat(b,n);text_cat(b," / 100");}}
static uint8_t browser_neighbor(uint8_t bank,uint8_t slot,int d){int hi=bank?(int)USER_SLOTS-1:(int)BUILTIN_SLOTS-1;int p=(int)slot+d;if(p<0)p=hi;if(p>hi)p=0;return (uint8_t)p;}
static const char *seq_kind_name(int k){return k==ACID_NOTE?"NOTE":(k==ACID_TIE?"TIE":"REST");}
static void seq_note_text(int note,char *b){static const char *const N[12]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};char q[12];b[0]=0;if(note<0){text_cat(b,"-");return;}text_cat(b,N[note%12]);int_text(note/12-1,q);text_cat(b,q);}
static void seq_pct(uint8_t v,char *b){char q[12];b[0]=0;int_text(v,q);text_cat(b,q);text_cat(b,"%");}
static void seq_value_text(int panel,int k,char *b){char q[16];b[0]=0;
    if(panel==0){if(k==0){int_text(set.seq.bpm,b);text_cat(b," BPM");}else if(k==1)int_text(set.seq.steps,b);else if(k==2)seq_pct(set.seq.note_density,b);else seq_pct(set.seq.range,b);}
    else if(panel==1){if(k==0)text_cat(b,acid_seq_root_name(set.seq.root));else if(k==1)text_cat(b,acid_seq_scale_name(set.seq.scale));else if(k==2)seq_pct(set.seq.gate,b);else seq_pct(set.seq.accent_density,b);}
    else{if(k==0)seq_pct(set.seq.slide_density,b);else if(k==1){text_cat(b,"C");int_text(set.seq.base_octave,q);text_cat(b,q);}else if(k==2)seq_pct(set.seq.slide_time,b);else text_cat(b,set.seq.clock_mode==ACID_CLOCK_MIDI?"MIDI":"INTERNAL");}
}
static void seq_art_text(uint8_t st,char *b){int a=acid_seq_accent(&set.seq,st),sl=acid_seq_slide(&set.seq,st);b[0]=0;if(a&&sl)text_cat(b,"ACC+SLIDE");else if(a)text_cat(b,"ACCENT");else if(sl)text_cat(b,"SLIDE");else text_cat(b,"PLAIN");}

static uint16_t ui_cyan(void){return RGB(54,205,232);}
static uint16_t ui_line(void){return RGB(25,73,88);}
static uint16_t ui_panel(void){return RGB(9,18,24);}
static uint16_t ui_panel2(void){return RGB(14,29,38);}
static uint16_t ui_dim(void){return RGB(118,136,143);}
static uint16_t ui_white(void){return RGB(242,246,247);}

static void ui_fit_text(int x,int y,int w,const char *t,uint16_t c){
    const felucca_font_t *f=&FONT_B;
    if(text_w(f,t)>w)f=&FONT_XS;
    cv_text(x,y,f,t,c);
}
static void ui_header(const char *title,const char *sub){
    uint16_t cyan=ui_cyan(),white=ui_white(),dim=ui_dim();
    cv_rect(0,0,240,25,ui_panel2());
    cv_text(7,6,&FONT_XS,"NOVATION",white);
    cv_text(71,6,&FONT_XS,"BASS STATION",cyan);
    cv_rect(0,25,240,2,cyan);
    if(plat_ms()<ui.msg_until){
        ui_fit_text(8,32,224,ui.msg[0],white);
        cv_text(8,51,&FONT_XS,ui.msg[1],cyan);
    }else{
        ui_fit_text(8,32,190,title,white);
        if(sub&&*sub)cv_text(8,51,&FONT_XS,sub,dim);
        if(set.custom)cv_text(194,35,&FONT_XS,"EDIT",cyan);
    }
}
static void ui_card(int x,int y,const char *label,const char *value,int k,int active){
    char kn[4];uint16_t cyan=ui_cyan(),line=ui_line(),dim=ui_dim(),white=ui_white();
    cv_rect(x,y,112,39,active?cyan:line);
    cv_rect(x+1,y+1,110,37,ui_panel());
    if(active)cv_rect(x+1,y+1,3,37,cyan);
    cv_text(x+7,y+4,&FONT_XS,label,dim);
    kn[0]='K';kn[1]=(char)('1'+k);kn[2]=0;
    cv_text(x+91,y+4,&FONT_XS,kn,cyan);
    ui_fit_text(x+7,y+19,96,value,white);
}
static void current_preset_name(char *b){
    b[0]=0;
    if(set.bank){user_name(set.preset,b);if(!b[0])text_cat(b,"EMPTY USER SLOT");}
    else if(set.preset==0u)text_cat(b,"INIT");
    else text_cat(b,bst_factory_name[set.preset-1u]);
}
static void current_preset_slot(char *b){
    char n[12];b[0]=0;
    if(set.bank){text_cat(b,"USER ");int_text((int32_t)set.preset+1,n);text_cat(b,n);text_cat(b,"/128");}
    else if(set.preset==0u)text_cat(b,"INIT");
    else{text_cat(b,"FACTORY ");int_text((int32_t)set.preset,n);text_cat(b,n);text_cat(b,"/100");}
}
static void ui_footer(const char *hint){
    char nm[32],sl[32];uint16_t cyan=ui_cyan(),white=ui_white(),dim=ui_dim();
    current_preset_name(nm);current_preset_slot(sl);
    cv_begin(240,96,C_BLACK);
    cv_rect(0,0,240,2,cyan);
    cv_text(8,7,&FONT_XS,sl,dim);
    ui_fit_text(8,20,224,nm,set.custom?cyan:white);
    cv_rect(8,41,224,1,ui_line());
    if(hint&&*hint)cv_text(8,47,&FONT_XS,hint,white);
    cv_text(8,63,&FONT_XS,"HOME  EDIT  FX  ENV  LFO  GLO  SEQ",dim);
    cv_text(8,79,&FONT_XS,"SEL PRESETS   PRESET PATCH   SAVE STORE",dim);
    cv_blit(0,144);
}
static void home_value_text(int k,char *b){
    bassstation_params_t *p=&set.p;b[0]=0;
    if(k==0){int_text((int32_t)p->cutoff_hz,b);text_cat(b," Hz");}
    else if(k==1)pct_text(p->resonance,b);
    else if(k==2)pct_text(p->osc_mix,b);
    else pct_text(p->volume,b);
}
static void draw_home(void){
    static const char *const HLAB[4]={"CUTOFF","RESONANCE","OSC MIX","VOLUME"};
    char b[48],sub[72],q[16];uint32_t i;uint16_t cyan=ui_cyan(),dim=ui_dim();
    sub[0]=0;text_cat(sub,"O1 ");text_cat(sub,WAVE[set.p.osc1_wave]);text_cat(sub,"  O2 ");text_cat(sub,RANGE[set.p.osc2_range]);text_cat(sub," ");text_cat(sub,WAVE[set.p.osc2_wave]);text_cat(sub,"  FILTER ");text_cat(sub,set.p.filter_24db?"24dB":"12dB");
    cv_begin(240,144,C_BLACK);ui_header("HOME / PERFORMANCE",sub);
    for(i=0;i<4u;i++){int x=(i&1u)?122:6,y=(i<2u)?61:103;home_value_text((int)i,b);ui_card(x,y,HLAB[i],b,(int)i,0);}
    cv_blit(0,0);
    b[0]=0;text_cat(b,"OCT ");int_text(set.octave,q);text_cat(b,q);text_cat(b,"   K1-4 PERFORMANCE CONTROLS");
    ui_footer(b);
    (void)cyan;(void)dim;
}
static void draw_regular(int page){
    static const char *const LAB[PG_PRESETS][4]={{"WAVE","PW / DEPTH","PWM SRC","LFO PITCH"},{"RANGE","WAVE","PW / DEPTH","SEMITONE"},{"O1 ENV PIT","O2 ENV PIT","O2 LFO PIT","DETUNE"},{"OSC MIX","SYNC","VOLUME","-"},{"CUTOFF","RESONANCE","ENV DEPTH","LFO DEPTH"},{"ATTACK","DECAY","SUSTAIN","RELEASE"},{"ATTACK","DECAY","SUSTAIN","RELEASE"},{"TRIGGER","GLIDE","AMP VEL","MOD VEL"},{"RATE","SHAPE","DELAY","MIDI CLOCK"},{"TUNE","BEND RNG","WHEEL PIT","WHEEL FILT"},{"PITCH MOD","FILTER MOD","EG BIAS","PRESSURE"},{"PITCH MOD","FILTER MOD","EG BIAS","BREATH"},{"AUDIO AVG","PEAK","MAX WORK","DEADLINES"}};
    char b[64],sub[48],q[16];uint32_t i;
    sub[0]=0;text_cat(sub,"PAGE ");int_text(page+1,q);text_cat(sub,q);text_cat(sub,"/13   SELECT KNOB = PAGE");
    cv_begin(240,144,C_BLACK);ui_header(PAGE_NAME[page],sub);
    for(i=0;i<4u;i++){int x=(i&1u)?122:6,y=(i<2u)?61:103;val_text(page,(int)i,b);ui_card(x,y,LAB[page][i],b,(int)i,0);}
    cv_blit(0,0);
    if(page==PG_PERF)ui_footer("TURN ANY K: RESET CPU COUNTERS");
    else ui_footer("K1-K4 = LARGE PAGE CONTROLS");
}
static void draw_presets(void){
    char nm[32],slot[32],prev[32],next[32],pn[32],nn[32],b[64];uint16_t cyan=ui_cyan(),white=ui_white(),dim=ui_dim();uint8_t ps=browser_neighbor(ui.browse_bank,ui.browse_preset,-1),ns=browser_neighbor(ui.browse_bank,ui.browse_preset,1);
    browser_name(ui.browse_bank,ui.browse_preset,nm);browser_slot_text(ui.browse_bank,ui.browse_preset,slot);browser_name(ui.browse_bank,ps,pn);browser_name(ui.browse_bank,ns,nn);
    prev[0]=0;text_cat(prev,"< ");text_cat(prev,pn);next[0]=0;text_cat(next,"> ");text_cat(next,nn);
    cv_begin(240,144,C_BLACK);ui_header("PRESET BROWSER",ui.browse_bank?"USER / COMMUNITY":"FACTORY / INIT");
    cv_text(8,61,&FONT_XS,slot,dim);ui_fit_text(8,79,224,nm,cyan);
    cv_rect(8,106,224,1,ui_line());cv_text(8,113,&FONT_XS,prev,dim);cv_text(8,129,&FONT_XS,next,dim);cv_blit(0,0);
    cv_begin(240,96,C_BLACK);cv_rect(0,0,240,2,cyan);
    b[0]=0;if(ui.browse_bank==set.bank&&ui.browse_preset==set.preset)text_cat(b,"CURRENTLY LOADED");else text_cat(b,"PRESS SEL TO LOAD");
    ui_fit_text(8,9,224,b,white);
    cv_text(8,36,&FONT_XS,"SELECT/PRESET: BROWSE   REC: BANK",dim);
    cv_text(8,54,&FONT_XS,"SEL: LOAD              HOME: EXIT",white);
    cv_text(8,72,&FONT_XS,ui.browse_bank&&!user_valid(ui.browse_preset)?"EMPTY USER SLOT = INIT SOUND":"BASS STATION PATCH LIBRARY",cyan);
    cv_blit(0,144);
}
static const char *seq_mode_name(uint8_t mode){
    static const char *const N[SQ_MODE_COUNT]={"GENERATE","MUTATE","PIANO ROLL","GATE","OCTAVE","ACCENT","SLIDE"};
    return N[mode<SQ_MODE_COUNT?mode:0u];
}
static void seq_bar_text(char *b){char q[12];uint8_t base=(uint8_t)((ui.seq_step/16u)*16u);b[0]=0;text_cat(b,"BAR ");int_text((int32_t)(base/16u)+1,q);text_cat(b,q);text_cat(b,"/4   STEPS ");int_text((int32_t)base+1,q);text_cat(b,q);text_cat(b,"-");int_text((int32_t)base+16,q);text_cat(b,q);}
static uint16_t seq_step_colour(uint8_t st){int k=acid_seq_kind(&set.seq,st);uint16_t c=k==ACID_NOTE?RGB(24,70,112):(k==ACID_TIE?ui_line():ui_panel2());if(acid_seq_slide(&set.seq,st))c=RGB(18,105,122);if(acid_seq_accent(&set.seq,st))c=RGB(185,215,222);return c;}
static void draw_seq_piano(void){
    static const uint8_t black[12]={0,1,0,1,0,0,1,0,1,0,1,0};
    char q[64],v[32],n[16];uint16_t cyan=ui_cyan(),white=ui_white(),dim=ui_dim();int row,col;uint8_t base=(uint8_t)((ui.seq_step/16u)*16u);
    seq_bar_text(q);cv_text(8,51,&FONT_XS,q,dim);
    for(row=0;row<12;row++){
        int semi=11-row,y=60+row*6,allowed=acid_seq_scale_has_semitone(&set.seq,semi);uint16_t key=black[semi]?RGB(22,28,31):RGB(142,153,157);
        if(!allowed)key=RGB(39,48,52);
        cv_rect(6,y,42,5,key);
        if(semi==0)cv_rect(6,y,3,5,cyan);
        for(col=0;col<16;col++){
            uint8_t st=(uint8_t)(base+col);int x=55+col*11,ps=-1;uint16_t c=allowed?ui_panel2():ui_panel();
            if(st>=set.seq.steps)c=RGB(5,10,13);else ps=seq_pitch_semitone(st);
            if(ps==semi&&acid_seq_kind(&set.seq,st)!=ACID_REST)c=seq_step_colour(st);
            if(st==ui.seq_step&&ps==semi)c=cyan;
            cv_rect(x,y,9,5,c);
        }
    }
    {int sx=55+((int)ui.seq_step-(int)base)*11;if(sx>=55&&sx<231){cv_rect(sx,60,1,72,cyan);if(seqrt.running&&seqrt.pos>=base&&seqrt.pos<base+16u)cv_rect(55+((int)seqrt.pos-(int)base)*11+8,60,1,72,white);}}
    q[0]=0;text_cat(q,"STEP ");int_text((int32_t)ui.seq_step+1,n);text_cat(q,n);text_cat(q,"  ");seq_note_text(acid_seq_midi_note(&set.seq,ui.seq_step),v);text_cat(q,v);text_cat(q,"  ");text_cat(q,seq_kind_name(acid_seq_kind(&set.seq,ui.seq_step)));cv_text(8,132,&FONT_XS,q,white);
}
static void draw_seq_lanes(uint8_t mode){
    static const char *const gate_lab[3]={"REST","NOTE","TIE"};
    static const char *const oct_lab[5]={"+2","+1","0","-1","-2"};
    static const char *const on_lab[2]={"OFF","ON"};
    char q[64],v[32],n[16];uint16_t cyan=ui_cyan(),white=ui_white(),dim=ui_dim();int rows,row,col,rowh,y0=64;const char *const *lab;uint8_t base=(uint8_t)((ui.seq_step/16u)*16u);
    if(mode==SQ_GATE){rows=3;rowh=18;lab=gate_lab;}else if(mode==SQ_OCT){rows=5;rowh=11;lab=oct_lab;}else{rows=2;rowh=27;lab=on_lab;}
    seq_bar_text(q);cv_text(8,51,&FONT_XS,q,dim);
    for(row=0;row<rows;row++){
        int y=y0+row*rowh;cv_text(7,y+3,&FONT_XS,lab[row],dim);
        for(col=0;col<16;col++){
            uint8_t st=(uint8_t)(base+col);int x=48+col*11,state=-1;uint16_t c=ui_panel2();
            if(st>=set.seq.steps)c=RGB(5,10,13);else if(mode==SQ_GATE)state=acid_seq_kind(&set.seq,st);else if(mode==SQ_OCT)state=2-(int)set.seq.step[st].octave;else if(mode==SQ_ACC)state=acid_seq_accent(&set.seq,st)?1:0;else state=acid_seq_slide(&set.seq,st)?1:0;
            if(state==row)c=seq_step_colour(st);
            if(st==ui.seq_step&&state==row)c=cyan;
            cv_rect(x,y,9,rowh-3,c);
        }
    }
    q[0]=0;text_cat(q,"STEP ");int_text((int32_t)ui.seq_step+1,n);text_cat(q,n);text_cat(q,"  ");
    if(mode==SQ_GATE)text_cat(q,seq_kind_name(acid_seq_kind(&set.seq,ui.seq_step)));else if(mode==SQ_OCT){text_cat(q,"OCT ");int_text((int32_t)set.seq.step[ui.seq_step].octave,n);text_cat(q,n);}else if(mode==SQ_ACC)text_cat(q,acid_seq_accent(&set.seq,ui.seq_step)?"ACCENT ON":"ACCENT OFF");else text_cat(q,acid_seq_slide(&set.seq,ui.seq_step)?"SLIDE ON":"SLIDE OFF");
    seq_note_text(acid_seq_midi_note(&set.seq,ui.seq_step),v);text_cat(q,"   ");text_cat(q,v);cv_text(8,126,&FONT_XS,q,white);
}
static void draw_seq(void){
    static const char *const LAB0[3][4]={{"TEMPO","STEPS","NOTE DENS","RANGE"},{"ROOT","SCALE","GATE","ACCENT"},{"SLIDE","BASE OCT","SLIDE TIME","CLOCK"}};
    char q[80],v[48],n[16];uint16_t cyan=ui_cyan(),white=ui_white(),dim=ui_dim();uint32_t i;
    cv_begin(240,144,C_BLACK);ui_header(seq_mode_name(ui.seq_mode),0);
    if(ui.seq_mode==SQ_GEN){
        q[0]=0;text_cat(q,"GENERATE PAGE ");int_text((int32_t)ui.seq_panel+1,n);text_cat(q,n);text_cat(q,"/3   SELECT/PRESET CHANGES PAGE");cv_text(8,51,&FONT_XS,q,dim);
        for(i=0;i<4u;i++){int x=(i&1u)?122:6,y=(i<2u)?63:104;seq_value_text(ui.seq_panel,(int)i,v);ui_card(x,y,LAB0[ui.seq_panel][i],v,(int)i,0);}
    }else if(ui.seq_mode==SQ_MUTATE){
        static const char *const MLAB[4]={"TIME OPS","NOTE/OCT OPS","ACC/SLD OPS","BLOCK SEED"};
        cv_text(8,51,&FONT_XS,"MUTATION STRENGTH / SEED LOCK",dim);
        for(i=0;i<4u;i++){int x=(i&1u)?122:6,y=(i<2u)?61:103;v[0]=0;if(i==0)int_text(set.seq.mut_time_ops,v);else if(i==1)int_text(set.seq.mut_note_ops,v);else if(i==2)int_text(set.seq.mut_art_ops,v);else text_cat(v,set.seq.block?"LOCKED":"UNLOCKED");ui_card(x,y,MLAB[i],v,(int)i,0);}
    }else if(ui.seq_mode==SQ_PIANO)draw_seq_piano();
    else draw_seq_lanes(ui.seq_mode);
    cv_blit(0,0);
    cv_begin(240,96,C_BLACK);cv_rect(0,0,240,2,cyan);
    if(ui.seq_mode==SQ_GEN){cv_text(8,8,&FONT_XS,"K1-K4 GENERATION CONTROLS",white);cv_text(8,27,&FONT_XS,"SEQ NEXT MODE   EDIT PIANO ROLL",dim);cv_text(8,46,&FONT_XS,"REC GEN   ARP BLOCK   ENV/LFO/GLO MUT",white);}
    else if(ui.seq_mode==SQ_MUTATE){cv_text(8,8,&FONT_XS,"K1 TIME OPS  K2 NOTE/OCT OPS",white);cv_text(8,27,&FONT_XS,"K3 ACC/SLD OPS   K4 BLOCK SEED",dim);cv_text(8,46,&FONT_XS,"ENV/LFO/GLO APPLY   REC ALL IF BLOCK",white);}
    else if(ui.seq_mode==SQ_PIANO){cv_text(8,8,&FONT_XS,"K1 PITCH   K2 OCT   K3 GATE   K4 ART",white);cv_text(8,27,&FONT_XS,"SELECT STEP   PRESET BAR   SEQ NEXT MODE",dim);cv_text(8,46,&FONT_XS,"REC GEN  ARP BLOCK  ENV/LFO/GLO MUT  FX UNDO",white);}
    else if(ui.seq_mode==SQ_GATE){cv_text(8,8,&FONT_XS,"K1 STEP GATE  K2 GATE%  K3 DENS  K4 STEPS",white);cv_text(8,27,&FONT_XS,"SELECT STEP   PRESET BAR   SEQ NEXT MODE",dim);cv_text(8,46,&FONT_XS,"EDIT RETURNS GENERATE/PIANO   FX UNDO",white);}
    else if(ui.seq_mode==SQ_OCT){cv_text(8,8,&FONT_XS,"K1 STEP OCT  K2 BASE OCT  K3 RANGE  K4 ROOT",white);cv_text(8,27,&FONT_XS,"SELECT STEP   PRESET BAR   SEQ NEXT MODE",dim);cv_text(8,46,&FONT_XS,"REC GEN  ENV/LFO/GLO MUT  FX UNDO",white);}
    else if(ui.seq_mode==SQ_ACC){cv_text(8,8,&FONT_XS,"K1 ACCENT  K2 DENS  K3 MUT OPS  K4 BLOCK",white);cv_text(8,27,&FONT_XS,"SELECT STEP   PRESET BAR   SEQ NEXT MODE",dim);cv_text(8,46,&FONT_XS,"GLO MUTATE ART   FX UNDO   REC GENERATE",white);}
    else {cv_text(8,8,&FONT_XS,"K1 SLIDE  K2 DENS  K3 TIME  K4 GATE",white);cv_text(8,27,&FONT_XS,"SELECT STEP   PRESET BAR   SEQ NEXT MODE",dim);cv_text(8,46,&FONT_XS,"GLO MUTATE ART   FX UNDO   REC GENERATE",white);}
    q[0]=0;text_cat(q,"MODE ");int_text((int32_t)ui.seq_mode+1,n);text_cat(q,n);text_cat(q,"/7   STEP ");int_text((int32_t)seqrt.pos+1,n);text_cat(q,n);text_cat(q,"/");int_text(set.seq.steps,n);text_cat(q,n);text_cat(q,set.seq.clock_mode==ACID_CLOCK_MIDI?"   MIDI":"   INTERNAL");text_cat(q,seqrt.running?" PLAY":" STOP");cv_text(8,72,&FONT_XS,q,seqrt.running?cyan:dim);cv_blit(0,144);
}

static void draw(void){
    if(ui.home){draw_home();return;}
    if(set.page==PG_SEQ){draw_seq();return;}
    if(set.page==PG_PRESETS){draw_presets();return;}
    draw_regular(set.page);
}
void app_splash(void){
    uint32_t band,y,x,p;
    /* 4 bpp indexed splash, decoded into the existing 240x144 canvas in two 120-row bands. */
    for(band=0;band<2u;band++){
        cv_begin(240,120,C_BLACK);
        for(y=0;y<120u;y++)for(x=0;x<240u;x++){
            uint32_t gp=(band*120u+y)*240u+x;uint8_t z=BST_SPLASH_4BPP[gp>>1];uint8_t pi=(gp&1u)?(z&15u):(z>>4);
            p=y*240u+x;cv_px[p]=swap16(BST_SPLASH_PAL[pi]);
        }
        cv_blit(0,band*120u);
    }
}

void app_ui_init(void){touch_on(1);lcd_fill(0,0,240,240,C_BLACK);ui.home=0;ui.redraw=1;}
void app_frame(void){input();if(seqrt.redraw_pending){seqrt.redraw_pending=0;ui.redraw=1;}if(ui.msg_until&&plat_ms()>=ui.msg_until){ui.msg_until=0;ui.redraw=1;}if(ui.redraw){ui.redraw=0;draw();}}
#ifdef PLAT_HOST
static int host_num(const char *s){int v=0,neg=0;if(!s)return 0;if(*s=='-'){neg=1;s++;}while(*s>='0'&&*s<='9'){v=v*10+(*s-'0');s++;}return neg?-v:v;}
void app_host_reboot(void){bw_connected=0;bank_cache_idx=0xffu;import_on=0;import_slot=0;dirty=0;seq_dirty=0;ui.keys=ui.btn=0;ui.msg_until=0;ui.home=0;browser_sync();mem_zero(&seqrt,sizeof seqrt);seqrt.current_note=-1;mem_zero(&seq_undo,sizeof seq_undo);}
int app_host_cmd(const char *cmd,const char *a,const char *b,const char *c,const char *d){uint8_t q[80],f[76];if(!strcmp(cmd,"rackrx")){if(bassstation_rack_sysex_encode(&set.p,f)==76u)app_sysex(f+1,74u);return 1;}if(!strcmp(cmd,"pluginrx")){int slot=host_num(a),src=host_num(b),i;if(slot<0||slot>=(int)USER_SLOTS||src<0||src>=100)return 1;q[0]=0x7d;q[1]='B';q[2]='S';q[3]=8;q[4]=0x30;q[5]=(uint8_t)slot;for(i=0;i<34;i++){q[6+i*2]=(uint8_t)(bst_factory_record[src][i]>>4);q[7+i*2]=(uint8_t)(bst_factory_record[src][i]&15u);}app_sysex(q,74);return 1;}if(!strcmp(cmd,"bwctl")){
        q[0]=0x7d;q[1]='B';q[2]='W';q[3]=BW_PROTO;
        if(!strcmp(a,"HELLO")){q[4]=BW_HELLO;app_sysex(q,5);}
        else if(!strcmp(a,"PARAM")){int id=host_num(b),v=host_num(c);q[4]=BW_SET_SYNTH;q[5]=(uint8_t)(id&127);q[6]=(uint8_t)((id>>7)&127);q[7]=(uint8_t)(v&127);q[8]=(uint8_t)((v>>7)&127);app_sysex(q,9);}
        else if(!strcmp(a,"LOAD")){int bank=host_num(b),slot=host_num(c);q[4]=BW_LOAD_PRESET;q[5]=(uint8_t)(bank?1:0);q[6]=(uint8_t)(slot&127);q[7]=(uint8_t)((slot>>7)&127);app_sysex(q,8);}
        else if(!strcmp(a,"SEQCTRL")){int id=host_num(b),v=host_num(c);q[4]=BW_SET_SEQ_CTRL;q[5]=(uint8_t)id;q[6]=(uint8_t)(v&127);q[7]=(uint8_t)((v>>7)&127);app_sysex(q,8);}
        else if(!strcmp(a,"STEP")){q[4]=BW_SET_SEQ_STEP;q[5]=(uint8_t)host_num(b);q[6]=(uint8_t)host_num(c);q[7]=(uint8_t)host_num(d);app_sysex(q,8);}
        else if(!strcmp(a,"ACTION")){q[4]=BW_SEQ_ACTION;q[5]=(uint8_t)host_num(b);app_sysex(q,6);}
        else if(!strcmp(a,"SAVE")){int slot=host_num(b);q[4]=BW_SAVE_USER;q[5]=(uint8_t)(slot&127);q[6]=(uint8_t)((slot>>7)&127);app_sysex(q,7);}
        else if(!strcmp(a,"SAVESTATE")){q[4]=BW_SAVE_SETTINGS;app_sysex(q,5);}
        else return 0;
        return 1;
    }
    if(strcmp(cmd,"bsctl"))return 0;
    q[0]=0x7d;q[1]='B';q[2]='S';q[3]=7;if(!strcmp(a,"START")){q[4]=0x20;q[5]=(uint8_t)host_num(b);app_sysex(q,6);}else if(!strcmp(a,"STOP")){q[4]=0x21;app_sysex(q,5);}else if(!strcmp(a,"EXPORT")){q[4]=0x22;q[5]=(uint8_t)host_num(b);app_sysex(q,6);}else if(!strcmp(a,"LOAD")){q[4]=0x23;q[5]=(uint8_t)host_num(b);app_sysex(q,6);}else if(!strcmp(a,"SAVE")){q[4]=0x24;q[5]=(uint8_t)host_num(b);app_sysex(q,6);}else if(!strcmp(a,"CLEAR")){q[4]=0x26;q[5]=(uint8_t)host_num(b);app_sysex(q,6);}else return 0;return 1;}
int app_expect(const char *what,int *got){if(!strcmp(what,"page"))*got=set.page;else if(!strcmp(what,"preset"))*got=set.preset;else if(!strcmp(what,"bank"))*got=set.bank;else if(!strcmp(what,"browse_preset"))*got=ui.browse_preset;else if(!strcmp(what,"browse_bank"))*got=ui.browse_bank;else if(!strcmp(what,"browse_valid"))*got=ui.browse_bank?user_valid(ui.browse_preset):1;else if(!strcmp(what,"user_valid"))*got=user_valid(set.preset);else if(!strcmp(what,"user_seeded"))*got=set.reserved[0]==USER_SEED_VERSION;else if(!strcmp(what,"user_count")){int n=0,i;for(i=0;i<(int)USER_SLOTS;i++)if(user_valid((uint8_t)i))n++;*got=n;}else if(!strcmp(what,"import"))*got=import_on;else if(!strcmp(what,"import_slot"))*got=import_slot;else if(!strcmp(what,"sync"))*got=set.p.osc_sync;else if(!strcmp(what,"trigger"))*got=set.p.trigger_mode;else if(!strcmp(what,"pwm1"))*got=set.p.osc1_pwm_source;else if(!strcmp(what,"pwm2"))*got=set.p.osc2_pwm_source;else if(!strcmp(what,"lfo_clock"))*got=set.p.lfo_clock_div;else if(!strcmp(what,"seq_running"))*got=seqrt.running;else if(!strcmp(what,"seq_step"))*got=seqrt.pos;else if(!strcmp(what,"seq_len"))*got=set.seq.steps;else if(!strcmp(what,"seq_block"))*got=set.seq.block;else if(!strcmp(what,"seq_clock"))*got=set.seq.clock_mode;else if(!strcmp(what,"seq_base_oct"))*got=set.seq.base_octave;else if(!strcmp(what,"seq_edit"))*got=ui.seq_mode>=SQ_PIANO;else if(!strcmp(what,"seq_mode"))*got=ui.seq_mode;else if(!strcmp(what,"seq_undo"))*got=seq_undo.valid;else if(!strcmp(what,"seq_mut_time"))*got=set.seq.mut_time_ops;else if(!strcmp(what,"seq_mut_note"))*got=set.seq.mut_note_ops;else if(!strcmp(what,"seq_mut_art"))*got=set.seq.mut_art_ops;else if(!strcmp(what,"seq_selected_kind"))*got=acid_seq_kind(&set.seq,ui.seq_step);else if(!strcmp(what,"seq_selected_degree"))*got=acid_seq_degree(&set.seq,ui.seq_step);else if(!strcmp(what,"seq_selected_oct"))*got=set.seq.step[ui.seq_step].octave;else if(!strcmp(what,"seq_selected_acc"))*got=acid_seq_accent(&set.seq,ui.seq_step);else if(!strcmp(what,"seq_selected_slide"))*got=acid_seq_slide(&set.seq,ui.seq_step);else if(!strcmp(what,"custom"))*got=set.custom;else if(!strcmp(what,"moog"))*got=(set.bank==0&&set.preset==1&&set.p.trigger_mode==BST_TRIG_MULTI&&set.p.osc1_wave==0&&set.p.osc2_wave==1&&set.p.osc2_range==1&&set.p.filter_24db==1&&set.p.resonance>0.31f&&set.p.resonance<0.32f&&set.p.cutoff_hz>10.0f&&set.p.cutoff_hz<12.0f);else if(!strcmp(what,"moog_wheel"))*got=(set.p.wheel_pitch>4.99f&&set.p.wheel_pitch<5.01f);else if(!strcmp(what,"factory_last"))*got=(set.bank==0&&set.preset==100u);else if(!strcmp(what,"lfo_fast"))*got=set.p.lfo_rate_hz>900.0f;else if(!strcmp(what,"env20"))*got=(set.p.amp_attack>19.9f&&set.p.amp_decay>19.9f&&set.p.amp_release>19.9f);else if(!strcmp(what,"o1_lfo_cents")){float x=set.p.osc1_lfo_pitch*100.0f;*got=(int)(x+(x>=0.0f?0.5f:-0.5f));}else if(!strcmp(what,"o2_semi"))*got=(int)(set.p.osc2_semitone+0.5f);else return 0;return 1;}
#endif
