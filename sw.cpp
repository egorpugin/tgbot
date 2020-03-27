#pragma sw require header org.sw.demo.google.protobuf.protoc

void build(Solution &s)
{
    // try to remove static later
    auto &tgbot = s.addStaticLibrary("tgbot", "1.2.2");
    //tgbot += Git("https://github.com/reo7sp/tgbot-cpp", "v{M}.{m}{po}");

    auto &apitool = tgbot.addExecutable("apitool");
    {
        auto &t = apitool;
        t += cpp17;
        t += "tools/.*"_rr;
        t += "pub.egorpugin.primitives.emitter-master"_dep;
        t += "pub.egorpugin.primitives.http-master"_dep;
        t += "pub.egorpugin.primitives.xml-master"_dep;
        t += "pub.egorpugin.primitives.sw.main-master"_dep;
        t += "org.sw.demo.nlohmann.json"_dep;
        t += "org.sw.demo.imageworks.pystring"_dep;
    }

    //
    {
        tgbot += cpp17;

        tgbot.ApiName = "TGBOT_API";

        tgbot += "include/.*"_rr;
        tgbot += "src/.*"_rr;

        if (tgbot.getCompilerType() == CompilerType::MSVC)
            tgbot.CompileOptions.push_back("/Zc:__cplusplus");

        tgbot += "org.sw.demo.boost.algorithm"_dep;
        tgbot += "org.sw.demo.nlohmann.json"_dep;
        tgbot.Public += "org.sw.demo.badger.curl.libcurl"_dep;

        {
            auto c = tgbot.addCommand();
            c << cmd::prog(apitool)
                << cmd::wdir(tgbot.BinaryDir)
                << cmd::in("TelegramBotAPI.html")
                << cmd::end()
                << cmd::out("tgapi.proto")
                ;
            ProtobufData d;
            d.public_protobuf = true;
            d.idirs.push_back(tgbot.BinaryDir);
            gen_protobuf_cpp("org.sw.demo.google.protobuf"_dep, tgbot, "tgapi.proto", d);
        }
    }

    {
        auto &bot = s.addTarget<ExecutableTarget>("test_bot");
        bot += cpp17;
        bot += "test_bot.cpp";
        bot += tgbot;
        bot +=
            "pub.egorpugin.primitives.filesystem-master"_dep,
            "pub.egorpugin.primitives.templates-master"_dep,
            "pub.egorpugin.primitives.sw.main-master"_dep
            ;
    }
}
