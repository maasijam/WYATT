#include "../../daisy_wyatt.h"
#include "daisysp.h"
#include "core_cm7.h"
#include "utils.h"
#include "grain.h"
#include "gateInEnhanced.h"
#include "settings.h"
#include "../../Tft/ili9341_ui_driver.hpp"
#include "../../common/tft_lib.h"
#include "../../common/osc_lfo.h"


using namespace daisy;
using namespace daisysp;
using namespace wyatt;
using namespace std;
using namespace settings;
using namespace tftlib;

const char* version = "v 0.1";
const char* project_name = "Grainstorm";


enum UiMode {
  UI_MODE_START,
  UI_MODE_CALIBRATION,
  UI_MODE_RESTORE_STATE,
  UI_MODE_SAVE
};

enum gw_params
{
	NO_CV,
	GW_PITCH,
	GW_LENGTH,
	GW_POSITION,
	GW_DENSITY,
	GW_COUNT,
	GW_SPLAY,
	GW_JITTER,
    GW_SCAN_POSITION,
	GW_RND_PAN,
	GW_RND_OCT,
    GW_REVERB,
	GW_RV_TIME,
	GW_RV_DAMP,
	GW_RV_HPF,
	GW_LFO_WAVE,
	GW_LFO_SPEED,
	GW_LFO_RANGE,
    GW_WINDOW,
    GW_LFO2_WAVE,
	GW_LFO2_SPEED,
	GW_LFO2_RANGE,
    GW_IN_LVL,
    GW_OUT_LVL,
    GW_WAV_FILE,
    GW_CV1_TARGET,
    GW_CV2_TARGET,
    GW_CV3_TARGET,
    GW_CV1_ATT,
    GW_CV2_ATT,
    GW_CV3_ATT,
    GW_LFO1_TARGET,
    GW_LFO2_TARGET,
    GW_LFO1_ATT,
    GW_LFO2_ATT,
	GW_PARAMS_LAST
};

enum CharSet {
    LFO_WAVE,
    LFO_RANGE,
    CHAR_SET_LAST
};


const int RECORDING_XFADE_OVERLAP = 100; // Samples
const int RECORDING_BUFFER_SIZE = 48000 * 5; // X seconds at 48kHz
const int MIN_GRAIN_SIZE = 480; // 10 ms
const int MAX_GRAIN_SIZE = 48000 * 3; // 3 seconds
const int MAX_GRAIN_COUNT = 20;

const uint8_t MAX_SPAWN_POINTS_POT = 5;
const uint8_t MAX_SPAWN_POINTS_CV = 5;
const uint8_t MAX_SPAWN_POINTS = MAX_SPAWN_POINTS_POT + MAX_SPAWN_POINTS_CV + 2;
const int SPAWN_BAR_FLASH_MILLIS = 250;
const int SPAWN_LED_FLASH_MILLIS = 250;
const int SPAWN_TRIGGER_OUT_MILLIS = 2;

DaisyWyatt hw;
ILI9341UiDriver tft;
Settings preset;
Tft_lib      tft_lib;

// using this because issue with creating FIL objects on stack
#define DSY_TEXT __attribute__((section(".text")))

DSY_TEXT FIL            file; /**< Can't be made on the stack (DTCMRAM) */
DSY_TEXT FatFSInterface fsi;
WAV_FormatTypeDef wavHeader;
DIR dir; 
FILINFO fno;
bool fsi_loaded = false;
std::vector<std::string> filenames;
const char* err_msg = "nix";
bool loadSample = false;

GateInEnhanced spawn_gate;

ReverbSc  DSY_SDRAM_BSS   reverb;
Svf      hpf_l;
Svf      hpf_r;

Oscillator_lfo lfo[2];

#define PARAM_BUFFER_SIZE 8
char val_str[PARAM_BUFFER_SIZE];

static uint32_t const buffer_size = 153600; // 320 * 240 * 2
static uint8_t DMA_BUFFER_MEM_SECTION frame_buffer[buffer_size];


float cv[hw.CV_LAST] = {0};
int encValue[5] = {0};

float DSY_SDRAM_BSS recording[RECORDING_BUFFER_SIZE];
size_t recording_length = RECORDING_BUFFER_SIZE;
size_t write_head = 0;


const size_t RENDERABLE_RECORDING_BUFFER_SIZE = 320;
const float RECORDING_TO_RENDERABLE_RECORDING_BUFFER_RATIO = RENDERABLE_RECORDING_BUFFER_SIZE / (float)RECORDING_BUFFER_SIZE;
const float RENDERABLE_RECORDING_TO_RECORDING_BUFFER_RATIO = RECORDING_BUFFER_SIZE / (float)RENDERABLE_RECORDING_BUFFER_SIZE;
float DSY_SDRAM_BSS renderable_recording[RENDERABLE_RECORDING_BUFFER_SIZE]; // Much lower resolution, for easy rendering
size_t last_written_renderable_recording_index = 0; 

int16_t DSY_SDRAM_BSS convertBuffer[RECORDING_BUFFER_SIZE];

bool is_recording = false;
bool is_stopping_recording = false;
size_t recording_xfade_step = 0;

float spawn_position_scan_speed;
float spawn_position_offset;
float spawn_positions_splay;
float spawn_positions_count;
float pitch_shift_in_octaves;
bool primed_for_manual_spawn = true;
int grain_length;
float grain_pan;
float grain_density; // Target concurrent grains
unsigned int spawn_time; // The number of samples between each new grain
float actual_spawn_time;
float spawn_time_spread; // The variance of the spawn rate
uint32_t last_spawn_time;
float reverb_wet_mix;

int next_spawn_position_index = 0;
float spawn_position = 0.f;
float next_spawn_offset;
uint32_t samples_since_last_non_manual_spawn = 0;

Grain grains[MAX_GRAIN_COUNT];
Stack<uint8_t, MAX_GRAIN_COUNT> available_grains;

uint32_t last_oled_update_millis = 0;
uint32_t last_debug_print_millis = 0;
uint32_t last_led_update_millis = 0;
uint32_t last_spawn_time_at_position[MAX_SPAWN_POINTS] = { 0 };

void ReadSwitches();
void RenderTab1();
void RenderTab2();
void RenderTab3();
void RenderTab4();
void loadSampleToBuffer(const char* fn,const char* path);
float clamp(float x, float a, float b);
float GetCvLfoValue(int param_idx = 0);
bool IsCvControlled(int param_idx);
void SetTftParamValue(int param_idx, float param_value);
bool IsOct(int val);
void RenderWindow(int x, int y, int value);
void RenderCalibration();
float GetLfoFreq(int range, float speed);

int wav_y = 15;
int encScale_ = 1;

float getRndOct(float potvalue);
void clearMem();
bool clearMemState = false;

UiMode mode_ = UI_MODE_START;

int paramTftValue[GW_PARAMS_LAST];
int paramTftCvValue[GW_PARAMS_LAST];
int paramTftPotValue[4];
int cvSelValue_[hw.CV_LAST] = {0,0,0};
int lfoSelValue_[2] = {0,0};
int cvAttValue_[hw.CV_LAST] = {0,0,0};
int lfoAttValue_[2] = {0,0};
int paramWavVal = -1;
float cvOffsetValues_[hw.CV_LAST] = {0,0,0};
const int kNumCvParams = 18;
const char* cv_selector[kNumCvParams] = {"---","Pitch","Length","Pos","Dens","Count","Splay","Jitter","Scan","RndPan","RndOct","Reverb","RvTime","RvDamp","RvHPF","LfoWave","LfoSpeed","LfoRange"};
const char* cv_labels[hw.CV_LAST] = {"CV1","CV2","CV3"};
const char* lfo_labels[2] = {"LFO1","LFO2"};
const char* lfo_wave_labels[10] = {"SINE","TRI","SAW","RAMP","SQR","SMOO","RND","PTRI","PSAW","PSQR"};
const char* lfo_range_labels[3] = {"SLOW","MED","FAST"};

const char* page2_tab_labels[4] = {"LEVEL","REVERB","LFO1","LFO2"};

// Performance metrics.
CpuLoadMeter cpu_load_meter;
int droppedFrames = 0;

float avg_cpu_;
float min_cpu_;
float max_cpu_;

float lfo_val[2];
int oct_val;

static const size_t kFadeLengthMin = 480;
size_t kFadeLength;

// settings
bool readyToSavePreset = false; 
bool readyToRestorePreset = false;
void SavePreset();
void LoadPreset();
bool readyToSaveCalibration = false;
void LoadCalibration();
void SaveCalibration();
int calibrationLoader = 0;

const uint8_t write_head_indicator_height = 10;
const uint8_t write_head_indicator_width = 9;

class ControlHandler
{
public:
	
	float cv;
	float cvOffset;
	float knob;
	
	float value;
	int param_idx;

	ControlHandler(int idx = 0)
	{	
		param_idx = idx;

		cv = 0.0;
		cvOffset = 0.0;

		knob = 0.0;
		value = 0.0;
	}

};

ControlHandler pitch_val(GW_PITCH);
ControlHandler length_val(GW_LENGTH);
ControlHandler position_val(GW_POSITION);
ControlHandler density_val(GW_DENSITY);
ControlHandler count_val(GW_COUNT);
ControlHandler splay_val(GW_SPLAY);
ControlHandler jitter_val(GW_JITTER);
ControlHandler reverb_mix_val(GW_REVERB);
ControlHandler rnd_pan_val(GW_RND_PAN);
ControlHandler rnd_oct_val(GW_RND_OCT);
ControlHandler in_lvl_val(GW_IN_LVL);
ControlHandler out_lvl_val(GW_OUT_LVL);
ControlHandler reverb_time_val(GW_RV_TIME);
ControlHandler reverb_damp_val(GW_RV_DAMP);
ControlHandler reverb_hpf_val(GW_RV_HPF);
ControlHandler lfo_wave_val(GW_LFO_WAVE);
ControlHandler lfo_speed_val(GW_LFO_SPEED);
ControlHandler lfo_range_val(GW_LFO_RANGE);
ControlHandler scan_position_val(GW_SCAN_POSITION);


void lightenPixel(iVec2 coords, uint8_t color) {
    tft.DrawPixel(coords.x, coords.y, color);
}

inline float envelope(float t) {
    return t * t * (3.0f - 2.0f * t);
}

int clamp_int(int x, int a, int b)
{
    return std::max(a,std::min(b,x));   
}

float GetParamTftValue(int idx) {
    float pVal = paramTftValue[idx] / 100.0;
    return pVal;
};

float WindowVal(float in) { return sin(HALFPI_F * in); }


// Returns the sample offset of the nth spawn
inline size_t get_spawn_position(int index) {
    int unwrapped_spawn_position = spawn_position_offset;
                    
    // Modify the spawn position based on splay and count
    if (index >= (int)spawn_positions_count - 1) {
        unwrapped_spawn_position += spawn_positions_splay;
    } else if (index != 0) {
        unwrapped_spawn_position += index * (spawn_positions_splay / (spawn_positions_count - 2));
    }
    
    return fwrap(unwrapped_spawn_position, 0.f, recording_length);
}

void draw_recorded_waveform() {
    uint8_t last_amplitude = 0;
    // Traversing backwards stops the leading wave of recording
    // affecting values infront of it due to how the smoothing filter works

    for (int x = 320 - 1; x >= 0; x--) {
        size_t renderable_recording_index = (x / (float)320) * RENDERABLE_RECORDING_BUFFER_SIZE;

        uint8_t amplitude = min(128.f, (renderable_recording[renderable_recording_index] / 0.1f * 128) * out_lvl_val.value);

        // Smooth out the waveform
        // TODO: Smooth differently, this produces weird classic LPF shapes
        if (x > 0) {
            amplitude = amplitude * 0.4 + last_amplitude * 0.6;
        }
        last_amplitude = amplitude;

        uint8_t margin = (128 - amplitude) / 2;
        if(amplitude > 0) {

        for (uint8_t y = margin; y < 128 - margin; y++) {
            tft.DrawPixel(x, y + wav_y, COLOR_GRAY);
        }
        }
    }   
}

void draw_write_head_indicator() {
    // Write head indicator
    uint16_t write_head_screen_x = ((write_head / (float)RECORDING_BUFFER_SIZE) * 320 - 1);
    for (int x = 0; x < write_head_indicator_width; x++) {
        for (int y = 0; y < write_head_indicator_height+2; y++) {
            uint16_t screen_x = wrap(x + write_head_screen_x - write_head_indicator_width / 2, 0, 320);
            uint16_t screen_y = wrap(y + 69 +wav_y - write_head_indicator_height, 0, 128);

            if (is_recording) {
                tft.DrawPixel(screen_x, screen_y, COLOR_RED);
            } else {
                tft.DrawPixel(screen_x, screen_y, IsOct(oct_val) ? COLOR_YELLOW : COLOR_BLUE);
            }
        }
    }
    tft.DrawLine(write_head_screen_x,12+wav_y,write_head_screen_x,115+wav_y,is_recording ? COLOR_RED : IsOct(oct_val) ? COLOR_YELLOW : COLOR_BLUE);
}

void draw_grain_spawn_positions() {
    // Grain start offset
    uint8_t y_margin = 8;

    for (int i = 0; i < (int)spawn_positions_count; i++) {
        float spawn_position_x = get_spawn_position(i) / (float)recording_length * 320;
        
        for (uint8_t y = y_margin; y < 128 - y_margin; y++) {
            tft.DrawPixel(spawn_position_x, y + wav_y, COLOR_BLUE);
        }
    }
}


void draw_grains() {
    // Grains
    for (int j = 0; j < MAX_GRAIN_COUNT; j++) {
        Grain grain = grains[j];

        if (grain.step <= grain.length) {
            uint8_t y = grain.pan * 128;
            uint32_t current_offset = wrap(grain.spawn_position + grain.step * grains[j].playback_speed, 0, recording_length);
            uint16_t x = (current_offset / (float)recording_length) * 320;

            tft.DrawPixel(x, y + wav_y, COLOR_ORANGE);
            tft.DrawPixel((x + 1) % 320, y + wav_y, COLOR_CYAN);
            tft.DrawPixel((x + 2) % 320, y + wav_y, COLOR_DARK_GREEN);
        }
    }
}


// Responsible for wrapping the index
inline float get_sample(int index) {
    return recording[wrap(index, 0, recording_length)];
}

inline void record_xfaded_sample(float sample_in) {
    float xfade_magnitude = (recording_xfade_step + 1) / ((float)RECORDING_XFADE_OVERLAP + 1.f);

    recording[write_head] = lerp(
        recording[write_head],
        sample_in,
        xfade_magnitude
    );
}


void process_controls() {
    hw.ProcessAnalogControls();

    pitch_val.knob = hw.knob[hw.KNOB_1].Value();
    pitch_val.value = pitch_val.knob;
    pitch_val.cv = 0.f;
    if(IsCvControlled(GW_PITCH)) {
        pitch_val.cv = GetCvLfoValue(GW_PITCH);
        pitch_val.value = clamp(pitch_val.knob + pitch_val.cv,0.0,1.0);
        SetTftParamValue(GW_PITCH,pitch_val.value);
    }
    paramTftPotValue[0] = clamp_int(static_cast<int>(pitch_val.value*100),0,100);

    length_val.knob = hw.knob[hw.KNOB_2].Value();
    length_val.value = length_val.knob;
    length_val.cv = 0.f;
    if(IsCvControlled(GW_LENGTH)) {
        length_val.cv = GetCvLfoValue(GW_LENGTH);
        length_val.value = clamp(length_val.knob + length_val.cv,0.0,1.0);
        SetTftParamValue(GW_LENGTH,length_val.value);
    }
    paramTftPotValue[1] = clamp_int(static_cast<int>(length_val.value*100),0,100);

    position_val.knob = hw.knob[hw.KNOB_3].Value();
    position_val.value = position_val.knob;
    position_val.cv = 0.f;
    if(IsCvControlled(GW_POSITION)) {
        position_val.cv = GetCvLfoValue(GW_POSITION);
        position_val.value = clamp(position_val.knob + position_val.cv,0.0,1.0);
        SetTftParamValue(GW_POSITION,position_val.value);
    }
    paramTftPotValue[2] = clamp_int(static_cast<int>(position_val.value*100),0,100);

    density_val.knob = hw.knob[hw.KNOB_4].Value();
    density_val.value = density_val.knob;
    density_val.cv = 0.f;
    if(IsCvControlled(GW_DENSITY)) {
        density_val.cv = GetCvLfoValue(GW_DENSITY);
        density_val.value = clamp(density_val.knob + density_val.cv,0.0,1.0);
        SetTftParamValue(GW_DENSITY,density_val.value);
    }
    paramTftPotValue[3] = clamp_int(static_cast<int>(density_val.value*100),0,100);


    jitter_val.knob = GetParamTftValue(GW_JITTER);
    jitter_val.value = jitter_val.knob;
    jitter_val.cv = 0.f;
    if(IsCvControlled(GW_JITTER)) {
        jitter_val.cv = GetCvLfoValue(GW_JITTER);
        jitter_val.value = clamp(jitter_val.knob + jitter_val.cv,0.0,1.0);
        SetTftParamValue(GW_JITTER,jitter_val.value);
    }

    splay_val.knob = GetParamTftValue(GW_SPLAY);
    splay_val.value = splay_val.knob;
    splay_val.cv = 0.f;
    if(IsCvControlled(GW_SPLAY)) {
        splay_val.cv = GetCvLfoValue(GW_SPLAY);
        splay_val.value = clamp(splay_val.knob + splay_val.cv,0.0,1.0);
        SetTftParamValue(GW_SPLAY,splay_val.value);
    }

    count_val.knob = GetParamTftValue(GW_COUNT);
    count_val.value = count_val.knob;
    count_val.cv = 0.f;
    if(IsCvControlled(GW_COUNT)) {
        count_val.cv = GetCvLfoValue(GW_COUNT);
        count_val.value = clamp(count_val.knob + count_val.cv,0.0,1.0);
        SetTftParamValue(GW_COUNT,count_val.value);
    }

    reverb_mix_val.knob = GetParamTftValue(GW_REVERB);
    reverb_mix_val.value = reverb_mix_val.knob;
    reverb_mix_val.cv = 0.f;
    if(IsCvControlled(GW_REVERB)) {
        reverb_mix_val.cv = GetCvLfoValue(GW_REVERB);
        reverb_mix_val.value = clamp(reverb_mix_val.knob + reverb_mix_val.cv,0.0,1.0);
        SetTftParamValue(GW_REVERB,reverb_mix_val.value);
    }

    reverb_time_val.knob = GetParamTftValue(GW_RV_TIME);
    reverb_time_val.value = reverb_time_val.knob;
    reverb_time_val.cv = 0.f;
    if(IsCvControlled(GW_RV_TIME)) {
        reverb_time_val.cv = GetCvLfoValue(GW_RV_TIME);
        reverb_time_val.value = clamp(reverb_time_val.knob + reverb_time_val.cv,0.0,1.0);
        SetTftParamValue(GW_RV_TIME,reverb_time_val.value);
    }

    reverb_damp_val.knob = GetParamTftValue(GW_RV_DAMP);
    reverb_damp_val.value = reverb_damp_val.knob;
    reverb_damp_val.cv = 0.f;
    if(IsCvControlled(GW_RV_DAMP)) {
        reverb_damp_val.cv = GetCvLfoValue(GW_RV_DAMP);
        reverb_damp_val.value = clamp(reverb_damp_val.knob + reverb_damp_val.cv,0.0,1.0);
        SetTftParamValue(GW_RV_DAMP,reverb_damp_val.value);
    }

    reverb_hpf_val.knob = GetParamTftValue(GW_RV_HPF);
    reverb_hpf_val.value = reverb_hpf_val.knob;
    reverb_hpf_val.cv = 0.f;
    if(IsCvControlled(GW_RV_HPF)) {
        reverb_hpf_val.cv = GetCvLfoValue(GW_RV_HPF);
        reverb_hpf_val.value = clamp(reverb_hpf_val.knob + reverb_hpf_val.cv,0.0,1.0);
        SetTftParamValue(GW_RV_HPF,reverb_hpf_val.value);
    }


    rnd_pan_val.knob = GetParamTftValue(GW_RND_PAN);
    rnd_pan_val.value = rnd_pan_val.knob;
    rnd_pan_val.cv = 0.f;
    if(IsCvControlled(GW_RND_PAN)) {
        rnd_pan_val.cv = GetCvLfoValue(GW_RND_PAN);
        rnd_pan_val.value = clamp(rnd_pan_val.knob + rnd_pan_val.cv,0.0,1.0);
        SetTftParamValue(GW_RND_PAN,rnd_pan_val.value);
    }

    rnd_oct_val.knob = GetParamTftValue(GW_RND_OCT);
    rnd_oct_val.value = rnd_oct_val.knob;
    rnd_oct_val.cv = 0.f;
    if(IsCvControlled(GW_RND_OCT)) {
        rnd_oct_val.cv = GetCvLfoValue(GW_RND_OCT);
        rnd_oct_val.value = clamp(rnd_oct_val.knob + rnd_oct_val.cv,0.0,1.0);
        SetTftParamValue(GW_RND_OCT,rnd_oct_val.value);
    }

    in_lvl_val.knob = GetParamTftValue(GW_IN_LVL);
    in_lvl_val.value = in_lvl_val.knob;
    in_lvl_val.cv = 0.f;
    if(IsCvControlled(GW_IN_LVL)) {
        in_lvl_val.cv = GetCvLfoValue(GW_IN_LVL);
        in_lvl_val.value = clamp(in_lvl_val.knob + in_lvl_val.cv,0.0,1.0);
        SetTftParamValue(GW_IN_LVL,in_lvl_val.value);
    }

    out_lvl_val.knob = GetParamTftValue(GW_OUT_LVL);
    out_lvl_val.value = out_lvl_val.knob;
    out_lvl_val.cv = 0.f;
    if(IsCvControlled(GW_OUT_LVL)) {
        out_lvl_val.cv = GetCvLfoValue(GW_OUT_LVL);
        out_lvl_val.value = clamp(out_lvl_val.knob + out_lvl_val.cv,0.0,1.0);
        SetTftParamValue(GW_OUT_LVL,out_lvl_val.value);
    }

    scan_position_val.knob = GetParamTftValue(GW_SCAN_POSITION);
    scan_position_val.value = scan_position_val.knob;
    scan_position_val.cv = 0.f;
    if(IsCvControlled(GW_SCAN_POSITION)) {
        scan_position_val.cv = GetCvLfoValue(GW_SCAN_POSITION);
        scan_position_val.value = clamp(scan_position_val.knob + scan_position_val.cv,0.0,1.0);
        SetTftParamValue(GW_SCAN_POSITION,scan_position_val.value);
    }
   
    lfo[0].SetFreq(GetLfoFreq(paramTftValue[GW_LFO_RANGE],GetParamTftValue(GW_LFO_SPEED)));
    lfo[0].SetAmp(GetParamTftValue(GW_LFO1_ATT));
    lfo[0].SetWaveform(paramTftValue[GW_LFO_WAVE]);

    lfo[1].SetFreq(GetLfoFreq(paramTftValue[GW_LFO2_RANGE],GetParamTftValue(GW_LFO2_SPEED)));
    lfo[1].SetAmp(GetParamTftValue(GW_LFO2_ATT));
    lfo[1].SetWaveform(paramTftValue[GW_LFO2_WAVE]);

    lfo_val[0] = lfo[0].Process();
    lfo_val[1] = lfo[1].Process();

    // Scan speed
    spawn_position_scan_speed = with_dead_zone(
        position_val.cv + map_to_range(position_val.knob, 1, -1) * abs(map_to_range(position_val.knob, 1, -1)),
        0.02f
    );

    // Splay
    // Deadzone at 0 to the spawn position splay
    float splay_value = with_dead_zone(
        splay_val.cv + map_to_range(splay_val.knob, 1, -1) * abs(map_to_range(splay_val.knob, 1, -1)) * 0.5f,
        0.1f
    );
    spawn_positions_splay = splay_value * recording_length;
     
    // Count
    spawn_positions_count = 2.f // This should technically be 1 if splay is 0, but it simpler if we just pretend there's always 2
            + 0.7f // Start precocked so it doesn't take much pot twiddling to see the 3rd spawn point
            + map_to_range(count_val.knob, 0, MAX_SPAWN_POINTS_POT) 
            + map_to_range(count_val.cv, 0, MAX_SPAWN_POINTS_CV);
    // Make sure it doesn't dip below 2.7 due to negative cv values
    spawn_positions_count = max(spawn_positions_count, 2.7f);

    // Pitch
    pitch_shift_in_octaves = with_dead_zone(
        map_to_range(pitch_val.knob, -2.1, 2.1),
        0.1f
    ) + pitch_val.cv * 5;
    oct_val = static_cast<int>(pitch_shift_in_octaves*100.0);

    pitch_shift_in_octaves += getRndOct(map_to_range(rnd_oct_val.value, 0, 1));

    // Length
    float grain_length_control = coerce_in_range(length_val.cv + length_val.knob * 2 - 1, -1, 1);
    grain_length = map_to_range(pow(abs(grain_length_control), 2), MIN_GRAIN_SIZE, MAX_GRAIN_SIZE);
    if (grain_length_control < 0) {
        grain_length = -grain_length;
    }

    // Density
    float density_control = coerce_in_range(density_val.value, 0, 1);
    
    grain_density = map_to_range(pow(density_control, 2), 0.5f, MAX_GRAIN_COUNT);
    spawn_time = abs(grain_length) / grain_density;
 

    // Jitter
    spawn_time_spread = jitter_val.value;
    if (density_control <= 0.001) {
        actual_spawn_time = INFINITY;
    } else {
        actual_spawn_time = spawn_time * (1 + next_spawn_offset * spawn_time_spread);
    }

    // Pan
    grain_pan = 0.5f + (randF(-0.5f, 0.5f) * map_to_range(rnd_pan_val.value, 0, 1));

    // Reverb
    float reverb_amount = max(0.f, reverb_mix_val.knob + reverb_mix_val.cv);
    reverb_wet_mix = fmap(reverb_amount, 0.f, 0.99f);

    float reverb_hpf = fmap(reverb_hpf_val.value, 20.f, 5000.f);
    hpf_l.SetFreq(reverb_hpf);
    hpf_r.SetFreq(reverb_hpf);
    
    /** Update Params with the four knobs */
    float time_knob = reverb_time_val.value;
    float reverb_time      = fmap(time_knob, 0.4f, 0.99f);

    float damp_knob = reverb_damp_val.value;
    float reverb_damp = fmap(damp_knob, 100.f, 24000.f, Mapping::LOG);

    reverb.SetFeedback(reverb_time);
    reverb.SetLpFreq(reverb_damp);
        
}

void spawn_grain() {
    size_t new_grain_index = available_grains.PopBack();

    grains[new_grain_index].length = abs(grain_length);
    grains[new_grain_index].step = 0;
    grains[new_grain_index].pan = grain_pan;
    
    grains[new_grain_index].spawn_position_index = next_spawn_position_index;
    grains[new_grain_index].spawn_position = get_spawn_position(next_spawn_position_index);

    grains[new_grain_index].spawn_time_millis = System::GetNow();
    last_spawn_time = grains[new_grain_index].spawn_time_millis;

    grains[new_grain_index].pitch_shift_in_octaves = pitch_shift_in_octaves;

    // Reverse the playback if the length is negative
    if (grain_length > 0) {
        grains[new_grain_index].playback_speed = pow(2, pitch_shift_in_octaves);
    } else {
        grains[new_grain_index].playback_speed = -pow(2, pitch_shift_in_octaves);
    }

    next_spawn_offset = randF(-1.f, 1.f); // +/- 100%
    next_spawn_position_index = wrap(next_spawn_position_index + 1, 0, (int)spawn_positions_count);
}

void record_sample(float sample) {
    if (is_stopping_recording) {
        // Record a little extra at the end of the recording so we can xfade the values
        // and stop the pop sound
        record_xfaded_sample(sample);

        recording_xfade_step--;

        if (recording_xfade_step == 0) {
            is_recording = false;
            is_stopping_recording = false;
        }
    } else if (recording_xfade_step < RECORDING_XFADE_OVERLAP) {
        // xfade in the start of the recording to stop the pop sound
        record_xfaded_sample(sample);

        recording_xfade_step++;
    } else {
        recording[write_head] = sample;
    }
}

void record_sample_for_display(float sample) {
    // TODO: Record positive and negative values seperately
    size_t renderable_recording_index = write_head * RECORDING_TO_RENDERABLE_RECORDING_BUFFER_RATIO;

    // Clear out the element when we first start writing fresh values to it
    if (write_head == 0 || renderable_recording_index > last_written_renderable_recording_index) {
        renderable_recording[renderable_recording_index] = 0;
    }

    // Downsample the samples into renderable_recording_index by averaging them
    renderable_recording[renderable_recording_index] += abs(sample) / RENDERABLE_RECORDING_TO_RECORDING_BUFFER_RATIO;
    last_written_renderable_recording_index = renderable_recording_index;
    
    if (recording_length < RECORDING_BUFFER_SIZE) {
        recording_length++;
    }
}

void increment_write_head() {
    write_head++;

    if (write_head >= RECORDING_BUFFER_SIZE) {
        write_head = 0;
    }
}

void calculate_audio_out(float in_l, float in_r, float &out_l, float &out_r) {
    float wet_l = 0.f;
    float wet_r = 0.f;

    for (int j = 0; j < MAX_GRAIN_COUNT; j++) {
        if (is_alive(grains[j])) {
            size_t buffer_index = grains[j].spawn_position + grains[j].step * grains[j].playback_speed;

            // playback_speed is a float so we need to interpolate between samples
            float sample = get_sample(buffer_index);
            float next_sample = get_sample(buffer_index + 1);

            float decimal_portion = modf(grains[j].step * grains[j].playback_speed);
            float interpolated_sample = sample * (1 - decimal_portion) + next_sample * decimal_portion;

            //float signal = 0.f;
            float vol = 0.5f;
            kFadeLength = ((((grains[j].length / 2) - kFadeLengthMin) * (100-paramTftValue[GW_WINDOW]))/100) + kFadeLengthMin; 
            if (grains[j].step <= kFadeLength) {
                vol = (grains[j].step / float(kFadeLength)) * 0.5f;
            }
            int from_end = abs(grains[j].length - grains[j].step);
            if (from_end <= kFadeLength) {
                vol = (from_end / float(kFadeLength)) * 0.5f;
            }
            float signal = interpolated_sample * envelope(vol);

            wet_l += (1.f - grains[j].pan) * signal;
            wet_r += grains[j].pan * signal;

            grains[j].step++;

            if (grains[j].step > grains[j].length) {
                available_grains.PushBack(j);
            }
        }
    }

    float reverb_in_l = wet_l;
    float reverb_in_r = wet_r;
    float reverb_wet_l, reverb_wet_r;
    reverb.Process(reverb_in_l, reverb_in_r, &reverb_wet_l, &reverb_wet_r);
    hpf_l.Process(reverb_wet_l);
    hpf_r.Process(reverb_wet_r);
    out_l = (reverb_in_l + (hpf_l.High() * reverb_wet_mix) + (in_l * in_lvl_val.value)) * out_lvl_val.value;
    out_r = (reverb_in_r + (hpf_r.High() * reverb_wet_mix) + (in_r * in_lvl_val.value)) * out_lvl_val.value;


}

void init() {
    
    // Populate available grains stack
    for (u_int8_t i = 0; i < MAX_GRAIN_COUNT; i++) {
        available_grains.PushBack(i);
    }

    // Clear the buffers
    clearMem();

    cpu_load_meter.Init(hw.AudioSampleRate(), hw.AudioBlockSize());
    reverb.Init(hw.AudioSampleRate());
    hpf_l.Init(hw.AudioSampleRate());
    hpf_l.SetFreq(400.0);
    hpf_l.SetRes(0);
    hpf_l.SetDrive(0);
    hpf_r.Init(hw.AudioSampleRate());
    hpf_r.SetFreq(400.0);
    hpf_r.SetRes(0);
    hpf_r.SetDrive(0);

    paramTftValue[GW_WAV_FILE] = paramWavVal;
    
}


void AudioCallback(
    AudioHandle::InputBuffer  in,
    AudioHandle::OutputBuffer out,
    size_t size
) {
    cpu_load_meter.OnBlockStart();
	droppedFrames++;

    process_controls();

    // Process audio output
    for(size_t i = 0; i < size; i++)
    {
        OUT_L[i] = 0.f;
        OUT_R[i] = 0.f;
        
        if(!loadSample) {
        
            if (is_recording) {
                record_sample(IN_L[i] * in_lvl_val.value);
                record_sample_for_display(IN_L[i] * in_lvl_val.value);
            } 

            // Progresses the write head regardless of if we're recording
            increment_write_head();
            
            // Work out if we need to spawn a grain this sample
            samples_since_last_non_manual_spawn++;

            bool has_spawn_timer_elapsed = actual_spawn_time != INFINITY 
                    && samples_since_last_non_manual_spawn >= actual_spawn_time;
            bool should_manual_spawn = (spawn_gate.RisingEdge()) && primed_for_manual_spawn;

            // Spawn grains
            if ((has_spawn_timer_elapsed || should_manual_spawn) && !available_grains.IsEmpty()) {
                spawn_grain();

                if (has_spawn_timer_elapsed) {
                    samples_since_last_non_manual_spawn = 0;
                }

                if (should_manual_spawn) {
                    primed_for_manual_spawn = false;
                }
            }

            calculate_audio_out(IN_L[i], IN_L[i], OUT_L[i], OUT_R[i]);
            if(spawn_position_scan_speed != 0) {
                spawn_position_offset += spawn_position_scan_speed;
            } else {
                spawn_position_offset = map_to_range(scan_position_val.value, 0.f, RECORDING_BUFFER_SIZE);
            }
            
            spawn_position_offset = fwrap(spawn_position_offset, 0, RECORDING_BUFFER_SIZE);
            
        }
    }
    
    cpu_load_meter.OnBlockEnd();
	droppedFrames--;
}

char* GetIntAsString_(int val) { 
        snprintf(val_str, PARAM_BUFFER_SIZE, "%d", val);
        return val_str; 
    }

void StoreFileNames(const char* path)
{
    FRESULT fr;
    fr = f_opendir(&dir, path);
    if (fr != FR_OK) {
        // Handle error (e.g., print to serial)
        err_msg = "Could not open dir";
        return;
    }
    for (;;) {
        // Read a directory item
        fr = f_readdir(&dir, &fno);
        if (fr != FR_OK || fno.fname[0] == 0) break; // Error or end of directory

        // Skip hidden files and directories that start with a dot
        if (fno.fname[0] == '.') continue;

        if (fno.fattrib & AM_DIR) {
            
        } else {
            // It is a file
            if(strstr(fno.fname, ".wav") || strstr(fno.fname, ".WAV"))
            {
            filenames.push_back(fno.fname);
            err_msg = "Filename saved";
            }
        }
    }

    // Close the directory
    f_closedir(&dir);
}


int main(void)
{
        
    hw.Init(true);
    tft.Init(frame_buffer);
    tft_lib.Init(&tft);
    
    if(tft.IsRender())
        {
            tft.Fill(COLOR_BLACK);
            tft_lib.RenderSplash(project_name,version);
            tft.Update();
        }
    
    hw.DelayMs(1500);
    preset.Init(&hw);
    LoadCalibration();
    LoadPreset();
    
    init();


     /** SD card next */
    SdmmcHandler::Config sd_config;
    SdmmcHandler         sdcard;
    sd_config.Defaults();
    sd_config.speed           = daisy::SdmmcHandler::Speed::FAST; 
    sd_config.width           = daisy::SdmmcHandler::BusWidth::BITS_4;
    sdcard.Init(sd_config);

    fsi.Init(FatFSInterface::Config::MEDIA_SD);
    FATFS& fs = fsi.GetSDFileSystem();
    f_mount(&fs, "0:", 1);

    hw.DelayMs(100);
    StoreFileNames("/samples");
    
    for (size_t i = 0; i < 2; i++)
    {
        lfo[i].Init(hw.AudioSampleRate());
        lfo[i].SetFreq(400.f);
        lfo[i].SetAmp(1.f);
        lfo[i].SetWaveform(lfo[i].WAVE_TRI);
    }
        
    //Start the adc
    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    uint32_t last_save_time = System::GetNow(); 
    uint32_t last_load_sample_time = System::GetNow(); 
    while (1) 
    {
                
        ReadSwitches();

        hw.SetLed(hw.LED_RED, is_recording ? true : false);
        hw.UpdateLeds();
        
        avg_cpu_ = cpu_load_meter.GetAvgCpuLoad();
        min_cpu_ = cpu_load_meter.GetMinCpuLoad();
        max_cpu_ = cpu_load_meter.GetMaxCpuLoad();

        if(tft.IsRender())
        {
            tft.Fill(COLOR_BLACK);

            switch (mode_)
            {
            case UI_MODE_START:
                RenderTab1();
                RenderTab2();
                RenderTab3();
                RenderTab4();
            break;
            case UI_MODE_SAVE:
                tft_lib.RenderSavePreset();
            break;
            case UI_MODE_RESTORE_STATE:
                tft_lib.RenderRestorePreset();
            break;
            case UI_MODE_CALIBRATION:
                RenderCalibration();
            break;
            default:
                break;
            }

            tft.Update();
        }

        if (System::GetNow() - last_load_sample_time > 100 && loadSample)
        {
            //err_msg = filenames[paramWavVal].c_str();
            loadSampleToBuffer(filenames[paramWavVal].c_str(),"/samples");  
            loadSample = false;
            
        }

        if (System::GetNow() - last_save_time > 100 && readyToSavePreset)
        {
          SavePreset();
          last_save_time = System::GetNow();
          readyToSavePreset = false;
        }
        if (readyToRestorePreset) {
            preset.RestoreTheSettings();
            readyToRestorePreset = false;
        }
        if (readyToSaveCalibration)
        {
          SaveCalibration();
          readyToSaveCalibration = false;
        }

        hw.DelayMs(1);
        
    }
}

int map_(int x, int in_min, int in_max, int out_min, int out_max) {
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }


void RenderWindow(int x, int y, int value) 
{
    int lineLeft = map_(value,0,100,60,15);
    int lineRight = map_(value,0,100,60,105);

    tft.DrawRect(x,y,120,70,COLOR_WHITE);
    
    tft.DrawLine(x+1,y+69,x+lineLeft,y+5,COLOR_BLUE);
    tft.DrawLine(x+lineRight,y+5,x+119,y+69,COLOR_BLUE);
    if(value > 0) {
        tft.DrawLine(x+lineLeft,y+5,x+lineRight,y+5,COLOR_BLUE);
    }
}


void RenderTab1()
{
    if(tft_lib.tab == 0) {
        tft_lib.RenderTabRect(tft_lib.tab);
        tft.DrawLine(0,65+wav_y,320,65+wav_y,COLOR_GRAY);
        draw_recorded_waveform();
        draw_write_head_indicator();
        draw_grain_spawn_positions();
        draw_grains();

        tft_lib.RenderParam(0,0,3,IsCvControlled(GW_COUNT) ? paramTftCvValue[GW_COUNT] : paramTftValue[GW_COUNT], "COUNT");
        tft_lib.RenderParam(1,0,3,IsCvControlled(GW_SPLAY) ? paramTftCvValue[GW_SPLAY] : paramTftValue[GW_SPLAY], "SPLAY");
        tft_lib.RenderParam(2,0,3,IsCvControlled(GW_JITTER) ? paramTftCvValue[GW_JITTER] : paramTftValue[GW_JITTER], "JITTER");
        tft_lib.RenderParam(3,0,3,IsCvControlled(GW_SCAN_POSITION) ? paramTftCvValue[GW_SCAN_POSITION] : paramTftValue[GW_SCAN_POSITION], "SCAN");
        tft_lib.RenderAreaInd(0,3);

        tft_lib.RenderParam(0,1,3,IsCvControlled(GW_RND_PAN) ? paramTftCvValue[GW_RND_PAN] : paramTftValue[GW_RND_PAN], "RND PAN");
        tft_lib.RenderParam(1,1,3,IsCvControlled(GW_RND_OCT) ? paramTftCvValue[GW_RND_OCT] : paramTftValue[GW_RND_OCT], "RND OCT");
        tft_lib.RenderParam(2,1,3,IsCvControlled(GW_REVERB) ? paramTftCvValue[GW_REVERB] : paramTftValue[GW_REVERB], "REVERB");
        tft_lib.RenderAreaInd(1,3);   
            
    }
}
void RenderTab2()
{
    if(tft_lib.tab == 1) {
        tft_lib.RenderTabRect(tft_lib.tab);
        
        tft_lib.RenderRowTabHeader(tft_lib.area[tft_lib.tab],page2_tab_labels,4);

        tft_lib.RenderParam(0,0,1,paramTftValue[GW_IN_LVL], "IN LVL");
        tft_lib.RenderParam(1,0,1,paramTftValue[GW_OUT_LVL], "OUT LVL");
        tft_lib.RenderAreaInd(0,1);
        
        tft_lib.RenderParam(0,1,1,IsCvControlled(GW_RV_TIME) ? paramTftCvValue[GW_RV_TIME] : paramTftValue[GW_RV_TIME], "TIME");
        tft_lib.RenderParam(1,1,1,IsCvControlled(GW_RV_DAMP) ? paramTftCvValue[GW_RV_DAMP] : paramTftValue[GW_RV_DAMP], "DAMP");
        tft_lib.RenderParam(2,1,1,IsCvControlled(GW_RV_HPF) ? paramTftCvValue[GW_RV_HPF] : paramTftValue[GW_RV_HPF], "HPF");
        tft_lib.RenderAreaInd(1,1);

        tft_lib.RenderParamChar(0,2,1,IsCvControlled(GW_LFO_WAVE) ? paramTftCvValue[GW_LFO_WAVE] : paramTftValue[GW_LFO_WAVE], "WAVE",lfo_wave_labels);
        tft_lib.RenderParam(1,2,1,IsCvControlled(GW_LFO_SPEED) ? paramTftCvValue[GW_LFO_SPEED] : paramTftValue[GW_LFO_SPEED], "SPEED");
        tft_lib.RenderParamChar(2,2,1,IsCvControlled(GW_LFO_RANGE) ? paramTftCvValue[GW_LFO_RANGE] : paramTftValue[GW_LFO_RANGE], "RANGE",lfo_range_labels);
        tft_lib.RenderAreaInd(2,1);

        tft_lib.RenderParamChar(0,3,1,paramTftValue[GW_LFO2_WAVE], "WAVE",lfo_wave_labels);
        tft_lib.RenderParam(1,3,1,paramTftValue[GW_LFO2_SPEED], "SPEED");
        tft_lib.RenderParamChar(2,3,1,paramTftValue[GW_LFO2_RANGE], "RANGE",lfo_range_labels);
        tft_lib.RenderAreaInd(3,1);

    }
}
void RenderTab3()
{
    if(tft_lib.tab == 2) {
        tft_lib.RenderTabRect(tft_lib.tab);
        tft.WriteString("WINDOW",20,20,Font_7x10,COLOR_WHITE);
        RenderWindow(20,35,paramTftValue[GW_WINDOW]);
        tft_lib.RenderAreaInd(0,0,49);
        

        tft.WriteString("CPU-AVG:",15,220,Font_7x10,COLOR_GRAY);
        tft.WriteString(GetIntAsString_(static_cast<int>(avg_cpu_*100.0)),70,220,Font_11x18,COLOR_GRAY);
        tft.WriteString("CPU-MIN:",115,220,Font_7x10,COLOR_GRAY);
        tft.WriteString(GetIntAsString_(static_cast<int>(min_cpu_*100.0)),170,220,Font_11x18,COLOR_GRAY);
        tft.WriteString("CPU-MAX:",215,220,Font_7x10,COLOR_GRAY);
        tft.WriteString(GetIntAsString_(static_cast<int>(max_cpu_*100.0)),270,220,Font_11x18,COLOR_GRAY);

        
    }
}
void RenderTab4()
{
    if(tft_lib.tab == 3) {
        tft_lib.RenderTabRect(tft_lib.tab);

        tft_lib.RenderWavSelector(0,0,0,paramTftValue[GW_WAV_FILE],"WAV",filenames);
        tft_lib.RenderAreaInd(0,0);
        
        tft_lib.RenderCvSelector(0,1,0,paramTftValue[GW_CV1_TARGET],paramTftValue[GW_CV1_ATT],"CV1",cv_selector);
        tft_lib.RenderCvSelector(2,1,0,paramTftValue[GW_CV2_TARGET],paramTftValue[GW_CV2_ATT],"CV2",cv_selector);
        tft_lib.RenderAreaInd(1,0);
        
        tft_lib.RenderCvSelector(0,2,0,paramTftValue[GW_CV3_TARGET],paramTftValue[GW_CV3_ATT],"CV3",cv_selector);
        tft_lib.RenderAreaInd(2,0);
        
        tft_lib.RenderCvSelector(0,3,0,paramTftValue[GW_LFO1_TARGET],paramTftValue[GW_LFO1_ATT],"LFO1",cv_selector);
        tft_lib.RenderCvSelector(2,3,0,paramTftValue[GW_LFO2_TARGET],paramTftValue[GW_LFO2_ATT],"LFO2",cv_selector);
        tft_lib.RenderAreaInd(3,0);

        tft_lib.RenderPotParam(0,paramTftPotValue[0],"PITCH");
        tft_lib.RenderPotParam(1,paramTftPotValue[1],"LENGTH");
        tft_lib.RenderPotParam(2,paramTftPotValue[2],"POSITION");
        tft_lib.RenderPotParam(3,paramTftPotValue[3],"DENSITY");
        
    }
}

float getRndOct(float potvalue) {
    if(potvalue < 0.05) {
        return 0;
    } else if(potvalue >= 0.05 && potvalue < 0.25) {
        float oct[2] = {0,1};
        return oct[rand() % 1];
    } else if(potvalue >= 0.25 && potvalue < 0.5) {
        float oct[3] = {0,1,-1};
        return oct[rand() % 2];
    } else if(potvalue >= 0.5 && potvalue < 0.75) {
        float oct[3] = {0,1,2};
        return oct[rand() % 2];
    } else if(potvalue >= 0.75) {
        float oct[5] = {0,1,-1,2,-2};
        return oct[rand() % 4];
    } else {
        return 0;
    }
} 

void clearMem() {
    // Clear the buffers
    memset(renderable_recording, 0, sizeof(renderable_recording));
    memset(recording, 0, sizeof(recording));
}

void ReadSwitches()
{
    hw.ProcessDigitalControls(encScale_);
    
    switch (mode_) {
      case UI_MODE_START:
        
        if(hw.SwitchRisingEdge(hw.S_TOP1)) {
            tft_lib.tab = 0;
        }
        if(hw.SwitchRisingEdge(hw.S_TOP2)) {
            tft_lib.tab = 1;
        }
        if(hw.SwitchRisingEdge(hw.S_TOP3)) {
            tft_lib.tab = 2;
        }
        if(hw.SwitchRisingEdge(hw.S_TOP4)) {
            tft_lib.tab = 3;
        }
        

        if(hw.SwitchRisingEdge(hw.S_PAGE_UP)) {
            switch (tft_lib.tab)
            {
            case 0:
                tft_lib.area[tft_lib.tab] -= 1;
                if(tft_lib.area[tft_lib.tab] < 0) {
                    tft_lib.area[tft_lib.tab] = 1;
                }
                break;
            case 1:
                tft_lib.area[tft_lib.tab] -= 1;
                if(tft_lib.area[tft_lib.tab] < 0) {
                    tft_lib.area[tft_lib.tab] = 3;
                }
                break;
            case 2:
            break;
            case 3:
                tft_lib.area[tft_lib.tab] -= 1;
                if(tft_lib.area[tft_lib.tab] < 0) {
                    tft_lib.area[tft_lib.tab] = 3;
                }
                break;
            default:
                break;
            }
            
        }
        if(hw.SwitchRisingEdge(hw.S_PAGE_DOWN)) {
            switch (tft_lib.tab)
            {
            case 0:
                tft_lib.area[tft_lib.tab] += 1;
                if(tft_lib.area[tft_lib.tab] > 1) {
                    tft_lib.area[tft_lib.tab] = 0;
                }
                break;
            case 1:
                tft_lib.area[tft_lib.tab] += 1;
                if(tft_lib.area[tft_lib.tab] > 3) {
                    tft_lib.area[tft_lib.tab] = 0;
                }
                break;
            case 3:
                tft_lib.area[tft_lib.tab] += 1;
                if(tft_lib.area[tft_lib.tab] > 3) {
                    tft_lib.area[tft_lib.tab] = 0;
                }
                break;
            default:
                break;
            }
        }
        // Encoder values
        for (size_t i = 0; i < 4; i++)
        {
            switch (tft_lib.tab)
            {
            case 0:
                if(tft_lib.area[tft_lib.tab] == 0) {
                    paramTftValue[i+5] += hw.enc[i].Increment();
                    paramTftValue[i+5] = clamp_int(paramTftValue[i+5],0,100);                    
                } else if(tft_lib.area[tft_lib.tab] == 1 && i < 3) {
                    paramTftValue[i+9] += hw.enc[i].Increment();
                    paramTftValue[i+9] = clamp_int(paramTftValue[i+9],0,100);
                } 
                break;
            case 1:
                if(tft_lib.area[tft_lib.tab] == 0 && i < 2) {
                    paramTftValue[i+GW_IN_LVL] += hw.enc[i].Increment();
                    paramTftValue[i+GW_IN_LVL] = clamp_int(paramTftValue[i+GW_IN_LVL],0,100);
                } else if(tft_lib.area[tft_lib.tab] == 1 && i < 3) {
                    paramTftValue[i+GW_RV_TIME] += hw.enc[i].Increment();
                    paramTftValue[i+GW_RV_TIME] = clamp_int(paramTftValue[i+GW_RV_TIME],0,100);
                } else if(tft_lib.area[tft_lib.tab] == 2 && i < 3) {
                    paramTftValue[i+GW_LFO_WAVE] += hw.enc[i].Increment();
                    if(i == 0) {
                        paramTftValue[GW_LFO_WAVE] = clamp_int(paramTftValue[GW_LFO_WAVE],0,9);
                    } else if(i == 2) {
                        paramTftValue[GW_LFO_RANGE] = clamp_int(paramTftValue[GW_LFO_RANGE],0,2);
                    } else {
                        paramTftValue[i+GW_LFO_WAVE] = clamp_int(paramTftValue[i+GW_LFO_WAVE],0,100);
                    }
                    
                } else if(tft_lib.area[tft_lib.tab] == 3 && i < 3) {
                    paramTftValue[i+GW_LFO2_WAVE] += hw.enc[i].Increment();
                    if(i == 0) {
                        paramTftValue[GW_LFO2_WAVE] = clamp_int(paramTftValue[GW_LFO2_WAVE],0,9);
                    } else if(i == 2) {
                        paramTftValue[GW_LFO2_RANGE] = clamp_int(paramTftValue[GW_LFO2_RANGE],0,2);
                    } else {
                        paramTftValue[i+GW_LFO2_WAVE] = clamp_int(paramTftValue[i+GW_LFO2_WAVE],0,100);
                    }
                    
                }
                break;
            case 2:
                if(tft_lib.area[tft_lib.tab] == 0 && i == 0) {
                    paramTftValue[GW_WINDOW] += hw.enc[i].Increment();
                    paramTftValue[GW_WINDOW] = clamp_int(paramTftValue[GW_WINDOW],0,100);
                }
                
                break;
            case 3:
                if(tft_lib.area[tft_lib.tab] == 0) {
                    if(i == 0) {
                        paramWavVal += hw.enc[i].Increment();
                        paramWavVal = clamp_int(paramWavVal,-1,filenames.size()-1);
                        paramTftValue[GW_WAV_FILE] = paramWavVal;
                        if(hw.SwitchRisingEdge(hw.S_ENC1) && paramWavVal > -1) {
                            loadSample = true;
                            tft_lib.tab = 0;
                        }
                        if(hw.SwitchRisingEdge(hw.S_ENC1) && paramWavVal == -1) {
                            clearMem();
                        }
                    }
                }
                else if(tft_lib.area[tft_lib.tab] == 1) {
                    if(i == 0) {
                        paramTftValue[GW_CV1_TARGET] += hw.enc[i].Increment();
                        paramTftValue[GW_CV1_TARGET] = clamp_int(paramTftValue[GW_CV1_TARGET],0,kNumCvParams-1);
                    } else if(i == 2){
                        paramTftValue[GW_CV2_TARGET] += hw.enc[i].Increment();
                        paramTftValue[GW_CV2_TARGET] = clamp_int(paramTftValue[GW_CV2_TARGET],0,kNumCvParams-1);
                    } else if(i == 1) {
                        paramTftValue[GW_CV1_ATT] += hw.enc[i].Increment();
                        paramTftValue[GW_CV1_ATT] = clamp_int(paramTftValue[GW_CV1_ATT],0,100);
                    } else {
                        paramTftValue[GW_CV2_ATT] += hw.enc[i].Increment();
                        paramTftValue[GW_CV2_ATT] = clamp_int(paramTftValue[GW_CV2_ATT],0,100);
                    }
                    
                }
                else if(tft_lib.area[tft_lib.tab] == 2) {
                    if(i == 0) {
                        paramTftValue[GW_CV3_TARGET] += hw.enc[i].Increment();
                        paramTftValue[GW_CV3_TARGET] = clamp_int(paramTftValue[GW_CV3_TARGET],0,kNumCvParams-1);
                    } else if(i == 2){
                        
                    } else if(i == 1) {
                        paramTftValue[GW_CV3_ATT] += hw.enc[i].Increment();
                        paramTftValue[GW_CV3_ATT] = clamp_int(paramTftValue[GW_CV3_ATT],0,100);
                    } else {
                        
                    }
                }
                else if(tft_lib.area[tft_lib.tab] == 3) {
                    if(i == 0) {
                        paramTftValue[GW_LFO1_TARGET] += hw.enc[i].Increment();
                        paramTftValue[GW_LFO1_TARGET] = clamp_int(paramTftValue[GW_LFO1_TARGET],0,kNumCvParams-1);
                    }  else if(i == 1) {
                        paramTftValue[GW_LFO1_ATT] += hw.enc[i].Increment();
                        paramTftValue[GW_LFO1_ATT] = clamp_int(paramTftValue[GW_LFO1_ATT],0,100);
                    } else if(i == 2) {
                        paramTftValue[GW_LFO2_TARGET] += hw.enc[i].Increment();
                        paramTftValue[GW_LFO2_TARGET] = clamp_int(paramTftValue[GW_LFO2_TARGET],0,kNumCvParams-1);
                    }  else if(i == 3) {
                        paramTftValue[GW_LFO2_ATT] += hw.enc[i].Increment();
                        paramTftValue[GW_LFO2_ATT] = clamp_int(paramTftValue[GW_LFO2_ATT],0,100);
                    }                    
                }
            break;
            default:
                break;
            }
        }

        // Record button/gate
        if(hw.SwitchRisingEdge(hw.S_REC) || hw.Trigger())
        {
            if (!is_recording) {
                is_recording = true;
                recording_xfade_step = 0;
            } else {
                is_stopping_recording = true;
            }
        }

        if(hw.SwitchState(hw.S_FUNC) && hw.SwitchState(hw.S_SHIFT)) {
            mode_ = UI_MODE_SAVE;
        }
        if(hw.SwitchIsPressedLong(hw.S_FUNC)) {
            mode_ = UI_MODE_CALIBRATION;
        }
        if(hw.SwitchState(hw.S_SHIFT) && hw.SwitchState(hw.S_TOP1)) {
            mode_ = UI_MODE_RESTORE_STATE;
        }
        
    break;
    case UI_MODE_SAVE:
        if(hw.SwitchRisingEdge(hw.S_ENC1) && !readyToSavePreset) {
            readyToSavePreset = true;
            mode_ = UI_MODE_START;
        }
        if(hw.SwitchRisingEdge(hw.S_ENC4)) {
            mode_ = UI_MODE_START;
        }
    break;
    case UI_MODE_RESTORE_STATE:
        if(hw.SwitchRisingEdge(hw.S_ENC1) && !readyToRestorePreset) {
            readyToRestorePreset = true;
            mode_ = UI_MODE_START;
        }
        if(hw.SwitchRisingEdge(hw.S_ENC4)) {
            mode_ = UI_MODE_START;
        }
    break;
    case UI_MODE_CALIBRATION:
        if(hw.SwitchRisingEdge(hw.S_ENC4)) {
            mode_ = UI_MODE_START;
        }
        if(hw.SwitchRisingEdge(hw.S_ENC1) && !readyToSaveCalibration) {
            readyToSaveCalibration = true;
        }
    break;
    }
}

void loadSampleToBuffer(const char* fn,const char* path) {
    UINT bytes_read;
    uint32_t numSamples;
    FRESULT fr;
    char path_buf[128];
    fr = f_opendir(&dir, path);
    if (fr != FR_OK) {
        // Handle error (e.g., print to serial)
        err_msg = "Could not open dir";
        return;
    }
    err_msg = fn;
    strcpy(path_buf, path);
    strcat(path_buf, "/");
    strcat(path_buf, fn);

    if (f_open(&file, path_buf, (FA_OPEN_EXISTING | FA_READ)) == FR_OK) {
        //fsi_loaded = true;
        err_msg = "Opened file";
        FRESULT fr = f_read(&file, &wavHeader, sizeof(wavHeader), &bytes_read);
        if ((fr == FR_OK) && (wavHeader.NbrChannels == 1)) {
            clearMem();
            write_head = 0;
            switch (wavHeader.BitPerSample) {

                case 16:
                    numSamples = wavHeader.SubCHunk2Size / 2;
                    if (numSamples > RECORDING_BUFFER_SIZE)
                        numSamples = RECORDING_BUFFER_SIZE;                           
                    
                    fr = f_read(&file, convertBuffer, (numSamples * 2), &bytes_read);
                    for (uint32_t i = 0; i < numSamples; i++) {
                        record_sample(s162f(convertBuffer[i]));
                        record_sample_for_display(s162f(convertBuffer[i]));
                        increment_write_head();
                    }
                    err_msg = "Success";
                break;

                case 32:
                    numSamples = wavHeader.SubCHunk2Size / 4;
                    if (numSamples > RECORDING_BUFFER_SIZE)
                        numSamples = RECORDING_BUFFER_SIZE;                           
                   
                    //fr = f_read(&file, &sampleBuff[s][0], (numSamples * 4), &bytes_read);
                break;

                default:
                break;
            }
        }
        f_close(&file);
    } else {
        err_msg = "File not opened";
    }
}

float clamp(float x, float a, float b)
{
    return std::max(a,std::min(b,x));
}


float GetCvLfoValue(int param_idx) {
	float tmp_cv[hw.CV_LAST] = {0,0,0};
    float tmp_lfo[2] = {0,0};
	for (int i = 0; i < hw.CV_LAST; i++)
	{       
        if(param_idx == paramTftValue[GW_CV1_TARGET + i] && param_idx != 0 && paramTftValue[GW_CV1_ATT + i] > 0) {
			tmp_cv[i] = (hw.GetCvValue(i) - cvOffsetValues_[i]) * (paramTftValue[GW_CV1_ATT + i] / 100.0);
		}
	}
    for (int i = 0; i < 2; i++) 
    {
        if(param_idx == paramTftValue[GW_LFO1_TARGET + i] && param_idx != 0) {
			tmp_lfo[i] = lfo_val[i];
		}
    }
    
	return clamp((tmp_cv[0] + tmp_cv[1] + tmp_cv[2] + tmp_lfo[0] + tmp_lfo[1]), -1.0, 1.0);	
}

void SetTftParamValue(int param_idx, float param_value) {
    paramTftCvValue[param_idx] = clamp_int(static_cast<int>(param_value*100.f),0,100);
}

bool IsCvControlled(int param_idx) {
    for (int i = 0; i < hw.CV_LAST; i++)
	{
		if(param_idx == paramTftValue[GW_CV1_TARGET + i] && param_idx != 0) {
			return true;
		}
	}
    for (int i = 0; i < 2; i++)
    {
        if(param_idx == paramTftValue[GW_LFO1_TARGET + i] && param_idx != 0) {
			return true;
		}
    }
	return false;	
}

bool IsOct(int octval) {
    switch (octval)
    {
    case 0:
    case 99:
    case 100:
    case 101:
    case 199:
    case 200:
    case 201:
    case -99:
    case -100:
    case -101:
    case -199:
    case -200:
    case -201:
        return true;
        break;
    
    default:
        return false;
        break;
    }
}


void LoadPreset()
{
    const the_settings &psettings = preset.thesettings();
    paramTftValue[GW_COUNT] = psettings.gw_count;
    paramTftValue[GW_SPLAY] = psettings.gw_splay;
    paramTftValue[GW_JITTER] = psettings.gw_jitter;
    paramTftValue[GW_REVERB] = psettings.gw_reverb;
    paramTftValue[GW_RND_PAN] = psettings.gw_rnd_pan;
    paramTftValue[GW_RND_OCT] = psettings.gw_rnd_oct;
    paramTftValue[GW_IN_LVL] = psettings.gw_in_lvl;
    paramTftValue[GW_OUT_LVL] = psettings.gw_out_lvl;
    paramTftValue[GW_RV_TIME] = psettings.gw_reverb_time;
    paramTftValue[GW_RV_DAMP] = psettings.gw_reverb_damp;
    paramTftValue[GW_RV_HPF] = psettings.gw_reverb_hpf;
    paramTftValue[GW_LFO_WAVE] = psettings.gw_lfo1_wave;
    paramTftValue[GW_LFO_SPEED] = psettings.gw_lfo1_speed;
    paramTftValue[GW_LFO_RANGE] = psettings.gw_lfo1_range;
    paramTftValue[GW_LFO2_WAVE] = psettings.gw_lfo2_wave;
    paramTftValue[GW_LFO2_SPEED] = psettings.gw_lfo2_speed;
    paramTftValue[GW_LFO2_RANGE] = psettings.gw_lfo2_range;
    paramTftValue[GW_WINDOW] = psettings.gw_window;
    paramTftValue[GW_SCAN_POSITION] = psettings.gw_scan_pos;
    paramTftValue[GW_CV1_TARGET] = psettings.gw_cv1_sel;
    paramTftValue[GW_CV2_TARGET] = psettings.gw_cv2_sel;
    paramTftValue[GW_CV3_TARGET] = psettings.gw_cv3_sel;
    paramTftValue[GW_CV1_ATT] = psettings.gw_cv1_att;
    paramTftValue[GW_CV2_ATT] = psettings.gw_cv2_att;
    paramTftValue[GW_CV3_ATT] = psettings.gw_cv3_att;
    paramTftValue[GW_LFO1_TARGET] = psettings.gw_lfo1_sel;
    paramTftValue[GW_LFO2_TARGET] = psettings.gw_lfo2_sel;
    paramTftValue[GW_LFO1_ATT] = psettings.gw_lfo1_att;
    paramTftValue[GW_LFO2_ATT] = psettings.gw_lfo2_att;
}

void SavePreset()
{
    the_settings *psettings = preset.mutable_thesettings();

    psettings->gw_count = paramTftValue[GW_COUNT];
    psettings->gw_splay = paramTftValue[GW_SPLAY];
    psettings->gw_jitter = paramTftValue[GW_JITTER];
    psettings->gw_reverb = paramTftValue[GW_REVERB];
    psettings->gw_rnd_pan = paramTftValue[GW_RND_PAN];
    psettings->gw_rnd_oct = paramTftValue[GW_RND_OCT];
    psettings->gw_in_lvl = paramTftValue[GW_IN_LVL];
    psettings->gw_out_lvl = paramTftValue[GW_OUT_LVL];
    psettings->gw_reverb_time = paramTftValue[GW_RV_TIME];
    psettings->gw_reverb_damp = paramTftValue[GW_RV_DAMP];
    psettings->gw_reverb_hpf = paramTftValue[GW_RV_HPF];
    psettings->gw_lfo1_wave = paramTftValue[GW_LFO_WAVE];
    psettings->gw_lfo1_speed = paramTftValue[GW_LFO_SPEED];
    psettings->gw_lfo1_range = paramTftValue[GW_LFO_RANGE];
    psettings->gw_lfo2_wave = paramTftValue[GW_LFO2_WAVE];
    psettings->gw_lfo2_speed = paramTftValue[GW_LFO2_SPEED];
    psettings->gw_lfo2_range = paramTftValue[GW_LFO2_RANGE];
    psettings->gw_window = paramTftValue[GW_WINDOW];
    psettings->gw_scan_pos = paramTftValue[GW_SCAN_POSITION];
    psettings->gw_cv1_sel = paramTftValue[GW_CV1_TARGET];
    psettings->gw_cv2_sel = paramTftValue[GW_CV2_TARGET];
    psettings->gw_cv3_sel = paramTftValue[GW_CV3_TARGET];
    psettings->gw_cv1_att = paramTftValue[GW_CV1_ATT];
    psettings->gw_cv2_att = paramTftValue[GW_CV2_ATT];
    psettings->gw_cv3_att = paramTftValue[GW_CV3_ATT];
    psettings->gw_lfo1_sel = paramTftValue[GW_LFO1_TARGET];
    psettings->gw_lfo2_sel = paramTftValue[GW_LFO2_TARGET];
    psettings->gw_lfo1_att = paramTftValue[GW_LFO1_ATT];
    psettings->gw_lfo2_att = paramTftValue[GW_LFO2_ATT];
    preset.SaveTheSettings();
}


void LoadCalibration(){
    const the_calibration &cvcal = preset.thecalibration();
    
    for (size_t i = 0; i < hw.CV_LAST; i++)
    {
            cvOffsetValues_[i] = cvcal.cvOffset[i];   
    }
}
void SaveCalibration(){
        const the_calibration &storedcvcal = preset.thecalibration();
        float temp_stored_cv[4];
        calibrationLoader = 0;
        for (size_t i = 0; i < hw.CV_LAST; i++)
        {
            temp_stored_cv[i] = storedcvcal.cvOffset[i];
        }
         
        // perform calibration routine
        int numSamples = 128;
        for(int i = 0; i < numSamples; i++) {
            // accumulate cv values
            for (size_t j = 0; j < hw.CV_LAST; j++)
            {
                temp_stored_cv[j] += hw.GetCvValue(j);
            }
            calibrationLoader++;
            // wait 10ms
            int calloader = map_(calibrationLoader,0,127,3,300);
            if(tft.IsRender())
            {
                tft.FillRect(Rectangle(0, 0, calloader, 6), COLOR_YELLOW);
                tft.Update();
            }
            System::Delay(100);
            
        }
        for (size_t i = 0; i < hw.CV_LAST; i++)
        {
            
            temp_stored_cv[i] = temp_stored_cv[i] / ((float)numSamples);
        }
               

        the_calibration *calsettings = preset.mutable_thecalibration();
        calsettings->calibrated = true;
        for (size_t i = 0; i < hw.CV_LAST; i++)
        {
            calsettings->cvOffset[i] = temp_stored_cv[i];
            cvOffsetValues_[i] = temp_stored_cv[i];
        }
        preset.SaveCalibration();
        
}

void RenderCalibration() {
    tft.WriteStringAligned("Calibration?",Font_11x18,Rectangle(0,30,320,20),Alignment::centered,COLOR_WHITE);
    
    if(calibrationLoader > 0) {
        tft.WriteStringAligned("The calibration was successful.",Font_7x10,Rectangle(0,80,320,20),Alignment::centered,COLOR_WHITE);
    } else {
        tft.WriteStringAligned("(Only necessary if the values of CV1-CV3",Font_7x10,Rectangle(0,60,320,20),Alignment::centered,COLOR_WHITE);
        tft.WriteStringAligned("are not equal to 0)",Font_7x10,Rectangle(0,80,320,20),Alignment::centered,COLOR_WHITE);
    }
    int tmpcv[4];
    tmpcv[0] = (hw.GetCvValue(hw.CV_1) - cvOffsetValues_[hw.CV_1]) * 1000;
    tmpcv[1] = (hw.GetCvValue(hw.CV_2) - cvOffsetValues_[hw.CV_2]) * 1000;
    tmpcv[2] = (hw.GetCvValue(hw.CV_3) - cvOffsetValues_[hw.CV_3]) * 1000;

    tft.WriteString("CV1:",5,130,Font_11x18,COLOR_WHITE);
    tft.WriteString(GetIntAsString_(static_cast<int>(tmpcv[0])),80,130,Font_11x18,COLOR_YELLOW);
    tft.WriteString("CV2:",170,130,Font_11x18,COLOR_WHITE);
    tft.WriteString(GetIntAsString_(static_cast<int>(tmpcv[1])),250,130,Font_11x18,COLOR_YELLOW);
    tft.WriteString("CV3:",5,155,Font_11x18,COLOR_WHITE);
    tft.WriteString(GetIntAsString_(static_cast<int>(tmpcv[2])),80,155,Font_11x18,COLOR_YELLOW);

    if(calibrationLoader == 0) {
        tft.FillRect(Rectangle(0, 213, 75, 26), COLOR_BLUE);
    }
    tft.FillRect(Rectangle(245, 213, 75, 26), COLOR_RED);

    if(calibrationLoader == 0) {
        tft.WriteStringAligned("YES",Font_7x10,Rectangle(0, 213, 75, 26),Alignment::centered,COLOR_WHITE);
        tft.WriteStringAligned("NO",Font_7x10,Rectangle(245, 213, 75, 26),Alignment::centered,COLOR_WHITE);
    } else {
        tft.WriteStringAligned("CLOSE",Font_7x10,Rectangle(245, 213, 75, 26),Alignment::centered,COLOR_WHITE);
    }
    
}

float GetLfoFreq(int range, float speed) {
    float lfofreq;
    switch (range)
    {
    case 0:
        lfofreq = map_to_range(speed,1.f,100.f);
        break;
    case 1:
        lfofreq = map_to_range(speed,100.f,200.f);
        break;
    case 2:
        lfofreq = map_to_range(speed,200.f,400.f);
        break;
    default:
        lfofreq = map_to_range(speed,1.f,100.f);
        break;
    }
    return lfofreq;
}