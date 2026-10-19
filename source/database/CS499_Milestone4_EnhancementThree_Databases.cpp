/*
 * CS 499 Computer Science Capstone
 * Milestone Four: Enhancement Three - Databases
 * Student: Brianna Reed
 * Date: October 4, 2026
 *
 * Artifact origin:
 * CS 410 Software Reverse Engineering Project One, enhanced during
 * CS 499 Milestone Two (Software Design and Engineering) and Milestone Three
 * (Algorithms and Data Structures)
 *
 * Enhancement purpose:
 * Move the client management application from volatile in memory storage to a
 * persistent relational SQLite database while preserving the stronger software
 * design, validation, search, ranking, analytics, security, and audit concepts
 * introduced in the previous capstone enhancements
 *
 * DATABASE ENHANCEMENT HIGHLIGHTS:
 *   1. Persistent SQLite storage replaces the in memory primary client store
 *   2. A relational schema separates clients from service and risk lookup data
 *   3. CHECK, UNIQUE, NOT NULL, PRIMARY KEY, and FOREIGN KEY constraints protect
 *      data integrity at the database layer in addition to application validation
 *   4. Prepared statements bind all user controlled values, reducing SQL injection
 *      risk and keeping SQL execution predictable
 *   5. Explicit transactions make client mutations, history records, and audit
 *      events atomic: all related changes commit together or roll back together
 *   6. Database indexes support common access paths for name, service, risk,
 *      active status, balance, and undo history lookup
 *   7. SQL performs filtering, sorting, ranking, grouping, and aggregation so the
 *      database engine participates directly in query optimization
 *   8. A persistent client_history table supports undo even after the application
 *      is closed and restarted
 *   9. A database audit_events table records data changes separately from the
 *      file based security/application log
 *  10. Database tools expose integrity checking, index metadata, row counts,
 *      query plan examples, recent audit events, and WAL checkpointing
 *
 * Build example (Linux/macOS with SQLite development package installed):
 *   g++ -std=c++17 -Wall -Wextra -pedantic \
 *       CS499_Milestone4_EnhancementThree_Databases.cpp -lsqlite3 -o milestone4
 *
/*
 * Visual Studio note:
 *   This Visual Studio version uses the WinSQLite implementation supplied
 *   through the Windows SDK. The local sqlite3.h compatibility header includes
 *   winsqlite3.h and links winsqlite3.lib
 *
 *   Configure CS499_USERNAME and CS499_PASSWORD as environment variables
 *   before running the application
 */

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "sqlite3.h"

using std::cin;
using std::cout;
using std::getline;
using std::optional;
using std::ofstream;
using std::string;
using std::vector;

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------

namespace Constants {
    constexpr int kMaxAuthenticationAttempts = 3;
    constexpr int kMinServiceChoice = 1;
    constexpr int kMaxServiceChoice = 2;
    constexpr int kMinRiskChoice = 1;
    constexpr int kMaxRiskChoice = 3;
    constexpr int kMinAge = 18;
    constexpr int kMaxAge = 120;
    constexpr double kMinBalance = 0.0;
    constexpr double kMaxBalance = 1000000000.0;
    constexpr int kDefaultTopCount = 5;
    constexpr int kAuditRowsToDisplay = 20;
    constexpr int kHistoryRowsToDisplay = 20;
}

// -----------------------------------------------------------------------------
// Enumerations and Models
// -----------------------------------------------------------------------------

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
    ServiceType service = ServiceType::Brokerage;
    int age = Constants::kMinAge;
    double accountBalance = 0.0;
    RiskLevel riskLevel = RiskLevel::Moderate;
    string email;
    string phone;
    bool active = true;
    string createdAt;
    string updatedAt;
};

string serviceTypeToString(ServiceType service) {
    switch (service) {
    case ServiceType::Brokerage:
        return "Brokerage";
    case ServiceType::Retirement:
        return "Retirement";
    default:
        return "Unknown";
    }
}

string riskLevelToString(RiskLevel risk) {
    switch (risk) {
    case RiskLevel::Conservative:
        return "Conservative";
    case RiskLevel::Moderate:
        return "Moderate";
    case RiskLevel::Aggressive:
        return "Aggressive";
    default:
        return "Unknown";
    }
}

// -----------------------------------------------------------------------------
// Logger
// -----------------------------------------------------------------------------

class Logger {
public:
    explicit Logger(string fileName)
        : fileName_(std::move(fileName)) {
    }

    void write(const string& message) const {
        ofstream logFile(fileName_, std::ios::app);
        if (logFile.is_open()) {
            logFile << message << '\n';
        }
    }

private:
    string fileName_;
};

// -----------------------------------------------------------------------------
// Input Validation
// -----------------------------------------------------------------------------

class InputValidator {
public:
    static int readIntInRange(
        const string& prompt,
        int minimum,
        int maximum) {

        while (true) {
            cout << prompt;

            string input;
            getline(cin, input);
            std::stringstream stream(input);

            int value = 0;
            char trailingCharacter = '\0';

            if ((stream >> value) &&
                !(stream >> trailingCharacter) &&
                value >= minimum &&
                value <= maximum) {
                return value;
            }

            cout << "Invalid input. Please enter a number from "
                << minimum << " through " << maximum << ".\n";
        }
    }

    static double readDoubleInRange(
        const string& prompt,
        double minimum,
        double maximum) {

        while (true) {
            cout << prompt;

            string input;
            getline(cin, input);
            std::stringstream stream(input);

            double value = 0.0;
            char trailingCharacter = '\0';

            if ((stream >> value) &&
                !(stream >> trailingCharacter) &&
                std::isfinite(value) &&
                value >= minimum &&
                value <= maximum) {
                return value;
            }

            cout << "Invalid input. Please enter a value from "
                << minimum << " through " << maximum << ".\n";
        }
    }

    static string readNonEmptyLine(const string& prompt) {
        while (true) {
            cout << prompt;

            string value;
            getline(cin, value);

            if (value.find_first_not_of(" \t\r\n") != string::npos) {
                return value;
            }

            cout << "Input cannot be blank. Please try again.\n";
        }
    }

    static bool readYesNo(const string& prompt) {
        while (true) {
            const string input = toLower(readNonEmptyLine(prompt));

            if (input == "y" || input == "yes") {
                return true;
            }

            if (input == "n" || input == "no") {
                return false;
            }

            cout << "Please enter yes/y or no/n.\n";
        }
    }

    static string toLower(string value) {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char character) {
                return static_cast<char>(std::tolower(character));
            });

        return value;
    }

    static string trim(const string& value) {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first == string::npos) {
            return "";
        }

        const auto last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    }
};

// -----------------------------------------------------------------------------
// Authentication
// -----------------------------------------------------------------------------

class Authenticator {
public:
    explicit Authenticator(const Logger& logger)
        : logger_(logger) {
    }

    bool authenticate() const {
        const string configuredUsername =
            readEnvironmentVariable("CS499_USERNAME");

        const string configuredPassword =
            readEnvironmentVariable("CS499_PASSWORD");

        if (configuredUsername.empty() || configuredPassword.empty()) {
            cout
                << "Authentication is not configured.\n"
                << "Set the CS499_USERNAME and CS499_PASSWORD environment "
                << "variables before running the program.\n";

            logger_.write("AUTH_CONFIGURATION_ERROR");
            return false;
        }

        for (int attempt = 1;
            attempt <= Constants::kMaxAuthenticationAttempts;
            ++attempt) {

            const string enteredUsername =
                InputValidator::readNonEmptyLine("Enter your username: ");

            const string enteredPassword =
                InputValidator::readNonEmptyLine("Enter your password: ");

            if (enteredUsername == configuredUsername &&
                enteredPassword == configuredPassword) {

                cout << "Authentication successful.\n\n";
                logger_.write("AUTH_SUCCESS username=" + enteredUsername);
                return true;
            }

            const int remainingAttempts =
                Constants::kMaxAuthenticationAttempts - attempt;

            cout << "Invalid credentials.";
            if (remainingAttempts > 0) {
                cout << " Attempts remaining: " << remainingAttempts;
            }
            cout << '\n';

            logger_.write(
                "AUTH_FAILURE attempt=" + std::to_string(attempt));
        }

        cout << "Maximum authentication attempts reached. Access denied.\n";
        logger_.write("AUTH_LOCKOUT");
        return false;
    }

private:
    static string readEnvironmentVariable(const char* name) {
#ifdef _MSC_VER
        char* value = nullptr;
        size_t length = 0;

        const errno_t result = _dupenv_s(&value, &length, name);
        if (result != 0 || value == nullptr) {
            return "";
        }

        const string environmentValue(value);
        std::free(value);
        return environmentValue;
#else
        const char* value = std::getenv(name);
        if (value == nullptr) {
            return "";
        }

        return string(value);
#endif
    }

    const Logger& logger_;
};

// -----------------------------------------------------------------------------
// SQLite Helpers
// -----------------------------------------------------------------------------

class DatabaseException : public std::runtime_error {
public:
    explicit DatabaseException(const string& message)
        : std::runtime_error(message) {
    }
};

class Statement {
public:
    Statement(sqlite3* db, const string& sql)
        : db_(db), statement_(nullptr) {

        const int result = sqlite3_prepare_v2(
            db_, sql.c_str(), -1, &statement_, nullptr);

        if (result != SQLITE_OK) {
            throw DatabaseException(
                "Failed to prepare SQL statement: " +
                string(sqlite3_errmsg(db_)) +
                " | SQL: " + sql);
        }
    }

    ~Statement() {
        if (statement_ != nullptr) {
            sqlite3_finalize(statement_);
        }
    }

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    sqlite3_stmt* get() const {
        return statement_;
    }

    void bindInt(int index, int value) {
        checkBind(sqlite3_bind_int(statement_, index, value));
    }

    void bindDouble(int index, double value) {
        checkBind(sqlite3_bind_double(statement_, index, value));
    }

    void bindText(int index, const string& value) {
        checkBind(sqlite3_bind_text(
            statement_,
            index,
            value.c_str(),
            -1,
            SQLITE_TRANSIENT));
    }

    void bindNull(int index) {
        checkBind(sqlite3_bind_null(statement_, index));
    }

    int step() {
        return sqlite3_step(statement_);
    }

    void executeDone() {
        const int result = step();
        if (result != SQLITE_DONE) {
            throw DatabaseException(
                "SQL execution failed: " + string(sqlite3_errmsg(db_)));
        }
    }

    int columnInt(int index) const {
        return sqlite3_column_int(statement_, index);
    }

    double columnDouble(int index) const {
        return sqlite3_column_double(statement_, index);
    }

    string columnText(int index) const {
        const unsigned char* text = sqlite3_column_text(statement_, index);
        return text == nullptr
            ? ""
            : reinterpret_cast<const char*>(text);
    }

    bool columnIsNull(int index) const {
        return sqlite3_column_type(statement_, index) == SQLITE_NULL;
    }

private:
    void checkBind(int result) {
        if (result != SQLITE_OK) {
            throw DatabaseException(
                "Failed to bind SQL parameter: " +
                string(sqlite3_errmsg(db_)));
        }
    }

    sqlite3* db_;
    sqlite3_stmt* statement_;
};

class Database {
public:
    explicit Database(const string& fileName)
        : db_(nullptr), fileName_(fileName) {

        const int result = sqlite3_open_v2(
            fileName.c_str(),
            &db_,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
            nullptr);

        if (result != SQLITE_OK) {
            const string message =
                db_ == nullptr
                ? "Unknown SQLite open error."
                : string(sqlite3_errmsg(db_));

            if (db_ != nullptr) {
                sqlite3_close(db_);
                db_ = nullptr;
            }

            throw DatabaseException(
                "Unable to open database '" + fileName + "': " + message);
        }

        sqlite3_busy_timeout(db_, 5000);
        execute("PRAGMA foreign_keys = ON;");
        execute("PRAGMA journal_mode = WAL;");
        execute("PRAGMA synchronous = NORMAL;");
    }

    ~Database() {
        if (db_ != nullptr) {
            sqlite3_close(db_);
        }
    }

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    sqlite3* handle() const {
        return db_;
    }

    const string& fileName() const {
        return fileName_;
    }

    void execute(const string& sql) const {
        char* errorMessage = nullptr;

        const int result = sqlite3_exec(
            db_, sql.c_str(), nullptr, nullptr, &errorMessage);

        if (result != SQLITE_OK) {
            const string message =
                errorMessage == nullptr
                ? string(sqlite3_errmsg(db_))
                : string(errorMessage);

            sqlite3_free(errorMessage);
            throw DatabaseException(
                "SQLite execution error: " + message + " | SQL: " + sql);
        }
    }

    long long lastInsertRowId() const {
        return sqlite3_last_insert_rowid(db_);
    }

    int changes() const {
        return sqlite3_changes(db_);
    }

private:
    sqlite3* db_;
    string fileName_;
};

class Transaction {
public:
    explicit Transaction(Database& database)
        : database_(database), active_(true) {
        database_.execute("BEGIN IMMEDIATE TRANSACTION;");
    }

    ~Transaction() {
        if (active_) {
            try {
                database_.execute("ROLLBACK;");
            }
            catch (...) {
                // Destructors must not throw. The original exception is preserved
            }
        }
    }

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    void commit() {
        if (active_) {
            database_.execute("COMMIT;");
            active_ = false;
        }
    }

    void rollback() {
        if (active_) {
            database_.execute("ROLLBACK;");
            active_ = false;
        }
    }

private:
    Database& database_;
    bool active_;
};

// -----------------------------------------------------------------------------
// Database Repository
// -----------------------------------------------------------------------------

class ClientRepository {
public:
    explicit ClientRepository(Database& database)
        : database_(database) {
        initializeSchema();
        seedReferenceData();
        seedClientsIfEmpty();
    }

    vector<Client> getAllClients() const {
        return queryClients(
            "SELECT " + clientSelectColumns() +
            " FROM clients c "
            "ORDER BY c.id ASC;");
    }

    optional<Client> findClientById(int clientId) const {
        Statement statement(
            database_.handle(),
            "SELECT " + clientSelectColumns() +
            " FROM clients c WHERE c.id = ?;");

        statement.bindInt(1, clientId);

        const int result = statement.step();
        if (result == SQLITE_ROW) {
            return readClient(statement);
        }

        if (result == SQLITE_DONE) {
            return std::nullopt;
        }

        throw DatabaseException(
            "Client lookup failed: " +
            string(sqlite3_errmsg(database_.handle())));
    }

    int addClient(Client client) {
        Transaction transaction(database_);

        Statement statement(
            database_.handle(),
            "INSERT INTO clients ("
            "name, normalized_name, service_id, age, account_balance, "
            "risk_level_id, email, phone, active"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);");

        statement.bindText(1, client.name);
        statement.bindText(2, normalizeName(client.name));
        statement.bindInt(3, static_cast<int>(client.service));
        statement.bindInt(4, client.age);
        statement.bindDouble(5, client.accountBalance);
        statement.bindInt(6, static_cast<int>(client.riskLevel));
        statement.bindText(7, client.email);
        statement.bindText(8, client.phone);
        statement.bindInt(9, client.active ? 1 : 0);
        statement.executeDone();

        client.id = static_cast<int>(database_.lastInsertRowId());

        recordHistory("ADD", std::nullopt, client);
        recordAudit(
            "CLIENT_ADD",
            client.id,
            "Added client name=" + client.name);

        transaction.commit();
        return client.id;
    }

    bool updateClient(const Client& before, const Client& after) {
        Transaction transaction(database_);

        Statement statement(
            database_.handle(),
            "UPDATE clients SET "
            "name = ?, normalized_name = ?, service_id = ?, age = ?, "
            "account_balance = ?, risk_level_id = ?, email = ?, phone = ?, "
            "active = ?, updated_at = CURRENT_TIMESTAMP "
            "WHERE id = ?;");

        statement.bindText(1, after.name);
        statement.bindText(2, normalizeName(after.name));
        statement.bindInt(3, static_cast<int>(after.service));
        statement.bindInt(4, after.age);
        statement.bindDouble(5, after.accountBalance);
        statement.bindInt(6, static_cast<int>(after.riskLevel));
        statement.bindText(7, after.email);
        statement.bindText(8, after.phone);
        statement.bindInt(9, after.active ? 1 : 0);
        statement.bindInt(10, before.id);
        statement.executeDone();

        if (database_.changes() != 1) {
            transaction.rollback();
            return false;
        }

        recordHistory("UPDATE", before, after);
        recordAudit(
            "CLIENT_UPDATE",
            before.id,
            "Updated client name=" + after.name);

        transaction.commit();
        return true;
    }

    bool removeClient(const Client& client) {
        Transaction transaction(database_);

        Statement statement(
            database_.handle(),
            "DELETE FROM clients WHERE id = ?;");

        statement.bindInt(1, client.id);
        statement.executeDone();

        if (database_.changes() != 1) {
            transaction.rollback();
            return false;
        }

        recordHistory("REMOVE", client, std::nullopt);
        recordAudit(
            "CLIENT_REMOVE",
            client.id,
            "Removed client name=" + client.name);

        transaction.commit();
        return true;
    }

    vector<Client> searchExactName(const string& name) const {
        Statement statement(
            database_.handle(),
            "SELECT " + clientSelectColumns() +
            " FROM clients c "
            "WHERE c.normalized_name = ? "
            "ORDER BY c.id ASC;");

        statement.bindText(1, normalizeName(name));
        return collectClients(statement);
    }

    vector<Client> searchNamePrefix(const string& prefix) const {
        Statement statement(
            database_.handle(),
            "SELECT " + clientSelectColumns() +
            " FROM clients c "
            "WHERE c.normalized_name LIKE ? ESCAPE '\\' "
            "ORDER BY c.normalized_name ASC, c.id ASC;");

        statement.bindText(1, escapeLike(normalizeName(prefix)) + "%");
        return collectClients(statement);
    }

    vector<Client> searchByService(ServiceType service) const {
        Statement statement(
            database_.handle(),
            "SELECT " + clientSelectColumns() +
            " FROM clients c "
            "WHERE c.service_id = ? "
            "ORDER BY c.id ASC;");

        statement.bindInt(1, static_cast<int>(service));
        return collectClients(statement);
    }

    vector<Client> searchByRisk(RiskLevel risk) const {
        Statement statement(
            database_.handle(),
            "SELECT " + clientSelectColumns() +
            " FROM clients c "
            "WHERE c.risk_level_id = ? "
            "ORDER BY c.id ASC;");

        statement.bindInt(1, static_cast<int>(risk));
        return collectClients(statement);
    }

    vector<Client> searchByActive(bool active) const {
        Statement statement(
            database_.handle(),
            "SELECT " + clientSelectColumns() +
            " FROM clients c "
            "WHERE c.active = ? "
            "ORDER BY c.id ASC;");

        statement.bindInt(1, active ? 1 : 0);
        return collectClients(statement);
    }

    vector<Client> searchByBalanceRange(double minimum, double maximum) const {
        Statement statement(
            database_.handle(),
            "SELECT " + clientSelectColumns() +
            " FROM clients c "
            "WHERE c.account_balance BETWEEN ? AND ? "
            "ORDER BY c.account_balance ASC, c.id ASC;");

        statement.bindDouble(1, minimum);
        statement.bindDouble(2, maximum);
        return collectClients(statement);
    }

    vector<Client> compoundFilter(
        ServiceType service,
        RiskLevel risk,
        bool active,
        double minimumBalance) const {

        Statement statement(
            database_.handle(),
            "SELECT " + clientSelectColumns() +
            " FROM clients c "
            "WHERE c.service_id = ? "
            "AND c.risk_level_id = ? "
            "AND c.active = ? "
            "AND c.account_balance >= ? "
            "ORDER BY c.account_balance DESC, c.id ASC;");

        statement.bindInt(1, static_cast<int>(service));
        statement.bindInt(2, static_cast<int>(risk));
        statement.bindInt(3, active ? 1 : 0);
        statement.bindDouble(4, minimumBalance);
        return collectClients(statement);
    }

    vector<Client> sortedClients(int sortChoice) const {
        // ORDER BY expressions come only from this fixed whitelist
        // User input is never inserted directly into SQL
        string orderBy;

        switch (sortChoice) {
        case 1:
            orderBy = "c.id ASC";
            break;
        case 2:
            orderBy = "c.normalized_name ASC, c.id ASC";
            break;
        case 3:
            orderBy = "c.account_balance DESC, c.id ASC";
            break;
        case 4:
            orderBy = "c.account_balance ASC, c.id ASC";
            break;
        case 5:
            orderBy = "c.service_id ASC, c.normalized_name ASC";
            break;
        case 6:
            orderBy = "c.risk_level_id ASC, c.normalized_name ASC";
            break;
        case 7:
            orderBy = "c.age ASC, c.id ASC";
            break;
        default:
            orderBy = "c.id ASC";
            break;
        }

        return queryClients(
            "SELECT " + clientSelectColumns() +
            " FROM clients c ORDER BY " + orderBy + ";");
    }

    vector<Client> topClientsByBalance(int count, bool highest) const {
        Statement statement(
            database_.handle(),
            "SELECT " + clientSelectColumns() +
            " FROM clients c "
            "ORDER BY c.account_balance " +
            string(highest ? "DESC" : "ASC") +
            ", c.id ASC LIMIT ?;");

        statement.bindInt(1, count);
        return collectClients(statement);
    }

    void displayAnalytics() const {
        cout << "\nDatabase Portfolio Analytics\n";
        cout << "----------------------------\n";

        Statement summary(
            database_.handle(),
            "SELECT "
            "COUNT(*), "
            "COALESCE(SUM(account_balance), 0), "
            "COALESCE(AVG(account_balance), 0), "
            "COALESCE(MIN(account_balance), 0), "
            "COALESCE(MAX(account_balance), 0), "
            "SUM(CASE WHEN active = 1 THEN 1 ELSE 0 END) "
            "FROM clients;");

        if (summary.step() == SQLITE_ROW) {
            const int count = summary.columnInt(0);
            const double total = summary.columnDouble(1);
            const double average = summary.columnDouble(2);
            const double minimum = summary.columnDouble(3);
            const double maximum = summary.columnDouble(4);
            const int activeCount = summary.columnInt(5);

            cout << "Client count: " << count << '\n';
            cout << "Active clients: " << activeCount << '\n';
            cout << "Inactive clients: " << (count - activeCount) << '\n';
            cout << std::fixed << std::setprecision(2);
            cout << "Total portfolio balance: $" << total << '\n';
            cout << "Average client balance: $" << average << '\n';
            cout << "Minimum client balance: $" << minimum << '\n';
            cout << "Maximum client balance: $" << maximum << '\n';
        }

        const vector<double> balances = orderedBalances();
        if (!balances.empty()) {
            double median = 0.0;
            const std::size_t middle = balances.size() / 2;

            if (balances.size() % 2 == 0) {
                median = (balances[middle - 1] + balances[middle]) / 2.0;
            }
            else {
                median = balances[middle];
            }

            cout << "Median client balance: $" << median << '\n';
        }

        cout << "\nService distribution:\n";
        displayGroupedCounts(
            "SELECT s.name, COUNT(*) "
            "FROM clients c "
            "JOIN service_types s ON s.id = c.service_id "
            "GROUP BY s.id, s.name ORDER BY s.id;");

        cout << "\nRisk distribution:\n";
        displayGroupedCounts(
            "SELECT r.name, COUNT(*) "
            "FROM clients c "
            "JOIN risk_levels r ON r.id = c.risk_level_id "
            "GROUP BY r.id, r.name ORDER BY r.id;");

        cout << '\n';
    }

    bool undoLastChange() {
        Transaction transaction(database_);

        Statement history(
            database_.handle(),
            "SELECT "
            "id, action_type, client_id, "
            "before_name, before_service_id, before_age, before_balance, "
            "before_risk_id, before_email, before_phone, before_active, "
            "after_name, after_service_id, after_age, after_balance, "
            "after_risk_id, after_email, after_phone, after_active "
            "FROM client_history "
            "WHERE undone = 0 "
            "ORDER BY id DESC LIMIT 1;");

        if (history.step() != SQLITE_ROW) {
            transaction.rollback();
            return false;
        }

        const int historyId = history.columnInt(0);
        const string actionType = history.columnText(1);
        const int clientId = history.columnInt(2);

        if (actionType == "ADD") {
            Statement deletion(
                database_.handle(),
                "DELETE FROM clients WHERE id = ?;");
            deletion.bindInt(1, clientId);
            deletion.executeDone();
        }
        else if (actionType == "REMOVE") {
            const Client before = readHistoryClient(history, 3, clientId);
            insertClientWithId(before);
        }
        else if (actionType == "UPDATE") {
            const Client before = readHistoryClient(history, 3, clientId);
            updateClientWithoutHistory(before);
        }
        else {
            throw DatabaseException(
                "Unsupported history action type: " + actionType);
        }

        Statement markUndone(
            database_.handle(),
            "UPDATE client_history SET undone = 1 WHERE id = ?;");
        markUndone.bindInt(1, historyId);
        markUndone.executeDone();

        recordAudit(
            "UNDO_" + actionType,
            clientId,
            "Reversed history record " + std::to_string(historyId));

        transaction.commit();
        return true;
    }

    void displayRecentAuditEvents() const {
        cout << "\nRecent Database Audit Events\n";
        cout << "----------------------------\n";

        Statement statement(
            database_.handle(),
            "SELECT id, event_type, client_id, details, occurred_at "
            "FROM audit_events "
            "ORDER BY id DESC LIMIT ?;");

        statement.bindInt(1, Constants::kAuditRowsToDisplay);

        bool found = false;
        while (statement.step() == SQLITE_ROW) {
            found = true;
            cout << "#" << statement.columnInt(0)
                << " | " << statement.columnText(4)
                << " | " << statement.columnText(1)
                << " | client=";

            if (statement.columnIsNull(2)) {
                cout << "n/a";
            }
            else {
                cout << statement.columnInt(2);
            }

            cout << " | " << statement.columnText(3) << '\n';
        }

        if (!found) {
            cout << "No database audit events are available.\n";
        }

        cout << '\n';
    }

    void displayRecentHistory() const {
        cout << "\nRecent Persistent Change History\n";
        cout << "--------------------------------\n";

        Statement statement(
            database_.handle(),
            "SELECT id, action_type, client_id, undone, created_at "
            "FROM client_history "
            "ORDER BY id DESC LIMIT ?;");

        statement.bindInt(1, Constants::kHistoryRowsToDisplay);

        bool found = false;
        while (statement.step() == SQLITE_ROW) {
            found = true;
            cout << "#" << statement.columnInt(0)
                << " | " << statement.columnText(4)
                << " | " << statement.columnText(1)
                << " | client=" << statement.columnInt(2)
                << " | undone=" << (statement.columnInt(3) ? "yes" : "no")
                << '\n';
        }

        if (!found) {
            cout << "No history records are available.\n";
        }

        cout << '\n';
    }

    void displayDatabaseStatus() const {
        cout << "\nDatabase Status\n";
        cout << "---------------\n";
        cout << "Database file: " << database_.fileName() << '\n';
        cout << "SQLite version: " << sqlite3_libversion() << '\n';

        displayScalar("Client rows", "SELECT COUNT(*) FROM clients;");
        displayScalar("Audit rows", "SELECT COUNT(*) FROM audit_events;");
        displayScalar("History rows", "SELECT COUNT(*) FROM client_history;");

        cout << "Foreign keys: " << pragmaValue("foreign_keys") << '\n';
        cout << "Journal mode: " << pragmaValue("journal_mode") << '\n';
        cout << "Synchronous mode: " << pragmaValue("synchronous") << "\n\n";
    }

    void displayIndexes() const {
        cout << "\nDatabase Indexes\n";
        cout << "----------------\n";

        Statement statement(
            database_.handle(),
            "SELECT name, tbl_name, sql "
            "FROM sqlite_master "
            "WHERE type = 'index' AND name NOT LIKE 'sqlite_autoindex_%' "
            "ORDER BY tbl_name, name;");

        bool found = false;
        while (statement.step() == SQLITE_ROW) {
            found = true;
            cout << statement.columnText(0)
                << " on " << statement.columnText(1) << '\n';
            cout << "  " << statement.columnText(2) << '\n';
        }

        if (!found) {
            cout << "No user-created indexes were found.\n";
        }

        cout << '\n';
    }

    void runIntegrityCheck() const {
        cout << "\nSQLite Integrity Check\n";
        cout << "----------------------\n";

        Statement statement(
            database_.handle(),
            "PRAGMA integrity_check;");

        while (statement.step() == SQLITE_ROW) {
            cout << statement.columnText(0) << '\n';
        }

        cout << '\n';
    }

    void displayQueryPlanExamples() const {
        cout << "\nQuery Plan Examples\n";
        cout << "-------------------\n";
        cout << "These plans demonstrate how SQLite can use database indexes.\n\n";

        displayQueryPlan(
            "Indexed name-prefix lookup",
            "EXPLAIN QUERY PLAN "
            "SELECT * FROM clients "
            "WHERE normalized_name >= 'a' "
            "AND normalized_name < 'b' "
            "ORDER BY normalized_name;");

        displayQueryPlan(
            "Indexed balance ranking",
            "EXPLAIN QUERY PLAN "
            "SELECT * FROM clients "
            "ORDER BY account_balance DESC LIMIT 5;");
    }

    void checkpointWal() const {
        Statement statement(
            database_.handle(),
            "PRAGMA wal_checkpoint(TRUNCATE);");

        if (statement.step() == SQLITE_ROW) {
            cout << "WAL checkpoint completed. Busy="
                << statement.columnInt(0)
                << ", log frames=" << statement.columnInt(1)
                << ", checkpointed frames=" << statement.columnInt(2)
                << ".\n\n";
        }
    }

private:
    Database& database_;

    void initializeSchema() {
        database_.execute(
            "CREATE TABLE IF NOT EXISTS schema_metadata ("
            "key TEXT PRIMARY KEY, "
            "value TEXT NOT NULL"
            ");");

        database_.execute(
            "CREATE TABLE IF NOT EXISTS service_types ("
            "id INTEGER PRIMARY KEY, "
            "name TEXT NOT NULL UNIQUE, "
            "CHECK(id IN (1, 2))"
            ");");

        database_.execute(
            "CREATE TABLE IF NOT EXISTS risk_levels ("
            "id INTEGER PRIMARY KEY, "
            "name TEXT NOT NULL UNIQUE, "
            "CHECK(id IN (1, 2, 3))"
            ");");

        database_.execute(
            "CREATE TABLE IF NOT EXISTS clients ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "name TEXT NOT NULL CHECK(length(trim(name)) > 0), "
            "normalized_name TEXT NOT NULL, "
            "service_id INTEGER NOT NULL, "
            "age INTEGER NOT NULL CHECK(age BETWEEN 18 AND 120), "
            "account_balance REAL NOT NULL "
            "CHECK(account_balance >= 0 AND account_balance <= 1000000000), "
            "risk_level_id INTEGER NOT NULL, "
            "email TEXT NOT NULL COLLATE NOCASE "
            "CHECK(length(trim(email)) > 0), "
            "phone TEXT NOT NULL CHECK(length(trim(phone)) > 0), "
            "active INTEGER NOT NULL DEFAULT 1 CHECK(active IN (0, 1)), "
            "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP, "
            "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP, "
            "FOREIGN KEY(service_id) REFERENCES service_types(id), "
            "FOREIGN KEY(risk_level_id) REFERENCES risk_levels(id), "
            "UNIQUE(email)"
            ");");

        database_.execute(
            "CREATE TABLE IF NOT EXISTS audit_events ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "event_type TEXT NOT NULL, "
            "client_id INTEGER, "
            "details TEXT NOT NULL DEFAULT '', "
            "occurred_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
            ");");

        database_.execute(
            "CREATE TABLE IF NOT EXISTS client_history ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "action_type TEXT NOT NULL CHECK(action_type IN ('ADD','UPDATE','REMOVE')), "
            "client_id INTEGER NOT NULL, "
            "before_name TEXT, "
            "before_service_id INTEGER, "
            "before_age INTEGER, "
            "before_balance REAL, "
            "before_risk_id INTEGER, "
            "before_email TEXT, "
            "before_phone TEXT, "
            "before_active INTEGER, "
            "after_name TEXT, "
            "after_service_id INTEGER, "
            "after_age INTEGER, "
            "after_balance REAL, "
            "after_risk_id INTEGER, "
            "after_email TEXT, "
            "after_phone TEXT, "
            "after_active INTEGER, "
            "undone INTEGER NOT NULL DEFAULT 0 CHECK(undone IN (0, 1)), "
            "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
            ");");

        database_.execute(
            "CREATE INDEX IF NOT EXISTS idx_clients_normalized_name "
            "ON clients(normalized_name);");

        database_.execute(
            "CREATE INDEX IF NOT EXISTS idx_clients_service "
            "ON clients(service_id);");

        database_.execute(
            "CREATE INDEX IF NOT EXISTS idx_clients_risk "
            "ON clients(risk_level_id);");

        database_.execute(
            "CREATE INDEX IF NOT EXISTS idx_clients_active "
            "ON clients(active);");

        database_.execute(
            "CREATE INDEX IF NOT EXISTS idx_clients_balance "
            "ON clients(account_balance DESC);");

        database_.execute(
            "CREATE INDEX IF NOT EXISTS idx_history_undo "
            "ON client_history(undone, id DESC);");

        database_.execute(
            "CREATE INDEX IF NOT EXISTS idx_audit_occurred_at "
            "ON audit_events(occurred_at DESC, id DESC);");

        database_.execute(
            "INSERT INTO schema_metadata(key, value) VALUES('schema_version', '1') "
            "ON CONFLICT(key) DO UPDATE SET value = excluded.value;");
    }

    void seedReferenceData() {
        Transaction transaction(database_);

        database_.execute(
            "INSERT OR IGNORE INTO service_types(id, name) VALUES"
            "(1, 'Brokerage'),"
            "(2, 'Retirement');");

        database_.execute(
            "INSERT OR IGNORE INTO risk_levels(id, name) VALUES"
            "(1, 'Conservative'),"
            "(2, 'Moderate'),"
            "(3, 'Aggressive');");

        transaction.commit();
    }

    void seedClientsIfEmpty() {
        // A metadata flag prevents the original demonstration records from being
        // recreated later if a user intentionally removes every client
        Statement seedFlag(
            database_.handle(),
            "SELECT value FROM schema_metadata "
            "WHERE key = 'initial_seed_complete';");

        if (seedFlag.step() == SQLITE_ROW) {
            return;
        }

        Statement countStatement(
            database_.handle(),
            "SELECT COUNT(*) FROM clients;");

        if (countStatement.step() != SQLITE_ROW) {
            throw DatabaseException("Unable to determine whether seed data exists.");
        }

        Transaction transaction(database_);

        if (countStatement.columnInt(0) == 0) {
            // These are the same demonstration records introduced in Milestone Three
            // They are inserted only during initial database setup
            const vector<Client> seeds = {
                { 1, "Bob Jones", ServiceType::Brokerage, 42, 148250.00,
                  RiskLevel::Moderate, "bob.jones@example.com", "520-555-0101", true, "", "" },
                { 2, "Sarah Davis", ServiceType::Retirement, 57, 286400.00,
                  RiskLevel::Conservative, "sarah.davis@example.com", "520-555-0102", true, "", "" },
                { 3, "Amy Friendly", ServiceType::Brokerage, 31, 72450.00,
                  RiskLevel::Aggressive, "amy.friendly@example.com", "520-555-0103", true, "", "" },
                { 4, "Johnny Smith", ServiceType::Brokerage, 48, 391750.00,
                  RiskLevel::Moderate, "johnny.smith@example.com", "520-555-0104", true, "", "" },
                { 5, "Carol Spears", ServiceType::Retirement, 63, 512900.00,
                  RiskLevel::Conservative, "carol.spears@example.com", "520-555-0105", true, "", "" }
            };

            for (const Client& client : seeds) {
                insertClientWithId(client);
            }

            recordAudit(
                "DATABASE_SEED",
                std::nullopt,
                "Inserted five demonstration clients from the capstone artifact.");
        }

        database_.execute(
            "INSERT INTO schema_metadata(key, value) "
            "VALUES('initial_seed_complete', '1') "
            "ON CONFLICT(key) DO UPDATE SET value = excluded.value;");

        transaction.commit();
    }

    static string clientSelectColumns() {
        return
            "c.id, c.name, c.service_id, c.age, c.account_balance, "
            "c.risk_level_id, c.email, c.phone, c.active, "
            "c.created_at, c.updated_at";
    }

    static Client readClient(const Statement& statement) {
        Client client;
        client.id = statement.columnInt(0);
        client.name = statement.columnText(1);
        client.service = static_cast<ServiceType>(statement.columnInt(2));
        client.age = statement.columnInt(3);
        client.accountBalance = statement.columnDouble(4);
        client.riskLevel = static_cast<RiskLevel>(statement.columnInt(5));
        client.email = statement.columnText(6);
        client.phone = statement.columnText(7);
        client.active = statement.columnInt(8) != 0;
        client.createdAt = statement.columnText(9);
        client.updatedAt = statement.columnText(10);
        return client;
    }

    static Client readHistoryClient(
        const Statement& statement,
        int startColumn,
        int clientId) {

        Client client;
        client.id = clientId;
        client.name = statement.columnText(startColumn + 0);
        client.service = static_cast<ServiceType>(
            statement.columnInt(startColumn + 1));
        client.age = statement.columnInt(startColumn + 2);
        client.accountBalance = statement.columnDouble(startColumn + 3);
        client.riskLevel = static_cast<RiskLevel>(
            statement.columnInt(startColumn + 4));
        client.email = statement.columnText(startColumn + 5);
        client.phone = statement.columnText(startColumn + 6);
        client.active = statement.columnInt(startColumn + 7) != 0;
        return client;
    }

    vector<Client> queryClients(const string& sql) const {
        Statement statement(database_.handle(), sql);
        return collectClients(statement);
    }

    static vector<Client> collectClients(Statement& statement) {
        vector<Client> clients;

        while (true) {
            const int result = statement.step();

            if (result == SQLITE_ROW) {
                clients.push_back(readClient(statement));
                continue;
            }

            if (result == SQLITE_DONE) {
                break;
            }

            throw DatabaseException("Client query did not complete successfully.");
        }

        return clients;
    }

    static string normalizeName(string value) {
        value = InputValidator::trim(value);

        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char character) {
                return static_cast<char>(std::tolower(character));
            });

        return value;
    }

    static string escapeLike(const string& value) {
        string escaped;
        escaped.reserve(value.size());

        for (const char character : value) {
            if (character == '%' || character == '_' || character == '\\') {
                escaped.push_back('\\');
            }
            escaped.push_back(character);
        }

        return escaped;
    }

    void bindClientSnapshot(
        Statement& statement,
        int startIndex,
        const optional<Client>& client) const {

        if (!client.has_value()) {
            for (int offset = 0; offset < 8; ++offset) {
                statement.bindNull(startIndex + offset);
            }
            return;
        }

        statement.bindText(startIndex + 0, client->name);
        statement.bindInt(startIndex + 1, static_cast<int>(client->service));
        statement.bindInt(startIndex + 2, client->age);
        statement.bindDouble(startIndex + 3, client->accountBalance);
        statement.bindInt(startIndex + 4, static_cast<int>(client->riskLevel));
        statement.bindText(startIndex + 5, client->email);
        statement.bindText(startIndex + 6, client->phone);
        statement.bindInt(startIndex + 7, client->active ? 1 : 0);
    }

    void recordHistory(
        const string& actionType,
        const optional<Client>& before,
        const optional<Client>& after) {

        const int clientId = before.has_value()
            ? before->id
            : after->id;

        Statement statement(
            database_.handle(),
            "INSERT INTO client_history ("
            "action_type, client_id, "
            "before_name, before_service_id, before_age, before_balance, "
            "before_risk_id, before_email, before_phone, before_active, "
            "after_name, after_service_id, after_age, after_balance, "
            "after_risk_id, after_email, after_phone, after_active"
            ") VALUES ("
            "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?"
            ");");

        statement.bindText(1, actionType);
        statement.bindInt(2, clientId);
        bindClientSnapshot(statement, 3, before);
        bindClientSnapshot(statement, 11, after);
        statement.executeDone();
    }

    void recordAudit(
        const string& eventType,
        optional<int> clientId,
        const string& details) const {

        Statement statement(
            database_.handle(),
            "INSERT INTO audit_events(event_type, client_id, details) "
            "VALUES (?, ?, ?);");

        statement.bindText(1, eventType);
        if (clientId.has_value()) {
            statement.bindInt(2, *clientId);
        }
        else {
            statement.bindNull(2);
        }
        statement.bindText(3, details);
        statement.executeDone();
    }

    void insertClientWithId(const Client& client) {
        Statement statement(
            database_.handle(),
            "INSERT INTO clients ("
            "id, name, normalized_name, service_id, age, account_balance, "
            "risk_level_id, email, phone, active"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");

        statement.bindInt(1, client.id);
        statement.bindText(2, client.name);
        statement.bindText(3, normalizeName(client.name));
        statement.bindInt(4, static_cast<int>(client.service));
        statement.bindInt(5, client.age);
        statement.bindDouble(6, client.accountBalance);
        statement.bindInt(7, static_cast<int>(client.riskLevel));
        statement.bindText(8, client.email);
        statement.bindText(9, client.phone);
        statement.bindInt(10, client.active ? 1 : 0);
        statement.executeDone();
    }

    void updateClientWithoutHistory(const Client& client) {
        Statement statement(
            database_.handle(),
            "UPDATE clients SET "
            "name = ?, normalized_name = ?, service_id = ?, age = ?, "
            "account_balance = ?, risk_level_id = ?, email = ?, phone = ?, "
            "active = ?, updated_at = CURRENT_TIMESTAMP "
            "WHERE id = ?;");

        statement.bindText(1, client.name);
        statement.bindText(2, normalizeName(client.name));
        statement.bindInt(3, static_cast<int>(client.service));
        statement.bindInt(4, client.age);
        statement.bindDouble(5, client.accountBalance);
        statement.bindInt(6, static_cast<int>(client.riskLevel));
        statement.bindText(7, client.email);
        statement.bindText(8, client.phone);
        statement.bindInt(9, client.active ? 1 : 0);
        statement.bindInt(10, client.id);
        statement.executeDone();
    }

    vector<double> orderedBalances() const {
        vector<double> balances;

        Statement statement(
            database_.handle(),
            "SELECT account_balance FROM clients "
            "ORDER BY account_balance ASC;");

        while (statement.step() == SQLITE_ROW) {
            balances.push_back(statement.columnDouble(0));
        }

        return balances;
    }

    void displayGroupedCounts(const string& sql) const {
        Statement statement(database_.handle(), sql);

        bool found = false;
        while (statement.step() == SQLITE_ROW) {
            found = true;
            cout << "  " << statement.columnText(0)
                << ": " << statement.columnInt(1) << '\n';
        }

        if (!found) {
            cout << "  No data.\n";
        }
    }

    void displayScalar(const string& label, const string& sql) const {
        Statement statement(database_.handle(), sql);
        if (statement.step() == SQLITE_ROW) {
            cout << label << ": " << statement.columnText(0) << '\n';
        }
    }

    string pragmaValue(const string& pragmaName) const {
        // pragmaName is supplied only by fixed application code
        Statement statement(
            database_.handle(),
            "PRAGMA " + pragmaName + ";");

        if (statement.step() == SQLITE_ROW) {
            return statement.columnText(0);
        }

        return "unknown";
    }

    void displayQueryPlan(
        const string& title,
        const string& sql) const {

        cout << title << ":\n";

        Statement statement(database_.handle(), sql);
        while (statement.step() == SQLITE_ROW) {
            cout << "  " << statement.columnText(3) << '\n';
        }

        cout << '\n';
    }
};

// -----------------------------------------------------------------------------
// Client Management User Interface
// -----------------------------------------------------------------------------

class ClientManager {
public:
    ClientManager(ClientRepository& repository, const Logger& logger)
        : repository_(repository), logger_(logger) {
    }

    void displayAllClients() const {
        displayClientCollection("All Clients", repository_.getAllClients());
    }

    void addClient() {
        cout << "\nAdd Client\n";
        cout << "----------\n";

        Client client;
        client.name = InputValidator::readNonEmptyLine("Client name: ");
        client.service = readServiceType();
        client.age = InputValidator::readIntInRange(
            "Age: ", Constants::kMinAge, Constants::kMaxAge);
        client.accountBalance = InputValidator::readDoubleInRange(
            "Account balance: $",
            Constants::kMinBalance,
            Constants::kMaxBalance);
        client.riskLevel = readRiskLevel();
        client.email = InputValidator::readNonEmptyLine("Email: ");
        client.phone = InputValidator::readNonEmptyLine("Phone: ");
        client.active = InputValidator::readYesNo("Active client? (y/n): ");

        try {
            const int id = repository_.addClient(client);
            logger_.write("CLIENT_ADD id=" + std::to_string(id));
            cout << "Client added successfully with ID " << id << ".\n\n";
        }
        catch (const DatabaseException& exception) {
            cout << "Client could not be added. " << exception.what() << "\n\n";
            logger_.write("CLIENT_ADD_DATABASE_ERROR");
        }
    }

    void editClient() {
        if (repository_.getAllClients().empty()) {
            cout << "There are no clients to edit.\n\n";
            return;
        }

        const int clientId = readExistingClientId("Enter client ID to edit: ");
        optional<Client> client = repository_.findClientById(clientId);

        if (!client.has_value()) {
            cout << "Client could not be found.\n\n";
            return;
        }

        const Client before = *client;
        Client after = *client;
        bool changed = false;
        bool continueEditing = true;

        while (continueEditing) {
            cout << "\nEdit Client: " << after.name
                << " (ID " << after.id << ")\n";
            cout << "1. Change name\n";
            cout << "2. Change service\n";
            cout << "3. Change age\n";
            cout << "4. Change account balance\n";
            cout << "5. Change risk level\n";
            cout << "6. Change email\n";
            cout << "7. Change phone\n";
            cout << "8. Change active status\n";
            cout << "9. Finish editing\n";

            const int choice = InputValidator::readIntInRange(
                "Selection: ", 1, 9);

            switch (choice) {
            case 1: {
                const string value =
                    InputValidator::readNonEmptyLine("New name: ");
                if (value != after.name) {
                    after.name = value;
                    changed = true;
                }
                break;
            }

            case 2: {
                const ServiceType value = readServiceType();
                if (value != after.service) {
                    after.service = value;
                    changed = true;
                }
                break;
            }

            case 3: {
                const int value = InputValidator::readIntInRange(
                    "New age: ", Constants::kMinAge, Constants::kMaxAge);
                if (value != after.age) {
                    after.age = value;
                    changed = true;
                }
                break;
            }

            case 4: {
                const double value = InputValidator::readDoubleInRange(
                    "New account balance: $",
                    Constants::kMinBalance,
                    Constants::kMaxBalance);
                if (value != after.accountBalance) {
                    after.accountBalance = value;
                    changed = true;
                }
                break;
            }

            case 5: {
                const RiskLevel value = readRiskLevel();
                if (value != after.riskLevel) {
                    after.riskLevel = value;
                    changed = true;
                }
                break;
            }

            case 6: {
                const string value =
                    InputValidator::readNonEmptyLine("New email: ");
                if (value != after.email) {
                    after.email = value;
                    changed = true;
                }
                break;
            }

            case 7: {
                const string value =
                    InputValidator::readNonEmptyLine("New phone: ");
                if (value != after.phone) {
                    after.phone = value;
                    changed = true;
                }
                break;
            }

            case 8: {
                const bool value =
                    InputValidator::readYesNo("Active client? (y/n): ");
                if (value != after.active) {
                    after.active = value;
                    changed = true;
                }
                break;
            }

            case 9:
                continueEditing = false;
                break;

            default:
                break;
            }
        }

        if (!changed) {
            cout << "No changes were made.\n\n";
            return;
        }

        try {
            if (repository_.updateClient(before, after)) {
                logger_.write("CLIENT_UPDATE id=" + std::to_string(clientId));
                cout << "Client updated successfully.\n\n";
            }
            else {
                cout << "Client update failed because the record changed or disappeared.\n\n";
            }
        }
        catch (const DatabaseException& exception) {
            cout << "Client could not be updated. " << exception.what() << "\n\n";
            logger_.write("CLIENT_UPDATE_DATABASE_ERROR");
        }
    }

    void removeClient() {
        if (repository_.getAllClients().empty()) {
            cout << "There are no clients to remove.\n\n";
            return;
        }

        const int clientId = readExistingClientId("Enter client ID to remove: ");
        const optional<Client> client = repository_.findClientById(clientId);

        if (!client.has_value()) {
            cout << "Client could not be found.\n\n";
            return;
        }

        if (!InputValidator::readYesNo(
            "Remove " + client->name + "? (y/n): ")) {
            cout << "Removal canceled.\n\n";
            return;
        }

        try {
            if (repository_.removeClient(*client)) {
                logger_.write("CLIENT_REMOVE id=" + std::to_string(clientId));
                cout << "Client removed successfully.\n\n";
            }
            else {
                cout << "Client could not be removed.\n\n";
            }
        }
        catch (const DatabaseException& exception) {
            cout << "Client could not be removed. " << exception.what() << "\n\n";
            logger_.write("CLIENT_REMOVE_DATABASE_ERROR");
        }
    }

    void searchMenu() const {
        bool running = true;

        while (running) {
            cout << "\nSearch Clients\n";
            cout << "--------------\n";
            cout << "1. Search by ID\n";
            cout << "2. Search exact name\n";
            cout << "3. Search name prefix\n";
            cout << "4. Search by service\n";
            cout << "5. Search by balance range\n";
            cout << "6. Search by risk level\n";
            cout << "7. Search by active status\n";
            cout << "8. Return to main menu\n";

            const int choice = InputValidator::readIntInRange(
                "Selection: ", 1, 8);

            switch (choice) {
            case 1:
                searchById();
                break;
            case 2:
                searchByExactName();
                break;
            case 3:
                searchByNamePrefix();
                break;
            case 4:
                displayClientCollection(
                    "Clients Matching Service",
                    repository_.searchByService(readServiceType()));
                break;
            case 5:
                searchByBalanceRange();
                break;
            case 6:
                displayClientCollection(
                    "Clients Matching Risk Level",
                    repository_.searchByRisk(readRiskLevel()));
                break;
            case 7:
                displayClientCollection(
                    "Clients Matching Status",
                    repository_.searchByActive(
                        InputValidator::readYesNo("Show active clients? (y/n): ")));
                break;
            case 8:
                running = false;
                break;
            default:
                break;
            }
        }
    }

    void filterMenu() const {
        bool running = true;

        while (running) {
            cout << "\nFilter Clients with SQL\n";
            cout << "-----------------------\n";
            cout << "1. Brokerage clients\n";
            cout << "2. Retirement clients\n";
            cout << "3. Conservative risk\n";
            cout << "4. Moderate risk\n";
            cout << "5. Aggressive risk\n";
            cout << "6. Active clients\n";
            cout << "7. Inactive clients\n";
            cout << "8. Balance at or above threshold\n";
            cout << "9. Compound filter\n";
            cout << "10. Return to main menu\n";

            const int choice = InputValidator::readIntInRange(
                "Selection: ", 1, 10);

            switch (choice) {
            case 1:
                displayClientCollection(
                    "Brokerage Clients",
                    repository_.searchByService(ServiceType::Brokerage));
                break;
            case 2:
                displayClientCollection(
                    "Retirement Clients",
                    repository_.searchByService(ServiceType::Retirement));
                break;
            case 3:
                displayClientCollection(
                    "Conservative Risk Clients",
                    repository_.searchByRisk(RiskLevel::Conservative));
                break;
            case 4:
                displayClientCollection(
                    "Moderate Risk Clients",
                    repository_.searchByRisk(RiskLevel::Moderate));
                break;
            case 5:
                displayClientCollection(
                    "Aggressive Risk Clients",
                    repository_.searchByRisk(RiskLevel::Aggressive));
                break;
            case 6:
                displayClientCollection(
                    "Active Clients",
                    repository_.searchByActive(true));
                break;
            case 7:
                displayClientCollection(
                    "Inactive Clients",
                    repository_.searchByActive(false));
                break;
            case 8: {
                const double minimum = InputValidator::readDoubleInRange(
                    "Minimum balance: $",
                    Constants::kMinBalance,
                    Constants::kMaxBalance);

                displayClientCollection(
                    "Clients At or Above Balance Threshold",
                    repository_.searchByBalanceRange(
                        minimum,
                        Constants::kMaxBalance));
                break;
            }
            case 9: {
                const ServiceType service = readServiceType();
                const RiskLevel risk = readRiskLevel();
                const bool active = InputValidator::readYesNo(
                    "Require active status? (y/n): ");
                const double minimum = InputValidator::readDoubleInRange(
                    "Minimum balance: $",
                    Constants::kMinBalance,
                    Constants::kMaxBalance);

                displayClientCollection(
                    "Compound SQL Filter Results",
                    repository_.compoundFilter(
                        service,
                        risk,
                        active,
                        minimum));
                break;
            }
            case 10:
                running = false;
                break;
            default:
                break;
            }
        }
    }

    void sortMenu() const {
        cout << "\nSort Clients with ORDER BY\n";
        cout << "--------------------------\n";
        cout << "1. ID ascending\n";
        cout << "2. Name ascending\n";
        cout << "3. Balance highest first\n";
        cout << "4. Balance lowest first\n";
        cout << "5. Service then name\n";
        cout << "6. Risk then name\n";
        cout << "7. Age ascending\n";

        const int choice = InputValidator::readIntInRange(
            "Selection: ", 1, 7);

        displayClientCollection(
            "Sorted Client View",
            repository_.sortedClients(choice));
    }

    void rankingsMenu() const {
        cout << "\nClient Rankings with ORDER BY and LIMIT\n";
        cout << "---------------------------------------\n";
        cout << "1. Highest balances\n";
        cout << "2. Lowest balances\n";

        const int choice = InputValidator::readIntInRange(
            "Selection: ", 1, 2);

        const int count = InputValidator::readIntInRange(
            "How many clients should be ranked? ",
            1,
            std::max(
                1,
                static_cast<int>(repository_.getAllClients().size())));

        displayClientCollection(
            choice == 1 ? "Highest Client Balances" : "Lowest Client Balances",
            repository_.topClientsByBalance(count, choice == 1));
    }

    void displayAnalytics() const {
        repository_.displayAnalytics();
    }

    void undoLastChange() {
        try {
            if (repository_.undoLastChange()) {
                logger_.write("CLIENT_UNDO");
                cout << "Most recent database change was reversed successfully.\n\n";
            }
            else {
                cout << "There is no database change available to undo.\n\n";
            }
        }
        catch (const DatabaseException& exception) {
            cout << "Undo failed. " << exception.what() << "\n\n";
            logger_.write("CLIENT_UNDO_DATABASE_ERROR");
        }
    }

    void databaseToolsMenu() const {
        bool running = true;

        while (running) {
            cout << "\nDatabase Tools\n";
            cout << "--------------\n";
            cout << "1. Database status\n";
            cout << "2. SQLite integrity check\n";
            cout << "3. List indexes\n";
            cout << "4. Show query plan examples\n";
            cout << "5. Show recent database audit events\n";
            cout << "6. Show persistent change history\n";
            cout << "7. Checkpoint WAL file\n";
            cout << "8. Return to main menu\n";

            const int choice = InputValidator::readIntInRange(
                "Selection: ", 1, 8);

            switch (choice) {
            case 1:
                repository_.displayDatabaseStatus();
                break;
            case 2:
                repository_.runIntegrityCheck();
                break;
            case 3:
                repository_.displayIndexes();
                break;
            case 4:
                repository_.displayQueryPlanExamples();
                break;
            case 5:
                repository_.displayRecentAuditEvents();
                break;
            case 6:
                repository_.displayRecentHistory();
                break;
            case 7:
                repository_.checkpointWal();
                break;
            case 8:
                running = false;
                break;
            default:
                break;
            }
        }
    }

private:
    ClientRepository& repository_;
    const Logger& logger_;

    int readExistingClientId(const string& prompt) const {
        while (true) {
            const int clientId = InputValidator::readIntInRange(
                prompt,
                1,
                std::numeric_limits<int>::max());

            if (repository_.findClientById(clientId).has_value()) {
                return clientId;
            }

            cout << "No client exists with ID "
                << clientId << ". Please try again.\n";
        }
    }

    void searchById() const {
        const int clientId = InputValidator::readIntInRange(
            "Client ID: ",
            1,
            std::numeric_limits<int>::max());

        const optional<Client> client = repository_.findClientById(clientId);

        if (client.has_value()) {
            displayClientCollection("Client ID Search", { *client });
        }
        else {
            cout << "No client exists with ID " << clientId << ".\n\n";
        }
    }

    void searchByExactName() const {
        const string name =
            InputValidator::readNonEmptyLine("Exact client name: ");

        displayClientCollection(
            "Exact Name Search",
            repository_.searchExactName(name));
    }

    void searchByNamePrefix() const {
        const string prefix =
            InputValidator::readNonEmptyLine("Name prefix: ");

        displayClientCollection(
            "Name Prefix Search",
            repository_.searchNamePrefix(prefix));
    }

    void searchByBalanceRange() const {
        const double minimum = InputValidator::readDoubleInRange(
            "Minimum balance: $",
            Constants::kMinBalance,
            Constants::kMaxBalance);

        const double maximum = InputValidator::readDoubleInRange(
            "Maximum balance: $",
            minimum,
            Constants::kMaxBalance);

        displayClientCollection(
            "Balance Range Search",
            repository_.searchByBalanceRange(minimum, maximum));
    }

    static ServiceType readServiceType() {
        const int choice = InputValidator::readIntInRange(
            "Select service (1 = Brokerage, 2 = Retirement): ",
            Constants::kMinServiceChoice,
            Constants::kMaxServiceChoice);

        return static_cast<ServiceType>(choice);
    }

    static RiskLevel readRiskLevel() {
        const int choice = InputValidator::readIntInRange(
            "Select risk level (1 = Conservative, 2 = Moderate, 3 = Aggressive): ",
            Constants::kMinRiskChoice,
            Constants::kMaxRiskChoice);

        return static_cast<RiskLevel>(choice);
    }

    static void displayClientCollection(
        const string& title,
        const vector<Client>& clients) {

        cout << "\n" << title << '\n';
        cout << string(title.size(), '-') << '\n';

        if (clients.empty()) {
            cout << "No matching client records were found.\n\n";
            return;
        }

        cout << std::left
            << std::setw(5) << "ID"
            << std::setw(22) << "Name"
            << std::setw(13) << "Service"
            << std::setw(6) << "Age"
            << std::setw(14) << "Balance"
            << std::setw(14) << "Risk"
            << std::setw(9) << "Status"
            << '\n';

        cout << string(83, '-') << '\n';

        for (const Client& client : clients) {
            std::ostringstream balance;
            balance << '$' << std::fixed << std::setprecision(2)
                << client.accountBalance;

            cout << std::left
                << std::setw(5) << client.id
                << std::setw(22) << truncate(client.name, 20)
                << std::setw(13) << serviceTypeToString(client.service)
                << std::setw(6) << client.age
                << std::setw(14) << balance.str()
                << std::setw(14) << riskLevelToString(client.riskLevel)
                << std::setw(9) << (client.active ? "Active" : "Inactive")
                << '\n';
        }

        cout << '\n';
    }

    static string truncate(const string& value, std::size_t maxLength) {
        if (value.size() <= maxLength) {
            return value;
        }

        if (maxLength <= 3) {
            return value.substr(0, maxLength);
        }

        return value.substr(0, maxLength - 3) + "...";
    }
};

// -----------------------------------------------------------------------------
// Main Application
// -----------------------------------------------------------------------------

class InvestmentCompanyApplication {
public:
    InvestmentCompanyApplication()
        : logger_("cs499_audit.log"),
        authenticator_(logger_),
        database_("cs499_clients.db"),
        repository_(database_),
        clientManager_(repository_, logger_) {
    }

    int run() {
        displayHeader();

        if (!authenticator_.authenticate()) {
            return 1;
        }

        bool running = true;

        while (running) {
            displayMainMenu();

            const int choice = InputValidator::readIntInRange(
                "Selection: ", 1, 12);

            try {
                switch (choice) {
                case 1:
                    clientManager_.displayAllClients();
                    break;

                case 2:
                    clientManager_.addClient();
                    break;

                case 3:
                    clientManager_.editClient();
                    break;

                case 4:
                    clientManager_.removeClient();
                    break;

                case 5:
                    clientManager_.searchMenu();
                    break;

                case 6:
                    clientManager_.filterMenu();
                    break;

                case 7:
                    clientManager_.sortMenu();
                    break;

                case 8:
                    clientManager_.rankingsMenu();
                    break;

                case 9:
                    clientManager_.displayAnalytics();
                    break;

                case 10:
                    clientManager_.undoLastChange();
                    break;

                case 11:
                    clientManager_.databaseToolsMenu();
                    break;

                case 12:
                    running = false;
                    logger_.write("APPLICATION_EXIT");
                    cout << "Program closed safely.\n";
                    break;

                default:
                    break;
                }
            }
            catch (const DatabaseException& exception) {
                cout << "Database operation failed: "
                    << exception.what() << "\n\n";
                logger_.write("DATABASE_OPERATION_ERROR");
            }
        }

        return 0;
    }

private:
    Logger logger_;
    Authenticator authenticator_;
    Database database_;
    ClientRepository repository_;
    ClientManager clientManager_;

    static void displayHeader() {
        cout << "Created by Brianna Reed\n";
        cout << "Investment Company Client Management System\n";
        cout << "Database Enhancement: Persistent SQLite Storage\n";
        cout << "=======================================================\n\n";
    }

    static void displayMainMenu() {
        cout << "Main Menu\n";
        cout << "---------\n";
        cout << "1.  Display all clients\n";
        cout << "2.  Add client\n";
        cout << "3.  Edit client\n";
        cout << "4.  Remove client\n";
        cout << "5.  Search clients\n";
        cout << "6.  Filter clients\n";
        cout << "7.  Sort clients\n";
        cout << "8.  Client rankings\n";
        cout << "9.  Portfolio analytics\n";
        cout << "10. Undo last database change\n";
        cout << "11. Database tools\n";
        cout << "12. Exit\n";
    }
};

// -----------------------------------------------------------------------------
// Program Entry Point
// -----------------------------------------------------------------------------

int main() {
    try {
        InvestmentCompanyApplication application;
        return application.run();
    }
    catch (const DatabaseException& exception) {
        std::cerr << "Fatal database error: " << exception.what() << '\n';
        return 1;
    }
    catch (const std::exception& exception) {
        std::cerr << "Fatal application error: " << exception.what() << '\n';
        return 1;
    }
}
