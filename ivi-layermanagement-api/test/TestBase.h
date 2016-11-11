
#include "wayland-client.h"
#include <ivi-application-client-protocol.h>
#include <vector>

class TestBase
{
public:
    TestBase();
    virtual ~TestBase();
    void SetWLCompositor(struct wl_compositor* wlCompositor);
    void SetIviApp(struct ivi_application* iviApp);

protected:
    std::vector<wl_surface *> wlSurfaces;
    wl_display*    wlDisplay;
    ivi_application* iviApp;

private:
    wl_registry*   wlRegistry;
    wl_compositor* wlCompositor;
};

inline void TestBase::SetWLCompositor(struct wl_compositor* wl_compositor)
    { wlCompositor = wl_compositor; }
inline void TestBase::SetIviApp(struct ivi_application* ivi_application)
    { iviApp = ivi_application; }
