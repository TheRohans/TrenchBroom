/*
 Copyright (C) 2010-2017 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TrenchBroom_wxUtils
#define TrenchBroom_wxUtils

#undef CursorShape
#include <QString>

#include "Color.h"
#include "StringUtils.h"

#include <QColor>

#include <vector>

class QWidget;
class QAbstractButton;
class QLineEdit;

namespace TrenchBroom {
    namespace View {
#if 0
        class MapFrame;

        MapFrame* findMapFrame(QWidget* window);
        wxFrame* findFrame(QWidget* window);

        void fitAll(QWidget* window);

        wxColor makeLighter(const wxColor& color);
        Color fromWxColor(const wxColor& color);
        wxColor toWxColor(const Color& color);

        std::vector<size_t> getListCtrlSelection(const wxListCtrl* listCtrl);
        void deselectAllListrCtrlItems(wxListCtrl* listCtrl);
#endif

        QAbstractButton* createBitmapButton(QWidget* parent, const String& image, const QString& tooltip);
#if 0
        QWidget* createBitmapToggleButton(QWidget* parent, const String& upImage, const String& downImage, const String& tooltip);

        QWidget* createDefaultPage(QWidget* parent, const QString& message);

        wxSizer* wrapDialogButtonSizer(wxSizer* buttonSizer, QWidget* parent);

        void setWindowIcon(wxTopLevelWindow* window);
        template <typename T>
        void setHint(T* ctrl, const QString& hint) {
#ifndef __WXGTK20__
            ctrl->SetHint(hint);
#endif
        }

        QStringList filterBySuffix(const QStringList& strings, const QString& suffix, bool caseSensitive = false);

        QString wxToQString(const QString& string);
#endif

        Color fromQColor(const QColor& color);
        void setWindowIconTB(QWidget* window);
        void setDebugBackgroundColor(QWidget* widget, const QColor& color);

        QLineEdit* createSearchBox();
    }
}

#endif /* defined(TrenchBroom_wxUtils) */
