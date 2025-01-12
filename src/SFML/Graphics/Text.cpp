////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2022 Laurent Gomila (laurent@sfml-dev.org)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

// This version was changed by PupperGump: Added text_props and alignment

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>

//text_props::text_props(sf::Text& t)
//{
//	fill_color = t.getFillColor();
//	outline_color = t.getOutlineColor();
//}

namespace
{
	// Add an underline or strikethrough line to the vertex array
	void addLine(sf::VertexArray& vertices,
        float            startX,
		float            lineLength,
		float            lineTop,
		const sf::Color& color,
		float            offset,
		float            thickness,
		float            outlineThickness = 0)
	{
		float top = std::floor(lineTop + offset - (thickness / 2) + 0.5f);
		float bottom = top + std::floor(thickness + 0.5f);

        // This appears to be a hack for drawing 2 triangles to make a rectangle due to m_vertices being of type sf::Triangles
        // Top left, top right, bottom left, bottom left, top right, bottom right

		vertices.append(sf::Vertex(sf::Vector2f(startX - outlineThickness, top - outlineThickness), color, sf::Vector2f(1, 1)));
		vertices.append(
			sf::Vertex(sf::Vector2f(lineLength + outlineThickness, top - outlineThickness), color, sf::Vector2f(1, 1)));
		vertices.append(sf::Vertex(sf::Vector2f(startX - outlineThickness, bottom + outlineThickness), color, sf::Vector2f(1, 1)));
		vertices.append(sf::Vertex(sf::Vector2f(startX - outlineThickness, bottom + outlineThickness), color, sf::Vector2f(1, 1)));
		vertices.append(
			sf::Vertex(sf::Vector2f(lineLength + outlineThickness, top - outlineThickness), color, sf::Vector2f(1, 1)));
		vertices.append(
			sf::Vertex(sf::Vector2f(lineLength + outlineThickness, bottom + outlineThickness), color, sf::Vector2f(1, 1)));
	}

	// Add a glyph quad to the vertex array
	void addGlyphQuad(sf::VertexArray& vertices, sf::Vector2f position, const sf::Color& color, const sf::Glyph& glyph, float italicShear)
	{
		float padding = 1.0;

		float left = glyph.bounds.left - padding;
		float top = glyph.bounds.top - padding;
		float right = glyph.bounds.left + glyph.bounds.width + padding;
		float bottom = glyph.bounds.top + glyph.bounds.height + padding;

		float u1 = static_cast<float>(glyph.textureRect.left) - padding;
		float v1 = static_cast<float>(glyph.textureRect.top) - padding;
		float u2 = static_cast<float>(glyph.textureRect.left + glyph.textureRect.width) + padding;
		float v2 = static_cast<float>(glyph.textureRect.top + glyph.textureRect.height) + padding;

		vertices.append(
			sf::Vertex(sf::Vector2f(position.x + left - italicShear * top, position.y + top), color, sf::Vector2f(u1, v1)));
		vertices.append(
			sf::Vertex(sf::Vector2f(position.x + right - italicShear * top, position.y + top), color, sf::Vector2f(u2, v1)));
		vertices.append(
			sf::Vertex(sf::Vector2f(position.x + left - italicShear * bottom, position.y + bottom), color, sf::Vector2f(u1, v2)));
		vertices.append(
			sf::Vertex(sf::Vector2f(position.x + left - italicShear * bottom, position.y + bottom), color, sf::Vector2f(u1, v2)));
		vertices.append(
			sf::Vertex(sf::Vector2f(position.x + right - italicShear * top, position.y + top), color, sf::Vector2f(u2, v1)));
		vertices.append(sf::Vertex(sf::Vector2f(position.x + right - italicShear * bottom, position.y + bottom),
			color,
			sf::Vector2f(u2, v2)));
	}
} // namespace


namespace sf
{
	////////////////////////////////////////////////////////////
	Text::Text() = default;


	////////////////////////////////////////////////////////////
	Text::Text(const String& string, const Font& font, unsigned int characterSize) :
		m_string(string),
		m_font(&font),
		m_characterSize(characterSize)
	{
	}


	////////////////////////////////////////////////////////////
	Text::Text(const Text&) = default;


	////////////////////////////////////////////////////////////
	Text& Text::operator=(const Text&) = default;


	////////////////////////////////////////////////////////////
	Text::Text(Text&&) noexcept = default;


	////////////////////////////////////////////////////////////
	Text& Text::operator=(Text&&) noexcept = default;


	////////////////////////////////////////////////////////////
	void Text::setString(const String& string)
	{
		if (m_string != string)
		{
			while (props.size() < string.getSize())
			{
				props.push_back(text_props(m_fillColor, m_outlineColor));
			}
			m_string = string;
			m_should_align = 1;
			m_geometryNeedUpdate = true;
		}
	}


	////////////////////////////////////////////////////////////
	void Text::setFont(const Font& font)
	{
		if (m_font != &font)
		{
			m_font = &font;
			m_geometryNeedUpdate = true;
		}
	}


	////////////////////////////////////////////////////////////
	void Text::setCharacterSize(unsigned int size)
	{
		if (m_characterSize != size)
		{
			m_characterSize = size;
			// Unfortunately it "refreshes" the glyphs in store if it changes too much. This may lead to noticable performance drops similar to minecraft every time its memory gets too high. If I find I way to trash the first half of the vector without causing corruption, I'll do it.
			m_font->clearPages(2000);
			m_geometryNeedUpdate = true;
		}
	}


	////////////////////////////////////////////////////////////
	void Text::setLetterSpacing(float spacingFactor)
	{
		if (m_letterSpacingFactor != spacingFactor)
		{
			m_letterSpacingFactor = spacingFactor;
			m_geometryNeedUpdate = true;
		}
	}


	////////////////////////////////////////////////////////////
	void Text::setLineSpacing(float spacingFactor)
	{
		if (m_lineSpacingFactor != spacingFactor)
		{
			m_lineSpacingFactor = spacingFactor;
			m_geometryNeedUpdate = true;
		}
	}


	////////////////////////////////////////////////////////////
	void Text::setStyle(std::uint32_t style)
	{
		if (m_style != style)
		{
			m_style = style;
			m_should_align = 1;
			m_geometryNeedUpdate = true;
		}
	}


	////////////////////////////////////////////////////////////
	void Text::setFillColor(const Color& color)
	{
		if (color != m_fillColor)
		{
			m_fillColor = color;

			// Change vertex colors directly, no need to update whole geometry
			// (if geometry is updated anyway, we can skip this step)
			if (!m_geometryNeedUpdate)
			{
				for (std::size_t i = 0; i < m_vertices.getVertexCount(); ++i)
					m_vertices[i].color = m_fillColor;
			}
		}
	}


	////////////////////////////////////////////////////////////
	void Text::setOutlineColor(const Color& color)
	{
		if (color != m_outlineColor)
		{
			m_outlineColor = color;

			// Change vertex colors directly, no need to update whole geometry
			// (if geometry is updated anyway, we can skip this step)
			if (!m_geometryNeedUpdate)
			{
				for (std::size_t i = 0; i < m_outlineVertices.getVertexCount(); ++i)
					m_outlineVertices[i].color = m_outlineColor;
			}
		}
	}


	////////////////////////////////////////////////////////////
	void Text::setOutlineThickness(float thickness)
	{
		if (thickness != m_outlineThickness)
		{
			m_outlineThickness = thickness;
			m_geometryNeedUpdate = true;
		}
	}


	////////////////////////////////////////////////////////////
	const String& Text::getString() const
	{
		return m_string;
	}


	////////////////////////////////////////////////////////////
	const Font* Text::getFont() const
	{
		return m_font;
	}


	////////////////////////////////////////////////////////////
	unsigned int Text::getCharacterSize() const
	{
		return m_characterSize;
	}


	////////////////////////////////////////////////////////////
	float Text::getLetterSpacing() const
	{
		return m_letterSpacingFactor;
	}


	////////////////////////////////////////////////////////////
	float Text::getLineSpacing() const
	{
		return m_lineSpacingFactor;
	}


	////////////////////////////////////////////////////////////
	std::uint32_t Text::getStyle() const
	{
		return m_style;
	}


	////////////////////////////////////////////////////////////
	const Color& Text::getFillColor() const
	{
		return m_fillColor;
	}


	////////////////////////////////////////////////////////////
	const Color& Text::getOutlineColor() const
	{
		return m_outlineColor;
	}


	////////////////////////////////////////////////////////////
	float Text::getOutlineThickness() const
	{
		return m_outlineThickness;
	}


	////////////////////////////////////////////////////////////
	Vector2f Text::findCharacterPos(std::size_t index) const
	{
		// Make sure that we have a valid font
		if (!m_font)
			return Vector2f();

		// Adjust the index if it's out of range
		if (index > m_string.getSize())
			index = m_string.getSize();

		// Precompute the variables needed by the algorithm
		bool  isBold = m_style & Bold;
		float whitespaceWidth = m_font->getGlyph(U' ', m_characterSize, isBold).advance;
		float letterSpacing = (whitespaceWidth / 3.f) * (m_letterSpacingFactor - 1.f);
		whitespaceWidth += letterSpacing;
		float lineSpacing = m_font->getLineSpacing(m_characterSize) * m_lineSpacingFactor;

		// Compute the position
		Vector2f      position;
		std::uint32_t prevChar = 0;
		for (std::size_t i = 0; i < index; ++i)
		{
			std::uint32_t curChar = m_string[i];

			// Apply the kerning offset
			position.x += m_font->getKerning(prevChar, curChar, m_characterSize, isBold);
			prevChar = curChar;

			// Handle special characters
			switch (curChar)
			{
			case U' ':
				position.x += whitespaceWidth;
				continue;
			case U'\t':
				position.x += whitespaceWidth * 4;
				continue;
			case U'\n':
				position.y += lineSpacing;
				position.x = 0;
				continue;
			}

			// For regular characters, add the advance offset of the glyph
			position.x += m_font->getGlyph(curChar, m_characterSize, isBold).advance + letterSpacing;
		}

		// Transform the position to global coordinates
		position = getTransform().transformPoint(position);

		return position;
	}


	////////////////////////////////////////////////////////////
	FloatRect Text::getLocalBounds() const
	{
		ensureGeometryUpdate();

		return m_bounds;
	}


	////////////////////////////////////////////////////////////
	FloatRect Text::getGlobalBounds() const
	{
		return getTransform().transformRect(getLocalBounds());
	}

	void Text::align(Align alignment, float width, sf::String token)
	{
        if (alignment != m_alignment || width != m_width || token != m_token)
		    m_geometryNeedUpdate = 1;
		m_should_align = 1;
		m_alignment = alignment;
		m_width = width;
        m_token = token;
	}


	////////////////////////////////////////////////////////////
	void Text::draw(RenderTarget& target, const RenderStates& states) const
	{
		// todo: At the cost of speed, I might separate each letter for customizability.
		if (m_font)
		{
			ensureGeometryUpdate();

			RenderStates statesCopy(states);

			statesCopy.transform *= getTransform();

			statesCopy.texture = &m_font->getTexture(m_characterSize);

			// Only draw the outline if there is something to draw
			if (m_outlineThickness != 0)
				target.draw(m_outlineVertices, statesCopy);

			target.draw(m_vertices, statesCopy);
		}
	}


	////////////////////////////////////////////////////////////
	void Text::ensureGeometryUpdate() const
	{
		if (!m_font)
			return;

		// Do nothing, if geometry has not changed and the font texture has not changed
		if (!m_geometryNeedUpdate && m_font->getTexture(m_characterSize).m_cacheId == m_fontTextureId)
			return;

		// Mark geometry as updated
		m_geometryNeedUpdate = false;

		// Clear the previous geometry
		m_vertices.clear();
		m_outlineVertices.clear();
		m_bounds = FloatRect();

		// No text: nothing to draw
		if (m_string.isEmpty())
			return;

		// Save the current fonts texture id
		m_fontTextureId = m_font->getTexture(m_characterSize).m_cacheId;

		// Compute values related to the text style
		bool  isBold = m_style & Bold;
		bool  isUnderlined = m_style & Underlined;
		bool  isStrikeThrough = m_style & StrikeThrough;
		float italicShear = (m_style & Italic) ? sf::degrees(12).asRadians() : 0.f;
		float underlineOffset = m_font->getUnderlinePosition(m_characterSize);
		float underlineThickness = m_font->getUnderlineThickness(m_characterSize);

		// Compute the location of the strike through dynamically
		// We use the center point of the lowercase 'x' glyph as the reference
		// We reuse the underline thickness as the thickness of the strike through as well
		FloatRect xBounds = m_font->getGlyph(U'x', m_characterSize, isBold).bounds;
		float     strikeThroughOffset = xBounds.top + xBounds.height / 2.f;

		// Precompute the variables needed by the algorithm
		float whitespaceWidth = m_font->getGlyph(U' ', m_characterSize, isBold).advance;
		float letterSpacing = (whitespaceWidth / 3.f) * (m_letterSpacingFactor - 1.f);
		whitespaceWidth += letterSpacing;
		float lineSpacing = m_font->getLineSpacing(m_characterSize) * m_lineSpacingFactor;
		float x = 0.f;
		auto  y = static_cast<float>(m_characterSize);

		// Create one quad for each character
		auto          minX = static_cast<float>(m_characterSize);
		auto          minY = static_cast<float>(m_characterSize);
		float         maxX = 0.f;
		float         maxY = 0.f;

		std::uint32_t prevChar = 0;

		size_t start = 0;
        float line_start = 0.f;

		for (std::size_t i = 0; i < m_string.getSize(); ++i)
		{
			std::uint32_t curChar = m_string[i];

			// Skip the \r char to avoid weird graphical issues
			if (curChar == U'\r')
				continue;

			// Extract the current glyph's description
			const Glyph& glyph = m_font->getGlyph(curChar, m_characterSize, isBold);

			// Update the current bounds
			float left = glyph.bounds.left;
			float top = glyph.bounds.top;
			float right = glyph.bounds.left + glyph.bounds.width;
			float bottom = glyph.bounds.top + glyph.bounds.height;



			// Align if told to (dosen't work with underlined and strikethrough until I fix the add_line() function)
            if (m_width > 0.f && i == start)
            {
                std::uint32_t prev = i == 0 ? 0 : m_string[i - 1];
                std::uint32_t curr = m_string[i];

                float offset = 0.f;
                float line_width = 0.f;
                float center = m_width / 2.f;
                bool found = 0;

                // Find last token before the next character's position is > line wrap width
                for (size_t j = start; j < m_string.getSize(); j++)
                {
                    curr = m_string[j];
                    const Glyph& temp_glyph = m_font->getGlyph(curr, m_characterSize, isBold);
                    offset += m_font->getKerning(prev, curr, m_characterSize, isBold);
                    offset += temp_glyph.advance + letterSpacing;

                    if (m_string[j] == m_token || m_string[j] == '\n')
                    {
                        if (j == m_string.getSize() - 1)
                            start = j;
                        else
                            start = j + 1;
                        found = 1;
                        line_width = offset;
                    }
                    if (offset > m_width || m_string[j] == '\n')
                    {
                        if (!found)
                            start = j + 1;
                        break;
                    }

                    if (j == m_string.getSize() - 1)
                    {
                        line_width = offset;
                        start = 0; // Needed to prevent last word from staying stuck at the bottom
                    }

                    prev = curr;
                }
                if (line_width == 0.f)
                    line_width = offset;

                if (m_alignment == Align::LEFT)
                    x = 0.f;
                else if (m_alignment == Align::CENTER)
                    x = center - line_width / 2.f;
                else if (m_alignment == Align::RIGHT)
                    x = m_width - line_width;
                line_start = x;

                if (i != 0)
                {
                    y += lineSpacing;
                }
            }
			

			// Apply the kerning offset
			x += m_font->getKerning(prevChar, curChar, m_characterSize, isBold);

			// If we're using the underlined style and there's a new line, draw a line
			//if (isUnderlined && (curChar == U'\n' && prevChar != U'\n'))

            // Condition to separate normal from aligned styles (aligned and line started) or (not aligned and newline)
            if (isUnderlined && (m_width > 0 && (i == start - 1) || (m_width == 0 && curChar == U'\n' && prevChar != U'\n')))
			{
				addLine(m_vertices, line_start, x, y, m_fillColor, underlineOffset, underlineThickness);

				if (m_outlineThickness != 0)
					addLine(m_outlineVertices, line_start, x, y, m_outlineColor, underlineOffset, underlineThickness, m_outlineThickness);
			}

			// If we're using the strike through style and there's a new line, draw a line across all characters
			if (isStrikeThrough && (m_width > 0 && (i == start - 1) || (m_width == 0 && curChar == U'\n' && prevChar != U'\n')))
			{
				addLine(m_vertices, line_start, x, y, m_fillColor, strikeThroughOffset, underlineThickness);

				if (m_outlineThickness != 0)
					addLine(m_outlineVertices, line_start, x, y, m_outlineColor, strikeThroughOffset, underlineThickness, m_outlineThickness);
			}

			prevChar = curChar;

			// Handle special characters
			if ((curChar == U' ') || (curChar == U'\n') || (curChar == U'\t'))
			{
				// Update the current bounds (min coordinates)
				minX = std::min(minX, x);
				minY = std::min(minY, y);

				switch (curChar)
				{
				case U' ':
					x += whitespaceWidth;
					break;
				case U'\t':
					x += whitespaceWidth * 4;
					break;
				case U'\n':
					y += lineSpacing;
					x = 0;
					break;
				}

				// Update the current bounds (max coordinates)
				maxX = std::max(maxX, x);
				maxY = std::max(maxY, y);

				// Next glyph, no need to create a quad for whitespace
				continue;
			}

			// Apply the outline
			if (m_outlineThickness != 0)
			{
				const Glyph& oglyph = m_font->getGlyph(curChar, m_characterSize, isBold, m_outlineThickness);

				// Add the outline glyph to the vertices
				addGlyphQuad(m_outlineVertices, Vector2f(x, y), props[i].outline_color, oglyph, italicShear);
			}

			// Extract the current glyph's description
			//const Glyph& glyph = m_font->getGlyph(curChar, m_characterSize, isBold);

			// Add the glyph to the vertices
			addGlyphQuad(m_vertices, Vector2f(x, y), props[i].fill_color, glyph, italicShear);

			//// Update the current bounds
			//float left = glyph.bounds.left;
			//float top = glyph.bounds.top;
			//float right = glyph.bounds.left + glyph.bounds.width;
			//float bottom = glyph.bounds.top + glyph.bounds.height;

			minX = std::min(minX, x + left - italicShear * bottom);
			maxX = std::max(maxX, x + right - italicShear * top);
			minY = std::min(minY, y + top);
			maxY = std::max(maxY, y + bottom);

			// Advance to the next character
			x += glyph.advance + letterSpacing;
		}

		// If we're using outline, update the current bounds
		if (m_outlineThickness != 0)
		{
			float outline = std::abs(std::ceil(m_outlineThickness));
			minX -= outline;
			maxX += outline;
			minY -= outline;
			maxY += outline;
		}

		// If we're using the underlined style, add the last line
		if (isUnderlined && (x > 0))
		{
			addLine(m_vertices, line_start, x, y, m_fillColor, underlineOffset, underlineThickness);

			if (m_outlineThickness != 0)
				addLine(m_outlineVertices, line_start, x, y, m_outlineColor, underlineOffset, underlineThickness, m_outlineThickness);
		}

		// If we're using the strike through style, add the last line across all characters
		if (isStrikeThrough && (x > 0))
		{
			addLine(m_vertices, line_start, x, y, m_fillColor, strikeThroughOffset, underlineThickness);

			if (m_outlineThickness != 0)
				addLine(m_outlineVertices, line_start, x, y, m_outlineColor, strikeThroughOffset, underlineThickness, m_outlineThickness);
		}

		// Update the bounding rectangle
		m_bounds.left = minX;
		m_bounds.top = minY;
		m_bounds.width = maxX - minX;
		m_bounds.height = maxY - minY;
	}

} // namespace sf
