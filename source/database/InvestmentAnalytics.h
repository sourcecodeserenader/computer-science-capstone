#pragma once

/*
 * AURELIA WEALTH STUDIO | InvestmentAnalytics.h
 * CS 499 Computer Science Capstone | Brianna Reed
 *
 * Database driven portfolio analytics and rule based review signals
 * All records are fictional. Quotes are stored mock prices, not live feeds
 * Snapshot change is a change in modeled account value, NOT investment
 * performance or a time weighted return. Contributions and withdrawals
 * are not removed from that measure
 *
 * Existing dashboard, desktop, PDF and Excel method names are preserved
 */

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "InvestmentCore.h"


struct AllocationSlice {

    string label;

    double value =
        0.0;

    double percent =
        0.0;
};


struct GoalStatusSummary {

    int total =
        0;

    int onTrack =
        0;

    int watch =
        0;

    int completed =
        0;
};


struct EnterpriseSummary {

    int clientCount =
        0;

    int activeClients =
        0;

    int openTasks =
        0;

    int overdueTasks =
        0;

    double assetsAdministered =
        0.0;

    double unrealizedGain =
        0.0;

    double twelveMonthChangePercent =
        0.0;

    GoalStatusSummary goals;

    // Read only diagnostic. Never triggers deletion automatically
    int legacyDemoDuplicateCandidates = 0;

    // Number of distinct months included in the snapshot comparison
    int historyPoints = 0;
};


struct ClientInsight {

    string severity;

    string title;

    string detail;
};


struct ClientDashboard {

    Client client;

    double currentValue =
        0.0;

    double costBasis =
        0.0;

    double unrealizedGain =
        0.0;

    double twelveMonthChangePercent =
        0.0;

    vector<AllocationSlice>
        actualAllocation;

    vector<AllocationSlice>
        targetAllocation;

    vector<Holding>
        topHoldings;

    vector<PortfolioSnapshot>
        history;

    vector<FinancialGoal>
        goals;

    vector<AdvisorTask>
        tasks;

    vector<ClientInsight>
        insights;

    // Additional source records for a detailed client workspace
    vector<Account> accounts;
    vector<TransactionRecord> transactions;
    vector<AdvisorNote> notes;
};


class AnalyticsService {
private:

    ClientRepository& repository_;

    Database& database_;

    // Calendar comparisons use the workstation's local business date. ISO dates compare
    // lexicographically; a valid stored date is therefore required
    string databaseToday() const {
        Statement query(database_.handle(), "SELECT date('now','-7 hours');");
        const int rc = query.step();
        if (rc != SQLITE_ROW) {
            throw DatabaseException("Unable to determine SQLite date.");
        }
        return query.columnText(0);
    }

    static double snapshotChange(const vector<PortfolioSnapshot>& history) {
        if (history.size() < 2 || history.front().value <= 0.0) {
            return 0.0;
        }
        return (history.back().value - history.front().value) /
            history.front().value * 100.0;
    }

    static double safePercent(
        double value,
        double total) {

        if (total <=
            0.0 || !std::isfinite(total) || !std::isfinite(value)) {

            return 0.0;
        }

        return value /
            total *
            100.0;
    }


    static vector<AllocationSlice>
        targetForRisk(
            RiskLevel risk,
            double total) {

        if (
            risk ==
            RiskLevel::Conservative) {

            return {

                {
                    "US Equity",
                    total * 0.25,
                    25.0
                },

                {
                    "International Equity",
                    total * 0.10,
                    10.0
                },

                {
                    "Fixed Income",
                    total * 0.50,
                    50.0
                },

                {
                    "Cash",
                    total * 0.10,
                    10.0
                },

                {
                    "Real Assets",
                    total * 0.05,
                    5.0
                }
            };
        }


        if (
            risk ==
            RiskLevel::Moderate) {

            return {

                {
                    "US Equity",
                    total * 0.45,
                    45.0
                },

                {
                    "International Equity",
                    total * 0.15,
                    15.0
                },

                {
                    "Fixed Income",
                    total * 0.27,
                    27.0
                },

                {
                    "Cash",
                    total * 0.08,
                    8.0
                },

                {
                    "Real Assets",
                    total * 0.05,
                    5.0
                }
            };
        }


        return {

            {
                "US Equity",
                total * 0.62,
                62.0
            },

            {
                "International Equity",
                total * 0.16,
                16.0
            },

            {
                "Fixed Income",
                total * 0.10,
                10.0
            },

            {
                "Cash",
                total * 0.05,
                5.0
            },

            {
                "Real Assets",
                total * 0.07,
                7.0
            }
        };
    }


    static double percentFor(
        const vector<AllocationSlice>& slices,
        const string& label) {

        for (
            const AllocationSlice& slice :
            slices) {

            if (
                slice.label ==
                label) {

                return slice.percent;
            }
        }

        return 0.0;
    }


public:

    explicit AnalyticsService(
        ClientRepository& repository)
        :
        repository_(
            repository),
        database_(
            repository.database()) {
    }


    vector<AllocationSlice>
        enterpriseAllocation() {

        Statement statement(
            database_.handle(),
            "SELECT "
            "s.asset_class,"
            "SUM("
            "h.quantity*"
            "s.mock_price"
            ") "
            "FROM holdings h "
            "JOIN securities s "
            "ON s.symbol=h.symbol "
            "GROUP BY s.asset_class "
            "ORDER BY "
            "SUM("
            "h.quantity*"
            "s.mock_price"
            ") DESC;");


        vector<AllocationSlice>
            slices;


        double total =
            0.0;


        while (
            statement.step() ==
            SQLITE_ROW) {

            AllocationSlice slice;

            slice.label =
                statement.columnText(
                    0);

            slice.value =
                statement.columnDouble(
                    1);

            total +=
                slice.value;

            slices.push_back(
                slice);
        }


        for (
            AllocationSlice& slice :
            slices) {

            slice.percent =
                safePercent(
                    slice.value,
                    total);
        }


        return slices;
    }


    vector<PortfolioSnapshot>
        enterpriseHistory() {

        Statement statement(
            database_.handle(),
            "SELECT "
            "snapshot_date,"
            "SUM(portfolio_value),"
            "SUM(contributions_to_date),"
            "SUM(withdrawals_to_date) "
            "FROM portfolio_snapshots "
            "GROUP BY snapshot_date "
            "ORDER BY snapshot_date;");


        vector<PortfolioSnapshot>
            history;


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

            history.push_back(
                row);
        }


        return history;
    }


    GoalStatusSummary goalSummary() {

        Statement statement(
            database_.handle(),
            "SELECT "
            "target_amount,"
            "current_amount,"
            "status "
            "FROM financial_goals;");


        GoalStatusSummary summary;


        while (
            statement.step() ==
            SQLITE_ROW) {

            summary.total++;


            const double target =
                statement.columnDouble(
                    0);


            const double current =
                statement.columnDouble(
                    1);


            const string status =
                statement.columnText(
                    2);


            if (
                status ==
                "Completed" ||
                current >=
                target) {

                summary.completed++;
            }

            else {

                const double ratio =
                    target >
                    0.0
                    ? current /
                    target
                    : 0.0;


                if (
                    ratio >=
                    0.55) {

                    summary.onTrack++;
                }

                else {

                    summary.watch++;
                }
            }
        }


        return summary;
    }


    EnterpriseSummary
        enterpriseSummary() {

        EnterpriseSummary summary;


        Statement clients(
            database_.handle(),
            "SELECT "
            "COUNT(*),"
            "SUM("
            "CASE "
            "WHEN active=1 "
            "THEN 1 "
            "ELSE 0 "
            "END"
            ") "
            "FROM clients;");


        if (
            clients.step() ==
            SQLITE_ROW) {

            summary.clientCount =
                clients.columnInt(
                    0);

            summary.activeClients =
                clients.columnInt(
                    1);
        }


        Statement holdings(
            database_.handle(),
            "SELECT "
            "COALESCE("
            "SUM("
            "h.quantity*"
            "s.mock_price"
            "),0"
            "),"
            "COALESCE("
            "SUM("
            "h.quantity*"
            "("
            "s.mock_price-"
            "h.cost_basis_per_share"
            ")"
            "),0"
            ") "
            "FROM holdings h "
            "JOIN securities s "
            "ON s.symbol=h.symbol;");


        if (
            holdings.step() ==
            SQLITE_ROW) {

            summary.assetsAdministered =
                holdings.columnDouble(
                    0);

            summary.unrealizedGain =
                holdings.columnDouble(
                    1);
        }


        Statement tasks(
            database_.handle(),
            "SELECT "
            "COUNT(*),"
            "COALESCE(SUM("
            "CASE "
            "WHEN date(due_date) < date('now','-7 hours') "
            "THEN 1 "
            "ELSE 0 "
            "END"
            "),0) "
            "FROM advisor_tasks "
            "WHERE completed=0;");


        if (
            tasks.step() ==
            SQLITE_ROW) {

            summary.openTasks =
                tasks.columnInt(
                    0);

            summary.overdueTasks =
                tasks.columnInt(
                    1);
        }


        summary.goals =
            goalSummary();


        const vector<PortfolioSnapshot>
            history =
            enterpriseHistory();

        summary.historyPoints = static_cast<int>(history.size());
        summary.legacyDemoDuplicateCandidates =
            repository_.countLegacyDemoDuplicates();

        if (
            history.size() >=
            2) {

            const double first =
                history.front().value;

            const double last =
                history.back().value;


            if (
                first >
                0.0) {

                summary
                    .twelveMonthChangePercent =
                    (
                        last -
                        first
                        ) /
                    first *
                    100.0;
            }
        }


        return summary;
    }


    ClientDashboard clientDashboard(
        int clientId) {

        const optional<Client>
            client =
            repository_
            .findClientById(
                clientId);


        if (!client) {

            throw DatabaseException(
                "Client not found.");
        }


        ClientDashboard dashboard;

        dashboard.client =
            *client;


        dashboard.topHoldings =
            repository_
            .getHoldingsForClient(
                clientId);


        dashboard.history =
            repository_
            .getPortfolioHistory(
                clientId);


        dashboard.goals =
            repository_
            .getGoalsForClient(
                clientId);


        dashboard.tasks =
            repository_
            .getOpenTasks(
                clientId);

        dashboard.accounts =
            repository_.getAccountsForClient(clientId);

        dashboard.transactions =
            repository_.getTransactionsForClient(clientId, 100);

        dashboard.notes =
            repository_.getNotesForClient(clientId);


        std::map<
            string,
            double>
            allocation;


        for (
            const Holding& holding :
            dashboard.topHoldings) {

            dashboard.currentValue +=
                holding.marketValue;


            dashboard.costBasis +=
                holding.quantity *
                holding.costBasisPerShare;


            dashboard.unrealizedGain +=
                holding.unrealizedGain;


            allocation[
                holding.assetClass] +=
                holding.marketValue;
        }


        for (
            const auto& pair :
            allocation) {

            AllocationSlice slice;

            slice.label =
                pair.first;

            slice.value =
                pair.second;

            slice.percent =
                safePercent(
                    pair.second,
                    dashboard.currentValue);

            dashboard
                .actualAllocation
                .push_back(
                    slice);
        }


        std::sort(
            dashboard
            .actualAllocation
            .begin(),
            dashboard
            .actualAllocation
            .end(),
            [](
                const AllocationSlice& left,
                const AllocationSlice& right) {

                    return left.value >
                        right.value;
            });


        dashboard.targetAllocation =
            targetForRisk(
                client->riskLevel,
                dashboard.currentValue);


        if (
            dashboard.history.size() >=
            2) {

            const double first =
                dashboard
                .history
                .front()
                .value;

            const double last =
                dashboard
                .history
                .back()
                .value;


            if (
                first >
                0.0) {

                dashboard
                    .twelveMonthChangePercent =
                    (
                        last -
                        first
                        ) /
                    first *
                    100.0;
            }
        }


        /*
         * These are monitoring and workflow
         * signals, not personalized
         * investment recommendations
         */


        if (dashboard.topHoldings.empty()) {
            dashboard.insights.push_back({
                "High",
                "Missing holdings",
                "No security positions exist for this client. Verify "
                "account setup and data completeness before using reports."
                });
        }

        if (dashboard.history.size() < 2) {
            dashboard.insights.push_back({
                "Watch",
                "Insufficient snapshot history",
                "At least two stored portfolio snapshots are required "
                "to show a period-over-period modeled value change."
                });
        }

        if (dashboard.goals.empty()) {
            dashboard.insights.push_back({
                "Info",
                "No financial goals recorded",
                "Document fictional client objectives before interpreting "
                "goal progress indicators."
                });
        }

        if (dashboard.currentValue > 0.0) for (
            const AllocationSlice& target :
            dashboard.targetAllocation) {

            const double actual =
                percentFor(
                    dashboard.actualAllocation,
                    target.label);


            const double drift =
                actual -
                target.percent;


            if (
                std::fabs(
                    drift) >=
                8.0) {

                ClientInsight insight;

                insight.severity =
                    std::fabs(
                        drift) >=
                    12.0
                    ? "High"
                    : "Watch";


                insight.title =
                    "Allocation drift";


                std::ostringstream detail;

                detail
                    << target.label
                    << " is "
                    << std::fixed
                    << std::setprecision(
                        1)
                    << std::fabs(
                        drift)
                    << " percentage points "
                    << (
                        drift >
                        0.0
                        ? "above"
                        : "below"
                        )
                    << " the illustrative risk-profile mix; "
                    << "this is not a documented investment policy.";


                insight.detail =
                    detail.str();


                dashboard.insights.push_back(
                    insight);
            }
        }


        const double cashPercent =
            percentFor(
                dashboard.actualAllocation,
                "Cash");


        if (
            cashPercent >
            15.0) {

            ClientInsight insight;

            insight.severity =
                "Watch";

            insight.title =
                "Cash position review";


            std::ostringstream detail;

            detail
                << std::fixed
                << std::setprecision(
                    1)
                << cashPercent
                << "% of portfolio value "
                << "is classified as cash.";


            insight.detail =
                detail.str();


            dashboard.insights.push_back(
                insight);
        }


        if (
            !dashboard.topHoldings.empty() &&
            dashboard.currentValue >
            0.0) {

            const Holding& largest =
                dashboard
                .topHoldings
                .front();


            const double weight =
                largest.marketValue /
                dashboard.currentValue *
                100.0;


            if (
                weight >
                20.0) {

                ClientInsight insight;

                insight.severity =
                    weight >
                    25.0
                    ? "High"
                    : "Watch";


                insight.title =
                    "Concentration review";


                std::ostringstream detail;

                detail
                    << largest.symbol
                    << " represents "
                    << std::fixed
                    << std::setprecision(
                        1)
                    << weight
                    << "% of modeled "
                    << "portfolio value.";


                insight.detail =
                    detail.str();


                dashboard.insights.push_back(
                    insight);
            }
        }


        const string today = databaseToday();

        for (
            const FinancialGoal& goal :
            dashboard.goals) {

            const double funded =
                goal.targetAmount >
                0.0
                ? goal.currentAmount /
                goal.targetAmount *
                100.0
                : 0.0;


            if (goal.status == "Active" && !goal.targetDate.empty() &&
                goal.targetDate < today && funded < 100.0) {
                dashboard.insights.push_back({
                    "High",
                    "Goal target date passed",
                    goal.name + " has a stored target date of " +
                    goal.targetDate + " and a modeled funding ratio below 100%. "
                    "Confirm that the date and goal remain current."
                    });
            }

            if (
                goal.status == "Active" && funded <
                50.0) {

                ClientInsight insight;

                insight.severity =
                    "Watch";

                insight.title =
                    "Goal progress";


                std::ostringstream detail;

                detail
                    << goal.name
                    << " is "
                    << std::fixed
                    << std::setprecision(
                        1)
                    << funded
                    << "% funded in the "
                    << "fictional model.";


                insight.detail =
                    detail.str();


                dashboard.insights.push_back(
                    insight);
            }
        }


        for (
            const AdvisorTask& task :
            dashboard.tasks) {

            if (
                !task.dueDate.empty() && task.dueDate < today) {

                ClientInsight insight;

                insight.severity =
                    "High";

                insight.title =
                    "Relationship task overdue";

                insight.detail =
                    task.title +
                    " was due " +
                    task.dueDate +
                    ".";

                dashboard.insights.push_back(
                    insight);
            }
        }


        if (
            dashboard.insights.empty()) {

            dashboard.insights.push_back(
                {
                    "Info",

                    "No immediate review flags",

                    "The current fictional data "
                    "does not trigger Aurelia's "
                    "configured monitoring thresholds."
                });
        }


        return dashboard;
    }
};