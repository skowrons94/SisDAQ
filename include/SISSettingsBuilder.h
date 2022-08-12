#ifndef SISSETTINGSBUILDER_H
#define SISSETTINGSBUILDER_H

#include <json/value.h>
#include <json/writer.h>
#include <json/json.h>

#include <map>
#include <string>
#include <sstream>

struct Setting{

    std::string reg_name;
    int         bit_lower;
    int         bit_upper;
    bool        is_adc; // If the setting address exists for different ADCs
    bool        is_cha; // If the setting address exists for different channels

    Setting( std::string name, int lower, int upper, bool adc, bool cha ){
        reg_name  = name;
        bit_lower = lower;
        bit_upper = upper;
        is_adc    = adc;
        is_cha    = cha;
    };

};

class SISSettingsBuilder {

    public:
        SISSettingsBuilder( );
        ~SISSettingsBuilder( );

        void Build( );
        void BuildRegisters( );
        void BuildSettings( );

        void AddBoardSetting( std::string name, uint32_t address, uint32_t value );
        void AddADCSetting( std::string name, uint32_t address, uint32_t value );
        void AddChannelSetting( std::string name, uint32_t address, uint32_t value );

        Json::Value GetRegisters( );
        std::map<std::string, Setting*> GetSettings( );

    private:
        
        Json::Value                    registers;
        std::map<std::string, Setting*> settings;

        std::ostringstream regName;
        std::ostringstream regAddr;
        std::ostringstream regVals;

};

#endif // SISSETTINGSBUILDER_H_INCLUDED