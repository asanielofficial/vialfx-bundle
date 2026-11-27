// Minimal LV2 host: loads a plugin through lilv, feeds it a sine and checks that audio comes out.
// Build: g++ -std=c++17 tests/lv2_host_test.cpp $(pkg-config --cflags --libs lilv-0 x11) -o lv2_host_test
// Run:   LV2_PATH=<dir containing the .lv2 bundles> ./lv2_host_test urn:vialfx:Reverb
#include <lilv/lilv.h>
#include <lv2/ui/ui.h>
#include <lv2/instance-access/instance-access.h>
#include <X11/Xlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <lv2/urid/urid.h>
#include <lv2/options/options.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/atom/atom.h>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

static std::map<std::string, LV2_URID> g_uris;
static std::vector<std::string> g_names;
static LV2_URID mapUri (LV2_URID_Map_Handle, const char* uri)
{
    auto it = g_uris.find (uri);
    if (it != g_uris.end()) return it->second;
    g_names.push_back (uri);
    return g_uris[uri] = (LV2_URID) g_names.size();
}
static const char* unmapUri (LV2_URID_Unmap_Handle, LV2_URID urid) { return urid >= 1 && urid <= g_names.size() ? g_names[urid - 1].c_str() : nullptr; }

int main (int argc, char** argv)
{
    if (argc < 2) { std::printf ("usage: %s <plugin uri>\n", argv[0]); return 2; }
    LilvWorld* world = lilv_world_new();
    lilv_world_load_all (world);
    LilvNode* uri = lilv_new_uri (world, argv[1]);
    const LilvPlugin* plugin = lilv_plugins_get_by_uri (lilv_world_get_all_plugins (world), uri);
    if (! plugin) { std::printf ("FAIL: plugin not found\n"); return 1; }

    LV2_URID_Map map { nullptr, mapUri };
    LV2_URID_Unmap unmap { nullptr, unmapUri };
    LV2_Feature fMap { LV2_URID__map, &map }, fUnmap { LV2_URID__unmap, &unmap };
    const int blockSize = 512;
    int minB = 1, maxB = blockSize, seqSize = 8192; float sr = 48000.f;
    LV2_Options_Option opts[] = {
        { LV2_OPTIONS_INSTANCE, 0, mapUri (nullptr, LV2_BUF_SIZE__minBlockLength), sizeof (int), mapUri (nullptr, LV2_ATOM__Int), &minB },
        { LV2_OPTIONS_INSTANCE, 0, mapUri (nullptr, LV2_BUF_SIZE__maxBlockLength), sizeof (int), mapUri (nullptr, LV2_ATOM__Int), &maxB },
        { LV2_OPTIONS_INSTANCE, 0, mapUri (nullptr, LV2_BUF_SIZE__sequenceSize), sizeof (int), mapUri (nullptr, LV2_ATOM__Int), &seqSize },
        { LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, nullptr } };
    LV2_Feature fOpts { LV2_OPTIONS__options, opts }, fBounded { LV2_BUF_SIZE__boundedBlockLength, nullptr };
    const LV2_Feature* features[] = { &fMap, &fUnmap, &fOpts, &fBounded, nullptr };

    LilvInstance* inst = lilv_plugin_instantiate (plugin, sr, features);
    if (! inst) { std::printf ("FAIL: instantiate\n"); return 1; }

    const uint32_t numPorts = lilv_plugin_get_num_ports (plugin);
    std::vector<float> ctl (numPorts, 0.f), inL (blockSize), inR (blockSize), outL (blockSize), outR (blockSize);
    std::vector<std::vector<char>> atoms;
    LilvNode* audioC = lilv_new_uri (world, LILV_URI_AUDIO_PORT);
    LilvNode* inputC = lilv_new_uri (world, LILV_URI_INPUT_PORT);
    LilvNode* atomC = lilv_new_uri (world, LV2_ATOM__AtomPort);
    int ai = 0, ao = 0, nCtl = 0, nAtom = 0;
    for (uint32_t i = 0; i < numPorts; ++i)
    {
        const LilvPort* p = lilv_plugin_get_port_by_index (plugin, i);
        const bool isIn = lilv_port_is_a (plugin, p, inputC);
        if (lilv_port_is_a (plugin, p, audioC))
            lilv_instance_connect_port (inst, i, isIn ? (ai++ == 0 ? inL.data() : inR.data()) : (ao++ == 0 ? outL.data() : outR.data()));
        else if (lilv_port_is_a (plugin, p, atomC))
        {   // empty atom sequence in / scratch buffer out
            atoms.emplace_back (seqSize, 0);
            auto* seq = (LV2_Atom_Sequence*) atoms.back().data();
            seq->atom.type = mapUri (nullptr, LV2_ATOM__Sequence);
            seq->atom.size = sizeof (LV2_Atom_Sequence_Body);
            if (! isIn) seq->atom.size = seqSize - sizeof (LV2_Atom);
            lilv_instance_connect_port (inst, i, seq); ++nAtom;
        }
        else
        {
            LilvNode *def, *mn, *mx; lilv_port_get_range (plugin, p, &def, &mn, &mx);
            ctl[i] = def ? lilv_node_as_float (def) : 0.f;
            lilv_instance_connect_port (inst, i, &ctl[i]); ++nCtl;
        }
    }
    std::printf ("ports: %u (audio in %d, out %d, control %d, atom %d)\n", numPorts, ai, ao, nCtl, nAtom);

    lilv_instance_activate (inst);
    double energyIn = 0, energyOut = 0; bool finite = true; double phase = 0;
    for (int b = 0; b < 100; ++b)
    {
        for (int i = 0; i < blockSize; ++i) { inL[i] = inR[i] = 0.5f * (float) std::sin (phase); phase += 2 * M_PI * 220.0 / sr; }
        // re-arm atom input sequences each block
        lilv_instance_run (inst, blockSize);
        for (int i = 0; i < blockSize; ++i)
        {
            if (! std::isfinite (outL[i]) || ! std::isfinite (outR[i])) finite = false;
            if (b > 50) { energyIn += inL[i] * inL[i]; energyOut += outL[i] * outL[i] + outR[i] * outR[i]; }
        }
    }
    lilv_instance_deactivate (inst);

    // --ui <screenshot-wait-seconds>: open the plugin's X11 editor in a window (needs an X display) and keep it up.
    if (argc >= 3 && std::strcmp (argv[2], "--ui") == 0)
    {
        const int waitSec = argc >= 4 ? std::atoi (argv[3]) : 3;
        Display* dpy = XOpenDisplay (nullptr);
        if (! dpy) { std::printf ("FAIL: no X display\n"); return 1; }
        LilvUIs* uis = lilv_plugin_get_uis (plugin);
        LilvNode* x11 = lilv_new_uri (world, LV2_UI__X11UI);
        const LilvUI* ui = nullptr; const LilvNode* uiClass = nullptr;
        LILV_FOREACH (uis, i, uis) { const LilvUI* u = lilv_uis_get (uis, i); if (lilv_ui_is_a (u, x11)) { ui = u; break; } }
        if (! ui) { std::printf ("FAIL: no X11 UI\n"); return 1; }
        (void) uiClass;
        Window parent = XCreateSimpleWindow (dpy, DefaultRootWindow (dpy), 0, 0, 1100, 700, 0, 0, 0x202020);
        XMapWindow (dpy, parent); XFlush (dpy);
        LV2_Feature fParent { LV2_UI__parent, (void*) parent };
        LV2_Feature fInst { LV2_INSTANCE_ACCESS_URI, lilv_instance_get_handle (inst) };
        const LV2_Feature* uiFeatures[] = { &fMap, &fUnmap, &fParent, &fInst, &fOpts, nullptr };
        // lilv only describes UIs (suil instantiates them); do it by hand: dlopen the binary and ask for the descriptor.
        const char* binPath = lilv_node_as_uri (lilv_ui_get_binary_uri (ui));
        char* binFile = lilv_file_uri_parse (binPath, nullptr);
        char* bundleFile = lilv_file_uri_parse (lilv_node_as_uri (lilv_plugin_get_bundle_uri (plugin)), nullptr);
        void* lib = dlopen (binFile, RTLD_NOW);
        auto getDesc = lib ? (const LV2UI_Descriptor* (*) (uint32_t)) dlsym (lib, "lv2ui_descriptor") : nullptr;
        if (! getDesc) { std::printf ("FAIL: lv2ui_descriptor not found\n"); return 1; }
        const LV2UI_Descriptor* desc = nullptr;
        for (uint32_t d = 0; (desc = getDesc (d)) != nullptr; ++d)
            if (std::strcmp (desc->URI, lilv_node_as_uri (lilv_ui_get_uri (ui))) == 0) break;
        if (! desc) { std::printf ("FAIL: UI descriptor for %s not found\n", lilv_node_as_uri (lilv_ui_get_uri (ui))); return 1; }
        auto write = [] (LV2UI_Controller, uint32_t, uint32_t, uint32_t, const void*) {};
        LV2UI_Widget widget = nullptr;
        LV2UI_Handle ui_inst = desc->instantiate (desc, argv[1], bundleFile, write, nullptr, &widget, uiFeatures);
        if (! ui_inst) { std::printf ("FAIL: UI instantiate\n"); return 1; }
        std::printf ("UI instantiated, waiting %d s\n", waitSec);
        for (int t = 0; t < waitSec * 20; ++t) { while (XPending (dpy)) { XEvent e; XNextEvent (dpy, &e); } usleep (50000); }
        if (const char* shot = std::getenv ("LV2_SHOT")) { char cmd[512]; std::snprintf (cmd, sizeof cmd, "import -window root %s", shot); std::system (cmd); }
        desc->cleanup (ui_inst);
        XDestroyWindow (dpy, parent); XCloseDisplay (dpy);
        std::printf ("UI closed cleanly\n");
    }
    lilv_instance_free (inst);
    std::printf ("energy in %.3f out %.3f finite %d\n", energyIn, energyOut, (int) finite);
    if (! finite) { std::printf ("FAIL: non-finite output\n"); return 1; }
    if (energyOut <= 0.0) { std::printf ("FAIL: silent output\n"); return 1; }
    std::printf ("PASS %s\n", argv[1]);
    lilv_world_free (world);
    return 0;
}
