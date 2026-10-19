/*
 * CS 499 Computer Science Capstone
 * Milestone Two: Enhancement One - Software Design and Engineering
 * Student: Brianna Reed
 * Date: September 20, 2026
 *
 * Artifact origin: CS 410 Software Reverse Engineering Project One
 *
 * Enhancement purpose:
 * Refactor the recovered legacy-style client management program into a more
 * modular, object-oriented, maintainable, and defensively coded C++ program.
 *
 * This version intentionally keeps client data in memory so later capstone
 * enhancements can focus separately on algorithms/data structures and databases.
 */

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using std::cin;
using std::cout;
using std::endl;
using std::getline;
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
}


// -----------------------------------------------------------------------------
// Service Type
// -----------------------------------------------------------------------------

enum class ServiceType {
    Brokerage = 1,
    Retirement = 2
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


// -----------------------------------------------------------------------------
// Client Model
// -----------------------------------------------------------------------------

struct Client {
    int id;
    string name;
    ServiceType service;
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
                << minimum
                << " through "
                << maximum
                << ".\n";
        }
    }


    static string readNonEmptyLine(const string& prompt) {

        while (true) {

            cout << prompt;

            string value;
            getline(cin, value);

            const bool containsVisibleCharacter =
                value.find_first_not_of(" \t\r\n") != string::npos;

            if (containsVisibleCharacter) {
                return value;
            }

            cout << "Input cannot be blank. Please try again.\n";
        }
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

        /*
         * Credentials are intentionally obtained from environment variables
         * instead of being hard coded directly into the source code.
         *
         * Visual Studio / Microsoft C++ uses _dupenv_s because it is the
         * Microsoft-recommended secure alternative to getenv().
         */

        const string configuredUsername =
            readEnvironmentVariable("CS499_USERNAME");

        const string configuredPassword =
            readEnvironmentVariable("CS499_PASSWORD");


        if (configuredUsername.empty() ||
            configuredPassword.empty()) {

            cout
                << "Authentication is not configured.\n"
                << "Set the CS499_USERNAME and CS499_PASSWORD environment "
                << "variables before running the program.\n";

            logger_.write("AUTH_CONFIGURATION_ERROR");

            return false;
        }


        for (
            int attempt = 1;
            attempt <= Constants::kMaxAuthenticationAttempts;
            ++attempt) {

            const string enteredUsername =
                InputValidator::readNonEmptyLine(
                    "Enter your username: ");

            const string enteredPassword =
                InputValidator::readNonEmptyLine(
                    "Enter your password: ");


            if (enteredUsername == configuredUsername &&
                enteredPassword == configuredPassword) {

                cout << "Authentication successful.\n\n";

                /*
                 * Passwords are intentionally NOT written to the audit log.
                 */

                logger_.write(
                    "AUTH_SUCCESS username=" + enteredUsername);

                return true;
            }


            const int remainingAttempts =
                Constants::kMaxAuthenticationAttempts - attempt;

            cout << "Invalid credentials.";

            if (remainingAttempts > 0) {

                cout
                    << " Attempts remaining: "
                    << remainingAttempts;
            }

            cout << "\n";

            logger_.write(
                "AUTH_FAILURE attempt=" +
                std::to_string(attempt));
        }


        cout
            << "Maximum authentication attempts reached. "
            << "Access denied.\n";

        logger_.write("AUTH_LOCKOUT");

        return false;
    }


private:

    /*
     * Reads an environment variable safely.
     *
     * Microsoft Visual C++ uses _dupenv_s().
     * Other compilers can use getenv().
     */

    static string readEnvironmentVariable(const char* name) {

#ifdef _MSC_VER

        char* value = nullptr;
        size_t length = 0;

        const errno_t result =
            _dupenv_s(&value, &length, name);

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
// Client Management
// -----------------------------------------------------------------------------

class ClientManager {

public:

    explicit ClientManager(const Logger& logger)
        : logger_(logger) {

        /*
         * Preserve the five client records recovered from the
         * original CS 410 application.
         */

        clients_.push_back(
            { 1, "Bob Jones", ServiceType::Brokerage });

        clients_.push_back(
            { 2, "Sarah Davis", ServiceType::Retirement });

        clients_.push_back(
            { 3, "Amy Friendly", ServiceType::Brokerage });

        clients_.push_back(
            { 4, "Johnny Smith", ServiceType::Brokerage });

        clients_.push_back(
            { 5, "Carol Spears", ServiceType::Retirement });
    }


    // -------------------------------------------------------------------------
    // Display Clients
    // -------------------------------------------------------------------------

    void displayClients() const {

        cout << "\nClient List\n";
        cout << "-----------\n";


        if (clients_.empty()) {

            cout
                << "No client records are currently available.\n\n";

            return;
        }


        for (const Client& client : clients_) {

            cout
                << "ID: "
                << client.id
                << " | Name: "
                << client.name
                << " | Service: "
                << serviceTypeToString(client.service)
                << '\n';
        }

        cout << '\n';
    }


    // -------------------------------------------------------------------------
    // Add Client
    // -------------------------------------------------------------------------

    void addClient() {

        cout << "\nAdd Client\n";
        cout << "----------\n";


        const int nextId =
            getNextClientId();


        const string name =
            InputValidator::readNonEmptyLine(
                "Client name: ");


        const ServiceType service =
            readServiceType();


        clients_.push_back(
            { nextId, name, service });


        logger_.write(
            "CLIENT_ADD id=" +
            std::to_string(nextId) +
            " name=" +
            name);


        cout
            << "Client added successfully with ID "
            << nextId
            << ".\n\n";
    }


    // -------------------------------------------------------------------------
    // Update Client Service
    // -------------------------------------------------------------------------

    void updateClientService() {

        if (clients_.empty()) {

            cout
                << "There are no clients to update.\n\n";

            return;
        }


        displayClients();


        const int clientId =
            readExistingClientId(
                "Enter the client ID to update: ");


        Client* client =
            findClientById(clientId);


        /*
         * This check provides an additional defensive layer even though
         * readExistingClientId() already verifies the ID.
         */

        if (client == nullptr) {

            cout
                << "Client could not be found.\n\n";

            return;
        }


        const ServiceType oldService =
            client->service;


        cout
            << "Current service: "
            << serviceTypeToString(oldService)
            << '\n';


        const ServiceType newService =
            readServiceType();


        client->service =
            newService;


        logger_.write(
            "CLIENT_UPDATE id=" +
            std::to_string(clientId) +
            " oldService=" +
            serviceTypeToString(oldService) +
            " newService=" +
            serviceTypeToString(newService));


        cout
            << "Client service updated successfully.\n\n";
    }


    // -------------------------------------------------------------------------
    // Remove Client
    // -------------------------------------------------------------------------

    void removeClient() {

        if (clients_.empty()) {

            cout
                << "There are no clients to remove.\n\n";

            return;
        }


        displayClients();


        const int clientId =
            readExistingClientId(
                "Enter the client ID to remove: ");


        const auto match =
            std::find_if(

                clients_.begin(),
                clients_.end(),

                [clientId](const Client& client) {

                    return client.id == clientId;
                });


        if (match == clients_.end()) {

            cout
                << "Client could not be found.\n\n";

            return;
        }


        const string removedName =
            match->name;


        clients_.erase(match);


        logger_.write(
            "CLIENT_REMOVE id=" +
            std::to_string(clientId) +
            " name=" +
            removedName);


        cout
            << "Client removed successfully.\n\n";
    }


private:

    vector<Client> clients_;

    const Logger& logger_;


    // -------------------------------------------------------------------------
    // Find Client
    // -------------------------------------------------------------------------

    Client* findClientById(int clientId) {

        const auto match =
            std::find_if(

                clients_.begin(),
                clients_.end(),

                [clientId](const Client& client) {

                    return client.id == clientId;
                });


        if (match == clients_.end()) {
            return nullptr;
        }

        return &(*match);
    }


    // -------------------------------------------------------------------------
    // Check Whether Client Exists
    // -------------------------------------------------------------------------

    bool clientExists(int clientId) const {

        return std::any_of(

            clients_.begin(),
            clients_.end(),

            [clientId](const Client& client) {

                return client.id == clientId;
            });
    }


    // -------------------------------------------------------------------------
    // Read Valid Client ID
    // -------------------------------------------------------------------------

    int readExistingClientId(
        const string& prompt) const {

        while (true) {

            const int clientId =
                InputValidator::readIntInRange(

                    prompt,

                    1,

                    std::numeric_limits<int>::max());


            if (clientExists(clientId)) {

                return clientId;
            }


            cout
                << "No client exists with ID "
                << clientId
                << ". Please try again.\n";
        }
    }


    // -------------------------------------------------------------------------
    // Read Service Type
    // -------------------------------------------------------------------------

    static ServiceType readServiceType() {

        const int serviceChoice =
            InputValidator::readIntInRange(

                "Select service "
                "(1 = Brokerage, 2 = Retirement): ",

                Constants::kMinServiceChoice,

                Constants::kMaxServiceChoice);


        return static_cast<ServiceType>(
            serviceChoice);
    }


    // -------------------------------------------------------------------------
    // Generate Next Client ID
    // -------------------------------------------------------------------------

    int getNextClientId() const {

        int highestId = 0;


        for (const Client& client : clients_) {

            if (client.id > highestId) {

                highestId =
                    client.id;
            }
        }


        return highestId + 1;
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

        cout
            << "Created by Brianna Reed\n";

        cout
            << "Investment Company Client Management System\n";

        cout
            << "===========================================\n\n";


        /*
         * Authentication must succeed before the client management
         * functionality becomes available.
         */

        if (!authenticator_.authenticate()) {

            return 1;
        }


        bool running = true;


        while (running) {

            displayMenu();


            const int choice =
                InputValidator::readIntInRange(
                    "Selection: ",
                    1,
                    5);


            switch (choice) {

            case 1:

                clientManager_.displayClients();

                break;


            case 2:

                clientManager_.addClient();

                break;


            case 3:

                clientManager_.updateClientService();

                break;


            case 4:

                clientManager_.removeClient();

                break;


            case 5:

                running = false;

                logger_.write(
                    "APPLICATION_EXIT");

                cout
                    << "Program closed safely.\n";

                break;


            default:

                /*
                 * InputValidator guarantees that only choices
                 * 1 through 5 reach this switch statement.
                 */

                break;
            }
        }


        return 0;
    }


private:

    Logger logger_;

    Authenticator authenticator_;

    ClientManager clientManager_;


    static void displayMenu() {

        cout
            << "What would you like to do?\n"

            << "1. Display client list\n"

            << "2. Add a client\n"

            << "3. Change a client's service\n"

            << "4. Remove a client\n"

            << "5. Exit\n";
    }
};


// -----------------------------------------------------------------------------
// Program Entry Point
// -----------------------------------------------------------------------------

int main() {

    InvestmentCompanyApplication application;

    return application.run();
}