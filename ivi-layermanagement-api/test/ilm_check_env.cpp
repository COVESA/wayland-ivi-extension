/***************************************************************************
 *
 * Copyright (C) 2023 Advanced Driver Information Technology Joint Venture GmbH
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

#include <gtest/gtest.h>
#include <wayland-client.h>
#include "ivi-input-client-protocol.h"
#include "ivi-wm-client-protocol.h"

class TestEnvChecking
{
  public:
    struct wl_display  *mpDisplay;
    struct wl_registry *mpRegistry;

    struct ivi_wm *mpController;
    struct ivi_input *mpInputController;
    bool mCheck;

    static constexpr uint32_t IVI_CONTROLLER_VERSION{2U};
    static constexpr uint32_t IVI_INPUT_VERSION{2U};

    static TestEnvChecking *GetInstance();
    ~TestEnvChecking();

  private:
    static void RegistryHandleGlobal(void *apData, struct wl_registry *apRegistry,
        uint32_t aId,
        const char *acpInterface,
        uint32_t aVersion);

    static constexpr struct wl_registry_listener mRegistryListener = {
      RegistryHandleGlobal,
      nullptr
    };

    static TestEnvChecking *mpInstance;
    TestEnvChecking();
};

TestEnvChecking* TestEnvChecking::mpInstance{nullptr};

TestEnvChecking::TestEnvChecking(): mpDisplay(nullptr), mpRegistry(nullptr),
    mpController(nullptr),
    mpInputController(nullptr),
    mCheck(true)
{
  int ret;

  mpDisplay = wl_display_connect(nullptr);
  if (mpDisplay == nullptr) {
    printf("ERROR: Failed to wl_display_connect()\n");
    mCheck = false;
    return;
  }

  mpRegistry = wl_display_get_registry(mpDisplay);
  if (mpRegistry == nullptr) {
    printf("ERROR: Failed to wl_display_get_registry()\n");
    mCheck = false;
    return;
  }

  ret = wl_registry_add_listener(mpRegistry, &mRegistryListener, this);
  if (ret < 0) {
    printf("ERROR: Failed to wl_registry_add_listener()\n");
    mCheck = false;
    return;
  }

  ret = wl_display_roundtrip(mpDisplay);
  if (ret < 0) {
    printf("ERROR: Failed to wl_display_roundtrip()\n");
    mCheck = false;
    return;
  }

  if ((mpController == nullptr) || (mpInputController == nullptr)) {
    printf("ERROR: can't get the ivi_wm (%lX) or ivi_input (%lX) protocols from compositor.\n",
        reinterpret_cast<uintptr_t>(mpController),
        reinterpret_cast<uintptr_t>(mpInputController));
    mCheck = false;
    return;
  }
}

TestEnvChecking::~TestEnvChecking()
{
  if (!mpInputController) {
    ivi_input_destroy(mpInputController);
  }

  if (!mpController) {
    ivi_wm_destroy(mpController);
  }

  if (!mpRegistry) {
    wl_registry_destroy(mpRegistry);
  }

  if (!mpDisplay) {
    wl_display_disconnect(mpDisplay);
  }
}

void TestEnvChecking::RegistryHandleGlobal(void *apData, struct wl_registry *apRegistry,
    uint32_t aId,
    const char *acpInterface,
    uint32_t aVersion)
{
  TestEnvChecking *lpTestEnvChecking = reinterpret_cast<TestEnvChecking*>(apData);

  if (0 == strcmp(acpInterface, "ivi_wm")) {
    if (IVI_CONTROLLER_VERSION != aVersion) {
      printf("ERROR: unexpected ivi_wm version from server. Expected: %d, got: %d\n",
          IVI_CONTROLLER_VERSION,
          aVersion);
      lpTestEnvChecking->mCheck = false;
      return;
    }

    lpTestEnvChecking->mpController = reinterpret_cast<struct ivi_wm*>
        (wl_registry_bind(apRegistry, aId, &ivi_wm_interface, IVI_CONTROLLER_VERSION));
    if (lpTestEnvChecking->mpController == nullptr) {
      printf("ERROR: Failed to wl_registry_bind ivi_wm\n");
      lpTestEnvChecking->mCheck = false;
      return;
    }
  }
  else if (0 == strcmp(acpInterface, "ivi_input")) {
    if (IVI_INPUT_VERSION != aVersion) {
      printf("ERROR: unexpected ivi_input version from server. Expected: %d, got: %d\n",
          IVI_INPUT_VERSION,
          aVersion);
      lpTestEnvChecking->mCheck = false;
      return;
    }

    lpTestEnvChecking->mpInputController = reinterpret_cast<struct ivi_input*>
        (wl_registry_bind(apRegistry, aId, &ivi_input_interface, IVI_INPUT_VERSION));
    if (lpTestEnvChecking->mpInputController == nullptr) {
      printf("ERROR: Failed to wl_registry_bind ivi_input\n");
      lpTestEnvChecking->mCheck = false;
      return;
    }
  }
}

TestEnvChecking *TestEnvChecking::GetInstance()
{
  if (mpInstance == nullptr) {
    mpInstance = new TestEnvChecking();
  }
  return mpInstance;
}

TEST(ilm_test, env_checking)
{
  TestEnvChecking *lpTestEnvChecking = TestEnvChecking::GetInstance();
  ASSERT_TRUE(lpTestEnvChecking->mCheck);
}
