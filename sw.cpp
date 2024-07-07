#pragma sw header on

//#include <iostream>
//auto var = #n;
//std::cerr << var << "\n";

#define MAKE_SIMPLE_BOT(n, ...) \
    void build(Solution &s) \
    { \
        auto &bot = s.addTarget<ExecutableTarget>(#n); \
        bot.PackageDefinitions = true; \
        bot += cpp23; \
        bot += #n ".cpp"; \
        bot += "org.sw.demo.tgbot.curl_skeleton"_dep __VA_OPT__(,) __VA_ARGS__ ; \
    }

#define MAKE_SIMPLE_BOT1(x) MAKE_SIMPLE_BOT(x)
#define MAKE_SIMPLE_BOT2 MAKE_SIMPLE_BOT1(__FILE__)

#pragma sw header off

void build(Solution &s)
{
    auto &tgbot = s.addLibrary("tgbot", "1.1.7.7.7");
    tgbot += Git("https://github.com/egorpugin/tgbot");

    auto &apitool = tgbot.addExecutable("apitool");
    {
        auto &t = apitool;
        t.PackageDefinitions = true;
        t += cpp23;
        t += "tools/.*"_rr;
        t += "pub.egorpugin.primitives.emitter"_dep;
        t += "pub.egorpugin.primitives.xml"_dep;
        t += "pub.egorpugin.primitives.http"_dep;
        t += "pub.egorpugin.primitives.sw.main"_dep;
        t += "org.sw.demo.nlohmann.json"_dep;
        t += "org.sw.demo.imageworks.pystring"_dep;
    }

    //
    {
        tgbot += cpp23;
        tgbot += "include/.*"_rr;
        if (tgbot.getCompilerType() == CompilerType::MSVC)
            tgbot.Public.CompileOptions.push_back("/Zc:__cplusplus");
        tgbot.Public += "org.sw.demo.nlohmann.json.natvis"_dep;
        // https://github.com/boostorg/pfr/issues/168 or clang bug
        //tgbot.Public += "org.sw.demo.boost.pfr"_dep;
        {
            auto c = tgbot.addCommand();
            c << cmd::prog(apitool)
                << cmd::wdir(tgbot.BinaryDir)
                << cmd::in("TelegramBotAPI.html")
                << cmd::end()
                << cmd::out("types.inl.h")
                << cmd::out("methods.inl.h")
                ;
            // to make dependency? (in VS IDE only?)
            // causes cyclic deps for some reason
            //tgbot.Public += "types.inl.h";
            //tgbot.Public += "methods.inl.h";
        }
    }

    auto &report_detection_bot = tgbot.addTarget<ExecutableTarget>("examples.report_detection_bot");
    {
        auto &bot = report_detection_bot;
        bot.PackageDefinitions = true;
        bot += cpp23;
        bot += "examples/report_detection_bot/src/.*"_rr;
        bot +=
            "pub.egorpugin.primitives.http"_dep,
            "pub.egorpugin.primitives.sw.main"_dep,
            tgbot
            ;
    }

    auto &curl_skeleton = tgbot.addTarget<Library>("curl_skeleton");
    {
        curl_skeleton.setRootDirectory("src/curl_skeleton");
        curl_skeleton += ".*"_rr;
        curl_skeleton.Public +=
            tgbot,
            "org.sw.demo.nlohmann.json.natvis"_dep,
            "pub.egorpugin.primitives.templates2"_dep,
            "pub.egorpugin.primitives.http"_dep,
            "pub.egorpugin.primitives.sw.main"_dep
            ;
    }
}
