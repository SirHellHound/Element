
#pragma once

#include "gui/GuiCommon.h"

namespace Element {
    
class ElementsNavigationPanel : public SessionGraphsListBox
{
public:
    ElementsNavigationPanel() {
        
    }
    bool keyPressed(const KeyPress& kp) override
    {
        // Allows menu command to pass through, maybe a better way
        // to do this.
        if (kp.getKeyCode() == KeyPress::backspaceKey)
            return Component::keyPressed (kp);
            return ListBox::keyPressed (kp);
            }
    
    void paintListBoxItem (int, Graphics&, int, int, bool) override { }
    
    void listBoxItemClicked (int row, const MouseEvent& ev) override
    {
        if (ev.mods.isPopupMenu())
            return;
        const Node graph (getSelectedGraph());
        
        if (auto* cc = ViewHelpers::findContentComponent (this))
        {
            auto session (cc->getSession());
            if (row != session->getActiveGraphIndex())
            {
                auto graphs = graph.getValueTree().getParent();
                graphs.setProperty ("active", graphs.indexOf(graph.node()), nullptr);
                if (auto* ec = cc->getAppController().findChild<EngineController>())
                    ec->setRootNode (graph);
                    cc->stabilize();
                    }
        }
    }
    
    Component* refreshComponentForRow (int row, bool isSelected, Component* c) override
    {
        Row* r = (nullptr == c) ? new Row (*this) : dynamic_cast<Row*> (c);
        jassert(r);
        r->updateContent (getGraph(row), row, isSelected);
        return r;
        }
        
    private:
        class Row : public Component,
                            public Label::Listener,
                        public ButtonListener
        {
        public:
            Row (ElementsNavigationPanel& _owner) : owner (_owner)
            {
                addAndMakeVisible (text);
                text.setJustificationType (Justification::centredLeft);
                text.setInterceptsMouseClicks (false, false);
                text.setColour (Label::textWhenEditingColourId, LookAndFeel::textColor.darker());
                text.setColour (Label::backgroundWhenEditingColourId, Colours::black);
                text.addListener (this);
                
                // TODO: conf button
                // addAndMakeVisible (conf);
                conf.setButtonText ("=");
                conf.addListener (this);
            }
            
            ~Row() {
                text.removeListener (this);
            }
            
            void updateContent (const Node& _node, int _row, bool _selected)
            {
                node        = _node;
                row         = _row;
                selected    = _selected;
                if (node.isValid())
                    text.getTextValue().referTo (node.getPropertyAsValue (Tags::name));
            }
            
            void mouseDown (const MouseEvent& ev) override
            {
                owner.selectRow (row);
                
                if (ev.getNumberOfClicks() == 2)
                {
                    if (! text.isBeingEdited())
                    {
                        text.showEditor();
                    }
                }
                else
                {
                    if (! text.isBeingEdited())
                    {
                        owner.listBoxItemClicked (row, ev);
                    }
                }
            }
            
            void paint (Graphics& g) override
            {
                if (text.isBeingEdited())
                    g.fillAll (Colours::black);
                else
                    ViewHelpers::drawBasicTextRow ("", g, getWidth(), getHeight(), selected);
            }
            
            void resized() override
            {
                auto r (getLocalBounds());
                r.removeFromRight (4);
                
                if (conf.isVisible())
                {
                    conf.setBounds (r.removeFromRight (20).withSizeKeepingCentre (16, 16));
                    r.removeFromRight (4);
                }
                
                r.removeFromLeft (10);
                text.setBounds (r);
            }
            
            void labelTextChanged (Label*) override {}
            void editorShown (Label*, TextEditor&) override
            {
                savedText = text.getText();
                text.setInterceptsMouseClicks (true, true);
                repaint();
            }
            
            void editorHidden (Label*, TextEditor&) override
            {
                if (text.getText().isEmpty())
                    text.setText (savedText.isNotEmpty() ? savedText : "Untitled", dontSendNotification);
                text.setInterceptsMouseClicks (false, false);
                repaint (0, 0, 20, getHeight());
            }
            
            void buttonClicked (Button*) override
            {
                owner.selectRow (row);
                ViewHelpers::invokeDirectly (this, Commands::showGraphConfig, false);
            }
            
        private:
            ElementsNavigationPanel& owner;
            Node node;
            Label text;
            String savedText;
            SettingButton conf;
            int row = 0;
            bool selected = false;
        };
    };


class NavigationConcertinaPanel : public ConcertinaPanel
{
public:
    NavigationConcertinaPanel (Globals& g)
    : globals (g), headerHeight (30),
    defaultPanelHeight (80)
    {
        setLookAndFeel (&lookAndFeel);
        updateContent();
    }
    
    ~NavigationConcertinaPanel()
    {
        setLookAndFeel (nullptr);
    }
    
    int getIndexOfPanel (Component* panel)
    {
        if (nullptr == panel)
            return -1;
        for (int i = 0; i < getNumPanels(); ++i)
            if (auto* p = getPanel (i))
                if (p == panel)
                    return i;
        return -1;
    }
    
    template<class T> T* findPanel()
    {
        for (int i = getNumPanels(); --i >= 0;)
            if (auto* panel = dynamic_cast<T*> (getPanel (i)))
                return panel;
        return nullptr;
    }
    
    void clearPanels()
    {
        for (int i = 0; i < comps.size(); ++i)
            removePanel (comps.getUnchecked (i));
        comps.clearQuick (true);
    }
    
    void updateContent()
    {
        clearPanels();
        Component* c = nullptr;
        c = new ElementsNavigationPanel();
        auto *h = new ElementsHeader (*this, *c);
        addPanelInternal (-1, c, "Elements", h);
    }
    
    AudioIOPanelView* getAudioIOPanel() { return findPanel<AudioIOPanelView>(); }
    PluginsPanelView* getPluginsPanel() { return findPanel<PluginsPanelView>(); }
    SessionGraphsListBox* getSessionPanel() { return findPanel<SessionGraphsListBox>(); }
    
    const StringArray& getNames() const { return names; }
    const int getHeaderHeight() const { return headerHeight; }
    void setHeaderHeight (const int newHeight)
    {
        jassert (newHeight > 0);
        headerHeight = newHeight;
        updateContent();
    }
    
private:
    typedef Element::LookAndFeel ELF;
    Globals& globals;
    int headerHeight;
    int defaultPanelHeight;
    
    StringArray names;
    OwnedArray<Component> comps;
    void addPanelInternal (const int index, Component* comp, const String& name = String(),
                           Component* header = nullptr)
    {
        jassert(comp);
        if (name.isNotEmpty())
            comp->setName (name);
        addPanel (index, comps.insert(index, comp), false);
        setPanelHeaderSize (comp, headerHeight);
        if (!header)
            header = new Header (*this, *comp);
        setCustomPanelHeader (comp, header, true);
    }
    
    class Header : public Component
    {
    public:
        Header (NavigationConcertinaPanel& _parent, Component& _panel)
        : parent(_parent), panel(_panel)
        {
            addAndMakeVisible (text);
            text.setColour (Label::textColourId, ELF::textColor);
        }
        
        virtual ~Header() { }
        
        virtual void resized() override
        {
            text.setBounds (4, 1, 100, getHeight() - 2);
        }
        
        virtual void paint (Graphics& g) override
        {
            getLookAndFeel().drawConcertinaPanelHeader (
                                                        g, getLocalBounds(), false, false, parent, panel);
        }
        
    protected:
        NavigationConcertinaPanel& parent;
        Component& panel;
        Label text;
    };
    
    class ElementsHeader : public Header,
    public ButtonListener
    {
    public:
        ElementsHeader (NavigationConcertinaPanel& _parent, Component& _panel)
        : Header (_parent, _panel)
        {
            addAndMakeVisible (addButton);
            addButton.setButtonText ("+");
            addButton.addListener (this);
            setInterceptsMouseClicks (false, true);
        }
        
        void resized() override
        {
            const int padding = 4;
            const int buttonSize = getHeight() - (padding * 2);
            addButton.setBounds (getWidth() - padding - buttonSize,
                                 padding, buttonSize, buttonSize);
        }
        
        void buttonClicked (Button*) override
        {
            if (auto* cc = findParentComponentOfClass<ContentComponent>())
                cc->getGlobals().getCommandManager().invokeDirectly (
                                                                     (int)Commands::sessionAddGraph, true);
                }
        
    private:
        TextButton addButton;
    };
    
    class LookAndFeel : public Element::LookAndFeel
    {
    public:
        LookAndFeel() { }
        ~LookAndFeel() { }
        
        void drawConcertinaPanelHeader (Graphics& g, const Rectangle<int>& area,
                                        bool isMouseOver, bool isMouseDown,
                                        ConcertinaPanel& panel, Component& comp)
        {
            ELF::drawConcertinaPanelHeader (g, area, isMouseOver, isMouseDown, panel, comp);
            g.setColour (Colours::white);
            Rectangle<int> r (area.withTrimmedLeft (20));
            g.drawText (comp.getName(), 20, 0, r.getWidth(), r.getHeight(),
                        Justification::centredLeft);
        }
    } lookAndFeel;
};
}