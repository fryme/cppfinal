#include "http_parser.h"
#include "common.h"

namespace  http
{

    std::vector<std::string> SplitToParts(const std::string& data, const std::string& delimiter)
    {
        std::vector<std::string> parts;
        std::string::size_type pos = 0;

        while (true)
        {
            int newPos = data.find_first_of(delimiter, pos);
            if (newPos == std::string::npos)
            {
                parts.push_back(data.substr(pos, data.length() - pos));
                break;
            }
            else
            {
                parts.push_back(data.substr(pos, newPos - pos));
            }

            pos = newPos + delimiter.length();
            if (pos > data.length())
            {
                parts.push_back(data.substr(newPos, data.length() - newPos));
                break;
            }
        }

        return parts;
    }

    Request::Request(const std::string& data)
    {
        const std::vector<std::string> httpParts = SplitToParts(data, http::HttpDelimiter);

        CHECK_THROW(httpParts.size() > 0, "Number of parts incorrect");

        auto rowParts = SplitToParts(httpParts[0], " ");

        if (rowParts[0] == std::string("GET"))
            m_type = RequestType::Get;
        else
            m_type = RequestType::Post;

        m_resourcePath = rowParts[1];

/*
        for (const auto& part : httpParts)
        {
            const auto headerParts = SplitToParts(part, std::string(":"));
            if (headerParts.size() > 1)
                throw std::runtime_error("Size of header is lesser, than expected");

            m_headerItems.push_back({headerParts[0], headerParts[1]});
        }
*/
        /*
        const std::string AcceptStr = "Accept";
        auto found = std::find_if(m_headerItems.begin(), m_headerItems.end(), [AcceptStr](const auto& item)->bool
        {
            return item.name == AcceptStr;
        });

        if (found == m_headerItems.end())
        {
            std::clog << "Request: header 'Accept' not found" << std::endl;
        }
        else
        {
            CHECK_THROW(found->name == "text/html", "Current 'Accept' header is not acceptable");
        }
         */
    }

    Request::RequestType Request::GetType() const
    {
        return m_type;
    }

    std::string Request::GetResourcePath() const
    {
        return m_resourcePath;
    }

    void Response::AddResponseCode(int code)
    {
        m_code = code;
    }

    void Response::AddHeader(const http::HeaderItem& header)
    {
        m_headers.push_back(header);
    }

    void Response::AddData(const std::string& data)
    {
        m_data = data;
    }

    std::string Response::GetRaw() const
    {
        std::stringstream raw;

        switch(m_code)
        {
            case 200:
            {
                raw << "HTTP/1.1 200 OK";
                break;
            }
            case 400:
            {
                raw << "HTTP/1.1 400 BadRequest";
                break;
            }
            case 404:
            {
                raw << "HTTP/1.1 404 Not found";
                break;
            }

            default:
                std::clog << "Unknown code: " << m_code << std::endl;
        }

        raw << HttpDelimiter;

        for (const auto h : m_headers)
            raw << h.name << ": " << h.value << HttpDelimiter;

        raw << HttpDelimiter;
        raw << m_data;

        return raw.str().c_str();
    }

}
