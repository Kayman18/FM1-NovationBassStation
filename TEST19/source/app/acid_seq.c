/* SPDX-License-Identifier: MPL-2.0 */
#include "acid_seq.h"

static int clampi(int v,int lo,int hi){return v<lo?lo:(v>hi?hi:v);}
static uint32_t mix32(uint32_t x){x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;x^=x>>16;return x?x:0x30330301u;}
static uint32_t rnd(uint32_t *x){uint32_t q=*x;if(!q)q=0x30330301u;q^=q<<13;q^=q>>17;q^=q<<5;*x=q;return q;}

static const uint8_t SCALE_LEN[8]={5,7,7,7,7,6,7,12};
static const uint8_t SCALE_NOTE[8][12]={
    {0,3,5,7,10,0,0,0,0,0,0,0},      /* acid minor pentatonic */
    {0,2,3,5,7,8,10,0,0,0,0,0},       /* natural minor */
    {0,1,3,5,7,8,10,0,0,0,0,0},       /* phrygian */
    {0,2,3,5,7,8,11,0,0,0,0,0},       /* harmonic minor */
    {0,2,3,5,7,9,10,0,0,0,0,0},       /* dorian */
    {0,3,5,6,7,10,0,0,0,0,0,0},       /* blues */
    {0,2,4,5,7,9,11,0,0,0,0,0},       /* major */
    {0,1,2,3,4,5,6,7,8,9,10,11}        /* chromatic */
};
static const char *const SCALE_NAME[8]={"ACID","MINOR","PHRYG","HARM MIN","DORIAN","BLUES","MAJOR","CHROM"};
static const char *const ROOT_NAME[12]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

static int kind_code(uint8_t c){int k=c&3u;return k==ACID_TIE?ACID_TIE:(k==ACID_NOTE?ACID_NOTE:ACID_REST);}
static int degree_code(uint8_t c){return (c>>2)&15u;}
static uint8_t make_code(int kind,int degree,int accent,int slide){
    return (uint8_t)((kind&3)|((degree&15)<<2)|(accent?0x40u:0u)|(slide?0x80u:0u));
}
static void snapshot(const acid_seq_t *s,acid_seq_undo_t *u){uint32_t i;if(!u)return;u->valid=1;u->mutation_counter=s->mutation_counter;for(i=0;i<ACID_SEQ_NSTEP;i++)u->step[i]=s->step[i];}
static int active_note_count(const acid_seq_t *s){uint32_t i;int n=0;for(i=0;i<ACID_SEQ_NSTEP;i++)if(kind_code(s->step[i].code)==ACID_NOTE)n++;return n;}
static int note_at(const acid_seq_t *s,uint8_t i){return kind_code(s->step[i].code)==ACID_NOTE;}

int acid_seq_scale_len(uint8_t scale){return SCALE_LEN[scale<8u?scale:0u];}
const char *acid_seq_scale_name(uint8_t scale){return SCALE_NAME[scale<8u?scale:0u];}
const char *acid_seq_root_name(uint8_t root){return ROOT_NAME[root<12u?root:0u];}
int acid_seq_degree_semitone(const acid_seq_t *s,int degree){int sc=s?(int)(s->scale%8u):0,len=SCALE_LEN[sc];degree=clampi(degree,0,len-1);return SCALE_NOTE[sc][degree];}
int acid_seq_degree_from_semitone(const acid_seq_t *s,int semitone){int sc=s?(int)(s->scale%8u):0,len=SCALE_LEN[sc],i;semitone=((semitone%12)+12)%12;for(i=0;i<len;i++)if(SCALE_NOTE[sc][i]==semitone)return i;return -1;}
int acid_seq_scale_has_semitone(const acid_seq_t *s,int semitone){return acid_seq_degree_from_semitone(s,semitone)>=0;}
int acid_seq_kind(const acid_seq_t *s,uint8_t step){return step<ACID_SEQ_NSTEP?kind_code(s->step[step].code):ACID_REST;}
int acid_seq_degree(const acid_seq_t *s,uint8_t step){return step<ACID_SEQ_NSTEP?degree_code(s->step[step].code):0;}
int acid_seq_accent(const acid_seq_t *s,uint8_t step){return step<ACID_SEQ_NSTEP&&(s->step[step].code&0x40u)!=0u;}
int acid_seq_slide(const acid_seq_t *s,uint8_t step){return step<ACID_SEQ_NSTEP&&(s->step[step].code&0x80u)!=0u;}

void acid_seq_set_kind(acid_seq_t *s,uint8_t step,int kind){if(!s||step>=ACID_SEQ_NSTEP)return;s->step[step].code=(uint8_t)((s->step[step].code&0xfcu)|(uint8_t)clampi(kind,0,2));acid_seq_normalize(s);}
void acid_seq_set_degree(acid_seq_t *s,uint8_t step,int degree){int len;if(!s||step>=ACID_SEQ_NSTEP)return;len=acid_seq_scale_len(s->scale);degree=clampi(degree,0,len-1);s->step[step].code=(uint8_t)((s->step[step].code&0xc3u)|((uint8_t)degree<<2));if(acid_seq_kind(s,step)!=ACID_NOTE)s->step[step].code=(uint8_t)((s->step[step].code&0xfcu)|ACID_NOTE);acid_seq_normalize(s);}
void acid_seq_set_octave(acid_seq_t *s,uint8_t step,int octave){if(!s||step>=ACID_SEQ_NSTEP)return;s->step[step].octave=(int8_t)clampi(octave,-2,2);if(acid_seq_kind(s,step)!=ACID_NOTE)s->step[step].code=(uint8_t)((s->step[step].code&0xfcu)|ACID_NOTE);acid_seq_normalize(s);}
void acid_seq_set_art(acid_seq_t *s,uint8_t step,int accent,int slide){if(!s||step>=ACID_SEQ_NSTEP)return;s->step[step].code=(uint8_t)((s->step[step].code&0x3fu)|(accent?0x40u:0u)|(slide?0x80u:0u));acid_seq_normalize(s);}

int acid_seq_midi_note(const acid_seq_t *s,uint8_t step){
    int len,degree,semi,note;
    if(!s||step>=ACID_SEQ_NSTEP||acid_seq_kind(s,step)!=ACID_NOTE)return -1;
    len=acid_seq_scale_len(s->scale);degree=acid_seq_degree(s,step)%len;semi=SCALE_NOTE[s->scale<8u?s->scale:0u][degree];
    note=12*((int)clampi(s->base_octave,1,5)+1)+(int)(s->root%12u)+semi+12*(int)s->step[step].octave;
    return clampi(note,0,127);
}

void acid_seq_normalize(acid_seq_t *s){
    uint32_t i;int prev_active=0,len;
    if(!s)return;
    s->version=ACID_SEQ_VERSION;
    s->bpm=(uint16_t)clampi((int)s->bpm,30,300);s->steps=(uint8_t)clampi((int)s->steps,1,64);
    s->note_density=(uint8_t)clampi((int)s->note_density,5,100);s->range=(uint8_t)clampi((int)s->range,0,100);
    s->root=(uint8_t)(s->root%12u);s->scale=(uint8_t)(s->scale%8u);s->gate=(uint8_t)clampi((int)s->gate,5,100);
    s->accent_density=(uint8_t)clampi((int)s->accent_density,0,100);s->slide_density=(uint8_t)clampi((int)s->slide_density,0,100);
    s->base_octave=(uint8_t)clampi((int)s->base_octave,1,5);s->slide_time=(uint8_t)clampi((int)s->slide_time,5,100);
    s->clock_mode=s->clock_mode?ACID_CLOCK_MIDI:ACID_CLOCK_INTERNAL;s->block=s->block?1u:0u;
    s->mut_time_ops=(uint8_t)clampi((int)(s->mut_time_ops?s->mut_time_ops:2u),1,8);
    s->mut_note_ops=(uint8_t)clampi((int)(s->mut_note_ops?s->mut_note_ops:2u),1,8);
    s->mut_art_ops=(uint8_t)clampi((int)(s->mut_art_ops?s->mut_art_ops:3u),1,8);
    len=acid_seq_scale_len(s->scale);
    for(i=0;i<ACID_SEQ_NSTEP;i++){
        int k=kind_code(s->step[i].code),d=degree_code(s->step[i].code)%len,a=(s->step[i].code&0x40u)!=0u,sl=(s->step[i].code&0x80u)!=0u;
        if(k==ACID_TIE&&!prev_active)k=ACID_NOTE;
        s->step[i].octave=(int8_t)clampi((int)s->step[i].octave,-2,2);
        if(k!=ACID_NOTE){a=0;sl=0;}
        s->step[i].code=make_code(k,d,a,sl);
        prev_active=(k==ACID_NOTE||k==ACID_TIE);
    }
    if(active_note_count(s)==0)s->step[0].code=make_code(ACID_NOTE,0,0,0);
    /* A slide belongs to an active Note and must lead directly into the next
       active Note attack.  The last active step wraps to step 0; do not use
       hidden steps above s->steps when validating a shorter pattern. */
    for(i=0;i<s->steps;i++)if(s->step[i].code&0x80u){
        uint8_t n=(uint8_t)((i+1u)>=s->steps?0u:i+1u);
        if(!note_at(s,(uint8_t)i)||!note_at(s,n))s->step[i].code&=0x7fu;
    }
}

static void generate_from_seed(acid_seq_t *s){
    uint32_t r=s->seed,i;int len=acid_seq_scale_len(s->scale),max_oct=s->range<34u?0:(s->range<67u?1:2),prev_active=0;
    for(i=0;i<ACID_SEQ_NSTEP;i++){
        int k,d,oct=0,a=0,sl=0;
        if(i==0u)k=ACID_NOTE;
        else if((int)(rnd(&r)%100u)>=(int)s->note_density)k=ACID_REST;
        else if(prev_active&&(rnd(&r)%100u)<(uint32_t)(8u+s->note_density/6u))k=ACID_TIE;
        else k=ACID_NOTE;
        d=(int)(rnd(&r)%(uint32_t)len);
        if(max_oct){int span=max_oct*2+1;oct=(int)(rnd(&r)%(uint32_t)span)-max_oct;}
        if(k==ACID_NOTE){a=(int)(rnd(&r)%100u)<s->accent_density;sl=(int)(rnd(&r)%100u)<s->slide_density;}
        s->step[i].code=make_code(k,d,a,sl);s->step[i].octave=(int8_t)oct;prev_active=(k!=ACID_REST);
    }
    acid_seq_normalize(s);
}

void acid_seq_defaults(acid_seq_t *s){uint8_t *p=(uint8_t*)s;uint32_t i;if(!s)return;for(i=0;i<(uint32_t)sizeof(*s);i++)p[i]=0;s->version=ACID_SEQ_VERSION;s->seed=0x30317017u;s->bpm=120;s->steps=16;s->note_density=65;s->range=45;s->root=0;s->scale=0;s->gate=85;s->accent_density=60;s->slide_density=50;s->base_octave=2;s->slide_time=55;s->clock_mode=ACID_CLOCK_INTERNAL;s->block=0;s->mut_time_ops=2;s->mut_note_ops=2;s->mut_art_ops=3;generate_from_seed(s);}

static void mutate_time_raw(acid_seq_t *s,uint32_t *r,int ops){int n;for(n=0;n<ops;n++){uint8_t i=(uint8_t)(1u+rnd(r)%63u);int k=acid_seq_kind(s,i);if(k==ACID_REST)k=ACID_NOTE;else if(k==ACID_NOTE)k=ACID_TIE;else k=ACID_REST;s->step[i].code=(uint8_t)((s->step[i].code&0xfcu)|k);}acid_seq_normalize(s);}
static void mutate_note_raw(acid_seq_t *s,uint32_t *r,int ops){int n,len=acid_seq_scale_len(s->scale);for(n=0;n<ops;n++){uint8_t i;int tries=0;do{i=(uint8_t)(rnd(r)%ACID_SEQ_NSTEP);}while(acid_seq_kind(s,i)!=ACID_NOTE&&++tries<80);if(acid_seq_kind(s,i)!=ACID_NOTE)continue;if(rnd(r)&1u){int d=(acid_seq_degree(s,i)+(rnd(r)&1u?1:-1)+len)%len;s->step[i].code=(uint8_t)((s->step[i].code&0xc3u)|((uint8_t)d<<2));}else{int o=(int)s->step[i].octave+(rnd(r)&1u?1:-1);if(o>2)o=-2;if(o<-2)o=2;s->step[i].octave=(int8_t)o;}}acid_seq_normalize(s);}
static void mutate_art_raw(acid_seq_t *s,uint32_t *r,int ops){int n;for(n=0;n<ops;n++){uint8_t i=(uint8_t)(rnd(r)%ACID_SEQ_NSTEP);if(acid_seq_kind(s,i)!=ACID_NOTE)continue;if(rnd(r)&1u)s->step[i].code^=0x40u;else s->step[i].code^=0x80u;}acid_seq_normalize(s);}

void acid_seq_mutate_time(acid_seq_t *s,acid_seq_undo_t *u){uint32_t r;if(!s)return;snapshot(s,u);r=mix32(s->seed^++s->mutation_counter^0x54494d45u);mutate_time_raw(s,&r,(int)s->mut_time_ops);}
void acid_seq_mutate_note(acid_seq_t *s,acid_seq_undo_t *u){uint32_t r;if(!s)return;snapshot(s,u);r=mix32(s->seed^++s->mutation_counter^0x4e4f5445u);mutate_note_raw(s,&r,(int)s->mut_note_ops);}
void acid_seq_mutate_art(acid_seq_t *s,acid_seq_undo_t *u){uint32_t r;if(!s)return;snapshot(s,u);r=mix32(s->seed^++s->mutation_counter^0x41525421u);mutate_art_raw(s,&r,(int)s->mut_art_ops);}

void acid_seq_generate(acid_seq_t *s,uint32_t entropy,acid_seq_undo_t *u){
    uint32_t r;if(!s)return;
    if(s->block){snapshot(s,u);r=mix32(s->seed^++s->mutation_counter^0x424c4f43u);mutate_time_raw(s,&r,(int)s->mut_time_ops);mutate_note_raw(s,&r,(int)s->mut_note_ops);mutate_art_raw(s,&r,(int)s->mut_art_ops);return;}
    if(u)u->valid=0;
    s->seed=mix32(s->seed^entropy^0x30330303u);
    s->mutation_counter=0;
    generate_from_seed(s);
}
void acid_seq_undo(acid_seq_t *s,acid_seq_undo_t *u){uint32_t i;if(!s||!u||!u->valid)return;for(i=0;i<ACID_SEQ_NSTEP;i++)s->step[i]=u->step[i];s->mutation_counter=u->mutation_counter;u->valid=0;acid_seq_normalize(s);}
void acid_seq_clear(acid_seq_t *s,acid_seq_undo_t *u){uint32_t i;if(!s)return;snapshot(s,u);for(i=0;i<ACID_SEQ_NSTEP;i++){s->step[i].code=make_code(ACID_REST,0,0,0);s->step[i].octave=0;}s->step[0].code=make_code(ACID_NOTE,0,0,0);acid_seq_normalize(s);}
