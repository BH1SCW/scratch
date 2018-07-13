﻿//
// CustomFormatter - shows how to implement a custom formatter.
//

#include <plog/Log.h>

namespace plog
{
    class MyFormatter
    {
    public:
        static std::string header() // This method returns a header for a new file. In our case it is empty.
        {
            return std::string();
        }

        static std::string format(const Record& record) // This method returns a string from a record.
        {
            std::stringstream ss;
            ss << record.getMessage() << "\n"; // Produce a simple string with a log message.

            return ss.str();
        }
    };
}

int main()
{
    plog::init<plog::MyFormatter>(plog::debug, "CustomFormatter.txt"); // Initialize the logger and pass our formatter as a template parameter to init function.

    LOGD << "A debug message!";

    return 0;
}
