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

#ifndef MCUMODULE_H
#define MCUMODULE_H

#include <QString>

#include "mcutypes.h"
#include "regsignal.h"

class eMcu;
class Interrupt;

class MAINMODULE_EXPORT McuModule
{
        friend class McuCreator;

    public:
        McuModule( eMcu* mcu, QString name );
        ~McuModule();

        virtual void configureA( uint8_t ){;}
        virtual void configureB( uint8_t ){;}

        Interrupt* getInterrupt() { return m_interrupt; }

        //RegSignal<uint8_t> interrupt;

    protected:
        QString m_name;
        eMcu*   m_mcu;

        Interrupt*  m_interrupt;

        regBits_t m_configBitsA;
        regBits_t m_configBitsB;
};
#endif