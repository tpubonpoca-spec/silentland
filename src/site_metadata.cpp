#include "site_metadata.hpp"

#include <windows.h>
#include <winhttp.h>

#include <regex>
#include <stdexcept>
#include <string>

namespace dppbot {

namespace {

class WinHttpHandle {
public:
    explicit WinHttpHandle(HINTERNET handle = nullptr) : handle_(handle) {}
    ~WinHttpHandle() {
        if (handle_) {
            WinHttpCloseHandle(handle_);
        }
    }
    WinHttpHandle(const WinHttpHandle&) = delete;
    WinHttpHandle& operator=(const WinHttpHandle&) = delete;
    WinHttpHandle(WinHttpHandle&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }
    WinHttpHandle& operator=(WinHttpHandle&& other) noexcept {
        if (this != &other) {
            if (handle_) {
                WinHttpCloseHandle(handle_);
            }
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }
    HINTERNET get() const { return handle_; }
private:
    HINTERNET handle_;
};

std::string HttpGet(const std::wstring& host, const std::wstring& path) {
    WinHttpHandle session(WinHttpOpen(L"dppbotcpp/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session.get()) {
        throw std::runtime_error("WinHttpOpen failed.");
    }

    WinHttpHandle connection(WinHttpConnect(session.get(), host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0));
    if (!connection.get()) {
        throw std::runtime_error("WinHttpConnect failed.");
    }

    WinHttpHandle request(WinHttpOpenRequest(connection.get(), L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE));
    if (!request.get()) {
        throw std::runtime_error("WinHttpOpenRequest failed.");
    }

    const wchar_t* headers = L"User-Agent: Mozilla/5.0\r\nAccept: text/html\r\n";
    if (!WinHttpSendRequest(request.get(), headers, static_cast<DWORD>(-1L), WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        throw std::runtime_error("WinHttpSendRequest failed.");
    }
    if (!WinHttpReceiveResponse(request.get(), nullptr)) {
        throw std::runtime_error("WinHttpReceiveResponse failed.");
    }

    std::string body;
    for (;;) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request.get(), &available)) {
            throw std::runtime_error("WinHttpQueryDataAvailable failed.");
        }
        if (available == 0) {
            break;
        }

        std::string buffer(available, '\0');
        DWORD read = 0;
        if (!WinHttpReadData(request.get(), buffer.data(), available, &read)) {
            throw std::runtime_error("WinHttpReadData failed.");
        }
        buffer.resize(read);
        body += buffer;
    }
    return body;
}

std::string StripTags(const std::string& value) {
    return std::regex_replace(value, std::regex("<[^>]+>"), "");
}

}  // namespace

SiteCatalog SiteMetadataClient::FetchCatalog() const {
    SiteCatalog catalog;
    catalog.sourceUrl = "https://ru.dota2changer.com/choose_heroes/";

    try {
        const std::string html = HttpGet(L"ru.dota2changer.com", L"/choose_heroes/");
        const std::regex linkRegex(R"__REGEX__(<a[^>]+href="([^"]+)"[^>]*>\s*([^<][^<]*)\s*</a>)__REGEX__", std::regex::icase);
        for (std::sregex_iterator it(html.begin(), html.end(), linkRegex), end; it != end; ++it) {
            const std::string href = (*it)[1].str();
            const std::string text = StripTags((*it)[2].str());
            if (href.find("skins_dota_2_mods") == std::string::npos && href.find("choose_heroes") == std::string::npos) {
                continue;
            }
            if (text.empty()) {
                continue;
            }
            SiteHeroInfo info;
            info.url = href.rfind("http", 0) == 0 ? href : "https://ru.dota2changer.com" + href;
            info.title = text;
            info.slug = text;
            catalog.heroes.push_back(std::move(info));
        }
        if (catalog.heroes.empty()) {
            catalog.warning = "Site request succeeded, but hero links were not recognized. The site layout may have changed.";
        }
    } catch (const std::exception& ex) {
        catalog.warning = ex.what();
    }

    return catalog;
}

}  // namespace dppbot
