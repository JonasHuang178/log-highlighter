#pragma once
#ifndef REPORT_API_H
#define REPORT_API_H

// =============================================================================
//  ReportApi - the surface a report author writes against.
//
//  This header is the firewall between config/CustomReports.h and the rest of
//  the plugin. It deliberately contains no <windows.h>, no Scintilla, and no
//  Parser.h: a report function never sees HWND, SendMessage, SCI_* or Match.
//  Keep it that way — if a platform type ever appears here, the whole point of
//  the design is lost.
// =============================================================================

#include "AhoCorasick.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>
#include <initializer_list>
#include <type_traits>
#include <cstddef>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdlib>

// Lines between progress ticks. Matches the interval used by Parser.cpp.
static constexpr int REPORT_TICK_INTERVAL = 500;

// Width of the dashed rule emitted by ReportBuilder::Section.
static constexpr int REPORT_SECTION_WIDTH = 46;


// -----------------------------------------------------------------------------
//  ProgressSink - how an iterator reports progress and learns about cancellation.
//
//  Implemented by the engine (Report.cpp). Modelled as a plain function pointer
//  rather than std::function so this header stays free of platform coupling and
//  the iterators stay cheap to copy.
// -----------------------------------------------------------------------------
struct ProgressSink
{
    // Returns false once the user has cancelled.
    bool (*tick)(void* self, int lineNo, int totalLines) = nullptr;
    void* self = nullptr;

    bool Tick(int lineNo, int totalLines) const
    {
        return !tick || tick(self, lineNo, totalLines);
    }
};


// -----------------------------------------------------------------------------
//  Hit - one keyword occurrence produced by ReportContext::FindAll.
// -----------------------------------------------------------------------------
struct Hit
{
    std::string_view keyword;   // which keyword matched
    int              lineNo;    // 1-based, matching the Notepad++ margin
    std::string_view line;      // the whole line, without its line ending
    std::string_view after;     // rest of the line, starting past the keyword
};


// -----------------------------------------------------------------------------
//  Internal helpers shared by the ranges below.
// -----------------------------------------------------------------------------
namespace report_detail
{
    inline const char* FindNewline(const char* p, const char* end)
    {
        if (p >= end) return nullptr;
        return static_cast<const char*>(
            std::memchr(p, '\n', static_cast<size_t>(end - p)));
    }

    // Content of the line starting at `start`, excluding the line ending and a
    // trailing '\r'.
    inline std::string_view LineContent(const char* start, const char* end)
    {
        const char* nl   = FindNewline(start, end);
        const char* stop = nl ? nl : end;
        if (stop > start && stop[-1] == '\r') --stop;
        return std::string_view(start, static_cast<size_t>(stop - start));
    }
}


// -----------------------------------------------------------------------------
//  LineRange - lazy line-by-line iteration.
//
//  Line numbers are 1-based. A line never includes its line ending and never
//  keeps a trailing '\r'. Empty lines are yielded so that line numbers stay
//  aligned with the editor, and a final line without a newline is yielded too.
// -----------------------------------------------------------------------------
class LineRange
{
public:
    struct Entry
    {
        int              lineNo;
        std::string_view line;
    };

    class Iterator
    {
    public:
        Iterator(const char* p, const char* end,
                 const ProgressSink* sink, int totalLines, bool atEnd)
            : p_(p), end_(end), sink_(sink), total_(totalLines), atEnd_(atEnd)
        {
            if (!atEnd_) Load();
        }

        const Entry& operator*()  const { return cur_; }
        const Entry* operator->() const { return &cur_; }

        bool operator!=(const Iterator& other) const { return atEnd_ != other.atEnd_; }
        bool operator==(const Iterator& other) const { return atEnd_ == other.atEnd_; }

        Iterator& operator++()
        {
            if (!hasNewline_) { atEnd_ = true; return *this; }

            p_ = stop_ + 1;
            ++lineNo_;

            if (sink_ && (lineNo_ % REPORT_TICK_INTERVAL == 0))
            {
                if (!sink_->Tick(lineNo_, total_)) { atEnd_ = true; return *this; }
            }

            Load();
            return *this;
        }

    private:
        void Load()
        {
            const char* nl = report_detail::FindNewline(p_, end_);
            hasNewline_ = (nl != nullptr);
            stop_       = nl ? nl : end_;

            const char* contentEnd = stop_;
            if (contentEnd > p_ && contentEnd[-1] == '\r') --contentEnd;

            cur_ = Entry{ lineNo_,
                          std::string_view(p_, static_cast<size_t>(contentEnd - p_)) };
        }

        const char*         p_          = nullptr;
        const char*         end_        = nullptr;
        const char*         stop_       = nullptr;
        const ProgressSink* sink_       = nullptr;
        int                 total_      = 0;
        int                 lineNo_     = 1;
        bool                hasNewline_ = false;
        bool                atEnd_      = false;
        Entry               cur_{ 1, {} };
    };

    LineRange(const char* text, size_t length,
              const ProgressSink* sink, int totalLines)
        : text_(text), length_(length), sink_(sink), total_(totalLines) {}

    Iterator begin() const
    {
        return Iterator(text_, text_ + length_, sink_, total_, length_ == 0);
    }

    Iterator end() const
    {
        return Iterator(nullptr, nullptr, nullptr, 0, true);
    }

private:
    const char*         text_;
    size_t              length_;
    const ProgressSink* sink_;
    int                 total_;
};


// -----------------------------------------------------------------------------
//  HitRange - one Aho-Corasick pass over any number of keywords.
//
//  Looking for ten keywords costs the same as looking for one. The keyword
//  characters must outlive the loop; string literals always do.
// -----------------------------------------------------------------------------
class HitRange
{
public:
    class Iterator
    {
    public:
        Iterator(const HitRange* owner, bool atEnd)
            : owner_(owner), atEnd_(atEnd)
        {
            if (!atEnd_)
            {
                p_   = owner_->text_;
                end_ = owner_->text_ + owner_->length_;
                lineStart_ = p_;
                Advance();
            }
        }

        const Hit& operator*()  const { return cur_; }
        const Hit* operator->() const { return &cur_; }

        bool operator!=(const Iterator& other) const { return atEnd_ != other.atEnd_; }
        bool operator==(const Iterator& other) const { return atEnd_ == other.atEnd_; }

        Iterator& operator++() { Advance(); return *this; }

    private:
        void Advance()
        {
            for (;;)
            {
                // Emit any remaining patterns that ended at the current position
                // before consuming another character.
                if (pending_ && pendingIdx_ < pending_->size())
                {
                    MakeHit((*pending_)[pendingIdx_++]);
                    return;
                }

                if (p_ >= end_) { atEnd_ = true; return; }

                const char ch = *p_;

                acState_ = owner_->ac_.step(acState_, ch);
                pending_    = &owner_->ac_.outputs(acState_);
                pendingIdx_ = 0;

                // Captured before the newline bookkeeping below, so a match is
                // attributed to the line it ends on.
                matchEnd_       = p_;
                matchLineNo_    = lineNo_;
                matchLineStart_ = lineStart_;

                if (ch == '\n')
                {
                    ++lineNo_;
                    lineStart_ = p_ + 1;

                    if (owner_->sink_ && (lineNo_ % REPORT_TICK_INTERVAL == 0))
                    {
                        if (!owner_->sink_->Tick(lineNo_, owner_->total_))
                        {
                            atEnd_ = true;
                            return;
                        }
                    }
                }

                ++p_;
            }
        }

        void MakeHit(const AhoCorasick::Output& out)
        {
            const char* matchStart = matchEnd_ - out.len + 1;
            const char* afterStart = matchEnd_ + 1;

            std::string_view line =
                report_detail::LineContent(matchLineStart_, end_);

            const char* lineEnd = line.data() + line.size();

            std::string_view after;
            if (afterStart < lineEnd)
                after = std::string_view(afterStart,
                                         static_cast<size_t>(lineEnd - afterStart));

            cur_ = Hit{
                owner_->keywords_[static_cast<size_t>(out.patternIndex)],
                matchLineNo_,
                line,
                after
            };

            (void)matchStart;   // match position is not part of the public Hit
        }

        const HitRange* owner_ = nullptr;
        const char*     p_     = nullptr;
        const char*     end_   = nullptr;

        const std::vector<AhoCorasick::Output>* pending_    = nullptr;
        size_t                                  pendingIdx_ = 0;

        int         acState_        = 0;
        int         lineNo_         = 1;
        const char* lineStart_      = nullptr;
        const char* matchEnd_       = nullptr;
        int         matchLineNo_    = 1;
        const char* matchLineStart_ = nullptr;

        bool atEnd_ = false;
        Hit  cur_{};
    };

    HitRange(const char* text, size_t length,
             const ProgressSink* sink, int totalLines,
             std::initializer_list<std::string_view> keywords)
        : text_(text), length_(length), sink_(sink), total_(totalLines)
    {
        // The initializer_list backing array does not survive the range-for
        // statement, so the views are copied out here. The characters they point
        // at must still outlive the loop — literals always do.
        keywords_.reserve(keywords.size());
        for (std::string_view kw : keywords) keywords_.push_back(kw);
        Build();
    }

    HitRange(const char* text, size_t length,
             const ProgressSink* sink, int totalLines,
             std::string_view keyword)
        : text_(text), length_(length), sink_(sink), total_(totalLines)
    {
        keywords_.push_back(keyword);
        Build();
    }

    Iterator begin() const { return Iterator(this, length_ == 0 || keywords_.empty()); }
    Iterator end()   const { return Iterator(this, true); }

private:
    void Build()
    {
        for (size_t i = 0; i < keywords_.size(); ++i)
            ac_.addPattern(keywords_[i].data(), keywords_[i].size(),
                           static_cast<int>(i));
        ac_.build();
    }

    const char*                   text_;
    size_t                        length_;
    const ProgressSink*           sink_;
    int                           total_;
    std::vector<std::string_view> keywords_;
    AhoCorasick                   ac_;
};


// -----------------------------------------------------------------------------
//  ReportContext - the document, as a report function sees it.
//
//  Every string_view obtained from this object points into a private snapshot
//  that stays alive until the report function returns, so views may be collected
//  into std::map / std::vector without copying any characters.
// -----------------------------------------------------------------------------
class ReportContext
{
public:
    const char*    text      = nullptr;   // start of the snapshot
    size_t         length    = 0;         // snapshot size in bytes
    int            lineCount = 0;         // total number of lines
    const wchar_t* fileName  = L"";       // file name of the active document
    const wchar_t* filePath  = L"";       // full path of the active document

    // Tier 1 — line by line. Progress and Cancel work automatically.
    LineRange Lines() const
    {
        return LineRange(text, length, sink, lineCount);
    }

    // Tier 2 — one Aho-Corasick pass, any number of keywords, same cost.
    HitRange FindAll(std::string_view keyword) const
    {
        return HitRange(text, length, sink, lineCount, keyword);
    }

    HitRange FindAll(std::initializer_list<std::string_view> keywords) const
    {
        return HitRange(text, length, sink, lineCount, keywords);
    }

    // Set by the engine; not part of the authoring surface.
    const ProgressSink* sink = nullptr;
};


// -----------------------------------------------------------------------------
//  ReportBuilder - report output.
//
//  KV / KVf / AtLine entries are buffered and their key column is aligned when
//  the section ends, so an author never counts spaces. Each Section aligns
//  independently.
// -----------------------------------------------------------------------------
class ReportBuilder
{
public:
    void Section(std::string_view title)
    {
        rows_.push_back(Row{ Row::Kind::Section, std::string(title), {}, 0 });
    }

    void Line(std::string_view text)
    {
        rows_.push_back(Row{ Row::Kind::Text, {}, std::string(text), 0 });
    }

    void Blank()
    {
        rows_.push_back(Row{ Row::Kind::Blank, {}, {}, 0 });
    }

    void KV(std::string_view key, std::string_view value)
    {
        rows_.push_back(Row{ Row::Kind::Pair, std::string(key),
                             std::string(value), 0 });
    }

    void KV(std::string_view key, const char* value)
    {
        KV(key, std::string_view(value ? value : ""));
    }

    template <class T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
    void KV(std::string_view key, T value)
    {
        KVf(key, "%lld", static_cast<long long>(value));
    }

    template <class T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
    void KV(std::string_view key, T value)
    {
        KVf(key, "%g", static_cast<double>(value));
    }

    void KVf(std::string_view key, const char* fmt, ...)
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        const int n = std::vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        rows_.push_back(Row{ Row::Kind::Pair, std::string(key),
                             std::string(buf, n > 0
                                 ? (static_cast<size_t>(n) < sizeof(buf)
                                        ? static_cast<size_t>(n)
                                        : sizeof(buf) - 1)
                                 : 0),
                             0 });
    }

    void AtLine(int lineNo, std::string_view value)
    {
        rows_.push_back(Row{ Row::Kind::LineRef, {}, std::string(value), lineNo });
    }

    // ---- engine side --------------------------------------------------------

    bool Empty() const { return rows_.empty(); }

    std::string Render() const
    {
        std::string out;

        for (size_t i = 0; i < rows_.size(); )
        {
            if (rows_[i].kind == Row::Kind::Section)
            {
                out += RenderSectionRule(rows_[i].key);
                out += '\n';
                ++i;
                continue;
            }

            // Collect the rows up to the next Section — one alignment group.
            size_t j = i;
            while (j < rows_.size() && rows_[j].kind != Row::Kind::Section) ++j;

            RenderGroup(i, j, out);
            i = j;
        }

        return out;
    }

private:
    struct Row
    {
        enum class Kind { Section, Text, Pair, LineRef, Blank };
        Kind        kind;
        std::string key;     // Pair key, or Section title
        std::string value;   // Pair / LineRef / Text
        int         lineNo;  // LineRef
    };

    static std::string RenderSectionRule(const std::string& title)
    {
        std::string s = "---- " + title + " ";
        while (static_cast<int>(s.size()) < REPORT_SECTION_WIDTH) s += '-';
        return s;
    }

    void RenderGroup(size_t from, size_t to, std::string& out) const
    {
        // Widest key in this group, counting LineRef keys as "L" + digits.
        size_t keyWidth  = 0;
        size_t maxDigits = 0;

        for (size_t i = from; i < to; ++i)
        {
            if (rows_[i].kind == Row::Kind::Pair)
                keyWidth = (std::max)(keyWidth, rows_[i].key.size());
            else if (rows_[i].kind == Row::Kind::LineRef)
                maxDigits = (std::max)(maxDigits,
                                       std::to_string(rows_[i].lineNo).size());
        }
        if (maxDigits > 0)
            keyWidth = (std::max)(keyWidth, maxDigits + 1);   // 'L' + digits

        for (size_t i = from; i < to; ++i)
        {
            const Row& r = rows_[i];
            switch (r.kind)
            {
            case Row::Kind::Blank:
                out += '\n';
                break;

            case Row::Kind::Text:
                out += r.value;
                out += '\n';
                break;

            case Row::Kind::Pair:
                out += Pad(r.key, keyWidth);
                out += " : ";
                out += r.value;
                out += '\n';
                break;

            case Row::Kind::LineRef:
            {
                std::string digits = std::to_string(r.lineNo);
                std::string key    = "L";
                key.append(maxDigits - digits.size(), ' ');
                key += digits;

                out += Pad(key, keyWidth);
                out += " : ";
                out += r.value;
                out += '\n';
                break;
            }

            case Row::Kind::Section:
                break;   // handled by Render()
            }
        }
    }

    static std::string Pad(const std::string& s, size_t width)
    {
        std::string p = s;
        if (p.size() < width) p.append(width - p.size(), ' ');
        return p;
    }

    std::vector<Row> rows_;
};


// =============================================================================
//  String helpers
//
//  Contract, relied on by every example and by the absence of a crash guard:
//  an extraction helper returns an empty string_view when it cannot produce a
//  result, empty input yields empty output, and none of them ever reads outside
//  its input or throws. One `.empty()` check covers a whole nested expression.
// =============================================================================

// True when `what` occurs in `s`. An empty needle is never considered found.
inline bool Contains(std::string_view s, std::string_view what)
{
    return !what.empty() && s.find(what) != std::string_view::npos;
}

inline bool StartsWith(std::string_view s, std::string_view what)
{
    return !what.empty() && s.size() >= what.size() &&
           s.compare(0, what.size(), what) == 0;
}

inline bool EndsWith(std::string_view s, std::string_view what)
{
    return !what.empty() && s.size() >= what.size() &&
           s.compare(s.size() - what.size(), what.size(), what) == 0;
}

// Everything after the first occurrence of `what`.
inline std::string_view After(std::string_view s, std::string_view what)
{
    if (what.empty()) return {};
    const size_t pos = s.find(what);
    if (pos == std::string_view::npos) return {};
    return s.substr(pos + what.size());
}

// Everything before the first occurrence of `what`.
inline std::string_view Before(std::string_view s, std::string_view what)
{
    if (what.empty()) return {};
    const size_t pos = s.find(what);
    if (pos == std::string_view::npos) return {};
    return s.substr(0, pos);
}

// The text between the first `open` and the first `close` that follows it.
inline std::string_view Between(std::string_view s,
                                std::string_view open,
                                std::string_view close)
{
    if (open.empty() || close.empty()) return {};

    const size_t a = s.find(open);
    if (a == std::string_view::npos) return {};

    const size_t start = a + open.size();
    const size_t b     = s.find(close, start);
    if (b == std::string_view::npos) return {};

    return s.substr(start, b - start);
}

// The n-th delimiter-separated field, 0-based. Runs of delimiters produce empty
// fields rather than being collapsed, which keeps field positions predictable.
inline std::string_view Field(std::string_view s, char delim, int n)
{
    if (n < 0 || s.empty()) return {};

    size_t start = 0;
    for (int i = 0; ; ++i)
    {
        size_t pos = s.find(delim, start);
        if (i == n)
        {
            const size_t stop = (pos == std::string_view::npos) ? s.size() : pos;
            return s.substr(start, stop - start);
        }
        if (pos == std::string_view::npos) return {};
        start = pos + 1;
    }
}

// Leading and trailing whitespace removed.
inline std::string_view Trim(std::string_view s)
{
    const auto isSpace = [](char c) {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n' ||
               c == '\v' || c == '\f';
    };

    size_t a = 0;
    size_t b = s.size();
    while (a < b && isSpace(s[a]))     ++a;
    while (b > a && isSpace(s[b - 1])) --b;
    return s.substr(a, b - a);
}

// Parses a whole (optionally signed) integer. Returns false and leaves `out`
// untouched on anything unexpected, including trailing garbage and overflow.
inline bool ToInt(std::string_view s, int& out)
{
    s = Trim(s);
    if (s.empty()) return false;

    size_t i        = 0;
    bool   negative = false;
    if (s[0] == '+' || s[0] == '-')
    {
        negative = (s[0] == '-');
        i = 1;
        if (i >= s.size()) return false;
    }

    long long value = 0;
    for (; i < s.size(); ++i)
    {
        const char c = s[i];
        if (c < '0' || c > '9') return false;

        value = value * 10 + (c - '0');
        if (value > 2147483648LL) return false;   // beyond |INT_MIN|
    }

    if (!negative && value > 2147483647LL) return false;
    if ( negative && value > 2147483648LL) return false;

    out = static_cast<int>(negative ? -value : value);
    return true;
}

// Parses a whole floating-point value. Returns false and leaves `out` untouched
// on anything unexpected.
inline bool ToDouble(std::string_view s, double& out)
{
    s = Trim(s);
    if (s.empty() || s.size() >= 64) return false;

    char buf[64];
    std::memcpy(buf, s.data(), s.size());
    buf[s.size()] = '\0';

    char*        endPtr = nullptr;
    const double value  = std::strtod(buf, &endPtr);

    if (endPtr != buf + s.size()) return false;

    out = value;
    return true;
}


// -----------------------------------------------------------------------------
//  Report registration
// -----------------------------------------------------------------------------
using ReportFn = void (*)(const ReportContext& ctx, ReportBuilder& out);

struct CustomReport
{
    const wchar_t* title;      // menu item text under Plugins > log-highlighter
    ReportFn       fn;         // the report function to invoke
    char           shortcut;   // letter for Ctrl+Alt+<letter>, or 0 for none
};

#endif // REPORT_API_H
