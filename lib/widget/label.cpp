/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/** @file
 *  Functions for the label widget.
 */

#include "lib/framework/frame.h"
#include "widget.h"
#include "widgint.h"
#include "label.h"
#include "form.h"
#include "tip.h"

#include <algorithm>

W_LABINIT::W_LABINIT()
	: FontID(font_regular)
{}

W_LABEL::W_LABEL(W_LABINIT const *init)
	: WIDGET(init, WIDG_LABEL)
	, FontID(init->FontID)
	, pTip(init->pTip)
	, fontColour(WZCOL_FORM_TEXT)
{
	ASSERT((init->style & ~(WLAB_PLAIN | WLAB_ALIGNLEFT | WLAB_ALIGNRIGHT | WLAB_ALIGNCENTRE | WLAB_ALIGNTOP | WLAB_ALIGNBOTTOM | WIDG_HIDDEN)) == 0, "Unknown button style");
	setString(init->pText);
}

W_LABEL::W_LABEL(WIDGET *parent)
	: WIDGET(parent, WIDG_LABEL)
	, FontID(font_regular)
	, fontColour(WZCOL_FORM_TEXT)
{}

int W_LABEL::setFormattedString(WzString string, uint32_t MaxWidth, iV_fonts fontID, int _lineSpacing /*= 0*/, bool ignoreNewlines /*= false*/)
{
	lineSpacing = _lineSpacing;
	FontID = fontID;
	aTextLines = iV_FormatText(string.toUtf8().c_str(), MaxWidth, FTEXT_LEFTJUSTIFY, fontID, ignoreNewlines);

	int requiredHeight = 0;
	if (!aTextLines.empty())
	{
		requiredHeight = aTextLines.back().offset.y + iV_GetTextLineSize(fontID);
		requiredHeight += ((static_cast<int>(aTextLines.size()) - 1) * lineSpacing);
	}

	maxLineWidth = 0;
	for (const auto& line : aTextLines)
	{
		maxLineWidth = std::max(maxLineWidth, line.dimensions.x);
	}

	return requiredHeight;
}

#ifdef DEBUG_BOUNDING_BOXES
# include "lib/ivis_opengl/pieblitfunc.h"
#endif

void W_LABEL::display(int xOffset, int yOffset)
{
	int maxWidth = 0;
	int textTotalHeight = 0;
	displayCache.wzText.resize(aTextLines.size());
	for (size_t idx = 0; idx < aTextLines.size(); idx++)
	{
		displayCache.wzText[idx].setText(aTextLines[idx].text, FontID);
		maxWidth = std::max(maxWidth, displayCache.wzText[idx].width());
		textTotalHeight += displayCache.wzText[idx].lineSize();
		if (idx < (aTextLines.size() - 1))
		{
			textTotalHeight += lineSpacing;
		}
	}

	if (displayCache.wzText.empty()) return;

	Vector2i textBoundingBoxOffset(0, 0);
	if ((style & WLAB_ALIGNTOPLEFT) != 0)  // Align top
	{
		textBoundingBoxOffset.y = yOffset + y();
	}
	else if ((style & WLAB_ALIGNBOTTOMLEFT) != 0)  // Align bottom
	{
		textBoundingBoxOffset.y = yOffset + y() + (height() - textTotalHeight);
	}
	else
	{
		textBoundingBoxOffset.y = yOffset + y() + (height() - textTotalHeight) / 2;
	}

	int jy = 0;
	for (auto& wzTextLine : displayCache.wzText)
	{
		int fx = 0;
		if (style & WLAB_ALIGNCENTRE)
		{
			int fw = wzTextLine.width();
			fx = xOffset + x() + (width() - fw) / 2;
		}
		else if (style & WLAB_ALIGNRIGHT)
		{
			int fw = wzTextLine.width();
			fx = xOffset + x() + width() - fw;
		}
		else
		{
			fx = xOffset + x();
		}


		float fy = float(textBoundingBoxOffset.y) + float(jy) - float(wzTextLine.aboveBase());

#ifdef DEBUG_BOUNDING_BOXES
		// Display bounding boxes.
		PIELIGHT col;
		col.byte.r = 128 + iSinSR(realTime, 2000, 127); col.byte.g = 128 + iSinSR(realTime + 667, 2000, 127); col.byte.b = 128 + iSinSR(realTime + 1333, 2000, 127); col.byte.a = 128;
		iV_Box(textBoundingBoxOffset.x + fx, textBoundingBoxOffset.y + jy + baseLineOffset, textBoundingBoxOffset.x + fx + wzTextLine.width() - 1, textBoundingBoxOffset.y + jy + baseLineOffset + wzTextLine.lineSize() - 1, col);
#endif
		wzTextLine.render(textBoundingBoxOffset.x + fx, fy, fontColour);
		jy += wzTextLine.lineSize() + lineSpacing;
	}
}

/* Respond to a mouse moving over a label */
void W_LABEL::highlight(W_CONTEXT *psContext)
{
	/* If there is a tip string start the tool tip */
	if (!pTip.empty())
	{
		if (auto lockedScreen = screenPointer.lock())
		{
			tipStart(this, pTip, lockedScreen->TipFontID, x() + psContext->xOffset, y() + psContext->yOffset, width(), height());
		}
	}
}


/* Respond to the mouse moving off a label */
void W_LABEL::highlightLost()
{
	if (!pTip.empty())
	{
		tipStop(this);
	}
}

WzString W_LABEL::getString() const
{
	if (aTextLines.empty()) return WzString();
	return WzString::fromUtf8(aTextLines.front().text);
}

void W_LABEL::setString(WzString string)
{
	aTextLines.clear();
	aTextLines.push_back({string.toStdString(), Vector2i(0,0), Vector2i(0,0)});
	dirty = true;
}

void W_LABEL::setTip(std::string string)
{
	pTip = string;
}

void W_LABEL::setTextAlignment(WzTextAlignment align)
{
	style &= ~(WLAB_ALIGNLEFT | WLAB_ALIGNCENTRE | WLAB_ALIGNRIGHT);
	style |= align;
	dirty = true;
}
