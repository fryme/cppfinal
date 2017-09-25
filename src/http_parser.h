#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iterator>

namespace http
{
    struct RequestDescription
    {
        //std::string ;
        std::string value;
    };

    /*
    GET /hello.html HTTP/1.1
    Host: localhost:12345
    User-Agent: curl/7.47.0
    Accept: text/html

    95
    */

    const std::string HttpDelimiter = "\r\n";

    std::vector<std::string> SplitToParts(const std::string& data, const std::string& delimiter);

    struct HeaderItem
    {
        std::string name;
        std::string value;
    };

    class Request
    {
    public:
        Request(const std::string& data);
        std::string GetPath() const;

        enum RequestType
        {
            Get,
            Post
        };

        RequestType GetType() const;
        std::string GetResourcePath() const;

    private:
        //std::ostringstream m_requestStream;
        std::string m_path;

        std::vector<HeaderItem> m_headerItems;
        RequestType m_type;
        std::string m_resourcePath;
    };

    class Response
    {
    public:
        void AddResponseCode(int code);
        void AddHeader(const HeaderItem& item);
        void AddData(const std::string& data);

        std::string GetRaw() const;

    private:
        int m_code;
        std::vector<HeaderItem> m_headers;
        std::string m_data;
    };

}

#endif // HTTP_PARSER_H
