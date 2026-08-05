#ifndef WYATT_SETTINGS_H_
#define WYATT_SETTINGS_H_

#include "../../daisy_wyatt.h"


namespace settings {

using namespace wyatt;
using namespace daisy;

#define FLASH_BLOCK 4096



struct the_settings {
    the_settings() :
    gw_count{100},
    gw_splay{50},
    gw_jitter{0},
    gw_reverb{0},
    gw_rnd_pan{100},
    gw_rnd_oct{0},
    gw_in_lvl{100},
    gw_out_lvl{100},
    gw_reverb_time{85},
    gw_reverb_damp{70},
    gw_reverb_hpf{15},
    gw_lfo1_wave{0},
    gw_lfo1_speed{10},
    gw_lfo1_range{0},
    gw_lfo2_wave{0},
    gw_lfo2_speed{10},
    gw_lfo2_range{0},
    gw_window{0},
    gw_wav{0},
    gw_scan_pos{0},
    gw_cv1_sel{0},
    gw_cv2_sel{0},
    gw_cv3_sel{0},
    gw_cv4_sel{0},
    gw_lfo_sel{0},
    gw_cv1_att{0},
    gw_cv2_att{0},
    gw_cv3_att{0},
    gw_cv4_att{0},
    gw_lfo1_att{0},
    gw_lfo2_att{0}
    {}
    
    int gw_count;
    int gw_splay;
    int gw_jitter;
    int gw_reverb;
    int gw_rnd_pan;
    int gw_rnd_oct;
    int gw_in_lvl;
    int gw_out_lvl;
    int gw_reverb_time;
    int gw_reverb_damp;
    int gw_reverb_hpf;
    int gw_lfo1_wave;
    int gw_lfo1_speed;
    int gw_lfo1_range;
    int gw_lfo2_wave;
    int gw_lfo2_speed;
    int gw_lfo2_range;
    int gw_window;
    int gw_wav;
    int gw_scan_pos;
    int gw_cv1_sel;
    int gw_cv2_sel;
    int gw_cv3_sel;
    int gw_cv4_sel;
    int gw_lfo_sel;
    int gw_cv1_att;
    int gw_cv2_att;
    int gw_cv3_att;
    int gw_cv4_att;
    int gw_lfo1_att;
    int gw_lfo2_att;
    
    bool operator==(const the_settings &rhs)
    { 

        if(gw_count != rhs.gw_count)
        {
            return false;
        } else if(gw_splay != rhs.gw_splay) {
            return false;
        } else if(gw_jitter != rhs.gw_jitter) {
            return false;
        } else if(gw_reverb != rhs.gw_reverb) {
            return false;
        } else if(gw_rnd_pan != rhs.gw_rnd_pan) {
            return false;
        } else if(gw_rnd_oct != rhs.gw_rnd_oct) {
            return false;
        } else if(gw_in_lvl != rhs.gw_in_lvl) {
            return false;
        } else if(gw_out_lvl != rhs.gw_out_lvl) {
            return false;
        } else if(gw_reverb_time != rhs.gw_reverb_time) {
            return false;
        } else if(gw_reverb_damp != rhs.gw_reverb_damp) {
            return false;
        } else if(gw_reverb_hpf != rhs.gw_reverb_hpf) {
            return false;
        } else if(gw_lfo1_wave != rhs.gw_lfo1_wave) {
            return false;
        } else if(gw_lfo1_speed != rhs.gw_lfo1_speed) {
            return false;
        } else if(gw_lfo1_range != rhs.gw_lfo1_range) {
            return false;
        } else if(gw_lfo2_wave != rhs.gw_lfo2_wave) {
            return false;
        } else if(gw_lfo2_speed != rhs.gw_lfo2_speed) {
            return false;
        } else if(gw_lfo2_range != rhs.gw_lfo2_range) {
            return false;
        } else if(gw_window != rhs.gw_window) {
            return false;
        } else if(gw_wav != rhs.gw_wav) {
            return false;
        } else if(gw_scan_pos != rhs.gw_scan_pos) {
            return false;
        } else if(gw_cv1_sel != rhs.gw_cv1_sel) {
            return false;
        } else if(gw_cv2_sel != rhs.gw_cv2_sel) {
            return false;
        } else if(gw_cv3_sel != rhs.gw_cv3_sel) {
            return false;
        } else if(gw_cv4_sel != rhs.gw_cv4_sel) {
            return false;
        } else if(gw_lfo_sel != rhs.gw_lfo_sel) {
            return false;
        } else if(gw_cv1_att != rhs.gw_cv1_att) {
            return false;
        } else if(gw_cv2_att != rhs.gw_cv2_att) {
            return false;
        } else if(gw_cv3_att != rhs.gw_cv3_att) {
            return false;
        } else if(gw_cv4_att != rhs.gw_cv4_att) {
            return false;
        } else if(gw_lfo1_att != rhs.gw_lfo1_att) {
            return false;
        } else if(gw_lfo2_att != rhs.gw_lfo2_att) {
            return false;
        }
        
        return true;
    }

    
    bool operator!=(const the_settings &rhs) { return !operator==(rhs); }

};

struct the_calibration {
	float cvOffset[4] = {0.0};
	int calibrated = false;

    bool operator==(const the_calibration &rhs)
    {
        if(calibrated != rhs.calibrated)
        {
            return false;
        }
        for(int i = 0; i < 4; i++)
        {
            if(cvOffset[i] != rhs.cvOffset[i])
            {
                return false;
            }
        }
    }
    bool operator!=(const the_calibration &rhs) { return !operator==(rhs); }
};


class Settings {
    public:
    Settings() { }
    ~Settings() { }
    void Init(DaisyWyatt* hw);    
    void SaveTheSettings();
    void LoadTheSettings();
    void RestoreTheSettings();
    void SaveCalibration();
    void LoadCalibration();
    void RestoreCalibration();

    
    inline void SetTheSettingsData(the_settings thesettingsdata);
    inline void GetTheSettingsData(the_settings &thesettingsdata);

    inline void SetCalData(the_calibration caldata);
    inline void GetCalData(the_calibration &caldata);


    inline const the_settings& thesettings() const {
        return thesettings_;
    };

    inline the_settings* mutable_thesettings() {
        return &thesettings_;
    };

    inline const the_calibration& thecalibration() const {
        return thecalibration_;
    };

    inline the_calibration* mutable_thecalibration() {
        return &thecalibration_;
    };
    
    
    private:
    the_settings thesettings_;
    the_calibration thecalibration_;

    DaisyWyatt* hw_;

};

} // namespace settings

#endif  // WYATT_SETTINGS_H_