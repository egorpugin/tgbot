#include "emitter.h"

#include <primitives/http.h>
#include <primitives/sw/main.h>
#include <primitives/sw/cl.h>

#include <iostream>

int main(int argc, char **argv)
{
    cl::opt<String> target(cl::Positional, cl::Required, cl::desc("Html file or url to parse"));
    cl::opt<path> json("j", cl::desc("Output api as json file"));
    cl::ParseCommandLineOptions(argc, argv);

    std::clog << "Trying to parse " << target << "\n";
    Parser p(read_file_or_download_file(target));

    p.enumerateSectionChildren("getting-updates");
    p.enumerateSectionChildren("available-types");
    p.enumerateSectionChildren("available-methods");
    p.enumerateSectionChildren("updating-messages");
    p.enumerateSectionChildren("stickers");
    p.enumerateSectionChildren("inline-mode");
    p.enumerateSectionChildren("payments");
    p.enumerateSectionChildren("telegram-passport");
    p.enumerateSectionChildren("games");

    // json
    {
        nlohmann::json j;
        for (auto &t : p.types)
            t.save(j["types"][t.name]);
        for (auto &t : p.methods)
            t.save(j["methods"][t.name]);
        if (!json.empty())
            write_file(json, j.dump(2));
    }

    Emitter e(p);
    e.emitTypes();
    //e.emitTypesSeparate();
    e.emitMethods();
    e.emitReflection();

    return 0;
}
