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

#include "PreferencePane.h"

namespace TrenchBroom {
    namespace View {
        PreferencePane::PreferencePane(wxWindow* parent) :
        wxPanel(parent, wxID_ANY) {}
        
        PreferencePane::~PreferencePane() {}
        
        bool PreferencePane::canResetToDefaults() {
            return doCanResetToDefaults();
        }

        void PreferencePane::resetToDefaults() {
            doResetToDefaults();
			updateControls();
        }

        void PreferencePane::updateControls() {
            doUpdateControls();
        }

        bool PreferencePane::validate() {
            return doValidate();
        }

        float PreferencePane::getSliderValue(wxSlider* slider) {
            return static_cast<float>(slider->GetValue()) / static_cast<float>(slider->GetMax());
        }

        void PreferencePane::setSliderValue(wxSlider* slider, const float value) {
            slider->SetValue(static_cast<int>(value * slider->GetMax()));
        }
    }
}
