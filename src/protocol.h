#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include <sstream>

/*
 * Type of messages:
 *
 *  |       type    |  sender  |       payload
 *   ---------------------------------------------
 *    SayHello      | worker   | [WorkerName:string]
 *    SendTask      | server   | [Data:string]
 *    RequestStatus | server   | []
 *    SendStatus    | worker   | [NumberOfTasks:int]
 *    Shutdown      | server   | []
 *
 */

namespace protocol
{

    enum class Command : int32_t {
        SayHello = 1,
        SendTask,
        RequestStatus,
        SendStatus,
        Shutdown
    };

    struct CommandData {
        Command cmd;
        std::string payload;
    };

    inline void CreateMessage(Command cmd, std::string &message, const std::string& payload = "")
    {
        std::stringstream ss;
        ss << (int32_t)cmd << ":" << payload;
        message = ss.str();
    }

    inline int ParseMessage(const std::string &message, CommandData &data) {
        // "command:payload"
        const size_t commandDelimPos = message.find_first_of(':');

        Command command = (Command)atoi(message.substr(0, commandDelimPos).c_str());
        data.cmd = command;
        switch (command) {
            case Command::SendTask:
                break;
            case Command::RequestStatus:
                break;
            case Command::SendStatus:
                data.payload = message.substr(commandDelimPos + 1, message.size());
                break;
            case Command::Shutdown:
                break;
            default:
                return -1;
        }

        return 0;
    }

}
#endif // _PROTOCOL_H_
