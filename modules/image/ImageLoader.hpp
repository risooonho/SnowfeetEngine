#ifndef __HEADER_SN_IMAGELOADER__
#define __HEADER_SN_IMAGELOADER__

#include <string>
#include <vector>

#include <core/types.h>
#include <core/math/Vector2.h>
#include <core/math/Color.h>
#include <core/asset/AssetLoader.h>

#include <modules/image/Image.h>

namespace sn
{

class SN_IMAGE_API ImageLoader : public AssetLoader
{
public:
    SN_OBJECT

    const ObjectType & getBaseAssetType() const override;
    bool canLoad(const AssetMetadata & meta) const override;
    bool isDirect(const AssetMetadata & meta) const override;
    bool load(std::ifstream & ifs, Asset & asset) const override;

private:
    static bool loadFromMemory(sn::Image & image, const u8 * data, u32 dataSize);

};

} // namespace sn

#endif // __HEADER_SN_IMAGELOADER__

