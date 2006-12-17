/* This file is part of the KDE project
 * Copyright (C) 2006 Thomas Zander <zander@kde.org>
 * Copyright (C) 2006 Jan Hambrecht <jaham@gmx.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KOSHAPEDISTRIBUTECOMMAND_H
#define KOSHAPEDISTRIBUTECOMMAND_H


#include "commands/KoShapeMoveCommand.h"
#include "KoSelection.h"

#include <koffice_export.h>
#include <kcommand.h>

/// The undo / redo command for distributing shapes
class FLAKE_EXPORT KoShapeDistributeCommand : public KCommand
{
public:
    /// The different options to ditribute with this command
    enum Distribute
    {
        HorizontalCenterDistribution,   ///< Horizontal centered
        HorizontalGapsDistribution,     ///< Horizontal Gaps
        HorizontalLeftDistribution,     ///< Horizontal Left
        HorizontalRightDistribution,    ///< Horizontal Right
        VerticalCenterDistribution,     ///< Vertical centered
        VerticalGapsDistribution,       ///< Vertical Gaps
        VerticalBottomDistribution,     ///< Vertical bottom
        VerticalTopDistribution         ///< Vertical top
    };
    /**
     * Command to align a set of shapes in a rect
     * @param shapes a set of all the shapes that should be distributed
     * @param distribute the distribution type
     * @param boundingRect the rect the shapes will be distributed in
     */
    KoShapeDistributeCommand( const KoSelectionSet &shapes, Distribute distribute, QRectF boundingRect );
    virtual ~KoShapeDistributeCommand();
    /// execute the command
    virtual void execute();
    /// revert the actions done in execute
    virtual void unexecute();
    /// return the name of this command
    virtual QString name () const;
private:
    double getAvailableSpace( KoShape *first, KoShape *last, double extent, QRectF boundingRect );
    Distribute m_distribute;
    KoShapeMoveCommand *m_command;
};

#endif
