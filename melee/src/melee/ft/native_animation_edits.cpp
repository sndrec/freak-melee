#include "native_animation_edits.h"

#include <algorithm>
#include <cmath>

namespace NativeAnimationEdits {
    bool apply(NativeFighterProject& project, NativeFighterAnimationClip clip,
               const std::vector<Change>& requested,
               std::vector<Change>& applied, std::string& error)
    {
        const auto fail = [&](const char* message) {
            error = message;
            return false;
        };
        NativeFighterAnimationInfo info;
        if (requested.empty() || requested.size() > 32768 ||
            !nativeFighterAnimation_Info(&project, clip, &info))
        {
            return fail(
                "Choose an available clip and 1 to 32768 distinct keys.");
        }
        auto changes = requested;
        std::sort(
            changes.begin(), changes.end(), [](const auto& a, const auto& b) {
                return a.ref.track < b.ref.track ||
                       (a.ref.track == b.ref.track && a.ref.key < b.ref.key);
            });
        std::vector<NativeFighterAnimationKey> keys;
        std::vector<NativeFighterAnimationKeyTime> times;
        unsigned previous_track = UINT32_MAX, previous_key = UINT32_MAX;
        for (auto& change : changes) {
            if (change.ref.track >= info.tracks || !change.fields ||
                (change.fields & ~7u) ||
                (change.ref.track == previous_track &&
                 change.ref.key == previous_key))
            {
                return fail("Use each inspected track/key pair once and "
                            "supply its changed fields.");
            }
            if (previous_track != change.ref.track) {
                size_t count = 0;
                if (!nativeFighterAnimation_Keys(&project, clip,
                                                 change.ref.track, nullptr, 0,
                                                 &count) ||
                    count > 65536)
                {
                    return fail("Could not decode the selected key track.");
                }
                keys.resize(count);
                if (count && !nativeFighterAnimation_Keys(
                                 &project, clip, change.ref.track, keys.data(),
                                 count, &count))
                {
                    return fail("Could not decode the selected key track.");
                }
            }
            if (change.ref.key >= keys.size()) {
                return fail("A key index is outside its inspected track.");
            }
            const auto& original = keys[change.ref.key];
            if (!(change.fields & Time)) {
                change.frame = original.frame;
            }
            if (!(change.fields & Value)) {
                change.value = original.value;
            }
            if (!(change.fields & Tangent)) {
                change.tangent = original.tangent;
            }
            if ((change.fields & Time) &&
                (change.frame < 0 || change.frame > info.frames ||
                 change.frame > 65535))
            {
                return fail("Keep edited key frames inside the clip's "
                            "playback range.");
            }
            if (change.value != original.value &&
                (!original.editable || !std::isfinite(change.value) ||
                 !nativeFighterAnimation_Snap(&project, clip, change.ref.track,
                                              false, change.value,
                                              &change.value)))
            {
                return fail("A value is read-only or outside this track's "
                            "representable range.");
            }
            if (change.tangent != original.tangent &&
                (!original.tangent_editable ||
                 !std::isfinite(change.tangent) ||
                 !nativeFighterAnimation_Snap(&project, clip, change.ref.track,
                                              true, change.tangent,
                                              &change.tangent)))
            {
                return fail("A tangent is read-only or outside this track's "
                            "representable range.");
            }
            if (change.value == original.value) {
                change.fields &= ~Value;
            }
            if (change.tangent == original.tangent) {
                change.fields &= ~Tangent;
            }
            if (change.frame == original.frame) {
                change.fields &= ~Time;
            }
            if (change.frame != original.frame) {
                times.push_back({ change.ref, change.frame });
            }
            previous_track = change.ref.track;
            previous_key = change.ref.key;
        }
        if (!times.empty() &&
            !nativeFighterAnimation_EditKeys(&project, clip, times.data(),
                                             times.size(), false))
        {
            return fail(
                "The selected keys would cross their neighbours, merge "
                "distinct frames, or exceed the source encoding.");
        }
        for (const auto& change : changes) {
            if ((change.fields & Value) &&
                !nativeFighterAnimation_SetKey(&project, clip,
                                               change.ref.track,
                                               change.ref.key, change.value))
            {
                return fail("Could not write a selected key value.");
            }
            if ((change.fields & Tangent) &&
                !nativeFighterAnimation_SetTangent(
                    &project, clip, change.ref.track, change.ref.key,
                    change.tangent))
            {
                return fail("Could not write a selected key tangent.");
            }
        }
        applied = std::move(changes);
        error.clear();
        return true;
    }
}
