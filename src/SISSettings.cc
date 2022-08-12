#include <fstream>

#include "SISSettings.h"

SISSettings::SISSettings( SISInterface* sisInterface ) : fInterface( sisInterface )
{    
    this->fBuilder = new SISSettingsBuilder();
    this->fBuilder->Build( );

    this->fRegisters = this->fBuilder->GetRegisters( );
    this->fSettings  = this->fBuilder->GetSettings( );
}

SISSettings::~SISSettings( )
{
    delete this->fBuilder;
}

int SISSettings::write( uint32_t address, uint32_t value )
{
    int error = fInterface->register_write( address, value );
    if( error != 0 ) return -1;
    else return 0;
}

int SISSettings::read( uint32_t address, uint32_t& value )
{
    uint32_t* reg_value = nullptr;
    int error = fInterface->register_read( address, reg_value );
    value = *reg_value;
    if( error != 0 ) return -1;
    else return 0;
}

int SISSettings::setRegisterSpecificBits( uint32_t reg_add, uint8_t bit_lower, uint8_t bit_upper, uint32_t value )
{    
    int error;
    uint8_t bit_number = bit_upper - bit_lower + 1;
    if( (value < ((uint32_t)1<<bit_number)) && bit_upper<32 && bit_lower<=bit_upper ){
        uint32_t* reg_data = nullptr;
        uint32_t  reg_mask = 0;
    
        for( int bit = 0; bit < 32; bit++ ) { if( bit<bit_lower || bit>bit_upper ) { reg_mask+=(1<<bit); } }
    
        error = fInterface->register_read( reg_add, reg_data );
        if( error == 0 ) {
            *reg_data = (*reg_data & reg_mask) + (value << bit_lower);
            error = fInterface->register_write( reg_add, *reg_data );
            if( error != 0 ) return -1;
        }
        else return -1;

        return 0;
    }

    return -1;
}

int SISSettings::getRegisterSpecificBits( uint32_t reg_add, uint8_t bit_lower, uint8_t bit_upper, uint32_t& value ) 
{
    int error;
    uint32_t* reg_data = nullptr;
    uint32_t reg_mask  = 0;
    if( bit_upper<32 && bit_lower<=bit_upper ){
        for( int bit = 0; bit < 32; bit++ ) { if (bit>=bit_lower && bit<=bit_upper) { reg_mask+=(1<<bit); } }
        error = fInterface->register_read( reg_add, reg_data );
        if( error != 0 ) return -1;
        else{
            value = ((*reg_data & reg_mask) >> bit_lower);
            return 0;
        }
    }
    return -1;
}

int SISSettings::commitRegisters( )
{
    uint32_t value, address;
    std::ostringstream regVal;

    Json::Value::Members registers_members = fRegisters.getMemberNames( );
    for( const std::string &member : registers_members ){
        sscanf( fRegisters[member]["address"].asString().c_str(), "%x", &address );
        sscanf( fRegisters[member]["value"].asString().c_str(), "%x", &value );
        if( write( address, value ) != 0 )
            return -1;
    }
    
    return 0;
}

int SISSettings::readRegisters( )
{
    uint32_t value, address;
    std::ostringstream regVal;

    Json::Value::Members registers_members = fRegisters.getMemberNames( );
    for( const std::string &member : registers_members ){
        sscanf( fRegisters[member]["address"].asString().c_str(), "%x", &address );        
        if( read( address, value ) != 0 )
            return -1;
        regVal << std::hex << value;
        fRegisters[member]["value"] = regVal.str( );
        regVal.str( "" );
    }
    
    return 0;
}

uint32_t SISSettings::getRegister( std::string reg_name )
{
    std::string name;
    uint32_t address;

    Json::Value::Members registers_members = fRegisters.getMemberNames( );
    for( const std::string &member : registers_members ){
        name = fRegisters[member]["name"].asString();
        if( name == reg_name ){
            sscanf( fRegisters[member]["address"].asString().c_str(), "%x", &address );
            return address;
        }
    }

    return -1;
}

void SISSettings::getChannelRegister( uint32_t& reg_add, uint32_t channel, bool is_adc, bool is_cha )
{
    uint32_t adc = (channel-1)/4;
    if( is_adc && is_cha ){
        reg_add += 0x1000*adc + 0x10*(channel-1);
    }
    else if( is_adc && !is_cha ){
        reg_add += 0x1000*adc;
    }
    else if( !is_adc && !is_cha ){
        reg_add = reg_add;
    }
}

int SISSettings::writeSetting( std::string setting, int channel, uint32_t value )
{
    std::string reg_name;
    uint32_t reg_add;
    uint8_t bit_lower, bit_upper;
    if( fSettings.find( setting ) != fSettings.end() ){
        bit_lower = fSettings[setting]->bit_lower;
        bit_upper = fSettings[setting]->bit_upper;
        reg_name  = fSettings[setting]->reg_name;

        reg_add = getRegister( reg_name );
        if( reg_add == -1 ) return -1;

        getChannelRegister( reg_add, channel, fSettings[setting]->is_adc, fSettings[setting]->is_cha ); 

        if( setRegisterSpecificBits( reg_add, bit_lower, bit_upper, value ) == 0 )
            return 0;
        else return -1;  
    }
    else{
        for( auto it : fSettings ){
            if( it.first == setting + " CH" + std::to_string( channel ) ){            
                bit_lower = it.second->bit_lower;
                bit_upper = it.second->bit_upper;
                reg_name  = it.second->reg_name;
                
                reg_add = getRegister( reg_name );
                if( reg_add == -1 ) return -1;

                getChannelRegister( reg_add, channel, it.second->is_adc, it.second->is_cha ); 

                if( setRegisterSpecificBits( reg_add, bit_lower, bit_upper, value ) == 0 )
                    return 0;
                else return -1;
            }
        }
        for( auto it : fSettings ){
            if( it.first == setting + " CH" + std::to_string( ((channel-1)%4)+1 ) ){            
                bit_lower = it.second->bit_lower;
                bit_upper = it.second->bit_upper;
                reg_name  = it.second->reg_name;
                
                reg_add = getRegister( reg_name );
                if( reg_add == -1 ) return -1;

                getChannelRegister( reg_add, channel, it.second->is_adc, it.second->is_cha ); 

                if( setRegisterSpecificBits( reg_add, bit_lower, bit_upper, value ) == 0 )
                    return 0;
                else return -1;
            }
        }
    }
    return -1;
}

int SISSettings::readSetting( std::string setting, int channel, uint32_t& value )
{
    std::string reg_name;
    uint32_t reg_add;
    uint8_t bit_lower, bit_upper;
    if( fSettings.find( setting ) != fSettings.end() ){
        bit_lower = fSettings[setting]->bit_lower;
        bit_upper = fSettings[setting]->bit_upper;
        reg_name  = fSettings[setting]->reg_name;

        reg_add = getRegister( reg_name );
        if( reg_add == -1 ) return -1;

        getChannelRegister( reg_add, channel, fSettings[setting]->is_adc, fSettings[setting]->is_cha ); 

        if( getRegisterSpecificBits( reg_add, bit_lower, bit_upper, value ) == 0 )
            return 0;
        else return -1;
    }
    else{
        for( auto it : fSettings ){
            if( it.first == setting + " CH" + std::to_string( channel ) ){            
                bit_lower = it.second->bit_lower;
                bit_upper = it.second->bit_upper;
                reg_name  = it.second->reg_name;
                
                reg_add = getRegister( reg_name );
                if( reg_add == -1 ) return -1;

                getChannelRegister( reg_add, channel, it.second->is_adc, it.second->is_cha ); 

                if( getRegisterSpecificBits( reg_add, bit_lower, bit_upper, value ) == 0 )
                    return 0;
                else return -1;
            }
        }
        for( auto it : fSettings ){
            if( it.first == setting + " CH" + std::to_string( ((channel-1)%4)+1 ) ){            
                bit_lower = it.second->bit_lower;
                bit_upper = it.second->bit_upper;
                reg_name  = it.second->reg_name;
                
                reg_add = getRegister( reg_name );
                if( reg_add == -1 ) return -1;

                getChannelRegister( reg_add, channel, it.second->is_adc, it.second->is_cha ); 

                if( getRegisterSpecificBits( reg_add, bit_lower, bit_upper, value ) == 0 )
                    return 0;
                else return -1;
            }
        }
    }
    return -1;
}

int SISSettings::saveRegisters( std::string dirOut )
{
    Json::StreamWriterBuilder builder;
    builder["commentStyle"]         = "None";
    builder["indentation"]          = "  "  ;
    builder["dropNullPlaceholders"] = true  ;

    std::ofstream file; 
    file.open(dirOut.c_str());
    if( !file.is_open() ) return -1;
    file << fRegisters;
    file.close();
    return 0;
}

int SISSettings::loadRegisters( std::string dirIn )
{
    std::ifstream file;
    file.open(dirIn.c_str());
    if( !file.is_open() ) return -1;
    file >> fRegisters;
    file.close();
    return 0;
}