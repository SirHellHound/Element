/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "gui/LookAndFeel.h"
#include "gui/nodes/OSCSenderNodeEditor.h"

namespace Element {

OSCSenderNodeEditor::OSCSenderNodeEditor (const Node& node)
    : NodeEditorComponent (node)
{
    oscSenderNodePtr = getNodeObjectOfType<OSCSenderNode>();
    currentPortNumber = oscSenderNodePtr->getCurrentPortNumber();
    currentHostName = oscSenderNodePtr->getCurrentHostName();
    connected = oscSenderNodePtr->isConnected();
    paused = oscSenderNodePtr->isPaused();

    int width = 540;
    int height = 320;

    resetBounds(width, height);

    hostNameField.setText(currentHostName, NotificationType::dontSendNotification);
    hostNameField.setEditable (true, true, true);
    portNumberField.setEditable (true, true, true);

    updateConnectionStatusLabel();

    addAndMakeVisible (hostNameLabel);
    addAndMakeVisible (hostNameField);
    addAndMakeVisible (portNumberLabel);
    addAndMakeVisible (portNumberField);
    addAndMakeVisible (connectButton);
    addAndMakeVisible (pauseButton);
    addAndMakeVisible (clearButton);
    addAndMakeVisible (connectionStatusLabel);
    addAndMakeVisible (oscSenderLog);

    setSize (width, height);

    /* Bind handlers */
    connectButton.onClick = std::bind (&OSCSenderNodeEditor::connectButtonClicked, this);
    pauseButton.onClick = std::bind (&OSCSenderNodeEditor::pauseButtonClicked, this);
    clearButton.onClick = std::bind (&OSCSenderNodeEditor::clearButtonClicked, this);
    hostNameField.onTextChange = std::bind (&OSCSenderNodeEditor::hostNameFieldChanged, this);

    startTimerHz (60);
}

OSCSenderNodeEditor::~OSCSenderNodeEditor()
{
    /* Unbind handlers */
    connectButton.onClick = nullptr;
    clearButton.onClick = nullptr;
    stopTimer ();
}

void OSCSenderNodeEditor::timerCallback() {
    std::vector<OSCMessage> oscMessages = oscSenderNodePtr->getOscMessages();
    for(auto msg : oscMessages)
    {
        oscSenderLog.addOSCMessage( msg );
    }
};

void OSCSenderNodeEditor::resized ()
{
    resetBounds(getWidth(), getHeight());
}

void OSCSenderNodeEditor::resetBounds (int fullWidth, int fullHeight)
{
    int margin = 5;

    int x = margin;
    int y = margin;
    int w;
    int h;

    h = 20;

    w = 40;
    hostNameLabel.setBounds (x, y, w, h);
    x += w + margin;

    w = 80;
    hostNameField.setBounds (x, y, w, h);
    x += w + margin;

    w = 40;
    portNumberLabel.setBounds (x, y, w, h);
    x += w;

    w = 50;
    portNumberField.setBounds (x, y, w, h);
    x += w + margin;

    // From right side

    w = 60;
    x = fullWidth - margin - w;
    clearButton.setBounds (x, y, w, h);

    w = 60;
    x -= margin + w;
    pauseButton.setBounds (x, y, w, h);

    w = 80;
    x -= margin + w;
    connectButton.setBounds (x, y, w, h);

    w = 30;
    x -= margin + w;
    connectionStatusLabel.setBounds (x, y, w, h);

    // Row
    x = 0;
    y += h + margin;
    oscSenderLog.setBounds (x, y, fullWidth, fullHeight - y);
}

void OSCSenderNodeEditor::paint (Graphics& g)
{
    g.fillAll (LookAndFeel::backgroundColor.brighter(0.1));
}

void OSCSenderNodeEditor::connectButtonClicked()
{
    if (! connected)
        connect();
    else
        disconnect();

    updateConnectionStatusLabel();
}

void OSCSenderNodeEditor::pauseButtonClicked()
{
    paused = oscSenderNodePtr->togglePause();
    updatePauseButton();
}

void OSCSenderNodeEditor::clearButtonClicked()
{
    oscSenderLog.clear();
}

void OSCSenderNodeEditor::hostNameFieldChanged()
{
    disconnect();
    currentHostName = hostNameField.getText();
}

void OSCSenderNodeEditor::connect()
{
    auto portToConnect = portNumberField.getText().getIntValue();

    if (! OSCProcessor::isValidOscPort (portToConnect))
    {
        handleInvalidPortNumberEntered();
        return;
    }

    auto hostName = hostNameField.getText();

    if (oscSenderNodePtr->connect (hostName, portToConnect))
    {
        currentHostName = hostName;
        currentPortNumber = portToConnect;
        connected = true;
        connectButton.setButtonText ("Disconnect");
    }
    else
    {
        handleConnectError (portToConnect);
    }
}

void OSCSenderNodeEditor::disconnect()
{
    if (oscSenderNodePtr->disconnect())
    {
        currentPortNumber = -1;
        currentHostName = "";
        connected = false;
        connectButton.setButtonText ("Connect");
    }
    else
    {
        handleDisconnectError();
    }
}

void OSCSenderNodeEditor::handleConnectError (int failedPort)
{
    AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                        "OSC Connection error",
                                        "Could not connect to port " + String (failedPort) + ".",
                                        "OK");
}

void OSCSenderNodeEditor::handleDisconnectError()
{
    AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                        "Unknown error",
                                        "An unknown error occurred while trying to disconnect from UDP port.",
                                        "OK");
}

void OSCSenderNodeEditor::handleInvalidPortNumberEntered()
{
    AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                        "Invalid port number",
                                        "You have entered an invalid UDP port number.",
                                        "OK");
}

void OSCSenderNodeEditor::updateConnectButton()
{
    connectButton.setButtonText ( connected ? "Disconnect" : "Connect" );
}

void OSCSenderNodeEditor::updateConnectionStatusLabel()
{
    String text = connected ? "On" : "Off";
    auto textColour = connected ? Colours::green.brighter(0.3) : Colours::red.brighter(0.3);

    connectionStatusLabel.setText (text, dontSendNotification);
    connectionStatusLabel.setColour (Label::textColourId, textColour);
}

void OSCSenderNodeEditor::updatePauseButton()
{
    pauseButton.setButtonText ( paused ? "Resume" : "Pause" );
};


};
