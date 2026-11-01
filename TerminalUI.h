/** @file TerminalUI.h
 * @brief C++11 ncurses presentation and input adapter; has no workflow dependencies.
 */
#ifndef TERMINAL_UI_H
#define TERMINAL_UI_H
#include <memory>
#include <string>
#include <vector>
#include <functional>

/** Owns the terminal session and restores streams/terminal modes on every exit. */
class TerminalUI {
public:
    struct Choice { std::string value; std::string label; };
    // Presentation-only values. Callers supply engine/session data and actions.
    struct Detail {
        std::string label, value;
        int tone;
        Detail(std::string key = "", std::string text = "", int colour = 1)
            : label(key), value(text), tone(colour) {}
    };
    struct View {
        std::string kind, title, heading, subtitle, sideTitle, badge;
        int tone = 2;
        std::vector<Detail> details, summary;
        std::vector<std::string> columns;
        std::vector<std::vector<std::string> > rows;
        std::vector<std::vector<Detail> > rowDetails;
        std::vector<Choice> actions;
    };
    struct Field {
        std::string label, value;
        std::vector<std::string> options;
        bool readonly;
        Field(std::string name, std::string text = "", std::vector<std::string> choices = {}, bool fixed = false)
            : label(name), value(text), options(choices), readonly(fixed) {}
    };
    static void configure(const View& view);
    /** Modal cards/tables; returns the selected action or row value. */
    static std::string present(const View& view);
    /** Editable structured form; false cancels without submitting. */
    static bool form(const std::string& title, std::vector<Field>& fields,
        std::function<std::string(const std::vector<Field>&)> validate);
    static void clearView();
    explicit TerminalUI(bool enabled);
    ~TerminalUI();
    TerminalUI(const TerminalUI&) = delete;
    TerminalUI& operator=(const TerminalUI&) = delete;
    static bool available();
    static bool active();
    static void title(const std::string& text);
    /** Returns original text to existing validators; false means end-of-input. */
    static bool read(std::string& value, const std::string& prompt, bool numeric = false,
                     int minimum = 0, int maximum = 0);
    /** Supply choices for menus that were previously printed inline. */
    static void choices(const std::vector<Choice>& options);
    static void showOutput(const std::string& title);
private:
    class Impl;
    std::unique_ptr<Impl> impl;
    static Impl* current;
};
#endif
