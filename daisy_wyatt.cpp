#include "daisy_wyatt.h"


#ifndef SAMPLE_RATE
#define SAMPLE_RATE DSY_AUDIO_SAMPLE_RATE /**< & */
#endif

using namespace daisy;
using namespace wyatt;


constexpr Pin PIN_ADC_KNOB_CV_MUX = seed::D15;

constexpr Pin PIN_MUX_SEL_0   = seed::D19;
constexpr Pin PIN_MUX_SEL_1   = seed::D20;
constexpr Pin PIN_MUX_SEL_2   = seed::D21;

constexpr Pin PIN_CD4021_D1 = seed::D26;
constexpr Pin PIN_CD4021_CS = seed::D27;
constexpr Pin PIN_CD4021_CLK = seed::D28;

constexpr Pin ENC1_A_PIN = seed::D9;
constexpr Pin ENC1_B_PIN = seed::D11;

constexpr Pin ENC2_A_PIN = seed::D12;
constexpr Pin ENC2_B_PIN = seed::D16;

constexpr Pin ENC3_A_PIN = seed::D29;
constexpr Pin ENC3_B_PIN = seed::D18;

constexpr Pin ENC4_A_PIN = seed::D25;
constexpr Pin ENC4_B_PIN = seed::D24;

constexpr Pin GATE_IN_PIN = seed::D0;
constexpr Pin LED_PIN_RED = seed::D30;
constexpr Pin LED_PIN_GREEN = seed::D22;


void DaisyWyatt::Init(bool boost)
{
    seed.Configure();
    seed.Init(boost);

    // gate in 
    //dsy_gpio_pin gateIn1_gpio = seed.GetPin(PIN_GATE_IN1);
    gate_in.Init(GATE_IN_PIN,false);

    // ADCs
   AdcChannelConfig adc_cfg[1];

    adc_cfg[0].InitMux(PIN_ADC_KNOB_CV_MUX,
                             KNOB_LAST + CV_LAST,
                             PIN_MUX_SEL_0,
                             PIN_MUX_SEL_1,
                             PIN_MUX_SEL_2);
    
    
    seed.adc.Init(adc_cfg,1,daisy::AdcHandle::OVS_128);
    //int cv_sort[3] = {2,1,0};
    for(size_t i = 0; i < (KNOB_LAST + CV_LAST); i++) {
        if(i < 4) {
            knob[i].Init(seed.adc.GetMuxPtr(0, i), AudioCallbackRate()); 
        } else if(i > 3 && i < 7) {
            cv[i-4].InitBipolarCv(seed.adc.GetMuxPtr(0, i), AudioCallbackRate()); 
        } 
    } 


    // Switches
    ShiftRegister4021<2>::Config switches_cfg;
    switches_cfg.clk     = PIN_CD4021_CLK;
    switches_cfg.latch   = PIN_CD4021_CS;
    switches_cfg.data[0] = PIN_CD4021_D1;
    switches_cfg.delay_ticks = 110;
    switches_sr_.Init(switches_cfg);

    for (int i = 0; i < 16; i++) {
        sw[i].Init(5); 
    }

    led_sw[LED_RED].Init(LED_PIN_RED,false);
    led_sw[LED_GREEN].Init(LED_PIN_GREEN,false);
    InitEncoder();
    InitMidi();
}

void DaisyWyatt::SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate sr)
{
    seed.SetAudioSampleRate(sr);
    SetHidUpdateRates();
}

float DaisyWyatt::AudioSampleRate()
{
    return seed.AudioSampleRate();
}

void DaisyWyatt::SetAudioBlockSize(size_t size)
{
    seed.SetAudioBlockSize(size);
    SetHidUpdateRates();
}

size_t DaisyWyatt::AudioBlockSize()
{
    return seed.AudioBlockSize();
}

float DaisyWyatt::AudioCallbackRate()
{
    return seed.AudioCallbackRate();
}

void DaisyWyatt::StartAudio(AudioHandle::AudioCallback cb)
{
    seed.StartAudio(cb);
}

void DaisyWyatt::StartAudio(AudioHandle::InterleavingAudioCallback cb)
{
    seed.StartAudio(cb);
}

void DaisyWyatt::StopAudio()
{
    seed.StopAudio();
}

void DaisyWyatt::ChangeAudioCallback(AudioHandle::AudioCallback cb)
{
    seed.ChangeAudioCallback(cb);
}

void DaisyWyatt::ChangeAudioCallback(AudioHandle::InterleavingAudioCallback cb)
{
    seed.ChangeAudioCallback(cb);
}

void DaisyWyatt::StartAdc()
{
    seed.adc.Start();
}
void DaisyWyatt::StopAdc()
{
    seed.adc.Stop();
}



void DaisyWyatt::ProcessAnalogControls()
{
    for(size_t i = 0; i < KNOB_LAST; i++)
    {
        knob[i].Process();
    }
    for(size_t i = 0; i < CV_LAST; i++)
    {
        cv[i].Process();
    }
}

bool DaisyWyatt::Gate()
{
    return gate_in.State();
}

bool DaisyWyatt::Trigger()
{
    return gate_in.Trig();
}


void DaisyWyatt::ProcessDigitalControls()
{
    switches_sr_.Update();
    
    for (int i = 0; i < 16; i++) {
        uint16_t keyidx, keyoffset;
        keyoffset = i > 7 ? 8 : 0;
        keyidx    = (7 - (i % 8)) + keyoffset;
        bool raw = switches_sr_.State(i);
        sw[keyidx].Process(raw);
    }

    for (size_t j = 0; j < ENC_LAST; j++)
    {
        enc[j].Debounce();
    }   
    
}


float DaisyWyatt::GetKnobValue(int idx) const
{
    return knob[idx < KNOB_LAST ? idx : 0].Value();
}
float DaisyWyatt::GetCvValue(int idx) const 
{
    return cv[idx < CV_LAST ? idx : 0].Value();
}

AnalogControl* DaisyWyatt::GetCv(size_t idx)
{
    return &cv[idx < CV_LAST ? idx : 0];
}
AnalogControl* DaisyWyatt::GetKnob(size_t idx)
{
    return &knob[idx < KNOB_LAST ? idx : 0];
}

void DaisyWyatt::DelayMs(size_t del)
{
    seed.DelayMs(del);
}

void DaisyWyatt::SetHidUpdateRates()
{
    for(size_t i = 0; i < KNOB_LAST; i++)
    {
        knob[i].SetSampleRate(AudioCallbackRate());
    }
    for(size_t i = 0; i < CV_LAST; i++)
    {
        cv[i].SetSampleRate(AudioCallbackRate());
    }
}

void DaisyWyatt::SetLed(Leds color, bool state)
{
    switch (color)
    {
    case LED_RED:
        led_sw[LED_RED].Set(state ? 255.f : 0);
        led_sw[LED_GREEN].Set(0);
        break;
    case LED_GREEN:
        led_sw[LED_GREEN].Set(state ? 255.f : 0);
        led_sw[LED_RED].Set(0);
        break;
    default:
        break;
    }
}
void DaisyWyatt::UpdateLeds()
{
    for (size_t i = 0; i < LED_LAST; i++)
    {
        led_sw[i].Update();
    } 
}


bool DaisyWyatt::SwitchState(size_t idx) const
{
    return sw[idx].State();
}

bool DaisyWyatt::SwitchRisingEdge(size_t idx) const
{
    return sw[idx].RisingEdge();
}

bool DaisyWyatt::SwitchFallingEdge(size_t idx) const
{
    return sw[idx].FallingEdge();
}

bool DaisyWyatt::SwitchIsPressedLong(size_t idx) 
{
    return sw[idx].IsPressedLong();
}



float DaisyWyatt::CVKnobCombo(float CV_Val,float Pot_Val)
{
    float output{};
    output = CV_Val + Pot_Val;

    if(output < 0.0f)
    {
        output = 0.0f;
    }

    if(output > 1.0f)
    {
        output = 1.0f;
    }

    return output;
}
  
void DaisyWyatt::InitEncoder()
{
    enc[0].Init(ENC1_A_PIN, ENC1_B_PIN);
    enc[1].Init(ENC2_A_PIN, ENC2_B_PIN);
    enc[2].Init(ENC3_A_PIN, ENC3_B_PIN);
    enc[3].Init(ENC4_A_PIN, ENC4_B_PIN);
   
} 

int DaisyWyatt::EncoderInc(size_t i, bool shiftState, int shiftInc) {
    int turnDirection = enc[i].Increment();
    if (turnDirection != 0) {
        int  stepSize = shiftState ? shiftInc : 1;
        return turnDirection * stepSize;
    } else {
        return 0;
    }

} 

void DaisyWyatt::InitMidi()
{
    MidiUartHandler::Config midi_config;
    midi.Init(midi_config);
}