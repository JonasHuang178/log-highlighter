#pragma once
#ifndef CUSTOM_REPORTS_H
#define CUSTOM_REPORTS_H

// =============================================================================
//  Custom Reports  —  Ctrl+Alt+E
//
//  Write your own report function here, add it to CUSTOM_REPORTS[] at the
//  bottom of this file, and rebuild. This is the only file you need to touch.
//
//  A report function always has this shape:
//
//      static void MyReport(const ReportContext& ctx, ReportBuilder& out)
//
//  It reads the active document through `ctx` and writes lines through `out`.
//  The result is shown in a dialog — read-only, right-click to copy.
//
//  ---- Guarantees you can rely on -------------------------------------------
//
//  * Every string_view you get from `ctx` or from a helper points into a
//    private snapshot of the document that stays alive until your function
//    returns. Collecting them into a std::map or std::vector is safe and
//    copies no characters. Just don't store them outside the function.
//
//  * Helpers never read out of range and never throw. When something is not
//    found they return an empty string_view, and empty input gives empty
//    output — so calls nest without checking every level:
//
//        auto ip = Field(After(line, "IP: "), ' ', 0);
//        if (ip.empty()) continue;          // one check is enough
//
//  * Line numbers are 1-based, matching the Notepad++ margin.
//
//  * A line never includes its line ending, and a trailing '\r' (CRLF files)
//    is stripped for you. Empty lines are still reported, so line numbers
//    stay aligned with the editor.
//
//  * Progress and Cancel work by themselves as long as you iterate with
//    ctx.Lines() or ctx.FindAll(). Reading ctx.text directly opts out of both.
//
//  ---- Things that will bite you --------------------------------------------
//
//  * There is no crash guard. A stray pointer in your report function takes
//    down Notepad++ and every unsaved tab with it. Stay on ctx.Lines(),
//    ctx.FindAll() and the helpers and you cannot go out of range.
//    To see what your parser is doing, turn on REPORT_DEBUG_MODE below.
//    If you need a breakpoint, build Debug x64 and attach Visual Studio to
//    notepad++.exe.
//
//  * Save this file as UTF-8 *with BOM* if you use non-ASCII keywords.
//    Without the BOM, MSVC reads the source in the system ANSI codepage and
//    your literal will silently never match the UTF-8 document.
//
//  * Keyword characters passed to FindAll must outlive the loop. String
//    literals always do; a temporary std::string does not.
//
//  * This file is included by src/Report.cpp only. It defines functions, not
//    just data — include it elsewhere and you get a second private copy of
//    every report.
//
//  ---- What you can call ----------------------------------------------------
//
//    ctx.Lines()               for (auto [lineNo, line] : ctx.Lines())
//    ctx.FindAll("kw")         for (auto hit : ctx.FindAll("kw"))
//    ctx.FindAll({"a","b"})    one pass over any number of keywords
//    ctx.text, ctx.length      raw snapshot (no progress, no cancel)
//    ctx.lineCount             total number of lines
//    ctx.fileName, ctx.filePath
//
//    hit.keyword  hit.lineNo  hit.line  hit.after
//
//    out.Section("Title")      ---- Title ------------------
//    out.Line(text)            free text, not aligned
//    out.KV(key, value)        key : value      (value may be text or a number)
//    out.KVf(key, fmt, ...)    value rendered with a printf format
//    out.AtLine(lineNo, text)  L  142 : text
//    out.Blank()               empty line
//
//    After(s, kw)      text after the first occurrence of kw
//    Before(s, kw)     text before the first occurrence of kw
//    Between(s, a, b)  text between a and the first b after it
//    Field(s, ' ', n)  n-th delimiter-separated field, 0-based
//    Trim(s)           leading and trailing whitespace removed
//    Contains(s, kw)   StartsWith(s, kw)   EndsWith(s, kw)      -> bool
//    ToInt(s, out)     ToDouble(s, out)    -> bool, out untouched on failure
//
//  ---- Debug mode -----------------------------------------------------------
//
//  Set REPORT_DEBUG_MODE to 1 below and rebuild. Notepad++ then opens a console
//  window at startup, and Debug / Debugf calls inside your report function print
//  to it — which is the fastest way to see what your parser is actually matching:
//
//      Debugf("L%-5d raw=[%s] ip=[%s]", lineNo, line, ip);
//
//  %s takes std::string_view, std::string and const char* directly; there is no
//  need to pass a size and pointer pair. Width and precision work as usual.
//  Debugf can never crash Notepad++ — a wrong conversion prints a marker.
//
//  While debug mode is on, the report cache is bypassed, so pressing the
//  shortcut twice really runs your report twice.
//
//  Worth knowing:
//    * Console output is slow. Debug against a small sample file, not a
//      production log. REPORT_DEBUG_MAX_LINES below caps it so a per-line print
//      cannot make Notepad++ look hung.
//    * Arguments are still evaluated when debug mode is off, so avoid
//      Debugf("%s", SomethingExpensive()) in a shipping build.
//    * The console cannot be closed while Notepad++ runs — closing it would
//      terminate the editor, so its close box is disabled. Minimise it instead,
//      or rebuild with REPORT_DEBUG_MODE 0.
//    * The console dies with the process, so it cannot show you the last lines
//      before a crash.
// =============================================================================

// 1 = open a debug console at startup and enable Debug / Debugf. 0 = off.
#define REPORT_DEBUG_MODE       0

// Maximum lines of your own debug output per report run. 0 = unlimited.
// Engine breadcrumbs are never counted or suppressed.
#define REPORT_DEBUG_MAX_LINES  1000

#include "../src/ReportApi.h"
#include <map>


// -----------------------------------------------------------------------------
//  Example 1 — the smallest possible report.
//  Nothing is scanned; ctx already knows the document size.
// -----------------------------------------------------------------------------
static void LineStats(const ReportContext& ctx, ReportBuilder& out)
{
    out.KV("Lines", ctx.lineCount);
    out.KV("Bytes", ctx.length);
}


// -----------------------------------------------------------------------------
//  Example 2 — walk every line, extract a field, aggregate.
//
//    input     "10:23:45 IP: 192.168.1.10 connected"
//    After()   "192.168.1.10 connected"      empty if "IP: " is not on the line
//    Field()   "192.168.1.10"                first space-separated field
// -----------------------------------------------------------------------------
static void IpReport(const ReportContext& ctx, ReportBuilder& out)
{
    std::map<std::string_view, int> hits;        // string_view keys: no copying
    std::map<std::string_view, int> firstLine;

    for (auto [lineNo, line] : ctx.Lines())
    {
        auto ip = Field(After(line, "IP: "), ' ', 0);

        // Set REPORT_DEBUG_MODE to 1 above and uncomment this to watch every
        // line go past in the console while you tune the extraction.
        // Debugf("L%-5d raw=[%s] ip=[%s]", lineNo, line, ip);

        if (ip.empty()) continue;

        if (++hits[ip] == 1)
            firstLine[ip] = lineNo;
    }

    out.Section("IP addresses");

    if (hits.empty())
    {
        out.Line("none found");
        return;
    }

    for (const auto& [ip, n] : hits)
        out.KVf(ip, "%5d hits   first @ L%d", n, firstLine[ip]);

    out.Blank();
    out.KV("Unique addresses", hits.size());
}


// -----------------------------------------------------------------------------
//  Example 3 — the fast path.
//
//  ctx.FindAll() runs a single Aho-Corasick pass over all the keywords at
//  once, so ten keywords cost the same as one. Prefer it over calling
//  Contains() ten times per line.
//
//    hit.keyword   which keyword matched
//    hit.lineNo    1-based line number
//    hit.line      the whole line
//    hit.after     rest of the line, starting right after the keyword
// -----------------------------------------------------------------------------
static void SeveritySummary(const ReportContext& ctx, ReportBuilder& out)
{
    int errors = 0, warns = 0, debugs = 0;
    int              firstErrLine = 0;
    std::string_view firstErrText;      // safe to keep — see Guarantees above

    for (auto hit : ctx.FindAll({ "[ ERROR ]", "[ WARN ]", "[ DEBUG ]" }))
    {
        if (hit.keyword == "[ ERROR ]")
        {
            ++errors;
            if (!firstErrLine)
            {
                firstErrLine = hit.lineNo;
                firstErrText = hit.line;
            }
        }
        else if (hit.keyword == "[ WARN ]") ++warns;
        else                                ++debugs;
    }

    out.Section("Severity");
    out.KV("ERROR", errors);
    out.KV("WARN",  warns);
    out.KV("DEBUG", debugs);

    if (firstErrLine)
    {
        out.Blank();
        out.Section("First error");
        out.AtLine(firstErrLine, Trim(firstErrText));
    }
}


// -----------------------------------------------------------------------------
//  Registration — a report only appears in the menu once it is listed here.
//
//    Menu title   shown under Plugins > log-highlighter
//    Function     the report function defined above
//    Shortcut     letter for Ctrl+Alt+<letter>, or 0 for no shortcut
//                 (a shortcut can also be assigned later in
//                  Settings > Shortcut Mapper > Plugin commands)
// -----------------------------------------------------------------------------
static const CustomReport CUSTOM_REPORTS[] = {
//   Menu title              Function           Shortcut
    { L"IP Report",         IpReport,          'E' },
    { L"Severity Summary",  SeveritySummary,   0   },
    { L"Line Stats",        LineStats,         0   },
};

#endif // CUSTOM_REPORTS_H
