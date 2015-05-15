#ifndef __HEADER_TGUI_DRAWBATCH__
#define __HEADER_TGUI_DRAWBATCH__

#include <core/scene/base/IDrawContext.hpp>
#include <core/asset/base/Mesh.hpp>
#include <core/asset/base/Material.hpp>
#include <core/asset/base/Font.hpp>
#include <core/math/Color.hpp>

#include "Border.h"
#include "TextAlignment.h"

namespace tgui
{

class DrawBatch
{
public:
    DrawBatch(sn::IDrawContext & dc);
    ~DrawBatch();

    void setMaterial(sn::Material & m);
    void flush();
    
    void fillRect(
        const sn::IntRect & r, 
        const sn::IntRect & texRect, 
        sn::Vector2u ts, 
        sn::Color color = sn::Color(1,1,1,1)
    );
    
    void fillNineSlices(
        const sn::IntRect & r, 
        const Border & b, 
        const sn::IntRect & texRect, 
        sn::Vector2u ts
    );
    
    void drawTextLine(
        const char * str,
        sn::u32 charCount,
        sn::IntRect area, 
        const sn::Font & font, 
        sn::FontFormat format, 
        TextAlignment align = TGUI_ALIGN_LEFT,
        sn::Color color = sn::Color(1,1,1,1)
    );

    void setScissor(sn::IntRect rect, sn::u32 windowID);
    void disableScissor() { m_dc.disableScissor(); }

private:
    void setTexture(sn::TextureBase * tex);
    sn::TextureBase * getTexture() const;

private:
    sn::Mesh * m_mesh;
    sn::IDrawContext & m_dc;
    sn::Material * r_material;

};

} // namespace tgui

#endif // __HEADER_TGUI_DRAWBATCH__
