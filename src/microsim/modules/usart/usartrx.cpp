/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "usartrx.h"
#include "mcuinterrupts.h"
#include "iopin.h"
#include "simulator.h"
#include "connector.h"

UartRx::UartRx( UsartModule* usart, eMcu* mcu, QString name )
      : UartTR( usart, mcu, name )
{
    m_period = 0;
    m_ignoreData = false;
}
UartRx::~UartRx( ){}

void UartRx::enable( uint8_t en )
{
    bool enabled = en > 0;
    if( enabled == m_enabled ) return;
    m_enabled = enabled;

    m_runHardware = m_ioPin->isConnected();
    while( !m_inBuffer.empty() ) m_inBuffer.pop();

    m_state = usartSTOPPED;

    if( enabled ) processData( 0 ); // Start reading
    m_frame = 0;
}

void UartRx::voltChanged()
{
    if( m_sleeping ) return;

    bool bit = m_ioPin->getInpState();

    if( m_state == usartRXEND ) rxEnd();

    if     ( !m_startHigh &&  bit ) m_startHigh = true;
    else if(  m_startHigh && !bit )                     // Start bit detected
    {
        m_state = usartRECEIVE;
        m_ioPin->changeCallBack( this, false );
        if( m_period ) Simulator::self()->addEvent( m_period/2, this ); // Shedule reception
    }
}

void UartRx::runEvent()
{
    if( m_state == usartSTOPPED ) return;

    if( m_state == usartRECEIVE )
    {
        if( m_runHardware )
        {
            readBit();
            if( m_state == usartRXEND ) rxEnd(); //Simulator::self()->addEvent( m_period/2, this ); // End of Byte
            else if( m_period )         Simulator::self()->addEvent( m_period, this );   // Shedule next sample
        }else{
            if( !m_inBuffer.empty() )
            {
                byteReceived( m_inBuffer.front() );
                m_inBuffer.pop();
            }
            if( m_period ) Simulator::self()->addEvent( m_period*(m_framesize ), this );
    }   }
    else if( m_state == usartRXEND ) rxEnd();
}

void UartRx::processData( uint8_t )
{
    m_framesize = 1+mDATABITS+mPARITY+mSTOPBITS;
    m_currentBit = 0;
    m_fifoP = 2;
    m_startHigh = false;

     if( m_runHardware )
     {
         m_startHigh = m_ioPin->getInpState();
         m_ioPin->changeCallBack( this, true );             // Wait for start bit
     }else{
         m_state = usartRECEIVE;
         if( m_period ) Simulator::self()->addEvent( m_period*m_framesize, this ); // Shedule Byte received
     }
}

void UartRx::readBit()
{
    bool bit = m_ioPin->getInpState();

    if( bit ) m_frame += 1<<m_currentBit;      // Get bit into frame
    if( ++m_currentBit == m_framesize )
    {
        m_ioPin->changeCallBack( this, true ); // Wait for next start bit
        m_state = usartRXEND;                  // Data reception finished
    }
}

void UartRx::rxEnd()
{
    m_frame >>= 1;  // Start bit
    byteReceived( m_frame );

    m_currentBit = 0;
    m_frame = 0;

    if( m_runHardware )
    {
        m_state = usartSTOPPED;
        m_ioPin->changeCallBack( this, true ); // Wait for next start bit
    }
    else m_state = usartRECEIVE;

    if( m_period ) Simulator::self()->cancelEvents( this );
}

void UartRx::byteReceived( uint16_t frame )
{
    if( m_fifoP == 0 )             // Overrun error
    {
        m_usart->overrunError();
        return;
    }
    if( mPARITY > parNONE )        // Check Parity bit
    {
        bool parity = getParity( frame );
        bool parityBit = frame & 1<<mDATABITS;
        if( parity != parityBit ) frame |= parityError;
    }
    if( (frame & 1<<(mDATABITS+mPARITY)) == 0 ) frame |= frameError; // Frame Error: wrong stop bit

    if( mDATABITS == 9 && m_ignoreData && (frame & 1<<8) == 0 ) return; // Multi-proccesor data frame

    m_fifoP--;
    m_fifo[m_fifoP] = frame;
    if( m_fifoP == 1 ) m_interrupt->raise();
    m_usart->byteReceived( frame & mDATAMASK );
}

uint8_t UartRx::getData()
{
    if( m_fifoP == 2 ) return 0; // No data available

    uint16_t frame = m_fifo[1];
    uint8_t  data = frame & mDATAMASK;

    if( mDATABITS == 9 )      m_usart->setBit9Rx( frame & (1<<8) );
    if( frame & parityError ) m_usart->parityError();
    if( frame & frameError )  m_usart->frameError();

    if( ++m_fifoP == 2 && m_interrupt ) m_interrupt->clearFlag(); // Fifo empty
    else                                m_fifo[1] = m_fifo[0];    // Advance fifo

    return data;
}

void UartRx::queueData( uint8_t data )
{
    if( !m_enabled ) return;
    if( m_runHardware )
    {
        m_runHardware = false;
        Simulator::self()->cancelEvents( this );
        Simulator::self()->addEvent( m_period*(m_framesize+2), this );
        m_state = usartRECEIVE;
    }
    if( m_inBuffer.size() > 1000 ) return;

    m_inBuffer.push( data );
}
