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

#ifndef SUBCIRCUIT_H
#define SUBCIRCUIT_H

#include "chip.h"

class Tunnel;
class LibraryItem;

class MAINMODULE_EXPORT SubCircuit : public Chip
{
    Q_OBJECT

    public:
        SubCircuit( QObject* parent, QString type, QString id );
        ~SubCircuit();

 static Component* construct( QObject* parent, QString type, QString id );
 static LibraryItem* libraryItem();

        virtual void remove() override;

        virtual void setLogicSymbol( bool ls ) override;

        QList<Tunnel*> getPinTunnels() { return m_pinTunnels.values(); }
        Component* getMainComp(){ return m_mainComponent; }

        virtual QString toString() override;

    protected:
        void contextMenuEvent( QGraphicsSceneContextMenuEvent* event );

        void loadSubCircuit( QString file );

        virtual void addPin( QString id, QString type, QString label,
                            int pos, int xpos, int ypos, int angle, int length=8 );

        virtual void updatePin( QString id, QString type, QString label,
                                int xpos, int ypos, int angle, int length=8  );

        Component* m_mainComponent;

        QList<Component*>       m_compList;
        QList<Tunnel*>          m_subcTunnels;
        QHash<QString, Tunnel*> m_pinTunnels;
};
#endif
