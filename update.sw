/*
c++: 23
package_definitions: true
dependencies:
    - pub.egorpugin.script_configs
*/

#include <script_configs.h>

using namespace primitives::deploy;

#define MAIN_LABEL
int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << std::format("Usage: {} version\n", argv[0]);
        return 1;
    }

    const auto sw_version = "1.1.6"s;
    const auto new_tg_version = argv[1];
    const auto new_sw_full_version = std::format("{}.{}", sw_version, new_tg_version);

    git g;
    auto tags = g("tag");
    boost::trim(tags);
    const auto old_sw_full_version = tags.substr(tags.rfind("\n")+1);
    auto old_tg_version = old_sw_full_version;
    old_tg_version = old_tg_version.substr(old_tg_version.find(".")+1);
    old_tg_version = old_tg_version.substr(old_tg_version.find(".")+1);
    old_tg_version = old_tg_version.substr(old_tg_version.find(".")+1);

    g.pull("--tags", "origin", "master");
    g("rebase");
    auto f = download_file("https://core.telegram.org/bots/api");
    boost::trim(f);
    f = f.substr(0, f.rfind("\n")) + "\n";
    write_file("TelegramBotAPI.html", f);

    // update versions
    auto repl = [](auto &&fn, auto &&from, auto &&to) {
        auto s = read_file(fn);
        boost::replace_all(s, from, to);
        write_file(fn, s);
    };
    repl("README.md", old_tg_version, new_tg_version);
    repl("sw.cpp", old_sw_full_version, new_sw_full_version);

    // test build
    command("sw", "build");

    g("commit", "-am", std::format("Update Bot API to {}.", new_tg_version));
    g("tag", "-a", new_sw_full_version, "-m", new_sw_full_version);
    g("push");
    g("push", "--tags");

    // publish gh
    command("gh", "api", "--method", "POST", "/repos/egorpugin/tgbot/releases",
        "-f", std::format("tag_name=\"{}\"", new_sw_full_version),
        "-f", std::format("name=\"{}\"", new_sw_full_version),
        "-f", std::format("body=\"Update to Telegram Bot API v{}.\"", new_tg_version)
    );

    // publish sw
    command("sw", "build");
    command("sw", "upload", "org.sw.demo");

    return 0;
}
