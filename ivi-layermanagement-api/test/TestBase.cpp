/***************************************************************************
 *
 * Copyright 2012 BMW Car IT GmbH
 * Copyright (C) 2016 Advanced Driver Information Technology Joint Venture GmbH
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/

#include "TestBase.h"
#include <cstring>
#include <stdexcept>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>

int
create_file(int size)
{
    static const char base_string[] = "/weston-shared-XXXXXX";
    const char *path;
    char *name;
    int fd;
    int ret;
    long flags;

    path = getenv("XDG_RUNTIME_DIR");
    if (!path) {
        errno = ENOENT;
        return -1;
    }

    name = (char*) malloc(strlen(path) + sizeof(base_string));
    if (!name)
        return -1;

    strcpy(name, path);
    strcat(name, base_string);

    fd = mkstemp(name);
    if (fd >= 0) {
        flags = fcntl(fd, F_GETFD);
        fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
        unlink(name);
    }

    free(name);

    if (fd < 0)
        return -1;

    ret = ftruncate(fd, size);
    if (ret < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

static void
shm_format(void *data, struct wl_shm *wl_shm, uint32_t format)
{
    TestBase* base = static_cast<TestBase*>(data);

    base->SetShmFormats(format);
}

static struct wl_shm_listener shm_listener = {
    shm_format
};

void registry_listener_callback(void* data, struct wl_registry* registry, uint32_t id, const char* interface, uint32_t version)
{
    TestBase* base = static_cast<TestBase*>(data);
    struct wl_shm *shm;

    if (0 == strcmp(interface, "wl_compositor"))
    {
        base->SetWLCompositor(reinterpret_cast<wl_compositor*>(wl_registry_bind(registry, id, &wl_compositor_interface, 1)));
    }

    if (0 == strcmp(interface, "wl_shm"))
    {
        shm = (struct wl_shm*) wl_registry_bind(registry, id, &wl_shm_interface, 1);
        wl_shm_add_listener(shm, &shm_listener, base);
        base->SetShm(shm);
    }

    if (0 == strcmp(interface, "ivi_application"))
    {
        base->SetIviApp(reinterpret_cast<ivi_application*>(wl_registry_bind(registry, id, &ivi_application_interface, 1)));

    }
}

TestBase::TestBase()
: wlDisplay(NULL)
, wlRegistry(NULL)
{
    int fd = -1;
    int size = 0;
    struct wl_shm_pool *pool;
    void* mmapData;

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

    shmFormats = 0;

    wl_registry_add_listener(wlRegistry, &registry_listener, this);

    if (wl_display_roundtrip(wlDisplay) == -1 || wl_display_roundtrip(wlDisplay) == -1)
    {
        throw std::runtime_error("wl_display error");
    }

    if (!wlShm)
    {
        throw std::runtime_error("shared memory buffers are not supported");
    }

    if (wl_display_roundtrip(wlDisplay) == -1)
    {
        throw std::runtime_error("wl_display error");
    }

    if (!(shmFormats & (1 << WL_SHM_FORMAT_ARGB8888)))
    {
        throw std::runtime_error("shared memory buffers format ARGB888 is not supported");
    }

    wlSurfaces.reserve(10);
    for (int i = 0; i < (int)wlSurfaces.capacity(); ++i)
    {
        wlSurfaces.push_back(wl_compositor_create_surface(wlCompositor));
    }

    // size = height * width * bpp * number_of_surfaces = 1 * 1 * 4 * 10
    size = 4 * wlSurfaces.capacity();
    fd = create_file(size);
    if (fd < 0)
    {
        throw std::runtime_error("cannot create shared memory file");
    }

    mmapData = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (MAP_FAILED == mmapData) {
        close(fd);
        throw std::runtime_error("mmap failed");
    }

    pool = wl_shm_create_pool(wlShm, fd, size);
    if (!pool)
    {
        throw std::runtime_error("wl_shm_create_pool error");
    }

    wlBuffers.reserve(10);
    for (int i = 0; i < (int)wlBuffers.capacity(); ++i)
    {
        wlBuffers.push_back(wl_shm_pool_create_buffer(pool, 0, 1, 1, 4, WL_SHM_FORMAT_ARGB8888));
        wl_surface_attach(wlSurfaces[i], wlBuffers[i], 0, 0);
        wl_surface_damage(wlSurfaces[i], 0, 0, 1, 1);
        wl_surface_commit(wlSurfaces[i]);
        wl_display_flush(wlDisplay);
    }

    munmap(mmapData, size);
    wl_shm_pool_destroy(pool);
    close(fd);
}

TestBase::~TestBase()
{
    for (std::vector<wl_buffer *>::reverse_iterator it = wlBuffers.rbegin();
         it != wlBuffers.rend();
         ++it)
    {
        wl_buffer_destroy(*it);
    }
    wlBuffers.clear();

    for (std::vector<wl_surface *>::reverse_iterator it = wlSurfaces.rbegin();
         it != wlSurfaces.rend();
         ++it)
    {
        wl_surface_destroy(*it);
    }
    wlSurfaces.clear();
    wl_shm_destroy(wlShm);
    wl_compositor_destroy(wlCompositor);
    ivi_application_destroy(iviApp);
    wl_registry_destroy(wlRegistry);
    wl_display_disconnect(wlDisplay);
}

