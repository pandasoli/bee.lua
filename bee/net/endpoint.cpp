#include <bee/net/endpoint.h>
#if defined(_WIN32)
#    include <Ws2tcpip.h>
#else
#    include <arpa/inet.h>
#    include <netdb.h>
#    include <sys/un.h>
#    if defined(__FreeBSD__)
#        include <netinet/in.h>
#    endif
#    ifndef UNIX_PATH_MAX
#        define UNIX_PATH_MAX (sizeof(sockaddr_un::sun_path) / sizeof(sockaddr_un::sun_path[0]))
#    endif
#endif
#include <bee/error.h>
#include <bee/nonstd/charconv.h>

#include <array>
#include <limits>

// see the https://blogs.msdn.microsoft.com/commandline/2017/12/19/af_unix-comes-to-windows/
#if defined(_WIN32)
//
// need Windows SDK >= 17063
// #include <afunix.h>
//
#    define UNIX_PATH_MAX 108
struct sockaddr_un {
    unsigned short sun_family;
    char sun_path[UNIX_PATH_MAX];
};
#endif

namespace bee::net {
    static constexpr socklen_t kMaxEndpointSize = 256;

    endpoint_buf::endpoint_buf()
        : m_data((size_t)kMaxEndpointSize)
        , m_size(kMaxEndpointSize) {}
    endpoint_buf::endpoint_buf(size_t size)
        : m_data(size)
        , m_size((socklen_t)size) {}
    sockaddr* endpoint_buf::addr() {
        return (sockaddr*)m_data.data();
    }
    socklen_t* endpoint_buf::addrlen() {
        return &m_size;
    }

#if defined(__linux__) && defined(BEE_DISABLE_DLOPEN)
#else
    struct autorelease_addrinfo {
        autorelease_addrinfo(autorelease_addrinfo const&)            = delete;
        autorelease_addrinfo& operator=(autorelease_addrinfo const&) = delete;
        autorelease_addrinfo& operator=(autorelease_addrinfo&&)      = delete;
        autorelease_addrinfo(addrinfo* i)
            : info(i) {}
        autorelease_addrinfo(autorelease_addrinfo&& o)
            : info(o.info) { o.info = 0; }
        ~autorelease_addrinfo() {
            if (info) ::freeaddrinfo(info);
        }
        operator bool() const { return !!info; }
        const addrinfo* operator->() const { return info; }
        const addrinfo& operator*() const { return *info; }
        addrinfo* info;
    };

    static bool needsnolookup(zstring_view ip) {
        size_t pos = ip.find_first_not_of("0123456789.");
        if (pos == std::string_view::npos) {
            return true;
        }
        pos = ip.find_first_not_of("0123456789abcdefABCDEF:");
        if (pos == std::string_view::npos) {
            return true;
        }
        if (ip[pos] != '.') {
            return false;
        }
        pos = ip.find_last_of(':');
        if (pos == std::string_view::npos) {
            return false;
        }
        pos = ip.find_first_not_of("0123456789.", pos);
        if (pos == std::string_view::npos) {
            return true;
        }
        return false;
    }
    static autorelease_addrinfo gethostaddr(const addrinfo& hint, zstring_view ip, const char* port) {
        addrinfo* info = 0;
        int err        = ::getaddrinfo(ip.data(), port, &hint, &info);
        if (err != 0) {
            if (info) ::freeaddrinfo(info);
            info = 0;
        }
        return autorelease_addrinfo(info);
    }
#endif

    endpoint endpoint::from_unixpath(zstring_view path) {
        if (path.size() >= UNIX_PATH_MAX) {
            return from_invalid();
        }
        endpoint_buf tmp(offsetof(struct sockaddr_un, sun_path) + path.size() + 1);
        struct sockaddr_un* su = (struct sockaddr_un*)tmp.addr();
        su->sun_family         = AF_UNIX;
        memcpy(&su->sun_path[0], path.data(), path.size() + 1);
        return from_buf(std::move(tmp));
    }
    endpoint endpoint::from_hostname(zstring_view ip, uint16_t port) {
#if defined(__linux__) && defined(BEE_DISABLE_DLOPEN)
        struct sockaddr_in sa4;
        if (1 == inet_pton(AF_INET, ip.data(), &sa4.sin_addr)) {
            sa4.sin_family = AF_INET;
            sa4.sin_port   = htons(port);
            sa4.sin_port   = htons(port);
            return { (std::byte const*)sa4, sizeof(sa4) };
        }
        struct sockaddr_in6 sa6;
        if (1 == inet_pton(AF_INET6, ip.data(), &sa6.sin6_addr)) {
            sa4.sin_family = AF_INET6;
            sa6.sin6_port  = htons(port);
            return { (std::byte const*)sa6, sizeof(sa6) };
        }
        return from_invalid();
#else
        addrinfo hint  = {};
        hint.ai_family = AF_UNSPEC;
        if (needsnolookup(ip)) {
            hint.ai_flags = AI_NUMERICHOST;
        }
        constexpr auto portn = std::numeric_limits<uint16_t>::digits10 + 1;
        char portstr[portn + 1];
        if (auto [p, ec] = std::to_chars(portstr, portstr + portn, port); ec != std::errc()) {
            return from_invalid();
        }
        else {
            p[0] = '\0';
        }
        auto info = gethostaddr(hint, ip, portstr);
        if (!info) {
            return from_invalid();
        }
        else if (info->ai_family != AF_INET && info->ai_family != AF_INET6) {
            return from_invalid();
        }
        return { (std::byte const*)info->ai_addr, (size_t)info->ai_addrlen };
#endif
    }

    endpoint endpoint::from_buf(endpoint_buf&& buf) {
        return { std::move(buf.m_data), (size_t)buf.m_size };
    }

    endpoint endpoint::from_invalid() {
        return endpoint();
    }

    endpoint::endpoint()
        : m_data() {}

    endpoint::endpoint(size_t size)
        : m_data(size) {}

    endpoint::endpoint(std::byte const* data, size_t size)
        : m_data(size) {
        memcpy(m_data.data(), data, sizeof(std::byte) * size);
    }

    endpoint::endpoint(dynarray<std::byte>&& data)
        : m_data(std::move(data)) {}

    endpoint::endpoint(dynarray<std::byte>&& data, size_t size)
        : m_data(std::move(data)) {
        m_data.resize(size);
    }

    endpoint_info endpoint::info() const {
        const sockaddr* sa = addr();
        if (sa->sa_family == AF_INET) {
            char tmp[sizeof "255.255.255.255"];
#if !defined(__MINGW32__)
            const char* s = inet_ntop(sa->sa_family, (const void*)&((struct sockaddr_in*)sa)->sin_addr, tmp, sizeof tmp);
#else
            const char* s = inet_ntop(sa->sa_family, (void*)&((struct sockaddr_in*)sa)->sin_addr, tmp, sizeof tmp);
#endif
            return { std::string(s), ntohs(((struct sockaddr_in*)sa)->sin_port) };
        }
        else if (sa->sa_family == AF_INET6) {
            char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"];
#if !defined(__MINGW32__)
            const char* s = inet_ntop(sa->sa_family, (const void*)&((const struct sockaddr_in6*)sa)->sin6_addr, tmp, sizeof tmp);
#else
            const char* s = inet_ntop(sa->sa_family, (void*)&((const struct sockaddr_in6*)sa)->sin6_addr, tmp, sizeof tmp);
#endif
            return { std::string(s), ntohs(((struct sockaddr_in6*)sa)->sin6_port) };
        }
        else if (sa->sa_family == AF_UNIX) {
            const char* path = ((struct sockaddr_un*)sa)->sun_path;
            int len          = addrlen() - offsetof(struct sockaddr_un, sun_path) - 1;
            if (len > 0 && path[0] != 0) {
                return { std::string(path, len), (uint16_t)un_format::pathname };
            }
            else if (len > 1) {
                return { std::string(path + 1, len - 1), (uint16_t)un_format::abstract };
            }
            else {
                return { std::string(), (uint16_t)un_format::unnamed };
            }
        }
        return { "", 0 };
    }
    const sockaddr* endpoint::addr() const {
        return (const sockaddr*)m_data.data();
    }
    socklen_t endpoint::addrlen() const {
        return (socklen_t)m_data.size();
    }
    unsigned short endpoint::family() const {
        return addr()->sa_family;
    }
    bool endpoint::valid() const {
        return addrlen() != 0;
    }
}
