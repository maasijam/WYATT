#ifndef TFT_LIB_H_
#define TFT_LIB_H_

#include "../Tft/ili9341_ui_driver.hpp"
#include <vector>
#include <string>

#define PARAM_BUFFER_SIZE 8

namespace tftlib {




class Tft_lib {
    public:
    Tft_lib() { }
    ~Tft_lib() { }
    void Init(ILI9341UiDriver* tft);    
    
    void RenderTabRect(int tab);
    void RenderAreaInd(int row, int rowOffset, int heightAdd = 0);
    void RenderParamChar(int pos, int row, int rowOffset, int value, const char *str, const char** labels);
    void RenderParamNoteChar(int pos, int row, int rowOffset, int value, const char *str, const char** labels, int noteSize, std::vector<int> scale);
    void RenderParamIntToStr(int pos, int row, int rowOffset, int value, const char *str, int idx);
    void RenderParamSw(int pos, int row, int rowOffset, bool value, const char *str);
    void RenderParam(int pos, int row, int rowOffset, int value, const char *str);
    void RenderBigBtn(int pos, int row, int rowOffset, bool encState, int area, int cur_area, const char *str1, const char *str2);
    void RenderRowDrumHeader(int x, int y, bool drumGate, bool drumGateEnc, const char *str);
    void RenderRowTabHeader(int area, const char** tabLabels, int arrSize);
    void RenderPadParam(int pos, int row, int rowOffset, int value, int idx, const char *str);
    void RenderPotParam(int pos, int value, const char *str); 
    void RenderToggleBtnBottom(int pos, int value,bool encState, const char *str1,const char *str2);
    void RenderPlayStopBtn(int pos, int row, int rowOffset, bool btnState, const char *str, TFT_COLOR activeColor,TFT_COLOR activeTxtColor, bool isStop = false);
    void RenderCvSelector(int pos, int row, int rowOffset,int cvVal, int attVal, const char *str, const char** cv_param_labels);
    void RenderWavSelector(int pos, int row, int rowOffset,int wavVal, const char *label, std::vector<std::string> wavnames);
    void RenderRowHeader(int pos, int row, int rowOffset, const char *str);
    void RenderLabel(int pos, int row, int rowOffset, const char *str1, TFT_COLOR bgColor = COLOR_GRAY,TFT_COLOR txtColor = COLOR_WHITE);
    void RenderSavePreset();
    void RenderRestorePreset();

    char* GetIntAsString(int val);
    int map(int x, int in_min, int in_max, int out_min, int out_max);

    int tab = 0;
    int area[4] = {0,0,0,0};

    private:
    ILI9341UiDriver* tft_;

    char val_str_[PARAM_BUFFER_SIZE];

      
    

    
};

} // namespace tftlib

#endif  // TFT_LIB_H_