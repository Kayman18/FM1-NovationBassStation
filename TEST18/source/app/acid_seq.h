/* SPDX-License-Identifier: MPL-2.0 */
#pragma once
#include <stdint.h>

#define ACID_SEQ_NSTEP 64u
#define ACID_SEQ_VERSION 1u

enum { ACID_REST=0, ACID_NOTE=1, ACID_TIE=2 };
enum { ACID_CLOCK_INTERNAL=0, ACID_CLOCK_MIDI=1 };

typedef struct {
    uint8_t code;       /* bits 0..1 kind, 2..5 scale degree, 6 accent, 7 outgoing slide */
    int8_t octave;      /* -2..+2 octaves from the selected base octave */
} acid_step_t;

typedef struct {
    uint32_t version;
    uint32_t seed;
    uint32_t mutation_counter;
    uint16_t bpm;             /* internal 16th-note clock, 30..300 */
    uint8_t steps;            /* 1..64 */
    uint8_t note_density;     /* 5..100 */
    uint8_t range;            /* 0..100 */
    uint8_t root;             /* C..B, 0..11 */
    uint8_t scale;            /* 0..7 */
    uint8_t gate;             /* 5..100 percent of a step */
    uint8_t accent_density;   /* 0..100 */
    uint8_t slide_density;    /* 0..100 */
    uint8_t base_octave;      /* C1..C5 */
    uint8_t slide_time;       /* 5..100 percent of a step */
    uint8_t clock_mode;       /* ACID_CLOCK_* */
    uint8_t block;            /* seed lock */
    uint8_t reserved[3];
    acid_step_t step[ACID_SEQ_NSTEP];
} acid_seq_t;

typedef struct {
    uint8_t valid;
    uint8_t reserved[3];
    uint32_t mutation_counter;
    acid_step_t step[ACID_SEQ_NSTEP];
} acid_seq_undo_t;

void acid_seq_defaults(acid_seq_t *s);
void acid_seq_normalize(acid_seq_t *s);
void acid_seq_generate(acid_seq_t *s, uint32_t entropy, acid_seq_undo_t *undo);
void acid_seq_mutate_time(acid_seq_t *s, acid_seq_undo_t *undo);
void acid_seq_mutate_note(acid_seq_t *s, acid_seq_undo_t *undo);
void acid_seq_mutate_art(acid_seq_t *s, acid_seq_undo_t *undo);
void acid_seq_undo(acid_seq_t *s, acid_seq_undo_t *undo);
void acid_seq_clear(acid_seq_t *s, acid_seq_undo_t *undo);

int acid_seq_kind(const acid_seq_t *s, uint8_t step);
int acid_seq_degree(const acid_seq_t *s, uint8_t step);
int acid_seq_accent(const acid_seq_t *s, uint8_t step);
int acid_seq_slide(const acid_seq_t *s, uint8_t step);
int acid_seq_midi_note(const acid_seq_t *s, uint8_t step);
int acid_seq_scale_len(uint8_t scale);
const char *acid_seq_scale_name(uint8_t scale);
const char *acid_seq_root_name(uint8_t root);

void acid_seq_set_kind(acid_seq_t *s, uint8_t step, int kind);
void acid_seq_set_degree(acid_seq_t *s, uint8_t step, int degree);
void acid_seq_set_octave(acid_seq_t *s, uint8_t step, int octave);
void acid_seq_set_art(acid_seq_t *s, uint8_t step, int accent, int slide);
