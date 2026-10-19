#pragma once
/*
 * AURELIA WEALTH STUDIO / ATELIER REPORTING
 * CS 499 Computer Science Capstone | Brianna Reed
 *
 * A native C++17 Open XML workbook exporter, using the current
 * SQLite repository and analytics interfaces
 *
 * Features:
 *   - Multiple reconciled SQLite backed worksheets
 *   - Editable native line and column charts, with cached values
 *   - Dark ink, ivory, antique gold and sage Excel styling
 *   - Financial number formatting, pane freezing and filtering
 *   - Formula driven holdings and goal calculations
 *   - Client scoped and enterprise level XLSX reports
 *   - Explicit provenance and fictional data disclosures
 *
 * No live data connections or investment advice are provided
 * Existing public export method names are intentionally preserved
 */

#include "InvestmentAnalytics.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

 // A minimal *uncompressed* ZIP writer. Valid Office Open XML does not
 // require ZIP compression, and an uncompressed archive is simpler to audit
class MiniZipWriter {
    struct Entry {
        std::string name;
        std::string data;
        std::uint32_t crc = 0;
        std::uint32_t offset = 0;
    };

    std::vector<Entry> entries_;

    static void write16(std::ofstream& out, std::uint16_t v) {
        out.put(static_cast<char>(v & 255));
        out.put(static_cast<char>((v >> 8) & 255));
    }

    static void write32(std::ofstream& out, std::uint32_t v) {
        write16(out, static_cast<std::uint16_t>(v & 65535));
        write16(out, static_cast<std::uint16_t>(v >> 16));
    }

    static std::uint32_t crc32(const std::string& data) {
        std::uint32_t crc = 0xFFFFFFFFu;
        for (unsigned char c : data) {
            crc ^= c;
            for (int bit = 0; bit < 8; ++bit) {
                crc = (crc & 1u) ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
            }
        }
        return crc ^ 0xFFFFFFFFu;
    }

    static std::uint32_t checked32(std::uint64_t value) {
        if (value > 0xFFFFFFFFu) {
            throw std::runtime_error("XLSX exceeds the ZIP32 format limit.");
        }
        return static_cast<std::uint32_t>(value);
    }

public:
    void add(const std::string& name, const std::string& data) {
        if (name.size() > 65535 || data.size() > 0xFFFFFFFFull) {
            throw std::runtime_error("XLSX entry is too large.");
        }
        entries_.push_back({ name, data, crc32(data), 0 });
    }

    void save(const std::string& path) {
        if (entries_.size() > 65535) {
            throw std::runtime_error("Too many XLSX archive entries.");
        }
        std::ofstream out(path, std::ios::binary);
        if (!out) {
            throw std::runtime_error("Cannot create the Excel workbook.");
        }

        for (Entry& e : entries_) {
            e.offset = checked32(static_cast<std::uint64_t>(out.tellp()));
            write32(out, 0x04034B50);
            write16(out, 20);            // Version needed
            write16(out, 0);             // General purpose bit flags
            write16(out, 0);             // Compression: stored
            write16(out, 0); write16(out, 0); // Time / date
            write32(out, e.crc);
            write32(out, checked32(e.data.size()));
            write32(out, checked32(e.data.size()));
            write16(out, static_cast<std::uint16_t>(e.name.size()));
            write16(out, 0);
            out.write(e.name.data(), static_cast<std::streamsize>(e.name.size()));
            out.write(e.data.data(), static_cast<std::streamsize>(e.data.size()));
        }

        const std::uint32_t directoryStart =
            checked32(static_cast<std::uint64_t>(out.tellp()));

        for (const Entry& e : entries_) {
            write32(out, 0x02014B50);
            write16(out, 20); write16(out, 20);
            write16(out, 0); write16(out, 0);
            write16(out, 0); write16(out, 0);
            write32(out, e.crc);
            write32(out, checked32(e.data.size()));
            write32(out, checked32(e.data.size()));
            write16(out, static_cast<std::uint16_t>(e.name.size()));
            write16(out, 0); write16(out, 0);
            write16(out, 0); write16(out, 0);
            write32(out, 0);
            write32(out, e.offset);
            out.write(e.name.data(), static_cast<std::streamsize>(e.name.size()));
        }

        const std::uint32_t directoryEnd =
            checked32(static_cast<std::uint64_t>(out.tellp()));
        write32(out, 0x06054B50);
        write16(out, 0); write16(out, 0);
        write16(out, static_cast<std::uint16_t>(entries_.size()));
        write16(out, static_cast<std::uint16_t>(entries_.size()));
        write32(out, directoryEnd - directoryStart);
        write32(out, directoryStart);
        write16(out, 0);
        if (!out) {
            throw std::runtime_error("The Excel workbook could not be written.");
        }
    }
};

class ExcelExporter {
    enum class Kind { Text, Integer, Number, Currency, Percent };

    struct Column {
        std::string heading;
        Kind kind = Kind::Text;
        int width = 19;
    };

    struct Cell {
        std::string value;
        bool numeric = false;
        std::string formula;          // Optional native Excel formula
        Kind kind = Kind::Text;
    };

    struct Sheet {
        std::string name;
        std::string description;
        std::vector<Column> columns;
        std::vector<std::vector<Cell>> rows;
        bool hasCharts = false;
    };

    ClientRepository& repository_;
    AnalyticsService& analytics_;
    Database& database_;

    static Cell txt(const std::string& s) {
        return { s, false, "", Kind::Text };
    }

    static Cell num(double n, Kind kind = Kind::Number) {
        if (!std::isfinite(n)) n = 0;
        std::ostringstream out;
        out << std::setprecision(15) << n;
        return { out.str(), true, "", kind };
    }

    static std::string xml(const std::string& raw) {
        std::string s;
        s.reserve(raw.size() + 10);
        for (unsigned char c : raw) {
            if (c == '&') s += "&amp;";
            else if (c == '<') s += "&lt;";
            else if (c == '>') s += "&gt;";
            else if (c == '"') s += "&quot;";
            else if (c == '\'') s += "&apos;";
            else if (c == 9 || c == 10 || c == 13 || c >= 32) s += static_cast<char>(c);
        }
        return s;
    }

    static std::string reference(int zeroBasedIndex) {
        std::string s;
        int n = zeroBasedIndex + 1;
        while (n) {
            s.insert(s.begin(), static_cast<char>('A' + (n - 1) % 26));
            n = (n - 1) / 26;
        }
        return s;
    }

    static std::string quotedSheet(const std::string& name) {
        std::string escaped;
        for (char c : name) {
            if (c == '\'') escaped += "''";
            else escaped.push_back(c);
        }
        return "'" + escaped + "'!";
    }

    static std::string fixed(double n, int decimals = 2) {
        std::ostringstream o;
        o << std::fixed << std::setprecision(decimals) << n;
        return o.str();
    }

    // Worksheet specific cell style IDs, defined in stylesXml()
    static int cellStyle(const Cell& cell, bool alternate) {
        if (!cell.numeric) return alternate ? 4 : 0;
        switch (cell.kind) {
        case Kind::Currency: return alternate ? 6 : 5;
        case Kind::Percent:  return alternate ? 8 : 7;
        case Kind::Integer:  return alternate ? 10 : 9;
        case Kind::Number:   return alternate ? 12 : 11;
        default:             return alternate ? 4 : 0;
        }
    }

    // Every SQL value comes from SQLite; no in memory manufactured client
    // data is inserted into detailed worksheets
    Sheet query(
        const std::string& name,
        const std::string& description,
        const std::vector<Column>& columns,
        const std::string& sql,
        int boundClientId = 0) {

        Sheet result{ name, description, columns, {}, false };
        Statement statement(database_.handle(), sql);
        if (boundClientId > 0) statement.bindInt(1, boundClientId);

        int code;
        while ((code = statement.step()) == SQLITE_ROW) {
            std::vector<Cell> row;
            for (size_t c = 0; c < columns.size(); ++c) {
                if (columns[c].kind == Kind::Text) {
                    row.push_back(txt(statement.columnText(static_cast<int>(c))));
                }
                else {
                    row.push_back(num(
                        statement.columnDouble(static_cast<int>(c)),
                        columns[c].kind));
                }
            }
            result.rows.push_back(std::move(row));
        }
        if (code != SQLITE_DONE)
            throw DatabaseException(sqlite3_errmsg(database_.handle()));
        return result;
    }

    static Sheet overview(
        const EnterpriseSummary& summary,
        bool clientReport,
        const std::string& clientName = "") {

        Sheet s;
        s.name = "Overview";
        s.description = clientReport
            ? "Client relationship / portfolio data"
            : "Enterprise intelligence / live SQLite records";
        s.columns = { {"MEASURE", Kind::Text, 28},
                     {"RECORDED VALUE", Kind::Text, 23},
                     {"HOW TO INTERPRET", Kind::Text, 63} };
        s.hasCharts = true;

        if (clientReport) {
            s.rows.push_back({ txt("Selected client"), txt(clientName),
                txt("All workbook relationship records belong to this client.") });
        }

        s.rows.push_back({ txt("Assets administered"),
            num(summary.assetsAdministered, Kind::Currency),
            txt("SUM(stored holding quantity x stored mock security price)") });
        s.rows.push_back({ txt("Unrealized change"),
            num(summary.unrealizedGain, Kind::Currency),
            txt("Current modeled value minus modeled acquisition cost") });
        s.rows.push_back({ txt("Relationship count"),
            num(summary.clientCount, Kind::Integer),
            txt("Relational client records within the report scope") });
        s.rows.push_back({ txt("Active relationships"),
            num(summary.activeClients, Kind::Integer),
            txt("Client.active = 1") });
        s.rows.push_back({ txt("Open advisor tasks"),
            num(summary.openTasks, Kind::Integer),
            txt("Stored tasks that have not been marked complete") });
        s.rows.push_back({ txt("Overdue tasks"),
            num(summary.overdueTasks, Kind::Integer),
            txt("Open tasks with due date before SQLite date('now','-7 hours')") });
        s.rows.push_back({ txt("Snapshot change"),
            num(summary.twelveMonthChangePercent, Kind::Percent),
            txt("Change in modeled portfolio value; NOT investment return") });
        s.rows.push_back({ txt("Data limitation"),
            txt("Fictional / demonstration only"),
            txt("No bank connectivity, live quotes or individualized advice") });
        return s;
    }

    static Sheet allocation(
        const std::vector<AllocationSlice>& slices) {

        Sheet s;
        s.name = "Allocation";
        s.description = "Actual modeled holdings / asset classes";
        s.columns = { {"ASSET CLASS", Kind::Text, 32},
                     {"MODELED VALUE", Kind::Currency, 22},
                     {"SHARE (%)", Kind::Percent, 16} };
        for (const AllocationSlice& a : slices)
            s.rows.push_back({ txt(a.label),
                num(a.value, Kind::Currency),
                num(a.percent, Kind::Percent) });

        const size_t end = s.rows.size() + 4;
        for (size_t i = 0; i < s.rows.size(); ++i) {
            if (end > 4) {
                const size_t row = i + 5;
                s.rows[i][2].formula =
                    "IFERROR(B" + std::to_string(row) +
                    "/SUM($B$5:$B$" + std::to_string(end) + ")*100,0)";
            }
        }
        return s;
    }

    static Sheet history(
        const std::vector<PortfolioSnapshot>& snapshots) {

        Sheet s;
        s.name = "History";
        s.description = "Stored monthly snapshots / change is not investment return";
        s.columns = { {"SNAPSHOT DATE", Kind::Text, 19},
                     {"MODELED VALUE", Kind::Currency, 25},
                     {"CONTRIBUTIONS", Kind::Currency, 22},
                     {"WITHDRAWALS", Kind::Currency, 22},
                     {"VALUE CHANGE (%)", Kind::Percent, 21} };

        const double first = snapshots.empty() ? 0 : snapshots.front().value;
        for (size_t i = 0; i < snapshots.size(); ++i) {
            const PortfolioSnapshot& p = snapshots[i];
            const double change = first > 0 ? (p.value / first - 1) * 100 : 0;
            s.rows.push_back({ txt(p.date),
                num(p.value, Kind::Currency),
                num(p.contributions, Kind::Currency),
                num(p.withdrawals, Kind::Currency),
                num(change, Kind::Percent) });
            s.rows.back()[4].formula =
                "IFERROR((B" + std::to_string(i + 5) +
                "/$B$5-1)*100,0)";
        }
        return s;
    }

    static void addHoldingsFormulas(Sheet& sh) {
        // Client, Account, Symbol, Security, Asset Class, Sector,
        // Quantity(G), Price(H), Cost/Unit(I), Value(J), Cost(K), Gain(L)
        for (size_t i = 0; i < sh.rows.size(); ++i) {
            auto row = std::to_string(i + 5);
            sh.rows[i][9].formula = "G" + row + "*H" + row;
            sh.rows[i][10].formula = "G" + row + "*I" + row;
            sh.rows[i][11].formula = "J" + row + "-K" + row;
        }
    }

    static void addGoalFormulas(Sheet& sh) {
        // Client, Goal, Type, Target(D), Current(E), Progress(F), etc
        for (size_t i = 0; i < sh.rows.size(); ++i) {
            auto row = std::to_string(i + 5);
            sh.rows[i][5].formula = "IFERROR(E" + row +
                "/D" + row + "*100,0)";
        }
    }

    static std::string stylesXml() {
        return R"XML(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<styleSheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">
 <numFmts count="4">
  <numFmt numFmtId="164" formatCode="&quot;$&quot;#,##0.00;[Red](&quot;$&quot;#,##0.00)"/>
  <numFmt numFmtId="165" formatCode="0.0&quot;%&quot;"/>
  <numFmt numFmtId="166" formatCode="#,##0"/>
  <numFmt numFmtId="167" formatCode="#,##0.0000"/>
 </numFmts>
 <fonts count="4">
  <font><sz val="10"/><name val="Aptos"/><color rgb="FF223033"/></font>
  <font><b/><sz val="20"/><name val="Georgia"/><color rgb="FFF7F1E5"/></font>
  <font><b/><sz val="10"/><name val="Aptos"/><color rgb="FFFFFFFF"/></font>
  <font><sz val="10"/><name val="Aptos"/><color rgb="FF796D61"/></font>
 </fonts>
 <fills count="5">
  <fill><patternFill patternType="none"/></fill>
  <fill><patternFill patternType="gray125"/></fill>
  <fill><patternFill patternType="solid"><fgColor rgb="FF1F292C"/></patternFill></fill>
  <fill><patternFill patternType="solid"><fgColor rgb="FFF3EDE3"/></patternFill></fill>
  <fill><patternFill patternType="solid"><fgColor rgb="FFB28A48"/></patternFill></fill>
 </fills>
 <borders count="2">
  <border><left/><right/><top/><bottom/><diagonal/></border>
  <border><left/><right/><top/><bottom style="thin"><color rgb="FFDFD5C5"/></bottom><diagonal/></border>
 </borders>
 <cellStyleXfs count="1"><xf numFmtId="0" fontId="0" fillId="0" borderId="0"/></cellStyleXfs>
 <cellXfs count="13">
  <xf numFmtId="0" fontId="0" fillId="1" borderId="0" xfId="0" applyAlignment="1"><alignment vertical="center" wrapText="1"/></xf>
  <xf numFmtId="0" fontId="1" fillId="2" borderId="0" xfId="0" applyFont="1" applyFill="1"/>
  <xf numFmtId="0" fontId="3" fillId="3" borderId="0" xfId="0" applyFont="1" applyFill="1"/>
  <xf numFmtId="0" fontId="2" fillId="2" borderId="0" xfId="0" applyFont="1" applyFill="1"/>
  <xf numFmtId="0" fontId="0" fillId="3" borderId="1" xfId="0" applyFill="1" applyAlignment="1"><alignment vertical="center" wrapText="1"/></xf>
  <xf numFmtId="164" fontId="0" fillId="0" borderId="1" xfId="0" applyNumberFormat="1"/>
  <xf numFmtId="164" fontId="0" fillId="3" borderId="1" xfId="0" applyNumberFormat="1" applyFill="1"/>
  <xf numFmtId="165" fontId="0" fillId="0" borderId="1" xfId="0" applyNumberFormat="1"/>
  <xf numFmtId="165" fontId="0" fillId="3" borderId="1" xfId="0" applyNumberFormat="1" applyFill="1"/>
  <xf numFmtId="166" fontId="0" fillId="0" borderId="1" xfId="0" applyNumberFormat="1"/>
  <xf numFmtId="166" fontId="0" fillId="3" borderId="1" xfId="0" applyNumberFormat="1" applyFill="1"/>
  <xf numFmtId="167" fontId="0" fillId="0" borderId="1" xfId="0" applyNumberFormat="1"/>
  <xf numFmtId="167" fontId="0" fillId="3" borderId="1" xfId="0" applyNumberFormat="1" applyFill="1"/>
 </cellXfs>
 <cellStyles count="1"><cellStyle name="Normal" xfId="0" builtinId="0"/></cellStyles>
</styleSheet>)XML";
    }

    static std::string worksheetXml(const Sheet& sh) {
        std::ostringstream out;
        const int nCols = static_cast<int>(std::max<size_t>(sh.columns.size(), 3));
        const int last = static_cast<int>(sh.rows.size() + 4);

        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            << "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
            << "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
            << "<dimension ref=\"A1:" << reference(nCols - 1) << std::max(4, last) << "\"/>"
            << "<sheetViews><sheetView workbookViewId=\"0\" showGridLines=\"0\">"
            << "<pane ySplit=\"4\" topLeftCell=\"A5\" activePane=\"bottomLeft\" state=\"frozen\"/>"
            << "</sheetView></sheetViews>"
            << "<sheetFormatPr defaultRowHeight=\"19\"/><cols>";

        for (int i = 0; i < nCols; ++i) {
            int width = i < static_cast<int>(sh.columns.size())
                ? sh.columns[i].width : 20;
            out << "<col min=\"" << i + 1 << "\" max=\"" << i + 1
                << "\" width=\"" << std::max(11, std::min(width, 67))
                << "\" customWidth=\"1\"/>";
        }
        out << "</cols><sheetData>";

        auto inlineCell = [&](const std::string& cellRef, int style,
            const std::string& value) {
                out << "<c r=\"" << cellRef << "\" s=\"" << style
                    << "\" t=\"inlineStr\"><is><t xml:space=\"preserve\">"
                    << xml(value) << "</t></is></c>";
            };

        out << "<row r=\"1\" ht=\"38\" customHeight=\"1\">";
        inlineCell("A1", 1, "AURELIA  /  ATELIER WEALTH  /  BRIANNA REED");
        out << "</row><row r=\"2\" ht=\"25\" customHeight=\"1\">";
        inlineCell("A2", 2, sh.name + "  |  " + sh.description);
        out << "</row><row r=\"3\" ht=\"13\" customHeight=\"1\"/>";

        out << "<row r=\"4\" ht=\"28\" customHeight=\"1\">";
        for (size_t c = 0; c < sh.columns.size(); ++c)
            inlineCell(reference(static_cast<int>(c)) + "4", 3,
                sh.columns[c].heading);
        out << "</row>";

        for (size_t i = 0; i < sh.rows.size(); ++i) {
            const int rowNo = static_cast<int>(i + 5);
            size_t lines=1;
            for(size_t ci=0;ci<sh.rows[i].size()&&ci<sh.columns.size();++ci){
                if(sh.rows[i][ci].numeric)continue;
                const size_t chars=static_cast<size_t>(std::max(10,std::min(sh.columns[ci].width,67)-5));
                const auto& text=sh.rows[i][ci].value;
                lines=std::max(lines,(text.size()+chars-1)/chars+static_cast<size_t>(std::count(text.begin(),text.end(),'\n')));
            }
            const double rowHeight=std::min(409.0,std::max(23.0,8.0+15.0*static_cast<double>(lines)));
            out << "<row r=\"" << rowNo << "\" ht=\"" << rowHeight << "\" customHeight=\"1\">";

            for (size_t c = 0; c < sh.rows[i].size(); ++c) {
                const Cell& cell = sh.rows[i][c];
                const std::string cellRef = reference(static_cast<int>(c)) +
                    std::to_string(rowNo);
                if (!cell.numeric) {
                    inlineCell(cellRef, cellStyle(cell, (i % 2) == 1), cell.value);
                }
                else {
                    out << "<c r=\"" << cellRef << "\" s=\""
                        << cellStyle(cell, (i % 2) == 1) << "\">";
                    if (!cell.formula.empty())
                        out << "<f>" << xml(cell.formula) << "</f>";
                    out << "<v>" << cell.value << "</v></c>";
                }
            }
            out << "</row>";
        }
        out << "</sheetData>";

        if (!sh.rows.empty() && sh.columns.size() >= 2) {
            out << "<autoFilter ref=\"A4:" << reference(nCols - 1)
                << last << "\"/>";
        }
        out << "<mergeCells count=\"2\"><mergeCell ref=\"A1:"
            << reference(nCols - 1) << "1\"/><mergeCell ref=\"A2:"
            << reference(nCols - 1) << "2\"/></mergeCells>";
        out << "<pageMargins left=\"0.35\" right=\"0.35\" "
            << "top=\"0.6\" bottom=\"0.6\" header=\"0.25\" footer=\"0.25\"/>";
        if (sh.hasCharts) out << "<drawing r:id=\"rId1\"/>";
        out << "</worksheet>";
        return out.str();
    }

    static std::string chartXml(
        const Sheet& source, bool lineChart) {

        const size_t count = source.rows.size();
        const std::string base = quotedSheet(source.name);
        const std::string category = base + "$A$5:$A$" +
            std::to_string(count + 4);
        const std::string values = base + (lineChart ? "$B$5:$B$" : "$C$5:$C$") +
            std::to_string(count + 4);

        std::ostringstream out;
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            << "<c:chartSpace xmlns:c=\"http://schemas.openxmlformats.org/drawingml/2006/chart\" "
            << "xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\">"
            << "<c:chart><c:title><c:tx><c:rich><a:bodyPr/><a:lstStyle/><a:p><a:r><a:rPr lang=\"en-US\" sz=\"1300\"/><a:t>"
            << (lineChart ? "Stored portfolio value history" : "Asset allocation (%)")
            << "</a:t></a:r></a:p></c:rich></c:tx><c:overlay val=\"0\"/></c:title><c:plotArea><c:layout/>";
        if (lineChart)
            out << "<c:lineChart><c:grouping val=\"standard\"/><c:varyColors val=\"0\"/>";
        else
            out << "<c:barChart><c:barDir val=\"col\"/><c:grouping val=\"clustered\"/><c:varyColors val=\"0\"/>";

        out << "<c:ser><c:idx val=\"0\"/><c:order val=\"0\"/>"
            << "<c:tx><c:v>" << (lineChart ? "Modeled value" : "Share of assets (%)")
            << "</c:v></c:tx>"
            << "<c:spPr><a:solidFill><a:srgbClr val=\"B28A48\"/></a:solidFill>"
            << "<a:ln w=\"25400\"><a:solidFill><a:srgbClr val=\"B28A48\"/>"
            << "</a:solidFill></a:ln></c:spPr>";
        if (lineChart)
            out << "<c:marker><c:symbol val=\"circle\"/><c:size val=\"5\"/><c:spPr><a:solidFill><a:srgbClr val=\"B28A48\"/></a:solidFill><a:ln><a:solidFill><a:srgbClr val=\"B28A48\"/></a:solidFill></a:ln></c:spPr></c:marker>";

        out << "<c:cat><c:strRef><c:f>" << xml(category)
            << "</c:f><c:strCache><c:ptCount val=\"" << count << "\"/>";
        for (size_t i = 0; i < count; ++i) {
            out << "<c:pt idx=\"" << i << "\"><c:v>"
                << xml(source.rows[i][0].value) << "</c:v></c:pt>";
        }
        out << "</c:strCache></c:strRef></c:cat>"
            << "<c:val><c:numRef><c:f>" << xml(values) << "</c:f>"
            << "<c:numCache><c:formatCode>General</c:formatCode>"
            << "<c:ptCount val=\"" << count << "\"/>";
        for (size_t i = 0; i < count; ++i) {
            const size_t column = lineChart ? 1 : 2;
            out << "<c:pt idx=\"" << i << "\"><c:v>"
                << source.rows[i][column].value << "</c:v></c:pt>";
        }
        out << "</c:numCache></c:numRef></c:val></c:ser>"
            << "<c:axId val=\"10001\"/><c:axId val=\"10002\"/>";
        out << (lineChart ? "</c:lineChart>" : "</c:barChart>");

        out << "<c:catAx><c:axId val=\"10001\"/>"
            << "<c:scaling><c:orientation val=\"minMax\"/></c:scaling><c:delete val=\"0\"/>"
            << "<c:axPos val=\"b\"/><c:tickLblPos val=\"nextTo\"/>"
            << "<c:txPr><a:bodyPr/><a:lstStyle/><a:p><a:pPr><a:defRPr sz=\"900\"><a:solidFill><a:srgbClr val=\"64605B\"/></a:solidFill><a:latin typeface=\"Calibri\"/></a:defRPr></a:pPr><a:endParaRPr lang=\"en-US\"/></a:p></c:txPr><c:crossAx val=\"10002\"/><c:crosses val=\"autoZero\"/><c:auto val=\"1\"/><c:lblAlgn val=\"ctr\"/><c:lblOffset val=\"100\"/></c:catAx>"
            << "<c:valAx><c:axId val=\"10002\"/>"
            << "<c:scaling><c:orientation val=\"minMax\"/></c:scaling><c:delete val=\"0\"/>"
            << "<c:axPos val=\"l\"/><c:majorGridlines><c:spPr><a:ln w=\"6350\"><a:solidFill><a:srgbClr val=\"E6E0D6\"/></a:solidFill></a:ln></c:spPr></c:majorGridlines><c:numFmt formatCode=\"" << (lineChart?"$#,##0":"0.0&quot;%&quot;") << "\" sourceLinked=\"0\"/><c:tickLblPos val=\"nextTo\"/>"
            << "<c:txPr><a:bodyPr/><a:lstStyle/><a:p><a:pPr><a:defRPr sz=\"900\"><a:solidFill><a:srgbClr val=\"64605B\"/></a:solidFill><a:latin typeface=\"Calibri\"/></a:defRPr></a:pPr><a:endParaRPr lang=\"en-US\"/></a:p></c:txPr><c:crossAx val=\"10001\"/><c:crosses val=\"autoZero\"/><c:crossBetween val=\"between\"/></c:valAx>"
            << "</c:plotArea><c:plotVisOnly val=\"1\"/>"
            << "</c:chart></c:chartSpace>";
        return out.str();
    }

    static std::string drawingXml() {
        std::ostringstream out;
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            << "<xdr:wsDr xmlns:xdr=\"http://schemas.openxmlformats.org/drawingml/2006/spreadsheetDrawing\" "
            << "xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
            << "xmlns:c=\"http://schemas.openxmlformats.org/drawingml/2006/chart\" "
            << "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">";

        for (int i = 0; i < 2; ++i) {
            const int start = i == 0 ? 4 : 23;
            out << "<xdr:twoCellAnchor>"
                << "<xdr:from><xdr:col>4</xdr:col><xdr:colOff>0</xdr:colOff>"
                << "<xdr:row>" << start << "</xdr:row><xdr:rowOff>0</xdr:rowOff></xdr:from>"
                << "<xdr:to><xdr:col>12</xdr:col><xdr:colOff>0</xdr:colOff>"
                << "<xdr:row>" << start + 17 << "</xdr:row><xdr:rowOff>0</xdr:rowOff></xdr:to>"
                << "<xdr:graphicFrame macro=\"\"><xdr:nvGraphicFramePr>"
                << "<xdr:cNvPr id=\"" << i + 2 << "\" name=\""
                << (i == 0 ? "Portfolio snapshot trend" : "Asset allocation")
                << "\"/><xdr:cNvGraphicFramePr/></xdr:nvGraphicFramePr>"
                << "<xdr:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/></xdr:xfrm>"
                << "<a:graphic><a:graphicData uri=\"http://schemas.openxmlformats.org/drawingml/2006/chart\">"
                << "<c:chart r:id=\"rId" << i + 1 << "\"/></a:graphicData></a:graphic>"
                << "</xdr:graphicFrame><xdr:clientData/></xdr:twoCellAnchor>";
        }
        out << "</xdr:wsDr>";
        return out.str();
    }

    static std::string workbookXml(const std::vector<Sheet>& sheets) {
        std::ostringstream out;
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            << "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
            << "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
            << "<bookViews><workbookView/></bookViews><sheets>";
        for (size_t i = 0; i < sheets.size(); ++i)
            out << "<sheet name=\"" << xml(sheets[i].name) << "\" sheetId=\""
            << i + 1 << "\" r:id=\"rId" << i + 1 << "\"/>";
        out << "</sheets><calcPr calcId=\"191029\" fullCalcOnLoad=\"1\"/>"
            << "</workbook>";
        return out.str();
    }

    static std::string workbookRels(size_t count) {
        std::ostringstream out;
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            << "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">";
        for (size_t i = 0; i < count; ++i)
            out << "<Relationship Id=\"rId" << i + 1 << "\" "
            << "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" "
            << "Target=\"worksheets/sheet" << i + 1 << ".xml\"/>";
        out << "<Relationship Id=\"rId" << count + 1 << "\" "
            << "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" "
            << "Target=\"styles.xml\"/></Relationships>";
        return out.str();
    }

    static std::string contentTypes(size_t count) {
        std::ostringstream out;
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            << "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
            << "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
            << "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
            << "<Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>"
            << "<Override PartName=\"/xl/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml\"/>";
        for (size_t i = 1; i <= count; ++i)
            out << "<Override PartName=\"/xl/worksheets/sheet" << i
            << ".xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>";
        out << "<Override PartName=\"/xl/drawings/drawing1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.drawing+xml\"/>"
            << "<Override PartName=\"/xl/charts/chart1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.drawingml.chart+xml\"/>"
            << "<Override PartName=\"/xl/charts/chart2.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.drawingml.chart+xml\"/>"
            << "</Types>";
        return out.str();
    }

    void write(const std::string& path, const std::vector<Sheet>& sheets) {
        if (sheets.size() < 3 || !sheets.front().hasCharts)
            throw std::runtime_error("Aurelia workbook needs a dashboard and chart data.");
        MiniZipWriter zip;
        zip.add("[Content_Types].xml", contentTypes(sheets.size()));
        zip.add("_rels/.rels",
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
            "<Relationship Id=\"rId1\" "
            "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" "
            "Target=\"xl/workbook.xml\"/></Relationships>");
        zip.add("xl/workbook.xml", workbookXml(sheets));
        zip.add("xl/_rels/workbook.xml.rels", workbookRels(sheets.size()));
        zip.add("xl/styles.xml", stylesXml());

        for (size_t i = 0; i < sheets.size(); ++i)
            zip.add("xl/worksheets/sheet" + std::to_string(i + 1) + ".xml",
                worksheetXml(sheets[i]));

        zip.add("xl/worksheets/_rels/sheet1.xml.rels",
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
            "<Relationship Id=\"rId1\" "
            "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/drawing\" "
            "Target=\"../drawings/drawing1.xml\"/></Relationships>");
        zip.add("xl/drawings/drawing1.xml", drawingXml());
        zip.add("xl/drawings/_rels/drawing1.xml.rels",
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
            "<Relationship Id=\"rId1\" "
            "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/chart\" "
            "Target=\"../charts/chart1.xml\"/>"
            "<Relationship Id=\"rId2\" "
            "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/chart\" "
            "Target=\"../charts/chart2.xml\"/></Relationships>");
        zip.add("xl/charts/chart1.xml", chartXml(sheets[1], true));
        zip.add("xl/charts/chart2.xml", chartXml(sheets[2], false));
        zip.save(path);
    }

    static Sheet methodology(bool clientReport) {
        Sheet s;
        s.name = "Methodology";
        s.description = "Definitions, limits and source lineage";
        s.columns = { {"TOPIC", Kind::Text, 26},
                     {"DESCRIPTION", Kind::Text, 92} };
        s.rows = {
            {txt("Purpose"),txt("CS 499 demonstration of relational queries, secure data access and export workflows.")},
            {txt("Scope"),txt(clientReport ? "Selected fictional client only." : "All stored fictional client relationships.")},
            {txt("Data source"),txt("Local SQLite: clients, accounts, securities, holdings, transactions, snapshots, goals, tasks and notes.")},
            {txt("Valuation"),txt("Market value is holding quantity multiplied by stored mock security price.")},
            {txt("Unrealized change"),txt("Market value minus quantity times stored modeled cost per share.")},
            {txt("History"),txt("Snapshots are fictional; changes do not isolate contributions and withdrawals.")},
            {txt("Risk"),txt("Risk profiles and illustrative asset-class comparisons are synthetic training inputs.")},
            {txt("Goal funding"),txt("Current modeled goal amount divided by target. Not a success probability.")},
            {txt("Task dates"),txt("Overdue status compares stored date with SQLite date('now','-7 hours').")},
            {txt("Duplicates"),txt("Known historical demo duplicate candidates may require deliberate backed-up review.")},
            {txt("Limitations"),txt("No custodian verification, live market quote feeds, performance methodology or suitability certification.")},
            {txt("Distribution"),txt("NOT an account statement. NOT client advice. Never use synthetic prices for real decisions.")}
        };
        return s;
    }

    static Sheet holdingsSheetHeader() {
        Sheet s;
        s.name = "Holdings";
        s.description = "Source positions / calculated cells use Excel formulas";
        s.columns = {
            {"CLIENT",Kind::Text,24},{"ACCOUNT",Kind::Text,26},
            {"SYMBOL",Kind::Text,13},{"SECURITY",Kind::Text,45},
            {"ASSET CLASS",Kind::Text,23},{"SECTOR",Kind::Text,22},
            {"QUANTITY",Kind::Number,19},{"MOCK PRICE",Kind::Currency,18},
            {"COST PER UNIT",Kind::Currency,20},{"MARKET VALUE",Kind::Currency,22},
            {"TOTAL COST",Kind::Currency,22},{"UNREALIZED CHANGE",Kind::Currency,25}
        };
        return s;
    }

    static std::vector<Column> goalsColumns() {
        return {
            {"CLIENT",Kind::Text,24},{"GOAL",Kind::Text,34},
            {"TYPE",Kind::Text,18},{"TARGET",Kind::Currency,20},
            {"CURRENT",Kind::Currency,20},{"FUNDED (%)",Kind::Percent,18},
            {"TARGET DATE",Kind::Text,19},{"PRIORITY",Kind::Integer,14},
            {"STATUS",Kind::Text,18}
        };
    }

    std::vector<Sheet> sharedDataSheets(int clientId) {
        std::vector<Sheet> s;
        const bool scoped = clientId > 0;
        const std::string predicate = scoped ? " WHERE c.id=? " : " ";
        const std::string forAccounts = scoped ? " WHERE a.client_id=? " : " ";

        s.push_back(query(
            "Clients", "Stored relationship profiles and contact data",
            { {"ID",Kind::Integer,10},{"CLIENT",Kind::Text,24},
             {"HOUSEHOLD",Kind::Text,28},{"ADVISOR",Kind::Text,25},
             {"EMAIL",Kind::Text,35},{"PHONE",Kind::Text,23},
             {"AGE",Kind::Integer,12},{"SERVICE",Kind::Text,20},
             {"RISK",Kind::Text,20},{"ANNUAL INCOME",Kind::Currency,23},
             {"NET WORTH",Kind::Currency,23},{"LIQUIDITY",Kind::Text,18},
             {"HORIZON (YEARS)",Kind::Integer,19},
             {"OBJECTIVE",Kind::Text,38},{"ACTIVE",Kind::Integer,12} },
            "SELECT c.id,c.name,c.household_name,c.advisor_name,c.email,c.phone,"
            "c.age,st.name,r.name,c.annual_income,c.net_worth,c.liquidity_need,"
            "c.time_horizon_years,c.investment_objective,c.active "
            "FROM clients c JOIN service_types st ON st.id=c.service_id "
            "JOIN risk_levels r ON r.id=c.risk_level_id " + predicate +
            " ORDER BY c.name,c.id", clientId));

        s.push_back(query(
            "Accounts", "Account identifiers, types and modeled custody",
            { {"CLIENT",Kind::Text,24},{"ACCOUNT ID",Kind::Integer,15},
             {"ACCOUNT",Kind::Text,32},{"TYPE",Kind::Text,26},
             {"CUSTODIAN",Kind::Text,34},{"TAX TREATMENT",Kind::Text,22},
             {"ACTIVE",Kind::Integer,12},{"OPENED",Kind::Text,23} },
            "SELECT c.name,a.id,a.account_name,a.account_type,"
            "a.custodian,a.tax_treatment,a.active,a.opened_at "
            "FROM accounts a JOIN clients c ON c.id=a.client_id " +
            forAccounts + " ORDER BY c.name,a.id", clientId));

        Sheet holdings = query(
            "Holdings", "Current stored positions and derived values",
            holdingsSheetHeader().columns,
            "SELECT c.name,a.account_name,h.symbol,s.security_name,"
            "s.asset_class,s.sector,h.quantity,s.mock_price,"
            "h.cost_basis_per_share,h.quantity*s.mock_price,"
            "h.quantity*h.cost_basis_per_share,"
            "h.quantity*(s.mock_price-h.cost_basis_per_share) "
            "FROM holdings h JOIN accounts a ON a.id=h.account_id "
            "JOIN clients c ON c.id=a.client_id "
            "JOIN securities s ON s.symbol=h.symbol " +
            std::string(scoped ? " WHERE a.client_id=? " : " ") +
            " ORDER BY c.name,h.id", clientId);
        addHoldingsFormulas(holdings);
        s.push_back(std::move(holdings));

        s.push_back(query(
            "Transactions", "Persisted synthetic cash and security activity",
            { {"CLIENT",Kind::Text,24},{"ACCOUNT",Kind::Text,27},
             {"DATE",Kind::Text,19},{"TYPE",Kind::Text,20},
             {"SYMBOL",Kind::Text,13},{"QUANTITY",Kind::Number,19},
             {"AMOUNT",Kind::Currency,23},{"DESCRIPTION",Kind::Text,58} },
            "SELECT c.name,a.account_name,t.transaction_date,t.transaction_type,"
            "COALESCE(t.symbol,''),t.quantity,t.amount,t.description "
            "FROM transactions t JOIN accounts a ON a.id=t.account_id "
            "JOIN clients c ON c.id=a.client_id " +
            std::string(scoped ? " WHERE a.client_id=? " : " ") +
            " ORDER BY t.transaction_date DESC,t.id DESC", clientId));

        Sheet goals = query(
            "Goals", "Stored objectives and formula-driven funding ratios",
            goalsColumns(),
            "SELECT c.name,g.goal_name,g.goal_type,g.target_amount,"
            "g.current_amount,CASE WHEN g.target_amount>0 "
            "THEN 100.0*g.current_amount/g.target_amount ELSE 0 END,"
            "g.target_date,g.priority,g.status "
            "FROM financial_goals g JOIN clients c ON c.id=g.client_id " +
            predicate + " ORDER BY c.name,g.priority,g.id", clientId);
        addGoalFormulas(goals);
        s.push_back(std::move(goals));

        s.push_back(query(
            "Tasks", "Open and completed workflow records",
            { {"CLIENT",Kind::Text,24},{"TASK",Kind::Text,55},
             {"CATEGORY",Kind::Text,20},{"DUE",Kind::Text,19},
             {"PRIORITY",Kind::Text,16},{"COMPLETED",Kind::Text,18},
             {"OVERDUE",Kind::Text,16},{"TICKET",Kind::Text,18},{"ASSIGNED TO",Kind::Text,25},{"STATUS",Kind::Text,20},{"ACTION DETAILS",Kind::Text,90} },
            "SELECT COALESCE(c.name,'Enterprise'),t.title,t.category,"
            "t.due_date,t.priority,CASE WHEN t.completed=1 THEN 'Yes' ELSE 'No' END,"
            "CASE WHEN t.completed=0 AND date(t.due_date)<date('now','-7 hours') "
            "THEN 'Overdue' ELSE '' END,'AW-'||t.id,t.assigned_to,CASE WHEN t.completed=1 THEN 'Completed' ELSE t.workflow_status END,t.description "
            "FROM advisor_tasks t LEFT JOIN clients c ON c.id=t.client_id " +
            std::string(scoped ? " WHERE t.client_id=? " : " ") +
            " ORDER BY t.completed,t.due_date,t.id", clientId));

        s.push_back(query(
            "Notes", "Fictional relationship activity notes",
            { {"CLIENT",Kind::Text,25},{"CREATED",Kind::Text,24},
             {"ADVISOR NOTE",Kind::Text,85} },
            "SELECT c.name,n.created_at,n.note_text FROM advisor_notes n "
            "JOIN clients c ON c.id=n.client_id " + predicate +
            " ORDER BY n.created_at DESC,n.id DESC", clientId));
        s.push_back(query(
            "Planning Profile", "Detailed fictional household context / entered by the advisor",
            {{"CLIENT",Kind::Text,25},{"OCCUPATION",Kind::Text,28},{"EMPLOYER",Kind::Text,30},{"LOCATION",Kind::Text,24},
             {"DEPENDENTS",Kind::Integer,15},{"RETIREMENT AGE",Kind::Integer,18},{"ANNUAL EXPENSES",Kind::Currency,23},
             {"EMERGENCY RESERVE",Kind::Currency,25},{"MONTHLY SAVING",Kind::Currency,22},{"CONTACT",Kind::Text,18},
             {"NEXT REVIEW",Kind::Text,19},{"PLANNING CONTEXT",Kind::Text,85}},
            "SELECT c.name,p.occupation,p.employer,p.location,p.dependents,p.retirement_age,p.annual_expenses,p.emergency_reserve,p.monthly_saving,p.contact_preference,p.review_date,p.planning_context FROM clients c JOIN client_profile_details p ON p.client_id=c.id "+predicate+" ORDER BY c.name",clientId));
        s.push_back(query(
            "Planning Checks", "Descriptive planning ratios and funding gaps / human review required",
            {{"CLIENT",Kind::Text,25},{"GOAL",Kind::Text,38},{"TARGET DATE",Kind::Text,19},{"FUNDING GAP",Kind::Currency,22},
             {"MONTHS LEFT",Kind::Integer,16},{"MONTHLY GAP",Kind::Currency,22},{"CASH RUNWAY MONTHS",Kind::Number,24},
             {"SAVINGS / INCOME (%)",Kind::Percent,26}},
            "SELECT c.name,g.goal_name,g.target_date,MAX(g.target_amount-g.current_amount,0),MAX(1,CAST((julianday(g.target_date)-julianday(date('now','-7 hours')))/30.4375 AS INTEGER)),MAX(g.target_amount-g.current_amount,0)/MAX(1,CAST((julianday(g.target_date)-julianday(date('now','-7 hours')))/30.4375 AS INTEGER)),CASE WHEN p.annual_expenses>0 THEN p.emergency_reserve/(p.annual_expenses/12.0) ELSE 0 END,CASE WHEN c.annual_income>0 THEN 1200.0*p.monthly_saving/c.annual_income ELSE 0 END FROM clients c JOIN financial_goals g ON g.client_id=c.id LEFT JOIN client_profile_details p ON p.client_id=c.id "+predicate+" ORDER BY c.name,g.priority,g.id",clientId));
        s.push_back(query(
            "Market Sources", "Public/provider observations; not a real-time feed or the demo valuation basis",
            {{"SYMBOL",Kind::Text,13},{"OBSERVED PRICE",Kind::Currency,23},{"MARKET DATE",Kind::Text,19},
             {"SOURCE URL",Kind::Text,100},{"IMPORTED AT (UTC)",Kind::Text,25},{"DAYS OLD",Kind::Integer,15}},
            "SELECT symbol,price,as_of_date,source,fetched_at,CAST(julianday(date('now','-7 hours'))-julianday(as_of_date) AS INTEGER) FROM market_quotes ORDER BY symbol"));
        return s;
    }

public:
    ExcelExporter(ClientRepository& repository, AnalyticsService& analytics)
        : repository_(repository), analytics_(analytics),
        database_(repository.database()) {
    }

    // This interface is already used by AureliaDesktop.cpp
    void exportEnterpriseWorkbook(const std::string& filePath) {
        EnterpriseSummary summary = analytics_.enterpriseSummary();
        auto snapshots = analytics_.enterpriseHistory();
        auto slices = analytics_.enterpriseAllocation();

        std::vector<Sheet> sheets;
        sheets.push_back(overview(summary, false));
        sheets.push_back(history(snapshots));
        sheets.push_back(allocation(slices));

        auto details = sharedDataSheets(0);
        for (auto& d : details) sheets.push_back(std::move(d));

        sheets.push_back(query(
            "Audit", "Recorded database and export events",
            { {"TIMESTAMP",Kind::Text,24},{"EVENT",Kind::Text,34},
             {"CLIENT ID",Kind::Integer,15},{"DETAILS",Kind::Text,80} },
            "SELECT occurred_at,event_type,COALESCE(client_id,0),details "
            "FROM audit_events ORDER BY id DESC LIMIT 1000"));

        sheets.push_back(methodology(false));
        write(filePath, sheets);
        repository_.recordAuditEvent("EXPORT_XLSX_ENTERPRISE", std::nullopt,
            "Generated color-styled enterprise XLSX with native charts and SQLite data.");
    }

    // This interface is already used by AureliaDesktop.cpp
    void exportClientWorkbook(int clientId, const std::string& filePath) {
        const ClientDashboard dashboard = analytics_.clientDashboard(clientId);
        EnterpriseSummary s;
        s.clientCount = 1;
        s.activeClients = dashboard.client.active ? 1 : 0;
        s.assetsAdministered = dashboard.currentValue;
        s.unrealizedGain = dashboard.unrealizedGain;
        s.twelveMonthChangePercent = dashboard.twelveMonthChangePercent;
        s.openTasks = static_cast<int>(dashboard.tasks.size());

        Statement now(database_.handle(), "SELECT date('now','-7 hours')");
        std::string today = now.step() == SQLITE_ROW ? now.columnText(0) : "";
        for (const auto& task : dashboard.tasks)
            if (!today.empty() && task.dueDate < today) ++s.overdueTasks;

        std::vector<Sheet> sheets;
        sheets.push_back(overview(s, true, dashboard.client.name));
        sheets.push_back(history(dashboard.history));
        sheets.push_back(allocation(dashboard.actualAllocation));

        auto details = sharedDataSheets(clientId);
        for (auto& d : details) sheets.push_back(std::move(d));

        Sheet signals;
        signals.name = "Review Signals";
        signals.description = "Rule-based signals / verification required";
        signals.columns = { {"SEVERITY",Kind::Text,18},
                         {"OBSERVATION",Kind::Text,48},
                         {"INTERPRETATION",Kind::Text,92} };
        for (const ClientInsight& in : dashboard.insights)
            signals.rows.push_back({ txt(in.severity),txt(in.title),txt(in.detail) });
        sheets.push_back(std::move(signals));
        sheets.push_back(methodology(true));

        write(filePath, sheets);
        repository_.recordAuditEvent("EXPORT_XLSX_CLIENT", clientId,
            "Generated color-styled client XLSX with native charts and SQLite data.");
    }
};
