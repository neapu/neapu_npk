#include "NPKImageHandler.h"

#include <NPKHandler.h>

using namespace neapu;

int main()
{
    std::shared_ptr<NPKHandler> npkHandler = std::make_shared<NPKHandler>();
    if (!npkHandler->loadNPK("D:\\code\\sprite_map_cutscene.NPK")) {
        return -1;
    }

    auto img = npkHandler->getImage(0);
    if (img == nullptr) {
        return -1;
    }

    auto png = img->getFramePngData(0,0);
    if (png.empty()) {
        return -1;
    }

    return 0;
}