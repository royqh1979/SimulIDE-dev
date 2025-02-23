/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#ifndef POTENTIOMETER_H
#define POTENTIOMETER_H

#include "dialed.h"
#include "e-resistor.h"
#include "pin.h"

class LibraryItem;

class Potentiometer : public Dialed, public eElement
{
    public:
        Potentiometer( QString type, QString id );
        ~Potentiometer();

 static Component* construct( QString type, QString id );
 static LibraryItem* libraryItem();

        virtual void initialize() override;
        virtual void updateStep() override;

        double getVal();
        void setVal( double val );

        double getRes() { return m_resist; }
        void setRes( double v );

        virtual void setLinkedValue( double v, int i=0 ) override;

        virtual void paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w ) override;

    protected:
        virtual void updateProxy() override;

    private:
        double m_resist;
        double m_voltOut;
        double m_res1;

        Pin m_pinA;
        Pin m_pinM;
        Pin m_pinB;
        ePin m_ePinA;
        ePin m_ePinB;
        
        eResistor m_resA;
        eResistor m_resB;
        
        eNode* m_midEnode;
};

#endif
