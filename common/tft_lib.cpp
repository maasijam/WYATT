#include "tft_lib.h"

using namespace tftlib;
using namespace daisy;


void Tft_lib::Init(
    ILI9341UiDriver *tft)
{
    tft_ = tft;
}

void Tft_lib::RenderTabRect(int tab)
{
    
    tft_->FillRect(Rectangle(0, 0, 75, 6), tab == 0 ? COLOR_YELLOW : COLOR_GRAY);
    tft_->FillRect(Rectangle(82, 0, 75, 6), tab == 1 ? COLOR_YELLOW : COLOR_GRAY);
    tft_->FillRect(Rectangle(164, 0, 75, 6), tab == 2 ? COLOR_YELLOW : COLOR_GRAY);
    tft_->FillRect(Rectangle(245, 0, 75, 6), tab == 3 ? COLOR_YELLOW : COLOR_GRAY);
    
}

void Tft_lib::RenderAreaInd(int row, int rowOffset, int heightAdd) 
{
    int y = 20 + ((row + rowOffset) * 45);
    int height = 37 + heightAdd;
    tft_->FillRect(Rectangle(0, y, 6, height), area[tab] == row ? COLOR_YELLOW : COLOR_GRAY);
}

void Tft_lib::RenderParamChar(int pos, int row, int rowOffset, int value, const char *str, const char** labels) 
{
    const char *label;
    label = labels[value];
    int x = 20 + (pos * 75);
    int y = 20 + ((row + rowOffset) * 45);
    
    tft_->DrawRect(x,y,68,36,COLOR_WHITE);
    tft_->FillRect(Rectangle(x + 1, y + 20, 67, 16), COLOR_GRAY);
    tft_->WriteStringAligned(str,Font_7x10,Rectangle(x,y-1,68,26),Alignment::centered,COLOR_WHITE);
    tft_->WriteStringAligned(label,Font_7x10,Rectangle(x,y+21,68,15),Alignment::centered,COLOR_WHITE);
}

void Tft_lib::RenderParamNoteChar(int pos, int row, int rowOffset, int value, const char *str, const char** labels, int noteSize, std::vector<int> scale) 
{
    const char *label;
    const char *octave = GetIntAsString(value / noteSize);
    int labelValue = value % noteSize;

    label = labels[scale[labelValue]];
    int x = 20 + (pos * 75);
    int y = 20 + ((row + rowOffset) * 45);

    char char_buf[128];
    strcpy(char_buf, label);
    strcat(char_buf, octave);
    
    tft_->DrawRect(x,y,68,36,COLOR_WHITE);
    tft_->FillRect(Rectangle(x + 1, y + 20, 67, 16), COLOR_GRAY);
    tft_->WriteStringAligned(str,Font_7x10,Rectangle(x,y-1,68,26),Alignment::centered,COLOR_WHITE);
    tft_->WriteStringAligned(char_buf,Font_7x10,Rectangle(x,y+21,68,15),Alignment::centered,COLOR_WHITE);
}

void Tft_lib::RenderParamIntToStr(int pos, int row, int rowOffset, int value, const char *str, int idx) 
{
    int x = 20 + (pos * 75);
    int y = 20 + ((row + rowOffset) * 45);

    tft_->DrawRect(x,y,68,36,COLOR_WHITE);
    tft_->FillRect(Rectangle(x + 1, y + 21, 67, 15), COLOR_GRAY);
	tft_->WriteStringAligned(GetIntAsString(value),Font_7x10,Rectangle(x+1,y+21,67,16),Alignment::centered,COLOR_WHITE);
    tft_->WriteStringAligned(str,Font_7x10,Rectangle(x,y-1,68,26),Alignment::centered,COLOR_WHITE);
}

void Tft_lib::RenderParamSw(int pos, int row, int rowOffset, bool value, const char *str) 
{
    int x = 20 + (pos * 75);
    int y = 20 + ((row + rowOffset) * 45);

    tft_->DrawRect(x,y,68,36,COLOR_WHITE);
    
	if(value) {
		tft_->FillRect(Rectangle(x + 1, y + 21, 67, 15), COLOR_GRAY);
		tft_->FillRect(Rectangle(x + 34, y + 22, 33, 13), COLOR_YELLOW);
		tft_->WriteStringAligned("ON",Font_7x10,Rectangle(x+34,y+22,33,16),Alignment::centered,COLOR_BLACK);
	} else {
		tft_->FillRect(Rectangle(x + 1, y + 21, 67, 15), COLOR_GRAY);
		tft_->FillRect(Rectangle(x + 2, y + 22, 33, 13), COLOR_WHITE);
		tft_->WriteStringAligned("OFF",Font_7x10,Rectangle(x+1,y+22,33,16),Alignment::centered,COLOR_BLACK);
	}
    
    tft_->WriteStringAligned(str,Font_7x10,Rectangle(x,y-1,68,26),Alignment::centered,COLOR_WHITE);
}

void Tft_lib::RenderPadParam(int pos, int row, int rowOffset, int value, int idx, const char *str) 
{
    int x = 20 + (pos * 75);
    int y = 20 + ((row  + rowOffset) * 45);
    int paramPos = map(value,0,100,0,61);
    tft_->DrawRect(x,y,68,36,COLOR_WHITE);
    tft_->FillRect(Rectangle(x + 1, y + 21, 67, 15), COLOR_YELLOW);
    tft_->FillRect(Rectangle(x + 1 + paramPos, y + 22, 6, 14), COLOR_BLACK);

    tft_->WriteStringAligned(str,Font_7x10,Rectangle(x,y-1,68,24),Alignment::centered,COLOR_WHITE);
}

void Tft_lib::RenderParam(int pos, int row, int rowOffset, int value, const char *str) 
{
    int x = 20 + (pos * 75);
    int y = 20 + ((row + rowOffset) * 45);
    int paramPos = map(value,0,100,0,61);
    tft_->DrawRect(x,y,68,36,COLOR_WHITE);
    tft_->FillRect(Rectangle(x + 1, y + 21, 67, 15), COLOR_YELLOW);
    tft_->FillRect(Rectangle(x + 1 + paramPos, y + 22, 6, 14), COLOR_BLACK);
    tft_->WriteStringAligned(str,Font_7x10,Rectangle(x,y-1,68,26),Alignment::centered,COLOR_WHITE);
}

void Tft_lib::RenderBigBtn(int pos, int row, int rowOffset, bool encState, int area, int cur_area,  const char *str1, const char *str2) 
{
    int x = 20 + (pos * 75);
    int y = 20 + ((row + rowOffset) * 45);

    tft_->FillRect(Rectangle(x, y, 69, 37), encState && cur_area == area ? COLOR_YELLOW : COLOR_GRAY);

    tft_->WriteStringAligned(str1,Font_7x10,Rectangle(x,y+1,69,19),Alignment::centered, encState && cur_area == area ? COLOR_BLACK : COLOR_WHITE);
    tft_->WriteStringAligned(str2,Font_7x10,Rectangle(x,y+19,69,19),Alignment::centered, encState && cur_area == area ? COLOR_BLACK : COLOR_WHITE);
    tft_->DrawPixel(x,y,COLOR_BLACK);
    tft_->DrawPixel(x+1,y,COLOR_BLACK);
    tft_->DrawPixel(x+2,y,COLOR_BLACK);
    tft_->DrawPixel(x,y+1,COLOR_BLACK);
    tft_->DrawPixel(x,y+2,COLOR_BLACK);

    tft_->DrawPixel(x+68,y,COLOR_BLACK);
    tft_->DrawPixel(x+67,y,COLOR_BLACK);
    tft_->DrawPixel(x+66,y,COLOR_BLACK);
    tft_->DrawPixel(x+68,y+1,COLOR_BLACK);
    tft_->DrawPixel(x+68,y+2,COLOR_BLACK);

    tft_->DrawPixel(x+68,y+36,COLOR_BLACK);
    tft_->DrawPixel(x+68,y+35,COLOR_BLACK);
    tft_->DrawPixel(x+68,y+34,COLOR_BLACK);
    tft_->DrawPixel(x+67,y+36,COLOR_BLACK);
    tft_->DrawPixel(x+66,y+36,COLOR_BLACK);

    tft_->DrawPixel(x,y+36,COLOR_BLACK);
    tft_->DrawPixel(x+1,y+36,COLOR_BLACK);
    tft_->DrawPixel(x+2,y+36,COLOR_BLACK);
    tft_->DrawPixel(x,y+35,COLOR_BLACK);
    tft_->DrawPixel(x,y+34,COLOR_BLACK);
}

void Tft_lib::RenderPotParam(int pos, int value, const char *str) 
{
    int x = 20 + (pos * 75);
    int y = 205;
    
    int paramPos = map(value,0,100,0,61);
    tft_->DrawRect(x,y,68,25,COLOR_WHITE);
    tft_->FillRect(Rectangle(x + 1, y + 18, 67, 7), COLOR_YELLOW);
    tft_->FillRect(Rectangle(x + 1 + paramPos, y + 19, 6, 6), COLOR_BLACK);
    tft_->WriteStringAligned(str,Font_7x10,Rectangle(x,y,68,21),Alignment::centered,COLOR_WHITE);

    if(pos == 0) {

    int centerX = 7;
    int centerY = 217;
    int radius = 7;

    tft_->DrawCircle(centerX, centerY, radius, COLOR_WHITE);
    // Map the encoder value to an angle (0 to 360 degrees)
    float angle = map(50, 0, 100, 135, 405);
    // Convert angle to radians for trigonometric functions
    float radians =   angle * (M_PI / 180);
        
    int line0AX = centerX + (radius - 0) * cos(radians);
    int line0AY = centerY + (radius - 0) * sin(radians);

    int line0BX = centerX + (radius - 5) * cos(radians);
    int line0BY = centerY + (radius - 5) * sin(radians);

    tft_->DrawLine(line0AX,line0AY,line0BX,line0BY,COLOR_WHITE);
    }
}

void Tft_lib::RenderCvSelector(int pos, int row, int rowOffset,int cvVal, int attVal, const char *str, const char** cv_param_labels)
{
    int x = 20 + (pos * 75);
    int y = 20 + ((row + rowOffset) * 45);
    int cvattPos;
    
    tft_->DrawRect(x,y,143,29,COLOR_WHITE);
    tft_->FillRect(Rectangle(x+1, y+1, 50, 28), COLOR_GRAY);
    tft_->DrawLine(x+50,y,x+50,y+29,COLOR_WHITE);
    tft_->WriteStringAligned(str,Font_7x10,Rectangle(x+10,y+1,30,29),Alignment::centered,COLOR_WHITE);
   
    cvattPos = map(attVal,0,100,3,144);
    tft_->WriteStringAligned(cv_param_labels[cvVal],Font_7x10,Rectangle(x+58,y+1,80,29),Alignment::centered,COLOR_YELLOW);
       
    tft_->DrawRect(x,y+29,143,7,COLOR_WHITE);
    tft_->FillRect(Rectangle(x+1, y+30, cvattPos, 6), COLOR_WHITE);
}

void Tft_lib::RenderWavSelector(int pos, int row, int rowOffset,int wavVal, const char *label, std::vector<std::string> wavnames)
{
    int x = 20 + (pos * 75);
    int y = 20 + ((row + rowOffset) * 45);
    
    tft_->DrawRect(x,y,292,37,COLOR_WHITE);
    tft_->FillRect(Rectangle(x+1, y+1, 50, 36), COLOR_GRAY);
    tft_->DrawLine(x+50,y,x+50,y+36,COLOR_WHITE);
    tft_->WriteStringAligned(label,Font_7x10,Rectangle(x+10,y+1,30,37),Alignment::centered,COLOR_WHITE);
    if(wavVal < 0) {
        tft_->WriteStringAligned("---",Font_7x10,Rectangle(x+50,y+1,230,37),Alignment::centeredRight,COLOR_YELLOW);
    } else {
        tft_->WriteStringAligned(wavnames[wavVal].c_str(),Font_7x10,Rectangle(x+50,y+1,230,37),Alignment::centeredRight,COLOR_YELLOW);
    }
}

void Tft_lib::RenderRowDrumHeader(int x, int y, bool drumGate, bool drumGateEnc, const char *str) 
{
    
    tft_->FillRect(Rectangle(x , y , 59, 16), COLOR_GRAY);
    tft_->DrawLine(x,y+15,x+270,y+15,COLOR_GRAY); 
    tft_->WriteStringAligned(str,Font_7x10,Rectangle(x,y-1,60,20),Alignment::centered,COLOR_WHITE);
    if(drumGate || drumGateEnc) {
        tft_->FillRect(Rectangle(x + 62 , y , 15, 15), COLOR_BLUE);
    }
    
}
void Tft_lib::RenderRowTabHeader(int area, const char** tabLabels, int arrSize) 
{
    int x = 20;
    int y = 25;
   
    for (int i = 0; i < arrSize; i++)
    {
        tft_->FillRect(Rectangle(x + (i*75) , y , 68, 25), area == i ? COLOR_YELLOW : COLOR_GRAY);
        tft_->WriteStringAligned(tabLabels[i],Font_7x10,Rectangle(x + (i*75),y-1,69,28),Alignment::centered, area == i ? COLOR_BLACK : COLOR_WHITE);
    }

    tft_->DrawLine(x,y+24,x+292,y+24,COLOR_YELLOW);     
    
}

void Tft_lib::RenderRowHeader(int pos, int row, int rowOffset, const char *str) 
{
    int x = 20 + (pos * 75);
    int y = 30 + ((row + rowOffset) * 45);
    //tft.DrawRect(x,y,60,18,COLOR_WHITE);
    size_t len = std::strlen(str);
    
    tft_->FillRect(Rectangle(x , y , len*8, 20), COLOR_GRAY);
    tft_->DrawLine(x,y+19,x+293,y+19,COLOR_GRAY); 
    tft_->WriteStringAligned(str,Font_7x10,Rectangle(x,y-1,len*8,22),Alignment::centered,COLOR_WHITE);
    
}

void Tft_lib::RenderToggleBtnBottom(int pos, int value,bool encState, const char *str1,const char *str2) 
{
    int x = 20 + (pos * 75);
    int y = 213;
    tft_->FillRect(Rectangle(x,y,68,25), encState ? COLOR_YELLOW : COLOR_GRAY);
    tft_->WriteStringAligned(encState ? str2 : str1,Font_7x10,Rectangle(x+22,y+2,50,25),Alignment::centeredLeft,encState ? COLOR_BLACK : COLOR_WHITE);
    if(!encState) {
        tft_->DrawLine(x+8,y+8,x+8,y+17,COLOR_WHITE);
        tft_->DrawLine(x+9,y+9,x+9,y+16,COLOR_WHITE);
        tft_->DrawLine(x+10,y+10,x+10,y+15,COLOR_WHITE);
        tft_->DrawLine(x+11,y+11,x+11,y+14,COLOR_WHITE);
        tft_->DrawLine(x+12,y+12,x+12,y+13,COLOR_WHITE);
    } else {
        tft_->FillRect(Rectangle(x+8,y+8,9,9), COLOR_BLACK);
    }
    
}

void Tft_lib::RenderPlayStopBtn(int pos, int row, int rowOffset, bool btnState, const char *str, TFT_COLOR activeColor,TFT_COLOR activeTxtColor, bool isStop) 
{
    int x = 20 + (pos * 75);
    int y = 20 + ((row + rowOffset) * 45);

    tft_->FillRect(Rectangle(x, y, 69, 37), btnState ? activeColor : COLOR_GRAY);
    tft_->WriteStringAligned(str,Font_7x10,Rectangle(x+22,y+2,69,37),Alignment::centeredLeft,btnState ? activeTxtColor : COLOR_WHITE);
    if(!isStop) {
        tft_->DrawLine(x+8,y+14,x+8,y+23,btnState ? activeTxtColor : COLOR_WHITE);
        tft_->DrawLine(x+9,y+15,x+9,y+22,btnState ? activeTxtColor :COLOR_WHITE);
        tft_->DrawLine(x+10,y+16,x+10,y+21,btnState ? activeTxtColor :COLOR_WHITE);
        tft_->DrawLine(x+11,y+17,x+11,y+20,btnState ? activeTxtColor :COLOR_WHITE);
        tft_->DrawLine(x+12,y+18,x+12,y+19,btnState ? activeTxtColor :COLOR_WHITE);
    } else {
        tft_->FillRect(Rectangle(x+8,y+15,8,8), btnState ? activeTxtColor :COLOR_WHITE);
    }
        tft_->DrawPixel(x,y,COLOR_BLACK);
    tft_->DrawPixel(x+1,y,COLOR_BLACK);
    tft_->DrawPixel(x+2,y,COLOR_BLACK);
    tft_->DrawPixel(x,y+1,COLOR_BLACK);
    tft_->DrawPixel(x,y+2,COLOR_BLACK);

    tft_->DrawPixel(x+68,y,COLOR_BLACK);
    tft_->DrawPixel(x+67,y,COLOR_BLACK);
    tft_->DrawPixel(x+66,y,COLOR_BLACK);
    tft_->DrawPixel(x+68,y+1,COLOR_BLACK);
    tft_->DrawPixel(x+68,y+2,COLOR_BLACK);

    tft_->DrawPixel(x+68,y+36,COLOR_BLACK);
    tft_->DrawPixel(x+68,y+35,COLOR_BLACK);
    tft_->DrawPixel(x+68,y+34,COLOR_BLACK);
    tft_->DrawPixel(x+67,y+36,COLOR_BLACK);
    tft_->DrawPixel(x+66,y+36,COLOR_BLACK);

    tft_->DrawPixel(x,y+36,COLOR_BLACK);
    tft_->DrawPixel(x+1,y+36,COLOR_BLACK);
    tft_->DrawPixel(x+2,y+36,COLOR_BLACK);
    tft_->DrawPixel(x,y+35,COLOR_BLACK);
    tft_->DrawPixel(x,y+34,COLOR_BLACK);
    
    
}

char* Tft_lib::GetIntAsString(int val) { 
    snprintf(val_str_, PARAM_BUFFER_SIZE, "%d", val);
    return val_str_; 
}

int Tft_lib::map(int x, int in_min, int in_max, int out_min, int out_max) {
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void Tft_lib::RenderSavePreset()
{
    tft_->WriteStringAligned("SAVE PRESET?",Font_11x18,Rectangle(0,80,320,34),Alignment::centered,COLOR_WHITE);
    
    tft_->FillRect(Rectangle(0, 213, 75, 26), COLOR_BLUE);
    tft_->FillRect(Rectangle(245, 213, 75, 26), COLOR_RED);

    tft_->WriteStringAligned("YES",Font_7x10,Rectangle(0, 213, 75, 26),Alignment::centered,COLOR_WHITE);
    tft_->WriteStringAligned("NO",Font_7x10,Rectangle(245, 213, 75, 26),Alignment::centered,COLOR_WHITE);
}

void Tft_lib::RenderRestorePreset()
{
    tft_->WriteStringAligned("RESTORE PRESET?",Font_11x18,Rectangle(0,80,320,34),Alignment::centered,COLOR_WHITE);
    tft_->WriteStringAligned("Turn off the module after successful recovery.",Font_7x10,Rectangle(0,110,320,34),Alignment::centered,COLOR_WHITE);
    
    tft_->FillRect(Rectangle(0, 213, 75, 26), COLOR_BLUE);
    tft_->FillRect(Rectangle(245, 213, 75, 26), COLOR_RED);

    tft_->WriteStringAligned("YES",Font_7x10,Rectangle(0, 213, 75, 26),Alignment::centered,COLOR_WHITE);
    tft_->WriteStringAligned("NO",Font_7x10,Rectangle(245, 213, 75, 26),Alignment::centered,COLOR_WHITE);
}

void Tft_lib::RenderLabel(int pos, int row, int rowOffset, const char *str, TFT_COLOR bgColor,TFT_COLOR txtColor) 
{
    int x = 20 + (pos * 75);
    int y = 20 + ((row + rowOffset) * 45);

    tft_->FillRect(Rectangle(x, y, 69, 37), bgColor);
    tft_->WriteStringAligned(str,Font_7x10,Rectangle(x,y+1,69,37),Alignment::centered, txtColor);
 
}

void Tft_lib::RenderSplash(const char *project_name, const char *version) {
    tft_->WriteStringAligned("WYATT",Font_16x26,Rectangle(0,30,320,34),Alignment::centered,COLOR_YELLOW);
    tft_->WriteStringAligned(project_name,Font_11x18,Rectangle(0,70,320,34),Alignment::centered,COLOR_YELLOW);
    tft_->WriteStringAligned(version,Font_11x18,Rectangle(0,110,320,34),Alignment::centered,COLOR_YELLOW);
}

void Tft_lib::RenderWindow(int pos, int row, int rowOffset, int value, const char *str) 
{
    int x = 20 + (pos * 75);
    int y = 20 + ((row + rowOffset) * 45);
    
    int lineLeft = map(value,0,100,34,10);
    int lineRight = map(value,0,100,34,58);

    tft_->DrawRect(x,y,68,36,COLOR_WHITE);
    tft_->FillRect(Rectangle(x + 1, y + 21, 67, 15), COLOR_YELLOW);
    
    tft_->DrawLine(x+5,y+35,x+lineLeft,y+23,COLOR_BLACK);
    tft_->DrawLine(x+lineRight,y+23,x+63,y+35,COLOR_BLACK);
    if(value > 0) {
        tft_->DrawLine(x+lineLeft,y+23,x+lineRight,y+23,COLOR_BLACK);
    }

    tft_->WriteStringAligned(str,Font_7x10,Rectangle(x,y-1,68,26),Alignment::centered,COLOR_WHITE);
}

void Tft_lib::RenderAudioMeter(int pos, int row, int rowOffset, int value, float left, float right, const char *str) 
{
    int x = 20 + (pos * 75);
    int y = 20 + ((row + rowOffset) * 45);
    float scale = 1.8f;
    TFT_COLOR color[2];
    float meterpos[2];

    tft_->DrawRect(x,y,68,36,COLOR_WHITE);
    tft_->DrawRect(x+68,y,225,36,COLOR_WHITE);
    tft_->FillRect(Rectangle(x + 1, y + 21, 67, 15), COLOR_GRAY);
	tft_->WriteStringAligned(GetIntAsString(value),Font_7x10,Rectangle(x+1,y+21,67,16),Alignment::centered,COLOR_WHITE);
    tft_->WriteStringAligned(str,Font_7x10,Rectangle(x,y-1,68,26),Alignment::centered,COLOR_WHITE);

    
    
    int totalPixels = 205;
    float normalizedLeft = linearToNormalizedDb(left);
    int litPixelsLeft = (int)(calculateBarWidth(normalizedLeft) * totalPixels * scale);

    for (int i = 0; i < litPixelsLeft; i++) {
        meterpos[0] = (float)i / (float)totalPixels; // 0.0 .. 1.0
        if (meterpos[0] < 0.70f) {
            color[0] = COLOR_GREEN;
        } else if (meterpos[0] < 0.90f) {
            color[0] = COLOR_YELLOW;
        } else {
            color[0] = COLOR_RED;
        }
        
        tft_->DrawLine(x + 78 + i, y + 6, x + 78 + i, y + 16, color[0]);
    }

    float normalizedRight = linearToNormalizedDb(right);
    int litPixelsRight = (int)(calculateBarWidth(normalizedRight) * totalPixels * scale);

    for (int i = 0; i < litPixelsRight; i++) {
        meterpos[1] = (float)i / (float)totalPixels; // 0.0 .. 1.0
        if (meterpos[1] < 0.70f) {
            color[1] = COLOR_GREEN;
        } else if (meterpos[1] < 0.90f) {
            color[1] = COLOR_YELLOW;
        } else {
            color[1] = COLOR_RED;
        }
        
        tft_->DrawLine(x + 78 + i, y + 21, x + 78 + i, y + 31, color[1]);
    }
    
}

float Tft_lib::calculateBarWidth(float audioSample) {
        
        float level_ = 0.0f;
        float attackCoeff_  = 0.5f; 
        float releaseCoeff_ = 0.05f;
        int clipHoldCounter_ = 0;
        const int clipHoldFrames_ = 20;

        float absVal = fabsf(audioSample);
        if (absVal > 1.0f) absVal = 1.0f;

        if (absVal > level_) {
            // Attack: schnell ansteigen
            level_ = level_ + (absVal - level_) * attackCoeff_;
        } else {
            // Release: langsam abfallen
            level_ = level_ + (absVal - level_) * releaseCoeff_;
        }

        // Übersteuerungs-Erkennung (Clip-Hold)
        if (absVal >= 0.999f) {
            clipHoldCounter_ = clipHoldFrames_;
        } else if (clipHoldCounter_ > 0) {
            clipHoldCounter_--;
        }

        return level_;
    }

float Tft_lib::linearToNormalizedDb(float linear) {
    float minDb_ = -40.0f;  // "leise" -> unteres Ende der Anzeige
    float maxDb_ = 0.0f;    
    if (linear < 0.00001f) linear = 0.00001f; // -100dB Untergrenze, verhindert log(0)

    float db = 20.0f * log10f(linear);        // linear -> dB
    db = clamp(db, minDb_, maxDb_);

    float normalized = (db - minDb_) / (maxDb_ - minDb_);
    return normalized;
}