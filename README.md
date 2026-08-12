# WYATT - A Daisy Seed based Eurorack module

> [!NOTE]
> This is a DIY project. Use at your own risk.

## Features
- Daisy Seed based
- Big color tft (ili9341)
- 8 Push buttons
- 1 Button with red/green led
- 4 Encoder
- 4 Pots
- Stereo IO
- 1 Gate/Trig in
- Midi IO (TRS Type B)
- 3 CV ins
- Micro SD Card
- 20 HP
<br/>

## Examples
| Project name | Description | App type | Version |
| ------------ | -------- | -----------| --------- |
| [Grainstorm](Examples/grainstorm/) | Granular synth / effect | BOOT_SRAM | [grainstorm-0.21.bin](Examples/grainstorm/firmware/) |
| [hwtest](Examples/hwtest/) | Wyatt Hardware test | BOOT_NONE | [hwtest-1.0.bin](Examples/hwtest/firmware/) |
<br/>
<br/>

> [!IMPORTANT]
> For the TFT display to work correctly, you need to change the following line in libDaisy:
> https://github.com/electro-smith/libDaisy/blob/master/src/sys/system.cpp#L528 to MPU_InitStruct.Size = MPU_REGION_SIZE_256KB;


## Images
![maasijam wyatt panel](Hardware/Images/DSC00858.jpg)
![maasijam wyatt back](Hardware/Images/DSC00860.jpg)