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

#include "mcuinterface.h"
#include "utils.h"

McuInterface::McuInterface( QString id )
            : eElement( id )
{
    m_ramTable = new RamTable( NULL, this );
    m_ramSize   = 0;
    m_flashSize = 0;
    m_romSize   = 0;
    m_wordSize  = 2;

    m_firmware = "";

    m_ramTable->hide();
}

McuInterface::~McuInterface()
{
}

QVector<int>* McuInterface::eeprom()
{
    for( uint32_t i=0; i<m_romSize; ++i ) m_eeprom[i] = getRomValue( i );
    return &m_eeprom;
}

void McuInterface::setEeprom( QVector<int>* eep )
{
    int size = m_romSize;
    if( eep->size() < size ) size = eep->size();

    for( int i=0; i<size; ++i ) setRomValue( i, eep->at(i) );
}

void McuInterface::updateRamValue( QString name )
{
    //if( !m_loadStatus ) return;

    name = name.toUpper();
    QString type = "u8";
    if( m_typeTable.contains( name )) type = m_typeTable[ name ];
    //else return;

    QByteArray ba;
    ba.resize(4);
    int address = getRegAddress( name );
    if( address < 0 ) return;

    int bits = 8;

    if( type.contains( "32" ) )    // 4 bytes
    {
        bits = 32;
        ba[0] = getRamValue( address );
        ba[1] = getRamValue( address+1 );
        ba[2] = getRamValue( address+2 );
        ba[3] = getRamValue( address+3 );
    }
    else if( type.contains( "16" ) )  // 2 bytes
    {
        bits = 16;
        ba[0] = getRamValue( address );
        ba[1] = getRamValue( address+1 );
        ba[2] = 0;
        ba[3] = 0;
    }
    else                                  // 1 byte
    {
        ba[0] = getRamValue( address );
        ba[1] = 0;
        ba[2] = 0;
        ba[3] = 0;
    }
    if( type.contains( "f" ) )                          // float, double
    {
        float value = 0;
        memcpy(&value, ba, 4);
        m_ramTable->setItemValue( 1, value  );
    }
    else                                              // char, int, long
    {
        int32_t value = 0;

        if( type.contains( "u" ) )
        {
            uint32_t val = 0;
            memcpy(&val, ba, 4);
            value = val;
        }
        else
        {
            if( bits == 32 )
            {
                int32_t val = 0;
                memcpy(&val, ba, 4);
                value = val;
            }
            else if( bits == 16 )
            {
                int16_t val = 0;
                memcpy(&val, ba, 2);
                value = val;
            }
            else
            {
                int8_t val = 0;
                memcpy(&val, ba, 1);
                value = val;
            }
        }
        m_ramTable->setItemValue( 2, value  );

        if     ( type.contains( "8" ) ) m_ramTable->setItemValue( 3, decToBase(value, 2, 8)  );
        else if( type.contains( "string" ) )
        {
            QString strVal = "";
            for( int i=address; i<=address+value; i++ )
            {
                QString str = "";
                const QChar cha = getRamValue( i );
                str.setRawData( &cha, 1 );

                strVal += str; //QByteArray::fromHex( getRamValue( i ) );
            }
            //qDebug() << "string" << name << value << strVal;
            m_ramTable->setItemValue( 3, strVal  );
        }
    }
    m_ramTable->setItemValue( 1, type  );
}