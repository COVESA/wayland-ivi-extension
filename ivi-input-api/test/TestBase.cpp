#include "TestBase.h"
#include <cstring>
#include <stdexcept>

void registry_listener_callback(void* data, struct wl_registry* registry, uint32_t id, const char* interface, uint32_t version)
{
    if (0 == strcmp(interface, "wl_compositor"))
    {
        wl_compositor** compositor = reinterpret_cast<wl_compositor**>(data);
        *compositor = reinterpret_cast<wl_compositor*>(wl_registry_bind(registry, id, &wl_compositor_interface, 1));
    }
}

TestBase::TestBase()
: wlDisplay(NULL)
, wlRegistry(NULL)
{
    wlDisplay = wl_display_connect(NULL);
    if (!wlDisplay)
    {
        throw std::runtime_error("could not connect to wayland display");
    }
    wlRegistry = wl_display_get_registry(wlDisplay);

    static const struct wl_registry_listener registry_listener = {
        registry_listener_callback,
        NULL
    };

    wl_registry_add_listener(wlRegistry, &registry_listener, &wlCompositor);

    if (wl_display_roundtrip(wlDisplay) == -1 || wl_display_roundtrip(wlDisplay) == -1)
    {
        throw std::runtime_error("wl_display error");
    }

    wlSurfaces.reserve(10);
    for (int i = 0; i < (int)wlSurfaces.capacity(); ++i)
    {
        wlSurfaces.push_back(wl_compositor_create_surface(wlCompositor));
    }
}

TestBase::~TestBase()
{
    for (std::vector<wl_surface *>::reverse_iterator it = wlSurfaces.rbegin();
         it != wlSurfaces.rend();
         ++it)
    {
        wl_surface_destroy(*it);
    }
    wlSurfaces.clear();
    wl_compositor_destroy(wlCompositor);
    wl_registry_destroy(wlRegistry);
    wl_display_disconnect(wlDisplay);
}
