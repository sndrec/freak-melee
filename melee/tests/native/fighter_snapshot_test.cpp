#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ft/native_fighter_snapshot.h>
#include <cstdio>
#include <cstring>
#include <string>

#define REQUIRE(x) do { if (!(x)) { std::fprintf(stderr, "FAIL line %u: %s (%s)\n", __LINE__, #x, error.c_str()); return 1; } } while (0)

int main(int argc, char** argv)
{
    std::string error;
    REQUIRE(argc == 2);
    NativeDraftAccess::Lease lease;
    REQUIRE(lease.acquire(argv[1], error));
    NativeFighterFamilyProject family{};
    REQUIRE(nativeFighterFamilyProject_Open(argv[1], &family));
    NativeFighterSources::Session sessions[NATIVE_FIGHTER_FAMILY_MEMBERS];
    NativeFighterSources::Session* sources[NATIVE_FIGHTER_FAMILY_MEMBERS]{};
    double expected[NATIVE_FIGHTER_FAMILY_MEMBERS]{};
    for (unsigned i = 0; i < family.member_count; ++i) {
        sources[i] = &sessions[i];
        REQUIRE(NativeFighterSources::open_project(sessions[i], family.paths[i], family.members[i]));
        REQUIRE(NativeFighterSources::import_parameters(sessions[i], family.members[i]));
        sessions[i].files[0].text += "\n# whole family transaction\n";
        sessions[i].parameters += "\n# saved alongside source and assets\n";
        double original;
        REQUIRE(nativeFighterProject_GetAttribute(&family.members[i], 0, &original));
        REQUIRE(nativeFighterProject_SetAttribute(&family.members[i], 0, original + 1));
        REQUIRE(nativeFighterProject_GetAttribute(&family.members[i], 0, &expected[i]));
    }
    std::string before, after;
    REQUIRE(NativeDraftAccess::revision(argv[1], before, error));
    auto unchanged = [&]() {
        std::string actual;
        return NativeDraftAccess::revision(argv[1], actual, error) && actual == before;
    };
    // A bad last member must prevent publishing the valid first member too.
    auto& last = family.members[family.member_count - 1];
    last.asset_record[0] ^= 1;
    REQUIRE(!NativeFighterSnapshot::save(family, sources, before, after, error));
    last.asset_record[0] ^= 1;
    REQUIRE(unchanged());
    REQUIRE(NativeFighterSources::dirty(sessions[0]));
    // Deny replacement partway through publishing. Release the handle and
    // recover the journal, as a later exclusive open would do after a crash.
    const std::string blocked = std::string(family.paths[family.member_count - 1]) + "/animation.dat";
    HANDLE file = CreateFileA(blocked.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    REQUIRE(file != INVALID_HANDLE_VALUE);
    bool saved = NativeFighterSnapshot::save(family, sources, before, after, error);
    CloseHandle(file);
    REQUIRE(!saved);
    REQUIRE(NativeDraftAccess::recover(argv[1], error));
    REQUIRE(unchanged());
    REQUIRE(NativeFighterSources::dirty(sessions[0]));
    REQUIRE(NativeFighterSnapshot::save(family, sources, before, after, error));
    REQUIRE(after != before);
    NativeFighterFamilyProject disk{};
    REQUIRE(nativeFighterFamilyProject_Open(argv[1], &disk));
    for (unsigned i = 0; i < family.member_count; ++i) {
        double current;
        REQUIRE(nativeFighterProject_GetAttribute(&disk.members[i], 0, &current) && current == expected[i]);
        REQUIRE(!NativeFighterSources::dirty(sessions[i]));
        REQUIRE(disk.members[i].parameter_size == sessions[i].parameters.size());
        REQUIRE(!std::memcmp(disk.members[i].parameter_text, sessions[i].parameters.data(), sessions[i].parameters.size()));
        NativeFighterSources::Session reopened;
        REQUIRE(NativeFighterSources::open_project(reopened, family.paths[i], disk.members[i]));
        REQUIRE(reopened.files[0].text == sessions[i].files[0].text);
    }
    nativeFighterFamilyProject_Close(&disk);
    // An install receipt changes independently of the saved authoring state.
    const std::string receipt = std::string(argv[1]) + "/installed.id";
    FILE* installed = std::fopen(receipt.c_str(), "wb");
    REQUIRE(installed);
    REQUIRE(std::fputs((std::string(64, 'a') + '\n').c_str(), installed) >= 0);
    REQUIRE(!std::fclose(installed));
    std::string installed_revision;
    REQUIRE(NativeDraftAccess::revision(argv[1], installed_revision, error) && installed_revision == after);
    REQUIRE(NativeFighterSnapshot::save(family, sources, after, installed_revision, error));
    // An independent disk edit is preserved, even while the in-memory source
    // has another unsaved change and its caller supplies the previous revision.
    sessions[0].files[0].text += "# keep me unsaved\n";
    const std::string outside = std::string(argv[1]) + "/outside.txt";
    FILE* external = std::fopen(outside.c_str(), "wb");
    REQUIRE(external);
    REQUIRE(std::fwrite("external edit", 1, 13, external) == 13);
    REQUIRE(!std::fclose(external));
    std::string changed;
    REQUIRE(NativeDraftAccess::revision(argv[1], changed, error));
    REQUIRE(!NativeFighterSnapshot::save(family, sources, after, before, error));
    REQUIRE(NativeFighterSources::dirty(sessions[0]));
    std::string retained;
    REQUIRE(NativeDraftAccess::revision(argv[1], retained, error) && retained == changed);
    nativeFighterFamilyProject_Close(&family);
    std::puts("PASS: whole-family source/asset save, failed-candidate isolation, interrupted-publish recovery and disk conflicts");
    return 0;
}
