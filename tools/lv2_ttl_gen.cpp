// Tiny helper that writes the LV2 turtle files (manifest.ttl, <name>.ttl, presets.ttl) for a plugin.
// JUCE 6's LV2 wrapper exports lv2_generate_ttl(); the Projucer used to call it after the build.
// Usage: lv2_ttl_gen <plugin library> <binary base name>    (run inside the .lv2 bundle folder)
#include <cstdio>
#ifdef _WIN32
 #include <windows.h>
 static void* openLib (const char* p) { return (void*) LoadLibraryA (p); }
 static void* findSym (void* h, const char* n) { return (void*) GetProcAddress ((HMODULE) h, n); }
#else
 #include <dlfcn.h>
 static void* openLib (const char* p) { return dlopen (p, RTLD_NOW); }
 static void* findSym (void* h, const char* n) { return dlsym (h, n); }
#endif

int main (int argc, char** argv)
{
    if (argc != 3) { std::fprintf (stderr, "usage: %s <library> <basename>\n", argv[0]); return 2; }
    void* h = openLib (argv[1]);
    if (! h) { std::fprintf (stderr, "cannot load %s\n", argv[1]); return 1; }
    auto fn = (void (*) (const char*)) findSym (h, "lv2_generate_ttl");
    if (! fn) { std::fprintf (stderr, "lv2_generate_ttl not found\n"); return 1; }
    fn (argv[2]);
    return 0;
}
