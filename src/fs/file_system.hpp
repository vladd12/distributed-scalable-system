#pragma once

#include <boost/noncopyable.hpp>
#include <fmt/core.h>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace fs
{

class file_system : private boost::noncopyable, public std::enable_shared_from_this<file_system>
{
public:
    std::shared_ptr<file_system> getptr()
    {
        return shared_from_this();
    }

    virtual void foo() noexcept = 0;
};

using file_system_ptr = std::shared_ptr<file_system>;

class local_file_system : public file_system
{
public:
    void foo() noexcept override
    {
    }
};

class distributed_file_system : public file_system
{
public:
    void foo() noexcept override
    {
    }
};

class file_system_provider
{
private:
    std::unordered_map<std::string, file_system_ptr> m_filesystems;

    void append(const std::string &host_formated, file_system_ptr &fs)
    {
        auto val = m_filesystems.insert({ host_formated, fs });
    }

public:
    file_system_ptr get(const std::string &host, std::uint64_t port)
    {
        auto host_formated { fmt::format("{0}:{1}", host, port) };
        if (auto search = m_filesystems.find(host_formated); search != m_filesystems.end())
        {
            return search->second;
        }
        else
        {
            if (host == "localhost")
            {
                file_system_ptr fs { std::make_shared<local_file_system>() };
                append(host_formated, fs);
                return fs;
            }
            else
            {
                file_system_ptr fs { std::make_shared<distributed_file_system>() };
                append(host_formated, fs);
                return fs;
            }
        }
    }
};

} // namespace fs
