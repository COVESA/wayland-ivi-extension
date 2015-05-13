
#include "wayland-client.h"
#include <vector>

class TestBase
{
public:
    TestBase();
    virtual ~TestBase();

protected:
    std::vector<wl_surface *> wlSurfaces;
    wl_display*    wlDisplay;

private:
    wl_registry*   wlRegistry;
    wl_compositor* wlCompositor;
};
