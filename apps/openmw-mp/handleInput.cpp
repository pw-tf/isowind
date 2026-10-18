namespace mwmp_input {
    std::string windowInputBuffer;
    void handler() {
        char c;
#ifndef WIN32
        while (kbhit()) {
            c = getch();
#else // on Windows conio.h getch() and kbhit() are deprecated, use _getch() and _kbhit() instead
        while (_kbhit()) {
            c = _getch();
#endif
            std::cout << c << std::flush;
            if (c == '\n' || c == '\r') { // handle carriage return as new line on Windows
                std::cout << std::endl;
                Script::Call<Script::CallbackIdentity("OnServerWindowInput")>(windowInputBuffer.c_str());
                windowInputBuffer.assign("");
            }
            else if (c == '\b') {
                auto size = windowInputBuffer.size();
                if (size > 0)
                    windowInputBuffer.erase(size - 1);
            }
            else windowInputBuffer += c;
        }
    }
}
