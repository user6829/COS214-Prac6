#include "TerminalUI.h"
#include <ncurses.h>
#include <unistd.h>
#include <termios.h>
#include <algorithm>
#include <chrono>
#include <clocale>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <streambuf>
#include <stdexcept>
#include <cwchar>
#include <map>

namespace {
volatile std::sig_atomic_t interrupted = 0;
void interruptTerminal(int) { interrupted = 1; }
std::string lower(std::string text) {
    for (char& ch : text) if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
    return text;
}
std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    return first == std::string::npos ? "" : text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}
std::string quoted(const std::string& text) {
    std::ostringstream out;
    out << '"';
    const char* hex = "0123456789abcdef";
    for (unsigned char ch : text) {
        if (ch == '"' || ch == '\\') out << '\\' << ch;
        else if (ch == '\n') out << "\\n";
        else if (ch == '\r') out << "\\r";
        else if (ch == '\t') out << "\\t";
        else if (ch < 32) out << "\\u00" << hex[ch >> 4] << hex[ch & 15];
        else out << ch;
    }
    out << '"';
    return out.str();
}
std::size_t previousCharacter(const std::string& text, std::size_t cursor) {
    if (!cursor) return 0;
    --cursor;
    while (cursor && (static_cast<unsigned char>(text[cursor]) & 0xc0) == 0x80) --cursor;
    return cursor;
}
std::size_t nextCharacter(const std::string& text, std::size_t cursor) {
    if (cursor >= text.size()) return text.size();
    ++cursor;
    while (cursor < text.size() && (static_cast<unsigned char>(text[cursor]) & 0xc0) == 0x80) ++cursor;
    return cursor;
}
int characterWidth(const std::string& text, std::size_t start, std::size_t length) {
    std::mbstate_t state = {};
    wchar_t ch = 0;
    const auto count = std::mbrtowc(&ch, text.data() + start, length, &state);
    if (count == static_cast<std::size_t>(-1) || count == static_cast<std::size_t>(-2)) return 1;
    return std::max(0, ::wcwidth(ch));
}
std::string clip(const std::string& text, int columns) {
    if (columns <= 0) return "";
    std::size_t end = 0;
    int width = 0;
    while (end < text.size()) {
        const auto next = nextCharacter(text, end);
        const int size = characterWidth(text, end, next - end);
        if (width + size > columns) break;
        width += size; end = next;
    }
    return text.substr(0, end);
}
std::vector<std::string> wrap(const std::string& text, int columns) {
    std::vector<std::string> result;
    if (columns < 1) return result;
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) result.push_back("");
        while (!line.empty()) {
            std::string part = clip(line, columns);
            if (part.empty()) break;
            if(part.size()<line.size()) {
                const auto space=part.find_last_of(' ');
                if(space!=std::string::npos && space>part.size()/3) part.resize(space);
            }
            result.push_back(part);
            line.erase(0,part.size());
            while(!line.empty() && line[0]==' ') line.erase(0,1);
        }
    }
    return result;
}
}

class TerminalUI::Impl {
    class Output : public std::streambuf {
        Impl& ui;
    public:
        explicit Output(Impl& owner) : ui(owner) {}
        std::streamsize xsputn(const char* data, std::streamsize count) override {
            ui.append(std::string(data, static_cast<std::size_t>(count))); return count;
        }
        int_type overflow(int_type ch) override {
            if (!traits_type::eq_int_type(ch, traits_type::eof())) ui.append(std::string(1, traits_type::to_char_type(ch)));
            return traits_type::not_eof(ch);
        }
        int sync() override { ui.activity(); return 0; }
    } output;
    SCREEN* screen = nullptr;
    termios savedModes = {};
    bool savedTerminal = false;
    std::streambuf* oldOut = nullptr;
    std::streambuf* oldErr = nullptr;
    using SignalHandler = void (*)(int);
    SignalHandler oldInt = SIG_DFL, oldTerm = SIG_DFL;
    std::ofstream trace;
    std::vector<std::string> lines;
    std::string line, pending, heading = "Dashboard", hint;
    std::vector<Choice> supplied, lastOptions;
    View scene;
    bool sceneSet = false, consumed = false;
    int ox = 0, oy = 0, cw = 0, ch = 0;
    int menuX = 0, menuY = 0, menuW = 0, menuH = 0, rowStride = 1;
    std::vector<std::pair<short, std::vector<short> > > restoredColors;
    std::string lastPrompt;
    int lastMinimum = 0, lastMaximum = 0;
    bool interacting = false;
    bool escapeSequence = false;
    int escapeState = 0;
    int step = 0;
    unsigned long eventId = 0;
    std::chrono::steady_clock::time_point lastFrame = std::chrono::steady_clock::now();
public:
    Impl() : output(*this) {
        // Only character handling uses the user's locale. Numeric parsing stays unchanged.
        std::setlocale(LC_CTYPE, "");
        const char* tracePath = std::getenv("TIVIDY_UI_TRACE");
        if (tracePath && *tracePath) trace.open(tracePath);
        savedTerminal = tcgetattr(STDIN_FILENO, &savedModes) == 0;
        screen = newterm(nullptr, stdout, stdin);
        if (!screen) throw std::runtime_error("Could not initialize ncurses; use --plain");
        set_term(screen);
        raw(); noecho(); keypad(stdscr, TRUE); wtimeout(stdscr, 100);
        set_escdelay(30);
        if (has_colors()) {
            start_color();
            // Exact storyboard palette on mutable terminals, xterm-256 approximations otherwise.
            const int rgb[][3] = {{2,7,13},{7,19,31},{11,29,44},{0,217,255},{22,117,209},
                {234,243,250},{155,175,190},{54,232,121},{255,212,71},{255,101,107},{108,123,139}};
            const short indexed[] = {232,233,234,45,26,255,145,83,221,203,66};
            const short basic[] = {COLOR_BLACK,COLOR_BLACK,COLOR_BLACK,COLOR_CYAN,COLOR_BLUE,
                COLOR_WHITE,COLOR_WHITE,COLOR_GREEN,COLOR_YELLOW,COLOR_RED,COLOR_WHITE};
            short palette[11];
            for (int i = 0; i < 11; ++i) {
                palette[i] = COLORS >= 256 ? indexed[i] : basic[i];
                if (can_change_color() && COLORS >= 32) {
                    const short slot = static_cast<short>(COLORS - 11 + i);
                    short r, g, b;
                    if (color_content(slot, &r, &g, &b) == OK) restoredColors.push_back({slot,{r,g,b}});
                    init_color(slot, rgb[i][0] * 1000 / 255, rgb[i][1] * 1000 / 255, rgb[i][2] * 1000 / 255);
                    palette[i] = slot;
                }
            }
            init_pair(1,palette[5],palette[1]); init_pair(2,palette[3],palette[1]);
            init_pair(3,COLOR_WHITE,palette[4]); init_pair(4,palette[7],palette[1]);
            init_pair(5,palette[9],palette[1]); init_pair(6,palette[3],palette[1]);
            init_pair(7,palette[8],palette[1]); init_pair(8,palette[6],palette[1]);
            init_pair(9,palette[10],palette[1]); init_pair(10,palette[5],palette[2]);
            init_pair(11,palette[5],palette[0]); init_pair(12,palette[7],palette[2]);
            init_pair(13,palette[9],palette[2]); init_pair(14,palette[8],palette[2]);
            init_pair(15,palette[6],palette[2]); init_pair(16,palette[3],palette[2]);
        }
        bkgd(COLOR_PAIR(11));
        mouseinterval(120);
        mousemask(ALL_MOUSE_EVENTS, nullptr);
        oldInt = std::signal(SIGINT, interruptTerminal);
        oldTerm = std::signal(SIGTERM, interruptTerminal);
        oldOut = std::cout.rdbuf(&output);
        oldErr = std::cerr.rdbuf(&output);
        curs_set(0);
    }
    ~Impl() {
        if (oldOut) std::cout.rdbuf(oldOut);
        if (oldErr) std::cerr.rdbuf(oldErr);
        std::signal(SIGINT, oldInt);
        std::signal(SIGTERM, oldTerm);
        if (screen) {
            mousemask(0, nullptr); curs_set(1);
            for (const auto& saved : restoredColors)
                init_color(saved.first, saved.second[0], saved.second[1], saved.second[2]);
            endwin(); delscreen(screen);
        }
        if (savedTerminal) tcsetattr(STDIN_FILENO, TCSANOW, &savedModes);
    }
    void emit(const std::string& event, const std::string& text = "", const std::string& kind = "") {
        if (trace) {
            trace << "{\"event\":" << quoted(event) << ",\"text\":" << quoted(text)
                  << ",\"kind\":" << quoted(kind) << ",\"id\":" << eventId << "}\n";
            trace.flush();
        }
    }
    void append(const std::string& text) {
        emit("output", text);
        for (unsigned char ch : text) {
            if (escapeSequence) {
                if (escapeState == 0) { escapeState = ch == '[' ? 1 : 2; if (escapeState == 2) escapeSequence = false; }
                else if (ch >= '@' && ch <= '~') escapeSequence = false;
                continue;
            }
            if (ch == 27) { escapeSequence = true; escapeState = 0; continue; }
            if (ch == '\r') continue;
            pending += static_cast<char>(ch);
            if (ch == '\n') {
                lines.push_back(line);
                line.clear();
            } else if (ch >= 32 || ch == '\t') line += ch == '\t' ? ' ' : static_cast<char>(ch);
        }
        const std::string recent = lines.empty() ? line : lines.back() + line;
        std::smatch match;
        static const std::regex progress("Step ([1-5]) of 5");
        if (std::regex_search(recent, match, progress)) step = std::stoi(match[1]);
    }
    void text(int row, int col, const std::string& value, int width, int pair = 1, bool bold = false) {
        int rows, cols; getmaxyx(stdscr, rows, cols);
        if (row < 0 || row >= rows || col < 0 || col >= cols || width < 1) return;
        attrset(COLOR_PAIR(pair) | (bold ? A_BOLD : 0));
        const std::string shown = clip(value, std::min(width, cols - col));
        mvaddnstr(row, col, shown.c_str(), static_cast<int>(shown.size()));
    }
    void fill(int y, int x, int height, int width, int pair) {
        int rows, cols; getmaxyx(stdscr, rows, cols);
        if (x < 0 || y < 0 || x >= cols || y >= rows) return;
        attrset(COLOR_PAIR(pair));
        for (int r = y; r < std::min(rows, y + height); ++r)
            mvhline(r, x, ' ', std::min(width, cols - x));
    }
    void panel(int y, int x, int height, int width, const std::string& label, int tone = 2) {
        int rows, cols; getmaxyx(stdscr, rows, cols);
        height = std::min(height, rows - y); width = std::min(width, cols - x);
        if (y < 0 || x < 0 || height < 3 || width < 4) return;
        fill(y,x,height,width,1);
        attrset(COLOR_PAIR(tone));
        mvaddch(y, x, ACS_ULCORNER); mvhline(y, x+1, ACS_HLINE,width-2); mvaddch(y,x+width-1,ACS_URCORNER);
        for (int r=1;r<height-1;++r) { mvaddch(y+r,x,ACS_VLINE); mvaddch(y+r,x+width-1,ACS_VLINE); }
        mvaddch(y+height-1,x,ACS_LLCORNER); mvhline(y+height-1,x+1,ACS_HLINE,width-2); mvaddch(y+height-1,x+width-1,ACS_LRCORNER);
        if (!label.empty()) text(y,x+2," " + label + " ",width-4,tone,true);
    }
    bool frame(const std::string& footer) {
        erase();
        int rows, cols; getmaxyx(stdscr,rows,cols);
        cw=std::min(116,cols-4);
        const int preferred=sceneSet && scene.kind=="menu" ? (scene.title=="Dashboard"?21:26) :
            sceneSet && scene.kind=="result"?25:
            sceneSet && scene.kind=="routing"?(scene.details.size()>8?29:27):
            sceneSet && scene.kind=="programme"?23:
            sceneSet && scene.kind=="review"?(scene.details.size()>8?32:24):
            sceneSet && scene.kind=="progress"?28:32;
        ch=std::min(preferred,rows-2); ox=(cols-cw)/2; oy=(rows-ch)/2;
        if (rows < 18 || cols < 50) {
            text(std::max(0,rows/2),1,"Resize to at least 50 columns x 18 rows",cols-2,2);
            refresh(); return false;
        }
        panel(oy,ox,ch,cw,"");
        if(sceneSet && scene.title=="Dashboard") {
            text(oy+1,ox+cw/4-8,"T I V I D Y",20,2,true);
            text(oy+2,ox+cw/4-15,"// WORKFLOW MANAGEMENT SYSTEM",36,8);
            button(oy+1,ox+cw-24,"+ Engine Ready");
        } else {
            text(oy+1,ox+3,"T I V I D Y  /",18,2,true);
            text(oy+1,ox+23,sceneSet ? scene.title : heading,cw-27,1,true);
        }
        attrset(COLOR_PAIR(2)); mvhline(oy+3,ox+3,ACS_HLINE,cw-6);
        const std::string navigation=cw<72?"Arrows/Tab Move  Enter Select  Esc Back":footer;
        if(hint.empty()) text(oy+ch-2,ox+3,navigation,cw-6,8);
        else {
            const auto message=wrap(hint,cw-6);
            for(std::size_t i=0;i<message.size() && i<2;++i)
                text(oy+ch-3+static_cast<int>(i),ox+3,message[i],cw-6,5,true);
        }
        return true;
    }
    void detailRows(int y, int x, int width, int limit, const std::vector<Detail>& details, bool cards=false, bool stacked=false) {
        int labelWidth = 0;
        for (const auto& d : details) labelWidth = std::max(labelWidth,static_cast<int>(d.label.size()));
        labelWidth = std::min(labelWidth,std::max(10,cards?width-10:width/3));
        int row=y;
        for (const auto& d : details) {
            if (row>=y+limit) break;
            if (cards) fill(row,x,1,width,10);
            const int ink=cards?(d.tone==4?12:d.tone==5?13:d.tone==7?14:d.tone==2?16:10):d.tone;
            if(stacked && !d.label.empty()) {
                text(row++,x,d.label,width,15);
                for(const auto& part:wrap(d.value,width)) {
                    if(row>=y+limit) break;
                    fill(row,x,1,width,10); text(row++,x,part,width,ink,true);
                }
                ++row; continue;
            }
            if (d.label.empty()) {
                for (const auto& part : wrap(d.value,width)) {
                    if (row>=y+limit) break;
                    text(row++,x,part,width,d.tone);
                }
            } else {
                text(row,x,d.label,labelWidth,cards?15:8);
                text(row,x+labelWidth," : ",3,cards?15:8);
                const auto pieces=wrap(d.value,std::max(1,width-labelWidth-3));
                for (const auto& part : pieces) {
                    if (row>=y+limit) break;
                    text(row++,x+labelWidth+3,part,width-labelWidth-3,ink,true);
                }
                if (pieces.empty()) ++row;
            }
            if (cards && row<y+limit) ++row;
        }
    }
    void button(int y,int x,const std::string& label,bool selected=false) {
        text(y,x,"[ " + label + " ]",static_cast<int>(label.size())+4,selected?3:2,true);
    }
    void renderScene(const std::vector<Choice>& options, int selected, int& top, const std::string& typed) {
        const int left=ox+3, right=ox+cw-3, body=oy+5, bottom=oy+ch-4;
        if(scene.title!="Dashboard") text(oy+4,left,scene.heading.empty()?heading:scene.heading,cw-6,2,true);
        if (scene.kind=="menu") {
            const bool split=cw>=72;
            const int side=split?std::max(27,cw/3):0;
            menuX=left; menuY=body+3; menuW=cw-6-(split?side+2:0); menuH=bottom-menuY;
            if (scene.title=="Dashboard") { menuY=body+1; menuH=bottom-menuY; }
            else text(body,left,scene.subtitle,menuW,8);
            panel(menuY-1,menuX,menuH+2,menuW,scene.title=="Dashboard"?"MAIN MENU":"ACTIONS");
            if (split) {
                panel(body,left+menuW+2,bottom-body+2,side,scene.sideTitle);
                detailRows(body+2,left+menuW+4,side-4,bottom-body-2,scene.summary,true);
            }
            menuX+=2; menuW-=4; menuH-=1;
            rowStride=menuH>=static_cast<int>(options.size())*2?2:1;
        } else if (!scene.columns.empty()) {
            const bool split=!scene.rowDetails.empty() && cw>=85;
            const int side=split?32:0;
            menuX=left; menuY=body+2; menuW=cw-6-(split?side+2:0); menuH=bottom-menuY-2;
            panel(body,menuX,bottom-body+1,menuW,scene.kind=="history"?"AUDIT TRAIL":"");
            const int usable=menuW-4;
            std::vector<std::size_t> visibleColumns;
            for(std::size_t i=0;i<scene.columns.size();++i) visibleColumns.push_back(i);
            if(usable<65 && scene.columns.size()==5) visibleColumns={0,1,4};
            if(usable<65 && scene.columns.size()==4) visibleColumns={0,2,3};
            std::vector<int> widths(visibleColumns.size(),usable/static_cast<int>(visibleColumns.size()));
            if (widths.size()==3) { widths[0]=5; widths[2]=10; widths[1]=usable-15; }
            if (widths.size()==4) { widths[0]=5; widths[1]=20; widths[3]=18; widths[2]=usable-43; }
            if (widths.size()==5) { widths[0]=4; widths[1]=13; widths[2]=std::max(14,usable-54); widths[3]=22; widths[4]=15; }
            if(visibleColumns.size()!=scene.columns.size()) {
                widths[0]=4; widths[2]=std::min(15,usable/3); widths[1]=usable-widths[0]-widths[2];
            }
            int x=menuX+2;
            for (std::size_t i=0;i<widths.size();++i) { text(body+1,x,scene.columns[visibleColumns[i]],widths[i]-1,2,true); x+=widths[i]; }
            menuX+=2; menuW-=4; rowStride=1;
            const int visible=std::max(1,menuH);
            if(selected<top) top=selected;
            if(selected>=top+visible) top=selected-visible+1;
            for(int r=0;r<visible && top+r<static_cast<int>(scene.rows.size());++r) {
                const int index=top+r;
                const bool focus=index==selected && scene.kind!="history";
                if(focus) fill(menuY+r,menuX,1,menuW,3);
                x=menuX;
                for(std::size_t c=0;c<widths.size() && c<scene.rows[index].size();++c) {
                    text(menuY+r,x,scene.rows[index][visibleColumns[c]],widths[c]-1,focus?3:color(scene.rows[index][visibleColumns[c]]),focus);
                    x+=widths[c];
                }
            }
            if(scene.rows.empty()) text(menuY+2,menuX+2,"No work items available in this session.",menuW-4,8);
            if(split) {
                panel(body,left+menuW+6,bottom-body+1,side,scene.sideTitle);
                if(selected>=0 && selected<static_cast<int>(scene.rowDetails.size()))
                    detailRows(body+2,left+menuW+8,side-4,bottom-body-2,scene.rowDetails[selected]);
            }
            if(scene.kind!="programme" && scene.kind!="history")
                text(bottom+1,left,"[ View Details: Enter ]   [ Refresh: R ]   [ Back: Esc ]",cw-6,2);
            if(scene.kind=="programme") text(bottom+1,left,"Up/Down Navigate   Enter Select   Esc Cancel",cw-6,8);
            if(scene.kind=="history") text(bottom+1,left,"Up/Down Scroll   PgUp/PgDn   Enter Event Details   Esc Back",cw-6,2);
            return;
        } else {
            const bool split=!scene.summary.empty() && cw>=78;
            const int side=split?30:0;
            const int width=cw-6-(split?side+2:0);
            if(scene.kind!="review") panel(body,left,bottom-body-1,width,"",scene.tone);
            int start=body+2;
            if(!scene.badge.empty() && scene.kind!="routing") {
                fill(start,left+2,2,width-4,scene.tone==5?13:scene.tone==7?14:12);
                text(start,left+4,(scene.tone==5?"(X)  ":scene.tone==7?"(!)  ":"(+)  ")+scene.badge,width-8,scene.tone==5?13:scene.tone==7?14:12,true);
                start+=3;
            }
            if(!scene.subtitle.empty()) { text(start++,left+3,scene.subtitle,width-6,8); ++start; }
            if(scene.kind=="progress") {
                int slots=std::min(4,static_cast<int>(scene.rows.size()));
                int cell=std::max(1,(width-6)/std::max(1,slots));
                for(int i=0;i<slots;++i) {
                    const auto& row=scene.rows[i];
                    int tone=row.size()>1?(row[1]=="done"?4:row[1]=="current"?7:9):9;
                    text(start,left+3+i*cell,tone==4?"(+) ---->":tone==7?"(@) ---->":"(o) ---->",cell-1,tone,true);
                    const auto label=wrap(row[0],cell-2);
                    for(int r=0;r<2 && r<static_cast<int>(label.size());++r)
                        text(start+2+r,left+3+i*cell,label[r],cell-2,tone,true);
                }
                start+=5;
            }
            const int offset=std::min(top,static_cast<int>(scene.details.size()));
            const std::vector<Detail> shown(scene.details.begin()+offset,scene.details.end());
            if(scene.kind=="review") {
                panel(body,left,6,width,scene.heading.find("MATERIAL")!=std::string::npos?"REQUEST DETAILS":"APPLICATION DETAILS");
                std::vector<Detail> identity(scene.details.begin(),scene.details.begin()+std::min<std::size_t>(3,scene.details.size()));
                detailRows(body+2,left+3,width-6,3,identity);
                panel(body+7,left,bottom-body-8,width,"REQUIREMENTS CHECK");
                const int first=std::min(3+top,static_cast<int>(scene.details.size()));
                std::vector<Detail> checks(scene.details.begin()+first,scene.details.end());
                detailRows(body+8,left+3,width-6,bottom-body-10,checks);
            } else detailRows(start,left+3,width-6,bottom-start-(scene.kind=="routing"?5:2),shown);
            if(scene.kind=="routing" && !scene.badge.empty()) {
                text(bottom-5,left+width/2,"|",2,4,true);
                fill(bottom-4,left+2,2,width-4,12);
                text(bottom-4,left+4,"Assigned: "+scene.badge.substr(scene.badge.find(':')+1),width-8,12,true);
            }
            if(split) {
                panel(body,left+width+2,scene.kind=="result"?std::min(13,bottom-body-1):bottom-body-1,side,scene.sideTitle,scene.kind=="result"?7:2);
                detailRows(body+2,left+width+4,side-4,bottom-body-4,scene.summary,true,true);
            }
            // Compact status card remains visible in stacked layouts.
            if(!split && !scene.summary.empty()) text(bottom-2,left+3,scene.summary.front().label+": "+scene.summary.front().value,width-6,7,true);
            menuX=left+2; menuY=bottom; menuW=cw-10; menuH=1; rowStride=1;
            int total=0;
            for(const auto& option:options) total+=static_cast<int>(option.label.size())+7;
            menuX=left+std::max(2,(cw-6-total)/2);
            int x=menuX;
            for(std::size_t i=0;i<options.size();++i) {
                if(x+static_cast<int>(options[i].label.size())+4>right) break;
                button(menuY,x,options[i].label,static_cast<int>(i)==selected);
                x+=static_cast<int>(options[i].label.size())+7;
            }
            return;
        }
        const int visible=std::max(1,menuH/rowStride);
        if(selected<top) top=selected;
        if(selected>=top+visible) top=selected-visible+1;
        for(int r=0;r<visible && top+r<static_cast<int>(options.size());++r) {
            int index=top+r, y=menuY+r*rowStride;
            if(index==selected) fill(y,menuX,1,menuW,3);
            text(y,menuX+1,"["+options[index].value+"] "+options[index].label,menuW-2,index==selected?3:1,index==selected);
        }
        if(!typed.empty()) text(oy+ch-2,left,"Shortcut: "+typed,cw-6,2);
    }
    std::vector<std::string> content() const {
        std::vector<std::string> result = lines;
        if (!line.empty()) result.push_back(line);
        return result;
    }
    int color(const std::string& value) const {
        const std::string lowered = lower(value);
        if (lowered.find("pending") != std::string::npos || lowered.find("awaiting") != std::string::npos) return 7;
        if (lowered.find("error") != std::string::npos || lowered.find("[fail]") != std::string::npos || lowered.find("reject") != std::string::npos ||
            lowered.find("this field is required") != std::string::npos || lowered.find("enter a valid") != std::string::npos ||
            lowered.find("enter a whole") != std::string::npos || lowered.find("please enter") != std::string::npos ||
            lowered.find("does not exist on the calendar") != std::string::npos) return 5;
        if (lowered.find("[pass]") != std::string::npos || lowered.find("accepted") != std::string::npos || lowered.find("approved") != std::string::npos || lowered.find("saved ") != std::string::npos) return 4;
        if (value.find("[State]") != std::string::npos || value.find("Step ") != std::string::npos) return 2;
        return 1;
    }
    void logPanel(int y, int x, int height, int width, const std::string& label) {
        panel(y, x, height, width, label);
        if (height < 3) return;
        const auto all = content();
        std::vector<std::string> wrapped;
        for (const auto& value : all) {
            // Keep navigation in its own panel; F2 retains the full transcript.
            static const std::regex menuRow("^[[:space:]]*[0-9]+[.)][[:space:]]+");
            if (std::regex_search(value, menuRow)) continue;
            const auto pieces = wrap(value, width - 4);
            wrapped.insert(wrapped.end(), pieces.begin(), pieces.end());
        }
        const int start = std::max(0, static_cast<int>(wrapped.size()) - height + 2);
        for (int r = 0; r < height - 2 && start + r < static_cast<int>(wrapped.size()); ++r)
            text(y + 1 + r, x + 2, wrapped[start + r], width - 4, color(wrapped[start + r]));
    }
    void activity() {
        if (interacting) return;
        const auto now = std::chrono::steady_clock::now();
        if (now - lastFrame < std::chrono::milliseconds(50)) return;
        lastFrame = now;
        if (frame("Processing through the original workflow engine...")) {
            const char spinner[] = "|/-\\";
            const auto tick = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() / 100;
            std::string progress = "Engine activity "; progress += spinner[tick % 4];
            if (step) progress += "    Step " + std::to_string(step) + " of 5  [" + std::string(step * 3, '=') + std::string((5 - step) * 3, ' ') + "]";
            text(oy+4, ox+3, progress, cw-6, 2, true);
            logPanel(oy+6, ox+3, ch-10, cw-6, "WORKFLOW ACTIVITY");
            refresh();
        }
    }
    int key() {
        if (interrupted) return 4;
        const int ch = wgetch(stdscr);
        if (ch == 3 || ch == 4) { interrupted = 1; return 4; }
        return ch;
    }
    void ready(const std::string& kind, const std::string& prompt) {
        ++eventId;
        if(trace) {
            int rows,cols; getmaxyx(stdscr,rows,cols);
            std::string cells, colours;
            for(int y=0;y<rows;++y) {
                for(int x=0;x<cols;++x) {
                    const chtype c=mvinch(y,x);
                    char glyph=static_cast<char>(c&A_CHARTEXT);
                    if(c&A_ALTCHARSET) {
                        const std::map<char,std::string> boxes={{'q',"─"},{'x',"│"},{'l',"┌"},{'k',"┐"},{'m',"└"},{'j',"┘"},{'t',"├"},{'u',"┤"},{'w',"┬"},{'v',"┴"}};
                        const auto box=boxes.find(glyph);
                        cells+=box==boxes.end()?"+":box->second;
                    } else cells+=glyph>=32 && glyph<127?glyph:'?';
                    const int pair=PAIR_NUMBER(c);
                    colours+=static_cast<char>('A'+pair);
                }
                cells+='\n'; colours+='\n';
            }
            emit("frame",cells,sceneSet?scene.kind:kind);
            emit("colours",colours);
        }
        emit("ready", prompt, kind);
    }
    void accepted(const std::string& value) {
        emit("answer", value);
        if (!line.empty()) { lines.push_back(line); line.clear(); }
        lines.push_back("  > " + value);
        pending.clear(); hint.clear();
    }
    std::vector<Choice> numbered(int minimum, int maximum) {
        if (!supplied.empty()) { auto result = supplied; supplied.clear(); return result; }
        std::vector<Choice> result;
        static const std::regex option("(^|[ \\t])([0-9]{1,2})[.] +");
        std::istringstream input(pending);
        std::string row;
        while (std::getline(input, row)) {
            std::vector<std::smatch> matches;
            for (std::sregex_iterator it(row.begin(), row.end(), option), end; it != end; ++it) matches.push_back(*it);
            for (std::size_t i = 0; i < matches.size(); ++i) {
                const int value = std::stoi(matches[i][2]);
                if (value < minimum || value > maximum) continue;
                if (value == 1 && std::any_of(result.begin(), result.end(), [](const Choice& c) { return c.value == "1"; })) result.clear();
                const auto start = static_cast<std::size_t>(matches[i].position() + matches[i].length());
                const auto finish = i + 1 < matches.size() ? static_cast<std::size_t>(matches[i + 1].position()) : row.size();
                const std::string label = trim(row.substr(start, finish - start));
                if (!label.empty()) result.push_back({std::to_string(value), label});
            }
        }
        return result;
    }
    std::string backValue(const std::vector<Choice>& options) const {
        for (const auto& c : options) if (lower(c.label).find("choose a different") != std::string::npos) return c.value;
        for (const auto& c : options) if (lower(c.label).find("cancel") != std::string::npos || lower(c.label) == "stop" || lower(c.label) == "back") return c.value;
        for (const auto& c : options) if (lower(c.label) == "exit") return c.value;
        return "";
    }
    bool select(std::string& value, const std::string& prompt, const std::vector<Choice>& options, bool confirmation) {
        int selected = confirmation ? 1 : 0;
        int top = 0;
        std::string typed;
        bool first = true;
        for (;;) {
            const bool card=sceneSet && scene.kind!="menu" && scene.columns.empty();
            if (frame(confirmation ? "Tab / Arrows Navigate    Enter Confirm    Esc Cancel" : card?
                "Left/Right Actions    Enter Select    PgUp/PgDn Details    Esc Back":
                "Up/Down Navigate    Enter Select    Esc Back    F2 Activity")) {
                if (!sceneSet) {
                    scene.kind="menu"; scene.title=heading; scene.heading=confirmation?"CONFIRM YOUR DECISION":heading;
                    scene.subtitle=prompt; scene.sideTitle="CONTEXT";
                    const auto all=content();
                    for(std::size_t i=all.size()>6?all.size()-6:0;i<all.size();++i)
                        scene.summary.push_back({"",all[i],8});
                    sceneSet=true;
                }
                renderScene(options,selected,top,typed);
                curs_set(0); refresh();
                if(first) { ready(confirmation?"confirm":"menu",prompt); first=false; }
            }
            const int ch = key();
            if (ch == 4) return false;
            if (ch == KEY_RESIZE) { first = true; continue; }
            if (ch == KEY_F(2)) { viewer("Full activity", content()); continue; }
            if(scene.kind!="menu" && scene.columns.empty() && (ch==KEY_NPAGE || ch==KEY_PPAGE)) {
                top=std::max(0,std::min(std::max(0,static_cast<int>(scene.details.size())-1),top+(ch==KEY_NPAGE?3:-3)));
                continue;
            }
            if (ch == 'r' && scene.kind=="work") { value="refresh"; return true; }
            if (ch == KEY_UP || ch == KEY_LEFT || ch == KEY_BTAB) { selected = (selected + options.size() - 1) % options.size(); typed.clear(); }
            else if (ch == KEY_DOWN || ch == KEY_RIGHT || ch == '\t') { selected = (selected + 1) % options.size(); typed.clear(); }
            else if (ch == KEY_HOME) { selected = 0; typed.clear(); }
            else if (ch == KEY_END) { selected = static_cast<int>(options.size()) - 1; typed.clear(); }
            else if (ch == KEY_PPAGE) { selected = std::max(0, selected - 5); typed.clear(); }
            else if (ch == KEY_NPAGE) { selected = std::min(static_cast<int>(options.size()) - 1, selected + 5); typed.clear(); }
            else if (ch == 27) {
                const std::string back = confirmation ? "n" : scene.kind=="programme" || scene.kind=="work" || scene.kind=="history" ? "back" : backValue(options);
                if (!back.empty()) { value = back; accepted(value); return true; }
                hint = "This selection is required. Use arrows and Enter, or F2 to inspect output.";
            } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) { if (!typed.empty()) typed.pop_back(); }
            else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
                value = typed.empty() ? options[selected].value : typed;
                if(!typed.empty() && std::all_of(typed.begin(),typed.end(),[](char c){return c>='0' && c<='9';}) &&
                    std::none_of(options.begin(),options.end(),[&](const Choice& c){return c.value==typed;})) {
                    try { const int index=std::stoi(typed)-1;
                        if(index>=0 && index<static_cast<int>(options.size())) value=options[index].value;
                    } catch(const std::exception&) {}
                }
                accepted(value); return true;
            } else if (ch == KEY_MOUSE) {
                MEVENT event;
                if (getmouse(&event) == OK) {
                    const int index=top+(event.y-menuY)/std::max(1,rowStride);
                    if ((event.bstate & (BUTTON1_CLICKED | BUTTON1_PRESSED | BUTTON1_DOUBLE_CLICKED)) &&
                        event.x>=menuX && event.x<menuX+menuW && event.y>=menuY && event.y<menuY+menuH &&
                        index>=0 && index<static_cast<int>(options.size())) {
                        if(scene.kind!="menu" && scene.columns.empty()) {
                            int x=menuX;
                            for(std::size_t i=0;i<options.size();++i) {
                                const int w=static_cast<int>(options[i].label.size())+7;
                                if(event.x>=x && event.x<x+w) { value=options[i].value; accepted(value); return true; }
                                x+=w;
                            }
                        } else { value=options[index].value; accepted(value); return true; }
                    }
                    if (event.bstate & BUTTON4_PRESSED) selected = std::max(0, selected - 1);
#ifdef BUTTON5_PRESSED
                    if (event.bstate & BUTTON5_PRESSED) selected = std::min(static_cast<int>(options.size()) - 1, selected + 1);
#endif
                }
            } else if (ch >= 32 && ch < 127) {
                typed += static_cast<char>(ch);
                for (std::size_t i = 0; i < options.size(); ++i) if (options[i].value == typed) selected = static_cast<int>(i);
            }
        }
    }
    bool input(std::string& value, const std::string& prompt) {
        value.clear(); std::size_t cursor = 0, offset = 0;
        int focus = 0;
        bool first = true;
        for (;;) {
            if (frame("Enter submits the original value. Tab: controls. Esc/Ctrl-U: clear field.")) {
                const int fieldWidth=cw-12;
                panel(oy+5,ox+3,ch-10,cw-6,"DETAILS / VALIDATION");
                text(oy+7,ox+6,prompt,cw-12,2,true);
                const auto recent=content();
                int row=oy+10;
                for(std::size_t i=recent.size()>6?recent.size()-6:0;i<recent.size() && row<oy+ch-10;++i)
                    text(row++,ox+6,recent[i],cw-12,color(recent[i]));
                if(cursor<offset) offset=cursor;
                while(clip(value.substr(offset,cursor-offset),fieldWidth-2).size()<cursor-offset)
                    offset=nextCharacter(value,offset);
                fill(oy+ch-8,ox+6,1,fieldWidth,focus==0?3:10);
                text(oy+ch-8,ox+7,value.substr(offset),fieldWidth-2,focus==0?3:1);
                button(oy+ch-5,ox+6,"Submit",focus==1);
                button(oy+ch-5,ox+24,"Activity",focus==2);
                button(oy+ch-5,ox+44,"Clear",focus==3);
                int cursorWidth=0;
                for(std::size_t pos=offset;pos<cursor;) {
                    const auto next=nextCharacter(value,pos); cursorWidth+=characterWidth(value,pos,next-pos); pos=next;
                }
                curs_set(focus==0?1:0); move(oy+ch-8,std::min(ox+cw-7,ox+7+cursorWidth)); refresh();
                if (first) { ready("input", prompt); first = false; }
            }
            const int ch = key();
            if (ch == 4) return false;
            if (ch == KEY_RESIZE) { first = true; continue; }
            if (ch == KEY_F(2)) { viewer("Full activity", content()); continue; }
            if (ch == '\t') { focus = (focus + 1) % 4; continue; }
            if (ch == KEY_BTAB) { focus = (focus + 3) % 4; continue; }
            if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
                if (focus == 2) { viewer("Full activity", content()); continue; }
                if (focus == 3) { value.clear(); cursor = offset = 0; focus = 0; continue; }
                accepted(value); return true;
            }
            if (ch == KEY_MOUSE) {
                MEVENT event;
                if (getmouse(&event) == OK && (event.bstate & (BUTTON1_CLICKED | BUTTON1_PRESSED))) {
                    if (event.y == oy+ch-5) {
                        if (event.x >= ox+6 && event.x <= ox+17) { accepted(value); return true; }
                        if (event.x >= ox+24 && event.x <= ox+37) viewer("Full activity", content());
                        if (event.x >= ox+44 && event.x <= ox+54) { value.clear(); cursor = offset = 0; focus = 0; }
                    } else focus = 0;
                }
                continue;
            }
            focus = 0;
            if (ch == 27 || ch == 21) { value.clear(); cursor = offset = 0; }
            else if (ch == KEY_LEFT) cursor = previousCharacter(value, cursor);
            else if (ch == KEY_RIGHT) cursor = nextCharacter(value, cursor);
            else if (ch == KEY_HOME || ch == 1) cursor = 0;
            else if (ch == KEY_END || ch == 5) cursor = value.size();
            else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
                const auto previous = previousCharacter(value, cursor); value.erase(previous, cursor - previous); cursor = previous;
            } else if (ch == KEY_DC) value.erase(cursor, nextCharacter(value, cursor) - cursor);
            else if (ch >= 32 && ch < 256) { value.insert(cursor, 1, static_cast<char>(ch)); ++cursor; }
        }
    }
    void viewer(const std::string& title, const std::vector<std::string>& source) {
        int position = 0;
        bool first = true;
        for (;;) {
            if (frame("Up/Down or Page Up/Down scroll. Home/End. Enter or Esc returns.")) {
                std::vector<std::string> wrapped;
                for (const auto& row : source) {
                    const auto part = wrap(row, cw - 12); wrapped.insert(wrapped.end(), part.begin(), part.end());
                }
                panel(oy+5, ox+3, ch-9, cw-6, title);
                const int visible = ch - 12;
                position = std::max(0, std::min(position, std::max(0, static_cast<int>(wrapped.size()) - visible)));
                for (int r = 0; r < visible && position + r < static_cast<int>(wrapped.size()); ++r)
                    text(oy+7+r, ox+5, wrapped[position+r], cw-10, color(wrapped[position+r]));
                text(oy+ch-4, ox+cw-32, "[ Back ]  " + std::to_string(position + 1) + "/" + std::to_string(wrapped.size()), 29, 2, true);
                curs_set(0); refresh();
                if (first) { ready("viewer", title); first = false; }
            }
            const int ch = key();
            if (ch == 4 || ch == '\n' || ch == '\r' || ch == KEY_ENTER || ch == 27) return;
            if (ch == KEY_RESIZE) { first = true; continue; }
            int rows, cols; getmaxyx(stdscr, rows, cols); (void)cols;
            if (ch == KEY_UP) --position;
            else if (ch == KEY_DOWN) ++position;
            else if (ch == KEY_PPAGE) position -= std::max(1, rows - 9);
            else if (ch == KEY_NPAGE) position += std::max(1, rows - 9);
            else if (ch == KEY_HOME) position = 0;
            else if (ch == KEY_END) position = static_cast<int>(source.size()) * 1000;
            else if (ch == KEY_MOUSE) {
                MEVENT event;
                if (getmouse(&event) == OK) {
                    if (event.bstate & BUTTON4_PRESSED) position -= 3;
#ifdef BUTTON5_PRESSED
                    if (event.bstate & BUTTON5_PRESSED) position += 3;
#endif
                    if ((event.bstate & (BUTTON1_CLICKED | BUTTON1_PRESSED)) && event.y == rows - 4) return;
                }
            }
        }
    }
    bool read(std::string& value, const std::string& prompt, bool numeric, int minimum, int maximum) {
        interacting = true;
        const std::string question = pending;
        std::vector<Choice> options;
        if (numeric) options = numbered(minimum, maximum);
        if (numeric && options.empty() && lastPrompt == prompt && minimum == lastMinimum && maximum == lastMaximum &&
            (lower(question).find("number") != std::string::npos || lower(question).find("whole") != std::string::npos)) options = lastOptions;
        const bool confirmation = !numeric && (prompt.find("(y/n)") != std::string::npos || question.find("Configure optional routing rule? (y/n)") != std::string::npos);
        append(prompt);
        bool result;
        if (confirmation) result = select(value, prompt, {{"y", "Yes"}, {"n", "No"}}, true);
        else if (!options.empty()) {
            lastOptions = options; lastPrompt = prompt; lastMinimum = minimum; lastMaximum = maximum;
            result = select(value, prompt, options, false);
        } else result = input(value, prompt);
        sceneSet=false; scene=View();
        interacting = false;
        if (!result) emit("eof");
        return result;
    }
    void configure(const View& view) { scene=view; sceneSet=true; heading=view.title; }
    std::string present(const View& view) {
        const View previous=scene; const bool hadScene=sceneSet;
        configure(view);
        std::vector<Choice> options=view.actions;
        if(options.empty()) options.push_back({"back","Back"});
        interacting=true;
        std::string result;
        const bool ok=select(result,view.heading,options,false);
        interacting=false; scene=previous; sceneSet=hadScene; consumed=true;
        return ok?result:"back";
    }
    bool form(const std::string& title,std::vector<Field>& fields,
              const std::function<std::string(const std::vector<Field>&)>& validate) {
        scene=View(); scene.title=title; sceneSet=true; heading=title;
        interacting=true;
        int focus=0,top=0;
        std::size_t cursor=fields.empty()?0:fields[0].value.size();
        bool first=true;
        const int count=static_cast<int>(fields.size());
        for(;;) {
            if(frame("Tab Next Field   Shift-Tab Previous   Left/Right Choice   Enter Next / Submit   Esc Cancel")) {
                text(oy+4,ox+3,cw<80 && focus<count?"FIELD: "+fields[focus].label:"APPLICATION DETAILS",cw-6,2,true);
                const int y=oy+6, visible=std::max(1,ch-12), width=cw-10;
                panel(y-1,ox+3,visible+2,cw-6,"");
                if(focus<count) {
                    if(focus<top) top=focus;
                    if(focus>=top+visible) top=focus-visible+1;
                }
                const int labelWidth=std::min(26,std::max(16,width/3));
                for(int r=0;r<visible && top+r<count;++r) {
                    const int index=top+r;
                    const Field& field=fields[index];
                    text(y+r,ox+5,field.label,labelWidth-1,index==focus?2:8,index==focus);
                    text(y+r,ox+5+labelWidth,":",2,8);
                    const int valueX=ox+7+labelWidth, valueWidth=cw-labelWidth-13;
                    fill(y+r,valueX,1,valueWidth,index==focus?3:10);
                    std::string value=field.value.empty()?" ":field.value;
                    if(!field.options.empty()) value="< "+value+" >";
                    if(field.label=="Mathematics type" && cw>=90)
                        value=field.value=="Mathematics"?"(o) Mathematics    ( ) Mathematical Literacy":"( ) Mathematics    (o) Mathematical Literacy";
                    text(y+r,valueX+1,value,valueWidth-2,index==focus?3:field.readonly?8:1,index==focus);
                }
                const int buttons=oy+ch-5;
                button(buttons,ox+cw/2-18,"Submit",focus==count);
                button(buttons,ox+cw/2+3,"Cancel",focus==count+1);
                if(hint.empty()) text(oy+ch-3,ox+5,"English NSC bands: 4=50-59%  5=60-69%  6=70-79%  7=80-100%",cw-10,8);
                if(focus<count && !fields[focus].readonly && fields[focus].options.empty()) {
                    curs_set(1);
                    move(y+focus-top, std::min(ox+cw-7,ox+8+labelWidth+static_cast<int>(cursor)));
                } else curs_set(0);
                refresh();
                if(first) { ready("form",title); first=false; }
            }
            const int keypress=key();
            if(keypress==KEY_RESIZE) { first=true; continue; }
            if(keypress==4 || keypress==27) { interacting=false; sceneSet=false; scene=View(); consumed=true; return false; }
            if(keypress==KEY_F(2)) { viewer("Full activity",content()); continue; }
            if(keypress=='\t' || keypress==KEY_DOWN || keypress==KEY_BTAB || keypress==KEY_UP) {
                focus=(focus+count+2+((keypress==KEY_BTAB || keypress==KEY_UP)?-1:1))%(count+2);
                cursor=focus<count?fields[focus].value.size():0; continue;
            }
            if(keypress=='\n' || keypress=='\r' || keypress==KEY_ENTER) {
                if(focus==count+1) { interacting=false; sceneSet=false; scene=View(); consumed=true; return false; }
                if(focus==count) {
                    hint=validate(fields);
                    if(hint.empty()) { emit("answer","form submitted"); interacting=false; sceneSet=false; scene=View(); consumed=true; return true; }
                    first=true; continue;
                }
                ++focus; cursor=focus<count?fields[focus].value.size():0; continue;
            }
            if(keypress==KEY_MOUSE) {
                MEVENT event;
                if(getmouse(&event)==OK && event.bstate&(BUTTON1_CLICKED|BUTTON1_PRESSED)) {
                    const int index=top+event.y-(oy+6);
                    if(event.y==oy+ch-5) {
                        const int submitX=ox+cw/2-18, cancelX=ox+cw/2+3;
                        if(event.x>=submitX && event.x<submitX+10) focus=count;
                        else if(event.x>=cancelX && event.x<cancelX+10) focus=count+1;
                        else continue;
                        if(focus==count+1) { interacting=false; sceneSet=false; scene=View(); consumed=true; return false; }
                        hint=validate(fields);
                        if(hint.empty()) { emit("answer","form submitted"); interacting=false; sceneSet=false; scene=View(); consumed=true; return true; }
                        first=true;
                    }
                    else if(index>=0 && index<count) { focus=index; cursor=fields[focus].value.size(); }
                }
                continue;
            }
            if(focus>=count || fields[focus].readonly) continue;
            Field& field=fields[focus];
            if(!field.options.empty()) {
                if(keypress==KEY_LEFT || keypress==KEY_RIGHT || keypress==' ') {
                    auto found=std::find(field.options.begin(),field.options.end(),field.value);
                    int index=found==field.options.end()?0:static_cast<int>(found-field.options.begin());
                    index=(index+static_cast<int>(field.options.size())+(keypress==KEY_LEFT?-1:1))%field.options.size();
                    field.value=field.options[index];
                }
                continue;
            }
            if(keypress==KEY_LEFT) cursor=previousCharacter(field.value,cursor);
            else if(keypress==KEY_RIGHT) cursor=nextCharacter(field.value,cursor);
            else if(keypress==KEY_HOME) cursor=0;
            else if(keypress==KEY_END) cursor=field.value.size();
            else if(keypress==21) { field.value.clear(); cursor=0; }
            else if(keypress==KEY_BACKSPACE || keypress==127 || keypress==8) {
                const auto previous=previousCharacter(field.value,cursor);
                field.value.erase(previous,cursor-previous); cursor=previous;
            } else if(keypress==KEY_DC) field.value.erase(cursor,nextCharacter(field.value,cursor)-cursor);
            else if(keypress>=32 && keypress<256 && field.value.size()<200) {
                field.value.insert(cursor,1,static_cast<char>(keypress)); ++cursor;
            }
        }
    }
    void setTitle(const std::string& value) { if (value != heading) step = 0; heading = value; sceneSet=false; scene=View(); }
    void setChoices(const std::vector<Choice>& value) { supplied = value; }
    void show(const std::string& title) {
        if(title=="Action needs attention") {
            View error; error.kind="details"; error.title="Action needs attention";
            error.heading="ACTION NEEDS ATTENTION"; error.badge="VALIDATION / DECISION ERROR"; error.tone=5;
            const auto recent=content();
            for(auto it=recent.rbegin();it!=recent.rend();++it)
                if(it->find("Error:")!=std::string::npos) { error.details.push_back({"",*it,5}); break; }
            error.actions={{"back","Back"}}; present(error); consumed=false; return;
        }
        if(consumed) { consumed=false; return; }
        if (trim(pending).empty()) return;
        interacting = true;
        viewer(title, wrap(pending, 100000));
        interacting = false;
        // Do not discard the pending text: it remains accessible through F2.
    }
};

TerminalUI::Impl* TerminalUI::current = nullptr;
TerminalUI::TerminalUI(bool enabled) {
    if (enabled) { impl.reset(new Impl()); current = impl.get(); }
}
TerminalUI::~TerminalUI() { current = nullptr; }
bool TerminalUI::available() {
    const char* term = std::getenv("TERM");
    return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO) && term && *term && std::string(term) != "dumb";
}
bool TerminalUI::active() { return current != nullptr; }
void TerminalUI::title(const std::string& text) { if (current) current->setTitle(text); }
bool TerminalUI::read(std::string& value, const std::string& prompt, bool numeric, int minimum, int maximum) {
    return current && current->read(value, prompt, numeric, minimum, maximum);
}
void TerminalUI::choices(const std::vector<Choice>& options) { if (current) current->setChoices(options); }
void TerminalUI::showOutput(const std::string& title) { if (current) current->show(title); }

void TerminalUI::configure(const View& view) { if(current) current->configure(view); }
std::string TerminalUI::present(const View& view) { return current?current->present(view):"back"; }
bool TerminalUI::form(const std::string& title,std::vector<Field>& fields,
                     std::function<std::string(const std::vector<Field>&)> validate) {
    return current && current->form(title,fields,validate);
}
void TerminalUI::clearView() { if(current) current->configure(View()); }
