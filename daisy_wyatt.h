#pragma once

#include "daisy_seed.h"
#include "libDaisyCustom/encoderNoSwitch.h"


namespace wyatt
{

using namespace daisy;

class DaisyWyatt
{
  public:

    enum Pots
    {
        KNOB_0,   /**< */
        KNOB_1,   /**< */
        KNOB_2,   /**< */
        KNOB_3,   /**< */
        KNOB_4,
        KNOB_5,
        KNOB_6,
        KNOB_7,
        KNOB_LAST /**< */
    };

    enum CvIns
    {
        CV_0,   /**< */
        CV_1,   /**< */
        CV_2,   /**< */
        CV_3,   /**< */
        CV_4,
        CV_5,
        CV_6,
        CV_7,
        CV_LAST /**< */
    };

    enum Encs
    {
        ENC1,   /**< */
        ENC2,   /**< */
        ENC3,   /**< */
        ENC4,   /**< */
        ENC_LAST /**< */
    };


enum Switches
{
    S_NO1,
    S_NO2,
    S_NO3,
    S_SHIFT, // idx 4
    S_ENC4, // idx 3
    S_ENC3, // idx 2
    S_ENC2, // idx 1
    S_ENC1, // idx 0
    S_REC, // idx 15
    S_FUNC, // idx 14
    S_PAGE_DOWN, // idx 13
    S_PAGE_UP, // idx 12
    S_TOP4, // idx 11
    S_TOP3, // idx 10
    S_TOP2, // idx 9
    S_TOP1, // idx 8
    S_LAST 
};

enum Leds
{
    LED_RED,
    LED_GREEN,
    LED_LAST
};

    
    /** Constructor */
    DaisyWyatt() {}
    /** Destructor */
    ~DaisyWyatt() {}

    /** Initializes the daisy seed, and bluemchen hardware.*/
    void Init(bool boost = false);

    /** Audio Block size defaults to 48.
  Change it using this function before StartingAudio _\param size Audio block size.
  */
    void SetAudioBlockSize(size_t size);

    /** Returns the number of samples per channel in a block of audio. */
    size_t AudioBlockSize();

    /** Set the sample rate for the audio */
    void SetAudioSampleRate(SaiHandle::Config::SampleRate samplerate);

    /** Get sample rate */
    float AudioSampleRate();


    /** Get callback rate */
    float AudioCallbackRate();

    /** Start the saul audio with the given callback function
    \cb AudioCallback callback function
    */
    void StartAudio(AudioHandle::AudioCallback cb);

    /** Starts the callback  _\param cb Interleaved callback function
    */
    void StartAudio(AudioHandle::InterleavingAudioCallback cb);

    /**
       Switch callback functions
       \param cb New interleaved callback function.
    */
    void ChangeAudioCallback(AudioHandle::InterleavingAudioCallback cb);


    /**
     Change the AudioCallback function _\param cb The new callback function.
  */
    void ChangeAudioCallback(AudioHandle::AudioCallback cb);

    /** Stops the audio */
    void StopAudio();

    /** Start the ADC */
    void StartAdc();

    /** Stop the ADC */
    void StopAdc();

    /** Process all analog controls */
    void ProcessAnalogControls();

    /** Process all digital controls */
    void ProcessDigitalControls(int encScale = 1);

    /** Process Analog and Digital Controls */
    inline void ProcessAllControls()
    {
        ProcessAnalogControls();
        ProcessDigitalControls();
    }


    /**
     Get value for a particular control _\param k Which control to get
   */
    float GetKnobValue(int idx) const;
    float GetCvValue(int idx) const;

    /** Getter for CV objects.
        \param idx The CV input of interest.
    */
    AnalogControl* GetCv(size_t idx);

    /** Getter for Knob objects.
        \param idx The Knob of interest.
    */
    AnalogControl* GetKnob(size_t idx);

    /** Returns true if gate in is HIGH */
    bool GateIn1();
    bool TrigIn1();

    /** Gets a random 32-bit value */
    inline uint32_t GetRandomValue() { return Random::GetValue(); }

    /** Gets a random floating point value between the specified minimum, and maxmimum */
    inline float GetRandomFloat(float min = 0.f, float max = 1.f)
    {
        return Random::GetFloat(min, max);
    }

    /** Returns true if the key has not been pressed recently
        \param idx the key of interest
    */
    bool SwitchState(size_t idx) const;

    /** Returns true if the key has just been pressed
        \param idx the key of interest
    */
    bool SwitchRisingEdge(size_t idx) const;

    /** Returns true if the key has just been released
        \param idx the key of interest
    */
    bool SwitchFallingEdge(size_t idx) const;
    

    /**
  General delay _\param del Delay time in ms.
  */
    void DelayMs(size_t del);
 

    float CVKnobCombo(float CV_Val,float Pot_Val);

    void SetLed(Leds color,bool state);
    void UpdateLeds();
    

    DaisySeed       seed;                /**< Seed object */
    AnalogControl   knob[KNOB_LAST]; /**< Array of AnalogControls */
    AnalogControl   cv[CV_LAST]; /**< Array of AnalogControls */
    Led             led_sw[LED_LAST]; 
    GateIn          gate_in;
    EncoderNoSwitch enc[ENC_LAST];     /**< & */
    ShiftRegister4021<2> switches_sr_; /**< Two 4021s daisy-chained. */
    uint8_t              switches_state_[16];
    MidiUartHandler midi;                     /**< Handles midi*/
    

  private:
    void SetHidUpdateRates();
    void InitEncoder();
    void InitMidi();

    

    
    
};

} // namespace white
