#include "MiscFunctions.hpp"
#include "../defines.hpp"
#include <algorithm>
#include "../Compositor.hpp"
#include <sys/utsname.h>
#include <iomanip>

#if defined(__DragonFly__) || defined(__FreeBSD__) || \
  defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__OpenBSD__)
# include <sys/sysctl.h>
# if defined(__DragonFly__)
#  include <sys/kinfo.h>      // struct kinfo_proc
# elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#  include <sys/user.h>       // struct kinfo_proc
# endif

# if defined(__NetBSD__)
#  undef KERN_PROC
#  define KERN_PROC KERN_PROC2
#  define KINFO_PROC struct kinfo_proc2
# else
#  define KINFO_PROC struct kinfo_proc
# endif
# if defined(__DragonFly__)
#  define KP_PPID(kp) kp.kp_ppid
# elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#  define KP_PPID(kp) kp.ki_ppid
# else
#  define KP_PPID(kp) kp.p_ppid
# endif
#endif

static const float transforms[][9] = {{
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
	},{
		0.0f, 1.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
	},{
		-1.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
	},{
		0.0f, -1.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
	},{
		-1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
	},{
		0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
	},{
		1.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
	},{
		0.0f, -1.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
	},
};

std::string absolutePath(const std::string& rawpath, const std::string& currentPath) {
    auto value = rawpath;

    if (value[0] == '.') {
        auto currentDir = currentPath.substr(0, currentPath.find_last_of('/'));

        if (value[1] == '.') {
            auto parentDir = currentDir.substr(0, currentDir.find_last_of('/'));
            value.replace(0, 2, parentDir);
        } else {
            value.replace(0, 1, currentDir);
        }
    }

    if (value[0] == '~') {
        static const char* const ENVHOME = getenv("HOME");
        value.replace(0, 1, std::string(ENVHOME));
    }

    return value;
}

void addWLSignal(wl_signal* pSignal, wl_listener* pListener, void* pOwner, std::string ownerString) {
    ASSERT(pSignal);
    ASSERT(pListener);

    wl_signal_add(pSignal, pListener);

    Debug::log(LOG, "Registered signal for owner %x: %x -> %x (owner: %s)", pOwner, pSignal, pListener, ownerString.c_str());
}

void handleNoop(struct wl_listener *listener, void *data) {
    // Do nothing
}

std::string getFormat(const char *fmt, ...) {
    char* outputStr = nullptr;

    va_list args;
    va_start(args, fmt);
    vasprintf(&outputStr, fmt, args);
    va_end(args);

    std::string output = std::string(outputStr);
    free(outputStr);

    return output;
}

std::string escapeJSONStrings(const std::string& str) {
    std::ostringstream oss;
    for (auto &c : str) {
        switch (c) {
        case '"': oss << "\\\""; break;
        case '\\': oss << "\\\\"; break;
        case '\b': oss << "\\b"; break;
        case '\f': oss << "\\f"; break;
        case '\n': oss << "\\n"; break;
        case '\r': oss << "\\r"; break;
        case '\t': oss << "\\t"; break;
        default:
            if ('\x00' <= c && c <= '\x1f') {
                oss << "\\u"
                    << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
            } else {
                oss << c;
            }
        }
    }
    return oss.str();
}

void scaleBox(wlr_box* box, float scale) {
    box->width = std::round(box->width * scale);
    box->height = std::round(box->height * scale);
    box->x = std::round(box->x * scale);
    box->y = std::round(box->y * scale);
}

std::string removeBeginEndSpacesTabs(std::string str) {
    int countBefore = 0;
    while (str[countBefore] == ' ' || str[countBefore] == '\t') {
        countBefore++;
    }

    int countAfter = 0;
    while (str.length() != 0 && (str[str.length() - countAfter - 1] == ' ' || str[str.length() - 1 - countAfter] == '\t')) {
        countAfter++;
    }

    str = str.substr(countBefore, str.length() - countBefore - countAfter);

    return str;
}

float getPlusMinusKeywordResult(std::string source, float relative) {
    float result = INT_MAX;

    if (source.find_first_of("+") == 0) {
        try {
            if (source.contains("."))
                result = relative + std::stof(source.substr(1));
            else
                result = relative + std::stoi(source.substr(1));
        } catch (...) {
            Debug::log(ERR, "Invalid arg \"%s\" in getPlusMinusKeywordResult!", source.c_str());
            return INT_MAX;
        }
    } else if (source.find_first_of("-") == 0) {
        try {
            if (source.contains("."))
                result = relative - std::stof(source.substr(1));
            else
                result = relative - std::stoi(source.substr(1));
        } catch (...) {
            Debug::log(ERR, "Invalid arg \"%s\" in getPlusMinusKeywordResult!", source.c_str());
            return INT_MAX;
        }
    } else {
        try {
            if (source.contains("."))
                result = stof(source);
            else
                result = stoi(source);
        } catch (...) {
            Debug::log(ERR, "Invalid arg \"%s\" in getPlusMinusKeywordResult!", source.c_str());
            return INT_MAX;
        }
    }

    return result;
}

bool isNumber(const std::string& str, bool allowfloat) {

    std::string copy = str;
    if (*copy.begin() == '-')
        copy = copy.substr(1);

    if (copy.empty())
        return false;

    bool point = !allowfloat;
    for (auto& c : copy) {
        if (c == '.') {
            if (point)
                return false;
            point = true;
            break;
        }

        if (!std::isdigit(c))
            return false;
    }

    return true;
}

bool isDirection(const std::string& arg) {
    return arg == "l" || arg == "r" || arg == "u" || arg == "d" || arg == "t" || arg == "b";
}

int getWorkspaceIDFromString(const std::string& in, std::string& outName) {
    int result = INT_MAX;
    if (in.find("special") == 0) {
        outName = "special";
        return SPECIAL_WORKSPACE_ID;
    } else if (in.find("name:") == 0) {
        const auto WORKSPACENAME = in.substr(in.find_first_of(':') + 1);
        const auto WORKSPACE = g_pCompositor->getWorkspaceByName(WORKSPACENAME);
        if (!WORKSPACE) {
            result = g_pCompositor->getNextAvailableNamedWorkspace();
        } else {
            result = WORKSPACE->m_iID;
        }
        outName = WORKSPACENAME;
    } else {
        if ((in[0] == 'm' || in[0] == 'e') && (in[1] == '-' || in[1] == '+') && isNumber(in.substr(2))) {
            bool onAllMonitors = in[0] == 'e';

            if (!g_pCompositor->m_pLastMonitor) {
                Debug::log(ERR, "Relative monitor workspace on monitor null!");
                result = INT_MAX;
                return result;
            }

            // monitor relative
            result = (int)getPlusMinusKeywordResult(in.substr(1), 0);

            // result now has +/- what we should move on mon
            int remains = (int)result;
            int currentID = g_pCompositor->m_pLastMonitor->activeWorkspace;
            int searchID = currentID;

            while (remains != 0) {
                if (remains < 0)
                    searchID--;
                else
                    searchID++;

                if (g_pCompositor->workspaceIDOutOfBounds(searchID)){
                    // means we need to wrap around
                    int lowestID = 99999;
                    int highestID = -99999;

                    for (auto& w : g_pCompositor->m_vWorkspaces) {
                        if (w->m_iID == SPECIAL_WORKSPACE_ID)
                            continue;

                        if (w->m_iID < lowestID)
                            lowestID = w->m_iID;

                        if (w->m_iID > highestID)
                            highestID = w->m_iID;
                    }

                    if (remains < 0)
                        searchID = highestID;
                    else
                        searchID = lowestID;
                }

                if (const auto PWORKSPACE = g_pCompositor->getWorkspaceByID(searchID); PWORKSPACE && PWORKSPACE->m_iID != SPECIAL_WORKSPACE_ID) {
                    if (onAllMonitors || PWORKSPACE->m_iMonitorID == g_pCompositor->m_pLastMonitor->ID) {
                        currentID = PWORKSPACE->m_iID;

                        if (remains < 0)
                            remains++;
                        else
                            remains--;
                    }
                }
            }

            result = currentID;
            outName = g_pCompositor->getWorkspaceByID(currentID)->m_szName;

        } else {
            if (in[0] == '+' || in[0] == '-') {
                if (g_pCompositor->m_pLastMonitor)
                    result = std::max((int)getPlusMinusKeywordResult(in, g_pCompositor->m_pLastMonitor->activeWorkspace), 1);
                else {
                    Debug::log(ERR, "Relative workspace on no mon!");
                    result = INT_MAX;
                }
            } else if (isNumber(in))
                result = std::max(std::stoi(in), 1);
            else {
                // maybe name
                const auto PWORKSPACE = g_pCompositor->getWorkspaceByName(in);
                if (PWORKSPACE)
                    result = PWORKSPACE->m_iID;
            }

            outName = std::to_string(result);
        }
    }

    return result;
}

float vecToRectDistanceSquared(const Vector2D& vec, const Vector2D& p1, const Vector2D& p2) {
    const float DX = std::max({0.0, p1.x - vec.x, vec.x - p2.x});
    const float DY = std::max({0.0, p1.y - vec.y, vec.y - p2.y});
    return DX * DX + DY * DY;
}

// Execute a shell command and get the output
std::string execAndGet(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    const std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        Debug::log(ERR, "execAndGet: failed in pipe");
        return "";
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

void logSystemInfo() {
    struct utsname unameInfo;

    uname(&unameInfo);

    Debug::log(LOG, "System name: %s", unameInfo.sysname);
    Debug::log(LOG, "Node name: %s", unameInfo.nodename);
    Debug::log(LOG, "Release: %s", unameInfo.release);
    Debug::log(LOG, "Version: %s", unameInfo.version);

    Debug::log(NONE, "\n");

    const std::string GPUINFO = execAndGet("lspci -vnn | grep VGA");
    Debug::log(LOG, "GPU information:\n%s\n", GPUINFO.c_str());

    if (GPUINFO.contains("NVIDIA")) {
        Debug::log(WARN, "Warning: you're using an NVIDIA GPU. Make sure you follow the instructions on the wiki if anything is amiss.\n");
    }

    // log etc
    Debug::log(LOG, "os-release:");

    Debug::log(NONE, "%s", execAndGet("cat /etc/os-release").c_str());
}

void matrixProjection(float mat[9], int w, int h, wl_output_transform tr) {
    memset(mat, 0, sizeof(*mat) * 9);

    const float* t = transforms[tr];
    float x = 2.0f / w;
    float y = 2.0f / h;

    // Rotation + reflection
    mat[0] = x * t[0];
    mat[1] = x * t[1];
    mat[3] = y * t[3];
    mat[4] = y * t[4];

    // Translation
    mat[2] = -copysign(1.0f, mat[0] + mat[1]);
    mat[5] = -copysign(1.0f, mat[3] + mat[4]);

    // Identity
    mat[8] = 1.0f;
}

int64_t getPPIDof(int64_t pid) {
#if defined(KERN_PROC_PID)
    int mib[] = {
        CTL_KERN,
        KERN_PROC,
        KERN_PROC_PID,
        (int)pid,
# if defined(__NetBSD__) || defined(__OpenBSD__)
        sizeof(KINFO_PROC),
        1,
# endif
    };
    u_int miblen = sizeof(mib) / sizeof(mib[0]);
    KINFO_PROC kp;
    size_t sz = sizeof(KINFO_PROC);
    if (sysctl(mib, miblen, &kp, &sz, NULL, 0) != -1)
        return KP_PPID(kp);

    return 0;
#else
    std::string dir = "/proc/" + std::to_string(pid) + "/status";
    FILE* infile;

    infile = fopen(dir.c_str(), "r");
    if (!infile)
        return 0;

    char* line = nullptr;
    size_t len = 0;
    ssize_t len2 = 0;

    std::string pidstr;

    while ((len2 = getline(&line, &len, infile)) != -1) {
        if (strstr(line, "PPid:")) {
            pidstr = std::string(line, len2);
            const auto tabpos = pidstr.find_last_of('\t');
            if (tabpos != std::string::npos)
                pidstr = pidstr.substr(tabpos);
            break;
        }
    }

    fclose(infile);
    if (line)
        free(line);

    try {
        return std::stoll(pidstr);
    } catch (std::exception& e) {
        return 0;
    }
#endif
}
