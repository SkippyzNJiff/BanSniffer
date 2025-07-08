#pragma once
#ifndef CONSOLE_UTILS_H
#define CONSOLE_UTILS_H

#include <windows.h>
#ifdef max
#undef max
#endif
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>

class ConsoleUtils {
public:
    // Expose console handle for redirect operations
    static HANDLE hConsole;
    static WORD originalAttributes;

    enum Color {
        BLACK = 0,
        DARK_BLUE = 1,
        DARK_GREEN = 2,
        DARK_CYAN = 3,
        DARK_RED = 4,
        DARK_MAGENTA = 5,
        DARK_YELLOW = 6,
        DARK_WHITE = 7,
        GRAY = 8,
        BLUE = 9,
        GREEN = 10,
        CYAN = 11,
        RED = 12,
        MAGENTA = 13,
        YELLOW = 14,
        WHITE = 15
    };

    static void initialize() {
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hConsole, &csbi);
        originalAttributes = csbi.wAttributes;
    }

    static void setColor(Color foreground, Color background = BLACK) {
        SetConsoleTextAttribute(hConsole, (background << 4) | foreground);
    }

    static void resetColor() {
        SetConsoleTextAttribute(hConsole, originalAttributes);
    }

    static void printColored(const std::string& text, Color color) {
        setColor(color);
        std::cout << text;
        resetColor();
    }

    static void printHeader(const std::string& title, Color color = CYAN) {
        std::cout << "\n";
        setColor(color);
        std::cout << std::string(70, '=') << "\n";
        std::cout << " " << title << "\n";
        std::cout << std::string(70, '=') << "\n";
        resetColor();
    }

    static void printItem(const std::string& label, const std::string& value,
        Color labelColor = DARK_WHITE, Color valueColor = WHITE) {
        setColor(labelColor);
        std::cout << std::left << std::setw(25) << label << ": ";
        setColor(valueColor);
        std::cout << value << "\n";
        resetColor();
    }

    static void printSuccess(const std::string& message) {
        setColor(GREEN);
        std::cout << "[SUCCESS] ";
        resetColor();
        std::cout << message << "\n";
    }

    static void printError(const std::string& message) {
        setColor(RED);
        std::cout << "[ERROR] ";
        resetColor();
        std::cout << message << "\n";
    }

    static void printWarning(const std::string& message) {
        setColor(YELLOW);
        std::cout << "[WARNING] ";
        resetColor();
        std::cout << message << "\n";
    }

    static void printInfo(const std::string& message) {
        setColor(CYAN);
        std::cout << "[INFO] ";
        resetColor();
        std::cout << message << "\n";
    }

    static void clearScreen() {
        system("cls");
    }

    static void pause() {
        std::cout << "\nPress any key to continue...";
        std::cin.get();
    }

    static void printSubHeader(const std::string& title, Color color = CYAN) {
        setColor(color);
        std::cout << std::string(40, '-') << "\n";
        std::cout << "  " << title << "\n";
        std::cout << std::string(40, '-') << "\n";
        resetColor();
    }

    static void printBox(const std::string& content, Color borderColor = CYAN) {
        size_t maxLength = 0;
        std::vector<std::string> lines;
        std::stringstream ss(content);
        std::string line;

        while (std::getline(ss, line)) {
            lines.push_back(line);
            maxLength = std::max(maxLength, line.length());
        }

        setColor(borderColor);
        std::cout << "+" << std::string(maxLength + 2, '-') << "+\n";
        resetColor();

        for (const auto& l : lines) {
            setColor(borderColor);
            std::cout << "| ";
            resetColor();
            std::cout << std::left << std::setw(maxLength) << l;
            setColor(borderColor);
            std::cout << " |\n";
            resetColor();
        }

        setColor(borderColor);
        std::cout << "+" << std::string(maxLength + 2, '-') << "+\n";
        resetColor();
    }
};

#endif // CONSOLE_UTILS_H