
/*
    AURELIA WEALTH STUDIO
    CS 499 Computer Science Capstone
    Milestone Four: Enhancement Three - Databases
    Student: Brianna Reed

    Fresh install SQLite relational schema, version 2

    InvestmentCore.h also performs additive runtime migrations
    so existing cs499_clients.db records do not need to be deleted

    All investor and security data in this capstone is fictional
    This file creates structures, not demonstration investors
    Never use this file to delete or reset a working database
*/

PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;
PRAGMA synchronous = NORMAL;


/* =========================================================
   SCHEMA METADATA
   ========================================================= */

CREATE TABLE IF NOT EXISTS schema_metadata (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL
);


/* =========================================================
   REFERENCE TABLES
   ========================================================= */

CREATE TABLE IF NOT EXISTS service_types (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    CHECK (id IN (1, 2))
);


CREATE TABLE IF NOT EXISTS risk_levels (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    CHECK (id IN (1, 2, 3))
);


/* =========================================================
   CLIENT RELATIONSHIPS
   ========================================================= */

CREATE TABLE IF NOT EXISTS clients (
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    name TEXT NOT NULL
        CHECK (length(trim(name)) > 0),

    normalized_name TEXT NOT NULL,

    service_id INTEGER NOT NULL,

    age INTEGER NOT NULL
        CHECK (age BETWEEN 18 AND 120),

    account_balance REAL NOT NULL
        CHECK (
            account_balance >= 0
            AND account_balance <= 1000000000
        ),

    risk_level_id INTEGER NOT NULL,

    email TEXT NOT NULL COLLATE NOCASE
        CHECK (length(trim(email)) > 0),

    phone TEXT NOT NULL
        CHECK (length(trim(phone)) > 0),

    active INTEGER NOT NULL DEFAULT 1
        CHECK (active IN (0, 1)),

    household_name TEXT NOT NULL DEFAULT '',

    advisor_name TEXT NOT NULL DEFAULT '',

    investment_objective TEXT NOT NULL DEFAULT '',

    annual_income REAL NOT NULL DEFAULT 0
        CHECK (annual_income >= 0),

    net_worth REAL NOT NULL DEFAULT 0
        CHECK (net_worth >= 0),

    liquidity_need TEXT NOT NULL DEFAULT '',

    time_horizon_years INTEGER NOT NULL DEFAULT 10
        CHECK (
            time_horizon_years >= 0
            AND time_horizon_years <= 80
        ),

    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,

    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (service_id)
        REFERENCES service_types(id),

    FOREIGN KEY (risk_level_id)
        REFERENCES risk_levels(id),

    UNIQUE (email)
);


/* =========================================================
   FINANCIAL ACCOUNTS
   ========================================================= */

CREATE TABLE IF NOT EXISTS accounts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    client_id INTEGER NOT NULL,

    account_name TEXT NOT NULL,

    account_type TEXT NOT NULL,

    custodian TEXT NOT NULL DEFAULT 'Aurelia Demo Custody',

    tax_treatment TEXT NOT NULL DEFAULT 'Taxable',

    active INTEGER NOT NULL DEFAULT 1
        CHECK (active IN (0, 1)),

    opened_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (client_id)
        REFERENCES clients(id)
        ON DELETE CASCADE
);


/* =========================================================
   SECURITIES
   ========================================================= */

CREATE TABLE IF NOT EXISTS securities (
    symbol TEXT PRIMARY KEY,

    security_name TEXT NOT NULL,

    asset_class TEXT NOT NULL,

    sector TEXT NOT NULL DEFAULT '',

    mock_price REAL NOT NULL
        CHECK (mock_price > 0),

    expense_ratio REAL NOT NULL DEFAULT 0
        CHECK (expense_ratio >= 0)
);


/* =========================================================
   PORTFOLIO HOLDINGS
   ========================================================= */

CREATE TABLE IF NOT EXISTS holdings (
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    account_id INTEGER NOT NULL,

    symbol TEXT NOT NULL,

    quantity REAL NOT NULL
        CHECK (quantity >= 0),

    cost_basis_per_share REAL NOT NULL
        CHECK (cost_basis_per_share >= 0),

    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (account_id)
        REFERENCES accounts(id)
        ON DELETE CASCADE,

    FOREIGN KEY (symbol)
        REFERENCES securities(symbol),

    UNIQUE (account_id, symbol)
);


/* =========================================================
   ACCOUNT TRANSACTIONS
   ========================================================= */

CREATE TABLE IF NOT EXISTS transactions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    account_id INTEGER NOT NULL,

    transaction_date TEXT NOT NULL,

    transaction_type TEXT NOT NULL,

    symbol TEXT,

    quantity REAL NOT NULL DEFAULT 0,

    amount REAL NOT NULL,

    description TEXT NOT NULL DEFAULT '',

    FOREIGN KEY (account_id)
        REFERENCES accounts(id)
        ON DELETE CASCADE,

    FOREIGN KEY (symbol)
        REFERENCES securities(symbol)
);


/* =========================================================
   PORTFOLIO HISTORY
   ========================================================= */

CREATE TABLE IF NOT EXISTS portfolio_snapshots (
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    client_id INTEGER NOT NULL,

    snapshot_date TEXT NOT NULL,

    portfolio_value REAL NOT NULL
        CHECK (portfolio_value >= 0),

    contributions_to_date REAL NOT NULL DEFAULT 0,

    withdrawals_to_date REAL NOT NULL DEFAULT 0,

    FOREIGN KEY (client_id)
        REFERENCES clients(id)
        ON DELETE CASCADE,

    UNIQUE (client_id, snapshot_date)
);


/* =========================================================
   FINANCIAL GOALS
   ========================================================= */

CREATE TABLE IF NOT EXISTS financial_goals (
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    client_id INTEGER NOT NULL,

    goal_name TEXT NOT NULL,

    goal_type TEXT NOT NULL,

    target_amount REAL NOT NULL
        CHECK (target_amount > 0),

    current_amount REAL NOT NULL DEFAULT 0
        CHECK (current_amount >= 0),

    target_date TEXT NOT NULL,

    priority INTEGER NOT NULL DEFAULT 2
        CHECK (priority BETWEEN 1 AND 3),

    status TEXT NOT NULL DEFAULT 'Active',

    FOREIGN KEY (client_id)
        REFERENCES clients(id)
        ON DELETE CASCADE
);


/* =========================================================
   ADVISOR NOTES
   ========================================================= */

CREATE TABLE IF NOT EXISTS advisor_notes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    client_id INTEGER NOT NULL,

    note_text TEXT NOT NULL,

    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (client_id)
        REFERENCES clients(id)
        ON DELETE CASCADE
);


/* =========================================================
   RELATIONSHIP TASKS
   ========================================================= */

CREATE TABLE IF NOT EXISTS advisor_tasks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    client_id INTEGER,

    title TEXT NOT NULL,

    category TEXT NOT NULL DEFAULT 'Review',

    due_date TEXT NOT NULL,

    priority TEXT NOT NULL DEFAULT 'Medium',

    completed INTEGER NOT NULL DEFAULT 0
        CHECK (completed IN (0, 1)),

    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (client_id)
        REFERENCES clients(id)
        ON DELETE CASCADE
);


/* =========================================================
   DATABASE AUDIT TRAIL
   ========================================================= */

CREATE TABLE IF NOT EXISTS audit_events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    event_type TEXT NOT NULL,

    client_id INTEGER,

    details TEXT NOT NULL DEFAULT '',

    occurred_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);


/* =========================================================
   PERSISTENT CLIENT HISTORY
   ========================================================= */

CREATE TABLE IF NOT EXISTS client_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    action_type TEXT NOT NULL
        CHECK (
            action_type IN (
                'ADD',
                'UPDATE',
                'REMOVE'
            )
        ),

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

    undone INTEGER NOT NULL DEFAULT 0
        CHECK (undone IN (0, 1)),

    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);


/* =========================================================
   INDEXES
   ========================================================= */

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

CREATE INDEX IF NOT EXISTS idx_accounts_client
    ON accounts(client_id);

CREATE INDEX IF NOT EXISTS idx_holdings_account
    ON holdings(account_id);

CREATE INDEX IF NOT EXISTS idx_holdings_symbol
    ON holdings(symbol);

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

CREATE INDEX IF NOT EXISTS idx_notes_client
    ON advisor_notes(
        client_id,
        created_at DESC
    );

CREATE INDEX IF NOT EXISTS idx_history_undo
    ON client_history(
        undone,
        id DESC
    );

CREATE INDEX IF NOT EXISTS idx_audit_occurred_at
    ON audit_events(
        occurred_at DESC,
        id DESC
    );

CREATE INDEX IF NOT EXISTS idx_client_household
    ON clients(household_name);

CREATE INDEX IF NOT EXISTS idx_client_advisor
    ON clients(advisor_name);

CREATE INDEX IF NOT EXISTS idx_goals_target_date
    ON financial_goals(target_date);

CREATE INDEX IF NOT EXISTS idx_tasks_client_status
    ON advisor_tasks(client_id, completed, due_date);


/* =========================================================
   REFERENCE DATA

   Repeatable seed of stable lookup values
   Does not add clients
   ========================================================= */

INSERT OR IGNORE INTO service_types(id, name)
VALUES
    (1, 'Brokerage'),
    (2, 'Retirement');

INSERT OR IGNORE INTO risk_levels(id, name)
VALUES
    (1, 'Conservative'),
    (2, 'Moderate'),
    (3, 'Aggressive');


/* =========================================================
   READ ONLY REPORTING VIEWS

   Live modeled values calculated from stored holdings
   and security prices

   These are modeled financial snapshots, not market returns
   ========================================================= */


/* ---------------------------------------------------------
   CLIENT PORTFOLIO SUMMARY

   Aggregates at the client level

   Financial balances are calculated from holdings rather
   than duplicated across account joins
   --------------------------------------------------------- */

CREATE VIEW IF NOT EXISTS v_client_portfolio_summary AS

SELECT
    c.id AS client_id,
    c.name AS client_name,
    c.household_name,
    c.advisor_name,
    c.active,

    COALESCE(a.account_count, 0)
        AS account_count,

    COALESCE(p.holding_count, 0)
        AS holding_count,

    COALESCE(p.modeled_market_value, 0)
        AS modeled_market_value,

    COALESCE(p.modeled_cost_basis, 0)
        AS modeled_cost_basis,

    COALESCE(p.unrealized_change, 0)
        AS unrealized_change

FROM clients AS c

LEFT JOIN (
    SELECT
        client_id,
        COUNT(*) AS account_count

    FROM accounts

    GROUP BY client_id

) AS a
    ON a.client_id = c.id

LEFT JOIN (
    SELECT
        ac.client_id,

        COUNT(*) AS holding_count,

        SUM(h.quantity * s.mock_price)
            AS modeled_market_value,

        SUM(h.quantity * h.cost_basis_per_share)
            AS modeled_cost_basis,

        SUM(
            h.quantity *
            (s.mock_price - h.cost_basis_per_share)
        ) AS unrealized_change

    FROM accounts AS ac

    JOIN holdings AS h
        ON h.account_id = ac.id

    JOIN securities AS s
        ON s.symbol = h.symbol

    GROUP BY ac.client_id

) AS p
    ON p.client_id = c.id;


/* ---------------------------------------------------------
   CLIENT ASSET ALLOCATION

   Identifies current modeled exposure by asset class
   A client may have multiple accounts
   --------------------------------------------------------- */

CREATE VIEW IF NOT EXISTS v_asset_allocation AS

SELECT
    ac.client_id,
    s.asset_class,

    SUM(h.quantity * s.mock_price)
        AS modeled_value

FROM accounts AS ac

JOIN holdings AS h
    ON h.account_id = ac.id

JOIN securities AS s
    ON s.symbol = h.symbol

GROUP BY
    ac.client_id,
    s.asset_class;


/* ---------------------------------------------------------
   OPEN ADVISOR TASKS

   Returns unfinished tasks, their associated client,
   priority, due date, and calculated overdue flag
   --------------------------------------------------------- */

CREATE VIEW IF NOT EXISTS v_open_advisor_work_queue AS

SELECT
    t.id AS task_id,
    t.client_id,

    COALESCE(c.name, 'Enterprise')
        AS client_name,

    t.title,
    t.category,
    t.due_date,
    t.priority,

    CASE
        WHEN date(t.due_date) < date('now')
        THEN 1
        ELSE 0
    END AS overdue

FROM advisor_tasks AS t

LEFT JOIN clients AS c
    ON c.id = t.client_id

WHERE t.completed = 0;


/* =========================================================
   SCHEMA VERSION

   Keep version 2 until InvestmentCore.h is deliberately
   updated with a coordinated schema migration
   ========================================================= */

INSERT INTO schema_metadata (
    key,
    value
)
VALUES (
    'schema_version',
    '2'
)
ON CONFLICT(key)
DO UPDATE SET
    value = excluded.value;

-- Additive 2026 planning extension. Legacy task columns are migrated by
-- ClientRepository::initializeSchema using guarded ensureColumn calls
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
