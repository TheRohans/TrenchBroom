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

#ifndef TransformEntityAttributesQuickFix_h
#define TransformEntityAttributesQuickFix_h

#include "Model/Issue.h"
#include "Model/IssueQuickFix.h"
#include "Model/MapFacade.h"
#include "Model/PushSelection.h"

#include <functional>

namespace TrenchBroom {
    namespace Model {
        class MapFacade;
        
        class TransformEntityAttributesQuickFix : public IssueQuickFix {
        public:
            using NameTransform = std::function<AttributeName(const AttributeName&)>;
            using ValueTransform = std::function<AttributeValue(const AttributeValue&)>;
        private:
            NameTransform m_nameTransform;
            ValueTransform m_valueTransform;
        public:
            TransformEntityAttributesQuickFix(const IssueType issueType, const String& description, const NameTransform& nameTransform, const ValueTransform& valueTransform);
        private:
            void doApply(MapFacade* facade, const Issue* issue) const override;
        };
    }
}

#endif /* TransformEntityAttributesQuickFix_h */
