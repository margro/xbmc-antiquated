/*!
\file GUIFont.h
\brief
*/

#ifndef CGUILIB_GUIFONTTTF_H
#define CGUILIB_GUIFONTTTF_H
#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

// forward definition
class CBaseTexture;

struct FT_FaceRec_;
struct FT_LibraryRec_;
struct FT_GlyphSlotRec_;
struct FT_BitmapGlyphRec_;

typedef struct FT_FaceRec_ *FT_Face;
typedef struct FT_LibraryRec_ *FT_Library;
typedef struct FT_GlyphSlotRec_ *FT_GlyphSlot;
typedef struct FT_BitmapGlyphRec_ *FT_BitmapGlyph;

typedef uint32_t character_t;
typedef uint32_t color_t;
typedef std::vector<character_t> vecText;
typedef std::vector<color_t> vecColors;

/*!
 \ingroup textures
 \brief
 */

typedef struct _SVertex
{
  float u, v;
  unsigned char r, g, b, a;    
  float x, y, z;
} SVertex;


class CGUIFontTTFBase
{
  friend class CGUIFont;
  
public:

  CGUIFontTTFBase(const CStdString& strFileName);
  virtual ~CGUIFontTTFBase(void);

  void Clear();

  bool Load(const CStdString& strFilename, float height = 20.0f, float aspect = 1.0f, float lineSpacing = 1.0f);

  virtual void Begin() = 0;
  virtual void End() = 0;

  const CStdString& GetFileName() const { return m_strFileName; };

protected:
  struct Character
  {
    short offsetX, offsetY;
    float left, top, right, bottom;
    float advance;
    character_t letterAndStyle;
  };
  void AddReference();
  void RemoveReference();

  float GetTextWidthInternal(vecText::const_iterator start, vecText::const_iterator end);
  float GetCharWidthInternal(character_t ch);
  float GetTextHeight(float lineSpacing, int numLines) const;
  float GetLineHeight(float lineSpacing) const;

  void DrawTextInternal(float x, float y, const vecColors &colors, const vecText &text,
                            uint32_t alignment, float maxPixelWidth, bool scrolling);

  float m_height;
  CStdString m_strFilename;

  // Stuff for pre-rendering for speed
  inline Character *GetCharacter(character_t letter);
  bool CacheCharacter(wchar_t letter, uint32_t style, Character *ch);
  void RenderCharacter(float posX, float posY, const Character *ch, color_t color, bool roundX);
  void ClearCharacterCache();

  virtual CBaseTexture* ReallocTexture(unsigned int& newHeight) = 0;
  virtual bool CopyCharToTexture(FT_BitmapGlyph bitGlyph, Character *ch) = 0;
  virtual void DeleteHardwareTexture() = 0;
  virtual void RenderInternal(SVertex* v) = 0;

  // modifying glyphs
  void EmboldenGlyph(FT_GlyphSlot slot);
  void ObliqueGlyph(FT_GlyphSlot slot);

  CBaseTexture* m_texture;        // texture that holds our rendered characters (8bit alpha only)

  unsigned int m_textureWidth;       // width of our texture
  unsigned int m_textureHeight;      // heigth of our texture
  int m_posX;                        // current position in the texture
  int m_posY;

  color_t m_color;

  Character *m_char;                 // our characters
  Character *m_charquick[256*4];     // ascii chars (4 styles) here
  int m_maxChars;                    // size of character array (can be incremented)
  int m_numChars;                    // the current number of cached characters

  float m_ellipsesWidth;               // this is used every character (width of '.')

  unsigned int m_cellBaseLine;
  unsigned int m_cellHeight;

  unsigned int m_nestedBeginCount;             // speedups

  // freetype stuff
  FT_Face    m_face;

  float m_originX;
  float m_originY;

  bool m_bTextureLoaded;
  unsigned int m_nTexture;

  SVertex* m_vertex;
  int      m_vertex_count;
  int      m_vertex_size;

  float    m_textureScaleX;
  float    m_textureScaleY;

  static int justification_word_weight;

  CStdString m_strFileName;

private:
  int m_referenceCount;
};

#if defined(HAS_GL)
#include "GUIFontTTFGL.h"
#define CGUIFontTTF CGUIFontTTFGL
#elif defined(HAS_GLES)
#include "GUIFontTTFGLES.h"
#define CGUIFontTTF CGUIFontTTFGLES
#elif defined(HAS_DX)
#include "GUIFontTTFDX.h"
#define CGUIFontTTF CGUIFontTTFDX
#endif

#endif
