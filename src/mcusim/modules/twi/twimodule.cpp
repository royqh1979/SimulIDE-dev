/***************************************************************************
 *   Copyright (C) 2021 by santiago González                               *
 *   santigoro@gmail.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 *                                                                         *
 ***************************************************************************/

#include "twimodule.h"
#include "iopin.h"
#include "simulator.h"

TwiModule::TwiModule( QString name )
         : eClockedDevice( name )
{
    m_sda = NULL;
    m_scl = NULL;
    m_addrBits = 7;
    m_address = 0;
}
TwiModule::~TwiModule( ){}

void TwiModule::initialize()
{
    m_mode      = TWI_OFF;
    m_twiState  = TWI_NO_STATE;
    m_i2cState  = I2C_IDLE;
    m_lastState = I2C_IDLE;

    m_sheduleSDA = false;
    m_toggleScl  = false;
    m_genCall    = false;

    m_lastSDA = true; // SDA High = inactive
}

void TwiModule::stamp()      // Called at Simulation Start
{ /* We are just avoiding eClockedDevice::stamp() call*/ }

void TwiModule::keepClocking()
{
    m_toggleScl = true;
    Simulator::self()->addEvent( m_clockPeriod/2, this );
}

void TwiModule::runEvent()
{
    if( m_sheduleSDA) // Used by Slave to set SDA state at 1/2 Clock
    {
        setSDA( m_nextSDA );
        m_sheduleSDA = false;
        return;
    }
    if( m_sheduleSCL) // Used by Slave to set SDA state at 1/2 Clock
    {
        setSCL( m_nextSCL );
        m_sheduleSCL = false;
        return;
    }
    if( m_mode != TWI_MASTER ) return;

    updateClock();
    bool clkLow = ((m_clkState == Clock_Low) || (m_clkState == Clock_Falling));

    if( m_toggleScl )
    {
        setSCL( clkLow );     // High if is LOW, LOW if is HIGH
        m_toggleScl = false;
        return;
    }
    Simulator::self()->addEvent( m_clockPeriod, this );
    if( m_i2cState == I2C_IDLE ) return;

    getSdaState();               // Update state of SDA pin

    twiState_t twiState;
    switch( m_i2cState )
    {
        case I2C_IDLE: return;

        case I2C_STOP:           // Send Stop Condition
        {
            if     (  m_sdaState && clkLow )  setSDA( false ); // Step 1: Lower SDA
            else if( !m_sdaState && clkLow )  keepClocking();  // Step 2: Raise Clock
            else if( !m_sdaState && !clkLow ) setSDA( true );  // Step 3: Raise SDA
            else if(  m_sdaState && !clkLow )                  // Step 4: Operation Finished
            {
                setTwiState( TWI_NO_STATE ); // Set State first so old m_i2cState is still avilable
                m_i2cState = I2C_IDLE;
            }
        } break;

        case I2C_START :         // Send Start Condition
        {
            if( m_sdaState ) setSDA( false ); // Step 1: SDA is High, Lower it
            else if( !clkLow )                // Step 2: SDA Already Low, Lower Clock
            {
                setSCL( false ); //keepClocking();
                setTwiState( TWI_START );
                m_i2cState = I2C_IDLE;
            }
        }break;

        case I2C_READ:            // We are Reading data
        {
            if( !clkLow )         // Read bit while clk is high
            {
                readBit();
                if( m_bitPtr == 8 ) readByte();
            }
            keepClocking();
        }break;

        case I2C_WRITE :          // We are Writting data
        {
            if( clkLow )
            {
                writeBit();    // Set SDA while clk is Low
            }
            keepClocking();
        }break;

        case I2C_ACK:             // Send ACK
        {
            if( clkLow )
            {
                if( m_sendACK ) setSDA( false);
                m_i2cState = I2C_ENDACK;
            }
            keepClocking();
        }break;

        case I2C_ENDACK:         // We sent ACK, release SDA
        {
            if( clkLow )
            {
                setSDA( true ); //if( m_lastState == I2C_READ )

                twiState = m_sendACK ? TWI_MRX_DATA_ACK : TWI_MRX_DATA_NACK ;
                setTwiState( twiState );
                m_i2cState = I2C_IDLE;
            }
            else keepClocking();
        }break;

        case I2C_READACK:         // Read ACK
        {
            if( clkLow )
            {
                setTwiState( m_nextState );
                m_i2cState = I2C_IDLE;
            }
            else
            {
                if( m_isAddr ) // ACK after sendind Slave address
                {
                    if( m_write ) m_nextState = m_sdaState ? TWI_MTX_ADR_NACK : TWI_MTX_ADR_ACK; // Transmition started
                    else          m_nextState = m_sdaState ? TWI_MRX_ADR_NACK : TWI_MRX_ADR_ACK; // Reception started
                }
                else           // ACK after sendind data
                    m_nextState = m_sdaState ? TWI_MTX_DATA_NACK : TWI_MTX_DATA_ACK;
                keepClocking();
            }
        }break;
    }
}

void TwiModule::voltChanged() // Used by slave
{
    if( m_mode != TWI_SLAVE ) return;

    updateClock();
    getSdaState();                             // State of SDA pin

    if(( m_clkState == Clock_High )&&( m_i2cState != I2C_ACK ))
    {
        if( m_lastSDA && !m_sdaState ) {       // We are in a Start Condition
            m_bitPtr = 0;
            m_rxReg = 0;
            m_i2cState = I2C_START;
        }
        else if( !m_lastSDA && m_sdaState ) {  // We are in a Stop Condition
           I2Cstop();
        }
    }
    else if( m_clkState == Clock_Rising )      // We are in a SCL Rissing edge
    {
        if( m_i2cState == I2C_START )          // Get Transaction Info
        {
            readBit();
            if( m_bitPtr > m_addrBits )
            {
                bool rw = m_rxReg % 2;         //Last bit is R/W
                m_rxReg >>= 1;

                m_addrMatch = m_rxReg == m_address;
                bool genCall = m_genCall && (m_rxReg == 0);

                if( m_addrMatch || genCall )   // Address match or General Call
                {
                    m_sendACK = true;
                    if( rw )                   // Master is Reading
                    {
                        m_nextState = TWI_STX_ADR_ACK;
                        m_i2cState = I2C_READ;
                        writeByte();
                    }
                    else                       // Master is Writting
                    {
                        m_nextState = m_addrMatch ? TWI_SRX_ADR_ACK : TWI_SRX_GEN_ACK;
                        m_i2cState = I2C_WRITE;
                        m_bitPtr = 0;
                        startWrite();          // Notify posible child class
                    }
                    ACK();
                }
                else {
                    m_i2cState = I2C_STOP;
                    m_rxReg = 0;
                }
            }
        }else if( m_i2cState == I2C_WRITE ){
            readBit();
            if( m_bitPtr == 8 )
            {
                if( m_addrMatch )
                     m_nextState = m_sendACK ? TWI_SRX_ADR_DATA_ACK : TWI_SRX_ADR_DATA_NACK;
                else m_nextState = m_sendACK ? TWI_SRX_GEN_DATA_ACK : TWI_SRX_GEN_DATA_NACK;
                readByte();
            }
        }
        else if( m_i2cState == I2C_READACK )      // We wait for Master ACK
        {
            setTwiState( m_sdaState ? TWI_STX_DATA_NACK : TWI_STX_DATA_ACK );
            if( !m_sdaState ) {                // ACK: Continue Sending
                m_i2cState = m_lastState;
                writeByte();
            }
            else m_i2cState = I2C_IDLE;
        }
    }
    else if( m_clkState == Clock_Falling )
    {
        if( m_i2cState == I2C_ACK ) {             // Send ACK
            sheduleSDA( !m_sendACK );
            m_i2cState = I2C_ENDACK;
        }
        else if( m_i2cState == I2C_ENDACK )      // We sent ACK, release SDA
        {
            setTwiState( m_nextState );
            m_i2cState = m_lastState;

            bool releaseSda = true;
            if( m_i2cState == I2C_READ ) releaseSda = m_txReg>>m_bitPtr & 1; // Keep Sending
            sheduleSDA( releaseSda );
            m_rxReg = 0;
        }
        if( m_i2cState == I2C_READ )
        {
           writeBit();
        }
    }
    m_lastSDA = m_sdaState;
}

void TwiModule::setMode( twiMode_t mode )
{
    if( mode == TWI_MASTER )
    {
        Simulator::self()->cancelEvents( this );
        Simulator::self()->addEvent( m_clockPeriod, this ); // Start Clock
    }

    m_scl->changeCallBack( this, mode == TWI_SLAVE );
    m_sda->changeCallBack( this, mode == TWI_SLAVE );

    sheduleSCL( true ); // Avoid false stop condition
    setSDA( true );

    m_mode = mode;
    m_i2cState = I2C_IDLE;
    m_sheduleSDA = false;
    m_toggleScl  = false;
}

void TwiModule::setSCL( bool st ) { m_scl->setOutState( st ); }
void TwiModule::setSDA( bool st ) { m_sda->setOutState( st ); }
void TwiModule::getSdaState() { m_sdaState = m_sda->getInpState(); }

void TwiModule::sheduleSDA( bool state )
{
    m_sheduleSDA = true;
    m_nextSDA = state;
    Simulator::self()->addEvent( m_clockPeriod/4, this );
}

void TwiModule::sheduleSCL( bool state )
{
    m_sheduleSCL = true;
    m_nextSCL = state;
    Simulator::self()->addEvent( m_clockPeriod/4, this );
}

void TwiModule::readBit()
{
    if( m_bitPtr > 0 ) m_rxReg <<= 1;
    m_rxReg += m_sdaState;            //Read one bit from sda
    m_bitPtr++;
}

void TwiModule::writeBit()
{
    if( m_bitPtr < 0 ) { waitACK(); return; }

    bool bit = m_txReg>>m_bitPtr & 1;
    m_bitPtr--;

    if( m_mode == TWI_MASTER ) setSDA( bit );
    else                       sheduleSDA( bit );
}

void TwiModule::readByte()
{
    m_bitPtr = 0;
    ACK();
}

void TwiModule::waitACK()
{
    setSDA( true );
    m_lastState = m_i2cState;
    m_i2cState = I2C_READACK;
}

void TwiModule::ACK()
{
    m_lastState = m_i2cState;
    m_i2cState = I2C_ACK;
}

void TwiModule::masterWrite( uint8_t data , bool isAddr, bool write )
{
    m_isAddr = isAddr;
    m_write  = write;

    m_i2cState = I2C_WRITE;
    m_txReg = data;
    writeByte();
}

void TwiModule::masterRead( bool ack )
{
    m_sendACK = ack;

    setSDA( true );
    m_bitPtr = 0;
    m_rxReg = 0;
    m_i2cState = I2C_READ;
}

void TwiModule::setFreqKHz( double f )
{
    m_freq = f*1e3;
    double stepsPerS = 1e12;
    m_clockPeriod = stepsPerS/m_freq/2;
}
void TwiModule::setSdaPin( IoPin* pin ) { m_sda = pin; }
void TwiModule::setSclPin( IoPin* pin )
{
    m_scl = pin;
    m_clkPin = pin;
}