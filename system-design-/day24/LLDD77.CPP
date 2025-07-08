#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <stdexcept>

using namespace std;

/////////////////////////////////////////////
// Product & Factory
/////////////////////////////////////////////
class Product {
private:
    int sku;
    string name;
    double price;

public:
    Product(int id, string nm, double pr) : sku(id), name(nm), price(pr) {}

    int getSku() const { return sku; }
    string getName() const { return name; }
    double getPrice() const { return price; }
};

class ProductFactory {
public:
    static Product* createProduct(int sku) {
        string name;
        double price;

        if (sku == 101) { name = "Apple"; price = 20; }
        else if (sku == 102) { name = "Banana"; price = 10; }
        else if (sku == 103) { name = "Chocolate"; price = 50; }
        else if (sku == 201) { name = "T-Shirt"; price = 500; }
        else if (sku == 202) { name = "Jeans"; price = 1000; }
        else { name = "Item" + to_string(sku); price = 100; }

        return new Product(sku, name, price);
    }
};

/////////////////////////////////////////////
// InventoryStore & DbInventoryStore
/////////////////////////////////////////////
class InventoryStore {
public:
    virtual ~InventoryStore() {}
    virtual void addProduct(Product* prod, int qty) = 0;
    virtual void removeProduct(int sku, int qty) = 0;
    virtual int checkStock(int sku) = 0;
    virtual vector<Product*> listAvailableProducts() = 0;
};

class DbInventoryStore : public InventoryStore {
private:
    map<int, int> stock;
    map<int, Product*> products;

public:
    ~DbInventoryStore() {
        for (auto& p : products) delete p.second;
    }

    void addProduct(Product* prod, int qty) override {
        int sku = prod->getSku();
        if (products.count(sku) == 0) products[sku] = prod;
        else delete prod;
        stock[sku] += qty;
    }

    void removeProduct(int sku, int qty) override {
        if (stock.count(sku) == 0) return;
        stock[sku] = max(0, stock[sku] - qty);
    }

    int checkStock(int sku) override {
        return stock.count(sku) ? stock[sku] : 0;
    }

    vector<Product*> listAvailableProducts() override {
        vector<Product*> result;
        for (auto& [sku, qty] : stock) {
            if (qty > 0 && products.count(sku)) result.push_back(products[sku]);
        }
        return result;
    }
};

/////////////////////////////////////////////
// InventoryManager
/////////////////////////////////////////////
class InventoryManager {
    InventoryStore* store;

public:
    InventoryManager(InventoryStore* store) : store(store) {}

    void addStock(int sku, int qty) {
        store->addProduct(ProductFactory::createProduct(sku), qty);
    }

    void removeStock(int sku, int qty) {
        store->removeProduct(sku, qty);
    }

    int checkStock(int sku) {
        return store->checkStock(sku);
    }

    vector<Product*> getAvailableProducts() {
        return store->listAvailableProducts();
    }
};

/////////////////////////////////////////////
// Strategy Pattern
/////////////////////////////////////////////
class ReplenishStrategy {
public:
    virtual void replenish(InventoryManager* manager, map<int, int> itemsToReplenish) = 0;
    virtual ~ReplenishStrategy() {}
};

class ThresholdReplenishStrategy : public ReplenishStrategy {
    int threshold;
public:
    ThresholdReplenishStrategy(int t) : threshold(t) {}
    void replenish(InventoryManager* manager, map<int, int> itemsToReplenish) override {
        for (auto& [sku, qty] : itemsToReplenish) {
            if (manager->checkStock(sku) < threshold)
                manager->addStock(sku, qty);
        }
    }
};

/////////////////////////////////////////////
// DarkStore & Manager
/////////////////////////////////////////////
class DarkStore {
    string name;
    double x, y;
    InventoryManager* inventoryManager;
    ReplenishStrategy* strategy;

public:
    DarkStore(string n, double x, double y) : name(n), x(x), y(y) {
        inventoryManager = new InventoryManager(new DbInventoryStore);
        strategy = nullptr;
    }

    ~DarkStore() {
        delete inventoryManager;
        delete strategy;
    }

    void setReplenishStrategy(ReplenishStrategy* strat) { strategy = strat; }
    void runReplenishment(map<int, int> items) {
        if (strategy) strategy->replenish(inventoryManager, items);
    }
    void addStock(int sku, int qty) { inventoryManager->addStock(sku, qty); }
    vector<Product*> getProducts() { return inventoryManager->getAvailableProducts(); }
    double distanceTo(double ux, double uy) {
        return sqrt(pow(x - ux, 2) + pow(y - uy, 2));
    }
    string getName() { return name; }
};

class DarkStoreManager {
    vector<DarkStore*> stores;
    static DarkStoreManager* instance;
    DarkStoreManager() {}

public:
    static DarkStoreManager* getInstance() {
        if (!instance) instance = new DarkStoreManager();
        return instance;
    }

    void registerDarkStore(DarkStore* store) { stores.push_back(store); }
    vector<DarkStore*> getNearbyStores(double x, double y, double maxDist) {
        vector<DarkStore*> result;
        for (auto s : stores) {
            if (s->distanceTo(x, y) <= maxDist) result.push_back(s);
        }
        return result;
    }
};

DarkStoreManager* DarkStoreManager::instance = nullptr;

/////////////////////////////////////////////
// User & Cart
/////////////////////////////////////////////
class Cart {
    vector<pair<Product*, int>> items;

public:
    void addItem(int sku, int qty) {
        items.push_back({ProductFactory::createProduct(sku), qty});
    }

    vector<pair<Product*, int>> getItems() { return items; }
    double getTotal() {
        double total = 0;
        for (auto& [p, q] : items) total += p->getPrice() * q;
        return total;
    }

    ~Cart() { for (auto& [p, _] : items) delete p; }
};

class User {
public:
    string name;
    double x, y;
    Cart* cart;

    User(string n, double x, double y) : name(n), x(x), y(y) { cart = new Cart(); }
    ~User() { delete cart; }
};

/////////////////////////////////////////////
// Order System
/////////////////////////////////////////////
class Order {
public:
    static int nextId;
    int orderId;
    User* user;
    double total;

    Order(User* u) : user(u), total(u->cart->getTotal()) { orderId = nextId++; }
};

int Order::nextId = 1;

class OrderManager {
    static OrderManager* instance;
    vector<Order*> orders;
    OrderManager() {}

public:
    static OrderManager* getInstance() {
        if (!instance) instance = new OrderManager();
        return instance;
    }

    void placeOrder(User* user) {
        Order* ord = new Order(user);
        orders.push_back(ord);
        cout << "Order Placed! ID: " << ord->orderId << ", Amount: Rs. " << ord->total << endl;
    }

    ~OrderManager() {
        for (auto o : orders) delete o;
    }
};

OrderManager* OrderManager::instance = nullptr;

/////////////////////////////////////////////
// MAIN
/////////////////////////////////////////////
int main() {
    ZeptoHelper::initialize();
    User* u = new User("Aditya", 1.5, 1.5);
    u->cart->addItem(101, 2);
    u->cart->addItem(102, 3);

    cout << "\nCart Total: Rs. " << u->cart->getTotal() << endl;

    OrderManager::getInstance()->placeOrder(u);

    delete u;
    return 0;
}

class ZeptoHelper {
public:
    static void initialize() {
        auto dm = DarkStoreManager::getInstance();

        auto a = new DarkStore("DS-A", 0.0, 0.0);
        a->setReplenishStrategy(new ThresholdReplenishStrategy(5));
        a->addStock(101, 10);
        a->addStock(102, 2);

        auto b = new DarkStore("DS-B", 2.0, 2.0);
        b->setReplenishStrategy(new ThresholdReplenishStrategy(5));
        b->addStock(102, 5);
        b->addStock(103, 7);

        dm->registerDarkStore(a);
        dm->registerDarkStore(b);
    }
};
