void build(Solution &s)
{
    auto &bot = s.addTarget<ExecutableTarget>("report_detection_bot");
    {
        bot.PackageDefinitions = true;
        bot += cpp23;
        bot += "src/.*"_rr;
        bot += "pub.egorpugin.tgbot.curl_skeleton"_dep;
    }
}
