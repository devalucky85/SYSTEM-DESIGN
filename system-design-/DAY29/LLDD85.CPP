#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <iomanip>

using namespace std;

enum class SplitType { EQUAL, EXACT, PERCENTAGE };

class Split {
public:
    string userId;
    double amount;
    Split(const string& userId, double amount) : userId(userId), amount(amount) {}
};

class User {
public:
    static int nextUserId;
    string userId, name;
    map<string, double> balances; // userId -> amount (positive: they owe you, negative: you owe them)

    User(const string& name) : name(name), userId("user" + to_string(++nextUserId)) {}

    void updateBalance(const string& otherUserId, double amount) {
        balances[otherUserId] += amount;
        if (abs(balances[otherUserId]) < 0.01) balances.erase(otherUserId);
    }
};
int User::nextUserId = 0;

class Group {
public:
    static int nextGroupId;
    string groupId, name;
    vector<User*> members;
    map<string, map<string, double>> balances; // memberId -> {otherMemberId -> balance}

    Group(const string& name) : name(name), groupId("group" + to_string(++nextGroupId)) {}

    void addMember(User* user) {
        members.push_back(user);
        balances[user->userId] = {};
        cout << user->name << " added to group " << name << endl;
    }

    bool isMember(const string& userId) {
        return balances.find(userId) != balances.end();
    }

    void addExpense(const string& desc, double amount, const string& paidBy, 
                   const vector<string>& users, SplitType type, 
                   const vector<double>& values = {}) {
        if (!isMember(paidBy) || !allMembersExist(users)) {
            cout << "Invalid users in expense!" << endl;
            return;
        }

        vector<Split> splits;
        if (type == SplitType::EQUAL) {
            double splitAmount = amount / users.size();
            for (const auto& userId : users)
                splits.emplace_back(userId, splitAmount);
        } else if (type == SplitType::EXACT) {
            for (size_t i = 0; i < users.size(); ++i)
                splits.emplace_back(users[i], values[i]);
        } else if (type == SplitType::PERCENTAGE) {
            for (size_t i = 0; i < users.size(); ++i)
                splits.emplace_back(users[i], amount * values[i] / 100.0);
        }

        for (const auto& split : splits) {
            if (split.userId != paidBy) {
                balances[paidBy][split.userId] += split.amount;
                balances[split.userId][paidBy] -= split.amount;
            }
        }

        cout << "Expense added: " << desc << " (Rs " << amount << ") paid by " 
             << getUserName(paidBy) << endl;
    }

    void settlePayment(const string& fromUserId, const string& toUserId, double amount) {
        if (!isMember(fromUserId) || !isMember(toUserId)) {
            cout << "Invalid users!" << endl;
            return;
        }
        balances[fromUserId][toUserId] += amount;
        balances[toUserId][fromUserId] -= amount;
        cout << getUserName(fromUserId) << " paid " << getUserName(toUserId) 
             << " Rs " << amount << endl;
    }

    void showBalances() {
        cout << "\nGroup Balances for " << name << ":\n";
        for (const auto& [userId, userBalances] : balances) {
            cout << getUserName(userId) << ":\n";
            if (userBalances.empty()) cout << "  No balances\n";
            for (const auto& [otherId, balance] : userBalances) {
                if (balance > 0)
                    cout << "  " << getUserName(otherId) << " owes Rs " 
                         << fixed << setprecision(2) << balance << endl;
                else if (balance < 0)
                    cout << "  Owes " << getUserName(otherId) << " Rs " 
                         << fixed << setprecision(2) << abs(balance) << endl;
            }
        }
    }

private:
    string getUserName(const string& userId) {
        for (auto* user : members)
            if (user->userId == userId) return user->name;
        return "";
    }

    bool allMembersExist(const vector<string>& userIds) {
        for (const auto& userId : userIds)
            if (!isMember(userId)) return false;
        return true;
    }
};
int Group::nextGroupId = 0;

class Splitwise {
private:
    map<string, User*> users;
    map<string, Group*> groups;
    static Splitwise* instance;

    Splitwise() {}
public:
    static Splitwise* getInstance() {
        if (!instance) instance = new Splitwise();
        return instance;
    }

    User* createUser(const string& name) {
        User* user = new User(name);
        users[user->userId] = user;
        cout << "User created: " << name << " (ID: " << user->userId << ")\n";
        return user;
    }

    Group* createGroup(const string& name) {
        Group* group = new Group(name);
        groups[group->groupId] = group;
        cout << "Group created: " << name << " (ID: " << group->groupId << ")\n";
        return group;
    }

    void addUserToGroup(const string& userId, const string& groupId) {
        auto* user = users.find(userId) != users.end() ? users[userId] : nullptr;
        auto* group = groups.find(groupId) != groups.end() ? groups[groupId] : nullptr;
        if (user && group) group->addMember(user);
        else cout << "User or group not found!\n";
    }

    void addExpense(const string& groupId, const string& desc, double amount, 
                    const string& paidBy, const vector<string>& users, 
                    SplitType type, const vector<double>& values = {}) {
        auto* group = groups.find(groupId) != groups.end() ? groups[groupId] : nullptr;
        if (group) group->addExpense(desc, amount, paidBy, users, type, values);
        else cout << "Group not found!\n";
    }

    void settlePayment(const string& groupId, const string& fromUserId, 
                       const string& toUserId, double amount) {
        auto* group = groups.find(groupId) != groups.end() ? groups[groupId] : nullptr;
        if (group) group->settlePayment(fromUserId, toUserId, amount);
        else cout << "Group not found!\n";
    }

    void showGroupBalances(const string& groupId) {
        auto* group = groups.find(groupId) != groups.end() ? groups[groupId] : nullptr;
        if (group) group->showBalances();
        else cout << "Group not found!\n";
    }
};
Splitwise* Splitwise::instance = nullptr;

int main() {
    Splitwise* splitwise = Splitwise::getInstance();

    // Create users
    User* user1 = splitwise->createUser("Aditya");
    User* user2 = splitwise->createUser("Rohit");
    User* user3 = splitwise->createUser("Manish");
    User* user4 = splitwise->createUser("Saurav");

    // Create group and add members
    Group* group = splitwise->createGroup("Hostel Expenses");
    splitwise->addUserToGroup(user1->userId, group->groupId);
    splitwise->addUserToGroup(user2->userId, group->groupId);
    splitwise->addUserToGroup(user3->userId, group->groupId);
    splitwise->addUserToGroup(user4->userId, group->groupId);

    // Add expenses
    vector<string> members = {user1->userId, user2->userId, user3->userId, user4->userId};
    splitwise->addExpense(group->groupId, "Lunch", 800.0, user1->userId, members, SplitType::EQUAL);

    vector<string> dinnerMembers = {user1->userId, user3->userId, user4->userId};
    vector<double> dinnerAmounts = {200.0, 300.0, 200.0};
    splitwise->addExpense(group->groupId, "Dinner", 700.0, user3->userId, dinnerMembers, 
                          SplitType::EXACT, dinnerAmounts);

    // Show balances
    splitwise->showGroupBalances(group->groupId);

    // Settle payment
    splitwise->settlePayment(group->groupId, user2->userId, user3->userId, 200.0);

    // Show updated balances
    splitwise->showGroupBalances(group->groupId);

    return 0;
}