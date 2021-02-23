void build(Solution &s)
{
    auto &tgbot = s.addLibrary("tgbot", "1.1.0");
    tgbot += Git("https://github.com/egorpugin/tgbot");

    auto &apitool = tgbot.addExecutable("apitool");
    {
        auto &t = apitool;
        t += cpp20;
        t += "tools/.*"_rr;
        t += "pub.egorpugin.primitives.emitter"_dep;
        t += "pub.egorpugin.primitives.http"_dep;
        t += "pub.egorpugin.primitives.xml"_dep;
        t += "pub.egorpugin.primitives.sw.main"_dep;
        t += "org.sw.demo.nlohmann.json"_dep;
        t += "org.sw.demo.imageworks.pystring"_dep;
    }

    //
    {
        tgbot += cpp20;

        tgbot.ApiName = "TGBOT_API";

        tgbot += "include/.*"_rr;
        tgbot += "src/.*"_rr;

        if (tgbot.getCompilerType() == CompilerType::MSVC)
            tgbot.CompileOptions.push_back("/Zc:__cplusplus");

        // we can make asio as feature. If selected, push as public dep
        //tgbot.Public += "TGBOT_ENABLE_ASYNC"_dep;
        tgbot.Public += "org.sw.demo.boost.asio"_dep;
        tgbot += "org.sw.demo.nlohmann.json"_dep;
        tgbot += "org.sw.demo.badger.curl.libcurl"_dep;

        {
            auto c = tgbot.addCommand();
            c << cmd::prog(apitool)
                << cmd::wdir(tgbot.BinaryDir)
                << cmd::in("TelegramBotAPI.html")
                << cmd::end()
                << cmd::out("types.inl.h")
                << cmd::out("methods.inl.h")
                << cmd::out("methods.inl.cpp", cmd::Skip)
                ;
        }
    }
}
