#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include <termios.h>
#include <unistd.h>

#define HOST "localhost"
#define USER "root"
#define PASS "your_mysql_password"
#define DB "atm_system"

void clear_input() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

// Get PIN input with '*' masking
void get_pin(char *pin, int maxlen) {
    struct termios oldt, newt;
    int i = 0;
    char ch;
    printf("Enter PIN: ");
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    while (i < maxlen - 1) {
        ch = getchar();
        if (ch == '\n') break;
        if (ch == 127 || ch == '\b') { // backspace
            if (i > 0) {
                i--;
                printf("\b \b");
            }
        } else if (ch >= '0' && ch <= '9') {
            pin[i++] = ch;
            printf("*");
        }
    }
    pin[i] = '\0';
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    printf("\n");
}

void register_account(MYSQL *conn) {
    char name[100], pin[10], pin2[10];
    printf("Enter your name: ");
    clear_input();
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = 0;

    while (1) {
        get_pin(pin, 10);
        printf("Re-enter PIN: ");
        get_pin(pin2, 10);
        if (strcmp(pin, pin2) == 0) break;
        printf("PINs do not match. Try again.\n");
    }

    char query[512];
    snprintf(query, sizeof(query),
        "INSERT INTO users (name, pin, balance) VALUES ('%s','%s', 0.0)", name, pin);

    if (mysql_query(conn, query)) {
        printf("Registration failed: %s\n", mysql_error(conn));
        return;
    }
    int account_no = (int)mysql_insert_id(conn);
    printf("Registration successful! Your account number is: %d\n", account_no);
}

int login(MYSQL *conn, int *account_no) {
    int acc;
    char pin[10];
    printf("Enter Account Number: ");
    scanf("%d", &acc);
    get_pin(pin, 10);

    char query[256];
    snprintf(query, sizeof(query),
        "SELECT account_no, name FROM users WHERE account_no=%d AND pin='%s'", acc, pin);

    if (mysql_query(conn, query)) {
        printf("Login error: %s\n", mysql_error(conn));
        return 0;
    }
    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row) {
        *account_no = atoi(row[0]);
        printf("Welcome, %s! (Account No: %s)\n", row[1], row[0]);
        mysql_free_result(res);
        return 1;
    } else {
        printf("Invalid credentials.\n");
        mysql_free_result(res);
        return 0;
    }
}

void view_balance(MYSQL *conn, int account_no) {
    char query[128];
    snprintf(query, sizeof(query), "SELECT balance FROM users WHERE account_no=%d", account_no);
    if (mysql_query(conn, query)) {
        printf("Balance inquiry failed: %s\n", mysql_error(conn));
        return;
    }
    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row) printf("Current Balance: %.2f\n", atof(row[0]));
    else printf("Account not found.\n");
    mysql_free_result(res);

    snprintf(query, sizeof(query),
        "INSERT INTO atm_transactions (account_no, amount, type) VALUES (%d, 0.0, 'balance_inquiry')", account_no);
    mysql_query(conn, query);
}

void deposit(MYSQL *conn, int account_no) {
    double amount;
    printf("Enter deposit amount: ");
    scanf("%lf", &amount);
    if (amount <= 0) {
        printf("Invalid amount.\n");
        return;
    }
    char query[256];
    snprintf(query, sizeof(query),
        "UPDATE users SET balance = balance + %.2f WHERE account_no=%d", amount, account_no);
    if (mysql_query(conn, query)) {
        printf("Deposit failed: %s\n", mysql_error(conn));
        return;
    }
    snprintf(query, sizeof(query),
        "INSERT INTO atm_transactions (account_no, amount, type) VALUES (%d, %.2f, 'deposit')", account_no, amount);
    mysql_query(conn, query);
    printf("Deposit successful.\n");
}

void withdraw(MYSQL *conn, int account_no) {
    double amount, balance = 0;
    printf("Enter withdrawal amount: ");
    scanf("%lf", &amount);
    if (amount <= 0) {
        printf("Invalid amount.\n");
        return;
    }
    char query[256];
    snprintf(query, sizeof(query),
        "SELECT balance FROM users WHERE account_no=%d", account_no);
    if (mysql_query(conn, query)) {
        printf("Balance check failed: %s\n", mysql_error(conn));
        return;
    }
    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row) balance = atof(row[0]);
    mysql_free_result(res);

    if (balance < amount) {
        printf("Insufficient funds.\n");
        return;
    }
    snprintf(query, sizeof(query),
        "UPDATE users SET balance = balance - %.2f WHERE account_no=%d", amount, account_no);
    if (mysql_query(conn, query)) {
        printf("Withdrawal failed: %s\n", mysql_error(conn));
        return;
    }
    snprintf(query, sizeof(query),
        "INSERT INTO atm_transactions (account_no, amount, type) VALUES (%d, %.2f, 'withdrawal')", account_no, amount);
    mysql_query(conn, query);
    printf("Withdrawal successful.\n");
}

void change_pin(MYSQL *conn, int account_no) {
    char old_pin[10], new_pin[10], new_pin2[10];
    printf("Enter current PIN: ");
    get_pin(old_pin, 10);

    char query[256];
    snprintf(query, sizeof(query),
        "SELECT pin FROM users WHERE account_no=%d", account_no);
    if (mysql_query(conn, query)) {
        printf("PIN check failed: %s\n", mysql_error(conn));
        return;
    }
    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row || strcmp(row[0], old_pin) != 0) {
        printf("Incorrect current PIN.\n");
        mysql_free_result(res);
        return;
    }
    mysql_free_result(res);

    while (1) {
        printf("Enter new PIN: ");
        get_pin(new_pin, 10);
        printf("Re-enter new PIN: ");
        get_pin(new_pin2, 10);
        if (strcmp(new_pin, new_pin2) == 0) break;
        printf("PINs do not match. Try again.\n");
    }
    snprintf(query, sizeof(query),
        "UPDATE users SET pin='%s' WHERE account_no=%d", new_pin, account_no);
    if (mysql_query(conn, query)) {
        printf("PIN update failed: %s\n", mysql_error(conn));
        return;
    }
    snprintf(query, sizeof(query),
        "INSERT INTO atm_transactions (account_no, amount, type) VALUES (%d, 0.0, 'pin_change')", account_no);
    mysql_query(conn, query);
    printf("PIN changed successfully.\n");
}

void mini_statement(MYSQL *conn, int account_no) {
    int n = 5;
    printf("Last %d transactions:\n", n);
    char query[256];
    snprintf(query, sizeof(query),
        "SELECT type, amount, trans_time FROM atm_transactions WHERE account_no=%d ORDER BY trans_id DESC LIMIT %d", account_no, n);
    if (mysql_query(conn, query)) {
        printf("Mini statement failed: %s\n", mysql_error(conn));
        return;
    }
    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row;
    printf("%-15s %-10s %-20s\n", "Type", "Amount", "Time");
    while ((row = mysql_fetch_row(res))) {
        printf("%-15s %-10s %-20s\n", row[0], row[1], row[2]);
    }
    mysql_free_result(res);
}

int main() {
    MYSQL *conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, HOST, USER, PASS, DB, 0, NULL, 0)) {
        printf("Connection error: %s\n", mysql_error(conn));
        return 1;
    }

    int choice, logged_in = 0, account_no = -1;
    while (1) {
        printf("\n--- ATM System ---\n");
        printf("1. Register\n2. Login\n3. Exit\n");
        printf("Choose an option: ");
        scanf("%d", &choice);

        if (choice == 1) {
            register_account(conn);
        } else if (choice == 2) {
            if (login(conn, &account_no)) {
                logged_in = 1;
                break;
            }
        } else if (choice == 3) {
            mysql_close(conn);
            exit(0);
        } else {
            printf("Invalid option.\n");
        }
    }

    while (logged_in) {
        printf("\n--- ATM Menu ---\n");
        printf("1. View Balance\n2. Deposit\n3. Withdraw\n4. Change PIN\n5. Mini Statement\n6. Logout\n");
        printf("Choose an option: ");
        scanf("%d", &choice);

        if (choice == 1) view_balance(conn, account_no);
        else if (choice == 2) deposit(conn, account_no);
        else if (choice == 3) withdraw(conn, account_no);
        else if (choice == 4) change_pin(conn, account_no);
        else if (choice == 5) mini_statement(conn, account_no);
        else if (choice == 6) {
            printf("Logged out.\n");
            logged_in = 0;
        } else {
            printf("Invalid option.\n");
        }
    }

    mysql_close(conn);
    return 0;
}