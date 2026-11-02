#pragma once
#include <stdint.h>

enum {
    BST_TRIG_AUTOGLIDE = 0,
    BST_TRIG_SINGLE = 1,
    BST_TRIG_MULTI = 2
};

typedef struct {
    uint8_t osc1_wave;       /* 0 saw, 1 pulse */
    uint8_t osc2_wave;
    uint8_t osc2_range;      /* 0..3 = 16', 8', 4', 2'; OSC1 is fixed at 8' */
    uint8_t osc_sync;        /* osc1 master -> osc2 reset */
    uint8_t osc1_pwm_source; /* 0 manual, 1 ENV2, 2 fixed PWM LFO */
    uint8_t osc2_pwm_source;
    uint8_t filter_24db;     /* 0 = 12dB, 1 = 24dB */
    uint8_t lfo_shape;       /* 0 tri, 1 rising saw, 2 S&H */
    uint8_t trigger_mode;    /* AUTOGLIDE / SINGLE / MULTI */
    uint8_t lfo_clock_div;   /* 0=free, 1..16 = MIDI-clock division */

    /* Software plug-in PW controls, normalized 0..1. MAN: centre is a 50%
       square and either side narrows toward the opposite pulse polarity.
       ENV2/LFO: 0.5=off, ends select bipolar modulation depth. */
    float osc1_pw;
    float osc2_pw;
    float osc2_semitone;     /* software plug-in: 0..+12 semitones */
    float osc2_detune;       /* semitones, software plug-in range +/-0.96 */
    float osc1_lfo_pitch;    /* semitones at full LFO */
    float osc2_lfo_pitch;
    float osc1_env_pitch;    /* semitones from ENV2 */
    float osc2_env_pitch;

    float osc_mix;          /* 0 = OSC1, 0.5 = equal, 1 = OSC2 */

    float cutoff_hz;
    float resonance;
    float filter_env_mod;    /* knob depth: bipolar in 24dB; positive-only transfer in 12dB */
    float filter_lfo_mod;    /* octave depth, bipolar */
    float filter_keytrack;   /* 24dB gentle tracking; ignored in 12dB */

    float amp_attack;
    float amp_decay;
    float amp_sustain;
    float amp_release;
    float amp_velocity;      /* 0..1 */
    float mod_attack;
    float mod_decay;
    float mod_sustain;
    float mod_release;
    float mod_velocity;      /* 0..1 */

    float lfo_rate_hz;
    float lfo_delay_s;
    float glide_s;
    float master_tune;      /* semitones, +/-0.5 */
    float bend_range;
    float wheel_pitch;       /* legacy pitch-mod depth 0..15; used as semitone LFO depth */
    float wheel_filter;      /* octave LFO depth at CC1=127 */
    float aftertouch_pitch;  /* plug-in native 0..7; Rack import may be -7..+7 */
    float aftertouch_filter; /* plug-in native 0..7; Rack import may be -7..+7 */
    float aftertouch_bias;   /* utility depth 0..15 */
    float breath_pitch;      /* plug-in native 0..7; Rack import may be -7..+7 */
    float breath_filter;     /* plug-in native 0..7; Rack import may be -7..+7 */
    float breath_bias;       /* utility depth 0..15 */
    float volume;
} bassstation_params_t;

typedef struct {
    bassstation_params_t p;
    uint8_t pressed[128];
    uint8_t sustained[128];
    uint8_t note_velocity[128];
    uint32_t age[128];
    uint32_t age_counter;
    int8_t note;
    uint8_t gate;
    uint8_t sustain;
    float velocity;
    float bend;
    float modwheel;
    float aftertouch;
    float breath;
    uint32_t midi_clock_age;
    float midi_clock_period;
    uint8_t midi_clock_seen;
    uint8_t midi_clock_running;
    /* Runtime pitch glide in Q16.16 MIDI-note units.  Keeping the ramp in
       pitch rather than Hz gives a constant semitone/CV ramp and lets the
       selected PORTA time land exactly on the destination sample. */
    int32_t target_pitch_q16;
    int32_t current_pitch_q16;
    int32_t glide_step_q16;
    int32_t glide_remainder_q16;
    int32_t glide_error_q16;
    uint32_t glide_samples_left;
    uint32_t glide_total_samples;
    uint8_t pitch_valid;
    float phase1;
    float phase2;
    float pwm_phase;
    float lfo_phase;
    float lfo_hold;
    float lfo_fade;
    uint32_t noise_state;
    float amp_env;
    float mod_env;
    uint8_t amp_stage;
    uint8_t mod_stage;
    float z1, z2, z3, z4;
    float filter_prev_in;
} bassstation_t;

void bassstation_defaults(bassstation_t *s);
void bassstation_note_on(bassstation_t *s, int note, int vel);
void bassstation_note_off(bassstation_t *s, int note);
void bassstation_note_slide(bassstation_t *s, int from_note, int to_note, int vel, float seconds);
void bassstation_sustain(bassstation_t *s, int on);
void bassstation_all_notes_off(bassstation_t *s);
void bassstation_pitch_bend(bassstation_t *s, int bend14);
void bassstation_modwheel(bassstation_t *s, int value);
void bassstation_aftertouch(bassstation_t *s, int value);
void bassstation_breath(bassstation_t *s, int value);
void bassstation_midi_clock(bassstation_t *s);
void bassstation_midi_start(bassstation_t *s);
void bassstation_midi_stop(bassstation_t *s);
void bassstation_render(bassstation_t *s, int32_t *lr, uint32_t n, uint32_t master_q12);
#ifdef BASSSTATION_TEST
float bassstation_test_lfo_step(bassstation_t *s);
float bassstation_test_env_step(float *v,uint8_t *stage,int gate,float attack,float decay,float sustain,float release);
#endif

/* Bass Station Rack 76-byte single-patch SysEx compatibility.
   Decode input excludes F0/F7 (74 bytes). Encode writes a complete F0..F7 frame. */
int bassstation_rack_sysex_decode(bassstation_params_t *p,const uint8_t *data,uint32_t n);
uint32_t bassstation_rack_sysex_encode(const bassstation_params_t *p,uint8_t out[76]);
/* Decode one 34-byte compact factory record from the classic Bass Station plug-in bank. */
int bassstation_plugin_factory_decode(bassstation_params_t *p,const uint8_t record[34]);
