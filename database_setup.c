#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>

#define HOST "localhost"
#define USER "root"
#define PASS "your_mysql_password"

int main() {
    MYSQL *conn = mysql_init(NULL);

    if (!mysql_real_connect(conn, HOST, USER, PASS, NULL, 0, NULL, 0)) {
        fprintf(stderr, "Connection error: %s\n", mysql_error(conn));
        return 1;
    }

    if (mysql_query(conn, "CREATE DATABASE IF NOT EXISTS atm_system")) {
        fprintf(stderr, "DB creation failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }

    if (mysql_select_db(conn, "atm_system")) {
        fprintf(stderr, "USE DB failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }

    if (mysql_query(conn,
        "CREATE TABLE IF NOT EXISTS users ("
        "account_no INT PRIMARY KEY AUTO_INCREMENT,"
        "name VARCHAR(100) NOT NULL,"
        "pin VARCHAR(10) NOT NULL,"
        "balance DECIMAL(12,2) NOT NULL DEFAULT 0,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)"
    )) {
        fprintf(stderr, "Create users failed: %s\n", mysql_error(conn));
    }

    if (mysql_query(conn,
        "CREATE TABLE IF NOT EXISTS atm_transactions ("
        "trans_id INT PRIMARY KEY AUTO_INCREMENT,"
        "account_no INT NOT NULL,"
        "amount DECIMAL(12,2) NOT NULL,"
        "type ENUM('deposit', 'withdrawal', 'balance_inquiry', 'pin_change') NOT NULL,"
        "trans_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (account_no) REFERENCES users(account_no))"
    )) {
        fprintf(stderr, "Create atm_transactions failed: %s\n", mysql_error(conn));
    }

    printf("Database and tables set up successfully.\n");
    mysql_close(conn);
    return 0;
}