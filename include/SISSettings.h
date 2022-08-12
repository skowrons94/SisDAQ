#ifndef SISSETTINGS_H
#define SISSETTINGS_H

#include "SISInterface.h"
#include "SISSettingsBuilder.h"

class SISSettings {

    public:
        SISSettings( SISInterface* sisInterface );
        SISSettings( std::string   fileIn );
        ~SISSettings( );

        int readSetting(  std::string setting, int channel, uint32_t& value );
        int writeSetting( std::string setting, int channel, uint32_t  value );

        int saveRegisters( std::string dirOut );
        int loadRegisters( std::string dirIn  );
        
        int commitRegisters( );
        int readRegisters( );

    private:

        uint32_t getRegister( std::string reg_name );
        void getChannelRegister( uint32_t& reg_add, uint32_t channel, bool is_adc, bool is_cha );

        int read(  uint32_t address, uint32_t& value );
        int write( uint32_t address, uint32_t  value );

        int setRegisterSpecificBits( uint32_t reg_add, uint8_t bit_lower, uint8_t bit_upper, uint32_t  value );
        int getRegisterSpecificBits( uint32_t reg_add, uint8_t bit_lower, uint8_t bit_upper, uint32_t& value );

        Json::Value                     fRegisters;
        std::map<std::string, Setting*>  fSettings;

        SISInterface*        fInterface;
        SISSettingsBuilder*  fBuilder;

};

#endif // SISSETTINGS_H_INCLUDED