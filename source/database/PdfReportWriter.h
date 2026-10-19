
#pragma once
/*
 * AURELIA WEALTH STUDIO | ATELIER REPORTING
 * CS 499 Computer Science Capstone | Brianna Reed
 *
 * Native, dependency free PDF generation using color vector graphics
 * The public exportEnterpriseReport/exportClientReport APIs match the
 * original AureliaDesktop.cpp and InvestmentAnalytics.h interfaces
 *
 * Values come from the persistent SQLite data layer. Every record is
 * fictional; snapshot changes are NOT investment returns or advice
 */

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "InvestmentAnalytics.h"

struct PdfInk {
    double r, g, b;
};

class SimplePdfDocument {
public:
    struct Page { std::string content; };

private:
    std::vector<Page> pages_;

    static std::string number(double x, int decimals = 2) {
        std::ostringstream out;
        out << std::fixed << std::setprecision(decimals) << x;
        return out.str();
    }

    static std::string pdfText(const std::string& text) {
        // PDF standard Type 1 fonts use WinAnsi, not UTF 8. Convert the
        // commonly used UTF 8 Latin letters and punctuation, and replace
        // unsupported glyphs rather than creating invalid PDF content
        std::string out;
        for (size_t i = 0; i < text.size(); ++i) {
            unsigned char c = static_cast<unsigned char>(text[i]);
            unsigned int cp = c;
            if (c >= 0xC2 && c <= 0xDF && i + 1 < text.size()) {
                unsigned char c2 = static_cast<unsigned char>(text[i + 1]);
                if ((c2 & 0xC0) == 0x80) { cp = ((c & 31) << 6) | (c2 & 63); ++i; }
            }
            else if (c >= 0xE0 && c <= 0xEF && i + 2 < text.size()) {
                unsigned char c2 = static_cast<unsigned char>(text[i + 1]);
                unsigned char c3 = static_cast<unsigned char>(text[i + 2]);
                if ((c2 & 0xC0) == 0x80 && (c3 & 0xC0) == 0x80) {
                    cp = ((c & 15) << 12) | ((c2 & 63) << 6) | (c3 & 63);
                    i += 2;
                }
            }
            if (cp == 0x2018 || cp == 0x2019) cp = '\'';
            if (cp == 0x201C || cp == 0x201D) cp = '"';
            if (cp == 0x2013 || cp == 0x2014) cp = '-';
            if (cp == 0x2022) cp = '*';
            if (cp > 255 || cp == 0 || (cp < 32 && cp != ' ')) cp = '?';
            const char ch = static_cast<char>(cp);
            if (ch == '(' || ch == ')' || ch == '\\') out += '\\';
            if (cp >= 128) {
                std::ostringstream oct;
                oct << '\\' << std::oct << std::setw(3) << std::setfill('0') << cp;
                out += oct.str();
            }
            else {
                out += ch;
            }
        }
        return out;
    }

    void append(int page, const std::string& commands) {
        pages_.at(static_cast<size_t>(page)).content += commands + "\n";
    }

public:
    static PdfInk ink() { return { 0.13, 0.17, 0.18 }; }
    static PdfInk muted() { return { 0.39, 0.38, 0.36 }; }
    static PdfInk antique() { return { 0.69, 0.53, 0.29 }; }
    static PdfInk sage() { return { 0.51, 0.57, 0.41 }; }
    static PdfInk clay() { return { 0.71, 0.37, 0.33 }; }
    static PdfInk paper() { return { 0.976, 0.957, 0.927 }; }
    static PdfInk pale() { return { 0.95, 0.93, 0.89 }; }
    static PdfInk white() { return { 1.0, 1.0, 1.0 }; }
    static PdfInk dark() { return { 0.105, 0.145, 0.16 }; }

    int addPage() {
        pages_.push_back({});
        return static_cast<int>(pages_.size() - 1);
    }
    size_t pageCount() const { return pages_.size(); }

    void fill(int page, PdfInk c) {
        append(page, number(c.r, 3) + " " + number(c.g, 3) + " " + number(c.b, 3) + " rg");
    }
    void stroke(int page, PdfInk c) {
        append(page, number(c.r, 3) + " " + number(c.g, 3) + " " + number(c.b, 3) + " RG");
    }

    void colorRectangle(int page, double x, double y, double w, double h,
        PdfInk background, PdfInk border, double weight = 0.0) {
        fill(page, background);
        stroke(page, border);
        append(page, number(weight) + " w " + number(x) + " " + number(y) + " " +
            number(w) + " " + number(h) + " re " + (weight > 0 ? "B" : "f"));
    }

    // Original PDF writer drawing calls are retained for compatibility
    void rectangle(int page, double x, double y, double w, double h, double grayFill) {
        colorRectangle(page, x, y, w, h, { grayFill,grayFill,grayFill }, { 0,0,0 });
    }

    void colorLine(int page, double x1, double y1, double x2, double y2,
        PdfInk c, double weight = 1.0) {
        stroke(page, c);
        append(page, number(weight) + " w " + number(x1) + " " + number(y1) + " m " +
            number(x2) + " " + number(y2) + " l S");
    }
    void line(int page, double x1, double y1, double x2, double y2,
        double weight = 1.0) {
        colorLine(page, x1, y1, x2, y2, ink(), weight);
    }

    void colorText(int page, double x, double y, double size,
        const std::string& s, PdfInk color,
        bool bold = false, bool serif = false) {
        fill(page, color);
        const int font = serif ? (bold ? 3 : 4) : (bold ? 2 : 1);
        append(page, "BT /F" + std::to_string(font) + " " + number(size, 1) +
            " Tf 1 0 0 1 " + number(x) + " " + number(y) +
            " Tm (" + pdfText(s) + ") Tj ET");
    }

    void text(int page, double x, double y, double size,
        const std::string& s, bool bold = false) {
        colorText(page, x, y, size, s, ink(), bold);
    }

    void paragraph(int page, double x, double y, double size,
        const std::string& content, double charsPerLine,
        int maxLines, PdfInk color = muted(), double leading = 14.0) {
        std::istringstream words(content);
        std::string word, row;
        int rendered = 0;
        while (words >> word) {
            if (!row.empty() && row.size() + word.size() + 1 > charsPerLine) {
                colorText(page, x, y - rendered * leading, size, row, color);
                row.clear();
                if (++rendered >= maxLines) return;
            }
            if (!row.empty()) row += " ";
            row += word;
        }
        if (!row.empty() && rendered < maxLines)
            colorText(page, x, y - rendered * leading, size, row, color);
    }

    // Circular vector crest; retain the public drawing interface
    void sketchedCrest(int page, double cx, double cy, double radius) {
        // Standard four quadrant cubic approximation of a circle
        const double control = radius * 0.5522847498307936;
        stroke(page, antique());
        append(page, "1.5 w " + number(cx - radius) + " " + number(cy) + " m " +
            number(cx - radius) + " " + number(cy + control) + " " +
            number(cx - control) + " " + number(cy + radius) + " " +
            number(cx) + " " + number(cy + radius) + " c " +
            number(cx + control) + " " + number(cy + radius) + " " +
            number(cx + radius) + " " + number(cy + control) + " " +
            number(cx + radius) + " " + number(cy) + " c " +
            number(cx + radius) + " " + number(cy - control) + " " +
            number(cx + control) + " " + number(cy - radius) + " " +
            number(cx) + " " + number(cy - radius) + " c " +
            number(cx - control) + " " + number(cy - radius) + " " +
            number(cx - radius) + " " + number(cy - control) + " " +
            number(cx - radius) + " " + number(cy) + " c h S");
        colorLine(page, cx - radius * .60, cy - radius * .35, cx - radius * .12, cy + radius * .05, sage(), 2.1);
        colorLine(page, cx - radius * .12, cy + radius * .05, cx + radius * .15, cy - radius * .09, sage(), 2.1);
        colorLine(page, cx + radius * .15, cy - radius * .09, cx + radius * .64, cy + radius * .50, sage(), 2.1);
        colorLine(page, cx + radius * .37, cy + radius * .50, cx + radius * .64, cy + radius * .50, antique(), 1.7);
        colorLine(page, cx + radius * .64, cy + radius * .50, cx + radius * .60, cy + radius * .23, antique(), 1.7);
    }

    void save(const std::string& path) const {
        if (pages_.empty()) throw std::runtime_error("Cannot save an empty PDF.");
        std::ofstream output(path, std::ios::binary);
        if (!output) throw std::runtime_error("Unable to create the PDF file.");

        std::vector<std::string> objects;
        objects.push_back("<< /Type /Catalog /Pages 2 0 R >>");
        std::ostringstream children;
        children << "[";
        for (size_t i = 0;i < pages_.size();++i) children << 7 + i * 2 << " 0 R ";
        children << "]";
        objects.push_back("<< /Type /Pages /Kids " + children.str() +
            " /Count " + std::to_string(pages_.size()) + " >>");
        objects.push_back("<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>");
        objects.push_back("<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold /Encoding /WinAnsiEncoding >>");
        objects.push_back("<< /Type /Font /Subtype /Type1 /BaseFont /Times-Bold /Encoding /WinAnsiEncoding >>");
        objects.push_back("<< /Type /Font /Subtype /Type1 /BaseFont /Times-Roman /Encoding /WinAnsiEncoding >>");

        for (size_t i = 0;i < pages_.size();++i) {
            objects.push_back("<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] "
                "/Resources << /Font << /F1 3 0 R /F2 4 0 R /F3 5 0 R /F4 6 0 R >> >> "
                "/Contents " + std::to_string(8 + i * 2) + " 0 R >>");
            const std::string& s = pages_[i].content;
            objects.push_back("<< /Length " + std::to_string(s.size()) + " >>\nstream\n" + s + "endstream");
        }
        output << "%PDF-1.4\n%\xC7\xEC\x8F\xA2\n";
        std::vector<uint64_t> offsets(objects.size() + 1, 0);
        for (size_t i = 0;i < objects.size();++i) {
            offsets[i + 1] = static_cast<uint64_t>(output.tellp());
            output << i + 1 << " 0 obj\n" << objects[i] << "\nendobj\n";
        }
        const uint64_t start = static_cast<uint64_t>(output.tellp());
        output << "xref\n0 " << offsets.size() << "\n0000000000 65535 f \n";
        for (size_t i = 1;i < offsets.size();++i) {
            output << std::setw(10) << std::setfill('0') << offsets[i] << " 00000 n \n";
        }
        output << "trailer\n<< /Size " << offsets.size()
            << " /Root 1 0 R >>\nstartxref\n" << start << "\n%%EOF\n";
        if (!output) throw std::runtime_error("Writing the PDF failed.");
    }
};

class PdfReportWriter {
    ClientRepository& repository_;
    AnalyticsService& analytics_;
    using P = SimplePdfDocument;

    static std::string money(double v) {
        std::ostringstream o;
        o << std::fixed << std::setprecision(0) << std::fabs(v);
        std::string digits = o.str();
        const size_t dot = digits.find('.');
        const size_t end = (dot == std::string::npos) ? digits.size() : dot;
        for (size_t i = end;i > 3;i -= 3) digits.insert(i - 3, ",");
        return std::string(v < 0 ? "-$" : "$") + digits;
    }
    static std::string quoteMoney(double v) { std::ostringstream n; n<<"$"<<std::fixed<<std::setprecision(2)<<v; return n.str(); }
    static std::string percent(double v) {
        std::ostringstream o;
        o << std::fixed << std::setprecision(1) << v << "%";
        return o.str();
    }
    static std::string shorten(const std::string& s, size_t max) {
        if (s.size() <= max) return s;
        return s.substr(0, max > 3 ? max - 3 : 0) + "...";
    }
    static std::string today(Database& db) {
        Statement q(db.handle(), "SELECT date('now','-7 hours');");
        return q.step() == SQLITE_ROW ? q.columnText(0) : "unknown";
    }

    static int newPage(P& pdf, const std::string& section,
        const std::string& heading,
        const std::string& subtitle = "") {
        int p = pdf.addPage();
        pdf.colorRectangle(p, 0, 0, 612, 792, P::paper(), P::paper());
        pdf.colorRectangle(p, 0, 718, 612, 74, P::dark(), P::dark());
        pdf.sketchedCrest(p, 59, 754, 19);
        pdf.colorText(p, 95, 757, 21, "AURELIA", P::white(), true, true);
        pdf.colorText(p, 201, 757, 9, "ATELIER / WEALTH STUDIO", P::antique());
        pdf.colorText(p, 42, 702, 9, section, P::antique(), true);
        pdf.colorText(p, 42, 669, 25, heading, P::ink(), true, true);
        if (!subtitle.empty())
            pdf.colorText(p, 43, 646, 10, shorten(subtitle, 94), P::muted());
        pdf.colorLine(p, 42, 634, 570, 634, P::antique(), .8);
        return p;
    }

    static void footers(P& pdf, const std::string& date) {
        for (size_t i = 0;i < pdf.pageCount();++i) {
            int p = static_cast<int>(i);
            pdf.colorLine(p, 42, 60, 570, 60, P::antique(), .7);
            pdf.colorText(p, 42, 47, 8,
                "FICTIONAL DATA  /  MODEL-BASED REVIEW  /  NOT INVESTMENT ADVICE",
                P::muted());
            pdf.colorText(p, 42, 33, 7.5, "Prepared " + date + " | Brianna Reed / SNHU CS 499", P::muted());
            pdf.colorText(p, 520, 45, 9,
                std::to_string(i + 1) + " / " + std::to_string(pdf.pageCount()),
                P::antique(), true);
        }
    }

    static void card(P& pdf, int p, double x, double y, double w,
        const std::string& title, const std::string& value,
        const std::string& subtitle) {
        pdf.colorRectangle(p, x, y, w, 76, P::white(), P::pale(), .75);
        pdf.colorRectangle(p, x, y, 3, 76, P::antique(), P::antique());
        pdf.colorText(p, x + 13, y + 55, 9, shorten(title, 26), P::muted(), true);
        pdf.colorText(p, x + 13, y + 25, 18, shorten(value, 18), P::ink(), true, true);
        pdf.colorText(p, x + 13, y + 9, 8, shorten(subtitle, 32), P::muted());
    }

    static void chartBars(P& pdf, int p, double x, double top, double /*width*/,
        const vector<AllocationSlice>& slices) {
        if (slices.empty()) {
            pdf.colorText(p, x, top - 28, 11, "No stored holdings available.", P::muted());
            return;
        }
        const PdfInk colors[] = { P::antique(),P::sage(),P::clay(),
                                  {0.58,.53,.43},{.42,.54,.60},{.71,.62,.51} };
        int row = 0;
        for (const auto& s : slices) {
            if (row >= 7) break;
            const double y = top - 26 - row * 32;
            pdf.colorText(p, x, y + 4, 9, shorten(s.label, 23), P::ink());
            pdf.colorRectangle(p, x + 153, y, 243, 12, P::pale(), P::pale());
            const double ratio = std::max(0.0, std::min(100.0, s.percent)) / 100.;
            if (ratio > 0) pdf.colorRectangle(p, x + 153, y, 243 * ratio, 12,
                colors[row % 6], colors[row % 6]);
            pdf.colorText(p, x + 403, y + 2, 9, percent(s.percent), P::ink(), true);
            pdf.colorText(p, x + 457, y + 2, 8, shorten(money(s.value), 14), P::muted());
            ++row;
        }
    }

    static void chartTrend(P& pdf, int p, double x, double y, double w, double h,
        const vector<PortfolioSnapshot>& samples) {
        pdf.colorRectangle(p, x, y, w, h, P::white(), P::pale(), .65);
        for (int i = 1;i < 4;++i)
            pdf.colorLine(p, x + 18, y + i * h / 4, x + w - 16, y + i * h / 4, P::pale(), .8);
        if (samples.size() < 2) {
            pdf.colorText(p, x + 25, y + h / 2, 10,
                "At least two stored snapshots are required.", P::muted());
            return;
        }
        double lo = samples.front().value, hi = lo;
        for (const auto& pt : samples) {
            lo = std::min(lo, pt.value);
            hi = std::max(hi, pt.value);
        }
        if (lo == hi) hi = lo + 1;
        for (size_t i = 1;i < samples.size();++i) {
            const double x1 = x + 21 + (w - 43) * (i - 1) / (samples.size() - 1);
            const double x2 = x + 21 + (w - 43) * i / (samples.size() - 1);
            const double y1 = y + 25 + (h - 53) * (samples[i - 1].value - lo) / (hi - lo);
            const double y2 = y + 25 + (h - 53) * (samples[i].value - lo) / (hi - lo);
            pdf.colorLine(p, x1, y1, x2, y2, P::antique(), 2.3);
        }
        pdf.colorText(p, x + 18, y + 9, 8, shorten(samples.front().date, 18), P::muted());
        pdf.colorText(p, x + w - 88, y + 9, 8, shorten(samples.back().date, 18), P::muted());
        pdf.colorText(p, x + w - 104, y + h - 16, 8, "Max " + money(hi), P::muted());
    }

    static void tableHead(P& pdf, int p, double top,
        const std::string& columns) {
        pdf.colorRectangle(p, 42, top - 8, 528, 25, P::dark(), P::dark());
        pdf.colorText(p, 52, top, 8.8, columns, P::white(), true);
    }

    static void tableRow(P& pdf, int p, int index, double y,
        const std::string& a, const std::string& b,
        const std::string& c, const std::string& d,
        const std::string& e = "") {
        if (index % 2 == 0)
            pdf.colorRectangle(p, 42, y - 7, 528, 26, P::white(), P::white());
        pdf.colorLine(p, 42, y - 8, 570, y - 8, P::pale(), .40);
        pdf.colorText(p, 50, y, 8.7, shorten(a, 24), P::ink());
        pdf.colorText(p, 210, y, 8.7, shorten(b, 24), P::muted());
        pdf.colorText(p, 351, y, 8.7, shorten(c, 16), P::ink());
        pdf.colorText(p, 451, y, 8.7, shorten(d, 14), P::ink());
        if (!e.empty())pdf.colorText(p, 521, y, 8, shorten(e, 9), P::muted());
    }

    static std::string clientName(ClientRepository& repo, const AdvisorTask& task) {
        if (!task.clientId)return "Firm-wide";
        const auto c = repo.findClientById(*task.clientId);
        return c ? c->name : "Former client";
    }

    static std::string goalFunding(const FinancialGoal& g) {
        return percent(g.targetAmount > 0.0 ? 100.0 * g.currentAmount / g.targetAmount : 0.0);
    }

public:
    PdfReportWriter(ClientRepository& r, AnalyticsService& a)
        :repository_(r), analytics_(a) {
    }

    void exportEnterpriseReport(const std::string& filePath) {
        const EnterpriseSummary summary = analytics_.enterpriseSummary();
        const vector<AllocationSlice> allocation = analytics_.enterpriseAllocation();
        const vector<PortfolioSnapshot> history = analytics_.enterpriseHistory();
        const vector<Client> clients = repository_.getAllClients();
        const vector<AdvisorTask> tasks = repository_.getOpenTasks();
        const std::string date = today(repository_.database());
        P pdf;

        // 01 / Firm dashboard, live allocation and historical trend
        int p = newPage(pdf, "01 / ENTERPRISE INTELLIGENCE",
            "Private wealth, in perspective",
            "Based on stored fictional securities, accounts and portfolio snapshots");
        card(pdf, p, 42, 532, 164, "ASSETS ADMINISTERED", money(summary.assetsAdministered),
            "Summed modeled holdings");
        card(pdf, p, 224, 532, 164, "ACTIVE RELATIONSHIPS",
            std::to_string(summary.activeClients), "of " + std::to_string(summary.clientCount) + " clients");
        card(pdf, p, 406, 532, 164, "OPEN REVIEW TASKS",
            std::to_string(summary.openTasks),
            std::to_string(summary.overdueTasks) + " overdue");
        pdf.colorText(p, 42, 501, 15, "Asset class exposure", P::ink(), true, true);
        chartBars(pdf, p, 45, 477, 515, allocation);
        pdf.colorText(p, 42, 249, 15, "Modeled account value trend", P::ink(), true, true);
        chartTrend(pdf, p, 44, 94, 522, 139, history);
        pdf.colorText(p, 46, 76, 8, "12-month snapshot change: " +
            percent(summary.twelveMonthChangePercent) +
            " | This is NOT a time-weighted return.", P::muted());

        // 02 / Client directory, paginated so records never run off page
        size_t offset = 0;
        do {
            p = newPage(pdf, "02 / CLIENT RELATIONSHIPS", "Relationship directory",
                "Persistent clients and values calculated from current mock holdings");
            tableHead(pdf, p, 599, "CLIENT                         ADVISOR                       RISK                 VALUE             STATE");
            double y = 577;
            int index = 0;
            for (;offset < clients.size() && index < 16;++offset, ++index) {
                const Client& c = clients[offset];
                const ClientDashboard d = analytics_.clientDashboard(c.id);
                tableRow(pdf, p, index, y, c.name, c.advisorName,
                    riskLevelToString(c.riskLevel), money(d.currentValue),
                    c.active ? "Active" : "Closed");
                y -= 32;
            }
            pdf.colorText(p, 45, 78, 9,
                "Roster values are drawn from account holdings, not the legacy balance field.",
                P::muted());
        } while (offset < clients.size());

        // 03 / All currently open advisor tasks, with current date flags
        offset = 0;
        do {
            p = newPage(pdf, "03 / OPERATIONAL OVERSIGHT", "Open advisor work queue",
                "Actions are stored in advisor_tasks and scoped to the appropriate client");
            tableHead(pdf, p, 599, "CLIENT                         FOLLOW-UP                       DUE DATE             PRIORITY");
            double y = 577;
            int index = 0;
            for (;offset < tasks.size() && index < 15;++offset, ++index) {
                const AdvisorTask& t = tasks[offset];
                const bool overdue = t.dueDate < date;
                if (index % 2 == 0)pdf.colorRectangle(p, 42, y - 7, 528, 28, P::white(), P::white());
                pdf.colorText(p, 50, y, 8.6, shorten(clientName(repository_, t), 26), P::ink());
                pdf.colorText(p, 209, y, 8.6, shorten(t.title, 31), P::ink());
                pdf.colorText(p, 422, y, 8.6, t.dueDate, overdue ? P::clay() : P::muted());
                pdf.colorText(p, 506, y, 8.6, shorten(t.priority, 12), P::muted());
                y -= 33;
            }
            if (tasks.empty())pdf.colorText(p, 53, 558, 11,
                "No incomplete advisor tasks are stored.", P::muted());
            pdf.colorText(p, 45, 78, 9,
                "Color marks past-due items against the SQLite current date: " + date, P::muted());
        } while (offset < tasks.size());

        // 04 / All stored goals and their observable funding ratios
        struct NamedGoal { string person; FinancialGoal goal; };
        vector<NamedGoal> namedGoals;
        for (const auto& c : clients)
            for (const auto& g : repository_.getGoalsForClient(c.id))
                namedGoals.push_back({ c.name,g });

        offset = 0;
        do {
            p = newPage(pdf, "04 / GOAL ADMINISTRATION", "Financial goal register",
                "Funding percentage is a simple current/target ratio, not a forecast");
            tableHead(pdf, p, 599, "CLIENT                         GOAL                            CURRENT             TARGET             FUNDED");
            double y = 577;
            int index = 0;
            for (;offset < namedGoals.size() && index < 16;++offset, ++index) {
                const NamedGoal& g = namedGoals[offset];
                tableRow(pdf, p, index, y, g.person, g.goal.name,
                    money(g.goal.currentAmount), money(g.goal.targetAmount),
                    goalFunding(g.goal));
                y -= 32;
            }
            if (namedGoals.empty())pdf.colorText(p, 53, 558, 11,
                "No financial goals have been recorded.", P::muted());
        } while (offset < namedGoals.size());

        // 05 / Interpretability and data lineage
        p = newPage(pdf, "05 / PROVENANCE & LIMITATIONS", "How to interpret this report",
            "Designed for a computer science capstone demonstration");
        pdf.colorText(p, 43, 588, 16, "Recorded sources", P::ink(), true, true);
        pdf.paragraph(p, 46, 560, 11,
            "Client relationships, brokerage and retirement accounts, holdings, "
            "securities, snapshots, goals and advisor tasks are read from the local "
            "SQLite database. Holdings are valued as stored quantity multiplied by "
            "the fictional security price. The same stored records are available "
            "to the Aurelia desktop interface.", 94, 6);
        pdf.colorText(p, 43, 444, 16, "Review observations", P::ink(), true, true);
        pdf.paragraph(p, 46, 416, 11,
            "Allocation charts describe modeled asset-class exposure. Goal funding "
            "ratios use current divided by target. Past-due task flags compare "
            "stored due dates with the SQLite current date. These checks identify "
            "records for human review, not appropriate trades or personalized advice.", 94, 6);
        pdf.colorText(p, 43, 306, 16, "Important limitations", P::ink(), true, true);
        pdf.paragraph(p, 46, 278, 11,
            "Market prices and transaction records are synthetic. Snapshot value "
            "changes do not adjust for contributions, withdrawals, benchmarks, "
            "fees or taxes and cannot be presented as investment returns. Legacy "
            "demo duplicates may remain until deliberately reviewed and cleaned. "
            "No client identity, custodian, balance or price is independently verified.", 94, 7);
        pdf.colorRectangle(p, 43, 97, 525, 69, P::pale(), P::pale());
        pdf.paragraph(p, 56, 140, 10,
            "NOT FOR CLIENT DISTRIBUTION OR FINANCIAL DECISIONS. "
            "A real deployment needs authenticated data feeds, permissions, "
            "secure document handling, reconciliation and supervised review.", 90, 3, P::ink(), 15);

        footers(pdf, date);
        pdf.save(filePath);
        repository_.recordAuditEvent("EXPORT_PDF_ENTERPRISE", std::nullopt,
            "Generated color enterprise portfolio report from SQLite records.");
    }

    void exportClientReport(int clientId, const std::string& filePath) {
        const ClientDashboard d = analytics_.clientDashboard(clientId);
        const vector<Account> accounts = repository_.getAccountsForClient(clientId);
        const vector<Holding> holdings = repository_.getHoldingsForClient(clientId);
        const vector<TransactionRecord> transactions = repository_.getTransactionsForClient(clientId, 1000000);
        const vector<AdvisorNote> notes = repository_.getNotesForClient(clientId);
        const std::string date = today(repository_.database());
        P pdf;

        int p = newPage(pdf, "01 / PRIVATE CLIENT INTELLIGENCE", d.client.name,
            shorten(d.client.householdName + "  /  Advisor: " + d.client.advisorName, 90));
        card(pdf, p, 42, 532, 164, "MODELED ASSETS", money(d.currentValue), "Based on stored holdings");
        card(pdf, p, 224, 532, 164, "UNREALIZED CHANGE", money(d.unrealizedGain), "Mock price vs cost basis");
        card(pdf, p, 406, 532, 164, "SNAPSHOT CHANGE",
            percent(d.twelveMonthChangePercent), "Not a modeled return");
        pdf.colorText(p, 42, 501, 15, "Asset allocation", P::ink(), true, true);
        chartBars(pdf, p, 45, 477, 515, d.actualAllocation);
        pdf.colorText(p, 42, 249, 15, "Stored value history", P::ink(), true, true);
        chartTrend(pdf, p, 44, 94, 522, 139, d.history);
        pdf.colorText(p, 46, 76, 8,
            "Policy comparisons are illustrative; stored prices are fictional.", P::muted());

        const PlanningProfile profile=repository_.getPlanningProfile(clientId);
        p=newPage(pdf,"02 / HOUSEHOLD PLANNING","Relationship profile","Fictional discovery record / advisor-entered context");
        const double runway=profile.annualExpenses>0?profile.emergencyReserve/(profile.annualExpenses/12):0;
        card(pdf,p,43,532,168,"ANNUAL INCOME",money(d.client.annualIncome),"Stored household income");
        card(pdf,p,222,532,168,"NET WORTH",money(d.client.netWorth),"Entered assets less liabilities");
        card(pdf,p,401,532,168,"CASH RUNWAY",percent(runway).substr(0,percent(runway).size()-1)+" months","Reserve / monthly expenses");
        std::vector<std::pair<std::string,std::string>> facts={{"Contact",d.client.email+" / "+d.client.phone},{"Occupation",profile.occupation+" / "+profile.employer},{"Location",profile.location},{"Household",d.client.householdName+" / dependents: "+std::to_string(profile.dependents)},{"Advisor",d.client.advisorName},{"Risk / horizon",riskLevelToString(d.client.riskLevel)+" / "+std::to_string(d.client.timeHorizonYears)+" years"},{"Annual spending",money(profile.annualExpenses)},{"Monthly saving",money(profile.monthlySaving)},{"Retirement / review",std::to_string(profile.retirementAge)+" / "+profile.reviewDate},{"Objective",d.client.investmentObjective}};
        double fy=497;for(const auto& f:facts){pdf.colorText(p,46,fy,9,f.first,P::muted(),true);pdf.colorText(p,182,fy,10,shorten(f.second,72),P::ink());fy-=29;}
        pdf.paragraph(p,46,178,10,profile.planningContext,96,5,P::muted(),14);
        p=newPage(pdf,"03 / PLANNING ANALYTICS","Funding gaps & priorities","Descriptive arithmetic / does not assume investment returns");
        double py=588;for(size_t gi=0;gi<d.goals.size();++gi){
            if(gi && gi%6==0){p=newPage(pdf,"03 / PLANNING ANALYTICS","Funding gaps & priorities","Descriptive arithmetic / does not assume investment returns");py=588;}
            const auto& goal=d.goals[gi];Statement months(repository_.database().handle(),"SELECT MAX(1,CAST((julianday(?)-julianday(date('now','-7 hours')))/30.4375 AS INTEGER));");months.bindText(1,goal.targetDate);months.step();int left=months.columnInt(0);double gap=std::max(goal.targetAmount-goal.currentAmount,0.0);
            pdf.colorText(p,46,py,12,shorten(goal.name,67),P::ink(),true);pdf.colorText(p,46,py-19,9,"Funding gap "+money(gap)+" / target "+goal.targetDate+" / simple monthly gap "+money(gap/left),P::muted());
            pdf.colorRectangle(p,46,py-36,520,8,P::pale(),P::pale());double funded=std::min(1.0,goal.currentAmount/goal.targetAmount);if(funded>0)pdf.colorRectangle(p,46,py-36,520*funded,8,P::sage(),P::sage());py-=77;
        }
        pdf.colorText(p,46,88,9,"Monthly gaps exclude returns, taxes and fees. Past target dates use one month for review.",P::muted());

        // All holdings are exported, not only those visible on the GUI's top list
        size_t offset = 0;
        do {
            p = newPage(pdf, "02 / PORTFOLIO RECORDS", "Account holdings",
                "Each line is the underlying SQLite holding and fictional quote");
            tableHead(pdf, p, 599, "SECURITY                         ASSET CLASS                   QUANTITY            VALUE             CHANGE");
            double y = 577;
            int index = 0;
            for (;offset < holdings.size() && index < 16;++offset, ++index) {
                const Holding& h = holdings[offset];
                std::ostringstream qty;
                qty << std::fixed << std::setprecision(2) << h.quantity;
                tableRow(pdf, p, index, y, h.symbol + " " + h.securityName, h.assetClass,
                    qty.str(), money(h.marketValue), money(h.unrealizedGain));
                y -= 32;
            }
            if (holdings.empty())pdf.colorText(p, 53, 558, 11,
                "No holdings are recorded for this client.", P::muted());
            pdf.colorText(p, 45, 79, 9,
                "Market value = quantity x stored mock price. Change = value - modeled cost.", P::muted());
        } while (offset < holdings.size());

        // Account register, including custodian and tax treatment
        p = newPage(pdf, "03 / ACCOUNT ADMINISTRATION", "Account register",
            "Account labels are fictional and do not indicate bank custody");
        tableHead(pdf, p, 599, "ACCOUNT                         TYPE                           CUSTODIAN            TREATMENT");
        double y = 577;
        for (size_t i = 0;i < accounts.size() && i < 14;++i) {
            const Account& a = accounts[i];
            tableRow(pdf, p, (int)i, y, a.accountName, a.accountType,
                a.custodian, a.taxTreatment, a.active ? "Active" : "Closed");
            y -= 34;
        }
        if (accounts.empty())pdf.colorText(p, 53, 558, 11,
            "No accounts are recorded.", P::muted());
        pdf.colorText(p, 45, 89, 9,
            "Client risk profile: " + riskLevelToString(d.client.riskLevel) +
            " | Time horizon: " + std::to_string(d.client.timeHorizonYears) + " years", P::muted());

        // Transaction ledger, fully paginated
        offset = 0;
        do {
            p = newPage(pdf, "04 / TRANSACTION JOURNAL", "Recorded activity",
                "Synthetic transaction ledger, newest date first");
            tableHead(pdf, p, 599, "DATE                            TYPE                           SYMBOL              AMOUNT             QTY");
            y = 577;
            int index = 0;
            for (;offset < transactions.size() && index < 16;++offset, ++index) {
                const TransactionRecord& t = transactions[offset];
                std::ostringstream qty;
                qty << std::fixed << std::setprecision(2) << t.quantity;
                tableRow(pdf, p, index, y, t.date, t.type, t.symbol,
                    money(t.amount), qty.str());
                y -= 32;
            }
            if (transactions.empty())pdf.colorText(p, 53, 558, 11,
                "No transactions are recorded.", P::muted());
        } while (offset < transactions.size());

        // Goals, paginated separately so no task is lost or overprinted
        offset = 0;
        do {
            p = newPage(pdf, "05 / GOAL PLANNING", "Funding goals",
                "Current/target funding is descriptive, not a probability of success");
            y = 590;
            int index = 0;
            for (;offset < d.goals.size() && index < 9;++offset, ++index) {
                const FinancialGoal& g = d.goals[offset];
                if (index % 2 == 0)
                    pdf.colorRectangle(p, 43, y - 13, 525, 46, P::white(), P::white());
                pdf.colorText(p, 53, y + 14, 10, shorten(g.name, 61), P::ink(), true);
                pdf.colorText(p, 53, y - 4, 9,
                    money(g.currentAmount) + " of " + money(g.targetAmount) +
                    " | " + goalFunding(g) + " funded | target date: " + g.targetDate,
                    P::muted());
                y -= 54;
            }
            if (d.goals.empty())
                pdf.colorText(p, 53, 568, 10, "No stored financial goals.", P::muted());
        } while (offset < d.goals.size());

        offset = 0;
        do {
            p = newPage(pdf, "06 / ADVISOR OPERATIONS", "Open relationship tasks",
                "Human follow-ups stored for the selected fictional client");
            tableHead(pdf, p, 599, "TASK                                                  DUE DATE                 PRIORITY");
            y = 577;
            int index = 0;
            for (;offset < d.tasks.size() && index < 15;++offset, ++index) {
                const AdvisorTask& t = d.tasks[offset];
                if (index % 2 == 0)
                    pdf.colorRectangle(p, 42, y - 7, 528, 26, P::white(), P::white());
                pdf.colorText(p, 50, y, 9, shorten(t.title, 64), P::ink());
                pdf.colorText(p, 433, y, 9, t.dueDate,
                    t.dueDate < date ? P::clay() : P::muted());
                pdf.colorText(p, 529, y, 8, shorten(t.priority, 8), P::muted());
                y -= 33;
            }
            if (d.tasks.empty())
                pdf.colorText(p, 53, 557, 10, "No open advisor tasks.", P::muted());
        } while (offset < d.tasks.size());

        // Insights are generated by the existing analytics service
        offset = 0;
        do {
            p = newPage(pdf, "07 / ADVISOR REVIEW", "Review observations",
                "Rule-based monitoring prompts for professional discussion");
            y = 600;
            int index = 0;
            for (;offset < d.insights.size() && index < 6;++offset, ++index) {
                const ClientInsight& in = d.insights[offset];
                pdf.colorRectangle(p, 43, y - 53, 525, 71, P::white(), P::pale(), .6);
                pdf.colorRectangle(p, 43, y - 53, 4, 71,
                    in.severity == "High" ? P::clay() : P::sage(), P::sage());
                pdf.colorText(p, 55, y, 11,
                    shorten(in.severity + "  /  " + in.title, 77), P::ink(), true);
                pdf.paragraph(p, 55, y - 19, 9, in.detail, 99, 2, P::muted(), 13);
                y -= 79;
            }
            if (d.insights.empty())pdf.colorText(p, 53, 580, 11,
                "No rule-based review signals are currently recorded.", P::muted());
            pdf.colorText(p, 45, 80, 9,
                "Review signals are hypotheses to verify, not buy/sell recommendations.", P::muted());
        } while (offset < d.insights.size());

        offset=0;
        do {
            p=newPage(pdf,"08 / TICKET ACTION DESK","Practical advisor follow-ups","Priority and ownership / documented human actions");double ay=584;int count=0;
            for(;offset<d.tasks.size()&&count<5;++offset,++count){const auto& task=d.tasks[offset];pdf.colorRectangle(p,43,ay-78,525,101,P::white(),P::pale(),.5);pdf.colorText(p,54,ay+5,11,shorten("AW-"+std::to_string(task.id)+" ["+task.priority+"] "+task.title,78),P::ink(),true);pdf.colorText(p,54,ay-13,9,task.assignedTo+" / "+task.status+" / due "+task.dueDate,P::antique());pdf.paragraph(p,54,ay-31,9,task.description,98,3,P::muted(),13);ay-=108;}
        }while(offset<d.tasks.size());

        // Advisor notes are distinct from automated observations
        offset = 0;
        do {
            p = newPage(pdf, "08 / RELATIONSHIP NOTES", "Advisor notes",
                "Recorded notes for this fictional relationship");
            y = 601;
            int index = 0;
            for (;offset < notes.size() && index < 9;++offset, ++index) {
                const AdvisorNote& note = notes[offset];
                pdf.colorText(p, 48, y, 9, shorten(note.createdAt, 30), P::antique(), true);
                pdf.paragraph(p, 48, y - 18, 9, note.noteText, 99, 3, P::ink(), 13);
                y -= 57;
            }
            if (notes.empty())pdf.colorText(p, 53, 575, 10,
                "No advisor notes have been recorded.", P::muted());
        } while (offset < notes.size());

        p = newPage(pdf, "09 / MODEL METHODOLOGY", "Scope and limitations",
            "Read before interpreting any client-specific numbers");
        pdf.colorText(p, 43, 588, 16, "Data definitions", P::ink(), true, true);
        pdf.paragraph(p, 46, 559, 11,
            "Values come from the persistent SQLite client, account, security, "
            "holding, snapshot, goal, transaction, note and task tables. "
            "Portfolio value is the sum of quantity times the mock price. "
            "Unrealized change compares that value with modeled cost basis.", 94, 6);
        pdf.colorText(p, 43, 435, 16, "Financial interpretation", P::ink(), true, true);
        pdf.paragraph(p, 46, 407, 11,
            "Historical changes include synthetic contributions and withdrawals "
            "and are not annualized, time-weighted or money-weighted performance. "
            "Illustrative allocations, risk labels and flags do not establish "
            "suitability or a recommendation. Confirm all financial assumptions "
            "with a qualified professional before applying any real-world model.", 94, 7);
        pdf.colorRectangle(p, 43, 124, 525, 95, P::pale(), P::pale());
        pdf.paragraph(p, 57, 193, 10,
            "CAPSTONE DATA ONLY. Not connected to a brokerage, custodian, "
            "bank or market-data service. This document is not an account "
            "statement, investment recommendation, tax report or "
            "compliance-approved client communication.", 88, 5, P::ink(), 15);

        p=newPage(pdf,"10 / MARKET PROVENANCE","Sourced market observations","Reference snapshots; portfolio values above retain the fictional price basis");
        Statement sources(repository_.database().handle(),"SELECT symbol,price,as_of_date,source FROM market_quotes ORDER BY symbol;");double my=584;
        while(sources.step()==SQLITE_ROW){if(my<150){p=newPage(pdf,"10 / MARKET PROVENANCE","Sourced market observations","Reference snapshots / source dates are explicit");my=584;}pdf.colorText(p,46,my,12,sources.columnText(0)+" / "+quoteMoney(sources.columnDouble(1))+" / "+sources.columnText(2),P::ink(),true);pdf.paragraph(p,46,my-18,8,sources.columnText(3),115,2,P::muted(),12);my-=72;}
        pdf.colorText(p,46,90,9,"A date-stamped observation is not a connected live feed. Missing quotes are not invented.",P::muted());

        footers(pdf, date);
        pdf.save(filePath);
        repository_.recordAuditEvent("EXPORT_PDF_CLIENT", clientId,
            "Generated color client portfolio report from SQLite records.");
    }
};
