/*
 * CS 499 Computer Science Capstone
 * Milestone Three: Enhancement Two - Algorithms and Data Structures
 * Student: Brianna Reed
 * Date: September 27, 2026
 *
 * Artifact origin:
 * CS 410 Software Reverse Engineering Project One, later refactored during
 * CS 499 Milestone Two (Software Design and Engineering).
 *
 * Enhancement purpose:
 * Expand the refactored client-management application into a richer in-memory
 * system that demonstrates deliberate selection and use of algorithms and data
 * structures. The primary client store is an unordered_map for average O(1)
 * lookup by ID. A map<string, set<int>> provides an ordered secondary name
 * index that supports exact-name and prefix searches. Vectors are created for
 * sortable views, a priority_queue supports Top-K rankings, and a stack stores
 * reversible mutations for one-level-at-a-time undo history.
 *
 * IMPORTANT CAPSTONE BOUNDARY:
 * This milestone intentionally remains in memory. Database persistence,
 * relational schema design, parameterized SQL, transactions, and database
 * indexing are reserved for Enhancement Three (Databases) so each milestone
 * demonstrates a distinct area of computer science growth.
 */

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using std::cin;
using std::cout;
using std::endl;
using std::getline;
using std::map;
using std::ofstream;
using std::optional;
using std::priority_queue;
using std::set;
using std::stack;
using std::string;
using std::unordered_map;
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
}

// -----------------------------------------------------------------------------
// Enumerations
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

enum class ActionType {
    Add,
    Remove,
    Update
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
// Client Model
// -----------------------------------------------------------------------------

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
};

// Stores enough state to reverse a mutation.
struct Action {
    ActionType type;
    optional<Client> before;
    optional<Client> after;
    string description;
};

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
// Client Manager
// -----------------------------------------------------------------------------

class ClientManager {
public:
    explicit ClientManager(const Logger& logger)
        : logger_(logger) {

        // Synthetic portfolio values/contact fields are demonstration data added
        // for the algorithms milestone. Original names and service selections are
        // preserved from the recovered CS 410 artifact.
        seedClient({
            1, "Bob Jones", ServiceType::Brokerage, 42, 148250.00,
            RiskLevel::Moderate, "bob.jones@example.com", "520-555-0101", true });

        seedClient({
            2, "Sarah Davis", ServiceType::Retirement, 57, 286400.00,
            RiskLevel::Conservative, "sarah.davis@example.com", "520-555-0102", true });

        seedClient({
            3, "Amy Friendly", ServiceType::Brokerage, 31, 72450.00,
            RiskLevel::Aggressive, "amy.friendly@example.com", "520-555-0103", true });

        seedClient({
            4, "Johnny Smith", ServiceType::Brokerage, 48, 391750.00,
            RiskLevel::Moderate, "johnny.smith@example.com", "520-555-0104", true });

        seedClient({
            5, "Carol Spears", ServiceType::Retirement, 63, 512900.00,
            RiskLevel::Conservative, "carol.spears@example.com", "520-555-0105", true });

        nextClientId_ = 6;
    }

    // -------------------------------------------------------------------------
    // Main Feature Entrypoints
    // -------------------------------------------------------------------------

    void displayAllClients() const {
        const vector<Client> clients = getClientsSortedById();
        displayClientCollection("All Clients", clients);
    }

    void addClient() {
        cout << "\nAdd Client\n";
        cout << "----------\n";

        Client client;
        client.id = nextClientId_++;
        client.name = InputValidator::readNonEmptyLine("Client name: ");
        client.service = readServiceType();
        client.age = InputValidator::readIntInRange(
            "Age: ", Constants::kMinAge, Constants::kMaxAge);
        client.accountBalance = InputValidator::readDoubleInRange(
            "Account balance: $", Constants::kMinBalance, Constants::kMaxBalance);
        client.riskLevel = readRiskLevel();
        client.email = InputValidator::readNonEmptyLine("Email: ");
        client.phone = InputValidator::readNonEmptyLine("Phone: ");
        client.active = InputValidator::readYesNo("Active client? (y/n): ");

        insertClient(client);

        undoHistory_.push({
            ActionType::Add,
            std::nullopt,
            client,
            "Add client ID " + std::to_string(client.id)
            });

        logger_.write(
            "CLIENT_ADD id=" + std::to_string(client.id) +
            " name=" + client.name);

        cout << "Client added successfully with ID " << client.id << ".\n\n";
    }

    void editClient() {
        if (clients_.empty()) {
            cout << "There are no clients to edit.\n\n";
            return;
        }

        const int clientId = readExistingClientId("Enter client ID to edit: ");
        Client* client = findClientById(clientId);

        if (client == nullptr) {
            cout << "Client could not be found.\n\n";
            return;
        }

        const Client before = *client;
        bool changed = false;
        bool continueEditing = true;

        while (continueEditing) {
            cout << "\nEdit Client: " << client->name << " (ID " << client->id << ")\n";
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
                const string oldName = client->name;
                const string newName =
                    InputValidator::readNonEmptyLine("New name: ");

                if (newName != oldName) {
                    removeFromNameIndex(oldName, client->id);
                    client->name = newName;
                    addToNameIndex(client->name, client->id);
                    changed = true;
                }
                break;
            }

            case 2: {
                const ServiceType newService = readServiceType();
                if (newService != client->service) {
                    client->service = newService;
                    changed = true;
                }
                break;
            }

            case 3: {
                const int newAge = InputValidator::readIntInRange(
                    "New age: ", Constants::kMinAge, Constants::kMaxAge);
                if (newAge != client->age) {
                    client->age = newAge;
                    changed = true;
                }
                break;
            }

            case 4: {
                const double newBalance = InputValidator::readDoubleInRange(
                    "New account balance: $",
                    Constants::kMinBalance,
                    Constants::kMaxBalance);
                if (newBalance != client->accountBalance) {
                    client->accountBalance = newBalance;
                    changed = true;
                }
                break;
            }

            case 5: {
                const RiskLevel newRisk = readRiskLevel();
                if (newRisk != client->riskLevel) {
                    client->riskLevel = newRisk;
                    changed = true;
                }
                break;
            }

            case 6: {
                const string newEmail =
                    InputValidator::readNonEmptyLine("New email: ");
                if (newEmail != client->email) {
                    client->email = newEmail;
                    changed = true;
                }
                break;
            }

            case 7: {
                const string newPhone =
                    InputValidator::readNonEmptyLine("New phone: ");
                if (newPhone != client->phone) {
                    client->phone = newPhone;
                    changed = true;
                }
                break;
            }

            case 8: {
                const bool newStatus =
                    InputValidator::readYesNo("Active client? (y/n): ");
                if (newStatus != client->active) {
                    client->active = newStatus;
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

        const Client after = *client;
        undoHistory_.push({
            ActionType::Update,
            before,
            after,
            "Update client ID " + std::to_string(clientId)
            });

        logger_.write(
            "CLIENT_UPDATE id=" + std::to_string(clientId));

        cout << "Client updated successfully.\n\n";
    }

    void removeClient() {
        if (clients_.empty()) {
            cout << "There are no clients to remove.\n\n";
            return;
        }

        const int clientId = readExistingClientId("Enter client ID to remove: ");
        const Client* client = findClientById(clientId);

        if (client == nullptr) {
            cout << "Client could not be found.\n\n";
            return;
        }

        const Client removedClient = *client;

        if (!InputValidator::readYesNo(
            "Remove " + removedClient.name + "? (y/n): ")) {
            cout << "Removal canceled.\n\n";
            return;
        }

        eraseClient(clientId);

        undoHistory_.push({
            ActionType::Remove,
            removedClient,
            std::nullopt,
            "Remove client ID " + std::to_string(clientId)
            });

        logger_.write(
            "CLIENT_REMOVE id=" + std::to_string(clientId) +
            " name=" + removedClient.name);

        cout << "Client removed successfully.\n\n";
    }

    // -------------------------------------------------------------------------
    // Search Menu
    // -------------------------------------------------------------------------

    void searchMenu() const {
        bool searching = true;

        while (searching) {
            cout << "\nSearch Clients\n";
            cout << "--------------\n";
            cout << "1. Search by client ID\n";
            cout << "2. Search by exact name\n";
            cout << "3. Search by name prefix\n";
            cout << "4. Search by service\n";
            cout << "5. Search by account balance range\n";
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
                searchByService();
                break;
            case 5:
                searchByBalanceRange();
                break;
            case 6:
                searchByRiskLevel();
                break;
            case 7:
                searchByActiveStatus();
                break;
            case 8:
                searching = false;
                break;
            default:
                break;
            }
        }
    }

    // -------------------------------------------------------------------------
    // Filter Menu
    // -------------------------------------------------------------------------

    void filterMenu() const {
        bool filtering = true;

        while (filtering) {
            cout << "\nFilter Clients\n";
            cout << "--------------\n";
            cout << "1. Brokerage clients\n";
            cout << "2. Retirement clients\n";
            cout << "3. Conservative risk clients\n";
            cout << "4. Moderate risk clients\n";
            cout << "5. Aggressive risk clients\n";
            cout << "6. Balance above threshold\n";
            cout << "7. Balance below threshold\n";
            cout << "8. Active clients\n";
            cout << "9. Compound filter (service + minimum balance)\n";
            cout << "10. Return to main menu\n";

            const int choice = InputValidator::readIntInRange(
                "Selection: ", 1, 10);

            switch (choice) {
            case 1:
                displayClientCollection(
                    "Brokerage Clients",
                    filterClients([](const Client& client) {
                        return client.service == ServiceType::Brokerage;
                        }));
                break;

            case 2:
                displayClientCollection(
                    "Retirement Clients",
                    filterClients([](const Client& client) {
                        return client.service == ServiceType::Retirement;
                        }));
                break;

            case 3:
                displayClientCollection(
                    "Conservative Risk Clients",
                    filterClients([](const Client& client) {
                        return client.riskLevel == RiskLevel::Conservative;
                        }));
                break;

            case 4:
                displayClientCollection(
                    "Moderate Risk Clients",
                    filterClients([](const Client& client) {
                        return client.riskLevel == RiskLevel::Moderate;
                        }));
                break;

            case 5:
                displayClientCollection(
                    "Aggressive Risk Clients",
                    filterClients([](const Client& client) {
                        return client.riskLevel == RiskLevel::Aggressive;
                        }));
                break;

            case 6: {
                const double threshold = InputValidator::readDoubleInRange(
                    "Minimum balance threshold: $",
                    Constants::kMinBalance,
                    Constants::kMaxBalance);

                displayClientCollection(
                    "Clients Above Balance Threshold",
                    filterClients([threshold](const Client& client) {
                        return client.accountBalance > threshold;
                        }));
                break;
            }

            case 7: {
                const double threshold = InputValidator::readDoubleInRange(
                    "Maximum balance threshold: $",
                    Constants::kMinBalance,
                    Constants::kMaxBalance);

                displayClientCollection(
                    "Clients Below Balance Threshold",
                    filterClients([threshold](const Client& client) {
                        return client.accountBalance < threshold;
                        }));
                break;
            }

            case 8:
                displayClientCollection(
                    "Active Clients",
                    filterClients([](const Client& client) {
                        return client.active;
                        }));
                break;

            case 9: {
                const ServiceType service = readServiceType();
                const double minimumBalance = InputValidator::readDoubleInRange(
                    "Minimum balance: $",
                    Constants::kMinBalance,
                    Constants::kMaxBalance);

                displayClientCollection(
                    "Compound Filter Results",
                    filterClients(
                        [service, minimumBalance](const Client& client) {
                            return client.service == service &&
                                client.accountBalance >= minimumBalance;
                        }));
                break;
            }

            case 10:
                filtering = false;
                break;

            default:
                break;
            }
        }
    }

    // -------------------------------------------------------------------------
    // Sort Menu
    // -------------------------------------------------------------------------

    void sortMenu() const {
        cout << "\nSort Clients\n";
        cout << "------------\n";
        cout << "1. Client ID ascending\n";
        cout << "2. Name A-Z\n";
        cout << "3. Name Z-A\n";
        cout << "4. Balance low-high\n";
        cout << "5. Balance high-low\n";
        cout << "6. Service type\n";
        cout << "7. Risk level\n";

        const int choice = InputValidator::readIntInRange(
            "Selection: ", 1, 7);

        vector<Client> clients = getAllClientsVector();

        switch (choice) {
        case 1:
            std::stable_sort(
                clients.begin(), clients.end(),
                [](const Client& left, const Client& right) {
                    return left.id < right.id;
                });
            break;

        case 2:
            std::stable_sort(
                clients.begin(), clients.end(),
                [](const Client& left, const Client& right) {
                    return normalizeName(left.name) < normalizeName(right.name);
                });
            break;

        case 3:
            std::stable_sort(
                clients.begin(), clients.end(),
                [](const Client& left, const Client& right) {
                    return normalizeName(left.name) > normalizeName(right.name);
                });
            break;

        case 4:
            std::stable_sort(
                clients.begin(), clients.end(),
                [](const Client& left, const Client& right) {
                    return left.accountBalance < right.accountBalance;
                });
            break;

        case 5:
            std::stable_sort(
                clients.begin(), clients.end(),
                [](const Client& left, const Client& right) {
                    return left.accountBalance > right.accountBalance;
                });
            break;

        case 6:
            std::stable_sort(
                clients.begin(), clients.end(),
                [](const Client& left, const Client& right) {
                    if (left.service != right.service) {
                        return static_cast<int>(left.service) <
                            static_cast<int>(right.service);
                    }
                    return normalizeName(left.name) < normalizeName(right.name);
                });
            break;

        case 7:
            std::stable_sort(
                clients.begin(), clients.end(),
                [](const Client& left, const Client& right) {
                    if (left.riskLevel != right.riskLevel) {
                        return static_cast<int>(left.riskLevel) <
                            static_cast<int>(right.riskLevel);
                    }
                    return left.accountBalance > right.accountBalance;
                });
            break;

        default:
            break;
        }

        displayClientCollection("Sorted Client View", clients);
    }

    // -------------------------------------------------------------------------
    // Rankings / Top-K
    // -------------------------------------------------------------------------

    void rankingsMenu() const {
        cout << "\nClient Rankings\n";
        cout << "---------------\n";
        cout << "1. Top clients by account balance\n";
        cout << "2. Bottom clients by account balance\n";
        cout << "3. Top Brokerage clients\n";
        cout << "4. Top Retirement clients\n";

        const int choice = InputValidator::readIntInRange(
            "Selection: ", 1, 4);

        const int requestedCount = InputValidator::readIntInRange(
            "How many results? ",
            1,
            static_cast<int>(std::max<std::size_t>(1, clients_.size())));

        vector<Client> eligible;
        eligible.reserve(clients_.size());

        for (const auto& entry : clients_) {
            const Client& client = entry.second;

            if (choice == 3 && client.service != ServiceType::Brokerage) {
                continue;
            }

            if (choice == 4 && client.service != ServiceType::Retirement) {
                continue;
            }

            eligible.push_back(client);
        }

        if (eligible.empty()) {
            cout << "No clients matched the ranking criteria.\n\n";
            return;
        }

        const int actualCount = std::min(
            requestedCount,
            static_cast<int>(eligible.size()));

        vector<Client> ranked;

        if (choice == 2) {
            ranked = getBottomKByBalance(eligible, actualCount);
        }
        else {
            ranked = getTopKByBalance(eligible, actualCount);
        }

        displayClientCollection("Client Rankings", ranked);
    }

    // -------------------------------------------------------------------------
    // Analytics
    // -------------------------------------------------------------------------

    void displayAnalytics() const {
        cout << "\nPortfolio Analytics\n";
        cout << "-------------------\n";

        if (clients_.empty()) {
            cout << "No client records are available.\n\n";
            return;
        }

        std::size_t brokerageCount = 0;
        std::size_t retirementCount = 0;
        std::size_t conservativeCount = 0;
        std::size_t moderateCount = 0;
        std::size_t aggressiveCount = 0;
        std::size_t activeCount = 0;

        double totalAssets = 0.0;
        double brokerageAssets = 0.0;
        double retirementAssets = 0.0;
        vector<double> balances;
        balances.reserve(clients_.size());

        const Client* highestClient = nullptr;
        const Client* lowestClient = nullptr;

        for (const auto& entry : clients_) {
            const Client& client = entry.second;

            totalAssets += client.accountBalance;
            balances.push_back(client.accountBalance);

            if (client.service == ServiceType::Brokerage) {
                ++brokerageCount;
                brokerageAssets += client.accountBalance;
            }
            else {
                ++retirementCount;
                retirementAssets += client.accountBalance;
            }

            switch (client.riskLevel) {
            case RiskLevel::Conservative:
                ++conservativeCount;
                break;
            case RiskLevel::Moderate:
                ++moderateCount;
                break;
            case RiskLevel::Aggressive:
                ++aggressiveCount;
                break;
            }

            if (client.active) {
                ++activeCount;
            }

            if (highestClient == nullptr ||
                client.accountBalance > highestClient->accountBalance) {
                highestClient = &client;
            }

            if (lowestClient == nullptr ||
                client.accountBalance < lowestClient->accountBalance) {
                lowestClient = &client;
            }
        }

        std::sort(balances.begin(), balances.end());

        double median = 0.0;
        const std::size_t count = balances.size();

        if (count % 2 == 1) {
            median = balances[count / 2];
        }
        else {
            median = (balances[count / 2 - 1] + balances[count / 2]) / 2.0;
        }

        const double average = totalAssets / static_cast<double>(count);

        cout << std::fixed << std::setprecision(2);
        cout << "Total clients: " << count << '\n';
        cout << "Active clients: " << activeCount << '\n';
        cout << "Inactive clients: " << (count - activeCount) << "\n\n";

        cout << "Service Distribution\n";
        cout << "  Brokerage: " << brokerageCount
            << " (" << percentage(brokerageCount, count) << "%)\n";
        cout << "  Retirement: " << retirementCount
            << " (" << percentage(retirementCount, count) << "%)\n\n";

        cout << "Asset Summary\n";
        cout << "  Total assets: $" << totalAssets << '\n';
        cout << "  Average balance: $" << average << '\n';
        cout << "  Median balance: $" << median << '\n';
        cout << "  Brokerage assets: $" << brokerageAssets << '\n';
        cout << "  Retirement assets: $" << retirementAssets << '\n';

        if (brokerageCount > 0) {
            cout << "  Avg. Brokerage balance: $"
                << (brokerageAssets / static_cast<double>(brokerageCount))
                << '\n';
        }

        if (retirementCount > 0) {
            cout << "  Avg. Retirement balance: $"
                << (retirementAssets / static_cast<double>(retirementCount))
                << '\n';
        }

        if (highestClient != nullptr) {
            cout << "  Highest balance: " << highestClient->name
                << " ($" << highestClient->accountBalance << ")\n";
        }

        if (lowestClient != nullptr) {
            cout << "  Lowest balance: " << lowestClient->name
                << " ($" << lowestClient->accountBalance << ")\n";
        }

        cout << "\nRisk Distribution\n";
        cout << "  Conservative: " << conservativeCount << '\n';
        cout << "  Moderate: " << moderateCount << '\n';
        cout << "  Aggressive: " << aggressiveCount << "\n\n";
    }

    // -------------------------------------------------------------------------
    // Undo Stack
    // -------------------------------------------------------------------------

    void undoLastChange() {
        if (undoHistory_.empty()) {
            cout << "There is no change available to undo.\n\n";
            return;
        }

        const Action action = undoHistory_.top();
        undoHistory_.pop();

        switch (action.type) {
        case ActionType::Add:
            if (action.after.has_value()) {
                eraseClient(action.after->id);
            }
            break;

        case ActionType::Remove:
            if (action.before.has_value()) {
                insertClient(*action.before);
            }
            break;

        case ActionType::Update:
            if (action.before.has_value() && action.after.has_value()) {
                eraseClient(action.after->id);
                insertClient(*action.before);
            }
            break;
        }

        logger_.write("UNDO action=" + action.description);
        cout << "Undo completed: " << action.description << ".\n\n";
    }

    // -------------------------------------------------------------------------
    // Index / Data Structure Tools
    // -------------------------------------------------------------------------

    void systemToolsMenu() {
        bool running = true;

        while (running) {
            cout << "\nSystem / Index Tools\n";
            cout << "--------------------\n";
            cout << "1. Validate secondary name index\n";
            cout << "2. Rebuild secondary name index\n";
            cout << "3. Display data structure statistics\n";
            cout << "4. Return to main menu\n";

            const int choice = InputValidator::readIntInRange(
                "Selection: ", 1, 4);

            switch (choice) {
            case 1:
                validateNameIndex();
                break;
            case 2:
                rebuildNameIndex();
                cout << "Secondary name index rebuilt successfully.\n\n";
                logger_.write("NAME_INDEX_REBUILT");
                break;
            case 3:
                displayStructureStatistics();
                break;
            case 4:
                running = false;
                break;
            default:
                break;
            }
        }
    }

private:
    // Primary structure: average O(1) exact lookup by numeric ID.
    unordered_map<int, Client> clients_;

    // Secondary ordered index: O(log n) key navigation plus returned matches.
    // The set allows duplicate normalized names while preserving unique IDs.
    map<string, set<int>> nameIndex_;

    // LIFO history is a natural fit for undo operations.
    stack<Action> undoHistory_;

    int nextClientId_ = 1;
    const Logger& logger_;

    // -------------------------------------------------------------------------
    // Core Storage Helpers
    // -------------------------------------------------------------------------

    void seedClient(const Client& client) {
        insertClient(client);
    }

    void insertClient(const Client& client) {
        clients_[client.id] = client;
        addToNameIndex(client.name, client.id);

        if (client.id >= nextClientId_) {
            nextClientId_ = client.id + 1;
        }
    }

    bool eraseClient(int clientId) {
        const auto match = clients_.find(clientId);
        if (match == clients_.end()) {
            return false;
        }

        removeFromNameIndex(match->second.name, clientId);
        clients_.erase(match);
        return true;
    }

    Client* findClientById(int clientId) {
        const auto match = clients_.find(clientId);
        if (match == clients_.end()) {
            return nullptr;
        }
        return &(match->second);
    }

    const Client* findClientById(int clientId) const {
        const auto match = clients_.find(clientId);
        if (match == clients_.end()) {
            return nullptr;
        }
        return &(match->second);
    }

    bool clientExists(int clientId) const {
        return clients_.find(clientId) != clients_.end();
    }

    int readExistingClientId(const string& prompt) const {
        while (true) {
            const int clientId = InputValidator::readIntInRange(
                prompt,
                1,
                std::numeric_limits<int>::max());

            if (clientExists(clientId)) {
                return clientId;
            }

            cout << "No client exists with ID " << clientId
                << ". Please try again.\n";
        }
    }

    // -------------------------------------------------------------------------
    // Name Index Helpers
    // -------------------------------------------------------------------------

    static string normalizeName(string value) {
        value = InputValidator::toLower(value);

        const std::size_t first = value.find_first_not_of(" \t\r\n");
        if (first == string::npos) {
            return "";
        }

        const std::size_t last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    }

    void addToNameIndex(const string& name, int clientId) {
        nameIndex_[normalizeName(name)].insert(clientId);
    }

    void removeFromNameIndex(const string& name, int clientId) {
        const string normalized = normalizeName(name);
        const auto indexMatch = nameIndex_.find(normalized);

        if (indexMatch == nameIndex_.end()) {
            return;
        }

        indexMatch->second.erase(clientId);

        if (indexMatch->second.empty()) {
            nameIndex_.erase(indexMatch);
        }
    }

    void rebuildNameIndex() {
        nameIndex_.clear();

        for (const auto& entry : clients_) {
            const Client& client = entry.second;
            addToNameIndex(client.name, client.id);
        }
    }

    void validateNameIndex() const {
        bool valid = true;
        std::size_t indexedIds = 0;

        for (const auto& entry : nameIndex_) {
            const string& normalizedName = entry.first;
            const set<int>& ids = entry.second;

            for (int clientId : ids) {
                ++indexedIds;

                const Client* client = findClientById(clientId);
                if (client == nullptr ||
                    normalizeName(client->name) != normalizedName) {
                    valid = false;
                }
            }
        }

        for (const auto& entry : clients_) {
            const Client& client = entry.second;
            const string normalized = normalizeName(client.name);
            const auto nameMatch = nameIndex_.find(normalized);

            if (nameMatch == nameIndex_.end() ||
                nameMatch->second.find(client.id) == nameMatch->second.end()) {
                valid = false;
            }
        }

        cout << "\nName Index Validation\n";
        cout << "---------------------\n";
        cout << "Primary client records: " << clients_.size() << '\n';
        cout << "Indexed client references: " << indexedIds << '\n';
        cout << "Index status: " << (valid ? "VALID" : "INVALID") << "\n\n";
    }

    // -------------------------------------------------------------------------
    // Search Implementations
    // -------------------------------------------------------------------------

    void searchById() const {
        const int id = InputValidator::readIntInRange(
            "Client ID: ", 1, std::numeric_limits<int>::max());

        const Client* client = findClientById(id);
        if (client == nullptr) {
            cout << "No client found with ID " << id << ".\n\n";
            return;
        }

        displayClientCollection("Search Result", { *client });
    }

    void searchByExactName() const {
        const string name = InputValidator::readNonEmptyLine(
            "Exact client name: ");

        const string normalized = normalizeName(name);
        const auto match = nameIndex_.find(normalized);

        if (match == nameIndex_.end()) {
            cout << "No exact name matches were found.\n\n";
            return;
        }

        vector<Client> results;
        for (int clientId : match->second) {
            const Client* client = findClientById(clientId);
            if (client != nullptr) {
                results.push_back(*client);
            }
        }

        displayClientCollection("Exact Name Matches", results);
    }

    void searchByNamePrefix() const {
        const string prefix = normalizeName(
            InputValidator::readNonEmptyLine("Name prefix: "));

        vector<Client> results;

        // lower_bound jumps directly to the first ordered key that could match.
        // This avoids scanning every name key from the beginning of the map.
        auto iterator = nameIndex_.lower_bound(prefix);

        while (iterator != nameIndex_.end() &&
            startsWith(iterator->first, prefix)) {

            for (int clientId : iterator->second) {
                const Client* client = findClientById(clientId);
                if (client != nullptr) {
                    results.push_back(*client);
                }
            }

            ++iterator;
        }

        std::stable_sort(
            results.begin(), results.end(),
            [](const Client& left, const Client& right) {
                return normalizeName(left.name) < normalizeName(right.name);
            });

        displayClientCollection("Prefix Search Results", results);
    }

    void searchByService() const {
        const ServiceType service = readServiceType();

        displayClientCollection(
            "Service Search Results",
            filterClients([service](const Client& client) {
                return client.service == service;
                }));
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
            "Balance Range Results",
            filterClients([minimum, maximum](const Client& client) {
                return client.accountBalance >= minimum &&
                    client.accountBalance <= maximum;
                }));
    }

    void searchByRiskLevel() const {
        const RiskLevel risk = readRiskLevel();

        displayClientCollection(
            "Risk Search Results",
            filterClients([risk](const Client& client) {
                return client.riskLevel == risk;
                }));
    }

    void searchByActiveStatus() const {
        const bool active = InputValidator::readYesNo(
            "Search for active clients? (y/n): ");

        displayClientCollection(
            "Status Search Results",
            filterClients([active](const Client& client) {
                return client.active == active;
                }));
    }

    static bool startsWith(const string& value, const string& prefix) {
        return value.size() >= prefix.size() &&
            value.compare(0, prefix.size(), prefix) == 0;
    }

    // -------------------------------------------------------------------------
    // Generic Filtering
    // -------------------------------------------------------------------------

    template <typename Predicate>
    vector<Client> filterClients(Predicate predicate) const {
        vector<Client> results;

        for (const auto& entry : clients_) {
            const Client& client = entry.second;
            if (predicate(client)) {
                results.push_back(client);
            }
        }

        std::stable_sort(
            results.begin(), results.end(),
            [](const Client& left, const Client& right) {
                return left.id < right.id;
            });

        return results;
    }

    // -------------------------------------------------------------------------
    // Top-K Ranking Algorithms
    // -------------------------------------------------------------------------

    struct MinBalanceComparator {
        bool operator()(const Client& left, const Client& right) const {
            if (left.accountBalance != right.accountBalance) {
                return left.accountBalance > right.accountBalance;
            }
            return left.id > right.id;
        }
    };

    struct MaxBalanceComparator {
        bool operator()(const Client& left, const Client& right) const {
            if (left.accountBalance != right.accountBalance) {
                return left.accountBalance < right.accountBalance;
            }
            return left.id > right.id;
        }
    };

    static vector<Client> getTopKByBalance(
        const vector<Client>& source,
        int k) {

        priority_queue<
            Client,
            vector<Client>,
            MinBalanceComparator> heap;

        for (const Client& client : source) {
            heap.push(client);

            if (static_cast<int>(heap.size()) > k) {
                heap.pop();
            }
        }

        vector<Client> results;
        while (!heap.empty()) {
            results.push_back(heap.top());
            heap.pop();
        }

        std::sort(
            results.begin(), results.end(),
            [](const Client& left, const Client& right) {
                return left.accountBalance > right.accountBalance;
            });

        return results;
    }

    static vector<Client> getBottomKByBalance(
        const vector<Client>& source,
        int k) {

        priority_queue<
            Client,
            vector<Client>,
            MaxBalanceComparator> heap;

        for (const Client& client : source) {
            heap.push(client);

            if (static_cast<int>(heap.size()) > k) {
                heap.pop();
            }
        }

        vector<Client> results;
        while (!heap.empty()) {
            results.push_back(heap.top());
            heap.pop();
        }

        std::sort(
            results.begin(), results.end(),
            [](const Client& left, const Client& right) {
                return left.accountBalance < right.accountBalance;
            });

        return results;
    }

    // -------------------------------------------------------------------------
    // Display Helpers
    // -------------------------------------------------------------------------

    vector<Client> getAllClientsVector() const {
        vector<Client> clients;
        clients.reserve(clients_.size());

        for (const auto& entry : clients_) {
            clients.push_back(entry.second);
        }

        return clients;
    }

    vector<Client> getClientsSortedById() const {
        vector<Client> clients = getAllClientsVector();

        std::stable_sort(
            clients.begin(), clients.end(),
            [](const Client& left, const Client& right) {
                return left.id < right.id;
            });

        return clients;
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

    // -------------------------------------------------------------------------
    // Structure Statistics / Complexity Notes
    // -------------------------------------------------------------------------

    void displayStructureStatistics() const {
        std::size_t indexReferences = 0;
        for (const auto& entry : nameIndex_) {
            indexReferences += entry.second.size();
        }

        cout << "\nData Structure Statistics\n";
        cout << "-------------------------\n";
        cout << "Primary unordered_map records: " << clients_.size() << '\n';
        cout << "Primary bucket count: " << clients_.bucket_count() << '\n';
        cout << "Primary load factor: " << clients_.load_factor() << '\n';
        cout << "Ordered name-index keys: " << nameIndex_.size() << '\n';
        cout << "Name-index client references: " << indexReferences << '\n';
        cout << "Undo stack depth: " << undoHistory_.size() << "\n\n";

        cout << "Expected complexity characteristics:\n";
        cout << "  ID lookup: average O(1) using unordered_map\n";
        cout << "  Exact indexed name lookup: O(log n + k)\n";
        cout << "  Prefix index navigation: O(log n + k)\n";
        cout << "  Sorted views: O(n log n)\n";
        cout << "  Full predicate filtering: O(n)\n";
        cout << "  Analytics aggregation: O(n), plus O(n log n) for median\n";
        cout << "  Top-K ranking: O(n log k) using a bounded heap\n";
        cout << "  Undo push/pop: O(1), excluding index maintenance\n\n";
    }

    // -------------------------------------------------------------------------
    // Shared Input Helpers
    // -------------------------------------------------------------------------

    static ServiceType readServiceType() {
        const int serviceChoice = InputValidator::readIntInRange(
            "Select service (1 = Brokerage, 2 = Retirement): ",
            Constants::kMinServiceChoice,
            Constants::kMaxServiceChoice);

        return static_cast<ServiceType>(serviceChoice);
    }

    static RiskLevel readRiskLevel() {
        const int riskChoice = InputValidator::readIntInRange(
            "Select risk level (1 = Conservative, 2 = Moderate, 3 = Aggressive): ",
            Constants::kMinRiskChoice,
            Constants::kMaxRiskChoice);

        return static_cast<RiskLevel>(riskChoice);
    }

    static double percentage(std::size_t part, std::size_t whole) {
        if (whole == 0) {
            return 0.0;
        }

        return (static_cast<double>(part) /
            static_cast<double>(whole)) * 100.0;
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
        clientManager_(logger_) {
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
                clientManager_.systemToolsMenu();
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

        return 0;
    }

private:
    Logger logger_;
    Authenticator authenticator_;
    ClientManager clientManager_;

    static void displayHeader() {
        cout << "Created by Brianna Reed\n";
        cout << "Investment Company Client Management System\n";
        cout << "Algorithms & Data Structures Enhancement\n";
        cout << "====================================================\n\n";
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
        cout << "10. Undo last change\n";
        cout << "11. System / index tools\n";
        cout << "12. Exit\n";
    }
};

// -----------------------------------------------------------------------------
// Program Entry Point
// -----------------------------------------------------------------------------

int main() {
    InvestmentCompanyApplication application;
    return application.run();
}
