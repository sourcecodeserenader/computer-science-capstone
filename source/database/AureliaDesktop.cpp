/*
 * AURELIA ATELIER | WEALTH OPERATIONS
 * CS 499 Computer Science Capstone
 * Milestone Four: Enhancement Three, Databases
 * Student: Brianna Reed
 *
 * Windows desktop interface for the relational
 * portfolio intelligence system
 *
 * All records are fictional demonstration data
 */

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <gdiplus.h>
#include <shellapi.h>

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "InvestmentCore.h"
#include "InvestmentAnalytics.h"
#include "PdfReportWriter.h"
#include "ExcelExporter.h"

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Comdlg32.lib")
#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "Shell32.lib")

using namespace Gdiplus;


// ============================================================
// BRAND COLORS
// ============================================================

static const Color COLOR_WHITE(
    255, 255, 255, 255);

static const Color COLOR_CREAM(
    255, 246, 240, 230);

static const Color COLOR_TEXT(
    255, 29, 38, 42);

static const Color COLOR_MUTED(
    255, 100, 96, 91);

static const Color COLOR_GOLD(
    255, 173, 132, 68);

static const Color COLOR_SAGE(
    255, 128, 143, 108);

static const Color COLOR_TERRACOTTA(
    255, 168, 103, 88);

static const Color COLOR_DARK(
    255, 29, 38, 42);

static const Color COLOR_LINE(
    255, 217, 207, 192);

static const Color COLOR_SOFT_GOLD(
    255, 251, 242, 226);


// ============================================================
// STRING HELPERS
// ============================================================

static std::wstring widen(
    const std::string& value) {

    if (value.empty()) {

        return L"";
    }

    const int size =
        MultiByteToWideChar(
            CP_UTF8,
            0,
            value.c_str(),
            static_cast<int>(
                value.size()),
            nullptr,
            0);

    std::wstring result(
        size,
        L'\0');

    MultiByteToWideChar(
        CP_UTF8,
        0,
        value.c_str(),
        static_cast<int>(
            value.size()),
        result.data(),
        size);

    return result;
}


static std::string narrow(
    const std::wstring& value) {

    if (value.empty()) {

        return "";
    }

    const int size =
        WideCharToMultiByte(
            CP_UTF8,
            0,
            value.c_str(),
            static_cast<int>(
                value.size()),
            nullptr,
            0,
            nullptr,
            nullptr);

    std::string result(
        size,
        '\0');

    WideCharToMultiByte(
        CP_UTF8,
        0,
        value.c_str(),
        static_cast<int>(
            value.size()),
        result.data(),
        size,
        nullptr,
        nullptr);

    return result;
}


static std::wstring controlText(
    HWND control) {

    const int length =
        GetWindowTextLengthW(
            control);

    std::wstring result(
        length +
        1,
        L'\0');

    GetWindowTextW(
        control,
        result.data(),
        length +
        1);

    result.resize(
        length);

    return result;
}


static std::wstring moneyText(double value) {
    std::wostringstream out;out<<std::fixed<<std::setprecision(0)<<std::fabs(value);
    std::wstring digits=out.str();for(size_t i=digits.size();i>3;i-=3)digits.insert(i-3,L",");
    return std::wstring(value<0?L"-$":L"$")+digits;
}

static std::wstring percentText(
    double value) {

    std::wostringstream output;

    output
        << std::showpos
        << std::fixed
        << std::setprecision(
            1)
        << value
        << L"%";

    return output.str();
}


static std::wstring environmentValue(
    const wchar_t* name) {

    wchar_t* value =
        nullptr;

    size_t length =
        0;

    _wdupenv_s(
        &value,
        &length,
        name);

    std::wstring result =
        value
        ? value
        : L"";

    std::free(
        value);

    return result;
}


static void showMessage(
    HWND owner,
    const std::wstring& message,
    const wchar_t* title =
    L"Aurelia Atelier") {

    MessageBoxW(
        owner,
        message.c_str(),
        title,
        MB_OK |
        MB_ICONINFORMATION);
}


// ============================================================
// WINDOWS CONTROL FONTS
// ============================================================

static HFONT createFont(
    HWND reference,
    int points,
    bool bold =
    false) {

    HDC deviceContext =
        GetDC(
            reference);

    const int dpi =
        GetDeviceCaps(
            deviceContext,
            LOGPIXELSY);

    ReleaseDC(
        reference,
        deviceContext);

    LOGFONTW fontData = {};

    fontData.lfHeight =
        -MulDiv(
            points,
            dpi,
            72);

    fontData.lfWeight =
        bold
        ? FW_SEMIBOLD
        : FW_NORMAL;

    wcscpy_s(
        fontData.lfFaceName,
        L"Segoe UI");

    return CreateFontIndirectW(
        &fontData);
}


static void assignFont(
    HWND control,
    int points =
    10,
    bool bold =
    false) {

    HFONT font =
        createFont(
            control,
            points,
            bold);

    SendMessageW(
        control,
        WM_SETFONT,
        reinterpret_cast<
        WPARAM>(
            font),
        TRUE);
}


// ============================================================
// DRAWING HELPERS
// ============================================================

static void drawRoundedBox(
    Graphics& graphics,
    REAL x,
    REAL y,
    REAL width,
    REAL height,
    REAL radius,
    const Color& fill,
    const Color& border =
    Color(
        0,
        0,
        0,
        0)) {

    GraphicsPath path;

    const REAL diameter =
        radius *
        2.0f;

    path.AddArc(
        x,
        y,
        diameter,
        diameter,
        180,
        90);

    path.AddArc(
        x +
        width -
        diameter,
        y,
        diameter,
        diameter,
        270,
        90);

    path.AddArc(
        x +
        width -
        diameter,
        y +
        height -
        diameter,
        diameter,
        diameter,
        0,
        90);

    path.AddArc(
        x,
        y +
        height -
        diameter,
        diameter,
        diameter,
        90,
        90);

    path.CloseFigure();

    SolidBrush fillBrush(
        fill);

    graphics.FillPath(
        &fillBrush,
        &path);

    if (
        border.GetA() >
        0) {

        Pen borderPen(
            border,
            1.0f);

        graphics.DrawPath(
            &borderPen,
            &path);
    }
}


static void drawLabel(
    Graphics& graphics,
    const std::wstring& value,
    REAL x,
    REAL y,
    REAL width,
    REAL height,
    int size,
    const Color& color,
    bool bold =
    false,
    StringAlignment alignment =
    StringAlignmentNear) {

    FontFamily family(
        L"Segoe UI");

    Font font(
        &family,
        static_cast<REAL>(
            size),
        bold
        ? FontStyleBold
        : FontStyleRegular,
        UnitPixel);

    SolidBrush brush(
        color);

    StringFormat format;

    format.SetAlignment(
        alignment);

    format.SetLineAlignment(
        StringAlignmentCenter);

    format.SetTrimming(
        StringTrimmingEllipsisCharacter);

    graphics.DrawString(
        value.c_str(),
        -1,
        &font,
        RectF(
            x,
            y,
            width,
            height),
        &format,
        &brush);
}


// Planning panels accept numeric layout values and cast only at the GDI+ boundary
static void drawPlanningLabel(Graphics& g,const std::wstring& text,double x,double y,double w,double h,int size,const Color& color,bool bold=false,StringAlignment align=StringAlignmentNear) {
    drawLabel(g,text,static_cast<REAL>(x),static_cast<REAL>(y),static_cast<REAL>(w),static_cast<REAL>(h),size,color,bold,align);
}
static void drawPlanningBox(Graphics& g,double x,double y,double w,double h,double radius,const Color& fill,const Color& border) {
    drawRoundedBox(g,static_cast<REAL>(x),static_cast<REAL>(y),static_cast<REAL>(w),static_cast<REAL>(h),static_cast<REAL>(radius),fill,border);
}

static void drawTrackedBrand(
    Graphics& graphics,
    const std::wstring& text,
    REAL startX,
    REAL startY,
    REAL fontSize,
    const Color& color,
    REAL tracking =
    -1.4f) {

    FontFamily family(
        L"Georgia");

    Font font(
        &family,
        fontSize,
        FontStyleBold,
        UnitPixel);

    SolidBrush brush(
        color);

    REAL currentX =
        startX;

    for (
        wchar_t character :
    text) {

        wchar_t characterText[
            2] = {

            character,

            L'\0'
        };

        RectF bounds;

        graphics.MeasureString(
            characterText,
            -1,
            &font,
            PointF(
                0.0f,
                0.0f),
            &bounds);

        graphics.DrawString(
            characterText,
            -1,
            &font,
            PointF(
                currentX,
                startY),
            &brush);

        currentX +=
            bounds.Width +
            tracking;
    }
}


static void drawWealthEmblem(
    Graphics& graphics, REAL x, REAL y, REAL size) {
    // A true circular border, centered inside the emblem bounds
    Pen gold(COLOR_GOLD, 2.1f);
    Pen sage(COLOR_SAGE, 2.0f);
    Pen clay(COLOR_TERRACOTTA, 1.1f);
    gold.SetLineJoin(LineJoinRound);
    sage.SetLineJoin(LineJoinRound);
    const REAL inset = size * .04f;
    const REAL diameter = size - 2.0f * inset;
    graphics.DrawEllipse(&gold, x + inset, y + inset, diameter, diameter);
    PointF points[] = {
        PointF(x + size * .19f, y + size * .72f),
        PointF(x + size * .38f, y + size * .54f),
        PointF(x + size * .51f, y + size * .60f),
        PointF(x + size * .78f, y + size * .29f)
    };
    graphics.DrawLines(&sage, points, 4);
    graphics.DrawLine(&gold,
        x + size * .65f, y + size * .31f,
        x + size * .78f, y + size * .29f);
    graphics.DrawLine(&gold,
        x + size * .78f, y + size * .29f,
        x + size * .76f, y + size * .44f);
    graphics.DrawArc(&clay,
        x + size * .20f, y + size * .17f,
        size * .57f, size * .63f, 201, 110);
}

// ============================================================
// NAVIGATION
// ============================================================

#include "AureliaForms.h"
#include "AureliaMarketData.h"

enum class Page {

    Welcome,

    Login,

    Dashboard,

    Clients,

    ClientWorkspace,

    AdvisorTasks,

    FinancialGoals,

    AllocationDetail,

    HistoryDetail,

    Loading,

    Reports,

    Database,

    Audit,
    ClientDetails,
    MarketData
};


enum Action {

    ACTION_LOGIN_PAGE =
    1,

    ACTION_LOGIN,

    ACTION_LOGOUT,

    ACTION_DASHBOARD,

    ACTION_CLIENTS,

    ACTION_REPORTS,

    ACTION_DATABASE,

    ACTION_AUDIT,

    ACTION_OPEN_CLIENT,

    ACTION_BACK_TO_CLIENTS,

    ACTION_REFRESH,

    ACTION_TASKS,

    ACTION_GOALS,

    ACTION_ALLOCATION,

    ACTION_HISTORY,

    ACTION_TASK_COMPLETE,

    ACTION_TASK_CLIENT,

    ACTION_GOAL_CLIENT,

    ACTION_DUPLICATE_REVIEW,

    ACTION_EXPORT_ENTERPRISE_PDF,

    ACTION_EXPORT_ENTERPRISE_XLSX,

    ACTION_EXPORT_CLIENT_PDF,

    ACTION_EXPORT_CLIENT_XLSX,
    ACTION_CLIENT_DETAILS, ACTION_PROFILE_PORTFOLIO, ACTION_EDIT_PROFILE,
    ACTION_NEW_TASK, ACTION_EDIT_TASK, ACTION_NEW_GOAL, ACTION_EDIT_GOAL,
    ACTION_ADD_NOTE, ACTION_ADD_ACCOUNT, ACTION_SET_HOLDING, ACTION_MARKET, ACTION_RECORD_QUOTE, ACTION_REFRESH_QUOTE
};


struct ClickArea {

    RECT rectangle;

    int action;
};


// ============================================================
// FORWARD DECLARATIONS
// ============================================================

class AureliaApplication;


static LRESULT CALLBACK MainWindowProc(
    HWND,
    UINT,
    WPARAM,
    LPARAM);


// ============================================================
// MAIN APPLICATION
// ============================================================

class AureliaApplication {
public:

    HINSTANCE instance =
        nullptr;

    HWND window =
        nullptr;

    HWND usernameField =
        nullptr;

    HWND passwordField =
        nullptr;

    HWND searchField =
        nullptr;

    HWND clientList =
        nullptr;

    HWND holdingsList =
        nullptr;

    HWND tasksList = nullptr;
    HWND mediumTasksList = nullptr, completedTasksList = nullptr;
    int taskSortColumn = 0, clientSortColumn = 0;
    bool taskSortAscending = true, clientSortAscending = true;
    bool isTaskList(HWND h) const { return h && (h == tasksList || h == mediumTasksList || h == completedTasksList); }
    int taskRowIndex(HWND h, int row) const {
        LVITEMW item{}; item.mask = LVIF_PARAM; item.iItem = row;
        return row >= 0 && ListView_GetItem(h, &item) ? static_cast<int>(item.lParam) : -1;
    }
    void markSort(HWND list, int column, bool ascending) {
        HWND header = ListView_GetHeader(list);
        for (int i = 0; i < Header_GetItemCount(header); ++i) {
            HDITEMW item{}; item.mask = HDI_FORMAT; Header_GetItem(header, i, &item);
            item.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
            if (i == column) item.fmt |= ascending ? HDF_SORTUP : HDF_SORTDOWN;
            Header_SetItem(header, i, &item);
        }
    }
    HWND goalsList = nullptr;
    std::vector<AdvisorTask> displayedTasks;
    std::vector<FinancialGoal> displayedGoals;
    Page loadingDestination = Page::Dashboard;
    int loadingFrame = 0;


    Page currentPage =
        Page::Welcome;


    int width =
        1200;

    int height =
        780;

    int failedLoginAttempts =
        0;

    int selectedWorkspaceClientId =
        -1;


    bool authenticated =
        false;


    Database database;

    ClientRepository repository;

    AnalyticsService analytics;

    PdfReportWriter pdfWriter;

    ExcelExporter excelExporter;

    std::future<MarketObservation> marketRefresh;
    std::wstring marketStatus=L"Public 2026 observations are snapshots, not a connected real-time feed. Inspect the market date before use.";

    Logger logger;


    std::vector<Client>
        displayedClients;


    std::vector<ClickArea>
        clickAreas;


public:

    explicit AureliaApplication(
        HINSTANCE applicationInstance)
        :
        instance(
            applicationInstance),

        database(
            "cs499_clients.db"),

        repository(
            database),

        analytics(
            repository),

        pdfWriter(
            repository,
            analytics),

        excelExporter(
            repository,
            analytics),

        logger(
            "cs499_audit.log") {
    }


    // ========================================================
    // WINDOW
    // ========================================================

    void createMainWindow() {

        WNDCLASSW windowClass = {};

        windowClass.lpfnWndProc =
            MainWindowProc;

        windowClass.hInstance =
            instance;

        windowClass.lpszClassName =
            L"AureliaMainWindow";

        windowClass.hCursor =
            LoadCursorW(
                nullptr,
                IDC_ARROW);

        windowClass.hbrBackground =
            reinterpret_cast<
            HBRUSH>(
                COLOR_WINDOW +
                1);


        RegisterClassW(
            &windowClass);


        window =
            CreateWindowExW(
                0,
                L"AureliaMainWindow",
                L"Aurelia Atelier | Private Wealth Operations",
                WS_OVERLAPPEDWINDOW |
                WS_CLIPCHILDREN,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                1320,
                860,
                nullptr,
                nullptr,
                instance,
                this);


        if (!window) {

            throw std::runtime_error(
                "Unable to create "
                "Aurelia Wealth Studio.");
        }


        createControls();


        ShowWindow(
            window,
            SW_SHOW);


        UpdateWindow(
            window);


        updateLayout();
    }


    // ========================================================
    // CONTROLS
    // ========================================================

    void createControls() {

        usernameField =
            CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"EDIT",
                L"",
                WS_CHILD |
                WS_TABSTOP |
                ES_AUTOHSCROLL,
                0,
                0,
                320,
                36,
                window,
                reinterpret_cast<
                HMENU>(
                    1001),
                instance,
                nullptr);


        passwordField =
            CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"EDIT",
                L"",
                WS_CHILD |
                WS_TABSTOP |
                ES_PASSWORD |
                ES_AUTOHSCROLL,
                0,
                0,
                320,
                36,
                window,
                reinterpret_cast<
                HMENU>(
                    1002),
                instance,
                nullptr);


        searchField =
            CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"EDIT",
                L"",
                WS_CHILD |
                WS_TABSTOP |
                ES_AUTOHSCROLL,
                0,
                0,
                360,
                36,
                window,
                reinterpret_cast<
                HMENU>(
                    1003),
                instance,
                nullptr);


        clientList =
            CreateWindowExW(
                WS_EX_CLIENTEDGE,
                WC_LISTVIEWW,
                L"",
                WS_CHILD |
                WS_TABSTOP |
                LVS_REPORT |
                LVS_SINGLESEL |
                LVS_SHOWSELALWAYS,
                0,
                0,
                900,
                420,
                window,
                reinterpret_cast<
                HMENU>(
                    1004),
                instance,
                nullptr);


        holdingsList =
            CreateWindowExW(
                WS_EX_CLIENTEDGE,
                WC_LISTVIEWW,
                L"",
                WS_CHILD |
                WS_TABSTOP |
                LVS_REPORT |
                LVS_SINGLESEL |
                LVS_SHOWSELALWAYS,
                0,
                0,
                900,
                260,
                window,
                reinterpret_cast<
                HMENU>(
                    1005),
                instance,
                nullptr);


        tasksList = CreateWindowExW(
            WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
            WS_CHILD | WS_TABSTOP | LVS_REPORT |
            LVS_SINGLESEL | LVS_SHOWSELALWAYS,
            0, 0, 920, 400, window,
            reinterpret_cast<HMENU>(1006), instance, nullptr);

        mediumTasksList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, 0, 0, 300, 400, window, reinterpret_cast<HMENU>(1008), instance, nullptr);
        completedTasksList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, 0, 0, 300, 400, window, reinterpret_cast<HMENU>(1009), instance, nullptr);
        for (HWND lane : { mediumTasksList, completedTasksList }) ListView_SetExtendedListViewStyle(lane, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES | LVS_EX_INFOTIP);
        goalsList = CreateWindowExW(
            WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
            WS_CHILD | WS_TABSTOP | LVS_REPORT |
            LVS_SINGLESEL | LVS_SHOWSELALWAYS,
            0, 0, 920, 400, window,
            reinterpret_cast<HMENU>(1007), instance, nullptr);

        ListView_SetExtendedListViewStyle(
            tasksList, LVS_EX_FULLROWSELECT |
            LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES | LVS_EX_INFOTIP);
        ListView_SetExtendedListViewStyle(
            goalsList, LVS_EX_FULLROWSELECT |
            LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES | LVS_EX_INFOTIP);

        const wchar_t* taskNames[] = {
            L"TICKET", L"CLIENT", L"ACTION", L"PRIORITY", L"STATUS", L"DUE", L"ASSIGNED TO", L"CATEGORY" };
        const int taskWidths[] = { 65,120,260,88,95,100,130,105 };
        for (int i = 0; i < 8; ++i) {
            LVCOLUMNW column = {};
            column.mask = LVCF_TEXT | LVCF_WIDTH;
            column.pszText = const_cast<LPWSTR>(taskNames[i]);
            column.cx = taskWidths[i];
            for (HWND lane : { tasksList, mediumTasksList, completedTasksList }) ListView_InsertColumn(lane, i, &column);
        }

        const wchar_t* goalNames[] = {
            L"CLIENT ID", L"CLIENT", L"GOAL", L"TARGET", L"CURRENT", L"FUNDED", L"TARGET DATE" };
        const int goalWidths[] = { 85,150,235,130,130,95,130 };
        for (int i = 0; i < 7; ++i) {
            LVCOLUMNW column = {};
            column.mask = LVCF_TEXT | LVCF_WIDTH;
            column.pszText = const_cast<LPWSTR>(goalNames[i]);
            column.cx = goalWidths[i];
            ListView_InsertColumn(goalsList, i, &column);
        }

        ListView_SetExtendedListViewStyle(
            clientList,

            LVS_EX_FULLROWSELECT |
            LVS_EX_DOUBLEBUFFER |
            LVS_EX_GRIDLINES);


        ListView_SetExtendedListViewStyle(
            holdingsList,

            LVS_EX_FULLROWSELECT |
            LVS_EX_DOUBLEBUFFER |
            LVS_EX_GRIDLINES);


        const wchar_t*
            clientColumns[] = {

            L"ID",

            L"CLIENT",

            L"HOUSEHOLD",

            L"ADVISOR",

            L"RISK",

            L"PORTFOLIO VALUE",

            L"STATUS"
        };


        const int clientWidths[] = {

            55,

            175,

            175,

            145,

            110,

            130,

            85
        };


        for (
            int index = 0;
            index <
            7;
            ++index) {

            LVCOLUMNW column = {};

            column.mask =
                LVCF_TEXT |
                LVCF_WIDTH;

            column.pszText =
                const_cast<
                LPWSTR>(
                    clientColumns[
                        index]);

            column.cx =
                clientWidths[
                    index];

            ListView_InsertColumn(
                clientList,
                index,
                &column);
        }


        const wchar_t*
            holdingColumns[] = {

            L"SYMBOL",

            L"SECURITY",

            L"ASSET CLASS",

            L"QTY",

            L"VALUE",

            L"GAIN / LOSS",

            L"WEIGHT"
        };


        const int holdingWidths[] = {

            80,

            225,

            135,

            90,

            130,

            120,

            90
        };


        for (
            int index = 0;
            index <
            7;
            ++index) {

            LVCOLUMNW column = {};

            column.mask =
                LVCF_TEXT |
                LVCF_WIDTH;

            column.pszText =
                const_cast<
                LPWSTR>(
                    holdingColumns[
                        index]);

            column.cx =
                holdingWidths[
                    index];

            ListView_InsertColumn(
                holdingsList,
                index,
                &column);
        }


        assignFont(
            usernameField,
            11);

        assignFont(
            passwordField,
            11);

        assignFont(
            searchField,
            11);

        assignFont(
            clientList,
            10);

        assignFont(
            holdingsList,
            10);
        for (HWND lane : { tasksList, mediumTasksList, completedTasksList }) assignFont(lane, 10);
        assignFont(goalsList, 10);
    }


    // ========================================================
    // LAYOUT
    // ========================================================

    void updateLayout() {

        if (!window) {

            return;
        }


        RECT clientRectangle = {};


        GetClientRect(
            window,
            &clientRectangle);


        width =
            clientRectangle.right;


        height =
            clientRectangle.bottom;


        ShowWindow(
            usernameField,
            SW_HIDE);


        ShowWindow(
            passwordField,
            SW_HIDE);


        ShowWindow(
            searchField,
            SW_HIDE);


        ShowWindow(
            clientList,
            SW_HIDE);


        ShowWindow(
            holdingsList,
            SW_HIDE);
        for (HWND lane : { tasksList, mediumTasksList, completedTasksList }) ShowWindow(lane, SW_HIDE);
        ShowWindow(goalsList, SW_HIDE);


        if (
            currentPage ==
            Page::Login) {

            const int centerX =
                width /
                2;


            MoveWindow(
                usernameField,
                centerX -
                165,
                height /
                2 -
                33,
                330,
                35,
                TRUE);


            MoveWindow(
                passwordField,
                centerX -
                165,
                height /
                2 +
                35,
                330,
                35,
                TRUE);


            ShowWindow(
                usernameField,
                SW_SHOW);


            ShowWindow(
                passwordField,
                SW_SHOW);
        }


        if (
            currentPage ==
            Page::Clients) {

            MoveWindow(
                searchField,
                252,
                150,
                380,
                35,
                TRUE);


            MoveWindow(
                clientList,
                252,
                205,
                std::max(
                    600,
                    width -
                    285),
                std::max(
                    280,
                    height -
                    300),
                TRUE);


            ShowWindow(
                searchField,
                SW_SHOW);


            ShowWindow(
                clientList,
                SW_SHOW);


            refreshClientList();
        }


        if (
            currentPage ==
            Page::ClientWorkspace) {

            MoveWindow(
                holdingsList,
                252,
                585,
                std::max(
                    650,
                    width -
                    285),
                std::max(
                    130,
                    height -
                    670),
                TRUE);


            ShowWindow(
                holdingsList,
                SW_SHOW);


            refreshHoldingsList();
        }


        if (currentPage == Page::AdvisorTasks) {
            int laneWidth = std::max(200, (width - 309) / 3), laneIndex = 0;
            for (HWND lane : { tasksList, mediumTasksList, completedTasksList }) {
                MoveWindow(lane, 252 + laneIndex++ * (laneWidth + 12), 207, laneWidth, std::max(238, height - 322), TRUE);
                ShowWindow(lane, SW_SHOW);
            }
            refreshTasksList();
        }

        if (currentPage == Page::FinancialGoals) {
            MoveWindow(goalsList, 252, 185,
                std::max(600, width - 285),
                std::max(260, height - 300), TRUE);
            ShowWindow(goalsList, SW_SHOW);
            refreshGoalsList();
        }

        InvalidateRect(window, nullptr, TRUE);
    }

    void navigate(Page requested) {
        const bool animate = authenticated &&
            requested != currentPage &&
            requested != Page::Welcome &&
            requested != Page::Login &&
            requested != Page::Loading;
        if (animate) {
            loadingDestination = requested;
            loadingFrame = 0;
            currentPage = Page::Loading;
            SetTimer(window, 27, 85, nullptr);
        }
        else {
            currentPage = requested;
        }
        updateLayout();
    }


    // ========================================================
    // LOGIN
    // ========================================================

    void login() {

        const std::wstring username =
            controlText(
                usernameField);


        const std::wstring password =
            controlText(
                passwordField);


        const std::wstring
            configuredUsername =
            environmentValue(
                L"CS499_USERNAME");


        const std::wstring
            configuredPassword =
            environmentValue(
                L"CS499_PASSWORD");


        if (
            configuredUsername.empty() ||
            configuredPassword.empty()) {

            showMessage(
                window,

                L"Authentication is not configured.\n\n"
                L"Project Properties > Debugging > Environment\n\n"
                L"CS499_USERNAME=demo_admin\n"
                L"CS499_PASSWORD=AureliaDemo2026");

            return;
        }


        if (
            username !=
            configuredUsername ||
            password !=
            configuredPassword) {

            failedLoginAttempts++;


            logger.write(
                "GUI_AUTH_FAILURE");


            showMessage(
                window,
                L"Username or password "
                L"was not recognized.");


            if (
                failedLoginAttempts >=
                3) {

                EnableWindow(
                    usernameField,
                    FALSE);

                EnableWindow(
                    passwordField,
                    FALSE);
            }


            return;
        }


        authenticated =
            true;


        failedLoginAttempts =
            0;


        SetWindowTextW(
            passwordField,
            L"");


        logger.write(
            "GUI_AUTH_SUCCESS");


        navigate(
            Page::Dashboard);
    }


    std::vector<std::string> clientChoices() {
        std::vector<std::string> out;for(const auto& c:repository.getAllClients())out.push_back(std::to_string(c.id)+" | "+c.name);return out;
    }
    std::string chosenClientLabel() {
        auto c=repository.findClientById(selectedWorkspaceClientId);if(c)return std::to_string(c->id)+" | "+c->name;auto list=clientChoices();return list.empty()?"":list.front();
    }
    void editClientProfile() {
        auto found=repository.findClientById(selectedWorkspaceClientId);if(!found)throw std::runtime_error("Open a client workspace first.");Client c=*found;PlanningProfile p=repository.getPlanningProfile(c.id);
        std::vector<AureliaFormField> f={
            {L"Client name",c.name},{L"Email",c.email},{L"Phone",c.phone},{L"Age",std::to_string(c.age)},
            {L"Household",c.householdName},{L"Assigned advisor",c.advisorName},{L"Objective",c.investmentObjective},
            {L"Annual income (USD)",formNumber(c.annualIncome)},{L"Net worth (USD)",formNumber(c.netWorth)},
            {L"Liquidity need",c.liquidityNeed,{"Low","Moderate","High"}},{L"Time horizon (years)",std::to_string(c.timeHorizonYears)},
            {L"Risk profile",riskLevelToString(c.riskLevel),{"Conservative","Moderate","Aggressive"}},
            {L"Occupation",p.occupation},{L"Employer / business",p.employer},{L"Location",p.location},
            {L"Preferred contact",p.contactPreference,{"Email","Phone","Video meeting"}},{L"Dependents",std::to_string(p.dependents)},
            {L"Retirement age",std::to_string(p.retirementAge)},{L"Annual expenses (USD)",formNumber(p.annualExpenses)},
            {L"Emergency reserve (USD)",formNumber(p.emergencyReserve)},{L"Monthly savings (USD)",formNumber(p.monthlySaving)},
            {L"Next review (YYYY-MM-DD)",p.reviewDate},{L"Planning context",p.planningContext,{},true}};
        runAureliaForm(window,L"Client profile and planning",f,[&](const std::vector<std::string>& v){
            c.name=v[0];c.email=v[1];c.phone=v[2];c.age=formInteger(v[3]);c.householdName=v[4];c.advisorName=v[5];c.investmentObjective=v[6];c.annualIncome=formAmount(v[7]);c.netWorth=formAmount(v[8]);c.liquidityNeed=v[9];c.timeHorizonYears=formInteger(v[10]);c.riskLevel=v[11]=="Conservative"?RiskLevel::Conservative:(v[11]=="Moderate"?RiskLevel::Moderate:RiskLevel::Aggressive);p.occupation=v[12];p.employer=v[13];p.location=v[14];p.contactPreference=v[15];p.dependents=formInteger(v[16]);p.retirementAge=formInteger(v[17]);p.annualExpenses=formAmount(v[18]);p.emergencyReserve=formAmount(v[19]);p.monthlySaving=formAmount(v[20]);p.reviewDate=v[21];p.planningContext=v[22];repository.savePlanningProfile(c,p);});updateLayout();
    }
    void editTicket(bool edit) {
        AdvisorTask t;auto choices=clientChoices();std::string label=chosenClientLabel();
        if(edit){int i=selectedTaskIndex();if(i<0||i>=(int)displayedTasks.size())throw std::runtime_error("Select a ticket first.");t=displayedTasks[i];if(t.clientId){auto c=repository.findClientById(*t.clientId);if(c)label=std::to_string(c->id)+" | "+c->name;}}
        std::vector<AureliaFormField> f={{L"Client",label,choices},{L"Ticket title",t.title},{L"Action / expected outcome",t.description,{},true},
            {L"Category",t.category.empty()?"Planning":t.category,{"Planning","Portfolio review","Client service","Documentation","Cash flow","Goal review"}},
            {L"Due date (YYYY-MM-DD)",t.dueDate},{L"Priority",t.priority.empty()?"Medium":t.priority,{"Urgent","High","Medium","Low"}},
            {L"Assigned advisor",t.assignedTo.empty()?"Brianna Reed":t.assignedTo},{L"Status",t.status,{"Open","In progress","Waiting","Completed"}}};
        runAureliaForm(window,edit?L"Update advisor ticket":L"Create advisor ticket",f,[&](const std::vector<std::string>& v){repository.saveTask(t.id,std::stoi(v[0]),v[1],v[2],v[3],v[4],v[5],v[6],v[7]);});updateLayout();
    }
    void editFinancialGoal(bool edit) {
        FinancialGoal g;std::string label=chosenClientLabel();if(edit){int i=selectedGoalIndex();if(i<0||i>=(int)displayedGoals.size())throw std::runtime_error("Select a goal first.");g=displayedGoals[i];auto c=repository.findClientById(g.clientId);if(c)label=std::to_string(c->id)+" | "+c->name;}
        std::vector<AureliaFormField> f={{L"Client",label,clientChoices()},{L"Goal name",g.name},{L"Goal type",g.type.empty()?"Retirement":g.type,{"Retirement","Emergency reserve","Education","Home purchase","Lifestyle","Legacy","Other"}},
            {L"Target amount (USD)",formNumber(g.targetAmount)},{L"Current funding (USD)",formNumber(g.currentAmount)},{L"Target date (YYYY-MM-DD)",g.targetDate},{L"Priority",g.priority==1?"High":(g.priority==3?"Low":"Medium"),{"High","Medium","Low"}}};
        runAureliaForm(window,edit?L"Update financial goal":L"Create financial goal",f,[&](const std::vector<std::string>& v){repository.saveGoal(g.id,std::stoi(v[0]),v[1],v[2],formAmount(v[3]),formAmount(v[4]),v[5],v[6]=="High"?1:(v[6]=="Low"?3:2));});updateLayout();
    }
    void addAdvisorNote() {
        runAureliaForm(window,L"Add client planning note",{{L"Client",chosenClientLabel(),clientChoices()},{L"Meeting / next steps / context","",{},true}},[&](const std::vector<std::string>& v){repository.addPlanningNote(std::stoi(v[0]),v[1]);});updateLayout();
    }
    void addAccount() {
        runAureliaForm(window,L"Create demonstration account",{{L"Client",chosenClientLabel(),clientChoices()},{L"Account name",""},{L"Account type","Individual brokerage",{"Individual brokerage","Joint brokerage","Traditional IRA","Roth IRA","Education account"}},{L"Tax treatment","Taxable",{"Taxable","Tax deferred","Tax exempt"}}},[&](const std::vector<std::string>& v){repository.addClientAccount(std::stoi(v[0]),v[1],v[2],v[3]);});updateLayout();
    }
    void recordHolding() {
        if(selectedWorkspaceClientId<0)throw std::runtime_error("Open a client workspace first.");std::vector<std::string> accounts,symbols;for(const auto& a:repository.getAccountsForClient(selectedWorkspaceClientId))accounts.push_back(std::to_string(a.id)+" | "+a.accountName);Statement q(database.handle(),"SELECT symbol FROM securities ORDER BY symbol;");while(q.step()==SQLITE_ROW)symbols.push_back(q.columnText(0));if(accounts.empty())throw std::runtime_error("Create an account first.");
        runAureliaForm(window,L"Set stored holding position",{{L"Client account",accounts.front(),accounts},{L"Security",symbols.front(),symbols},{L"Total position quantity","0"},{L"Average cost per share (USD)","0"}},[&](const std::vector<std::string>& v){repository.saveAccountHolding(selectedWorkspaceClientId,std::stoi(v[0]),v[1],formAmount(v[2]),formAmount(v[3]));});updateLayout();
    }
    void paintClientDetails(Graphics& graphics,int x,int w) {
        if(selectedWorkspaceClientId<0)return;auto d=analytics.clientDashboard(selectedWorkspaceClientId);auto p=repository.getPlanningProfile(selectedWorkspaceClientId);int cw=(w-36)/4;
        drawPlanningLabel(graphics,widen(d.client.name+" / "+d.client.householdName),x,110,w,35,23,COLOR_TEXT,true);
        drawMetricCard(graphics,x,155,cw,L"ANNUAL INCOME",moneyText(d.client.annualIncome),L"Recorded household income");
        drawMetricCard(graphics,x+cw+12,155,cw,L"NET WORTH",moneyText(d.client.netWorth),L"Recorded assets less liabilities");
        double months=p.annualExpenses>0?p.emergencyReserve/(p.annualExpenses/12):0;
        drawMetricCard(graphics,x+2*(cw+12),155,cw,L"CASH RUNWAY",widen(formNumber(months)+" months"),L"Reserve / monthly expenses");
        drawMetricCard(graphics,x+3*(cw+12),155,cw,L"MONTHLY SAVING",moneyText(p.monthlySaving),L"Recorded contribution capacity");
        int half=(w-16)/2;drawPlanningBox(graphics,x,285,half,198,14,COLOR_WHITE,COLOR_LINE);drawPlanningBox(graphics,x+half+16,285,half,198,14,COLOR_WHITE,COLOR_LINE);
        drawPlanningLabel(graphics,L"Relationship essentials",x+20,296,half-40,30,18,COLOR_TEXT,true);
        std::vector<std::string> lines={d.client.email+" | "+d.client.phone,p.occupation+" | "+p.employer,p.location+" | Contact: "+p.contactPreference,"Advisor: "+d.client.advisorName+" | Next review: "+p.reviewDate,"Age "+std::to_string(d.client.age)+" | Dependents "+std::to_string(p.dependents)+" | Retirement age "+std::to_string(p.retirementAge)};
        for(size_t i=0;i<lines.size();++i)drawPlanningLabel(graphics,widen(lines[i]),x+20,333+static_cast<int>(i)*26,half-40,25,11,COLOR_MUTED);
        drawPlanningLabel(graphics,L"Goal funding",x+half+36,296,half-40,30,18,COLOR_TEXT,true);int gy=337;
        for(size_t i=0;i<d.goals.size()&&i<3;++i){auto g=d.goals[i];double pct=g.targetAmount>0?g.currentAmount/g.targetAmount:0;drawPlanningLabel(graphics,widen(g.name+" / "+formNumber(pct*100)+"%"),x+half+36,gy,half-40,22,11,COLOR_TEXT,true);drawPlanningBox(graphics,x+half+36,gy+24,half-40,7,3,COLOR_LINE,Color(0,0,0,0));if(pct>0)drawPlanningBox(graphics,x+half+36,gy+24,(half-40)*std::min(pct,1.0),7,3,COLOR_SAGE,Color(0,0,0,0));gy+=43;}
        drawPlanningBox(graphics,x,500,w,185,14,COLOR_SOFT_GOLD,COLOR_LINE);drawPlanningLabel(graphics,L"Planning context & practical next steps",x+20,511,w-40,29,18,COLOR_TEXT,true);
        drawPlanningLabel(graphics,widen(d.client.investmentObjective+" | "+p.planningContext),x+20,548,w-40,30,12,COLOR_MUTED);
        int ty=585;for(size_t i=0;i<d.tasks.size()&&i<3;++i){const auto& t=d.tasks[i];drawPlanningLabel(graphics,widen("AW-"+std::to_string(t.id)+"  ["+t.priority+"] "+t.title+" | "+t.assignedTo+" | Due "+t.dueDate),x+20,ty,w-40,25,11,t.priority=="High"||t.priority=="Urgent"?COLOR_TERRACOTTA:COLOR_TEXT);ty+=27;}
        drawButton(graphics,L"Portfolio",x,height-80,110,38,ACTION_PROFILE_PORTFOLIO,false);drawButton(graphics,L"Edit profile",x+121,height-80,115,38,ACTION_EDIT_PROFILE,true);drawButton(graphics,L"New ticket",x+247,height-80,110,38,ACTION_NEW_TASK,false);drawButton(graphics,L"New goal",x+368,height-80,105,38,ACTION_NEW_GOAL,false);drawButton(graphics,L"Add note",x+484,height-80,100,38,ACTION_ADD_NOTE,false);drawButton(graphics,L"Add account",x+595,height-80,125,38,ACTION_ADD_ACCOUNT,false);drawButton(graphics,L"Set holding",x+731,height-80,110,38,ACTION_SET_HOLDING,false);
    }

    void enterMarketObservation() {
        std::vector<std::string> symbols;Statement q(database.handle(),"SELECT symbol FROM securities WHERE symbol<>'CASH' ORDER BY symbol;");while(q.step()==SQLITE_ROW)symbols.push_back(q.columnText(0));
        runAureliaForm(window,L"Record a sourced market observation",{{L"Security",symbols.front(),symbols},{L"Observed market price (USD)",""},{L"Source date (YYYY-MM-DD)",""},{L"HTTPS source URL","https://"}},[&](const std::vector<std::string>& v){repository.storeMarketObservation(v[0],formAmount(v[1]),v[2],v[3]);});updateLayout();
    }
    void refreshMarketObservation() {
        if(marketRefresh.valid()){marketStatus=L"A market request is already running.";return;}
        std::string key=narrow(environmentValue(L"ALPHAVANTAGE_API_KEY"));if(key.empty()){marketStatus=L"Offline: set ALPHAVANTAGE_API_KEY locally to connect a provider. Dated public observations remain available.";InvalidateRect(window,nullptr,TRUE);return;}
        std::vector<std::string> symbols;Statement q(database.handle(),"SELECT symbol FROM securities WHERE symbol<>'CASH' ORDER BY symbol;");while(q.step()==SQLITE_ROW)symbols.push_back(q.columnText(0));std::string selected;
        bool accepted=runAureliaForm(window,L"Refresh one market observation",{{L"Security to refresh",symbols.front(),symbols}},[&](const std::vector<std::string>& v){selected=v[0];});if(!accepted)return;
        marketStatus=L"Contacting market provider... portfolio records remain available.";marketRefresh=std::async(std::launch::async,[selected,key]{return fetchMarketObservation(selected,key);});SetTimer(window,28,200,nullptr);InvalidateRect(window,nullptr,TRUE);
    }
    void pollMarketObservation() {
        if(!marketRefresh.valid()||marketRefresh.wait_for(std::chrono::milliseconds(0))!=std::future_status::ready)return;KillTimer(window,28);
        try{auto o=marketRefresh.get();repository.storeMarketObservation(o.symbol,o.price,o.asOfDate,o.source);marketStatus=widen(o.symbol+" observation refreshed. Provider market date: "+o.asOfDate+". Not a real-time price guarantee.");}catch(const std::exception& e){marketStatus=widen(e.what());}InvalidateRect(window,nullptr,TRUE);
    }
    void paintMarketData(Graphics& graphics,int x,int w) {
        drawPlanningLabel(graphics,L"Dated source observations / demonstration valuations stay separate",x,117,w,32,17,COLOR_TEXT,true);
        drawPlanningBox(graphics,x,165,w,110,14,COLOR_SOFT_GOLD,COLOR_LINE);
        drawPlanningLabel(graphics,marketStatus,x+20,177,w-40,38,12,COLOR_MUTED);
        drawPlanningLabel(graphics,L"Dashboard totals use stored fictional prices. Source observations are reference data with an explicit market date.",x+20,222,w-40,32,11,COLOR_MUTED);
        drawPlanningBox(graphics,x,292,w,390,14,COLOR_WHITE,COLOR_LINE);
        drawPlanningLabel(graphics,L"SECURITY",x+18,305,110,30,10,COLOR_TEXT,true);drawPlanningLabel(graphics,L"DEMO PRICE",x+138,305,125,30,10,COLOR_TEXT,true);drawPlanningLabel(graphics,L"SOURCED PRICE",x+277,305,130,30,10,COLOR_TEXT,true);drawPlanningLabel(graphics,L"MARKET DATE",x+420,305,125,30,10,COLOR_TEXT,true);drawPlanningLabel(graphics,L"SOURCE / COVERAGE",x+558,305,w-577,30,10,COLOR_TEXT,true);
        Statement q(database.handle(),"SELECT s.symbol,s.mock_price,q.price,q.as_of_date,q.source FROM securities s LEFT JOIN market_quotes q ON q.symbol=s.symbol ORDER BY s.symbol;");int y=342;while(q.step()==SQLITE_ROW){drawPlanningLabel(graphics,widen(q.columnText(0)),x+18,y,110,26,12,COLOR_TEXT,true);drawPlanningLabel(graphics,widen("$"+formNumber(q.columnDouble(1))),x+138,y,125,26,12,COLOR_MUTED);drawPlanningLabel(graphics,q.columnIsNull(2)?L"Not sourced":widen("$"+formNumber(q.columnDouble(2))),x+277,y,130,26,12,COLOR_TEXT);drawPlanningLabel(graphics,widen(q.columnText(3)),x+420,y,125,26,11,COLOR_MUTED);drawPlanningLabel(graphics,q.columnIsNull(2)?L"Offline demonstration price only":widen(q.columnText(4)),x+558,y,w-577,26,10,COLOR_MUTED);y+=31;}
        drawButton(graphics,L"Record observation",x,height-80,175,39,ACTION_RECORD_QUOTE,true);drawButton(graphics,L"Refresh from provider",x+188,height-80,200,39,ACTION_REFRESH_QUOTE,false);drawButton(graphics,L"Back to dashboard",x+401,height-80,180,39,ACTION_DASHBOARD,false);
    }

    // The dashboard metrics and panels navigate to their underlying records
    void paintOperationalRegister(Graphics& graphics,
        int x, int w, bool taskPage) {
        drawLabel(graphics,
            taskPage ? L"Click headers to sort | Scroll across for all fields | Hover for full ticket details"
            : L"Stored financial goals | Select a row for client context",
            x, 121, w, 40, 13, COLOR_MUTED);
        drawButton(graphics, taskPage?L"Create ticket":L"Create goal",x+w-145,121,145,37,taskPage?ACTION_NEW_TASK:ACTION_NEW_GOAL,true);
        if (taskPage) {
            int laneWidth = std::max(200, (width - 309) / 3);
            const wchar_t* names[] = { L"HIGH / URGENT", L"MEDIUM / LOW", L"COMPLETED" };
            HWND lanes[] = { tasksList, mediumTasksList, completedTasksList };
            for (int i = 0; i < 3; ++i) drawPlanningLabel(graphics, std::wstring(names[i]) + L"  (" + std::to_wstring(ListView_GetItemCount(lanes[i])) + L")", 252 + i * (laneWidth + 12), 175, laneWidth, 26, 11, COLOR_TEXT, true);
            int selected = selectedTaskIndex();
            if (selected >= 0 && selected < static_cast<int>(displayedTasks.size())) {
                const auto& t = displayedTasks[selected];
                drawPlanningLabel(graphics, widen("AW-" + std::to_string(t.id) + " | " + t.title + " | " + t.priority + " | " + t.status + " | Due " + t.dueDate), x, height - 112, w, 26, 10, COLOR_TEXT);
            }
            drawButton(graphics, L"Open selected client", x,
                height - 80, 195, 39, ACTION_TASK_CLIENT, true);
            drawButton(graphics,L"Edit ticket",x+418,height-80,140,39,ACTION_EDIT_TASK,false);
            drawButton(graphics, L"Complete selected task", x + 209,
                height - 80, 195, 39, ACTION_TASK_COMPLETE, false);
        }
        else {
            drawButton(graphics, L"Open goal client", x,
                height - 80, 195, 39, ACTION_GOAL_CLIENT, true);
            drawButton(graphics,L"Edit goal",x+209,height-80,140,39,ACTION_EDIT_GOAL,false);
        }
    }

    void paintAllocationDetail(Graphics& graphics, int x, int w) {
        drawRoundedBox(graphics, x, 139, w, 390, 14,
            COLOR_WHITE, COLOR_LINE);
        drawLabel(graphics, L"Asset-class exposure from persisted holdings",
            x + 20, 152, w - 40, 39, 19, COLOR_TEXT, true);
        drawAllocationBars(graphics, x + 29, 217, w - 75,
            analytics.enterpriseAllocation(), L"Share of administered assets");
        drawLabel(graphics,
            L"Includes only fictional stored positions and modeled prices. "
            L"No security-level risk or suitability verification is implied.",
            x + 22, 484, w - 44, 31, 11, COLOR_MUTED);
        drawButton(graphics, L"Review client profiles", x, 558,
            206, 42, ACTION_CLIENTS, true);
    }

    void paintHistoryDetail(Graphics& graphics, int x, int w) {
        drawRoundedBox(graphics, x, 139, w, 416, 14,
            COLOR_WHITE, COLOR_LINE);
        drawLabel(graphics, L"Aggregated SQLite portfolio snapshots",
            x + 20, 151, w - 40, 43, 20, COLOR_TEXT, true);
        drawHistoryChart(graphics, x + 30, 215, w - 68, 286,
            analytics.enterpriseHistory(), L"Monthly modeled assets");
        drawLabel(graphics,
            L"Change in stored snapshot value is not a time-weighted return. "
            L"Cash flows and modeling assumptions affect comparisons.",
            x + 22, 508, w - 44, 30, 11, COLOR_MUTED);
        drawButton(graphics, L"Export detailed workbook", x, 573,
            240, 42, ACTION_EXPORT_ENTERPRISE_XLSX, true);
    }

    void paintLoading(Graphics& graphics) {
        graphics.Clear(COLOR_CREAM);
        drawWealthEmblem(graphics, (REAL)width / 2 - 30,
            (REAL)height / 2 - 120, 60);
        drawLabel(graphics, L"Preparing your workspace",
            width / 2 - 260, height / 2 - 12, 520, 46, 27, COLOR_TEXT,
            true, StringAlignmentCenter);
        drawLabel(graphics, L"Reading persisted relational data",
            width / 2 - 250, height / 2 + 41, 500, 30, 13, COLOR_MUTED,
            false, StringAlignmentCenter);
        for (int i = 0;i < 3;++i) {
            const Color color = (i + loadingFrame) % 3 == 0
                ? COLOR_GOLD : ((i + loadingFrame) % 3 == 1
                    ? COLOR_SAGE : COLOR_TERRACOTTA);
            SolidBrush dot(color);
            graphics.FillEllipse(&dot,
                (REAL)(width / 2 - 30 + i * 24), (REAL)(height / 2 + 98), 9.0f, 9.0f);
        }
    }

    // ========================================================
    // CLIENT LIST
    // ========================================================

    void refreshClientList() {

        if (!authenticated) {

            return;
        }


        const int previouslySelectedClient = selectedClientId();
        const std::string search =
            narrow(
                controlText(
                    searchField));


        displayedClients =
            search.empty()
            ? repository
            .getAllClients()
            : repository
            .searchNamePrefix(
                search);


        std::vector<std::pair<int, double>> clientValues;
        for (const auto& c : displayedClients) clientValues.emplace_back(c.id, analytics.clientDashboard(c.id).currentValue);
        auto portfolioValue = [&](int id) { for (const auto& v : clientValues) if (v.first == id) return v.second; return 0.0; };
        std::stable_sort(displayedClients.begin(), displayedClients.end(), [&](const Client& a, const Client& b) {
            int result = 0;
            if (clientSortColumn == 0) result = (a.id > b.id) - (a.id < b.id);
            else if (clientSortColumn == 5) { double av = portfolioValue(a.id), bv = portfolioValue(b.id); result = (av > bv) - (av < bv); }
            else {
                auto value = [&](const Client& c) { switch(clientSortColumn) { case 1: return c.name; case 2: return c.householdName; case 3: return c.advisorName; case 4: return riskLevelToString(c.riskLevel); default: return std::string(c.active ? "Active" : "Inactive"); } };
                result = _stricmp(value(a).c_str(), value(b).c_str());
            }
            return clientSortAscending ? result < 0 : result > 0;
        });
        markSort(clientList, clientSortColumn, clientSortAscending);
        ListView_DeleteAllItems(
            clientList);


        for (
            size_t row = 0;
            row <
            displayedClients.size();
            ++row) {

            const Client& client =
                displayedClients[
                    row];


            std::wstring values[] = {

                std::to_wstring(
                    client.id),

                widen(
                    client.name),

                widen(
                    client.householdName),

                widen(
                    client.advisorName),

                widen(
                    riskLevelToString(
                        client.riskLevel)),

                moneyText(
                    portfolioValue(client.id)),

                client.active
                ? L"Active"
                : L"Inactive"
            };


            LVITEMW item = {};

            item.mask =
                LVIF_TEXT;

            item.iItem =
                static_cast<int>(
                    row);

            item.pszText =
                values[
                    0]
                .data();


            const int inserted =
                ListView_InsertItem(
                    clientList,
                    &item);


            if (client.id == previouslySelectedClient) ListView_SetItemState(clientList, inserted, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

            for (
                int column = 1;
                column <
                7;
                ++column) {

                ListView_SetItemText(
                    clientList,
                    inserted,
                    column,
                    values[
                        column]
                    .data());
            }
        }
    }


    int selectedClientId() const {

        const int row =
            ListView_GetNextItem(
                clientList,
                -1,
                LVNI_SELECTED);


        if (
            row <
            0 ||
            static_cast<size_t>(
                row) >=
            displayedClients.size()) {

            return -1;
        }


        return displayedClients[
            row]
            .id;
    }


    void refreshHoldingsList() {

        ListView_DeleteAllItems(
            holdingsList);


        if (
            selectedWorkspaceClientId <
            0) {

            return;
        }


        const ClientDashboard dashboard =
            analytics.clientDashboard(
                selectedWorkspaceClientId);


        for (
            size_t row = 0;
            row <
            dashboard
            .topHoldings
            .size();
            ++row) {

            const Holding& holding =
                dashboard
                .topHoldings[
                    row];


            const double weight =
                dashboard.currentValue >
                0.0
                ? holding.marketValue /
                dashboard.currentValue *
                100.0
                : 0.0;


            std::wostringstream quantity;


            quantity
                << std::fixed
                << std::setprecision(
                    2)
                << holding.quantity;


            std::wostringstream weightText;


            weightText
                << std::fixed
                << std::setprecision(
                    1)
                << weight
                << L"%";


            std::wstring values[] = {

                widen(
                    holding.symbol),

                widen(
                    holding.securityName),

                widen(
                    holding.assetClass),

                quantity.str(),

                moneyText(
                    holding.marketValue),

                moneyText(
                    holding.unrealizedGain),

                weightText.str()
            };


            LVITEMW item = {};

            item.mask =
                LVIF_TEXT;

            item.iItem =
                static_cast<int>(
                    row);

            item.pszText =
                values[
                    0]
                .data();


            const int inserted =
                ListView_InsertItem(
                    holdingsList,
                    &item);


            for (
                int column = 1;
                column <
                7;
                ++column) {

                ListView_SetItemText(
                    holdingsList,
                    inserted,
                    column,
                    values[
                        column]
                    .data());
            }
        }
    }


    // ========================================================
    // OPERATIONAL REGISTERS. ALL ROWS COME FROM SQLITE
    // ========================================================

    void fillListRow(HWND list, int index,
        const std::vector<std::wstring>& columns) {
        if (columns.empty()) return;
        LVITEMW item = {};
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.pszText = const_cast<LPWSTR>(columns[0].c_str());
        const int inserted = ListView_InsertItem(list, &item);
        for (size_t j = 1; j < columns.size(); ++j)
            ListView_SetItemText(list, inserted, (int)j,
                const_cast<LPWSTR>(columns[j].c_str()));
    }

    void refreshTasksList() {
        if (!authenticated || !tasksList) return;
        int oldIndex = selectedTaskIndex();
        int selectedId = oldIndex >= 0 && oldIndex < static_cast<int>(displayedTasks.size()) ? displayedTasks[oldIndex].id : -1;
        displayedTasks = repository.getOpenTasks(std::nullopt, true);
        auto clientName = [&](const AdvisorTask& t) { auto c = t.clientId ? repository.findClientById(*t.clientId) : std::optional<Client>{}; return c ? c->name : std::string("Firm wide"); };
        auto rank = [](const std::string& p) { return p == "Urgent" ? 0 : p == "High" ? 1 : p == "Medium" ? 2 : 3; };
        std::stable_sort(displayedTasks.begin(), displayedTasks.end(), [&](const AdvisorTask& a, const AdvisorTask& b) {
            int result = 0;
            if (taskSortColumn == 0) result = (a.id > b.id) - (a.id < b.id);
            else if (taskSortColumn == 3) result = rank(a.priority) - rank(b.priority);
            else {
                auto value = [&](const AdvisorTask& t) { switch(taskSortColumn) { case 1: return clientName(t); case 2: return t.title; case 4: return t.status; case 5: return t.dueDate; case 6: return t.assignedTo; default: return t.category; } };
                result = _stricmp(value(a).c_str(), value(b).c_str());
            }
            return taskSortAscending ? result < 0 : result > 0;
        });
        for (HWND lane : { tasksList, mediumTasksList, completedTasksList }) ListView_DeleteAllItems(lane);
        for (size_t i = 0; i < displayedTasks.size(); ++i) {
            const auto& t = displayedTasks[i];
            HWND lane = t.completed ? completedTasksList : (t.priority == "High" || t.priority == "Urgent" ? tasksList : mediumTasksList);
            int row = ListView_GetItemCount(lane);
            fillListRow(lane, row, { widen("AW-" + std::to_string(t.id)), widen(clientName(t)), widen(t.title), widen(t.priority), widen(t.status), widen(t.dueDate), widen(t.assignedTo), widen(t.category) });
            LVITEMW item{}; item.mask = LVIF_PARAM; item.iItem = row; item.lParam = static_cast<LPARAM>(i); ListView_SetItem(lane, &item);
            if (t.id == selectedId) ListView_SetItemState(lane, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        }
        for (HWND lane : { tasksList, mediumTasksList, completedTasksList }) markSort(lane, taskSortColumn, taskSortAscending);
        InvalidateRect(window, nullptr, FALSE);
    }

    void refreshGoalsList() {
        if (!authenticated || !goalsList) return;
        displayedGoals.clear();
        for (const auto& c : repository.getAllClients()) {
            auto goals = repository.getGoalsForClient(c.id);
            displayedGoals.insert(displayedGoals.end(),
                goals.begin(), goals.end());
        }
        ListView_DeleteAllItems(goalsList);
        for (size_t i = 0; i < displayedGoals.size(); ++i) {
            const auto& goal = displayedGoals[i];
            auto client = repository.findClientById(goal.clientId);
            const double progress = goal.targetAmount > 0 ?
                100.0 * goal.currentAmount / goal.targetAmount : 0;
            std::wostringstream p;
            p << std::fixed << std::setprecision(1) << progress << L"%";
            fillListRow(goalsList, (int)i, {
                std::to_wstring(goal.clientId),
                client ? widen(client->name) : L"Unknown",
                widen(goal.name), moneyText(goal.targetAmount),
                moneyText(goal.currentAmount), p.str(),
                widen(goal.targetDate) });
        }
    }

    int selectedTaskIndex() const {
        for (HWND lane : { tasksList, mediumTasksList, completedTasksList }) {
            int row = ListView_GetNextItem(lane, -1, LVNI_SELECTED);
            if (row >= 0) return taskRowIndex(lane, row);
        }
        return -1;
    }

    int selectedGoalIndex() const {
        return ListView_GetNextItem(goalsList, -1, LVNI_SELECTED);
    }

    int demoDuplicateCount() const {
        Statement q(database.handle(),
            "SELECT COUNT(*) FROM clients d JOIN clients p "
            "ON d.id<>p.id "
            "AND lower(d.name)=lower(p.name) "
            "AND d.age=p.age "
            "AND abs(d.account_balance-p.account_balance)<0.01 "
            "WHERE d.email LIKE 'demo.investor%@aurelia.example' "
            "AND p.email LIKE '%@example.com' "
            "AND p.email NOT LIKE 'demo.investor%@aurelia.example';");
        return q.step() == SQLITE_ROW ? q.columnInt(0) : 0;
    }

    void cleanLegacyDemoDuplicates() {
        int number = demoDuplicateCount();
        if (!number) {
            showMessage(window, L"No known legacy fictional duplicates found.");
            return;
        }
        std::wstring question =
            L"Found " + std::to_wstring(number) +
            L" duplicate fictional investor records.\n\n"
            L"Create a SQLite backup and remove ONLY the "
            L"verified 'demo.investor' duplicates?\n"
            L"Do not use this feature for real financial data.";
        if (MessageBoxW(window, question.c_str(),
            L"Back up and clean demo clients",
            MB_YESNO | MB_ICONWARNING) != IDYES) return;

        const std::string backupName =
            database.fileName() + ".before_demo_cleanup.bak";
        sqlite3* backupDb = nullptr;
        if (sqlite3_open(backupName.c_str(), &backupDb) != SQLITE_OK) {
            if (backupDb) sqlite3_close(backupDb);
            throw DatabaseException("Unable to open backup file");
        }
        sqlite3_backup* copy = sqlite3_backup_init(
            backupDb, "main", database.handle(), "main");
        if (!copy) {
            string reason = sqlite3_errmsg(backupDb);
            sqlite3_close(backupDb);
            throw DatabaseException("Backup initialization failed: " + reason);
        }
        const int rc = sqlite3_backup_step(copy, -1);
        const int finished = sqlite3_backup_finish(copy);
        sqlite3_close(backupDb);
        if (rc != SQLITE_DONE || finished != SQLITE_OK)
            throw DatabaseException("Backup incomplete. Nothing deleted.");

        Transaction tx(database);
        database.execute(
            "DELETE FROM clients WHERE id IN ("
            "SELECT d.id FROM clients d JOIN clients p "
            "ON d.id<>p.id "
            "AND lower(d.name)=lower(p.name) "
            "AND d.age=p.age "
            "AND abs(d.account_balance-p.account_balance)<0.01 "
            "WHERE d.email LIKE 'demo.investor%@aurelia.example' "
            "AND p.email LIKE '%@example.com' "
            "AND p.email NOT LIKE 'demo.investor%@aurelia.example');");
        const int deleted = database.changes();
        repository.recordAuditEvent(
            "DEMO_DUPLICATES_CLEANED", 0,
            std::to_string(deleted) + " exact demo-only records consolidated; backup: " + backupName);
        tx.commit();
        showMessage(window,
            L"Consolidated " + std::to_wstring(deleted) +
            L" verified demo records.\nA backup was saved alongside the SQLite file.");
        updateLayout();
    }

    // ========================================================
    // BUTTON
    // ========================================================

    void drawButton(
        Graphics& graphics,
        const std::wstring& text,
        int x,
        int y,
        int buttonWidth,
        int buttonHeight,
        int action,
        bool primary =
        false) {

        drawRoundedBox(
            graphics,
            static_cast<REAL>(
                x),
            static_cast<REAL>(
                y),
            static_cast<REAL>(
                buttonWidth),
            static_cast<REAL>(
                buttonHeight),
            9,
            primary
            ? COLOR_DARK
            : COLOR_WHITE,
            primary
            ? COLOR_DARK
            : COLOR_LINE);


        drawLabel(
            graphics,
            text,
            static_cast<REAL>(
                x +
                5),
            static_cast<REAL>(
                y),
            static_cast<REAL>(
                buttonWidth -
                10),
            static_cast<REAL>(
                buttonHeight),
            12,
            primary
            ? COLOR_WHITE
            : COLOR_TEXT,
            true,
            StringAlignmentCenter);


        clickAreas.push_back(
            {
                {
                    x,
                    y,
                    x +
                    buttonWidth,
                    y +
                    buttonHeight
                },

                action
            });
    }


    // ========================================================
    // METRIC CARD
    // ========================================================

    void drawMetricCard(
        Graphics& graphics,
        int x,
        int y,
        int cardWidth,
        const std::wstring& title,
        const std::wstring& value,
        const std::wstring& subtitle) {

        drawRoundedBox(
            graphics,
            static_cast<REAL>(
                x),
            static_cast<REAL>(
                y),
            static_cast<REAL>(
                cardWidth),
            110,
            14,
            COLOR_WHITE,
            COLOR_LINE);


        drawLabel(
            graphics,
            title,
            static_cast<REAL>(
                x +
                18),
            static_cast<REAL>(
                y +
                10),
            static_cast<REAL>(
                cardWidth -
                36),
            24,
            10,
            COLOR_MUTED,
            true);


        drawLabel(
            graphics,
            value,
            static_cast<REAL>(
                x +
                18),
            static_cast<REAL>(
                y +
                35),
            static_cast<REAL>(
                cardWidth -
                36),
            38,
            23,
            COLOR_TEXT,
            true);


        drawLabel(
            graphics,
            subtitle,
            static_cast<REAL>(
                x +
                18),
            static_cast<REAL>(
                y +
                78),
            static_cast<REAL>(
                cardWidth -
                36),
            20,
            9,
            COLOR_MUTED);
    }


    // ========================================================
    // ALLOCATION CHART
    // ========================================================

    void drawAllocationBars(
        Graphics& graphics,
        int x,
        int y,
        int width,
        const vector<AllocationSlice>& slices,
        const std::wstring& title) {

        drawLabel(
            graphics,
            title,
            static_cast<REAL>(
                x),
            static_cast<REAL>(
                y),
            static_cast<REAL>(
                width),
            28,
            15,
            COLOR_TEXT,
            true);


        drawPlanningLabel(graphics, L"Share of portfolio (%)", x, y + 25, width, 18, 9, COLOR_MUTED);
        int rowY = y + 48;


        const int maximumBar =
            std::max(
                120,
                width -
                185);


        int index =
            0;


        for (
            const AllocationSlice& slice :
            slices) {

            if (
                index >=
                5) {

                break;
            }


            drawLabel(
                graphics,
                widen(
                    slice.label),
                static_cast<REAL>(
                    x),
                static_cast<REAL>(
                    rowY -
                    5),
                145,
                22,
                10,
                COLOR_TEXT,
                true);


            drawRoundedBox(
                graphics,
                static_cast<REAL>(
                    x +
                    150),
                static_cast<REAL>(
                    rowY),
                static_cast<REAL>(
                    maximumBar),
                11,
                5,
                Color(
                    255,
                    235,
                    232,
                    225));


            const int bar =
                std::max(
                    2,

                    static_cast<int>(
                        maximumBar *
                        std::min(
                            100.0,
                            slice.percent) /
                        100.0));


            const Color fill =
                index %
                3 ==
                0
                ? COLOR_GOLD
                : (
                    index %
                    3 ==
                    1
                    ? COLOR_SAGE
                    : COLOR_TERRACOTTA
                    );


            drawRoundedBox(
                graphics,
                static_cast<REAL>(
                    x +
                    150),
                static_cast<REAL>(
                    rowY),
                static_cast<REAL>(
                    bar),
                11,
                5,
                fill);


            std::wostringstream percentage;


            percentage
                << std::fixed
                << std::setprecision(
                    1)
                << slice.percent
                << L"%";


            drawLabel(
                graphics,
                percentage.str(),
                static_cast<REAL>(
                    x +
                    150 +
                    maximumBar +
                    8),
                static_cast<REAL>(
                    rowY -
                    6),
                55,
                22,
                10,
                COLOR_TEXT,
                true);


            rowY +=
                31;


            index++;
        }
    }


    // ========================================================
    // HISTORY CHART
    // ========================================================

    void drawHistoryChart(
        Graphics& graphics,
        int x,
        int y,
        int width,
        int height,
        const vector<PortfolioSnapshot>& history,
        const std::wstring& title) {

        drawPlanningLabel(graphics, title, x, y, width, 28, 15, COLOR_TEXT, true);
        drawPlanningLabel(graphics, L"Portfolio value (USD) | Stored snapshots", x, y + 27, width, 20, 9, COLOR_MUTED);
        if (history.size() < 2) { drawPlanningLabel(graphics, L"At least two dated snapshots are required", x, y + 55, width, 25, 10, COLOR_MUTED); return; }
        double minimum = history.front().value, maximum = minimum;
        for (const auto& p : history) { minimum = std::min(minimum, p.value); maximum = std::max(maximum, p.value); }
        if (maximum <= minimum) maximum = minimum + 1;
        int left = x + 100, top = y + 58, plotWidth = std::max(60, width - 110), plotHeight = std::max(35, height - 90);
        Pen grid(COLOR_LINE, 1.0f), line(COLOR_GOLD, 2.5f);
        for (int i = 0; i <= 2; ++i) {
            int gy = top + i * plotHeight / 2;
            graphics.DrawLine(&grid, static_cast<REAL>(left), static_cast<REAL>(gy), static_cast<REAL>(left + plotWidth), static_cast<REAL>(gy));
            drawPlanningLabel(graphics, moneyText(maximum - (maximum - minimum) * i / 2), x, gy - 9, 96, 20, 9, COLOR_MUTED);
        }
        std::vector<PointF> points;
        for (size_t i = 0; i < history.size(); ++i) points.emplace_back(static_cast<REAL>(left + plotWidth * static_cast<double>(i) / (history.size() - 1)), static_cast<REAL>(top + plotHeight - plotHeight * (history[i].value - minimum) / (maximum - minimum)));
        graphics.DrawLines(&line, points.data(), static_cast<INT>(points.size()));
        drawPlanningLabel(graphics, widen(history.front().date), left, top + plotHeight + 5, plotWidth / 2, 22, 9, COLOR_MUTED);
        drawPlanningLabel(graphics, widen(history.back().date), left + plotWidth - 100, top + plotHeight + 5, 110, 22, 9, COLOR_MUTED);
    }


    // ========================================================
    // SIDEBAR
    // ========================================================

    void drawSidebar(
        Graphics& graphics) {

        SolidBrush sidebarBrush(
            COLOR_DARK);


        graphics.FillRectangle(
            &sidebarBrush,
            0,
            0,
            225,
            height);


        drawWealthEmblem(
            graphics,
            24,
            22,
            38);


        drawTrackedBrand(
            graphics,
            L"AURELIA",
            72,
            25,
            19,
            COLOR_WHITE,
            -1.4f);


        drawLabel(
            graphics,
            L"ATELIER / WEALTH",
            72,
            58,
            135,
            22,
            9,
            COLOR_GOLD,
            true);


        struct NavigationEntry {

            const wchar_t* title;

            Page page;

            int action;
        };


        const NavigationEntry navigation[] = {
            {L"Portfolio intelligence", Page::Dashboard, ACTION_DASHBOARD},
            {L"Client relationships", Page::Clients, ACTION_CLIENTS},
            {L"Advisor ticket desk", Page::AdvisorTasks, ACTION_TASKS},
            {L"Financial goals", Page::FinancialGoals, ACTION_GOALS},
            {L"Asset allocation", Page::AllocationDetail, ACTION_ALLOCATION},
            {L"Portfolio history", Page::HistoryDetail, ACTION_HISTORY},
            {L"Reports & exports", Page::Reports, ACTION_REPORTS},
            {L"Data integrity", Page::Database, ACTION_DATABASE},
            {L"Governance log", Page::Audit, ACTION_AUDIT}
        };

        int y =
            120;


        for (
            const NavigationEntry& entry :
            navigation) {

            if (
                currentPage ==
                entry.page) {

                drawRoundedBox(
                    graphics,
                    12,
                    static_cast<REAL>(
                        y),
                    201,
                    46,
                    8,
                    Color(
                        255,
                        63,
                        72,
                        68));
            }


            drawLabel(
                graphics,
                entry.title,
                27,
                static_cast<REAL>(
                    y +
                    5),
                176,
                36,
                12,
                currentPage ==
                entry.page
                ? COLOR_WHITE
                : Color(
                    255,
                    206,
                    211,
                    207),
                currentPage ==
                entry.page);


            clickAreas.push_back(
                {
                    {
                        12,
                        y,
                        213,
                        y +
                        46
                    },

                    entry.action
                });


            y +=
                47;
        }


        drawLabel(
            graphics,
            L"CAPSTONE DEMO",
            24,
            static_cast<REAL>(
                height -
                110),
            180,
            23,
            10,
            COLOR_GOLD,
            true);


        drawLabel(
            graphics,
            L"Fictional financial records",
            24,
            static_cast<REAL>(
                height -
                85),
            180,
            20,
            9,
            Color(
                255,
                205,
                209,
                203));


        drawButton(
            graphics,
            L"Sign out",
            24,
            height -
            53,
            165,
            36,
            ACTION_LOGOUT,
            false);
    }


    // ========================================================
    // MAIN PAINT
    // ========================================================

    void paint(
        Graphics& graphics) {

        clickAreas.clear();


        graphics.SetSmoothingMode(
            SmoothingModeAntiAlias);


        graphics.Clear(
            COLOR_WHITE);


        if (
            currentPage ==
            Page::Welcome) {

            paintWelcome(
                graphics);

            return;
        }


        if (
            currentPage ==
            Page::Login) {

            paintLogin(
                graphics);

            return;
        }


        if (currentPage == Page::Loading) {
            paintLoading(graphics);
            return;
        }

        SolidBrush backgroundBrush(
            COLOR_CREAM);


        graphics.FillRectangle(
            &backgroundBrush,
            225,
            0,
            width -
            225,
            height);


        drawSidebar(
            graphics);


        paintApplicationPage(
            graphics);
    }


    // ========================================================
    // WELCOME
    // ========================================================

    void paintWelcome(
        Graphics& graphics) {

        SolidBrush backgroundBrush(
            COLOR_CREAM);


        graphics.FillRectangle(
            &backgroundBrush,
            0,
            0,
            width,
            height);


        drawWealthEmblem(
            graphics,
            48,
            34,
            58);


        drawTrackedBrand(
            graphics,
            L"AURELIA",
            122,
            42,
            25,
            COLOR_TEXT,
            -2.1f);


        drawLabel(
            graphics,
            L"ATELIER / WEALTH",
            122,
            75,
            250,
            25,
            10,
            COLOR_MUTED,
            true);


        drawButton(
            graphics,
            L"Secure portal",
            width -
            250,
            42,
            190,
            45,
            ACTION_LOGIN_PAGE,
            true);


        const int heroY =
            height /
            2 -
            175;


        drawLabel(
            graphics,
            L"Private wealth,",
            75,
            static_cast<REAL>(
                heroY),
            static_cast<REAL>(
                width -
                140),
            65,
            48,
            COLOR_TEXT,
            true);


        drawLabel(
            graphics,
            L"beautifully understood.",
            75,
            static_cast<REAL>(
                heroY +
                63),
            static_cast<REAL>(
                width -
                140),
            65,
            48,
            COLOR_TEXT,
            true);


        drawLabel(
            graphics,
            L"Portfolio intelligence, relationship operations, "
            L"database analytics, reports, goals, and governance.",
            78,
            static_cast<REAL>(
                heroY +
                158),
            static_cast<REAL>(
                width -
                155),
            35,
            16,
            COLOR_MUTED);


        drawLabel(graphics,L"Brianna Reed  |  SNHU CS 499 Capstone",78.0f,
            static_cast<REAL>(heroY+198),static_cast<REAL>(width-155),30.0f,15,COLOR_GOLD,true);

        drawButton(
            graphics,
            L"Enter demonstration",
            78,
            heroY +
            236,
            250,
            48,
            ACTION_LOGIN_PAGE,
            true);


        drawLabel(
            graphics,
            L"CREATED BY BRIANNA REED / SNHU CS 499 COMPUTER SCIENCE CAPSTONE",
            78,
            static_cast<REAL>(
                height -
                85),
            static_cast<REAL>(
                width -
                150),
            22,
            10,
            COLOR_GOLD,
            true);


        drawLabel(
            graphics,
            L"Fictional client portfolios / clearly dated public market observations / created by Brianna Reed.",
            78,
            static_cast<REAL>(
                height -
                59),
            static_cast<REAL>(
                width -
                150),
            22,
            10,
            COLOR_MUTED);
    }


    // ========================================================
    // LOGIN
    // ========================================================

    void paintLogin(
        Graphics& graphics) {

        SolidBrush backgroundBrush(
            COLOR_CREAM);


        graphics.FillRectangle(
            &backgroundBrush,
            0,
            0,
            width,
            height);


        const int cardWidth =
            470;


        const int cardHeight =
            525;


        const int cardX =
            width /
            2 -
            cardWidth /
            2;


        const int cardY =
            height /
            2 -
            cardHeight /
            2;


        drawRoundedBox(
            graphics,
            static_cast<REAL>(
                cardX),
            static_cast<REAL>(
                cardY),
            static_cast<REAL>(
                cardWidth),
            static_cast<REAL>(
                cardHeight),
            20,
            COLOR_WHITE,
            COLOR_LINE);


        drawWealthEmblem(
            graphics,
            static_cast<REAL>(
                cardX +
                52),
            static_cast<REAL>(
                cardY +
                34),
            45);


        drawTrackedBrand(
            graphics,
            L"AURELIA",
            static_cast<REAL>(
                cardX +
                112),
            static_cast<REAL>(
                cardY +
                41),
            22,
            COLOR_TEXT,
            -1.8f);


        drawLabel(
            graphics,
            L"Secure demonstration portal",
            static_cast<REAL>(
                cardX +
                52),
            static_cast<REAL>(
                cardY +
                88),
            static_cast<REAL>(
                cardWidth -
                104),
            28,
            12,
            COLOR_MUTED);


        drawLabel(
            graphics,
            L"Username",
            static_cast<REAL>(
                cardX +
                70),
            static_cast<REAL>(
                height /
                2 -
                66),
            330,
            26,
            12,
            COLOR_TEXT,
            true);


        drawLabel(
            graphics,
            L"Password",
            static_cast<REAL>(
                cardX +
                70),
            static_cast<REAL>(
                height /
                2 +
                3),
            330,
            26,
            12,
            COLOR_TEXT,
            true);


        drawButton(
            graphics,
            L"Sign in",
            cardX +
            70,
            height /
            2 +
            100,
            330,
            46,
            ACTION_LOGIN,
            true);


        drawButton(
            graphics,
            L"Return to welcome",
            cardX +
            70,
            height /
            2 +
            160,
            330,
            42,
            ACTION_LOGIN_PAGE,
            false);
    }


    // ========================================================
    // APPLICATION PAGE ROUTING
    // ========================================================

    void paintApplicationPage(
        Graphics& graphics) {

        const int contentX =
            254;


        const int contentWidth =
            std::max(
                700,
                width -
                288);


        std::wstring title;


        switch (
            currentPage) {

        case Page::Dashboard:

            title =
                L"Portfolio intelligence";

            break;


        case Page::Clients:

            title =
                L"Client relationships";

            break;


        case Page::AdvisorTasks:
            title = L"Advisor ticket desk";
            break;
        case Page::FinancialGoals:
            title = L"Financial goals";
            break;
        case Page::AllocationDetail:
            title = L"Asset allocation";
            break;
        case Page::HistoryDetail:
            title = L"Portfolio history";
            break;

        case Page::ClientWorkspace:

            title =
                L"Client portfolio workspace";

            break;


        case Page::Reports:

            title =
                L"Reports & exports";

            break;


        case Page::Database:

            title =
                L"Data integrity";

            break;


        case Page::ClientDetails: title=L"Client profile & planning"; break;
        case Page::MarketData: title=L"Market sources & provenance"; break;
        case Page::Audit:

            title =
                L"Governance log";

            break;


        default:

            title =
                L"Aurelia Wealth Studio";

            break;
        }


        drawLabel(
            graphics,
            title,
            static_cast<REAL>(
                contentX),
            22,
            static_cast<REAL>(
                contentWidth),
            46,
            27,
            COLOR_TEXT,
            true);


        drawLabel(
            graphics,
            L"AURELIA ATELIER / PRIVATE WEALTH OPERATIONS",
            static_cast<REAL>(
                contentX),
            68,
            static_cast<REAL>(
                contentWidth),
            24,
            10,
            COLOR_MUTED,
            true);


        switch (
            currentPage) {

        case Page::Dashboard:

            paintDashboard(
                graphics,
                contentX,
                contentWidth);

            break;


        case Page::Clients:

            paintClients(
                graphics,
                contentX,
                contentWidth);

            break;


        case Page::ClientWorkspace:

            paintClientWorkspace(
                graphics,
                contentX,
                contentWidth);

            break;


        case Page::MarketData: paintMarketData(graphics,contentX,contentWidth); break;
        case Page::ClientDetails:
            paintClientDetails(graphics,contentX,contentWidth);
            break;
        case Page::AdvisorTasks:
            paintOperationalRegister(graphics, contentX, contentWidth, true);
            break;
        case Page::FinancialGoals:
            paintOperationalRegister(graphics, contentX, contentWidth, false);
            break;
        case Page::AllocationDetail:
            paintAllocationDetail(graphics, contentX, contentWidth);
            break;
        case Page::HistoryDetail:
            paintHistoryDetail(graphics, contentX, contentWidth);
            break;

        case Page::Reports:

            paintReports(
                graphics,
                contentX,
                contentWidth);

            break;


        case Page::Database:

            paintDatabase(
                graphics,
                contentX,
                contentWidth);

            break;


        case Page::Audit:

            paintAudit(
                graphics,
                contentX,
                contentWidth);

            break;


        default:

            break;
        }


        drawLabel(
            graphics,
            L"Fictional demonstration environment. "
            L"Review signals are not personalized investment advice.",
            static_cast<REAL>(
                contentX),
            static_cast<REAL>(
                height -
                27),
            static_cast<REAL>(
                contentWidth),
            18,
            9,
            COLOR_MUTED);
    }


    // ========================================================
    // DASHBOARD
    // ========================================================

    void paintDashboard(
        Graphics& graphics,
        int contentX,
        int contentWidth) {

        const EnterpriseSummary summary =
            analytics.enterpriseSummary();


        const int gap =
            12;


        const int cardWidth =
            std::max(
                150,
                (
                    contentWidth -
                    gap *
                    3
                    ) /
                4);


        drawMetricCard(
            graphics,
            contentX,
            120,
            cardWidth,
            L"ASSETS ADMINISTERED",
            moneyText(
                summary
                .assetsAdministered),
            L"Live value from stored holdings");


        drawMetricCard(
            graphics,
            contentX +
            cardWidth +
            gap,
            120,
            cardWidth,
            L"CLIENT RELATIONSHIPS",
            std::to_wstring(summary.clientCount),
            std::to_wstring(summary.activeClients)+L" active / "+std::to_wstring(summary.clientCount-summary.activeClients)+L" inactive");


        drawMetricCard(
            graphics,
            contentX +
            (
                cardWidth +
                gap
                ) *
            2,
            120,
            cardWidth,
            L"SNAPSHOT CHANGE",
            percentText(
                summary
                .twelveMonthChangePercent),
            L"Stored portfolio snapshots");


        drawMetricCard(
            graphics,
            contentX +
            (
                cardWidth +
                gap
                ) *
            3,
            120,
            cardWidth,
            L"OPEN TASKS",
            std::to_wstring(
                summary.openTasks),
            std::to_wstring(
                summary.overdueTasks) +
            L" overdue");


        const int half =
            (
                contentWidth -
                16
                ) /
            2;


        drawRoundedBox(
            graphics,
            static_cast<REAL>(
                contentX),
            250,
            static_cast<REAL>(
                half),
            305,
            15,
            COLOR_WHITE,
            COLOR_LINE);


        drawAllocationBars(
            graphics,
            contentX +
            20,
            267,
            half -
            40,
            analytics
            .enterpriseAllocation(),
            L"Enterprise asset allocation");


        drawRoundedBox(
            graphics,
            static_cast<REAL>(
                contentX +
                half +
                16),
            250,
            static_cast<REAL>(
                half),
            305,
            15,
            COLOR_WHITE,
            COLOR_LINE);


        drawHistoryChart(
            graphics,
            contentX +
            half +
            36,
            267,
            half -
            56,
            255,
            analytics
            .enterpriseHistory(),
            L"Portfolio value trend");


        drawRoundedBox(
            graphics,
            static_cast<REAL>(
                contentX),
            575,
            static_cast<REAL>(
                contentWidth),
            110,
            15,
            COLOR_SOFT_GOLD,
            COLOR_LINE);


        drawLabel(
            graphics,
            L"Operating signals",
            static_cast<REAL>(
                contentX +
                20),
            588,
            static_cast<REAL>(
                contentWidth -
                40),
            28,
            15,
            COLOR_TEXT,
            true);


        std::wostringstream signalText;


        signalText
            << L"Goals funded >=55%: "
            << summary.goals.onTrack

            << L"     Goals needing review: "
            << summary.goals.watch

            << L"     Completed goals: "
            << summary.goals.completed

            << L"     Unrealized gain: "
            << moneyText(
                summary
                .unrealizedGain);


        drawLabel(
            graphics,
            signalText.str(),
            static_cast<REAL>(
                contentX +
                20),
            624,
            static_cast<REAL>(
                contentWidth -
                40),
            32,
            11,
            COLOR_MUTED);


        drawButton(
            graphics,
            L"Open clients",
            contentX,
            height -
            82,
            135,
            39,
            ACTION_CLIENTS,
            true);


        drawButton(
            graphics,
            L"Reports",
            contentX +
            148,
            height -
            82,
            115,
            39,
            ACTION_REPORTS,
            false);

        drawButton(graphics,L"Market sources",contentX+277,height-82,145,39,ACTION_MARKET,false);
        drawButton(graphics,L"Create ticket",contentX+435,height-82,140,39,ACTION_NEW_TASK,false);
        drawButton(graphics,L"Create goal",contentX+588,height-82,130,39,ACTION_NEW_GOAL,false);
        // Entire cards and charts are clickable, with matching records
        const int actionByCard[4] = {
            ACTION_ALLOCATION, ACTION_CLIENTS, ACTION_HISTORY, ACTION_TASKS };
        for (int i = 0; i < 4; ++i) {
            const int left = contentX + i * (cardWidth + gap);
            clickAreas.push_back({
                {left,120,left + cardWidth,230}, actionByCard[i] });
        }
        clickAreas.push_back({
            {contentX,250,contentX + half,555}, ACTION_ALLOCATION });
        clickAreas.push_back({
            {contentX + half + 16,250,contentX + contentWidth,555}, ACTION_HISTORY });
        clickAreas.push_back({
            {contentX,575,contentX + contentWidth,685}, ACTION_GOALS });
    }





    // ========================================================
    // CLIENT LIST
    // ========================================================

    void paintClients(
        Graphics& graphics,
        int contentX,
        int contentWidth) {

        drawLabel(
            graphics,
            L"Search client relationships",
            static_cast<REAL>(
                contentX),
            116,
            380,
            28,
            11,
            COLOR_MUTED);


        drawLabel(
            graphics,
            L"Double-click a client or select one "
            L"and choose Open workspace.",
            static_cast<REAL>(
                contentX +
                410),
            145,
            static_cast<REAL>(
                contentWidth -
                410),
            35,
            10,
            COLOR_MUTED);


        drawButton(
            graphics,
            L"Open workspace",
            contentX,
            height -
            75,
            160,
            39,
            ACTION_OPEN_CLIENT,
            true);


        drawButton(
            graphics,
            L"Refresh",
            contentX +
            173,
            height -
            75,
            105,
            39,
            ACTION_REFRESH,
            false);
    }


    // ========================================================
    // CLIENT WORKSPACE
    // ========================================================

    void paintClientWorkspace(
        Graphics& graphics,
        int contentX,
        int contentWidth) {

        if (
            selectedWorkspaceClientId <
            0) {

            drawLabel(
                graphics,
                L"No client selected.",
                static_cast<REAL>(
                    contentX),
                150,
                static_cast<REAL>(
                    contentWidth),
                40,
                18,
                COLOR_MUTED);

            return;
        }


        const ClientDashboard dashboard =
            analytics.clientDashboard(
                selectedWorkspaceClientId);


        drawLabel(
            graphics,
            widen(
                dashboard.client.name),
            static_cast<REAL>(
                contentX),
            108,
            static_cast<REAL>(
                contentWidth),
            38,
            24,
            COLOR_TEXT,
            true);


        drawLabel(
            graphics,
            widen(
                dashboard
                .client
                .householdName +
                "  |  " +
                dashboard
                .client
                .advisorName +
                "  |  " +
                riskLevelToString(
                    dashboard
                    .client
                    .riskLevel)),
            static_cast<REAL>(
                contentX),
            143,
            static_cast<REAL>(
                contentWidth),
            27,
            10,
            COLOR_MUTED);


        const int gap =
            12;


        const int cardWidth =
            std::max(
                160,
                (
                    contentWidth -
                    gap *
                    3
                    ) /
                4);


        drawMetricCard(
            graphics,
            contentX,
            180,
            cardWidth,
            L"PORTFOLIO VALUE",
            moneyText(
                dashboard
                .currentValue),
            L"Stored holdings");


        drawMetricCard(
            graphics,
            contentX +
            cardWidth +
            gap,
            180,
            cardWidth,
            L"UNREALIZED GAIN",
            moneyText(
                dashboard
                .unrealizedGain),
            L"Price minus cost basis");


        drawMetricCard(
            graphics,
            contentX +
            (
                cardWidth +
                gap
                ) *
            2,
            180,
            cardWidth,
            L"SNAPSHOT CHANGE",
            percentText(
                dashboard
                .twelveMonthChangePercent),
            L"Stored monthly snapshots");


        double primaryGoal =
            0.0;


        if (
            !dashboard.goals.empty() &&
            dashboard
            .goals
            .front()
            .targetAmount >
            0.0) {

            primaryGoal =
                dashboard
                .goals
                .front()
                .currentAmount /
                dashboard
                .goals
                .front()
                .targetAmount *
                100.0;
        }


        drawMetricCard(
            graphics,
            contentX +
            (
                cardWidth +
                gap
                ) *
            3,
            180,
            cardWidth,
            L"PRIMARY GOAL",
            percentText(
                primaryGoal),
            dashboard.goals.empty()
            ? L"No active goal"
            : widen(
                dashboard
                .goals
                .front()
                .name));


        const int half =
            (
                contentWidth -
                16
                ) /
            2;


        drawRoundedBox(
            graphics,
            static_cast<REAL>(
                contentX),
            310,
            static_cast<REAL>(
                half),
            215,
            14,
            COLOR_WHITE,
            COLOR_LINE);


        drawAllocationBars(
            graphics,
            contentX +
            16,
            322,
            half -
            32,
            dashboard.actualAllocation,
            L"Actual allocation");


        drawRoundedBox(
            graphics,
            static_cast<REAL>(
                contentX +
                half +
                16),
            310,
            static_cast<REAL>(
                half),
            215,
            14,
            COLOR_WHITE,
            COLOR_LINE);


        drawHistoryChart(
            graphics,
            contentX +
            half +
            34,
            322,
            half -
            52,
            182,
            dashboard.history,
            L"Portfolio history");


        drawRoundedBox(
            graphics,
            static_cast<REAL>(
                contentX),
            538,
            static_cast<REAL>(
                contentWidth),
            40,
            12,
            COLOR_SOFT_GOLD,
            COLOR_LINE);


        if (
            !dashboard.insights.empty()) {

            drawLabel(
                graphics,
                widen(
                    dashboard
                    .insights
                    .front()
                    .title +
                    ": " +
                    dashboard
                    .insights
                    .front()
                    .detail),
                static_cast<REAL>(
                    contentX +
                    14),
                543,
                static_cast<REAL>(
                    contentWidth -
                    28),
                28,
                10,
                dashboard
                .insights
                .front()
                .severity ==
                "High"
                ? COLOR_TERRACOTTA
                : COLOR_MUTED,
                dashboard
                .insights
                .front()
                .severity ==
                "High");
        }


        drawButton(
            graphics,
            L"Back to clients",
            contentX,
            height -
            75,
            145,
            39,
            ACTION_BACK_TO_CLIENTS,
            false);


        drawButton(
            graphics,
            L"Client PDF",
            contentX +
            158,
            height -
            75,
            120,
            39,
            ACTION_EXPORT_CLIENT_PDF,
            true);


        drawButton(
            graphics,
            L"Client Excel",
            contentX +
            291,
            height -
            75,
            125,
            39,
            ACTION_EXPORT_CLIENT_XLSX,
            false);
        drawButton(graphics,L"Profile & planning",contentX+430,height-75,165,39,ACTION_CLIENT_DETAILS,false);
    }


    // ========================================================
    // REPORTS
    // ========================================================

    void paintReports(
        Graphics& graphics,
        int contentX,
        int contentWidth) {

        drawRoundedBox(
            graphics,
            static_cast<REAL>(
                contentX),
            125,
            static_cast<REAL>(
                contentWidth),
            210,
            16,
            COLOR_WHITE,
            COLOR_LINE);


        drawLabel(
            graphics,
            L"Enterprise reporting",
            static_cast<REAL>(
                contentX +
                24),
            143,
            static_cast<REAL>(
                contentWidth -
                48),
            34,
            20,
            COLOR_TEXT,
            true);


        drawLabel(
            graphics,
            L"Generate branded reports directly from "
            L"stored clients, accounts, holdings, securities, "
            L"portfolio history, goals, transactions, and tasks.",
            static_cast<REAL>(
                contentX +
                24),
            182,
            static_cast<REAL>(
                contentWidth -
                48),
            46,
            11,
            COLOR_MUTED);


        drawButton(
            graphics,
            L"Enterprise PDF",
            contentX +
            24,
            252,
            165,
            46,
            ACTION_EXPORT_ENTERPRISE_PDF,
            true);


        drawButton(
            graphics,
            L"Enterprise Excel",
            contentX +
            204,
            252,
            175,
            46,
            ACTION_EXPORT_ENTERPRISE_XLSX,
            false);


        drawRoundedBox(
            graphics,
            static_cast<REAL>(
                contentX),
            360,
            static_cast<REAL>(
                contentWidth),
            220,
            16,
            COLOR_SOFT_GOLD,
            COLOR_LINE);


        drawLabel(
            graphics,
            L"Client reporting",
            static_cast<REAL>(
                contentX +
                24),
            378,
            static_cast<REAL>(
                contentWidth -
                48),
            32,
            18,
            COLOR_TEXT,
            true);


        drawLabel(
            graphics,
            L"Open a client workspace first, then generate "
            L"a PDF or Excel workbook scoped to that relationship. "
            L"PDF reports include charts and review signals. "
            L"Excel workbooks contain the underlying relational data.",
            static_cast<REAL>(
                contentX +
                24),
            418,
            static_cast<REAL>(
                contentWidth -
                48),
            62,
            11,
            COLOR_MUTED);


        drawButton(
            graphics,
            L"Selected client PDF",
            contentX +
            24,
            505,
            180,
            44,
            ACTION_EXPORT_CLIENT_PDF,
            false);


        drawButton(
            graphics,
            L"Selected client Excel",
            contentX +
            218,
            505,
            190,
            44,
            ACTION_EXPORT_CLIENT_XLSX,
            false);
    }


    // ========================================================
    // DATA INTEGRITY
    // ========================================================

    void paintDatabase(
        Graphics& graphics,
        int contentX,
        int contentWidth) {

        Statement integrity(
            database.handle(),
            "PRAGMA integrity_check;");


        std::wstring result =
            L"Unavailable";


        if (
            integrity.step() ==
            SQLITE_ROW) {

            result =
                widen(
                    integrity.columnText(
                        0));
        }


        drawRoundedBox(
            graphics,
            static_cast<REAL>(
                contentX),
            125,
            static_cast<REAL>(
                contentWidth),
            390,
            15,
            COLOR_WHITE,
            COLOR_LINE);


        const std::vector<
            std::pair<
            std::wstring,
            std::wstring>>
            rows = {

            {
                L"Storage engine",

                L"SQLite / WinSQLite"
            },

            {
                L"Database file",

                widen(
                    database.fileName())
            },

            {
                L"SQLite version",

                widen(
                    sqlite3_libversion())
            },

            {
                L"Integrity check",

                result
            },

            {
                L"Schema version",

                L"2"
            },

            {
                L"Relational domains",

                L"Clients • Accounts • Holdings • "
                L"Transactions • Snapshots • Goals • "
                L"Notes • Tasks"
            }
        };


        int y =
            150;


        for (
            const auto& row :
            rows) {

            drawLabel(
                graphics,
                row.first,
                static_cast<REAL>(
                    contentX +
                    22),
                static_cast<REAL>(
                    y),
                210,
                30,
                12,
                COLOR_MUTED);


            drawLabel(
                graphics,
                row.second,
                static_cast<REAL>(
                    contentX +
                    250),
                static_cast<REAL>(
                    y),
                static_cast<REAL>(
                    contentWidth -
                    280),
                30,
                12,
                COLOR_TEXT,
                true);


            y +=
                55;
        }


        drawLabel(
            graphics,
            L"Prepared statements • transactions • foreign keys • "
            L"indexes • audit history • WAL journal",
            static_cast<REAL>(
                contentX),
            545,
            static_cast<REAL>(
                contentWidth),
            35,
            11,
            COLOR_MUTED);
        paintDemoDuplicateNotice(graphics, contentX, contentWidth);
    }


    // Legacy duplicate count is read only until confirmed by the user
    void paintDemoDuplicateNotice(Graphics& graphics,
        int x, int w) {
        const int n = demoDuplicateCount();
        drawRoundedBox(graphics, x, 598, w, 94, 12,
            COLOR_SOFT_GOLD, COLOR_LINE);
        drawLabel(graphics,
            n ? (L"Known exact demo duplicates: " + std::to_wstring(n))
            : L"No known exact demo-seed duplicates detected",
            x + 19, 602, w - 40, 29, 13,
            n ? COLOR_TERRACOTTA : COLOR_SAGE, true);
        if (n) drawButton(graphics, L"Back up and consolidate demo records",
            x + 19, 640, 297, 40, ACTION_DUPLICATE_REVIEW, true);
        else drawLabel(graphics,
            L"Your existing database is preserved. No cleanup is needed.",
            x + 19, 640, w - 35, 30, 11, COLOR_MUTED);
    }

    // ========================================================
    // GOVERNANCE LOG
    // ========================================================

    void paintAudit(
        Graphics& graphics,
        int contentX,
        int contentWidth) {

        drawRoundedBox(
            graphics,
            static_cast<REAL>(
                contentX),
            125,
            static_cast<REAL>(
                contentWidth),
            static_cast<REAL>(
                std::max(
                    360,
                    height -
                    205)),
            15,
            COLOR_WHITE,
            COLOR_LINE);


        drawLabel(
            graphics,
            L"Recent database and export events",
            static_cast<REAL>(
                contentX +
                20),
            140,
            static_cast<REAL>(
                contentWidth -
                40),
            32,
            16,
            COLOR_TEXT,
            true);


        Statement statement(
            database.handle(),
            "SELECT "
            "occurred_at,"
            "event_type,"
            "details "
            "FROM audit_events "
            "ORDER BY id DESC "
            "LIMIT 20;");


        int y =
            184;


        while (
            statement.step() ==
            SQLITE_ROW) {

            drawLabel(
                graphics,
                widen(
                    statement.columnText(
                        1)),
                static_cast<REAL>(
                    contentX +
                    22),
                static_cast<REAL>(
                    y),
                180,
                26,
                10,
                COLOR_TEXT,
                true);


            drawLabel(
                graphics,
                widen(
                    statement.columnText(
                        0)),
                static_cast<REAL>(
                    contentX +
                    205),
                static_cast<REAL>(
                    y),
                150,
                26,
                9,
                COLOR_GOLD,
                true);


            drawLabel(
                graphics,
                widen(
                    statement.columnText(
                        2)),
                static_cast<REAL>(
                    contentX +
                    365),
                static_cast<REAL>(
                    y),
                static_cast<REAL>(
                    contentWidth -
                    390),
                26,
                9,
                COLOR_MUTED);


            y +=
                32;


            if (
                y >
                height -
                70) {

                break;
            }
        }
    }


    // ========================================================
    // SAVE FILE DIALOG
    // ========================================================

    std::wstring chooseSavePath(
        const wchar_t* filter,
        const wchar_t* defaultExtension,
        const wchar_t* defaultFileName) {

        wchar_t path[
            MAX_PATH] = {};


            wcscpy_s(
                path,
                defaultFileName);


            OPENFILENAMEW dialog = {};


            dialog.lStructSize =
                sizeof(
                    dialog);


            dialog.hwndOwner =
                window;


            dialog.lpstrFilter =
                filter;


            dialog.lpstrFile =
                path;


            dialog.nMaxFile =
                MAX_PATH;


            dialog.lpstrDefExt =
                defaultExtension;


            dialog.Flags =
                OFN_OVERWRITEPROMPT |
                OFN_PATHMUSTEXIST;


            if (
                GetSaveFileNameW(
                    &dialog)) {

                return path;
            }


            return L"";
    }


    void openFile(
        const std::wstring& path) {

        ShellExecuteW(
            window,
            L"open",
            path.c_str(),
            nullptr,
            nullptr,
            SW_SHOWNORMAL);
    }


    // ========================================================
    // ACTIONS
    // ========================================================

    void handleAction(
        int action) {

        try {

            switch (
                action) {

            case ACTION_CLIENT_DETAILS: navigate(Page::ClientDetails); break;
            case ACTION_PROFILE_PORTFOLIO: navigate(Page::ClientWorkspace); break;
            case ACTION_EDIT_PROFILE: editClientProfile(); break;
            case ACTION_NEW_TASK: editTicket(false); break;
            case ACTION_EDIT_TASK: editTicket(true); break;
            case ACTION_NEW_GOAL: editFinancialGoal(false); break;
            case ACTION_EDIT_GOAL: editFinancialGoal(true); break;
            case ACTION_ADD_NOTE: addAdvisorNote(); break;
            case ACTION_ADD_ACCOUNT: addAccount(); break;
            case ACTION_SET_HOLDING: recordHolding(); break;
            case ACTION_MARKET: navigate(Page::MarketData); break;
            case ACTION_RECORD_QUOTE: enterMarketObservation(); break;
            case ACTION_REFRESH_QUOTE: refreshMarketObservation(); break;
            case ACTION_LOGIN_PAGE:

                navigate(
                    currentPage ==
                    Page::Login
                    ? Page::Welcome
                    : Page::Login);

                break;


            case ACTION_LOGIN:

                if (
                    failedLoginAttempts <
                    3) {

                    login();
                }

                break;


            case ACTION_LOGOUT:

                authenticated =
                    false;

                selectedWorkspaceClientId =
                    -1;

                SetWindowTextW(
                    usernameField,
                    L"");

                SetWindowTextW(
                    passwordField,
                    L"");

                navigate(
                    Page::Welcome);

                break;


            case ACTION_DASHBOARD:

                navigate(
                    Page::Dashboard);

                break;


            case ACTION_CLIENTS:

                navigate(
                    Page::Clients);

                break;


            case ACTION_TASKS:
                navigate(Page::AdvisorTasks);
                break;
            case ACTION_GOALS:
                navigate(Page::FinancialGoals);
                break;
            case ACTION_ALLOCATION:
                navigate(Page::AllocationDetail);
                break;
            case ACTION_HISTORY:
                navigate(Page::HistoryDetail);
                break;
            case ACTION_TASK_CLIENT: {
                const int row = selectedTaskIndex();
                if (row < 0 || row >= (int)displayedTasks.size() ||
                    !displayedTasks[row].clientId) {
                    showMessage(window, L"Select a task assigned to a client.");
                    break;
                }
                selectedWorkspaceClientId = *displayedTasks[row].clientId;
                navigate(Page::ClientWorkspace);
                break;
            }
            case ACTION_GOAL_CLIENT: {
                const int row = selectedGoalIndex();
                if (row < 0 || row >= (int)displayedGoals.size()) {
                    showMessage(window, L"Select a goal row first.");
                    break;
                }
                selectedWorkspaceClientId = displayedGoals[row].clientId;
                navigate(Page::ClientWorkspace);
                break;
            }
            case ACTION_TASK_COMPLETE: {
                const int row = selectedTaskIndex();
                if (row < 0 || row >= (int)displayedTasks.size()) {
                    showMessage(window, L"Select an open task first.");
                    break;
                }
                if (MessageBoxW(window,
                    L"Mark the selected advisor task completed?",
                    L"Confirm action", MB_YESNO | MB_ICONQUESTION) != IDYES)
                    break;
                const auto& t=displayedTasks[row];
                repository.saveTask(t.id,t.clientId.value_or(0),t.title,
                    t.description.empty()?"Complete the documented advisor follow-up.":t.description,
                    t.category,t.dueDate,t.priority,t.assignedTo.empty()?"Brianna Reed":t.assignedTo,"Completed");
                refreshTasksList();
                break;
            }
            case ACTION_DUPLICATE_REVIEW:
                cleanLegacyDemoDuplicates();
                break;

            case ACTION_REPORTS:

                navigate(
                    Page::Reports);

                break;


            case ACTION_DATABASE:

                navigate(
                    Page::Database);

                break;


            case ACTION_AUDIT:

                navigate(
                    Page::Audit);

                break;


            case ACTION_OPEN_CLIENT:
            {
                const int id =
                    selectedClientId();


                if (
                    id <
                    0) {

                    showMessage(
                        window,
                        L"Select a client relationship first.");

                    break;
                }


                selectedWorkspaceClientId =
                    id;


                navigate(
                    Page::ClientWorkspace);

                break;
            }


            case ACTION_BACK_TO_CLIENTS:

                navigate(
                    Page::Clients);

                break;


            case ACTION_REFRESH:

                if (currentPage == Page::Clients) refreshClientList();
                if (currentPage == Page::AdvisorTasks) refreshTasksList();
                if (currentPage == Page::FinancialGoals) refreshGoalsList();

                break;


            case ACTION_EXPORT_ENTERPRISE_PDF:
            {
                const std::wstring path =
                    chooseSavePath(
                        L"PDF Files\0"
                        L"*.pdf\0"
                        L"All Files\0"
                        L"*.*\0",

                        L"pdf",

                        L"Aurelia_Enterprise_Portfolio_Report.pdf");


                if (
                    path.empty()) {

                    break;
                }


                pdfWriter
                    .exportEnterpriseReport(
                        narrow(
                            path));


                openFile(
                    path);


                showMessage(
                    window,
                    L"Enterprise PDF created successfully.");

                break;
            }


            case ACTION_EXPORT_ENTERPRISE_XLSX:
            {
                const std::wstring path =
                    chooseSavePath(
                        L"Excel Workbook\0"
                        L"*.xlsx\0"
                        L"All Files\0"
                        L"*.*\0",

                        L"xlsx",

                        L"Aurelia_Enterprise_Portfolio_Data.xlsx");


                if (
                    path.empty()) {

                    break;
                }


                excelExporter
                    .exportEnterpriseWorkbook(
                        narrow(
                            path));


                openFile(
                    path);


                showMessage(
                    window,
                    L"Enterprise Excel workbook "
                    L"created successfully.");

                break;
            }


            case ACTION_EXPORT_CLIENT_PDF:
            {
                if (
                    selectedWorkspaceClientId <
                    0) {

                    showMessage(
                        window,
                        L"Open a client workspace first.");

                    break;
                }


                const std::wstring path =
                    chooseSavePath(
                        L"PDF Files\0"
                        L"*.pdf\0"
                        L"All Files\0"
                        L"*.*\0",

                        L"pdf",

                        L"Aurelia_Client_Portfolio_Report.pdf");


                if (
                    path.empty()) {

                    break;
                }


                pdfWriter
                    .exportClientReport(
                        selectedWorkspaceClientId,
                        narrow(
                            path));


                openFile(
                    path);


                showMessage(
                    window,
                    L"Client PDF created successfully.");

                break;
            }


            case ACTION_EXPORT_CLIENT_XLSX:
            {
                if (
                    selectedWorkspaceClientId <
                    0) {

                    showMessage(
                        window,
                        L"Open a client workspace first.");

                    break;
                }


                const std::wstring path =
                    chooseSavePath(
                        L"Excel Workbook\0"
                        L"*.xlsx\0"
                        L"All Files\0"
                        L"*.*\0",

                        L"xlsx",

                        L"Aurelia_Client_Portfolio_Data.xlsx");


                if (
                    path.empty()) {

                    break;
                }


                excelExporter
                    .exportClientWorkbook(
                        selectedWorkspaceClientId,
                        narrow(
                            path));


                openFile(
                    path);


                showMessage(
                    window,
                    L"Client Excel workbook "
                    L"created successfully.");

                break;
            }


            default:

                break;
            }


            InvalidateRect(
                window,
                nullptr,
                FALSE);
        }


        catch (
            const std::exception& exception) {

            showMessage(
                window,
                L"Operation failed:\n\n" +
                widen(
                    exception.what()));


            logger.write(
                "GUI_OPERATION_ERROR");
        }
    }
};


// ============================================================
// MAIN WINDOW PROCEDURE
// ============================================================

static LRESULT CALLBACK MainWindowProc(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam) {

    AureliaApplication* app =
        reinterpret_cast<
        AureliaApplication*>(
            GetWindowLongPtrW(
                window,
                GWLP_USERDATA));


    if (
        message ==
        WM_NCCREATE) {

        CREATESTRUCTW* create =
            reinterpret_cast<
            CREATESTRUCTW*>(
                lParam);


        app =
            reinterpret_cast<
            AureliaApplication*>(
                create
                ->lpCreateParams);


        SetWindowLongPtrW(
            window,
            GWLP_USERDATA,
            reinterpret_cast<
            LONG_PTR>(
                app));


        return TRUE;
    }


    if (!app) {

        return DefWindowProcW(
            window,
            message,
            wParam,
            lParam);
    }


    switch (
        message) {
    case WM_TIMER:
        if(wParam==28){app->pollMarketObservation();return 0;}
        if (wParam == 27 && app->currentPage == Page::Loading) {
            ++app->loadingFrame;
            InvalidateRect(window, nullptr, FALSE);
            if (app->loadingFrame >= 3) {
                KillTimer(window, 27);
                app->currentPage = app->loadingDestination;
                app->updateLayout();
            }
            return 0;
        }
        break;


    case WM_SIZE:

        app->updateLayout();

        return 0;


    case WM_COMMAND:

        if (
            LOWORD(
                wParam) ==
            1003 &&
            HIWORD(
                wParam) ==
            EN_CHANGE &&
            app->currentPage ==
            Page::Clients) {

            app->refreshClientList();
        }

        return 0;


    case WM_NOTIFY:
    {
        NMHDR* header =
            reinterpret_cast<
            NMHDR*>(
                lParam);


        if (
            header &&
            header->idFrom ==
            1004 &&
            header->code ==
            NM_DBLCLK &&
            app->currentPage ==
            Page::Clients) {

            app->handleAction(
                ACTION_OPEN_CLIENT);

            return 0;
        }


        if (header && app->isTaskList(header->hwndFrom) && header->code == LVN_GETINFOTIP) {
            auto* tip = reinterpret_cast<NMLVGETINFOTIPW*>(lParam);
            int index = app->taskRowIndex(header->hwndFrom, tip->iItem);
            if (index >= 0 && index < static_cast<int>(app->displayedTasks.size())) {
                const auto& t = app->displayedTasks[index];
                auto client = t.clientId ? app->repository.findClientById(*t.clientId) : std::optional<Client>{};
                std::wstring text = widen("AW-" + std::to_string(t.id) + " | " + (client ? client->name : "Firm wide") + "\n" + t.title + "\nPriority: " + t.priority + " | Status: " + t.status + "\nDue: " + t.dueDate + " | Assigned to: " + t.assignedTo + "\nCategory: " + t.category + "\n" + t.description);
                wcsncpy_s(tip->pszText, tip->cchTextMax, text.c_str(), _TRUNCATE);
            }
            return 0;
        }
        if (header && header->code == LVN_COLUMNCLICK) {
            int column = reinterpret_cast<NMLISTVIEW*>(lParam)->iSubItem;
            if (app->isTaskList(header->hwndFrom)) {
                app->taskSortAscending = app->taskSortColumn == column ? !app->taskSortAscending : true;
                app->taskSortColumn = column; app->refreshTasksList(); return 0;
            }
            if (header->hwndFrom == app->clientList) {
                app->clientSortAscending = app->clientSortColumn == column ? !app->clientSortAscending : true;
                app->clientSortColumn = column; app->refreshClientList(); return 0;
            }
        }
        if (header && app->isTaskList(header->hwndFrom) && header->code == LVN_ITEMCHANGED) {
            auto* change = reinterpret_cast<NMLISTVIEW*>(lParam);
            if ((change->uNewState & LVIS_SELECTED) && !(change->uOldState & LVIS_SELECTED)) {
                for (HWND lane : { app->tasksList, app->mediumTasksList, app->completedTasksList }) if (lane != header->hwndFrom) ListView_SetItemState(lane, -1, 0, LVIS_SELECTED);
                InvalidateRect(app->window, nullptr, FALSE);
            }
        }
        if(header && app->isTaskList(header->hwndFrom) && header->code==NM_CUSTOMDRAW){
            auto* cd=reinterpret_cast<NMLVCUSTOMDRAW*>(lParam);
            if(cd->nmcd.dwDrawStage==CDDS_PREPAINT)return CDRF_NOTIFYITEMDRAW;
            if(cd->nmcd.dwDrawStage==CDDS_ITEMPREPAINT){size_t row=static_cast<size_t>(app->taskRowIndex(header->hwndFrom, static_cast<int>(cd->nmcd.dwItemSpec)));if(row<app->displayedTasks.size()){const auto& t=app->displayedTasks[row];cd->clrText=t.completed?RGB(110,110,110):(t.priority=="High"||t.priority=="Urgent"?RGB(155,65,55):RGB(29,38,42));cd->clrTextBk=row%2?RGB(249,246,240):RGB(255,255,255);}return CDRF_DODEFAULT;}
        }
        if (header && header->code == NM_DBLCLK &&
            app->isTaskList(header->hwndFrom) &&
            app->currentPage == Page::AdvisorTasks) {
            app->handleAction(ACTION_TASK_CLIENT);
            return 0;
        }
        if (header && header->code == NM_DBLCLK &&
            header->idFrom == 1007 &&
            app->currentPage == Page::FinancialGoals) {
            app->handleAction(ACTION_GOAL_CLIENT);
            return 0;
        }

        break;
    }


    case WM_LBUTTONUP:
    {
        POINT point = {

            static_cast<short>(
                LOWORD(
                    lParam)),

            static_cast<short>(
                HIWORD(
                    lParam))
        };


        for (
            const ClickArea& area :
            app->clickAreas) {

            if (
                PtInRect(
                    &area.rectangle,
                    point)) {

                app->handleAction(
                    area.action);

                return 0;
            }
        }


        return 0;
    }


    case WM_SETCURSOR:
    {
        if (
            LOWORD(
                lParam) ==
            HTCLIENT) {

            POINT cursor = {};


            GetCursorPos(
                &cursor);


            ScreenToClient(
                window,
                &cursor);


            for (
                const ClickArea& area :
                app->clickAreas) {

                if (
                    PtInRect(
                        &area.rectangle,
                        cursor)) {

                    SetCursor(
                        LoadCursorW(
                            nullptr,
                            IDC_HAND));

                    return TRUE;
                }
            }
        }


        break;
    }


    case WM_PAINT:
    {
        PAINTSTRUCT paint = {};


        HDC deviceContext =
            BeginPaint(
                window,
                &paint);


        {
            Graphics graphics(
                deviceContext);


            try {

                app->paint(
                    graphics);
            }


            catch (
                const std::exception& exception) {

                OutputDebugStringA(
                    exception.what());
            }
        }


        EndPaint(
            window,
            &paint);


        return 0;
    }


    case WM_DESTROY:

        PostQuitMessage(
            0);

        return 0;


    default:

        break;
    }


    return DefWindowProcW(
        window,
        message,
        wParam,
        lParam);
}


// ============================================================
// WINDOWS ENTRY POINT
// ============================================================

int WINAPI wWinMain(
    HINSTANCE instance,
    HINSTANCE,
    LPWSTR,
    int) {

    INITCOMMONCONTROLSEX controls = {};


    controls.dwSize =
        sizeof(
            controls);


    controls.dwICC =
        ICC_LISTVIEW_CLASSES |
        ICC_STANDARD_CLASSES;


    InitCommonControlsEx(
        &controls);


    GdiplusStartupInput startupInput;


    ULONG_PTR gdiplusToken =
        0;


    if (
        GdiplusStartup(
            &gdiplusToken,
            &startupInput,
            nullptr) !=
        Ok) {

        MessageBoxW(
            nullptr,
            L"Unable to initialize graphical interface.",
            L"Aurelia Wealth Studio Startup Error",
            MB_OK |
            MB_ICONERROR);

        return 1;
    }


    int exitCode =
        0;


    try {

        AureliaApplication app(
            instance);


        app.createMainWindow();


        MSG message = {};


        while (
            GetMessageW(
                &message,
                nullptr,
                0,
                0) >
            0) {

            TranslateMessage(
                &message);


            DispatchMessageW(
                &message);
        }


        exitCode =
            static_cast<int>(
                message.wParam);
    }


    catch (
        const std::exception& exception) {

        MessageBoxW(
            nullptr,
            widen(
                exception.what())
            .c_str(),
            L"Aurelia Wealth Studio Startup Error",
            MB_OK |
            MB_ICONERROR);


        exitCode =
            1;
    }


    GdiplusShutdown(
        gdiplusToken);


    return exitCode;
}