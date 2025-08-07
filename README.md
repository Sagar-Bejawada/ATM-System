# Console-Based ATM Management System in C

## Features

- Register account with PIN (secure input with '*')
- Login with account number and PIN (masked)
- View balance, deposit, withdraw, change PIN, view mini statement
- All data and transactions securely stored in MySQL

## Setup

1. **Install MySQL and dev headers:**  
   Ubuntu:
   ```
   sudo apt-get install libmysqlclient-dev mysql-server
   ```

2. **Update the password in `database_setup.c` and `atm_app.c`**  
   Replace `your_mysql_password` with your MySQL root password.

3. **Compile:**
   ```
   make
   ```

4. **Setup database:**
   ```
   ./database_setup
   ```

5. **Run application:**
   ```
   ./atm_app
   ```

## Notes

- The database is named `atm_system`.
- PINs are stored as plain text for demo purposes. For production, use hashed PINs and secure practices.