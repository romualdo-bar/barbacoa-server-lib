#include <server_lib/emergency_helper.h>
#include <server_lib/platform_config.h>
#include <server_lib/asserts.h>

#include <server_clib/macro.h>

#include <boost/version.hpp>
#include <boost/filesystem.hpp>
#include <boost/stacktrace.hpp>

#include <iostream>
#include <sstream>
#include <fstream>

namespace {
//There is no pretty formatting functions (xprintf) that are async-signal-safe
static void __print_trace_s_(char* buff, size_t sz, const char* text, int out_fd)
{
    memset(buff, 0, sz);
    int ln = SRV_C_MIN(sz - 1, strnlen(text, sz));
    strncpy(buff, text, ln);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
    write(out_fd, buff, strnlen(buff, sz));
#pragma GCC diagnostic pop
}

static char MAGIC_FOR_DUMP_FMT[] = "~stack trace:\n\n";
} // namespace

#define __write_s(file, buff, text) \
    __print_trace_s_(buff, sizeof(buff), text, file)

namespace server_lib {

bool emergency_helper::test_file_for_write(const char* file_path)
{
    if (boost::filesystem::exists(file_path))
        return false;

    {
        std::ofstream ofs(file_path);

        if (!ofs)
            return false;
    }

    boost::filesystem::remove(file_path);
    return true;
}

void emergency_helper::__print_trace_s(const char* text, int out_fd)
{
    char buff[1024];

    __print_trace_s_(buff, sizeof(buff), text, out_fd);
}

bool emergency_helper::save_raw_dump_s(const char* raw_dump_file_path)
{
    //Dumps are binary serialized arrays of void*, so you could read them by using
    //'od -tx8 -An stacktrace_dump_failename' Linux command
    //or using boost::stacktrace::stacktrace::from_dump functions.
    return boost::stacktrace::safe_dump_to(raw_dump_file_path) > 0;
}

std::string emergency_helper::load_raw_dump(const char* raw_dump_file_path, bool remove)
{
    std::stringstream ss;

    if (boost::filesystem::exists(raw_dump_file_path))
    {
        try
        {
            std::ifstream ifs(raw_dump_file_path);

            boost::stacktrace::stacktrace st = boost::stacktrace::stacktrace::from_dump(ifs);
            ss << st;

            // cleaning up
            ifs.close();
        }
        catch (const std::exception& e)
        {
            ss << e.what();
        }
        ss << '\n';

        try
        {
            if (remove)
                boost::filesystem::remove(raw_dump_file_path);
        }
        catch (const std::exception&)
        {
            // ignore
        }
    }

    return ss.str();
}

bool emergency_helper::save_demangled_dump(const char* raw_dump_file_path, const char* demangled_file_path)
{
    if (!boost::filesystem::exists(raw_dump_file_path))
        return false;

    auto demangled = load_raw_dump(raw_dump_file_path, false);
    if (demangled.empty())
        return false;

    std::ofstream out(demangled_file_path, std::ifstream::binary);

    if (!out)
        return false;

    out.write(MAGIC_FOR_DUMP_FMT, sizeof(MAGIC_FOR_DUMP_FMT));
    out.write(demangled.c_str(), demangled.size());

    return true;
}

std::string emergency_helper::load_dump(const char* dump_file_path, bool remove)
{
    std::ifstream input(dump_file_path, std::ifstream::binary);

    if (!input)
        return {};

    char buff[sizeof(MAGIC_FOR_DUMP_FMT)];
    if (!input.read(buff, sizeof(buff)))
        return {};

    std::string dump;

    std::streamsize bytes_read = input.gcount();
    if (bytes_read == sizeof(MAGIC_FOR_DUMP_FMT) && 0 == memcmp(buff, MAGIC_FOR_DUMP_FMT, bytes_read))
    {
        std::stringstream ss;

        for (; input.read(buff, sizeof(buff)) || bytes_read > 0;)
        {
            bytes_read = input.gcount();
            if (bytes_read > 0)
            {
                ss.write(buff, static_cast<uint32_t>(bytes_read));
            }
        }

        input.close();

        dump = ss.str();
    }
    else
    {
        input.close();

        dump = load_raw_dump(dump_file_path, false);
    }

    try
    {
        if (remove)
            boost::filesystem::remove(dump_file_path);
    }
    catch (const std::exception&)
    {
        // ignore
    }

    return dump;
}

std::string emergency_helper::get_executable_name()
{
    std::string name;
#if defined(SERVER_LIB_PLATFORM_LINUX)

    std::ifstream("/proc/self/comm") >> name;

#elif defined(SERVER_LIB_PLATFORM_WINDOWS)

    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    name = buf;

#endif
    return name;
}

std::string emergency_helper::create_dump_filename(const std::string& extension)
{
    SRV_ASSERT(!extension.empty(), "Extension required");

    std::string name = get_executable_name();
    if (name.empty())
        name = "core";

    name.push_back('.');
    name.append(extension);

    return name;
}

} // namespace server_lib
