
#include "wayland-client.h"

class TestBase
{
public:
    TestBase();
    virtual ~TestBase();

protected:
    wl_surface*    wlSurface;
    wl_display*    wlDisplay;

private:
    wl_registry*   wlRegistry;
    wl_compositor* wlCompositor;
};
