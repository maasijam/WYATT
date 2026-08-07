#include "../../daisy_wyatt.h"
#include "daisysp.h"
#include "../../Tft/ili9341_ui_driver.hpp"
#include "../../common/tft_lib.h"

using namespace daisy;
using namespace daisysp;
using namespace wyatt;
using namespace tftlib;

const char* version = "v 1.0";
const char* project_name = "Hardware-Test";

DaisyWyatt hw;
ILI9341UiDriver driver;
Tft_lib      tft_lib;

FIL            file; /**< Can't be made on the stack (DTCMRAM) */
FatFSInterface fsi;
WAV_FormatTypeDef wavHeader;
bool fsi_loaded = false;
bool colorToggle = false;
int trigCount = 0;

static Metro  intclock;

#define PARAM_BUFFER_SIZE 8
char val_str[PARAM_BUFFER_SIZE];

static uint32_t const buffer_size = 153600; // 320 * 240 * 2
static uint8_t DMA_BUFFER_MEM_SECTION frame_buffer[buffer_size];

void TftProcess();
void TftDisplayManager();
void RenderTest_();
void drawRotarySlider(int value, int minValue, int maxValue, int centerX, int centerY, int radius);
void generateHzValues();
float getHzFromBpm(int bpm);  
void MIDISendNoteOn(uint8_t channel, uint8_t notenum, uint8_t velocity);
void MIDISendNoteOff(uint8_t channel, uint8_t notenum);

float pos[4] = {0};
float cv[3] = {0};
int encValue[4] = {0};

void HandleMidiMessage(MidiEvent m);
int renderNote = 0;

bool trigMidiOut = false;
int startBpm = 20;
int endBpm = 200;
bool isPlaying = false;
    
// Größe des Arrays berechnen: 240 - 20 + 1 = 221
int size_hz = endBpm - startBpm + 1;
    
// Vektor (Array) erstellen, um die Hz-Werte zu speichern
std::vector<double> hertzValues(size_hz);


void AudioCallback(
    AudioHandle::InputBuffer  in,
    AudioHandle::OutputBuffer out,
    size_t size
) {
    
    hw.ProcessAnalogControls();

    uint8_t tic;
    intclock.SetFreq(getHzFromBpm(40)*4);
    
  
    pos[0] = hw.GetKnobValue(hw.KNOB_1) * 1000;
    pos[1] = hw.GetKnobValue(hw.KNOB_2) * 1000;
    pos[2] = hw.GetKnobValue(hw.KNOB_3) * 1000;
    pos[3] = hw.GetKnobValue(hw.KNOB_4) * 1000;

    cv[0] = hw.GetCvValue(hw.CV_1) * 1000;
    cv[1] = hw.GetCvValue(hw.CV_2) * 1000;
    cv[2] = hw.GetCvValue(hw.CV_3) * 1000;
    
    // Process audio output
    for(size_t i = 0; i < size; i++)
    {
        tic = intclock.Process();
        if(tic)
        {
            trigMidiOut = !trigMidiOut;
        } 
        
        
        // store signal = loop signal * loop gain + in * in_gain
        float sig_l = IN_L[i];
        float sig_r = IN_L[i];

        // send that signal to the outputs
        OUT_L[i] = sig_l;
        OUT_R[i] = sig_r;
    }

    
}

int main(void)
{
    hw.Init(false);
    driver.Init(frame_buffer);
    tft_lib.Init(&driver);
    float samplerate = hw.AudioSampleRate();

    if(driver.IsRender())
    {
        driver.Fill(COLOR_BLACK);
        tft_lib.RenderSplash(project_name,version);
        driver.Update();
    }

    hw.DelayMs(1500);
    
     /** SD card next */
    SdmmcHandler::Config sd_config;
    SdmmcHandler         sdcard;
    sd_config.Defaults();
    sd_config.speed           = daisy::SdmmcHandler::Speed::FAST; 
    sd_config.width           = daisy::SdmmcHandler::BusWidth::BITS_4;
    sdcard.Init(sd_config);

    fsi.Init(FatFSInterface::Config::MEDIA_SD);
    FATFS& fs = fsi.GetSDFileSystem();

    if (f_mount(&fs, "/", 1) == FR_OK) {
        fsi_loaded = true;
    }

    intclock.Init(8, samplerate);
    generateHzValues();

    hw.midi.StartReceive();
    //Start the adc
    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    while (1) 
    {
        hw.ProcessDigitalControls();

        if(hw.SwitchState(hw.S_FUNC)) {
            if(!isPlaying) {
                uint8_t note_on[3] = { 0x90, 48, 100 };
                hw.midi.SendMessage(note_on, 3);
            }
            hw.SetLed(hw.LED_GREEN, true);
            isPlaying = true;
        } else {
            if(isPlaying) {
                uint8_t note_off[3] = { 0x80, 48, 0 };
                hw.midi.SendMessage(note_off, 3);
                isPlaying = false;
            }
            hw.SetLed(hw.LED_GREEN,false);
        }

        if(hw.SwitchRisingEdge(hw.S_REC)) {
            colorToggle = !colorToggle;
        }
        
        if(hw.SwitchState(hw.S_REC) || hw.Gate()) {
            if(colorToggle) {
                hw.SetLed(hw.LED_GREEN, true);
            } else {
                hw.SetLed(hw.LED_RED, true);
            }
        } else {
            if(colorToggle) {
                hw.SetLed(hw.LED_GREEN, false);
            } else {
                hw.SetLed(hw.LED_RED, false);
            }
        }

        hw.UpdateLeds();
        
        for (size_t i = 0; i < 4; i++)
        {
            encValue[i] += hw.enc[i].Increment();
        }

        hw.midi.Listen();
        // Handle MIDI Events
        while(hw.midi.HasEvents())
        {
            HandleMidiMessage(hw.midi.PopEvent());
        }

        
        if(trigMidiOut) {
            if(trigCount == 0) {
                MIDISendNoteOn(1,48,64);
                trigCount++;
            }
        } else {
            if(trigCount == 1) {
                MIDISendNoteOff(1,48);
                trigCount = 0;
            }
        }
        

        TftProcess();
    }
}
char* GetIntAsString(int val) { 
        snprintf(val_str, PARAM_BUFFER_SIZE, "%d", val);
        return val_str; 
    }

void TftProcess()
{
    TftDisplayManager();
}

void TftDisplayManager()
{
    if(driver.IsRender())
        {
            driver.Fill(COLOR_BLACK);  
            RenderTest_();       
            driver.Update();
           
        }
   
}




void RenderTest_() {
    
    if(hw.SwitchState(hw.S_TOP1)) {
        driver.FillRect(Rectangle(5, 225, 15, 10), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_TOP2)) {
        driver.FillRect(Rectangle(25, 225, 15, 10), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_TOP3)) {
        driver.FillRect(Rectangle(45, 225, 15, 10), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_TOP4)) {
        driver.FillRect(Rectangle(65, 225, 15, 10), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_PAGE_UP)) {
        driver.FillRect(Rectangle(85, 225, 15, 10), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_PAGE_DOWN)) {
        driver.FillRect(Rectangle(105, 225, 15, 10), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_FUNC)) {
        driver.FillRect(Rectangle(125, 225, 15, 10), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_SHIFT)) {
        driver.FillRect(Rectangle(145, 225, 15, 10), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_REC)) {
        driver.FillRect(Rectangle(245, 225, 15, 10), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_ENC1)) {
        driver.FillRect(Rectangle(165, 225, 15, 10), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_ENC2)) {
        driver.FillRect(Rectangle(185, 225, 15, 10), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_ENC3)) {
        driver.FillRect(Rectangle(205, 225, 15, 10), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_ENC4)) {
        driver.FillRect(Rectangle(225, 225, 15, 10), COLOR_BLUE);
    }
    
    

    driver.WriteString("KNOB1:",5,5,Font_11x18,COLOR_YELLOW);
    driver.WriteString(GetIntAsString(static_cast<int>(pos[0])),80,5,Font_11x18,COLOR_YELLOW);
    driver.WriteString("KNOB2:",170,5,Font_11x18,COLOR_YELLOW);
    driver.WriteString(GetIntAsString(static_cast<int>(pos[1])),250,5,Font_11x18,COLOR_YELLOW);
    driver.WriteString("KNOB3:",5,30,Font_11x18,COLOR_YELLOW);
    driver.WriteString(GetIntAsString(static_cast<int>(pos[2])),80,30,Font_11x18,COLOR_YELLOW);
    driver.WriteString("KNOB4:",170,30,Font_11x18,COLOR_YELLOW);
    driver.WriteString(GetIntAsString(static_cast<int>(pos[3])),250,30,Font_11x18,COLOR_YELLOW);
    driver.WriteString("ENC1:",5,55,Font_11x18,COLOR_YELLOW);
    driver.WriteString(GetIntAsString(encValue[0]),80,55,Font_11x18,COLOR_YELLOW);
    driver.WriteString("ENC2:",170,55,Font_11x18,COLOR_YELLOW);
    driver.WriteString(GetIntAsString(encValue[1]),250,55,Font_11x18,COLOR_YELLOW);
    driver.WriteString("ENC3:",5,80,Font_11x18,COLOR_YELLOW);
    driver.WriteString(GetIntAsString(encValue[2]),80,80,Font_11x18,COLOR_YELLOW);
    driver.WriteString("ENV4:",170,80,Font_11x18,COLOR_YELLOW);
    driver.WriteString(GetIntAsString(encValue[3]),250,80,Font_11x18,COLOR_YELLOW);
    
    driver.WriteString("CV1:",5,105,Font_11x18,COLOR_YELLOW);
    driver.WriteString(GetIntAsString(static_cast<int>(cv[0])),80,105,Font_11x18,COLOR_YELLOW);
    driver.WriteString("CV2:",170,105,Font_11x18,COLOR_YELLOW);
    driver.WriteString(GetIntAsString(static_cast<int>(cv[1])),250,105,Font_11x18,COLOR_YELLOW);
    driver.WriteString("CV3:",5,130,Font_11x18,COLOR_YELLOW);
    driver.WriteString(GetIntAsString(static_cast<int>(cv[2])),80,130,Font_11x18,COLOR_YELLOW);
    
    driver.WriteString("SD:",5,155,Font_11x18,COLOR_YELLOW);
    driver.WriteString((fsi_loaded ? "TRUE" : "FALSE"),80,155,Font_11x18,COLOR_YELLOW);
    driver.WriteString("GATE:",170,155,Font_11x18,COLOR_YELLOW);
    driver.WriteString((hw.Gate() ? "ON" : "OFF"),250,155,Font_11x18,COLOR_YELLOW);

    driver.WriteString("MIDI IN NOTE:",5,180,Font_11x18,COLOR_YELLOW);
    driver.WriteString(GetIntAsString(renderNote),200,180,Font_11x18,COLOR_YELLOW);
    driver.WriteString("MIDI OUT NOTE:",5,205,Font_11x18,COLOR_YELLOW);
    driver.WriteString((trigMidiOut ? "48" : ""),200,205,Font_11x18,COLOR_YELLOW);
    
}

long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}



// Typical Switch case for Message Type.
void HandleMidiMessage(MidiEvent m)
{
    switch(m.type)
    {
        case NoteOn:
        {
            NoteOnEvent p = m.AsNoteOn();
            // This is to avoid Max/MSP Note outs for now..
            if(m.data[1] != 0)
            {
                p = m.AsNoteOn();
                renderNote = p.note;
                //osc.SetAmp((p.velocity / 127.0f));
            }
        }
        break;
        case ControlChange:
        {
            ControlChangeEvent p = m.AsControlChange();
            switch(p.control_number)
            {
                case 1:
                    // CC 1 for cutoff.
                    //filt.SetFreq(mtof((float)p.value));
                    break;
                case 2:
                    // CC 2 for res.
                    //filt.SetRes(((float)p.value / 127.0f));
                    break;
                default: break;
            }
            break;
        }
        default: break;
    }
}

void generateHzValues() {
    // Befüllen des Arrays
    for (int i = 0; i < size_hz; ++i) {
        int currentBpm = startBpm + i;
        hertzValues[i] = static_cast<double>(currentBpm) / 60.0;
    }
}
float getHzFromBpm(int bpm){
    return hertzValues[bpm - startBpm];
}

void MIDISendNoteOn(uint8_t channel, uint8_t notenum, uint8_t velocity)
{
    uint8_t data[3] = {0};

    data[0] = (channel & 0x0F) + 0x90; // limit channel byte, add status byte
    data[1] = notenum & 0x7F;             // remove MSB on data
    data[2] = velocity & 0x7F;

    hw.midi.SendMessage(data, 3);
}
void MIDISendNoteOff(uint8_t channel, uint8_t notenum)
{
    uint8_t data[3] = {0};

    data[0] = (channel & 0x0F) + 0x80; // limit channel byte, add status byte
    data[1] = notenum & 0x7F;             // remove MSB on data
    data[2] = 0 & 0x7F;

    hw.midi.SendMessage(data, 3);
}