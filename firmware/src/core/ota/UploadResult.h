#ifndef UPLOADRESULT_H
#define UPLOADRESULT_H

// What the /update page answers after a browser upload (issue #55). No Arduino
// types, so the host tests can build it (test/test_uploadresult).
//
// Before #55 the answer was one line of plain text and the orbs always
// restarted: pressing Upload with no file chosen read "Update FAILED" and
// restarted them for nothing, and a good update left the browser on a page
// with no way back to the settings.

#include <cstddef>
#include <string>

namespace ota {

enum class Outcome { NoFile, Ok, Failed };

// Decided by the bytes that arrived, not by the file name: an empty name (no
// file chosen) and an empty file both send nothing, and neither should start an
// update. Update.end() on nothing also sets an error, so bytes come first.
inline Outcome outcome(size_t bytes, bool hasError) {
    if (bytes == 0) {
        return Outcome::NoFile;
    }
    return hasError ? Outcome::Failed : Outcome::Ok;
}

// No file: nothing was touched, so the orbs keep running. A failed write still
// restarts, because the restart is what redraws the screens the progress
// display took over. Rejected for now: redrawing in place, which needs every
// widget to repaint on demand.
inline bool restartAfter(Outcome o) {
    return o != Outcome::NoFile;
}

inline std::string escapeHtml(const char *s) {
    std::string out;
    for (; s && *s; ++s) {
        switch (*s) {
        case '<':
            out += "&lt;";
            break;
        case '>':
            out += "&gt;";
            break;
        case '&':
            out += "&amp;";
            break;
        case '\'':
            out += "&#39;";
            break;
        case '"':
            out += "&quot;";
            break;
        default:
            out += *s;
        }
    }
    return out;
}

// After a restart the orbs refuse connections for about 50 s. The script waits
// 10 s (so it never catches the old firmware before it restarts), then asks
// every 3 s and goes to `next` on any answer, a 401 included: an answer means
// they are back. After 3 minutes it stops and says so; the links still work.
inline std::string waitScript(const char *next) {
    std::string s =
        "<script>(function(){var n='";
    s += next;
    s += "',t0=Date.now(),m=document.getElementById('w');"
         "function a(){if(Date.now()-t0>180000){m.textContent='The orbs have not answered for 3 minutes. "
         "Check their screens, then use a link below.';return}"
         "var c=new AbortController(),k=setTimeout(function(){c.abort()},3000);"
         "fetch(n,{cache:'no-store',signal:c.signal}).then(function(){location.href=n})"
         ".catch(function(){clearTimeout(k);setTimeout(a,3000)})}"
         "setTimeout(a,10000)})()</script>";
    return s;
}

inline std::string resultPage(Outcome o, const char *reason) {
    std::string p =
        "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width'>"
        "<title>Info Orbs update</title></head><body style='font-family:sans-serif'>";
    switch (o) {
    case Outcome::NoFile:
        p += "<h2>No file chosen</h2>"
             "<p>Nothing was uploaded, and the orbs keep running. Choose <code>firmware.bin</code> first, "
             "then press Upload.</p>"
             "<p><a href='/update'>Back to the update page</a> &middot; <a href='/settings'>Settings</a></p>";
        break;
    case Outcome::Ok:
        p += "<h2>Update OK</h2>"
             "<p>The orbs are restarting with the new firmware. That takes about a minute.</p>"
             "<p id='w'>This page goes to the settings by itself when they are back.</p>"
             "<p><a href='/settings'>Settings</a> &middot; <a href='/update'>Update page</a></p>";
        p += waitScript("/settings");
        break;
    case Outcome::Failed:
        p += "<h2>Update failed</h2>"
             "<p>The old firmware is kept. Reason: ";
        p += escapeHtml(reason);
        p += "</p>"
             "<p>The orbs are restarting to redraw their screens. That takes about a minute.</p>"
             "<p id='w'>This page goes back to the update page by itself when they are back.</p>"
             "<p><a href='/update'>Try again</a> &middot; <a href='/settings'>Settings</a></p>";
        p += waitScript("/update");
        break;
    }
    p += "</body></html>";
    return p;
}

} // namespace ota

#endif
