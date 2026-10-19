#include <iostream>
#include <string>

using namespace std;

// Original global data recovered from the binary.
char username[20];

char name1[] = "Bob Jones";
char name2[] = "Sarah Davis";
char name3[] = "Amy Friendly";
char name4[] = "Johnny Smith";
char name5[] = "Carol Spears";

int num1 = 1;
int num2 = 2;
int num3 = 1;
int num4 = 1;
int num5 = 2;

int choice;
int changechoice;
int newservice;

char password[20] = "123";
int answer;

// Function declarations.
int CheckUserPermissionAccess();
void DisplayInfo();
void ChangeCustomerChoice();

int main()
{
    // Required added output statement for Project One.
    cout << "Created by Brianna Reed" << endl;

    cout << "Hello! Welcome to our Investment Company\n";

    // Continue asking for credentials until the correct password is entered.
    do
    {
        answer = CheckUserPermissionAccess();

        if (answer != 1)
        {
            cout << "Invalid Password. Please try again\n";
        }

    } while (answer != 1);

    // Main application menu.
    do
    {
        cout << "What would you like to do?\n";
        cout << "DISPLAY the client list (enter 1)\n";
        cout << "CHANGE a client's choice (enter 2)\n";
        cout << "Exit the program.. (enter 3)\n";

        cin >> choice;

        cout << "You chose " << choice << endl;

        if (choice == 1)
        {
            DisplayInfo();
        }
        else if (choice == 2)
        {
            ChangeCustomerChoice();
        }

    } while (choice != 3);

    return 0;
}

int CheckUserPermissionAccess()
{
    string enteredPassword = "";
    int passwordMatch = 0;

    cout << "Enter your username: \n";
    cin >> username;

    cout << "Enter your password: \n";
    cin >> enteredPassword;

    // The original binary compares the entered password with "123".
    passwordMatch = enteredPassword.compare("123");

    if (passwordMatch == 0)
    {
        return 1;
    }
    else
    {
        return 2;
    }
}

void DisplayInfo()
{
    cout << "  Client's Name    Service Selected (1 = Brokerage, 2 = Retirement)" << endl;

    cout << "1. " << name1 << " selected option " << num1 << endl;
    cout << "2. " << name2 << " selected option " << num2 << endl;
    cout << "3. " << name3 << " selected option " << num3 << endl;
    cout << "4. " << name4 << " selected option " << num4 << endl;
    cout << "5. " << name5 << " selected option " << num5 << endl;
}

void ChangeCustomerChoice()
{
    cout << "Enter the number of the client that you wish to change\n";
    cin >> changechoice;

    cout << "Please enter the client's new service choice (1 = Brokerage, 2 = Retirement)\n";
    cin >> newservice;

    if (changechoice == 1)
    {
        num1 = newservice;
    }
    else if (changechoice == 2)
    {
        num2 = newservice;
    }
    else if (changechoice == 3)
    {
        num3 = newservice;
    }
    else if (changechoice == 4)
    {
        num4 = newservice;
    }
    else if (changechoice == 5)
    {
        num5 = newservice;
    }
}