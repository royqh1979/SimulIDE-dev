/***************************************************************************
 *   Copyright (C) 2020 by santiago González                               *
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

#include "oscopechannel.h"
#include "plotbase.h"
#include "simulator.h"
#include "utils.h"

OscopeChannel::OscopeChannel( QString id )
             : DataChannel( id )
{
    m_filter = 0.1;
    m_vTick = 1;
    m_points = &m_pointsA;
}
OscopeChannel::~OscopeChannel()  { }

void OscopeChannel::initialize()
{
    m_rising   = false;
    m_falling  = false;
    m_chCondFlag = false;

    m_period = 0;
    m_risEdge = 0;
    m_nCycles = 0;
    m_totalP = 0;
    m_numMax = 0;
    m_lastMax = 0;
    m_ampli = 0;
    m_maxVal = -1e12;
    m_minVal = 1e12;
    m_dispMax = 5;
    m_dispMin =-5;

    m_lastValue = 0;
    m_bufferCounter = 0;

    m_vTick = 1;
    m_freq = 0;

    m_buffer.fill(0);
    m_time.fill(0);
    m_pointsA.clear();
    m_pointsB.clear();

    m_dataPlotW->m_data1Label[m_channel]->setText( "---" );
    m_dataPlotW->m_data2Label[m_channel]->setText( "---" );
    m_dataPlotW->m_display->update();
}

void OscopeChannel::updateValues()
{
    double val = m_freq*1e12;
    int decs;
    QString unit = " ";
    if( val >= 1 ) valToUnit( val, unit, decs );
    m_dataPlotW->m_data1Label[m_channel]->setText( " "+QString::number( val, 'f', decs )+unit+"Hz" );

    unit = " ";
    val = m_ampli*1e12;
    if( val >= 1 ) valToUnit( val, unit, decs );
    m_dataPlotW->m_data2Label[m_channel]->setText( "Amp "+QString::number( val,'f', decs )+unit+"V" );
}

void OscopeChannel::updateStep()
{
    uint64_t simTime = Simulator::self()->circTime();

    if( m_period > 10 )  // We have a wave
    {
        if( m_numMax > 1 )  // Got enought maximums to calculate Freq
        {
            m_freq = (m_freq+1e12/((double)m_totalP/(double)(m_numMax-1)))/2;
            m_totalP  = 0;
            m_numMax  = 0;
        }
    }
    else
    {
        m_freq = 0;
        m_maxVal  =-1e12;
        m_minVal  = 1e12;
    }

    if( m_dataPlotW->m_auto == m_channel )
    {
        m_dataPlotW->setHPos( m_channel, 0 );
        if( m_period > 10 )
        {
            m_vTick = m_ampli/10;
            m_dataPlotW->setHTick( (double)m_period/5 );
            m_dataPlotW->setVTick( m_channel, m_vTick );
            m_dataPlotW->setVPos( m_channel, -m_ampli/2 );
            m_dataPlotW->m_display->setLimits( m_channel, m_dispMax, m_dispMin );
        }
    } else
    {
        m_dispMax =  m_dataPlotW->m_vTick[m_channel]*10;
        m_dispMin = 0;
        m_dataPlotW->m_display->setLimits( m_channel, m_dispMax, m_dispMin );
    }
    updateValues();

    if( m_period > 10 )  // Do we still have a wave?
    {
        uint64_t stepsPF  = Simulator::self()->stepsPerFrame();
        uint64_t stepSize = Simulator::self()->stepSize();
        uint64_t lost = m_period*2;
        if( lost < stepsPF*2 ) lost = stepsPF*stepSize*2;

        if( simTime-m_lastMax > lost ) // Wave lost
        {
            m_freq    = 0;
            m_period  = 0;
            m_risEdge = 0;
            m_nCycles = 0;
            m_totalP  = 0;
            m_numMax  = 0;
            m_lastMax = 0;
            m_ampli   = 0;
        }
    }
}

void OscopeChannel::voltChanged()
{
    uint64_t simTime = Simulator::self()->circTime();

    double d0 = m_ePin[0]->getVolt();
    double d1 = m_ePin[1]->getVolt();
    double data = d0+d1;

    if( data > m_maxVal ) m_maxVal = data;
    if( data < m_minVal ) m_minVal = data;

    if( ++m_bufferCounter >= m_buffer.size() ) m_bufferCounter = 0;
    m_buffer[m_bufferCounter] = data;
    uint64_t time = simTime;
    m_time[m_bufferCounter] = time;

    double delta = data-m_lastValue;

    if( delta > 0 )               // Rising
    {
        if( delta > m_filter )
        {
            if( m_falling && !m_rising )     // Min To Rising
            {
                if( m_numMax > 0 ) m_totalP += simTime-m_lastMax;
                m_lastMax = simTime;

                m_numMax++;
                m_nCycles++;
                m_falling = false;

                if( m_dataPlotW->m_paOnCond )
                {
                    if( (m_chCond == Rising) || (m_chCond == High) ) // Pause on Rising or High
                    {
                        m_chCondFlag = true;
                        m_dataPlotW->m_plotB->pauseOnCond();
                        if( m_chCond == Rising ) m_chCondFlag = false;
                    }
                    else if( m_chCond == Low ) m_chCondFlag = false;
                }
            }
            else if( m_dataPlotW->m_paOnCond )
            {
                if( m_chCond == Rising ) m_chCondFlag = false;
            }
            m_rising = true;
            m_lastValue = data;
        }
        if( m_nCycles > 1 )     // Wait for a full wave
        {
            m_ampli = m_maxVal-m_minVal;
            double mid = m_minVal + m_ampli/2;

            if( data >= mid )            // Rising edge
            {
                if( m_numMax > 1 )
                {
                    m_dispMax = m_maxVal;
                    m_dispMin = m_minVal;
                    m_maxVal  =-1e12;
                    m_minVal  = 1e12;
                }
                m_nCycles--;

                if( m_risEdge > 0 ) m_period = simTime-m_risEdge; // period = this_edge_time - last_edge_time
                m_risEdge = simTime;
            }
        }
    }
    else if( delta < -m_filter )         // Falling
    {
        if( m_rising && !m_falling )    // Max Found
        {
            m_rising = false;

            if( m_dataPlotW->m_paOnCond )
            {
                if( (m_chCond == Falling) || (m_chCond == Low) ) // Pause on Falling or Low
                {
                    m_chCondFlag = true;
                    m_dataPlotW->m_plotB->pauseOnCond();
                    if( m_chCond == Falling ) m_chCondFlag = false;
                }
                else if( m_chCond == High ) m_chCondFlag = false;
            }
        }
        else if( m_dataPlotW->m_paOnCond )
        {
            if( m_chCond == Falling ) m_chCondFlag = false;
        }
        m_falling = true;
        m_lastValue = data;
    }
}

void OscopeChannel::setFilter( double f )
{
    m_risEdge = 0;
    m_nCycles = 0;
    m_totalP  = 0;
    m_numMax  = 0;

    m_filter = f;
    m_dataPlotW->m_display->setFilter(f);
}
