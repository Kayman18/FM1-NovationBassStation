/* SPDX-License-Identifier: MPL-2.0 */
#include <stdio.h>
#include <string.h>
#include "../app/acid_seq.h"

static int bad;
static void check(const char *n,int ok){if(!ok){fprintf(stderr,"FAIL %s\n",n);bad++;}}

int main(void){
    acid_seq_t a,b;acid_seq_undo_t u;int i,notes=0;
    acid_seq_defaults(&a);acid_seq_defaults(&b);memset(&u,0,sizeof u);
    check("default length",a.steps==16);
    check("default bpm",a.bpm==120);
    check("deterministic defaults",memcmp(&a,&b,sizeof a)==0);
    for(i=0;i<64;i++){int k=acid_seq_kind(&a,(uint8_t)i);check("valid kind",k>=ACID_REST&&k<=ACID_TIE);if(k==ACID_NOTE){notes++;check("midi note",acid_seq_midi_note(&a,(uint8_t)i)>=0);}}
    check("has note",notes>0);

    check("default mut time",a.mut_time_ops==2);
    check("default mut note",a.mut_note_ops==2);
    check("default mut art",a.mut_art_ops==3);
    check("acid scale C",acid_seq_degree_semitone(&a,0)==0);
    check("acid scale Eb",acid_seq_degree_from_semitone(&a,3)==1);
    check("acid scale rejects C#",acid_seq_scale_has_semitone(&a,1)==0);

    b=a;acid_seq_mutate_time(&a,&u);check("mutation changed",memcmp(a.step,b.step,sizeof a.step)!=0);check("undo armed",u.valid!=0);acid_seq_undo(&a,&u);check("undo exact",memcmp(a.step,b.step,sizeof a.step)==0);

    a=b;a.mut_time_ops=8;acid_seq_mutate_time(&a,&u);check("variable time mutation",memcmp(a.step,b.step,sizeof a.step)!=0);acid_seq_undo(&a,&u);
    a=b;a.mut_note_ops=8;acid_seq_mutate_note(&a,&u);check("variable note mutation",memcmp(a.step,b.step,sizeof a.step)!=0);acid_seq_undo(&a,&u);
    a=b;a.mut_art_ops=8;acid_seq_mutate_art(&a,&u);check("variable art mutation",memcmp(a.step,b.step,sizeof a.step)!=0);acid_seq_undo(&a,&u);

    a=b;a.block=0;acid_seq_generate(&a,0x12345678u,&u);check("new seed",a.seed!=b.seed);check("new generation clears undo",u.valid==0);
    b=a;a.block=1;acid_seq_generate(&a,0,&u);check("block keeps seed",a.seed==b.seed);check("block generate mutates",memcmp(a.step,b.step,sizeof a.step)!=0);check("block undo armed",u.valid!=0);

    acid_seq_clear(&a,&u);notes=0;for(i=0;i<64;i++)if(acid_seq_kind(&a,(uint8_t)i)==ACID_NOTE)notes++;check("clear invariant",notes==1&&acid_seq_kind(&a,0)==ACID_NOTE);

    /* Active-pattern wrap matters for a 303 slide: with a 16-step sequence,
       step 15 slides into step 0, not the hidden step 16. */
    acid_seq_clear(&a,&u);a.steps=16;acid_seq_set_kind(&a,15,ACID_NOTE);acid_seq_set_art(&a,15,0,1);
    a.step[16].code=ACID_REST;acid_seq_normalize(&a);check("wrap slide preserved",acid_seq_slide(&a,15)!=0);
    acid_seq_set_kind(&a,0,ACID_REST);acid_seq_normalize(&a);check("wrap slide cleared into rest",acid_seq_slide(&a,15)==0);
    return bad?1:0;
}
