/*
 * Copyright (C) 2018-2020 The LineageOS Project
 * Copyright (C) 2020      Andreas Schneider <asn@cryptomilk.org>
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "power@1.0-exynos9820"

#include <android-base/logging.h>

#include <android-base/file.h>
#include <android-base/properties.h>
#include <android-base/strings.h>

#include "Power.h"

#include <iostream>

namespace android {
namespace hardware {
namespace power {
namespace V1_0 {
namespace implementation {

using ::android::hardware::power::V1_0::Feature;
using ::android::hardware::power::V1_0::PowerHint;
using ::android::hardware::power::V1_0::PowerStatePlatformSleepState;
using ::android::hardware::power::V1_0::Status;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

static std::string interactive_node_paths[] = {
    "/sys/class/sec/tsp/input/enabled",
    "/sys/class/sec/tsp1/input/enabled",
    "/sys/class/sec/tsp2/input/enabled",
    "/sys/class/sec/sec_epen/input/enabled",
    "/sys/class/sec/sec_touchkey/input/enabled",
    "/sys/class/sec/sec_sidekey/input/enabled",
    "/sys/class/power_supply/battery/lcd"
};

template <typename T>
static void writeNode(const std::string& path, const T& value)
{
    std::ofstream node(path);
    if (!node) {
        PLOG(ERROR) << "Failed to open: " << path;
        return;
    }

    LOG(DEBUG) << "writeNode node: " << path << " value: " << value;

    node << value << std::endl;
    if (!node) {
        PLOG(ERROR) << "Failed to write: " << value;
    }
}

static bool doesNodeExist(const std::string& path)
{
    std::ifstream f(path.c_str());
    if (f.good()) {
        LOG(DEBUG) << "Found node: " << path;
    }
    return f.good();
}

Power::Power() {
    mInteractionHandler.Init();
    size_t inode_size = ARRAY_SIZE(interactive_node_paths);

    //android::base::SetMinimumLogSeverity(android::base::VERBOSE);

    LOG(DEBUG) << "Looking for touchsceen/lcd nodes";
    for (size_t i = 0; i < inode_size; i++) {
        std::string node_path = interactive_node_paths[i];

        if (doesNodeExist(node_path)) {
            interactiveNodes.push_back(node_path);
        }
    }
}

// Methods from ::android::hardware::power::V1_0::IPower follow.
Return<void> Power::setInteractive(bool interactive) {
    writeNode("/sys/power/cpufreq_max_limit",
              interactive ? "2730000" : "1560000");

    for (size_t i = 0; i < interactiveNodes.size(); i++) {
        writeNode(interactiveNodes[i], interactive ? "1" : "0");
    }

    if (mDoubleTapEnabled) {
        /* Enable double tap to wake */
        writeNode("/sys/class/sec/tsp/cmd", "aot_enable,1");
    }

    return Void();
}

Return<void> Power::powerHint(PowerHint hint, int32_t data) {
    switch (hint) {
    case PowerHint::INTERACTION:
        mInteractionHandler.Acquire(data);
        break;
    default:
        return Void();
    }
    return Void();
}

Return<void> Power::setFeature(Feature feature, bool activate)
{
    switch (feature) {
    case Feature::POWER_FEATURE_DOUBLE_TAP_TO_WAKE:
        if (activate) {
            LOG(INFO) << "Enable double tap to wake";
            mDoubleTapEnabled = true;
        } else {
            LOG(INFO) << "Disable double tap to wake";
            mDoubleTapEnabled = false;
        }
        break;
    default:
        break;
    }

    return Void();
}

Return<void> Power::getPlatformLowPowerStats(getPlatformLowPowerStats_cb _hidl_cb) {
    hidl_vec<PowerStatePlatformSleepState> states;
    _hidl_cb(states, Status::SUCCESS);
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace power
}  // namespace hardware
}  // namespace android
