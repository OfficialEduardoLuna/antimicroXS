/* antimicro Gamepad to KB+M event mapper
 * Copyright (C) 2015 Travis Nickles <nickles.travis@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "joydpad.h"
#include "inputdevice.h"

#include <QDebug>
#include <QHashIterator>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>


const QString JoyDPad::xmlName = "dpad";
const int JoyDPad::DEFAULTDPADDELAY = 0;

JoyDPad::JoyDPad(int index, int originset, SetJoystick *parentSet, QObject *parent) :
    QObject(parent)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    this->index = index;
    buttons = QHash<int, JoyDPadButton*> ();
    activeDiagonalButton = nullptr;
    prevDirection = JoyDPadButton::DpadCentered;
    pendingDirection = prevDirection;
    this->originset = originset;
    currentMode = StandardMode;
    this->parentSet = parentSet;
    this->dpadDelay = DEFAULTDPADDELAY;

    populateButtons();

    pendingEvent = false;
    pendingEventDirection = prevDirection;
    pendingIgnoreSets = false;

    directionDelayTimer.setSingleShot(true);
    connect(&directionDelayTimer, SIGNAL(timeout()), this, SLOT(dpadDirectionChangeEvent()));
}

JoyDPad::~JoyDPad()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    QHashIterator<int, JoyDPadButton*> iter(buttons);
    while (iter.hasNext())
    {
        JoyDPadButton *button = iter.next().value();
        delete button;
        button = nullptr;
    }

    buttons.clear();
}

JoyDPadButton *JoyDPad::getJoyButton(int index)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    return buttons.value(index);
}

void JoyDPad::populateButtons()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    JoyDPadButton* button = new JoyDPadButton (JoyDPadButton::DpadUp, originset, this, parentSet, this);
    buttons.insert(JoyDPadButton::DpadUp, button);

    button = new JoyDPadButton (JoyDPadButton::DpadDown, originset, this, parentSet, this);
    buttons.insert(JoyDPadButton::DpadDown, button);

    button = new JoyDPadButton(JoyDPadButton::DpadRight, originset, this, parentSet, this);
    buttons.insert(JoyDPadButton::DpadRight, button);

    button = new JoyDPadButton(JoyDPadButton::DpadLeft, originset, this, parentSet, this);
    buttons.insert(JoyDPadButton::DpadLeft, button);

    button = new JoyDPadButton(JoyDPadButton::DpadLeftUp, originset, this, parentSet, this);
    buttons.insert(JoyDPadButton::DpadLeftUp, button);

    button = new JoyDPadButton(JoyDPadButton::DpadRightUp, originset, this, parentSet, this);
    buttons.insert(JoyDPadButton::DpadRightUp, button);

    button = new JoyDPadButton(JoyDPadButton::DpadRightDown, originset, this, parentSet, this);
    buttons.insert(JoyDPadButton::DpadRightDown, button);

    button = new JoyDPadButton(JoyDPadButton::DpadLeftDown, originset, this, parentSet, this);
    buttons.insert(JoyDPadButton::DpadLeftDown, button);
}

QString JoyDPad::getName(bool fullForceFormat, bool displayNames)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    QString label = QString();

    if (!dpadName.isEmpty() && displayNames)
    {
        if (fullForceFormat)
        {
            label.append(trUtf8("DPad")).append(" ");
        }

        label.append(dpadName);
    }
    else if (!defaultDPadName.isEmpty())
    {
        if (fullForceFormat)
        {
            label.append(trUtf8("DPad")).append(" ");
        }
        label.append(defaultDPadName);
    }
    else
    {
        label.append(trUtf8("DPad")).append(" ");
        label.append(QString::number(getRealJoyNumber()));
    }

    return label;
}

int JoyDPad::getJoyNumber()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    return index;
}

int JoyDPad::getIndex()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    return index;
}

int JoyDPad::getRealJoyNumber()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    return index + 1;
}

QString JoyDPad::getXmlName()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    return this->xmlName;
}

void JoyDPad::readConfig(QXmlStreamReader *xml)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    if (xml->isStartElement() && (xml->name() == getXmlName()))
    {
        xml->readNextStartElement();
        while (!xml->atEnd() && (!xml->isEndElement() && (xml->name() != getXmlName())))
        {
            bool found = readMainConfig(xml);
            if (!found)
            {
                xml->skipCurrentElement();
            }

            xml->readNextStartElement();
        }
    }
}

bool JoyDPad::readMainConfig(QXmlStreamReader *xml)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    bool found = false;

    if ((xml->name() == "dpadbutton") && xml->isStartElement())
    {
        found = true;
        int index = xml->attributes().value("index").toString().toInt();
        JoyDPadButton* button = this->getJoyButton(index);
        if (button != nullptr)
        {
            button->readConfig(xml);
        }
        else
        {
            xml->skipCurrentElement();
        }
    }
    else if ((xml->name() == "mode") && xml->isStartElement())
    {
        found = true;
        QString temptext = xml->readElementText();
        if (temptext == "eight-way")
        {
            this->setJoyMode(EightWayMode);
        }
        else if (temptext == "four-way")
        {
            this->setJoyMode(FourWayCardinal);
        }
        else if (temptext == "diagonal")
        {
            this->setJoyMode(FourWayDiagonal);
        }
    }
    else if ((xml->name() == "dpadDelay") && xml->isStartElement())
    {
        found = true;
        QString temptext = xml->readElementText();
        int tempchoice = temptext.toInt();
        this->setDPadDelay(tempchoice);
    }

    return found;
}

void JoyDPad::writeConfig(QXmlStreamWriter *xml)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    if (!isDefault())
    {
        xml->writeStartElement(getXmlName());
        xml->writeAttribute("index", QString::number(index+1));
        if (currentMode == EightWayMode)
        {
            xml->writeTextElement("mode", "eight-way");
        }
        else if (currentMode == FourWayCardinal)
        {
            xml->writeTextElement("mode", "four-way");
        }
        else if (currentMode == FourWayDiagonal)
        {
            xml->writeTextElement("mode", "diagonal");
        }

        if (dpadDelay > DEFAULTDPADDELAY)
        {
            xml->writeTextElement("dpadDelay", QString::number(dpadDelay));
        }

        QHashIterator<int, JoyDPadButton*> iter(buttons);
        while (iter.hasNext())
        {
            JoyDPadButton *button = iter.next().value();
            button->writeConfig(xml);
        }

        xml->writeEndElement();
    }
}

void JoyDPad::queuePendingEvent(int value, bool ignoresets)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    pendingEvent = true;
    pendingEventDirection = value;
    pendingIgnoreSets = ignoresets;
}

void JoyDPad::activatePendingEvent()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    if (pendingEvent)
    {
        joyEvent(pendingEventDirection, pendingIgnoreSets);

        pendingEvent = false;
        pendingEventDirection = static_cast<int>(JoyDPadButton::DpadCentered);
        pendingIgnoreSets = false;
    }
}

bool JoyDPad::hasPendingEvent()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    return pendingEvent;
}

void JoyDPad::clearPendingEvent()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    pendingEvent = false;
    pendingEventDirection = static_cast<int>(JoyDPadButton::DpadCentered);
    pendingIgnoreSets = false;
}

void JoyDPad::joyEvent(int value, bool ignoresets)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    if (value != static_cast<int>(pendingDirection))
    {
        if (value != static_cast<int>(JoyDPadButton::DpadCentered))
        {
            if (prevDirection == JoyDPadButton::DpadCentered)
            {
                emit active(value);
            }

            pendingDirection = (JoyDPadButton::JoyDPadDirections)value;

            if (ignoresets || (dpadDelay == 0))
            {
                if (directionDelayTimer.isActive())
                {
                    directionDelayTimer.stop();
                }

                createDeskEvent(ignoresets);
            }
            else if (pendingDirection != prevDirection)
            {
                if (!directionDelayTimer.isActive())
                {
                    directionDelayTimer.start(dpadDelay);
                }
            }
            else
            {
                if (directionDelayTimer.isActive())
                {
                    directionDelayTimer.stop();
                }
            }
        }
        else
        {
            emit released(value);
            pendingDirection = JoyDPadButton::DpadCentered;
            if (ignoresets || dpadDelay == 0)
            {
                if (directionDelayTimer.isActive())
                {
                    directionDelayTimer.stop();
                }

                createDeskEvent(ignoresets);
            }
            else
            {
                if (!directionDelayTimer.isActive())
                {
                    directionDelayTimer.start(dpadDelay);
                }
            }
        }
    }
}

QHash<int, JoyDPadButton*>* JoyDPad::getJoyButtons()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    return &buttons;
}

int JoyDPad::getCurrentDirection()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    return prevDirection;
}

void JoyDPad::setJoyMode(JoyMode mode)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    currentMode = mode;
    emit joyModeChanged();
    emit propertyUpdated();
}

JoyDPad::JoyMode JoyDPad::getJoyMode()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    return currentMode;
}

void JoyDPad::releaseButtonEvents()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    QHashIterator<int, JoyDPadButton*> iter(buttons);
    while (iter.hasNext())
    {
        JoyDPadButton *button = iter.next().value();
        button->joyEvent(false, true);
    }
}

QHash<int, JoyDPadButton*>* JoyDPad::getButtons()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    return &buttons;
}

bool JoyDPad::isDefault()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    bool value = true;
    value = value && (currentMode == StandardMode);
    value = value && (dpadDelay == DEFAULTDPADDELAY);

    QHashIterator<int, JoyDPadButton*> iter(buttons);
    while (iter.hasNext())
    {
        JoyDPadButton *button = iter.next().value();
        value = value && (button->isDefault());
    }
    return value;
}

void JoyDPad::setButtonsMouseMode(JoyButton::JoyMouseMovementMode mode)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    QHashIterator<int, JoyDPadButton*> iter(buttons);
    while (iter.hasNext())
    {
        JoyDPadButton *button = iter.next().value();
        button->setMouseMode(mode);
    }
}

bool JoyDPad::hasSameButtonsMouseMode()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    bool result = true;

    JoyButton::JoyMouseMovementMode initialMode = JoyButton::MouseCursor;

    QHash<int, JoyDPadButton*> temphash = getApplicableButtons();
    QHashIterator<int, JoyDPadButton*> iter(temphash);
    while (iter.hasNext())
    {
        if (!iter.hasPrevious())
        {
            JoyDPadButton *button = iter.next().value();
            initialMode = button->getMouseMode();
        }
        else
        {
            JoyDPadButton *button = iter.next().value();
            JoyButton::JoyMouseMovementMode temp = button->getMouseMode();
            if (temp != initialMode)
            {
                result = false;
                iter.toBack();
            }
        }
    }

    return result;
}

JoyButton::JoyMouseMovementMode JoyDPad::getButtonsPresetMouseMode()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    JoyButton::JoyMouseMovementMode resultMode = JoyButton::MouseCursor;

    QHash<int, JoyDPadButton*> temphash = getApplicableButtons();
    QHashIterator<int, JoyDPadButton*> iter(temphash);
    while (iter.hasNext())
    {
        if (!iter.hasPrevious())
        {
            JoyDPadButton *button = iter.next().value();
            resultMode = button->getMouseMode();
        }
        else
        {
            JoyDPadButton *button = iter.next().value();
            JoyButton::JoyMouseMovementMode temp = button->getMouseMode();
            if (temp != resultMode)
            {
                resultMode = JoyButton::MouseCursor;
                iter.toBack();
            }
        }
    }

    return resultMode;
}

void JoyDPad::setButtonsMouseCurve(JoyButton::JoyMouseCurve mouseCurve)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    QHashIterator<int, JoyDPadButton*> iter(buttons);
    while (iter.hasNext())
    {
        JoyDPadButton *button = iter.next().value();
        button->setMouseCurve(mouseCurve);
    }
}

bool JoyDPad::hasSameButtonsMouseCurve()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    bool result = true;

    JoyButton::JoyMouseCurve initialCurve = JoyButton::LinearCurve;

    QHash<int, JoyDPadButton*> temphash = getApplicableButtons();
    QHashIterator<int, JoyDPadButton*> iter(temphash);
    while (iter.hasNext())
    {
        if (!iter.hasPrevious())
        {
            JoyDPadButton *button = iter.next().value();
            initialCurve = button->getMouseCurve();
        }
        else
        {
            JoyDPadButton *button = iter.next().value();
            JoyButton::JoyMouseCurve temp = button->getMouseCurve();
            if (temp != initialCurve)
            {
                result = false;
                iter.toBack();
            }
        }
    }

    return result;
}

JoyButton::JoyMouseCurve JoyDPad::getButtonsPresetMouseCurve()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    JoyButton::JoyMouseCurve resultCurve = JoyButton::LinearCurve;

    QHash<int, JoyDPadButton*> temphash = getApplicableButtons();
    QHashIterator<int, JoyDPadButton*> iter(temphash);
    while (iter.hasNext())
    {
        if (!iter.hasPrevious())
        {
            JoyDPadButton *button = iter.next().value();
            resultCurve = button->getMouseCurve();
        }
        else
        {
            JoyDPadButton *button = iter.next().value();
            JoyButton::JoyMouseCurve temp = button->getMouseCurve();
            if (temp != resultCurve)
            {
                resultCurve = JoyButton::LinearCurve;
                iter.toBack();
            }
        }
    }

    return resultCurve;
}

void JoyDPad::setButtonsSpringWidth(int value)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    QHashIterator<int, JoyDPadButton*> iter(buttons);
    while (iter.hasNext())
    {
        JoyDPadButton *button = iter.next().value();
        button->setSpringWidth(value);
    }
}

void JoyDPad::setButtonsSpringHeight(int value)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    QHashIterator<int, JoyDPadButton*> iter(buttons);
    while (iter.hasNext())
    {
        JoyDPadButton *button = iter.next().value();
        button->setSpringHeight(value);
    }
}

int JoyDPad::getButtonsPresetSpringWidth()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    int presetSpringWidth = 0;

    QHash<int, JoyDPadButton*> temphash = getApplicableButtons();
    QHashIterator<int, JoyDPadButton*> iter(temphash);
    while (iter.hasNext())
    {
        if (!iter.hasPrevious())
        {
            JoyDPadButton *button = iter.next().value();
            presetSpringWidth = button->getSpringWidth();
        }
        else
        {
            JoyDPadButton *button = iter.next().value();
            int temp = button->getSpringWidth();
            if (temp != presetSpringWidth)
            {
                presetSpringWidth = 0;
                iter.toBack();
            }
        }
    }

    return presetSpringWidth;
}

int JoyDPad::getButtonsPresetSpringHeight()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    int presetSpringHeight = 0;

    QHash<int, JoyDPadButton*> temphash = getApplicableButtons();
    QHashIterator<int, JoyDPadButton*> iter(temphash);
    while (iter.hasNext())
    {
        if (!iter.hasPrevious())
        {
            JoyDPadButton *button = iter.next().value();
            presetSpringHeight = button->getSpringHeight();
        }
        else
        {
            JoyDPadButton *button = iter.next().value();
            int temp = button->getSpringHeight();
            if (temp != presetSpringHeight)
            {
                presetSpringHeight = 0;
                iter.toBack();
            }
        }
    }

    return presetSpringHeight;
}

void JoyDPad::setButtonsSensitivity(double value)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    QHashIterator<int, JoyDPadButton*> iter(buttons);
    while (iter.hasNext())
    {
        JoyDPadButton *button = iter.next().value();
        button->setSensitivity(value);
    }
}

double JoyDPad::getButtonsPresetSensitivity()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    double presetSensitivity = 1.0;

    QHash<int, JoyDPadButton*> temphash = getApplicableButtons();
    QHashIterator<int, JoyDPadButton*> iter(temphash);
    while (iter.hasNext())
    {
        if (!iter.hasPrevious())
        {
            JoyDPadButton *button = iter.next().value();
            presetSensitivity = button->getSensitivity();
        }
        else
        {
            JoyDPadButton *button = iter.next().value();
            double temp = button->getSensitivity();
            if (temp != presetSensitivity)
            {
                presetSensitivity = 1.0;
                iter.toBack();
            }
        }
    }

    return presetSensitivity;
}

QHash<int, JoyDPadButton*> JoyDPad::getApplicableButtons()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    QHash<int, JoyDPadButton*> temphash;

    if ((currentMode == StandardMode) || (currentMode == EightWayMode) ||
        (currentMode == FourWayCardinal))
    {
        temphash.insert(JoyDPadButton::DpadUp, buttons.value(JoyDPadButton::DpadUp));
        temphash.insert(JoyDPadButton::DpadDown, buttons.value(JoyDPadButton::DpadDown));
        temphash.insert(JoyDPadButton::DpadLeft, buttons.value(JoyDPadButton::DpadLeft));
        temphash.insert(JoyDPadButton::DpadRight, buttons.value(JoyDPadButton::DpadRight));
    }

    if ((currentMode == EightWayMode) || (currentMode == FourWayDiagonal))
    {
        temphash.insert(JoyDPadButton::DpadLeftUp, buttons.value(JoyDPadButton::DpadLeftUp));
        temphash.insert(JoyDPadButton::DpadRightUp, buttons.value(JoyDPadButton::DpadRightUp));
        temphash.insert(JoyDPadButton::DpadRightDown, buttons.value(JoyDPadButton::DpadRightDown));
        temphash.insert(JoyDPadButton::DpadLeftDown, buttons.value(JoyDPadButton::DpadLeftDown));
    }

    return temphash;
}

void JoyDPad::setDPadName(QString tempName)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    if ((tempName.length() <= 20) && (tempName != dpadName))
    {
        dpadName = tempName;
        emit dpadNameChanged();
        emit propertyUpdated();
    }
}

const QString JoyDPad::getDpadName()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    return dpadName;
}

const QString JoyDPad::getDefaultDpadName() {

    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    return defaultDPadName;
}

void JoyDPad::setButtonsWheelSpeedX(int value)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    QHashIterator<int, JoyDPadButton*> iter(buttons);
    while (iter.hasNext())
    {
        JoyDPadButton *button = iter.next().value();
        button->setWheelSpeedX(value);
    }
}

void JoyDPad::setButtonsWheelSpeedY(int value)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    QHashIterator<int, JoyDPadButton*> iter(buttons);
    while (iter.hasNext())
    {
        JoyDPadButton *button = iter.next().value();
        button->setWheelSpeedY(value);
    }
}

void JoyDPad::setDefaultDPadName(QString tempname)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    defaultDPadName = tempname;
    emit dpadNameChanged();
}

QString JoyDPad::getDefaultDPadName()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    return defaultDPadName;
}

SetJoystick* JoyDPad::getParentSet()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    return parentSet;
}

void JoyDPad::establishPropertyUpdatedConnection()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    connect(this, SIGNAL(propertyUpdated()), getParentSet()->getInputDevice(), SLOT(profileEdited()));
}

void JoyDPad::disconnectPropertyUpdatedConnection()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    disconnect(this, SIGNAL(propertyUpdated()), getParentSet()->getInputDevice(), SLOT(profileEdited()));
}

bool JoyDPad::hasSlotsAssigned()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    bool hasSlots = false;

    QHash<int, JoyDPadButton*> temphash = getApplicableButtons();
    QHashIterator<int, JoyDPadButton*> iter(temphash);
    while (iter.hasNext())
    {
        JoyDPadButton *button = iter.next().value();
        if (button != nullptr)
        {
            if (button->getAssignedSlots()->count() > 0)
            {
                hasSlots = true;
                iter.toBack();
            }
        }
    }

    return hasSlots;
}

void JoyDPad::setButtonsSpringRelativeStatus(bool value)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    QHashIterator<int, JoyDPadButton*> iter(buttons);
    while (iter.hasNext())
    {
        JoyDPadButton *button = iter.next().value();
        button->setSpringRelativeStatus(value);
    }
}

bool JoyDPad::isRelativeSpring()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    bool relative = false;

    QHash<int, JoyDPadButton*> temphash = getApplicableButtons();
    QHashIterator<int, JoyDPadButton*> iter(temphash);
    while (iter.hasNext())
    {
        if (!iter.hasPrevious())
        {
            JoyDPadButton *button = iter.next().value();
            relative = button->isRelativeSpring();
        }
        else
        {
            JoyDPadButton *button = iter.next().value();
            bool temp = button->isRelativeSpring();
            if (temp != relative)
            {
                relative = false;
                iter.toBack();
            }
        }
    }

    return relative;
}

void JoyDPad::copyAssignments(JoyDPad *destDPad)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    destDPad->activeDiagonalButton = activeDiagonalButton;
    destDPad->prevDirection = prevDirection;
    destDPad->currentMode = currentMode;
    destDPad->dpadDelay = dpadDelay;

    QHashIterator<int, JoyDPadButton*> iter(destDPad->buttons);
    while (iter.hasNext())
    {
        JoyDPadButton *destButton = iter.next().value();
        if (destButton != nullptr)
        {
            JoyDPadButton *sourceButton = buttons.value(destButton->getDirection());
            if (sourceButton != nullptr)
            {
                sourceButton->copyAssignments(destButton);
            }
        }
    }

    if (!destDPad->isDefault())
    {
        emit propertyUpdated();
    }
}

void JoyDPad::createDeskEvent(bool ignoresets)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    JoyDPadButton *curButton = nullptr;
    JoyDPadButton *prevButton = nullptr;
    JoyDPadButton::JoyDPadDirections value = pendingDirection;

    if (pendingDirection != prevDirection)
    {
        if (activeDiagonalButton != nullptr)
        {
            activeDiagonalButton->joyEvent(false, ignoresets);
            activeDiagonalButton = nullptr;
        }
        else {
            if (currentMode == StandardMode)
            {
                if ((prevDirection & JoyDPadButton::DpadUp) && (!(value & JoyDPadButton::DpadUp)))
                {
                    prevButton = buttons.value(JoyDPadButton::DpadUp);
                    prevButton->joyEvent(false, ignoresets);
                }

                if ((prevDirection & JoyDPadButton::DpadDown) && (!(value & JoyDPadButton::DpadDown)))
                {
                    prevButton = buttons.value(JoyDPadButton::DpadDown);
                    prevButton->joyEvent(false, ignoresets);
                }

                if ((prevDirection & JoyDPadButton::DpadLeft) && (!(value & JoyDPadButton::DpadLeft)))
                {
                    prevButton = buttons.value(JoyDPadButton::DpadLeft);
                    prevButton->joyEvent(false, ignoresets);
                }

                if ((prevDirection & JoyDPadButton::DpadRight) && (!(value & JoyDPadButton::DpadRight)))
                {
                    prevButton = buttons.value(JoyDPadButton::DpadRight);
                    prevButton->joyEvent(false, ignoresets);
                }
            }
            else if ((currentMode == EightWayMode) && prevDirection)
            {
                prevButton = buttons.value(prevDirection);
                prevButton->joyEvent(false, ignoresets);
            }
            else if ((currentMode == FourWayCardinal) && (static_cast<int>(prevDirection) != 0))
            {
                if (((prevDirection == JoyDPadButton::DpadUp) ||
                    (prevDirection == JoyDPadButton::DpadRightUp)) &&
                    ((value != JoyDPadButton::DpadUp) && (value != JoyDPadButton::DpadRightUp)))
                {
                    prevButton = buttons.value(JoyDPadButton::DpadUp);
                }
                else if (((prevDirection == JoyDPadButton::DpadDown) ||
                         (prevDirection == JoyDPadButton::DpadLeftDown)) &&
                         ((value != JoyDPadButton::DpadDown) && (value != JoyDPadButton::DpadLeftDown)))
                {
                    prevButton = buttons.value(JoyDPadButton::DpadDown);
                }
                else if (((prevDirection == JoyDPadButton::DpadLeft) ||
                         (prevDirection == JoyDPadButton::DpadLeftUp)) &&
                         ((value != JoyDPadButton::DpadLeft) && (value != JoyDPadButton::DpadLeftUp)))
                {
                    prevButton = buttons.value(JoyDPadButton::DpadLeft);
                }
                else if (((prevDirection == JoyDPadButton::DpadRight) ||
                         (prevDirection == JoyDPadButton::DpadRightDown)) &&
                         ((value != JoyDPadButton::DpadRight) && (value != JoyDPadButton::DpadRightDown)))
                {
                    prevButton = buttons.value(JoyDPadButton::DpadRight);
                }

                if (prevButton != nullptr)
                {
                    prevButton->joyEvent(false, ignoresets);
                }

            }
            else if ((currentMode == FourWayDiagonal) && (static_cast<int>(prevDirection) != 0))
            {
                prevButton = buttons.value(prevDirection);
                prevButton->joyEvent(false, ignoresets);
            }
        }

        if (currentMode == StandardMode)
        {
            if ((value & JoyDPadButton::DpadUp) && (!(prevDirection & JoyDPadButton::DpadUp)))
            {
                curButton = buttons.value(JoyDPadButton::DpadUp);
                curButton->joyEvent(true, ignoresets);
            }

            if ((value & JoyDPadButton::DpadDown) && (!(prevDirection & JoyDPadButton::DpadDown)))
            {
                curButton = buttons.value(JoyDPadButton::DpadDown);
                curButton->joyEvent(true, ignoresets);
            }

            if ((value & JoyDPadButton::DpadLeft) && (!(prevDirection & JoyDPadButton::DpadLeft)))
            {
                curButton = buttons.value(JoyDPadButton::DpadLeft);
                curButton->joyEvent(true, ignoresets);
            }

            if ((value & JoyDPadButton::DpadRight) && (!(prevDirection & JoyDPadButton::DpadRight)))
            {
                curButton = buttons.value(JoyDPadButton::DpadRight);
                curButton->joyEvent(true, ignoresets);
            }
        }
        else if (currentMode == EightWayMode)
        {
            if (value == JoyDPadButton::DpadLeftUp)
            {
                activeDiagonalButton = buttons.value(JoyDPadButton::DpadLeftUp);
                activeDiagonalButton->joyEvent(true, ignoresets);
            }
            else if (value == JoyDPadButton::DpadRightUp)
            {
                activeDiagonalButton = buttons.value(JoyDPadButton::DpadRightUp);
                activeDiagonalButton->joyEvent(true, ignoresets);
            }
            else if (value == JoyDPadButton::DpadRightDown)
            {
                activeDiagonalButton = buttons.value(JoyDPadButton::DpadRightDown);
                activeDiagonalButton->joyEvent(true, ignoresets);
            }
            else if (value == JoyDPadButton::DpadLeftDown)
            {
                activeDiagonalButton = buttons.value(JoyDPadButton::DpadLeftDown);
                activeDiagonalButton->joyEvent(true, ignoresets);
            }
            else if (value == JoyDPadButton::DpadUp)
            {
                curButton = buttons.value(JoyDPadButton::DpadUp);
                curButton->joyEvent(true, ignoresets);
            }
            else if (value == JoyDPadButton::DpadDown)
            {
                curButton = buttons.value(JoyDPadButton::DpadDown);
                curButton->joyEvent(true, ignoresets);
            }
            else if (value == JoyDPadButton::DpadLeft)
            {
                curButton = buttons.value(JoyDPadButton::DpadLeft);
                curButton->joyEvent(true, ignoresets);
            }
            else if (value == JoyDPadButton::DpadRight)
            {
                curButton = buttons.value(JoyDPadButton::DpadRight);
                curButton->joyEvent(true, ignoresets);
            }
        }
        else if (currentMode == FourWayCardinal)
        {
            if ((value == JoyDPadButton::DpadUp) ||
                (value == JoyDPadButton::DpadRightUp))
            {
                curButton = buttons.value(JoyDPadButton::DpadUp);
                curButton->joyEvent(true, ignoresets);
            }
            else if ((value == JoyDPadButton::DpadDown) ||
                     (value == JoyDPadButton::DpadLeftDown))
            {
                curButton = buttons.value(JoyDPadButton::DpadDown);
                curButton->joyEvent(true, ignoresets);
            }
            else if ((value == JoyDPadButton::DpadLeft) ||
                     (value == JoyDPadButton::DpadLeftUp))
            {
                curButton = buttons.value(JoyDPadButton::DpadLeft);
                curButton->joyEvent(true, ignoresets);
            }
            else if ((value == JoyDPadButton::DpadRight) ||
                     (value == JoyDPadButton::DpadRightDown))
            {
                curButton = buttons.value(JoyDPadButton::DpadRight);
                curButton->joyEvent(true, ignoresets);
            }
        }
        else if (currentMode == FourWayDiagonal)
        {
            if (value == JoyDPadButton::DpadLeftUp)
            {
                activeDiagonalButton = buttons.value(JoyDPadButton::DpadLeftUp);
                activeDiagonalButton->joyEvent(true, ignoresets);
            }
            else if (value == JoyDPadButton::DpadRightUp)
            {
                activeDiagonalButton = buttons.value(JoyDPadButton::DpadRightUp);
                activeDiagonalButton->joyEvent(true, ignoresets);
            }
            else if (value == JoyDPadButton::DpadRightDown)
            {
                activeDiagonalButton = buttons.value(JoyDPadButton::DpadRightDown);
                activeDiagonalButton->joyEvent(true, ignoresets);
            }
            else if (value == JoyDPadButton::DpadLeftDown)
            {
                activeDiagonalButton = buttons.value(JoyDPadButton::DpadLeftDown);
                activeDiagonalButton->joyEvent(true, ignoresets);
            }
        }

        prevDirection = pendingDirection;
    }
}

void JoyDPad::dpadDirectionChangeEvent()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    createDeskEvent();
}

void JoyDPad::setDPadDelay(int value)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    if (((value >= 10) && (value <= 1000)) || (value == 0))
    {
        this->dpadDelay = value;
        emit dpadDelayChanged(value);
        emit propertyUpdated();
    }
}

int JoyDPad::getDPadDelay()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    return dpadDelay;
}

void JoyDPad::setButtonsEasingDuration(double value)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    QHash<int, JoyDPadButton*> temphash = getApplicableButtons();
    QHashIterator<int, JoyDPadButton*> iter(temphash);
    while (iter.hasNext())
    {
        JoyDPadButton *button = iter.next().value();
        button->setEasingDuration(value);
    }
}

double JoyDPad::getButtonsEasingDuration()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    double result = JoyButton::DEFAULTEASINGDURATION;

    QHash<int, JoyDPadButton*> temphash = getApplicableButtons();
    QHashIterator<int, JoyDPadButton*> iter(temphash);
    while (iter.hasNext())
    {
        if (!iter.hasPrevious())
        {
            JoyDPadButton *button = iter.next().value();
            result = button->getEasingDuration();
        }
        else
        {
            JoyDPadButton *button = iter.next().value();
            double temp = button->getEasingDuration();
            if (temp != result)
            {
                result = JoyButton::DEFAULTEASINGDURATION;
                iter.toBack();
            }
        }
    }

    return result;
}

void JoyDPad::setButtonsSpringDeadCircleMultiplier(int value)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    QHash<int, JoyDPadButton*> temphash = getApplicableButtons();
    QHashIterator<int, JoyDPadButton*> iter(temphash);
    while (iter.hasNext())
    {
        JoyDPadButton *button = iter.next().value();
        button->setSpringDeadCircleMultiplier(value);
    }
}

int JoyDPad::getButtonsSpringDeadCircleMultiplier()
{

    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;
    int result = JoyButton::DEFAULTSPRINGRELEASERADIUS;

    QHash<int, JoyDPadButton*> temphash = getApplicableButtons();
    QHashIterator<int, JoyDPadButton*> iter(temphash);
    while (iter.hasNext())
    {
        if (!iter.hasPrevious())
        {
            JoyDPadButton *button = iter.next().value();
            result = button->getSpringDeadCircleMultiplier();
        }
        else
        {
            JoyDPadButton *button = iter.next().value();
            int temp = button->getSpringDeadCircleMultiplier();
            if (temp != result)
            {
                result = JoyButton::DEFAULTSPRINGRELEASERADIUS;
                iter.toBack();
            }
        }
    }

    return result;
}

void JoyDPad::setButtonsExtraAccelerationCurve(JoyButton::JoyExtraAccelerationCurve curve)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    QHash<int, JoyDPadButton*> temphash = getApplicableButtons();
    QHashIterator<int, JoyDPadButton*> iter(temphash);
    while (iter.hasNext())
    {
        JoyDPadButton *button = iter.next().value();
        button->setExtraAccelerationCurve(curve);
    }
}

JoyButton::JoyExtraAccelerationCurve JoyDPad::getButtonsExtraAccelerationCurve()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    JoyButton::JoyExtraAccelerationCurve result = JoyButton::LinearAccelCurve;

    QHash<int, JoyDPadButton*> temphash = getApplicableButtons();
    QHashIterator<int, JoyDPadButton*> iter(temphash);
    while (iter.hasNext())
    {
        if (!iter.hasPrevious())
        {
            JoyDPadButton *button = iter.next().value();
            result = button->getExtraAccelerationCurve();
        }
        else
        {
            JoyDPadButton *button = iter.next().value();
            JoyButton::JoyExtraAccelerationCurve temp = button->getExtraAccelerationCurve();
            if (temp != result)
            {
                result = JoyButton::LinearAccelCurve;
                iter.toBack();
            }
        }
    }

    return result;
}

QHash<int, JoyDPadButton*> JoyDPad::getDirectionButtons(JoyDPadButton::JoyDPadDirections direction)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    QHash<int, JoyDPadButton*> temphash;
    if (currentMode == StandardMode)
    {
        if (direction & JoyDPadButton::DpadUp)
        {
            temphash.insert(JoyDPadButton::DpadUp, buttons.value(JoyDPadButton::DpadUp));
        }

        if (direction & JoyDPadButton::DpadDown)
        {
            temphash.insert(JoyDPadButton::DpadDown, buttons.value(JoyDPadButton::DpadDown));
        }

        if (direction & JoyDPadButton::DpadLeft)
        {
            temphash.insert(JoyDPadButton::DpadLeft, buttons.value(JoyDPadButton::DpadLeft));
        }

        if (direction & JoyDPadButton::DpadRight)
        {
            temphash.insert(JoyDPadButton::DpadRight, buttons.value(JoyDPadButton::DpadRight));
        }
    }
    else if (currentMode == EightWayMode)
    {
        if (direction != JoyDPadButton::DpadCentered)
        {
            temphash.insert(direction, buttons.value(direction));
        }
    }
    else if (currentMode == FourWayCardinal)
    {
        if ((direction == JoyDPadButton::DpadUp) ||
            (direction == JoyDPadButton::DpadDown) ||
            (direction == JoyDPadButton::DpadLeft) ||
            (direction == JoyDPadButton::DpadRight))
        {
            temphash.insert(direction, buttons.value(direction));
        }
    }
    else if (currentMode == FourWayDiagonal)
    {
        if ((direction == JoyDPadButton::DpadRightUp) ||
            (direction == JoyDPadButton::DpadRightDown) ||
            (direction == JoyDPadButton::DpadLeftDown) ||
            (direction == JoyDPadButton::DpadLeftUp))
        {
            temphash.insert(direction, buttons.value(direction));
        }
    }

    return temphash;
}

void JoyDPad::setDirButtonsUpdateInitAccel(JoyDPadButton::JoyDPadDirections direction, bool state)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    QHash<int, JoyDPadButton*> apphash = getDirectionButtons(direction);
    QHashIterator<int, JoyDPadButton*> iter(apphash);
    while (iter.hasNext())
    {
        JoyDPadButton *button = iter.next().value();
        if (button != nullptr)
        {
            button->setUpdateInitAccel(state);
        }
    }
}

void JoyDPad::copyLastDistanceValues(JoyDPad *srcDPad)
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    QHash<int, JoyDPadButton*> apphash = srcDPad->getApplicableButtons();
    QHashIterator<int, JoyDPadButton*> iter(apphash);
    while (iter.hasNext())
    {
        JoyDPadButton *button = iter.next().value();
        if ((button != nullptr) && button->getButtonState())
        {
            this->buttons.value(iter.key())->copyLastAccelerationDistance(button);
            this->buttons.value(iter.key())->copyLastMouseDistanceFromDeadZone(button);
        }
    }
}

void JoyDPad::eventReset()
{
    qDebug() << "[" << __FILE__ << ": " << __LINE__ << "] " << __FUNCTION__;

    QHash<int, JoyDPadButton*> temphash = getApplicableButtons();
    QHashIterator<int, JoyDPadButton*> iter(temphash);
    while (iter.hasNext())
    {
        JoyDPadButton *button = iter.next().value();
        button->eventReset();
    }
}
