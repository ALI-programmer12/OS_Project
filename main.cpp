#include <iostream>
#include <vector>
#include <limits>
#include <string>
#include <tuple>
#include <raylib.h>
#include "process.h"
#include "simulator.h"
#include "ranker.h"

using namespace std;

// ─────────────────────────────────────────────
//  LAYOUT CONSTANTS
// ─────────────────────────────────────────────
static const int SW  = 1400;
static const int SH  = 800;
static const int PAD = 48;

// ─────────────────────────────────────────────
//  COLOUR PALETTE  (dark navy theme)
// ─────────────────────────────────────────────
static const Color C_BG        = { 10,  13,  22, 255 }; // page background
static const Color C_CARD      = { 20,  25,  42, 255 }; // card / panel
static const Color C_CARD2     = { 28,  34,  56, 255 }; // alt row / inner card
static const Color C_BORDER    = { 55,  66,  98, 255 }; // subtle border
static const Color C_TEXT      = {230, 235, 250, 255 }; // primary text
static const Color C_MUTED     = {130, 142, 170, 255 }; // secondary / labels
static const Color C_ACCENT    = { 99, 162, 255, 255 }; // blue accent
static const Color C_ACCENT2   = { 72, 199, 142, 255 }; // green accent
static const Color C_WARN      = {255, 196,  80, 255 }; // gold / highlight
static const Color C_DANGER    = {255, 100, 120, 255 }; // red / danger

// Process colour palette (6 vivid colours)
static const Color PROC_COL[] = {
    { 99, 162, 255, 255 }, // blue
    {255, 154,  86, 255 }, // orange
    { 82, 210, 140, 255 }, // green
    {210,  98, 255, 255 }, // purple
    {255,  88, 116, 255 }, // pink
    {255, 210,  90, 255 }, // yellow
};
static const int PALETTE_SZ = 6;

static Color procColor(int pid) {
    if (pid <= 0) return Color{60, 60, 70, 255};
    return PROC_COL[(pid - 1) % PALETTE_SZ];
}

// ─────────────────────────────────────────────
//  FILE PATHS
// ─────────────────────────────────────────────
static const char* IMG_PATH = "images/bg.jpg";
static const char* SND_PATH = "audios/a2.wav";

// ─────────────────────────────────────────────
//  HELPERS
// ─────────────────────────────────────────────
static void DrawCard(Rectangle r, Color fill, Color border) {
    DrawRectangleRounded(r, 0.12f, 10, fill);
    DrawRectangleRoundedLines(r, 0.12f, 10, border);
}

// Returns true when mouse is inside r
static bool Hovered(Rectangle r) {
    return CheckCollisionPointRec(GetMousePosition(), r);
}

// Draw a styled button; returns true if clicked this frame
static bool Button(Rectangle b, const char *label,
                   bool active = false, Color accentOverride = {0,0,0,0}) {
    bool hov = Hovered(b);
    bool clicked = hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    Color accent = (accentOverride.a > 0) ? accentOverride : C_ACCENT;
    Color fill, border, text;
    if (active)     { fill = Color{40,100,200,255}; border = accent;    text = RAYWHITE; }
    else if (hov)   { fill = Color{32, 40, 66,255}; border = accent;    text = C_TEXT;   }
    else            { fill = C_CARD;                 border = C_BORDER;  text = C_MUTED;  }

    DrawRectangleRounded(b, 0.20f, 10, fill);
    DrawRectangleRoundedLines(b, 0.20f, 10, border);
    int tw = MeasureText(label, 18);
    DrawText(label,
             (int)(b.x + (b.width  - tw) / 2),
             (int)(b.y + (b.height - 18) / 2),
             18, text);
    return clicked;
}

// Centre text horizontally on the screen
static void TextCentre(const char *s, int y, int sz, Color col) {
    DrawText(s, (SW - MeasureText(s, sz)) / 2, y, sz, col);
}

// Thin horizontal separator
static void Separator(int y) {
    DrawRectangle(PAD, y, SW - PAD * 2, 1, C_BORDER);
}

// Draw a labelled input field; active = currently selected
static void InputField(Rectangle r, const char *label,
                       const string &val, bool active, bool labelAbove = true) {
    int labelY = labelAbove ? (int)(r.y - 24) : (int)(r.y + (r.height - 18) / 2);
    Color labelCol = active ? C_ACCENT : C_MUTED;
    if (labelAbove) DrawText(label, (int)r.x, labelY, 17, labelCol);

    Color border = active ? C_ACCENT : C_BORDER;
    Color fill   = active ? Color{25,40,75,255} : C_CARD;
    DrawRectangleRounded(r, 0.15f, 8, fill);
    DrawRectangleRoundedLines(r, 0.15f, 8, border);

    // Value + blinking cursor
    string display = val;
    if (active && ((int)(GetTime() * 2) % 2 == 0)) display += "|";
    DrawText(display.c_str(), (int)r.x + 10, (int)(r.y + (r.height - 18) / 2), 18, C_TEXT);
}

// ─────────────────────────────────────────────
//  GANTT CHART
// ─────────────────────────────────────────────
struct GanttEntry { int start, pid, dur; };

static void DrawGanttChart(const vector<tuple<int,int,int>> &ganttRaw,
                            int x, int y, int maxW) {
    if (ganttRaw.empty()) {
        DrawText("No Gantt data.", x, y, 18, C_MUTED);
        return;
    }

    // Find total time
    int totalTime = 0;
    for (auto &g : ganttRaw) totalTime = max(totalTime, get<0>(g) + get<2>(g));
    if (totalTime <= 0) totalTime = 1;

    const int BAR_H   = 48;
    const int LABEL_H = 24;   // space above for pid labels
    const int TIME_H  = 22;   // space below for time markers
    float scale = (float)maxW / (float)totalTime;

    // Background rail
    DrawRectangle(x, y + LABEL_H, maxW, BAR_H, Color{18, 22, 38, 255});
    DrawRectangleLines(x, y + LABEL_H, maxW, BAR_H, C_BORDER);

    // Blocks
    for (auto &g : ganttRaw) {
        int start = get<0>(g), pid = get<1>(g), dur = get<2>(g);
        int bx = x + (int)(start * scale);
        int bw = max(1, (int)(dur * scale));

        Color col = procColor(pid);
        Color dark = {(unsigned char)(col.r * 0.5f),
                      (unsigned char)(col.g * 0.5f),
                      (unsigned char)(col.b * 0.5f), 255};

        // Block body
        DrawRectangle(bx + 1, y + LABEL_H + 1, bw - 2, BAR_H - 2, dark);
        // Bright top strip
        DrawRectangle(bx + 1, y + LABEL_H + 1, bw - 2, 4, col);

        // PID label above
        if (pid > 0) {
            string lbl = "P" + to_string(pid);
            int lw = MeasureText(lbl.c_str(), 16);
            if (bw >= lw + 4) {
                DrawText(lbl.c_str(), bx + (bw - lw) / 2, y, 16, col);
            }
        } else {
            // Idle — hatching
            for (int hx = bx + 1; hx < bx + bw - 1; hx += 6)
                DrawLine(hx, y + LABEL_H + 1, hx + 4, y + LABEL_H + BAR_H - 2, C_BORDER);
            int lw = MeasureText("idle", 14);
            if (bw >= lw + 4)
                DrawText("idle", bx + (bw - lw)/2, y, 14, C_MUTED);
        }

        // Left border tick
        DrawLine(bx, y + LABEL_H, bx, y + LABEL_H + BAR_H + 6, C_BORDER);
    }

    // Right border tick
    DrawLine(x + maxW, y + LABEL_H, x + maxW, y + LABEL_H + BAR_H + 6, C_BORDER);

    // Time markers  (only every N units to avoid clutter)
    int step = max(1, totalTime / 20);
    for (int t = 0; t <= totalTime; t += step) {
        int tx = x + (int)(t * scale);
        string ts = to_string(t);
        int tw = MeasureText(ts.c_str(), 14);
        DrawText(ts.c_str(), tx - tw / 2, y + LABEL_H + BAR_H + 6, 14, C_MUTED);
    }
    // Always mark the last
    {
        string ts = to_string(totalTime);
        int tw = MeasureText(ts.c_str(), 14);
        DrawText(ts.c_str(), x + maxW - tw / 2, y + LABEL_H + BAR_H + 6, 14, C_MUTED);
    }
}

// ─────────────────────────────────────────────
//  STATES & DATA
// ─────────────────────────────────────────────
enum State {
    INPUT_NUM, INPUT_PRIORITY, INPUT_PROCESSES,
    INPUT_QUANTUM, SELECT_ALGO, VIEW_ALGO, COMPARE
};

struct ProcInput { int at=0, bt=0, pr=0; string at_s, bt_s, pr_s; };

// ─────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────
int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(SW, SH, "CPU Scheduling Simulator");
    SetTargetFPS(60);

    // ── Audio and Image Setup ──────────────────
    InitAudioDevice();
    Sound bgSound = LoadSound(SND_PATH);
    if (bgSound.frameCount > 0) {
        PlaySound(bgSound);
    }

    Texture2D bgTexture = LoadTexture(IMG_PATH);

    // ── State ──────────────────────────────────
    State state      = INPUT_NUM;
    int   numProc    = 0;
    string numStr    = "";
    int   quantum    = 4;
    string qStr      = "4";
    bool  hasPriority = false;
    int   curProc    = 0;
    int   activeField = 0;   // 0=AT, 1=BT, 2=PR
    int   selectedAlgo = -1;

    vector<ProcInput> procInputs;
    Process *p       = nullptr;
    extern vector<Result> results; // results are usually global in these simulator setups

    // Scrolling for Gantt in VIEW_ALGO
    float ganttScroll = 0.f;

    while (!WindowShouldClose()) {

        // ── INPUT HANDLING ──────────────────────
        Vector2 mouse = GetMousePosition();

        // Wheel scroll for Gantt
        if (state == VIEW_ALGO || state == COMPARE)
            ganttScroll -= GetMouseWheelMove() * 30.f;

        int key = GetKeyPressed();
        // Tab to advance field
        if (key == KEY_TAB) {
            if (state == INPUT_PROCESSES) activeField = (activeField + 1) % (hasPriority ? 3 : 2);
        }
        // Backspace
        if (IsKeyPressed(KEY_BACKSPACE)) {
            auto delLast = [](string &s){ if (!s.empty()) s.pop_back(); };
            if (state == INPUT_NUM) delLast(numStr);
            else if (state == INPUT_QUANTUM) delLast(qStr);
            else if (state == INPUT_PROCESSES) {
                if (activeField == 0) delLast(procInputs[curProc].at_s);
                else if (activeField == 1) delLast(procInputs[curProc].bt_s);
                else if (activeField == 2) delLast(procInputs[curProc].pr_s);
            }
        }
        // Printable chars
        if (key >= 32 && key <= 126) {
            char c = (char)key;
            if (state == INPUT_NUM) numStr += c;
            else if (state == INPUT_QUANTUM) qStr += c;
            else if (state == INPUT_PROCESSES) {
                if (activeField == 0) procInputs[curProc].at_s += c;
                else if (activeField == 1) procInputs[curProc].bt_s += c;
                else if (activeField == 2) procInputs[curProc].pr_s += c;
            }
        }
        // Enter to advance
        if (IsKeyPressed(KEY_ENTER)) {
            if (state == INPUT_NUM && !numStr.empty()) {
                numProc = stoi(numStr);
                if (numProc > 0) {
                    procInputs.resize(numProc);
                    state = INPUT_PRIORITY;
                }
            } else if (state == INPUT_PROCESSES) {
                // commit current, advance
                auto &pi = procInputs[curProc];
                if (!pi.at_s.empty()) pi.at = stoi(pi.at_s);
                if (!pi.bt_s.empty()) pi.bt = stoi(pi.bt_s);
                if (hasPriority && !pi.pr_s.empty()) pi.pr = stoi(pi.pr_s);
                curProc++;
                activeField = 0;
                if (curProc >= numProc) state = INPUT_QUANTUM;
            } else if (state == INPUT_QUANTUM && !qStr.empty()) {
                quantum = stoi(qStr);
                p = new Process[numProc];
                for (int i = 0; i < numProc; i++) {
                    p[i].pid = i + 1;
                    p[i].at  = procInputs[i].at;
                    p[i].bt  = procInputs[i].bt;
                    p[i].pr  = hasPriority ? procInputs[i].pr : 0;
                    p[i].rt  = p[i].bt;
                    p[i].wt  = 0;
                    p[i].tat = 0;
                }
                state = SELECT_ALGO;
            }
        }

        // ── DRAW ────────────────────────────────
        BeginDrawing();
        ClearBackground(C_BG);

        // ── Draw Background Image ───────────────
        if (bgTexture.id != 0) {
            DrawTexturePro(bgTexture,
                Rectangle{ 0, 0, (float)bgTexture.width, (float)bgTexture.height },
                Rectangle{ 0, 0, (float)SW, (float)SH },
                Vector2{ 0, 0 }, 0.0f, WHITE);
        }

        // ── Semi-transparent Overlay ────────────
        DrawRectangle(0, 0, SW, SH, ColorAlpha(C_BG, 0.85f));

        // ── TOP HEADER BAR ──────────────────────
        DrawRectangle(0, 0, SW, 56, C_CARD);
        DrawRectangle(0, 55, SW, 1, C_BORDER);
        TextCentre("CPU Scheduling Simulator", 16, 26, C_TEXT);

        // ── PAGE CONTENT ────────────────────────
        int contentY = 80;

        // ════════════════════════════════════════
        //  INPUT_NUM
        // ════════════════════════════════════════
        if (state == INPUT_NUM) {
            TextCentre("How many processes?", contentY, 22, C_MUTED);

            Rectangle card = {(float)(SW/2 - 220), (float)(contentY + 50), 440, 160};
            DrawCard(card, C_CARD, C_BORDER);

            Rectangle field = {card.x + 40, card.y + 48, 360, 48};
            InputField(field, "Number of Processes", numStr, true);

            Rectangle btn = {card.x + 140, card.y + 110, 160, 38};
            if (Button(btn, "Continue  →", false, C_ACCENT2) && !numStr.empty()) {
                numProc = stoi(numStr);
                if (numProc > 0) { procInputs.resize(numProc); state = INPUT_PRIORITY; }
            }
        }

        // ════════════════════════════════════════
        //  INPUT_PRIORITY
        // ════════════════════════════════════════
        else if (state == INPUT_PRIORITY) {
            TextCentre("Do processes have priority values?", contentY, 22, C_MUTED);

            float cx = SW / 2.f;
            Rectangle yBtn = { cx - 180, (float)(contentY + 80), 160, 52 };
            Rectangle nBtn = { cx +  20, (float)(contentY + 80), 160, 52 };

            if (Button(yBtn, "Yes — with Priority", false, C_ACCENT)) {
                hasPriority = true;
                state = INPUT_PROCESSES;
            }
            if (Button(nBtn, "No — skip Priority", false, C_ACCENT2)) {
                hasPriority = false;
                state = INPUT_PROCESSES;
            }
        }

        // ════════════════════════════════════════
        //  INPUT_PROCESSES
        // ════════════════════════════════════════
        else if (state == INPUT_PROCESSES) {
            string heading = "Process " + to_string(curProc + 1) + " of " + to_string(numProc);
            TextCentre(heading.c_str(), contentY, 22, C_TEXT);

            // Progress bar
            int barW = SW - PAD * 4;
            int barX = PAD * 2;
            DrawRectangle(barX, contentY + 34, barW, 6, C_CARD2);
            DrawRectangle(barX, contentY + 34, (int)(barW * (curProc + 1) / (float)numProc), 6, C_ACCENT);

            Rectangle card = { (float)(SW/2 - 340), (float)(contentY + 60), 680.f, 220.f };
            if (hasPriority) card.height = 270;
            DrawCard(card, C_CARD, C_BORDER);

            float fx = card.x + 40, fy = card.y + 50, fw = 180, fh = 48;
            float gap = 200;

            // Field rectangles
            Rectangle atR = { fx,        fy, fw, fh };
            Rectangle btR = { fx + gap,  fy, fw, fh };
            Rectangle prR = { fx + gap*2, fy, fw, fh };

            // Click to focus
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (Hovered(atR)) activeField = 0;
                if (Hovered(btR)) activeField = 1;
                if (hasPriority && Hovered(prR)) activeField = 2;
            }

            auto &pi = procInputs[curProc];
            InputField(atR, "Arrival Time",  pi.at_s, activeField == 0);
            InputField(btR, "Burst Time",    pi.bt_s, activeField == 1);
            if (hasPriority)
                InputField(prR, "Priority", pi.pr_s, activeField == 2);

            DrawText("TAB to switch fields  |  ENTER to confirm", (int)card.x + 14, (int)(card.y + fh + 80), 15, C_MUTED);

            // Previous process summary
            if (curProc > 0) {
                int sy = (int)(card.y + fh + 110);
                DrawText("Previous entries:", (int)card.x + 14, sy, 16, C_MUTED);
                int colW = 80, rx = (int)card.x + 14;
                DrawText("PID",  rx,       sy + 22, 15, C_MUTED);
                DrawText("AT",   rx+colW,  sy + 22, 15, C_MUTED);
                DrawText("BT",   rx+colW*2,sy + 22, 15, C_MUTED);
                if (hasPriority) DrawText("PR", rx+colW*3, sy + 22, 15, C_MUTED);
                for (int i = 0; i < curProc && i < 4; i++) {
                    int ry = sy + 42 + i * 20;
                    DrawText(("P" + to_string(i+1)).c_str(),       rx,        ry, 15, procColor(i+1));
                    DrawText(procInputs[i].at_s.c_str(),           rx+colW,   ry, 15, C_TEXT);
                    DrawText(procInputs[i].bt_s.c_str(),           rx+colW*2, ry, 15, C_TEXT);
                    if (hasPriority) DrawText(procInputs[i].pr_s.c_str(), rx+colW*3, ry, 15, C_TEXT);
                }
            }

            Rectangle nextBtn = { card.x + card.width - 200, card.y + card.height - 55, 170, 40 };
            if (Button(nextBtn, curProc < numProc - 1 ? "Next  →" : "Finish  ✓", false, C_ACCENT2)) {
                auto &p2 = procInputs[curProc];
                if (!p2.at_s.empty()) p2.at = stoi(p2.at_s);
                if (!p2.bt_s.empty()) p2.bt = stoi(p2.bt_s);
                if (hasPriority && !p2.pr_s.empty()) p2.pr = stoi(p2.pr_s);
                curProc++;
                activeField = 0;
                if (curProc >= numProc) state = INPUT_QUANTUM;
            }
        }

        // ════════════════════════════════════════
        //  INPUT_QUANTUM
        // ════════════════════════════════════════
        else if (state == INPUT_QUANTUM) {
            TextCentre("Round Robin Time Quantum", contentY, 22, C_MUTED);

            Rectangle card = { (float)(SW/2 - 240), (float)(contentY + 50), 480, 160 };
            DrawCard(card, C_CARD, C_BORDER);

            Rectangle field = { card.x + 50, card.y + 48, 380, 48 };
            InputField(field, "Time Quantum (used by Round Robin & MLFQ)", qStr, true);

            Rectangle btn = { card.x + 160, card.y + 108, 160, 38 };
            if (Button(btn, "Run Simulator  →", false, C_ACCENT) && !qStr.empty()) {
                quantum = stoi(qStr);
                p = new Process[numProc];
                for (int i = 0; i < numProc; i++) {
                    p[i].pid = i + 1;
                    p[i].at  = procInputs[i].at;
                    p[i].bt  = procInputs[i].bt;
                    p[i].pr  = hasPriority ? procInputs[i].pr : 0;
                    p[i].rt  = p[i].bt;
                    p[i].wt  = 0;
                    p[i].tat = 0;
                }
                state = SELECT_ALGO;
            }
        }

        // ════════════════════════════════════════
        //  SELECT_ALGO
        // ════════════════════════════════════════
        else if (state == SELECT_ALGO) {
            TextCentre("Select an Algorithm", contentY, 22, C_MUTED);
            Separator(contentY + 36);

            const char *names[8] = {
                "FCFS", "SJF (Non-Pre)", "Round Robin", "SRTF",
                "LRTF", "MLFQ", "Pre-Priority", "Non-Pre Priority"
            };
            Color accs[8] = {
                C_ACCENT, C_ACCENT2, C_WARN, C_DANGER,
                C_ACCENT, C_ACCENT2, C_WARN, C_DANGER
            };

            // 4 columns × 2 rows of algo buttons
            int cols = 4, rows = 2;
            float bW = 230, bH = 64, gx = 16, gy = 16;
            float totalW = cols * bW + (cols - 1) * gx;
            float startX = (SW - totalW) / 2.f;
            float startY = contentY + 55.f;

            for (int i = 0; i < 8; i++) {
                int col = i % cols, row = i / cols;
                Rectangle b = { startX + col * (bW + gx), startY + row * (bH + gy), bW, bH };

                bool hov = Hovered(b);
                Color fill   = selectedAlgo == i ? Color{30,60,120,255} : (hov ? C_CARD2 : C_CARD);
                Color border = selectedAlgo == i ? accs[i] : (hov ? accs[i] : C_BORDER);
                Color txt    = selectedAlgo == i ? RAYWHITE : (hov ? C_TEXT : C_MUTED);

                DrawRectangleRounded(b, 0.18f, 10, fill);
                DrawRectangleRoundedLines(b, 0.18f, 10, border);
                // Accent left stripe
                if (selectedAlgo == i || hov)
                    DrawRectangleRounded({b.x, b.y + 12, 4, b.height - 24}, 0.5f, 4, accs[i]);

                int tw2 = MeasureText(names[i], 19);
                DrawText(names[i], (int)(b.x + (b.width - tw2) / 2), (int)(b.y + (b.height - 19) / 2), 19, txt);

                if (hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    selectedAlgo = i;
                    results.clear();
                    simulate(p, numProc, names[i], i + 1, quantum);
                    ganttScroll = 0.f;
                    state = VIEW_ALGO;
                }
            }

            // Compare All button — centred below
            float compY = startY + rows * (bH + gy) + 24;
            Rectangle compBtn = { (SW - 320.f) / 2.f, compY, 320, 56 };
            bool hov2 = Hovered(compBtn);
            Color cfill = hov2 ? Color{50,120,80,255} : C_CARD;
            DrawRectangleRounded(compBtn, 0.22f, 10, cfill);
            DrawRectangleRoundedLines(compBtn, 0.22f, 10, hov2 ? C_ACCENT2 : C_BORDER);
            const char *cl = "⚡  Compare All Algorithms";
            int clw = MeasureText(cl, 20);
            DrawText(cl, (int)(compBtn.x + (compBtn.width - clw) / 2),
                     (int)(compBtn.y + (compBtn.height - 20) / 2), 20, hov2 ? C_ACCENT2 : C_MUTED);
            if (hov2 && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                results.clear();
                simulate(p, numProc, "FCFS",             1);
                simulate(p, numProc, "Round Robin",      2, quantum);
                simulate(p, numProc, "SJF",              3);
                simulate(p, numProc, "SRTF",             4);
                simulate(p, numProc, "LRTF",             5);
                simulate(p, numProc, "MLFQ",             6, quantum);
                if (hasPriority) {
                    simulate(p, numProc, "Pre-Priority",     7);
                    simulate(p, numProc, "Non-Pre Priority", 8);
                }
                rankAlgorithms();
                ganttScroll = 0.f;
                state = COMPARE;
            }
        }

        // ════════════════════════════════════════
        //  VIEW_ALGO  (single algo result)
        // ════════════════════════════════════════
        else if (state == VIEW_ALGO) {
            // Back
            if (Button({(float)PAD, (float)contentY, 90, 34}, "← Back")) {
                state = SELECT_ALGO;
            }

            if (!results.empty()) {
                Result &res = results.back();

                // Title
                string title = res.name + "  —  Results";
                TextCentre(title.c_str(), contentY, 24, C_TEXT);
                Separator(contentY + 34);

                // ── Stats cards ──────────────────────────
                float cW = 220, cH = 76, cGap = 20;
                float cTotalW = cW * 3 + cGap * 2;
                float cX = (SW - cTotalW) / 2.f;
                int cY = contentY + 48;

                auto StatCard = [&](float x, float y, const char *lbl, const char *val, Color acc) {
                    Rectangle r = { x, (float)y, cW, cH };
                    DrawCard(r, C_CARD, C_BORDER);
                    DrawRectangle((int)x, y, 4, (int)cH, acc);
                    DrawText(lbl, (int)x + 14, y + 10, 15, C_MUTED);
                    DrawText(val, (int)x + 14, y + 34, 24, acc);
                };

                char wtBuf[64], tatBuf[64];
                sprintf(wtBuf,  "%.2f", res.avgWT);
                sprintf(tatBuf, "%.2f", res.avgTAT);

                StatCard(cX,           cY, "Avg Waiting Time",      wtBuf,  C_ACCENT);
                StatCard(cX + cW+cGap, cY, "Avg Turnaround Time",   tatBuf, C_ACCENT2);
                StatCard(cX+(cW+cGap)*2, cY, "Algorithm", res.name.c_str(), C_WARN);

                // ── Process table ────────────────────────
                int tY = cY + (int)cH + 24;
                int tX = PAD;
                int tW = SW - PAD * 2;

                // Header
                DrawRectangle(tX, tY, tW, 30, C_CARD2);
                DrawRectangleLines(tX, tY, tW, 30, C_BORDER);
                int cols2[] = { 60, 130, 130, 160, 160, 160 };
                const char *hdrs[] = { "PID", "Arrival", "Burst", "Waiting", "Turnaround", "Finish" };
                int cx2 = tX + 10;
                for (int i = 0; i < 6; i++) {
                    DrawText(hdrs[i], cx2, tY + 6, 16, C_MUTED);
                    cx2 += cols2[i];
                }

                for (int i = 0; i < numProc; i++) {
                    int ry = tY + 30 + i * 28;
                    Color rowBg = (i % 2 == 0) ? C_CARD : C_CARD2;
                    DrawRectangle(tX, ry, tW, 28, rowBg);
                    DrawRectangleLines(tX, ry, tW, 28, C_BORDER);

                    // Colour swatch
                    DrawRectangle(tX, ry + 6, 4, 16, procColor(p[i].pid));

                    int vx = tX + 10, vy = ry + 5;
                    auto Col = [&](int v, int idx) {
                        DrawText(to_string(v).c_str(), vx, vy, 16, C_TEXT);
                        vx += cols2[idx];
                    };
                    Col(p[i].pid, 0);
                    Col(p[i].at,  1);
                    Col(p[i].bt,  2);
                    Col(p[i].wt,  3);
                    Col(p[i].tat, 4);
                    Col(p[i].at + p[i].tat, 5); // finish time
                }

                // ── Gantt chart ───────────────────────────
                int gY = tY + 30 + numProc * 28 + 36;
                DrawText("Gantt Chart", tX, gY - 24, 18, C_ACCENT);
                DrawGanttChart(res.gantt, tX, gY, tW);
            }
        }

        // ════════════════════════════════════════
        //  COMPARE
        // ════════════════════════════════════════
        else if (state == COMPARE) {
            if (Button({(float)PAD, (float)contentY, 90, 34}, "← Back")) {
                state = SELECT_ALGO;
            }
            TextCentre("Algorithm Comparison", contentY, 24, C_TEXT);
            Separator(contentY + 34);

            int tX = PAD, tY = contentY + 48, tW = SW - PAD * 2;

            // Column widths
            int colW[] = { 240, 200, 200, 200 };
            const char *hdrs2[] = { "Algorithm", "Avg Waiting Time", "Avg Turnaround", "Rank" };

            // Header
            DrawRectangle(tX, tY, tW, 34, C_CARD2);
            DrawRectangleLines(tX, tY, tW, 34, C_BORDER);
            int hx = tX + 10;
            for (int i = 0; i < 4; i++) {
                DrawText(hdrs2[i], hx, tY + 8, 16, C_MUTED);
                hx += colW[i];
            }

            // Find best WT and TAT for highlighting
            float bestWT = 1e9f, bestTAT = 1e9f;
            for (auto &r : results) { bestWT = min(bestWT, r.avgWT); bestTAT = min(bestTAT, r.avgTAT); }

            for (int i = 0; i < (int)results.size(); i++) {
                Result &r = results[i];
                int ry = tY + 34 + i * 32;
                Color rowBg = (i % 2 == 0) ? C_CARD : C_CARD2;
                DrawRectangle(tX, ry, tW, 32, rowBg);
                DrawRectangleLines(tX, ry, tW, 32, C_BORDER);

                // Left accent stripe
                DrawRectangle(tX, ry + 6, 4, 20, procColor(i + 1));

                int vx = tX + 10, vy = ry + 7;
                // Name
                DrawText(r.name.c_str(), vx, vy, 17, C_TEXT); vx += colW[0];
                // Avg WT
                char buf[32];
                sprintf(buf, "%.3f", r.avgWT);
                DrawText(buf, vx, vy, 17, r.avgWT == bestWT ? C_ACCENT2 : C_TEXT); vx += colW[1];
                // Avg TAT
                sprintf(buf, "%.3f", r.avgTAT);
                DrawText(buf, vx, vy, 17, r.avgTAT == bestTAT ? C_ACCENT2 : C_TEXT); vx += colW[2];
                // Rank (1-based from position)
                DrawText(to_string(i + 1).c_str(), vx, vy, 17, i == 0 ? C_WARN : C_MUTED);
            }

            // Legend
            int legY = tY + 34 + (int)results.size() * 32 + 20;
            DrawRectangle(tX, legY, 12, 12, C_ACCENT2);
            DrawText("= Best in category", tX + 18, legY - 1, 15, C_MUTED);
        }

        EndDrawing();
    }

    // ── Cleanup ─────────────────────────────────
    UnloadTexture(bgTexture);
    UnloadSound(bgSound);
    CloseAudioDevice();
    CloseWindow();
    if (p) delete[] p;
    return 0;
}