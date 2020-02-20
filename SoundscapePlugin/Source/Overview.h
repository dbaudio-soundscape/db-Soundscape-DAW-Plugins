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


#pragma once

#include "About.h"
#include "Gui.h"
#include "Common.h"


namespace dbaudio
{


/**
 * Forward declarations
 */
class COverview;
class CTabbedComponent;
class CTabBarButton;
class COverviewComponent;
class COverviewTableContainer;
class COverviewMultiSurface;
class CTableModelComponent;
class CComboBoxContainer;
class CTextEditorContainer;
class CRadioButtonContainer;
class CEditableLabelContainer;


/**
 * Class COverviewManager which takes care of opening and closing the overview window.
 */
class COverviewManager
{
public:
	COverviewManager();
	virtual ~COverviewManager();
	static COverviewManager* GetInstance();

	void OpenOverview();
	void CloseOverview(bool destroy);
	void SaveLastOverviewBounds(Rectangle<int> bounds);
	Rectangle<int> GetOverviewBounds() const;

	int GetActiveTab() const;
	void SetActiveTab(int tabIdx);

	int GetSelectedMapping() const;
	void SetSelectedMapping(int mapping);

protected:
	/**
	 * The one and only instance of COverviewManager.
	 */
	static COverviewManager		*m_singleton;

	/**
	 * Pointer to the Overview winodw, if any.
	 */
	COverview*				m_overview;

	/**
	 * Default position and size of the overview window.
	 */
	Rectangle<int>			m_overviewBounds;

	/**
	 * Remember the last active tab.
	 */
	int						m_selectedTab = 0;// CTabbedComponent::OTI_Table;

	/**
	 * Remember the last selected coordinate mapping for the multi-slider
	 */
	int						m_selectedMapping = 1;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(COverviewManager)
};


/**
 * Class COverview inherits from DocumentWindow to provide a resizeable window with minimize, maximize and close buttons.
 */
class COverview : public DocumentWindow
{
public:
	COverview();
	~COverview() override;

private:
	virtual void closeButtonPressed() override;

	/**
	 *	Component within the overview window, used as container for all other components.
	 */
	std::unique_ptr<COverviewComponent> m_contentComponent;


	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(COverview)
};


/**
 * Class COverviewComponent is a simple container used to hold the GUI controls.
 */
class COverviewComponent : public Component,
	public TextEditor::Listener,
	private Timer
{
public:
	COverviewComponent();
	~COverviewComponent() override;
	void UpdateGui(bool init);

private:
	void paint(Graphics&) override;
	void resized() override;

	void textEditorFocusLost(TextEditor &) override;
	void textEditorReturnKeyPressed(TextEditor &) override;

	void timerCallback() override;

private:
	/**
	 * App version label
	 */
	std::unique_ptr<CLabel>	m_versionLabel;

	/**
	 * App name
	 */
	std::unique_ptr<CLabel>	m_nameLabel;

	/**
	 * Overview title label
	 */
	std::unique_ptr<CLabel>	m_titleLabel;

	/*
	 * Logo image.
	 */
	Image					m_dbLogo;

	/**
	 * DS100 IP Address label
	 */
	std::unique_ptr<CLabel>	m_ipAddressLabel;

	/**
	 * Text editor for the DS100 IP Address
	 */
	std::unique_ptr<CTextEditor>	m_ipAddressTextEdit;

	/**
	 * Send/receive rate label
	 */
	std::unique_ptr<CLabel>	m_rateLabel;

	/**
	 * Text editor for the OSC send/receive rate in ms.
	 */
	std::unique_ptr<CTextEditor>	m_rateTextEdit;

	/**
	 * Button used as Online indicator LED.
	 */
	std::unique_ptr<CButton> m_onlineLed;

	/**
	 * A container for tabs.
	 */
	std::unique_ptr<CTabbedComponent> m_tabbedComponent;

	/**
	 * The actual table container inside this component.
	 */
	std::unique_ptr<COverviewTableContainer> m_tableContainer;

	/**
	 * Container for multi-slider.
	 */
	std::unique_ptr<COverviewMultiSurface> m_multiSliderContainer;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(COverviewComponent)
};


/** 
 * Reimplemented TabbedComponent which overrides the createTabButton method in order
 * to provide custom tabBar buttons (See CTabBarButton).
 */
class CTabbedComponent : public TabbedComponent
{
public:

	/**
	 * Overview tab indeces
	 */
	enum OverviewTabIndex
	{
		OTI_Table = 0,
		OTI_MultiSlider
	};

	CTabbedComponent();
	~CTabbedComponent() override;

	static void GetIconPath(int tabIdx, Point<float> iconSize, float strokeThickness, Path& p);

protected:
	TabBarButton* createTabButton(const String& tabName, int tabIndex) override;
	void currentTabChanged(int newCurrentTabIndex, const String &newCurrentTabName) override;
	void resized() override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CTabbedComponent)
};


/** 
 * Reimplemented TabBarButton which overrides the paintButton method 
 * to show an icon instead of the standard tab name text.
 */
class CTabBarButton : public TabBarButton
{
public:
	CTabBarButton(int tabIdx, TabbedButtonBar& ownerBar);
	~CTabBarButton() override;

protected:
	void paintButton(Graphics&, bool, bool) override;

private:
	int m_tabIndex;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CTabBarButton)
};


/**
 * Class COverviewTableContainer is just a component which contains the overview table 
 * and it's quick selection buttons.
 */
class COverviewTableContainer : public AOverlay,
	public Button::Listener
{
public:
	COverviewTableContainer();
	~COverviewTableContainer() override;

	void UpdateGui(bool init) override;
	void buttonClicked(Button*) override;

protected:
	void paint(Graphics&) override;
	void resized() override;

private:
	/**
	 * The actual table model / component inside this component.
	 */
	std::unique_ptr<CTableModelComponent> m_overviewTable;

	/**
	 * Quick select label
	 */
	std::unique_ptr<CLabel>	m_selectLabel;

	/**
	 * Select all rows button.
	 */
	std::unique_ptr<CButton> m_selectAll;

	/**
	 * Select no rows button.
	 */
	std::unique_ptr<CButton> m_selectNone;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(COverviewTableContainer)
};


/**
 * Class COverviewMultiSurface is just a component which contains the multi-source slider
 * and the mapping selection control.
 */
class COverviewMultiSurface : public AOverlay,
	public ComboBox::Listener
{
public:
	COverviewMultiSurface();
	~COverviewMultiSurface() override;

	void UpdateGui(bool init) override;

protected:
	void paint(Graphics&) override;
	void resized() override;
	void comboBoxChanged(ComboBox *comboBox) override;

private:
	/**
	 * Multi-source 2D-Slider.
	 */
	std::unique_ptr<Component> m_multiSlider;

	/*
	 * Mapping selector label
	 */
	std::unique_ptr<CLabel>	m_posAreaLabel;

	/**
	 * ComboBox selector for the coordinate mapping area
	 */
	std::unique_ptr<ComboBox>	m_areaSelector;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(COverviewMultiSurface)
};


/**
 * Class CTableModelComponent acts as a table model and a component at the same time.
 */
class CTableModelComponent : public Component,
	public TableListBoxModel
{
public:

	/**
	 * Enum defininig the table columns used in the overview table.
	 */
	enum OverviewColumn
	{
		OC_None = 0,		//< Juce column IDs start at 1
		OC_TrackID,
		OC_SourceID,
		OC_Mapping,
		OC_ComsMode,
		OC_MAX_COLUMNS
	};

	CTableModelComponent();
	~CTableModelComponent() override;

	static bool LessThanSourceId(PluginId pId1, PluginId pId2);
	static bool LessThanMapping(PluginId pId1, PluginId pId2);
	static bool LessThanComsMode(PluginId pId1, PluginId pId2);

	PluginId GetPluginIdForRow(int rowNumber);
	std::vector<PluginId> GetPluginIdsForRows(std::vector<int> rowNumbers);
	std::vector<int> GetSelectedRows() const;
	void SelectAllRows(bool all);
	void RecreateTableRowIds();
	void UpdateTable();
	TableListBox& GetTable() { return m_table; }


	// Overriden methods from TableListBoxModel
	void backgroundClicked(const MouseEvent &) override;
	int getNumRows() override;
	void paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
	void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
	void sortOrderChanged(int newSortColumnId, bool isForwards) override;
	Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate) override;
	int getColumnAutoSizeWidth(int columnId) override;


	// Overriden methods from Component
	void resized() override;

private:
	/**
	 * The table component itself.
	 */
	TableListBox	m_table;

	/**
	 * Local list of Plug-in instance IDs, one for each row in the table.
	 */
	std::vector<PluginId> m_ids;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CTableModelComponent)
};


/**
 * Class CComboBoxContainer is a container for the MappingId Combo box component used in the Overview table.
 */
class CComboBoxContainer : public Component,
	public ComboBox::Listener
{
public:
	explicit CComboBoxContainer(CTableModelComponent& td);
	~CComboBoxContainer() override;

	void comboBoxChanged(ComboBox *comboBox) override;
	void resized() override;
	void SetRow(int newRow);

private:
	/**
	 * Table where this component is contained.
	 */
	CTableModelComponent&	m_owner;

	/**
	 * Actual combo box component.
	 */
	ComboBox				m_comboBox;

	/**
	 * Row number where this component is located inside the table.
	 */
	int						m_row;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CComboBoxContainer)
};


/**
 * Class CTextEditorContainer is a container for the SourceID CTextEditor component used in the Overview table.
 */
class CTextEditorContainer : public Component,
	public TextEditor::Listener
{
public:
	explicit CTextEditorContainer(CTableModelComponent& td);
	~CTextEditorContainer() override;

	void textEditorFocusLost(TextEditor &) override;
	void textEditorReturnKeyPressed(TextEditor &) override;
	void resized() override;
	void SetRow(int newRow);

private:
	/**
	 * Table where this component is contained.
	 */
	CTableModelComponent&	m_owner;

	/**
	 * Actual text editor.
	 */
	CTextEditor				m_editor;

	/**
	 * Row number where this component is located inside the table.
	 */
	int						m_row;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CTextEditorContainer)
};


/**
 * Class CRadioButtonContainer is a container for the Tx/Rx buttons used in the Overview table.
 */
class CRadioButtonContainer : public Component,
	public Button::Listener
{
public:
	explicit CRadioButtonContainer(CTableModelComponent& td);
	~CRadioButtonContainer() override;

	void buttonClicked(Button*) override;
	void resized() override;
	void SetRow(int newRow);

private:
	/**
	 * Table where this component is contained.
	 */
	CTableModelComponent&	m_owner;

	/**
	 * Actual Tx button.
	 */
	CButton				m_txButton;

	/**
	 * Actual Rx button.
	 */
	CButton				m_rxButton;

	/**
	 * Row number where this component is located inside the table.
	 */
	int						m_row;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CRadioButtonContainer)
};


/**
 * Class CEditableLabelContainer is a container for editable labels used in the Overview table.
 */
class CEditableLabelContainer : public Label
{
public:
	explicit CEditableLabelContainer(CTableModelComponent& td);
	~CEditableLabelContainer() override;

	void mouseDown(const MouseEvent& event) override;
	void mouseDoubleClick(const MouseEvent &) override;
	void SetRow(int newRow);

private:
	/**
	 * Table where this component is contained.
	 */
	CTableModelComponent&	m_owner;

	/**
	 * Row number where this component is located inside the table.
	 */
	int						m_row;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CEditableLabelContainer)
};


} // namespace dbaudio
