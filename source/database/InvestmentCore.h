#pragma once

/*
 * AURELIA WEALTH STUDIO
 * CS 499 Computer Science Capstone
 * Milestone Four: Enhancement Three, Databases
 * Student: Brianna Reed
 *
 * Persistent SQLite backend and relational portfolio data layer
 * Revision: additive migrations, safe idempotent demo seeding,
 * historic data preservation, and database backup utilities
 *
 * All financial records, market prices, transactions,
 * securities, and client profiles are fictional
 * demonstration data
 */

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "sqlite3.h"

using std::optional;
using std::string;
using std::vector;


// ============================================================
// CONSTANTS
// ============================================================

namespace Constants {

    constexpr int kMinAge =
        18;

    constexpr int kMaxAge =
        120;

    constexpr double kMaxBalance =
        1000000000.0;
}


// ============================================================
// CLIENT MODELS
// ============================================================

enum class ServiceType {
    Brokerage = 1,
    Retirement = 2
};


enum class RiskLevel {
    Conservative = 1,
    Moderate = 2,
    Aggressive = 3
};


struct Client {

    int id = 0;

    string name;

    ServiceType service =
        ServiceType::Brokerage;

    int age =
        18;

    double accountBalance =
        0.0;

    RiskLevel riskLevel =
        RiskLevel::Moderate;

    string email;

    string phone;

    bool active =
        true;

    string householdName;

    string advisorName;

    string investmentObjective;

    double annualIncome =
        0.0;

    double netWorth =
        0.0;

    string liquidityNeed;

    int timeHorizonYears =
        10;

    string createdAt;

    string updatedAt;
};


struct Account {

    int id = 0;

    int clientId = 0;

    string accountName;

    string accountType;

    string custodian;

    string taxTreatment;

    bool active = true;

    string openedAt;
};


struct Security {

    string symbol;

    string name;

    string assetClass;

    string sector;

    double mockPrice = 0.0;

    double expenseRatio = 0.0;
};


struct Holding {

    int id = 0;

    int accountId = 0;

    string symbol;

    string securityName;

    string assetClass;

    string sector;

    double quantity = 0.0;

    double price = 0.0;

    double costBasisPerShare = 0.0;

    double marketValue = 0.0;

    double unrealizedGain = 0.0;
};


struct TransactionRecord {

    int id = 0;

    int accountId = 0;

    string date;

    string type;

    string symbol;

    double quantity = 0.0;

    double amount = 0.0;

    string description;
};


struct PortfolioSnapshot {

    string date;

    double value = 0.0;

    double contributions = 0.0;

    double withdrawals = 0.0;
};


struct FinancialGoal {

    int id = 0;

    int clientId = 0;

    string name;

    string type;

    double targetAmount = 0.0;

    double currentAmount = 0.0;

    string targetDate;

    int priority = 2;

    string status;
};


struct AdvisorTask {
    string description, assignedTo, status = "Open";

    int id = 0;

    optional<int> clientId;

    string title;

    string category;

    string dueDate;

    string priority;

    bool completed = false;
};


struct AdvisorNote {

    int id = 0;

    int clientId = 0;

    string noteText;

    string createdAt;
};


// ============================================================
// ENUM HELPERS
// ============================================================

inline string serviceTypeToString(
    ServiceType value) {

    return value ==
        ServiceType::Brokerage
        ? "Brokerage"
        : "Retirement";
}


inline string riskLevelToString(
    RiskLevel value) {

    switch (value) {

    case RiskLevel::Conservative:
        return "Conservative";

    case RiskLevel::Moderate:
        return "Moderate";

    default:
        return "Aggressive";
    }
}


// ============================================================
// LOGGER
// ============================================================

class Logger {
private:

    string file_;

public:

    explicit Logger(
        string file)
        :
        file_(
            std::move(file)) {
    }

    void write(
        const string& message) const {

        std::ofstream out(
            file_,
            std::ios::app);

        if (out) {

            out
                << message
                << '\n';
        }
    }
};


// ============================================================
// VALIDATION
// ============================================================

class InputValidator {
public:

    static string trim(
        const string& value) {

        const size_t first =
            value.find_first_not_of(
                " \t\r\n");

        if (first ==
            string::npos) {

            return "";
        }

        const size_t last =
            value.find_last_not_of(
                " \t\r\n");

        return value.substr(
            first,
            last -
            first +
            1);
    }
};


// ============================================================
// DATABASE EXCEPTION
// ============================================================

class DatabaseException :
    public std::runtime_error {

public:

    explicit DatabaseException(
        const string& message)
        :
        std::runtime_error(
            message) {
    }
};


// ============================================================
// PREPARED SQL STATEMENT
// ============================================================

class Statement {
private:

    sqlite3* database_ =
        nullptr;

    sqlite3_stmt* statement_ =
        nullptr;


    void require(
        int result) const {

        if (result !=
            SQLITE_OK) {

            throw DatabaseException(
                sqlite3_errmsg(
                    database_));
        }
    }


public:

    Statement(
        sqlite3* database,
        const string& sql)
        :
        database_(
            database) {

        require(
            sqlite3_prepare_v2(
                database_,
                sql.c_str(),
                -1,
                &statement_,
                nullptr));
    }


    ~Statement() {

        if (statement_) {

            sqlite3_finalize(
                statement_);
        }
    }


    Statement(
        const Statement&) =
        delete;


    Statement& operator=(
        const Statement&) =
        delete;


    void bindInt(
        int index,
        int value) {

        require(
            sqlite3_bind_int(
                statement_,
                index,
                value));
    }


    void bindDouble(
        int index,
        double value) {

        require(
            sqlite3_bind_double(
                statement_,
                index,
                value));
    }


    void bindText(
        int index,
        const string& value) {

        require(
            sqlite3_bind_text(
                statement_,
                index,
                value.c_str(),
                -1,
                SQLITE_TRANSIENT));
    }


    void bindNull(
        int index) {

        require(
            sqlite3_bind_null(
                statement_,
                index));
    }


    int step() {

        const int result = sqlite3_step(
            statement_);

        if (result != SQLITE_ROW &&
            result != SQLITE_DONE) {

            throw DatabaseException(
                sqlite3_errmsg(database_));
        }

        return result;
    }


    void executeDone() {

        if (step() !=
            SQLITE_DONE) {

            throw DatabaseException(
                sqlite3_errmsg(
                    database_));
        }
    }


    int columnInt(
        int index) const {

        return sqlite3_column_int(
            statement_,
            index);
    }


    double columnDouble(
        int index) const {

        return sqlite3_column_double(
            statement_,
            index);
    }


    string columnText(
        int index) const {

        const auto* value =
            sqlite3_column_text(
                statement_,
                index);

        return value
            ? reinterpret_cast<
            const char*>(
                value)
            : "";
    }


    bool columnIsNull(
        int index) const {

        return sqlite3_column_type(
            statement_,
            index) ==
            SQLITE_NULL;
    }


    int columnCount() const {

        return sqlite3_column_count(
            statement_);
    }


    string columnName(
        int index) const {

        const char* name =
            sqlite3_column_name(
                statement_,
                index);

        return name
            ? name
            : "";
    }
};


// ============================================================
// DATABASE CONNECTION
// ============================================================

class Database {
private:

    sqlite3* database_ =
        nullptr;

    string path_;


public:

    explicit Database(
        string path)
        :
        path_(
            std::move(path)) {

        const int result =
            sqlite3_open_v2(
                path_.c_str(),
                &database_,
                SQLITE_OPEN_READWRITE |
                SQLITE_OPEN_CREATE,
                nullptr);

        if (result !=
            SQLITE_OK) {

            string reason =
                database_
                ? sqlite3_errmsg(
                    database_)
                : "Unable to create SQLite connection";

            if (database_) {

                sqlite3_close(
                    database_);
            }

            database_ =
                nullptr;

            throw DatabaseException(
                "Database open failed: " +
                reason);
        }

        sqlite3_busy_timeout(
            database_,
            5000);

        execute(
            "PRAGMA foreign_keys=ON;");

        execute(
            "PRAGMA journal_mode=WAL;");

        execute(
            "PRAGMA synchronous=NORMAL;");
    }


    ~Database() {

        if (database_) {

            sqlite3_close(
                database_);
        }
    }


    Database(
        const Database&) =
        delete;


    Database& operator=(
        const Database&) =
        delete;


    sqlite3* handle() const {

        return database_;
    }


    const string& fileName() const {

        return path_;
    }


    void execute(
        const string& sql) const {

        char* error =
            nullptr;

        const int result =
            sqlite3_exec(
                database_,
                sql.c_str(),
                nullptr,
                nullptr,
                &error);

        if (result !=
            SQLITE_OK) {

            string message =
                error
                ? error
                : sqlite3_errmsg(
                    database_);

            sqlite3_free(
                error);

            throw DatabaseException(
                "SQL error: " +
                message);
        }
    }


    // SQLite Online Backup API handles active WAL databases safely
    // Always use a distinct destination name for a fresh snapshot
    void backupTo(const string& destination) const {

        if (destination == path_) {
            throw DatabaseException(
                "Database backup must use a different file name.");
        }

        sqlite3* target = nullptr;
        const int opened = sqlite3_open_v2(
            destination.c_str(), &target,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
            nullptr);

        if (opened != SQLITE_OK) {
            string detail = target
                ? sqlite3_errmsg(target)
                : "Unable to open backup destination";
            if (target) sqlite3_close(target);
            throw DatabaseException("Backup failed: " + detail);
        }

        sqlite3_backup* backup = sqlite3_backup_init(
            target, "main", database_, "main");

        if (!backup) {
            string detail = sqlite3_errmsg(target);
            sqlite3_close(target);
            throw DatabaseException("Backup failed: " + detail);
        }

        const int result = sqlite3_backup_step(backup, -1);
        const int finish = sqlite3_backup_finish(backup);
        const int close = sqlite3_close(target);

        if (result != SQLITE_DONE ||
            finish != SQLITE_OK ||
            close != SQLITE_OK) {
            throw DatabaseException("SQLite backup did not complete.");
        }
    }


    int changes() const {

        return sqlite3_changes(
            database_);
    }


    int lastId() const {

        return static_cast<int>(
            sqlite3_last_insert_rowid(
                database_));
    }
};


// ============================================================
// TRANSACTION
// ============================================================

class Transaction {
private:

    Database& database_;

    bool active_ =
        false;


public:

    explicit Transaction(
        Database& database)
        :
        database_(
            database) {

        database_.execute(
            "BEGIN IMMEDIATE;");

        active_ =
            true;
    }


    ~Transaction() {

        if (active_) {

            try {

                database_.execute(
                    "ROLLBACK;");
            }

            catch (...) {

                /*
                 * Never throw from
                 * a destructor
                 */
            }
        }
    }


    void commit() {

        database_.execute(
            "COMMIT;");

        active_ =
            false;
    }


    void rollback() {

        database_.execute(
            "ROLLBACK;");

        active_ =
            false;
    }
};


// ============================================================
// CLIENT / PORTFOLIO REPOSITORY
// ============================================================

struct PlanningProfile {
    int clientId = 0;
    string occupation, employer, location, contactPreference = "Email";
    int dependents = 0, retirementAge = 65;
    double annualExpenses = 0, emergencyReserve = 0, monthlySaving = 0;
    string reviewDate, planningContext;
};

class ClientRepository {
private:

    Database& database_;


    static string normalized(
        string value) {

        value =
            InputValidator::trim(
                value);

        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char character) {

                return static_cast<char>(
                    std::tolower(
                        character));
            });

        return value;
    }


    static string escapeLike(
        const string& value) {

        string output;

        for (char character :
        value) {

            if (
                character == '%' ||
                character == '_' ||
                character == '\\') {

                output.push_back(
                    '\\');
            }

            output.push_back(
                character);
        }

        return output;
    }


    static string clientColumns() {

        return
            "id,name,service_id,age,account_balance,"
            "risk_level_id,email,phone,active,"
            "household_name,advisor_name,investment_objective,"
            "annual_income,net_worth,liquidity_need,"
            "time_horizon_years,created_at,updated_at";
    }


    static Client readClient(
        const Statement& statement) {

        Client client;

        client.id =
            statement.columnInt(
                0);

        client.name =
            statement.columnText(
                1);

        client.service =
            static_cast<ServiceType>(
                statement.columnInt(
                    2));

        client.age =
            statement.columnInt(
                3);

        client.accountBalance =
            statement.columnDouble(
                4);

        client.riskLevel =
            static_cast<RiskLevel>(
                statement.columnInt(
                    5));

        client.email =
            statement.columnText(
                6);

        client.phone =
            statement.columnText(
                7);

        client.active =
            statement.columnInt(
                8) !=
            0;

        client.householdName =
            statement.columnText(
                9);

        client.advisorName =
            statement.columnText(
                10);

        client.investmentObjective =
            statement.columnText(
                11);

        client.annualIncome =
            statement.columnDouble(
                12);

        client.netWorth =
            statement.columnDouble(
                13);

        client.liquidityNeed =
            statement.columnText(
                14);

        client.timeHorizonYears =
            statement.columnInt(
                15);

        client.createdAt =
            statement.columnText(
                16);

        client.updatedAt =
            statement.columnText(
                17);

        return client;
    }


    vector<Client> collectClients(
        Statement& statement) const {

        vector<Client> results;

        int result =
            SQLITE_OK;

        while (
            (result =
                statement.step()) ==
            SQLITE_ROW) {

            results.push_back(
                readClient(
                    statement));
        }

        if (result !=
            SQLITE_DONE) {

            throw DatabaseException(
                sqlite3_errmsg(
                    database_.handle()));
        }

        return results;
    }


    vector<Client> queryClients(
        const string& sql) const {

        Statement statement(
            database_.handle(),
            sql);

        return collectClients(
            statement);
    }


    bool columnExists(
        const string& table,
        const string& column) const {

        Statement statement(
            database_.handle(),
            "PRAGMA table_info(" +
            table +
            ");");

        while (
            statement.step() ==
            SQLITE_ROW) {

            if (
                statement.columnText(
                    1) ==
                column) {

                return true;
            }
        }

        return false;
    }


    void ensureColumn(
        const string& table,
        const string& column,
        const string& definition) {

        if (
            !columnExists(
                table,
                column)) {

            database_.execute(
                "ALTER TABLE " +
                table +
                " ADD COLUMN " +
                column +
                " " +
                definition +
                ";");
        }
    }


    void audit(
        const string& eventType,
        optional<int> clientId,
        const string& details) {

        Statement statement(
            database_.handle(),
            "INSERT INTO audit_events"
            "(event_type,client_id,details) "
            "VALUES(?,?,?);");

        statement.bindText(
            1,
            eventType);

        if (clientId) {

            statement.bindInt(
                2,
                *clientId);
        }

        else {

            statement.bindNull(
                2);
        }

        statement.bindText(
            3,
            details);

        statement.executeDone();
    }


    void initializeSchema() {

        database_.execute(R"SQL(

CREATE TABLE IF NOT EXISTS schema_metadata (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS service_types (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE
);

CREATE TABLE IF NOT EXISTS risk_levels (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE
);

CREATE TABLE IF NOT EXISTS clients (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    normalized_name TEXT NOT NULL,
    service_id INTEGER NOT NULL,
    age INTEGER NOT NULL,
    account_balance REAL NOT NULL,
    risk_level_id INTEGER NOT NULL,
    email TEXT NOT NULL UNIQUE COLLATE NOCASE,
    phone TEXT NOT NULL,
    active INTEGER NOT NULL DEFAULT 1,
    household_name TEXT NOT NULL DEFAULT '',
    advisor_name TEXT NOT NULL DEFAULT '',
    investment_objective TEXT NOT NULL DEFAULT '',
    annual_income REAL NOT NULL DEFAULT 0,
    net_worth REAL NOT NULL DEFAULT 0,
    liquidity_need TEXT NOT NULL DEFAULT '',
    time_horizon_years INTEGER NOT NULL DEFAULT 10,
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS accounts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    client_id INTEGER NOT NULL,
    account_name TEXT NOT NULL,
    account_type TEXT NOT NULL,
    custodian TEXT NOT NULL DEFAULT 'Aurelia Demo Custody',
    tax_treatment TEXT NOT NULL DEFAULT 'Taxable',
    active INTEGER NOT NULL DEFAULT 1,
    opened_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(client_id)
        REFERENCES clients(id)
        ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS securities (
    symbol TEXT PRIMARY KEY,
    security_name TEXT NOT NULL,
    asset_class TEXT NOT NULL,
    sector TEXT NOT NULL DEFAULT '',
    mock_price REAL NOT NULL,
    expense_ratio REAL NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS holdings (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    account_id INTEGER NOT NULL,
    symbol TEXT NOT NULL,
    quantity REAL NOT NULL,
    cost_basis_per_share REAL NOT NULL,
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(account_id)
        REFERENCES accounts(id)
        ON DELETE CASCADE,
    FOREIGN KEY(symbol)
        REFERENCES securities(symbol),
    UNIQUE(account_id,symbol)
);

CREATE TABLE IF NOT EXISTS transactions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    account_id INTEGER NOT NULL,
    transaction_date TEXT NOT NULL,
    transaction_type TEXT NOT NULL,
    symbol TEXT,
    quantity REAL NOT NULL DEFAULT 0,
    amount REAL NOT NULL,
    description TEXT NOT NULL DEFAULT '',
    FOREIGN KEY(account_id)
        REFERENCES accounts(id)
        ON DELETE CASCADE,
    FOREIGN KEY(symbol)
        REFERENCES securities(symbol)
);

CREATE TABLE IF NOT EXISTS portfolio_snapshots (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    client_id INTEGER NOT NULL,
    snapshot_date TEXT NOT NULL,
    portfolio_value REAL NOT NULL,
    contributions_to_date REAL NOT NULL DEFAULT 0,
    withdrawals_to_date REAL NOT NULL DEFAULT 0,
    FOREIGN KEY(client_id)
        REFERENCES clients(id)
        ON DELETE CASCADE,
    UNIQUE(client_id,snapshot_date)
);

CREATE TABLE IF NOT EXISTS financial_goals (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    client_id INTEGER NOT NULL,
    goal_name TEXT NOT NULL,
    goal_type TEXT NOT NULL,
    target_amount REAL NOT NULL,
    current_amount REAL NOT NULL DEFAULT 0,
    target_date TEXT NOT NULL,
    priority INTEGER NOT NULL DEFAULT 2,
    status TEXT NOT NULL DEFAULT 'Active',
    FOREIGN KEY(client_id)
        REFERENCES clients(id)
        ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS advisor_notes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    client_id INTEGER NOT NULL,
    note_text TEXT NOT NULL,
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(client_id)
        REFERENCES clients(id)
        ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS advisor_tasks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    client_id INTEGER,
    title TEXT NOT NULL,
    category TEXT NOT NULL DEFAULT 'Review',
    due_date TEXT NOT NULL,
    priority TEXT NOT NULL DEFAULT 'Medium',
    completed INTEGER NOT NULL DEFAULT 0,
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(client_id)
        REFERENCES clients(id)
        ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS audit_events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    event_type TEXT NOT NULL,
    client_id INTEGER,
    details TEXT NOT NULL DEFAULT '',
    occurred_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_accounts_client
    ON accounts(client_id);

CREATE INDEX IF NOT EXISTS idx_holdings_account
    ON holdings(account_id);

CREATE INDEX IF NOT EXISTS idx_transactions_account_date
    ON transactions(
        account_id,
        transaction_date DESC
    );

CREATE INDEX IF NOT EXISTS idx_snapshots_client_date
    ON portfolio_snapshots(
        client_id,
        snapshot_date
    );

CREATE INDEX IF NOT EXISTS idx_goals_client
    ON financial_goals(client_id);

CREATE INDEX IF NOT EXISTS idx_tasks_due
    ON advisor_tasks(
        completed,
        due_date
    );

CREATE TABLE IF NOT EXISTS client_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    action_type TEXT NOT NULL,
    client_id INTEGER NOT NULL,
    before_name TEXT,
    before_service_id INTEGER,
    before_age INTEGER,
    before_balance REAL,
    before_risk_id INTEGER,
    before_email TEXT,
    before_phone TEXT,
    before_active INTEGER,
    after_name TEXT,
    after_service_id INTEGER,
    after_age INTEGER,
    after_balance REAL,
    after_risk_id INTEGER,
    after_email TEXT,
    after_phone TEXT,
    after_active INTEGER,
    undone INTEGER NOT NULL DEFAULT 0,
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_clients_normalized_name
    ON clients(normalized_name);
CREATE INDEX IF NOT EXISTS idx_clients_service
    ON clients(service_id);
CREATE INDEX IF NOT EXISTS idx_clients_risk
    ON clients(risk_level_id);
CREATE INDEX IF NOT EXISTS idx_clients_active
    ON clients(active);
CREATE INDEX IF NOT EXISTS idx_clients_balance
    ON clients(account_balance DESC);
CREATE INDEX IF NOT EXISTS idx_holdings_symbol
    ON holdings(symbol);
CREATE INDEX IF NOT EXISTS idx_notes_client
    ON advisor_notes(client_id,created_at DESC);
CREATE INDEX IF NOT EXISTS idx_history_undo
    ON client_history(undone,id DESC);
CREATE INDEX IF NOT EXISTS idx_audit_occurred_at
    ON audit_events(occurred_at DESC,id DESC);

)SQL");


        /*
         * Runtime migration for existing Milestone Four data
         * Never blindly copy a WAL file or reset populated tables
         * Preserve the first available pre migration SQLite snapshot
         */

        if (!columnExists("clients", "household_name") ||
            !columnExists("clients", "advisor_name") ||
            !columnExists("clients", "investment_objective") ||
            !columnExists("clients", "annual_income") ||
            !columnExists("clients", "net_worth") ||
            !columnExists("clients", "liquidity_need") ||
            !columnExists("clients", "time_horizon_years")) {

            const string backupName =
                database_.fileName() + ".before_schema_migration.bak";

            std::ifstream existingBackup(backupName, std::ios::binary);
            if (!existingBackup.good()) {
                database_.backupTo(backupName);
            }
        }

        ensureColumn(
            "clients",
            "household_name",
            "TEXT NOT NULL DEFAULT ''");

        ensureColumn(
            "clients",
            "advisor_name",
            "TEXT NOT NULL DEFAULT ''");

        ensureColumn(
            "clients",
            "investment_objective",
            "TEXT NOT NULL DEFAULT ''");

        ensureColumn(
            "clients",
            "annual_income",
            "REAL NOT NULL DEFAULT 0");

        ensureColumn(
            "clients",
            "net_worth",
            "REAL NOT NULL DEFAULT 0");

        ensureColumn(
            "clients",
            "liquidity_need",
            "TEXT NOT NULL DEFAULT ''");

        ensureColumn(
            "clients",
            "time_horizon_years",
            "INTEGER NOT NULL DEFAULT 10");

        ensureColumn("advisor_tasks", "description", "TEXT NOT NULL DEFAULT ''");
        ensureColumn("advisor_tasks", "assigned_to", "TEXT NOT NULL DEFAULT ''");
        ensureColumn("advisor_tasks", "workflow_status", "TEXT NOT NULL DEFAULT 'Open'");
        database_.execute(R"SQL(
CREATE TABLE IF NOT EXISTS client_profile_details (
 client_id INTEGER PRIMARY KEY REFERENCES clients(id) ON DELETE CASCADE,
 occupation TEXT NOT NULL DEFAULT '', employer TEXT NOT NULL DEFAULT '',
 location TEXT NOT NULL DEFAULT '', contact_preference TEXT NOT NULL DEFAULT 'Email',
 dependents INTEGER NOT NULL DEFAULT 0 CHECK(dependents BETWEEN 0 AND 20),
 retirement_age INTEGER NOT NULL DEFAULT 65 CHECK(retirement_age BETWEEN 18 AND 100),
 annual_expenses REAL NOT NULL DEFAULT 0 CHECK(annual_expenses>=0),
 emergency_reserve REAL NOT NULL DEFAULT 0 CHECK(emergency_reserve>=0),
 monthly_saving REAL NOT NULL DEFAULT 0 CHECK(monthly_saving>=0),
 review_date TEXT NOT NULL DEFAULT '', planning_context TEXT NOT NULL DEFAULT '',
 updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE TABLE IF NOT EXISTS market_quotes (
 symbol TEXT PRIMARY KEY REFERENCES securities(symbol),
 price REAL NOT NULL CHECK(price>0), as_of_date TEXT NOT NULL,
 source TEXT NOT NULL, fetched_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
INSERT INTO schema_metadata(key,value) VALUES('planning_schema_v1','1')
 ON CONFLICT(key) DO NOTHING;
)SQL");

        // Indices on migrated columns must be created after ALTER TABLE
        database_.execute(R"SQL(
CREATE INDEX IF NOT EXISTS idx_client_household
    ON clients(household_name);
CREATE INDEX IF NOT EXISTS idx_client_advisor
    ON clients(advisor_name);
CREATE INDEX IF NOT EXISTS idx_goals_target_date
    ON financial_goals(target_date);
CREATE INDEX IF NOT EXISTS idx_tasks_client_status
    ON advisor_tasks(client_id,completed,due_date);

CREATE VIEW IF NOT EXISTS v_client_portfolio_summary AS
SELECT c.id AS client_id,c.name AS client_name,
       c.household_name,c.advisor_name,c.active,
       COALESCE(a.account_count,0) AS account_count,
       COALESCE(p.holding_count,0) AS holding_count,
       COALESCE(p.modeled_market_value,0) AS modeled_market_value,
       COALESCE(p.modeled_cost_basis,0) AS modeled_cost_basis,
       COALESCE(p.unrealized_change,0) AS unrealized_change
FROM clients AS c
LEFT JOIN (
    SELECT client_id,COUNT(*) AS account_count
    FROM accounts GROUP BY client_id
) AS a ON a.client_id=c.id
LEFT JOIN (
    SELECT ac.client_id,COUNT(*) AS holding_count,
        SUM(h.quantity*s.mock_price) AS modeled_market_value,
        SUM(h.quantity*h.cost_basis_per_share) AS modeled_cost_basis,
        SUM(h.quantity*(s.mock_price-h.cost_basis_per_share))
            AS unrealized_change
    FROM accounts AS ac
    JOIN holdings AS h ON h.account_id=ac.id
    JOIN securities AS s ON s.symbol=h.symbol
    GROUP BY ac.client_id
) AS p ON p.client_id=c.id;

CREATE VIEW IF NOT EXISTS v_asset_allocation AS
SELECT a.client_id,s.asset_class,
       SUM(h.quantity*s.mock_price) AS modeled_value
FROM accounts AS a
JOIN holdings AS h ON h.account_id=a.id
JOIN securities AS s ON s.symbol=h.symbol
GROUP BY a.client_id,s.asset_class;

CREATE VIEW IF NOT EXISTS v_open_advisor_work_queue AS
SELECT t.id AS task_id,t.client_id,
       COALESCE(c.name,'Enterprise') AS client_name,
       t.title,t.category,t.due_date,t.priority,
       CASE WHEN date(t.due_date)<date('now','-7 hours') THEN 1 ELSE 0 END
           AS overdue
FROM advisor_tasks AS t
LEFT JOIN clients AS c ON c.id=t.client_id
WHERE t.completed=0;
)SQL");

        database_.execute(
            "INSERT INTO schema_metadata("
            "key,value) "
            "VALUES('schema_version','2') "
            "ON CONFLICT(key) "
            "DO UPDATE SET value='2';");
    }


    bool clientEmailExists(
        const string& email) const {

        Statement statement(
            database_.handle(),
            "SELECT 1 "
            "FROM clients "
            "WHERE email=? "
            "LIMIT 1;");

        statement.bindText(
            1,
            email);

        return statement.step() ==
            SQLITE_ROW;
    }


    int insertClient(
        const Client& client) {

        Statement statement(
            database_.handle(),
            "INSERT INTO clients("
            "name,normalized_name,service_id,age,"
            "account_balance,risk_level_id,email,phone,"
            "active,household_name,advisor_name,"
            "investment_objective,annual_income,net_worth,"
            "liquidity_need,time_horizon_years)"
            "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);");

        statement.bindText(
            1,
            client.name);

        statement.bindText(
            2,
            normalized(
                client.name));

        statement.bindInt(
            3,
            static_cast<int>(
                client.service));

        statement.bindInt(
            4,
            client.age);

        statement.bindDouble(
            5,
            client.accountBalance);

        statement.bindInt(
            6,
            static_cast<int>(
                client.riskLevel));

        statement.bindText(
            7,
            client.email);

        statement.bindText(
            8,
            client.phone);

        statement.bindInt(
            9,
            client.active
            ? 1
            : 0);

        statement.bindText(
            10,
            client.householdName);

        statement.bindText(
            11,
            client.advisorName);

        statement.bindText(
            12,
            client.investmentObjective);

        statement.bindDouble(
            13,
            client.annualIncome);

        statement.bindDouble(
            14,
            client.netWorth);

        statement.bindText(
            15,
            client.liquidityNeed);

        statement.bindInt(
            16,
            client.timeHorizonYears);

        statement.executeDone();

        return database_.lastId();
    }


    void seedClients() {

        database_.execute(
            "INSERT OR IGNORE "
            "INTO service_types "
            "VALUES"
            "(1,'Brokerage'),"
            "(2,'Retirement');");

        database_.execute(
            "INSERT OR IGNORE "
            "INTO risk_levels "
            "VALUES"
            "(1,'Conservative'),"
            "(2,'Moderate'),"
            "(3,'Aggressive');");


        // Seed an empty installation only; never recreate an edited identity
        { Statement existing(database_.handle(), "SELECT COUNT(*) FROM clients;");
          if (existing.step()==SQLITE_ROW && existing.columnInt(0)>0) return; }

        const vector<Client> demo = {

            {
                0,
                "Bob Jones",
                ServiceType::Brokerage,
                42,
                148250,
                RiskLevel::Moderate,
                "bob.jones@example.com",
                "520-555-0101",
                true,
                "Jones Household",
                "Avery Bennett",
                "Balanced growth",
                128000,
                540000,
                "Moderate",
                14
            },

            {
                0,
                "Sarah Davis",
                ServiceType::Retirement,
                57,
                286400,
                RiskLevel::Conservative,
                "sarah.davis@example.com",
                "520-555-0102",
                true,
                "Davis Household",
                "Avery Bennett",
                "Retirement income",
                164000,
                980000,
                "Low",
                8
            },

            {
                0,
                "Amy Friendly",
                ServiceType::Brokerage,
                31,
                72450,
                RiskLevel::Aggressive,
                "amy.friendly@example.com",
                "520-555-0103",
                true,
                "Friendly Household",
                "Noah Sinclair",
                "Long term growth",
                94000,
                245000,
                "Moderate",
                28
            },

            {
                0,
                "Johnny Smith",
                ServiceType::Brokerage,
                48,
                391750,
                RiskLevel::Moderate,
                "johnny.smith@example.com",
                "520-555-0104",
                true,
                "Smith Household",
                "Mira Cole",
                "Growth and income",
                182000,
                1250000,
                "Moderate",
                16
            },

            {
                0,
                "Carol Spears",
                ServiceType::Retirement,
                63,
                512900,
                RiskLevel::Conservative,
                "carol.spears@example.com",
                "520-555-0105",
                true,
                "Spears Household",
                "Mira Cole",
                "Capital preservation",
                148000,
                1680000,
                "Low",
                6
            },

            {
                0,
                "Jordan Ellis",
                ServiceType::Brokerage,
                41,
                231500,
                RiskLevel::Moderate,
                "jordan.ellis@example.com",
                "520-555-0111",
                true,
                "Ellis Household",
                "Noah Sinclair",
                "Balanced growth",
                137000,
                720000,
                "Moderate",
                18
            },

            {
                0,
                "Morgan Reed",
                ServiceType::Retirement,
                56,
                428900,
                RiskLevel::Conservative,
                "morgan.reed@example.com",
                "520-555-0112",
                true,
                "Reed Household",
                "Avery Bennett",
                "Retirement readiness",
                156000,
                1320000,
                "Low",
                9
            },

            {
                0,
                "Taylor Hayes",
                ServiceType::Brokerage,
                33,
                119750,
                RiskLevel::Aggressive,
                "taylor.hayes@example.com",
                "520-555-0113",
                true,
                "Hayes Household",
                "Noah Sinclair",
                "Aggressive accumulation",
                112000,
                355000,
                "High",
                30
            },

            {
                0,
                "Cameron Blake",
                ServiceType::Brokerage,
                48,
                365000,
                RiskLevel::Moderate,
                "cameron.blake@example.com",
                "520-555-0114",
                true,
                "Blake Household",
                "Mira Cole",
                "Tax aware growth",
                201000,
                1460000,
                "Moderate",
                15
            },

            {
                0,
                "Avery Collins",
                ServiceType::Retirement,
                62,
                547200,
                RiskLevel::Conservative,
                "avery.collins@example.com",
                "520-555-0115",
                true,
                "Collins Household",
                "Avery Bennett",
                "Income and preservation",
                172000,
                1920000,
                "Low",
                5
            },

            {
                0,
                "Drew Parker",
                ServiceType::Brokerage,
                29,
                83750,
                RiskLevel::Aggressive,
                "drew.parker@example.com",
                "520-555-0116",
                true,
                "Parker Household",
                "Noah Sinclair",
                "Long horizon growth",
                88000,
                215000,
                "Moderate",
                32
            },

            {
                0,
                "Quinn Harper",
                ServiceType::Retirement,
                51,
                306480,
                RiskLevel::Moderate,
                "quinn.harper@example.com",
                "520-555-0117",
                true,
                "Harper Household",
                "Mira Cole",
                "Balanced retirement growth",
                149000,
                870000,
                "Moderate",
                13
            },

            {
                0,
                "Riley Brooks",
                ServiceType::Brokerage,
                39,
                192600,
                RiskLevel::Moderate,
                "riley.brooks@example.com",
                "520-555-0118",
                false,
                "Brooks Household",
                "Noah Sinclair",
                "Balanced growth",
                121000,
                610000,
                "High",
                20
            },

            {
                0,
                "Jamie Monroe",
                ServiceType::Retirement,
                60,
                641000,
                RiskLevel::Conservative,
                "jamie.monroe@example.com",
                "520-555-0119",
                true,
                "Monroe Household",
                "Avery Bennett",
                "Retirement income",
                189000,
                2240000,
                "Low",
                7
            },

            {
                0,
                "Alex Morgan",
                ServiceType::Brokerage,
                45,
                278950,
                RiskLevel::Aggressive,
                "alex.morgan@example.com",
                "520-555-0120",
                true,
                "Morgan Household",
                "Mira Cole",
                "Growth",
                176000,
                990000,
                "Moderate",
                19
            }
        };


        for (const Client& client :
            demo) {

            // Preserve the canonical record if it already exists
            // On older demo databases, this same fictional person may
            // instead have a demo.investorN@aurelia.example email
            // Match only the known old demo pattern with exact identity
            // fields; never merge or delete ordinary user entered data
            int matchedId = 0;
            {
                Statement canonical(
                    database_.handle(),
                    "SELECT id FROM clients WHERE email=? LIMIT 1;");
                canonical.bindText(1, client.email);
                if (canonical.step() == SQLITE_ROW) {
                    matchedId = canonical.columnInt(0);
                }
            }

            if (matchedId == 0) {
                Statement legacy(
                    database_.handle(),
                    "SELECT id FROM clients "
                    "WHERE email LIKE 'demo.investor%@aurelia.example' "
                    "AND normalized_name=? AND age=? "
                    "AND service_id=? AND risk_level_id=? "
                    "AND abs(account_balance-?)<0.01 "
                    "ORDER BY id LIMIT 1;");
                legacy.bindText(1, normalized(client.name));
                legacy.bindInt(2, client.age);
                legacy.bindInt(3, static_cast<int>(client.service));
                legacy.bindInt(4, static_cast<int>(client.riskLevel));
                legacy.bindDouble(5, client.accountBalance);
                if (legacy.step() == SQLITE_ROW) {
                    matchedId = legacy.columnInt(0);
                }
            }

            if (matchedId == 0) {
                insertClient(client);
            }
            else {
                // Enrich only blank legacy fields. Keep the original
                // client ID, email, balances and child relationships
                Statement update(
                    database_.handle(),
                    "UPDATE clients SET "
                    "household_name="
                    "CASE WHEN household_name='' "
                    "THEN ? ELSE household_name END,"
                    "advisor_name="
                    "CASE WHEN advisor_name='' "
                    "THEN ? ELSE advisor_name END,"
                    "investment_objective="
                    "CASE WHEN investment_objective='' "
                    "THEN ? ELSE investment_objective END,"
                    "annual_income="
                    "CASE WHEN annual_income=0 "
                    "THEN ? ELSE annual_income END,"
                    "net_worth="
                    "CASE WHEN net_worth=0 "
                    "THEN ? ELSE net_worth END,"
                    "liquidity_need="
                    "CASE WHEN liquidity_need='' "
                    "THEN ? ELSE liquidity_need END "
                    "WHERE id=?;");

                update.bindText(
                    1,
                    client.householdName);

                update.bindText(
                    2,
                    client.advisorName);

                update.bindText(
                    3,
                    client.investmentObjective);

                update.bindDouble(
                    4,
                    client.annualIncome);

                update.bindDouble(
                    5,
                    client.netWorth);

                update.bindText(
                    6,
                    client.liquidityNeed);

                update.bindInt(
                    7,
                    matchedId);

                update.executeDone();
            }
        }
    }


    void insertSecurity(
        const string& symbol,
        const string& name,
        const string& assetClass,
        const string& sector,
        double price,
        double expenseRatio) {

        Statement statement(
            database_.handle(),
            "INSERT OR IGNORE "
            "INTO securities("
            "symbol,security_name,asset_class,"
            "sector,mock_price,expense_ratio)"
            "VALUES(?,?,?,?,?,?);");

        statement.bindText(
            1,
            symbol);

        statement.bindText(
            2,
            name);

        statement.bindText(
            3,
            assetClass);

        statement.bindText(
            4,
            sector);

        statement.bindDouble(
            5,
            price);

        statement.bindDouble(
            6,
            expenseRatio);

        statement.executeDone();
    }


    void seedSecurities() {

        insertSecurity(
            "VTI",
            "Vanguard Total Stock Market ETF",
            "US Equity",
            "Broad Market",
            289.42,
            0.0003);

        insertSecurity(
            "VXUS",
            "Vanguard Total International Stock ETF",
            "International Equity",
            "Broad Market",
            72.18,
            0.0005);

        insertSecurity(
            "BND",
            "Vanguard Total Bond Market ETF",
            "Fixed Income",
            "Broad Bond",
            75.26,
            0.0003);

        insertSecurity(
            "SCHD",
            "Schwab US Dividend Equity ETF",
            "US Equity",
            "Dividend",
            31.84,
            0.0006);

        insertSecurity(
            "QQQ",
            "Invesco QQQ Trust",
            "US Equity",
            "Technology",
            602.17,
            0.0020);

        insertSecurity(
            "VNQ",
            "Vanguard Real Estate ETF",
            "Real Assets",
            "Real Estate",
            94.66,
            0.0013);

        insertSecurity(
            "BIL",
            "SPDR Bloomberg 1-3 Month T-Bill ETF",
            "Cash",
            "Treasury",
            91.44,
            0.0014);

        insertSecurity(
            "MSFT",
            "Microsoft Corporation",
            "US Equity",
            "Technology",
            518.39,
            0.0);

        insertSecurity(
            "AAPL",
            "Apple Inc.",
            "US Equity",
            "Technology",
            271.18,
            0.0);

        insertSecurity(
            "CASH",
            "Cash and Equivalents",
            "Cash",
            "Cash",
            1.0,
            0.0);
    }


    int insertAccount(
        int clientId,
        const string& name,
        const string& type,
        const string& taxTreatment) {

        Statement statement(
            database_.handle(),
            "INSERT INTO accounts("
            "client_id,account_name,account_type,"
            "custodian,tax_treatment,active)"
            "VALUES("
            "?,?,?,"
            "'Aurelia Demo Custody',"
            "?,1);");

        statement.bindInt(
            1,
            clientId);

        statement.bindText(
            2,
            name);

        statement.bindText(
            3,
            type);

        statement.bindText(
            4,
            taxTreatment);

        statement.executeDone();

        return database_.lastId();
    }


    void insertHolding(
        int accountId,
        const string& symbol,
        double marketValue,
        double costFactor) {

        Statement priceStatement(
            database_.handle(),
            "SELECT mock_price "
            "FROM securities "
            "WHERE symbol=?;");

        priceStatement.bindText(
            1,
            symbol);

        if (
            priceStatement.step() !=
            SQLITE_ROW) {

            throw DatabaseException(
                "Missing security: " +
                symbol);
        }

        const double price =
            priceStatement.columnDouble(
                0);

        const double quantity =
            marketValue /
            price;

        const double costBasis =
            price *
            costFactor;


        Statement statement(
            database_.handle(),
            "INSERT OR IGNORE "
            "INTO holdings("
            "account_id,symbol,quantity,"
            "cost_basis_per_share)"
            "VALUES(?,?,?,?);");

        statement.bindInt(
            1,
            accountId);

        statement.bindText(
            2,
            symbol);

        statement.bindDouble(
            3,
            quantity);

        statement.bindDouble(
            4,
            costBasis);

        statement.executeDone();
    }


    void seedHoldingsForAccount(
        int accountId,
        double accountValue,
        RiskLevel risk,
        int clientId,
        int accountIndex) {

        struct Allocation {

            const char* symbol;

            double percent;
        };


        vector<Allocation> mix;


        if (
            risk ==
            RiskLevel::Conservative) {

            mix = {

                {"VTI", 0.24},

                {"VXUS", 0.10},

                {"BND", 0.46},

                {"VNQ", 0.05},

                {"CASH", 0.15}
            };
        }

        else if (
            risk ==
            RiskLevel::Moderate) {

            mix = {

                {"VTI", 0.38},

                {"VXUS", 0.14},

                {"BND", 0.28},

                {"SCHD", 0.10},

                {"CASH", 0.10}
            };
        }

        else {

            mix = {

                {"VTI", 0.38},

                {"VXUS", 0.12},

                {"QQQ", 0.22},

                {"MSFT", 0.15},

                {"CASH", 0.05},

                {"VNQ", 0.08}
            };
        }


        /*
         * Deliberate fictional outliers allow
         * the analytics engine to surface
         * meaningful monitoring signals
         */

        if (
            clientId %
            5 ==
            0) {

            mix = {

                {"MSFT", 0.27},

                {"VTI", 0.33},

                {"BND", 0.24},

                {"VXUS", 0.08},

                {"CASH", 0.08}
            };
        }


        if (
            clientId %
            7 ==
            0) {

            mix = {

                {"VTI", 0.31},

                {"BND", 0.28},

                {"VXUS", 0.12},

                {"CASH", 0.21},

                {"VNQ", 0.08}
            };
        }


        const double adjustment =
            accountIndex ==
            0
            ? 1.0
            : 0.985;


        for (
            size_t index = 0;
            index < mix.size();
            ++index) {

            const double costFactor =
                0.83 +
                0.025 *
                static_cast<double>(
                    (
                        clientId +
                        accountIndex +
                        static_cast<int>(
                            index)
                        ) %
                    5);

            insertHolding(
                accountId,
                mix[index].symbol,
                accountValue *
                mix[index].percent *
                adjustment,
                costFactor);
        }
    }


    void seedRelationalData() {

        // The previous version dropped every child record whenever
        // relational_seed_v2 was absent. That destroys user entered
        // holdings, notes, transactions and advisor work
        // This revision NEVER clears existing business tables
        // Seed only clients that have no account records at all
        Transaction transaction(database_);

        const vector<Client> clients = getAllClients();

        vector<string> snapshotDates;
        {
            Statement dates(
                database_.handle(),
                "WITH RECURSIVE months(n) AS ("
                "SELECT 0 UNION ALL SELECT n+1 FROM months WHERE n<11) "
                "SELECT date('now','start of month',"
                "printf('-%d months',11-n)) FROM months ORDER BY n;");

            while (dates.step() == SQLITE_ROW) {
                snapshotDates.push_back(dates.columnText(0));
            }
        }

        int addedPortfolios = 0;

        for (
            const Client& client :
            clients) {

            Statement existingAccounts(
                database_.handle(),
                "SELECT 1 FROM accounts WHERE client_id=? LIMIT 1;");
            existingAccounts.bindInt(1, client.id);

            if (existingAccounts.step() == SQLITE_ROW) {
                // Preserve all linked records, even if the prior
                // demonstration seed marker is missing
                continue;
            }

            ++addedPortfolios;

            const bool retirement =
                client.service ==
                ServiceType::Retirement;


            const int accountOne =
                insertAccount(
                    client.id,
                    retirement
                    ? "Primary Retirement"
                    : "Core Brokerage",
                    retirement
                    ? "Traditional IRA"
                    : "Individual Brokerage",
                    retirement
                    ? "Tax Deferred"
                    : "Taxable");


            const int accountTwo =
                insertAccount(
                    client.id,
                    retirement
                    ? "Roth & Liquidity"
                    : "Opportunity Account",
                    retirement
                    ? "Roth IRA"
                    : "Joint Brokerage",
                    retirement
                    ? "Tax Free"
                    : "Taxable");


            const double firstValue =
                client.accountBalance *
                0.68;


            const double secondValue =
                client.accountBalance *
                0.32;


            seedHoldingsForAccount(
                accountOne,
                firstValue,
                client.riskLevel,
                client.id,
                0);


            seedHoldingsForAccount(
                accountTwo,
                secondValue,
                client.riskLevel,
                client.id,
                1);


            for (
                int accountId :
            {
                accountOne,
                    accountTwo
            }) {

                Statement deposit(
                    database_.handle(),
                    "INSERT INTO transactions("
                    "account_id,"
                    "transaction_date,"
                    "transaction_type,"
                    "symbol,"
                    "quantity,"
                    "amount,"
                    "description)"
                    "VALUES("
                    "?,?"
                    ",'Contribution',"
                    "NULL,"
                    "0,"
                    "?,?"
                    ");");


                deposit.bindInt(
                    1,
                    accountId);

                deposit.bindText(
                    2,
                    "2026-01-15");

                deposit.bindDouble(
                    3,
                    2500.0 +
                    client.id *
                    175.0);

                deposit.bindText(
                    4,
                    "Scheduled client contribution");

                deposit.executeDone();


                Statement dividend(
                    database_.handle(),
                    "INSERT INTO transactions("
                    "account_id,"
                    "transaction_date,"
                    "transaction_type,"
                    "symbol,"
                    "quantity,"
                    "amount,"
                    "description)"
                    "VALUES("
                    "?,?"
                    ",'Dividend',"
                    "'VTI',"
                    "0,"
                    "?,?"
                    ");");


                dividend.bindInt(
                    1,
                    accountId);

                dividend.bindText(
                    2,
                    "2026-06-28");

                dividend.bindDouble(
                    3,
                    315.0 +
                    client.id *
                    22.0);

                dividend.bindText(
                    4,
                    "Mock quarterly distribution");

                dividend.executeDone();
            }


            // Match the chart's latest modeled value to the actual
            // stored holdings, not a second invented balance factor
            double currentValue = 0.0;
            {
                Statement total(
                    database_.handle(),
                    "SELECT COALESCE(SUM(h.quantity*s.mock_price),0) "
                    "FROM holdings h "
                    "JOIN accounts a ON a.id=h.account_id "
                    "JOIN securities s ON s.symbol=h.symbol "
                    "WHERE a.client_id=?;");
                total.bindInt(1, client.id);
                if (total.step() == SQLITE_ROW) {
                    currentValue = total.columnDouble(0);
                }
            }


            for (
                size_t month = 0;
                month <
                snapshotDates.size();
                ++month) {

                const double progress =
                    static_cast<double>(
                        month) /
                    static_cast<double>(
                        snapshotDates.size() -
                        1);


                const double baseFactor =
                    0.885 +
                    0.115 *
                    progress;


                const double wobble =
                    (
                        (
                            client.id +
                            month
                            ) %
                        3 ==
                        0
                        )
                    ? -0.012
                    : (
                        (
                            (
                                client.id +
                                month
                                ) %
                            4 ==
                            0
                            )
                        ? 0.009
                        : 0.0
                        );


                const double snapshotValue =
                    month + 1 == snapshotDates.size()
                    ? currentValue
                    : currentValue * (baseFactor + wobble);


                Statement snapshot(
                    database_.handle(),
                    "INSERT INTO "
                    "portfolio_snapshots("
                    "client_id,"
                    "snapshot_date,"
                    "portfolio_value,"
                    "contributions_to_date,"
                    "withdrawals_to_date)"
                    "VALUES(?,?,?,?,?);");


                snapshot.bindInt(
                    1,
                    client.id);

                snapshot.bindText(
                    2,
                    snapshotDates[
                        month]);

                snapshot.bindDouble(
                    3,
                    snapshotValue);

                snapshot.bindDouble(
                    4,
                    6000.0 *
                    progress +
                    client.id *
                    150.0);

                snapshot.bindDouble(
                    5,
                    client.id %
                    6 ==
                    0
                    ? 1800.0
                    : 0.0);

                snapshot.executeDone();
            }


            const double target =
                client.accountBalance *
                (
                    client.age >=
                    55
                    ? 2.15
                    : 3.30
                    );


            const double goalCurrent =
                currentValue *
                (
                    client.id %
                    4 ==
                    0
                    ? 0.74
                    : 0.94
                    );


            Statement goal(
                database_.handle(),
                "INSERT INTO financial_goals("
                "client_id,"
                "goal_name,"
                "goal_type,"
                "target_amount,"
                "current_amount,"
                "target_date,"
                "priority,"
                "status)"
                "VALUES(?,?,?,?,?,?,?,?);");


            goal.bindInt(
                1,
                client.id);

            goal.bindText(
                2,
                client.age >=
                50
                ? "Retirement Readiness"
                : "Long Term Wealth");

            goal.bindText(
                3,
                client.age >=
                50
                ? "Retirement"
                : "Wealth");

            goal.bindDouble(
                4,
                target);

            goal.bindDouble(
                5,
                goalCurrent);

            goal.bindText(
                6,
                client.age >=
                55
                ? "2034-12-31"
                : "2042-12-31");

            goal.bindInt(
                7,
                1);

            goal.bindText(
                8,
                "Active");

            goal.executeDone();


            if (
                client.id %
                3 ==
                0) {

                Statement secondGoal(
                    database_.handle(),
                    "INSERT INTO financial_goals("
                    "client_id,"
                    "goal_name,"
                    "goal_type,"
                    "target_amount,"
                    "current_amount,"
                    "target_date,"
                    "priority,"
                    "status)"
                    "VALUES(?,?,?,?,?,?,?,?);");


                secondGoal.bindInt(
                    1,
                    client.id);

                secondGoal.bindText(
                    2,
                    "Education Reserve");

                secondGoal.bindText(
                    3,
                    "Education");

                secondGoal.bindDouble(
                    4,
                    90000.0);

                secondGoal.bindDouble(
                    5,
                    28000.0 +
                    client.id *
                    900.0);

                secondGoal.bindText(
                    6,
                    "2031-08-01");

                secondGoal.bindInt(
                    7,
                    2);

                secondGoal.bindText(
                    8,
                    "Active");

                secondGoal.executeDone();
            }


            Statement task(
                database_.handle(),
                "INSERT INTO advisor_tasks("
                "client_id,"
                "title,"
                "category,"
                "due_date,"
                "priority,"
                "completed)"
                "VALUES(?,?,?,?,?,?);");


            task.bindInt(
                1,
                client.id);

            task.bindText(
                2,
                client.id %
                4 ==
                0
                ? "Annual portfolio review overdue"
                : "Prepare annual portfolio review");

            task.bindText(
                3,
                "Review");

            task.bindText(
                4,
                client.id %
                4 ==
                0
                ? "2026-09-20"
                : "2026-11-15");

            task.bindText(
                5,
                client.id %
                4 ==
                0
                ? "High"
                : "Medium");

            task.bindInt(
                6,
                0);

            task.executeDone();


            Statement note(
                database_.handle(),
                "INSERT INTO advisor_notes("
                "client_id,"
                "note_text)"
                "VALUES(?,?);");


            note.bindInt(
                1,
                client.id);

            note.bindText(
                2,
                "Fictional demonstration note: "
                "review goals, allocation drift, "
                "liquidity needs, and upcoming "
                "relationship tasks at the next meeting.");

            note.executeDone();
        }


        database_.execute(
            "INSERT INTO schema_metadata("
            "key,value) "
            "VALUES('relational_seed_v2','1') "
            "ON CONFLICT(key) DO UPDATE SET value='1';");

        if (addedPortfolios > 0) {
            audit(
                "RELATIONAL_SEED_INCREMENTAL",
                std::nullopt,
                "Created synthetic portfolio data only for " +
                std::to_string(addedPortfolios) +
                " clients missing accounts. Existing rows retained.");
        }

        transaction.commit();
    }


public:

    explicit ClientRepository(
        Database& database)
        :
        database_(
            database) {

        initializeSchema();

        seedClients();

        seedSecurities();

        seedRelationalData();
    }


    Database& database() {

        return database_;
    }


    const Database& database() const {

        return database_;
    }


    vector<Client> getAllClients() const {

        return queryClients(
            "SELECT " +
            clientColumns() +
            " FROM clients "
            "ORDER BY normalized_name,id;");
    }


    optional<Client> findClientById(
        int clientId) const {

        Statement statement(
            database_.handle(),
            "SELECT " +
            clientColumns() +
            " FROM clients "
            "WHERE id=?;");

        statement.bindInt(
            1,
            clientId);

        const int result =
            statement.step();

        if (
            result ==
            SQLITE_ROW) {

            return readClient(
                statement);
        }

        if (
            result ==
            SQLITE_DONE) {

            return std::nullopt;
        }

        throw DatabaseException(
            sqlite3_errmsg(
                database_.handle()));
    }


    vector<Client> searchNamePrefix(
        const string& prefix) const {

        Statement statement(
            database_.handle(),
            "SELECT " +
            clientColumns() +
            " FROM clients "
            "WHERE normalized_name "
            "LIKE ? ESCAPE '\\' "
            "ORDER BY normalized_name,id;");

        statement.bindText(
            1,
            escapeLike(
                normalized(
                    prefix)) +
            "%");

        return collectClients(
            statement);
    }


    vector<Account> getAccountsForClient(
        int clientId) const {

        Statement statement(
            database_.handle(),
            "SELECT "
            "id,"
            "client_id,"
            "account_name,"
            "account_type,"
            "custodian,"
            "tax_treatment,"
            "active,"
            "opened_at "
            "FROM accounts "
            "WHERE client_id=? "
            "ORDER BY id;");

        statement.bindInt(
            1,
            clientId);

        vector<Account> results;

        while (
            statement.step() ==
            SQLITE_ROW) {

            Account account;

            account.id =
                statement.columnInt(
                    0);

            account.clientId =
                statement.columnInt(
                    1);

            account.accountName =
                statement.columnText(
                    2);

            account.accountType =
                statement.columnText(
                    3);

            account.custodian =
                statement.columnText(
                    4);

            account.taxTreatment =
                statement.columnText(
                    5);

            account.active =
                statement.columnInt(
                    6) !=
                0;

            account.openedAt =
                statement.columnText(
                    7);

            results.push_back(
                account);
        }

        return results;
    }


    vector<Holding> getHoldingsForClient(
        int clientId) const {

        Statement statement(
            database_.handle(),
            "SELECT "
            "h.id,"
            "h.account_id,"
            "h.symbol,"
            "s.security_name,"
            "s.asset_class,"
            "s.sector,"
            "h.quantity,"
            "s.mock_price,"
            "h.cost_basis_per_share,"
            "(h.quantity*s.mock_price),"
            "("
            "h.quantity*"
            "("
            "s.mock_price-"
            "h.cost_basis_per_share"
            ")"
            ") "
            "FROM holdings h "
            "JOIN accounts a "
            "ON a.id=h.account_id "
            "JOIN securities s "
            "ON s.symbol=h.symbol "
            "WHERE a.client_id=? "
            "ORDER BY "
            "(h.quantity*s.mock_price) "
            "DESC;");

        statement.bindInt(
            1,
            clientId);

        vector<Holding> results;

        while (
            statement.step() ==
            SQLITE_ROW) {

            Holding holding;

            holding.id =
                statement.columnInt(
                    0);

            holding.accountId =
                statement.columnInt(
                    1);

            holding.symbol =
                statement.columnText(
                    2);

            holding.securityName =
                statement.columnText(
                    3);

            holding.assetClass =
                statement.columnText(
                    4);

            holding.sector =
                statement.columnText(
                    5);

            holding.quantity =
                statement.columnDouble(
                    6);

            holding.price =
                statement.columnDouble(
                    7);

            holding.costBasisPerShare =
                statement.columnDouble(
                    8);

            holding.marketValue =
                statement.columnDouble(
                    9);

            holding.unrealizedGain =
                statement.columnDouble(
                    10);

            results.push_back(
                holding);
        }

        return results;
    }


    vector<TransactionRecord>
        getTransactionsForClient(
            int clientId,
            int limit = 50) const {

        Statement statement(
            database_.handle(),
            "SELECT "
            "t.id,"
            "t.account_id,"
            "t.transaction_date,"
            "t.transaction_type,"
            "COALESCE(t.symbol,''),"
            "t.quantity,"
            "t.amount,"
            "t.description "
            "FROM transactions t "
            "JOIN accounts a "
            "ON a.id=t.account_id "
            "WHERE a.client_id=? "
            "ORDER BY "
            "t.transaction_date DESC,"
            "t.id DESC "
            "LIMIT ?;");

        statement.bindInt(
            1,
            clientId);

        statement.bindInt(
            2,
            limit);

        vector<TransactionRecord> results;

        while (
            statement.step() ==
            SQLITE_ROW) {

            TransactionRecord row;

            row.id =
                statement.columnInt(
                    0);

            row.accountId =
                statement.columnInt(
                    1);

            row.date =
                statement.columnText(
                    2);

            row.type =
                statement.columnText(
                    3);

            row.symbol =
                statement.columnText(
                    4);

            row.quantity =
                statement.columnDouble(
                    5);

            row.amount =
                statement.columnDouble(
                    6);

            row.description =
                statement.columnText(
                    7);

            results.push_back(
                row);
        }

        return results;
    }


    vector<PortfolioSnapshot>
        getPortfolioHistory(
            int clientId) const {

        Statement statement(
            database_.handle(),
            "SELECT "
            "snapshot_date,"
            "portfolio_value,"
            "contributions_to_date,"
            "withdrawals_to_date "
            "FROM portfolio_snapshots "
            "WHERE client_id=? "
            "ORDER BY snapshot_date;");

        statement.bindInt(
            1,
            clientId);

        vector<PortfolioSnapshot> results;

        while (
            statement.step() ==
            SQLITE_ROW) {

            PortfolioSnapshot row;

            row.date =
                statement.columnText(
                    0);

            row.value =
                statement.columnDouble(
                    1);

            row.contributions =
                statement.columnDouble(
                    2);

            row.withdrawals =
                statement.columnDouble(
                    3);

            results.push_back(
                row);
        }

        return results;
    }


    vector<FinancialGoal>
        getGoalsForClient(
            int clientId) const {

        Statement statement(
            database_.handle(),
            "SELECT "
            "id,"
            "client_id,"
            "goal_name,"
            "goal_type,"
            "target_amount,"
            "current_amount,"
            "target_date,"
            "priority,"
            "status "
            "FROM financial_goals "
            "WHERE client_id=? "
            "ORDER BY priority,id;");

        statement.bindInt(
            1,
            clientId);

        vector<FinancialGoal> results;

        while (
            statement.step() ==
            SQLITE_ROW) {

            FinancialGoal goal;

            goal.id =
                statement.columnInt(
                    0);

            goal.clientId =
                statement.columnInt(
                    1);

            goal.name =
                statement.columnText(
                    2);

            goal.type =
                statement.columnText(
                    3);

            goal.targetAmount =
                statement.columnDouble(
                    4);

            goal.currentAmount =
                statement.columnDouble(
                    5);

            goal.targetDate =
                statement.columnText(
                    6);

            goal.priority =
                statement.columnInt(
                    7);

            goal.status =
                statement.columnText(
                    8);

            results.push_back(
                goal);
        }

        return results;
    }


    vector<AdvisorTask> getOpenTasks(
        optional<int> clientId =
        std::nullopt, bool includeCompleted = false) const {

        string sql =
            "SELECT "
            "id,"
            "client_id,"
            "title,"
            "category,"
            "due_date,"
            "priority,"
            "completed,description,assigned_to,workflow_status "
            "FROM advisor_tasks WHERE 1=1 ";
        if (!includeCompleted) sql += "AND completed=0 ";

        if (clientId) {

            sql +=
                "AND client_id=? ";
        }

        sql +=
            "ORDER BY completed,CASE priority WHEN 'Urgent' THEN 0 WHEN 'High' THEN 1 WHEN 'Medium' THEN 2 ELSE 3 END,due_date,id;";


        Statement statement(
            database_.handle(),
            sql);


        if (clientId) {

            statement.bindInt(
                1,
                *clientId);
        }


        vector<AdvisorTask> results;


        while (
            statement.step() ==
            SQLITE_ROW) {

            AdvisorTask task;

            task.id =
                statement.columnInt(
                    0);

            if (
                !statement.columnIsNull(
                    1)) {

                task.clientId =
                    statement.columnInt(
                        1);
            }

            task.title =
                statement.columnText(
                    2);

            task.category =
                statement.columnText(
                    3);

            task.dueDate =
                statement.columnText(
                    4);

            task.priority =
                statement.columnText(
                    5);

            task.completed =
                statement.columnInt(
                    6) !=
                0;
            task.description = statement.columnText(7);
            task.assignedTo = statement.columnText(8);
            task.status = task.completed ? "Completed" : statement.columnText(9);

            results.push_back(
                task);
        }


        return results;
    }


    vector<AdvisorNote> getNotesForClient(
        int clientId) const {

        Statement statement(
            database_.handle(),
            "SELECT "
            "id,"
            "client_id,"
            "note_text,"
            "created_at "
            "FROM advisor_notes "
            "WHERE client_id=? "
            "ORDER BY id DESC;");

        statement.bindInt(
            1,
            clientId);

        vector<AdvisorNote> results;

        while (
            statement.step() ==
            SQLITE_ROW) {

            AdvisorNote note;

            note.id =
                statement.columnInt(
                    0);

            note.clientId =
                statement.columnInt(
                    1);

            note.noteText =
                statement.columnText(
                    2);

            note.createdAt =
                statement.columnText(
                    3);

            results.push_back(
                note);
        }

        return results;
    }


    // Read only diagnostic for the exact legacy demo email pattern
    // This does not delete or merge client records
    int countLegacyDemoDuplicates() const {

        Statement q(
            database_.handle(),
            "SELECT COUNT(*) FROM clients d JOIN clients p "
            "ON d.id<>p.id AND lower(d.name)=lower(p.name) "
            "AND d.age=p.age "
            "AND abs(d.account_balance-p.account_balance)<0.01 "
            "WHERE d.email LIKE 'demo.investor%@aurelia.example' "
            "AND p.email LIKE '%@example.com' "
            "AND p.email NOT LIKE 'demo.investor%@aurelia.example';");

        return q.step() == SQLITE_ROW ? q.columnInt(0) : 0;
    }


    void recordAuditEvent(
        const string& eventType,
        optional<int> clientId,
        const string& details) {

        audit(
            eventType,
            clientId,
            details);
    }


    static void requireText(const string& value, size_t limit, const char* label) {
        if (InputValidator::trim(value).empty() || value.size()>limit)
            throw DatabaseException(string(label)+" is required and must fit within "+std::to_string(limit)+" characters.");
    }
    static void requireAmount(double value, const char* label) {
        if (!std::isfinite(value) || value<0 || value>Constants::kMaxBalance)
            throw DatabaseException(string(label)+" must be a finite nonnegative amount below one billion.");
    }
    static void requireDate(const string& value) {
        if(value.size()!=10 || value[4]!='-' || value[7]!='-') throw DatabaseException("Enter a date as YYYY-MM-DD.");
        for(size_t i=0;i<value.size();++i) if(i!=4 && i!=7 && !std::isdigit((unsigned char)value[i])) throw DatabaseException("Invalid date.");
        int y=std::stoi(value.substr(0,4)),m=std::stoi(value.substr(5,2)),d=std::stoi(value.substr(8,2));
        const int days[]={31,28,31,30,31,30,31,31,30,31,30,31};
        if(y<1900 || y>2200 || m<1 || m>12 || d<1 || d>days[m-1]+(m==2 && y%4==0 && (y%100!=0 || y%400==0))) throw DatabaseException("Enter a valid calendar date.");
    }
    PlanningProfile getPlanningProfile(int clientId) const {
        PlanningProfile p; p.clientId=clientId;
        Statement q(database_.handle(),"SELECT occupation,employer,location,contact_preference,dependents,retirement_age,annual_expenses,emergency_reserve,monthly_saving,review_date,planning_context FROM client_profile_details WHERE client_id=?;");
        q.bindInt(1,clientId);
        if(q.step()==SQLITE_ROW){p.occupation=q.columnText(0);p.employer=q.columnText(1);p.location=q.columnText(2);p.contactPreference=q.columnText(3);p.dependents=q.columnInt(4);p.retirementAge=q.columnInt(5);p.annualExpenses=q.columnDouble(6);p.emergencyReserve=q.columnDouble(7);p.monthlySaving=q.columnDouble(8);p.reviewDate=q.columnText(9);p.planningContext=q.columnText(10);}return p;
    }
    void savePlanningProfile(const Client& c,const PlanningProfile& p) {
        requireText(c.name,100,"Client name");requireText(c.email,150,"Email");
        if(c.email.find('@')==string::npos || c.email.find('.',c.email.find('@'))==string::npos || c.email.find(' ')!=string::npos) throw DatabaseException("Enter a valid email address.");
        if(c.age<18 || c.age>120 || c.timeHorizonYears<0 || c.timeHorizonYears>80 || p.dependents<0 || p.dependents>20 || p.retirementAge<18 || p.retirementAge>100) throw DatabaseException("Age, dependents, retirement age or time horizon is outside its valid range.");
        requireAmount(c.annualIncome,"Income");requireAmount(c.netWorth,"Net worth");requireAmount(p.annualExpenses,"Expenses");requireAmount(p.emergencyReserve,"Emergency reserve");requireAmount(p.monthlySaving,"Monthly savings");requireDate(p.reviewDate);
        if(!findClientById(c.id)) throw DatabaseException("Client no longer exists.");
        Statement unique(database_.handle(),"SELECT COUNT(*) FROM clients WHERE id<>? AND (lower(trim(name))=lower(trim(?)) OR lower(email)=lower(?));");unique.bindInt(1,c.id);unique.bindText(2,c.name);unique.bindText(3,c.email);unique.step();if(unique.columnInt(0)>0)throw DatabaseException("Another client already uses this name or email. Review the relationship rather than creating a duplicate.");
        Transaction tx(database_);
        Statement q(database_.handle(),"UPDATE clients SET name=?,normalized_name=?,email=?,phone=?,household_name=?,advisor_name=?,investment_objective=?,annual_income=?,net_worth=?,liquidity_need=?,time_horizon_years=?,age=?,risk_level_id=?,updated_at=CURRENT_TIMESTAMP WHERE id=?;");
        q.bindText(1,c.name);q.bindText(2,normalized(c.name));q.bindText(3,c.email);q.bindText(4,c.phone);q.bindText(5,c.householdName);q.bindText(6,c.advisorName);q.bindText(7,c.investmentObjective);q.bindDouble(8,c.annualIncome);q.bindDouble(9,c.netWorth);q.bindText(10,c.liquidityNeed);q.bindInt(11,c.timeHorizonYears);q.bindInt(12,c.age);q.bindInt(13,(int)c.riskLevel);q.bindInt(14,c.id);q.executeDone();
        Statement d(database_.handle(),"INSERT INTO client_profile_details(client_id,occupation,employer,location,contact_preference,dependents,retirement_age,annual_expenses,emergency_reserve,monthly_saving,review_date,planning_context) VALUES(?,?,?,?,?,?,?,?,?,?,?,?) ON CONFLICT(client_id) DO UPDATE SET occupation=excluded.occupation,employer=excluded.employer,location=excluded.location,contact_preference=excluded.contact_preference,dependents=excluded.dependents,retirement_age=excluded.retirement_age,annual_expenses=excluded.annual_expenses,emergency_reserve=excluded.emergency_reserve,monthly_saving=excluded.monthly_saving,review_date=excluded.review_date,planning_context=excluded.planning_context,updated_at=CURRENT_TIMESTAMP;");
        d.bindInt(1,c.id);d.bindText(2,p.occupation);d.bindText(3,p.employer);d.bindText(4,p.location);d.bindText(5,p.contactPreference);d.bindInt(6,p.dependents);d.bindInt(7,p.retirementAge);d.bindDouble(8,p.annualExpenses);d.bindDouble(9,p.emergencyReserve);d.bindDouble(10,p.monthlySaving);d.bindText(11,p.reviewDate);d.bindText(12,p.planningContext);d.executeDone();audit("CLIENT_PROFILE_UPDATED",c.id,"Updated contact, household and planning information; account records retained.");tx.commit();
    }
    int saveTask(int taskId,int clientId,const string& title,const string& description,const string& category,const string& dueDate,const string& priority,const string& owner,const string& status) {
        requireText(title,160,"Ticket title");requireText(description,2000,"Ticket details");requireDate(dueDate);if(!findClientById(clientId))throw DatabaseException("Choose an existing client.");
        if(priority!="Urgent"&&priority!="High"&&priority!="Medium"&&priority!="Low")throw DatabaseException("Choose a valid priority.");
        if(status!="Open"&&status!="In progress"&&status!="Waiting"&&status!="Completed")throw DatabaseException("Choose a valid ticket status.");
        requireText(owner,100,"Assigned advisor");Transaction tx(database_);
        Statement q(database_.handle(),taskId ? "UPDATE advisor_tasks SET client_id=?,title=?,description=?,category=?,due_date=?,priority=?,assigned_to=?,workflow_status=?,completed=? WHERE id=?;" : "INSERT INTO advisor_tasks(client_id,title,description,category,due_date,priority,assigned_to,workflow_status,completed) VALUES(?,?,?,?,?,?,?,?,?);");
        q.bindInt(1,clientId);q.bindText(2,title);q.bindText(3,description);q.bindText(4,category);q.bindText(5,dueDate);q.bindText(6,priority);q.bindText(7,owner);q.bindText(8,status);q.bindInt(9,status=="Completed");if(taskId)q.bindInt(10,taskId);q.executeDone();if(taskId && !database_.changes())throw DatabaseException("Ticket no longer exists.");int id=taskId?taskId:database_.lastId();audit(taskId?"TICKET_UPDATED":"TICKET_CREATED",clientId,"AW-"+std::to_string(id)+" | "+title+" | "+priority+" | "+status);tx.commit();return id;
    }
    int saveGoal(int goalId,int clientId,const string& name,const string& type,double target,double current,const string& date,int priority) {
        requireText(name,160,"Goal name");requireDate(date);requireAmount(target,"Goal target");requireAmount(current,"Current funding");if(target<=0 || priority<1 || priority>3 || !findClientById(clientId))throw DatabaseException("Choose a client, positive goal target and valid priority.");
        Transaction tx(database_);Statement q(database_.handle(),goalId ? "UPDATE financial_goals SET client_id=?,goal_name=?,goal_type=?,target_amount=?,current_amount=?,target_date=?,priority=?,status=? WHERE id=?;":"INSERT INTO financial_goals(client_id,goal_name,goal_type,target_amount,current_amount,target_date,priority,status) VALUES(?,?,?,?,?,?,?,?);");
        q.bindInt(1,clientId);q.bindText(2,name);q.bindText(3,type);q.bindDouble(4,target);q.bindDouble(5,current);q.bindText(6,date);q.bindInt(7,priority);q.bindText(8,current>=target?"Completed":"Active");if(goalId)q.bindInt(9,goalId);q.executeDone();if(goalId && !database_.changes())throw DatabaseException("Goal no longer exists.");int id=goalId?goalId:database_.lastId();audit(goalId?"GOAL_UPDATED":"GOAL_CREATED",clientId,name+" | Target date "+date);tx.commit();return id;
    }
    void addPlanningNote(int clientId,const string& note) {
        requireText(note,3000,"Advisor note");if(!findClientById(clientId))throw DatabaseException("Client no longer exists.");Transaction tx(database_);Statement q(database_.handle(),"INSERT INTO advisor_notes(client_id,note_text) VALUES(?,?);");q.bindInt(1,clientId);q.bindText(2,note);q.executeDone();audit("ADVISOR_NOTE_ADDED",clientId,"Recorded a client planning note.");tx.commit();
    }
    int addClientAccount(int clientId,const string& name,const string& type,const string& tax) {
        requireText(name,100,"Account name");if(!findClientById(clientId))throw DatabaseException("Client no longer exists.");Transaction tx(database_);Statement q(database_.handle(),"INSERT INTO accounts(client_id,account_name,account_type,tax_treatment,custodian) VALUES(?,?,?,?,?);");q.bindInt(1,clientId);q.bindText(2,name);q.bindText(3,type);q.bindText(4,tax);q.bindText(5,"Aurelia Demonstration Custody");q.executeDone();int id=database_.lastId();audit("ACCOUNT_CREATED",clientId,name);tx.commit();return id;
    }
    void saveAccountHolding(int clientId,int accountId,const string& symbol,double quantity,double cost) {
        requireAmount(quantity,"Quantity");requireAmount(cost,"Cost per share");Statement q(database_.handle(),"SELECT COUNT(*) FROM accounts a JOIN securities s ON s.symbol=? WHERE a.id=? AND a.client_id=?;");q.bindText(1,symbol);q.bindInt(2,accountId);q.bindInt(3,clientId);q.step();if(q.columnInt(0)!=1)throw DatabaseException("Choose a security and an account belonging to this client.");Transaction tx(database_);Statement h(database_.handle(),"INSERT INTO holdings(account_id,symbol,quantity,cost_basis_per_share) VALUES(?,?,?,?) ON CONFLICT(account_id,symbol) DO UPDATE SET quantity=excluded.quantity,cost_basis_per_share=excluded.cost_basis_per_share,updated_at=CURRENT_TIMESTAMP;");h.bindInt(1,accountId);h.bindText(2,symbol);h.bindDouble(3,quantity);h.bindDouble(4,cost);h.executeDone();audit("HOLDING_UPDATED",clientId,"Account "+std::to_string(accountId)+" | "+symbol+" | quantity "+std::to_string(quantity));tx.commit();
    }

    void storeMarketObservation(const string& symbol,double price,const string& date,const string& source) {
        requireAmount(price,"Market price");if(price<=0)throw DatabaseException("Market price must be positive.");requireDate(date);if(source.rfind("https://",0)!=0)throw DatabaseException("Provide the HTTPS source URL for this observation.");Statement d(database_.handle(),"SELECT date('now','-7 hours');");d.step();if(date>d.columnText(0))throw DatabaseException("Market observation cannot be dated in the future.");Statement known(database_.handle(),"SELECT COUNT(*) FROM securities WHERE symbol=?;");known.bindText(1,symbol);known.step();if(known.columnInt(0)!=1||symbol=="CASH")throw DatabaseException("Choose an existing tradable security.");Transaction tx(database_);Statement q(database_.handle(),"INSERT INTO market_quotes(symbol,price,as_of_date,source) VALUES(?,?,?,?) ON CONFLICT(symbol) DO UPDATE SET price=excluded.price,as_of_date=excluded.as_of_date,source=excluded.source,fetched_at=CURRENT_TIMESTAMP WHERE excluded.as_of_date>=market_quotes.as_of_date;");q.bindText(1,symbol);q.bindDouble(2,price);q.bindText(3,date);q.bindText(4,source);q.executeDone();if(!database_.changes())throw DatabaseException("An older observation cannot replace a newer stored quote.");audit("MARKET_OBSERVATION_STORED",std::nullopt,symbol+" | "+date+" | "+source);tx.commit();
    }

    void runIntegrityCheck() const {

        Statement statement(
            database_.handle(),
            "PRAGMA integrity_check;");

        while (
            statement.step() ==
            SQLITE_ROW) {

            std::cout
                << statement.columnText(
                    0)
                << '\n';
        }
    }


    void checkpointWal() const {

        Statement statement(
            database_.handle(),
            "PRAGMA wal_checkpoint("
            "TRUNCATE);");

        if (
            statement.step() ==
            SQLITE_ROW) {

            std::cout
                << "WAL busy: "
                << statement.columnInt(
                    0)
                << '\n';
        }
    }
};