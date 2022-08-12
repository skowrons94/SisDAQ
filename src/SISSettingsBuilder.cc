#include <string>

#include "Registers.h"
#include "SISSettingsBuilder.h"

SISSettingsBuilder::SISSettingsBuilder( )
{

}

SISSettingsBuilder::~SISSettingsBuilder( )
{

}

void SISSettingsBuilder::AddBoardSetting( std::string name, uint32_t address, uint32_t value )
{
    regName.str("");regAddr.str("");regVals.str("");
    regName << "reg_" << std::hex << address;
    regAddr << "0x"   << std::hex << address;
    regVals << "0x"   << std::hex << value;
    registers[regName.str()]["name"]    = name;
    registers[regName.str()]["address"] = regAddr.str();
    registers[regName.str()]["value"]   = regVals.str();
}

void SISSettingsBuilder::AddADCSetting( std::string name, uint32_t address, uint32_t value )
{
    int adc = 0;
    for( uint32_t offset_adc = 0x000; offset_adc < 0x4000 ; offset_adc += 0x1000 ){
        regName.str("");regAddr.str("");regVals.str("");
        regName << "reg_" << std::hex << address+offset_adc;
        regAddr << "0x"   << std::hex << address+offset_adc;
        regVals << "0x"   << std::hex << value;
        registers[regName.str()]["name"]    = name;
        registers[regName.str()]["adc"]     = std::to_string(adc);
        registers[regName.str()]["address"] = regAddr.str();
        registers[regName.str()]["value"]   = regVals.str();
        adc++;
    }
}

void SISSettingsBuilder::AddChannelSetting( std::string name, uint32_t address, uint32_t value )
{
    int adc = 0;
    int cha = 0;
    for( uint32_t offset_adc = 0x000; offset_adc < 0x4000 ; offset_adc += 0x1000 ){
        for( uint32_t offset_cha = 0x000; offset_cha < 0x0040 ; offset_cha += 0x0010 ){
            regName.str("");regAddr.str("");regVals.str("");
            regName << "reg_" << std::hex << address+offset_adc+offset_cha;
            regAddr << "0x"   << std::hex << address+offset_adc+offset_cha;
            regVals << "0x"   << std::hex << value;
            registers[regName.str()]["name"]    = name;
            registers[regName.str()]["adc"]     = std::to_string(adc);
            registers[regName.str()]["channel"] = std::to_string(cha);
            registers[regName.str()]["address"] = regAddr.str();
            registers[regName.str()]["value"]   = regVals.str();
            cha++;
        }
        adc++;
    }
}

void SISSettingsBuilder::Build( )
{
    BuildRegisters();
    BuildSettings();
}

void SISSettingsBuilder::BuildRegisters( )
{

    AddBoardSetting( "Broadcast Setup"     , SIS3316_CBLT_BROADCAST                    ,  0x0  );
    AddBoardSetting( "ADC Frequency"       , SIS3316_ADC_CLK_OSC_I2C_REG               ,  0x21 );
    AddBoardSetting( "Clock Source"        , SIS3316_SAMPLE_CLOCK_DISTRIBUTION_CONTROL ,  0x0  );
    AddBoardSetting( "FP LVDS Bus"         , SIS3316_FP_LVDS_BUS_CONTROL               ,  0x0  );
    AddBoardSetting( "Acquisition Control" , SIS3316_ACQUISITION_CONTROL_STATUS        ,  0x0  );
    AddBoardSetting( "LEMO Out CO"         , SIS3316_LEMO_OUT_CO_SELECT_REG            ,  0x0  );
    AddBoardSetting( "LEMO Out TO"         , SIS3316_LEMO_OUT_TO_SELECT_REG            ,  0x0  );
    AddBoardSetting( "LEMO Out UO"         , SIS3316_LEMO_OUT_UO_SELECT_REG            ,  0x0  );

    AddADCSetting( "Input TAP Delay"            , SIS3316_ADC_INPUT_TAP_DELAY_REG               ,  0x1050     );
    AddADCSetting( "Gain Control"               , SIS3316_ADC_ANALOG_CTRL_REG                   ,  0x1010101  );
    AddADCSetting( "DC Offset"                  , SIS3316_ADC_DAC_OFFSET_CTRL_REG               ,  0xF80000   );
    AddADCSetting( "Event Configuration"        , SIS3316_ADC_EVENT_CONFIG_REG                  ,  0x4040404  );
    AddADCSetting( "Trigger Gate Window"        , SIS3316_ADC_TRIGGER_GATE_WINDOW_LENGTH_REG    ,  0x0        );
    AddADCSetting( "Pile-Up Window"             , SIS3316_ADC_PILEUP_CONFIG_REG                 ,  0x0        );
    AddADCSetting( "Pre-Trigger Delay"          , SIS3316_ADC_PRE_TRIGGER_DELAY_REG             ,  0x0        );
    AddADCSetting( "Data Format"                , SIS3316_ADC_DATAFORMAT_CONFIG_REG             ,  0x1010101  );
    AddADCSetting( "Internal Trigger Delay"     , SIS3316_ADC_INTERNAL_TRIGGER_DELAY_CONFIG_REG ,  0x0        );
    AddADCSetting( "Internal Gate Length"       , SIS3316_ADC_INTERNAL_GATE_LENGTH_CONFIG_REG   ,  0x0        );
    AddADCSetting( "Sum Trigger Threshold"      , SIS3316_ADC_SUM_FIR_TRIGGER_SETUP_REG         ,  0x80000064 );
    AddADCSetting( "Sum Trigger Threshold High" , SIS3316_ADC_SUM_FIR_HIGH_ENERGY_THRESHOLD_REG ,  0x80000064 );
    AddADCSetting( "Peak/Charge Configuration"  , SIS3316_ADC_PEAK_CHARGE_CONFIGURATION_REG     ,  0x0        );

    AddChannelSetting( "FIR Trigger"             , SIS3316_ADC_FIR_TRIGGER_SETUP_REG         ,  0x0        );
    AddChannelSetting( "Trigger Threshold"       , SIS3316_ADC_FIR_TRIGGER_THRESHOLD_REG     ,  0x80000064 );
    AddChannelSetting( "Trigger Threshold High"  , SIS3316_ADC_FIR_HIGH_ENERGY_THRESHOLD_REG ,  0x80000064 );
    AddChannelSetting( "Accumulator Gate"        , SIS3316_ADC_ACCUMULATOR_GATE_CONFIG_REG   ,  0x0        );
    AddChannelSetting( "FIR Energy"              , SIS3316_ADC_FIR_ENERGY_SETUP_REG          ,  0x0        );
    AddChannelSetting( "Energy Histogram"        , SIS3316_ADC_HISTOGRAM_CONF_REG            ,  0x0        );

}

void SISSettingsBuilder::BuildSettings( )
{
    settings["Enable Broadcast"]        = new Setting( "Broadcast Setup",  4,  4, false, false );
    settings["Enable Broadcast Master"] = new Setting( "Broadcast Setup",  5,  5, false, false );
    settings["Enable Address"]          = new Setting( "Broadcast Setup", 24, 31, false, false );

    settings["ADC Frequency"] = new Setting( "ADC Frequency", 0, 7, false, false );
    settings["Clock Source"]  = new Setting( "Clock Source",  0, 1, false, false );

    settings["FP Control"]          = new Setting( "FP LVDS Bus",  0, 0, false, false );
    settings["FP Status"]           = new Setting( "FP LVDS Bus",  1, 1, false, false );
    settings["FP Clock Out Enable"] = new Setting( "FP LVDS Bus",  4, 4, false, false );
    settings["FP Clock Out MUX"]    = new Setting( "FP LVDS Bus",  5, 5, false, false );

    settings["Single Bank Mode"]         = new Setting( "Acquisition Control",  0,  0, false, false );
    settings["FP-In Trigger"]            = new Setting( "Acquisition Control",  4,  4, false, false );
    settings["FP-In Veto"]               = new Setting( "Acquisition Control",  5,  5, false, false );
    settings["FP-In Control 2"]          = new Setting( "Acquisition Control",  6,  6, false, false );
    settings["FP-In Sample Control"]     = new Setting( "Acquisition Control",  7,  7, false, false );
    settings["External Trigger"]         = new Setting( "Acquisition Control",  8,  8, false, false );
    settings["External Trigger Veto"]    = new Setting( "Acquisition Control",  9,  9, false, false );
    settings["External Timestamp Clear"] = new Setting( "Acquisition Control", 10, 10, false, false );
    settings["Local Veto"]               = new Setting( "Acquisition Control", 11, 11, false, false );
    settings["NIM TI as Disarm"]         = new Setting( "Acquisition Control", 12, 12, false, false );
    settings["NIM UI as Disarm"]         = new Setting( "Acquisition Control", 13, 13, false, false );

    settings["CO Sample Clock"]         = new Setting( "LEMO Out CO",   0,  0, false, false );
    settings["CO SUM High Energy ADC1"] = new Setting( "LEMO Out CO",  16, 16, false, false );
    settings["CO SUM High Energy ADC2"] = new Setting( "LEMO Out CO",  17, 17, false, false );
    settings["CO SUM High Energy ADC3"] = new Setting( "LEMO Out CO",  18, 18, false, false );
    settings["CO SUM High Energy ADC4"] = new Setting( "LEMO Out CO",  19, 19, false, false );
    settings["CO Sample Bank Swap"]     = new Setting( "LEMO Out CO",  20, 20, false, false );
    settings["CO Sample Logic Bankx"]   = new Setting( "LEMO Out CO",  21, 21, false, false );
    settings["CO Sample Logic Bank2"]   = new Setting( "LEMO Out CO",  22, 22, false, false );

    settings["TO Trigger CH1"]        = new Setting( "LEMO Out TO",   0,  0, false, false );
    settings["TO Trigger CH2"]        = new Setting( "LEMO Out TO",   1,  1, false, false );
    settings["TO Trigger CH3"]        = new Setting( "LEMO Out TO",   2,  2, false, false );
    settings["TO Trigger CH4"]        = new Setting( "LEMO Out TO",   3,  3, false, false );
    settings["TO Trigger CH5"]        = new Setting( "LEMO Out TO",   4,  4, false, false );
    settings["TO Trigger CH6"]        = new Setting( "LEMO Out TO",   5,  5, false, false );
    settings["TO Trigger CH7"]        = new Setting( "LEMO Out TO",   6,  6, false, false );
    settings["TO Trigger CH8"]        = new Setting( "LEMO Out TO",   7,  7, false, false );
    settings["TO Trigger CH9"]        = new Setting( "LEMO Out TO",   8,  8, false, false );
    settings["TO Trigger CH10"]       = new Setting( "LEMO Out TO",   9,  9, false, false );
    settings["TO Trigger CH11"]       = new Setting( "LEMO Out TO",  10, 10, false, false );
    settings["TO Trigger CH12"]       = new Setting( "LEMO Out TO",  11, 11, false, false );
    settings["TO Trigger CH13"]       = new Setting( "LEMO Out TO",  12, 12, false, false );
    settings["TO Trigger CH14"]       = new Setting( "LEMO Out TO",  13, 13, false, false );
    settings["TO Trigger CH15"]       = new Setting( "LEMO Out TO",  14, 14, false, false );
    settings["TO Trigger CH16"]       = new Setting( "LEMO Out TO",  15, 15, false, false );
    settings["TO SUM Trigger ADC1"]   = new Setting( "LEMO Out TO",  16, 16, false, false );
    settings["TO SUM Trigger ADC2"]   = new Setting( "LEMO Out TO",  17, 17, false, false );
    settings["TO SUM Trigger ADC3"]   = new Setting( "LEMO Out TO",  18, 18, false, false );
    settings["TO SUM Trigger ADC4"]   = new Setting( "LEMO Out TO",  19, 19, false, false );
    settings["TO Sample Bank Swap"]   = new Setting( "LEMO Out TO",  20, 20, false, false );
    settings["TO Sample Logic Bankx"] = new Setting( "LEMO Out TO",  21, 21, false, false );
    settings["TO Sample Logic Bank2"] = new Setting( "LEMO Out TO",  22, 22, false, false );

    settings["UO Sample Logic"]       = new Setting( "LEMO Out UO",   0,  0, false, false );
    settings["UO Sample Logic Bankx"] = new Setting( "LEMO Out UO",   1,  1, false, false );
    settings["UO Sample ADC Busy"]    = new Setting( "LEMO Out UO",   2,  2, false, false );
    settings["UO Address Threshold"]  = new Setting( "LEMO Out UO",   3,  3, false, false );
    settings["UO Sample Event Flag"]  = new Setting( "LEMO Out UO",   4,  4, false, false );
    settings["UO Sample Bank Swap"]   = new Setting( "LEMO Out UO",  20, 20, false, false );
    settings["UO Sample Logic Bankx"] = new Setting( "LEMO Out UO",  21, 21, false, false );
    settings["UO Sample Logic Bank2"] = new Setting( "LEMO Out UO",  22, 22, false, false );

    settings["Input Tap Delay"]                = new Setting( "Input TAP Delay",  0,  7, true, false );
    settings["Input Tap Delay Channel Select"] = new Setting( "Input TAP Delay",  8,  9, true, false );
    settings["Input Tap Delay Calibration"]    = new Setting( "Input TAP Delay", 11, 11, true, false );
    settings["Input Tap Delay Add 1/2 Sample"] = new Setting( "Input TAP Delay", 12, 12, true, false );

    settings["Gain Control CH1"]    = new Setting( "Gain Control",  0,  1, true, false );
    settings["Ohm Termination CH1"] = new Setting( "Gain Control",  2,  2, true, false );
    settings["Gain Control CH2"]    = new Setting( "Gain Control",  8,  9, true, false );
    settings["Ohm Termination CH2"] = new Setting( "Gain Control", 10, 10, true, false );
    settings["Gain Control CH3"]    = new Setting( "Gain Control", 16, 17, true, false );
    settings["Ohm Termination CH3"] = new Setting( "Gain Control", 18, 18, true, false );
    settings["Gain Control CH4"]    = new Setting( "Gain Control", 24, 25, true, false );
    settings["Ohm Termination CH4"] = new Setting( "Gain Control", 26, 26, true, false );

    settings["DC Offset"]         = new Setting( "DC Offset",  4, 19, true, false );
    settings["DC Offset Address"] = new Setting( "DC Offset", 20, 23, true, false );
    settings["DC Offset Command"] = new Setting( "DC Offset", 24, 27, true, false );
    settings["DC Offset Control"] = new Setting( "DC Offset", 29, 31, true, false );

    settings["Input Invert CH1"]           = new Setting( "Event Configuration",  0,  0, true, false );
    settings["SUM Trigger Enable CH1"]     = new Setting( "Event Configuration",  1,  1, true, false );
    settings["Trigger Enable CH1"]         = new Setting( "Event Configuration",  2,  2, true, false );
    settings["EXT Trigger Enable CH1"]     = new Setting( "Event Configuration",  3,  3, true, false );
    settings["Internal Gate 1 Enable CH1"] = new Setting( "Event Configuration",  4,  4, true, false );
    settings["Internal Gate 2 Enable CH1"] = new Setting( "Event Configuration",  5,  5, true, false );
    settings["External Gate Enable CH1"]   = new Setting( "Event Configuration",  6,  6, true, false );
    settings["External Veto Enable CH1"]   = new Setting( "Event Configuration",  7,  7, true, false );
    settings["Input Invert CH2"]           = new Setting( "Event Configuration",  8,  8, true, false );
    settings["SUM Trigger Enable CH2"]     = new Setting( "Event Configuration",  9,  9, true, false );
    settings["Trigger Enable CH2"]         = new Setting( "Event Configuration", 10, 10, true, false );
    settings["EXT Trigger Enable CH2"]     = new Setting( "Event Configuration", 11, 11, true, false );
    settings["Internal Gate 1 Enable CH2"] = new Setting( "Event Configuration", 12, 12, true, false );
    settings["Internal Gate 2 Enable CH2"] = new Setting( "Event Configuration", 13, 13, true, false );
    settings["External Gate Enable CH2"]   = new Setting( "Event Configuration", 14, 14, true, false );
    settings["External Veto Enable CH2"]   = new Setting( "Event Configuration", 15, 15, true, false );
    settings["Input Invert CH3"]           = new Setting( "Event Configuration", 16, 16, true, false );
    settings["SUM Trigger Enable CH3"]     = new Setting( "Event Configuration", 17, 17, true, false );
    settings["Trigger Enable CH3"]         = new Setting( "Event Configuration", 18, 18, true, false );
    settings["EXT Trigger Enable CH3"]     = new Setting( "Event Configuration", 19, 19, true, false );
    settings["Internal Gate 1 Enable CH3"] = new Setting( "Event Configuration", 20, 20, true, false );
    settings["Internal Gate 2 Enable CH3"] = new Setting( "Event Configuration", 21, 21, true, false );
    settings["External Gate Enable CH3"]   = new Setting( "Event Configuration", 22, 22, true, false );
    settings["External Veto Enable CH3"]   = new Setting( "Event Configuration", 23, 23, true, false );
    settings["Input Invert CH4"]           = new Setting( "Event Configuration", 24, 24, true, false );
    settings["SUM Trigger Enable CH4"]     = new Setting( "Event Configuration", 25, 25, true, false );
    settings["Trigger Enable CH4"]         = new Setting( "Event Configuration", 26, 26, true, false );
    settings["EXT Trigger Enable CH4"]     = new Setting( "Event Configuration", 27, 27, true, false );
    settings["Internal Gate 1 Enable CH4"] = new Setting( "Event Configuration", 28, 28, true, false );
    settings["Internal Gate 2 Enable CH4"] = new Setting( "Event Configuration", 29, 29, true, false );
    settings["External Gate Enable CH4"]   = new Setting( "Event Configuration", 30, 30, true, false );
    settings["External Veto Enable CH4"]   = new Setting( "Event Configuration", 31, 31, true, false );

    settings["Trigger Gate Window"] = new Setting( "Trigger Gate Widnow",  0, 15, true, false );

    settings["Pile-Up Window"]    = new Setting( "Pile-Up Window",  0, 15, true, false );
    settings["Re Pile-Up Window"] = new Setting( "Pile-Up Window", 16, 31, true, false );

    settings["Pre-Trigger Delay"]            = new Setting( "Pre-Trigger Delay",  0, 10, true, false );
    settings["Additional Pre-Trigger Delay"] = new Setting( "Pre-Trigger Delay", 15, 15, true, false );

    settings["Save Peak High CH1"]    = new Setting( "Data Format",  0,  0, true, false );
    settings["Save Accumulator CH1"]  = new Setting( "Data Format",  1,  1, true, false );
    settings["Save Fast Trigger CH1"] = new Setting( "Data Format",  2,  2, true, false );
    settings["Save Start Energy CH1"] = new Setting( "Data Format",  3,  3, true, false );
    settings["Save MAW Test CH1"]     = new Setting( "Data Format",  4,  4, true, false );
    settings["Select Energy MAW CH1"] = new Setting( "Data Format",  5,  5, true, false );
    settings["Save Peak High CH2"]    = new Setting( "Data Format",  8,  8, true, false );
    settings["Save Accumulator CH2"]  = new Setting( "Data Format",  9,  9, true, false );
    settings["Save Fast Trigger CH2"] = new Setting( "Data Format", 10, 10, true, false );
    settings["Save Start Energy CH2"] = new Setting( "Data Format", 11, 11, true, false );
    settings["Save MAW Test CH2"]     = new Setting( "Data Format", 12, 12, true, false );
    settings["Select Energy MAW CH2"] = new Setting( "Data Format", 13, 13, true, false );
    settings["Save Peak High CH3"]    = new Setting( "Data Format", 16, 16, true, false );
    settings["Save Accumulator CH3"]  = new Setting( "Data Format", 17, 17, true, false );
    settings["Save Fast Trigger CH3"] = new Setting( "Data Format", 18, 18, true, false );
    settings["Save Start Energy CH3"] = new Setting( "Data Format", 19, 19, true, false );
    settings["Save MAW Test CH3"]     = new Setting( "Data Format", 20, 20, true, false );
    settings["Select Energy MAW CH3"] = new Setting( "Data Format", 21, 21, true, false );
    settings["Save Peak High CH4"]    = new Setting( "Data Format", 24, 24, true, false );
    settings["Save Accumulator CH4"]  = new Setting( "Data Format", 25, 25, true, false );
    settings["Save Fast Trigger CH4"] = new Setting( "Data Format", 26, 26, true, false );
    settings["Save Start Energy CH4"] = new Setting( "Data Format", 27, 27, true, false );
    settings["Save MAW Test CH4"]     = new Setting( "Data Format", 28, 28, true, false );
    settings["Select Energy MAW CH4"] = new Setting( "Data Format", 29, 29, true, false );

    settings["Internal Trigger Delay CH1"] = new Setting( "Internal Trigger Delay",  0,  7, true, false );
    settings["Internal Trigger Delay CH2"] = new Setting( "Internal Trigger Delay",  8, 15, true, false );
    settings["Internal Trigger Delay CH3"] = new Setting( "Internal Trigger Delay", 16, 23, true, false );
    settings["Internal Trigger Delay CH4"] = new Setting( "Internal Trigger Delay", 24, 31, true, false );

    settings["Internal Gate Coincidence"] = new Setting( "Internal Gate Length",  0,  7, true, false );
    settings["Internal Gate Length"]      = new Setting( "Internal Gate Length",  8, 15, true, false );
    settings["Internal Gate 1 Enable"]    = new Setting( "Internal Gate Length", 16, 20, true, false );
    settings["Internal Gate 2 Enable"]    = new Setting( "Internal Gate Length", 20, 23, true, false );

    settings["Peaking Time Trigger"] = new Setting( "FIR Trigger",  0, 11, true, true );
    settings["Gap Time Trigger"]     = new Setting( "FIR Trigger", 12, 23, true, true );
    settings["NIM Out Pulse Length"] = new Setting( "FIR Trigger", 24, 31, true, true );

    settings["Trigger Threshold"]    = new Setting( "Trigger Threshold",  0, 27, true, true );
    settings["CFD Control"]          = new Setting( "Trigger Threshold", 28, 29, true, true );
    settings["High Energy Suppress"] = new Setting( "Trigger Threshold", 30, 30, true, true );
    settings["Trigger Enable"]       = new Setting( "Trigger Threshold", 31, 31, true, true );

    settings["Sum Trigger Threshold"]    = new Setting( "Sum Trigger Threshold",  0, 27, true, false );
    settings["Sum CFD Control"]          = new Setting( "Sum Trigger Threshold", 28, 29, true, false );
    settings["Sum High Energy Suppress"] = new Setting( "Sum Trigger Threshold", 30, 30, true, false );
    settings["Sum Trigger Enable"]       = new Setting( "Sum Trigger Threshold", 31, 31, true, false );

    settings["Trigger Threshold High"] = new Setting( "Trigger Threshold High",  0, 27, true, true );
    settings["Stretched Trigger Out"]  = new Setting( "Trigger Threshold High", 28, 28, true, true );
    settings["Trigger on Both Edges"]  = new Setting( "Trigger Threshold High", 31, 31, true, true );

    settings["Sum Trigger Threshold High"] = new Setting( "Sum Trigger Threshold High",  0, 27, true, false );
    settings["Sum Stretched Trigger Out"]  = new Setting( "Sum Trigger Threshold High", 28, 28, true, false );
    settings["Sum Trigger on Both Edges"]  = new Setting( "Sum Trigger Threshold High", 31, 31, true, false );

    settings["Baseline Pre-Gate Delay"] = new Setting( "Peak/Charge Configuration", 16, 27, true, false );
    settings["Baseline Average Mode"]   = new Setting( "Peak/Charge Configuration", 28, 29, true, false );
    settings["Enable Peak/Charge Mode"] = new Setting( "Peak/Charge Configuration", 31, 31, true, false );

    settings["Gate Start Index"] = new Setting( "Accumulator Gate",  0, 15, true, true );
    settings["Gate Length"]      = new Setting( "Accumulator Gate", 16, 24, true, true );

    settings["Peaking Time Energy"] = new Setting( "FIR Energy",  0, 11, true, true );
    settings["Gap Time Energy"]     = new Setting( "FIR Energy", 12, 21, true, true );
    settings["Extra Filter"]        = new Setting( "FIR Energy", 22, 23, true, true );
    settings["Tau Factor"]          = new Setting( "FIR Energy", 24, 29, true, true );
    settings["Tau Table"]           = new Setting( "FIR Energy", 30, 31, true, true );

    settings["Histogram Enable"]        = new Setting( "Energy Histogram",  0,  0, true, true );
    settings["Pile-Up Enable"]          = new Setting( "Energy Histogram",  1,  1, true, true );
    settings["Subtract Offset"]         = new Setting( "Energy Histogram",  8, 11, true, true );
    settings["Divider"]                 = new Setting( "Energy Histogram", 16, 27, true, true );
    settings["Histogram Clear Disable"] = new Setting( "Energy Histogram", 30, 30, true, true );
    settings["Writing Events Disable"]  = new Setting( "Energy Histogram", 31, 31, true, true );

}

Json::Value SISSettingsBuilder::GetRegisters( )
{
    return this->registers;
}

std::map<std::string, Setting*> SISSettingsBuilder::GetSettings( )
{
    return this->settings;
}