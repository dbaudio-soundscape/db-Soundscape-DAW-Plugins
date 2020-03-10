/*
===============================================================================

Copyright (C) 2019 d&b audiotechnik GmbH & Co. KG. All Rights Reserved.

This file is part of the Soundscape VST, AU, and AAX Plug-in.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. The name of the author may not be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY d&b audiotechnik GmbH & Co. KG "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

===============================================================================
*/


#include "Overview.h"
#include "PluginProcessor.h"
#include "Controller.h"
#include "SurfaceSlider.h"


namespace dbaudio
{


/**
 * Rate at which the Overview GUI will refresh, when the multi-slider tab is selected.
 */
static constexpr int GUI_UPDATE_RATE_FAST = 75;

/**
 * Rate at which the Overview GUI will refresh.
 */
static constexpr int GUI_UPDATE_RATE_SLOW = 120;


/*
===============================================================================
 Class COverviewManager
===============================================================================
*/

/**
 * The one and only instance of COverviewManager.
 */
COverviewManager* COverviewManager::m_singleton = nullptr;

/**
 * Class constructor.
 */
COverviewManager::COverviewManager()
{
	jassert(!m_singleton);	// only one instnce allowed!!
	m_singleton = this;

	//Default overview window properties.
	m_overview = nullptr;
	m_overviewBounds = Rectangle<int>(50, 50, 500, 500);
}

/**
 * Destroys the COverviewManager.
 */
COverviewManager::~COverviewManager()
{
	jassert(m_overview == nullptr);

	m_singleton = nullptr;
}

/**
 * Returns the one and only instance of COverviewManager.
 * @return A COverviewManager object or 0.
 * @sa m_singleton, COverviewManager
 */
COverviewManager* COverviewManager::GetInstance()
{
	if (m_singleton == nullptr)
	{
		m_singleton = new COverviewManager();
	}
	return m_singleton;
}

/**
 * Function called when the "Overview" button on the GUI is clicked.
 */
void COverviewManager::OpenOverview()
{
	// Overview window is not currently open -> create it.
	if (m_overview == nullptr)
	{
		m_overview = new COverview();
		m_overview->setBounds(m_overviewBounds);
		m_overview->setResizeLimits(410, 370, 1920, 1080);
		m_overview->setResizable(true, false);
		m_overview->setUsingNativeTitleBar(true);
		m_overview->setVisible(true);
	}

	// Overview window already exists -> bring it to the front.
	else
	{
		m_overview->toFront(true);
	}
}

/**
 * Function called by COverview's destructor to set the local pointer to zero.
 * @param destroy	True to also destroy the COverviewManager itself.
 */
void COverviewManager::CloseOverview(bool destroy)
{
	if (m_overview != nullptr)
	{
		SaveLastOverviewBounds(GetOverviewBounds());

		// Close the overview window.
		delete m_overview;
		m_overview = nullptr;
	}

	// Closed overview, so manager no longer needed.
	if (destroy)
		delete this;
}

/**
 * If the Overview window is currently closed (m_overview points to zero), return it's
 * last known position and size. If Overview is open, get it's current pos and size.
 * @return	A rectangle defining the window's position and size.
 */
Rectangle<int> COverviewManager::GetOverviewBounds() const
{
	if (m_overview)
	{
		return Rectangle<int>(	m_overview->getScreenPosition().getX(),
								m_overview->getScreenPosition().getY(),
								m_overview->getLocalBounds().getWidth(),
								m_overview->getLocalBounds().getHeight());
	}
	return m_overviewBounds;
}

/**
 * Save the Overview window's position and size for later use.
 * @param bounds	A rectangle defining the window's position and size.
 */
void COverviewManager::SaveLastOverviewBounds(Rectangle<int> bounds)
{
	if (!bounds.isEmpty())
		m_overviewBounds = bounds;
}

/**
 * Get the currently active tab within the overview window.
 * @return The currently active tab.
 */
int COverviewManager::GetActiveTab() const
{
	return m_selectedTab;
}

/**
 * Set the currently active tab within the overview window.
 * @param tabIdx	The currently active tab index.
 */
void COverviewManager::SetActiveTab(int tabIdx)
{
	m_selectedTab = tabIdx;
}

/**
 * Get the currently selected coordinate mapping used for the multi-slider.
 * @return The selected mapping area.
 */
int COverviewManager::GetSelectedMapping() const
{
	return m_selectedMapping;
}

/**
 * Set the currently selected coordinate mapping used for the multi-slider.
 * @param mapping	The new selected mapping area.
 */
void COverviewManager::SetSelectedMapping(int mapping)
{
	m_selectedMapping = mapping;
}


/*
===============================================================================
 Class COverview
===============================================================================
*/

/**
 * Class constructor.
 */
COverview::COverview() :
	DocumentWindow("Overview",
	Colours::black,
	DocumentWindow::allButtons,
	true)
{
	// Set the proper plugin name on the Overview window's title bar.
	CController* ctrl = CController::GetInstance();
	if (ctrl)
	{
		String pluginName = ctrl->GetProcessor(0)->getName();
		setName(pluginName + String(" Overview"));
	}

	// DocumentWindow can only have one child component set via setContentOwned,
	// so we use a dummy content component and all all controls to it.
	m_contentComponent = std::make_unique<COverviewComponent>();

	// Component resizes automatically anyway, but need size > 0 to prevent 
	// stupid juce assert.
	m_contentComponent->setBounds(Rectangle<int>(1, 1));

	// The Overview window now takes ownership of the content component and resizes it too.
	setContentOwned(m_contentComponent.get(), true);
}

/**
 * Class destructor.
 */
COverview::~COverview()
{
}

/**
 * Reimplemented to simply destroy the overview window when the close button is pressed.
 */
void COverview::closeButtonPressed()
{
	COverviewManager* ovrMgr = COverviewManager::GetInstance();
	if (ovrMgr)
	{
		ovrMgr->CloseOverview(false);
	}
}


/*
===============================================================================
 Class COverviewComponent
===============================================================================
*/

/**
 * Class constructor.
 */
COverviewComponent::COverviewComponent()
{
	// IP Settings
	m_ipAddressTextEdit = std::make_unique<CTextEditor>("IP Address");
	m_ipAddressTextEdit->addListener(this);
	addAndMakeVisible(m_ipAddressTextEdit.get());
	m_ipAddressLabel = std::make_unique<CLabel>("IP Address Label", "IP Address:");
	addAndMakeVisible(m_ipAddressLabel.get());

	// Online
	m_onlineLed = std::make_unique<CButton>("");
	m_onlineLed->setEnabled(false);
	m_onlineLed->SetCornerRadius(10);
	addAndMakeVisible(m_onlineLed.get());

	// Interval
	m_rateTextEdit = std::make_unique<CTextEditor>("OSC Send Rate");
	m_rateTextEdit->SetSuffix("ms");
	m_rateTextEdit->addListener(this);
	addAndMakeVisible(m_rateTextEdit.get());
	m_rateLabel = std::make_unique<CLabel>("OSC Send Rate", "Interval:");
	addAndMakeVisible(m_rateLabel.get());

	// d&b logo and Plugin version label
	m_dbLogo = ImageCache::getFromMemory(BinaryData::logo_dbaudio_15x15_png, BinaryData::logo_dbaudio_15x15_pngSize);
	m_versionLabel = std::make_unique<CLabel>("PluginVersion", String(JUCE_STRINGIFY(JUCE_APP_VERSION)));
	m_versionLabel->setFont(Font(11));
	addAndMakeVisible(m_versionLabel.get());
	m_nameLabel = std::make_unique<CLabel>("PluginName", "Soundscape");
	m_nameLabel->setFont(Font(11));
	m_nameLabel->setColour(Label::textColourId, CDbStyle::GetDbColor(CDbStyle::DarkTextColor));
	addAndMakeVisible(m_nameLabel.get());

	m_titleLabel = std::make_unique<CLabel>("Title", "Overview");
	addAndMakeVisible(m_titleLabel.get());

	// Create the table container.
	m_tableContainer = std::make_unique<COverviewTableContainer>();
	m_multiSliderContainer = std::make_unique<COverviewMultiSurface>();

	// Create a tab container, where the COverviewTableContainer will be one of the tabs.
	m_tabbedComponent = std::make_unique<CTabbedComponent>();
	m_tabbedComponent->setTabBarDepth(44);
	m_tabbedComponent->setOutline(0);
	m_tabbedComponent->setIndent(0);
	addAndMakeVisible(m_tabbedComponent.get());

	// Add the overview tabs.
	m_tabbedComponent->addTab("Table", CDbStyle::GetDbColor(CDbStyle::DarkColor), m_tableContainer.get(), false);
	m_tabbedComponent->addTab("Slider", CDbStyle::GetDbColor(CDbStyle::DarkColor), m_multiSliderContainer.get(), false);

	// Remember which tab was active before the last time the overview was closed.
	COverviewManager* ovrMgr = COverviewManager::GetInstance();
	if (ovrMgr)
		m_tabbedComponent->setCurrentTabIndex(ovrMgr->GetActiveTab());

	// Start GUI-refreshing timer.
	startTimer(GUI_UPDATE_RATE_SLOW);
}

/**
 * Class destructor.
 */
COverviewComponent::~COverviewComponent()
{
	// Remember which tab was active before the last time the overview was closed.
	COverviewManager* ovrMgr = COverviewManager::GetInstance();
	if (ovrMgr && m_tabbedComponent)
		ovrMgr->SetActiveTab(m_tabbedComponent->getCurrentTabIndex());
}

/**
 * Reimplemented to paint background and logo.
 * @param g		Graphics context that must be used to do the drawing operations.
 */
void COverviewComponent::paint(Graphics& g)
{
	int w = getLocalBounds().getWidth();
	int h = getLocalBounds().getHeight();	

	// Bars above and below
	g.setColour(CDbStyle::GetDbColor(CDbStyle::MidColor));
	g.fillRect(getLocalBounds());

	// Background
	g.setColour(CDbStyle::GetDbColor(CDbStyle::DarkColor));
	g.fillRect(Rectangle<int>(0, 43, w, h - 87));

	// Little lines between version and logo
	g.setColour(CDbStyle::GetDbColor(CDbStyle::ButtonColor));
	g.fillRect(Rectangle<int>(w - 35, 6, 1, 30));
	g.fillRect(Rectangle<int>(w - 102, 6, 1, 30));

	// Add d&b logo 
	g.drawImage(m_dbLogo, getLocalBounds().getWidth() - 25, 15, 15, 15, 0, 0, 15, 15);

	// Draw little line below "Overview" to match with the line which is automatically drawn 
	// by the CTabbedComponent's CTabBarButton.
	g.setColour(Colour(108, 113, 115));
	g.drawRect(Rectangle<int>(0, 43, 100, 1), 1);
}

/**
 * Reimplemented to resize and re-postion controls on the overview window.
 */
void COverviewComponent::resized()
{
	int w = getLocalBounds().getWidth();
	int vStartPos2 = getLocalBounds().getHeight() - 35;

	// Ip Address
	m_ipAddressLabel->setBounds(Rectangle<int>(5, vStartPos2, 75, 25));
	m_ipAddressTextEdit->setBounds(Rectangle<int>(80, vStartPos2, 140, 25));

	// Rate
	m_rateLabel->setBounds(Rectangle<int>(233, vStartPos2, 65, 25));
	m_rateTextEdit->setBounds(Rectangle<int>(296, vStartPos2, 50, 25));

	// Online
	m_onlineLed->setBounds(Rectangle<int>(w - 40, vStartPos2, 24, 24));

	// Name and Version label
	m_nameLabel->setBounds(w - 105, 3, 75, 25);
	m_versionLabel->setBounds(w - 103, 21, 42, 15);

	// Title label
	m_titleLabel->setBounds(Rectangle<int>(5, 10, 80, 25));

	// Tab container takes up the entire window minus the bottom bar (with the IP etc).
	// See CTabbedComponent::resized().
	m_tabbedComponent->setBounds(Rectangle<int>(0, 0, w, getLocalBounds().getHeight() - 45));

	// Resize overview table container.
	m_tableContainer->setBounds(Rectangle<int>(0, 44, w, getLocalBounds().getHeight() - 89));
	m_multiSliderContainer->setBounds(Rectangle<int>(0, 44, w, getLocalBounds().getHeight() - 89));
}

/**
 * Callback function for changes to our textEditors.
 * @param textEditor	The TextEditor object whose content has just changed.
 */
void COverviewComponent::textEditorFocusLost(TextEditor& textEditor)
{
	CTextEditor *myEditor = static_cast<CTextEditor*>(&textEditor);
	CController* ctrl = CController::GetInstance();
	if (ctrl && myEditor)
	{
		// IP Address changed
		if (myEditor == m_ipAddressTextEdit.get())
		{
			// IP address validation: If IPAddress::toString() returns the same string
			// which was entered, it is a valid IP address.
			IPAddress ip(myEditor->getText());
			if (ip.toString() == myEditor->getText())
				ctrl->SetIpAddress(DCS_Overview, myEditor->getText());
			else
				myEditor->setText(ctrl->GetIpAddress(), false);
		}

		// OSC message rate has changed
		else if (myEditor == m_rateTextEdit.get())
		{
			ctrl->SetRate(DCS_Overview, myEditor->getText().getIntValue());
		}
	}
}

/**
 * Callback function for Enter key presses on textEditors.
 * @param textEditor	The TextEditor object whose where enter key was pressed.
 */
void COverviewComponent::textEditorReturnKeyPressed(TextEditor& textEditor)
{
	ignoreUnused(textEditor);

	// Remove keyboard focus from this editor.
	// Function textEditorFocusLost will then take care of setting values.
	getParentComponent()->grabKeyboardFocus();
}

/**
 * Timer callback function, which will be called at regular intervals to update the GUI.
 * Reimplemented from base class Timer.
 */
void COverviewComponent::timerCallback()
{
	UpdateGui(false);
}

/**
 * Update GUI elements with the current parameter values.
 * @param init	True to ignore any changed flags and update the OSC config parameters 
 *				in the GUI anyway. Good for when opening the Overview for the first time.
 */
void COverviewComponent::UpdateGui(bool init)
{
	CController* ctrl = CController::GetInstance();
	if (ctrl)
	{
		if (ctrl->PopParameterChanged(DCS_Overview, DCT_IPAddress) || init)
			m_ipAddressTextEdit->setText(ctrl->GetIpAddress(), false);

		if (ctrl->PopParameterChanged(DCS_Overview, DCT_MessageRate) || init)
			m_rateTextEdit->setText(String(ctrl->GetRate()), false);

		if (ctrl->PopParameterChanged(DCS_Overview, DCT_Online) || init)
			m_onlineLed->setToggleState(ctrl->GetOnline(), NotificationType::dontSendNotification);
	}

	// Save some performance: only update the component inside the currently active tab.
	if (m_tabbedComponent->getCurrentTabIndex() == CTabbedComponent::OTI_Table)
	{
		if (m_tableContainer)
			m_tableContainer->UpdateGui(init);

		// When the overview table is active, no need to refresh GUI very quickly
		if (getTimerInterval() == GUI_UPDATE_RATE_FAST)
		{
			//DBG("COverviewComponent::timerCallback(): Switching to GUI_UPDATE_RATE_SLOW");
			startTimer(GUI_UPDATE_RATE_SLOW);
		}
	}
	else if (m_tabbedComponent->getCurrentTabIndex() == CTabbedComponent::OTI_MultiSlider)
	{
		if (m_multiSliderContainer)
			m_multiSliderContainer->UpdateGui(init);

		// When multi-slider is active, we refresh the GUI faster
		if (getTimerInterval() == GUI_UPDATE_RATE_SLOW)
		{
			//DBG("COverviewComponent::timerCallback: Switching to GUI_UPDATE_RATE_FAST");
			startTimer(GUI_UPDATE_RATE_FAST);
		}
	}
}


/*
===============================================================================
 Class CTabbedComponent
===============================================================================
*/

/**
 * Class constructor.
 */
CTabbedComponent::CTabbedComponent()
	: TabbedComponent(TabbedButtonBar::TabsAtTop)
{
}

/**
 * Class destructor.
 */
CTabbedComponent::~CTabbedComponent()
{
}

/**
 * Reimplemented to create and return custom tab bar buttons.
 * @param tabName	Text on the tab button. Not used in this implementation.
 * @param tabIndex	Index of the tab from left to right, starting at 0.
 * @return	Pointer to a CTabBarButton.
 */
TabBarButton* CTabbedComponent::createTabButton(const String& tabName, int tabIndex)
{
	ignoreUnused(tabName);
	return new CTabBarButton(tabIndex, getTabbedButtonBar());
}

/**
 * Callback method to indicate the selected tab has been changed. 
 * Reimplemented to trigger a GUI update on the newly active tab.
 * @param newCurrentTabIndex	Index of the tab from left to right, starting at 0.
 * @param newCurrentTabName		Name of the tab.
 * @return	Pointer to a CTabBarButton.
 */
void CTabbedComponent::currentTabChanged(int newCurrentTabIndex, const String &newCurrentTabName)
{
	ignoreUnused(newCurrentTabIndex);
	ignoreUnused(newCurrentTabName);
	
	COverviewComponent* parent = dynamic_cast<COverviewComponent*>(getParentComponent());
	if (parent)
		parent->UpdateGui(true);
}

/**
 * Reimplemented to re-postion the tabBar so make the tab buttons start further to the right.
 */
void CTabbedComponent::resized()
{
	int w = getLocalBounds().getWidth();
	getTabbedButtonBar().setBounds(Rectangle<int>(90, 0, w - 90, 44));
}

/**
 * Static method to provide a path, which can then be used to draw the corresponding icon on top of a button or tab.
 * @param tabIdx	Entry in enum OverviewTabIndex which determines which icon path is returned. 
 * @param iconSize	Absolute width and height which the icon should be bound to.
 * @param strokeThickness	For nicer icons, this should be the same thickness later used in Graphics::strokePath(..).
 * @param path		The resulting path.
 * OverviewTabIndex
 */
void CTabbedComponent::GetIconPath(int tabIdx, Point<float> iconSize, float strokeThickness, Path& path)
{
	switch (tabIdx)
	{
	case CTabbedComponent::OTI_Table: // Overview table COverviewTableContainer
		path.addRectangle(0, 0, iconSize.x, iconSize.y);
		path.addRectangle(0, strokeThickness, iconSize.x, strokeThickness);
		break;

	case CTabbedComponent::OTI_MultiSlider: // Multi-slider COverviewMultiSurface
		path.addEllipse(0, 0, strokeThickness, strokeThickness);
		path.addEllipse(iconSize.x - strokeThickness, 0, strokeThickness, strokeThickness);
		path.addEllipse(0, iconSize.y - strokeThickness, strokeThickness, strokeThickness);
		path.addEllipse(iconSize.x - strokeThickness, iconSize.y - strokeThickness, strokeThickness, strokeThickness);
		path.addEllipse((iconSize.x - strokeThickness) / 2, (iconSize.y - strokeThickness) / 2, strokeThickness, strokeThickness);
		break;

	default:
		jassertfalse; // missing implementation!
		break;
	}
}


/*
===============================================================================
 Class CTabBarButton
===============================================================================
*/

/**
 * Class constructor.
 * @param tabIdx	Tab index starting at 0.
 * @param ownerBar	TabbedButtonBar object which contains this button.
 */
CTabBarButton::CTabBarButton(int tabIdx, TabbedButtonBar& ownerBar)
	: TabBarButton(String(), ownerBar),
	m_tabIndex(tabIdx)
{

}

/**
 * Class destructor.
 */
CTabBarButton::~CTabBarButton()
{
}

/**
 * Reimplemented paint function, to display an icon.
 * @param g					A graphics context, used for drawing a component or image.
 * @param isMouseOverButton	True if the mouse is over the button.
 * @param isButtonDown		True if the button is being pressed.
 */
void CTabBarButton::paintButton(Graphics& g, bool isMouseOverButton, bool isButtonDown)
{
	// The original TabBarButton::paintButton draws a gradient on the buttons which
	// are inactive. We don't want that, just paint them with the background color.
	Colour buttonBackground(CDbStyle::GetDbColor(CDbStyle::MidColor));
	if (getToggleState())
		buttonBackground = CDbStyle::GetDbColor(CDbStyle::DarkColor);
	else if (isButtonDown)
		buttonBackground = buttonBackground.brighter(0.1f);
	else if (isMouseOverButton)
		buttonBackground = buttonBackground.brighter(0.05f);

	Rectangle<int> activeArea(getActiveArea());
	g.setColour(buttonBackground);
	g.fillRect(getActiveArea());

	// Get the icon path for this tab.
	Path iconPath;
	float thickness = 2.0f;
	CTabbedComponent::GetIconPath(m_tabIndex, Point<float>(22.0f, 16.0f), thickness, iconPath);

	// Move path so that is centered within the button's activeArea.
	float xOffset = (activeArea.getWidth() / 2) - (iconPath.getBounds().getWidth() / 2);
	float yOffset = (activeArea.getHeight() / 2) - (iconPath.getBounds().getHeight() / 2);
	iconPath.applyTransform(AffineTransform::translation(xOffset, yOffset));

	// Draw icon path.
	g.setColour(CDbStyle::GetDbColor(CDbStyle::LightColor));
	g.strokePath(iconPath, PathStrokeType(thickness, PathStrokeType::curved, PathStrokeType::rounded));
}


/*
===============================================================================
 Class COverviewTableContainer
===============================================================================
*/

/**
 * Class constructor.
 */
COverviewTableContainer::COverviewTableContainer()
	: AOverlay(OT_Overview)
{
	// Create the table model/component.
	m_overviewTable = std::make_unique<CTableModelComponent>();
	addAndMakeVisible(m_overviewTable.get());

	// Create quick selection buttons
	m_selectLabel = std::make_unique<CLabel>("Select:", "Select:");
	addAndMakeVisible(m_selectLabel.get());

	m_selectAll = std::make_unique<CButton>("All");
	m_selectAll->setEnabled(true);
	m_selectAll->addListener(this);
	addAndMakeVisible(m_selectAll.get());

	m_selectNone = std::make_unique<CButton>("None");
	m_selectNone->setEnabled(true);
	m_selectNone->addListener(this);
	addAndMakeVisible(m_selectNone.get());
}

/**
 * Class destructor.
 */
COverviewTableContainer::~COverviewTableContainer()
{
}

/**
 * Reimplemented to paint background and frame.
 * @param g		Graphics context that must be used to do the drawing operations.
 */
void COverviewTableContainer::paint(Graphics& g)
{
	int w = getLocalBounds().getWidth();
	int h = getLocalBounds().getHeight();	

	// Background
	g.setColour(CDbStyle::GetDbColor(CDbStyle::MidColor));
	g.fillRect(Rectangle<int>(8, h - 41, w - 16, 34));

	// Frame
	g.setColour(CDbStyle::GetDbColor(CDbStyle::DarkLineColor));
	g.drawRect(Rectangle<int>(8, h - 41, w - 16, 34), 1);
}

/**
 * Reimplemented to resize and re-postion controls on the overview window.
 */
void COverviewTableContainer::resized()
{
	int w = getLocalBounds().getWidth();

	// Resize overview table.
	m_overviewTable->setBounds(Rectangle<int>(0, 0, w, getLocalBounds().getHeight() - 32));

	// Resize quick selection buttons
	m_selectLabel->setBounds(Rectangle<int>(w - 170, getLocalBounds().getHeight() - 40, 80, 30));
	m_selectAll->setBounds(Rectangle<int>(w - 106, getLocalBounds().getHeight() - 38, 40, 26));
	m_selectNone->setBounds(Rectangle<int>(w - 65, getLocalBounds().getHeight() - 38, 46, 26));
}

/**
 * Reimplemented from Button::Listener, gets called whenever the buttons are clicked.
 * @param button	The button which has been clicked.
 */
void COverviewTableContainer::buttonClicked(Button *button)
{
	if ((button == m_selectAll.get()) || (button == m_selectNone.get()))
	{
		// Send true to select all rows, false to deselect all.
		m_overviewTable->SelectAllRows(button == m_selectAll.get());

		// Un-toggle button.			
		button->setToggleState(false, NotificationType::dontSendNotification);
	}
}

/**
 * If any relevant parameters have been marked as changed, update the table contents.
 * @param init	True to ignore any changed flags and update the plugin parameters
 *				in the GUI anyway. Good for when opening the Overview for the first time.
 */
void COverviewTableContainer::UpdateGui(bool init)
{
	CController* ctrl = CController::GetInstance();
	if (ctrl && m_overviewTable)
	{
		if (ctrl->PopParameterChanged(DCS_Overview, DCT_NumPlugins) || init)
		{
			m_overviewTable->RecreateTableRowIds();
			m_overviewTable->UpdateTable();
		}

		else
		{
			// Iterate through all plugin instances and see if anything changed there.
			for (int pIdx = 0; pIdx < ctrl->GetProcessorCount(); pIdx++)
			{
				CPlugin* plugin = ctrl->GetProcessor(pIdx);
				if (plugin && plugin->PopParameterChanged(DCS_Overview, DCT_PluginInstanceConfig))
				{
					m_overviewTable->UpdateTable();
				}
			}
		}
	}
}


/*
===============================================================================
 Class COverviewMultiSurface
===============================================================================
*/

/**
 * Class constructor.
 */
COverviewMultiSurface::COverviewMultiSurface()
	: AOverlay(OT_MultiSlide)
{
	// Add multi-slider
	m_multiSlider = std::make_unique<CSurfaceMultiSlider>();
	addAndMakeVisible(m_multiSlider.get());

	// Add mapping label
	m_posAreaLabel = std::make_unique<CLabel>("Coordinate mapping label", "View mapping:");
	addAndMakeVisible(m_posAreaLabel.get());

	// Add mapping selector
	m_areaSelector = std::make_unique<ComboBox>("Coordinate mapping");
	m_areaSelector->setEditableText(false);
	m_areaSelector->addItem("1", 1);
	m_areaSelector->addItem("2", 2);
	m_areaSelector->addItem("3", 3);
	m_areaSelector->addItem("4", 4);
	m_areaSelector->addListener(this);
	m_areaSelector->setColour(ComboBox::backgroundColourId, CDbStyle::GetDbColor(CDbStyle::DarkColor));
	m_areaSelector->setColour(ComboBox::textColourId, CDbStyle::GetDbColor(CDbStyle::TextColor));
	m_areaSelector->setColour(ComboBox::outlineColourId, CDbStyle::GetDbColor(CDbStyle::WindowColor));
	m_areaSelector->setColour(ComboBox::buttonColourId, CDbStyle::GetDbColor(CDbStyle::MidColor));
	m_areaSelector->setColour(ComboBox::arrowColourId, CDbStyle::GetDbColor(CDbStyle::TextColor));
	addAndMakeVisible(m_areaSelector.get());
}

/**
 * Class destructor.
 */
COverviewMultiSurface::~COverviewMultiSurface()
{
}

/**
 * Reimplemented to paint background.
 * @param g		Graphics context that must be used to do the drawing operations.
 */
void COverviewMultiSurface::paint(Graphics& g)
{
	// Paint background to cover the controls behind this overlay.
	g.setColour(CDbStyle::GetDbColor(CDbStyle::DarkColor));
	g.fillRect(Rectangle<int>(0, 0, getLocalBounds().getWidth(), getLocalBounds().getHeight()));
}

/**
 * Reimplemented to resize and re-postion controls on the overview window.
 */
void COverviewMultiSurface::resized()
{
	// Resize multi-slider.
	CController* ctrl = CController::GetInstance();
	if (ctrl)
	{
		CPlugin* plugin = ctrl->GetProcessor(0);
		if (plugin && plugin->IsTargetHostAvidConsole())
		{
			// For consoles, keep aspect ratio of slider at 1:1, as plugin window usually cannot be resized.
			int side = jmin<int>((getLocalBounds().getWidth() - 40), (getLocalBounds().getHeight() - 52));
			m_multiSlider->setBounds((getLocalBounds().getWidth() / 2) - (side / 2), 10, side, side);
		}
		else
			m_multiSlider->setBounds(Rectangle<int>(20, 10, getLocalBounds().getWidth() - 40, getLocalBounds().getHeight() - 52));
	}

	// Mapping selector
	m_posAreaLabel->setBounds(Rectangle<int>(70, getLocalBounds().getHeight() - 32, 100, 25));
	m_areaSelector->setBounds(Rectangle<int>(170, getLocalBounds().getHeight() - 32, 50, 25));
}

/**
 * If any relevant parameters have been marked as changed, update the table contents.
 * @param init	True to ignore any changed flags and update the plugin parameters
 *				in the GUI anyway. Good for when opening the Overview for the first time.
 */
void COverviewMultiSurface::UpdateGui(bool init)
{
	// Will be set to true if any changes relevant to the multi-slider are found.
	bool update = init;

	// Update the selected mapping area.
	int selectedMapping = 0;
	COverviewManager* ovrMgr = COverviewManager::GetInstance();
	if (ovrMgr)
	{
		selectedMapping = ovrMgr->GetSelectedMapping();
		if (selectedMapping != m_areaSelector->getSelectedId())
		{
			m_areaSelector->setSelectedId(selectedMapping, dontSendNotification);
			update = true;
		}
	}

	CController* ctrl = CController::GetInstance();
	if (ctrl && m_multiSlider)
	{
		if (ctrl->PopParameterChanged(DCS_Overview, DCT_NumPlugins))
			update = true;
		
		// Iterate through all plugin instances and see if anything changed there.
		// At the same time collect all sources positions for updating.
		CSurfaceMultiSlider::PositionCache cachedPositions;
		for (int pIdx = 0; pIdx < ctrl->GetProcessorCount(); pIdx++)
		{
			CPlugin* plugin = ctrl->GetProcessor(pIdx);
			if (plugin)
			{
				if (plugin->GetMappingId() == selectedMapping)
				{
					// NOTE: only sources are included, which match the selected viewing mapping.
					Point<float> p(plugin->GetParameterValue(ParamIdx_X), plugin->GetParameterValue(ParamIdx_Y));
					cachedPositions.insert(std::make_pair(pIdx, std::make_pair(plugin->GetSourceId(), p)));
				}

				if (plugin->PopParameterChanged(DCS_Overview, (DCT_PluginInstanceConfig | DCT_SourcePosition)))
					update = true;
			}
		}

		CSurfaceMultiSlider* multiSlider = dynamic_cast<CSurfaceMultiSlider*>(m_multiSlider.get());
		if (update && multiSlider)
		{
			// Update all nipple positions on the 2D-Slider.
			multiSlider->UpdatePositions(cachedPositions);
			multiSlider->repaint();
		}
	}
}

/**
 * Called when a ComboBox has its selected item changed. 
 * @param comboBox	The combo box which has changed.
 */
void COverviewMultiSurface::comboBoxChanged(ComboBox *comboBox)
{
	COverviewManager* ovrMgr = COverviewManager::GetInstance();
	if (ovrMgr)
	{
		if (ovrMgr->GetSelectedMapping() != comboBox->getSelectedId())
		{
			ovrMgr->SetSelectedMapping(comboBox->getSelectedId());

			// Trigger an update on the multi-slider, so that only sources with the
			// selected mapping are visible.
			UpdateGui(true);
		}
	}
}


/*
===============================================================================
 Class CTableModelComponent
===============================================================================
*/

/**
 * Class constructor.
 */
CTableModelComponent::CTableModelComponent()
{
	// This fills m_ids.
	RecreateTableRowIds();

	// Create our table component and add it to this component..
	addAndMakeVisible(m_table);
	m_table.setModel(this);

	// Add columns to the table header
	int tableHeaderFlags = (TableHeaderComponent::visible | TableHeaderComponent::sortable);
	m_table.getHeader().addColumn("Track", OC_TrackID, 50, 30, -1, tableHeaderFlags);
	m_table.getHeader().addColumn("Input", OC_SourceID, 50, 30, -1, tableHeaderFlags);
	m_table.getHeader().addColumn("Mapping", OC_Mapping, 50, 30, -1, tableHeaderFlags);
	m_table.getHeader().addColumn("Mode", OC_ComsMode, 50, 30, -1, tableHeaderFlags);
	m_table.getHeader().setSortColumnId(OC_SourceID, true); // sort forwards by the Input number column
	m_table.getHeader().setStretchToFitActive(true);

	// Header colors
	m_table.getHeader().setColour(TableHeaderComponent::textColourId, CDbStyle::GetDbColor(CDbStyle::TextColor));
	m_table.getHeader().setColour(TableHeaderComponent::backgroundColourId, CDbStyle::GetDbColor(CDbStyle::MidColor));
	m_table.getHeader().setColour(TableHeaderComponent::outlineColourId, CDbStyle::GetDbColor(CDbStyle::DarkLineColor));
	m_table.getHeader().setColour(TableHeaderComponent::highlightColourId, CDbStyle::GetDbColor(CDbStyle::HighlightColor));

	// Scroll bar colors
	m_table.getVerticalScrollBar().setColour(ScrollBar::backgroundColourId, CDbStyle::GetDbColor(CDbStyle::MidColor));
	m_table.getVerticalScrollBar().setColour(ScrollBar::thumbColourId, CDbStyle::GetDbColor(CDbStyle::DarkTextColor));
	m_table.getVerticalScrollBar().setColour(ScrollBar::trackColourId, CDbStyle::GetDbColor(CDbStyle::MidColor));

	// Table colors
	m_table.setColour(TableListBox::backgroundColourId, CDbStyle::GetDbColor(CDbStyle::DarkColor));
	m_table.setColour(TableListBox::outlineColourId, CDbStyle::GetDbColor(CDbStyle::DarkLineColor));
	m_table.setColour(TableListBox::textColourId, CDbStyle::GetDbColor(CDbStyle::TextColor));

	m_table.setRowHeight(33);
	m_table.setOutlineThickness(1);
	m_table.setClickingTogglesRowSelection(false);
	m_table.setMultipleSelectionEnabled(true);
}

/**
 * Class destructor.
 */
CTableModelComponent::~CTableModelComponent()
{
}

/**
 * Get the ID of the plugin instance corresponding to the given table row number.
 * @param rowNumber	The desired row number (starts at 0).
 * @return	The ID of the plugin instance at that row number, if any.
 */
PluginId CTableModelComponent::GetPluginIdForRow(int rowNumber)
{
	if ((unsigned int)rowNumber > (m_ids.size() - 1))
	{
		jassertfalse; // Unexpected row number!
		return 0;
	}

	return m_ids.at(rowNumber);
}

/**
 * Get the IDs of the plugin instances corresponding to the given table row numbers.
 * @param rowNumbers	A list of desired row numbers.
 * @return	A list of IDs of the plugin instances at those rows.
 */
std::vector<PluginId> CTableModelComponent::GetPluginIdsForRows(std::vector<int> rowNumbers)
{
	std::vector<PluginId> ids;
	ids.reserve(rowNumbers.size());
	for (std::size_t i = 0; i < rowNumbers.size(); ++i)
		ids.push_back(GetPluginIdForRow(rowNumbers[i]));

	return ids;
}

/**
 * Get the list of rows which are currently selected on the table.
 * @return	A std::vector containing all selected row numbers.
 */
std::vector<int> CTableModelComponent::GetSelectedRows() const
{
	std::vector<int> selectedRows;
	selectedRows.reserve(m_table.getSelectedRows().size());
	for (int i = 0; i < m_table.getSelectedRows().size(); ++i)
		selectedRows.push_back(m_table.getSelectedRows()[i]);

	return selectedRows;
}

/**
 * Select all (or none) of the rows on the table.
 * @param all	True to select all rows. False to de-select all (clear selection).
 */
void CTableModelComponent::SelectAllRows(bool all)
{
	if (all)
		m_table.selectRangeOfRows(0, m_table.getNumRows(), true /* Do not scroll */);
	else
		m_table.deselectAllRows();
}

/**
 * Helper sorting function used by std::sort(). This version is used to sort by plugin's SourceId.
 * @param pId1	Id of the first plugin processor.
 * @param pId2	Id of the second plugin processor.
 * @return	True if the first plugin's SourceId is less than the second's.
 */
bool CTableModelComponent::LessThanSourceId(PluginId pId1, PluginId pId2)
{
	CController* ctrl = CController::GetInstance();
	if (ctrl)
	{
		if ((pId1 < (PluginId)ctrl->GetProcessorCount()) && (pId2 < (PluginId)ctrl->GetProcessorCount()))
			return (ctrl->GetProcessor(pId1)->GetSourceId() < ctrl->GetProcessor(pId2)->GetSourceId());
	}

	jassertfalse; // Index out of range!
	return false;
}

/**
 * Helper sorting function used by std::sort(). This version is used to sort by plugin's MappingId.
 * @param pId1	Id of the first plugin processor.
 * @param pId2	Id of the second plugin processor.
 * @return	True if the first plugin's MappingId is less than the second's.
 */
bool CTableModelComponent::LessThanMapping(PluginId pId1, PluginId pId2)
{
	CController* ctrl = CController::GetInstance();
	if (ctrl)
	{
		if ((pId1 < (PluginId)ctrl->GetProcessorCount()) && (pId2 < (PluginId)ctrl->GetProcessorCount()))
			return (ctrl->GetProcessor(pId1)->GetMappingId() < ctrl->GetProcessor(pId2)->GetMappingId());
	}

	jassertfalse; // Index out of range!
	return false;
}

/**
 * Helper sorting function used by std::sort(). This version is used to sort by plugin's ComsMode. 
 * @param pId1	Id of the first plugin processor.
 * @param pId2	Id of the second plugin processor.
 * @return	True if the first plugin's ComsMode is less than the second's.
 */
bool CTableModelComponent::LessThanComsMode(PluginId pId1, PluginId pId2)
{
	CController* ctrl = CController::GetInstance();
	if (ctrl)
	{
		if ((pId1 < (PluginId)ctrl->GetProcessorCount()) && (pId2 < (PluginId)ctrl->GetProcessorCount()))
			return (ctrl->GetProcessor(pId1)->GetComsMode() < ctrl->GetProcessor(pId2)->GetComsMode());
	}

	jassertfalse; // Index out of range!
	return false;
}

/**
 * This clears and re-fills m_ids.
 */
void CTableModelComponent::RecreateTableRowIds()
{
	m_ids.clear();
	CController* ctrl = CController::GetInstance();
	if (ctrl)
	{
		m_ids.reserve(ctrl->GetProcessorCount());
		for (int idx = 0; idx < ctrl->GetProcessorCount(); ++idx)
			m_ids.push_back(idx);
	}

	// Clear row selection, since rows may have changed.
	m_table.deselectAllRows();
}

/**
 * This refreshes the table contents.
 */
void CTableModelComponent::UpdateTable()
{
	// Re-sort table again depending on the currently selected column.
	sortOrderChanged(m_table.getHeader().getSortColumnId(), m_table.getHeader().isSortedForwards());

	// Refresh table
	m_table.updateContent();
}

/**
 * This can be overridden to react to the user double-clicking on a part of the list where there are no rows. 
 * @param event	Contains position and status information about a mouse event.
 */
void CTableModelComponent::backgroundClicked(const MouseEvent &event)
{
	// Clear selection
	m_table.deselectAllRows();

	// Base class implementation.
	TableListBoxModel::backgroundClicked(event);
}

/**
 * This is overloaded from TableListBoxModel, and must return the total number of rows in our table.
 * @return	Number of rows on the table, equal to number of plugin instances.
 */
int CTableModelComponent::getNumRows()
{
	int ret = 0;

	CController* ctrl = CController::GetInstance();
	if (ctrl)
		ret = ctrl->GetProcessorCount();

	return ret;
}

/**
 * This is overloaded from TableListBoxModel, and should fill in the background of the whole row.
 * @param g					Graphics context that must be used to do the drawing operations.
 * @param rowNumber			Number of row to paint.
 * @param width				Width of area to paint.
 * @param height			Height of area to paint.
 * @param rowIsSelected		True if row is currently selected.
 */
void CTableModelComponent::paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
	ignoreUnused(rowNumber);

	// Selected rows have a different background color.
	if (rowIsSelected)
		g.setColour(CDbStyle::GetDbColor(CDbStyle::HighlightColor));
	else
		g.setColour(CDbStyle::GetDbColor(CDbStyle::MidColor));
	g.fillRect(0, 0, width, height - 1);

	// Line between rows.
	g.setColour(CDbStyle::GetDbColor(CDbStyle::DarkLineColor));
	g.fillRect(0, height - 1, width, height - 1);
}

/**
 * This is overloaded from TableListBoxModel, and must paint any cells that aren't using custom components.
 * This reimplementation does nothing (all cells use custom components).
 * @param g					Graphics context that must be used to do the drawing operations.
 * @param rowNumber			Number of row to paint (starts at 0)
 * @param columnId			Number of column to paint (starts at 1).
 * @param width				Width of area to paint.
 * @param height			Height of area to paint.
 * @param rowIsSelected		True if row is currently selected.
 */
void CTableModelComponent::paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
	ignoreUnused(g);
	ignoreUnused(rowNumber);
	ignoreUnused(columnId);
	ignoreUnused(width);
	ignoreUnused(height);
	ignoreUnused(rowIsSelected);
}

/**
 * This is overloaded from TableListBoxModel, and tells us that the user has clicked a table header
 * to change the sort order.
 * @param newSortColumnId ID of the column selected for sorting.
 * @param isForwards True if sorting from smallest to largest.
 */
void CTableModelComponent::sortOrderChanged(int newSortColumnId, bool isForwards)
{
	// Remember row selection so it can be restored after sorting.
	std::vector<PluginId> selectedPlugins = GetPluginIdsForRows(GetSelectedRows());
	m_table.deselectAllRows();

	// Use a different helper sorting function depending on which column is selected for sorting.
	switch (newSortColumnId)
	{
	case OC_TrackID:
		std::sort(m_ids.begin(), m_ids.end());
		break;
	case OC_SourceID:
		std::sort(m_ids.begin(), m_ids.end(), CTableModelComponent::LessThanSourceId);
		break;
	case OC_Mapping:
		std::sort(m_ids.begin(), m_ids.end(), CTableModelComponent::LessThanMapping);
		break;
	case OC_ComsMode:
		std::sort(m_ids.begin(), m_ids.end(), CTableModelComponent::LessThanComsMode);
		break;
	default:
		break;
	}

	// If reverse order is selected, reverse the list.
	if (!isForwards)
		std::reverse(m_ids.begin(), m_ids.end());

	m_table.updateContent();

	// Restore row selection after sorting order has been changed, BUT make sure that
	// it is the same Plugins which are selected after the sorting, NOT the same rows.
	for (PluginId pId : selectedPlugins)
	{
		int rowNo = static_cast<int>(std::find(m_ids.begin(), m_ids.end(), pId) - m_ids.begin());
		m_table.selectRow(rowNo, true /* don't scroll */, false /* do not deselect other rows*/);
	}
}

/**
 * This is overloaded from TableListBoxModel, and must update any custom components that we're using.
 * @param rowNumber			Number of row on the table (starting at 0).
 * @param columnId			Number of column on the table (starting at 1).
 * @param isRowSelected		True if row is currently selected.
 * @param existingComponentToUpdate		Pointer to existing component for this cell. Null if no component exists yet.
 * @return	Pointer to component which should be used for this cell. Null if no component is necessary.
 */
Component* CTableModelComponent::refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate)
{
	ignoreUnused(isRowSelected);

	Component* ret = nullptr;

	switch (columnId)
	{
		case OC_TrackID:
		{
			CEditableLabelContainer* label = static_cast<CEditableLabelContainer*> (existingComponentToUpdate);

			// If an existing component is being passed-in for updating, we'll re-use it, but
			// if not, we'll have to create one.
			if (label == nullptr)
				label = new CEditableLabelContainer(*this);

			// Ensure that the component knows which row number it is located at.
			label->SetRow(rowNumber);

			// Return a pointer to the component.
			ret = label;
		}
		break;

	case OC_Mapping:
		{
			CComboBoxContainer* comboBox = static_cast<CComboBoxContainer*> (existingComponentToUpdate);

			// If an existing component is being passed-in for updating, we'll re-use it, but
			// if not, we'll have to create one.
			if (comboBox == nullptr)
				comboBox = new CComboBoxContainer(*this);

			// Ensure that the comboBox knows which row number it is located at.
			comboBox->SetRow(rowNumber);

			// Return a pointer to the comboBox.
			ret = comboBox;
		}
		break;
	case OC_SourceID:
		{
			CTextEditorContainer* textEdit = static_cast<CTextEditorContainer*> (existingComponentToUpdate);

			// If an existing component is being passed-in for updating, we'll re-use it, but
			// if not, we'll have to create one.
			if (textEdit == nullptr)
				textEdit = new CTextEditorContainer(*this);

			// Ensure that the component knows which row number it is located at.
			textEdit->SetRow(rowNumber);

			// Return a pointer to the component.
			ret = textEdit;
		}
		break;

	case OC_ComsMode:
		{
			CRadioButtonContainer* radioButton = static_cast<CRadioButtonContainer*> (existingComponentToUpdate);

			// If an existing component is being passed-in for updating, we'll re-use it, but
			// if not, we'll have to create one.
			if (radioButton == nullptr)
				radioButton = new CRadioButtonContainer(*this);

			// Ensure that the component knows which row number it is located at.
			radioButton->SetRow(rowNumber);

			// Return a pointer to the component.
			ret = radioButton;
		}
		break;

	default:
		jassert(existingComponentToUpdate == nullptr);
		break;
	}

	return ret;
}

/**
 *  This is overloaded from TableListBoxModel, and should choose the best width for the specified column.
 * @param columnId	Desired column ID.
 * @return	Width to be used for the desired column.
 */
int CTableModelComponent::getColumnAutoSizeWidth(int columnId)
{
	switch (columnId)
	{
	case OC_TrackID:
		return 50;
	case OC_SourceID:
		return 50;
	case OC_Mapping:
		return 100;
	case OC_ComsMode:
		return 100;
	default:
		break;
	}

	return 0;
}

/**
 *  This is overloaded from Component, and will reposition the TableListBox inside it. 
 */
void CTableModelComponent::resized()
{
	// position our table with a gap around its edge
	m_table.setBoundsInset(BorderSize<int>(8));
}


/*
===============================================================================
 Class CComboBoxContainer
===============================================================================
*/

/**
 * Class constructor.
 */
CComboBoxContainer::CComboBoxContainer(CTableModelComponent& td)
	: m_owner(td)
{
	// Create and configure actual combo box component inside this container.
	m_comboBox.setEditableText(false);
	m_comboBox.addItem("1", 1);
	m_comboBox.addItem("2", 2);
	m_comboBox.addItem("3", 3);
	m_comboBox.addItem("4", 4);
	m_comboBox.addListener(this);
	m_comboBox.setColour(ComboBox::backgroundColourId, CDbStyle::GetDbColor(CDbStyle::DarkColor));
	m_comboBox.setColour(ComboBox::textColourId, CDbStyle::GetDbColor(CDbStyle::TextColor));
	m_comboBox.setColour(ComboBox::outlineColourId, CDbStyle::GetDbColor(CDbStyle::WindowColor));
	m_comboBox.setColour(ComboBox::buttonColourId, CDbStyle::GetDbColor(CDbStyle::MidColor));
	m_comboBox.setColour(ComboBox::arrowColourId, CDbStyle::GetDbColor(CDbStyle::TextColor));
	m_comboBox.setWantsKeyboardFocus(false);
	addAndMakeVisible(m_comboBox);
}

/**
 * Class destructor.
 */
CComboBoxContainer::~CComboBoxContainer()
{
}

/**
 * Reimplemented from ComboBox::Listener, gets called whenever the selected combo box item is changed.
 * @param comboBox	The comboBox which has been changed.
 */
void CComboBoxContainer::comboBoxChanged(ComboBox *comboBox)
{
	// Get the list of rows which are currently selected on the table.
	std::vector<int> selectedRows = m_owner.GetSelectedRows();
	if ((selectedRows.size() < 2) ||
		(std::find(selectedRows.begin(), selectedRows.end(), m_row) == selectedRows.end()))
	{
		// If this comboBoxes row (m_row) is NOT selected, or if no multi-selection was made 
		// then modify the selectedRows list so that it only contains m_row.
		selectedRows.clear();
		selectedRows.push_back(m_row);
	}

	// Get the IDs of the plugins on the selected rows.
	std::vector<PluginId> pluginIds = m_owner.GetPluginIdsForRows(selectedRows);

	CController* ctrl = CController::GetInstance();
	if (ctrl)
	{
		// New MappingID which should be applied to all plugins in the selected rows.
		int newMapping = comboBox->getSelectedId();
		for (std::size_t i = 0; i < pluginIds.size(); ++i)
		{
			// Set the value of the combobox to the current MappingID of the corresponding plugin.
			CPlugin* plugin = ctrl->GetProcessor(pluginIds[i]);
			if (plugin)
				plugin->SetMappingId(DCS_Overview, newMapping);
		}
	}
}

/**
 * Reimplemented from Component, used to resize the actual combo box component inside.
 */
void CComboBoxContainer::resized()
{
	m_comboBox.setBoundsInset(BorderSize<int>(4));
}

/**
 * Saves the row number where this component is located inside the overview table.
 * It also updated the combo box's selected item according to that plugin's MappingID.
 * @param newRow	The new row number.
 */
void CComboBoxContainer::SetRow(int newRow)
{
	m_row = newRow;

	// Find the plugin instance corresponding to the given row number.
	PluginId pluginId = m_owner.GetPluginIdForRow(newRow);
	CController* ctrl = CController::GetInstance();
	if (ctrl)
	{
		// Set the value of the combobox to the current MappingID of the corresponding plugin.
		const CPlugin* plugin = ctrl->GetProcessor(pluginId);
		if (plugin)
			m_comboBox.setSelectedId(plugin->GetMappingId(), dontSendNotification);
	}
}


/*
===============================================================================
 Class CTextEditorContainer
===============================================================================
*/

/**
 * Class constructor.
 */
CTextEditorContainer::CTextEditorContainer(CTableModelComponent& td)
	: m_owner(td)
{
	// Create and configure actual textEditor component inside this container.
	m_editor.addListener(this);
	addAndMakeVisible(m_editor);
}

/**
 * Class destructor.
 */
CTextEditorContainer::~CTextEditorContainer()
{
}

/**
 * Reimplemented from TextEditor::Listener, gets called whenever the TextEditor loses keyboard focus.
 * @param textEditor	The textEditor which has been changed.
 */
void CTextEditorContainer::textEditorFocusLost(TextEditor& textEditor)
{
	// Get the list of rows which are currently selected on the table.
	std::vector<int> selectedRows = m_owner.GetSelectedRows();
	if ((selectedRows.size() < 2) ||
		(std::find(selectedRows.begin(), selectedRows.end(), m_row) == selectedRows.end()))
	{
		// If this comboBoxes row (m_row) is NOT selected, or if no multi-selection was made 
		// then modify the selectedRows list so that it only contains m_row.
		selectedRows.clear();
		selectedRows.push_back(m_row);
	}

	// Get the IDs of the plugins on the selected rows.
	std::vector<PluginId> pluginIds = m_owner.GetPluginIdsForRows(selectedRows);

	CController* ctrl = CController::GetInstance();
	CTextEditor *myEditor = static_cast<CTextEditor*>(&textEditor);
	if (myEditor && ctrl)
	{
		// New SourceID which should be applied to all plugins in the selected rows.
		int newSourceId;
		newSourceId = myEditor->getText().getIntValue();
		for (std::size_t i = 0; i < pluginIds.size(); ++i)
		{
			// Set the value of the combobox to the current MappingID of the corresponding plugin.
			CPlugin* plugin = ctrl->GetProcessor(pluginIds[i]);
			if (plugin)
				plugin->SetSourceId(DCS_Overview, newSourceId);
		}
	}
}

/**
 * Callback function for Enter key presses on textEditors.
 * @param textEditor	The TextEditor object whose where enter key was pressed.
 */
void CTextEditorContainer::textEditorReturnKeyPressed(TextEditor& textEditor)
{
	ignoreUnused(textEditor);

	// Remove keyboard focus from this editor. 
	// Function textEditorFocusLost will then take care of setting values.
	//m_owner.grabKeyboardFocus();

	textEditor.unfocusAllComponents();
	unfocusAllComponents();
}

/**
 * Reimplemented from Component, used to resize the actual component inside.
 */
void CTextEditorContainer::resized()
{
	m_editor.setBoundsInset(BorderSize<int>(4));
}

/**
 * Saves the row number where this component is located inside the overview table.
 * It also updates the text inside the textEditor with the current SourceID
 * @param newRow	The new row number.
 */
void CTextEditorContainer::SetRow(int newRow)
{
	m_row = newRow;

	// Find the plugin instance corresponding to the given row number.
	PluginId pluginId = m_owner.GetPluginIdForRow(newRow);
	CController* ctrl = CController::GetInstance();
	if (ctrl)
	{
		// Set the value of the textEditor to the current SourceID of the corresponding plugin.
		const CPlugin* plugin = ctrl->GetProcessor(pluginId);
		if (plugin)
			m_editor.setText(String(plugin->GetSourceId()), false);
	}
}


/*
===============================================================================
 Class CRadioButtonContainer
===============================================================================
*/

/**
 * Class constructor.
 */
CRadioButtonContainer::CRadioButtonContainer(CTableModelComponent& td)
	: m_owner(td)
{
	// Create and configure button components inside this container.
	m_txButton.setName("Tx");
	m_txButton.setEnabled(true);
	m_txButton.addListener(this);
	addAndMakeVisible(m_txButton);

	m_rxButton.setName("Rx");
	m_rxButton.setEnabled(true);
	m_rxButton.addListener(this);
	addAndMakeVisible(m_rxButton);
}

/**
 * Class destructor.
 */
CRadioButtonContainer::~CRadioButtonContainer()
{
}

/**
 * Reimplemented from Button::Listener, gets called whenever the buttons are clicked.
 * @param button	The button which has been clicked.
 */
void CRadioButtonContainer::buttonClicked(Button *button)
{
	CController* ctrl = CController::GetInstance();
	if (ctrl && 
		((button == &m_txButton) || (button == &m_rxButton))) 
	{
		bool newToggleState = button->getToggleState();

		// Get the list of rows which are currently selected on the table.
		std::vector<int> selectedRows = m_owner.GetSelectedRows();
		if ((selectedRows.size() < 2) ||
			(std::find(selectedRows.begin(), selectedRows.end(), m_row) == selectedRows.end()))
		{
			// If this button's row (m_row) is NOT selected, or if no multi-selection was made 
			// then modify the selectedRows list so that it only contains m_row.
			selectedRows.clear();
			selectedRows.push_back(m_row);
		}

		// Get the IDs of the plugins on the selected rows.
		std::vector<PluginId> pluginIds = m_owner.GetPluginIdsForRows(selectedRows);

		for (std::size_t i = 0; i < pluginIds.size(); ++i)
		{
			CPlugin* plugin = ctrl->GetProcessor(pluginIds[i]);
			if (plugin)
			{
				ComsMode oldMode = plugin->GetComsMode();
				ComsMode newFlag = (button == &m_txButton) ? CM_Tx : CM_Rx;

				if (newToggleState == true)
					oldMode |= newFlag;
				else
					oldMode &= ~newFlag;

				plugin->SetComsMode(DCS_Overview, oldMode);
			}
		}
	}
}

/**
 * Reimplemented from Component, used to resize the actual component inside.
 */
void CRadioButtonContainer::resized()
{
	int w = getLocalBounds().getWidth();
	int h = getLocalBounds().getHeight();
	m_txButton.setBounds(2, 2, (w / 2) - 3, h - 5);
	m_rxButton.setBounds(w / 2, 2, (w / 2) - 3, h - 5);
}

/**
 * Saves the row number where this component is located inside the overview table.
 * It also updates the radio buttons with the current ComsMode.
 * @param newRow	The new row number.
 */
void CRadioButtonContainer::SetRow(int newRow)
{
	m_row = newRow;

	// Find the plugin instance corresponding to the given row number.
	PluginId pluginId = m_owner.GetPluginIdForRow(newRow);
	CController* ctrl = CController::GetInstance();
	if (ctrl)
	{
		// Toggle the correct radio buttons to the current ComsMode of the corresponding plugin.
		const CPlugin* plugin = ctrl->GetProcessor(pluginId);
		if (plugin)
		{
			const Array<AudioProcessorParameter*>& params = plugin->getParameters();
			AudioParameterChoice* param = dynamic_cast<AudioParameterChoice*>(params[ParamIdx_DelayMode]);
			if (param)
			{
				ComsMode newMode = plugin->GetComsMode();
				m_txButton.setToggleState(((newMode & CM_Tx) == CM_Tx), dontSendNotification);
				m_rxButton.setToggleState(((newMode & CM_Rx) == CM_Rx), dontSendNotification);
			}
		}
	}
}


/*
===============================================================================
 Class CEditableLabelContainer
===============================================================================
*/

/**
 * Class constructor.
 */
CEditableLabelContainer::CEditableLabelContainer(CTableModelComponent& td) 
	: m_owner(td)
{
	// Here we set the 'editOnDoubleClick' to true, but then override the 
	// mouseDoubleClick() method to prevent editing. This is to prevent the
	// TextEdit components on the OC_SourceID column from getting keyboard 
	// focus automatically when a row is selected.
	setEditable(false, true, false);
}

/**
 * Class destructor.
 */
CEditableLabelContainer::~CEditableLabelContainer()
{
}

/**
 * Reimplemented from Label, gets called whenever the label is clicked.
 * @param event		The mouse event properties.
 */
void CEditableLabelContainer::mouseDown(const MouseEvent& event)
{
	// Emulate R1 behaviour that is not standard for Juce: if multiple rows are selected
	// and one of the selected rows is clicked, only this row should remain selected.
	// So here we clear the selection and further down the clicked row is selected.
	if ((m_owner.GetTable().getNumSelectedRows() > 1) && m_owner.GetTable().isRowSelected(m_row))
		m_owner.GetTable().deselectAllRows();

	// Single click on the label should simply select the row
	m_owner.GetTable().selectRowsBasedOnModifierKeys(m_row, event.mods, false);

	// Base class implementation.
	Label::mouseDown(event);
}

/**
 * Reimplemented from Label to prevent label editing (see setEditable(..)).
 * @param event		The mouse event properties.
 */
void CEditableLabelContainer::mouseDoubleClick(const MouseEvent& event)
{
	ignoreUnused(event);

	// Do nothing.
}

/**
 * Saves the row number where this component is located inside the overview table.
 * It also updates the text to the current plugins name.
 * @param newRow	The new row number.
 */
void CEditableLabelContainer::SetRow(int newRow)
{
	m_row = newRow;
	String displayName;

	// Find the plugin instance corresponding to the given row number.
	PluginId pluginId = m_owner.GetPluginIdForRow(newRow);
	CController* ctrl = CController::GetInstance();
	if (ctrl)
	{
		// Set the value of the combobox to the current MappingID of the corresponding plugin.
		CPlugin* plugin = ctrl->GetProcessor(pluginId);
		if (plugin)
		{
			displayName = plugin->getProgramName(0);
			if (displayName.isEmpty())
				displayName = String("Input ") + String(plugin->GetSourceId());

		}
	}

	setText(displayName, dontSendNotification);
}


} // namespace dbaudio
