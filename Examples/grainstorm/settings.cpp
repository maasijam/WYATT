#include "Settings.h"

using namespace settings;
using namespace daisy;
using namespace wyatt;




void Settings::Init(DaisyWyatt* hw) 
{
    hw_ = hw;    
    LoadCalibration();
    LoadTheSettings();
    //RestoreTheSettings();
    //RestoreCalibration();
}


 /** @brief Sets the cv offset from an externally array of data */
inline void Settings::SetTheSettingsData(the_settings thesettings_data)
{
    thesettings_ = thesettings_data;
      
}
/** @brief Sets the cv offset from an externally array of data */
inline void Settings::GetTheSettingsData(the_settings &thesettings_data)
{
    thesettings_data = thesettings_;
       
}

 /** @brief Sets the cv offset from an externally array of data */
inline void Settings::SetCalData(the_calibration caldata)
{
    thecalibration_ = caldata;
      
}
/** @brief Sets the cv offset from an externally array of data */
inline void Settings::GetCalData(the_calibration &caldata)
{
    caldata = thecalibration_;
}

/** @brief Loads and sets settings data */
void Settings::LoadTheSettings()
{
    daisy::PersistentStorage<the_settings> thesettings_storage(hw_->seed.qspi);
    the_settings default_thesettings;
    thesettings_storage.Init(default_thesettings, FLASH_BLOCK*0);
    the_settings &thesettings_data = thesettings_storage.GetSettings();
    SetTheSettingsData(thesettings_data);
        
}
void Settings::SaveTheSettings()
{
    daisy::PersistentStorage<the_settings> thesettings_storage(hw_->seed.qspi);
    the_settings default_thesettings;
    thesettings_storage.Init(default_thesettings, FLASH_BLOCK*0);
    the_settings &thesettings_data = thesettings_storage.GetSettings();
    
    GetTheSettingsData(thesettings_data);   
    thesettings_storage.Save();
}
void Settings::RestoreTheSettings()
{
    daisy::PersistentStorage<the_settings> thesettings_storage(hw_->seed.qspi);
    the_settings default_thesettings;
    thesettings_storage.Init(default_thesettings, FLASH_BLOCK*0);
    thesettings_storage.RestoreDefaults();
    the_settings &thesettings_data = thesettings_storage.GetSettings();
    SetTheSettingsData(thesettings_data);
}

/** @brief Loads and sets calibration data */
void Settings::LoadCalibration()
{
    daisy::PersistentStorage<the_calibration> cal_storage(hw_->seed.qspi);
    the_calibration default_cvcal;
    cal_storage.Init(default_cvcal, FLASH_BLOCK*2);
    the_calibration &cvcal_data = cal_storage.GetSettings();
    SetCalData(cvcal_data);
        
}
void Settings::SaveCalibration()
{
    daisy::PersistentStorage<the_calibration> cal_storage(hw_->seed.qspi);
    the_calibration default_cvcal;
    cal_storage.Init(default_cvcal, FLASH_BLOCK*2);
    the_calibration &cvcal_data = cal_storage.GetSettings();
    
    GetCalData(cvcal_data);   
    cal_storage.Save();
}
void Settings::RestoreCalibration()
{
    daisy::PersistentStorage<the_calibration> cal_storage(hw_->seed.qspi);
    the_calibration default_cvcal;
    cal_storage.Init(default_cvcal, FLASH_BLOCK*2);
    cal_storage.RestoreDefaults();
    the_calibration &cvcal_data = cal_storage.GetSettings();
    SetCalData(cvcal_data);
}