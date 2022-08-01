void build(Solution &s)
{
    auto &bot = s.addTarget<ExecutableTarget>("report_detection_bot");
    {
        bot.PackageDefinitions = true;
        bot += cpp23;
        bot += "src/.*"_rr;
        bot +=
            "pub.egorpugin.primitives.http"_dep,
            "pub.egorpugin.primitives.sw.main"_dep,
            "pub.egorpugin.tgbot"_dep,
            "org.sw.demo.nlohmann.json"_dep
            ;
    }
}
