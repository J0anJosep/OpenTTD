/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file settings_widget.h Types related to the settings widgets. */

#ifndef WIDGETS_SETTINGS_WIDGET_H
#define WIDGETS_SETTINGS_WIDGET_H

/** Widgets of the #GameOptionsWindow class. */
enum GameOptionsWidgets {
	WID_GO_FONTS = 7,              ///< Number of fonts to show in the window.
	WID_GO_BACKGROUND = 0,         ///< Background of the window.

	WID_GO_TAB_BEGIN,              ///< Tabs.
	WID_GO_TAB_GENERAL = WID_GO_TAB_BEGIN,
	WID_GO_TAB_GRAPHICS,
	WID_GO_TAB_SOUND,
	WID_GO_TAB_MUSIC,
	WID_GO_TAB_FONTS,
	WID_GO_TAB_END,

	WID_GO_SEL_PANEL_BEGIN,        ///< Panels.
	WID_GO_SEL_GENERAL = WID_GO_SEL_PANEL_BEGIN,
	WID_GO_SEL_GRAPHICS,
	WID_GO_SEL_SOUND,
	WID_GO_SEL_MUSIC,
	WID_GO_SEL_FONTS,
	WID_GO_SEL_PANEL_END,

	WID_GO_CURRENCY_DROPDOWN,      ///< Currency dropdown.
	WID_GO_DISTANCE_DROPDOWN,      ///< Measuring unit dropdown.
	WID_GO_ROADSIDE_DROPDOWN,      ///< Dropdown to select the road side (to set the right side ;)).
	WID_GO_TOWNNAME_DROPDOWN,      ///< Town name dropdown.
	WID_GO_AUTOSAVE_DROPDOWN,      ///< Dropdown to say how often to autosave.
	WID_GO_LANG_DROPDOWN,          ///< Language dropdown.
	WID_GO_RESOLUTION_DROPDOWN,    ///< Dropdown for the resolution.
	WID_GO_FULLSCREEN_BUTTON,      ///< Toggle fullscreen.
	WID_GO_GUI_ZOOM_DROPDOWN,      ///< Dropdown for the GUI zoom level.
	WID_GO_BASE_GRF_DROPDOWN,      ///< Use to select a base GRF.
	WID_GO_BASE_GRF_STATUS,        ///< Info about missing files etc.
	WID_GO_BASE_GRF_TEXTFILE,      ///< Open base GRF readme, changelog (+1) or license (+2).
	WID_GO_BASE_GRF_DESCRIPTION = WID_GO_BASE_GRF_TEXTFILE + TFT_END,     ///< Description of selected base GRF.
	WID_GO_BASE_SFX_DROPDOWN,      ///< Use to select a base SFX.
	WID_GO_BASE_SFX_TEXTFILE,      ///< Open base SFX readme, changelog (+1) or license (+2).
	WID_GO_BASE_SFX_DESCRIPTION = WID_GO_BASE_SFX_TEXTFILE + TFT_END,     ///< Description of selected base SFX.
	WID_GO_BASE_MUSIC_DROPDOWN,    ///< Use to select a base music set.
	WID_GO_BASE_MUSIC_STATUS,      ///< Info about corrupted files etc.
	WID_GO_BASE_MUSIC_TEXTFILE,    ///< Open base music readme, changelog (+1) or license (+2).
	WID_GO_BASE_MUSIC_DESCRIPTION = WID_GO_BASE_MUSIC_TEXTFILE + TFT_END, ///< Description of selected base music set.

	WID_GO_BASE_AUTOSIZING,             ///< Automatically set sizing of fonts and some window parts.
	WID_GO_BASE_MIN_BUTTON_BUTTON,      ///< Min size of buttons.
	WID_GO_BASE_MIN_STEP_BUTTON,        ///< Min size of small steps on lists.

	WID_GO_BASE_FONTS_CUSTOM_MEDIUM,    ///< Button for allowing custom TrueType fonts for medium font.
	WID_GO_BASE_FONTS_CUSTOM_SMALL,     ///< Button for allowing custom TrueType fonts for small font.
	WID_GO_BASE_FONTS_CUSTOM_LARGE,     ///< Button for allowing custom TrueType fonts for large font.
	WID_GO_BASE_FONTS_CUSTOM_MONO,      ///< Button for allowing custom TrueType fonts for monospaced font.
	WID_GO_BASE_FONTS_CUSTOM_SYMBOL_BIG,///< Button for allowing custom TrueType fonts for big symbols in OpenTTD.
	WID_GO_BASE_FONTS_CUSTOM_SYMBOL,    ///< Button for allowing custom TrueType fonts for symbols in OpenTTD.
	WID_GO_BASE_FONTS_CUSTOM_SYMBOL_SMALL,    ///< Button for allowing custom TrueType fonts for symbols in OpenTTD.
	WID_GO_BASE_FONTS_CUSTOM_BEGIN = WID_GO_BASE_FONTS_CUSTOM_MEDIUM,
	WID_GO_BASE_FONTS_CUSTOM_END = WID_GO_BASE_FONTS_CUSTOM_BEGIN + WID_GO_FONTS,

	WID_GO_BASE_FONTS_FONT_MEDIUM,    ///< Button for selecting a TrueType font for medium font.
	WID_GO_BASE_FONTS_FONT_SMALL,     ///< Button for selecting a TrueType font for small font.
	WID_GO_BASE_FONTS_FONT_LARGE,     ///< Button for selecting a TrueType font for large font.
	WID_GO_BASE_FONTS_FONT_MONO,      ///< Button for selecting a TrueType font for monospaced font.
	WID_GO_BASE_FONTS_FONT_SYMBOL_BIG,///< Button for selecting a TrueType font for big symbols in OpenTTD.
	WID_GO_BASE_FONTS_FONT_SYMBOL,    ///< Button for selecting a TrueType font for symbols in OpenTTD.
	WID_GO_BASE_FONTS_FONT_SYMBOL_SMALL, ///< Button for selecting a TrueType font for small symbols in OpenTTD.
	WID_GO_BASE_FONTS_FONT_BEGIN = WID_GO_BASE_FONTS_FONT_MEDIUM,
	WID_GO_BASE_FONTS_FONT_END = WID_GO_BASE_FONTS_FONT_BEGIN + WID_GO_FONTS,

	WID_GO_BASE_FONTS_SIZE_MEDIUM,    ///< Button for selecting the size for medium font.
	WID_GO_BASE_FONTS_SIZE_SMALL,     ///< Button for selecting the size for small font.
	WID_GO_BASE_FONTS_SIZE_LARGE,     ///< Button for selecting the size for large font.
	WID_GO_BASE_FONTS_SIZE_MONO,      ///< Button for selecting the size for monospaced font.
	WID_GO_BASE_FONTS_SIZE_SYMBOL_BIG,///< Button for selecting the size for big symbols in OpenTTD.
	WID_GO_BASE_FONTS_SIZE_SYMBOL,    ///< Button for selecting the size for symbols in OpenTTD.
	WID_GO_BASE_FONTS_SIZE_SYMBOL_SMALL,///< Button for selecting the size for small symbols in OpenTTD.
	WID_GO_BASE_FONTS_SIZE_BEGIN = WID_GO_BASE_FONTS_SIZE_MEDIUM,
	WID_GO_BASE_FONTS_SIZE_END = WID_GO_BASE_FONTS_SIZE_BEGIN + WID_GO_FONTS,

	WID_GO_BASE_FONTS_AA_MEDIUM,      ///< Button for enable/disable antialiasing for medium font.
	WID_GO_BASE_FONTS_AA_SMALL,       ///< Button for enable/disable antialiasing for small font.
	WID_GO_BASE_FONTS_AA_LARGE,       ///< Button for enable/disable antialiasing for large font.
	WID_GO_BASE_FONTS_AA_MONO,        ///< Button for enable/disable antialiasing for monospaced font.
	WID_GO_BASE_FONTS_AA_SYMBOL_BIG,  ///< Button for enable/disable antialiasing for big symbols in OpenTTD.
	WID_GO_BASE_FONTS_AA_SYMBOL,      ///< Button for enable/disable antialiasing for symbols in OpenTTD.
	WID_GO_BASE_FONTS_AA_SYMBOL_SMALL,  ///< Button for enable/disable antialiasing for small symbols in OpenTTD.
	WID_GO_BASE_FONTS_AA_BEGIN = WID_GO_BASE_FONTS_AA_MEDIUM,
	WID_GO_BASE_FONTS_AA_END = WID_GO_BASE_FONTS_AA_BEGIN + WID_GO_FONTS,
};

/** Widgets of the #GameSettingsWindow class. */
enum GameSettingsWidgets {
	WID_GS_FILTER,             ///< Text filter.
	WID_GS_OPTIONSPANEL,       ///< Panel widget containing the option lists.
	WID_GS_SCROLLBAR,          ///< Scrollbar.
	WID_GS_HELP_TEXT,          ///< Information area to display help text of the selected option.
	WID_GS_EXPAND_ALL,         ///< Expand all button.
	WID_GS_COLLAPSE_ALL,       ///< Collapse all button.
	WID_GS_RESTRICT_CATEGORY,  ///< Label upfront to the category drop-down box to restrict the list of settings to show
	WID_GS_RESTRICT_TYPE,      ///< Label upfront to the type drop-down box to restrict the list of settings to show
	WID_GS_RESTRICT_DROPDOWN,  ///< The drop down box to restrict the list of settings
	WID_GS_TYPE_DROPDOWN,      ///< The drop down box to choose client/game/company/all settings
};

/** Widgets of the #CustomCurrencyWindow class. */
enum CustomCurrencyWidgets {
	WID_CC_RATE_DOWN,      ///< Down button.
	WID_CC_RATE_UP,        ///< Up button.
	WID_CC_RATE,           ///< Rate of currency.
	WID_CC_SEPARATOR_EDIT, ///< Separator edit button.
	WID_CC_SEPARATOR,      ///< Current separator.
	WID_CC_PREFIX_EDIT,    ///< Prefix edit button.
	WID_CC_PREFIX,         ///< Current prefix.
	WID_CC_SUFFIX_EDIT,    ///< Suffix edit button.
	WID_CC_SUFFIX,         ///< Current suffix.
	WID_CC_YEAR_DOWN,      ///< Down button.
	WID_CC_YEAR_UP,        ///< Up button.
	WID_CC_YEAR,           ///< Year of introduction.
	WID_CC_PREVIEW,        ///< Preview.
};

#endif /* WIDGETS_SETTINGS_WIDGET_H */
