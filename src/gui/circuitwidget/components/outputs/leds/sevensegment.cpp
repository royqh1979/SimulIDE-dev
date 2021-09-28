/***************************************************************************
 *   Copyright (C) 2012 by santiago González                               *
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

#include "sevensegment.h"
#include "circuitwidget.h"
#include "itemlibrary.h"
#include "connector.h"
#include "simulator.h"
#include "circuit.h"
#include "pin.h"

#include "stringprop.h"
#include "doubleprop.h"
#include "boolprop.h"
#include "intprop.h"

Component* SevenSegment::construct( QObject* parent, QString type, QString id )
{ return new SevenSegment( parent, type, id ); }

LibraryItem* SevenSegment::libraryItem()
{
    return new LibraryItem(
        tr( "7 Segment" ),
        tr( "Leds" ),
        "seven_segment.png",
        "Seven Segment",
        SevenSegment::construct );
}

SevenSegment::SevenSegment( QObject* parent, QString type, QString id )
            : Component( parent, type, id )
            , eElement( id )
{
    m_graphical = true;

    setLabelPos( 20,-44, 0 );

    m_color = QColor(0,0,0);
    m_ledColor = "Yellow";
    m_commonCathode = true;
    m_verticalPins  = false;
    m_numDisplays = 0;
    m_threshold  = 2.4;
    m_maxCurrent = 0.02;
    m_resistance = 1;

    m_ePin.resize(8);
    m_pin.resize(8);

    QString pinid;
    for( int i=0; i<7; ++i ) // Create Pins & eNodes for 7 segments
    {
        pinid = m_id+"-pin_"+QString( 97+i ); // a..g
        m_ePin[i] = m_pin[i] = new Pin( 180, QPoint(-16-8,-24+i*8 ), pinid, 0, this );
    }
    // Pin dot
    m_ePin[7] = m_pin[7] = new Pin( 270, QPoint( -8, 24+8 ), m_id+"-pin_dot", 0, this );

    setNumDisplays( 1 );

    addPropGroup( { tr("Main"), {
new IntProp   <SevenSegment>( "NumDisplays"  , tr("Size")         ,"_7seg", this, &SevenSegment::numDisplays,  &SevenSegment::setNumDisplays ),
new StringProp<SevenSegment>( "Color"        , tr("Color")        ,""     , this, &SevenSegment::colorStr,     &SevenSegment::setColorStr, "enum" ),
new BoolProp  <SevenSegment>( "Vertical_Pins", tr("Vertical Pins"),""     , this, &SevenSegment::verticalPins, &SevenSegment::setVerticalPins )
    }} );
    addPropGroup( { tr("Electric"), {
new BoolProp<SevenSegment>( "CommonCathode", tr("Common Cathode"),"",  this, &SevenSegment::isComCathode, &SevenSegment::setComCathode),
new DoubProp<SevenSegment>( "Threshold" , tr("Forward Voltage")  ,"V", this, &SevenSegment::threshold,    &SevenSegment::setThreshold ),
new DoubProp<SevenSegment>( "MaxCurrent", tr("Max Current")      ,"A", this, &SevenSegment::maxCurrent,   &SevenSegment::setMaxCurrent ),
new DoubProp<SevenSegment>( "Resistance", tr("Resistance")       ,"Ω", this, &SevenSegment::resistance,   &SevenSegment::setResistance ),
    }} );
}
SevenSegment::~SevenSegment() { }

void SevenSegment::attach()
{
    for( int i=0; i<8; ++i ) m_enode[i] = m_ePin[i]->getEnode(); // Get eNode of pin i
    for( int i=0; i<m_numDisplays; ++i )
    {
        eNode* commonEnode = m_commonPin[i]->getEnode();     // Get eNode of common

        int pin;
        for( int j=0; j<8; ++j )
        {
            pin = i*8+j;
            if( m_commonCathode )
            {
                m_cathodePin[pin]->setEnode( commonEnode );
                m_anodePin[pin]->setEnode( m_enode[j] );
            }else{
                m_anodePin[pin]->setEnode( commonEnode );
                m_cathodePin[pin]->setEnode( m_enode[j] );
}   }   }   }

void SevenSegment::setColorStr( QString color )
{
    m_ledColor = color;
    for( LedSmd* segment : m_segment ) segment->setColorStr( color );
}

QStringList SevenSegment::getEnums( QString e )
{ return m_segment[0]->getEnums( e ); }

void SevenSegment::setNumDisplays( int displays )
{
    if( displays < 1 ) displays = 1;
    if( displays == m_numDisplays ) return;

    if( Simulator::self()->isRunning() )  CircuitWidget::self()->powerCircOff();

    m_area = QRect(-18,-24-4, 32*displays+4, 48+8 );

    if( displays > m_numDisplays )
    {
        resizeData( displays );
        for( int i=m_numDisplays; i<displays; ++i ) createDisplay( i );
    }else{
        for( int i=displays; i<m_numDisplays; ++i ) deleteDisplay( i );
        resizeData( displays );
    }
    m_numDisplays = displays;
    setResistance( m_resistance );
    setThreshold( m_threshold );
    setMaxCurrent( m_maxCurrent );

    Circuit::self()->update();
}

void SevenSegment::resizeData( int displays )
{
    m_commonPin.resize( displays );
    m_cathodePin.resize( displays*8 );
    m_anodePin.resize( displays*8 );
    m_segment.resize( displays*8 );
}

void SevenSegment::setComCathode( bool isCommonCathode )
{
    if( Simulator::self()->isRunning() )  CircuitWidget::self()->powerCircOff();
    m_commonCathode = isCommonCathode;
}

void SevenSegment::setVerticalPins( bool v )
{
    if( v == m_verticalPins ) return;
    m_verticalPins = v;
    
    if( v ) {
        for( int i=0; i<5; ++i ){
            m_pin[i]->setPos(-16+8*i,-24-8 );
            m_pin[i]->setRotation( 90 );
        }
        for( int i=5; i<8; ++i ){
            m_pin[i]->setPos(-16+8*(i-5), 24+8 );
            m_pin[i]->setRotation(-90 );
        }
    }else{
        for( int i=0; i<7; ++i ){
            m_pin[i]->setPos(-16-8,-24+i*8 );
            m_pin[i]->setRotation( 0 );
        }
        m_pin[7]->setPos(-8, 24+8 );
        m_pin[7]->setRotation(-90 );
    }
    m_area = QRect(-18,-24-4, 32*m_numDisplays+4, 48+8 );
    
    for( int i=0; i<8; ++i ) m_pin[i]->isMoved();
    Circuit::self()->update();
}
        
void SevenSegment::setResistance( double res )
{
    if( res < 1e-6 ) res = 1;
    m_resistance = res;
    for( uint i=0; i<m_segment.size(); ++i ) m_segment[i]->setRes( res );
}

void SevenSegment::setThreshold( double threshold )
{
    if( threshold < 1e-6 ) threshold = 2.4;
    m_threshold = threshold;
    for( uint i=0; i<m_segment.size(); ++i ) m_segment[i]->setThreshold( threshold );
}

void SevenSegment::setMaxCurrent( double current )
{
    if( current < 1e-6 ) current = 0.02;
    m_maxCurrent = current;
    for( uint i=0; i<m_segment.size(); ++i ) m_segment[i]->setMaxCurrent( current );
}

void SevenSegment::deleteDisplay( int dispNumber )
{
    Pin* pin = m_commonPin[dispNumber];
    pin->removeConnector();
    pin->reset();
    delete pin;

    for( int i=0; i<8; ++i ) Circuit::self()->removeComp( m_segment[dispNumber*8+i] );
}

void SevenSegment::createDisplay( int dispNumber )
{
    int x = 32*dispNumber;

    // Pin common
    QString pinid = m_id+"-pin_common"+QString( 97+dispNumber );
    m_commonPin[dispNumber] = new Pin( 270, QPoint( x+8, 24+8 ), pinid, 0, this );

    for( int i=0; i<8; ++i ) // Create segments
    {
        pinid = m_id+"-led_"+QString( 97+i );
        LedSmd* lsmd;
        if( i<7 ) lsmd = new LedSmd( this, "LEDSMD", pinid, QRectF(0, 0, 13.5, 1.5) ); // Segment
        else      lsmd = new LedSmd( this, "LEDSMD", pinid, QRectF(0, 0, 1.5, 1.5) );  // Point

        lsmd->setParentItem(this);
        lsmd->setFlag( QGraphicsItem::ItemIsSelectable, false );
        lsmd->setAcceptedMouseButtons( 0 );
        lsmd->setRes( m_resistance );
        lsmd->setMaxCurrent( m_maxCurrent );
        lsmd->setThreshold( m_threshold );
        lsmd->setColorStr( m_ledColor );

        m_anodePin  [dispNumber*8+i] = lsmd->getEpin(0);
        m_cathodePin[dispNumber*8+i] = lsmd->getEpin(1);
        m_segment   [dispNumber*8+i] = lsmd;
    }
    m_segment[dispNumber*8+0]->setPos( x-5, -20 );
    m_segment[dispNumber*8+1]->setPos( x+11.5, -16 );
    m_segment[dispNumber*8+1]->setRotation(96);
    m_segment[dispNumber*8+2]->setPos( x+10, 3 );
    m_segment[dispNumber*8+2]->setRotation(96);
    m_segment[dispNumber*8+3]->setPos( x-8, 19 );
    m_segment[dispNumber*8+4]->setPos( x-9, 3 );
    m_segment[dispNumber*8+4]->setRotation(96);
    m_segment[dispNumber*8+5]->setPos( x-7.5, -16 );
    m_segment[dispNumber*8+5]->setRotation(96);
    m_segment[dispNumber*8+6]->setPos( x-6.5, 0 );
    m_segment[dispNumber*8+7]->setPos( x+12, 19 );
}

void SevenSegment::remove()
{
    for( int i=0; i<m_numDisplays; ++i ) deleteDisplay( i );
    Component::remove();
}

void SevenSegment::paint( QPainter* p, const QStyleOptionGraphicsItem* option, QWidget* widget )
{
    Component::paint( p, option, widget );
    p->drawRect( m_area );
}