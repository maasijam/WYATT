#include "../../daisy_wyatt.h"
#include "daisysp.h"
///#include "core_cm7.h"
#include "../../Tft/ili9341_ui_driver.hpp"

using namespace daisy;
using namespace daisysp;
using namespace wyatt;
//using namespace std;

float PI = 3.14159265359;


DaisyWyatt hw;
ILI9341UiDriver driver;

FIL            file; /**< Can't be made on the stack (DTCMRAM) */
FatFSInterface fsi;
WAV_FormatTypeDef wavHeader;
bool fsi_loaded = false;

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

float pos[4] = {0};
float cv[3] = {0};
int encValue[5] = {0};

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
    
  
    float in_gain = hw.GetKnobValue(hw.KNOB_0);
    pos[0] = hw.GetKnobValue(hw.KNOB_0) * 1000;
    pos[1] = hw.GetKnobValue(hw.KNOB_1) * 1000;
    pos[2] = hw.GetKnobValue(hw.KNOB_2) * 1000;
    pos[3] = hw.GetKnobValue(hw.KNOB_3) * 1000;

    cv[0] = hw.GetCvValue(hw.CV_0) * 1000;
    cv[1] = hw.GetCvValue(hw.CV_1) * 1000;
    cv[2] = hw.GetCvValue(hw.CV_2) * 1000;
    

    //float in_gain = 1.f;

    // Process audio output
    for(size_t i = 0; i < size; i++)
    {
        tic = intclock.Process();
        if(tic)
        {
            //trigMidiOut = !trigMidiOut;
        } 
        
        
        // store signal = loop signal * loop gain + in * in_gain
        float sig_l = IN_L[i] * in_gain;
        float sig_r = IN_L[i] * in_gain;


        

        // send that signal to the outputs
        OUT_L[i] = sig_l;
        OUT_R[i] = sig_r;
    }

    
}

int main(void)
{
    hw.Init(false);
    driver.Init(frame_buffer);
    float samplerate = hw.AudioSampleRate();


     /** SD card next */
    SdmmcHandler::Config sd_config;
    SdmmcHandler         sdcard;
    sd_config.Defaults();
    sd_config.speed           = daisy::SdmmcHandler::Speed::FAST; 
    sd_config.width           = daisy::SdmmcHandler::BusWidth::BITS_4;
    sdcard.Init(sd_config);

    fsi.Init(FatFSInterface::Config::MEDIA_SD);
    FATFS& fs = fsi.GetSDFileSystem();
    char filename[32];
    UINT bytes_read;

    //sampler.Init(fsi.GetSDPath());
    //sampler.SetLooping(true);
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

        
        
        if(hw.SwitchState(hw.S_REC) || hw.GateIn1()) {
            //hw.SetLed(hw.LED_RED, true);
        } else {
            //hw.SetLed(hw.LED_RED,false);
        }

        hw.UpdateLeds();
        
        for (size_t i = 0; i < 5; i++)
        {
            encValue[i] += hw.enc[i].Increment();
        }

        hw.midi.Listen();
        // Handle MIDI Events
        while(hw.midi.HasEvents())
        {
            HandleMidiMessage(hw.midi.PopEvent());
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
            //hw.driver.WriteString("pos:",5,5,Font_11x18,COLOR_YELLOW);        
            driver.Update();
           
        }
   
}




void RenderTest_() {
    //driver.Fill(COLOR_BLACK);
    //driver.FillRect(Rectangle(10, 200, 15, 15), COLOR_YELLOW);
    if(hw.SwitchState(hw.S_TOP1)) {
        driver.FillRect(Rectangle(10, 200, 15, 15), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_TOP2)) {
        driver.FillRect(Rectangle(30, 200, 15, 15), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_TOP3)) {
        driver.FillRect(Rectangle(50, 200, 15, 15), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_TOP4)) {
        driver.FillRect(Rectangle(70, 200, 15, 15), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_PAGE_UP)) {
        driver.FillRect(Rectangle(90, 200, 15, 15), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_PAGE_DOWN)) {
        driver.FillRect(Rectangle(110, 200, 15, 15), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_FUNC)) {
        driver.FillRect(Rectangle(130, 200, 15, 15), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_REC)) {
        driver.FillRect(Rectangle(150, 200, 15, 15), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_ENC1)) {
        driver.FillRect(Rectangle(170, 200, 15, 15), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_ENC2)) {
        driver.FillRect(Rectangle(190, 200, 15, 15), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_ENC3)) {
        driver.FillRect(Rectangle(210, 200, 15, 15), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_ENC4)) {
        driver.FillRect(Rectangle(230, 200, 15, 15), COLOR_BLUE);
    }
    if(hw.SwitchState(hw.S_SHIFT)) {
        driver.FillRect(Rectangle(250, 200, 15, 15), COLOR_BLUE);
    }
    

    driver.WriteString("gain:",5,5,Font_11x18,COLOR_YELLOW);
    driver.WriteString(GetIntAsString(static_cast<int>(pos[0])),80,5,Font_11x18,COLOR_YELLOW);
    driver.WriteString("start:",170,5,Font_11x18,COLOR_YELLOW);
    driver.WriteString(GetIntAsString(static_cast<int>(pos[1])),250,5,Font_11x18,COLOR_YELLOW);
    driver.WriteString("recsize:",5,30,Font_11x18,COLOR_YELLOW);
    driver.WriteString(GetIntAsString(static_cast<int>(pos[2])),100,30,Font_11x18,COLOR_YELLOW);
    driver.WriteString("length:",170,30,Font_11x18,COLOR_YELLOW);
    driver.WriteString(GetIntAsString(static_cast<int>(pos[3])),250,30,Font_11x18,COLOR_YELLOW);
    driver.WriteString("enc1:",5,55,Font_11x18,COLOR_YELLOW);
    driver.WriteString(GetIntAsString(encValue[0]),80,55,Font_11x18,COLOR_YELLOW);
    driver.WriteString("enc2:",170,55,Font_11x18,COLOR_YELLOW);
    driver.WriteString(GetIntAsString(encValue[1]),250,55,Font_11x18,COLOR_YELLOW);
    driver.WriteString("enc3:",5,80,Font_11x18,COLOR_YELLOW);
    driver.WriteString(GetIntAsString(encValue[2]),80,80,Font_11x18,COLOR_YELLOW);
    driver.WriteString("enc4:",170,80,Font_11x18,COLOR_YELLOW);
    driver.WriteString(GetIntAsString(encValue[3]),250,80,Font_11x18,COLOR_YELLOW);
    //driver.WriteString("enc5:",5,105,Font_11x18,COLOR_YELLOW);
    //driver.WriteString(GetIntAsString(encValue[4]),80,105,Font_11x18,COLOR_YELLOW);
    driver.WriteString("sd:",170,105,Font_11x18,COLOR_YELLOW);
    driver.WriteString((fsi_loaded ? "ON" : "OFF"),250,105,Font_11x18,COLOR_YELLOW);

    driver.WriteString("cv1:",5,130,Font_11x18,COLOR_YELLOW);
    driver.WriteString(GetIntAsString(static_cast<int>(cv[0])),80,130,Font_11x18,COLOR_YELLOW);
    driver.WriteString("cv2:",170,130,Font_11x18,COLOR_YELLOW);
    driver.WriteString(GetIntAsString(static_cast<int>(cv[1])),250,130,Font_11x18,COLOR_YELLOW);
    driver.WriteString("cv3:",5,155,Font_11x18,COLOR_YELLOW);
    driver.WriteString(GetIntAsString(static_cast<int>(cv[2])),80,155,Font_11x18,COLOR_YELLOW);
    //driver.WriteString("cv4:",170,155,Font_11x18,COLOR_YELLOW);
    //driver.WriteString(GetIntAsString(static_cast<int>(cv[3])),250,155,Font_11x18,COLOR_YELLOW);

    //drawRotarySlider(encValue[0],0,100,280,210,25);
    driver.WriteString(GetIntAsString(renderNote),20,220,Font_11x18,COLOR_YELLOW);
    
}

long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void drawRotarySlider(int value, int minValue, int maxValue, int centerX, int centerY, int radius) {
    
    if(value < 0) {
        value = 0;
    }
    if(value > 100) {
        value = 100;
    }   
    // Draw the outer circle of the slider
    driver.DrawCircle(centerX, centerY, radius, COLOR_WHITE);
    
    
    // Map the encoder value to an angle (0 to 360 degrees)
    float angle = map(value, minValue, maxValue, 135, 405);
    // Convert angle to radians for trigonometric functions
    float radians =   angle * (PI / 180);
    float radiansLine0 =   135 * (PI / 180);
    float radiansLine1 =   270 * (PI / 180);
    float radiansLine2 =   405 * (PI / 180);
    
    // Calculate the position of the indicator "knob" using polar to Cartesian conversion
    int knobX = centerX + (radius - 10) * cos(radians);
    int knobY = centerY + (radius - 10) * sin(radians);

    /*int line0AX = centerX + (radius - 0) * cos(radiansLine0);
    int line0AY = centerY + (radius - 0) * sin(radiansLine0);

    int line0BX = centerX + (radius - 5) * cos(radiansLine0);
    int line0BY = centerY + (radius -5) * sin(radiansLine0);

    int line1AX = centerX + ((radius - 0) * cos(radiansLine1));
    int line1AY = centerY + ((radius - 0) * sin(radiansLine1));

    int line1BX = centerX + ((radius + 2) * cos(radiansLine1));
    int line1BY = centerY + ((radius + 2) * sin(radiansLine1));

    int line2AX = centerX + (radius - 0) * cos(radiansLine2);
    int line2AY = centerY + (radius - 0) * sin(radiansLine2);

    int line2BX = centerX + (radius - 5) * cos(radiansLine2);
    int line2BY = centerY + (radius - 5) * sin(radiansLine2);*/
    
    // Draw the indicator (e.g., a filled circle or line)
    driver.FillCircle(knobX, knobY, 3, COLOR_YELLOW);
    //driver.DrawLine(line0AX,line0AY,line0BX,line0BY,COLOR_WHITE);
    //driver.DrawLine(line1AX,line1AY,line1BX,line1BY,COLOR_WHITE);
    //driver.DrawLine(line2AX,line2AY,line2BX,line2BY,COLOR_WHITE);
   
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