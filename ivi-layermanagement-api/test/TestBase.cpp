#include "TestBase.h"
#include <cstring>

void registry_listener_callback(void* data, struct wl_registry* registry, uint32_t id, const char* interface, uint32_t version)
{
    if (0 == strcmp(interface, "wl_compositor"))
    {
        wl_compositor** compositor = reinterpret_cast<wl_compositor**>(data);
        *compositor = reinterpret_cast<wl_compositor*>(wl_registry_bind(registry, id, &wl_compositor_interface, 1));
    }
}

TestBase::TestBase()
: wlSurface(NULL)
, wlDisplay(NULL)
, wlRegistry(NULL)
{
    wlDisplay = wl_display_connect(NULL);
    wlRegistry = wl_display_get_registry(wlDisplay);

    static const struct wl_registry_listener registry_listener = {
        registry_listener_callback,
        NULL
    };

    wl_registry_add_listener(wlRegistry, &registry_listener, &wlCompositor);
    wl_display_dispatch(wlDisplay);
    wl_display_roundtrip(wlDisplay);

    wlSurface = wl_compositor_create_surface(wlCompositor);
}

TestBase::~TestBase()
{
    wl_surface_destroy(wlSurface);
    wl_compositor_destroy(wlCompositor);
    wl_registry_destroy(wlRegistry);
    wl_display_disconnect(wlDisplay);
}

